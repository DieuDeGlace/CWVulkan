// File-backed native font resolution. Owner faces are used locally, never embedded in the test.
#include "PlasmaText.h"
#include "Assets.h"
#include <chrono>
#include <fstream>
#include <iterator>
#include <iostream>

int fontCatalogTests() {
    int failures = 0;
    auto check = [&](bool pass, const char *what) {
        if (!pass) {
            std::cerr << "FAIL font catalog: " << what << '\n';
            ++failures;
        }
    };
    plasma::TextStyle style;
    check(!style.templateFont, "immediate text does not request the template fallback");
    plx::Object shape;
    shape.kind = "TextShape";
    check(plasma::textStyle(shape, 0, style) && style.templateFont, "TextShape owns its Arial retry");
    Assets assets;
    if (!assets.locate("") || !fs::exists(assets.gameDir() / "resource1.dat") || !fs::exists(assets.gameDir() / "resource2.dat")) {
        std::cout << "SKIP owner-face font catalog checks: no local installation\n";
        return failures;
    }
    const auto root = fs::temp_directory_path() /
                      ("CubeVulkan-font-catalog-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(root / "alternate");
    fs::create_directories(root / "first");
    fs::create_directories(root / "second");
    auto copy = [&](int resource, const fs::path &to) {
        fs::copy_file(assets.gameDir() / ("resource" + std::to_string(resource) + ".dat"), root / to, fs::copy_options::overwrite_existing);
    };
    copy(1, "resource1.dat");
    copy(2, "alternate/resource1.dat");
    copy(2, "first/arial.ttf");
    copy(2, "first/search.dat");
    copy(1, "second/search.dat");
    copy(1, "own.dat");
    copy(2, "first/own.dat");
    plasma::Font primary, reference;
    std::ifstream primaryFile(assets.gameDir() / "resource1.dat", std::ios::binary);
    check(primary.load(std::vector<unsigned char>(std::istreambuf_iterator<char>(primaryFile), {})), "local primary face loads");
    std::ifstream file(assets.gameDir() / "resource2.dat", std::ios::binary);
    check(reference.load(std::vector<unsigned char>(std::istreambuf_iterator<char>(file), {})), "local reference face loads");
    plasma::Raster raster;
    raster.size = glm::vec2(24);
    auto matches = [&](const plasma::Font &font, const plasma::Font &expected) {
        if (!font.loaded())
            return false;
        for (wchar_t c : {L'A', L'a', L'W', L'g'}) {
            const auto &a = font.glyph(c, raster), &b = expected.glyph(c, raster);
            if (a.advance != b.advance || a.offset != b.offset || a.size != b.size || a.alpha != b.alpha)
                return false;
        }
        return true;
    };
    auto sameFace = [&](const plasma::Font &font) { return matches(font, reference); };
    plasma::FontCatalog fonts(root, {root / "first", root / "second"});
    const auto alias = fonts.resolve("alternate/resource1.dat");
    check(alias == 0 && sameFace(fonts[alias]), "first full path selects the face behind a basename");
    check(fonts.resolve("resource1.dat") == alias && sameFace(fonts[alias]), "later basename request retains its first face");
    const auto search = fonts.resolve("absent/search.dat");
    check(search != plasma::FontCatalog::EMPTY && sameFace(fonts[search]), "directory retry uses basename and list order");
    const auto own = fonts.resolve("own.dat");
    check(own != plasma::FontCatalog::EMPTY && matches(fonts[own], primary), "requested file precedes registered directories");
    check(fonts.resolve("missing.dat") == plasma::FontCatalog::EMPTY, "immediate failed request remains null");
    const auto fallback = fonts.resolve("missing.dat", true);
    check(fallback != plasma::FontCatalog::EMPTY && sameFace(fonts[fallback]), "template failure retries Arial");
    check(fonts.resolve("arial.ttf") == fallback && fonts.resolve("missing.dat") == plasma::FontCatalog::EMPTY,
          "fallback caches Arial without aliasing the failed request");
    check(fonts.resolve("late.dat") == plasma::FontCatalog::EMPTY, "initial absent file fails");
    copy(1, "late.dat");
    check(fonts.resolve("late.dat") != plasma::FontCatalog::EMPTY, "failed requests can load a subsequently available file");
    // The loader callback sees complete shapes inline, in the same encounter-index order retained by the document.
    plx::Document document;
    struct ParsedShape {
        std::string kind, font;
        size_t properties;
    };
    std::vector<ParsedShape> encountered;
    std::string error;
    check(plx::read(assets.gameDir() / "gui.plx", document, {plx::keyOf("PlasmaXGraphics")}, &error, plx::LOAD_SCALABLE_TEXT,
                    [&](const plx::Object &object) {
                        check(document.shapes.size() == encountered.size() + 1, "shape hook runs before the next shape is constructed");
                        plasma::TextStyle text;
                        plasma::textStyle(object, plx::LOAD_SCALABLE_TEXT, text);
                        encountered.push_back({object.kind, text.font, object.properties.size()});
                    }),
          "owner document with shape callback loads");
    check(encountered.size() == document.shapes.size(), "each parsed shape reaches its inline callback once");
    for (size_t i = 0; i < encountered.size() && i < document.shapes.size(); ++i) {
        plasma::TextStyle text;
        plasma::textStyle(document.shapes[i], document.loaderFlags, text);
        check(encountered[i].kind == document.shapes[i].kind && encountered[i].properties == document.shapes[i].properties.size() &&
                  encountered[i].font == text.font,
              "callback receives complete properties in shared shape-index order");
    }
    // Only this generated direct child of the system temporary directory can be removed.
    if (!failures && fs::equivalent(root.parent_path(), fs::temp_directory_path()) && !fs::is_symlink(root) &&
        root.filename().string().rfind("CubeVulkan-font-catalog-", 0) == 0)
        fs::remove_all(root);
    else
        std::cerr << "Font catalog fixture retained: " << root << '\n';
    std::cout << "Font catalog: " << failures << " failures\n";
    return failures;
}
