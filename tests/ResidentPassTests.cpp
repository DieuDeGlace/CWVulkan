// Asset-free contracts from steam-authoritative-residents.md and its cited numeric rules.
#include "ResidentPass.h"
#include "GameMath.h"
#include "worldgen/Generator.h"
#include "worldgen/Residents.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <map>

namespace {
using Record = std::array<uint8_t, 0x1600>;
template <class T> void put(uint8_t *record, size_t offset, const T &value) {
    std::memcpy(record + offset, &value, sizeof value);
}
template <class T> T get(const uint8_t *record, size_t offset) {
    T value;
    std::memcpy(&value, record + offset, sizeof value);
    return value;
}
Record actor(int64_t id, uint8_t kind, uint16_t state = 0, residentpass::Region region = {}) {
    Record result{}; // Zero HP deliberately exercises the HP-independent collections.
    put(result.data(), 8, id);
    result[0x60] = kind;
    put(result.data(), 0x128, state);
    put(result.data(), 0x150, region);
    return result;
}
} // namespace
int residentPassTests() {
    int failures = 0;
    auto check = [&](bool pass, const char *message) {
        if (!pass) {
            ++failures;
            std::printf("FAIL Resident pass: %s\n", message);
        }
    };
    {
        auto monster = actor(9, 1, 0x8800, {99, 99});
        auto guest = actor(42, 0, 0x8800, {3, -4});
        auto local = actor(1, 0);
        residentpass::Collection collection;
        for (const auto *record : {monster.data(), guest.data(), local.data()})
            collection.collect(record);
        check(collection.players() == std::vector<const uint8_t *>({guest.data(), local.data()}),
              "dead kind0 actors retain registration order without local priority");
        check(collection.contains(9) && collection.contains(42) && collection.contains(1),
              "all nonnull registered IDs suppress construction without an HP test");
        put(guest.data(), 0x150, residentpass::Region{8, 8});
        put(guest.data(), 0x128, uint16_t(0));
        check(collection.portalCleared({3, -4}) && !collection.portalCleared({8, 8}) && !collection.portalCleared({99, 99}),
              "region permissions are snapshotted only from kind0 membership");
        for (uint8_t kind : {uint8_t(0), uint8_t(1), uint8_t(7)}) {
            auto current = actor(100, kind, 0, {3, -4});
            put(current.data(), 0x80, uint32_t(0x400000 | 0x8000 | 0x20));
            Record expected = current;
            put(expected.data(), 0x80, uint32_t(0x8000 | 0x20));
            collection.releasePetrified(current.data());
            check(current == expected, "final flag pass admits dead/new actors of any kind and changes only its own bit");
        }
        auto elsewhere = actor(101, 3, 0, {8, 8});
        put(elsewhere.data(), 0x80, uint32_t(0x400000));
        collection.releasePetrified(elsewhere.data());
        check(get<uint32_t>(elsewhere.data(), 0x80) == 0x400000, "later region changes do not expand the permission snapshot");
    }
    {
        auto player = actor(1, 0);
        residentpass::Collection collection;
        collection.collect(player.data());
        constexpr int64_t edge = 256 * 65536;
        check(collection.withinRange(edge, 0, 0) && collection.withinRange(0, 0, -edge), "256-block boundary is inclusive in XYZ");
        check(!collection.withinRange(edge + 1, 0, 0) && !collection.withinRange(0, 0, edge + 1) && !collection.withinRange(edge, 65536, 0),
              "one fixed unit and diagonal excess are rejected");
        put(player.data(), 0x10, int64_t(300 * 65536));
        check(collection.withinRange(300 * 65536, 0, 0) && !collection.withinRange(0, 0, 0),
              "range reads current physical position rather than the collection-time tuple");
        std::map<int64_t, const uint8_t *> current{{1, player.data()}};
        residentpass::Collection moving([&](int64_t id) { return current.at(id); });
        moving.collect(player.data());
        auto replacement = player;
        put(replacement.data(), 0x10, int64_t(1000 * 65536));
        current[1] = replacement.data();
        check(moving.withinRange(1000 * 65536, 0, 0) && !moving.withinRange(300 * 65536, 0, 0),
              "stable actor resolution survives engine record-storage moves");
        worldgen::ResidentWorld world;
        world.alive = [&](int64_t id) { return collection.contains(id); };
        int deadQueries = 0, rangeQueries = 0;
        world.dead = [&](int64_t id) {
            ++deadQueries;
            return id == 2;
        };
        world.nearPlayer = [&](int64_t x, int64_t y, int64_t z) {
            ++rangeQueries;
            return collection.withinRange(x, y, z);
        };
        worldgen::SpawnPoint point;
        point.id = 1;
        check(!worldgen::residentReady(point, 0, 0, world) && deadQueries == 0 && rangeQueries == 0,
              "nonnull ID admission precedes dead-set and range queries");
        point.id = 2;
        check(!worldgen::residentReady(point, 0, 0, world) && rangeQueries == 0, "dead-set admission precedes player range");
        point.id = 3;
        point.x = 300 * 65536;
        check(worldgen::residentReady(point, 0, 0, world), "live-position resolver admits a new nearby ID");
        collection.added(3);
        check(!worldgen::residentReady(point, 0, 0, world), "a newly registered stored ID suppresses later points in the same pass");
        residentpass::Collection empty;
        check(!empty.withinRange(0, 0, 0), "a streaming stand-in is not a native player");
    }
    {
        using Player = worldgen::ResidentWorld::Player;
        const auto zones = worldgen::residentZones({Player{63 * 65536 + 32768, -32768, 0}});
        check(zones.size() == 81 && zones.front() == worldgen::Int2{-4, -5} && zones.back() == worldgen::Int2{4, 3},
              "fractional negative endpoints select inclusive9x9 zone keys");
        const auto positive = worldgen::residentZones({Player{int64_t(1) << 48, 0, 0}});
        const auto negative = worldgen::residentZones({Player{-(int64_t(1) << 48), 0, 0}});
        check(positive.size() == 81 && positive.front().x == -4 && positive.back().x == 4,
              "positive extreme endpoints narrow to32 bits before zoning");
        check(negative.size() == 81 && negative.front().x == -5 && negative.back().x == 3,
              "negative extreme endpoints retain the native narrowing-before-exactness quirk");
    }
    {
        worldgen::SpawnPoint point;
        point.id = 7;
        point.x = 19 * 65536 + 32768;
        point.y = -32768;
        point.z = 100 * 65536;
        point.style = -19088744; // Low32 bits fedcba98 must be zero-extended to64 bits.
        point.mode = 0x8a;
        point.appearance = worldgen::Appearance::constructed();
        point.appearance.setFlags(0x04000000 | 0x8000);
        point.appearance.set<float>(worldgen::Appearance::SCALE, 2);
        point.appearance.set<float>(worldgen::Appearance::SCALE + 4, 3);
        point.appearance.set<float>(worldgen::Appearance::SCALE + 8, 4);
        int draws = 0, hpCalls = 0;
        auto next = [&] {
            ++draws;
            return 49;
        };
        auto hp = [&](const uint8_t *record) {
            ++hpCalls;
            check(get<float>(record, 0x84) == 2 && record[0x68] == 0, "HP is computed before mode copy and demon size postprocessing");
            return 123.0f;
        };
        const auto made = worldgen::makeResident(point, worldgen::Draw(next), false, hp);
        const auto *record = made.record.data();
        check(get<int32_t>(record, worldgen::creature::HOME_CELL) == 1 && get<int32_t>(record, worldgen::creature::HOME_CELL + 4) == -1,
              "home cells floor into16-block cells");
        check(get<uint64_t>(record, worldgen::creature::STYLE) == 0xfedcba98ull,
              "resident style copies an unsigned32-bit value into64 bits");
        const auto region = worldgen::regionAt(19, 0);
        check(get<int32_t>(record, 0x150) == region.x && get<int32_t>(record, 0x154) == region.y,
              "region lookup truncates negative fractional copied position toward zero");
        check(draws == 1 && hpCalls == 1 && get<int64_t>(record, 0x20) == 229 * 65536 &&
                  get<int64_t>(record, worldgen::creature::HOME + 16) == 100 * 65536,
              "mode8a consumes one supplied draw and lifts only raw physical Z by129 blocks");
        check(get<float>(record, 0x180) == 123 && record[0x60] == 1 && get<uint32_t>(record, 0x80) == 0x04000000 &&
                  get<float>(record, 0x84) == 2.0f * 1.1f,
              "demon postprocessing preserves the earlier HP and native flag/size behavior");
    }
    std::printf("Resident pass: %s\n", failures ? "FAILED" : "passed");
    return failures;
}
