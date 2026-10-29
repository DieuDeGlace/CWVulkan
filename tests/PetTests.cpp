#include "NativePet.h"
#include "worldgen/Residents.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <list>

namespace {
template<class T> void put(uint8_t *c, size_t offset, T value) { std::memcpy(c + offset, &value, sizeof value); }
template<class T> T get(const uint8_t *c, size_t offset) {
    T value;
    std::memcpy(&value, c + offset, sizeof value);
    return value;
}
struct Entry {
    worldgen::CreatureRecord record;
    nativeai::Creature side;
    explicit Entry(int64_t id) : record(worldgen::newCreature(id)) { side.record = record.data(); }
};
struct Fixture {
    std::list<Entry> entries;
    nativepet::World world;
    int setups = 0, births = 0;
    Entry &add(int64_t id) {
        entries.emplace_back(id);
        world.creatures.push_back(&entries.back().side);
        return entries.back();
    }
    Fixture() {
        world.construct = [&](int64_t id) {
            ++births;
            entries.emplace_back(id);
            return &entries.back().side;
        };
        world.setup = [&](int64_t, nativeai::Creature *) { ++setups; };
        world.maximumHealth = [](const nativeai::Creature &c) {
            return get<int32_t>(c.record, 0x144) * 100.0f + c.record[0x3b8 + 0x14] * 10.0f;
        };
    }
    nativeai::Creature *find(int64_t id) {
        for (auto &entry : entries)
            if (get<int64_t>(entry.record.data(), 8) == id)
                return &entry.side;
        return nullptr;
    }
    void equip(Entry &owner) {
        owner.record[0x60] = 0;
        owner.record[0x8b8] = 20;
        put(owner.record.data(), 0x8bc, int32_t(12));
        put(owner.record.data(), 0x144, int32_t(7));
        world.local = &owner.side;
    }
};
} // namespace

int petTests() {
    int failures = 0;
    auto check = [&](bool pass, const char *what) {
        if (!pass) { std::printf("FAIL pets: %s\n", what); ++failures; }
    };
    Fixture f;
    Entry &owner = f.add(1);
    f.equip(owner);
    put(owner.record.data(), 0x10, int64_t(123456));
    nativepet::step(f.world);
    const int64_t petId = get<int64_t>(owner.record.data(), 0x9d8);
    auto *pet = f.find(petId);
    check(pet && petId < 0 && f.births == 1 && f.setups == 1, "equipped item creates one pet");
    if (!pet)
        return failures;
    check(pet->record[0x60] == 6 && get<int32_t>(pet->record, 0x144) == 1, "creation leaves level one");
    check(get<int64_t>(pet->record, 0x10) == 123456 && get<float>(pet->record, 0x180) == 100, "creation position and pre-setup health");
    check(get<int32_t>(owner.record.data(), 0x8d0) == 1, "local item field floor");
    nativepet::step(f.world);
    check(get<int32_t>(pet->record, 0x144) == 7 && get<float>(pet->record, 0x180) == 700, "subsequent pass scales health with level");
    owner.record[0x955] = 3;
    owner.record[0x8d8] = 'P', owner.record[0x8dc] = 'i', owner.record[0x8e0] = 'p';
    nativepet::step(f.world);
    check(std::strcmp(reinterpret_cast<char *>(pet->record + 0x958), "Pip") == 0 && f.setups == 1,
          "same-race name change does not reroll gear");
    put(owner.record.data(), 0x8bc, int32_t(13));
    nativepet::step(f.world);
    check(get<int32_t>(pet->record, 0x64) == 13 && f.setups == 2, "race change runs setup");
    owner.side.damage[9] = 12;
    put(pet->record, 0x180, 0.0f);
    put(owner.record.data(), 0x9d8, int64_t(0));
    nativepet::step(f.world);
    check(f.births == 1 && owner.side.damage.empty() && get<int64_t>(owner.record.data(), 0x9d8) == petId,
          "dead owned pet repairs link without duplication and clears damage map");
    pet->buffs.push_back(nativebuffs::make(7, 1, 1000, 1));
    owner.record[0x8b8] = 0;
    put(pet->record, 0x180, 50.0f);
    nativepet::step(f.world);
    check(get<int64_t>(owner.record.data(), 0x9d8) == petId && get<float>(pet->record, 0x180) == 50,
          "buff seven preserves unequipped companion");
    pet->buffs.clear();
    nativepet::step(f.world);
    check(get<int64_t>(owner.record.data(), 0x9d8) == 0 && get<float>(pet->record, 0x180) == 0 && pet->record[0x60] == 3,
          "unequipped stale companion detaches");

    Fixture mirror;
    Entry &remoteOwner = mirror.add(5);
    mirror.equip(remoteOwner);
    mirror.world.client = true;
    nativepet::step(mirror.world);
    check(mirror.births == 0, "mirror never constructs pets");
    Entry &remotePet = mirror.add(6);
    put(remotePet.record.data(), 0x1a0, int64_t(5));
    nativepet::step(mirror.world);
    check(get<int64_t>(remoteOwner.record.data(), 0x9d8) == 6 && mirror.setups == 0, "mirror repairs link only");

    Fixture glide;
    Entry &pilot = glide.add(20);
    glide.equip(pilot);
    nativepet::step(glide.world);
    auto *companion = glide.find(get<int64_t>(pilot.record.data(), 0x9d8));
    if (companion) {
        pilot.record[0x68] = 0x78;
        put(pilot.record.data(), 0xbb4, 3000.0f);
        nativepet::step(glide.world);
        check(get<int64_t>(companion->record, 0x1a0) == 20, "three-second glide boundary stays attached");
        put(pilot.record.data(), 0xbb4, 3000.1f);
        nativepet::step(glide.world);
        check(get<int64_t>(companion->record, 0x1a0) == 0 && companion->record[0x60] == 3 &&
              get<float>(companion->record, 0x6c) == 5000, "long glide detaches with native transient reset");
    }
    Fixture equipment;
    Entry &wearer = equipment.add(30);
    equipment.equip(wearer);
    wearer.record[0x3b8] = 6;
    wearer.record[0x3cc] = 4;
    put(wearer.record.data(), 0x3c0, uint32_t(1)); // Not region-adapting.
    put(wearer.record.data(), 0x3c4, int32_t(5));
    put(wearer.record.data(), 0x3c8, int32_t(6));
    put(wearer.record.data(), 0x150, int32_t(5));
    put(wearer.record.data(), 0x154, int32_t(6));
    nativepet::step(equipment.world);
    auto *equippedPet = equipment.find(get<int64_t>(wearer.record.data(), 0x9d8));
    if (equippedPet) {
        put(equippedPet->record, 0x180, 70.0f);
        equippedPet->record[0x64c] = 9;
        put(equippedPet->record, 0x644, int32_t(-2));
        wearer.side.buffs.push_back(nativebuffs::make(0x13, 1, 1000, 99));
        nativepet::step(equipment.world);
        check(get<float>(equippedPet->record, 0x180) == 370, "level and equipment changes preserve health fraction");
        check(equippedPet->record[0x3cc] == 4 && get<int32_t>(equippedPet->record, 0x3c4) == 5,
              "chest rarity and region inherited");
        check(equippedPet->record[0x64c] == 0 && get<int32_t>(equippedPet->record, 0x644) == -2,
              "empty owner weapon clears rarity without replacing region");
        check(equippedPet->buffs.size() == 1 && equipment.world.buffNotes.size() == 1, "owner buff copied with a notice");
        nativepet::step(equipment.world);
        check(equipment.world.buffNotes.size() == 1, "existing copied buff does not emit another notice");
    }
    Fixture blocked;
    Entry &traveler = blocked.add(40);
    blocked.equip(traveler);
    for (uint8_t ability : {0x78, 0x79, 0x98, 0x9c, 0x9d}) {
        traveler.record[0x68] = ability;
        nativepet::step(blocked.world);
    }
    check(blocked.births == 0, "travel abilities suppress creation");
    Entry transient(50);
    put(transient.record.data(), 0x10, int64_t(987));
    put(transient.record.data(), 0x180, 42.0f);
    transient.record[0x1548] = 0x5a;
    transient.side.buffs.push_back(nativebuffs::make(7, 1, 1000, 1));
    nativepet::resetTransient(transient.side);
    check(get<int64_t>(transient.record.data(), 8) == 50 && get<int64_t>(transient.record.data(), 0x10) == 987 &&
          get<float>(transient.record.data(), 0x180) == 42 && transient.record[0x1548] == 0x5a,
          "transient reset preserves identity, position, health and cached item");
    check(transient.side.buffs.empty() && get<float>(transient.record.data(), 0x130) == -3000 &&
          get<float>(transient.record.data(), 0x9a0) == 1, "transient reset clears buffs and restores timers/stamina");
    std::puts("pets: creation, caching, naming, level, unequip, mirror and glide rules checked");
    return failures;
}
