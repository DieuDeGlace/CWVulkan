#pragma once
#include <array>
#include <cstdint>
#include <glm/glm.hpp>
// Ordinary queued WORLD descriptors, from steam-world-pass-controls.md section2.1.
struct DrawItem;
namespace nativepose {
struct Draw;
}

namespace worldmodel {
struct Order {
    int32_t x = 0, y = 0, index = 0;
    bool ground = false;
};
using Point = std::array<int64_t, 3>;
struct Frame {
    glm::vec4 material{1};
    Point eye{};
    float actorRange = 1000000;
};
std::array<int32_t, 3> lightBlock(Point physical);
glm::vec4 baseMaterial(glm::vec4 frame, int light, int race);
glm::vec4 buffFactor(uint8_t kind);
float opacity(const Frame &, const Point &drawn, float visibility, bool half, int race);
struct Controls {
    float glow = 0, shininess = 0, white = 0, depthOffset = 0;
    bool opposite = false;
};
enum class Queue { Hidden, Opaque, Faded };
// The native contract covers finite opacity; invalid engine input is rejected.
Queue queue(float opacity);
bool applyPose(DrawItem &, const nativepose::Draw &);
// Verified playable-hair catalog entries; other factories remain unclassified.
bool hidesAttachmentMarkers(int catalogId);
} // namespace worldmodel
