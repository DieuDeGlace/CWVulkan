#include "HairStyle.h"
#include "Config.h"
#include "Frontend.h"
#include "NativeProgress.h"
#include "World.h"
#include "CubModel.h"
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
    check(color(1, 1) == glm::u8vec3(0) && color(2, 1) == glm::u8vec3(180, 120, 80), "markers are hidden and skin stays unchanged");
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
