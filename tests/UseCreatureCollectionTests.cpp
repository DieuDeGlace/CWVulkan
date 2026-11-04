#include "UseCreatureCollection.h"
#include "UsePrompt.h"
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>

namespace {
template <class T> T read(const uint8_t *record, size_t offset) {
    T value;
    std::memcpy(&value, record + offset, sizeof value);
    return value;
}
template <class T> void set(uint8_t *record, size_t offset, T value) {
    std::memcpy(record + offset, &value, sizeof value);
}
} // namespace

int useCreatureCollectionTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *label) {
        if (!passed) {
            std::cout << "FAIL use collection: " << label << '\n';
            ++failures;
        }
    };
    usecollection::Collection copies;
    auto source = worldgen::newCreature(-5);
    std::fill(source.begin() + 0x10, source.end(), 0xab);
    auto *copy = copies.add(source.data());
    check(copy != source.data() && read<float>(copy, 0x9a0) == 1.0f && read<int64_t>(copy, 8) == -5,
          "new copies use the existing creature constructor and retain defaults outside assigned state");
    std::fill(copy, copy + worldgen::CREATURE_SIZE, 0xcd);
    set(copy, 8, int64_t(-5));
    const worldgen::CreatureRecord before(copy, copy + worldgen::CREATURE_SIZE);
    auto *updated = copies.add(source.data());
    check(copy == updated && copies.size() == 1, "duplicate IDs update the same independently owned object");
    // The expected complement comes from the spec's unassigned-gap table,
    // independent of the producer's positive transfer spans.
    std::vector<bool> assigned(worldgen::CREATURE_SIZE, false);
    std::fill(assigned.begin() + 0x10, assigned.begin() + 0x980, true);
    const auto gap = [&](size_t first, size_t last) {
        std::fill(assigned.begin() + first, assigned.begin() + last + 1, false);
    };
    for (const auto [first, last] : {std::pair<size_t, size_t>{0x61, 0x63},
                                     {0x69, 0x6b},
                                     {0x7d, 0x7f},
                                     {0x12a, 0x12b},
                                     {0x14e, 0x14f},
                                     {0x19e, 0x19f},
                                     {0x971, 0x973}})
        gap(first, last);
    for (int slot = -1; slot < 11; ++slot) {
        const size_t base = slot < 0 ? 0x1d8 : 0x278 + size_t(slot) * 0xa0;
        gap(base + 1, base + 3);
        gap(base + 0x15, base + 0x17);
        gap(base + 0x9e, base + 0x9f);
    }
    std::fill(assigned.begin() + 0x998, assigned.begin() + 0x9a0, true);
    std::fill(assigned.begin() + 0xc40, assigned.begin() + 0xc48, true);
    bool exact = true;
    for (size_t i = 0; i < assigned.size(); ++i)
        exact &= updated[i] == (assigned[i] ? source[i] : before[i]);
    check(exact,
          "every assigned value transfers while padding, inventory, character and pose state retain prior bytes");

    usecollection::Collection ordered;
    auto negative = worldgen::newCreature(-4), middle = worldgen::newCreature(2), last = worldgen::newCreature(7);
    ordered.add(last.data());
    ordered.add(negative.data());
    ordered.add(middle.data());
    for (const auto [index, id] : {std::pair<int32_t, int64_t>{-1, 7}, {-3, -4}, {3, -4}, {5, 7}, {INT32_MIN, 2}}) {
        ordered.selectionIndex = index;
        check(read<int64_t>(ordered.wrapped(), 8) == id && ordered.selectionIndex == index,
              "signed64 key order and local modulo selection preserve the original index");
    }
    auto player = worldgen::newCreature(100);
    const auto prepare = [&](int ability, int64_t target, float elapsed) {
        player[0x68] = uint8_t(ability);
        set(player.data(), 0x6c, elapsed);
        set(player.data(), 0x1a8, target);
    };
    std::vector<int> sounds;
    const auto resolve = [&](int64_t id) -> const uint8_t * { return id == 2 ? middle.data() : nullptr; };
    ordered.selectedId = 123;
    ordered.selectionIndex = 99;
    prepare(0x96, 2, 400.0f);
    ordered.complete(player.data(), 400.0f, resolve, [&](int id) { sounds.push_back(id); });
    check(player[0x68] == 0x96 && sounds.empty(), "effect-threshold equality does not complete");
    set(player.data(), 0x6c, std::numeric_limits<float>::quiet_NaN());
    ordered.complete(player.data(), 400.0f, resolve, {});
    check(player[0x68] == 0x96, "unordered elapsed comparison does not complete");
    prepare(0x96, 2, std::nextafter(400.0f, 500.0f));
    const auto liveBefore = middle;
    ordered.complete(player.data(), 400.0f, resolve, [&](int id) {
        sounds.push_back(id);
        check(id == 0x30 && player[0x68] == 0 && read<float>(player.data(), 0x6c) == 0 && ordered.find(2) &&
                  ordered.selectedId == 123 && ordered.selectionIndex == 99,
              "add/update and ability resets precede direct sound; selection writes follow it");
    });
    check(ordered.selectedId == 2 && ordered.selectionIndex == 1 && middle == liveBefore &&
              read<int64_t>(player.data(), 0x1a8) == 2,
          "completion selects signed-order position without mutating the live target or clearing its ID");
    prepare(0x97, 2, 401);
    ordered.complete(player.data(), 400, resolve, [&](int id) {
        sounds.push_back(id);
        check(id == 0x3c && !ordered.find(2) && ordered.selectedId == 2 && ordered.selectionIndex == 1,
              "owned-copy removal precedes sound and wrapped reselection");
    });
    check(ordered.selectedId == 7 && ordered.selectionIndex == 1 && middle == liveBefore,
          "removal selects the retained index in the shorter list and preserves the World creature");
    for (const int ability : {0x96, 0x97}) {
        prepare(ability, 999, 401);
        const auto count = sounds.size();
        ordered.complete(player.data(), 400, resolve, [&](int id) { sounds.push_back(id); });
        check(player[0x68] == 0 && read<float>(player.data(), 0x6c) == 0 && ordered.selectedId == 7 &&
                  ordered.selectionIndex == 1 && sounds.size() == count && read<int64_t>(player.data(), 0x1a8) == 999,
              "missing target still resets ability/time without sound or selection repair");
    }
    usecollection::Collection single;
    single.add(last.data());
    single.selectionIndex = -1;
    single.selectedId = 7;
    prepare(0x97, 7, 401);
    single.complete(player.data(), 400, {}, {});
    check(single.size() == 0 && single.selectedId == 0 && single.selectionIndex == -1 && !single.wrapped(),
          "removing the final owned copy selects none without changing the index");

    useprompt::Scene scene;
    scene.player = player.data();
    scene.abilityDuration = 1000;
    scene.abilityEffectTime = 400;
    scene.collection = &ordered;
    scene.creatureById = resolve;
    scene.collectionSound = [&](int id) { sounds.push_back(id); };
    for (int gate = 0; gate < 3; ++gate) {
        prepare(0x96, 2, 500);
        scene.dodgeHeld = gate == 0;
        scene.walkHeld = gate == 1;
        scene.dialogOpen = gate == 2;
        useprompt::KeyState key;
        useprompt::decide(scene, {}, key);
        check(player[0x68] == 0x96, "earlier prompt suppression gates defer automatic completion");
    }
    scene.dodgeHeld = scene.walkHeld = scene.dialogOpen = false;
    prepare(0x96, 2, 500);
    set(player.data(), 0x180, 0.0f);
    useprompt::KeyState key;
    const auto prompt = useprompt::decide(scene, {}, key);
    check(player[0x68] == 0 && ordered.selectedId == 2 && prompt.action == useprompt::Action::Revive && !prompt.pressed,
          "actual prompt completes after effect time without a Use edge, then continues to Revive");
    const auto soundCount = sounds.size();
    useprompt::decide(scene, {}, key);
    check(sounds.size() == soundCount, "later prompt visits do not replay a completed collection sound");

    prepare(0x96, 2, 1001);
    set(player.data(), 0x40, 1.0f);
    set(player.data(), 0x5c, uint32_t(4));
    set(player.data(), 0x12c, 0.0f);
    set(player.data(), 0x9a0, 1.0f);
    check(useprompt::decide(scene, {}, key).action == useprompt::Action::Climb && player[0x68] == 0x96,
          "admitted Climb preempts collection completion even beyond both time thresholds");
    set(player.data(), 0x40, 0.0f);
    set(player.data(), 0x5c, uint32_t(0));

    auto zero = worldgen::newCreature(0);
    set(zero.data(), 0x10, int64_t(100000) * 65536);
    set(zero.data(), 0x180, 0.0f);
    scene.creatureById = [&](int64_t id) -> const uint8_t * { return id == 0 ? zero.data() : nullptr; };
    prepare(0x96, 0, 500);
    useprompt::decide(scene, {}, key);
    check(ordered.find(0) && ordered.selectedId == 0 && player[0x68] == 0,
          "actual completion passes zero IDs to World lookup without target distance or health admission");
    std::cout << "Use collection: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
