#include "ItemCatalog.h"
#include "Creatures.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace {
// Equipment slot -> column of the creature's 14 recorded sprites.
int spriteColumn(int slot) {
    switch (slot) {
    case 1: return 11; // neck
    case 2: return 2;  // chest
    case 3: return 7;  // feet
    case 4: return 4;  // hands
    case 5: return 10; // shoulder
    case 6: return 9;  // weapon left
    case 7: return 8;  // weapon right
    default: return -1;
    }
}
} // namespace

bool ItemCatalog::loadClassRules(const fs::path &file, std::string &error) {
    std::ifstream in(file);
    if (!in) {
        error = "no " + file.filename().string() + " (ZoneExtractor F8 writes it)";
        return false;
    }
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        int c, i, m, a, b, d, e;
        if (std::sscanf(line.c_str(), "%d %d %d %d %d %d %d", &c, &i, &m, &a, &b, &d, &e) == 7)
            rules[{c, i, m}] = {a != 0, b != 0, d != 0, e != 0};
    }
    return !rules.empty();
}

bool ItemCatalog::mayUse(int category, int id, int material, int playerClass) const {
    auto it = rules.find({category, id, material});
    if (it == rules.end() || playerClass < 1 || playerClass > 4)
        return true;
    return it->second[playerClass - 1];
}

std::string ItemCatalog::typeOfSprite(const std::string &sprite) const {
    // "<material>-<type><n>[-random<k>]" -> type; one-word names are the type.
    size_t dash = sprite.find('-');
    std::string rest = dash == std::string::npos ? sprite : sprite.substr(dash + 1);
    size_t cut = rest.find("-random");
    if (cut != std::string::npos)
        rest.resize(cut);
    while (!rest.empty() && std::isdigit((unsigned char)rest.back()))
        rest.pop_back();
    return rest;
}

size_t ItemCatalog::loadModels(const fs::path &creaturesTxt) {
    std::ifstream in(creaturesTxt);
    std::string line;
    while (std::getline(in, line)) {
        size_t e = line.find(" equip="), s = line.find(" sprites=");
        if (e == std::string::npos || s == std::string::npos)
            continue;
        std::vector<Key> gear;
        std::istringstream items(line.substr(e + 7, line.find(' ', e + 7) - e - 7));
        std::string item;
        while (std::getline(items, item, ',')) {
            int c = 0, i = 0, m = 0, r = 0;
            std::sscanf(item.c_str(), "%d:%d:%d:%d", &c, &i, &m, &r);
            gear.push_back({c, i, m});
        }
        std::vector<std::string> sprites;
        std::istringstream names(line.substr(s + 9));
        std::string name;
        while (std::getline(names, name, ','))
            sprites.push_back(name);
        for (int slot = 0; slot < (int)gear.size(); slot++) {
            int column = spriteColumn(slot);
            if (std::get<0>(gear[slot]) == 0 || column < 0 || column >= (int)sprites.size() ||
                sprites[column] == "-" || sprites[column].empty())
                continue;
            Model &m = known[gear[slot]];
            m.sprites.insert(sprites[column]);
            m.slot = slot;
            m.type = typeOfSprite(sprites[column]);
            size_t dash = sprites[column].find('-');
            m.material = dash == std::string::npos ? "" : sprites[column].substr(0, dash);
        }
    }
    return known.size();
}

const ItemCatalog::Model *ItemCatalog::model(int category, int id, int material) const {
    auto it = known.find({category, id, material});
    return it == known.end() ? nullptr : &it->second;
}

void ItemCatalog::learnGrips(const CreatureRecordings &creatures) {
    // Weapon relative to its hand, from the first recorded frame of each creature
    // holding one (weapon right: matrix 7 with hand 10; left: 8 with 9).
    for (auto &c : creatures.all()) {
        std::map<int, size_t> partOfMatrix;
        for (size_t i = 0; i < c.parts.size(); i++)
            partOfMatrix[c.parts[i].matrix] = i;
        for (auto [weapon, hand] : {std::pair{7, 10}, std::pair{8, 9}}) {
            auto w = partOfMatrix.find(weapon), h = partOfMatrix.find(hand);
            if (w == partOfMatrix.end() || h == partOfMatrix.end() || c.poses.empty())
                continue;
            const glm::mat4 &W = c.poses[0][w->second], &H = c.poses[0][h->second];
            if (W[3][3] == 0 || H[3][3] == 0)
                continue;
            std::string type = typeOfSprite(c.parts[w->second].sprite);
            if (!grips.count(type)) {
                grips[type] = glm::inverse(H) * W;
                gripSamples[type] = 1;
            } else
                gripSamples[type]++;
        }
    }
}

const glm::mat4 *ItemCatalog::grip(const std::string &type) const {
    auto it = grips.find(type);
    return it == grips.end() ? nullptr : &it->second;
}

std::string ItemCatalog::summary() const {
    std::set<std::string> types, materials;
    for (auto &[k, m] : known) {
        types.insert(m.type);
        materials.insert(m.material);
    }
    std::string s = std::to_string(rules.size()) + " class rules, " + std::to_string(known.size()) +
                    " item models (" + std::to_string(types.size()) + " types, " + std::to_string(materials.size()) +
                    " materials), " + std::to_string(grips.size()) + " weapon grips";
    return s;
}

void ItemCatalog::addInferredRangerArmor() {
    static const std::pair<int, const char *> materials[] = {{27, "linen"}, {31, "felt"}, {33, "mail"}};
    static const std::pair<int, const char *> parts[] = {{4, "chest"}, {5, "hand"}, {6, "foot"}, {7, "shoulder"}};
    static const int slots[] = {2, 4, 3, 5}; // the game's slots: chest, hands, feet, shoulder
    for (auto &[material, name] : materials)
        for (size_t p = 0; p < 4; p++) {
            Key key{parts[p].first, 0, material};
            if (known.count(key))
                continue;
            Model m;
            m.sprites.insert(std::string(name) + "-" + parts[p].second + "1");
            m.type = parts[p].second;
            m.material = name;
            m.slot = slots[p];
            m.inferred = true;
            known[key] = m;
        }
}
