#pragma once
// What the game's items are, from the game itself:
//   * class rules: the game's own "may this class use this item" check, asked
//     for every category, id and material (Extract/character/class-rules.txt);
//   * models: the gear creatures were seen wearing (creatures.txt "equip=")
//     paired with the sprites the game drew for it, which also names each
//     type and material ("iron-greatmace5-random2": material iron, type greatmace);
//   * grips: how the game holds each weapon type, from the hand and weapon
//     matrices of creatures carrying one.
#include <glm/glm.hpp>
#include <filesystem>
#include <array>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

class CreatureRecordings;

class ItemCatalog {
  public:
    using Key = std::tuple<int, int, int>; // category, id, material
    struct Model {
        std::set<std::string> sprites; // every variant seen
        std::string type, material;    // from the sprite names
        int slot = -1;                 // equipment slot it was worn in
        bool inferred = false;         // not seen on a creature: named from the game's sprite sets
    };
    bool loadClassRules(const std::filesystem::path &file, std::string &error);
    // Pairs creatures' equipment with the sprites they wore (creatures.txt lines).
    size_t loadModels(const std::filesystem::path &creaturesTxt);
    // Weapon grips (weapon matrix relative to its hand) per weapon sprite type.
    void learnGrips(const CreatureRecordings &creatures);
    // Ranger armor no recorded creature wore: materials 27, 31 and 33 are linen, felt
    // and mail, the game's three remaining 5-sprite armor sets (warrior iron / bronze /
    // steel, rogue cotton / wool / rivet and mage silk / flannell / filament take the
    // others, in the same tier order). Added where missing; see docs/gear.md.
    void addInferredRangerArmor();

    // The game's rule; true when unknown (no rules loaded or outside the table).
    bool mayUse(int category, int id, int material, int playerClass) const;
    const Model *model(int category, int id, int material) const;
    const std::map<Key, Model> &models() const { return known; }
    // Grip for a weapon type ("dagger"), or nullptr if no creature held one.
    const glm::mat4 *grip(const std::string &type) const;
    std::string typeOfSprite(const std::string &sprite) const;
    std::string summary() const;

  private:
    std::map<Key, std::array<bool, 4>> rules;
    std::map<Key, Model> known;
    std::map<std::string, glm::mat4> grips;
    std::map<std::string, int> gripSamples;
};
