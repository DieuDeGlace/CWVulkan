// Independent steam-resident-effects-prerequisites.md arithmetic and lifecycle cases.
#include "ResidentEffects.h"
#include "worldgen/Residents.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <limits>

namespace {
template <class T> T get(const nativeeffects::Effect &effect, size_t offset) {
    T value;
    std::memcpy(&value, effect.data() + offset, sizeof value);
    return value;
}
template <class T> void put(nativeeffects::Effect &effect, size_t offset, const T &value) {
    std::memcpy(effect.data() + offset, &value, sizeof value);
}
} // namespace
int residentEffectsTests() {
    int failures = 0;
    auto check = [&](bool pass, const char *what) {
        if (!pass) {
            ++failures;
            std::printf("FAIL Resident effects: %s\n", what);
        }
    };
    {
        residentfx::Request existingFirst;
        existingFirst.existing(0x8a);
        existingFirst.created(0x8a);
        check(!existingFirst.requested(), "existing full-word mode8a closes permission before a new resident");
        residentfx::Request newFirst;
        newFirst.created(0x18a);
        newFirst.existing(0x8a);
        check(newFirst.requested(), "new low-byte mode8a requests once and later existing actors cannot retract it");
        residentfx::Request noncanonical;
        noncanonical.existing(0x18a);
        noncanonical.created(0x8a);
        check(noncanonical.requested(), "existing mode comparison uses the full32-bit word");
        worldgen::SpawnPoint point;
        point.mode = 0x18a;
        int draws = 0;
        auto next = [&] {
            ++draws;
            return 0;
        };
        const auto made = worldgen::makeResident(point, worldgen::Draw(next), false, {});
        int64_t z;
        std::memcpy(&z, made.record.data() + 0x20, sizeof z);
        check(made.record[0x68] == 0x8a && draws == 1 && z == 80 * 65536, "resident copy narrows mode before altitude admission");
    }
    int heightQueries = 0;
    const auto zero = residentfx::candidate(0, 0, 0, [&](int32_t x, int32_t y) {
        ++heightQueries;
        check(x == 32 && y == 32, "height is queried at the wrapped zone center");
        return 7.25f;
    });
    check(heightQueries == 1 && zero.position == std::array<int64_t, 3>{232 * 65536, 32 * 65536, 26689536},
          "day0 candidate is200 blocks east with center height plus400");
    const auto one = residentfx::candidate(0, 0, 1, [](int32_t, int32_t) { return 0.0f; });
    check(one.position[0] == 14853643 && one.position[1] == 5108897,
          "day1 preserves day-only xorshift, float angle/trig and separate fixed offsets");
    const auto wrapped =
        residentfx::candidate(std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min(), 0, [&](int32_t x, int32_t y) {
            check(x == -32 && y == 32, "zone center uses low32 multiply/add before sign extension");
            return 0.0f;
        });
    check(wrapped.position[0] == 168 * 65536 && wrapped.position[1] == 32 * 65536, "wrapped center does not become an absolute float");
    {
        nativeeffects::Effect effect;
        effect.fill(0x5a);
        put(effect, 0x50, uint32_t(0xdeadbeef));
        residentfx::initialize(effect, zero);
        check(get<int64_t>(effect, 0) == -1 && get<std::array<int64_t, 3>>(effect, 8) == zero.position &&
                  get<std::array<int64_t, 3>>(effect, 0x20) == std::array<int64_t, 3>{},
              "owner, position and target fields match the producer");
        check(get<float>(effect, 0x44) == -20 && get<float>(effect, 0x48) == 0 && get<float>(effect, 0x4c) == 0,
              "finite day0 velocity points toward the center at20 blocks per second");
        check(get<float>(effect, 0x54) == 80 && get<float>(effect, 0x58) == 20 && get<uint64_t>(effect, 0x5c) == 0 &&
                  get<int32_t>(effect, 0x68) == 9 && get<float>(effect, 0x70) == 0 && get<int32_t>(effect, 0x74) == 20000 &&
                  get<int32_t>(effect, 0x78) == 0 && effect[0x64] == 0 && effect[0x6c] == 0,
              "radius,size,auxiliary,kind,age,lifetime and freeze fields have their specified values");
        check(get<uint32_t>(effect, 0x50) == 0xdeadbeef && effect[0x65] == 0x5a && effect[0x6d] == 0x5a && effect[0x7c] == 0x5a,
              "unwritten damage and padding retain the supplied defined storage");
    }
    {
        std::list<nativeeffects::Effect> live;
        std::vector<nativeeffects::Effect> outgoing;
        check(residentfx::append(zero, live, outgoing) && live.size() == 1 && outgoing.size() == 1,
              "candidate appends one live record followed by one independent outgoing record");
        const auto creation = outgoing.front();
        check(!residentfx::append(zero, live, outgoing) && live.size() == 1 && outgoing.size() == 1,
              "an earlier zone's live effect suppresses another candidate without changing either list");
        nativeeffects::World world;
        world.ground = [](int, int) { return 0.0f; };
        nativeeffects::step(world, live, 16);
        check(live.size() == 1 && get<float>(live.front(), 0x70) == 16 && get<int64_t>(live.front(), 8) < zero.position[0] &&
                  outgoing.front() == creation,
              "same-step live movement/age leaves the outgoing creation copy unchanged");
        nativeeffects::Effect other{};
        put(other, 0x68, int32_t(9));
        put(other, 8, zero.position[0] + 200 * 65536);
        put(other, 0x10, zero.position[1]);
        put(other, 0x18, std::numeric_limits<int64_t>::max());
        put(other, 0x70, 100000.0f);
        live = {other};
        check(residentfx::suppressed(zero, live), "inclusive200-block XY suppression ignores Z and age, even before expiry");
        put(live.front(), 8, zero.position[0] + 200 * 65536 + 1);
        check(!residentfx::suppressed(zero, live), "one fixed unit outside suppression radius is admitted");
        put(live.front(), 8, zero.position[0]);
        put(live.front(), 0x68, int32_t(8));
        check(!residentfx::suppressed(zero, live), "other effect kinds cannot suppress a candidate");
    }
    std::printf("Resident effects: %s\n", failures ? "FAILED" : "passed");
    return failures;
}
