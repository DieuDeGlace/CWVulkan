#include "Extracted.h"
#include "NativeAI.h"
#include "NativeRules.h"
#include "NativeWorldClock.h"
#include "NetWorlds.h"
#include "Spawned.h"
#include "worldgen/Generator.h"
#include <array>
#include <bit>
#include <cstdio>
#include <cstring>
#include <limits>

namespace {
float word(uint32_t bits) { return std::bit_cast<float>(bits); }
uint32_t word(float value) { return std::bit_cast<uint32_t>(value); }
template <class T> void write(uint8_t *record, size_t offset, T value) { std::memcpy(record + offset, &value, sizeof value); }
float health(const uint8_t *record) { float value; std::memcpy(&value, record + 0x180, 4); return value; }
struct Clock {
    int32_t day = 7;
    float time = 0, elapsed = 0;
    bool client = false;
    std::vector<int32_t> notices;
    nativeclock::World world{day, time, elapsed, client, true, notices, {}};
    Clock() { world.reset = [] {}; }
};
struct Actor {
    std::array<uint8_t, 0x200> record{}; // clock-only fixture; not passed to other creature routines
    nativeai::Creature side;
    Actor() {
        side.record = record.data();
        record[0x68] = 0x58;
        write(record.data(), 0x180, 100.0f);
    }
};
void arithmetic(const auto &check) {
    Clock c;
    c.time = word(0x4594e767u);
    const float dt = word(0x415299e7u);
    nativeclock::advance(c.world, dt);
    // Literal finite binary32 operands: round product, then round addition. A
    // double product/add rounded only at the end would yield45990469.
    check(word(c.time) == 0x45990468u && c.elapsed == dt && c.day == 7,
          "calendar product/add each round to binary32 while elapsed receives raw milliseconds");
    c.elapsed = 16777216.0f;
    c.time = 0;
    nativeclock::advance(c.world, 1);
    check(c.elapsed == 16777216.0f && c.time == 10,
          "elapsed accumulator retains binary32 rounding independently of the calendar rate");
    c.time = 86400000;
    nativeclock::advance(c.world, 0);
    check(c.day == 7 && c.time == 86400000 && c.notices.empty(), "exact day equality does not wrap");
    std::vector<std::pair<int32_t, float>> seen;
    c.world.reset = [&] {
        check(c.notices.size() == seen.size(), "each notice follows its synchronous reset callback");
        seen.push_back({c.day, c.time});
    };
    c.time = 172800000;
    nativeclock::advance(c.world, 0);
    check(c.day == 8 && c.time == 86400000 && seen == std::vector<std::pair<int32_t, float>>{{8, 86400000.0f}},
          "two exact periods wrap once; callbacks see the updated day and stored remainder");
    c.day = 7; c.time = 259200000; c.notices.clear(); seen.clear();
    nativeclock::advance(c.world, 0);
    check(c.day == 9 && c.time == 86400000 && c.notices == std::vector<int32_t>({0, 0}) &&
          seen == std::vector<std::pair<int32_t, float>>{{8, 172800000.0f}, {9, 86400000.0f}},
          "three periods perform two ordered crossings rather than a single modulo/reset");
    c.day = 7; c.time = 259200000; c.notices.clear(); seen.clear();
    c.world.reset = [&] { seen.push_back({c.day, c.time}); c.client = true; };
    nativeclock::advance(c.world, 0);
    check(c.day == 9 && c.time == 86400000 && seen.size() == 1 && c.notices == std::vector<int32_t>{0},
          "role is read again for the second crossing; day/time advance in either role");
    c.client = false; c.day = 7; c.time = 259200000; c.notices.clear();
    c.world.reset = [&] { c.time = 12; };
    nativeclock::advance(c.world, 0);
    check(c.day == 8 && c.time == 12 && c.notices == std::vector<int32_t>{0}, "crossing loop reloads stored time after reset");
    c.client = true; c.day = INT32_MAX; c.time = 86400008;
    nativeclock::advance(c.world, 0);
    check(c.day == INT32_MIN && c.time == 8, "day increment retains the low32 bits without signed overflow");
    c.time = -10; c.day = 4;
    nativeclock::advance(c.world, 0);
    check(c.time == -10 && c.day == 4, "negative time has no invented reverse-day branch");
}
void actors(const auto &check) {
    Clock c;
    Actor local, other;
    std::array<nativeai::Creature *, 2> entries{&local.side, &other.side};
    c.world.actors = entries;
    c.world.local = local.record.data();
    unsigned references = 0;
    c.world.maxHealth = [&] { ++references; return 100.0f; };
    write(local.record.data(), 0x180, 50.0f);
    nativeclock::advance(c.world, 16);
    check(c.time == 1600 && c.elapsed == 16 && references == 2 && health(local.record.data()) == 50.08f &&
          health(other.record.data()) == 100,
          "multiple qualifiers select100x once; only the local pointer heals with two fresh reference calls");
    for (int invalid = 0; invalid < 5; ++invalid) {
        c.world.actors = std::span(entries).first(1);
        local.record[0x60] = invalid == 0 ? 1 : 0;
        local.record[0x68] = invalid == 1 ? 0x57 : 0x58;
        write(local.record.data(), 0x180, invalid == 2 ? 0.0f : invalid == 3 ? -1.0f :
                                              invalid == 4 ? std::numeric_limits<float>::quiet_NaN() : 100.0f);
        c.time = 0; references = 0;
        nativeclock::advance(c.world, 1);
        check(c.time == 10 && references == 0, "kind/ability/ordered-positive-HP predicate rejects each invalid actor");
    }
    local.record[0x60] = 0; local.record[0x68] = 0x58;
    write(local.record.data(), 0x180, 90.0f);
    const std::array<float, 3> changing{100, 10, 77};
    references = 0;
    c.world.maxHealth = [&] { return changing.at(references++); };
    nativeclock::advance(c.world, 16);
    check(health(local.record.data()) == 77 && references == 3,
          "clamp compares a fresh second reference and stores a fresh third reference");
    references = 0;
    write(local.record.data(), 0x180, 100.0f);
    c.world.maxHealth = [&] { ++references; return 100.0f; };
    nativeclock::advance(c.world, 0);
    check(health(local.record.data()) == 100 && references == 2, "zero-input equality skips the third clamp reference");
    write(local.record.data(), 0x180, 101.0f);
    references = 0;
    nativeclock::advance(c.world, 0);
    check(health(local.record.data()) == 100 && references == 3, "zero input can still clamp local health");
    const float dt = word(0x417fc6e3u);
    write(local.record.data(), 0x180, word(0x3d83872bu));
    c.world.maxHealth = [] { return word(0x43523030u); };
    nativeclock::advance(c.world, dt);
    check(word(health(local.record.data())) == 0x3e6dccd8u,
          "healing preserves step*.001, then*.05, then reference*product; rearranging would yield3e6dccd6");
    c.world.named = false;
    c.time = 259200000; c.day = 4; c.elapsed = 0; c.notices.clear();
    write(local.record.data(), 0x180, 50.0f);
    c.world.maxHealth = [] { return 100.0f; };
    nativeclock::advance(c.world, 16);
    check(c.time == 32400000 && c.day == 4 && c.elapsed == 16 && c.notices.empty() && health(local.record.data()) == 50.08f,
          "empty name assigns9am after arithmetic without undoing elapsed/healing or crossing out-of-range input");
    c.time = 0;
    nativeclock::advance(c.world, 0);
    check(c.time == 32400000, "empty-name assignment also runs on a final zero-input call");
    c.world.named = true; c.world.local = nullptr; c.time = 0; references = 0;
    c.world.maxHealth = [&] { ++references; return 100.0f; };
    nativeclock::advance(c.world, 1);
    check(c.time == 100 && references == 0, "a nonlocal qualifying actor accelerates without asking for local max health");
}
void integration(const auto &check) {
    ExtractedWorld blocks;
    SpawnedCreatures viewed;
    viewed.setRemoteClient(true);
    viewed.setClock(7, 500);
    viewed.setClockRate(10);
    viewed.simulate(blocks, 1.25);
    check(viewed.timeOfDay() == 512.5f && viewed.elapsedWorldTime() == 1.25f,
          "viewed World substeps advance their calendar independently between clock packets");
    viewed.markDead(99);
    viewed.setClock(7, 86400000);
    viewed.advanceClock(1);
    check(viewed.day() == 8 && viewed.timeOfDay() == 10,
          "strict crossing updates day and remainder even for a viewed World");
    check(viewed.isWorldDead(99), "role-nonzero crossing preserves runtime death membership");
    check(viewed.events().sleeps.empty(), "role-nonzero crossing emits no authority reset notice");
    viewed.setNativeClock(true);
    viewed.setClock(4, 36000000);
    const float priorElapsed = viewed.elapsedWorldTime();
    viewed.simulate(blocks, 16.25);
    check(viewed.day() == 4 && viewed.timeOfDay() == 36000164 && viewed.elapsedWorldTime() == priorElapsed + 16.25f,
          "actual viewed16ms/fractional substeps use separately rounded native calendar stores");
    MirrorSide mirror(viewed, blocks);
    mirror.setClock(5, 200);
    viewed.simulate(blocks, 0.25);
    check(viewed.day() == 5 && viewed.timeOfDay() == 202.5f, "packet8 assignment does not disable later viewed clock stepping");
    const int32_t generatorDay = worldgen::worldDay();
    viewed.setClock(5, 86400000); viewed.markDead(99);
    viewed.simulate(blocks, 1);
    check(viewed.day() == 6 && viewed.timeOfDay() == 8 && viewed.isWorldDead(99) && viewed.events().sleeps.empty() &&
          worldgen::worldDay() == generatorDay, "native viewed crossing preserves authority state and generator cache ownership");
    viewed.setNativeClock(false); viewed.setClock(6, 0); viewed.simulate(blocks, 0);
    check(viewed.timeOfDay() == 32400000 && viewed.day() == 6, "actual unnamed World uses the zero-step9am fallback");
    viewed.setClockRate(0); viewed.setClock(6, 321); viewed.simulate(blocks, 16);
    check(viewed.timeOfDay() == 321, "explicit custom zero-speed freezes calendar while elapsed World time still advances");
    SpawnedCreatures host;
    host.setNativeClock(true);
    host.setEnemyRespawn({enemyrespawn::Mode::Permanent, 300});
    host.setClock(10, 36000000);
    HostSide authority(host, blocks);
    authority.advance(17);
    check(host.timeOfDay() == 36000168 && host.elapsedWorldTime() == 17,
          "actual integer Host partition ticks once per16+1 step, without the old extra clock advance");
    authority.advance(2000);
    check(host.elapsedWorldTime() == 1017, "Host cap discards excess wall milliseconds without a catch-up tick");
    bool made = false;
    uint8_t *npc = host.networkCreature(7002, made);
    npc[0x60] = 3; write(npc, 0x180, 100.0f);
    host.markDead(99); host.setClock(10, 86400000);
    unsigned laterResets = 0;
    authority.reset = [&] { ++laterResets; check(host.day() == 11 && host.timeOfDay() == 8, "later reset sees the crossed clock"); };
    authority.advance(1);
    check(host.day() == 11 && host.timeOfDay() == 8 && !host.record(7002) && !host.isWorldDead(99) && laterResets == 1 &&
          host.events().sleeps == std::vector<int32_t>{0},
          "authority clock cleanup rebuilds borrowed actor views, followed by the separate later notice reset");
    worldgen::setWorldDay(generatorDay); worldgen::setRefreshDay(generatorDay);
    SpawnedCreatures local;
    local.setNativeClock(true); local.setNativePlayer(true);
    local.setPlayer(blocks, {0, 0, 10}, 0, {}, 50, 1, true);
    auto *p = local.localPlayer();
    check(p && p->record, "actual local-player clock fixture is registered");
    if (p && p->record) {
        p->record[0x68] = 0x58;
        local.setClock(3, 0);
        unsigned referenceCalls = 0, atAdmission = 0;
        float admissionHealth = 0, admissionTime = 0;
        bool clockPhase = true;
        nativerules::Callbacks hooks;
        hooks.maxHealth = [&](const uint8_t *record, float &value) {
            if (clockPhase && record == p->record) { ++referenceCalls; value = 200; }
        };
        const int hook = nativerules::addCallbacks(std::move(hooks));
        local.simulate(blocks, 16, {}, [&] {
            atAdmission = referenceCalls; admissionHealth = local.playerHealth(); admissionTime = local.timeOfDay(); clockPhase = false;
        });
        nativerules::removeCallbacks(hook);
        check(atAdmission == 2 && admissionHealth == 50.16f && admissionTime == 1600,
              "real local player uses fresh owned stat-provider calls and100x clock before resident/body phases");
    }
}
} // namespace
int worldClockTests() {
    int failures = 0;
    const auto check = [&](bool okay, const char *message) {
        if (!okay) { std::printf("FAIL World clock: %s\n", message); ++failures; }
    };
    arithmetic(check); actors(check); integration(check);
    std::printf("World clock: float arithmetic, predicates, healing, crossings and Host/viewed substeps: %d failure(s)\n", failures);
    return failures;
}
