// SPDX-License-Identifier: MIT
// Copyright (c) 2026 DieuDeGlace
// Shared integer coordinate mapping for Alpha terrain and the Steam terrain mod.
// It does not change generation or infer native wrapping outside Alpha's finite grid.
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstddef>
#include <limits>
#include <optional>

namespace alphacoords {
constexpr int32_t WORLD_SIDE = 1 << 24;
constexpr int32_t HOME = 8396928;
constexpr int32_t ALPHA_ZONE = 256, STEAM_ZONE = 64;

struct Point {
    int32_t x, y;
};
struct Patch {
    int32_t zoneX, zoneY;     // Absolute Alpha parent zone.
    int32_t alphaX, alphaY;   // Offset inside that 256-column parent.
    int32_t sourceX, sourceY; // Offset inside the requested Steam 64-column zone.
    int32_t width, height;
};
struct Coverage {
    std::array<Patch, 4> patches{};
    std::size_t count = 0;
};

// Anchors are explicit world identity, not a camera-relative offset. Translation
// stays integral; noise must be evaluated at the resulting absolute Alpha point.
struct Mapping {
    Point steamAnchor{0, 0};
    Point alphaAnchor{HOME, HOME};

    bool valid() const { return alphaAnchor.x >= 0 && alphaAnchor.y >= 0 && alphaAnchor.x < WORLD_SIDE && alphaAnchor.y < WORLD_SIDE; }
    std::optional<Point> point(int32_t x, int32_t y) const {
        if (!valid())
            return std::nullopt;
        const int64_t ax = int64_t(x) - steamAnchor.x + alphaAnchor.x;
        const int64_t ay = int64_t(y) - steamAnchor.y + alphaAnchor.y;
        if (ax < 0 || ay < 0 || ax >= WORLD_SIDE || ay >= WORLD_SIDE)
            return std::nullopt;
        return Point{int32_t(ax), int32_t(ay)};
    }

    // A Steam zone can cross two Alpha parents per axis. A partial world-edge
    // intersection returns only valid columns; uncovered columns are not wrapped.
    // Null means invalid framing or a Steam block coordinate outside signed32.
    std::optional<Coverage> zone(int32_t zx, int32_t zy) const {
        if (!valid())
            return std::nullopt;
        const int64_t sx = int64_t(zx) * STEAM_ZONE, sy = int64_t(zy) * STEAM_ZONE;
        constexpr auto low = std::numeric_limits<int32_t>::min();
        constexpr auto high = std::numeric_limits<int32_t>::max();
        if (sx < low || sy < low || sx + STEAM_ZONE - 1 > high || sy + STEAM_ZONE - 1 > high)
            return std::nullopt;
        const int64_t ax = sx - steamAnchor.x + alphaAnchor.x;
        const int64_t ay = sy - steamAnchor.y + alphaAnchor.y;
        const int64_t x0 = std::max(int64_t(0), ax), y0 = std::max(int64_t(0), ay);
        const int64_t x1 = std::min(int64_t(WORLD_SIDE), ax + STEAM_ZONE);
        const int64_t y1 = std::min(int64_t(WORLD_SIDE), ay + STEAM_ZONE);
        Coverage result;
        for (int64_t x = x0; x < x1;) {
            const auto width = std::min(x1 - x, ALPHA_ZONE - x % ALPHA_ZONE);
            for (int64_t y = y0; y < y1;) {
                const auto height = std::min(y1 - y, ALPHA_ZONE - y % ALPHA_ZONE);
                result.patches[result.count++] = {int32_t(x / ALPHA_ZONE), int32_t(y / ALPHA_ZONE), int32_t(x % ALPHA_ZONE),
                                                  int32_t(y % ALPHA_ZONE), int32_t(x - ax),         int32_t(y - ay),
                                                  int32_t(width),          int32_t(height)};
                y += height;
            }
            x += width;
        }
        return result;
    }
};
} // namespace alphacoords
