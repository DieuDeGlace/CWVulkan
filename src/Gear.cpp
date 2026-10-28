#include "Gear.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace {
const char *SLOT_NAMES[] = {"unk", "neck", "chest", "feet", "hands", "shoulder", "weapon_left",
                            "weapon_right", "ring_left", "ring_right", "pet"};
// Which of the 12 recorded sprite columns each slot changes (animation state
// order: face, hair, chest, unk, hands, wings, tail, feet, weapon R, weapon L,
// shoulder, neck).
int spriteColumn(int slot) {
    switch (slot) {
    case Gear::Chest: return 2;
    case Gear::Hands: return 4;
    case Gear::Feet: return 7;
    case Gear::WeaponRight: return 8;
    case Gear::WeaponLeft: return 9;
    case Gear::Shoulder: return 10;
    case Gear::Neck: return 11;
    default: return -1;
    }
}
int slotNamed(const std::string &n) {
    for (int i = 0; i < Gear::SLOTS; i++)
        if (n == SLOT_NAMES[i])
            return i;
    return -1;
}
// key=value fields of a gear.txt line, the stats after "player", the sprites after "sprites".
struct Line {
    std::map<std::string, std::string> fields;
    Gear::Stats stats;
    std::vector<std::string> sprites;
};
Line parse(const std::string &text) {
    Line l;
    std::istringstream in(text);
    std::string word;
    enum { Fields, Player, Sprites } part = Fields;
    while (in >> word) {
        if (word == "|")
            continue;
        if (word == "player") {
            part = Player;
            continue;
        }
        if (word == "sprites") {
            part = Sprites;
            continue;
        }
        if (part == Sprites) {
            if (word.size() > 4 && word.compare(word.size() - 4, 4, ".cub") == 0)
                word.resize(word.size() - 4);
            l.sprites.push_back(word == "-" ? "" : word);
            continue;
        }
        size_t eq = word.find('=');
        if (eq == std::string::npos)
            continue;
        std::string k = word.substr(0, eq), v = word.substr(eq + 1);
        if (part == Player) {
            float f = (float)std::atof(v.c_str());
            if (k == "maxhp") l.stats.maxHp = f;
            else if (k == "attack") l.stats.attack = f;
            else if (k == "armor") l.stats.armor = f;
            else if (k == "critical") l.stats.critical = f;
        } else
            l.fields[k] = v;
    }
    return l;
}
int num(const Line &l, const char *k) {
    auto it = l.fields.find(k);
    return it == l.fields.end() ? 0 : (int)std::strtoll(it->second.c_str(), nullptr, 10);
}
} // namespace

bool Gear::load(const fs::path &dir, std::string &error) {
    std::ifstream in(dir / "gear.txt");
    if (!in) {
        error = "no gear.txt in " + dir.string() + " (run the gear survey, ZoneExtractor F8)";
        return false;
    }
    std::map<int, Stats> baseline;
    std::string text;
    while (std::getline(in, text)) {
        if (text.rfind("# Gear survey:", 0) == 0) {
            auto p = text.find("class=");
            if (p != std::string::npos)
                classType = std::atoi(text.c_str() + p + 6);
            continue;
        }
        std::istringstream head(text);
        std::string kind;
        head >> kind;
        Line l = parse(text);
        int slot = slotNamed(l.fields["slot"]);
        auto itemFrom = [&](const Line &x) {
            Item it;
            it.category = num(x, "category");
            it.id = num(x, "id");
            it.material = num(x, "material");
            it.rarity = num(x, "rarity");
            it.modifier = (unsigned)num(x, "modifier");
            if (auto f = x.fields.find("region"); f != x.fields.end())
                std::sscanf(f->second.c_str(), "%d,%d", &it.region[0], &it.region[1]);
            it.formula = num(x, "formula");
            return it;
        };
        int column = slot >= 0 ? spriteColumn(slot) : -1;
        std::string sprite = column >= 0 && column < (int)l.sprites.size() ? l.sprites[column] : "";
        if (kind == "equipped" && slot >= 0) {
            worn[slot] = itemFrom(l);
        } else if (kind == "baseline" && slot >= 0) {
            baseline[slot] = l.stats;
            empty[slot] = sprite;
        } else if (kind == "restored" && slot >= 0) {
            Item &it = worn[slot];
            it.sprite = sprite;
            it.adds = l.stats - baseline[slot];
            it.measured = true;
            totals = l.stats; // The original gear, as the game totalled it.
        } else if (kind == "inventory") { // what the character carried that is no gear
            Item it = itemFrom(l);
            const bool gearKind = (it.category >= 3 && it.category <= 9) || it.category == 0x1a;
            if (it.category && !gearKind)
                extras.push_back({it, std::max(1, num(l, "quantity"))});
        } else if (kind == "item" && slot >= 0) {
            Item it = itemFrom(l);
            it.tab = num(l, "tab");
            it.index = num(l, "index");
            it.sprite = sprite;
            it.adds = l.stats - baseline[slot];
            it.measured = true;
            carried.push_back(it);
            carriedSlot.push_back((Slot)slot);
        }
    }
    if (!totals.maxHp) {
        error = "gear.txt has no measured slots";
        return false;
    }
    for (auto &w : worn)
        if (!w.category)
            w = Item{}; // Empty slots (the game leaves stale fields behind).
    for (int s = 0; s < SLOTS; s++)
        originalWorn[s] = worn[s];
    for (int s = 0; s < SLOTS; s++)
        if (worn[s].measured) {
            wornIndex[s] = (int)all.size();
            all.push_back(worn[s]);
            allSlot.push_back((Slot)s);
        }
    for (size_t i = 0; i < carried.size(); i++) {
        all.push_back(carried[i]);
        allSlot.push_back(carriedSlot[i]);
    }
    return true;
}

Gear::Stats Gear::base() const {
    // The character without gear: the game's totals minus what each original item added.
    Stats b = totals;
    for (int s = 0; s < SLOTS; s++)
        if (originalWorn[s].measured)
            b = b - originalWorn[s].adds;
    return b;
}

Gear::Stats Gear::stats() const {
    Stats s = base();
    for (auto &w : worn)
        if (w.measured)
            s = s + w.adds;
    return s;
}

std::string Gear::sprite(Slot s) const {
    if (worn[s].measured)
        return worn[s].sprite;
    auto it = empty.find(s);
    return it == empty.end() ? "" : it->second;
}

bool Gear::cycle(Slot s, const std::function<bool(const Item &)> &allowed) {
    std::vector<Item> options;
    if (originalWorn[s].measured)
        options.push_back(originalWorn[s]);
    for (size_t i = 0; i < carried.size(); i++)
        if (carriedSlot[i] == s && (!allowed || allowed(carried[i])))
            options.push_back(carried[i]);
    if (options.size() < 2)
        return false;
    size_t at = 0;
    for (size_t i = 0; i < options.size(); i++)
        if (options[i].category == worn[s].category && options[i].id == worn[s].id &&
            options[i].modifier == worn[s].modifier && options[i].material == worn[s].material)
            at = i;
    worn[s] = options[(at + 1) % options.size()];
    return true;
}

std::string Gear::describe(const Item &it) const {
    char s[160];
    std::snprintf(s, sizeof s, "category %d id %d material %d rarity %d (%s): +%.1f HP, +%.2f attack, +%.2f armor",
                  it.category, it.id, it.material, it.rarity, it.sprite.empty() ? "no sprite" : it.sprite.c_str(),
                  it.adds.maxHp, it.adds.attack, it.adds.armor);
    return s;
}

bool Gear::isWorn(size_t i) const {
    for (int s = 0; s < SLOTS; s++)
        if (wornIndex[s] == (int)i)
            return true;
    return false;
}

void Gear::equip(size_t i) {
    if (i >= all.size())
        return;
    Slot s = allSlot[i];
    worn[s] = all[i];
    wornIndex[s] = (int)i;
}

void Gear::unequip(Slot s) {
    worn[s] = Item{};
    wornIndex[s] = -1;
}

void Gear::remove(size_t i) {
    if (i >= all.size())
        return;
    for (int s = 0; s < SLOTS; s++) {
        if (wornIndex[s] == (int)i)
            unequip((Slot)s);
        else if (wornIndex[s] > (int)i)
            wornIndex[s]--;
    }
    all.erase(all.begin() + (long long)i);
    allSlot.erase(allSlot.begin() + (long long)i);
}

size_t Gear::add(const Item &item, Slot slot) {
    all.push_back(item);
    allSlot.push_back(slot);
    return all.size() - 1;
}

const Gear::Item *Gear::reference(Slot s, int category) const {
    const Item *best = nullptr;
    for (size_t i = 0; i < all.size(); i++) {
        const Item &it = all[i];
        if (allSlot[i] != s || !it.measured || it.found)
            continue;
        if (!best || (it.category == category && best->category != category))
            best = &it;
    }
    return best;
}

void Gear::update(size_t i, const Item &item) {
    if (i >= all.size())
        return;
    all[i] = item;
    if (wornIndex[allSlot[i]] == (int)i)
        worn[allSlot[i]] = item;
}

void Gear::updateWorn(Slot s, const Item &item) {
    if (s < 0 || s >= SLOTS)
        return;
    worn[s] = item;
    if (wornIndex[s] >= 0 && wornIndex[s] < (int)all.size())
        all[(size_t)wornIndex[s]] = item;
}

void Gear::cubesInto(const Item &item, uint8_t *record) {
    std::memcpy(record + CUBES_AT, item.cubes.data(), CUBE_BYTES);
    record[CUBE_COUNT_AT] = item.cubeCount;
}

void Gear::cubesFrom(const uint8_t *record, Item &item) {
    std::memcpy(item.cubes.data(), record + CUBES_AT, CUBE_BYTES);
    item.cubeCount = (uint8_t)std::min<int>(record[CUBE_COUNT_AT], (int)(CUBE_BYTES / 4));
}
