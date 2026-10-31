#include "LinkedPlacement.h"
#include "GameMath.h"
#include "NativeCrt.h"
#include "NativeUpkeep.h"
#include <array>
#include <cstring>

namespace linkedplacement {
namespace {
using Point = std::array<int64_t, 3>;
constexpr size_t RAW = 0x10, DRAWN = 0xb10, ANGLES = 0x28, DRAWN_ANGLES = 0xb34;
constexpr int64_t J = 1966;
template <class T> T read(const uint8_t *record, size_t offset) {
    T value;
    std::memcpy(&value, record + offset, sizeof value);
    return value;
}
template <class T> void write(uint8_t *record, size_t offset, T value) {
    std::memcpy(record + offset, &value, sizeof value);
}
bool alive(const uint8_t *record) {
    return record && read<float>(record, 0x180) > 0;
}
int64_t fixed(float blocks) {
    return int64_t(blocks * 65536.0f);
}
void zero(uint8_t *record, size_t offset, size_t bytes) {
    std::memset(record + offset, 0, bytes);
}
void addState(uint8_t *record, uint16_t bits) {
    write(record, 0x128, uint16_t(read<uint16_t>(record, 0x128) | bits));
}
Point shifted(const Point &point, const Point &offset) {
    return {gamemath::addFixed(point[0], offset[0]), gamemath::addFixed(point[1], offset[1]), gamemath::addFixed(point[2], offset[2])};
}
void place(uint8_t *target, size_t field, const uint8_t *base, const Point &offset) {
    write(target, field, shifted(read<Point>(base, field), offset));
}
void copyAngles(uint8_t *target, const uint8_t *base) {
    std::memcpy(target + ANGLES, base + ANGLES, 12);
    std::memcpy(target + DRAWN_ANGLES, base + DRAWN_ANGLES, 12);
}
bool moving(const uint8_t *record) {
    const auto v = read<std::array<float, 3>>(record, 0x40);
    return (v[0] * v[0] + v[1] * v[1]) + v[2] * v[2] >= .001f;
}

void relationship(World &world, uint8_t *creature, float frameMs) {
    const uint8_t ability = creature[0x68];
    if (ability == 0x80) {
        auto *target = world.byId(read<int64_t>(creature, 0x1a0));
        const bool eligible =
            alive(target) && ((!(read<uint16_t>(target, 0x128) & 2) && ((read<uint32_t>(target, 0x80) & 0x4000) || target[0x68] == 0x82)) ||
                              (target[0x60] == 6 && read<int64_t>(creature, 0x9d8) == read<int64_t>(target, 8)));
        if (!eligible) {
            write<int64_t>(creature, 0x1a8, 0);
            creature[0x68] = 0;
            return;
        }
        addState(target, 0x10);
        target[0x68] = 0;
        const float height = read<float>(creature, 0x8c) * .5f + read<float>(target, 0x8c) * .25f;
        const Point offset{0, 0, fixed(height)};
        place(target, DRAWN, creature, offset);
        std::memcpy(target + DRAWN_ANGLES, creature + DRAWN_ANGLES, 12);
        place(target, RAW, creature, offset);
        std::memcpy(target + ANGLES, creature + ANGLES, 12);
        std::memcpy(target + 0x990, creature + 0x990, 4);
        if (!(read<uint32_t>(target, 0x80) & 2))
            zero(target, 0x998, 8);
        std::memcpy(target + 0x34, creature + 0x34, 12);
        zero(target, 0x40, 12);
        return;
    }
    const int32_t race = read<int32_t>(creature, 0x64);
    if (race != 0xbb && race != 0xbc && ability != 0x83)
        return;
    auto *target = world.byId(read<int64_t>(creature, 0x1a8));
    if (!target)
        return;
    if (ability != 0x83 && moving(target)) {
        write<int64_t>(creature, 0x1a8, 0);
        return;
    }
    if (!alive(target))
        return;
    if (ability != 0x83 && moving(target)) {
        write<int64_t>(creature, 0x1a8, 0);
        return;
    }
    std::memcpy(target + 0x990, creature + 0x990, 4);
    std::memcpy(target + 0x34, creature + 0x34, 12);
    zero(target, 0x40, 12);
    zero(target, 0x998, 8);

    const float h = read<float>(creature, 0x8c) * .5f;
    const float q = read<float>(target, 0x8c) * .25f;
    const float u = read<float>(target, 0x8c) * .5f;
    const float v = read<float>(target, 0x8c) * .2f;
    float rawHeight = (h + .03f) + q, drawnHeight = rawHeight;
    uint8_t targetAbility = 0x58;
    bool seated = true, resetTimer = false, keepAngles = false, spin = false;
    int64_t rawXY = J;
    if (creature[0x60] == 8 && race >= 0xc2 && race <= 0xc5) {
        rawHeight = drawnHeight = (.03f - h) + u;
        targetAbility = 0x57;
    } else {
        switch (race) {
        case 0x10f:
            targetAbility = 0x5b;
            resetTimer = true;
            break;
        case 0x111:
            rawHeight = drawnHeight = (.03f - h) + u;
            targetAbility = 0x57;
            break;
        case 0x112:
            rawHeight = drawnHeight = (.03f - h) + u;
            targetAbility = 0x5c;
            resetTimer = true;
            break;
        case 0x113:
            rawHeight = drawnHeight = (.03f - h) - v;
            targetAbility = 0x5d;
            break;
        case 0xdd:
        case 0xde:
            rawHeight = drawnHeight = (h + .03f) + u;
            targetAbility = 0;
            keepAngles = true;
            break;
        case 0x12b:
            rawHeight = drawnHeight = (h + .03f) + u;
            targetAbility = 0;
            spin = true;
            break;
        default:
            if (ability == 0x83) {
                rawHeight = h;
                drawnHeight = h + .03f;
                rawXY = 0;
                targetAbility = 0;
            } else {
                seated = false;
            }
        }
    }
    place(target, RAW, creature, {rawXY, rawXY, fixed(rawHeight)});
    if (spin) {
        const float increment = frameMs * .01f;
        write(target, 0x30, read<float>(target, 0x30) + increment);
        std::memcpy(target + DRAWN, target + RAW, 24);
        std::memcpy(target + DRAWN_ANGLES, target + ANGLES, 12);
    } else {
        place(target, DRAWN, creature, {J, J, fixed(drawnHeight)});
        if (!keepAngles)
            copyAngles(target, creature);
    }
    target[0x68] = targetAbility;
    if (resetTimer)
        write(target, 0x6c, 0.0f);
    if (seated)
        addState(target, 2);
}

void followLook(World &world, uint8_t *creature) {
    const int64_t id = read<int64_t>(creature, 0x1a8);
    if (!id)
        return;
    auto *target = world.byId(id);
    if (!alive(target))
        return;
    const auto look = read<std::array<float, 3>>(creature, 0x174);
    const Point offset{fixed(look[0]), fixed(look[1]), fixed(look[2])};
    place(creature, RAW, target, offset);
    place(creature, DRAWN, target, offset);
}

void focus(World &world, uint8_t *creature) {
    const int64_t id = read<int64_t>(creature, 0x1a8);
    uint8_t *target = nullptr;
    if (read<float>(creature, 0x6c) < float(world.duration(creature)) && id)
        target = world.byId(id);
    if (!alive(target)) {
        if (world.focusFailure)
            world.focusFailure(id);
        creature[0x68] = 0;
        target = id ? world.byId(id) : nullptr;
        if (alive(target))
            std::memcpy(creature + RAW, target + RAW, 24);
        return;
    }
    const auto offset = nativeupkeep::ridingOffset(read<int32_t>(target, 0x64));
    const float zBase = offset[2] + read<float>(target, 0x8c) * .75f;
    constexpr double PI = 3.141592653589793;
    const float radians = float((double(read<float>(target, 0x30)) * PI) / 180.0 + PI / 2.0);
    const float radius = std::min(read<float>(target, 0x84), read<float>(target, 0x88)) * -.5f;
    const int64_t radialX = fixed(nativecrt::cosf(radians) * radius);
    const int64_t radialY = fixed(nativecrt::sinf(radians) * radius);
    const float correction = (read<float>(target, 0x8c) * .5f - read<float>(creature, 0x8c) * .5f) + .01f;
    const Point placement{gamemath::addFixed(fixed(offset[0]), radialX), gamemath::addFixed(fixed(offset[1]), radialY),
                          gamemath::addFixed(gamemath::subtractFixed(0, fixed(correction)), fixed(zBase))};
    place(creature, RAW, target, placement);
    place(creature, DRAWN, target, placement);
    std::memcpy(creature + ANGLES, target + ANGLES, 12);
    std::memcpy(creature + 0x990, target + 0x990, 4);
    zero(creature, 0x998, 8);
    zero(creature, 0x34, 36); // velocity, intent and knockback
    write(creature, 0x12c, 0.0f);
}

void riddenPet(World &world, uint8_t *creature) {
    auto *owner = world.byId(read<int64_t>(creature, 0x1a0));
    if (!owner || owner[0x68] != 0x76)
        return;
    const float height = (read<float>(creature, 0x8c) * .5f - read<float>(owner, 0x8c) * .5f) + .01f;
    const Point offset{0, 0, fixed(height)};
    place(creature, DRAWN, owner, offset);
    std::memcpy(creature + DRAWN_ANGLES, owner + DRAWN_ANGLES, 12);
    place(creature, RAW, owner, offset);
    std::memcpy(creature + ANGLES, owner + ANGLES, 12);
    write(creature, 0xb58, 0.0f);
    std::memcpy(creature + 0x990, owner + 0x990, 4);
    std::memcpy(creature + 0x998, owner + 0x998, 4);
    const float scaled = read<float>(owner, 0x99c) * read<float>(owner, 0x84);
    write(creature, 0x99c, scaled / read<float>(creature, 0x84));
    zero(creature, 0x34, 24);
}
} // namespace

void update(World &world, uint8_t *creature, float frameMs) {
    relationship(world, creature, frameMs);
    if (creature[0x68] == 0x91)
        followLook(world, creature);
    if (creature[0x68] == 0x8c)
        focus(world, creature);
    if (creature[0x60] == 6)
        riddenPet(world, creature);
}
} // namespace linkedplacement
