#include "ItemPreview.h"
#include "ItemInformation.h"
#include "Assets.h"
#include "Hud.h"
#include "CubModel.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdio>
#include <limits>
#include <cstring>

int itemPreviewTests() {
    int failures = 0;
    auto check = [&](bool pass, const char *what) {
        if (!pass) {
            std::printf("FAIL item preview: %s\n", what);
            ++failures;
        }
    };
    check(std::bit_cast<uint32_t>(itempreview::roundTenth(1.25f)) == 0x3fa66667 &&
              std::bit_cast<uint32_t>(itempreview::roundTenth(-1.25f)) == 0xbfa66667,
          "half-tenth ties use float multiply/conversion/multiply stages");
    check(std::bit_cast<uint32_t>(itempreview::roundTenth(-.01f)) == 0, "rounded zero has a positive sign");
    check(itempreview::formatNumber(itempreview::roundTenth(1.25f)) == "1.3" && itempreview::formatNumber(1) == "1",
          "fresh stream formatting does not force a decimal suffix");
    auto weapon = nativesaves::makeItem(3, 0, 1);
    inventoryslots::Inventory ringInventory;
    ringInventory.tabs.resize(1);
    ringInventory.tabs[0] = {{-1, nativesaves::makeItem(1, 2, 0)}, {3, nativesaves::makeItem(1, 1, 0)}};
    int32_t runningIndex = 0;
    const auto beforeRepeat = itempreview::sampleRing(ringInventory, true, runningIndex);
    runningIndex = 1;
    check(beforeRepeat.selected == inventoryslots::Index{0, 0} &&
              itempreview::sampleRing(ringInventory, true, runningIndex).selected == inventoryslots::Index{0, 1},
          "Ring source retains its earlier canonical key across a later index change, including negative quantities");
    check(!itempreview::sampleRing(ringInventory, false, runningIndex).selected && runningIndex == 1,
          "hidden Ring sampling does not change the running index or create a candidate");
    auto alternateWeapon = weapon;
    auto selectedWeapon = weapon;
    itempreview::Equipment comparisons;
    comparisons.weapons = {&weapon, &alternateWeapon};
    check(itempreview::comparison(&selectedWeapon, selectedWeapon, comparisons, false) == &weapon &&
              itempreview::comparison(&selectedWeapon, selectedWeapon, comparisons, true) == &alternateWeapon,
          "comparison excludes by address rather than value, and Shift selects the second weapon");
    check(itempreview::comparison(&weapon, weapon, comparisons, true) == nullptr,
          "either equipped weapon source excludes the paired weapon comparison");
    const auto shield = nativesaves::makeItem(3, 13, 1);
    check(itempreview::comparison(&shield, shield, comparisons, false) == &alternateWeapon,
          "subtype13 skips a first non-two-handed weapon without filtering the second case");
    auto twoHanded = nativesaves::makeItem(3, 5, 1);
    comparisons.weapons[0] = &twoHanded;
    check(itempreview::comparison(&shield, shield, comparisons, false) == &twoHanded, "subtype13 admits a first two-handed comparison");

    std::array<uint8_t, 0x3000> player{};
    iteminformation::Context textContext;
    textContext.player = player.data();
    textContext.classType = 1;
    textContext.classWarning = false;
    textContext.fullName = [](const auto &) { return "Synthetic"; };
    textContext.regionName = [](const auto &) { return "Named region"; };
    textContext.realmName = [](uint32_t seed) { return "Realm " + std::to_string(seed); };
    const auto originalWeapon = weapon;
    auto detail = iteminformation::format(weapon, textContext);
    const auto findLine = [](const auto &lines, const char *prefix) {
        return std::find_if(lines.begin(), lines.end(), [prefix](const auto &line) { return line.text.starts_with(prefix); });
    };
    auto damage = findLine(detail, "DAMAGE ");
    check(damage != detail.end() && damage->size == 15 && damage->advance == 20 && damage->stroke == 3,
          "damage uses its own native font and20-unit advance");
    check(findLine(detail, "Named region") != detail.end() && weapon == originalWeapon,
          "zero-valued region still receives its dictionary line without mutating the source");
    alternateWeapon[0x14] = 4;
    textContext.comparison = &alternateWeapon;
    detail = iteminformation::format(weapon, textContext);
    auto difference = std::find_if(detail.begin(), detail.end(), [](const auto &line) { return line.suffix; });
    check(difference != detail.end() && difference->over && difference->advance == 0 && difference->size == 12 &&
              difference->color == glm::vec4(1, .3f, .3f, 1) && difference->text.starts_with(" -"),
          "comparison loss is an inline red suffix with no extra line advance");
    textContext.comparison = nullptr;
    textContext.scale = .75f;
    detail = iteminformation::format(weapon, textContext);
    damage = findLine(detail, "DAMAGE ");
    check(damage != detail.end() && damage->size == 11.25f && damage->advance == 15 && damage->stroke == 3,
          "comparison factor scales fonts/advances while outline stays3");
    textContext.scale = 1;
    auto artifact = nativesaves::makeItem(23, 0, 0);
    const uint32_t effectIndex = 6, realmSeed = 42;
    std::memcpy(artifact.data() + 8, &effectIndex, 4);
    std::memcpy(artifact.data() + 0x18, &realmSeed, 4);
    detail = iteminformation::format(artifact, textContext);
    const auto artifactEffect = findLine(detail, "Increases the radius of your lamp.");
    check(findLine(detail, "Realm 42") != detail.end() && artifactEffect != detail.end() && artifactEffect->advance == 0,
          "artifact has distinct realm and nonadvancing effect text");
    auto rank = nativesaves::makeItem(24, 4, 0);
    const uint32_t maximumRank = 0x7fffffff;
    std::memcpy(rank.data() + 8, &maximumRank, 4);
    detail = iteminformation::format(rank, textContext);
    auto rankLine = findLine(detail, "RANK -2147483648");
    check(rankLine != detail.end() && rankLine->advance == 0, "rank increment wraps signed32 without advancing textY");
    check(iteminformation::ridable(0x13) && !iteminformation::ridable(0x15) && iteminformation::ridable(0x127) &&
              !iteminformation::ridable(0x128),
          "ridable race ranges retain gaps and endpoints");
    detail = iteminformation::format(nativesaves::makeItem(20, 0x13, 0), textContext);
    auto ridable = findLine(detail, "Ridable");
    check(ridable != detail.end() && ridable->advance == 14, "pet row preserves native spelling and advance");
    detail = iteminformation::format(nativesaves::makeItem(28, 99, 0), textContext);
    check(detail.size() >= 3 && detail[detail.size() - 2].text.empty() && detail[detail.size() - 2].fromEnd &&
              detail.back().text == "Effect lasts 10 Minutes",
          "unknown effect subtype still submits empty description and duration");
    auto hoveredCube = nativesaves::makeItem(11, 14, 128);
    itempreview::Sources sources;
    sources.voxel = &hoveredCube;
    auto preview = itempreview::select(sources);
    check(preview.item == &hoveredCube && preview.anchor == itempreview::Anchor::Crafting,
          "palette preview borrows the original and anchors to crafting");
    sources.formulaShown = true;
    check(itempreview::select(sources).item == nullptr, "shown formula details block the palette even with no hovered ingredient");
    sources.formula = &weapon;
    check(itempreview::select(sources).item == &weapon, "formula details retain their earlier choice");
    sources.inventory = &hoveredCube;
    check(itempreview::select(sources).item == &hoveredCube && itempreview::select(sources).anchor == itempreview::Anchor::Inventory,
          "canonical hover precedes formula and voxel state");
    sources.map = &weapon;
    check(itempreview::select(sources).item == &weapon && itempreview::select(sources).anchor == itempreview::Anchor::Cursor,
          "embedded map item has no added shown test and precedes canonical hover");
    sources.ringShown = true;
    check(itempreview::select(sources).item == nullptr, "Ring bypasses the whole ordinary hover chain");
    sources.ground = &hoveredCube;
    check(itempreview::select(sources).groundOrigin, "a missing Ring source retains the separate ground fallback");
    sources.ring = &weapon;
    check(itempreview::select(sources).item == &weapon && itempreview::select(sources).groundOrigin,
          "Ring selection retains independent ground-origin state");
    sources.ringShown = false;
    check(!itempreview::select(sources).groundOrigin, "an ordinary UI source clears ground-origin state");

    Assets assets;
    if (assets.locate("")) {
        Hud hud;
        std::string error;
        check(hud.load(assets.gameDir(), error), "owner UI load");
        Renderer renderer;
        renderer.initHeadless(RenderSettings{});
        CubModel model;
        model.sx = model.sy = model.sz = 1;
        model.rgb.assign(3, 255);
        hud.addItemIcon("synthetic-information", model);
        hud.setGuiScale(1);
        hud.layout(renderer, {1920, 1080});
        Hud::State ui;
        std::vector<OverlayVertex> overlay;
        hud.draw(ui, overlay);
        const auto precedingVertices = overlay.size();
        Hud::Inventory information;
        information.infoItem = weapon;
        information.infoIcon = "synthetic-information";
        information.infoAnchor = itempreview::Anchor::Cursor;
        information.pointer = {500, 200};
        information.info.push_back({"Synthetic item", 12, 3, glm::vec4(1), 16});
        ui.information = &information;
        auto backgroundHeight = [&] {
            hud.draw(ui, overlay);
            const auto &requests = hud.itemDraws();
            if (requests.size() != 1 || requests[0].beforeVertex <= precedingVertices || requests[0].beforeVertex > overlay.size()) {
                check(false, "one native icon follows its owner background");
                return 0.0f;
            }
            float low = std::numeric_limits<float>::infinity(), high = -low;
            for (size_t i = precedingVertices; i < requests[0].beforeVertex; ++i) {
                low = std::min(low, overlay[i].y);
                high = std::max(high, overlay[i].y);
            }
            return high - low;
        };
        const auto first = backgroundHeight(), second = backgroundHeight();
        check(std::abs(first - second - 100) < .01f, "initial 300px background precedes the item's late resize to 200px");
        ui.information = nullptr;
        hud.draw(ui, overlay);
        ui.information = &information;
        information.info[0].advance = 420;
        const auto afterHidden = backgroundHeight(), afterGrowth = backgroundHeight();
        check(std::abs(afterHidden - second) < .01f, "a skipped draw retains its earlier size");
        check(std::abs(afterGrowth - afterHidden - 245) < .01f, "a later text-derived size changes the following background only");
        information.info[0].advance = 16;
        const auto beforeShrink = backgroundHeight(), afterShrink = backgroundHeight();
        check(std::abs(beforeShrink - afterGrowth) < .01f && std::abs(afterShrink - second) < .01f,
              "shorter text uses the old background before retaining its smaller size");
        const glm::vec4 comparisonColor(.2f, .8f, .4f, 1);
        for (float scale : {1.f, .4096f}) {
            hud.setGuiScale(scale);
            hud.layout(renderer, {1920, 1080});
            for (const glm::vec2 font :
                 {glm::vec2(11.25f, 4), glm::vec2(11.25f, 3), glm::vec2(9, 3), glm::vec2(13.5f, 3), glm::vec2(9, 2)}) {
                information.comparisonInfo = {{"M", font.x, font.y, comparisonColor, 16}};
                hud.draw(ui, overlay);
                check(hud.itemDraws().size() == 1, "equipped comparison does not submit another item image");
                const auto colored = std::count_if(overlay.begin(), overlay.end(),
                                                   [](const auto &vertex) { return vertex.r == 51 && vertex.g == 204 && vertex.b == 102; });
                if (!colored) {
                    std::printf("Comparison glyph missing: GUI %.4f font %.2f stroke %.0f\n", scale, font.x, font.y);
                    check(false, "fractional comparison font has actual owner glyph geometry");
                }
            }
        }
        hud.setGuiScale(1);
        hud.layout(renderer, {1920, 1080});
        information.comparisonInfo.clear();
        auto textGeometry = [&] {
            hud.draw(ui, overlay);
            std::vector<std::pair<float, float>> positions;
            if (hud.itemDraws().size() == 1)
                for (size_t i = hud.itemDraws()[0].beforeVertex; i < overlay.size(); ++i)
                    positions.emplace_back(overlay[i].x, overlay[i].y);
            std::sort(positions.begin(), positions.end());
            return positions;
        };
        information.info = {{"A +1", 12, 3, glm::vec4(1), 16}};
        const auto combined = textGeometry();
        information.info = {{"A", 12, 3, glm::vec4(1), 16}, {" +1", 12, 3, glm::vec4(1), 0, false, true, true}};
        const auto suffix = textGeometry();
        check(!combined.empty() && combined == suffix, "inline suffix follows the real returned glyph pen at the original baseline");
        information.infoAnchor = itempreview::Anchor::Ground;
        hud.draw(ui, overlay);
        check(hud.itemDraws().size() == 1 && glm::length(hud.itemDraws()[0].at.screen - glm::vec2(1395, 947)) < .001f,
              "ground placement uses viewport half-width, previous parent height and native offset");
        information.info[0].advance = 420;
        hud.draw(ui, overlay);
        const auto beforeGroundGrowth = hud.itemDraws()[0].at.screen;
        hud.draw(ui, overlay);
        check(glm::length(beforeGroundGrowth - glm::vec2(1395, 947)) < .001f &&
                  glm::length(hud.itemDraws()[0].at.screen - glm::vec2(1395, 702)) < .001f,
              "ground placement observes the previous admitted size without a new viewport clamp");
        renderer.shutdown();
    } else
        std::puts("SKIP item preview owner tree: owner installation unavailable");
    std::printf("Item preview source/lifecycle: %s\n", failures ? "FAILED" : "PASS");
    return failures;
}
