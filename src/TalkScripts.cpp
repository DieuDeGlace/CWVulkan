#include "TalkScripts.h"
#include "TalkGen.h"
#include <algorithm>
#include <cmath>
#include <iterator>
#include <set>
#include <fstream>
#include <sstream>

const char *locationKind(int type) {
    // RVA 0x16b390's switch on type - 1 (types 6 and 11 have no name).
    static const char *kinds[] = {"House", "Farm", "Inn", "Crypt", "Village", "", "Tower", "Castle", "Graveyard",
                                  "Mountain", "", "Cave", "Lake", "LavaLake", "AncientRuins", "CircleOfPower", "ManaPump",
                                  "Crater", "Volcano", "Tree", "Rock", "OldHut", "Island", "Arena", "Forest", "DarkForest",
                                  "EnchantedForest", "Canyon", "Rocks", "Plain", "Hill", "Grove", "Glade", "Camp",
                                  "BanditCamp", "Beach", "Altar", "Sanctuary", "Menhir", "Catacombs", "Sewers", "Mines",
                                  "Shrine", "Grotto", "Pyramid", "Temple", "Vault"};
    return type >= 1 && type <= 47 ? kinds[type - 1] : "";
}

bool TalkScripts::parse(const std::string &text) {
    std::istringstream in(text);
    std::string line;
    int day = 0;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.rfind("# ", 0) == 0) {
            size_t d = line.find("day=");
            if (d != std::string::npos)
                day = std::atoi(line.c_str() + d + 4);
            continue;
        }
        size_t colon = line.find(" :");
        if (line.rfind("id=", 0) != 0 || colon == std::string::npos)
            continue;
        TalkScript s;
        s.day = day;
        std::istringstream head(line.substr(0, colon));
        std::string field;
        while (head >> field) {
            size_t eq = field.find('=');
            if (eq == std::string::npos)
                continue;
            std::string key = field.substr(0, eq), value = field.substr(eq + 1);
            if (key == "id")
                s.id = std::strtoll(value.c_str(), nullptr, 10);
            else if (key == "day") // per line in newer exports; else the "# ... day=" above
                s.day = std::atoi(value.c_str());
            else if (key == "race")
                s.race = std::atoi(value.c_str());
            else if (key == "speaker")
                s.speaker = value;
        }
        // " tok | tok | ... |"
        std::string rest = line.substr(colon + 2);
        size_t at = 0;
        while (at < rest.size()) {
            size_t bar = rest.find('|', at);
            std::string token = rest.substr(at, bar == std::string::npos ? std::string::npos : bar - at);
            size_t a = token.find_first_not_of(' '), b = token.find_last_not_of(' ');
            if (a != std::string::npos)
                s.tokens.push_back(token.substr(a, b - a + 1));
            if (bar == std::string::npos)
                break;
            at = bar + 1;
        }
        if (s.id)
            scripts[s.id][s.day] = std::move(s); // later lines are newer
    }
    return !scripts.empty();
}

bool TalkScripts::load(const std::filesystem::path &file, std::string &error) {
    std::ifstream in(file, std::ios::binary);
    if (!in) {
        error = "no " + file.filename().string() + " (ZoneExtractor writes it as townspeople load)";
        return false;
    }
    std::stringstream all;
    all << in.rdbuf();
    if (!parse(all.str())) {
        error = "no scripts in " + file.filename().string();
        return false;
    }
    return true;
}

const TalkScript *TalkScripts::find(int64_t id) const {
    auto it = scripts.find(id);
    return it == scripts.end() || it->second.empty() ? nullptr : &it->second.rbegin()->second;
}

const TalkScript *TalkScripts::find(int64_t id, int day, bool *exact) const {
    auto it = scripts.find(id);
    if (it == scripts.end() || it->second.empty())
        return nullptr;
    const auto &byDay = it->second;
    auto at = byDay.lower_bound(day);
    if (exact)
        *exact = at != byDay.end() && at->first == day;
    if (at == byDay.end())
        return &byDay.rbegin()->second;
    if (at->first != day && at != byDay.begin() && day - std::prev(at)->first < at->first - day)
        --at;
    return &at->second;
}

std::pair<int, int> TalkScripts::days() const {
    int first = 0, last = -1;
    for (auto &[id, byDay] : scripts)
        if (!byDay.empty()) {
            if (last < first)
                first = byDay.begin()->first, last = byDay.rbegin()->first;
            first = std::min(first, byDay.begin()->first);
            last = std::max(last, byDay.rbegin()->first);
        }
    return {first, last};
}

const char *landKind(int biome) {
    static const char *lands[14] = {"Plains",    "Ocean",    "Hills",  "Woodlands", "Jungle",    "DarkWoods", "SnowForest",
                                    "Mountains", "Wetlands", "Desert", "Savannah",  "Snowlands", "Deadlands", "Firelands"};
    return biome >= 0 && biome < 14 ? lands[biome] : "";
}

namespace {
#include "RaceKeys.inc"

// A creature kind's <name> in the dictionary ("Wolf"), empty for none (the special models).
std::string creatureKey(int kind) {
    for (const auto &[race, key] : RACE_KEYS)
        if (race == kind)
            return key;
    return "";
}

// A specialisation's speech id ("SpecializationBerserker"), by class 1-4 and specialisation 0/1.
std::string specializationId(int cls, int spec) {
    static const char *names[4][2] = {{"Berserker", "Guardian"}, {"Sniper", "Scout"}, {"Fire", "Water"}, {"Assassin", "Ninja"}};
    return cls >= 1 && cls <= 4 && spec >= 0 && spec <= 1 ? std::string("Specialization") + names[cls - 1][spec] : "";
}

// A name's grammar tags ("n p the") as tags with a prefix ("land_the", "mob_p").
void setNameTags(std::set<std::string> &tags, const Speech &speech, const std::string &key, const std::string &prefix) {
    for (const char *tag : {"on", "at", "the", "p", "an"})
        tags.erase(prefix + "_" + tag);
    std::istringstream kindTags(speech.nameTags(key));
    std::string tag;
    while (kindTags >> tag)
        tags.insert(prefix + "_" + tag);
}
} // namespace

TalkPlay playScript(const TalkScript &script, const Speech &speech, std::mt19937 &random, const TalkNames *names) {
    TalkPlay play;
    // What the defines bound so far, and the grammar tags they give.
    std::string zoneKind, zone, item, itemKey, mobName = script.speaker, creature, land, number, specialization;
    std::string mob2Name, creature2, number2, properName; // a second person or creature, number and a name (the lore tales)
    std::set<std::string> tags;
    std::set<std::string> lineTags; // "#inn" and the like: for the next line only
    const auto &t = script.tokens;
    auto annotation = [&](size_t &i, const char *key) { // "key=value" right after a command
        std::string prefix = std::string(key) + "=";
        if (i + 1 < t.size() && t[i + 1].rfind(prefix, 0) == 0)
            return t[++i].substr(prefix.size());
        return std::string();
    };
    auto numberAt = [&](size_t i) { return i < t.size() ? std::atoi(t[i].c_str()) : 0; };
    auto unrendered = [&](const std::string &what) {
        if (std::find(play.unrendered.begin(), play.unrendered.end(), what) == play.unrendered.end())
            play.unrendered.push_back(what);
    };
    // A line of the dictionary with what the defines bound, into `into`; "%" starts a new line.
    auto say = [&](const std::string &id, std::vector<std::string> &into) {
        Speech::Context context;
        context.tags = tags;
        context.tags.insert(lineTags.begin(), lineTags.end());
        std::set<std::string> missing;
        context.value = [&](const std::string &fn, const std::string &args) -> std::string {
            const bool plural = args.find("plural") != std::string::npos;
            std::string value;
            if (fn == "mob_name") value = mobName;
            else if (fn == "mob") value = creature.empty() ? "" : speech.name(creature, plural);
            else if (fn == "zone") value = zone;
            else if (fn == "item") value = itemKey.empty() ? item : speech.name(itemKey, plural);
            else if (fn == "completeitem") value = item;
            else if (fn == "land") value = land;
            else if (fn == "number") value = number;
            else if (fn == "specialization") value = specialization;
            else if (fn == "mob2_name") value = mob2Name;
            else if (fn == "mob2") value = creature2.empty() ? "" : speech.name(creature2, plural);
            else if (fn == "number2") value = number2;
            else if (fn == "name") value = properName;
            if (value.empty())
                missing.insert("@" + fn);
            return value;
        };
        const std::string said = speech.say(id, context, random);
        for (const std::string &fn : missing) // (realms' and rulers' names are not made yet)
            unrendered(id + ": " + fn);
        size_t from = 0;
        while (from <= said.size()) {
            size_t cut = said.find('%', from);
            std::string part = said.substr(from, cut == std::string::npos ? std::string::npos : cut - from);
            size_t a = part.find_first_not_of(' '), b = part.find_last_not_of(' ');
            if (a != std::string::npos)
                into.push_back(part.substr(a, b - a + 1));
            if (cut == std::string::npos)
                break;
            from = cut + 1;
        }
    };
    for (size_t i = 0; i < t.size(); i++) {
        const std::string &token = t[i];
        if (token == "new_page" || token.empty() || token[0] == '!')
            continue; // pauses and emotes: nothing to say
        if (token[0] == '#') {
            lineTags.insert(token.substr(1));
            continue;
        }
        if (token == "define_location" && i + 3 < t.size()) {
            const int x = numberAt(i + 1), y = numberAt(i + 2), kind = numberAt(i + 3);
            zoneKind = locationKind(kind);
            i += 3;
            const std::string proper = annotation(i, "name");
            if (!proper.empty())
                zone = zoneKind.empty() ? proper : speech.place(zoneKind, proper);
            else
                zone = names && names->place ? names->place(x, y, kind) : "";
            for (const char *tag : {"zone_on", "zone_at", "zone_the"})
                tags.erase(tag);
            std::istringstream kindTags(speech.nameTags(zoneKind));
            std::string tag;
            while (kindTags >> tag)
                if (tag == "on" || tag == "at" || tag == "the")
                    tags.insert("zone_" + tag);
        } else if (token == "define_direction" && i + 2 < t.size()) {
            // The game's x is east and y north, as in CubeVulkan's world.
            float dx = (float)numberAt(i + 1), dy = (float)numberAt(i + 2);
            static const char *dirs[] = {"east", "northeast", "north", "northwest", "west", "southwest", "south", "southeast"};
            for (const char *d : dirs)
                tags.erase(d);
            if (std::hypot(dx, dy) > 40) {
                int octant = (int)std::lround(std::atan2(dy, dx) / (3.14159265f / 4));
                tags.insert(dirs[(octant + 8) % 8]);
            }
            i += 2;
        } else if (token == "define_item" && i + 5 < t.size()) {
            // category, id, modifier, material, rarity
            const int numbers[5] = {numberAt(i + 1), numberAt(i + 2), numberAt(i + 3), numberAt(i + 4), numberAt(i + 5)};
            i += 5;
            item = annotation(i, "item");
            itemKey = annotation(i, "key");
            if (item.empty() && itemKey.empty() && names && names->item)
                names->item(numbers, item, itemKey);
            for (const char *tag : {"item_an", "item_p", "completeitem", "completeitem_an"})
                tags.erase(tag);
            if (!itemKey.empty() && speech.name(itemKey) != "") { // a thing the dictionary names
                std::string kt = " " + speech.nameTags(itemKey) + " ";
                if (kt.find(" an ") != std::string::npos) tags.insert("item_an");
                if (kt.find(" p ") != std::string::npos) tags.insert("item_p");
            } else { // equipment: the game's whole name ("well-made Sword")
                tags.insert("completeitem");
                if (!item.empty() && std::string("AEIOUaeiou").find(item[0]) != std::string::npos)
                    tags.insert("completeitem_an");
            }
        } else if (token == "define_mob" && i + 2 < t.size()) {
            const int id = numberAt(i + 1), race = numberAt(i + 2);
            i += 2;
            std::string name = annotation(i, "name");
            if (name.empty() && names && names->person)
                name = names->person(id, race);
            if (!name.empty())
                mobName = name;
            tags.erase("mob_f"), tags.erase("mob_m");
            tags.insert(talkgen::femaleRace(race) ? "mob_f" : "mob_m");
        } else if (token == "define_model" && i + 1 < t.size()) {
            const int kind = numberAt(++i);
            creature = creatureKey(kind);
            if (creature.empty())
                unrendered("define_model " + std::to_string(kind) + " (a special model)");
        } else if (token == "define_land" && i + 2 < t.size()) {
            const int biome = numberAt(i + 1), seed = numberAt(i + 2);
            i += 2;
            const std::string kind = landKind(biome), proper = names && names->land ? names->land(biome, seed) : "";
            land = kind.empty() || proper.empty() ? "" : speech.place(kind, proper);
            setNameTags(tags, speech, kind, "land");
        } else if (token == "define_number" && i + 1 < t.size()) {
            number = t[++i];
        } else if (token == "define_number2" && i + 1 < t.size()) {
            number2 = t[++i];
        } else if (token == "define_name" && i + 1 < t.size()) {
            properName = t[++i]; // (the name itself, not a number)
        } else if (token == "define_mob2" && i + 2 < t.size()) {
            const int id = numberAt(i + 1), race = numberAt(i + 2);
            i += 2;
            mob2Name = names && names->person ? names->person(id, race) : "";
            tags.erase("mob2_f"), tags.erase("mob2_m");
            tags.insert(talkgen::femaleRace(race) ? "mob2_f" : "mob2_m");
        } else if (token == "define_model2" && i + 1 < t.size()) {
            const int kind = numberAt(++i);
            creature2 = creatureKey(kind);
            if (creature2.empty())
                unrendered("define_model2 " + std::to_string(kind) + " (a special model)");
        } else if (token == "define_faction" && i + 1 < t.size()) {
            i += 1; // (factions' names are not made yet: the lines that use one say so)
        } else if (token == "define_specialization" && i + 2 < t.size()) {
            const std::string id = specializationId(numberAt(i + 1), numberAt(i + 2));
            i += 2;
            std::vector<std::string> said;
            if (!id.empty() && speech.has(id))
                say(id, said);
            specialization = said.empty() ? "" : said[0];
        } else if (token == "define_realm" && i + 2 < t.size()) {
            i += 2; // (realm names are not made yet: the lines that use one say so)
        } else if (token == "define_ruler" && i + 1 < t.size()) {
            i += 1; // (nor rulers' titles)
        } else if (token == "mark_location" && i + 2 < t.size()) {
            play.marked.push_back({numberAt(i + 1), numberAt(i + 2), zone, item});
            i += 2;
        } else if (speech.has(token)) {
            if (token.back() == '?') { // an answer the player may choose
                say(token, play.choices);
                play.choiceIds.push_back(token);
            } else
                say(token, play.lines);
            lineTags.clear();
        } else {
            if (play.unknown.empty())
                play.unknown = token;
            unrendered(token);
        }
    }
    return play;
}
