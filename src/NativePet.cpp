#include "NativePet.h"
#include "NativeInteract.h"
#include "NativeRules.h"
#include "NativeSaves.h"
#include "worldgen/Residents.h"

#include <algorithm>
#include <cstring>
#include <limits>

namespace nativepet {
namespace {
using nativeai::Creature;
using namespace worldgen::creature;
constexpr size_t PET_ITEM = 0x8b8, PET_VALUE = 0x190, CACHED_VALUE = 0x1544, CACHED_ITEM = 0x1548;
template<class T> T get(const uint8_t *record, size_t offset) {
    T value;
    std::memcpy(&value, record + offset, sizeof value);
    return value;
}
template<class T> void put(uint8_t *record, size_t offset, T value) { std::memcpy(record + offset, &value, sizeof value); }
int64_t id(const Creature &c) { return get<int64_t>(c.record, ID); }
Creature *find(const World &world, int64_t wanted) {
    if (wanted)
        for (Creature *c : world.creatures)
            if (id(*c) == wanted)
                return c;
    return nullptr;
}
float maximum(const World &world, const Creature &c) {
    if (world.maximumHealth)
        return world.maximumHealth(c);
    const auto buffs = nativebuffs::forStats(c);
    return nativerules::maxHealth(c.record, buffs.data(), buffs.size());
}
void nameFromItem(Creature &pet, const uint8_t *item) {
    const uint8_t length = item[0x9d];
    if (length < 16) {
        for (unsigned i = 0; i < length; ++i)
            pet.record[NAME + i] = item[0x20 + 4 * i];
        pet.record[NAME + length] = 0;
    }
}
void setup(World &world, Creature &owner, Creature &pet) {
    if (world.setup)
        world.setup(id(owner), &pet);
    put(owner.record, PET_ID, id(pet));
    put(pet.record, OWNER, id(owner));
    put(pet.record, ROAM, uint32_t(PET_ROAM));
}
bool blocksCreation(uint8_t ability) {
    return ability == 0x78 || ability == 0x79 || ability == 0x98 || ability == 0x9c || ability == 0x9d;
}
bool gliding(const uint8_t *c) {
    return c[ABILITY] == 0x78 && !(get<uint32_t>(c, 0x5c) & 1) && get<float>(c, STUN) <= 0 && get<float>(c, 0x12c) <= 0;
}
void reconcile(World &world, Creature &owner) {
    uint8_t *c = owner.record;
    Creature *pet = find(world, get<int64_t>(c, PET_ID));
    if (c[HOSTILITY] != 0 || c[PET_ITEM] != 20) {
        if (pet && nativebuffs::find(*pet, 7))
            return;
        put(c, PET_ID, int64_t(0));
        if (pet)
            put(pet->record, HEALTH, 0.0f);
        return;
    }
    if (!pet) {
        put(c, PET_ID, int64_t(0));
        for (Creature *candidate : world.creatures)
            if (get<int64_t>(candidate->record, OWNER) == id(owner)) {
                pet = candidate;
                put(c, PET_ID, id(*pet));
                break;
            }
    }
    if (world.client)
        return;
    bool changed = false;
    if (pet) {
        if (get<float>(pet->record, HEALTH) <= 0)
            owner.damage.clear();
        changed = get<float>(c, PET_VALUE) != get<float>(c, CACHED_VALUE) ||
                  !nativeinteract::sameItem(c + PET_ITEM, c + CACHED_ITEM);
        if (changed) {
            const int32_t race = get<int32_t>(c, PET_ITEM + 4);
            const bool changedRace = race != get<int32_t>(c, CACHED_ITEM + 4);
            if (changedRace)
                resetTransient(*pet);
            put(pet->record, RACE, race);
            nameFromItem(*pet, c + PET_ITEM);
            put(pet->record, OWNER, id(owner));
            if (changedRace)
                setup(world, owner, *pet);
        }
        if (const auto *buff = nativebuffs::find(owner, 0x13); buff && !nativebuffs::find(*pet, 0x13)) {
            nativebuffs::give(*pet, *buff, true);
            world.buffNotes.push_back(nativebuffs::notice(id(owner), id(*pet), *buff, 1));
        }
        const float oldMaximum = maximum(world, *pet);
        put(pet->record, LEVEL, get<int32_t>(c, LEVEL));
        put(pet->record, HEALTH, (maximum(world, *pet) / oldMaximum) * get<float>(pet->record, HEALTH));
    }
    put(c, CACHED_VALUE, get<float>(c, PET_VALUE));
    std::memcpy(c + CACHED_ITEM, c + PET_ITEM, ITEM_SIZE);
    if (!get<int64_t>(c, PET_ID) && !changed && !blocksCreation(c[ABILITY]) && world.construct) {
        int64_t lowest = std::numeric_limits<int64_t>::min() + 1000;
        for (const Creature *candidate : world.creatures)
            lowest = std::min(lowest, id(*candidate));
        pet = world.construct((int64_t)((uint64_t)lowest - 1));
        if (!pet)
            return;
        world.creatures.push_back(pet);
        if (world.added)
            world.added(pet);
        resetTransient(*pet);
        put(pet->record, OWNER, id(owner));
        pet->record[HOSTILITY] = 6;
        put(pet->record, FIELD_1530, get<float>(c, FIELD_1530));
        put(pet->record, RACE, get<int32_t>(c, PET_ITEM + 4));
        std::memcpy(pet->record + POSITION, c + POSITION, 24);
        put(pet->record, HEALTH, maximum(world, *pet));
        pet->record[PEOPLE] = c[PEOPLE];
        nameFromItem(*pet, c + PET_ITEM);
        put(pet->record, FLAGS, get<uint32_t>(pet->record, FLAGS) | (get<uint32_t>(c, FLAGS) & 0x200));
        setup(world, owner, *pet);
    }
}
void inheritEquipment(World &world, Creature &pet, Creature &owner) {
    uint8_t *c = pet.record;
    const uint8_t *o = owner.record;
    std::memcpy(c + REGION, o + REGION, 8);
    if (get<int64_t>(o, PET_ID) != id(pet) || o[ABILITY] == 0x79 || (gliding(o) && get<float>(o, 0xbb4) > 3000.0f)) {
        resetTransient(pet);
        put(c, HEALTH, 0.0f);
        c[HOSTILITY] = 3;
        put(c, 0x6c, 5000.0f);
        put(c, OWNER, int64_t(0));
        return;
    }
    const float oldMaximum = maximum(world, pet);
    const int32_t region[2] = {get<int32_t>(o, REGION), get<int32_t>(o, REGION + 4)};
    for (const size_t slot : {size_t(0x638), size_t(0x6d8), size_t(0x3b8)}) {
        const uint8_t *item = o + slot;
        c[slot + nativesaves::item::RARITY] = item[0] ? item[nativesaves::item::RARITY] : 0;
        if (item[0] || slot == 0x3b8)
            std::memcpy(c + slot + nativesaves::item::REGION, item + nativesaves::item::REGION, 8);
        if (nativerules::renowned(item) && nativerules::itemLevel(item, region) == item[nativesaves::item::RARITY])
            std::memcpy(c + slot + nativesaves::item::REGION, c + REGION, 8);
    }
    if (oldMaximum > 0)
        put(c, HEALTH, (maximum(world, pet) / oldMaximum) * get<float>(c, HEALTH));
}
} // namespace

void resetTransient(Creature &creature) {
    uint8_t *c = creature.record;
    auto zero = [&](size_t offset, size_t count) { std::memset(c + offset, 0, count); };
    zero(0x34, 0x24), zero(0x68, 0x10), zero(0x128, 2), zero(0x134, 0x10);
    std::memcpy(c + APPEARANCE, worldgen::Appearance::constructed().bytes.data(), worldgen::Appearance::SIZE);
    put(c, STUN, -3000.0f);
    put(c, LEVEL, int32_t(1));
    zero(0x148, 4), zero(0x14c, 2), zero(0x158, 0x1c), zero(0x184, 0x10), zero(0x19c, 2);
    put(c, GRIP, 1.0f), put(c, LIGHT_RADIUS, 10.0f);
    zero(0x1a0, 0x10);
    put(c, HOME_PLACE, int32_t(-1)), put(c, HOME_PLACE + 4, int32_t(-1));
    put(c, HOME_CELL, int32_t(-1)), put(c, HOME_CELL + 4, int32_t(-1));
    zero(EQUIPMENT, 0x6e0), zero(NAME, 0x28), zero(PET_ID, 0x10);
    zero(0xad8, 4), zero(CACHED_VALUE, 4), zero(0xc18, 4), zero(0xbc0, 8), zero(0xbc8, 4);
    zero(0x994, 8), zero(0xb58, 4), zero(0x99c, 4), zero(0xbb0, 8), zero(0x9e8, 4), zero(0xba8, 4), zero(0xbb8, 1);
    put(c, STAMINA, 1.0f);
    creature.interactions.clear(), creature.aggro.clear(), creature.damage.clear();
    creature.cooldowns.clear(), creature.inventory.clear(), creature.buffs.clear();
}

void step(World &world) {
    if (world.active)
        for (size_t i = 0; i < world.creatures.size(); ++i)
            reconcile(world, *world.creatures[i]);
    if (world.local && get<int32_t>(world.local->record, PET_ITEM + 0x18) < 1)
        put(world.local->record, PET_ITEM + 0x18, int32_t(1));
    if (!world.client)
        for (Creature *pet : world.creatures)
            if (get<int64_t>(pet->record, OWNER) && !(get<uint32_t>(pet->record, FLAGS) & 0x800))
                if (Creature *owner = find(world, get<int64_t>(pet->record, OWNER)); owner && owner->record[HOSTILITY] == 0)
                    inheritEquipment(world, *pet, *owner);
}
} // namespace nativepet
