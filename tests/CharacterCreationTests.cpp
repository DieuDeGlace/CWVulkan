#include "CharacterCreation.h"
#include "Frontend.h"
#include "Spawned.h"
#include "TalkGen.h"
#include <algorithm>
#include <bit>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>

namespace {
template <class T> T read(const uint8_t *record, size_t at) {
    T value;
    std::memcpy(&value, record + at, sizeof value);
    return value;
}
template <class T> void put(uint8_t *record, size_t at, const T &value) {
    std::memcpy(record + at, &value, sizeof value);
}
bool sameInventory(const std::vector<std::vector<nativesaves::Stack>> &a,
                   const std::vector<std::vector<nativesaves::Stack>> &b) {
    if (a.size() != b.size())
        return false;
    for (size_t tab = 0; tab < a.size(); ++tab) {
        if (a[tab].size() != b[tab].size())
            return false;
        for (size_t slot = 0; slot < a[tab].size(); ++slot)
            if (a[tab][slot].count != b[tab][slot].count || a[tab][slot].item != b[tab][slot].item)
                return false;
    }
    return true;
}
void starterReplacement(const auto &check) {
    for (uint8_t cls : {uint8_t(1), uint8_t(2), uint8_t(3), uint8_t(4)}) {
        nativesaves::Character current;
        current.classType = cls;
        current.gold = 421;
        current.fieldAb0 = -91;
        frontend::giveStartingKit(current, 0, {7, -8});
        check(current.gold == 0, "each style rebuild replaces the starter aggregate's gold");
        check(current.fieldAb0 == 0, "each style rebuild replaces its separate aggregate counter");
        check(current.recipes.size() == (cls == 1 ? 19u : 20u),
              "new starter templates retain the exact nineteen/twenty defined entries");
        check(current.inventory.size() == 6 && current.inventory[0].size() == 10 && current.inventory[1].size() == 10 &&
                  current.inventory[2].size() == 10 && current.inventory[3].size() == 30 &&
                  current.inventory[4].size() == 30 && current.inventory[5].empty(),
              "the fresh starter aggregate retains all six category capacities");
        const size_t last = current.recipes.size() - 3;
        check(current.recipes[last] == nativesaves::makeItem(11, 10, 1) &&
                  current.recipes[last + 1] == nativesaves::makeItem(11, 10, 2) &&
                  current.recipes[last + 2] == nativesaves::makeItem(11, 10, 11),
              "the final template material order is exactly1,2,11");
    }
}
void coreFootprint(const auto &check) {
    worldgen::CreatureRecord source(worldgen::CREATURE_SIZE), destination(worldgen::CREATURE_SIZE, 0xa7);
    for (size_t i = 0; i < source.size(); ++i)
        source[i] = uint8_t(i * 17 + 3);
    characterstate::copyCore(destination.data(), source.data());
    size_t copied = 0, omitted = 0;
    bool okay = true;
    for (size_t at = 0; at < destination.size(); ++at) {
        bool gap = (at >= 0x61 && at <= 0x63) || (at >= 0x69 && at <= 0x6b) || (at >= 0x7d && at <= 0x7f) ||
                   (at >= 0x12a && at <= 0x12b) || (at >= 0x14e && at <= 0x14f) || (at >= 0x19e && at <= 0x19f) ||
                   (at >= 0x971 && at <= 0x973);
        if (at >= 0x1d8 && at < 0x958) {
            const size_t byte = (at - 0x1d8) % 160;
            gap = (byte >= 1 && byte <= 3) || (byte >= 0x15 && byte <= 0x17) || byte >= 0x9e;
        }
        const bool inside = at >= 0x10 && at < 0x980;
        const bool transfer = inside && !gap;
        if (transfer)
            ++copied;
        else if (inside)
            ++omitted;
        okay = okay && destination[at] == (transfer ? source[at] : uint8_t(0xa7));
    }
    check(okay && copied == 2302 && omitted == 114,
          "the complete core copy writes2302 defined bytes and retains114 omissions plus outer identity/state");
}
void retainedCreation(const auto &check) {
    ExtractedWorld terrain;
    terrain.openGenerated(0, 0);
    SpawnedCreatures actor;
    actor.setStoredZones(terrain, nullptr, false);
    actor.setNativePlayer(true);
    nativesaves::Character old;
    old.name = "OLD";
    old.position = {101, 203, 307};
    old.health = 37;
    old.field9a4 = 2.25f;
    old.field9a8 = -0.0f;
    actor.seedPlayerSave(std::make_shared<const nativesaves::Character>(old));
    actor.setPlayer(terrain, {}, 0, {}, 100, 1, false);
    auto *entity = actor.localPlayer();
    auto *record = entity->record;
    const int64_t identity = read<int64_t>(record, 8);
    entity->counted = {71};
    entity->struck = {72};
    entity->skills = {73};
    entity->summonRaces = {74};
    entity->group = {75};
    entity->interactions.push_back({});
    entity->aggro[1] = 2;
    entity->damage[3] = 4;
    entity->cooldowns[5] = 6;
    entity->buffs.resize(1);
    put(record, 0x150, int32_t(-7));
    put(record, 0x154, int32_t(9));
    put(record, 0x968, uint64_t(0x1122334455667788));
    put(record, 0x12c, uint32_t(0x12345678));
    put(record, 0x174, uint32_t(0x76543210));
    std::memset(record + 0x1d8, 0x5a, 160);
    const std::array<size_t, 10> gaps{0x61, 0x69, 0x7d, 0x12a, 0x14e, 0x19e, 0x971, 0xbbc, 0xbcc, 0xbe8};
    for (size_t at : gaps)
        record[at] = 0xa7;
    auto data = std::make_shared<nativesaves::Character>(old);
    data->usedCells.insert({99, 101});
    data->finishedMissions.insert({8, 9});
    data->worldName = "old-data";
    bool searchedAfterReset = false;
    check(actor.beginCharacterCreation(
              data,
              [&] {
                  searchedAfterReset = read<float>(record, 0x180) == 100 && read<uint64_t>(record, 0x968) == 0 &&
                                       read<int32_t>(record, 0x144) == 1 && record[0x14c] == 1 && record[0x6d8] == 3 &&
                                       read<int32_t>(record, 0x6dc) == 0 && record[0x6f4] == 1 &&
                                       read<int64_t>(record, 0x10) == 101 && data->usedCells.count({99, 101}) == 1;
                  return charactercreation::Placement{
                      {-11, 13}, {(int64_t(1) << 35) + 1, -(int64_t(1) << 35) + 3, 47 * 65536 + 5}};
              }),
          "native creation enters on the existing owned actor");
    check(searchedAfterReset,
          "reset and preliminary100HP/equipment precede home search; prior data survives until placement");
    check(actor.localPlayer() == entity && actor.localPlayer()->record == record &&
              read<int64_t>(record, 8) == identity,
          "creation retains the Entity and its registration identity");
    check(entity->counted == std::set<int64_t>{71} && entity->struck == std::set<int64_t>{72} &&
              entity->skills == std::vector<int32_t>{73} && entity->summonRaces == std::vector<int32_t>{74} &&
              entity->group == std::set<int64_t>{75},
          "separate identifier sets, skill/summon ranges and group survive the selected reset");
    bool kept = read<uint32_t>(record, 0x12c) == 0x12345678 && read<uint32_t>(record, 0x174) == 0x76543210;
    for (size_t at : gaps)
        kept = kept && record[at] == 0xa7;
    for (size_t i = 0; i < 160; ++i)
        kept = kept && record[0x1d8 + i] == 0x5a;
    check(kept,
          "reset-alone preserves active item, omitted widths and outer state rather than applying decoder defaults");
    check(entity->interactions.empty() && entity->aggro.empty() && entity->damage.empty() &&
              entity->cooldowns.empty() && entity->buffs.empty(),
          "the traced transient work/maps/buffs clear without clearing separate lifetimes");
    check(data->homeCell == nativesaves::Cell{-11, 13} && data->position[0] == (int64_t(1) << 35) + 1 &&
              data->usedCells.empty() && data->finishedMissions.empty() && data->worldName.empty() &&
              data->field9a4 == 2.25f && std::bit_cast<uint32_t>(data->field9a8) == 0x80000000u,
          "fresh CharacterData follows exact placement while9A4/9A8 survive");
    talkgen::CRand random;
    random.seed(0);
    check(read<uint64_t>(record, 0x968) == 0 &&
              read<uint32_t>(data->equipment[7].data(), 8) == uint32_t(random.next()) &&
              read<int32_t>(data->equipment[7].data(), 12) == -7 && read<int32_t>(data->equipment[7].data(), 16) == 9,
          "the immediate rebuild seeds zero after search and stamps surviving150/154 words");
    if (worldgen::creatureTablesLoaded())
        check(read<uint16_t>(record, 0x90) == 0x5ad && read<uint16_t>(record, 0x92) == 0x5b3 &&
                  read<uint16_t>(record, 0x94) == 0x265 && read<uint16_t>(record, 0x96) == 0x267 &&
                  read<uint16_t>(record, 0x98) == 1,
              "owner tables match the independently specified default Human/Male selectors");
    entity->inventorySource([&](nativeai::InventorySlot slot) {
        slot.setCount(9);
        return false;
    });
    check(data->inventory[0][0].count == 9 && entity->inventory.empty(),
          "preview inventory is the current owned canonical source");
    constexpr uint64_t account = 0x11223344aabbccddull;
    int providerCalls = 0;
    actor.publishPlayerAccount([&] { ++providerCalls; return account; });
    actor.publishPlayerAccount([&] { ++providerCalls; return 0x9988776655443322ull; });
    check(providerCalls == 1, "the actual actor facade skips provider reads after full-width publication");
    put(record, 0x150, int32_t(123));
    put(record, 0x154, int32_t(-456));
    put(record, 0x148, int32_t(73));
    record[0x14d] = 1;
    put(record, 0x180, -7.25f);
    data->gold = 42;
    data->fieldAb0 = -19;
    std::memset(record + 0xa0c, 0x6b, 160);
    put(record, 0xa08, int32_t(7));
    data->looks.bytes[5] = 0xa7;
    data->looks.bytes[0x13] = 0xb8;
    nativesaves::Looks looks;
    looks.set(3, 1, 0, 0, {7, 8, 9});
    check(actor.rebuildCharacterCreation(*data, looks, 4), "style rebuild uses the same live actor");
    random.seed(uint32_t(account));
    check(read<uint64_t>(record, 0x968) == account &&
              read<uint32_t>(data->equipment[6].data(), 8) == uint32_t(random.next()) &&
              read<int32_t>(data->equipment[6].data(), 12) == 123 &&
              read<int32_t>(data->equipment[6].data(), 16) == -456,
          "ordinary account publication is once-if-zero; later rebuild uses current low32/current region words");
    check(data->health == actor.playerMaxHealth() && data->health > 0 && data->experience == 73 &&
              data->specialization == 1,
          "style HP is freshly computed after items while XP/specialization survive");
    check(data->gold == 0 && data->fieldAb0 == 0 && read<int32_t>(record, 0xa08) == 0 && data->looks.bytes[5] == 0xa7 &&
              data->looks.bytes[0x13] == 0xb8,
          "style replaces counters/cursor while preserving omitted CharacterData header bytes");
    bool cursor = true;
    for (size_t byte = 0; byte < 160; ++byte) {
        const bool gap = (byte >= 1 && byte <= 3) || (byte >= 0x15 && byte <= 0x17) || byte >= 0x9e;
        cursor = cursor && record[0xa0c + byte] == (gap ? 0x6b : 0);
    }
    check(cursor, "style clears all defined cursor-item fields and retains padding");

    const std::array<int64_t, 3> latest{-(int64_t(1) << 39) + 7, (int64_t(1) << 39) + 9, 0};
    std::memcpy(record + 0x10, latest.data(), 24);
    put(record, 0x180, -0.0f);
    put(record, 0x144, int32_t(19));
    put(record, 0x148, INT32_MIN);
    put(record, 0x34, 17.5f);
    data->usedCells.insert({5, -7});
    data->finishedMissions.insert({17, 19});
    data->regionHistory = {{3, 4}, {-5, 6}, {3, 4}};
    data->worldName = "live-data";
    data->worldSeed = 73;
    entity->buffs.resize(1);
    entity->buffs[0][0] = 19;
    entity->buffs[0][23] = 0xa7;
    actor.restoreMapHistory(data->regionHistory);
    check(actor.confirmCharacterCreation(*data, "A\xe2\x82\xac"
                                                "B"),
          "Create confirms from the live owner");
    check(data->position == latest && std::bit_cast<uint32_t>(data->health) == 0x80000000u && data->level == 1 &&
              data->experience == INT32_MIN && data->specialization == 1 &&
              data->name == std::string("A\xac"
                                        "B"),
          "Create preserves latest raw position/HP/XP/spec and narrows UTF16 units while setting only level/name");
    check(data->usedCells == std::set<nativesaves::Cell>{{-11, 13}, {5, -7}} && data->buffs.size() == 1 &&
              data->buffs[0].bytes[23] == 0xa7 && data->finishedMissions.count({17, 19}) == 1,
          "Create inserts the live home key without dropping full CharacterData or buffs");
    auto snapshot = actor.creationSelectionSnapshot(*data);
    check(snapshot && snapshot->character.usedCells.empty() && snapshot->character.finishedMissions.empty() &&
              snapshot->character.regionHistory.empty() && snapshot->character.worldName.empty() &&
              snapshot->character.worldSeed == 0 && snapshot->character.recipes == data->recipes &&
              sameInventory(snapshot->character.inventory, data->inventory),
          "the later selection snapshot copies header/templates/inventory but leaves other data constructor-empty");
    check(snapshot && snapshot->character.field9a4 == 1 && snapshot->character.field9a8 == 0 &&
              read<int64_t>(snapshot->record.data(), 8) == 0 && read<float>(snapshot->record.data(), 0x34) == 17.5f &&
              std::memcmp(snapshot->record.data() + 0xb10, latest.data(), 24) == 0,
          "list snapshot retains its own identity/bookkeeping defaults and copied core/drawn position");
    check(actor.localPlayer() == entity && actor.playerHealth() == -0.0f && read<float>(record, 0x34) == 17.5f,
          "snapshot creation never replaces or reinitializes the live actor");

    namespace fs = std::filesystem;
    const auto folder =
        fs::temp_directory_path() /
        fs::path("CubeVulkan-creation-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directory(folder);
    const auto file = folder / "characters.db";
    {
        nativesaves::CharacterStore store;
        check(store.open(file), "disposable character store opens");
        const int index = store.create(*data);
        nativesaves::Character loaded;
        check(index == 0 && store.count() == 1 && store.slot() == std::optional<int>(0) && store.load(index, loaded),
              "ordinary first creation stores count1, numbered0 and slot0");
        check(nativesaves::writeCharacter(loaded) == nativesaves::writeCharacter(*data) &&
                  loaded.usedCells == data->usedCells && loaded.finishedMissions == data->finishedMissions,
              "disk receives the full live CharacterData snapshot rather than the limited list owner");
    }
    fs::remove(file);
    fs::remove(folder);
    nativeprogress::ProgressFile progress;
    SaveGame restored;
    progress.restore(restored, *data, "", {}, "creation-profile",
                     [](const nativesaves::Item &) { return std::string(); });
    check(!progress.isOpen() && restored.level == 1 && restored.xp == INT32_MIN && restored.specialization == 1 &&
              restored.nativeInventory && sameInventory(restored.nativeInventory->inventory.tabs, data->inventory) &&
              restored.world == "creation-profile",
          "Main can restore owned creation data without opening/reading a database or resetting its actor");
    check(actor.confirmCharacterCreation(*data, std::u16string_view(u"\u0100A")) && data->name.empty() &&
              data->position == latest && std::bit_cast<uint32_t>(data->health) == 0x80000000u,
          "literal wide-length-two caption confirms its first low-byte zero without revalidating or resetting the actor");
    const std::u16string rawCaption{char16_t(0xd801),u'B'};
    check(actor.confirmCharacterCreation(*data, rawCaption) && data->name == std::string({char(1),'B'}) &&
              actor.localPlayer() == entity && data->level == 1,
          "raw UTF16 surrogate units reach the live actor through the same bounded low-byte commit");
}
void lazyAccountPublication(const auto &check) {
    auto record = worldgen::newCreature(7);
    put(record.data(), 0x968, uint64_t(0));
    int calls = 0;
    uint64_t result = 0;
    const charactercreation::AccountProvider provider = [&] { ++calls; return result; };
    charactercreation::publishAccount(record, provider);
    charactercreation::publishAccount(record, provider);
    check(calls == 2 && read<uint64_t>(record.data(), 0x968) == 0,
          "zero provider results remain eligible for a fresh query at the next reached ordinary update");
    result = 0x1122334400000000ull;
    charactercreation::publishAccount(record, provider);
    result = 0xffeeddccbbaa9988ull;
    charactercreation::publishAccount(record, provider);
    check(calls == 3 && read<uint64_t>(record.data(), 0x968) == 0x1122334400000000ull,
          "a nonzero high half suppresses the entire provider even when its seed low word is zero");
    put(record.data(), 0x968, uint64_t(0));
    charactercreation::publishAccount(record, [&] {
        ++calls;
        put(record.data(), 0x968, uint64_t(0x76543210));
        return uint64_t(0x8877665544332211ull);
    });
    check(calls == 4 && read<uint64_t>(record.data(), 0x968) == 0x8877665544332211ull,
          "the provider's returned full word is stored without repeating the admission test after its call");
    put(record.data(), 0x968, uint64_t(0));
    charactercreation::publishAccount(record, {});
    check(read<uint64_t>(record.data(), 0x968) == 0,
          "a missing engine provider is a safe adapter rather than an invented native account");
    SpawnedCreatures absent;
    absent.publishPlayerAccount(provider);
    absent.setNativePlayer(true);
    absent.publishPlayerAccount(provider);
    check(calls == 4, "an unavailable actor facade never queries the provider");
}
void paletteDrawState(const auto &check) {
    frontend::Look look;
    look.reset();
    check(look.preview == std::array<uint8_t, 3>{255, 255, 255},
          "synchronous new selection retains initial white working bytes before any actual palette draw");
    check(look.drawPalette({-100, -100}) && look.preview == std::array<uint8_t, 3>{255, 200, 50},
          "the first admitted off-grid draw restores base color and requests its one changed-color rebuild");
    check(!look.drawPalette({-100, -100}), "an unchanged actual draw does not request another inventory/HP rebuild");
    struct Entry { int column, row; std::array<uint8_t, 3> bytes; };
    const std::array<Entry, 10> entries{{
        {0, 0, {51, 0, 0}}, {0, 1, {114, 0, 0}}, {0, 2, {178, 0, 0}},
        {0, 3, {255, 0, 0}}, {0, 4, {255, 63, 63}}, {0, 5, {255, 127, 127}},
        {0, 6, {255, 191, 191}}, {9, 3, {0, 63, 255}},
        {11, 0, {12, 0, 51}}, {15, 2, {89, 89, 89}}
    }};
    for (const auto &entry : entries) {
        const glm::vec2 point{13.f + 17.f * entry.column + 8.5f, 197.f + 17.f * entry.row + 8.5f};
        look.drawPalette(point);
        check(look.preview == entry.bytes && look.committed == std::array<uint8_t, 3>{255, 200, 50},
              "independent palette row/column literals retain float scaling and truncation without committing base");
        check(!look.drawPalette(point), "identical hovered working bytes suppress a repeat draw-triggered rebuild");
    }
    look.committed = look.preview; // separate native color-commit callback
    check(!look.drawPalette({-100, -100}), "committing the retained working triple makes later off-grid draws unchanged");
    look.preview = {9, 8, 7};
    look.arrow(frontend::ROW_CLASS, 1);
    check(look.record().hairColour() == std::array<uint8_t, 3>{9, 8, 7},
          "an arrow callback uses current working bytes without performing a palette draw first");
}

} // namespace
int characterCreationTests() {
    int failures = 0;
    const auto check = [&](bool okay, const char *message) {
        if (!okay) {
            ++failures;
            std::printf("FAIL character creation: %s\n", message);
            std::fflush(stdout);
        }
    };
    starterReplacement(check);
    lazyAccountPublication(check);
    paletteDrawState(check);
    coreFootprint(check);
    retainedCreation(check);
    check(charactercreation::nameBytes("\xc3\xa9\xf0\x9f\x98\x80"
                                       "Z") == std::string("\xe9="),
          "narrowing retains each surrogate low byte and stops the Entity name at the first zero byte");
    check(charactercreation::nameBytes("\xc4\x80"
                                       "A")
                  .empty() &&
              charactercreation::nameBytes(std::string(40, 'x')).size() == 15,
          "embedded low-byte zero is preserved and oversized inputs use the explicit safe bound");
    check(charactercreation::nameBytes(std::u16string_view(u"\u00e9\u20acZ")) == std::string("\xe9\xacZ") &&
              charactercreation::nameBytes(std::u16string_view(u"\u0100A")).empty() &&
              charactercreation::nameBytes(std::u16string(40,u'X')).size() == 15,
          "caption-unit bridge retains per-unit low bytes, C-string termination and the established safe bound");
    std::printf("Character creation: live reset, style and commit: %d failure(s)\n", failures);
    return failures;
}
