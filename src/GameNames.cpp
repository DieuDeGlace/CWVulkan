#include "GameNames.h"
#include "worldgen/Noise.h"

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

std::string GameNames::name(std::int32_t seed, std::int32_t selector) const {
    if (!loaded())
        return {};
    const auto seedWord = static_cast<std::uint32_t>(seed), selectorWord = static_cast<std::uint32_t>(selector);
    const auto first = worldgen::random(seedWord + 57u * selectorWord);
    const auto second = worldgen::random(57u * seedWord + selectorWord + 0x3acaa7u);
    const auto table = namefragments::layout(selector);
    return syllables[table.prefix + first % table.count] + syllables[table.suffix + second % table.count];
}
