#pragma once
// The game's generated names (the name generator, 0x611d0): two syllables, each picked from a table
// by a hash of the seed and race; townsfolk, tinted monsters and regions are named this way.
// The syllables are the owner's: tools/ZoneExtractor reads them from the running game into
// Extract/names.txt (380 lines in the game's table order).
#include <filesystem>
#include <string>
#include <vector>

class GameNames {
  public:
    bool load(const std::filesystem::path &namesTxt);
    bool loaded() const { return syllables.size() == 380; }
    // the name generator(seed, race); race -1 (none) takes the default syllables. Empty if not loaded.
    std::string name(int seed, int race) const;

  private:
    std::vector<std::string> syllables;
};
