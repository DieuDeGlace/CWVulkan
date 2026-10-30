#include "SteamCreatureLights.h"
#include "SteamTransientLights.h"
#include <cmath>
#include <cstring>

namespace steampoints {
namespace {
template <class T> T read(const uint8_t *record, size_t offset) {
    T value;
    std::memcpy(&value, record + offset, sizeof value);
    return value;
}
Point point(const uint8_t *record, size_t offset) { return read<Point>(record, offset); }
Point offset(Point at, glm::vec3 by) {
    for (int axis = 0; axis < 3; ++axis) at[axis] += (int64_t)(by[axis] * 65536.f);
    return at;
}
glm::vec3 rotated(glm::vec3 local, float degrees) {
    const float radians = degrees * .0174532924f;
    const float c = std::cos(radians), s = std::sin(radians);
    return {local.x * c - local.y * s, local.x * s + local.y * c, local.z};
}
}

void collectCreature(const uint8_t *record, const nativerules::HasBuff &hasBuff,
                     const Frame &frame, std::vector<Light> &out) {
    if (!record || !(read<float>(record, 0x180) > 0)) return;
    const Point physical = point(record, 0x10), drawn = point(record, 0xb10);
    const float drop = read<float>(record, 0x990), height = read<float>(record, 0x8c);
    const int32_t race = read<int32_t>(record, 0x64);
    const uint32_t flags = read<uint32_t>(record, 0x80);
    const bool alternate = record[0x19c] != 0;
    auto buff = [&](int type) { return hasBuff && hasBuff(type); };
    auto emit = [&](Point at, float radius, glm::vec3 colour) { out.push_back({at, radius, colour}); };
    const Point lamp = offset(drawn, rotated({.5f,1,.3f * height - drop}, read<float>(record, 0x30)));
    const float lampRadius = read<float>(record, 0x198);
    if (read<int32_t>(record, 0x63c) == 0x14 || read<int32_t>(record, 0x6dc) == 0x14)
        emit(lamp, lampRadius, {1.2f,1,.5f});
    if (read<uint16_t>(record, 0x128) & 1) emit(lamp, lampRadius, {.8f,.7f,.4f});
    if (buff(9)) emit(lamp, 16, {.6f,.4f,0});
    if (race == 0xb9) emit(lamp, 32, {.6f,.4f,0});

    Point lowered = drawn;
    const int64_t negativeDrop = (int64_t)(-drop * 65536.f);
    lowered[2] += negativeDrop;
    if (race == 0x75) emit(lowered, 16, {0,.4f,.8f});
    if (race == 0xdd && !alternate) emit(lowered, 16, {.8f,.6f,.1f});
    if (race == 0xde && !alternate) emit(lowered, 16, {0,.4f,.8f});
    if (race == 0xdf && !alternate) emit(lowered, 16, {0,.8f,.3f});
    if (race == 0x10e && !alternate) emit(lowered, 20, {0,.8f,.3f});
    if ((flags & 0x100000) || buff(0x13)) emit(lowered, 16, {.05f,.2f,.3f});
    if (flags & 0x4000000) emit(lowered, 16, {.25f,.1f,.1f});
    if (flags & 0x200) emit(lowered, 16, {.25f,.1f,.5f});
    if (race == 0x116) {
        Point raised = lowered;
        raised[2] += (int64_t)(height * .5f * 65536.f);
        emit(raised, 16, {.5f,.5f,.3f});
    }
    if (race >= 0x108 && race <= 0x10b && !alternate)
        emit(lowered, 20, {((float)record[0x7a] / 255.f) * .5f,
                           ((float)record[0x7b] / 255.f) * .5f, ((float)record[0x7c] / 255.f) * .5f});
    if (race == 0x10c && alternate) emit(lowered, 20, {.1f,.2f,.4f});
    if (race == 0x103) emit(lowered, 16, {0,.25f,.5f});
    Point physicalRaised = physical;
    physicalRaised[2] += negativeDrop;
    physicalRaised[2] += 98304;
    switch (race) {
    case 0x121: emit(physicalRaised, 16, {.3f,.5f,.6f}); break;
    case 0x87: emit(physicalRaised, 16, {.1f,.5f,.1f}); break;
    case 0x88: emit(physicalRaised, 16, {.1f,.1f,.5f}); break;
    case 0x89: emit(physicalRaised, 16, {.5f,.1f,.2f}); break;
    case 0x8a: emit(physicalRaised, 16, {.8f,.8f,.6f}); break;
    }

    const int ability = record[0x68];
    const float timer = read<float>(record, 0x6c);
    const bool duringDuration = ability == 0x1d || ability == 0x25 || (ability >= 0x68 && ability <= 0x6b);
    const bool duringWindup = ability == 0x26 || ability == 0x27 || ability == 0x2c || ability == 0x2d ||
                              ability == 0x2f || ability == 0x60 || ability == 0x61 || ability == 0x62 || ability == 0x65;
    const bool second = (ability >= 0x1f && ability <= 0x22) || ability == 0x86 || ability == 0x87;
    const int duration = duringDuration || duringWindup || second ? nativerules::abilityDuration(record, hasBuff) : 0;
    const int windup = duringWindup ? nativerules::abilityEffectTime(record, -1, hasBuff) : 0;
    if ((duringDuration && timer < duration) || (duringWindup && timer < windup)) {
        const float angle = (float)((double)(timer * 2.f) * 3.141592653589793 / (double)duration);
        const float flicker = 1.f + .1f * std::cos(angle);
        emit(offset(lowered, rotated({0,1,0}, read<float>(record, 0xb3c))), 20, glm::vec3(.3f,.2f,.1f) * flicker);
    }
    if (second && timer < duration) {
        const auto castOffset = read<glm::vec3>(record, 0xb4c);
        if (frame.visible && frame.visible(offset(physical, castOffset), 4, frame.abilityRange)) {
            Point at = physical;
            at[2] += negativeDrop;
            if (ability != 0x87) at = offset(at, castOffset);
            emit(at, 20, glm::vec3(.3f,.2f,.1f) * (1.f + .1f * std::cos(timer * .01f)));
        }
    }

    const float combo = (float)read<int32_t>(record, 0x70) / (float)nativerules::comboThreshold(record);
    if (combo != 0) {
        auto spirit = [&](size_t item, Point at, float factor) {
            const auto colour = spiritGlow(record + item);
            if (colour.w > 0) emit(at, 10, (glm::vec3(colour) * combo) * factor);
        };
        auto posePoint = [&](size_t field) {
            Point at = offset({}, read<glm::vec3>(record, field));
            at[0] -= read<int64_t>(record, 0x1508);
            at[1] -= read<int64_t>(record, 0x1510);
            return at;
        };
        if (record[0x6d8] == 3) spirit(0x6d8, posePoint(0x11f4), .3f);
        if (record[0x638] == 3) spirit(0x638, posePoint(0x1374), .5f);
        if (record[0x598]) spirit(0x598, physical, .1f);
        if (record[0x3b8]) spirit(0x3b8, physical, .1f);
    }
}
} // namespace steampoints
