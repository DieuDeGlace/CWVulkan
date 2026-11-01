#include "NativeWorldObjects.h"
#include "GameMath.h"
#include "NativeBuffs.h"
#include "NativeInteract.h"
#include "worldgen/Residents.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace nativeobjects {
namespace {
constexpr size_t ID = worldgen::creature::ID, POSITION = 0x10, KIND = 0x60, STATE_FLAGS = 0x128;
constexpr size_t OBJECT_TYPE = 0, OBJECT_POSITION = 8;
constexpr int64_t ACTIVATION_DISTANCE = 0x1900000, HIT_DISTANCE = 0x190000;
using Point = std::array<int64_t, 3>;

template <class T> T read(const uint8_t *record, size_t offset) {
    T value;
    std::memcpy(&value, record + offset, sizeof value);
    return value;
}
template <class T> void write(uint8_t *record, size_t offset, T value) {
    std::memcpy(record + offset, &value, sizeof value);
}
Point position(const uint8_t *record, size_t offset) {
    Point result;
    std::memcpy(result.data(), record + offset, sizeof result);
    return result;
}
int64_t distance(const Point &a, const Point &b) {
    int64_t sum = 0;
    for (size_t axis = 0; axis < a.size(); ++axis) {
        const int64_t delta = gamemath::subtractFixed(a[axis], b[axis]);
        const int64_t square = gamemath::multiplyFixed(delta, delta);
        sum = gamemath::addFixed(sum, square / 65536);
    }
    return sum;
}
int32_t truncateFloat(float value) {
    // CVTTSS2SI's masked invalid result, avoiding undefined C++ float-to-int casts.
    if (!(value >= -2147483648.0f && value < 2147483648.0f))
        return std::numeric_limits<int32_t>::min();
    return int32_t(value);
}
void emit(World &world, const soundrules::RequestRecord &sound) {
    if (world.sound)
        world.sound(sound);
}
bool playerNear(const World &world, const nativeai::Object &object) {
    if (world.creatures)
        for (nativeai::Creature *creature : *world.creatures)
            if (creature && creature->record && creature->record[KIND] == 0 &&
                distance(position(object.data(), OBJECT_POSITION), position(creature->record, POSITION)) < ACTIVATION_DISTANCE)
                return true;
    return false;
}
float maximumHealth(const World &world, const nativeai::Creature &creature) {
    if (world.maxHealth)
        return world.maxHealth(creature);
    const auto buffs = nativebuffs::forStats(creature);
    return nativerules::maxHealth(creature.record, buffs.data(), buffs.size());
}
} // namespace

void collectZones(World &world, const std::function<void(Cell)> &lookup) {
    std::vector<const uint8_t *> players;
    if (world.creatures)
        for (nativeai::Creature *creature : *world.creatures) {
            if (!creature || !creature->record)
                continue;
            auto *record = creature->record;
            write<uint16_t>(record, STATE_FLAGS, read<uint16_t>(record, STATE_FLAGS) & uint16_t(0xdffd));
            if (record[KIND] == 0)
                players.push_back(record);
        }
    if (!lookup)
        return;
    for (const uint8_t *player : players) {
        const int32_t x = gamemath::floorFixedZone(read<int64_t>(player, POSITION));
        const int32_t y = gamemath::floorFixedZone(read<int64_t>(player, POSITION + 8));
        for (int32_t zx = x - 3; zx <= x + 3; ++zx)
            for (int32_t zy = y - 3; zy <= y + 3; ++zy)
                lookup({zx, zy});
    }
}

void advanceClock(nativeai::Object &object, float milliseconds, const std::set<Cell> *usedCells) {
    auto *record = object.data();
    write<float>(record, nativeinteract::OBJECT_SWING, read<float>(record, nativeinteract::OBJECT_SWING) + milliseconds);
    const int32_t type = read<int32_t>(record, OBJECT_TYPE);
    if (usedCells && (type == 0 || type == 0x16)) {
        const Cell cell{gamemath::floorFixedCell(read<int64_t>(record, OBJECT_POSITION)),
                        gamemath::floorFixedCell(read<int64_t>(record, OBJECT_POSITION + 8))};
        const uint8_t state = usedCells->count(cell) ? 0 : 1;
        if (record[nativeinteract::OBJECT_STATE] != state) {
            record[nativeinteract::OBJECT_STATE] = state;
            write<float>(record, nativeinteract::OBJECT_SWING, 0);
        }
    }
}

void step(World &world, nativeai::Object &object, float milliseconds) {
    advanceClock(object, milliseconds, world.usedCells);
    const int32_t type = read<int32_t>(object.data(), OBJECT_TYPE);
    if (world.client || type < 0x11 || type > 0x13)
        return;
    if (read<float>(object.data(), nativeinteract::OBJECT_SWING) > 2000.0f) {
        const uint8_t oldState = object[nativeinteract::OBJECT_STATE];
        if (oldState != 0 || playerNear(world, object)) {
            std::vector<nativeinteract::Sound> sounds;
            if (nativeinteract::openOrShut(object, oldState ? 0 : 1, sounds)) {
                for (const auto &sound : sounds)
                    emit(world, sound);
                if (world.objectsChanged)
                    world.objectsChanged();
            }
        }
    }
    if (object[nativeinteract::OBJECT_STATE] == 0)
        return;
    if (type == 0x13) {
        if (read<float>(object.data(), nativeinteract::OBJECT_SWING) != 0.0f)
            return;
    } else {
        const float future = world.clockMs + milliseconds;
        const int32_t futureBucket = truncateFloat(future) / 200;
        const int32_t currentBucket = truncateFloat(world.clockMs) / 200;
        if (futureBucket == currentBucket)
            return;
    }
    // Read the current list length after callbacks, so appended tail entries participate.
    for (size_t index = 0; world.creatures && index < world.creatures->size(); ++index) {
        nativeai::Creature *target = (*world.creatures)[index];
        if (!target || !target->record || target->record[KIND] == 1)
            continue;
        const Point origin = position(object.data(), OBJECT_POSITION);
        const Point at = position(target->record, POSITION);
        if (distance(origin, at) >= HIT_DISTANCE)
            continue;
        nativeupkeep::Hit hit;
        hit.target = read<int64_t>(target->record, ID);
        hit.type = type == 0x11 ? 1 : 0;
        hit.last = 1;
        std::copy(origin.begin(), origin.end(), hit.where);
        float x = gamemath::fixedBlocksBetween(at[0], origin[0]);
        float y = gamemath::fixedBlocksBetween(at[1], origin[1]);
        const float lengthSquared = (x * x + y * y) + 0.0f;
        if (lengthSquared > 0.0f) {
            const float inverseLength = 1.0f / std::sqrt(lengthSquared);
            x *= inverseLength;
            y *= inverseLength;
        }
        hit.push[0] = x * 20.0f;
        hit.push[1] = y * 20.0f;
        hit.push[2] = 5.0f;
        hit.damage = maximumHealth(world, *target) * 0.1f;
        if (type == 0x13) {
            hit.push[0] += hit.push[0];
            hit.push[1] += hit.push[1];
            hit.push[2] = 10.0f;
            hit.damage *= 5.0f;
        }
        if (world.hit)
            world.hit(*target, hit);
        if (type != 0x11) {
            soundrules::Request sound;
            const Point after = position(target->record, POSITION);
            std::copy(after.begin(), after.end(), sound.position);
            sound.number = 1;
            float pitch = float(world.rand ? world.rand() : 0) * 0.3f;
            pitch /= 32767.0f;
            sound.pitch = pitch + 1.0f;
            // Owned request storage defines padding; the native padding is unconstrained.
            emit(world, soundrules::makeRequest(sound));
        }
    }
}
} // namespace nativeobjects
