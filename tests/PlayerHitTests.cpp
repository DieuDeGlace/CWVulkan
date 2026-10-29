#include "NativePlayerHit.h"
#include "NativeBuffs.h"
#include "NativeRules.h"
#include "worldgen/Residents.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace {
int failures = 0;
#define CHECK_HIT(condition) do { if (!(condition)) { \
    std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); ++failures; } } while (0)

template<class T> void set(uint8_t *record, size_t offset, T value) { std::memcpy(record + offset, &value, sizeof value); }
template<class T> T get(const uint8_t *record, size_t offset) {
    T value;
    std::memcpy(&value, record + offset, sizeof value);
    return value;
}
bool near(float a, float b) { return std::abs(a - b) < 1e-6f; }
struct Player {
    worldgen::CreatureRecord record = worldgen::newCreature(7);
    nativeai::Creature side;
    Player() {
        side.record = record.data();
        set(record.data(), worldgen::creature::LEVEL, int32_t(1));
        record[worldgen::creature::CLASS] = 1;
        set(record.data(), 0x180, 30.0f);
        set(record.data(), 0x184, .8f);
        set(record.data(), 0x188, .4f);
        set(record.data(), 0x158, .1f);
        set(record.data(), 0x18c, .7f);
        set(record.data(), 0x130, 80.0f);
        set(record.data(), 0x4c, 9.0f);
    }
    float value(size_t offset) const { return get<float>(record.data(), offset); }
};
nativehit::Number hit(uint8_t kind, float damage, int32_t stun = 0) {
    nativehit::Number number{};
    set(number.data(), 8, int64_t(7));
    set(number.data(), 0x10, damage);
    set(number.data(), 0x18, stun);
    set(number.data(), 0x1c, int32_t(9000)); // the local player must ignore this timer
    set(number.data(), 0x38, -2.0f);
    set(number.data(), 0x3c, 3.0f);
    set(number.data(), 0x40, 4.0f);
    number[0x45] = kind;
    return number;
}

void blocked() {
    Player player;
    nativeplayerhit::apply(player.side, hit(1, 0, 5000));
    CHECK_HIT(player.value(0x34) == -2 && player.value(0x38) == 3 && player.value(0x3c) == 4);
    CHECK_HIT(player.value(0x4c) == 9 && player.value(0x130) == 80 && player.value(0x18c) == .7f);
    CHECK_HIT(player.value(0x180) == 30 && player.value(0x188) == .4f);

    player.record[worldgen::creature::ABILITY] = 9;
    player.side.buffs.push_back(nativebuffs::make(0x22, 2, 10000, 9));
    CHECK_HIT(nativerules::blockValue(player.record.data()) == 5);
    nativeplayerhit::apply(player.side, hit(1, 2.5f));
    CHECK_HIT(near(player.value(0x188), .6f) && near(player.value(0x158), .3f));
    CHECK_HIT(near(player.value(0x184), .3f) && player.value(0x994) == 1);

    for (uint8_t ability : {0x48, 0x49, 0x82}) {
        Player exempt;
        exempt.record[worldgen::creature::ABILITY] = ability;
        nativeplayerhit::apply(exempt.side, hit(1, 10000));
        CHECK_HIT(exempt.value(0x184) == .8f);
    }
    Player longGuard;
    longGuard.record[worldgen::creature::ABILITY] = 0x5f;
    nativeplayerhit::apply(longGuard.side, hit(1, 2.5f));
    CHECK_HIT(near(longGuard.value(0x184), .55f) && longGuard.value(0x158) == .1f);
    nativeplayerhit::apply(longGuard.side, hit(1, 10000));
    CHECK_HIT(longGuard.value(0x184) == -1);
}

void shields() {
    Player player;
    player.side.buffs = {nativebuffs::make(6, 5, 3000, 8), nativebuffs::make(6, 20, 5000, 9),
                         nativebuffs::make(2, 7, 1000, 10)};
    const auto result = nativeplayerhit::apply(player.side, hit(5, 7, 5000));
    CHECK_HIT(result.buffNotices.size() == 2 && result.sounds.empty());
    CHECK_HIT(nativebuffs::strength(player.side.buffs[0]) == 0 && nativebuffs::duration(player.side.buffs[0]) == 0);
    CHECK_HIT(nativebuffs::strength(player.side.buffs[1]) == 13 && nativebuffs::duration(player.side.buffs[1]) == 5000);
    CHECK_HIT(nativebuffs::strength(player.side.buffs[2]) == 7);
    for (const auto &notice : result.buffNotices)
        CHECK_HIT(get<int64_t>(notice.data(), 0) == 7 && get<int64_t>(notice.data(), 8) == 7 && notice[0x28] == 0);
    CHECK_HIT(player.value(0x180) == 30 && player.value(0x188) == .4f && player.value(0x130) == 80);
    set(player.record.data(), 0x180, 0.0f);
    nativeplayerhit::apply(player.side, hit(5, -2)); // shield consumption does not require a living target
    CHECK_HIT(nativebuffs::strength(player.side.buffs[1]) == 15);
}

void ordinary() {
    Player ignored;
    nativeplayerhit::apply(ignored.side, hit(6, 100, 5000));
    CHECK_HIT(ignored.value(0x180) == 30 && ignored.value(0x130) == 80 && ignored.value(0x18c) == .7f);
    Player player;
    nativeplayerhit::apply(player.side, hit(2, 5, 500));
    CHECK_HIT(player.value(0x180) == 25 && near(player.value(0x18c), .45f) && player.value(0x130) == 500);
    CHECK_HIT(player.value(0x34) == 0 && player.value(0x4c) == 9); // ordinary push is not applied
    const float maximum = nativerules::maxHealth(player.record.data());
    nativeplayerhit::apply(player.side, hit(3, -10000));
    CHECK_HIT(player.value(0x180) == maximum);
    Player exact;
    CHECK_HIT(nativeplayerhit::apply(exact.side, hit(0, 30)).sounds.empty());
    CHECK_HIT(exact.value(0x180) == 0);
    nativeplayerhit::apply(exact.side, hit(0, -10));
    CHECK_HIT(exact.value(0x180) == 0); // healing does not revive a dead local player
    Player lethal;
    const auto result = nativeplayerhit::apply(lethal.side, hit(0, 31));
    CHECK_HIT(lethal.value(0x180) == 0 && result.sounds.size() == 1);
    if (!result.sounds.empty()) {
        const auto sound = soundrules::readRequest(result.sounds[0]);
        CHECK_HIT(sound.number == 0x1b && sound.owner == 7 && sound.pitch == 1);
    }
    Player other;
    auto number = hit(0, 10);
    set(number.data(), 8, int64_t(8));
    nativeplayerhit::apply(other.side, number);
    CHECK_HIT(other.value(0x180) == 30);
}
} // namespace

int playerHitTests() {
    failures = 0;
    blocked();
    shields();
    ordinary();
    std::puts("local-player received hits: blocking, shields, health and lethal boundaries checked");
    return failures;
}
