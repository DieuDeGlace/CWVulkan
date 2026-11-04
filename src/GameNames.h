#pragma once
// The game's generated names (the name generator, 0x611d0): two syllables, each picked from a table
// by wrapped hashes of the seed and original selector (often a race). Townsfolk,
// tinted monsters, item names and regions use this shared routine.
// The syllables are the owner's: tools/ZoneExtractor reads them from the running game into
// Extract/names.txt (380 lines in the game's table order).
#include "NameLayout.h"
#include <filesystem>
#include <string>
#include <vector>

class GameNames {
  public:
    bool load(const std::filesystem::path &namesTxt);
    bool loaded() const { return syllables.size() == namefragments::COUNT; }
    // The original signed selector participates in both wrapped hashes before
    // table dispatch. Shared-table aliases retain their distinct inputs.
    // Unknown selectors (including -1 and 6) use generic fragments. Empty if unloaded.
    std::string name(std::int32_t seed, std::int32_t selector) const;

  private:
    std::vector<std::string> syllables;
};
