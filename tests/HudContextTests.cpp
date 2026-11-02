#include "NativeHudContext.h"
#include <array>
#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstring>
#include <map>

int hudContextTests() {
    using namespace hudcontext;
    int failures = 0;
    auto check = [&](bool pass, const char *why) {
        if (!pass) {
            std::printf("HUD context FAIL: %s\n", why);
            ++failures;
        }
    };
    std::array<uint8_t, 0x200> player{}; // HP0 is intentionally not a context gate.
    Tabs inventory(3);
    std::map<nativesaves::Cell, nativesaves::ZoneLists> zones;
    std::vector<nativesaves::Cell> visited;
    Environment environment;
    environment.player = player.data();
    environment.namedWorld = true;
    environment.inventory = &inventory;
    environment.loadedZone = [&](nativesaves::Cell key) -> std::optional<Zone> {
        visited.push_back(key);
        const auto found = zones.find(key);
        if (found == zones.end())
            return std::nullopt;
        return Zone{found->second.groundItems, found->second.staticObjects};
    };
    auto item = [](int category, int subtype) { return nativesaves::makeItem(category, subtype); };
    auto ground = [](const nativesaves::Item &value) {
        nativesaves::Record record{};
        std::copy(value.begin(), value.end(), record.begin());
        return record;
    };
    auto object = [](const nativesaves::Item &value, bool active) {
        nativesaves::Record record{};
        record[0] = 0xff; // no object-type filter belongs to this scan
        record[0x28] = active;
        std::copy(value.begin(), value.end(), record.begin() + 0x38);
        return record;
    };
    auto add = [&](int tab, const nativesaves::Item &value, int32_t count) { inventory[size_t(tab)].push_back({count, value}); };
    State state;
    check(state.item[0] == 0 && state.count == 0, "fresh context defaults");
    state.item = item(24, 7);
    state.count = 9;
    const auto previous = state.item;
    environment.namedWorld = false;
    rebuild(state, environment);
    check(state.item == previous && state.count == 9 && visited.empty(), "unnamed World preserves the cache");
    environment.namedWorld = true;
    for (uint8_t ability : {0x98, 0x9c, 0x9d}) {
        player[0x68] = ability;
        rebuild(state, environment);
        check(state.item == previous && state.count == 9 && visited.empty(), "skipped ability preserves the cache");
    }
    player[0x68] = 0x99;
    rebuild(state, environment);
    check(state.item[0] == 0 && state.count == 0 && visited.size() == 9, "ability99 and zero-step rebuild are admitted");
    check(visited.front() == nativesaves::Cell{-1, -1} && visited[1] == nativesaves::Cell{-1, 0} &&
              visited[3] == nativesaves::Cell{0, -1} && visited.back() == nativesaves::Cell{1, 1},
          "loaded lookup preserves coordinate order without requesting missing zones");

    auto first = item(22, 1), middle = item(22, 2), last = item(1, 3);
    zones[{-1, -1}].groundItems = {ground(first)};
    zones[{-1, -1}].staticObjects = {object(item(24, 1), true)};
    zones[{0, 0}].groundItems = {ground(middle)};
    zones[{1, -1}].staticObjects = {object(last, true), object(item(1, 4), false)};
    add(1, first, 2);
    add(1, middle, 3);
    add(0, last, 100); // matching record in the wrong tab must not authorize the context
    visited.clear();
    rebuild(state, environment);
    check(state.item[0] == 0 && state.count == 0, "unavailable last candidate suppresses earlier available items");
    add(2, last, 2);
    rebuild(state, environment);
    check(state.item == last && state.count == 2, "last active requirement wins and counts only its category tab");
    check(countText(state.count) == "2" && countText(1).empty() && countText(0).empty() && countText(-1).empty(),
          "context quantity text is decimal only above one");

    inventory[2] = {{INT_MAX, last}, {1, last}};
    state.item[1] = 0xab;
    state.item[0x15] = 0xcd;
    state.item[0x9f] = 0xef;
    rebuild(state, environment);
    check(state.item[0] == 0 && state.count == INT_MIN, "signed total wraps and survives final item rejection");
    check(state.item[1] == 0xab && state.item[0x15] == 0xcd && state.item[0x9f] == 0xef,
          "semantic copies and clears preserve destination padding");

    inventory[2] = {{5, last}, {-4, last}};
    inventory[2][0].item[1] = 0x77;
    inventory[2][0].item[0x15] = 0x88;
    inventory[2][0].item[0x21] = 0x99; // inactive attribute storage is ignored by equality
    rebuild(state, environment);
    check(state.item[0] == 1 && state.count == 1 && countText(state.count).empty(),
          "negative quantities participate; padding and inactive attributes do not reject a match");
    const uint32_t formula = 0xa1b20003u;
    std::memcpy(last.data() + nativesaves::item::FORMULA, &formula, 4);
    last[nativesaves::item::ATTRIBUTE_COUNT] = 1;
    last[0x1d] = 7;
    last[0x9c] = 0x55; // copy all attributes even though only the first is active
    zones[{1, -1}].staticObjects = {object(last, true)};
    inventory[2] = {{4, last}};
    rebuild(state, environment);
    check(state.item[0x9c] == 0x55 && state.item[0x1d] == 7 && state.count == 4, "original full semantic item is copied");
    inventory[2][0].item[nativesaves::item::FORMULA + 3] ^= 1;
    rebuild(state, environment);
    check(state.item[0] == 0 && state.count == 0, "full formula word participates in availability");
    inventory[2][0].item = last;
    inventory[2][0].item[0x1d] ^= 1;
    rebuild(state, environment);
    check(state.item[0] == 0, "active attribute bytes participate in availability");

    zones.clear();
    inventory[2] = {{8, nativesaves::Item{}}};
    rebuild(state, environment);
    check(state.item[0] == 0 && state.count == 8 && countText(state.count) == "8",
          "empty-category total does not fabricate a model or impose a category text gate");
    int64_t physical[2]{-1, int64_t(64) * 65536};
    std::memcpy(player.data() + 0x10, physical, sizeof physical);
    visited.clear();
    rebuild(state, environment);
    check(visited.front() == nativesaves::Cell{-2, 0} && visited.back() == nativesaves::Cell{0, 2},
          "negative fractional fixed positions choose the correct neighborhood");
    environment.player = nullptr;
    visited.clear();
    rebuild(state, environment);
    check(state.count == 8 && visited.empty(), "missing local player preserves prior state");
    State rowContext;
    rowContext.count = 8; // model admission follows category, not count text
    Tabs specials(3);
    auto regional = [&](int subtype, nativesaves::Cell region) {
        auto value = item(24, subtype);
        std::memcpy(value.data() + nativesaves::item::REGION, region.data(), sizeof region);
        return value;
    };
    const auto a = regional(1, {7, -8}), b = regional(18, {7, -8});
    specials[1] = {{0, a}, {-1, a}, {2, regional(14, {7, -8})}, {1, item(22, 1)}, {1, regional(18, {7, 8})}, {1, b}, {4, a}};
    auto row = modelRow(rowContext, specials, {7, -8});
    check(row.size() == 3 && row[0].x == 225 && row[1].x == 275 && row[2].x == 325 && row[0].item == &specials[1][1].item &&
              row[1].item == &specials[1][5].item && row[2].item == &specials[1][6].item,
          "special row preserves physical duplicates/negative counts, filters exact region/subtypes, skips no positions");
    rowContext.item = item(3, 1);
    row = modelRow(rowContext, specials, {7, -8});
    check(row.size() == 4 && row[0].item == &rowContext.item && row[0].x == 225 && row[1].x == 325 && row[3].x == 425,
          "any nonzero context category reserves the larger special-row start");
    check(modelRow(rowContext, {}, {7, -8}).size() == 1, "absent special tab retains the context model");
    std::printf("HUD context: %d failures\n", failures);
    return failures;
}
