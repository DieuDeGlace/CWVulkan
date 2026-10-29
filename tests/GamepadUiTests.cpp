#include "GamepadUi.h"
#include "InventoryUi.h"
#include <cstdio>

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
    std::puts("gamepad UI: button gates, ranking, cursor, repeat, tab rows and region rules checked");
    return failures;
}
