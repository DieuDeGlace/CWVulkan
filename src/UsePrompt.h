#pragma once
// The use key (E, "use/interact"): what it would do now, shown on its quick button, and what
// it does on the frame it goes down. Behaviour from docs/specs/use-prompt.md ("spec 2.2" and
// the like below name its sections); the game's routines are cited by RVA only as references.
//
// In short, once a frame:
//   1. Targets are chosen (spec 2): the creature under the crosshair nearest the middle of the
//      view; a static object or a ground item under the crosshair within 4 blocks, nearer the
//      camera; failing those, a friendly creature, an object or an item within 4 blocks.
//   2. The prompt routine (spec 3) goes through the player's own states (let go of a wall,
//      climb, revive, throw), then the targeted creature, then the targeted object. The first
//      that applies sets the button's text and takes the key.
//   3. If none took the key (spec 4): the shown Ring Menu, the targeted ground item, or the
//      equipped vehicle item.
//
// The module decides; it changes only the player's own record and pending interactions
// (applyToPlayer). Everything else a press does (talking, the shop, the crafting window,
// chests, lore) is the caller's, by the Prompt's action.
//
// Positions are CubeVulkan's local blocks (ExtractedWorld::toLocal). Creature records are the
// game's layout (nativepose::CREATURE_SIZE bytes).
#include "NativeAI.h"

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace useprompt {

using FixedPoint = std::array<int64_t, 3>;
using Item = std::array<uint8_t, 0xa0>; // a game item: type +0, subtype (i32) +4, rarity +0x14

// The view the crosshair is in: the camera's place and its unit axes, and the tangents of half
// its field of view across and up.
struct Camera {
    glm::vec3 eye{0};
    glm::vec3 forward{1, 0, 0}, right{0, -1, 0}, up{0, 0, 1};
    float tanHalfWidth = 1, tanHalfHeight = 1;
    glm::vec3 rayOrigin{0};                         // retained look-at position, distinct from eye
    std::optional<std::array<int64_t, 3>> fixedEye; // native fixed eye when supplied by the camera
    std::optional<std::array<int64_t, 3>> fixedRayOrigin;

    // Whether a sphere is at least partly in view (the renderer's visibility test, 0xe08c0).
    bool sees(glm::vec3 centre, float radius) const;
    // Whether the crosshair, the middle of the screen, falls inside a box (spec 2.2): of the
    // corners in front of the camera, one is left of the middle, one right, one above and one
    // below.
    bool crosshairIn(const std::array<glm::vec3, 8> &corners) const;
};

// The corners of a box from `low` to `low + size`.
std::array<glm::vec3, 8> boxCorners(glm::vec3 low, glm::vec3 size);

// Line of sight through the world's blocks (0x2b5d90): clear when the points are closer than
// 0.01 blocks; blocked when they are farther apart than `limit`; otherwise clear unless a
// solid block is entered before the whole distance less 0.01. The block the ray starts in is
// not tested.
bool clearLine(glm::vec3 from, glm::vec3 to, float limit, const std::function<bool(int x, int y, int z)> &solid);

// A creature the crosshair might fall on.
struct Creature {
    const uint8_t *record = nullptr;
    glm::vec3 at{0};              // its drawn position
    bool hostileToPlayer = false; // the damage permission check (0x50550) between the player and it
};

// A static object of a zone near the player (zone +0x60, spec 0.3).
struct Object {
    int32_t zoneX = 0, zoneY = 0, index = 0; // where the zone keeps it
    int type = 0;
    glm::vec3 at{0};       // its position: the middle of its foot
    glm::vec3 size{1};     // its box (the type's size times its scale), before a quarter turn
    bool quarterTurn = false; // flag bit 0: x and y sizes swapped
    bool active = true;    // a door: shut; an object that takes an item: not yet given it
    int64_t occupant = 0;  // who sits or sleeps on it
    Item item{};           // the item it takes (type 0: none)
    int itemCount = 1;     // how many of it
    std::string itemName;  // that item's name (0x1628b0), for the prompt
    int carried = 0;       // how many of that item the player carries (0x5e780)
    // A chest (type 0x16): whether its 16-block cell is a known place already (opened), and
    // whether the zone holds an item for it.
    bool chestCellKnown = false, chestHasItem = false;
    std::optional<FixedPoint> fixedPosition; // authoritative unadjusted origin, when available
};

// An item lying on the ground (zone +0x30, spec 0.4).
struct GroundItem {
    int type = 0, subtype = 0;
    glm::vec3 at{0};
    bool noAutoPickup = false; // flag bit 1
    float settleMs = 0;        // picked up by walking over it only once this is 0
    bool pending = false;      // the player has already asked to pick it up
    int64_t reservedFor = 0;   // 0: anyone
    // What its validity depends on (0x2b8140): for type 0x17, whether the player carries one
    // already; for 0x1e, whether its 16-block cell is a known place; for 0x18, whether the
    // player owns that special item.
    bool carriesOne = false, cellKnown = false, ownsSpecial = false;
    int carriedCount = 0;      // how many equal to it the player carries (for mayCarryMore)
    bool bagHasRoom = true;    // the bags have room for one (0x8a370)
    int bag = 0;               // the bag it goes in (0x5f100): 0 equipment, 2 items, 3 ingredients, 4 pets
    // Its box as it is drawn (corners in local blocks); none without a model.
    bool hasModel = false;
    std::array<glm::vec3, 8> box{};
    int32_t quantity = 1;                    // Native validity requires nonzero, including negative values.
    std::optional<FixedPoint> fixedPosition; // raw ground tuple, independent of the drawn model
};

// Whether the player may pick up one more of an item when it carries `count` equal to it
// (0x50670, spec 4.2): no for types 0x15 and 0x18 once one is carried, 1:0x0c and 1:0x0e past
// 4, 0x0b:0x27..0x29 past 9, and other items of types 1, 0x0b, 0x15 and 0x18 past 49.
bool mayCarryMore(int type, int subtype, int count);

// Whether a ground item may be targeted at all (0x2b8140, spec 2.4).
bool groundItemValid(const GroundItem &g, int64_t playerOwnerId);

// Everything the use key looks at this frame.
struct Scene {
    uint8_t *player = nullptr;   // the local player's record
    glm::vec3 playerAt{0};       // its position
    float abilityDuration = 0;   // its current ability's whole time (0x4f220)
    Camera camera;
    std::vector<Creature> creatures;     // every creature of the world (spec 2.1 picks among them)
    std::vector<Object> objects;         // the static objects of the zones around the player
    std::vector<GroundItem> groundItems; // the ground items of those zones
    bool useDown = false;        // the use key is held
    bool dodgeHeld = false, walkHeld = false;
    bool dialogOpen = false;     // the dialog window (lore tales, chests, examining)
    bool selectorShown = false;
    bool ownsGoddessGift = true; // the player owns special item 4 (0x5e890 always says so)
    Item vehicle{};              // the equipped vehicle or special item (type 0: none)
    std::string vehicleName;     // its short name
    std::function<bool(glm::vec3 from, glm::vec3 to, float limit)> sight; // clearLine through the world
    // Query the completed renderer epoch, distinct from the stored aim camera.
    std::function<bool(const FixedPoint &, float radius, float range)> sphereVisible;
    // Prefer exact fixed endpoints for target LOS when native coordinates are retained.
    std::function<bool(const std::array<int64_t, 3> &, const std::array<int64_t, 3> &, float)> fixedSight;
    std::function<const uint8_t *(int64_t id)> creatureById; // null when it is gone
};

// What the use key aims at (spec 0.5). Objects and ground items are indices into the
// Scene's lists, -1 for none.
struct Targets {
    int64_t creature = 0;
    int object = -1;
    int groundItem = -1;
    bool overCreature = false;   // the crosshair is over some creature's box
    // Ground items picked up by walking over them (spec 2.4 step 1).
    std::vector<int> walkedOver;
};

// Creature selection precedes movement/ultimate/assistance. Only value IDs/depth
// survive into the later object/item/fallback pass; do not rerun selection there.
struct CreatureTargets {
    int64_t creature = 0, nearby = 0;
    bool overCreature = false;
    float aimDepth = -1;
    int64_t chargeTarget = 0; // independent per-pass choice retained until late strong attack
};
CreatureTargets chooseCreatures(const Scene &scene);
Targets chooseTargets(const Scene &scene, const CreatureTargets &creatures);
// Compatibility selection for callers without interleaved input stages.
Targets chooseTargets(const Scene &scene);

// The mission zone a targeted creature marks on the map (spec 2.6): its zone key (+0x1b0),
// when it has one and is not a plain townsperson. The caller marks it unless it is known.
std::optional<std::array<int32_t, 2>> markedZone(const uint8_t *target);

// What a press does.
enum class Action {
    None,
    LetGo,          // ability 0x7a (climbing) ends
    Climb,          // ability 0x7a starts
    Revive,
    Throw,          // the carried creature is thrown (ability 0x81)
    PickUp,         // a creature is taken up to carry (ability 0x80), and activated
    ExaminePetrified, // dialog: define_model race, he_is_petrified
    Shop,
    ClassWindow,    // the window of townsfolk of class 0x83 (not identified)
    FlightMaster,   // the map, in flight mode
    Converse,       // the creature's conversation, or speeding up its speech bubble
    Pet,            // ability 0xa2, and activated
    Activate,       // an interaction of kind 3 for the creature
    Shrine,         // the map, in shrine-choice mode
    Crafting,       // the crafting window at a station (`station`)
    GiveItem,       // the object's item is given up (kind 5 with the item)
    UseObject,      // an interaction of kind 5 for the object (doors, seats, beds)
    OpenChest,
    Customize,      // the customization window
    ExamineLore,    // the lore tale of the object's cell
    GoddessGift,    // dialog: gift_of_the_goddess (unreachable in the game)
    UseSelectedItem, // the Ring Menu's selected category1 item
    PickUpItem,     // an interaction of kind 7 for the ground item
    CannotCarry,    // "You can't carry more of these items."
    BagFull,        // "Your ... bag is full." (by `bag`)
    Vehicle,        // the vehicle item is used or put away
};

// The quick button and what a press does.
struct Prompt {
    std::string text;            // the button's "info" text, "" for none
    glm::vec4 colour{1};         // its colour
    enum class Icon { None, ObjectItem, GroundItem, Vehicle } icon = Icon::None;
    bool takesKey = false;       // the prompt routine took the key (spec 3 returned 1)
    bool pressed = false;        // the key went down this frame
    Action action = Action::None; // what pressing does (also set when not pressed)
    int64_t creature = 0;        // the creature it is about
    int64_t highlighted = 0;     // the creature a creature prompt highlights
    int object = -1;             // the object it is about (Scene::objects)
    int groundItem = -1;         // the ground item it is about (Scene::groundItems)
    int station = 0;             // the crafting station's mode (0 anvil and workbench, 1 loom, 3 campfire, 5 furnace, spinning wheel and saw)
    int bag = 0;                 // the bag that is full
};

// The key's state between frames (the routine keeps its own last state).
struct KeyState {
    bool wasDown = false;
};
// Ordinary-use ability/editor gates (steam-ring-selector-rendering7.2). World readiness
// and hidden frontend roots remain caller admission; rejected visits retain key histories.
bool inputAdmitted(const uint8_t *player, bool focusedEditor, bool chatActive);

// The prompt routine (0x98180, spec 3) and, when it takes nothing, the fall-through
// (0x8ae50, spec 4).
Prompt decide(const Scene &scene, const Targets &targets, KeyState &key);

// What a press does to the player itself: its ability, the creature it carries, its held
// item, and the interactions it hands the world (kinds 3 and 5). Nothing unless pressed.
void applyToPlayer(const Prompt &prompt, const Scene &scene, nativeai::Creature &player);

// A chest's item held up (spec 3.7.3 step 4): ability 0x7c with the item, if the player's
// ability is 0 or over. A received vehicle is held up with its own ability (loot-pickup.md §4).
constexpr int HOLD_UP_ABILITY = 0x7c;
void holdUp(const Scene &scene, const Item &item, int ability = HOLD_UP_ABILITY);

} // namespace useprompt
