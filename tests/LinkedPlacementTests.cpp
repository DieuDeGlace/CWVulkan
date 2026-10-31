// Independent linked-placement contract: finite distinct records, no game assets.
#include "LinkedPlacement.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {
using Record = std::array<uint8_t, 0x2000>;
using Point = std::array<int64_t, 3>;
using Vector = std::array<float, 3>;
template <class T> void put(Record &r, size_t offset, T value) {
    std::memcpy(r.data() + offset, &value, sizeof value);
}
template <class T> T get(const Record &r, size_t offset) {
    T value;
    std::memcpy(&value, r.data() + offset, sizeof value);
    return value;
}
void setup(Record &c, Record &t) {
    c.fill(0);
    t.fill(0);
    put<int64_t>(c, 8, 1);
    put<int64_t>(t, 8, 2);
    put<int64_t>(c, 0x1a8, 2);
    put<float>(c, 0x180, 100);
    put<float>(t, 0x180, 100);
    put<Vector>(c, 0x84, {2, 3, 4});
    put<Vector>(t, 0x84, {6, 8, 10});
    put<Point>(c, 0x10, {10000, 20000, 30000});
    put<Point>(c, 0xb10, {40000, 50000, 60000});
    put<Point>(t, 0x10, {70000, 80000, 90000});
    put<Point>(t, 0xb10, {100000, 110000, 120000});
    put<Vector>(c, 0x28, {10, 20, 30});
    put<Vector>(c, 0xb34, {40, 50, 60});
    put<Vector>(t, 0x28, {1, 2, 3});
    put<Vector>(t, 0xb34, {4, 5, 6});
    put<Vector>(c, 0x34, {1, 2, 3});
    put<Vector>(c, 0x4c, {7, 8, 9});
    put<float>(c, 0x6c, 77);
    put<float>(t, 0x6c, 77);
    put<float>(c, 0x990, .2f);
    put<float>(t, 0x990, 5);
    put<float>(t, 0x998, 9);
    put<float>(t, 0x99c, 2);
    put<float>(c, 0xb58, 31);
    put<float>(t, 0xb58, 32);
    put<uint16_t>(t, 0x128, 0x100);
    put<float>(c, 0x12c, 55);
}
} // namespace

int linkedPlacementTests() {
    int failures = 0;
    auto check = [&](bool pass, const char *what) {
        if (!pass) {
            std::printf("FAIL Linked placement: %s\n", what);
            ++failures;
        }
    };
    Record c, t;
    linkedplacement::World world;
    world.byId = [&](int64_t id) -> uint8_t * { return id == 1 ? c.data() : id == 2 ? t.data() : nullptr; };
    world.duration = [](const uint8_t *) { return 100; };
    std::vector<int64_t> notes;
    world.focusFailure = [&](int64_t id) { notes.push_back(id); };
    setup(c, t);
    c[0x68] = 0x80;
    put<int64_t>(c, 0x1a0, 2);
    put<uint32_t>(t, 0x80, 0x4000);
    put<float>(c, 0x180, 0);
    linkedplacement::update(world, c.data(), 16);
    check(get<Point>(t, 0x10) == Point{10000, 20000, 324912} && get<Point>(t, 0xb10) == Point{40000, 50000, 354912},
          "owner80 uses separate raw/drawn origins and half carrier plus quarter target height without J");
    check(t[0x68] == 0 && get<uint16_t>(t, 0x128) == 0x110 && get<float>(t, 0x6c) == 77 && get<float>(t, 0xb58) == 32 &&
              get<float>(t, 0x998) == 0 && get<float>(t, 0x99c) == 0,
          "owner80 finish adds only state10, preserves clocks and applies conditional pose reset even when carrier is dead");
    setup(c, t);
    c[0x68] = 0x80;
    put<int64_t>(c, 0x1a0, 2);
    put<float>(t, 0x180, 0);
    linkedplacement::update(world, c.data(), 16);
    check(c[0x68] == 0 && get<int64_t>(c, 0x1a8) == 0 && get<int64_t>(c, 0x1a0) == 2 && get<float>(c, 0x6c) == 77,
          "failed owner80 clears linked ID rather than owner and does not reset timer");
    setup(c, t);
    c[0x68] = 0x80;
    put<int64_t>(c, 0x1a0, 2);
    put<int64_t>(c, 0x9d8, 2);
    t[0x60] = 6;
    put<uint16_t>(t, 0x128, 2);
    put<uint32_t>(t, 0x80, 2);
    linkedplacement::update(world, c.data(), 16);
    check(c[0x68] == 0x80 && get<float>(t, 0x998) == 9 && get<float>(t, 0x99c) == 2,
          "kind6 pet alternative bypasses carriable predicate and appearance bit2 retains pose values");
    struct Row {
        int race;
        uint8_t kind, sourceAbility, targetAbility;
        int64_t rawZ, drawnZ;
        bool resetTimer, keepAngles, spin, seated;
    };
    for (const Row &row : std::vector<Row>{{0xc2, 8, 0x83, 0x57, 198574, 198574, false, false, false, true},
                                           {0x10f, 0, 0x83, 0x5b, 296878, 296878, true, false, false, true},
                                           {0x111, 0, 0x83, 0x57, 198574, 198574, false, false, false, true},
                                           {0x112, 0, 0x83, 0x5c, 198574, 198574, true, false, false, true},
                                           {0x113, 0, 0x83, 0x5d, -260177, -260177, false, false, false, true},
                                           {0xdd, 0, 0x83, 0, 460718, 460718, false, true, false, true},
                                           {0xde, 0, 0x83, 0, 460718, 460718, false, true, false, true},
                                           {0x12b, 0, 0x83, 0, 460718, 460718, false, false, true, true},
                                           {2, 0, 0x83, 0, 131072, 133038, false, false, false, true},
                                           {0xbb, 0, 0, 0x58, 296878, 296878, false, false, false, false}}) {
        setup(c, t);
        c[0x60] = row.kind;
        c[0x68] = row.sourceAbility;
        put<int32_t>(c, 0x64, row.race);
        linkedplacement::update(world, c.data(), 16);
        const int64_t rawXY = row.race == 2 ? 0 : 1966;
        const Point raw{10000 + rawXY, 20000 + rawXY, 30000 + row.rawZ};
        const Point drawn = row.spin ? raw : Point{41966, 51966, 60000 + row.drawnZ};
        check(get<Point>(t, 0x10) == raw && get<Point>(t, 0xb10) == drawn && t[0x68] == row.targetAbility,
              "ordered placement row retains exact fixed XY/Z and raw-versus-drawn exceptions");
        check(get<float>(t, 0x6c) == (row.resetTimer ? 0 : 77) && get<float>(t, 0xb58) == 32 &&
                  get<uint16_t>(t, 0x128) == (row.seated ? 0x102 : 0x100),
              "only specified rows reset timer/add state2; no helper row changes smoothing control");
        if (row.keepAngles)
            check(get<Vector>(t, 0x28) == Vector{1, 2, 3} && get<Vector>(t, 0xb34) == Vector{4, 5, 6}, "dd/de preserve target angles");
        else if (row.spin)
            check(get<float>(t, 0x30) == 3.16f && get<Vector>(t, 0xb34) == get<Vector>(t, 0x28),
                  "12b spins target raw yaw then copies its own pose");
        else
            check(get<Vector>(t, 0x28) == Vector{10, 20, 30} && get<Vector>(t, 0xb34) == Vector{40, 50, 60},
                  "angle-copy row preserves both source angle sets");
    }
    setup(c, t);
    put<int32_t>(c, 0x64, 0xbb);
    put<Vector>(t, 0x40, {.1f, 0, 0});
    put<float>(t, 0x180, 0);
    linkedplacement::update(world, c.data(), 16);
    check(get<int64_t>(c, 0x1a8) == 0, "non83 intent rejection clears link before the failed target HP test");
    setup(c, t);
    put<int32_t>(c, 0x64, 0xbb);
    put<float>(t, 0x180, 0);
    linkedplacement::update(world, c.data(), 16);
    check(get<int64_t>(c, 0x1a8) == 2, "dead nonmoving target does not clear a valid linked ID");
    setup(c, t);
    c[0x68] = 0x91;
    put<Vector>(c, 0x174, {.25f, -.5f, 1});
    linkedplacement::update(world, c.data(), 16);
    check(get<Point>(c, 0x10) == Point{86384, 47232, 155536} && get<Point>(c, 0xb10) == Point{116384, 77232, 185536},
          "91 uses unnormalized raw look offsets on separate target raw/drawn origins");
    setup(c, t);
    c[0x68] = 0x8c;
    put<int32_t>(t, 0x64, 0xd2);
    put<float>(t, 0x30, -90);
    linkedplacement::update(world, c.data(), 16);
    check(get<Point>(c, 0x10) == Point{-126608, 60340, 364596} && get<Point>(c, 0xb10) == Point{-96608, 90340, 394596},
          "8c independently truncates race, radial and height terms without rotating race Y offset");
    check(get<Vector>(c, 0x28) == Vector{1, 2, -90} && get<Vector>(c, 0xb34) == Vector{40, 50, 60} && get<Vector>(c, 0x34) == Vector{} &&
              get<Vector>(c, 0x4c) == Vector{} && get<float>(c, 0x12c) == 0 && c[0x68] == 0x8c,
          "8c success copies raw angles only and clears motion, knockback and dodge");
    setup(c, t);
    notes.clear();
    c[0x68] = 0x8c;
    put<float>(c, 0x6c, 100);
    linkedplacement::update(world, c.data(), 16);
    check(c[0x68] == 0 && get<Point>(c, 0x10) == Point{70000, 80000, 90000} && get<Point>(c, 0xb10) == Point{40000, 50000, 60000} &&
              get<int64_t>(c, 0x1a8) == 2 && get<float>(c, 0xb58) == 31 && notes == std::vector<int64_t>{2},
          "8c equality fails, notes focus and snaps raw position only without clearing link or smoothing");
    setup(c, t);
    c[0x60] = 6;
    put<int64_t>(c, 0x1a0, 2);
    t[0x68] = 0x76;
    put<float>(t, 0x180, 0);
    linkedplacement::update(world, c.data(), 16);
    check(get<Point>(c, 0x10) == Point{70000, 80000, -105952} && get<Point>(c, 0xb10) == Point{100000, 110000, -75952} &&
              get<float>(c, 0xb58) == 0 && get<float>(c, 0x998) == 9 && get<float>(c, 0x99c) == 6,
          "kind6 owner76 follows even a dead owner and alone resets b58 with ordered pose scaling");
    check(get<Vector>(c, 0x34) == Vector{} && get<Vector>(c, 0x4c) == Vector{7, 8, 9} && get<float>(c, 0x12c) == 55,
          "final kind6 stage clears velocity/intent while preserving knockback and dodge");
    std::printf("Linked placement: %s\n", failures ? "FAILED" : "passed");
    return failures;
}
