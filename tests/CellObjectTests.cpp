#include "worldgen/CellObjects.h"
#include "worldgen/Height.h"
#include "worldgen/Options.h"

#include <array>
#include <cstdio>

int cellObjectTests() {
    using namespace worldgen;
    int failures = 0;
    auto check = [&](bool passed, const char *description) {
        if (!passed) {
            ++failures;
            std::printf("FAIL cell objects: %s\n", description);
        }
    };

    // Literal expectations from the held classifier contract. Actual owned terrain
    // queries supply the stated preconditions; these are not original-game captures.
    setWorldScale(1);
    setModHooks({});
    const WorldValues world = worldValues(123);
    CellObjects objects(world);
    struct Candidate {
        int32_t x, y;
        int expected;
        bool inner;
    };
    const Candidate candidates[] = {
        {-38363, -3881, 53, false}, {-38218, 32750, 53, false}, {-38199, -15830, 53, false}, {-38090, 8150, 53, false},
        {-37857, 8671, 30, true},   {-37592, -7697, 27, true},  {-37334, 2017, 27, true},    {-37033, 11800, 31, true},
    };
    for (const auto &candidate : candidates) {
        const Int2 q = cellPoint(candidate.x, candidate.y);
        const Place p = placeAt(q.x, q.y);
        const Int2 group{floorDivBy(candidate.x, 3), floorDivBy(candidate.y, 3)};
        const int32_t x = wrapAdd(wrapMul(group.x, 48), 16);
        const int32_t y = wrapAdd(wrapMul(group.y, 48), 16);
        const float weight = placeWeight(2, p.big, warpedDist2(p.point, x, y));
        check(p.kind == 2 && (weight > .5f) == candidate.inner && roads(x, y, 2, weight) < .4f,
              "category-two fixtures retain their distinct group-point weights and road admission");
        check(areaLandmark(floorDiv(candidate.x, 6), floorDiv(candidate.y, 6)) == Int2{candidate.x, candidate.y},
              "fixtures query the exact canonical point rather than another cell with the same parity");
        const int actual = objects.type(candidate.x, candidate.y);
        if (actual != candidate.expected)
            std::printf("  cell %d %d: expected %d, actual %d\n", candidate.x, candidate.y, candidate.expected, actual);
        check(actual == candidate.expected, candidate.inner
                                                ? "inner outside-box category-two group marker preempts the canonical landmark"
                                                : "low group-point weight skips all category-two markers before canonical lookup");
        check(objects.type(candidate.x, candidate.y) == actual, "cached classification preserves the query result");
        check(objects.type(candidate.x + 1, candidate.y) != 53,
              "another point in the same odd group does not inherit the canonical result");
    }

    // The independent branch-control correction keeps an unmatched inside-box
    // point out of the outside-box query, then reaches the exact canonical point.
    for (const Int2 cell : {Int2{-58138, 11932}, Int2{-58005, 30867}}) {
        const Int2 q = cellPoint(cell.x, cell.y);
        const Place p = placeAt(q.x, q.y);
        const Int2 centre{floorDiv(p.point.x, 4), floorDiv(p.point.y, 4)};
        const int32_t x = wrapAdd(wrapMul(floorDivBy(cell.x, 3), 48), 16);
        const int32_t y = wrapAdd(wrapMul(floorDivBy(cell.y, 3), 48), 16);
        check(p.kind == 2 && std::abs(cell.x - centre.x) == 1 && cell.y == centre.y &&
                  placeWeight(2, p.big, warpedDist2(p.point, x, y)) > .5f,
              "nearby-miss fixtures retain the admitted weight and inside-box geometry");
        check(areaLandmark(floorDiv(cell.x, 6), floorDiv(cell.y, 6)) == cell,
              "nearby-miss fixtures are exact canonical points");
        check(objects.type(cell.x, cell.y) == 53,
              "unmatched nearby category-two point continues to canonical selection");
    }

    // Independent integer vectors include wrapped-negative hash words. These do
    // not require the numerical terrain/provider closure to determine the answer.
    check(bigStructure(-304488, 0, 5, true) == 41 && bigStructure(266718, 0, 5, true) == 41,
          "unsigned-anchor fixtures enter the big-structure route");
    check(structureAnchor(-304488, 0, 5, true) == Int2{-19487124, 92},
          "structure X offset uses the unsigned remainder of hash word fffffffe");
    check(structureAnchor(266718, 0, 5, true) == Int2{17069972, 28},
          "structure Y offset uses the unsigned remainder of hash word ffffffff");

    // Complete synthetic vectors isolate current-count selection from imported
    // shuffle state. Their nonzero entries deliberately identify selected tables.
    for (const auto [count, expected] : std::array<std::array<int, 2>, 3>{{{64, 6}, {65, 10}, {128, 10}}}) {
        WorldValues tables;
        tables.townLayouts.resize(count);
        for (int i = 0; i < count; ++i)
            tables.townLayouts[size_t(i)].fill(2 + i % 10);
        check(townBlock(tables, 24, 24, 5) == expected,
              "town query reduces unsigned hash 35908 by the current table count");
    }
    WorldValues singleTable;
    singleTable.townLayouts.resize(1);
    for (int i = 0; i < 16; ++i)
        singleTable.townLayouts[0][size_t(i)] = 100 + i;
    bool rowMajor = true;
    for (int row = 0; row < 4; ++row)
        for (int column = 0; column < 4; ++column)
            for (int phase = 0; phase < 3; ++phase)
                rowMajor &=
                    townBlock(singleTable, 24 + 3 * column + phase, 24 + 3 * row + phase, 5) == 100 + 4 * row + column;
    check(rowMajor, "one current table covers all sixteen row-major entries and their cell phases");
    check(townBlock(singleTable, 23, 24, 5) == 0 && townBlock(singleTable, 36, 24, 5) == 0,
          "the table window includes four groups and excludes both adjacent groups");

    WorldValues zeroSlots;
    zeroSlots.townLayouts.resize(64);
    check(townBlock(zeroSlots, -1881, -1602, 5) == 1 && townBlock(zeroSlots, -1881, -1599, 5) == 1,
          "a zero in-window slot continues into the numerical fallback");
    check(townBlock(zeroSlots, -1881, -1839, 5) == 1 && townBlock(zeroSlots, -1881, -1827, 5) == 13,
          "outside the table window fallback can produce either one or thirteen");

    const Int2 rareQuery = cellPoint(10563862, 30);
    check(placeAt(rareQuery.x, rareQuery.y).kind == 0 && areaLandmark(165060, 0) == Int2{10563862, 30},
          "rare even-group fixture reaches the canonical point without a category gate");
    check(objects.type(10563862, 30) == 15,
          "canonical even-group selection uses unsigned word fffffffd remainder eleven");

    WorldValues noTables;
    check(townBlock(noTables, 24, 24, 5) == 0,
          "unconfigured in-window storage follows the explicit safe engine policy");
    check(townBlock(noTables, 24, 24, 4) == 0, "non-five categories skip table and fallback work");
    check(townBlock(noTables, 0, 0, 5) == 0, "outside the table window no table header is read");

    std::printf("Cell objects: ordered category-two gates, unsigned anchors and table selection: %d failure(s)\n",
                failures);
    return failures;
}
