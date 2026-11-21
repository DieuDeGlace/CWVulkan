#pragma once
// CubeVulkan's villager requests ([Quests] CubeVulkanQuests, off by default; not the game's): some
// villagers ask the player to defeat a group of enemies outside the village and reward them on their
// return. The user's add-on, at parity with their later Infinite-Cubes Vulkan work. Who counts as a
// villager or a service keeper and what makes a village follow docs/specs/steam-resident-class-roles-20261121.md.
#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace villagerquests {

constexpr int ACTIVE_LIMIT = 3, TARGET_LIMIT = 5;
constexpr float OFFER_RANGE = 640, TOWN_MARGIN = 32, TARGET_GROUP_RADIUS = 96, SERVICE_REACH = 10;
constexpr int GOLD_PER_TARGET = 20;

// A request, kept with the character for the world it was given in.
struct Request {
    std::string world;
    int64_t giver = 0;
    int race = 0;
    std::array<int32_t, 3> giverAt{}, targetAt{}; // blocks
    std::array<int64_t, TARGET_LIMIT> targets{};  // the creatures to defeat; 0 ends the list
    uint8_t defeated = 0;                          // a bit per target
    bool rewarded = false;

    int needed() const;
    int progress() const;
    bool ready() const { return needed() > 0 && progress() == needed(); }
    // Marks a target defeated; false when the id is not one or already was.
    bool recordKill(int64_t id);
};

// What a request needs of a creature, from its record.
struct Creature {
    int64_t id = 0;
    int hostility = 0, classType = 0, race = 0;
    float health = 0;
    uint32_t flags = 0;
    glm::dvec3 at{0}; // blocks
};
Creature fromRecord(const uint8_t *record);

// Shopkeepers and the other residents whose Talk serves (classes 128..137 and 154..160).
bool offersService(int classType);
// Inside a village (place kind 5 with influence), or within `margin` blocks of one's radius.
bool inVillage(int32_t x, int32_t y);
bool nearVillage(int32_t x, int32_t y, float margin);
// A villager (class 138, friendly, alive, not petrified or a class master) in a village with no
// service keeper within SERVICE_REACH blocks.
bool canOffer(const Creature &giver, const std::vector<Creature> &everyone);
// Enemies outside every village by TOWN_MARGIN, within OFFER_RANGE of the giver, not wanted by another
// open request; the nearest leads, and up to TARGET_LIMIT of its race within TARGET_GROUP_RADIUS of it
// (nearest first, ties by id) are the targets.
bool selectTargets(const Creature &giver, const std::vector<Creature> &everyone, const std::vector<Request> &existing,
                   const std::string &world, Request &request);
// Whether a villager offers a request now: one in three by id, fewer than ACTIVE_LIMIT open in this world,
// none from it yet, able to offer, and a target available.
bool offers(const Creature &giver, const std::vector<Creature> &everyone, const std::vector<Request> &requests,
            const std::string &world);
// "N blocks northeast", or "nearby" within 10 blocks (east +x, north +y).
std::string direction(const std::array<int32_t, 3> &from, const std::array<int32_t, 3> &to);
std::array<int32_t, 3> blocks(const glm::dvec3 &at);

} // namespace villagerquests
