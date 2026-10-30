#include "RegionLevels.h"
#include "NetWorlds.h"
#include "Multiplayer.h"
#include "Spawned.h"
#include "Progression.h"
#include <cstdio>
#include <cstring>

int regionLevelsTests() {
    int failures = 0;
    auto check = [&](bool result, const char *message) {
        if (!result) { std::printf("FAIL region levels: %s\n", message); ++failures; }
    };
    regionlevels::Occupancy regions;
    const worldgen::Int2 a{-3, 7}, b{8, -11};
    regions.update(1, a, 12);
    regions.update(2, a, 80);
    check(regions.basis(a) == 12, "first arrival wins, rather than highest level");
    regions.update(1, a, 20);
    check(regions.basis(a) == 12, "leveling in place keeps the active baseline");
    regions.leave(1);
    check(regions.basis(a) == 12, "another occupant retains the original baseline");
    regions.update(2, b, 80);
    check(regions.snapshot().count({a.x, a.y}) == 0 && regions.basis(b) == 80, "last departure clears old region");
    regions.update(3, a, 31);
    check(regions.basis(a) == 31, "returning region chooses a fresh baseline");
    regions.leave(2);
    regions.leave(2);
    check(regions.snapshot().count({b.x, b.y}) == 0, "disconnect clears the last occupant, once");
    regions.unload(a);
    check(regions.snapshot().empty() && !regions.regionOf(3), "unload removes occupancy and history");
    regions.update(3, a, 42);
    check(regions.basis(a) == 42, "same player can reclaim an unloaded region");
    regions.clear();
    regions.update(0, a, 90);
    regions.update(-1, a, 90);
    check(regions.snapshot().empty(), "unassigned players cannot claim regions");
    regions.update(1, a, -2);
    regions.update(2, b, 2000000);
    check(regions.basis(a) == 1 && regions.basis(b) == 1000000, "levels stay within progression limits");

    regionlevels::Snapshot snapshot{true, regions.snapshot()};
    auto bytes = regionlevels::encode(snapshot);
    auto decoded = regionlevels::decode(bytes);
    check(decoded && decoded->enabled && decoded->levels == snapshot.levels, "signed regions survive snapshot round trip");
    check(decoded && decoded->basis({200, 200}) == 1, "unoccupied regions have no inherited difficulty");
    auto invalid = bytes;
    invalid.pop_back();
    check(!regionlevels::decode(invalid), "truncated snapshot rejected");
    invalid = bytes;
    invalid.push_back(0);
    check(!regionlevels::decode(invalid), "trailing snapshot bytes rejected");
    invalid = bytes;
    invalid[0] = 2;
    check(!regionlevels::decode(invalid), "unknown snapshot version rejected");
    invalid = bytes;
    invalid[4] = 0;
    check(!regionlevels::decode(invalid), "disabled snapshots cannot carry levels");
    invalid = bytes;
    std::memcpy(invalid.data() + 24, invalid.data() + 12, 8);
    check(!regionlevels::decode(invalid), "duplicate region entries rejected");
    check(regionlevels::decode(regionlevels::encode({false, {}})).has_value(), "disabled host can advertise its mode");

    // Refreshing the baseline must not flatten the existing difficulty regions.
    {
        regionlevels::Occupancy difficulty;
        difficulty.update(1, a, 100);
        const uint64_t roll = progression::regionRoll(123, a.x, a.y), entity = 42;
        auto tiers = [&] {
            const int baseline = difficulty.basis(a);
            return std::array<int, 3>{progression::enemyLevel(baseline, 0, roll, entity),
                                      progression::enemyLevel(baseline, 5, roll, entity),
                                      progression::enemyLevel(baseline, 4, roll, entity)};
        };
        const auto first = tiers();
        check(first[0] < first[1] && first[1] < first[2], "easy, moderate and hard regions retain different difficulty");
        difficulty.leave(1);
        difficulty.update(2, a, 150);
        const auto refreshed = tiers();
        check(refreshed[0] > first[0] && refreshed[0] < refreshed[1] && refreshed[1] < refreshed[2],
              "refreshed baseline preserves regional difficulty ordering");
        check(progression::enemyLevel(difficulty.basis(a), 0, 0, entity) <
              progression::enemyLevel(difficulty.basis(a), 0, 8, entity), "region roll still varies enemies within a tier");
    }

    // The host must not claim a default position/level from an initial partial delta.
    SpawnedCreatures creatures;
    ExtractedWorld blocks;
    HostSide host(creatures, blocks);
    regionlevels::Occupancy observed;
    int observations = 0;
    host.playerObserved = [&](const uint8_t *record) {
        int64_t id, position[3];
        int32_t level;
        std::memcpy(&id, record + worldgen::creature::ID, sizeof id);
        std::memcpy(position, record + worldgen::creature::POSITION, sizeof position);
        std::memcpy(&level, record + worldgen::creature::LEVEL, sizeof level);
        observed.update(id, {(int32_t)(position[0] >> 16), (int32_t)(position[1] >> 16)}, level);
        ++observations;
    };
    host.playerLeft = [&](int64_t id) { observed.leave(id); };
    cwnet::CreatureSnapshot record{};
    const int64_t position[3]{-4LL * 65536, 9LL * 65536, 0};
    const int32_t level = 27;
    std::memcpy(record.at(worldgen::creature::POSITION), position, sizeof position);
    std::memcpy(record.at(worldgen::creature::LEVEL), &level, sizeof level);
    auto part = [&](uint64_t mask, bool cut = false) {
        cwnet::CreatureDelta delta;
        delta.id = 4;
        delta.mask = mask;
        for (const auto &field : cwnet::creatureFields())
            if (mask & (1ull << field.bit))
                delta.fields.insert(delta.fields.end(), record.at(field.offset), record.at(field.offset) + field.size);
        delta.cut = cut;
        host.receiveCreature(delta);
    };
    part(1ull << 0);
    part(1ull << 7);
    check(observations == 0, "position and player kind do not substitute for missing level");
    part(1ull << 21, true);
    check(observations == 0, "a cut field is not considered observed");
    part(1ull << 21);
    check(observations == 1 && observed.basis({-4, 9}) == 27, "complete player fields choose the actual baseline");
    host.removeCreature(4);
    check(observed.snapshot().empty(), "host removal releases occupancy");
    // Exercise the actual host/guest wiring over local UDP, including host-only
    // baseline publication. No extracted data, game process or save is needed.
    {
        SpawnedCreatures homeWorld, homePlayer, guestWorld, guestPlayer;
        ExtractedWorld terrain;
        const glm::vec3 firstPosition{20, 30, 100}, nextPosition{8020, 8030, 100};
        const auto firstRegion = worldgen::regionAt(20, 30);
        const auto nextRegion = worldgen::regionAt(8020, 8030);
        Multiplayer home, guest;
        home.setRegionScaling(true);
        guest.setRegionScaling(true);
        homePlayer.setPlayer(terrain, firstPosition, 0, {}, 20, 18, true);
        guestPlayer.setPlayer(terrain, firstPosition, 0, {}, 20, 77, true);
        Multiplayer::Setup setup;
        setup.way = Multiplayer::Way::Address;
        setup.hostWorld = &homeWorld;
        setup.clientWorld = &homePlayer;
        setup.blocks = &terrain;
        std::string error;
        const bool homeOpened = home.start(setup, error);
        check(homeOpened, "local UDP host opens");
        setup.hostWorld = &guestWorld;
        setup.clientWorld = &guestPlayer;
        const bool guestOpened = guest.start(setup, error);
        check(guestOpened, "local UDP guest opens");
        if (homeOpened && guestOpened) {
            uint32_t now = Multiplayer::clockNow();
            auto frames = [&] {
                for (int i = 0; i < 18; ++i) {
                    now += 32;
                    home.beginFrame(now); home.consume(); home.endFrame(now);
                    guest.beginFrame(now); guest.consume(); guest.endFrame(now);
                }
            };
            frames();
            check(home.regionLevel(firstRegion, 99) == 18, "single player uses host arrival level");
            guest.join(home.net()->self());
            frames();
            check(guest.guestElsewhere() && home.guests() == 2, "second player joins the actual host");
            check(home.regionLevel(firstRegion, 99) == 18 && guest.regionLevel(firstRegion, 99) == 18,
                  "host baseline replaces the guest world's previous baseline");
            homePlayer.setPlayer(terrain, nextPosition, 0, {}, 20, 33, true);
            frames();
            check(home.regionLevel(firstRegion, 99) == 18 && guest.regionLevel(firstRegion, 99) == 18,
                  "remaining guest retains the first entrant's baseline");
            guestPlayer.setPlayer(terrain, nextPosition, 0, {}, 20, 77, true);
            frames();
            check(home.regionLevel(firstRegion, 99) == 1 && guest.regionLevel(firstRegion, 99) == 1,
                  "last departure removes the baseline on host and guest");
            check(home.regionLevel(nextRegion, 99) == 33 && guest.regionLevel(nextRegion, 99) == 33,
                  "moving into another occupied region keeps its first entrant");
            guestPlayer.setPlayer(terrain, firstPosition, 0, {}, 20, 77, true);
            frames();
            check(home.regionLevel(firstRegion, 99) == 77 && guest.regionLevel(firstRegion, 99) == 77,
                  "re-entry publishes a new baseline without discovery history");
        }
    }
    std::printf("Region levels: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
