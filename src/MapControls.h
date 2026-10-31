#pragma once
// Native map-center arithmetic and zoom limits (steam-map-region-history sections6–8).
#include <array>
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>

namespace mapcontrol {
using Position = std::array<int64_t, 3>;
constexpr int MAX_LAYER = 8, MAX_WHEEL_LAYER = 6, INITIAL_LAYER = 3;
constexpr int64_t NO_CURSOR_X = int64_t(0x7fffffff) * 65536;
Position fromBlocks(glm::dvec3);
glm::dvec3 blocks(const Position &);
Position terrainHeight(Position, const std::function<float(int32_t, int32_t)> &height);
Position ease(const Position &drawn, const Position &target, double frameMilliseconds);
Position wheelCenter(Position target, const Position &cursor, bool zoomedIn);
Position mousePan(Position target, glm::vec2 guiDelta, int layer, float displayedYawDegrees);
Position padPan(Position target, glm::vec2 filteredStick, int layer, float displayedYawDegrees, double frameMilliseconds);
int wheelLayer(int oldLayer, int signedDelta);
int mappedZoom(int oldLayer, bool inEdge, bool outEdge);
bool movementCloses(bool fullMap, bool lastDevicePad, bool digitalMovement, bool chatOpen, glm::vec2 filteredStick);
} // namespace mapcontrol
