#pragma once
// CubeVulkan's travelling parties ([Creatures] TravelParties, off by default; not the game's): groups of
// two to four of a faction walk from one village to the next and back. The user's add-on, at parity
// with their later Infinite-Cubes Vulkan work: half are hostile (they keep out of villages), the parties
// leave the world when nobody is near and walk on unseen, and return when a player comes close.
#include "worldgen/Regions.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <random>
#include <set>
#include <vector>

class SpawnedCreatures;

namespace travelparties {

constexpr int MAX_PARTIES = 6;
constexpr double SPAWN_DISTANCE = 155, PARK_DISTANCE = 300, RETURN_NEAR = 100, RETURN_FAR = 220;
constexpr double ARRIVAL = 110, HOSTILE_VILLAGE_MARGIN = 64, UNSEEN_SPEED = 10; // blocks, blocks per second
constexpr double SPAWN_COOLDOWN_MS = 60000, RETRY_MS = 5000, ARRIVAL_WAIT_MS = 15000, UNSEEN_LIMIT_MS = 300000;

// A village's centre and radius.
struct Village {
    worldgen::Int2 centre;
    double radius = 250;
};
// The villages whose place cells lie within 7 cells of the point's (big places once, at their even corner).
std::vector<Village> villagesAround(double x, double y);
// Whether (x, y) is within `margin` blocks of a village's radius.
bool nearVillage(double x, double y, double margin);
// Where a party goes: the village nearest `from` and the next nearest, or with `otherRegion` the nearest
// in a different region when there is one. False with fewer than two villages.
bool chooseRoute(const std::vector<Village> &villages, glm::dvec2 from, bool otherRegion, Village &origin, Village &destination);
// The next waypoint from `at` towards `to`: a 22-block stretch, else 10, straight first and then detours
// of 16 and 27 blocks (scaled with the stretch) to either side, whose order turns with `variation`; the
// ground may climb 4 + 0.9 x the stretch; hostile parties keep 64 blocks outside every village.
// `ground` gives a column's height (below 1: water).
template <class Ground>
bool nextWaypoint(glm::dvec2 at, glm::dvec2 to, bool hostile, unsigned variation, Ground ground, glm::dvec2 &next);

struct Party {
    Village origin, destination;
    glm::dvec2 position{0};
    bool hostile = false;
    int stars = 0;
    std::vector<int> races;       // one per member
    std::vector<int64_t> members; // their ids while in the world (empty while parked)
    double waitMs = 0, unseenMs = 0;
    unsigned variation = 0;
    glm::dvec2 waypoint{0};
    bool hasWaypoint = false;
};

// Runs the parties on the authoritative world. Positions are absolute blocks.
class Manager {
  public:
    // `players`: every player's position; `playerReady`: the local player stands on the ground outside menus.
    void update(SpawnedCreatures &world, const std::vector<glm::dvec3> &players, bool playerReady, double elapsedMs);
    const std::vector<Party> &parties() const { return list; }
    bool member(int64_t id) const;
    void clear() { list.clear(); }

  private:
    bool spawn(SpawnedCreatures &world, const glm::dvec3 &player);
    void place(SpawnedCreatures &world, Party &party);
    std::vector<Party> list;
    double sinceUpdateMs = 0, spawnDelayMs = 0;
    std::mt19937 rng{0x7a4e1u};
};

// --- template definition ------------------------------------------------------------------------

template <class Ground>
bool nextWaypoint(glm::dvec2 at, glm::dvec2 to, bool hostile, unsigned variation, Ground ground, glm::dvec2 &next) {
    const glm::dvec2 toward = to - at;
    const double length = glm::length(toward);
    if (length < 1e-6)
        return false;
    const glm::dvec2 forward = toward / length, side(-forward.y, forward.x);
    const double here = ground(at.x, at.y);
    for (double stretch : {22.0, 10.0}) {
        const double step = std::min(stretch, length);
        const double scale = step / 22.0;
        double offsets[5] = {0, 16 * scale, -16 * scale, 27 * scale, -27 * scale};
        if ((variation / 5) % 2) // the detours' order turns every five updates
            std::swap(offsets[1], offsets[2]), std::swap(offsets[3], offsets[4]);
        for (double offset : offsets) {
            const glm::dvec2 candidate = at + forward * step + side * offset;
            const double height = ground(candidate.x, candidate.y);
            if (height < 1 || height - here > 4 + 0.9 * stretch)
                continue;
            if (hostile && nearVillage(candidate.x, candidate.y, HOSTILE_VILLAGE_MARGIN))
                continue;
            next = candidate;
            return true;
        }
    }
    return false;
}

} // namespace travelparties
