#include "Saves.h"
#include "Gear.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <limits>
#include <atomic>
#include <windows.h>

namespace fs = std::filesystem;

namespace {
std::string lower(std::string s) {
    for (auto &c : s)
        c = (char)std::tolower((unsigned char)c);
    return s;
}

// Name, class, level and race from a snapshot.txt. The race is named after the
// head sprite the game drew ("elf-head-m04" -> elf), the game's own word for it.
bool readSnapshot(const fs::path &dir, CharacterEntry &e) {
    std::ifstream in(dir / "snapshot.txt");
    if (!in)
        return false;
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind("name=", 0) == 0)
            e.name = line.substr(5);
        else if (line.rfind("race=", 0) == 0)
            e.race = std::atoi(line.c_str() + 5);
        else if (line.rfind("class=", 0) == 0)
            e.classType = std::atoi(line.c_str() + 6);
        else if (line.rfind("level=", 0) == 0)
            e.level = std::atoi(line.c_str() + 6);
        else if (line.rfind("specialization=", 0) == 0)
            e.specialization = std::atoi(line.c_str() + 15);
        else if (line.rfind("sprite face ", 0) == 0) {
            auto slash = line.rfind('/'), head = line.find("-head");
            if (slash != std::string::npos && head != std::string::npos && head > slash)
                e.raceName = line.substr(slash + 1, head - slash - 1);
        }
    }
    return true;
}
} // namespace

const char *className(int c) {
    switch (c) {
    case 1: return "warrior";
    case 2: return "ranger";
    case 3: return "mage";
    case 4: return "rogue";
    default: return "adventurer";
    }
}

const char *specializationName(int c, int s) {
    static const char *names[5][2] = {
        {"", ""}, {"berserker", "guardian"}, {"sniper", "scout"}, {"fire", "water"}, {"assassin", "ninja"}};
    return c >= 1 && c <= 4 && (s == 0 || s == 1) ? names[c][s] : "";
}

std::string CharacterEntry::saveId() const {
    std::string out;
    for (char c : name.empty() ? id : name)
        out += std::isalnum((unsigned char)c) ? (char)std::tolower((unsigned char)c) : '_';
    return out + "-" + className(classType);
}

std::string CharacterEntry::describe() const {
    std::string spec = specializationName(classType, specialization);
    return (name.empty() ? id : name) + ", level " + std::to_string(level) + " " + className(classType) +
           (spec.empty() ? "" : " (" + spec + ")") + " [" + id + "]";
}

std::vector<CharacterEntry> listCharacters(const fs::path &gameDir) {
    std::vector<std::pair<fs::file_time_type, CharacterEntry>> found;
    auto add = [&](const fs::path &dir, const std::string &id) {
        CharacterEntry e;
        e.dir = dir;
        e.id = id;
        std::error_code ec;
        if (readSnapshot(dir, e))
            found.push_back({fs::last_write_time(dir / "snapshot.txt", ec), e});
    };
    std::error_code ec;
    for (auto &d : fs::directory_iterator(gameDir / "Extract" / "characters", ec))
        if (d.is_directory())
            add(d.path(), d.path().filename().string());
    add(gameDir / "Extract" / "character", "character");
    std::stable_sort(found.begin(), found.end(), [](auto &a, auto &b) { return a.first > b.first; });
    std::vector<CharacterEntry> out;
    for (auto &f : found)
        out.push_back(f.second);
    return out;
}

const CharacterEntry *pickCharacter(const std::vector<CharacterEntry> &all, const std::string &wanted) {
    if (all.empty())
        return nullptr;
    if (wanted.empty())
        return &all.front();
    std::string w = lower(wanted);
    for (auto &e : all)
        if (lower(e.id) == w)
            return &e;
    for (auto &e : all)
        if (lower(e.name) == w)
            return &e;
    for (auto &e : all) // "warrior": the newest character of that class
        if (w == className(e.classType))
            return &e;
    for (auto &e : all) // "guardian": the newest character with that specialization
        if (w == specializationName(e.classType, e.specialization))
            return &e;
    return nullptr;
}

// One "key value..." per line, so a save can be read and edited by hand.
bool SaveGame::parse(const std::string &text) {
    SaveGame candidate;
    if (!candidate.parseFields(text)) return false;
    *this = std::move(candidate);
    return true;
}

bool SaveGame::parseFields(const std::string &text) {
    if (text.size() > 32 * 1024 * 1024) return false;
    std::istringstream in(text);
    std::string line;
    worn.clear();
    found.clear();
    level = xp = gold = 0;
    specialization = -1;
    hour = -1, day = 0;
    created = Created{};
    quests.clear();
    killed.clear();
    usedCells.clear();
    discoveredPlaces.clear();
    discoveredAreas.clear();
    treasures.clear();
    flightPoints.clear();
    finishedMissions.clear();
    solvedMissions.clear();
    missionTimers.clear();
    takenItems.clear();
    usedObjects.clear();
    deathPosition = {0, 0, 0}, tombstoneMs = 0;
    marked.clear();
    hasPosition = hasGear = false;
    bool any = false;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::istringstream s(line);
        std::string key;
        s >> key;
        if (key.empty() || key[0] == '#') continue;
        if (key == "cubevulkan-save") {
            int version = 0;
            if (any || !(s >> version) || version != 1) return false;
            any = true;
            continue;
        }
        if (!any) return false;
        if (key == "world") {
            if (!(s >> std::quoted(world))) return false;
        } else if (key == "saved") {
            if (!(s >> savedAt) || savedAt < 0) return false;
        } else if (key == "position") {
            if (!(s >> position.x >> position.y >> position.z) || !std::isfinite(position.x) ||
                !std::isfinite(position.y) || !std::isfinite(position.z)) return false;
            hasPosition = true;
        } else if (key == "yaw") {
            if (!(s >> yaw) || !std::isfinite(yaw)) return false;
        } else if (key == "heading") {
            if (!(s >> heading) || !std::isfinite(heading)) return false;
        } else if (key == "hp") {
            if (!(s >> hpFraction) || !std::isfinite(hpFraction) || hpFraction < 0 || hpFraction > 1) return false;
        }
        else if (key == "worn") {
            Worn w;
            if (!(s >> w.category >> w.id >> w.material >> w.rarity >> w.modifier) || worn.size() >= Gear::SLOTS) return false;
            worn.push_back(w);
            hasGear = true;
        } else if (key == "level") {
            if (!(s >> level) || level < 0) return false;
        } else if (key == "xp") {
            if (!(s >> xp) || xp < 0) return false;
        } else if (key == "specialization") {
            if (!(s >> specialization) || specialization < 0 || specialization > 255) return false;
        } else if (key == "gold") {
            if (!(s >> gold) || gold < 0) return false;
        } else if (key == "time") {
            if (!(s >> day >> hour) || day < 0 || !std::isfinite(hour) || hour < 0 || hour > 24) return false;
        }
        else if (key == "created") {
            int r = 255, g = 255, b = 255;
            if (!(s >> created.classType >> created.specialization >> std::quoted(created.race) >> std::quoted(created.gender) >> std::quoted(created.head) >>
                std::quoted(created.hair) >> std::quoted(created.hand) >> std::quoted(created.foot) >> r >> g >> b) ||
                created.classType < 1 || created.classType > 4 || created.specialization < 0 || created.specialization > 1 ||
                r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) return false;
            created.hairColor = glm::u8vec3(std::clamp(r, 0, 255), std::clamp(g, 0, 255), std::clamp(b, 0, 255));
        } else if (key == "marked") {
            Marked m;
            if (!(s >> m.x >> m.y >> std::quoted(m.place) >> std::quoted(m.item)) || marked.size() >= 100000) return false;
            marked.push_back(m);
        } else if (key == "opened") {
            std::array<int32_t, 2> c;
            if (!(s >> c[0] >> c[1]) || usedCells.size() >= 1000000) return false;
            usedCells.push_back(c);
        } else if (key == "place" || key == "area") {
            std::array<int32_t, 2> c;
            auto &list = key == "place" ? discoveredPlaces : discoveredAreas;
            if (!(s >> c[0] >> c[1]) || list.size() >= 1000000) return false;
            list.push_back(c);
        } else if (key == "flight") {
            std::array<int32_t, 2> f;
            if (!(s >> f[0] >> f[1]) || flightPoints.size() >= 1000000) return false;
            flightPoints.push_back(f);
        } else if (key == "lore") { // (older saves kept the lore by realm: its cells are used cells)
            uint32_t realm;
            std::array<int32_t, 2> c;
            if (!(s >> realm >> c[0] >> c[1]) || usedCells.size() >= 1000000) return false;
            if (std::find(usedCells.begin(), usedCells.end(), c) == usedCells.end())
                usedCells.push_back(c);
        } else if (key == "timer") {
            std::array<int32_t, 2> c;
            float ms;
            if (!(s >> c[0] >> c[1] >> ms) || missionTimers.size() >= 1000000) return false;
            missionTimers[c] = ms;
        } else if (key == "finished" || key == "solved") {
            std::array<int32_t, 2> c;
            auto &list = key == "finished" ? finishedMissions : solvedMissions;
            if (!(s >> c[0] >> c[1]) || list.size() >= 1000000) return false;
            list.insert(c);
        } else if (key == "treasure") {
            std::array<int32_t, 3> t;
            if (!(s >> t[0] >> t[1] >> t[2]) || treasures.size() >= 1000000) return false;
            treasures.push_back(t);
        } else if (key == "death") {
            long long x, y, z;
            if (!(s >> x >> y >> z >> tombstoneMs)) return false;
            deathPosition = {x, y, z};
        } else if (key == "used") {
            std::array<int32_t, 3> t;
            if (!(s >> t[0] >> t[1] >> t[2]) || usedObjects.size() >= 1000000) return false;
            usedObjects.push_back(t);
        } else if (key == "taken") {
            std::array<int32_t, 3> t;
            if (!(s >> t[0] >> t[1] >> t[2]) || takenItems.size() >= 1000000) return false;
            takenItems.push_back(t);
        } else if (key == "killed") {
            long long id;
            while (s >> id) {
                if (killed.size() >= 1000000) return false;
                killed.push_back(id);
            }
        } else if (key == "quest") {
            Quest q;
            if (!(s >> q.giver >> q.state >> q.need >> q.done >> std::quoted(q.target)) || q.state < 0 || q.state > 2 ||
                q.need < 0 || q.done < 0 || quests.size() >= 100000) return false;
            std::getline(s >> std::ws, q.zone);
            quests.push_back(q);
        } else if (key == "name") {
            std::getline(s, created.name);
            if (!created.name.empty() && (created.name[0] == ' ' || created.name[0] == '\t')) created.name.erase(0, 1);
        }
        else if (key == "bag") {
            Bagged b;
            if (!(s >> b.category >> b.id >> b.material >> b.rarity >> b.modifier >> b.count >> b.region[0] >> b.region[1]) ||
                b.count <= 0 || bag.size() >= 100000)
                return false;
            bag.push_back(b);
        }
        else if (key == "found") {
            Found f;
            if (!(s >> f.category >> f.id >> f.material >> f.rarity >> f.modifier >> f.level >> f.slot >> std::quoted(f.sprite) >> f.hp >>
                f.attack >> f.armor >> f.critical) || f.slot < 0 || f.slot >= Gear::SLOTS || f.level < 0 ||
                !std::isfinite(f.hp) || !std::isfinite(f.attack) || !std::isfinite(f.armor) || !std::isfinite(f.critical) ||
                found.size() >= 100000) return false;
            if (f.sprite == "-") f.sprite.clear();
            if (!(s >> f.region[0] >> f.region[1])) // (older saves have no region)
                f.region[0] = f.region[1] = 0;
            found.push_back(f);
        }
    }
    hpFraction = std::clamp(hpFraction, .01f, 1.f);
    return any && (!hasGear || worn.size() == Gear::SLOTS) &&
           ((created.classType != 0) == !created.name.empty());
}

std::string SaveGame::text() const {
    std::ostringstream out;
    out << std::setprecision(std::numeric_limits<float>::max_digits10);
    out << "cubevulkan-save 1\n# CubeVulkan progress (Steam Cube World's own save is not touched)\n";
    out << "saved " << savedAt << '\n';
    if (!world.empty())
        out << "world " << std::quoted(world) << '\n';
    if (hasPosition)
        out << "position " << position.x << ' ' << position.y << ' ' << position.z << '\n';
    out << "yaw " << yaw << "\nheading " << heading << "\nhp " << hpFraction << '\n';
    for (auto &w : worn)
        out << "worn " << w.category << ' ' << w.id << ' ' << w.material << ' ' << w.rarity << ' ' << w.modifier << '\n';
    if (level > 0)
        out << "level " << level << "\nxp " << xp << '\n';
    out << "gold " << gold << '\n';
    if (specialization >= 0) out << "specialization " << specialization << '\n';
    if (hour >= 0)
        out << "time " << day << ' ' << hour << '\n';
    // marked <x> <y> "<place>" "<item>"
    for (auto &m : marked)
        out << "marked " << m.x << ' ' << m.y << ' ' << std::quoted(m.place) << ' ' << std::quoted(m.item) << '\n';
    for (auto &c : usedCells) // opened <cell x> <cell y> (a used cell: a chest opened or lore read)
        out << "opened " << c[0] << ' ' << c[1] << '\n';
    for (auto &c : discoveredPlaces) // place <cell x> <cell y>
        out << "place " << c[0] << ' ' << c[1] << '\n';
    for (auto &c : discoveredAreas) // area <region x> <region y>
        out << "area " << c[0] << ' ' << c[1] << '\n';
    for (auto &f : flightPoints) // flight <x> <y>
        out << "flight " << f[0] << ' ' << f[1] << '\n';
    for (auto &c : finishedMissions) // finished <cell x> <cell y>
        out << "finished " << c[0] << ' ' << c[1] << '\n';
    for (auto &c : solvedMissions) // solved <cell x> <cell y>
        out << "solved " << c[0] << ' ' << c[1] << '\n';
    for (auto &[c, ms] : missionTimers) // timer <cell x> <cell y> <ms left>
        out << "timer " << c[0] << ' ' << c[1] << ' ' << ms << '\n';
    for (auto &t : treasures) // treasure <x> <y> <z>
        out << "treasure " << t[0] << ' ' << t[1] << ' ' << t[2] << '\n';
    if (tombstoneMs > 0) // death <x> <y> <z> <ms left>
        out << "death " << deathPosition[0] << ' ' << deathPosition[1] << ' ' << deathPosition[2] << ' ' << tombstoneMs << '\n';
    for (auto &t : usedObjects) // used <zone x> <zone y> <index>
        out << "used " << t[0] << ' ' << t[1] << ' ' << t[2] << '\n';
    for (auto &t : takenItems) // taken <zone x> <zone y> <index>
        out << "taken " << t[0] << ' ' << t[1] << ' ' << t[2] << '\n';
    // killed <id> ... (64 to a line)
    for (size_t i = 0; i < killed.size(); i += 64) {
        out << "killed";
        for (size_t j = i; j < std::min(killed.size(), i + 64); j++)
            out << ' ' << killed[j];
        out << '\n';
    }
    // quest <giver> <state> <need> <done> <target> <zone...>
    for (auto &q : quests)
        out << "quest " << q.giver << ' ' << q.state << ' ' << q.need << ' ' << q.done << ' ' << std::quoted(q.target) << ' ' << q.zone << '\n';
    if (!created.name.empty()) {
        // created <class> <specialization> <race> <gender> <head> <hair> <hand> <foot> <r> <g> <b>
        out << "created " << created.classType << ' ' << created.specialization << ' ' << std::quoted(created.race) << ' '
            << std::quoted(created.gender) << ' ' << std::quoted(created.head) << ' ' << std::quoted(created.hair) << ' ' << std::quoted(created.hand) << ' ' << std::quoted(created.foot)
            << ' ' << (int)created.hairColor.r << ' ' << (int)created.hairColor.g << ' ' << (int)created.hairColor.b
            << "\nname " << created.name << '\n';
    }
    // bag <category> <id> <material> <rarity> <modifier> <count> <region x> <region y>
    for (auto &b : bag)
        out << "bag " << b.category << ' ' << b.id << ' ' << b.material << ' ' << b.rarity << ' ' << b.modifier << ' ' << b.count
            << ' ' << b.region[0] << ' ' << b.region[1] << '\n';
    // found <category> <id> <material> <rarity> <modifier> <level> <slot> <sprite> <hp> <attack> <armor> <critical>
    //       <region x> <region y>
    for (auto &f : found)
        out << "found " << f.category << ' ' << f.id << ' ' << f.material << ' ' << f.rarity << ' ' << f.modifier << ' '
            << f.level << ' ' << f.slot << ' ' << std::quoted(f.sprite) << ' ' << f.hp << ' ' << f.attack
            << ' ' << f.armor << ' ' << f.critical << ' ' << f.region[0] << ' ' << f.region[1] << '\n';
    return out.str();
}

bool SaveGame::load(const fs::path &file) {
    std::ifstream in(file, std::ios::binary);
    if (!in)
        return false;
    std::error_code ec;
    if (fs::file_size(file, ec) > 32 * 1024 * 1024 || ec) return false;
    std::ostringstream all;
    all << in.rdbuf();
    return parse(all.str());
}

bool SaveGame::save(const fs::path &file) const {
    std::string bytes = text();
    SaveGame checked;
    if (!checked.parse(bytes)) return false;
    std::error_code ec;
    if (!file.parent_path().empty()) fs::create_directories(file.parent_path(), ec);
    if (ec) return false;
    static std::atomic<unsigned> serial{0};
    fs::path temp = file;
    temp += L".tmp-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(serial++);
    HANDLE out = CreateFileW(temp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (out == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    bool ok = WriteFile(out, bytes.data(), (DWORD)bytes.size(), &written, nullptr) && written == bytes.size() && FlushFileBuffers(out);
    if (!CloseHandle(out)) ok = false;
    if (ok) ok = MoveFileExW(temp.c_str(), file.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
    fs::remove(temp, ec); // only the newly created temporary save; old save stays intact on failure
    return ok;
}

void SaveGame::takeGear(const Gear &gear) {
    worn.clear();
    for (int s = 0; s < Gear::SLOTS; s++) {
        auto &it = gear.equipped((Gear::Slot)s);
        worn.push_back({it.category, it.id, it.material, it.rarity, it.modifier});
    }
    hasGear = true;
}

void SaveGame::applyGear(Gear &gear) const {
    if (!hasGear)
        return;
    for (int s = 0; s < Gear::SLOTS && s < (int)worn.size(); s++) {
        auto &w = worn[s];
        auto &now = gear.equipped((Gear::Slot)s);
        if (now.category == w.category && now.id == w.id && now.material == w.material && now.modifier == w.modifier)
            continue;
        if (w.category == 0) {
            gear.unequip((Gear::Slot)s);
            continue;
        }
        for (size_t i = 0; i < gear.itemCount(); i++) {
            auto &it = gear.item(i);
            if (gear.slotOf(i) == s && it.category == w.category && it.id == w.id && it.material == w.material &&
                it.modifier == w.modifier) {
                gear.equip(i);
                break;
            }
        }
    }
}

namespace {
// The bundle's sections: id -> text, in file order.
bool bundleHeader(const std::string &bundle) {
    std::istringstream in(bundle); std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream header(line); std::string key, extra; int version = 0;
        return (header >> key >> version) && key == "cubevulkan-cloud" && version == 1 && !(header >> extra);
    }
    return false;
}

std::vector<std::pair<std::string, std::string>> sections(const std::string &bundle) {
    std::vector<std::pair<std::string, std::string>> out;
    std::istringstream in(bundle);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.rfind("character ", 0) == 0)
            out.push_back({line.substr(10), ""});
        else if (!out.empty())
            out.back().second += line + "\n";
    }
    return out;
}
} // namespace

std::set<int64_t> SaveGame::worldKills(const std::string &bundle, const std::string &world) {
    std::set<int64_t> all;
    if (!bundleHeader(bundle)) return all;
    for (auto &[name, text] : sections(bundle)) {
        SaveGame s;
        if (s.parse(text) && s.world == world)
            all.insert(s.killed.begin(), s.killed.end());
    }
    return all;
}

std::string SaveGame::fromBundle(const std::string &bundle, const std::string &id) {
    if (!bundleHeader(bundle)) return "";
    for (auto &[name, text] : sections(bundle))
        if (name == id)
            return text;
    return "";
}

std::string SaveGame::intoBundle(const std::string &bundle, const std::string &id, const std::string &text) {
    SaveGame candidate;
    if (id.empty() || id.find_first_of("\r\n") != std::string::npos || !candidate.parse(text) ||
        (!bundle.empty() && !bundleHeader(bundle))) return "";
    auto all = sections(bundle);
    std::set<std::string> ids;
    for (const auto &[name, body] : all) {
        SaveGame existing;
        if (name.empty() || !ids.insert(name).second || !existing.parse(body)) return "";
    }
    bool replaced = false;
    for (auto &[name, body] : all)
        if (name == id)
            body = text, replaced = true;
    if (!replaced)
        all.push_back({id, text});
    std::string out = "cubevulkan-cloud 1\n";
    for (auto &[name, body] : all)
        out += "character " + name + "\n" + body;
    return out;
}

std::vector<CharacterEntry> listCreated(const fs::path &saveDir, const std::string &cloudBundle,
                                        const std::vector<CharacterEntry> &recorded) {
    // Every save that carries a created look, the newer copy of each character winning.
    std::map<std::string, SaveGame> saves;
    auto consider = [&](const std::string &text) {
        SaveGame s;
        if (!s.parse(text) || s.created.name.empty() || !s.created.classType)
            return;
        CharacterEntry probe;
        probe.name = s.created.name, probe.classType = s.created.classType;
        auto &slot = saves[probe.saveId()];
        if (slot.created.name.empty() || s.savedAt >= slot.savedAt)
            slot = s;
    };
    std::error_code ec;
    for (auto &f : fs::directory_iterator(saveDir, ec))
        if (f.path().extension() == ".sav") {
            std::ifstream in(f.path(), std::ios::binary);
            std::ostringstream all;
            all << in.rdbuf();
            consider(all.str());
        }
    std::istringstream bundle(cloudBundle);
    std::string line, section;
    while (std::getline(bundle, line)) {
        if (line.rfind("character ", 0) == 0) {
            consider(section);
            section.clear();
        } else
            section += line + "\n";
    }
    consider(section);
    std::vector<CharacterEntry> out;
    for (auto &[id, s] : saves) {
        const auto &c = s.created;
        // The recording it moves with: same class and specialization, else same class.
        const CharacterEntry *base = nullptr;
        for (auto &r : recorded)
            if (r.classType == c.classType && (!base || (r.specialization == c.specialization && base->specialization != c.specialization)))
                base = &r;
        if (!base)
            continue;
        CharacterEntry e;
        e.dir = base->dir;
        e.name = c.name;
        e.classType = c.classType;
        e.specialization = base->specialization;
        e.level = std::max(1, s.level);
        e.raceName = c.race;
        e.id = e.saveId();
        e.created = true;
        e.head = c.head, e.hair = c.hair, e.hand = c.hand, e.foot = c.foot;
        e.hairColor = c.hairColor;
        out.push_back(e);
    }
    return out;
}
