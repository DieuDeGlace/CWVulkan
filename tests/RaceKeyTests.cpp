// Checks of the race-number-to-dictionary-key table (src/RaceKeys.inc) against
// docs/specs/race-keys.md, and that its keys name entries of the owner's dict_en.xml.
#include "Assets.h"
#include "Speech.h"
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <utility>

namespace {
#include "RaceKeys.inc"

int failures = 0;
#define RACE_CHECK(x)                                                                              \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);                              \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

// The keys the game's own table names but its English dictionary has no entry for: the spec
// names Bullterrier; Vase (race 143) is the other one.
const std::set<std::string> NOT_IN_DICTIONARY = {"Bullterrier", "Vase"};

void table() {
    // 193 distinct race numbers, in order, each once.
    RACE_CHECK(std::size(RACE_KEYS) == 193);
    bool ascending = true;
    for (size_t i = 1; i < std::size(RACE_KEYS); i++)
        ascending = ascending && RACE_KEYS[i - 1].first < RACE_KEYS[i].first;
    RACE_CHECK(ascending);
    const std::map<int, std::string> keys(std::begin(RACE_KEYS), std::end(RACE_KEYS));
    auto key = [&](int race) { auto it = keys.find(race); return it == keys.end() ? std::string() : it->second; };
    RACE_CHECK(key(0) == "ElfMale" && key(28) == "Wolf" && key(36) == "Porcupine" && key(62) == "Mosquito");
    RACE_CHECK(key(172) == "Makake" && key(80) == "Spectrino" && key(230) == "Necromancer");
    RACE_CHECK(key(260) == "OrcMale" && key(261) == "OrcFemale" && key(304) == "Gnome" && key(305) == "Gnome");
    // Gaps the game's table leaves.
    for (int race : {84, 89, 95, 129, 151, 156, 164, 181, 190, 194, 199, 202, 208, 212, 214, 218, 219, 221, 223, 227,
                     228, 231, 259, 262, 291, 297, 303, 306})
        RACE_CHECK(key(race).empty());
}

void dictionary() {
    Assets assets;
    Speech speech;
    std::string why;
    if (!assets.locate("") || !speech.load(assets.gameDir() / "dict_en.xml", why)) {
        std::printf("race keys: no dict_en.xml in the install, dictionary check skipped\n");
        return;
    }
    int missing = 0;
    for (const auto &[race, key] : RACE_KEYS) {
        const bool listed = NOT_IN_DICTIONARY.count(key) > 0;
        if (speech.hasEntry(key) == listed) {
            std::printf("race %d: key %s %s the dictionary\n", race, key, listed ? "is now in" : "is not in");
            missing++;
        }
    }
    RACE_CHECK(missing == 0);
}
} // namespace

int raceKeyTests() {
    failures = 0;
    table();
    dictionary();
    return failures;
}
