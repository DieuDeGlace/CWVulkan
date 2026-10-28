#pragma once
// What a townsperson says, built the way Steam Cube World builds it (the conversation
// builder, RVA 0x2d9480, and its townsperson part 0x12a6f0). The result is a script: the
// same list of tokens TalkScripts holds (speech ids, emotes, pauses and define_ commands),
// which playScript turns into lines.
//
// A script depends on:
//   - the speaker: its id, race, class, flags, home and where it stands;
//   - the player: class, specialisation, gear, pet, gold, special items and missions;
//   - the world's day (every draw is seeded by the id and the day, so it changes daily),
//     the time of day (innkeepers only) and the world's list of pets;
//   - the world generator: the planner of the speaker's home region (its places' roles,
//     artifacts and encounters), place kinds, realms and lands (worldgen/Generator.h).
//
// The behaviour is specified in docs/specs/conversation-script.md; "spec 4.6" and the like below
// name its sections.
#include "worldgen/ZoneRecords.h"

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace talkgen {

// --- Random numbers (spec 2) ---------------------------------------------------------------

// The xorshift step every draw goes through: shifts 13, 17 and 5.
uint32_t xorshift(uint32_t n);
// The base of every draw in a person's script on a day: xorshift((id mod 333333) + 13 * day).
uint32_t scriptSeed(uint64_t id, int day);

// The C runtime's rand(), which the game keeps as one generator for its whole thread. The
// talk draws from it for a person's relative (unseeded, so it carries on from earlier calls)
// and seeds it for the "strong creatures" small talk. Keep one for the session.
struct CRand {
    uint32_t state = 1; // as after srand(1), the runtime's start
    void seed(uint32_t value) { state = value; }
    int next() {
        state = state * 214013u + 2531011u;
        return (int)((state >> 16) & 0x7fff);
    }
};

// --- Inputs (spec 1) ---------------------------------------------------------------------

// The speaking creature, from its game-layout record (worldgen/Residents.h).
struct Speaker {
    uint64_t id = 0;              // +0x08
    int64_t x = 0, y = 0;         // +0x10, +0x18: where it stands now, in 1/65536 blocks
    int hostility = 0;            // +0x60 (3: a friendly townsperson)
    int race = 0;                 // +0x64
    uint32_t appearance = 0;      // +0x80 (bit 0x2000: a mission resident)
    uint16_t toggles = 0;         // +0x128 (bit 0x2: calling for help; 0x100: its task is done)
    int cls = 0;                  // +0x14c
    int stage = 0;                // +0x19c (the arena master's; 0xff once the fight is over)
    int64_t homeX = 0, homeY = 0; // +0x1b8, +0x1c0: where it was made, in 1/65536 blocks
};
Speaker speakerOf(const uint8_t *record);

using Cell = std::array<int32_t, 2>; // a 1024-block place cell (a big place's even corner)

// The local player.
struct Player {
    int cls = 0, spec = 0; // class 1..4 and specialisation 0/1
    // The ten equipment slots in the game's order (an unnamed first slot, neck, chest, feet,
    // hands, shoulder, left and right weapon, left and right ring); category 0 is empty.
    std::array<worldgen::Item, 10> equipment{};
    worldgen::Item pet; // the pet slot: category 0 for none, its id the creature kind
    int gold = 0;
    // The missions (each false when unset): finished, completed (finished or solved) and
    // known (marked by a townsperson, or solved).
    std::function<bool(Cell)> finished, completed, known;
    // Whether the player has special item `id` of its current region (4 always counts),
    // and whether it carries an item (an artifact).
    std::function<bool(int id)> hasSpecial;
    std::function<bool(const worldgen::Item &)> carries;
};

struct World {
    int day = 0;
    float timeOfDayMs = 0;            // milliseconds since midnight
    std::vector<int32_t> pets;        // the creature kinds that can be pets (world+0x360)
};

// --- The script (spec 3) ---------------------------------------------------------------------

// What the speaker says to the player now. `rand` is the session's C rand() (above). `branch`, when
// given, receives which part of the builder spoke ("help: gnome", "small talk 7"...), for tests.
std::vector<std::string> script(const Speaker &speaker, const Player &player, const World &world, CRand &rand,
                                std::string *branch = nullptr);

// Whether a race is female (0x5ee20): the female playable races, nomad women (84) and 228.
bool femaleRace(int race);

// --- A realm's draws (spec 5.4), shared by the land history and the lore tales (Lore.h) ---

// Whether xorshift(n) is odd: the draws' coin toss.
inline bool odd(uint32_t n) { return (xorshift(n) & 1) != 0; }
// The realm's kind, 12..22, which names it (0x10cee0).
int realmKind(uint32_t realm);
// Its ruler's race (0x10f320): one of the people's races, male or female.
int rulerRace(uint32_t realm);
// Its ruler's title, 0..25: odd for a ruler of a female race (0x10ec60).
int rulerTitle(uint32_t realm);
// Its ruler's person id (define_mob's first number).
int32_t rulerId(uint32_t realm);
// What the realm's nation is made of, drawn with `n` (0x10eb10).
int nationModel(uint32_t realm, uint32_t n);
// The year the realm's history starts in, 100..9099 (0x10ead0).
int32_t realmYear(uint32_t realm);
// Whether the realm's civilisation perished.
bool perishedRealm(uint32_t realm);
// The faction it fights, 1..8 (define_faction's number).
int realmFaction(uint32_t realm);
// The realm's magic, a grammar tag (spec 6.4; 0x10e6f0).
const char *realmMagic(uint32_t realm);
// A good and a bad trait drawn from a number (spec 6.5, 6.6; 0x10d000, 0x10bbd0).
const char *goodTrait(uint32_t n);
const char *badTrait(uint32_t n);
// The ruler's trait: good or bad, as the realm decides.
bool goodRuler(uint32_t realm);
const char *rulerTrait(uint32_t realm);
// The relatives a race is drawn from (spec 5.6): itself alone for most races.
std::vector<int> siblingGroup(int race);

} // namespace talkgen
