#include "GamepadUi.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace gamepadui {
namespace {
int32_t add(int32_t a, int32_t b) { return (int32_t)((uint32_t)a + (uint32_t)b); }
int32_t countdown(int32_t remaining, int32_t elapsed) {
    return std::max(int32_t(0), (int32_t)((uint32_t)remaining - (uint32_t)elapsed));
}
}

int navigationTarget(Point cursor, int16_t x, int16_t y, const std::vector<Point> &points) {
    const bool horizontal = std::abs((int)x) > std::abs((int)y);
    const bool vertical = std::abs((int)y) > std::abs((int)x);
    for (int pass = 0; pass < 2; ++pass) {
        int selected = -1;
        int64_t best = std::numeric_limits<int64_t>::max();
        for (size_t i = 0; i < points.size(); ++i) {
            const int64_t dx = (int64_t)points[i].x - cursor.x, dy = (int64_t)points[i].y - cursor.y;
            if (dx * dx + dy * dy <= 24)
                continue;
            int64_t score = pass ? 11000 : 101000;
            if (horizontal || vertical) {
                const int64_t forward = horizontal ? dx : -dy, perpendicular = horizontal ? dy : dx;
                const int16_t axis = horizontal ? x : y;
                if (axis > 0 ? forward <= 0 : forward >= 0)
                    continue;
                if (!pass && std::abs((float)perpendicular / (float)forward) > 1.0f)
                    continue;
                score = std::abs(forward) + (pass ? 10 : 100) * std::abs(perpendicular);
            }
            if (score < best)
                best = score, selected = (int)i;
        }
        if (selected >= 0)
            return selected;
    }
    return -1;
}

Frame Input::update(const GamepadButtons &pad, bool ui, bool mapMode, Cursor at,
                    const std::vector<Point> &points, Eligible eligible) {
    Frame frame;
    frame.held = held;
    if (!pad.connected)
        return frame; // Failed polls preserve the last successful edge state and timers.
    const auto mapped = gamepadControls(pad, ui, mapMode);
    frame.padBeforeCallbacks = std::any_of(mapped.begin(), mapped.end(), [](bool down) { return down; }) ||
        ((!ui || mapMode) && (std::abs((int)pad.leftY) > 7849 || std::abs((int)pad.rightX) > 7849));
    uiMs = ui && !mapMode ? add(uiMs, pad.elapsedMs) : 0;
    const std::array<bool, 3> now{pad.a, pad.x, pad.y};
    for (size_t i = 0; i < now.size(); ++i) {
        const bool allowed = (ui || mapMode) && (i != 2 || mapMode) &&
                             (i != 0 || mapMode || uiMs > 200);
        if (allowed && now[i] != previous[i]) {
            frame.padAfterCallbacks = true;
            frame.pressed[i] = now[i];
            frame.released[i] = !now[i];
            held[i] = now[i];
        }
        if (i < 2 || mapMode)
            previous[i] = now[i];
    }
    frame.held = held;
    if (mapMode) {
        // steam-controller-handoff section2: only successful map polls update these edges.
        frame.olderRegion = pad.leftShoulder && !previousMapLeft;
        frame.newerRegion = pad.rightShoulder && !previousMapRight;
        previousMapLeft = pad.leftShoulder;
        previousMapRight = pad.rightShoulder;
        if (frame.olderRegion || frame.newerRegion)
            frame.padAfterCallbacks = true;
    }
    if (!ui || mapMode)
        return frame;
    const bool rightX = std::abs((int)pad.rightX) > 17378, rightY = std::abs((int)pad.rightY) > 17378;
    const bool leftTrigger = pad.leftTrigger > 128, rightTrigger = pad.rightTrigger > 128;
    if (eligible.any() && rightY) {
        scrollMs = countdown(scrollMs, pad.elapsedMs);
        if (!scrollMs) {
            frame.scrollPanel = eligible.inventory ? 0 : eligible.shop ? 2 : 1;
            frame.scrollRows = pad.rightY > 0 ? -1 : 1;
            scrollMs = previousRightY ? 100 : 400;
        }
    } else
        scrollMs = 0;
    if (eligible.inventory && rightX && !previousRightX)
        frame.regionDirection = pad.rightX < 0 ? -1 : 1;
    const bool panels[3] = {eligible.inventory, eligible.crafting, eligible.shop};
    for (const int direction : {-1, 1}) {
        const bool edge = direction < 0 ? leftTrigger && !previousLeftTrigger : rightTrigger && !previousRightTrigger;
        if (edge)
            for (int panel = 0; panel < 3; ++panel)
                if (panels[panel])
                    frame.tabs.push_back({panel, direction});
    }
    previousRightX = rightX, previousRightY = rightY;
    previousLeftTrigger = leftTrigger, previousRightTrigger = rightTrigger;
    navigationMs = countdown(navigationMs, pad.elapsedMs);
    const bool navigating = std::abs((int)pad.leftX) >= 15699 || std::abs((int)pad.leftY) >= 15699;
    if (!navigating) {
        firstNavigation = true;
        navigationMs = 0;
        return frame;
    }
    frame.padBeforeCallbacks = true;
    frame.moveCursor = true;
    frame.cursor = {(int32_t)at.x, (int32_t)at.y};
    if (points.empty()) {
        const float dx = (float)pad.leftX * (1.0f / 65536.0f) * 20.0f;
        const float dy = (float)pad.leftY * (1.0f / 65536.0f) * 20.0f;
        frame.cursor = {(int32_t)(at.x + dx), (int32_t)(at.y - dy)};
    } else {
        if (!navigationMs) {
            const int selected = navigationTarget(frame.cursor, pad.leftX, pad.leftY, points);
            if (selected >= 0) {
                frame.cursor = points[(size_t)selected];
                frame.navigationSound = true;
            }
            navigationMs = firstNavigation ? 250 : 100;
            firstNavigation = false;
        }
    }
    return frame;
}
} // namespace gamepadui
