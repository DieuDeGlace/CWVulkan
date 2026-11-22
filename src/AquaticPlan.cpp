#include "AquaticPlan.h"

#include <algorithm>
#include <cmath>
#include <iterator>

namespace aquatic {
namespace {
// Race numbers (src/RaceKeys.inc).
constexpr int SEA_CRAB = 107, CRAB = 106, SAPPHIRE_FISH = 145, LEMON_FISH = 146, SEAHORSE = 147, MERMAN = 149, SHARK = 150,
              LANTERN_FISH = 152, MAW_FISH = 153, PIRANHA = 154, BLOWFISH = 155, SNAIL = 295, CROCODILE = 75, SPITTER = 86,
              GREEN_SLIME = 37, BLUE_SLIME = 40;
constexpr int SEA_RACES[] = {SEA_CRAB, SAPPHIRE_FISH, LEMON_FISH, SEAHORSE, MERMAN, SHARK, LANTERN_FISH, MAW_FISH, PIRANHA, BLOWFISH, SNAIL};
constexpr int INLAND_RACES[] = {CRAB, SAPPHIRE_FISH, LEMON_FISH, SEAHORSE, SHARK, LANTERN_FISH, MAW_FISH, PIRANHA, BLOWFISH, SNAIL};
constexpr int BOSS_RACES[] = {CROCODILE, SHARK, MAW_FISH, PIRANHA, SEA_CRAB};
constexpr std::array<std::array<int, 2>, 7> SCHOOL_OFFSETS = {{{0, 0}, {3, 1}, {-3, -1}, {1, -3}, {-1, 3}, {4, -3}, {-4, 3}}};
constexpr int ZONE = 64;
constexpr uint8_t WATER = 2;

int32_t modulo(int32_t a, int32_t n) { return ((a % n) + n) % n; }
int32_t floorDivide(int32_t a, int32_t n) { return a >= 0 ? a / n : -((-a + n - 1) / n); }
} // namespace

uint32_t waterHash(int32_t x, int32_t y, int32_t seed) {
    uint32_t hash = uint32_t(x) * 0x9e3779b9u ^ uint32_t(y) * 0x85ebca6bu ^ uint32_t(seed);
    hash = (hash ^ (hash >> 16)) * 0x7feb352du;
    return hash ^ (hash >> 15);
}

bool passiveFish(int race) {
    return race == SAPPHIRE_FISH || race == LEMON_FISH || race == SEAHORSE || race == LANTERN_FISH || race == BLOWFISH || race == SNAIL;
}

bool bottomDweller(int race) { return race == CRAB || race == SEA_CRAB || race == SPITTER || (race >= GREEN_SLIME && race <= BLUE_SLIME); }

std::optional<School> plan(int32_t zx, int32_t zy, int32_t seed, const WaterColumn &column, const std::function<bool(int32_t, int32_t)> &ocean) {
    const uint32_t roll = waterHash(zx, zy, seed);
    const int32_t startX = zx * ZONE, startY = zy * ZONE;
    for (unsigned probe = 0; probe < 16; ++probe) {
        const unsigned slot = (probe + roll % 16) % 16;
        const int32_t x = startX + 8 + int32_t(slot % 4) * 16, y = startY + 8 + int32_t(slot / 4) * 16;
        int32_t floor, surface;
        if (!column(x, y, floor, surface))
            continue;
        const int depth = surface - floor;
        const bool sea = depth >= 6 && ocean(x, y);
        if (sea ? modulo(zx + zy, 2) != 0 : modulo(zx + zy + seed % 3, 3) != 0)
            continue;
        School school;
        school.boss = (roll >> 16) % 4 == 0 && depth >= 6;
        school.race = sea ? SEA_RACES[(roll >> 8) % std::size(SEA_RACES)] : INLAND_RACES[(roll >> 8) % std::size(INLAND_RACES)];
        if (school.boss) {
            school.race = BOSS_RACES[(roll >> 4) % (sea ? 5 : 4)];
        } else if (!sea && (roll >> 20) % 3 == 0) { // a lake's own kind
            const uint32_t lake = waterHash(floorDivide(x, 128), floorDivide(y, 128), seed);
            school.race = lake % 3 == 0 ? CRAB : lake % 3 == 1 ? SPITTER : GREEN_SLIME + int((lake / 3) % 4);
        }
        school.hostility = passiveFish(school.race) ? NEUTRAL : HOSTILE;
        school.stars = school.boss ? 4 : int((roll >> 11) % 3);
        const unsigned wanted = school.boss ? 1 : 2 + (roll >> 5) % 2;
        for (const auto &offset : SCHOOL_OFFSETS) {
            const int32_t px = x + offset[0], py = y + offset[1];
            int32_t low, high;
            if (!column(px, py, low, high) || high - low < 2)
                continue;
            const int columnDepth = high - low;
            int below = 2 + int(roll % 3);
            if (sea && columnDepth >= 8 && (roll >> 4) % 2 != 0)
                below = columnDepth / 4 + int((roll >> 12) % uint32_t(columnDepth - columnDepth / 4 - 1));
            const double z = bottomDweller(school.race) ? low + 0.6 : std::clamp(double(high - below), low + 0.6, high - 0.6);
            school.members.push_back(glm::dvec3(px + 0.5, py + 0.5, z));
            if (school.members.size() >= wanted)
                break;
        }
        if (school.members.size() >= (school.boss ? 1u : 2u))
            return school;
    }
    return std::nullopt;
}

void Planner::update(const Env &env, const std::vector<glm::dvec3> &players, int32_t seed, double elapsedMs) {
    sinceUpdateMs += elapsedMs;
    if (sinceUpdateMs < 1000)
        return;
    sinceUpdateMs = 0;
    const WaterColumn column = [&](int32_t x, int32_t y, int32_t &floor, int32_t &surface) {
        const int ground = int(std::floor(env.ground(x, y)));
        int top = std::min(ground + 40, 400);
        const int lowest = std::max(ground - 60, -200);
        bool loaded = false;
        while (top > lowest) { // down through the air to the water; land or a roof first: none
            const uint8_t type = env.blockType(x, y, top, loaded);
            if (!loaded)
                return false;
            if (type == WATER)
                break;
            if (type != 0)
                return false;
            --top;
        }
        int bottom = top;
        while (bottom > lowest && env.blockType(x, y, bottom, loaded) == WATER)
            --bottom;
        floor = bottom + 1, surface = top + 1;
        return surface - floor >= 2;
    };
    for (const glm::dvec3 &player : players) {
        const int32_t pzx = floorDivide(int32_t(std::floor(player.x)), ZONE), pzy = floorDivide(int32_t(std::floor(player.y)), ZONE);
        for (int32_t zx = pzx - 2; zx <= pzx + 2; ++zx)
            for (int32_t zy = pzy - 2; zy <= pzy + 2; ++zy) {
                if (done.count({zx, zy}) || !env.zoneLoaded(zx, zy)) // a zone not in memory yet is planned when it is
                    continue;
                const auto school = plan(zx, zy, seed, column, env.ocean);
                if (!school) {
                    done.insert({zx, zy});
                    continue;
                }
                bool made = false;
                for (const glm::dvec3 &at : school->members) {
                    const int64_t id = env.make(*school, at);
                    if (!id)
                        continue;
                    made = true;
                    env.addFlags(id, FISH_FLAG | (school->boss ? BOSS_FLAG : 0u));
                }
                if (made)
                    done.insert({zx, zy});
            }
    }
}

} // namespace aquatic
