#include "InventorySlots.h"
#include "NativeUpkeep.h"
#include "NativeVehicle.h"
#include "UsePrompt.h"
#include "worldgen/Residents.h"
#include <algorithm>
#include <bit>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>

namespace {
template <class T> void set(uint8_t *record, size_t offset, T value) {
    std::memcpy(record + offset, &value, sizeof value);
}
template <class T> T read(const uint8_t *record, size_t offset) {
    T value;
    std::memcpy(&value, record + offset, sizeof value);
    return value;
}
struct Fixture {
    worldgen::CreatureRecord player = worldgen::newCreature(100);
    nativevehicle::Tabs inventory{6};
    std::map<nativesaves::Cell, std::vector<nativesaves::Record>> zones;
    nativevehicle::Environment world;
    nativesaves::Item selection{};
    int durationCalls = 0;
    std::vector<nativesaves::Cell> visits;
    Fixture() {
        set(player.data(), 0x150, int32_t(7));
        set(player.data(), 0x154, int32_t(-8));
        set(player.data(), 0x6c, 1000.0f);
        set(player.data(), 0x5c, uint32_t(1));
        world.player = player.data();
        world.namedWorld = true;
        world.inventory = &inventory;
        world.wholeDuration = [&] {
            ++durationCalls;
            return 400.0f;
        };
        world.loadedObjects = [&](nativesaves::Cell key) -> std::span<const nativesaves::Record> {
            visits.push_back(key);
            const auto found = zones.find(key);
            return found == zones.end() ? std::span<const nativesaves::Record>{} : found->second;
        };
        world.blockFlags = [](auto) -> std::optional<uint8_t> { return 0; };
    }
    void own(int subtype, int count = 1, int tab = 1) {
        auto item = nativesaves::makeItem(24, subtype);
        std::memcpy(item.data() + 0x0c, player.data() + 0x150, 8);
        inventory[size_t(tab)].push_back({count, item});
    }
    void object(int kind, uint8_t state = 0, int64_t x = 0) {
        nativesaves::Record record{};
        set(record.data(), 0, int32_t(kind));
        set(record.data(), 8, x);
        record[0x28] = state;
        zones[{0, 0}].push_back(record);
    }
    int subtype() const { return selection[0] ? read<int32_t>(selection.data(), 4) : -1; }
    void rebuild() { nativevehicle::rebuild(selection, world); }
};
} // namespace

int vehicleTests() {
    int failures = 0;
    const auto check = [&](bool result, const char *label) {
        if (!result) {
            std::cout << "FAIL vehicle: " << label << '\n';
            ++failures;
        }
    };
    Fixture availability;
    check(nativevehicle::ownsSpecialItem(nullptr, nullptr, 4), "subtype4 bypasses inventory and region");
    availability.own(1, 2, 0);
    availability.own(1, -2, 5);
    check(!nativevehicle::ownsSpecialItem(availability.player.data(), &availability.inventory, 1),
          "all physical tabs contribute signed quantities rather than first-match admission");
    availability.inventory[5][0].count = -1;
    auto &payload = availability.inventory[0][0].item;
    payload[1] = 0xcd; // padding and unused attribute bytes are not equality fields
    payload[0x1d] = 0xab;
    check(nativevehicle::ownsSpecialItem(availability.player.data(), &availability.inventory, 1),
          "positive total accepts padding and stale unused attributes");
    for (const size_t field : {size_t(8), size_t(0x0c), size_t(0x14), size_t(0x18), size_t(0x1c), size_t(0x9d)}) {
        ++payload[field];
        check(!nativevehicle::ownsSpecialItem(availability.player.data(), &availability.inventory, 1),
              "availability compares every stated semantic field against the regional empty template");
        --payload[field];
    }
    availability.inventory[0][0].count = INT32_MAX;
    availability.inventory[5][0].count = 1;
    check(!nativevehicle::ownsSpecialItem(availability.player.data(), &availability.inventory, 1),
          "positive signed32 overflow wraps negative instead of widening");

    const auto climbStamina = [](const std::vector<int32_t> &counts) {
        auto record = worldgen::newCreature(200);
        record[0x68] = 0x7a;
        set(record.data(), 0x5c, uint32_t(4));
        set(record.data(), 0x40, 1.0f);
        set(record.data(), 0x9a0, 1.0f);
        nativeai::Creature actor;
        actor.record = record.data();
        actor.inventory.resize(1);
        auto item = nativesaves::makeItem(24, 18);
        std::memcpy(item.data() + 0x0c, record.data() + 0x150, 8);
        for (int32_t count : counts) {
            std::array<uint8_t, 0xa4> slot{};
            set(slot.data(), 0, count);
            std::copy(item.begin(), item.end(), slot.begin() + 4);
            actor.inventory[0].push_back(slot);
        }
        nativeupkeep::World world;
        world.local = record.data();
        nativeupkeep::upkeep(world, &actor, 100.0f);
        return read<float>(record.data(), 0x9a0);
    };
    const float drain = climbStamina({});
    const float protectedStamina = climbStamina({1});
    check(protectedStamina > drain && climbStamina({INT32_MAX, 1}) == drain &&
              climbStamina({2, -1}) == protectedStamina,
          "actual climbing upkeep uses the same wrapped signed32 ownership total as vehicle selection");

    Fixture priority;
    set(priority.player.data(), 0x180, 0.0f); // no HP gate on the World producer
    for (int subtype : {1, 2, 3, 5, 7, 8, 13})
        priority.own(subtype);
    for (int kind : {1, 9, 0, 2, 16})
        priority.object(kind, 1);
    for (int expected : {7, 13, 4, 8, 5}) {
        priority.rebuild();
        check(priority.subtype() == expected, "static-object candidates retain their specified priority");
        if (expected == 4)
            priority.zones[{0, 0}][2][0x28] = 0;
        else
            for (auto &stack : priority.inventory[1])
                if (read<int32_t>(stack.item.data(), 4) == expected)
                    stack.count = 0;
    }
    Fixture alternate;
    alternate.own(8);
    alternate.object(15);
    alternate.rebuild();
    check(alternate.subtype() == 8, "subtype8 accepts its second exact object kind");
    alternate.zones[{0, 0}].clear();
    alternate.object(2);
    alternate.visits.clear();
    alternate.rebuild();
    check(alternate.subtype() == 8 && alternate.visits.size() == 8,
          "successful first subtype8 probe skips the second probe after the separate subtype4 query");
    set(alternate.player.data(), 0x5c, uint32_t(0));
    alternate.rebuild();
    check(alternate.subtype() == -1, "object probes do not replace the low-two-flag requirement");

    for (const auto [flags, expected] : {std::pair<uint32_t, int>{1, -1}, {2, 2}, {6, -1}, {0, 1}}) {
        Fixture travel;
        travel.own(1);
        travel.own(2);
        travel.own(3);
        set(travel.player.data(), 0x5c, flags);
        travel.selection.fill(0xcd);
        travel.rebuild();
        check(travel.subtype() == expected, "environment flags choose boat/glider without selecting carried subtype3");
        if (expected >= 0) {
            auto wanted = nativesaves::makeItem(24, expected);
            bool exact = true;
            for (size_t i = 0; i < wanted.size(); ++i) {
                const bool padding = (i >= 1 && i <= 3) || (i >= 0x15 && i <= 0x17) || i >= 0x9e;
                exact &= travel.selection[i] == (padding ? 0xcd : wanted[i]);
            }
            check(exact, "derived record clears all semantic fields, retains padding and emits zero region");
        }
    }
    Fixture physicalBlock;
    physicalBlock.own(1);
    set(physicalBlock.player.data(), 0x5c, uint32_t(0));
    set(physicalBlock.player.data(), 0x10, int64_t(-1));
    set(physicalBlock.player.data(), 0x18, int64_t(-65537));
    set(physicalBlock.player.data(), 0x20, int64_t(-65536));
    set(physicalBlock.player.data(), 0xb10, int64_t(999 * 65536));
    bool physicalQuery = false;
    physicalBlock.world.blockFlags = [&](nativevehicle::BlockPosition at) -> std::optional<uint8_t> {
        physicalQuery = at == nativevehicle::BlockPosition{-1, -2, -1};
        return 0;
    };
    physicalBlock.rebuild();
    check(physicalQuery && physicalBlock.subtype() == 1,
          "effective block uses narrowed physical XYZ rather than drawn position");

    Fixture protectedAir;
    protectedAir.own(1);
    set(protectedAir.player.data(), 0x5c, uint32_t(0));
    protectedAir.world.blockFlags = [](auto) -> std::optional<uint8_t> { return 0x20; };
    protectedAir.rebuild();
    check(protectedAir.subtype() == -1, "effective physical block0x20 blocks a new glider");
    protectedAir.player[0x68] = 0x78;
    protectedAir.rebuild();
    check(protectedAir.subtype() == 1, "failed environmental candidates retain an active provisional glider");
    protectedAir.world.blockFlags = [](auto) -> std::optional<uint8_t> { return std::nullopt; };
    protectedAir.player[0x68] = 0;
    protectedAir.rebuild();
    check(protectedAir.subtype() == -1, "unavailable provider flags are not guessed to admit travel");
    protectedAir.world.blockFlags = [](auto) -> std::optional<uint8_t> { return 0; };
    protectedAir.player[0x68] = 0x53;
    protectedAir.rebuild();
    check(protectedAir.subtype() == -1, "ability0x53 excludes a new glider");

    Fixture timing;
    timing.own(1);
    set(timing.player.data(), 0x5c, uint32_t(0));
    timing.selection = nativesaves::makeItem(24, 7);
    for (int ability : {0x98, 0x9c, 0x9d}) {
        timing.player[0x68] = uint8_t(ability);
        timing.rebuild();
        check(timing.subtype() == 7, "skipped outer ability slice retains the prior cache");
    }
    timing.player[0x68] = 0x79;
    timing.world.namedWorld = false;
    timing.rebuild();
    check(timing.subtype() == 7 && timing.player[0x68] == 0x79,
          "unnamed World slice retains the cache and skips availability cancellation");
    timing.world.namedWorld = true;
    timing.player[0x68] = 0x99;
    timing.rebuild();
    check(timing.subtype() == 1, "ability0x99 remains admitted");
    timing.player[0x68] = 0;
    set(timing.player.data(), 0x6c, 400.0f);
    timing.rebuild();
    check(timing.subtype() == -1, "whole-duration equality fails strict initial eligibility");
    set(timing.player.data(), 0x6c, std::numeric_limits<float>::quiet_NaN());
    timing.rebuild();
    check(timing.subtype() == -1, "unordered age does not pass eligibility");
    set(timing.player.data(), 0x6c, 1000.0f);
    set(timing.player.data(), 0x12c, std::numeric_limits<float>::quiet_NaN());
    timing.durationCalls = 0;
    timing.rebuild();
    check(timing.subtype() == -1 && timing.durationCalls == 0, "unordered dodge timer stops before duration query");
    set(timing.player.data(), 0x12c, -0.0f);
    timing.player[0x68] = 0x99;
    timing.durationCalls = 0;
    timing.world.wholeDuration = [&] { return ++timing.durationCalls == 1 ? 400.0f : 1001.0f; };
    timing.rebuild();
    check(timing.subtype() == -1 && timing.durationCalls == 2,
          "nonzero ability glider test performs its separate non-strict duration query");
    for (int ability : {0x76, 0x78, 0x79}) {
        Fixture cancel;
        cancel.player[0x68] = uint8_t(ability);
        cancel.world.wholeDuration = [&] {
            check(cancel.player[0x68] == 0, "availability cancellation precedes duration query");
            return 400.0f;
        };
        cancel.rebuild();
        check(cancel.player[0x68] == 0 && read<float>(cancel.player.data(), 0x6c) == 1000.0f,
              "lost ownership cancels the appropriate ability without resetting age");
    }

    Fixture spatial;
    spatial.object(1, 0, 8 * 65536);
    check(nativevehicle::nearbyObject(spatial.world, 1, false), "radius includes the exact8-block boundary");
    check(!nativevehicle::nearbyObject(spatial.world, 1, true), "stateful probes require a nonzero object byte");
    set(spatial.zones[{0, 0}][0].data(), 8, int64_t(8 * 65536 + 1));
    check(!nativevehicle::nearbyObject(spatial.world, 1, false), "one fixed unit beyond radius is excluded");
    set(spatial.player.data(), 0x10, int64_t(-1));
    set(spatial.player.data(), 0x18, int64_t(-1));
    spatial.visits.clear();
    nativevehicle::nearbyObject(spatial.world, 99, false);
    check(spatial.visits == std::vector<nativesaves::Cell>{{-1, -1}, {-1, 0}, {0, -1}, {0, 0}},
          "negative fractional bounds and X-outer/Y-inner lookup order use physical coordinates");

    Fixture use;
    use.own(1);
    set(use.player.data(), 0x5c, uint32_t(0));
    use.rebuild();
    std::fill(use.player.begin() + 0x1d8, use.player.begin() + 0x278, 0xcd);
    useprompt::Scene scene;
    scene.player = use.player.data();
    scene.useDown = true;
    scene.vehicle = use.selection;
    scene.vehicleName = "vehicle fixture";
    useprompt::KeyState key;
    nativeai::Creature actor;
    actor.record = use.player.data();
    const auto prompt = useprompt::decide(scene, {}, key);
    useprompt::applyToPlayer(prompt, scene, actor);
    check(prompt.action == useprompt::Action::Vehicle && use.player[0x68] == 0x78 &&
              read<int32_t>(use.player.data(), 0x1e4) == 0 && use.inventory[1][0].count == 1,
          "actual Use starts with the synthetic zero-region record and does not consume inventory");
    check(use.player[0x1d9] == 0xcd && use.player[0x1ed] == 0xcd && use.player[0x276] == 0xcd,
          "actual vehicle Use assigns semantic fields while retaining prior held-item padding");
    use.player[0x68] = 0;
    inventoryslots::PlayerState direct;
    direct.inventory.tabs = use.inventory;
    const auto directUse = inventoryslots::useItem(direct, {1, 0}, use.player.data());
    check(directUse.action == inventoryslots::Action::Used && use.player[0x68] == 0x78 &&
              read<int32_t>(use.player.data(), 0x1e4) == 7 && read<int32_t>(use.player.data(), 0x1e8) == -8 &&
              direct.inventory.tabs[1][0].count == 1 && read<int32_t>(use.selection.data(), 0x0c) == 0,
          "direct inventory Use retains the original region and leaves the generated cache separate");
    std::cout << "Derived vehicle: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
