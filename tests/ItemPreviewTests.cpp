#include "ItemPreview.h"
#include "Assets.h"
#include "Hud.h"
#include "CubModel.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>

int itemPreviewTests() {
    int failures = 0;
    auto check = [&](bool pass, const char *what) {
        if (!pass) {
            std::printf("FAIL item preview: %s\n", what);
            ++failures;
        }
    };
    auto weapon = nativesaves::makeItem(3, 0, 1);
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
        renderer.shutdown();
    } else
        std::puts("SKIP item preview owner tree: owner installation unavailable");
    std::printf("Item preview source/lifecycle: %s\n", failures ? "FAILED" : "PASS");
    return failures;
}
