#include "NativeDeadCreatures.h"
#include "GameMath.h"
#include "NativeBuffs.h"
#include "worldgen/Residents.h"
#include <bit>
#include <cmath>
#include <cstring>

namespace nativedeath {
namespace {
constexpr size_t KIND = 0x60, RACE = 0x64, CLOCK = 0x6c, HEALTH = 0x180;
constexpr size_t OWNER = 0x1a0, LAST_FOE = 0x9e0;
constexpr size_t SIZE_X = 0x84, SIZE_Z = 0x8c, FADE = 0x12c, DAMAGE = 0x184, DAMAGE_SCALE = 0x188;
template <class T> T read(const uint8_t *record, size_t offset) {
    T value;
    std::memcpy(&value, record + offset, sizeof value);
    return value;
}
template <class T> void write(uint8_t *record, size_t offset, T value) {
    std::memcpy(record + offset, &value, sizeof value);
}
float maximumHealth(const World &world, const nativeai::Creature &creature) {
    if (world.maxHealth)
        return world.maxHealth(creature);
    const auto buffs = nativebuffs::forStats(creature);
    return nativerules::maxHealth(creature.record, buffs.data(), buffs.size());
}
bool hostile(const World &world, const nativeai::Creature &source, const nativeai::Creature &target) {
    return world.hostile ? world.hostile(source, target) : nativerules::hostile(source.record, target.record);
}
void firstBurst(World &world, const nativeai::Creature &source, bool water) {
    // Native leaves alpha/padding unspecified. Keep defined zero storage here;
    // this is not oracle data. The traced style1/2 draw submits alpha1 (§2.1).
    World::Record burst{};
    std::memcpy(burst.data(), source.record + worldgen::creature::POSITION, 24);
    write(burst.data(), 0x20, 20.0f);
    write(burst.data(), 0x24, water ? 0.0f : 1.0f);
    write(burst.data(), 0x28, water ? 0.5f : 0.2f);
    write(burst.data(), 0x2c, water ? 1.0f : 0.5f);
    write(burst.data(), 0x34, 0.25f);
    write<int32_t>(burst.data(), 0x38, 20);
    write<int32_t>(burst.data(), 0x3c, water ? 2 : 1);
    write(burst.data(), 0x40, 3.0f);
    if (world.burst)
        world.burst(burst);
}
void destroyTerrain(World &world, nativeai::Creature &source) {
    std::array<int64_t, 3> center;
    std::memcpy(center.data(), source.record + worldgen::creature::POSITION, sizeof center);
    bool accepted = false;
    for (int x = -8; x <= 8; ++x)
        for (int y = -8; y <= 8; ++y)
            for (int z = -8; z <= 8; ++z) {
                const std::array<int64_t, 3> sample{gamemath::addFixed(center[0], int64_t(x) * 65536),
                                                    gamemath::addFixed(center[1], int64_t(y) * 65536),
                                                    gamemath::addFixed(center[2], int64_t(z) * 65536)};
                const float dx = gamemath::fixedBlocksBetween(sample[0], center[0]),
                            dy = gamemath::fixedBlocksBetween(sample[1], center[1]),
                            dz = gamemath::fixedBlocksBetween(sample[2], center[2]);
                const float horizontal = dx * dx + dy * dy;
                if (!(horizontal + dz * dz < 64.0f))
                    continue;
                const std::array<int32_t, 3> at{gamemath::floorFixedBlock(sample[0]), gamemath::floorFixedBlock(sample[1]),
                                                gamemath::floorFixedBlock(sample[2])};
                const auto before = world.block(at[0], at[1], at[2]);
                if (!(before[5] & 0x11))
                    continue;
                World::Record debris{};
                for (int axis = 0; axis < 3; ++axis) {
                    write(debris.data(), axis * 8, gamemath::addFixed(sample[axis], 32768));
                    write(debris.data(), 0x24 + axis * 4, float(before[axis]) * (1.0f / 128.0f));
                }
                write(debris.data(), 0x20, 10.0f);
                write(debris.data(), 0x30, 1.0f);
                write(debris.data(), 0x34, 0.5f);
                write<int32_t>(debris.data(), 0x38, 3);
                write<int32_t>(debris.data(), 0x3c, 5);
                write(debris.data(), 0x40, 3.0f);
                if (world.burst)
                    world.burst(debris);
                world.removeBlock(sample[0], sample[1], sample[2]);
                std::array<uint8_t, 0x20> change{};
                std::memcpy(change.data(), at.data(), sizeof at);
                std::memcpy(change.data() + 0x0c, before.data(), before.size());
                const std::array<uint8_t, 6> empty{255, 255, 255, 0, 0, 0};
                std::memcpy(change.data() + 0x12, empty.data(), empty.size());
                write(change.data(), 0x18, world.day);
                if (world.blockChange)
                    world.blockChange(change);
                accepted = true;
            }
    if (!accepted)
        return;
    std::memcpy(center.data(), source.record + worldgen::creature::POSITION, sizeof center);
    float pitch = float(world.rand ? world.rand() : 0) * 0.4f;
    pitch /= 32767.0f;
    pitch += 0.5f;
    std::array<uint8_t, 0x30> sound{};
    std::memcpy(sound.data(), center.data(), sizeof center);
    write<int32_t>(sound.data(), 0x18, 1);
    write(sound.data(), 0x1c, pitch);
    write(sound.data(), 0x20, 1.0f);
    if (world.sound)
        world.sound(sound);
    const auto edge = [](int64_t fixed, int offset) {
        const uint32_t bits = uint32_t(gamemath::addFixed(fixed, int64_t(offset) * 65536) / 65536);
        int32_t value;
        std::memcpy(&value, &bits, sizeof value);
        return value;
    };
    world.rebuild(edge(center[0], -8), edge(center[1], -8), edge(center[0], 8), edge(center[1], 8));
}
bool areaPush(const uint8_t *source, const uint8_t *target, float (&push)[3]) {
    const auto difference = [&](int axis) {
        const size_t offset = worldgen::creature::POSITION + axis * 8;
        return gamemath::subtractFixed(read<int64_t>(source, offset), read<int64_t>(target, offset));
    };
    const double dx = double(difference(0)) * (1.0 / 65536.0);
    const double dy = double(difference(1)) * (1.0 / 65536.0);
    float radius = read<float>(target, SIZE_X) * 0.7f;
    radius += 8.0f;
    const float radiusSquared = radius * radius;
    if (std::abs(dx) > double(radius) || std::abs(dy) > double(radius) || dx * dx + dy * dy > double(radiusSquared))
        return false;
    const float dz = float(difference(2)) * (1.0f / 65536.0f);
    float height = read<float>(target, SIZE_Z) * 0.5f;
    height += read<float>(source, SIZE_Z);
    height += 8.0f;
    if (std::abs(dz) >= height)
        return false;
    for (int axis = 0; axis < 2; ++axis) {
        const size_t offset = worldgen::creature::POSITION + axis * 8;
        push[axis] = gamemath::fixedBlocksBetween(read<int64_t>(target, offset), read<int64_t>(source, offset));
    }
    float lengthSquared = push[0] * push[0] + push[1] * push[1];
    lengthSquared += 0.0f;
    if (lengthSquared > 0.01f) {
        const float reciprocal = 1.0f / std::sqrt(lengthSquared);
        push[0] *= reciprocal;
        push[1] *= reciprocal;
    }
    push[0] *= 10.0f;
    push[1] *= 10.0f;
    push[2] = 2.5f;
    return true;
}
void area(World &world, nativeai::Creature &source, bool water, bool destructive = false) {
    firstBurst(world, source, water);
    if (destructive)
        destroyTerrain(world, source);
    for (size_t index = 0; index < world.creatures->size(); ++index) {
        nativeai::Creature *target = (*world.creatures)[index];
        if (!target || !target->record || target == &source || !hostile(world, source, *target) ||
            !(read<float>(target->record, HEALTH) > 0.0f) || read<float>(target->record, FADE) != 0.0f)
            continue;
        const int64_t id = read<int64_t>(target->record, worldgen::creature::ID);
        if (source.counted.contains(id))
            continue;
        nativeupkeep::Hit hit;
        if (!areaPush(source.record, target->record, hit.push))
            continue;
        hit.critical = (world.rand ? world.rand() : 0) % 4 == 0;
        hit.damage = read<float>(source.record, DAMAGE);
        if (hit.critical)
            hit.damage += hit.damage;
        source.counted.insert(id);
        if (target->record[KIND] == 8)
            continue;
        hit.target = id;
        hit.attacker = read<int64_t>(source.record, worldgen::creature::ID);
        hit.flag = hit.last = 1;
        hit.type = 1;
        std::memcpy(hit.where, source.record + worldgen::creature::POSITION, 24);
        if (water) {
            hit.damage *= read<float>(source.record, DAMAGE_SCALE);
            if (!(hit.damage > 0.0f))
                continue;
            hit.attacker = read<int64_t>(source.record, OWNER);
            nativeai::Creature *owner = world.byId ? world.byId(hit.attacker) : nullptr;
            if (!owner || !owner->record)
                continue;
            if (!hostile(world, *owner, *target)) {
                if (target->record[KIND] == 7 || !(read<float>(target->record, HEALTH) > 0.0f))
                    continue;
                World::Record number{};
                write(number.data(), 0, hit.attacker);
                write(number.data(), 8, id);
                write(number.data(), 0x10, std::bit_cast<float>(std::bit_cast<uint32_t>(hit.damage) ^ 0x80000000u));
                number[0x14] = hit.critical;
                std::memcpy(number.data() + 0x20, target->record + worldgen::creature::POSITION, 24);
                if (world.number) {
                    world.number(number);
                    world.number(number);
                }
                if (world.apply)
                    world.apply(number);
                continue;
            }
            hit.type = 2;
        }
        if (world.hit)
            world.hit(hit);
    }
}
} // namespace

std::vector<int64_t> step(World &world, float milliseconds) {
    std::vector<int64_t> retired;
    if (world.client || !world.creatures)
        return retired;
    for (size_t index = 0; index < world.creatures->size(); ++index) {
        nativeai::Creature *source = (*world.creatures)[index];
        if (!source || !source->record)
            continue;
        uint8_t *record = source->record;
        if (record[KIND] == 0 || !(read<float>(record, HEALTH) <= 0.0f))
            continue;
        if (world.markDead)
            world.markDead(*source);
        const int32_t race = read<int32_t>(record, RACE);
        const bool firstVisit = read<float>(record, CLOCK) == 0.0f;
        if (race == 0x90 && firstVisit && (!world.block || !world.removeBlock || !world.rebuild))
            continue;
        if ((race == 0x90 || race == 0x119 || race == 0x11d) && firstVisit)
            area(world, *source, race == 0x11d, race == 0x90);
        const float clock = read<float>(record, CLOCK) + milliseconds;
        write(record, CLOCK, clock);
        const int64_t ownerId = read<int64_t>(record, OWNER);
        if (record[KIND] == 6 && ownerId != 0) {
            nativeai::Creature *owner = world.byId ? world.byId(ownerId) : nullptr;
            if (owner && owner->record && clock >= 20000.0f && read<float>(owner->record, HEALTH) >= 0.0f) {
                write(record, HEALTH, maximumHealth(world, *source));
                std::memcpy(record + worldgen::creature::POSITION, owner->record + worldgen::creature::POSITION, 24);
                write<int64_t>(record, LAST_FOE, 0);
                source->aggro.clear();
            }
        } else if (clock >= 1000.0f) {
            const int64_t id = read<int64_t>(record, worldgen::creature::ID);
            retired.push_back(id);
            for (nativeai::Creature *other : *world.creatures)
                if (other) {
                    other->aggro.erase(id);
                    other->damage.erase(id);
                }
        }
    }
    return retired;
}
} // namespace nativedeath
