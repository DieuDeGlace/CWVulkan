#include "CharacterCreation.h"
#include "DialogueWords.h"
#include "Frontend.h"
#include "NativeRules.h"
#include <cstring>

namespace charactercreation {
namespace {
template <class T> T read(const worldgen::CreatureRecord &record, size_t at) {
    T value;
    std::memcpy(&value, record.data() + at, sizeof value);
    return value;
}
template <class T> void put(worldgen::CreatureRecord &record, size_t at, const T &value) {
    std::memcpy(record.data() + at, &value, sizeof value);
}
float maximum(const worldgen::CreatureRecord &record, const nativeai::Creature &side) {
    std::vector<nativerules::Buff> buffs;
    buffs.reserve(side.buffs.size());
    for (const auto &buff : side.buffs) {
        float strength;
        std::memcpy(&strength, buff.data() + 4, 4);
        buffs.push_back({buff[0], strength});
    }
    return nativerules::maxHealth(record.data(), buffs.data(), buffs.size());
}
} // namespace
void rebuild(worldgen::CreatureRecord &record, nativeai::Creature &side, nativesaves::Character &data,
             nativesaves::Looks looks, uint8_t characterClass) {
    // Style writes only its defined CharacterData header fields.
    std::memcpy(data.looks.bytes.data(), looks.bytes.data(), 5);
    std::memcpy(data.looks.bytes.data() + 8, looks.bytes.data() + 8, 11);
    data.classType = characterClass;
    record[0x14c] = characterClass;
    put(record, 0x64, nativesaves::raceOf(looks.people(), looks.sex()));
    static const auto defaults = worldgen::newCreature(0);
    characterstate::copyAppearance(record.data() + 0x78, defaults.data() + 0x78);
    std::array<uint8_t, 0xb0> appearance;
    if (nativeprogress::appearanceOf(data.looks, appearance))
        characterstate::copyAppearance(record.data() + 0x78, appearance.data());
    const nativesaves::Cell region{read<int32_t>(record, 0x150), read<int32_t>(record, 0x154)};
    frontend::giveStartingKit(data, read<uint32_t>(record, 0x968), region);
    for (size_t slot = 0; slot < data.equipment.size(); ++slot)
        characterstate::copyItem(record.data() + 0x278 + slot * nativesaves::ITEM_SIZE, data.equipment[slot].data());
    put(record, 0xa08, int32_t(0));
    const nativesaves::Item empty{};
    characterstate::copyItem(record.data() + 0xa0c, empty.data());
    put(record, 0xaac, int32_t(0));
    put(record, 0xab0, int32_t(0));
    if (!side.inventorySource) {
        side.inventory.resize(data.inventory.size());
        for (size_t tab = 0; tab < data.inventory.size(); ++tab) {
            side.inventory[tab].resize(data.inventory[tab].size());
            for (size_t slot = 0; slot < data.inventory[tab].size(); ++slot) {
                const auto &stack = data.inventory[tab][slot];
                auto &destination = side.inventory[tab][slot];
                std::memcpy(destination.data(), &stack.count, 4);
                characterstate::copyItem(destination.data() + 4, stack.item.data());
            }
        }
    }
    put(record, 0x180, maximum(record, side));
    characterstate::capture(record.data(), side, data);
}
void begin(worldgen::CreatureRecord &record, nativeai::Creature &side, nativesaves::Character &data,
           const FindHome &home) {
    characterstate::reset(record, side);
    record[0x958] = record[0x60] = 0;
    std::memset(record.data() + 0x28, 0, 24);
    put(record, 0x5c, uint32_t(0));
    put(record, 0x144, int32_t(1));
    put(record, 0x180, maximum(record, side));
    for (size_t at : {size_t(0x6d8), size_t(0x638)}) {
        record[at] = 3;
        put(record, at + 4, int32_t(0));
        record[at + 0x1c] = 1;
    }
    record[0x3d4] = 1;
    record[0x60] = 0;
    record[0x14c] = 1;
    const Placement place = home(); // original retained CharacterData survives through this search
    std::memcpy(record.data() + 0x1d0, place.home.data(), 8);
    std::memcpy(record.data() + 0x10, place.position.data(), 24);
    data = nativesaves::Character{}; // fresh owned CharacterData, after placement
    data.looks.set(0, 0, 0, 0, {255, 255, 255});
    rebuild(record, side, data, data.looks, 1); // immediate reset cleared the seed to zero
}
void publishAccount(worldgen::CreatureRecord &record, const AccountProvider &provider) {
    if (read<uint64_t>(record, 0x968) == 0 && provider) {
        const uint64_t account = provider();
        put(record, 0x968, account); // no second test after the provider returns
    }
}
std::string nameBytes(std::string_view utf8) {
    std::string out;
    const auto append = [&](uint16_t unit) {
        const char byte = char(unit & 0xff);
        if (!byte || out.size() == 15)
            return false;
        out.push_back(byte);
        return true;
    };
    for (wchar_t unit : dialoguewords::wide(std::string(utf8))) {
        const uint32_t value = uint32_t(unit);
        if (value > 0xffff) {
            const uint32_t scalar = value - 0x10000;
            if (!append(uint16_t(0xd800 + (scalar >> 10))) || !append(uint16_t(0xdc00 + (scalar & 0x3ff))))
                break;
        } else if (!append(uint16_t(value)))
            break;
    }
    return out;
}
void confirm(worldgen::CreatureRecord &record, nativeai::Creature &side, nativesaves::Character &data,
             std::string_view name) {
    const std::string bytes = nameBytes(name);
    std::memcpy(record.data() + 0x958, bytes.c_str(), bytes.size() + 1);
    put(record, 0x144, int32_t(1));
    nativesaves::Cell home;
    std::memcpy(home.data(), record.data() + 0x1d0, 8);
    data.usedCells.insert(home);
    characterstate::capture(record.data(), side, data);
}
SelectionSnapshot selectionSnapshot(const worldgen::CreatureRecord &record, const nativeai::Creature &side,
                                    const nativesaves::Character &data) {
    SelectionSnapshot out;
    out.record = worldgen::newCreature(0);
    characterstate::copyCore(out.record.data(), record.data());
    std::memcpy(out.record.data() + 0xb10, out.record.data() + 0x10, 24);
    auto &copy = out.character;
    characterstate::capture(out.record.data(), side, copy);
    copy.inventory = data.inventory;
    copy.gold = data.gold;
    copy.fieldAb0 = data.fieldAb0;
    std::memcpy(copy.looks.bytes.data(), data.looks.bytes.data(), 5);
    std::memcpy(copy.looks.bytes.data() + 8, data.looks.bytes.data() + 8, 11);
    copy.recipes = data.recipes;
    // Remaining CharacterData and constructor-only9A4/9A8 retain their own defaults.
    return out;
}
} // namespace charactercreation
