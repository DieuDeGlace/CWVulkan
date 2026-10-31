// CubeVulkan's progress in the game's save format; see NativeProgress.h.
#include "NativeProgress.h"
#include "ItemTypes.h"

#include "Gear.h"
#include "NativePlayer.h"
#include "NativeRules.h"
#include "worldgen/CreatureTables.h"
#include "worldgen/Generator.h"
#include "worldgen/Looks.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
using nativesaves::Character;
using nativesaves::Item;
namespace field = nativesaves::item;

namespace nativeprogress {

void resetDay(SaveGame &progress) {
    progress.perPlayerTaken.clear();
    progress.finishedMissions.clear();
    progress.purchases.clear();
    if (progress.inventoryAuthoritative && progress.nativeInventory)
        progress.nativeInventory->inventory.clearType(0x16);
    progress.bag.erase(std::remove_if(progress.bag.begin(), progress.bag.end(),
                                      [](const SaveGame::Bagged &item) { return item.category == 0x16; }),
                       progress.bag.end());
}

namespace {
constexpr double UNITS = 65536.0;      // world units in a block
constexpr float RECORD_ABOVE_FEET = 1.0f; // SpawnedCreatures::NATIVE_FEET
constexpr float LOCAL_Z_OFFSET = 128.0f;  // ExtractedWorld::Z_OFFSET
} // namespace

glm::vec3 localFeet(const std::array<int64_t, 3> &p, const Frame &frame) {
    return {(float)((double)(p[0] - frame.originX) / UNITS), (float)((double)(p[1] - frame.originY) / UNITS),
            (float)((double)p[2] / UNITS + LOCAL_Z_OFFSET - RECORD_ABOVE_FEET)};
}

std::array<int64_t, 3> gamePosition(glm::vec3 feet, const Frame &frame) {
    return {(int64_t)std::llround((double)feet.x * UNITS) + frame.originX, (int64_t)std::llround((double)feet.y * UNITS) + frame.originY,
            (int64_t)std::llround(((double)feet.z + RECORD_ABOVE_FEET - LOCAL_Z_OFFSET) * UNITS)};
}

// --- Sprites and looks -----------------------------------------------------------------------

bool SpriteTable::load(const fs::path &extractDir) {
    std::ifstream in(extractDir / "sprites.txt");
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream l(line);
        int id;
        std::string path;
        if (!(l >> id >> path) || id < 0)
            continue;
        if (path.rfind("sprites/", 0) == 0)
            path = path.substr(8);
        if (path.size() > 4 && path.compare(path.size() - 4, 4, ".cub") == 0)
            path.resize(path.size() - 4);
        if ((size_t)id >= names.size())
            names.resize((size_t)id + 1);
        names[(size_t)id] = path;
        ids.emplace(path, id);
    }
    return !names.empty();
}

std::string SpriteTable::name(int id) const { return id >= 0 && (size_t)id < names.size() ? names[(size_t)id] : std::string(); }
bool SpriteTable::has(const std::string &n) const { return ids.count(n) != 0; }

const char *peopleWord(int32_t people) {
    static const char *const WORDS[8] = {"human", "elf", "dwarf", "orc", "goblin", "lizard", "undead", "frogman"};
    return people >= 0 && people < 8 ? WORDS[people] : "";
}

namespace {
int peopleOfWord(const std::string &word) {
    for (int p = 0; p < 8; p++)
        if (word == peopleWord(p))
            return p;
    return -1;
}

// "elf-head-f03" -> the people "elf", the sex 1 and the number 3; false for another name.
bool splitSprite(const std::string &sprite, const char *part, int &people, int &sex, int &number) {
    const std::string marker = std::string("-") + part + "-";
    const size_t at = sprite.find(marker);
    if (at == std::string::npos || sprite.size() < at + marker.size() + 2)
        return false;
    people = peopleOfWord(sprite.substr(0, at));
    const char g = sprite[at + marker.size()];
    sex = g == 'f' ? 1 : 0;
    number = std::atoi(sprite.c_str() + at + marker.size() + 1);
    return people >= 0 && (g == 'm' || g == 'f') && number > 0;
}

// The people choice and sex of a race id (the inverse of nativesaves::raceOf).
bool peopleOfRace(int32_t race, int32_t &people, int &sex) {
    for (int32_t p = 0; p < 8; p++)
        for (int s = 0; s < 2; s++)
            if (nativesaves::raceOf(p, s) == race) {
                people = p, sex = s;
                return true;
            }
    return false;
}
} // namespace

bool looksFromSprites(const std::string &race, const std::string &gender, const std::string &head, const std::string &hair,
                      glm::u8vec3 colour, nativesaves::Looks &out) {
    int people = peopleOfWord(race), sex = gender == "f" ? 1 : 0, face = 1, haircut = 1, p2, s2;
    if (splitSprite(head, "head", p2, s2, face))
        people = people < 0 ? p2 : people, sex = s2;
    else
        face = 1;
    if (!splitSprite(hair, "hair", p2, s2, haircut))
        haircut = 1;
    if (people < 0)
        return false;
    out.set(people, sex, face - 1, haircut - 1, {colour.r, colour.g, colour.b});
    return true;
}

bool looksFromSnapshot(const fs::path &snapshot, nativesaves::Looks &out) {
    std::ifstream in(snapshot);
    if (!in)
        return false;
    int32_t race = -1;
    int r = 255, g = 255, b = 255;
    std::string head, hair, line;
    while (std::getline(in, line)) {
        if (line.rfind("race=", 0) == 0)
            race = std::atoi(line.c_str() + 5);
        else if (line.rfind("hair_color=", 0) == 0)
            std::sscanf(line.c_str() + 11, "%d %d %d", &r, &g, &b);
        else if (line.rfind("sprite face ", 0) == 0 || line.rfind("sprite hair ", 0) == 0) {
            const size_t slash = line.rfind('/');
            std::string name = slash == std::string::npos ? "" : line.substr(slash + 1);
            if (name.size() > 4)
                name.resize(name.size() - 4);
            (line[7] == 'f' ? head : hair) = name;
        }
    }
    int32_t people;
    int sex;
    if (!peopleOfRace(race, people, sex))
        return false;
    int p2, s2, face = 1, haircut = 1;
    if (!splitSprite(head, "head", p2, s2, face))
        face = 1;
    if (!splitSprite(hair, "hair", p2, s2, haircut))
        haircut = 1;
    out.set(people, sex, face - 1, haircut - 1, {(uint8_t)r, (uint8_t)g, (uint8_t)b});
    return true;
}

bool appearanceOf(const nativesaves::Looks &looks, std::array<uint8_t, 0xb0> &out) {
    const int32_t race = nativesaves::raceOf(looks.people(), looks.sex());
    if (race < 0 || !worldgen::creatureTablesLoaded())
        return false;
    // The looks routine's draws, answered from the record: the colour's three (blue, green,
    // red, as it stores them), then the head and hair picks (§5.4 of frontend-characters.md).
    const auto colour = looks.hairColour();
    const int answers[5] = {colour[2], colour[1], colour[0], looks.face() % 100, looks.haircut() % 100};
    int next = 0;
    auto scripted = [&]() { return next < 5 ? answers[next++] : 0; };
    worldgen::Appearance a = worldgen::Appearance::constructed();
    if (!worldgen::makeLooks(worldgen::sharedCreatureTables(), race, false, worldgen::Draw(scripted), a))
        return false;
    std::memcpy(out.data(), a.bytes.data(), out.size());
    return true;
}

const char *raceWord(int32_t race) {
    switch (race) {
    case 0: case 1: return "Elf";
    case 2: case 3: return "Human";
    case 4: case 5: return "Goblin";
    case 7: case 8: return "Lizardman";
    case 9: case 10: return "Dwarf";
    case 11: case 12: return "Orc";
    case 15: case 16: return "Undead";
    default: return "";
    }
}

bool entryFor(const Character &c, int slot, const std::vector<CharacterEntry> &recorded, const SpriteTable &sprites,
              CharacterEntry &out) {
    const CharacterEntry *base = nullptr;
    for (const CharacterEntry &r : recorded)
        if (!r.created && !r.native && r.classType == c.classType &&
            (!base || (r.specialization == c.specialization && base->specialization != c.specialization)))
            base = &r;
    if (!base)
        return false;
    out = CharacterEntry{};
    out.dir = base->dir;
    out.name = c.name;
    out.classType = c.classType;
    out.specialization = c.specialization;
    out.level = std::max(1, c.level);
    out.race = nativesaves::raceOf(c.looks.people(), c.looks.sex());
    out.raceName = peopleWord(c.looks.people());
    out.native = true;
    out.slot = slot;
    out.created = true;
    out.id = "slot-" + std::to_string(slot);
    const auto colour = c.looks.hairColour();
    out.hairColor = glm::u8vec3(colour[0], colour[1], colour[2]);
    out.hasAppearance = appearanceOf(c.looks, out.appearance);
    // The sprites the recorded body wears for this look (the old drawing without the game's pose).
    if (out.hasAppearance && sprites.loaded()) {
        auto model = [&](size_t at) {
            int16_t id;
            std::memcpy(&id, &out.appearance[at], 2);
            return sprites.name(id);
        };
        out.head = model(worldgen::Appearance::HEAD), out.hair = model(worldgen::Appearance::HAIR);
        out.hand = model(worldgen::Appearance::HANDS), out.foot = model(worldgen::Appearance::FEET);
    } else {
        const std::string race = out.raceName, g = c.looks.sex() ? "f" : "m";
        auto numbered = [](int n) { return (n < 9 ? "0" : "") + std::to_string(n + 1); };
        out.head = race + "-head-" + g + numbered(c.looks.face());
        out.hair = race + "-hair-" + g + numbered(c.looks.haircut());
        out.hand = sprites.has(race + "-hand") ? race + "-hand" : "hand2";
        out.foot = sprites.has(race + "-foot") ? race + "-foot" : "foot";
    }
    return true;
}

float maxHealth(const Character &c) {
    nativeplayer::Who who;
    who.race = std::max(0, nativesaves::raceOf(c.looks.people(), c.looks.sex()));
    who.cls = c.classType, who.spec = c.specialization, who.level = std::max(1, c.level);
    std::array<nativeplayer::Item, 11> worn{};
    for (size_t s = 0; s < worn.size(); s++)
        std::memcpy(worn[s].data(), c.equipment[s].data(), 0xa0);
    const worldgen::CreatureRecord r = nativeplayer::record(who, worn);
    return nativerules::maxHealth(r.data());
}

// --- Items -----------------------------------------------------------------------------------

Item gameItem(int category, int id, int material, int rarity, uint32_t modifier, const int region[2], int formula) {
    const nativeplayer::Item made = nativeplayer::item(category, id, material, rarity, modifier, region[0], region[1], formula);
    Item out;
    std::memcpy(out.data(), made.data(), out.size());
    return out;
}

namespace {

int32_t i32At(const Item &it, size_t at) {
    int32_t v;
    std::memcpy(&v, &it[at], 4);
    return v;
}
bool isEmpty(const Item &it) { return it[field::TYPE] == 0; }
bool isGear(int category) { return category >= 3 && category <= 9; }

// The inventory tab an item goes in (the game's tab of an item, as the inventory screen sorts).
int tabOf(int category) {
    return itemtypes::inventoryTab((uint8_t)category);
}

// The equipment slot gear of a category is found for (as gameplay places it).
int slotFor(int category) {
    switch (category) {
    case 3: return Gear::WeaponRight;
    case 4: return Gear::Chest;
    case 5: return Gear::Hands;
    case 6: return Gear::Feet;
    case 7: return Gear::Shoulder;
    case 8: return Gear::Neck;
    case 9: return Gear::RingRight;
    default: return Gear::SLOTS;
    }
}

// The fields gameplay keeps of an item, to recognise a saved record (and keep its attributes).
struct Identity {
    int category, id, material;
    uint32_t modifier;
    auto operator<=>(const Identity &) const = default;
};
Identity identityOf(const Item &it) {
    return {it[field::TYPE], i32At(it, field::SUBTYPE), it[field::MATERIAL], (uint32_t)i32At(it, field::MODIFIER)};
}

// The records already in the character, to be reused for the same items.
class ItemPool {
  public:
    explicit ItemPool(const Character &c) {
        for (const Item &it : c.equipment)
            if (!isEmpty(it))
                pool.emplace(identityOf(it), it);
        for (const auto &tab : c.inventory)
            for (const auto &s : tab)
                if (s.count != 0)
                    pool.emplace(identityOf(s.item), s.item);
    }
    Item take(const Item &fresh) {
        auto it = pool.find(identityOf(fresh));
        if (it == pool.end())
            return fresh;
        Item kept = it->second;
        pool.erase(it);
        return kept;
    }

  private:
    std::multimap<Identity, Item> pool;
};

Item fromGear(const Gear::Item &it) {
    return Gear::toRecord(it);
}
// The record kept for a piece of gear (the saved one when there is one), with the upgrade cubes
// it has now.
Item keptGear(ItemPool &pool, const Gear::Item &it) {
    const Item fallback = pool.take(fromGear(it));
    Gear::Item current = it;
    if (!current.storedRecord)
        current.storedRecord = fallback;
    return Gear::toRecord(current);
}

Item keptBag(ItemPool &pool, const SaveGame::Bagged &bag) {
    const auto fallback = pool.take(gameItem(bag.category, bag.id, bag.material, bag.rarity, bag.modifier, bag.region, bag.formula));
    auto item = Gear::fromRecord(bag.storedRecord.value_or(fallback));
    item.category = bag.category, item.id = bag.id, item.material = bag.material, item.rarity = bag.rarity;
    item.modifier = bag.modifier, item.region[0] = bag.region[0], item.region[1] = bag.region[1];
    if (bag.storedRecord || bag.formula != 0)
        item.formula = bag.formula;
    return Gear::toRecord(item);
}

Item keptFound(ItemPool &pool, const SaveGame::Found &found) {
    const auto fallback =
        pool.take(gameItem(found.category, found.id, found.material, found.rarity, found.modifier, found.region, found.formula));
    auto item = Gear::fromRecord(found.storedRecord.value_or(fallback));
    item.category = found.category, item.id = found.id, item.material = found.material, item.rarity = found.rarity;
    item.modifier = found.modifier, item.region[0] = found.region[0], item.region[1] = found.region[1];
    if (found.storedRecord || found.formula != 0)
        item.formula = found.formula;
    if (found.storedRecord || found.cubeCount != 0)
        item.cubes = found.cubes, item.cubeCount = found.cubeCount;
    return Gear::toRecord(item);
}

// The notes: what CubeVulkan keeps of a character beyond the game's format.
std::string notesOf(const SaveGame &s) {
    SaveGame n;
    n.hairStyle = s.hairStyle;
    n.quests = s.quests;
    n.marked = s.marked;
    n.takenItems = s.takenItems;
    n.usedObjects = s.usedObjects;
    return n.text();
}

} // namespace

// --- Progress --------------------------------------------------------------------------------

SaveGame toProgress(const Character &c, const std::string &notes, const Frame &frame,
                    const std::function<std::string(const Item &)> &itemSprite) {
    SaveGame s;
    {
        SaveGame n;
        if (!notes.empty() && n.parse(notes)) {
            s.hairStyle = n.hairStyle;
            s.quests = n.quests, s.marked = n.marked, s.takenItems = n.takenItems, s.usedObjects = n.usedObjects;
        }
    }
    s.nativeInventory = inventoryslots::PlayerState::fromCharacter(c);
    s.hasPosition = c.position != std::array<int64_t, 3>{0, 0, 0};
    s.position = localFeet(c.position, frame);
    s.yaw = s.heading = 0; // (the game zeroes the orientation on load)
    const float most = maxHealth(c);
    s.hpFraction = most > 0 ? std::clamp(c.health / most, 0.01f, 1.0f) : 1.0f;
    s.level = std::max(1, c.level);
    s.xp = c.experience;
    s.gold = c.gold;
    s.recipes = c.recipes;
    s.specialization = c.specialization;
    s.hasGear = true;
    auto found = [&](const Item &it, int slot) {
        SaveGame::Found f;
        f.category = it[field::TYPE], f.id = i32At(it, field::SUBTYPE), f.material = it[field::MATERIAL];
        f.rarity = it[field::RARITY], f.modifier = (uint32_t)i32At(it, field::MODIFIER), f.slot = slot;
        f.region[0] = i32At(it, field::REGION), f.region[1] = i32At(it, field::REGION + 4);
        f.sprite = itemSprite ? itemSprite(it) : "";
        std::memcpy(f.cubes.data(), &it[Gear::CUBES_AT], f.cubes.size());
        f.cubeCount = it[Gear::CUBE_COUNT_AT];
        f.formula = i32At(it, field::FORMULA);
        f.storedRecord = it;
        return f;
    };
    for (size_t slot = 0; slot < c.equipment.size(); slot++) {
        const Item &it = c.equipment[slot];
        SaveGame::Worn w;
        w.category = it[field::TYPE], w.id = i32At(it, field::SUBTYPE), w.material = it[field::MATERIAL];
        w.rarity = it[field::RARITY], w.modifier = (uint32_t)i32At(it, field::MODIFIER);
        s.worn.push_back(w);
        if (!isEmpty(it))
            s.found.push_back(found(it, (int)slot));
    }
    for (const auto &tab : c.inventory)
        for (const nativesaves::Stack &stack : tab) {
            const Item &it = stack.item;
            if (stack.count == 0 || isEmpty(it))
                continue;
            if (isGear(it[field::TYPE])) {
                for (int k = 0; k < std::max(1, stack.count); k++)
                    s.found.push_back(found(it, slotFor(it[field::TYPE])));
                continue;
            }
            SaveGame::Bagged b;
            b.category = it[field::TYPE], b.id = i32At(it, field::SUBTYPE), b.material = it[field::MATERIAL];
            b.rarity = it[field::RARITY], b.modifier = (uint32_t)i32At(it, field::MODIFIER), b.count = stack.count;
            b.region[0] = i32At(it, field::REGION), b.region[1] = i32At(it, field::REGION + 4);
            b.formula = i32At(it, field::FORMULA);
            b.storedRecord = it;
            s.bag.push_back(b);
        }
    s.usedCells.assign(c.usedCells.begin(), c.usedCells.end());
    s.discoveredPlaces.assign(c.discoveredZones.begin(), c.discoveredZones.end());
    s.discoveredAreas.assign(c.discoveredLands.begin(), c.discoveredLands.end());
    s.flightPoints = c.flightPoints;
    s.finishedMissions = c.finishedMissions;
    s.solvedMissions = c.setC8;
    s.missionTimers = c.missionTimers;
    s.purchases = c.purchases;
    for (const auto &[block, z] : c.map88) // (the file keeps x and y as the key and z as the value)
        s.perPlayerTaken.insert({block[0], block[1], z});
    s.regionCounters = c.regionValues;
    for (const auto &[group, list] : c.stars)
        for (const nativesaves::Star &star : list)
            s.treasures.push_back({star.x, star.y, star.z});
    s.created.classType = c.classType;
    s.created.specialization = c.specialization;
    s.created.name = c.name;
    return s;
}

void fromProgress(const SaveGame &s, const Gear *gear, float health, const Frame &frame, Character &c, std::string &notes) {
    if (s.hasPosition)
        c.position = gamePosition(s.position, frame);
    c.health = health;
    c.level = std::max(1, s.level);
    c.experience = s.xp;
    c.gold = s.gold;
    if (s.recipes) c.recipes = *s.recipes;
    if (s.specialization >= 0) c.specialization = (uint8_t)s.specialization;
    if (s.inventoryAuthoritative && s.nativeInventory) {
        s.nativeInventory->writeCharacter(c);
    } else {
        ItemPool pool(c);
        std::vector<std::vector<nativesaves::Stack>> inventory(nativesaves::INVENTORY_TABS);
        std::array<Item, nativesaves::EQUIPMENT_SLOTS> equipment{};
        if (gear) {
            for (int slot = 0; slot < Gear::SLOTS && slot < nativesaves::EQUIPMENT_SLOTS; slot++) {
                const Gear::Item &it = gear->equipped((Gear::Slot)slot);
                if (it.category)
                    equipment[(size_t)slot] = keptGear(pool, it);
            }
            for (size_t i = 0; i < gear->itemCount(); i++)
                if (gear->item(i).found && !gear->isWorn(i))
                    inventory[0].push_back({1, keptGear(pool, gear->item(i))});
        } else {
            std::vector<SaveGame::Found> rest = s.found;
            for (size_t slot = 0; slot < s.worn.size() && slot < equipment.size(); slot++) {
                const SaveGame::Worn &w = s.worn[slot];
                if (!w.category)
                    continue;
                int region[2] = {0, 0};
                auto same = std::find_if(rest.begin(), rest.end(), [&](const SaveGame::Found &f) {
                    return f.category == w.category && f.id == w.id && f.material == w.material && f.modifier == w.modifier;
                });
                if (same != rest.end()) {
                    auto current = *same;
                    current.rarity = w.rarity;
                    equipment[slot] = keptFound(pool, current);
                    rest.erase(same);
                } else
                    equipment[slot] = pool.take(gameItem(w.category, w.id, w.material, w.rarity, w.modifier, region));
            }
            for (const SaveGame::Found &f : rest)
                inventory[0].push_back({1, keptFound(pool, f)});
        }
        for (const SaveGame::Bagged &b : s.bag)
            if (b.category)
                inventory[(size_t)tabOf(b.category)].push_back({b.count, keptBag(pool, b)});
        c.equipment = equipment;
        c.inventory = std::move(inventory);
    }
    c.usedCells = std::set<nativesaves::Cell>(s.usedCells.begin(), s.usedCells.end());
    c.discoveredZones = std::set<nativesaves::Cell>(s.discoveredPlaces.begin(), s.discoveredPlaces.end());
    c.discoveredLands = std::set<nativesaves::Cell>(s.discoveredAreas.begin(), s.discoveredAreas.end());
    c.flightPoints = s.flightPoints;
    c.finishedMissions = s.finishedMissions;
    c.setC8 = s.solvedMissions;
    c.missionTimers = s.missionTimers;
    c.purchases = s.purchases;
    c.map88.clear();
    for (const auto &block : s.perPlayerTaken)
        c.map88[{block[0], block[1]}] = block[2];
    c.regionValues = s.regionCounters;
    std::map<nativesaves::Cell, std::vector<nativesaves::Star>> stars;
    for (const auto &t : s.treasures) {
        nativesaves::Star star{0, t[0], t[1], t[2]};
        for (const auto &[group, list] : c.stars) // (the value not established is kept)
            for (const nativesaves::Star &old : list)
                if (old.x == t[0] && old.y == t[1] && old.z == t[2])
                    star.unknown = old.unknown;
        stars[nativesaves::starGroup(t[0], t[1])].push_back(star);
    }
    c.stars = std::move(stars);
    notes = notesOf(s);
}

// --- The old text saves ----------------------------------------------------------------------

int migrateOldSaves(nativesaves::CharacterStore &store, const fs::path &saveDir, const std::string &cloudBundle,
                    const std::vector<CharacterEntry> &recorded, const std::string &worldName, const Frame &frame,
                    const std::function<void(const std::string &)> &log, std::set<int64_t> *worldDead) {
    if (!store.isOpen() || store.count() > 0)
        return 0;
    // Every character's newer copy, by its save id (the file's name, the bundle's section).
    std::map<std::string, SaveGame> saves;
    auto consider = [&](const std::string &id, const std::string &text) {
        SaveGame s;
        if (id.empty() || !s.parse(text))
            return;
        auto [it, added] = saves.emplace(id, s);
        if (!added && s.savedAt > it->second.savedAt)
            it->second = s;
    };
    std::error_code ec;
    for (auto &f : fs::directory_iterator(saveDir, ec))
        if (f.path().extension() == ".sav") {
            std::ifstream in(f.path(), std::ios::binary);
            std::ostringstream all;
            all << in.rdbuf();
            consider(f.path().stem().string(), all.str());
        }
    {
        std::istringstream bundle(cloudBundle);
        std::string line, section, id;
        while (std::getline(bundle, line)) {
            if (line.rfind("character ", 0) == 0) {
                consider(id, section);
                id = line.substr(10), section.clear();
            } else
                section += line + "\n";
        }
        consider(id, section);
    }
    int added = 0;
    for (const auto &[id, s] : saves) {
        Character c;
        const CharacterEntry *rec = nullptr;
        for (const CharacterEntry &r : recorded)
            if (!r.created && !r.native && r.saveId() == id)
                rec = &r;
        if (!s.created.name.empty() && s.created.classType) { // made in the old character creation
            c.name = s.created.name;
            c.classType = (uint8_t)s.created.classType;
            c.specialization = (uint8_t)s.created.specialization;
            looksFromSprites(s.created.race, s.created.gender, s.created.head, s.created.hair, s.created.hairColor, c.looks);
        } else if (rec) {
            c.name = rec->name;
            c.classType = (uint8_t)rec->classType;
            c.specialization = (uint8_t)rec->specialization;
            looksFromSnapshot(rec->dir / "snapshot.txt", c.looks);
        } else {
            log("Saves: old save " + id + " has no character to belong to; left in place");
            continue;
        }
        SaveGame moved = s;
        moved.hasPosition = s.hasPosition && s.world == worldName; // (a position in another world is dropped)
        std::string notes;
        fromProgress(moved, nullptr, 0, frame, c, notes);
        c.health = maxHealth(c) * std::clamp(s.hpFraction, 0.01f, 1.0f);
        const int index = store.create(c);
        if (index < 0)
            continue;
        store.writeNotes(index, notes);
        if (worldDead)
            worldDead->insert(s.killed.begin(), s.killed.end());
        log("Saves: moved the old save " + id + " into characters.db as character " + std::to_string(index));
        added++;
    }
    return added;
}

// --- The session's Save folder -----------------------------------------------------------------

bool ProgressFile::open(const fs::path &folder, const fs::path &gameSave, const fs::path &importFrom, bool writable,
                        const std::function<void(const std::string &)> &log) {
    std::error_code ec;
    if (fs::exists(folder, ec) && fs::exists(gameSave, ec) && fs::equivalent(folder, gameSave, ec)) {
        log("Saves: " + folder.string() + " is the game's own Save folder; nothing is saved");
        return false;
    }
    folder_ = folder;
    std::string why;
    if (!writable) {
        world_ = nativesaves::WorldState{};
        if (fs::exists(folder / "world.db", ec)) {
            nativesaves::BlobDatabase db;
            std::vector<uint8_t> state;
            if (db.openReadOnly(folder / "world.db") && db.read("state", state))
                nativesaves::readWorld(state, world_);
        }
        return store_.openReadOnly(folder / "characters.db", &why);
    }
    fs::create_directories(folder, ec);
    if (!importFrom.empty()) {
        std::string report;
        nativesaves::importGameSaves(importFrom, folder, report);
        std::istringstream lines(report);
        for (std::string line; std::getline(lines, line);)
            log("Saves: import " + line);
    }
    if (!store_.open(folder / "characters.db", &why)) {
        log("Saves: " + why);
        return false;
    }
    world_ = nativesaves::WorldState{};
    if (fs::exists(folder / "world.db", ec) && !nativesaves::readWorldFile(folder / "world.db", world_, &why))
        log("Saves: world.db unread (" + why + ")");
    return true;
}

std::vector<CharacterEntry> ProgressFile::characters(const std::vector<CharacterEntry> &recorded, const SpriteTable &sprites) {
    std::vector<CharacterEntry> out = recorded;
    for (int i = 0; i < store_.count(); i++) {
        Character c;
        CharacterEntry e;
        if (store_.load(i, c) && entryFor(c, i, recorded, sprites, e))
            out.push_back(e);
    }
    return out;
}

void ProgressFile::play(const CharacterEntry &entry) {
    playing_ = entry;
    slot_ = entry.native ? entry.slot : -1;
    if (entry.native)
        return;
    for (int i = 0; i < store_.count() && slot_ < 0; i++) { // a recorded character's own progress
        Character c;
        if (store_.load(i, c) && c.classType == entry.classType && c.name == entry.name)
            slot_ = i;
    }
}

bool ProgressFile::load(SaveGame &out, const Frame &frame, const std::string &worldName,
                        const std::function<std::string(const Item &)> &itemSprite) {
    out = SaveGame{};
    out.day = world_.day;
    out.hour = std::fmod(world_.timeMs / 3600000.0f, 24.0f);
    out.world = worldName;
    out.killed.assign(world_.dead.begin(), world_.dead.end());
    if (slot_ < 0)
        return false;
    Character c;
    if (!store_.load(slot_, c))
        return false;
    std::string notes;
    store_.readNotes(slot_, notes);
    SaveGame s = toProgress(c, notes, frame, itemSprite);
    s.day = out.day, s.hour = out.hour, s.world = worldName, s.killed = out.killed;
    out = std::move(s);
    return true;
}

bool ProgressFile::saveWorld(int32_t day, float timeMs, const std::set<int64_t> &dead) {
    if (!store_.isOpen())
        return false;
    world_.day = std::max(world_.day, day);
    world_.timeMs = timeMs;
    world_.dead.insert(dead.begin(), dead.end());
    return nativesaves::writeWorldFile(folder_ / "world.db", world_);
}

bool ProgressFile::save(const SaveGame &s, const Gear *gear, float health, const Frame &frame, bool world) {
    if (!store_.isOpen())
        return false;
    Character c;
    if (slot_ < 0 || !store_.load(slot_, c)) { // a recorded character's first save
        c = Character{};
        c.name = playing_.name;
        c.classType = (uint8_t)playing_.classType;
        c.specialization = (uint8_t)playing_.specialization;
        looksFromSnapshot(playing_.dir / "snapshot.txt", c.looks);
    }
    std::string notes;
    fromProgress(s, gear, health, frame, c, notes);
    bool ok;
    if (slot_ < 0) {
        slot_ = store_.create(c);
        ok = slot_ >= 0;
    } else
        ok = store_.save(slot_, c);
    if (ok)
        store_.writeNotes(slot_, notes);
    if (!world)
        return ok;
    world_.day = std::max(world_.day, s.day);
    world_.timeMs = std::fmod(std::max(0.0f, s.hour), 24.0f) * 3600000.0f;
    world_.dead.insert(s.killed.begin(), s.killed.end());
    return nativesaves::writeWorldFile(folder_ / "world.db", world_) && ok;
}

} // namespace nativeprogress
