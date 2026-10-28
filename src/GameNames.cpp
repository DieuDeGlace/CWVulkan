#include "GameNames.h"

#include <fstream>

bool GameNames::load(const std::filesystem::path &namesTxt) {
    syllables.clear();
    std::ifstream in(namesTxt);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        syllables.push_back(line);
    }
    return loaded();
}

namespace {
// The hash the name generator picks a syllable with (0..100000).
unsigned nameHash(int x, int y) {
    unsigned n = (unsigned)x + (unsigned)y * 57u; // (32-bit wraparound, as the game's int arithmetic)
    n = (n << 13) ^ n;
    return 100000u - ((n * (n * n * 60493u + 19990303u) + 1376312589u) & 0x7fffffffu) / 21474u;
}

// The table pair for a race: the index of its first half among the game's halves (the
// default ones are 0 and 1, 30 syllables each; then each race's first and second, 10 each).
int tablePair(int race) {
    switch (race) {
    case 0: return 3;                                                   // elf male
    case 1: return 4;                                                   // elf female
    case 2: case 18: case 43: case 83: case 202: case 208: return 1;    // human male, old man, wizard, nomad, pirates
    case 3: case 45: case 84: case 203: return 2;                       // human female, witch, nomad, pirate
    case 4: return 5;                                                   // goblin male
    case 5: return 6;                                                   // goblin female
    case 7: return 7;                                                   // lizardman male
    case 8: return 8;                                                   // lizardman female
    case 9: return 9;                                                   // dwarf male
    case 10: return 10;                                                 // dwarf female
    case 11: case 260: return 11;                                       // orc male
    case 12: case 261: return 12;                                       // orc female
    case 13: return 15;                                                 // frogman male
    case 14: return 16;                                                 // frogman female
    case 15: case 17: case 96: case 97: case 165: case 269: return 13;  // undead male, skeletons, zombie, vampire, monstrosity
    case 16: return 14;                                                 // undead female
    default: return 0;
    }
}
} // namespace

std::string GameNames::name(int seed, int race) const {
    if (!loaded())
        return {};
    const unsigned first = nameHash(seed, race), second = nameHash(race + 3852967, seed);
    const int pair = tablePair(race);
    if (pair == 0)
        return syllables[first % 30] + syllables[30 + second % 30];
    // The race tables follow the default ones in the order the game declares them: human male,
    // human female, elf male, elf female, goblin, lizardman, dwarf, orc, undead, frogman (each
    // male then female), a first half and a second half of 10 each.
    static const int order[17] = {0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    const size_t at = 60 + (size_t)order[pair] * 20;
    return syllables[at + first % 10] + syllables[at + 10 + second % 10];
}
