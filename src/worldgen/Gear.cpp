// A creature's gear, written from docs/specs/worldgen-creatures.md §4.3. The steps are named A-H
// as there.
#include "worldgen/Gear.h"

#include "worldgen/CreatureTables.h"
#include "worldgen/StructureTables.h"

namespace worldgen {
namespace {

// Item categories of worn things.
namespace cat {
constexpr uint8_t CONSUMABLE = 1;
constexpr uint8_t WEAPON = 3;
constexpr uint8_t CHEST = 4;
constexpr uint8_t GLOVES = 5;
constexpr uint8_t BOOTS = 6;
constexpr uint8_t SHOULDERS = 7;
constexpr uint8_t AMULET = 8;
constexpr uint8_t RING = 9;
} // namespace cat

void put(Item &it, uint8_t category, int32_t id, uint8_t material) {
    it.category = category, it.id = id, it.material = material;
}
void clear(Item &it) { it.category = 0, it.id = 0, it.material = 0; }

struct Worn {
    Gear &g;
    Item &neck() { return g.equipment[NECK]; }
    Item &chest() { return g.equipment[CHEST]; }
    Item &feet() { return g.equipment[FEET]; }
    Item &hands() { return g.equipment[HANDS]; }
    Item &shoulder() { return g.equipment[SHOULDER]; }
    Item &left() { return g.equipment[LEFT_HAND]; }
    Item &right() { return g.equipment[RIGHT_HAND]; }
    void both(uint8_t category, int32_t id, uint8_t material) { put(right(), category, id, material), put(left(), category, id, material); }
};

void add(std::vector<int32_t> &list, std::initializer_list<int32_t> values) { list.insert(list.end(), values); }

// C: the classes and races with fixed gear. True when it goes straight to E.
bool fixedGear(const GearInput &in, Draw rand, Worn w) {
    const uint8_t cls = w.g.cls;
    if (cls == 0x8a) {
        put(w.chest(), cat::CHEST, rand() % 5 + 2, 6);
        return true;
    }
    if ((cls >= 0x80 && cls <= 0x87) || cls == 0x8b || cls == 0x9b) {
        put(w.chest(), cat::CHEST, 0, 6);
        return true;
    }
    if (in.race != 0x12) {
        switch (in.race) {
        case 0xfc: put(w.right(), cat::WEAPON, 10, 4); return true;
        case 0xfd: put(w.right(), cat::WEAPON, 10, 0x2c); return true;
        case 0x100:
            put(w.right(), cat::WEAPON, 0xf, 4);
            w.chest().material = 0xb;
            w.hands().category = cat::GLOVES, w.hands().material = 0xb;
            w.feet().category = cat::BOOTS, w.feet().material = 0xb;
            return true;
        case 0x106:
            w.chest().material = 4, w.hands().material = 4, w.feet().material = 4;
            return true;
        case 0xfe:
            put(w.right(), cat::WEAPON, 5, 5);
            w.shoulder().category = cat::SHOULDERS, w.shoulder().material = 5;
            w.chest().category = cat::CHEST, w.chest().material = 5;
            w.hands().category = cat::GLOVES, w.hands().material = 5;
            w.feet().category = cat::BOOTS, w.feet().material = 5;
            return true;
        case 0xff: case 0x103: return true;
        default: return false;
        }
    }
    if (cls == 0) {
        put(w.chest(), cat::CHEST, 0, 6);
        return true;
    }
    return false;
}

// D7: the races with their own weapons and abilities. True when the race is one of them.
bool raceWeapons(int32_t race, Draw rand, Worn w) {
    Gear &g = w.g;
    switch (race) {
    case 0x75:
        add(g.abilities, {0x69, 0x2f, 0x65});
        g.abilityPick = 0;
        add(g.abilities2, {0x11, 0x60});
        return true;
    case 0x6c: case 0x72: put(w.right(), cat::WEAPON, 0x11, 2), clear(w.left()); return true;
    case 0x6e: put(w.right(), cat::WEAPON, 5, 5), clear(w.left()); return true;
    case 0x2e: put(w.right(), cat::WEAPON, 0x11, 7), clear(w.left()); return true;
    case 0x73: add(g.abilities, {0x5f, 0x67}), w.both(cat::WEAPON, 4, 1); return true;
    case 0x6d: add(g.abilities, {0x5f, 0x67, 100}), w.both(cat::WEAPON, 2, 7); return true;
    case 0x51: add(g.abilities, {0x60}), w.both(cat::WEAPON, 0xc, 0xb); return true;
    case 0x52: w.both(cat::WEAPON, 0, 1); return true;
    case 0x2f: case 0x6f: case 0x70: case 0x71: return true;
    case 0x2b: case 0x2d: case 0xa7: case 0xc7: case 0xe6: // staff casters
        if (race == 0xe6) {
            add(g.abilities, {0x65});
            g.abilityPick = 0;
            add(g.abilities2, {0x11, 0x60, 0xa5});
        }
        add(g.abilities, {0x2f});
        g.spec = race == 0xa7 ? 0 : 2;
        put(w.right(), cat::WEAPON, rand() % 2 + 10, 2);
        clear(w.left());
        g.cls = 0;
        return true;
    default: return false;
    }
}

// D9: the weapons of a class.
void classWeapons(Draw rand, Worn w) {
    Gear &g = w.g;
    auto swords = [&](uint8_t material) {
        put(w.right(), cat::WEAPON, rand() % 3 + 0xf, material);
        clear(w.left());
    };
    switch (g.cls) {
    case 1: case 0x8d: {
        if (g.cls == 1)
            g.spec = (uint8_t)(rand() % 2);
        switch (rand() % 3) {
        case 0: swords(1); break;
        case 1: {
            const int a = rand() % 3;
            put(w.right(), cat::WEAPON, a, 1);
            put(w.left(), cat::WEAPON, rand() % 3, 1);
            break;
        }
        default:
            put(w.right(), cat::WEAPON, rand() % 3, 1);
            put(w.left(), cat::WEAPON, 0xd, 1);
            break;
        }
        break;
    }
    case 2:
        g.spec = (uint8_t)(rand() % 2);
        put(w.right(), cat::WEAPON, rand() % 3 + 6, 2);
        break;
    case 3:
        g.spec = (uint8_t)(rand() % 2);
        switch (rand() % 3) {
        case 0: put(w.right(), cat::WEAPON, 10, 2), clear(w.left()); break;
        case 1: put(w.right(), cat::WEAPON, 0xb, 2), clear(w.left()); break;
        default: w.both(cat::WEAPON, 0xc, 0xb); break;
        }
        break;
    case 4: {
        g.spec = (uint8_t)(rand() % 2);
        const int32_t id = rand() % 3 + 3;
        put(w.right(), cat::WEAPON, id, 1);
        if (id == 5)
            clear(w.left());
        else
            put(w.left(), cat::WEAPON, id, 1);
        break;
    }
    case 0x8e:
        switch (rand() % 4) {
        case 0: swords(5); break;
        case 1:
            put(w.right(), cat::WEAPON, rand() % 2 == 0 ? 1 : 0, 5);
            put(w.left(), cat::WEAPON, rand() % 2 == 0 ? 1 : 0, 5);
            break;
        case 2: put(w.right(), cat::WEAPON, 5, 5); break;
        default: put(w.right(), cat::WEAPON, 10, 5); break;
        }
        break;
    default: break;
    }
}

// D11, D12: armour of materials suiting the weapon, then the jewellery.
void armour(const GearInput &in, Draw rand, Worn w) {
    const bool people6 = in.people == 6;
    std::vector<uint8_t> list;
    const int32_t id = w.right().id;
    if (id >= 10 && id <= 12)
        list = people6 ? std::vector<uint8_t>{0x23} : std::vector<uint8_t>{0x1a, 0x20, 0x23};
    else if (id >= 6 && id <= 8)
        list = people6 ? std::vector<uint8_t>{0x21} : std::vector<uint8_t>{0x1b, 0x1f, 0x21};
    else if (id >= 3 && id <= 5)
        list = people6 ? std::vector<uint8_t>{0x22} : std::vector<uint8_t>{0x1c, 0x1e, 0x22};
    else
        list = people6 ? std::vector<uint8_t>{4} : std::vector<uint8_t>{1, 0x1d, 4};
    if (w.g.cls == 0x8d)
        list = {0xb};
    else if (w.g.cls == 0x8e)
        list = {5};
    auto material = [&] { return list[(size_t)(rand() % (int)list.size())]; };
    w.chest().category = cat::CHEST, w.chest().material = material();
    w.shoulder().category = cat::SHOULDERS, w.shoulder().material = material();
    w.hands().category = cat::GLOVES, w.hands().material = material();
    w.feet().category = cat::BOOTS, w.feet().material = material();
    auto jewel = [&](Item &it, uint8_t category) {
        it.category = category;
        it.material = (uint8_t)((rand() % 2 == 0) + 0xb);
    };
    jewel(w.neck(), cat::AMULET);
    jewel(w.g.equipment[RIGHT_RING], cat::RING);
    jewel(w.g.equipment[LEFT_RING], cat::RING);
}

// D: the general case.
void generalGear(const GearInput &in, Draw rand, Worn w) {
    Gear &g = w.g;
    const uint32_t f = in.flags;
    // 1-3.
    if (g.cls == 0x90) {
        if (!(f & looksflag::PLAIN_CHEST))
            put(w.chest(), cat::CHEST, 0, 0x18);
        return;
    }
    if (g.cls == 0x8f) {
        if (!(f & looksflag::PLAIN_CHEST)) {
            put(w.chest(), cat::CHEST, 0, 0x2c);
            put(w.right(), cat::WEAPON, 10, 2);
            w.right().modifier = (uint32_t)rand();
        }
        return;
    }
    if (g.cls == 0x88) {
        put(w.chest(), cat::CHEST, 1, 6);
        w.right().category = cat::WEAPON, w.right().id = 0x15;
        return;
    }
    // 4-6.
    for (Item *it : {&w.left(), &w.right(), &w.shoulder(), &w.chest(), &w.feet(), &w.hands(), &w.neck()})
        it->category = 0, it->id = 0;
    if (in.hostility == hostility::ANIMAL || in.hostility == hostility::PET)
        return;
    if ((f & (looksflag::TAKE_DOWN_WEAPONS | looksflag::ONE_WEAPON)) || in.race == 0x76) {
        w.right().category = 0, w.right().id = 0;
        w.left().category = 0, w.left().id = 0;
        return;
    }
    // 7.
    if (raceWeapons(in.race, rand, w))
        return;
    // 8.
    if (g.cls == 0)
        g.cls = (uint8_t)(rand() % 4 + 1);
    if (in.race == 0x61)
        g.cls = 4;
    else if (in.race == 0xa5)
        g.cls = 1;
    // 9-12.
    classWeapons(rand, w);
    if (f & looksflag::PLAIN_CHEST)
        return;
    armour(in, rand, w);
}

// E: weapons and chest by flags.
void flagGear(uint32_t f, Worn w) {
    if (f & looksflag::TAKE_DOWN_WEAPONS) {
        w.right().category = cat::WEAPON, w.right().id = 0x16;
        w.left().category = cat::WEAPON, w.left().id = 0x16;
    }
    if (f & looksflag::ONE_WEAPON) {
        w.right().category = cat::WEAPON, w.right().id = 0x17;
        w.left() = Item();
    }
    if (f & looksflag::PLAIN_CHEST)
        w.chest().category = cat::CHEST, w.chest().id = 1;
}

// F: class and abilities by race.
void raceAbilities(int32_t race, Gear &g) {
    auto cls = [&](uint8_t c) { g.cls = c; };
    auto summoner = [&](std::initializer_list<int32_t> second) {
        g.cls = 0x99;
        g.abilityPick = 0;
        add(g.abilities2, second);
    };
    switch (race) {
    case 0x11f: case 0x120: case 0x122: case 0xe2: cls(0x92), add(g.abilities, {0x88}); break;
    case 0x11: case 0x5e: case 0x61: add(g.abilities, {0x67}); break;
    case 0x15: add(g.abilities, {0x46}); break;
    case 0x19: case 0xae: case 0xd7: add(g.abilities, {0x49}); break;
    case 0x2e: add(g.abilities, {0x5f}), cls(0x94); break;
    case 0x2f: case 0x6f: cls(0x92), add(g.abilities, {0x7b}); break;
    case 0x45: cls(0x91), add(g.abilities, {0x82}); break;
    case 0x46: cls(0x93), add(g.abilities, {0x82}); break;
    case 0x47: case 0x48: add(g.abilities, {0x82}); break;
    case 0x51: add(g.abilities, {0x65, 0x88}); break;
    case 0x55: case 0x6e: add(g.abilities, {0x88}); break;
    case 0x52: add(g.abilities, {0x5f}); break;
    case 0x56: cls(0x98), g.spec = 1; break;
    case 0x57: cls(0x92), add(g.abilities, {0x82}); break;
    case 0x58: cls(0x93), add(g.abilities, {0x82}); break;
    case 0x5a: cls(0x92); break;
    case 0x68: cls(0x95); break;
    case 0x6c: case 0x6d: case 0x72: add(g.abilities, {0x37, 100, 0x67}); break;
    case 0x70: cls(0x92), add(g.abilities, {0x88, 0x2f}); break;
    case 0xaf: summoner({0x55}); break;
    case 0xb0: summoner({0x28, 0x25, 0x26, 0x27}); break;
    case 0xb1: summoner({0x35}); break;
    case 0xb2: summoner({0x60}); break;
    case 0xb3: summoner({0x11}); break;
    case 0xb4: summoner({0x57}); break;
    case 200: case 0xc9: add(g.abilities, {0x4b}); break;
    case 0xe1: add(g.abilities, {0x69}), g.spec = 0; break;
    case 0x106: cls(0x92), add(g.abilities, {0x5f}); break;
    case 0x10d: cls(0x92), add(g.abilities, {0x37}); break;
    default: break;
    }
}

// G: a strong creature's extra ability.
void strongAbility(uint32_t f, Draw rand, Gear &g) {
    if (!(f & looksflag::STRONG))
        return;
    static constexpr int32_t ABILITIES[9] = {0x65, 0x67, 0x5f, 0x62, 0x60, 0x37, 0x73, 0x71, 0x2f};
    const int32_t a = ABILITIES[rand() % 9];
    g.abilities.push_back(a);
    if (a == 0x65) {
        g.abilityPick = rand() % 3;
        add(g.abilities2, {0x11, 0x60});
    }
}

// H: modifiers and levels.
void modifiersAndLevels(const GearInput &in, Draw rand, Gear &g) {
    static constexpr EquipSlot FIRST[9] = {LEFT_HAND, RIGHT_HAND, SHOULDER, CHEST, HANDS, FEET, NECK, LEFT_RING, RIGHT_RING};
    static constexpr EquipSlot SECOND[9] = {RIGHT_HAND, LEFT_HAND, SHOULDER, CHEST, HANDS, FEET, NECK, LEFT_RING, RIGHT_RING};
    for (int k = 0; k < 9; k++)
        g.equipment[FIRST[k]].modifier = in.hostility == hostility::TAMED ? xorshift((uint32_t)in.race + 100u * (uint32_t)k) : (uint32_t)rand();
    for (EquipSlot s : SECOND)
        g.equipment[s].rarity = (uint8_t)in.level;
    for (EquipSlot s : SECOND)
        g.equipment[s].modifier = (uint32_t)rand();
}

} // namespace

// --- The inventory (§4.5) -----------------------------------------------------------------------

namespace {

// The same item, for stacking (0x4df60). Generation leaves an item's attachments empty.
bool sameItem(const Item &a, const Item &b) {
    return a.category == b.category && a.id == b.id && a.modifier == b.modifier && a.region == b.region && a.realm == b.realm &&
           a.material == b.material && a.rarity == b.rarity;
}

constexpr uint8_t COUNTED_CATEGORY = 12, COUNTED_MATERIAL = 11, COUNTED_CATEGORY_14 = 14;

} // namespace

int Inventory::tabOf(uint8_t category) {
    switch (category) {
    case 3: case 4: case 5: case 6: case 7: case 8: case 9: case 15: case 25: case 26: return 0;
    case 22: case 24: case 29: return 1;
    case 2: case 11: return 3;
    case 20: case 21: return 4;
    case 23: return 5;
    default: return 2;
    }
}

bool Inventory::stackable(uint8_t category) {
    switch (category) {
    case 1: case 10: case 11: case 12: case 13: case 18: case 28: return true;
    default: return false;
    }
}

void Inventory::add(const Item &item, int32_t quantity, int tab) {
    if (quantity == 0)
        return;
    if (tab == -1)
        tab = tabOf(item.category);
    if (item.category == 0)
        return;
    if ((int)tabs.size() <= tab)
        tabs.resize((size_t)tab + 1);
    if (item.category == COUNTED_CATEGORY && item.material == COUNTED_MATERIAL) {
        countedMaterial += quantity;
        return;
    }
    if (item.category == COUNTED_CATEGORY_14) {
        countedCategory14 += quantity;
        return;
    }
    std::vector<Stack> &stacks = tabs[(size_t)tab];
    Stack *firstEmpty = nullptr;
    for (Stack &s : stacks) {
        if (s.quantity == 0 && !firstEmpty)
            firstEmpty = &s;
        if (stackable(item.category) && sameItem(s.item, item)) {
            s.quantity = quantity == -1 ? -1 : s.quantity + quantity;
            return;
        }
    }
    if (firstEmpty) {
        firstEmpty->item = item;
        firstEmpty->quantity = quantity;
    } else {
        stacks.push_back({quantity, item});
    }
}

void makeGear(const GearInput &in, Draw rand, Gear &g) {
    // A.
    g.equipment = {};
    // B.
    if (carriesConsumables(in.race)) {
        const int n = rand() % 4;
        if (n - 1 > 0) {
            Item potion;
            potion.category = cat::CONSUMABLE, potion.id = 1;
            g.inventory.add(potion, n - 1);
        }
    }
    // C, D.
    Worn w{g};
    if (!fixedGear(in, rand, w))
        generalGear(in, rand, w);
    // E-H.
    flagGear(in.flags, w);
    raceAbilities(in.race, g);
    strongAbility(in.flags, rand, g);
    modifiersAndLevels(in, rand, g);
}

} // namespace worldgen
