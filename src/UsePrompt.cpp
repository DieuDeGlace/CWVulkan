#include "UsePrompt.h"
#include "GameMath.h"

#include "worldgen/Residents.h"
#include "NativeInteract.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace useprompt {

namespace {

// The fields of a creature record the use key reads (spec 0.2), besides worldgen::creature's.
constexpr size_t INTENT = 0x40;       // three floats: the way the movement keys want to go
constexpr size_t MOVE_FLAGS = 0x5c;   // bit 0 on the ground, bit 1 in water, bit 2 against a wall
constexpr size_t ABILITY_MS = 0x6c;   // float: time into the current ability
constexpr size_t SIZE = 0x84;         // three floats: size factors x, y, z
constexpr size_t DODGE_MS = 0x12c;    // float: the dodge roll's timer
constexpr size_t REVIVED = 0x140;     // float the revival sets to -0.5
constexpr size_t CARRIED = 0x1a0;     // i64: the creature the player carries
constexpr size_t PARTNER = 0x1a8;     // i64: who uses a bed creature (0: nobody)
constexpr size_t MISSION_ZONE = 0x1b0; // two i32: a zone key, 0x7fffffff for none
constexpr size_t HELD_ITEM = 0x1d8;   // an item held up or ridden
constexpr size_t OWNER = 0x968;       // i64: the owner id ground items may be reserved for
constexpr size_t AIM_DROP = 0x990;    // float: how far below its position a creature is aimed at
constexpr size_t STAMINA = 0x9a0;     // float
constexpr size_t PET = 0x9d8;         // i64: the player's active pet

// Creature flags (+0x80).
constexpr uint32_t CARRIABLE = 0x4000, MARKS_MISSION = 0x1000, USABLE = 0x200000, PETRIFIED = 0x400000;

// The player's abilities the use key starts or ends.
constexpr int CARRYING = 0x80, THROWING = 0x81, CLIMBING = 0x7a, HOLDING_UP = 0x7c, PETTING = 0xa2;
constexpr int GLIDING = 0x78, SAILING = 0x79, RIDING = 0x76, OTHER_VEHICLE = 0x89;

constexpr int32_t NO_ZONE = 0x7fffffff;

// Distances, squared blocks (spec 6.1).
constexpr float NEARBY2 = 16.0f;            // the nearby creature, object or item: 4 blocks
constexpr float CANDIDATE2 = 25600.0f;      // creatures within 160 blocks are looked at
constexpr float PICK_UP2 = 25.0f;           // a creature is taken up within 5 blocks
constexpr float CARRY_PENALTY2 = 16.0f;     // added for an item the player may not carry more of
constexpr float SIGHT_TO_CREATURE = 256.0f; // blocks
constexpr float SIGHT_TO_OBJECT = 8.0f;
constexpr float SIGHT_TO_ITEM = 200.0f;

const glm::vec4 SHORT_RED{1.0f, 0.3f, 0.3f, 1.0f}; // not enough items, full
const glm::vec4 RESERVED_RED{1.0f, 0.1f, 0.1f, 1.0f};

template <class T> T read(const uint8_t *r, size_t at) {
    T v;
    std::memcpy(&v, r + at, sizeof v);
    return v;
}
template <class T> void write(uint8_t *r, size_t at, T v) { std::memcpy(r + at, &v, sizeof v); }

int64_t idOf(const uint8_t *r) { return read<int64_t>(r, worldgen::creature::ID); }
int hostilityOf(const uint8_t *r) { return r[worldgen::creature::HOSTILITY]; }
int raceOf(const uint8_t *r) { return read<int32_t>(r, worldgen::creature::RACE); }
int abilityOf(const uint8_t *r) { return r[worldgen::creature::ABILITY]; }
uint32_t flagsOf(const uint8_t *r) { return read<uint32_t>(r, worldgen::creature::FLAGS); }
glm::vec3 sizeOf(const uint8_t *r) { return {read<float>(r, SIZE), read<float>(r, SIZE + 4), read<float>(r, SIZE + 8)}; }

float distance2(glm::vec3 a, glm::vec3 b) { return glm::dot(a - b, a - b); }

void startAbility(uint8_t *r, int ability) {
    r[worldgen::creature::ABILITY] = (uint8_t)ability;
    write<float>(r, ABILITY_MS, 0.0f);
}

// An ability is over once its time is past its whole length (0x4f220). An idle player
// (ability 0) counts as over after its first frame: in the game its time keeps running, while
// CubeVulkan's creature update holds it at the idle length.
bool abilityOver(const Scene &s) {
    const float ms = read<float>(s.player, ABILITY_MS);
    return ms > s.abilityDuration || (abilityOf(s.player) == 0 && ms > 0.0f);
}

// The abilities that leave friendly creatures out of the crosshair while they run (0x5eea0):
// every one but these.
bool offensive(int ability) {
    switch (ability) {
    case 0x33: case 0x34: case 0x50: case 0x6d: case 0x6f: case 0x70: case 0x71: case 0x72: case 0x73: case 0x86:
    case 0x87: case 0x8d: case 0x8e: case 0x8f: case 0x90: case 0x92: case 0x95:
        return false;
    default:
        return true;
    }
}

// The races of race category 4 (0x65750): the animals that can be petted (spec 6.3).
bool petCategory(int race) {
    auto in = [&](int low, int high) { return race >= low && race <= high; };
    return in(0x13, 0x24) || race == 0x32 || race == 0x35 || in(0x37, 0x44) || race == 0x4a || race == 0x4b ||
           in(0x57, 0x5d) || in(0x62, 0x64) || in(0x66, 0x6b) || race == 0x74 || in(0x91, 0x93) || in(0x96, 0x9f) ||
           in(0xa1, 0xa4) || in(0xa8, 0xac) || race == 0xae || race == 0xc8 || race == 0xc9 || in(0xd1, 0xd3) ||
           race == 0xd5 || race == 0xd7 || in(0xd9, 0xdc) || race == 0xe0 || race == 0x10e || race == 0x115 ||
           in(0x125, 0x127) || race == 0x12f;
}

// The races a pet of which cannot be taken up (0x5ef80, spec 6.3).
bool tooBigToCarry(int race) {
    auto in = [&](int low, int high) { return race >= low && race <= high; };
    return race == 0x13 || race == 0x14 || race == 0x16 || race == 0x17 || in(0x19, 0x28) || in(0x3f, 0x43) ||
           race == 0x4a || race == 0x4b || in(0x62, 0x64) || in(0x66, 0x69) || race == 0x97 || race == 0x9e ||
           race == 0xa3 || race == 0xa4 || race == 0xa6 || race == 0xad || race == 0xc8 || race == 0xc9 ||
           in(0xd1, 0xd3) || race == 0xd5 || race == 0xd7 || race == 0x106 || race == 0x107 || race == 0x10e ||
           in(0x125, 0x127);
}

// The static object types the targeting looks at even without an item (spec 2.3).
bool targetable(int type) {
    switch (type) {
    case 0x00: case 0x03: case 0x04: case 0x05: case 0x06: case 0x15: case 0x16: case 0x1b: case 0x1c: case 0x1d:
    case 0x1e: case 0x1f: case 0x20: case 0x3a: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57: case 0x5f:
    case 0x60: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67: case 0x68: case 0x73: case 0x76:
    case 0x77: case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x82:
        return true;
    default:
        return false;
    }
}

const Creature *findCreature(const Scene &s, int64_t id) {
    if (!id)
        return nullptr;
    for (const Creature &c : s.creatures)
        if (c.record && idOf(c.record) == id)
            return &c;
    return nullptr;
}

const uint8_t *recordOf(const Scene &s, int64_t id) {
    if (!id)
        return nullptr;
    if (s.creatureById)
        return s.creatureById(id);
    const Creature *c = findCreature(s, id);
    return c ? c->record : nullptr;
}

bool sight(const Scene &s, glm::vec3 from, glm::vec3 to, float limit) { return !s.sight || s.sight(from, to, limit); }

// Whether the crosshair leaves creature `c` out (spec 2.2, the seven reasons).
bool leftOut(const Scene &s, const uint8_t *c, bool hostile) {
    const uint8_t *me = s.player;
    const int mine = abilityOf(me);
    const bool running = !abilityOver(s);
    const int race = raceOf(c), hostility = hostilityOf(c);
    const bool staged = c[worldgen::creature::STAGE] != 0;
    if (idOf(c) == idOf(me))
        return true;
    if (mine == RIDING && idOf(c) == read<int64_t>(me, PET))
        return true;
    if (mine == 0x23 && running && (hostility == 1 || hostility == 7))
        return true;
    if (hostility == 8 && race >= 0xf4 && race <= 0xf7 && staged)
        return true;
    if (race == 0x11d || (race == 0x12e && staged))
        return true;
    if (running && offensive(mine) && !hostile)
        return true;
    if (running && (mine == 0x86 || mine == 0x87) && hostile)
        return true;
    return false;
}

// A creature's reach for talking (0x5ec30): half its largest size and 5 blocks, or half its
// width and 40 blocks in ability 0x5d.
float talkReach(const uint8_t *c) {
    const glm::vec3 size = sizeOf(c);
    if (abilityOf(c) == 0x5d)
        return 0.5f * size.x + 40.0f;
    return 0.5f * std::max({size.x, size.y, size.z}) + 5.0f;
}
float useReach(const uint8_t *c) { return 0.5f * sizeOf(c).x + 5.0f; }

// The creature prompts (spec 3.6). True when one applies.
bool creaturePrompt(const Scene &s, const Creature &target, Prompt &p) {
    const uint8_t *c = target.record;
    const int64_t id = idOf(c);
    const int hostility = hostilityOf(c), race = raceOf(c), cls = c[worldgen::creature::CLASS];
    const uint32_t flags = flagsOf(c);
    const float d2 = distance2(s.playerAt, target.at);
    const bool inTalkReach = d2 < talkReach(c) * talkReach(c);
    const bool inUseReach = d2 < useReach(c) * useReach(c);
    auto show = [&](const char *text, Action action) {
        p.text = text, p.action = action, p.creature = id, p.highlighted = id;
        return true;
    };

    const bool isPet = id == read<int64_t>(s.player, PET);
    if ((flags & CARRIABLE || (hostility == 1 && abilityOf(c) == 0x82) || (isPet && !tooBigToCarry(race))) && d2 < PICK_UP2)
        return show("Pick up", Action::PickUp);

    if (hostility == 3 && inTalkReach) {
        if (flags & PETRIFIED) {
            show("Examine", Action::ExaminePetrified);
            p.highlighted = 0;
            return true;
        }
        show("Talk", Action::Converse);
        switch (cls) {
        case 0x80: case 0x81: case 0x82: case 0x9c: p.action = Action::Shop; break;
        case 0x83: p.action = Action::ClassWindow; break;
        case 0x9a: p.action = Action::FlightMaster; break;
        default: break;
        }
        return true;
    }
    if ((hostility == 4 || hostility == 1) && petCategory(race) && abilityOver(s) && abilityOf(c) == 1 && inTalkReach)
        return show("Pet", Action::Pet);
    if (hostility == 4 && inTalkReach)
        return show("Interact", Action::Activate);
    if (inTalkReach && ((race >= 0xb5 && race <= 0xb8) || race == 0xf5 || (hostility == 8 && race >= 0xc2 && race <= 0xc5) ||
                        race == 0x10f || race == 0x111))
        return show("Open", Action::Activate);
    if (flags & USABLE && inTalkReach)
        return show("Use", Action::Activate);
    if (race == 0x10c && inTalkReach)
        return show("Use", Action::Shrine);
    if (race == 0xb9 && inUseReach) {
        p.station = 3;
        return show("Use", Action::Crafting);
    }
    if ((race == 0xbb || race == 0xbc) && inUseReach && read<int64_t>(c, PARTNER) == 0)
        return show("Sleep", Action::Activate);
    if (hostility == 8 && inUseReach)
        return show("Examine", Action::Activate);
    return false;
}

// The static object prompts (spec 3.7). Returns whether the routine takes the key.
bool objectPrompt(const Scene &s, const Targets &t, Prompt &p) {
    if (t.object < 0 || t.object >= (int)s.objects.size())
        return false;
    const Object &o = s.objects[(size_t)t.object];
    p.object = t.object;

    // An object waiting for an item (spec 3.7.1), whatever its type.
    if (o.item[0] != 0 && o.active) {
        p.icon = Prompt::Icon::ObjectItem;
        p.text = o.itemCount >= 2 ? std::to_string(o.itemCount) + " x " + o.itemName : o.itemName;
        if (o.carried < o.itemCount)
            p.colour = SHORT_RED;
        else
            p.action = Action::GiveItem;
        return true;
    }

    const bool mine = o.occupant == idOf(s.player);
    auto use = [&](const char *text, Action action) {
        p.text = mine ? "" : text;
        p.action = action;
        return true;
    };
    auto station = [&](const char *text, int mode) {
        if (mine)
            return true;
        p.text = text, p.action = Action::Crafting, p.station = mode;
        return true;
    };
    switch (o.type) {
    case 0x00:
        if (s.ownsGoddessGift)
            return false;
        p.text = "Examine", p.action = Action::GoddessGift;
        return true;
    case 0x03: case 0x04: case 0x05: case 0x06: case 0x15: case 0x54: case 0x55: case 0x56: case 0x57: case 0x59:
    case 0x5a: case 0x69: // doors, gates and hatches
        p.text = o.active ? "Open" : "Close", p.action = Action::UseObject;
        return true;
    case 0x16: // a chest
        if (o.active && !o.chestCellKnown && o.chestHasItem)
            p.text = "Open", p.action = Action::OpenChest;
        return true;
    case 0x1c: case 0x1e: case 0x1f: case 0x73: // seats: a press while seated stands up
        return use("Sit", Action::UseObject);
    case 0x20: case 0x5f: case 0x60: // beds
        return use("Sleep", Action::UseObject);
    case 0x3a: // the teleporter shows its word but does nothing here
        p.text = mine ? "" : "Teleport";
        return true;
    case 0x53: return station("Campfire", 3);
    case 0x62: return station("Furnace", 5);
    case 0x63: return station("Anvil", 0);
    case 0x64: return station("Spinning Wheel", 5);
    case 0x65: return station("Loom", 1);
    case 0x66: return station("Saw", 5);
    case 0x67: return station("Workbench", 0);
    case 0x68:
        if (!mine)
            p.text = "Customization Bench", p.action = Action::Customize;
        return true;
    case 0x76: case 0x77: case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x82:
        p.text = "Examine", p.action = Action::ExamineLore;
        return true;
    default:
        return false;
    }
}

// The prompt routine (0x98180, spec 3): true when it takes the key.
bool promptRoutine(const Scene &s, const Targets &t, Prompt &p) {
    const uint8_t *me = s.player;
    if (s.dodgeHeld || s.walkHeld || s.dialogOpen)
        return false;

    const int ability = abilityOf(me);
    if (ability == CLIMBING) {
        p.text = "Fall", p.action = Action::LetGo;
        return true;
    }
    const glm::vec3 intent{read<float>(me, INTENT), read<float>(me, INTENT + 4), read<float>(me, INTENT + 8)};
    const uint32_t moving = read<uint32_t>(me, MOVE_FLAGS);
    if (abilityOver(s) && read<float>(me, DODGE_MS) == 0.0f && ability != GLIDING && ability != RIDING && ability != SAILING &&
        glm::dot(intent, intent) > 0.1f && (moving & 5) == 4 && read<float>(me, STAMINA) > 0.0f) {
        p.text = "Climb", p.action = Action::Climb;
        return true;
    }

    // (Abilities 0x96 and 0x97 end here, keeping or letting go of a pet in the controller's
    // collection, spec 3.3; CubeVulkan has no such collection yet.)

    if (!(read<float>(me, worldgen::creature::HEALTH) > 0.0f)) {
        p.text = "Revive", p.action = Action::Revive;
        return true;
    }
    if (ability == CARRYING && recordOf(s, read<int64_t>(me, CARRIED))) {
        p.text = "Throw", p.action = Action::Throw;
        return true;
    }
    if (const Creature *target = findCreature(s, t.creature); target && creaturePrompt(s, *target, p))
        return true;
    if (objectPrompt(s, t, p))
        return true;
    p.object = -1;
    return false;
}

// When the prompt routine takes nothing (0x8ae50, spec 4).
void fallThrough(const Scene &s, const Targets &t, Prompt &p) {
    if (s.selectorShown) {
        p.action = Action::UseSelectedItem;
        return;
    }
    if (t.groundItem >= 0 && t.groundItem < (int)s.groundItems.size()) {
        const GroundItem &g = s.groundItems[(size_t)t.groundItem];
        p.groundItem = t.groundItem;
        p.icon = Prompt::Icon::GroundItem;
        if (g.reservedFor != 0 && g.reservedFor != read<int64_t>(s.player, OWNER)) {
            p.colour = RESERVED_RED, p.text = "Reserved";
            return;
        }
        const bool more = mayCarryMore(g.type, g.subtype, g.carriedCount);
        if (!more || !g.bagHasRoom)
            p.colour = SHORT_RED, p.text = "Full";
        else
            p.text = "Pick up";
        p.action = !more ? Action::CannotCarry : g.bagHasRoom ? Action::PickUpItem : Action::BagFull;
        p.bag = g.bag;
        return;
    }
    if (s.dialogOpen)
        return;
    const int type = s.vehicle[0], subtype = read<int32_t>(s.vehicle.data(), 4);
    p.action = Action::Vehicle;
    if (type == 0)
        return;
    p.icon = Prompt::Icon::Vehicle;
    const int ability = abilityOf(s.player);
    p.text = (ability == GLIDING && subtype == 1) || (ability == SAILING && subtype == 2) ? "Cancel" : s.vehicleName;
}

void handOver(nativeai::Creature &player, const nativeai::Interaction &in) { player.interactions.push_back(in); }

} // namespace

bool Camera::sees(glm::vec3 centre, float radius) const {
    const glm::vec3 v = centre - eye;
    const float depth = glm::dot(v, forward), across = glm::dot(v, right), upward = glm::dot(v, up);
    if (depth < -radius)
        return false;
    const float slackAcross = radius * std::sqrt(1.0f + tanHalfWidth * tanHalfWidth);
    const float slackUp = radius * std::sqrt(1.0f + tanHalfHeight * tanHalfHeight);
    return std::abs(across) <= depth * tanHalfWidth + slackAcross && std::abs(upward) <= depth * tanHalfHeight + slackUp;
}

bool Camera::crosshairIn(const std::array<glm::vec3, 8> &corners) const {
    bool left = false, right_ = false, below = false, above = false;
    for (const glm::vec3 &corner : corners) {
        const glm::vec3 v = corner - eye;
        const float depth = glm::dot(v, forward);
        if (!(depth > 0.0f))
            continue;
        // In front of the camera, the side of the middle a corner projects to is the side it is on.
        const float across = glm::dot(v, right), upward = glm::dot(v, up);
        left = left || across < 0.0f, right_ = right_ || across > 0.0f;
        below = below || upward < 0.0f, above = above || upward > 0.0f;
    }
    return left && right_ && below && above;
}

std::array<glm::vec3, 8> boxCorners(glm::vec3 low, glm::vec3 size) {
    std::array<glm::vec3, 8> corners;
    for (int k = 0; k < 8; k++)
        corners[(size_t)k] = low + glm::vec3(k & 1 ? size.x : 0.0f, k & 2 ? size.y : 0.0f, k & 4 ? size.z : 0.0f);
    return corners;
}

bool clearLine(glm::vec3 from, glm::vec3 to, float limit, const std::function<bool(int x, int y, int z)> &solid) {
    const glm::vec3 d = to - from;
    const float length = glm::length(d);
    if (length < 0.01f)
        return true;
    if (length > limit)
        return false;
    const float reach = length - 0.01f;
    const glm::vec3 dir = d / length;
    // Walk the blocks the ray passes through, in order (each step crosses one block face).
    glm::ivec3 block(std::floor(from.x), std::floor(from.y), std::floor(from.z));
    glm::ivec3 step(0);
    glm::vec3 nextFace(std::numeric_limits<float>::infinity()), perBlock(std::numeric_limits<float>::infinity());
    for (int a = 0; a < 3; a++) {
        if (dir[a] > 0.0f)
            step[a] = 1, nextFace[a] = ((float)block[a] + 1.0f - from[a]) / dir[a], perBlock[a] = 1.0f / dir[a];
        else if (dir[a] < 0.0f)
            step[a] = -1, nextFace[a] = (from[a] - (float)block[a]) / -dir[a], perBlock[a] = -1.0f / dir[a];
    }
    for (;;) {
        const int a = nextFace.x <= nextFace.y && nextFace.x <= nextFace.z ? 0 : nextFace.y <= nextFace.z ? 1 : 2;
        if (nextFace[a] >= reach)
            return true;
        block[a] += step[a];
        nextFace[a] += perBlock[a];
        if (solid(block.x, block.y, block.z))
            return false;
    }
}

bool mayCarryMore(int type, int subtype, int count) {
    if (type == 0x15 || type == 0x18)
        return count <= 0;
    if (type == 1 && (subtype == 0x0c || subtype == 0x0e))
        return count <= 4;
    if (type == 0x0b && subtype >= 0x27 && subtype <= 0x29)
        return count <= 9;
    if (type == 1 || type == 0x0b)
        return count <= 49;
    return true;
}

bool groundItemValid(const GroundItem &g, int64_t playerOwnerId) {
    if (g.type == 0 || g.quantity == 0)
        return false;
    if (g.type == 0x17 && g.carriesOne)
        return false;
    if (g.type == 0x1e && g.cellKnown)
        return false;
    if (g.type == 0x18 && g.ownsSpecial)
        return false;
    return g.reservedFor == 0 || g.reservedFor == playerOwnerId;
}

CreatureTargets chooseCreatures(const Scene &s) {
    CreatureTargets t;
    if (!s.player)
        return t;
    const Camera &camera = s.camera;

    // The creatures looked at (spec 2.1): alive, within 160 blocks and in view, nearest first.
    std::vector<std::pair<float, const Creature *>> near;
    for (const Creature &c : s.creatures) {
        if (!c.record || !(read<float>(c.record, worldgen::creature::HEALTH) > 0.0f))
            continue;
        float d2 = distance2(s.playerAt, c.at);
        if (camera.fixedEye) {
            int64_t player[3], drawn[3];
            std::memcpy(player, s.player + worldgen::creature::POSITION, sizeof player);
            std::memcpy(drawn, c.record + 0xb10, sizeof drawn);
            d2 = gamemath::fixedDistanceSquared(player, drawn);
        }
        const glm::vec3 size = sizeOf(c.record);
        if (d2 > CANDIDATE2 || !camera.sees(c.at, 0.6f * std::max(size.x, size.y)))
            continue;
        near.push_back({d2, &c});
    }
    std::stable_sort(near.begin(), near.end(), [](const auto &a, const auto &b) { return a.first < b.first; });

    // The creature under the crosshair nearest the view's middle line, and the nearest one
    // within 4 blocks (spec 2.2).
    int64_t nearbyCreature = 0;
    float nearbyCreature2 = -1.0f, bestOffLine = -1.0f;
    for (const auto &[d2, c] : near) {
        if (leftOut(s, c->record, c->hostileToPlayer))
            continue;
        const glm::vec3 aim = c->at - glm::vec3(0, 0, read<float>(c->record, AIM_DROP));
        if (!camera.sees(aim, 0.0f))
            continue;
        if (d2 < NEARBY2 && (nearbyCreature2 < 0.0f || d2 < nearbyCreature2))
            nearbyCreature = idOf(c->record), nearbyCreature2 = d2;
        const glm::vec3 box = sizeOf(c->record) + glm::vec3(1.0f);
        if (!camera.crosshairIn(boxCorners(aim - box * 0.5f, box)))
            continue;
        t.overCreature = true;
        bool visible;
        if (camera.fixedEye && s.fixedSight) {
            std::array<int64_t, 3> endpoint;
            std::memcpy(endpoint.data(), c->record + 0xb10, sizeof endpoint);
            const float drop = -read<float>(c->record, AIM_DROP) * 65536.0f;
            endpoint[2] = gamemath::addFixed(endpoint[2], int64_t(drop));
            visible = s.fixedSight(*camera.fixedEye, endpoint, SIGHT_TO_CREATURE);
        } else {
            visible = sight(s, camera.eye, aim, SIGHT_TO_CREATURE);
        }
        if (!visible)
            continue;
        // Depth and perpendicular rank use raw drawn position, not the step-adjusted
        // LOS endpoint. Native closest points quantize independently per fixed axis.
        float along = glm::dot(c->at - camera.eye, camera.forward);
        float offLine = 0;
        if (camera.fixedEye) {
            int64_t drawn[3], closest[3];
            std::memcpy(drawn, c->record + 0xb10, sizeof drawn);
            const auto &eye = *camera.fixedEye;
            const float dx = gamemath::fixedBlocksBetween(drawn[0], eye[0]);
            const float dy = gamemath::fixedBlocksBetween(drawn[1], eye[1]);
            const float dz = gamemath::fixedBlocksBetween(drawn[2], eye[2]);
            along = (dy * camera.forward.y + dx * camera.forward.x) + dz * camera.forward.z;
            if (!(along >= 0.0f))
                continue;
            for (int axis = 0; axis < 3; ++axis) {
                const float blocks = camera.forward[axis] * along;
                closest[axis] = gamemath::addFixed(eye[axis], int64_t(blocks * 65536.0f));
            }
            offLine = std::sqrt(gamemath::fixedDistanceSquared(closest, drawn));
        } else {
            if (!(along >= 0.0f))
                continue;
            offLine = glm::length(c->at - (camera.eye + camera.forward * along));
        }
        const bool healing = abilityOf(s.player) == 0x86 || abilityOf(s.player) == 0x87;
        if (c->hostileToPlayer != healing && (t.aimDepth < 0.0f || along < t.aimDepth))
            t.aimDepth = along;
        if (bestOffLine < 0.0f || offLine < bestOffLine)
            bestOffLine = offLine, t.creature = idOf(c->record);
    }

    t.nearby = nearbyCreature;
    return t;
}

Targets chooseTargets(const Scene &s, const CreatureTargets &creatures) {
    Targets t;
    if (!s.player)
        return t;
    const Camera &camera = s.camera;
    t.creature = creatures.creature;
    t.overCreature = creatures.overCreature;
    int64_t nearbyCreature = creatures.nearby;

    const auto playerFixed = read<FixedPoint>(s.player, worldgen::creature::POSITION);

    // Static objects within 4 blocks (spec 2.3): one under the crosshair, seen from the
    // player, beats a creature; the nearest seen is kept for later.
    int nearbyObject = -1;
    float nearbyObject2 = -1.0f, bestCamera2 = -1.0f;
    for (size_t i = 0; i < s.objects.size(); i++) {
        const Object &o = s.objects[i];
        if (o.item[0] == 0 && !targetable(o.type))
            continue;
        const float d2 =
            o.fixedPosition ? gamemath::fixedDistanceSquared(playerFixed.data(), o.fixedPosition->data()) : distance2(s.playerAt, o.at);
        if (d2 > NEARBY2)
            continue;
        const glm::vec3 centre = o.at + glm::vec3(0, 0, o.size.z * 0.5f);
        auto fixedCentre = o.fixedPosition;
        if (fixedCentre) {
            const float halfHeight = o.size.z * .5f;
            (*fixedCentre)[2] = gamemath::addFixed((*fixedCentre)[2], int64_t(halfHeight * 65536.0f));
        }
        const float camera2 = camera.fixedEye && fixedCentre ? gamemath::fixedDistanceSquared(camera.fixedEye->data(), fixedCentre->data())
                                                             : distance2(camera.eye, centre);
        const auto visible = [&] {
            if (s.fixedSight && fixedCentre)
                return s.fixedSight(playerFixed, *fixedCentre, SIGHT_TO_OBJECT);
            return sight(s, s.playerAt, centre, SIGHT_TO_OBJECT);
        };
        if ((nearbyObject2 < 0.0f || d2 < nearbyObject2) && visible()) {
            nearbyObject = (int)i, nearbyObject2 = d2;
            nearbyCreature = 0;
        }
        if (bestCamera2 >= 0.0f && bestCamera2 <= camera2)
            continue;
        if (!visible())
            continue;
        const glm::vec3 size = o.quarterTurn ? glm::vec3(o.size.y, o.size.x, o.size.z) : o.size;
        if (camera.crosshairIn(boxCorners(o.at - glm::vec3(size.x * 0.5f, size.y * 0.5f, 0.0f), size))) {
            t.creature = 0;
            t.object = (int)i;
            bestCamera2 = camera2;
        }
    }

    // A dead player aims at nothing, and nothing nearby stands in (spec 2.5).
    const bool dead = !(read<float>(s.player, worldgen::creature::HEALTH) > 0.0f);
    if (dead)
        t.creature = 0, t.object = -1;

    // Ground items within 4 blocks (spec 2.4).
    const int64_t owner = read<int64_t>(s.player, OWNER);
    int nearbyItem = -1;
    float nearbyItem2 = -1.0f;
    for (size_t i = 0; i < s.groundItems.size(); i++) {
        const GroundItem &g = s.groundItems[i];
        if (!groundItemValid(g, owner))
            continue;
        float d2 =
            g.fixedPosition ? gamemath::fixedDistanceSquared(playerFixed.data(), g.fixedPosition->data()) : distance2(s.playerAt, g.at);
        if (d2 > NEARBY2)
            continue;
        if ((g.type == 0x0c || g.type == 0x0e) && !g.noAutoPickup) { // picked up by walking over it
            if (g.settleMs == 0.0f && !g.pending)
                t.walkedOver.push_back((int)i);
            continue;
        }
        float camera2 = camera.fixedEye && g.fixedPosition
                            ? gamemath::fixedDistanceSquared(camera.fixedEye->data(), g.fixedPosition->data())
                            : distance2(camera.eye, g.at);
        if (!mayCarryMore(g.type, g.subtype, g.carriedCount))
            d2 += CARRY_PENALTY2, camera2 += CARRY_PENALTY2;
        if (nearbyItem2 < 0.0f || d2 < nearbyItem2) {
            nearbyItem = (int)i, nearbyItem2 = d2;
            nearbyCreature = 0, nearbyObject = -1;
        }
        if (bestCamera2 >= 0.0f && camera2 >= bestCamera2)
            continue;
        const bool visible = s.fixedSight && camera.fixedEye && g.fixedPosition
                                 ? s.fixedSight(*camera.fixedEye, *g.fixedPosition, SIGHT_TO_ITEM)
                                 : sight(s, camera.eye, g.at, SIGHT_TO_ITEM);
        if (!visible || !g.hasModel)
            continue;
        if (camera.crosshairIn(g.box)) {
            t.groundItem = (int)i;
            t.creature = 0, t.object = -1;
            bestCamera2 = camera2;
        }
    }

    // Without a crosshair target, what is near stands in (spec 2.5).
    if (!dead && t.creature == 0 && t.object < 0) {
        if (const uint8_t *c = recordOf(s, nearbyCreature); c && hostilityOf(c) != 1 && hostilityOf(c) != 7)
            t.creature = nearbyCreature;
        if (nearbyObject >= 0)
            t.object = nearbyObject;
    }
    if (t.groundItem < 0)
        t.groundItem = nearbyItem;
    return t;
}

Targets chooseTargets(const Scene &scene) {
    return chooseTargets(scene, chooseCreatures(scene));
}

std::optional<std::array<int32_t, 2>> markedZone(const uint8_t *target) {
    if (!target)
        return std::nullopt;
    const int hostility = hostilityOf(target);
    if (hostility == 0)
        return std::nullopt;
    if (hostility == 3 && target[worldgen::creature::CLASS] != 0x9d && !(flagsOf(target) & MARKS_MISSION))
        return std::nullopt;
    const int32_t x = read<int32_t>(target, MISSION_ZONE), y = read<int32_t>(target, MISSION_ZONE + 4);
    if (x == NO_ZONE)
        return std::nullopt;
    return std::array<int32_t, 2>{x, y};
}

bool inputAdmitted(const uint8_t *player, bool focusedEditor, bool chatActive) {
    if (!player || focusedEditor || chatActive)
        return false;
    const int ability = abilityOf(player);
    if (ability == 0x9c || ability == 0x9d || (ability == 0x7d && read<float>(player, ABILITY_MS) < 2000))
        return false;
    const bool exhaustedGlider = ability == GLIDING && !(read<uint32_t>(player, MOVE_FLAGS) & 1) &&
                                 read<float>(player, 0x130) <= 0 && read<float>(player, DODGE_MS) <= 0 &&
                                 read<float>(player, STAMINA) < 0.001f;
    return !exhaustedGlider;
}

Prompt decide(const Scene &s, const Targets &t, KeyState &key) {
    Prompt p;
    p.pressed = s.useDown && !key.wasDown;
    key.wasDown = s.useDown;
    if (!s.player)
        return p;
    p.takesKey = promptRoutine(s, t, p);
    if (!p.takesKey)
        fallThrough(s, t, p);
    return p;
}

void applyToPlayer(const Prompt &p, const Scene &s, nativeai::Creature &player) {
    uint8_t *r = player.record;
    if (!p.pressed || !r)
        return;
    auto activate = [&] {
        nativeai::Interaction in = nativeinteract::fresh(nativeinteract::ACTIVATE);
        write<int64_t>(in.data(), nativeinteract::TARGET, p.creature);
        handOver(player, in);
    };
    auto useObject = [&](bool withItem) {
        if (p.object < 0 || p.object >= (int)s.objects.size())
            return;
        const Object &o = s.objects[(size_t)p.object];
        nativeai::Interaction in = nativeinteract::fresh(nativeinteract::USE_OBJECT);
        write<int32_t>(in.data(), nativeinteract::ZONE_X, o.zoneX);
        write<int32_t>(in.data(), nativeinteract::ZONE_Y, o.zoneY);
        write<int32_t>(in.data(), nativeinteract::OBJECT, o.index);
        if (withItem)
            std::memcpy(in.data() + nativeinteract::ITEM, o.item.data(), o.item.size());
        handOver(player, in);
    };
    switch (p.action) {
    case Action::LetGo: r[worldgen::creature::ABILITY] = 0; break; // (its time runs on)
    case Action::Climb: r[worldgen::creature::ABILITY] = CLIMBING; break;
    case Action::Revive: write<float>(r, REVIVED, -0.5f); break;
    case Action::Throw: startAbility(r, THROWING); break;
    case Action::PickUp:
        write<int64_t>(r, CARRIED, p.creature);
        startAbility(r, CARRYING);
        activate();
        break;
    case Action::Pet:
        startAbility(r, PETTING);
        activate();
        break;
    case Action::Activate: activate(); break;
    case Action::GiveItem: useObject(true); break;
    case Action::UseObject: useObject(false); break;
    case Action::Vehicle: {
        const int type = s.vehicle[0], subtype = read<int32_t>(s.vehicle.data(), 4);
        const int ability = abilityOf(r);
        const glm::vec3 intent{read<float>(r, INTENT), read<float>(r, INTENT + 4), read<float>(r, INTENT + 8)};
        const bool putAway = (ability == GLIDING && subtype == 1) || (ability == SAILING && subtype == 2) ||
                             (ability == RIDING && subtype == 3 && glm::dot(intent, intent) < 0.001f) ||
                             (ability == CLIMBING && type == 0);
        if (putAway)
            r[worldgen::creature::ABILITY] = 0;
        else if (type != 0) {
            std::memcpy(r + HELD_ITEM, s.vehicle.data(), s.vehicle.size());
            startAbility(r, subtype == 1 ? GLIDING : subtype == 2 ? SAILING : OTHER_VEHICLE);
        }
        break;
    }
    default: break;
    }
}

void holdUp(const Scene &s, const Item &item, int ability) {
    if (!s.player || (abilityOf(s.player) != 0 && !abilityOver(s)))
        return;
    startAbility(s.player, ability);
    std::memcpy(s.player + HELD_ITEM, item.data(), item.size());
}

} // namespace useprompt
