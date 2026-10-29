#include "NativePlayerHit.h"
#include "NativeBuffs.h"
#include "NativeRules.h"
#include "worldgen/Residents.h"

#include <algorithm>
#include <cstring>

namespace nativeplayerhit {
namespace {
constexpr size_t VELOCITY = 0x34, SECONDARY_MANA = 0x158, HEALTH = 0x180;
constexpr size_t BLOCK_POWER = 0x184, MANA = 0x188, STEALTH = 0x18c, STUN = 0x130, FLASH = 0x994;
template<class T> T read(const uint8_t *record, size_t offset) {
    T value;
    std::memcpy(&value, record + offset, sizeof value);
    return value;
}
template<class T> void write(uint8_t *record, size_t offset, T value) {
    std::memcpy(record + offset, &value, sizeof value);
}
} // namespace

Result apply(nativeai::Creature &player, const nativehit::Number &number) {
    Result result;
    uint8_t *record = player.record;
    if (!record || read<int64_t>(record, worldgen::creature::ID) != read<int64_t>(number.data(), 8))
        return result;
    const float damage = read<float>(number.data(), 0x10);
    const uint8_t kind = number[0x45], ability = record[worldgen::creature::ABILITY];
    const auto stats = nativebuffs::forStats(player);
    if (kind == 1) { // a block changes ordinary velocity, even for zero damage
        std::memcpy(record + VELOCITY, number.data() + 0x38, 12);
        if (damage > 0)
            write(record, FLASH, 1.0f);
        float cost = damage / nativerules::blockValue(record);
        if (ability == 0x48 || ability == 0x49 || ability == 0x82)
            cost = 0;
        if (ability == 0x5f)
            cost *= .5f;
        const float gain = std::max((1.0f - cost) * .5f, 0.0f);
        if (nativerules::isGuarding(record, [&](int type) { return nativebuffs::find(player, (uint8_t)type) != nullptr; })) {
            const float amount = nativerules::manaFactor(stats.data(), stats.size()) * gain;
            write(record, MANA, read<float>(record, MANA) + amount);
            if (ability != 0x5f)
                write(record, SECONDARY_MANA, read<float>(record, SECONDARY_MANA) + amount);
        }
        const float mana = std::min(read<float>(record, MANA), 1.0f);
        write(record, MANA, mana);
        if (read<float>(record, SECONDARY_MANA) > mana)
            write(record, SECONDARY_MANA, mana);
        write(record, BLOCK_POWER, std::max(read<float>(record, BLOCK_POWER) - cost, -1.0f));
    } else if (kind == 5) { // each shield is drained and announces its resulting buff
        const int64_t id = read<int64_t>(record, worldgen::creature::ID);
        for (auto &buff : player.buffs)
            if (buff[0] == 6) {
                const float strength = nativebuffs::strength(buff) - damage;
                write(buff.data(), 4, strength <= 0 ? 0.0f : strength);
                if (strength <= 0)
                    write(buff.data(), 8, 0.0f);
                result.buffNotices.push_back(nativebuffs::notice(id, id, buff, 0));
            }
    } else if (read<float>(record, HEALTH) > 0 && kind != 6) {
        if (damage > 0) {
            write(record, STEALTH, std::max(read<float>(record, STEALTH) - .25f, 0.0f));
            write(record, FLASH, 1.0f);
        }
        const int32_t stun = read<int32_t>(number.data(), 0x18);
        if (stun > 0)
            write(record, STUN, std::max(read<float>(record, STUN), (float)stun));
        float health = read<float>(record, HEALTH) - damage;
        if (health < 0) {
            health = 0;
            soundrules::Request sound;
            std::memcpy(sound.position, record + worldgen::creature::POSITION, sizeof sound.position);
            sound.number = 0x1b;
            sound.owner = read<int64_t>(record, worldgen::creature::ID);
            result.sounds.push_back(soundrules::makeRequest(sound));
        }
        write(record, HEALTH, std::min(health, nativerules::maxHealth(record, stats.data(), stats.size())));
    }
    return result;
}
} // namespace nativeplayerhit
