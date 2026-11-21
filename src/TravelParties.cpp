#include "TravelParties.h"

#include "Spawned.h"
#include "worldgen/Height.h"
#include "worldgen/Places.h"
#include "worldgen/Residents.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>

namespace travelparties {
namespace {
constexpr int VILLAGE = 5, HOSTILE = 1, FRIENDLY = 3;
constexpr uint32_t FRIENDLY_FLAG = 1u << 15;
constexpr double MEMBER_SPACING = 6, STRAY = 24, WAYPOINT_REACHED = 6;

// The factions (by race; male and female alike). Friendly: Order of the Light (humans), Druids of
// Mana (elves), Explorers Guild (dwarves), Monster Hunters (goblins). Hostile: Cult of Doom (dark
// cultists), Steel Empire (empire soldiers), Obsidian Knights (humans), Bloodaxe Clan (orcs).
const std::vector<std::vector<int>> FRIENDLY_FACTIONS = {{2, 3}, {0, 1}, {9, 10}, {4, 5}};
const std::vector<std::vector<int>> HOSTILE_FACTIONS = {{227}, {236}, {2, 3}, {11, 12}};

double ground(double x, double y) { return worldgen::groundHeight(int32_t(std::floor(x)), int32_t(std::floor(y))); }
glm::dvec3 at(const uint8_t *record) {
    int64_t p[3];
    std::memcpy(p, record + worldgen::creature::POSITION, sizeof p);
    return glm::dvec3(p[0], p[1], p[2]) / 65536.0;
}
float health(const uint8_t *record) {
    float h;
    std::memcpy(&h, record + worldgen::creature::HEALTH, sizeof h);
    return h;
}
void setHome(uint8_t *record, glm::dvec2 where) {
    const int64_t home[3] = {int64_t(where.x * 65536), int64_t(where.y * 65536), int64_t((std::floor(ground(where.x, where.y)) + 1) * 65536)};
    std::memcpy(record + worldgen::creature::HOME, home, sizeof home);
    uint32_t roam;
    std::memcpy(&roam, record + worldgen::creature::ROAM, sizeof roam);
    roam |= 2; // walk to the home, as road travel does
    std::memcpy(record + worldgen::creature::ROAM, &roam, sizeof roam);
}
double nearestPlayer(const std::vector<glm::dvec3> &players, glm::dvec2 p) {
    double best = 1e18;
    for (const auto &q : players)
        best = std::min(best, glm::length(glm::dvec2(q) - p));
    return best;
}
} // namespace

std::vector<Village> villagesAround(double x, double y) {
    std::vector<Village> out;
    std::set<std::pair<int32_t, int32_t>> seen;
    const int32_t cx = int32_t(std::floor(x / 1024.0)), cy = int32_t(std::floor(y / 1024.0));
    for (int32_t i = cx - 7; i <= cx + 7; ++i)
        for (int32_t j = cy - 7; j <= cy + 7; ++j) {
            const bool big = worldgen::bigCell(i, j);
            const int32_t ki = big ? 2 * int32_t(std::floor(i / 2.0)) : i, kj = big ? 2 * int32_t(std::floor(j / 2.0)) : j;
            if (!seen.insert({ki, kj}).second || worldgen::placeKind(i, j, big) != VILLAGE)
                continue;
            out.push_back({worldgen::cellPoint(i, j, big), double(worldgen::placeRadius(big))});
        }
    return out;
}

bool nearVillage(double x, double y, double margin) {
    for (const Village &v : villagesAround(x, y)) {
        const double reach = v.radius + margin;
        if (glm::length(glm::dvec2(v.centre.x, v.centre.y) - glm::dvec2(x, y)) < reach)
            return true;
    }
    return false;
}

bool chooseRoute(const std::vector<Village> &villages, glm::dvec2 from, bool otherRegion, Village &origin, Village &destination) {
    if (villages.size() < 2)
        return false;
    std::vector<Village> sorted = villages;
    std::stable_sort(sorted.begin(), sorted.end(), [&](const Village &a, const Village &b) {
        return glm::length(glm::dvec2(a.centre.x, a.centre.y) - from) < glm::length(glm::dvec2(b.centre.x, b.centre.y) - from);
    });
    origin = sorted[0];
    destination = sorted[1];
    if (otherRegion) {
        const worldgen::Int2 home = worldgen::regionOf(origin.centre.x, origin.centre.y);
        for (size_t i = 1; i < sorted.size(); ++i)
            if (worldgen::regionOf(sorted[i].centre.x, sorted[i].centre.y) != home) {
                destination = sorted[i];
                break;
            }
    }
    return true;
}

bool Manager::member(int64_t id) const {
    for (const Party &p : list)
        if (std::find(p.members.begin(), p.members.end(), id) != p.members.end())
            return true;
    return false;
}

void Manager::place(SpawnedCreatures &world, Party &party) {
    const glm::dvec2 toward = glm::dvec2(party.destination.centre.x, party.destination.centre.y) - party.position;
    const glm::dvec2 forward = glm::length(toward) > 1e-6 ? glm::normalize(toward) : glm::dvec2(1, 0);
    const glm::dvec2 side(-forward.y, forward.x);
    party.members.clear();
    for (size_t i = 0; i < party.races.size(); ++i) {
        const glm::dvec2 p = party.position + side * (MEMBER_SPACING * double(i));
        const int64_t dots[3] = {int64_t(p.x * 65536), int64_t(p.y * 65536), int64_t((std::floor(ground(p.x, p.y)) + 1) * 65536)};
        const int64_t id = world.make(party.races[i], party.hostile ? HOSTILE : FRIENDLY, party.stars, dots,
                                      worldgen::regionOf(int32_t(p.x), int32_t(p.y)));
        if (!id)
            continue;
        if (uint8_t *r = world.record(id); r && party.hostile) {
            uint32_t flags;
            std::memcpy(&flags, r + worldgen::creature::FLAGS, sizeof flags);
            flags &= ~FRIENDLY_FLAG;
            std::memcpy(r + worldgen::creature::FLAGS, &flags, sizeof flags);
        }
        party.members.push_back(id);
    }
    world.setGroup(party.members);
}

bool Manager::spawn(SpawnedCreatures &world, const glm::dvec3 &player) {
    Party party;
    if (!chooseRoute(villagesAround(player.x, player.y), glm::dvec2(player), rng() % 3 != 0, party.origin, party.destination))
        return false;
    party.hostile = rng() % 2 == 0;
    party.stars = int(rng() % 5);
    const auto &factions = party.hostile ? HOSTILE_FACTIONS : FRIENDLY_FACTIONS;
    const auto &faction = factions[rng() % factions.size()];
    const int count = 2 + int(rng() % 3);
    for (int i = 0; i < count; ++i)
        party.races.push_back(faction[rng() % faction.size()]);
    // 155 blocks from the player towards the origin, or a little to one side.
    glm::dvec2 toward = glm::dvec2(party.origin.centre.x, party.origin.centre.y) - glm::dvec2(player);
    toward = glm::length(toward) > 1e-6 ? glm::normalize(toward) : glm::dvec2(1, 0);
    const glm::dvec2 side(-toward.y, toward.x);
    for (double offset : {0.0, 32.0, -32.0, 64.0, -64.0}) {
        const glm::dvec2 p = glm::dvec2(player) + toward * SPAWN_DISTANCE + side * offset;
        if (ground(p.x, p.y) < 1 || (party.hostile && nearVillage(p.x, p.y, HOSTILE_VILLAGE_MARGIN)))
            continue;
        party.position = p;
        place(world, party);
        if (party.members.empty())
            return false;
        list.push_back(std::move(party));
        return true;
    }
    return false;
}

void Manager::update(SpawnedCreatures &world, const std::vector<glm::dvec3> &players, bool playerReady, double elapsedMs) {
    sinceUpdateMs += elapsedMs;
    spawnDelayMs = std::max(0.0, spawnDelayMs - elapsedMs);
    if (sinceUpdateMs < 1000 || players.empty())
        return;
    const double dt = sinceUpdateMs;
    sinceUpdateMs = 0;
    if (playerReady && spawnDelayMs <= 0 && int(list.size()) < MAX_PARTIES)
        spawnDelayMs = spawn(world, players.front()) ? SPAWN_COOLDOWN_MS : RETRY_MS;
    for (auto it = list.begin(); it != list.end();) {
        Party &party = *it;
        ++party.variation;
        // Members that died or left the world drop out; a party with none left in the world is parked.
        std::vector<int64_t> alive;
        const uint8_t *leader = nullptr;
        for (int64_t id : party.members)
            if (const uint8_t *r = world.record(id); r && health(r) > 0) {
                alive.push_back(id);
                if (!leader)
                    leader = r;
            }
        const bool wasInWorld = !party.members.empty();
        if (wasInWorld && alive.empty()) { // all defeated or gone with their zone
            if (std::all_of(party.members.begin(), party.members.end(), [&](int64_t id) { return world.record(id) != nullptr; })) {
                it = list.erase(it);
                continue;
            }
        }
        party.members = alive;
        if (leader)
            party.position = glm::dvec2(at(leader));
        const bool seen = !party.members.empty() && std::any_of(party.members.begin(), party.members.end(), [&](int64_t id) {
            const uint8_t *r = world.record(id);
            return r && nearestPlayer(players, glm::dvec2(at(r))) <= PARK_DISTANCE;
        });
        if (!party.members.empty() && !seen) { // nobody near: leave the world, keep walking unseen
            for (int64_t id : party.members)
                world.remove(id);
            party.members.clear();
        }
        const double near = nearestPlayer(players, party.position);
        if (party.members.empty() && near >= RETURN_NEAR && near <= RETURN_FAR)
            place(world, party);
        if (party.members.empty()) {
            party.unseenMs += dt;
            if (party.unseenMs > UNSEEN_LIMIT_MS) {
                it = list.erase(it);
                continue;
            }
        } else {
            party.unseenMs = 0;
        }
        // Arrival: wait, then go back the way it came.
        const glm::dvec2 goal(party.destination.centre.x, party.destination.centre.y);
        const double arrive = party.hostile ? party.destination.radius + HOSTILE_VILLAGE_MARGIN + 10 : ARRIVAL;
        if (glm::length(goal - party.position) <= arrive) {
            party.waitMs += dt;
            if (party.waitMs >= ARRIVAL_WAIT_MS) {
                std::swap(party.origin, party.destination);
                party.waitMs = 0;
                party.hasWaypoint = false;
            }
            ++it;
            continue;
        }
        if (!party.hasWaypoint || glm::length(party.waypoint - party.position) <= WAYPOINT_REACHED)
            party.hasWaypoint = nextWaypoint(party.position, goal, party.hostile, party.variation, ground, party.waypoint);
        if (party.members.empty()) { // unseen: straight on at walking pace
            if (party.hasWaypoint) {
                const glm::dvec2 d = party.waypoint - party.position;
                const double step = UNSEEN_SPEED * dt / 1000;
                party.position = glm::length(d) <= step ? party.waypoint : party.position + glm::normalize(d) * step;
            }
        } else if (party.hasWaypoint) {
            for (int64_t id : party.members)
                if (uint8_t *r = world.record(id)) {
                    // The leader walks to the waypoint; a straggler first back to the party.
                    const bool straggler = glm::length(glm::dvec2(at(r)) - party.position) > STRAY;
                    if (id == party.members.front() || straggler)
                        setHome(r, straggler && id != party.members.front() ? party.position : party.waypoint);
                }
        }
        ++it;
    }
}

} // namespace travelparties
