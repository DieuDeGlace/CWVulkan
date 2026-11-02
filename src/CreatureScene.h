#pragma once
// Normal actor admission, from steam-world-pose-invocation.md and NPC speech.
#include "SceneVisibility.h"

namespace creaturescene {
using Point = std::array<int64_t, 3>;
struct Actor {
    int64_t id = 0;
    Point physical{}, drawn{};
    glm::vec3 body{0};
    float yaw = 0, drop = 0, extraHeight = 0, health = 0;
    int32_t race = 0;
};
Actor actor(const uint8_t *record);
scenevisibility::Sphere actorSphere(const Actor &);
struct View {
    uint32_t flags = 0;                    // ordinary gameplay mask; only bits0x01/0x80 select pose membership
    std::array<int64_t, 2> renderOffset{}; // current negative look-at XY, in fixed units
    std::function<bool(const scenevisibility::Sphere &)> visible;
};
// Flags and health precede visibility. Rejected entries retain their pose caches.
std::optional<scenevisibility::Sphere> admit(const Actor &, bool localPlayer, const View &);
} // namespace creaturescene
