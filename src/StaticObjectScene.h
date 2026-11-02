#pragma once
// Ordinary static-object admission, docs/specs/steam-static-object-scene-admission.md.
#include "SceneVisibility.h"

namespace staticobjectscene {
using Point = std::array<int64_t, 3>;
scenevisibility::Sphere sphere(int32_t type, float scale, Point position);
std::array<int32_t, 2> playerZone(const Point &physical);
bool inPlayerSquare(const Point &physical, int32_t zx, int32_t zy);
} // namespace staticobjectscene
