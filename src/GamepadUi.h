#pragma once
// Native controller GUI events and point ranking (remaining-runtime-gaps.md §4).
#include "GameInput.h"

namespace gamepadui {
struct Point { int32_t x = 0, y = 0; };
struct Cursor { float x = 0, y = 0; };
struct Eligible {
    bool inventory = false, crafting = false, shop = false;
    bool any() const { return inventory || crafting || shop; }
};
// The original point is used for both scans; list order resolves equal scores.
int navigationTarget(Point cursor, int16_t x, int16_t y, const std::vector<Point> &points);
struct Frame {
    std::array<bool, 3> pressed{}, released{}, held{};
    bool moveCursor = false, navigationSound = false;
    bool padBeforeCallbacks = false, padAfterCallbacks = false;
    Point cursor;
    int scrollPanel = -1, scrollRows = 0, regionDirection = 0;
    bool olderRegion = false, newerRegion = false; // Map history: process older before newer.
    struct Tab {
        int panel, direction;
    }; // 0 inventory, 1 crafting, 2 shop
    std::vector<Tab> tabs; // All previous-tab actions precede all next-tab actions.
};
class Input {
  public:
    Frame update(const GamepadButtons &pad, bool ui, bool mapMode, Cursor cursor,
                 const std::vector<Point> &points, Eligible eligible = {});
  private:
    std::array<bool, 3> previous{}, held{};
    int32_t uiMs = 0, navigationMs = 0;
    bool firstNavigation = true;
    bool previousRightX = false, previousRightY = false, previousLeftTrigger = false, previousRightTrigger = false;
    bool previousMapLeft = false, previousMapRight = false;
    int32_t scrollMs = 0;
};
} // namespace gamepadui
