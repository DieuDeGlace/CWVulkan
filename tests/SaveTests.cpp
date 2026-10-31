// Checks of the game's save format (src/NativeSaves.*) against docs/specs/native-saves.md: the
// character blob v11 and the world state v44 written and read back byte for byte, the reading
// quirks the spec lists, zlib, the four table statements, deleting characters, and importing.
// Every save here is a test character made by the writer itself, in a temporary folder; no real
// save is opened.
#include "NativeSaves.h"
#include "NativeProgress.h"
#include "Gear.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using namespace nativesaves;

namespace {
int failures = 0;
#define SAVE_CHECK(...)                                                                            \
    do {                                                                                           \
        if (!(__VA_ARGS__)) {                                                                      \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #__VA_ARGS__);                    \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

// A test character with something in every field.
Character testCharacter() {
    Character c;
    c.position = {(int64_t)0x20020000000ll + 12345, (int64_t)0x20020000000ll - 777, (int64_t)140 << 16};
    c.orientation = {0.0f, 0.0f, 45.0f};
    c.health = 123.5f;
    c.level = 7;
    c.experience = 333;
    c.classType = 1;
    c.specialization = 0;
    c.field9a4 = 1.0f;
    c.field9a8 = 0.25f;
    c.equipment[2] = makeItem(4, 0, 1);
    c.equipment[7] = makeItem(3, 17, 2);
    std::memcpy(&c.equipment[7][item::MODIFIER], "\x39\x30\x00\x00", 4);
    c.name = "warrior-berserker";
    c.looks.set(1, 1, 3, 5, {255, 200, 50});
    c.inventory[0].push_back({1, makeItem(3, 0, 1)});
    c.inventory[0].push_back({1, makeItem(3, 13, 1)});
    c.inventory[2].push_back({5, makeItem(1, 1)});
    c.gold = 42;
    c.fieldAb0 = 9;
    c.recipes = {makeItem(1, 1), makeItem(1, 2), makeItem(1, 7)};
    c.discoveredZones = {{3, 4}, {-2, 7}};
    c.set48 = {{1, 1}};
    c.discoveredLands = {{0, 0}, {5, -5}};
    c.map88 = {{{1, 2}, 3}};
    c.map98 = {{{-1, -2}, 4}};
    c.usedCells = {{100, 200}, {-100, 5}};
    c.setB8 = {{7, 7}};
    c.setC8 = {{8, 8}};
    c.finishedMissions = {{9, 9}};
    c.missionTimers = {{{9, 9}, 1500.5f}};
    c.worldSeed = 0;
    c.worldName = "";
    c.flightPoints = {{1000, 2000}, {-30, 40}};
    c.homeCell = {2097280, 2097283};
    Buff b;
    b.bytes[0] = 5;
    float strength = 2.0f, ms = 9000.0f;
    std::memcpy(&b.bytes[4], &strength, 4), std::memcpy(&b.bytes[8], &ms, 4);
    c.buffs.push_back(b);
    c.regionValues = {{{3, 3}, 77}};
    c.stars[starGroup(5000, 9000)].push_back({0, 5000, 9000, 120});
    c.stars[starGroup(-5000, 100)].push_back({1, -5000, 100, 90});
    c.regionHistory = {{2, 3}};
    c.purchases[123456789012345ll] = {makeItem(1, 4)};
    return c;
}

WorldState testWorld() {
    WorldState w;
    w.day = 135;
    w.timeMs = 50400000.0f;
    w.dead = {-5, 7, 0x7ffffd407fffffc2ll};
    w.rewards = {11, 12};
    ZoneState z;
    z.flags = ZoneState::GROUND_ITEMS_SAVED | ZoneState::STATIC_OBJECTS_SAVED;
    Record r{};
    r[0] = 3, r[0x28] = 1;
    z.groundItems = {r};
    z.staticObjects = {r, r};
    BlockChange change;
    const int32_t at[3] = {10, 20, 30};
    std::memcpy(change.bytes.data(), at, 12);
    change.bytes[0x12] = 200;
    z.blockChanges = {change};
    z.placeCounters = {{{1, 1}, 2}};
    z.deadTracked = 3;
    w.zones[{-1, 4}] = z;
    w.zones[{2, 2}] = ZoneState{};
    return w;
}

void inventorySlotLoading() {
    Gear::Record record;
    for (size_t i = 0; i < record.size(); ++i)
        record[i] = (uint8_t)(i * 37 + 11);
    record[0] = 15;
    record[0x14] = 201; // retain the byte; display-color clamping is a separate operation
    record[0x9d] = 3;
    auto editable = Gear::fromRecord(record);
    SAVE_CHECK(Gear::toRecord(editable) == record);
    editable.rarity = 4;
    editable.region[0] = -7;
    editable.cubes[3] ^= 0x55;
    auto expected = record;
    expected[0x14] = 4;
    const int32_t changedRegion = -7;
    std::memcpy(expected.data() + 0xc, &changedRegion, 4);
    expected[Gear::CUBES_AT + 3] ^= 0x55;
    SAVE_CHECK(Gear::toRecord(editable) == expected);
    for (size_t tabCount : {0u, 2u, 8u}) {
        Character original = testCharacter();
        original.inventory.assign(tabCount, {});
        if (tabCount) {
            Item stale = makeItem(3, 17, 2);
            stale[0x9e] = 0x77;
            original.inventory[0] = {{0, stale}, {-1, makeItem(22, 2)}, {3, makeItem(1, 7)}};
            original.inventory.back().push_back({2, makeItem(21, 5)});
        }
        const auto blob = writeCharacter(original);
        Character loaded;
        readCharacter(blob, loaded);
        SAVE_CHECK(loaded.inventory.size() == tabCount);
        SAVE_CHECK(writeCharacter(loaded) == blob); // tabs, holes, quantities and all item bytes
    }
    Character source;
    source.inventory[0] = {{0, makeItem(3, 17, 2)}, {0, makeItem(1, 7)}, {-1, makeItem(22, 2)}};
    const auto view = nativeprogress::toProgress(source, {}, {}, {});
    SAVE_CHECK(view.found.empty());
    SAVE_CHECK(view.bag.size() == 1 && view.bag[0].category == 22 && view.bag[0].count == -1);
    Character pets;
    auto stalePet = makeItem(20, 1), livePet = stalePet;
    stalePet[0x9d] = livePet[0x9d] = 1;
    stalePet[0x20] = 'X';
    livePet[0x20] = 'N';
    pets.inventory[4] = {{0, stalePet}, {1, livePet}};
    const auto petView = nativeprogress::toProgress(pets, {}, {}, {});
    std::string notes;
    nativeprogress::fromProgress(petView, nullptr, 0, {}, pets, notes);
    SAVE_CHECK(pets.inventory[4].size() == 1 && pets.inventory[4][0].item[0x20] == 'N');
    Character distinct;
    auto first = makeItem(20, 1), second = first;
    first[0x9d] = second[0x9d] = 1;
    first[0x20] = 'A';
    second[0x20] = 'B';
    distinct.inventory[4] = {{1, first}, {1, second}};
    auto remaining = nativeprogress::toProgress(distinct, {}, {}, {});
    remaining.bag.erase(remaining.bag.begin()); // remove A; the remaining live item is B
    nativeprogress::fromProgress(remaining, nullptr, 0, {}, distinct, notes);
    SAVE_CHECK(distinct.inventory[4].size() == 1 && distinct.inventory[4][0].item[0x20] == 'B');
    Character weapons;
    auto common = makeItem(3, 17, 2), rare = common;
    common[item::RARITY] = 1;
    rare[item::RARITY] = 3;
    weapons.inventory[0] = {{1, common}, {1, rare}};
    auto carried = nativeprogress::toProgress(weapons, {}, {}, {});
    carried.found.erase(carried.found.begin());
    nativeprogress::fromProgress(carried, nullptr, 0, {}, weapons, notes);
    SAVE_CHECK(weapons.inventory[0].size() == 1 && weapons.inventory[0][0].item[item::RARITY] == 3);
    Character canonical = testCharacter();
    const auto unknown = makeItem(15, 8);
    canonical.inventory.resize(8);
    canonical.inventory[7] = {{0, first}, {-1, unknown}};
    auto live = nativeprogress::toProgress(canonical, {}, {}, {});
    live.inventoryAuthoritative = true;
    live.nativeInventory->inventory.gold = 71;
    live.nativeInventory->inventory.fieldAb0 = 19;
    live.nativeInventory->inventory.tabs[7][1].count = 4;
    Character written;
    nativeprogress::fromProgress(live, nullptr, canonical.health, {}, written, notes);
    SAVE_CHECK(written.inventory.size() == 8 && written.inventory[7][0].item == first && written.inventory[7][0].count == 0 &&
               written.inventory[7][1].count == 4 && written.inventory[7][1].item == unknown && written.gold == 71 &&
               written.fieldAb0 == 19);
}

void characterRoundTrip() {
    const Character c = testCharacter();
    const std::vector<uint8_t> blob = writeCharacter(c);
    Character back;
    readCharacter(blob, back);
    SAVE_CHECK(writeCharacter(back) == blob); // byte for byte
    SAVE_CHECK(back.name == c.name && back.level == 7 && back.experience == 333 && back.gold == 42);
    SAVE_CHECK(back.position == c.position && back.health == 123.5f && back.classType == 1);
    SAVE_CHECK(back.looks.people() == 1 && back.looks.sex() == 1 && back.looks.face() == 3 && back.looks.haircut() == 5);
    SAVE_CHECK(back.looks.hairColour()[1] == 200);
    SAVE_CHECK(back.inventory.size() == 6 && back.inventory[0].size() == 2 && back.inventory[2][0].count == 5);
    SAVE_CHECK(back.usedCells == c.usedCells && back.missionTimers == c.missionTimers && back.flightPoints == c.flightPoints);
    SAVE_CHECK(back.homeCell == c.homeCell && back.buffs.size() == 1 && back.purchases == c.purchases);
    SAVE_CHECK(back.stars.size() == 2 && back.regionValues == c.regionValues);
    // The layout of §9: the equipment starts at 0x3e (slot 2, the chest, at 0x3e + 2 * 0xa0)
    // and the name's length is at 0x71e.
    SAVE_CHECK(blob[0x3e + 2 * 0xa0] == 4);
    int32_t nameLength;
    std::memcpy(&nameLength, &blob[0x71e], 4);
    SAVE_CHECK(nameLength == (int32_t)c.name.size());
    int32_t version;
    std::memcpy(&version, blob.data(), 4);
    SAVE_CHECK(version == CHARACTER_VERSION);
}

void worldRoundTrip() {
    const WorldState w = testWorld();
    const std::vector<uint8_t> blob = writeWorld(w);
    WorldState back;
    readWorld(blob, back);
    SAVE_CHECK(writeWorld(back) == blob);
    SAVE_CHECK(back.day == 135 && back.timeMs == 50400000.0f && back.dead == w.dead && back.rewards == w.rewards);
    SAVE_CHECK(back.zones.size() == 2 && back.zones[{-1, 4}].staticObjects.size() == 2 && back.zones[{-1, 4}].deadTracked == 3);
    SAVE_CHECK(back.zones[{-1, 4}].blockChanges[0].block()[2] == 30 && back.zones[{-1, 4}].blockChanges[0].now()[0] == 200);
    // Day at +4, time at +8, then the dead creatures (§9).
    int32_t day, deadCount;
    std::memcpy(&day, &blob[4], 4), std::memcpy(&deadCount, &blob[12], 4);
    SAVE_CHECK(day == 135 && deadCount == 3);
    int64_t firstDead;
    std::memcpy(&firstDead, &blob[16], 8);
    SAVE_CHECK(firstDead == -5); // sorted as signed numbers

    // Another version: day and time are taken, the rest stays as the new world had it.
    std::vector<uint8_t> other = blob;
    const int32_t v43 = 43;
    std::memcpy(other.data(), &v43, 4);
    WorldState fresh;
    fresh.dead = {99};
    readWorld(other, fresh);
    SAVE_CHECK(fresh.day == 135 && fresh.dead == std::set<int64_t>{99} && fresh.zones.empty());
}

void readingQuirks() {
    const Character c = testCharacter();
    const std::vector<uint8_t> blob = writeCharacter(c);
    // Truncated in the middle of the looks: everything from there on keeps its default.
    std::vector<uint8_t> cut(blob.begin(), blob.begin() + 0x71e + 4 + (long long)c.name.size() + 10);
    Character partial;
    readCharacter(cut, partial);
    SAVE_CHECK(partial.name == c.name && partial.level == 7);
    SAVE_CHECK(partial.looks.people() == 0 && partial.gold == 0 && partial.recipes.empty() && partial.inventory.size() == 6);
    SAVE_CHECK(partial.inventory[0].empty() && partial.usedCells.empty());
    // Truncated inside a list: the entries that fit are kept, the reading stops at the end.
    Character listCut;
    readCharacter(std::vector<uint8_t>(blob.begin(), blob.end() - 0xa0 / 2), listCut);
    SAVE_CHECK(listCut.purchases.size() == 1 && listCut.purchases.begin()->second.empty());
    SAVE_CHECK(listCut.regionHistory == c.regionHistory);
    // An empty blob: a default character.
    Character none;
    readCharacter({}, none);
    SAVE_CHECK(none.level == 1 && none.name.empty() && none.version == CHARACTER_VERSION);

    // Gold: negative loads as 1,410,065,408; any positive amount stays.
    Character poor = c;
    poor.gold = -5;
    Character loaded;
    readCharacter(writeCharacter(poor), loaded);
    SAVE_CHECK(loaded.gold == 1410065408);
    poor.gold = 2147483647;
    Character rich;
    readCharacter(writeCharacter(poor), rich);
    SAVE_CHECK(rich.gold == 2147483647);

    // The recipe every load adds: {type 1, subtype 7}, once, at the end.
    Character learner = c;
    learner.recipes = {makeItem(1, 1)};
    ensureLoadRecipe(learner);
    SAVE_CHECK(learner.recipes.size() == 2 && learner.recipes[1][0] == 1 && learner.recipes[1][4] == 7);
    ensureLoadRecipe(learner);
    SAVE_CHECK(learner.recipes.size() == 2);
    auto paddedRecipe = makeItem(1, 7);
    paddedRecipe[1] = paddedRecipe[0x15] = paddedRecipe[0x16] = paddedRecipe[0x17] = 0x5a;
    paddedRecipe[0x90] = 0x71;
    learner.recipes = {paddedRecipe};
    ensureLoadRecipe(learner);
    SAVE_CHECK(learner.recipes == std::vector<Item>{paddedRecipe});

    // Version 9: no sets block; the fields after it come straight after the recipes.
    std::vector<uint8_t> v9;
    {
        Character old = c;
        old.discoveredZones.clear(), old.set48.clear(), old.discoveredLands.clear(), old.map88.clear(), old.map98.clear();
        old.usedCells.clear(), old.setB8.clear(), old.setC8.clear(), old.finishedMissions.clear(), old.missionTimers.clear();
        const std::vector<uint8_t> full = writeCharacter(old);
        // The sets block is 9 empty counts and one empty map: 40 bytes after the recipes, before the tail.
        const size_t recipesEnd = full.size() - 40 - (4 + 4 + 4 + 2 * 8 + 8 + 4 + 0x18 + 4 + 12 + 4 + 2 * 16 + 4 + 8 + 4 + 8 + 4 + 0xa0);
        v9.assign(full.begin(), full.begin() + (long long)recipesEnd);
        v9.insert(v9.end(), full.begin() + (long long)recipesEnd + 40, full.end());
        const int32_t nine = 9;
        std::memcpy(v9.data(), &nine, 4);
    }
    Character nine;
    readCharacter(v9, nine);
    SAVE_CHECK(nine.version == 9);
    SAVE_CHECK(nine.flightPoints == c.flightPoints);
    SAVE_CHECK(nine.homeCell == c.homeCell);
    SAVE_CHECK(nine.purchases == c.purchases);

    // Stars group by truncation toward zero: x -1 falls in group 0, as x 1 does.
    SAVE_CHECK((starGroup(-1, 5000) == Cell{0, 1}) && (starGroup(-4097, -1) == Cell{-1, 0}));
    // A duplicate key keeps the first.
    {
        Character dup;
        dup.missionTimers = {{{1, 1}, 5.0f}};
        std::vector<uint8_t> b = writeCharacter(dup);
        Character d2;
        readCharacter(b, d2);
        SAVE_CHECK(d2.missionTimers.at({1, 1}) == 5.0f);
    }
    // The race of each people choice (§3.5).
    SAVE_CHECK(raceOf(0, 0) == 2 && raceOf(1, 1) == 1 && raceOf(2, 0) == 9 && raceOf(3, 1) == 12 && raceOf(4, 0) == 4 &&
               raceOf(5, 1) == 8 && raceOf(6, 0) == 15 && raceOf(7, 1) == 14 && raceOf(8, 0) == -1);
}

void zlibStreams() {
    const std::vector<uint8_t> blob = writeCharacter(testCharacter());
    const std::vector<uint8_t> packed = compress(blob);
    SAVE_CHECK(packed.size() > 2 && packed[0] == 0x78 && packed[1] == 0x9c); // level 6
    SAVE_CHECK(inflate(packed) == blob);
    // A damaged stream gives what inflated before the damage (possibly nothing).
    std::vector<uint8_t> damaged(packed.begin(), packed.begin() + (long long)packed.size() / 2);
    const std::vector<uint8_t> some = inflate(damaged);
    SAVE_CHECK(some.size() <= blob.size() && std::equal(some.begin(), some.end(), blob.begin()));
    SAVE_CHECK(inflate({1, 2, 3}).empty());
}

fs::path scratchFolder(const char *name) {
    const fs::path dir = fs::temp_directory_path() / ("cubevulkan-save-tests-" + std::string(name));
    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir, ec);
    return dir;
}

void characterFile() {
    const fs::path dir = scratchFolder("characters");
    {
        CharacterStore store;
        SAVE_CHECK(store.open(dir / "characters.db"));
        SAVE_CHECK(store.count() == 0 && !store.slot());
        for (int i = 0; i < 4; i++) {
            Character c = testCharacter();
            c.name = "test" + std::to_string(i);
            c.level = 10 + i;
            SAVE_CHECK(store.create(c) == i);
            SAVE_CHECK(store.writeNotes(i, "notes " + std::to_string(i)));
        }
        SAVE_CHECK(store.count() == 4 && store.slot() == 3);
        store.setSlot(2);
        // The stored value is the zlib stream of the blob.
        std::vector<uint8_t> raw;
        SAVE_CHECK(store.database().read("1", raw) && raw[0] == 0x78);
        Character second;
        SAVE_CHECK(store.load(1, second) && second.name == "test1" && second.level == 11);
        // Delete the second: the keys above it move down one; "slot" stays.
        SAVE_CHECK(store.remove(1));
        SAVE_CHECK(store.count() == 3 && store.slot() == 2);
        const std::vector<std::string> keys = store.database().keys();
        SAVE_CHECK((keys == std::vector<std::string>{"0", "1", "2", "cubevulkan:0", "cubevulkan:1", "cubevulkan:2", "num", "slot"}));
        Character c;
        SAVE_CHECK(store.load(0, c) && c.name == "test0");
        SAVE_CHECK(store.load(1, c) && c.name == "test2");
        SAVE_CHECK(store.load(2, c) && c.name == "test3");
        SAVE_CHECK(!store.load(3, c));
        std::string notes;
        SAVE_CHECK(store.readNotes(1, notes) && notes == "notes 2");
        // Deleting the last leaves the rest.
        SAVE_CHECK(store.remove(2) && store.count() == 2 && !store.remove(5));
        // A character loaded from the file gets the load recipe even when it was saved without.
        Character bare = testCharacter();
        bare.recipes.clear();
        SAVE_CHECK(store.save(0, bare));
        SAVE_CHECK(store.load(0, c) && c.recipes.size() == 1 && c.recipes[0][0] == 1 && c.recipes[0][4] == 7);
        // A negative selection saves nothing.
        SAVE_CHECK(!store.save(-1, bare));
    }
    { // world.db
        SAVE_CHECK(writeWorldFile(dir / "world.db", testWorld()));
        WorldState w;
        SAVE_CHECK(readWorldFile(dir / "world.db", w) && w.day == 135 && w.dead.size() == 3);
    }
    { // the statements: an empty value is a null of length 0; rename; remove
        BlobDatabase db;
        SAVE_CHECK(db.open(dir / "statements.db"));
        SAVE_CHECK(db.write("a", {}) && db.write("a", {1, 2}) && db.rename("a", "b"));
        std::vector<uint8_t> v;
        SAVE_CHECK(!db.read("a", v) && db.read("b", v) && v.size() == 2 && db.remove("b") && !db.read("b", v));
        SAVE_CHECK(db.write("empty", {}) && db.read("empty", v) && v.empty());
    }
    { // read-only: nothing can be written
        BlobDatabase db;
        SAVE_CHECK(db.openReadOnly(dir / "characters.db"));
        SAVE_CHECK(!db.write("num", {9, 0, 0, 0}) && !db.remove("0") && !db.rename("0", "x"));
        int32_t n;
        SAVE_CHECK(db.readInt("num", n) && n == 2);
        SAVE_CHECK(!BlobDatabase().openReadOnly(dir / "missing.db"));
    }
    std::error_code ec;
    fs::remove_all(dir, ec);
}

void importing() {
    const fs::path game = scratchFolder("import-game"), own = scratchFolder("import-own");
    {
        CharacterStore store;
        store.open(game / "characters.db");
        store.create(testCharacter());
        writeWorldFile(game / "world.db", testWorld());
    }
    auto bytesOf = [](const fs::path &p) {
        std::ifstream in(p, std::ios::binary);
        return std::vector<char>((std::istreambuf_iterator<char>(in)), {});
    };
    const auto before = bytesOf(game / "characters.db"), worldBefore = bytesOf(game / "world.db");
    const auto when = fs::last_write_time(game / "characters.db");
    std::string report;
    SAVE_CHECK(importGameSaves(game, own, report));
    SAVE_CHECK(bytesOf(game / "characters.db") == before && bytesOf(game / "world.db") == worldBefore);
    SAVE_CHECK(fs::last_write_time(game / "characters.db") == when);
    {
        CharacterStore copy;
        Character c;
        SAVE_CHECK(copy.open(own / "characters.db") && copy.count() == 1 && copy.load(0, c) && c.name == "warrior-berserker");
        WorldState w;
        SAVE_CHECK(readWorldFile(own / "world.db", w) && w.day == 135);
    }
    // A second import keeps CubeVulkan's own files; the same folder is refused.
    SAVE_CHECK(!importGameSaves(game, own, report) && report.find("kept") != std::string::npos);
    SAVE_CHECK(!importGameSaves(game, game, report));
    std::error_code ec;
    fs::remove_all(game, ec), fs::remove_all(own, ec);
}

void zoneStates() {
    WorldState w;
    ZoneLists generated;
    generated.groundItems.resize(3);
    generated.staticObjects.resize(2);
    // Nothing changed: no record is made.
    recordZone(w, {1, 1}, generated);
    SAVE_CHECK(w.zones.empty());
    // A chest opened: the static objects are recorded (bit 1), with the dead count.
    ZoneLists played = generated;
    played.staticObjects[1][0x28] = 1;
    played.staticObjectsChanged = true;
    played.deadTracked = 2;
    recordZone(w, {1, 1}, played);
    SAVE_CHECK(w.zones.size() == 1 && w.zones[{1, 1}].flags == ZoneState::STATIC_OBJECTS_SAVED);
    SAVE_CHECK(w.zones[{1, 1}].deadTracked == 2 && w.zones[{1, 1}].groundItems.empty());
    // Applied to the zone generated again: the objects are replaced (and changed again), the
    // ground items stay as generated.
    ZoneLists again = generated;
    BlockChange change;
    change.bytes[0x12] = 7;
    w.zones[{1, 1}].blockChanges = {change, change};
    int setBlocks = 0;
    applyStoredZone(w, {1, 1}, again, false, [&](const BlockChange &b) { setBlocks += b.now()[0] == 7; });
    SAVE_CHECK(again.staticObjects[1][0x28] == 1 && again.staticObjectsChanged && again.groundItems.size() == 3);
    SAVE_CHECK(setBlocks == 2 && again.deadTracked == 2 && again.blockChanges.size() == 2);
    ZoneLists server = generated;
    applyStoredZone(w, {1, 1}, server, true, nullptr);
    SAVE_CHECK(server.staticObjects[1][0x28] == 0 && server.deadTracked == 0);
    // The world reset empties the dead and the zones, keeps the rewards.
    w.dead = {1}, w.rewards = {2};
    resetWorld(w);
    SAVE_CHECK(w.dead.empty() && w.zones.empty() && w.rewards.size() == 1);
    Character c = testCharacter();
    c.inventory[1].push_back({1, makeItem(0x16, 0)});
    resetCharacterForWorld(c);
    SAVE_CHECK(c.inventory[1].empty() && c.map88.empty() && c.finishedMissions.empty() && c.purchases.empty());
    SAVE_CHECK(!c.usedCells.empty());
    // Create Character's own steps: level 1, the home cell used.
    Character fresh;
    fresh.level = 9, fresh.homeCell = {4, 5};
    prepareNewCharacter(fresh);
    SAVE_CHECK(fresh.level == 1 && fresh.usedCells.count({4, 5}));
}

} // namespace

int saveFormatTests() {
    failures = 0;
    inventorySlotLoading();
    characterRoundTrip();
    worldRoundTrip();
    readingQuirks();
    zlibStreams();
    characterFile();
    importing();
    zoneStates();
    return failures;
}
