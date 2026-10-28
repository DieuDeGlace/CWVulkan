#pragma once
// The player's gear as the game measured it (ZoneExtractor F8 gear survey,
// Extract/character/gear.txt). For each equipment slot the survey recorded the
// player's stats with the slot empty and with each carried item in it, and the
// sprite the game drew for it. Item types and materials are the game's own ids
// (category, id, material); stats come from the game's stat functions, so an
// item's effect here is exactly what it did in the game for this character.
#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <vector>

class Gear {
  public:
    struct Stats {
        float maxHp = 0, attack = 0, armor = 0, critical = 0;
        Stats operator+(const Stats &o) const { return {maxHp + o.maxHp, attack + o.attack, armor + o.armor, critical + o.critical}; }
        Stats operator-(const Stats &o) const { return {maxHp - o.maxHp, attack - o.attack, armor - o.armor, critical - o.critical}; }
    };
    // The item record's cube entries (+0x1d .. +0x9c) and their count (+0x9d).
    static constexpr size_t CUBES_AT = 0x1d, CUBE_BYTES = 0x80, CUBE_COUNT_AT = 0x9d;
    struct Item {
        int category = 0, id = 0, material = 0, rarity = 0;
        unsigned modifier = 0;
        int region[2] = {0, 0}; // the region it comes from (game item +0xc): it counts fully only there
        int formula = 0;        // a formula's item category (+0x18)
        int level = 0;            // pyro progression: the item's level (0 = unknown)
        bool found = false;       // picked up in CubeVulkan (not from the survey)
        int tab = -1, index = -1; // inventory position, -1 when it was equipped
        std::string sprite;       // what the game drew for it in its slot ("" = nothing)
        Stats adds;               // what it added to the player's stats
        bool measured = false;
        // The upgrade cubes set into a weapon (loot-crafting.md §4), as the item record keeps
        // them: four bytes each (x, y, z, material) from +0x1d, their number at +0x9d.
        std::array<uint8_t, CUBE_BYTES> cubes{};
        uint8_t cubeCount = 0;
    };
    // Copies an item's cubes into a 0xa0-byte item record, or takes them from one.
    static void cubesInto(const Item &item, uint8_t *record);
    static void cubesFrom(const uint8_t *record, Item &item);
    // Equipment slots in the game's order.
    enum Slot { Unk, Neck, Chest, Feet, Hands, Shoulder, WeaponLeft, WeaponRight, RingLeft, RingRight, Pet, SLOTS };

    // `dir`: the character's recording folder (Saves.h).
    bool load(const std::filesystem::path &dir, std::string &error);
    int playerClass() const { return classType; }
    // Stats with the currently equipped items (the game's totals at survey time
    // when nothing has been changed).
    Stats stats() const;
    const Item &equipped(Slot s) const { return worn[s]; }
    // Sprite the game draws for slot `s` as equipped now ("" = keep the recorded one).
    std::string sprite(Slot s) const;
    std::string originalSprite(Slot s) const { return originalWorn[s].sprite; }
    // Equips the next carried item that the survey measured in slot `s`
    // (cycling back to the original). Returns false if there is nothing else.
    bool cycle(Slot s, const std::function<bool(const Item &)> &allowed = nullptr);
    std::string describe(const Item &item) const;
    // Everything measured (worn originally or carried), for the inventory screen.
    size_t itemCount() const { return all.size(); }
    const Item &item(size_t i) const { return all[i]; }
    Slot slotOf(size_t i) const { return allSlot[i]; }
    bool isWorn(size_t i) const;
    void equip(size_t i);   // into its slot; what was there goes back to the list
    void unequip(Slot s);   // the slot is left empty
    // Adds an item found in CubeVulkan to the carried items; returns its index.
    size_t add(const Item &item, Slot slot);
    // Takes item `i` out (sold): unworn first; the indices after it move down.
    void remove(size_t i);
    // Replaces item `i` (new stats); worn copies follow.
    void update(size_t i, const Item &item);
    // Replaces what slot `s` wears, and its entry in the list when it has one (the upgrade cubes
    // set into a worn weapon).
    void updateWorn(Slot s, const Item &item);
    // A measured item for slot `s` to scale a new item's stats from (same category
    // preferred); null if the survey measured nothing that fits.
    const Item *reference(Slot s, int category) const;
    bool wornMeasured(Slot s) const { return wornIndex[s] >= 0; }
    // What the character carried that is no gear (food, potions, resources...), with how
    // many, as the survey listed it.
    const std::vector<std::pair<Item, int>> &carriedExtras() const { return extras; }

  private:
    int classType = 0;
    Item worn[SLOTS], originalWorn[SLOTS]; // now, and as surveyed
    std::vector<Item> carried;          // measured inventory items, with their slot
    std::vector<Slot> carriedSlot;
    Stats totals;                       // the game's totals with the original gear
    std::map<int, std::string> empty;   // slot -> sprite drawn when it is empty
    std::vector<Item> all;
    std::vector<std::pair<Item, int>> extras;
    std::vector<Slot> allSlot;
    int wornIndex[SLOTS] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
    Stats base() const;
};
