// Where a revived player stands (docs/specs/steam-shrine-of-life-activation-20261112.md §6.2).
#include "ShrineRespawn.h"
#include "Extracted.h"
#include "Spawned.h"
#include <cstdio>
#include <cstring>

int shrineRespawnTests() {
    using namespace shrinerespawn;
    int failures = 0;
    const auto check = [&](bool passed, const char *message) {
        if (!passed) {
            std::printf("FAIL shrine respawn: %s\n", message);
            ++failures;
        }
    };
    const auto fixed = [](int64_t blocks) { return blocks * 65536; };
    // Lands are 8192-block squares here; shrines stand where the test says.
    std::set<Cell> shrines;
    shrinerespawn::World w;
    w.landOf = [](int32_t bx, int32_t by) {
        return Cell{bx >= 0 ? bx / 8192 : -((-bx + 8191) / 8192), by >= 0 ? by / 8192 : -((-by + 8191) / 8192)};
    };
    w.isShrine = [&](int32_t cx, int32_t cy) { return shrines.count({cx, cy}) != 0; };
    const Cell land{0, 0};

    // Pass 1: the nearest known shrine of the land wins, even when an unknown site is nearer.
    const Cell site = siteOf(0, 0); // a real site in region (0, 0)
    shrines = {site, {200, 200}, {300, 300}};
    const int64_t px = fixed(site[0] * 16 + 8 + 40), py = fixed(site[1] * 16 + 8);
    std::set<Cell> known{{300, 300}, {200, 200}, {5, 5}}; // (5, 5): a chest cell, not a shrine
    Cell got = choose(w, known, px, py, land);
    check(got == Cell({200, 200}), "the nearest known shrine in the land wins over a nearer unknown site");
    // A known shrine in another land is ignored; then pass 2 takes the land's nearest site.
    known = {{600, 600}}; // block 9608: land (1, 1)
    shrines = {site, {600, 600}};
    got = choose(w, known, px, py, land);
    check(got == site, "known shrines in other lands are ignored; the nearest shrine site of the land is taken");
    // Nothing: the death cell.
    shrines.clear();
    known.clear();
    got = choose(w, known, fixed(-17), fixed(33), land);
    check(got == Cell({-2, 2}), "with no shrine at all the player revives in its death cell (floored cells)");
    // Pass 2 needs the building: a site whose building is not a shrine is skipped.
    shrines = {};
    got = choose(w, {}, px, py, land);
    check(got == Cell({(int32_t)((site[0] * 16 + 8 + 40) / 16), site[1]}), "a site without the shrine building is not a respawn point");
    // Ties keep the first known cell in set order.
    shrines = {{100, 100}, {100, 120}};
    known = {{100, 120}, {100, 100}};
    got = choose(w, known, fixed(100 * 16 + 8), fixed(110 * 16 + 8), land);
    check(got == Cell({100, 100}), "equal distances keep the first known cell in its sort order");
    // §6.1 step 4: the revived player takes the respawn point exactly, also far from the
    // origin where a float would round it (here x near 122 million blocks rounds to 8).
    {
        ExtractedWorld blocks;
        SpawnedCreatures creatures;
        creatures.setNativePlayer(true);
        const int64_t point[3] = {fixed(7650981LL * 16 + 6), fixed(59604LL * 16 + 8), fixed(228)};
        check(!creatures.placePlayer(blocks, point), "no record is placed before the player exists");
        creatures.setPlayer(blocks, {0, 0, 130}, 0, {0, 0, 0}, 100, 1, true);
        creatures.setPlayer(blocks, {0, 0, 130}, 0, {0, 0, 0}, 100, 1, true);
        check(creatures.placePlayer(blocks, point), "the native player is placed");
        const auto at = [&](int axis) {
            int64_t v;
            std::memcpy(&v, creatures.playerRecord() + 0x10 + axis * 8, sizeof v);
            return v;
        };
        const auto drawn = [&](int axis) {
            int64_t v;
            std::memcpy(&v, creatures.playerRecord() + 0xb10 + axis * 8, sizeof v);
            return v;
        };
        check(at(0) == point[0] && at(1) == point[1] && at(2) == point[2], "the record stands exactly at the respawn point");
        check(drawn(0) == point[0] && drawn(1) == point[1] && drawn(2) == point[2], "it is drawn there too");
        // The legacy bridge then passes CubeVulkan's feet, a block below the record: no move.
        glm::vec3 feet = blocks.toLocal(point);
        feet.z -= SpawnedCreatures::NATIVE_FEET;
        creatures.setPlayer(blocks, feet, 0, {0, 0, 0}, 100, 1, true);
        check(at(0) == point[0] && at(1) == point[1] && at(2) == point[2], "the next bridge frame keeps the exact point");
    }
    std::puts("shrine respawn: known shrines, land limit, sites, death cell, ties and the revival place checked");
    return failures;
}
