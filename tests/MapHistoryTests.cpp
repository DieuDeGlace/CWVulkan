#include "MapHistory.h"
#include "Assets.h"
#include "Hud.h"
#include "worldgen/Regions.h"
#include "worldgen/Places.h"
#include <cstdio>

int mapHistoryTests() {
    using namespace maphistory;
    int failures = 0;
    auto check = [&](bool pass, const char *message) {
        if (!pass) {
            std::printf("FAIL map history: %s\n", message);
            ++failures;
        }
    };
    const Region a{3, -7}, b{-4, 9}, c{8, 2};
    History history({a, b, c});
    check(history.snapshot() == std::vector<Region>{a, b, c} && !history.selected(), "saved order loads without selecting a node");
    history.show(true);
    check(history.selected() == a && history.view().older && !history.view().newer, "own-hidden to shown selects the first node");
    check(history.older() == b && history.older() == c && history.older() == c,
          "older navigation stops at the final node and returns its point");
    check(!history.view().older && history.view().newer, "boundary refresh hides only the older arrow");
    check(history.newer() == b && history.newer() == a && history.newer() == a, "newer navigation has no wrap");
    history.older();
    history.show(true);
    check(history.selected() == b, "reached shown updates retain selection without reopening");
    history.show(false);
    history.show(true);
    check(history.selected() == a, "closing and reopening reinitializes selection");
    check(!history.observe(a, 0) && history.snapshot() == std::vector<Region>{a, b, c},
          "an unchanged front region does not allocate or reorder");
    check(!history.observe(c, 0x9c) && !history.observe(c, 0x9d), "excluded ability modes preserve history");
    history.older();
    check(history.observe(c, 0) && history.snapshot() == std::vector<Region>{c, a, b} && history.selected() == b,
          "a revisit prepends a new node while unmatched selection identity survives");
    const auto caption = history.view().caption;
    check(history.observe(b, 0) && !history.selected() && !history.older() && history.view().caption == caption,
          "revisiting the selected node invalidates its borrow without automatic reselection or fabricated destination");
    history.show(false);
    history.show(true);
    check(history.selected() == b, "a later own-visible transition establishes a new valid selection");

    History duplicates({a, b, a, c});
    check(!duplicates.observe(a, 0) && duplicates.snapshot() == std::vector<Region>{a, b, a, c},
          "a matching front leaves saved duplicates alone");
    check(duplicates.observe(b, 0) && duplicates.snapshot() == std::vector<Region>{b, a, a, c},
          "revisits remove only the first older matching node");
    History empty;
    empty.show(true);
    check(!empty.selected() && !empty.older() && !empty.newer() && !empty.view().caption,
          "empty history provides no region-zero target or invented caption");

    check(hovered(true, 299.99f, 910.01f, 1000, 1) && !hovered(true, 300, 950, 1000, 1) && !hovered(true, 100, 910, 1000, 1),
          "label pointer rectangle uses strict native boundaries");
    check(hovered(true, -100, 2000, 1000, 1) && hovered(true, 449, 866, 1000, 1.5f) && !hovered(false, 0, 1000, 1000, 1),
          "label hover has no extra lower-X, upper-Y or unscaled-height gates");
    check(worldgen::regionAtFixed(-65535, 65535, false) == worldgen::regionAt(0, 0),
          "fractional physical fixed coordinates truncate toward zero before querying");
    const auto point = worldgen::cellPoint(1, 0, worldgen::bigCell(1, 0));
    check(worldgen::regionAtFixed(int64_t(1024) * 65536, 0, true) == worldgen::regionAt(point.x, point.y),
          "flagged blocks use their original landmark cells before the helper's internal even rounding");
    Assets assets;
    if (assets.locate("")) {
        Hud hud;
        std::string error;
        check(hud.load(assets.gameDir(), error), "owner map UI loads");
        Renderer renderer;
        renderer.initHeadless(RenderSettings{});
        for (float scale : {1.f, 1.5f}) {
            hud.setGuiScale(scale);
            hud.layout(renderer, {1920, 1080});
            History owner({a, b, c});
            owner.show(true);
            hud.bindMapHistory(owner.view(), "Test Land", true);
            const auto *root = hud.mapHistoryRoot();
            const auto *left = root ? const_cast<plasma::SceneNode *>(root)->find("leftbutton") : nullptr;
            const auto *right = root ? const_cast<plasma::SceneNode *>(root)->find("rightbutton") : nullptr;
            check(root && left && right && root->transform.translation == glm::vec2(150, 1080.f / scale - 50) &&
                      left->transform.translation == glm::vec2(-100, 2) - left->transform.pivot &&
                      right->transform.translation == glm::vec2(100, 2) - right->transform.pivot,
                  "owner arrows retain pivot transforms under the physical-height label anchor");
            check(left && right && left->visible && !right->visible && left->doc && right->doc && left->widget && right->widget,
                  "initial native selection exposes only the older owner widget");
            owner.older();
            hud.bindMapHistory(owner.view(), "Other Land", true);
            check(left && right && left->visible && right->visible, "callback refresh exposes both arrows at an interior entry");
            glm::vec2 olderPoint{};
            bool found = false;
            for (int dy = -40; dy <= 40 && !found; dy += 2)
                for (int dx = -40; dx <= 40 && !found; dx += 2) {
                    const glm::vec2 point((50 + dx) * scale, 1080 + (dy - 48) * scale);
                    if (hud.mapHistoryAction(point) == -1) {
                        olderPoint = point;
                        found = true;
                    }
                }
            check(found && hud.mapHistoryArrowHit(true, olderPoint), "owner arrow geometry produces the native mouse-down action");
            hud.bindMapHistory(owner.view(), "Other Land", false);
            check(found && hud.mapHistoryArrowHit(true, olderPoint) && !hud.mapHistoryAction(olderPoint) && owner.selected() == b,
                  "hidden ancestors suppress dispatch while standalone arrow hits and selection remain live");
        }
        renderer.shutdown();
    }
    std::printf("Map history: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
