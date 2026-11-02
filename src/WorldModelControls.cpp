#include "WorldModelControls.h"
#include "NativePose.h"
#include "Renderer.h"
#include "GameMath.h"
#include "SteamScenery.h"
#include <cmath>
#include <cstring>
#include <array>

namespace worldmodel {
std::array<int32_t, 3> lightBlock(Point physical) {
    physical[2] = gamemath::addFixed(physical[2], -6553);
    return {gamemath::floorFixedBlock(physical[0]), gamemath::floorFixedBlock(physical[1]), gamemath::floorFixedBlock(physical[2])};
}

glm::vec4 baseMaterial(glm::vec4 frame, int light, int race) {
    const float factor = race == 0x114 ? 1.f : float(light) / 255.f;
    frame.a = factor * frame.a;
    return frame;
}

glm::vec4 buffFactor(uint8_t kind) {
    switch (kind) {
    case 1:
    case 2:
    case 0x12:
        return {1, .5f, .5f, 1};
    case 4:
        return {.5f, 1.5f, .5f, 1};
    case 0x0f:
        return {.2f, .1f, .3f, 1};
    case 0x1d:
        return {.5f, .5f, 1.5f, 1};
    default:
        return glm::vec4(1);
    }
}

float opacity(const Frame &frame, const Point &drawn, float visibility, bool half, int race) {
    float factor = 1.f - visibility * .8f;
    if (half)
        factor *= .5f;
    if (race == 0x11d)
        factor *= .75f;
    return steamscenery::fade(steamscenery::distanceSquaredFixed(drawn, frame.eye), frame.actorRange) * factor;
}

bool hidesAttachmentMarkers(int catalogId) {
    constexpr std::array<int, 2> ranges[] = {
        {0x9c, 0xa1},   // goblin male
        {0xa7, 0xac},   // goblin female
        {0x5b3, 0x5c1}, // human male
        {0x5c8, 0x5d0}, // human female
        {0x5d1, 0x5da}, // elf male
        {0x5db, 0x5e5}, // elf female
        {0x5f3, 0x5fa}, // orc male
        {0x5fd, 0x600}, // orc female
        {0x606, 0x60a}, // frogman male
        {0x60f, 0x612}, // frogman female
        {0xb0, 0xb5},   // lizard male
        {0x194, 0x196}, // dwarf male
        {0x19c, 0x1a1}, // dwarf female
        {0x1ab, 0x1b0}, // undead male
        {0x1b7, 0x1bc}, // undead female
    };
    for (const auto &range : ranges)
        if (catalogId >= range[0] && catalogId <= range[1])
            return true;
    return false;
}

Queue queue(float opacity) {
    if (!std::isfinite(opacity) || opacity <= 0)
        return Queue::Hidden;
    return opacity >= 1 ? Queue::Opaque : Queue::Faded;
}

bool applyPose(DrawItem &item, const nativepose::Draw &pose) {
    if (pose.kind == nativepose::WEAPON_EFFECT_DRAW || queue(pose.opacity) == Queue::Hidden)
        return false;
    std::memcpy(&item.material[0], pose.tint, sizeof pose.tint);
    item.opacity = pose.opacity;
    item.nativeControls = Controls{pose.glow, pose.shininess, pose.white, pose.depthOffset, pose.kind == nativepose::MIRRORED_DRAW};
    return true;
}
} // namespace worldmodel
