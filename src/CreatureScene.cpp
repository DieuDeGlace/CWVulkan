#include "CreatureScene.h"
#include "GameMath.h"
#include <algorithm>
#include <cstring>

namespace creaturescene {
namespace {
template <class T> T read(const uint8_t *record, size_t field) {
    T value;
    std::memcpy(&value, record + field, sizeof value);
    return value;
}
} // namespace
Actor actor(const uint8_t *record) {
    Actor result;
    if (!record)
        return result;
    result.id = read<int64_t>(record, 8);
    result.physical = read<Point>(record, 0x10);
    result.drawn = read<Point>(record, 0xb10);
    std::memcpy(&result.body.x, record + 0x84, 12);
    result.yaw = read<float>(record, 0x30);
    result.drop = read<float>(record, 0x990);
    result.extraHeight = read<float>(record, 0x17c);
    result.health = read<float>(record, 0x180);
    result.race = read<int32_t>(record, 0x64);
    return result;
}
scenevisibility::Sphere actorSphere(const Actor &actor) {
    scenevisibility::Sphere result{actor.drawn, std::max(actor.body.z, .6f * std::max(actor.body.x, actor.body.y))};
    if (actor.race == 0x12e) {
        result.centre[2] = gamemath::addFixed(result.centre[2], int64_t((actor.extraHeight * .5f) * 65536.f));
        result.radius += actor.extraHeight * 1.2f;
    } else
        result.centre[2] = gamemath::addFixed(result.centre[2], int64_t(-actor.drop * 65536.f));
    return result;
}
std::optional<scenevisibility::Sphere> admit(const Actor &actor, bool localPlayer, const View &view) {
    if (((view.flags & 1) && localPlayer) || ((view.flags & 0x80) && !localPlayer) || !(actor.health > 0))
        return {};
    const auto sphere = actorSphere(actor);
    if (!view.visible || !view.visible(sphere))
        return {};
    return sphere;
}
} // namespace creaturescene
