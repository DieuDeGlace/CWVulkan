#pragma once
// The rules of the map's markers, as Steam Cube World draws them (docs/specs/map-markers.md;
// "spec 1.3.2" and the like below name its sections):
//   - the overlay's flat markers (the map overlay's draw): places, missions, lands, flight
//     points and stars, each an icon with a label and maybe a blinking "NEW";
//   - the world map's 3D markers: the creatures leading the character to a mission and the
//     tombstone where the character last died.
// Everything here is plain data and arithmetic; main.cpp gathers the facts, Hud draws the
// overlay's markers and WorldMap the 3D ones.
#include "worldgen/ZoneRecords.h" // worldgen::Item

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace mapmarks {

using Cell = std::array<int32_t, 2>;

// --- Sizes (spec 1.3) ---------------------------------------------------------------------------

// Markers are sized by the map's zoom: `unit` is blocks per map unit (4 << level) and
// `mapScale` the map's eased scale option.
constexpr float HIDE_BELOW = 0.3f;        // missions, flight points and stars smaller than this are not shown
constexpr float SMALLEST = 0.66f;         // ... and shown at least this big
constexpr float SMALLEST_PLACE = 0.35f;   // places are never hidden, and at least this big
struct Size {
    bool shown = true;
    float scale = 1;
};
// A place's icon (spec 1.3.1): 4 / unit, eight times that for a shrine and four times for a
// lore building.
float placeScale(int kind, int unit, float mapScale);
// The others: numerator / unit / 2.5, between 0.66 and 1.
Size missionSize(int unit, float mapScale); // 128: hidden below 0.3
Size landSize(int unit, float mapScale);    // 512: never hidden
Size pointSize(int unit, float mapScale);   // 64: flight points and stars, hidden below 0.3
Size playerSize(int unit, float mapScale);  // 64: other players' names, never hidden
// Whether a mission's name is shown at this zoom (with its place discovered): levels 0..4.
inline bool namesShown(int unit) { return unit < 65; }

// --- Icons and colours (spec 1.3) -----------------------------------------------------------------

// The interface sprites (gui.plx) of a place's kind and of a mission's kind (none for 0).
const char *placeIcon(int kind);
const char *missionIcon(int kind);
// A known mission's label colour by its level (-1..4, clamped), and a land name's by its kind.
glm::vec4 levelColour(int level);
glm::vec4 landColour(int kind);

// --- Mission markers (spec 1.3.2) -----------------------------------------------------------------

// What a mission marker shows, from what the character knows of it.
struct MissionFacts {
    int kind = 0, level = 0;  // the planner's role and level for the place
    int unit = 4;
    float mapScale = 1;
    bool discovered = false;  // the place is a discovered zone
    bool known = false;       // marked or solved (after the lore reveal)
    bool completed = false;   // finished or solved
    bool newNote = false;     // the place has a "NEW" timer
    int clockMs = 0;          // the interface's clock
    bool showRarity = true;   // options.cfg showRarity
};
struct MissionLook {
    bool shown = false;
    float scale = 1;
    glm::vec4 colour{1};
    bool named = false;       // the label is the place's name ...
    int stars = 0;            // ... and this many stars under it (0: none)
    const char *icon = nullptr;
    bool solved = false;      // the "solved" mark over the icon
    bool note = false;        // "NEW" is on (it blinks)
    bool reportsHover = false;
};
MissionLook missionLook(const MissionFacts &facts);
// Whether an unknown artifact place (kind 14) is revealed by its realm's lore (spec 5.4):
// ten lore pieces or more.
bool revealedByLore(int kind, bool known, size_t realmLorePieces);
// The "NEW" note blinks: on for 600 ms out of every 1200.
inline bool noteShown(int clockMs) { return (clockMs / 600) % 2 != 0; }

// --- The "NEW" timers (spec 1.6) -------------------------------------------------------------------

using NoteTimers = std::map<Cell, float>; // place -> ms left
constexpr float TALK_NOTE_MS = 180000.0f; // a place marked in a talk: three minutes
constexpr float LORE_NOTE_MS = 18000.0f;  // a place revealed by lore: eighteen seconds
// Each world update: every timer runs down by the elapsed time; those at zero are dropped.
void tickNotes(NoteTimers &timers, float elapsedMs);

// --- The tombstone (spec 2.3) -------------------------------------------------------------------------

constexpr float TOMBSTONE_MS = 300000.0f;      // shown for five minutes after a revival ...
constexpr double TOMBSTONE_CLEAR_DISTANCE2 = 400.0; // ... or until the player is within 20 blocks
// Whether the tombstone's timer runs while the player's ability is `ability` (not while
// flying to a flight point or the like: 0x98, 0x9c, 0x9d).
bool tombstoneCounts(int ability);
// The timer after one world update, given the squared distance in blocks to the tombstone.
float tombstoneLeft(float timerMs, float elapsedMs, double distance2);

// --- The world map's creature markers (spec 2.2, 2.5) ------------------------------------------------

// Whether a creature gets a marker: a player, or one leading the character to a mission
// (toggle 0x100) whose mission is not completed; never one in ability 0x9d.
bool showsCreature(int hostility, int ability, uint16_t toggles, bool destinationCompleted);
// The markers' size against the map view's scale: 0.4 / (0.8 * viewScale).
float creatureMarkerScale(float viewScale);
// The outline's glow, 0..1: the tombstone and non-players pulse about once a second,
// players do not.
float markerPulse(double worldClockMs, bool pulses);
// A marker beyond the shown square (`reach` blocks each way of the centre) is pulled onto its
// edge along the line to the centre: first by x, then by y.
glm::dvec3 pullToSquare(glm::dvec3 offset, double reach);

// --- The overlay's markers (spec 1.1, 1.7, 1.8) --------------------------------------------------------

struct Marker {
    std::string label, note;
    float scale = 1;
    glm::vec4 colour{1};
    glm::vec2 at{0};          // window pixels
    float depth = 0;
    glm::ivec3 block{0};      // where it stands, in blocks (the hover's identity)
    std::string icon;         // a gui.plx sprite, or none
    std::string overlay;      // a sprite over the icon ("solved"), or none
    bool shrineTarget = false; // a shrine to choose (flag A)
    bool flightTarget = false; // a flight to choose (flag B)
    bool reportsHover = false; // hovering it reports it (flag C)
    bool reportsCell = false;  // ... and its 16-block cell, when it carries no item (flag D)
    bool flightPoint = false;  // a flight point (flag E)
    worldgen::Item item;   // the item it shows when hovered (an artifact), category 0 for none
    bool enlarged = false;    // its icon drawn at 1.25: a hover target
    // CubeVulkan's own: a quest sprite from OverworldEvents' atlas drawn instead, its bottom
    // centre on the marker. Not a map marker of the game's; never hovered.
    std::string questSprite;
};

constexpr float HOVERED_ICON_SCALE = 1.25f;
constexpr float HOVER_HALF_SIZE = 25.0f; // the hit box's half height a unit of scale
// The markers' icons rise 15 interface units above the marker; the label sits 10 units lower
// when there is an icon, and the note 18 above the label. Icons bob 5 units at 0.005 a ms.
constexpr float ICON_RISE = 15.0f, LABEL_DROP = 10.0f, NOTE_RISE = 18.0f;
float bob(int clockMs);

// What the pointer chose (spec 1.7). Every field is the last one written by a hit nearer than
// the ones before it, so a later hit only overwrites the fields its own flags write.
struct Hover {
    int best = -1; // the nearest marker hit
    std::optional<glm::ivec2> flight, shrine, chosen; // flight target, shrine target, hover-chosen spot
    bool flightToPlayer = false; // the flight target is not a flight point but a player
    std::optional<Cell> cell;  // a lore building's cell
    worldgen::Item item;    // an item to show
    glm::ivec3 cursor{0};      // the map's cursor, in blocks
};
// The hover pass over markers sorted far to near: `uiScale` is the interface's scale and
// `labelWidth` measures a label at a marker's scale (in pixels).
Hover hover(const std::vector<Marker> &markers, glm::vec2 pointer, float uiScale,
            const std::function<float(const Marker &)> &labelWidth);
// Marks the markers standing on a hover target as drawn enlarged.
void enlargeTargets(std::vector<Marker> &markers, const Hover &hovered);

} // namespace mapmarks
