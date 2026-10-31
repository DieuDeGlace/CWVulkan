#pragma once
// Controls/Multiplayer wheel state (steam-physical-wheel-panel-fallbacks).
#include <cstdint>
#include <optional>

namespace panelscroll {
struct Change {
    int32_t first = 0;
    bool refreshThumb = false;
};
struct Thumb {
    float x = 0, y = 0, width = 0, height = 0;
};
struct Drag {
    int32_t first = 0;
    float y = 0;
};
// Geometry must come from the owner's frame/Q queries; this does not infer hit/capture state.
std::optional<Drag> controlsDrag(int32_t first, float localHeight, float thumbHeight, float thumbY, float pointerDeltaY);
Change controls(int32_t first, int wheelDelta, float localHeight);
Change friends(int32_t first, int wheelDelta, float localHeight, uint64_t storedRows);
// Native undefined division cases have no fabricated/clamped geometry.
std::optional<Thumb> controlsThumb(int32_t first, float width, float height, float thumbWidth);
std::optional<Thumb> friendsThumb(int32_t first, float width, float height, float thumbWidth, uint64_t storedRows);
} // namespace panelscroll
