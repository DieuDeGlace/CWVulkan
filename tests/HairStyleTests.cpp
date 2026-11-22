#include "HairStyle.h"
#include "Config.h"
#include "Frontend.h"
#include "NativeProgress.h"
#include "World.h"
#include "CubModel.h"
#include "Assets.h"
#include "MapLook.h"
#include <cmath>
#include <cstdio>
#include <limits>

int hairStyleTests() {
    int failures = 0;
    auto check = [&](bool pass, const char *what) {
        if (!pass) {
            std::printf("FAIL hair style: %s\n", what);
            ++failures;
        }
    };
    hair::Style style{true, {0, 0, 255}, 0, 1};
    CubModel cub;
    cub.sx = 3;
    cub.sy = 1;
    cub.sz = 3;
    for (int z = 0; z < 3; ++z)
        cub.rgb.insert(cub.rgb.end(), {200, 200, 200, 0, 255, 0, 180, 120, 80});
    World::tintHair(cub, {255, 0, 0}, &style);
    auto color = [&](int x, int z) {
        const size_t at = 3 * (z * 3 + x);
        return glm::u8vec3(cub.rgb[at], cub.rgb[at + 1], cub.rgb[at + 2]);
    };
    check(color(0, 2) == glm::u8vec3(200, 1, 1) && color(0, 1) == glm::u8vec3(100, 1, 100) && color(0, 0) == glm::u8vec3(1, 1, 200),
          "gradient preserves primary roots and secondary tips");
    check(color(1, 1) == glm::u8vec3(0, 255, 0) && color(2, 1) == glm::u8vec3(180, 120, 80), "gradient keeps attachment occupancy and authored skin");
    CubModel neutral;
    neutral.sx = 5;
    neutral.sy = neutral.sz = 1;
    neutral.rgb = {255,255,255, 255,254,253, 0,17,255, 55,251,0, 0,0,255};
    const auto authored = neutral.rgb;
    World::tintHair(neutral, glm::u8vec3(255), nullptr);
    check(neutral.rgb == authored, "neutral primary preserves every drawn paint byte");
    Assets assets;
    if (assets.locate("")) {
        unsigned compared = 0;
        size_t portraitFaces = 0;
        for (int sex = 0; sex < 2; ++sex) {
            for (int pick = 1; pick <= (sex ? 4 : 5); ++pick) {
                const auto name = std::string("frogman-hair-") + (sex ? "f0" : "m0") + std::to_string(pick) + ".cub";
                CubModel model;
                if (!model.load(assets.gameDir() / "sprites" / name)) {
                    check(false, "could not load a numbered owner Frogman haircut");
                    continue;
                }
                const auto expected = model.rgb;
                int red = 0, blue = 0, green = 0;
                for (size_t i = 0; i < expected.size(); i += 3) {
                    const auto *c = expected.data() + i;
                    if (c[0] == 255 && c[1] == 0 && c[2] == 0) ++red;
                    else if (c[0] == 0 && c[1] == 255 && c[2] == 0) ++green;
                    else if (c[0] == 0 && c[1] == 0 && c[2] == 255) ++blue;
                }
                World::tintHair(model, glm::u8vec3(255), nullptr);
                check(red == (!sex && pick == 5 ? 190 : 188) && blue == 70 && green == 0,
                      "numbered Frogman haircut matches the original marker counts");
                check(model.rgb == expected, "every non-marker cell retains its authored paint");
                Mesh portrait;
                worldmap::meshModel({model.sx, model.sy, model.sz, model.rgb.data(), true},
                                    0, model.sx - 1, 0, model.sy - 1, true, portrait);
                size_t faces = 0;
                for (int z = 0; z < model.sz; ++z)
                    for (int y = 0; y < model.sy; ++y)
                        for (int x = 0; x < model.sx; ++x) {
                            if (!model.solid(x, y, z) || model.attachmentMarker(x, y, z)) continue;
                            for (const auto &axis : FACE_NORMALS)
                                faces += !model.solid(x + axis[0], y + axis[1], z + axis[2]);
                        }
                check(portrait.vertices.size() == faces * 4 && portrait.indices.size() == faces * 6,
                      "map haircut emits exactly the exposed paint faces");
                if (pick == 1) {
                    const auto paint = model.color(3, 7, 2);
                    check(model.attachmentMarker(2, 7, 2) && paint.r == 55 && paint.g == 251 && paint.b == 0,
                          "first haircut has the documented marker touching authored green");
                    for (size_t i = 0; i < portrait.vertices.size(); i += 4) {
                        const auto &vertex = portrait.vertices[i];
                        check(!(vertex.normal == worldmap::FACE_NORMAL_INDEX[worldmap::WEST] &&
                                vertex.x == 3 && vertex.y == 7 && vertex.z == 2),
                              "marker occludes the west side of the documented painted cell");
                    }
                }
                portraitFaces += portrait.indices.size() / 6;
                ++compared;
            }
        }
        check(portraitFaces == 982, "all nine portrait meshes reproduce the independent exposed-paint census");
        std::printf("Frogman exposed map faces: %zu\n", portraitFaces);
        std::printf("Frogman owner haircuts compared: %u\n", compared);
    }
    const auto invalid = hair::normalised({true, {1, 2, 3}, std::numeric_limits<float>::quiet_NaN(), -1});
    check(invalid.start == .4f && invalid.blend == 0, "invalid gradient parameters have finite bounds");
    check(hair::cacheKey(style) != hair::cacheKey({true, {0, 255, 0}, 0, 1}), "secondary colors have distinct mesh keys");
    check(hair::cacheKey({false, {1, 2, 3}, 0, 0}) == hair::cacheKey({}), "disabled secondary settings do not multiply caches");
    Config config;
    config.set("Characters", "_NewCharacter", "127,255,255,20,45");
    const auto imported = hair::fromConfig(config, "_NewCharacter");
    check(imported.dual && imported.second == glm::u8vec3(127, 255, 255) && imported.start == .2f && imported.blend == .45f,
          "existing color and gradient settings import without the DLL");
    config.set("DualHair", "Enabled", "0");
    config.set("DualHair", "StartPercent", "nan");
    const auto disabled = hair::fromConfig(config, "_NewCharacter");
    check(!disabled.dual && std::isfinite(disabled.start), "disabled imports still normalize gradient parameters");
    frontend::HairEditor editor;
    editor.reset(style);
    editor.hover(glm::u8vec3(0, 255, 0));
    check(editor.preview.second == glm::u8vec3(0, 255, 0) && editor.committed.second == style.second, "hover is temporary");
    editor.hover(std::nullopt);
    check(editor.preview == style, "leaving the palette restores the committed style");
    editor.arrow(2, 1);
    check(editor.committed.blend == 1, "blend upper bound");
    editor.arrow(1, -1);
    check(editor.committed.start == 0, "start lower bound");
    editor.arrow(0, 1);
    editor.choose({5, 6, 7});
    check(editor.committed.dual && editor.committed.second == glm::u8vec3(5, 6, 7), "choosing a color enables dual tone");
    int column = -1, row = -1;
    check(frontend::secondHairCellAt(frontend::SECOND_HAIR_GRID, column, row) && column == 0 && row == 0,
          "secondary palette uses its own origin");
    check(!frontend::secondHairCellAt(frontend::SECOND_HAIR_GRID + glm::vec2(16 * 17, 0), column, row), "palette right edge is excluded");
    editor.reset({});
    check(editor.press({115, 50}, {20, 20}) && editor.committed.dual, "pressing the enable arrow toggles the setting");
    check(editor.press({265, 80}, {20, 20}) && editor.committed.start == .45f, "pressing the start arrow changes the gradient");
    check(editor.press(frontend::SECOND_HAIR_GRID + glm::vec2(8, 8), {20, 20}) &&
              editor.committed.second == glm::u8vec3(frontend::hairCellColour(0, 0) * 255.0f),
          "pressing the secondary palette commits its color");
    const auto beforeMiss = editor.committed;
    check(!editor.press({-1, -1}, {20, 20}) && editor.committed == beforeMiss, "an outside press leaves the style unchanged");
    const auto wide = frontend::hairPanels({1300, 800}, 640, 40, true);
    check(wide.scale == 1 && wide.primary == glm::vec2(20, 450) && wide.secondary == glm::vec2(330, 450),
          "both full-size panels fit at exact footer clearance");
    const auto overlap = frontend::hairPanels({1300, 800}, 639, 40, true);
    check(overlap.primary.y == wide.primary.y - 60, "footer overlap reserves 80 units instead of 20");
    const auto compact = frontend::hairPanels({800, 600}, 250, 40, true);
    check(std::abs(compact.scale - 360.f / 610) < 1e-6f &&
          std::abs(compact.secondary.x - (20 + 310 * compact.scale)) < 1e-4f &&
          std::abs(compact.primary.y + 330 * compact.scale - 520) < 1e-4f,
          "narrow-screen panels leave room for the character name and submit controls");
    const auto shortWindow = frontend::hairPanels({1600, 250}, 700, 40, true);
    check(std::abs(shortWindow.primary.y - 70) < 1e-4f && shortWindow.scale < wide.scale,
          "short window respects the back button");
    const auto themed = frontend::hairPanels({1300,800}, 640, 40, true, {280,300});
    check(themed.primary == glm::vec2(20, 480) && themed.secondary.x == 310,
          "panel placement follows the measured template content");
    const auto tiny = frontend::hairPanels({30, 15}, 0, 40, true);
    check(tiny.scale == .01f, "tiny viewport retains an invertible pointer transform");
    const auto legacy = frontend::hairPanels({800,600}, 250, 40, false);
    check(legacy.scale == 1 && legacy.primary == glm::vec2(0,270) && legacy.secondary == glm::vec2(490,20),
          "existing panel layout");
    const glm::vec2 middle = frontend::tipsGrid(true) + glm::vec2(3.5f, 4.5f) * frontend::HAIR_CELL;
    const auto pointer = compact.secondary + middle * compact.scale;
    editor.reset({});
    check(editor.press((pointer - compact.secondary) / compact.scale, {20,20}, true) && editor.committed.dual &&
          editor.committed.second == glm::u8vec3(frontend::hairCellColour(3,4) * 255.f),
          "scaled painted tips cell commits the indicated colour directly from off");
    check(frontend::secondHairCellAt(frontend::HAIR_GRID, column, row, true) && column == 0 && row == 0 &&
          !frontend::secondHairCellAt(frontend::HAIR_GRID + glm::vec2(16*17,0), column, row, true),
          "paired palette uses the primary grid's inclusive and exclusive edges");
    check(!editor.press({20, 178}, {20,20}, true), "the former tips origin cannot select a paired swatch");
    SaveGame saved, parsed;
    saved.hairStyle = imported;
    check(parsed.parse(saved.text()) && parsed.hairStyle == saved.hairStyle, "appearance metadata round trip");
    check(!parsed.parse("cubevulkan-save 1\nhair_style 1 0 0 255 101 0\n"), "out-of-range metadata is rejected");
    nativesaves::Character character;
    character.looks.set(1, 0, 2, 3, {50, 60, 70});
    const auto primary = character.looks.bytes;
    std::string notes;
    nativeprogress::fromProgress(saved, nullptr, 100, {}, character, notes);
    const auto restored = nativeprogress::toProgress(character, notes, {}, {});
    check(restored.hairStyle == saved.hairStyle && character.looks.bytes == primary,
          "native progress retains extra appearance without overwriting primary appearance bytes");
    SaveGame plain;
    check(parsed.parse(plain.text()) && !parsed.hairStyle, "a profile without extra appearance does not inherit the previous one");
    std::printf("Dual-tone hair: %s\n", failures ? "FAILED" : "PASS");
    return failures;
}
