#include "VillagerQuests.h"

#include "worldgen/Places.h"
#include "worldgen/Residents.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace villagerquests {
namespace {
constexpr int FRIENDLY = 3, ENEMY = 1, VILLAGER = 138, VILLAGE = 5;
constexpr uint32_t PETRIFIED = 0x400000, CLASS_MASTER = 0x40;

template <class T> T at(const uint8_t *record, size_t offset) {
    T value;
    std::memcpy(&value, record + offset, sizeof value);
    return value;
}
double squared(const glm::dvec3 &a, const glm::dvec3 &b) {
    const glm::dvec3 d = a - b;
    return glm::dot(d, d);
}
} // namespace

int Request::needed() const { return int(std::find(targets.begin(), targets.end(), int64_t(0)) - targets.begin()); }

int Request::progress() const {
    int count = 0;
    for (int i = 0; i < needed(); ++i)
        count += (defeated >> i) & 1;
    return count;
}

bool Request::recordKill(int64_t id) {
    if (rewarded || !id)
        return false;
    const auto found = std::find(targets.begin(), targets.end(), id);
    if (found == targets.end())
        return false;
    const uint8_t bit = uint8_t(1u << (found - targets.begin()));
    if (defeated & bit)
        return false;
    defeated |= bit;
    return true;
}

Creature fromRecord(const uint8_t *record) {
    namespace c = worldgen::creature;
    Creature out;
    out.id = at<int64_t>(record, c::ID);
    out.hostility = record[c::HOSTILITY];
    out.classType = record[c::CLASS];
    out.race = at<int32_t>(record, c::RACE);
    out.health = at<float>(record, c::HEALTH);
    out.flags = at<uint32_t>(record, c::FLAGS);
    for (int axis = 0; axis < 3; ++axis)
        out.at[axis] = double(at<int64_t>(record, c::POSITION + 8 * size_t(axis))) / 65536.0;
    return out;
}

bool offersService(int classType) { return (classType >= 128 && classType <= 137) || (classType >= 154 && classType <= 160); }

bool inVillage(int32_t x, int32_t y) {
    const worldgen::Place p = worldgen::placeAt(x, y);
    return p.kind == VILLAGE && p.weight > 0.0f;
}

bool nearVillage(int32_t x, int32_t y, float margin) {
    const int32_t cx = int32_t(std::floor(x / 1024.0)), cy = int32_t(std::floor(y / 1024.0));
    for (int32_t i = cx - 1; i <= cx + 1; ++i)
        for (int32_t j = cy - 1; j <= cy + 1; ++j) {
            const bool big = worldgen::bigCell(i, j);
            if (worldgen::placeKind(i, j, big) != VILLAGE)
                continue;
            const worldgen::Int2 centre = worldgen::cellPoint(i, j, big);
            const double reach = double(worldgen::placeRadius(big)) + margin;
            const double dx = double(x) - centre.x, dy = double(y) - centre.y;
            if (dx * dx + dy * dy < reach * reach)
                return true;
        }
    return false;
}

bool canOffer(const Creature &giver, const std::vector<Creature> &everyone) {
    if (giver.classType != VILLAGER || giver.hostility != FRIENDLY || giver.health <= 0.0f || (giver.flags & (PETRIFIED | CLASS_MASTER)))
        return false;
    if (!inVillage(int32_t(std::floor(giver.at.x)), int32_t(std::floor(giver.at.y))))
        return false;
    // A villager in a shop or beside its keeper: Talk there reaches the keeper's service.
    return std::none_of(everyone.begin(), everyone.end(), [&](const Creature &other) {
        return offersService(other.classType) && squared(other.at, giver.at) < double(SERVICE_REACH) * SERVICE_REACH;
    });
}

namespace {
bool available(const Creature &giver, const Creature &target, const std::vector<Request> &existing, const std::string &world) {
    if (!target.id || target.hostility != ENEMY || target.health <= 0.0f)
        return false;
    if (squared(target.at, giver.at) >= double(OFFER_RANGE) * OFFER_RANGE ||
        nearVillage(int32_t(std::floor(target.at.x)), int32_t(std::floor(target.at.y)), TOWN_MARGIN))
        return false;
    return std::none_of(existing.begin(), existing.end(), [&](const Request &other) {
        return other.world == world && !other.rewarded && std::find(other.targets.begin(), other.targets.end(), target.id) != other.targets.end();
    });
}
} // namespace

bool selectTargets(const Creature &giver, const std::vector<Creature> &everyone, const std::vector<Request> &existing,
                   const std::string &world, Request &request) {
    struct Candidate {
        const Creature *creature;
        double distance2;
    };
    std::vector<Candidate> nearby;
    for (const Creature &creature : everyone)
        if (available(giver, creature, existing, world))
            nearby.push_back({&creature, squared(creature.at, giver.at)});
    if (nearby.empty())
        return false;
    const auto order = [](const Candidate &a, const Candidate &b) {
        return a.distance2 != b.distance2 ? a.distance2 < b.distance2 : a.creature->id < b.creature->id;
    };
    std::sort(nearby.begin(), nearby.end(), order);
    const Creature &anchor = *nearby.front().creature;
    request.race = anchor.race;
    request.targetAt = blocks(anchor.at);
    request.targets = {};
    int count = 0;
    for (const Candidate &candidate : nearby) {
        const Creature &c = *candidate.creature;
        if (c.race == anchor.race && squared(c.at, anchor.at) <= double(TARGET_GROUP_RADIUS) * TARGET_GROUP_RADIUS) {
            request.targets[size_t(count++)] = c.id;
            if (count == TARGET_LIMIT)
                break;
        }
    }
    return count > 0;
}

bool offers(const Creature &giver, const std::vector<Creature> &everyone, const std::vector<Request> &requests, const std::string &world) {
    if (uint64_t(giver.id) % 3 != 0)
        return false;
    int open = 0;
    for (const Request &r : requests) {
        if (r.world != world)
            continue;
        if (r.giver == giver.id)
            return false;
        open += !r.rewarded;
    }
    if (open >= ACTIVE_LIMIT || !canOffer(giver, everyone))
        return false;
    return std::any_of(everyone.begin(), everyone.end(), [&](const Creature &target) { return available(giver, target, requests, world); });
}

std::array<int32_t, 3> blocks(const glm::dvec3 &at) {
    return {int32_t(std::floor(at.x)), int32_t(std::floor(at.y)), int32_t(std::floor(at.z))};
}

std::string direction(const std::array<int32_t, 3> &from, const std::array<int32_t, 3> &to) {
    static const char *const NAMES[] = {"east", "northeast", "north", "northwest", "west", "southwest", "south", "southeast"};
    const double x = double(to[0]) - from[0], y = double(to[1]) - from[1];
    const double distance = std::hypot(x, y);
    if (distance < 10.0)
        return "nearby";
    constexpr double OCTANTS_PER_RADIAN = 1.2732395447351627; // 4 / pi
    const int octant = (int(std::lround(std::atan2(y, x) * OCTANTS_PER_RADIAN)) + 8) % 8;
    return std::to_string(int(distance)) + " blocks " + NAMES[octant];
}

} // namespace villagerquests
