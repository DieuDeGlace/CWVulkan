#include "NativeDeadCreatures.h"
#include "NativeBuffs.h"
#include "worldgen/Residents.h"
#include <cstring>

namespace nativedeath {
namespace {
constexpr size_t KIND = 0x60, RACE = 0x64, CLOCK = 0x6c, HEALTH = 0x180;
constexpr size_t OWNER = 0x1a0, LAST_FOE = 0x9e0;
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
        // These need their first-clock-zero burst, terrain/hit/heal work before the
        // clock advance. Preserve that input rather than consuming it prematurely.
        if (race == 0x90 || race == 0x119 || race == 0x11d)
            continue;
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
