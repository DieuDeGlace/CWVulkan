#pragma once
// Decisions after pointer publication, from the held Steam mouse contracts.
#include <glm/vec2.hpp>
#include <cstdint>

namespace mousemotion {
// Backend cursor positions stay distinct from relative handler arguments.
struct Sample {
    glm::dvec2 windowPosition{};
    glm::vec2 absolutePosition{};
    bool interfaceOwned = false;
    glm::vec2 relativeMotion{}; // Sampled device counts, not cursor-position differences.
};
struct State {
    bool admitted = false;
    bool interfaceAtEntry = false, interfaceOwned = false;
    bool padSelected = false, pointerTarget = false;
    bool mapOpen = false, customizationOpen = false;
    uint32_t buttons = 0;
    glm::vec2 arguments{}, pointerDelta{};
};
struct Routes {
    bool pointerMoved = false, panMap = false, turnModel = false, turnCamera = false;
    glm::vec2 cameraDelta{};
};
inline Routes route(const State &s) {
    Routes out;
    if (!s.admitted || (s.interfaceAtEntry && s.padSelected))
        return out;
    if (s.interfaceAtEntry) {
        out.pointerMoved = s.pointerDelta != glm::vec2(0);
        out.panMap = s.mapOpen && (s.buttons & 2);
        out.turnModel = s.customizationOpen && (s.buttons & 2);
    }
    out.turnCamera = !s.interfaceOwned || ((s.buttons & 1) && !s.pointerTarget);
    if (out.turnCamera)
        out.cameraDelta = s.interfaceOwned ? s.pointerDelta : s.arguments;
    return out;
}
} // namespace mousemotion
