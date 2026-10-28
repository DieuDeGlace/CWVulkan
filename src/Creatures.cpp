#include "Creatures.h"
#include "Recording.h"
#include <algorithm>
#include <charconv>
#include <cstdio>
#include <fstream>
#include <set>
#include <sstream>
#include <string_view>
#include <unordered_map>

namespace fs = std::filesystem;

namespace {
CreatureRecordings::Creature::Worn parseWorn(std::string_view value) {
    CreatureRecordings::Creature::Worn out;
    int *fields[] = {&out.category, &out.id, &out.material, &out.rarity};
    for (int i = 0; i < 4; i++) {
        size_t colon = value.find(':');
        auto field = value.substr(0, colon);
        auto parsed = std::from_chars(field.data(), field.data() + field.size(), *fields[i]);
        if (parsed.ec != std::errc() || parsed.ptr != field.data() + field.size()) return {};
        if (colon == std::string_view::npos) {
            if (i != 3) return {};
            value = {};
        } else {
            value.remove_prefix(colon + 1);
            if (i == 3) {
                uint32_t modifier;
                auto parsedModifier = std::from_chars(value.data(), value.data() + value.size(), modifier);
                if (parsedModifier.ec != std::errc() || parsedModifier.ptr != value.data() + value.size()) return {};
                out.modifier = modifier;
            }
        }
    }
    if (out.category < 0 || out.category > 255 || out.id < 0 || out.material < 0 || out.material > 255 ||
        out.rarity < 0 || out.rarity > 4) return {};
    return out;
}
}

size_t CreatureRecordings::load(const fs::path &dir, const std::function<glm::vec3(const int64_t *)> &toLocal) {
    creatures.clear();
    loadStats = {};
    // A creature's sprites may not have been initialized when its manifest
    // entry was written. Recover known names from later recorded slot IDs.
    std::unordered_map<int, std::string> spriteOf;
    std::ifstream spriteTable(dir.parent_path().parent_path() / "sprites.txt");
    std::string tableLine;
    while (std::getline(spriteTable, tableLine)) {
        std::istringstream row(tableLine);
        int slot;
        std::string file;
        if (!(row >> slot >> file) || file == "?")
            continue;
        if (file.rfind("sprites/", 0) == 0)
            file.erase(0, 8);
        if (file.size() > 4 && file.compare(file.size() - 4, 4, ".cub") == 0)
            file.resize(file.size() - 4);
        spriteOf[slot] = file;
    }
    std::ifstream list(dir / "creatures.txt");
    std::string line;
    std::vector<std::string> entries;
    while (std::getline(list, line))
        entries.push_back(line);
    std::set<std::string> seen;
    // A later tour may replace a creature's clip and appearance under its ID.
    for (auto entry = entries.rbegin(); entry != entries.rend(); ++entry) {
        line = *entry;
        // id=<n> race=<n> hostility=<n> ... hair=r,g,b ... sprites=a,b,-,...
        std::istringstream in(line);
        std::string field, id, sprites;
        Creature c;
        glm::u8vec3 hair(255);
        while (in >> field) {
            size_t eq = field.find('=');
            if (eq == std::string::npos)
                continue;
            std::string key = field.substr(0, eq), value = field.substr(eq + 1);
            if (key == "id")
                id = value, c.id = std::strtoull(value.c_str(), nullptr, 10);
            else if (key == "race")
                c.race = std::atoi(value.c_str());
            else if (key == "hostility")
                c.hostility = std::atoi(value.c_str());
            else if (key == "level")
                c.level = std::atoi(value.c_str());
            else if (key == "maxhp")
                c.maxHp = (float)std::atof(value.c_str());
            else if (key == "attack")
                c.attack = (float)std::atof(value.c_str());
            else if (key == "armor")
                c.armor = (float)std::atof(value.c_str());
            else if (key == "spell")
                c.spell = (float)std::atof(value.c_str());
            else if (key == "resistance")
                c.resistance = (float)std::atof(value.c_str());
            else if (key == "critical")
                c.critical = (float)std::atof(value.c_str());
            else if (key == "flags2") {
                uint32_t flags;
                auto parsed = std::from_chars(value.data(), value.data() + value.size(), flags, 16);
                if (parsed.ec == std::errc() && parsed.ptr == value.data() + value.size()) c.flags2 = flags;
            }
            else if (key == "equip") { // category:id:material:rarity[:modifier] per slot
                std::istringstream slots(value);
                std::string slot;
                while (c.worn.size() < 11 && std::getline(slots, slot, ',')) {
                    c.worn.push_back(parseWorn(slot));
                }
            }
            else if (key == "hair") {
                int r = 255, g = 255, b = 255;
                std::sscanf(value.c_str(), "%d,%d,%d", &r, &g, &b);
                hair = glm::u8vec3(r, g, b);
            } else if (key == "sprites")
                sprites = value;
        }
        if (id.empty() || sprites.empty())
            continue;
        if (!seen.insert(id).second)
            continue;
        loadStats.listed++;
        std::vector<recording::Frame> frames;
        bool positioned = false;
        if (!recording::read(dir / ("c_" + id + ".cwa"), frames, &positioned) || !positioned) {
            loadStats.missingFrames++;
            continue;
        }
        // Parts: (sprite, matrix index), in slot order.
        std::vector<int> matrixOf;
        std::istringstream names(sprites);
        std::string name;
        for (int slot = 0; std::getline(names, name, ',') && slot < recording::SPRITES; slot++) {
            if (name == "-" || name.empty()) {
                name.clear();
                for (const auto &f : frames) {
                    if (!recording::isPart(recording::matrix(f, 0)))
                        continue;
                    auto found = spriteOf.find(f.slots[slot]);
                    if (found != spriteOf.end()) {
                        name = found->second;
                        break;
                    }
                }
                if (name.empty())
                    continue;
            }
            for (int m : recording::SLOT_MATRICES[slot])
                if (m >= 0) {
                    c.parts.push_back({name, slot == recording::HAIR_SLOT ? hair : glm::u8vec3(255),
                                       slot == recording::HAIR_SLOT, m});
                    matrixOf.push_back(m);
                }
        }
        if (c.parts.empty()) {
            loadStats.missingAppearance++;
            continue;
        }
        std::vector<bool> used(c.parts.size(), false);
        uint64_t first = 0;
        bool haveFirst = false;
        for (auto &f : frames) {
            glm::mat4 root = recording::matrix(f, 0);
            if (!recording::isPart(root))
                continue;
            if (!haveFirst) {
                first = f.ms;
                haveFirst = true;
            }
            std::vector<glm::mat4> pose(c.parts.size(), glm::mat4(0));
            for (size_t i = 0; i < c.parts.size(); i++) {
                glm::mat4 m = recording::matrix(f, matrixOf[i]);
                if (!recording::isPart(m))
                    continue;
                m[3] = glm::vec4(glm::vec3(m[3]) - glm::vec3(root[3]), 1);
                pose[i] = m;
                used[i] = true;
            }
            c.times.push_back((f.ms - first) / 1000.0);
            c.positions.push_back(toLocal(f.position));
            c.poses.push_back(std::move(pose));
            c.angles.push_back(std::atan2(root[0].y, root[0].x));
            c.states.push_back(f.state);
            c.abilities.push_back(f.ability);
        }
        if (c.times.empty()) {
            loadStats.missingPose++;
            continue;
        }
        // Drop parts the game never drew with a real matrix.
        for (size_t i = c.parts.size(); i-- > 0;)
            if (!used[i]) {
                c.parts.erase(c.parts.begin() + i);
                for (auto &p : c.poses)
                    p.erase(p.begin() + i);
            }
        if (!c.parts.empty())
            creatures.push_back(std::move(c));
        else
            loadStats.missingPose++;
    }
    loadStats.loaded = creatures.size();
    return creatures.size();
}

size_t CreatureRecordings::frameAt(const Creature &c, double time) const {
    if (c.times.size() < 2)
        return 0;
    double span = c.times.back() + 1.0 / 30;
    double t = std::fmod(std::max(0.0, time), span);
    auto it = std::upper_bound(c.times.begin(), c.times.end(), t);
    return it == c.times.begin() ? 0 : size_t(it - c.times.begin()) - 1;
}
