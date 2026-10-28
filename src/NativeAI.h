#pragma once
// The game's creature AI (docs/specs/creature-ai.md). The game runs it on the server
// world's own thread: 0x83f20 calls the tick 0x13830 about every 40 ms with the ms since
// its last call. A tick copies each creature the server runs into its AI record, picks what
// it does (the turn 0x14e00 and its behaviours: riding along, guarding, fighting, greeting,
// following, idling), then moves each record toward its goal (a block search, 0x15640) and
// gives the creatures the way they want to go (+0x40) and what they do. Creatures are
// game-layout records (nativepose::CREATURE_SIZE bytes) with the containers the game keeps
// beside them; an AI record is the game's map node (0x178 bytes) with its containers.
#include <array>
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <set>
#include <vector>

namespace nativeai {

// A block (x, y, z), ordered as the game's maps order it.
using IntVec = std::array<int32_t, 3>;
// A block as the world's lookup (0xe9a0) gives it: colour, type (+4) and flags (+5).
using Block = std::array<uint8_t, 6>;
// An interaction (0xc8 bytes: an item, then who and where).
using Interaction = std::array<uint8_t, 0xc8>;

// A creature and what the game keeps beside its record.
struct Creature {
    uint8_t *record = nullptr;                    // nativepose::CREATURE_SIZE bytes
    std::vector<std::array<uint8_t, 0x18>> buffs; // +0x980: type +0, floats +4 +8, source +0x10
    std::list<Interaction> interactions;          // +0xab8
    std::map<int64_t, float> aggro;               // +0xb88: creature id to how angry at it
    std::map<int64_t, float> damage;              // +0xb98: creature id to the damage it did
    std::vector<int32_t> skills;                  // +0xbd0: special abilities it may use
    std::set<int64_t> group;                      // +0xc08: the ids of its group
    std::set<int64_t> struck;                     // +0x9c8: whom its current attack hit
    std::set<int64_t> counted;                    // +0x9b8: of those, the ones already counted
    std::map<int32_t, float> cooldowns;           // +0xb78: skill to the ms before it may be used again
    std::vector<std::vector<std::array<uint8_t, 0xa4>>> inventory; // +0x9f0: tabs of stacks
    std::vector<int32_t> summonRaces;             // +0xbf0: the races a summoner may call up
};

// One block of a search (the step map): its costs and the block it came from.
struct Step {
    int32_t g = 0, h = 0, f = 0;
    IntVec from{};
};

// A creature's AI record (the map node, its bytes at node offsets) and its containers.
struct Entry {
    std::array<uint8_t, 0x178> raw{};
    std::list<Interaction> waiting;    // +0x78: interactions to hand to the creature
    std::map<IntVec, Step> steps;      // +0xc0: the search's blocks
    std::set<IntVec> open;             // +0xd0: those still to look from
    std::map<IntVec, Step>::iterator best; // +0xb8: the one nearest the goal, or steps.end()
    std::list<IntVec> path;            // +0x138: from the start toward the goal
    std::set<uint64_t> taken;          // +0x148: objects it has taken (a seat)
    std::list<std::array<uint8_t, 0x10>> other; // +0x158 (never touched by the tick)
    Entry() : best(steps.end()) {}
    Entry(const Entry &o) { *this = o; }
    Entry &operator=(const Entry &o);
};

// A zone's object (0xe0 bytes: type +0, position +8, ...), as the zone keeps them.
using Object = std::array<uint8_t, 0xe0>;

struct World {
    float time = 0;                           // world+0x440 (ms)
    float day = 0;                            // world+0xc: the time of day (ms)
    bool client = false;                      // world+0x358: a remote client runs only its player
    Creature *local = nullptr;                // world+0x448
    std::vector<Creature *> creatures;        // world+0x40, in the world's order
    std::map<int64_t, Creature *> byId;       // world+0x450
    std::function<Block(int x, int y, int z)> block;  // 0xe9a0 (a missing zone gives the default)
    std::function<bool(int zx, int zy)> zone;         // a zone (64 blocks square) is loaded
    std::function<const std::vector<Object> *(int zx, int zy)> objects; // a loaded zone's objects
    // An object's key in the creatures' `taken` sets (the game keeps its address).
    std::function<uint64_t(int zx, int zy, int index)> objectKey;
    // 0x31c0e0 on a loaded zone: a spot in the 16-block cell (x, y) to walk to (the zone
    // generator's; it draws its own rand() when it takes a spot from the cell's list).
    // Unset: such a creature stays where it is.
    std::function<bool(int cellX, int cellY, int64_t out[3])> spot;
    std::function<int()> rand;                // the AI thread's rand()
    std::function<uint32_t()> clock;          // timeGetTime()
    // What a tick tells the world.
    std::vector<std::array<uint8_t, 0x18>> aggroNotes; // world+0x140
    std::vector<std::array<uint8_t, 0x30>> buffNotes;  // world+0x150
    std::vector<int64_t> calls;                         // world+0x170
    // For tests: told each creature id as its turn starts.
    std::function<void(int64_t id)> turn;
};

// One tick of 0x13830 over the world's creatures (dt in ms).
void tick(World &world, std::map<int64_t, Entry> &ai, uint32_t dt);

// 0x2b5d90 over the world's blocks: whether `to` can be seen from `from` (dots), no
// farther than `range` blocks; with `glass`, glass (block flag 0x40) blocks the sight too.
bool canSee(World &world, const int64_t *from, const int64_t *to, bool glass, float range);
// 0x2d1d20: how far a ray from `from` along `dir` (one long) gets through open blocks
// (with `glass`, not through glass either), at most `limit` blocks.
float ray(World &world, const int64_t *from, const float *dir, float limit, bool glass);
// 0x654a0: where a creature's shots start (in front of it, for those that shoot from the body).
void shotPoint(const uint8_t *record, int64_t out[3]);
// What the attack keys and the AI's attacks start (0x8ae50 calls them with the class-skill
// key as `force`): 0x60460, the next hit of the normal attack; 0x60720, charging the strong
// one; 0x5fc40, which hit is next.
bool startHit(Creature *c, bool force);
bool startCharge(Creature *c, bool force);
int nextHit(Creature *c, bool force);
// 0x507b0 (nativerules::canStartAbility) with the creature's buffs and cooldowns.
bool canUse(Creature *c, int ability);

} // namespace nativeai
