#include "GamepadUi.h"
#include "InventoryUi.h"
#include <cstdio>
#include <limits>
#include <numeric>
#include <random>

int gamepadUiTests() {
    using namespace gamepadui;
    int failures = 0;
    auto check = [&](bool pass, const char *what) {
        if (!pass) { std::printf("FAIL gamepad UI: %s\n", what); ++failures; }
    };
    check(navigationTarget({}, 20000, 0, {{8, 1}, {50, 0}}) == 1, "alignment wins over raw proximity");
    check(navigationTarget({}, 20000, 0, {{50, 1}, {150, 0}}) == 0, "equal score preserves registration order");
    check(navigationTarget({}, 20000, 0, {{1, 10}, {2, 30}}) == 0, "fallback outside the preferred cone");
    check(navigationTarget({}, 0, 20000, {{0, 10}, {0, -10}}) == 1, "positive stick Y navigates upward");
    check(navigationTarget({}, 20000, 20000, {{-100, 100}, {5, 0}}) == 0, "exact diagonal chooses first eligible point");
    check(navigationTarget({}, 20000, 0, {{4, 2}, {5, 0}}) == 1, "squared-distance exclusion is inclusive at 24");
    check(navigationTarget({}, 20000, 0, {{-10, 0}}) == -1, "navigation never wraps behind the cursor");

    GamepadButtons pad;
    pad.connected = true;
    Input input;
    pad.a = true, pad.elapsedMs = 100;
    auto frame = input.update(pad, true, false, {}, {});
    check(!frame.pressed[0], "A down within entry gate is suppressed");
    frame = input.update(pad, true, false, {}, {});
    check(!frame.pressed[0], "exact 200 ms remains gated");
    pad.elapsedMs = 1;
    frame = input.update(pad, true, false, {}, {});
    check(!frame.pressed[0], "holding through the gate never creates a delayed down edge");
    pad.a = false;
    frame = input.update(pad, true, false, {}, {});
    check(frame.released[0], "release after gate emits mouse-up even when down was suppressed");
    pad.a = true;
    frame = input.update(pad, true, false, {}, {});
    check(frame.pressed[0] && frame.held[0], "fresh A press sets GUI held bit");
    frame = input.update(pad, true, false, {}, {});
    check(!frame.pressed[0] && frame.held[0], "hold does not repeat mouse-down");
    pad.connected = false, pad.a = false;
    frame = input.update(pad, true, false, {}, {});
    check(!frame.released[0] && frame.held[0], "failed poll preserves edge and held state");
    pad.connected = true;
    frame = input.update(pad, true, false, {}, {});
    check(frame.released[0] && !frame.held[0], "next successful poll observes the release");

    Input carried;
    pad.a = true, pad.elapsedMs = 0;
    carried.update(pad, false, false, {}, {});
    pad.elapsedMs = 201;
    frame = carried.update(pad, true, false, {}, {});
    check(!frame.pressed[0], "gameplay A hold is not a new GUI press");
    Input immediate;
    pad.a = false, pad.x = true, pad.elapsedMs = 0;
    frame = immediate.update(pad, true, false, {}, {});
    check(frame.pressed[1] && frame.held[1], "X has no entry gate");

    Input navigation;
    pad = {};
    pad.connected = true, pad.leftX = 20000;
    const std::vector<Point> points{{10, 0}, {20, 0}, {30, 0}, {40, 0}};
    frame = navigation.update(pad, true, false, {}, points);
    check(frame.cursor.x == 10 && frame.navigationSound, "first navigation scan is immediate");
    pad.elapsedMs = 249;
    frame = navigation.update(pad, true, false, {10.9f, 0}, points);
    check(frame.moveCursor && frame.cursor.x == 10 && !frame.navigationSound, "waiting truncates cursor without scanning");
    pad.elapsedMs = 1;
    frame = navigation.update(pad, true, false, {10, 0}, points);
    check(frame.cursor.x == 20 && frame.navigationSound, "first repeat waits 250 ms");
    pad.elapsedMs = 1000;
    frame = navigation.update(pad, true, false, {20, 0}, points);
    check(frame.cursor.x == 30, "large poll gap performs only one scan");
    pad.elapsedMs = 0;
    frame = navigation.update(pad, true, false, {30, 0}, points);
    check(!frame.navigationSound, "repeat overshoot is discarded");
    pad.elapsedMs = 100;
    frame = navigation.update(pad, true, false, {30, 0}, {});
    check(frame.moveCursor && frame.cursor.x == 36, "empty-list cursor movement uses raw input per poll");
    pad.elapsedMs = 0;
    frame = navigation.update(pad, true, false, {30, 0}, points);
    check(frame.cursor.x == 40 && frame.navigationSound, "empty-list polls still advance the repeat countdown");
    pad.leftX = 15698;
    frame = navigation.update(pad, true, false, {}, points);
    check(!frame.moveCursor, "cursor threshold is not the movement stick dead zone");
    pad.leftX = 15699;
    frame = navigation.update(pad, true, false, {}, points);
    check(frame.cursor.x == 10 && frame.navigationSound, "recenter restores immediate navigation");

    Input scrolling;
    pad = {};
    pad.connected = true, pad.rightY = 17378;
    frame = scrolling.update(pad, true, false, {}, {}, {true, true, true});
    check(frame.scrollRows == 0, "right-stick scrolling uses strict raw threshold");
    pad.rightY = 17379;
    frame = scrolling.update(pad, true, false, {}, {}, {true, true, true});
    check(frame.scrollPanel == 0 && frame.scrollRows == -1, "inventory wins wheel routing and up decrements row");
    pad.elapsedMs = 399;
    check(scrolling.update(pad, true, false, {}, {}, {true, false, false}).scrollRows == 0, "first wheel delay is 400 ms");
    pad.elapsedMs = 1;
    check(scrolling.update(pad, true, false, {}, {}, {true, false, false}).scrollRows == -1, "wheel fires at exact countdown boundary");
    pad.elapsedMs = 5000;
    frame = scrolling.update(pad, true, false, {}, {}, {false, true, true});
    check(frame.scrollPanel == 2 && frame.scrollRows == -1, "shop precedes crafting and overshoot produces one wheel action");
    pad.elapsedMs = 0;
    check(scrolling.update(pad, true, false, {}, {}, {false, true, true}).scrollRows == 0, "wheel repeat overshoot is discarded");
    scrolling.update(pad, true, false, {}, {}, {});
    check(scrolling.update(pad, true, false, {}, {}, {false, true, false}).scrollRows == -1,
          "entering an eligible panel during a right-Y hold scrolls immediately");

    Input tabs;
    pad = {};
    pad.connected = true, pad.leftTrigger = pad.rightTrigger = 129;
    frame = tabs.update(pad, true, false, {}, {}, {true, true, true});
    check(frame.tabs.size() == 6, "both trigger edges route independently to all eligible panels");
    if (frame.tabs.size() == 6)
        for (int i = 0; i < 6; ++i)
            check(frame.tabs[(size_t)i].panel == i % 3 && frame.tabs[(size_t)i].direction == (i < 3 ? -1 : 1),
                  "native previous-before-next tab order");
    check(tabs.update(pad, true, false, {}, {}, {true, true, true}).tabs.empty(), "trigger hold does not repeat");
    pad.leftTrigger = pad.rightTrigger = 0;
    tabs.update(pad, false, false, {}, {}); // Ordinary-UI saved edges freeze in gameplay.
    pad.leftTrigger = pad.rightTrigger = 129;
    check(tabs.update(pad, true, false, {}, {}, {true, true, true}).tabs.empty(), "gameplay release does not clear saved UI trigger edge");
    Input entering;
    entering.update(pad, true, false, {}, {}, {});
    check(entering.update(pad, true, false, {}, {}, {true, false, false}).tabs.empty(),
          "ineligible trigger press is consumed rather than delayed until panel entry");
    Input device;
    pad = {};
    pad.connected = true, pad.leftTrigger = 129;
    frame = device.update(pad, true, false, {}, {}, {true, false, false});
    check(!frame.padBeforeCallbacks && !frame.padAfterCallbacks && !frame.tabs.empty(), "left-trigger tabs do not explicitly mark pad");
    pad.leftX = 20000;
    frame = device.update(pad, true, false, {}, points);
    check(frame.padBeforeCallbacks, "UI navigation marks pad even during a repeat countdown");
    pad.leftX = 0, pad.rightTrigger = 129;
    frame = device.update(pad, true, false, {}, {});
    check(frame.padBeforeCallbacks, "always-on right trigger marks pad");
    Input buttonDevice;
    pad = {};
    pad.connected = true, pad.a = true, pad.elapsedMs = 200;
    frame = buttonDevice.update(pad, true, false, {}, {});
    check(!frame.padAfterCallbacks, "suppressed A does not mark pad");
    pad.a = false, pad.elapsedMs = 1;
    frame = buttonDevice.update(pad, true, false, {}, {});
    check(frame.released[0] && frame.padAfterCallbacks, "emitted A release marks pad after callback");
    Input map;
    pad = {};
    pad.connected = true, pad.y = true;
    map.update(pad, false, false, {}, {});
    frame = map.update(pad, true, true, {}, {});
    check(frame.pressed[2], "interactive map has its own saved Y edge");
    inventoryui::Tabs<6> inventoryTabs;
    check(inventoryui::wheelRowStep(3.5) == -1 && inventoryui::wheelRowStep(.125) == -1 && inventoryui::wheelRowStep(-9) == 1 &&
              inventoryui::wheelRowStep(0) == 0,
          "wheel messages use sign rather than magnitude or fractional accumulation");
    std::vector<int> wheelHits;
    check(inventoryui::wheelTarget([&](int which) {
              wheelHits.push_back(which);
              return true;
          }) == 0 &&
              wheelHits == std::vector<int>{0},
          "inventory wheel hit stops before overlapping shop and crafting");
    check(inventoryui::wheelTarget([](int which) { return which != 0; }) == 1, "shop wheel priority precedes crafting");
    check(inventoryui::wheelTarget([](int which) { return which == 2; }) == 2, "crafting can receive wheel with inventory shown elsewhere");
    check(inventoryui::wheelTarget([](int) { return false; }) == -1, "no grid hit leaves every row unchanged");
    inventoryTabs.scroll = 7;
    inventoryTabs.selected = 42;
    inventoryTabs.selectTab(-1);
    check(inventoryTabs.tab == 5 && inventoryTabs.scroll == 0, "previous tab wraps through the widget's tabs");
    inventoryTabs.scroll = 3;
    inventoryTabs.selectTab(6);
    check(inventoryTabs.tab == 0 && inventoryTabs.scroll == 7, "tab row offsets survive switching away and back");
    check(inventoryTabs.selected == 42, "row storage does not invent a new selected entry");
    inventoryui::Tabs<2> shopTabs;
    shopTabs.selectTab(-1);
    check(shopTabs.tab == 1, "shop wraps through two tabs");
    using inventoryui::Region;
    const auto regions = inventoryui::regionChoices({{4, 4}, {1, 0}, {-1, 0}, {1, 0}, {0, 1}}, {0, 0});
    check(regions == std::vector<Region>({{0, 0}, {-1, 0}, {0, 1}, {1, 0}, {4, 4}}),
          "region choices deduplicate, include the player, and preserve lexicographic distance ties");
    Region active{0, 0};
    inventoryui::changeRegion(active, regions, -1);
    check(active == Region{4, 4}, "region selection wraps backward");
    inventoryui::changeRegion(active, regions, 1);
    check(active == Region{0, 0}, "region selection wraps forward");
    active = {9, 9};
    inventoryui::changeRegion(active, regions, 1);
    check(active == Region{9, 9}, "an absent active region stays unchanged");
    check(inventoryui::regionChoices({}, {3, 7}) == std::vector<Region>({{3, 7}}), "empty tabs retain the player's region choice");
    std::vector<Region> shell;
    for (int x = -4; x <= 4; ++x) for (int y = -4; y <= 4; ++y)
        if (std::max(std::abs(x), std::abs(y)) == 4) shell.push_back({x,y});
    auto lexical = shell;
    lexical.push_back({0,0});
    std::sort(lexical.begin(), lexical.end());
    std::vector<Region> expected{lexical[16]};
    expected.insert(expected.end(), lexical.begin() + 1, lexical.begin() + 16);
    expected.push_back(lexical[0]);
    expected.insert(expected.end(), lexical.begin() + 17, lexical.end());
    check(inventoryui::regionChoices(shell, {0,0}) == expected,
          "33-choice shell matches the independently derived native tie permutation");
    const int32_t low = std::numeric_limits<int32_t>::min(), high = std::numeric_limits<int32_t>::max();
    check(inventoryui::regionChoices({{low,low},{0,0}}, {high,high}) ==
          std::vector<Region>{{high,high},{low,low},{0,0}}, "region distance subtracts with native signed32 wrapping");
    check(inventoryui::regionChoices({{low,low}}, {0,0}) == std::vector<Region>{{low,low},{0,0}},
          "INT_MIN remains negative after native absolute-value wrapping");

    struct Entry { float key; int id; };
    const auto byKey = [](const Entry &entry) { return entry.key; };
    std::vector<Entry> ties;
    for (int id = 0; id < 6; ++id) ties.push_back({1.f,id});
    keyorder::detail::Sorter<Entry, decltype(byKey)>(ties, byKey).heap(0,ties.size());
    std::vector<int> tieIds;
    for (const auto &entry : ties) tieIds.push_back(entry.id);
    // Independently derived from the specified right-on-equality heap descent.
    check(tieIds == std::vector<int>{3,4,1,0,2,5}, "heap fallback retains native equal-child and repair decisions");
    std::mt19937 random(0x51ced);
    for (int count : {0,1,2,31,32,33,41,42,63,256,513}) for (int pattern = 0; pattern < 4; ++pattern) {
        std::vector<Entry> entries;
        for (int id = 0; id < count; ++id) {
            const float value = pattern == 0 ? (float)id : pattern == 1 ? (float)(count-id) :
                                pattern == 2 ? (float)(random()%13) : 100000000.f + (float)(random()%64);
            entries.push_back({value,id});
        }
        if (pattern == 3) keyorder::detail::Sorter<Entry, decltype(byKey)>(entries, byKey).sort(0,entries.size(),0);
        else keyorder::sort(entries, byKey);
        check(std::is_sorted(entries.begin(), entries.end(), [](const Entry &a, const Entry &b) { return a.key < b.key; }),
              "sample, partition and heap paths order finite float keys");
        std::vector<int> ids;
        for (const auto &entry : entries) ids.push_back(entry.id);
        std::sort(ids.begin(), ids.end());
        std::vector<int> original((size_t)count);
        std::iota(original.begin(), original.end(), 0);
        check(ids == original, "key ordering preserves the complete entry multiset");
        }
    {
        Input mapEdges;
        GamepadButtons shoulders;
        shoulders.connected = true;
        shoulders.leftShoulder = true;
        auto frame = mapEdges.update(shoulders, false, false, {}, {});
        check(!frame.olderRegion && !frame.newerRegion, "gameplay does not consume map shoulder edges");
        frame = mapEdges.update(shoulders, true, true, {}, {});
        check(frame.olderRegion && !frame.newerRegion && frame.padAfterCallbacks,
              "a shoulder held on entering the map emits its retained fresh edge");
        check(!mapEdges.update(shoulders, true, true, {}, {}).olderRegion, "map shoulders have no held repeat");
        shoulders.leftShoulder = false;
        mapEdges.update(shoulders, true, false, {}, {});
        shoulders.leftShoulder = true;
        check(!mapEdges.update(shoulders, true, true, {}, {}).olderRegion,
              "a release outside the map does not reset its retained edge state");
        shoulders.connected = false;
        shoulders.leftShoulder = false;
        mapEdges.update(shoulders, true, true, {}, {});
        shoulders.connected = true;
        shoulders.leftShoulder = true;
        check(!mapEdges.update(shoulders, true, true, {}, {}).olderRegion, "a failed poll cannot reset the previous successful map edge");
        shoulders.leftShoulder = false;
        mapEdges.update(shoulders, true, true, {}, {});
        shoulders.leftShoulder = shoulders.rightShoulder = true;
        frame = mapEdges.update(shoulders, true, true, {}, {});
        check(frame.olderRegion && frame.newerRegion, "simultaneous shoulders preserve both ordered history actions");
    }
    std::puts("gamepad UI: button gates, ranking, cursor, repeat, tab rows and native region/key ordering checked");
    return failures;
}
