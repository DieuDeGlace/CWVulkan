#include "MapMarks.h"

#include <algorithm>
#include <cmath>

namespace mapmarks {

namespace {

// Size from a numerator: numerator / unit * mapScale / 2.5.
Size sized(float numerator, int unit, float mapScale, bool hides) {
    const float s = numerator / (float)unit * mapScale / 2.5f;
    Size size;
    size.shown = !hides || !(s < HIDE_BELOW);
    size.scale = std::clamp(s, SMALLEST, 1.0f);
    return size;
}

// The places' icons by kind (spec 1.3.1): the hamlets' inn and shops, shrines, lore buildings
// and treasure; "home" for any other.
struct KindIcon {
    int kind;
    const char *icon;
};
const KindIcon PLACE_ICONS[] = {
    {2, "inn"},          {3, "item-shop"},       {4, "armor-shop"},   {5, "weapon-shop"},
    {6, "identifier"},   {7, "smithy"},          {8, "carpenters-shop"}, {9, "tailors-shop"},
    {10, "flight-master"}, {11, "guild-hall"},   {12, "shrine"},      {14, "npc"},
    {15, "treasure"},
};
// The missions' icons by kind (spec 1.3.2); "enemy" for any other.
const KindIcon MISSION_ICONS[] = {
    {1, "gnome"},          {2, "crafting1"}, {3, "crafting2"}, {4, "crafting3"},  {5, "crafting4"},
    {6, "hangglider"},     {7, "boat"},      {8, "climbingspikes"}, {9, "reins"}, {10, "harp"},
    {11, "whistle"},       {12, "bell"},     {13, "spirit"},   {14, "destination"}, {16, "monster"},
    {17, "monster"},       {18, "flower"},   {19, "witch"},    {20, "demonportal"}, {21, "crystal"},
    {22, "brazier"},       {23, "manapump"}, {24, "key"},
};
template <size_t N> const char *iconOf(const KindIcon (&table)[N], int kind, const char *otherwise) {
    for (const KindIcon &k : table)
        if (k.kind == kind)
            return k.icon;
    return otherwise;
}

constexpr int SHRINE = 12, LORE_BUILDING = 14, ARTIFACT_PLACE = 14;

} // namespace

// --- Sizes -------------------------------------------------------------------------------------

float placeScale(int kind, int unit, float mapScale) {
    float s = 4.0f / (float)unit * mapScale;
    if (kind == SHRINE)
        s *= 8.0f;
    else if (kind == LORE_BUILDING)
        s *= 4.0f;
    return std::clamp(s, SMALLEST_PLACE, 1.0f);
}

Size missionSize(int unit, float mapScale) { return sized(128.0f, unit, mapScale, true); }
Size landSize(int unit, float mapScale) { return sized(512.0f, unit, mapScale, false); }
Size pointSize(int unit, float mapScale) { return sized(64.0f, unit, mapScale, true); }
Size playerSize(int unit, float mapScale) { return sized(64.0f, unit, mapScale, false); }

// --- Icons and colours ----------------------------------------------------------------------------

const char *placeIcon(int kind) { return iconOf(PLACE_ICONS, kind, "home"); }

const char *missionIcon(int kind) { return kind == 0 ? nullptr : iconOf(MISSION_ICONS, kind, "enemy"); }

glm::vec4 levelColour(int level) {
    static const glm::vec4 COLOURS[6] = {
        {0.7f, 0.7f, 0.7f, 1}, // -1
        {1, 1, 1, 1},          // 0
        {0, 1, 0, 1},          // 1
        {0.3f, 0.45f, 1, 1},   // 2
        {0.55f, 0.2f, 1, 1},   // 3
        {1, 1, 0, 1},          // 4
    };
    return COLOURS[std::clamp(level, -1, 4) + 1];
}

glm::vec4 landColour(int kind) {
    static const glm::vec4 COLOURS[14] = {
        {0.5f, 1, 0.25f, 1},  {0.4f, 0.65f, 1, 1}, {0.5f, 1, 0.25f, 1}, {0.5f, 0.9f, 0.1f, 1}, {0.2f, 0.8f, 0.2f, 1},
        {0.4f, 0.3f, 0.9f, 1}, {0.6f, 0.7f, 1, 1},  {0.7f, 0.7f, 0.7f, 1}, {0.3f, 0.7f, 1, 1},  {1, 0.9f, 0.2f, 1},
        {0.9f, 1, 0.2f, 1},   {0.7f, 0.9f, 1, 1},  {0.5f, 0.1f, 1, 1},   {0.7f, 0.2f, 0.2f, 1},
    };
    return kind >= 0 && kind < 14 ? COLOURS[kind] : glm::vec4(1);
}

// --- Mission markers --------------------------------------------------------------------------------

MissionLook missionLook(const MissionFacts &f) {
    MissionLook look;
    const bool named = namesShown(f.unit) && f.discovered;
    if (!named && !f.known)
        return look;
    const Size size = missionSize(f.unit, f.mapScale);
    if (!size.shown)
        return look;
    look.shown = true;
    look.scale = size.scale;
    look.colour = f.known ? levelColour(f.level) : glm::vec4(1);
    look.named = named;
    if (named && f.known && f.showRarity)
        look.stars = f.level + 1;
    look.note = f.newNote && noteShown(f.clockMs) && !f.completed;
    if (f.kind != 0 && f.known) {
        look.icon = missionIcon(f.kind);
        look.reportsHover = f.kind == ARTIFACT_PLACE;
    }
    look.solved = f.kind != 0 && f.completed;
    return look;
}

bool revealedByLore(int kind, bool known, size_t realmLorePieces) {
    return kind == ARTIFACT_PLACE && !known && realmLorePieces >= 10;
}

// --- The "NEW" timers -------------------------------------------------------------------------------

void tickNotes(NoteTimers &timers, float elapsedMs) {
    for (auto it = timers.begin(); it != timers.end();) {
        it->second -= elapsedMs;
        it = it->second > 0.0f ? std::next(it) : timers.erase(it);
    }
}

// --- The tombstone ----------------------------------------------------------------------------------

bool tombstoneCounts(int ability) { return ability != 0x98 && ability != 0x9c && ability != 0x9d; }

float tombstoneLeft(float timerMs, float elapsedMs, double distance2) {
    if (distance2 < TOMBSTONE_CLEAR_DISTANCE2)
        return 0.0f;
    return std::max(0.0f, timerMs - elapsedMs);
}

// --- The world map's creature markers -------------------------------------------------------------

bool showsCreature(int hostility, int ability, uint16_t toggles, bool destinationCompleted) {
    constexpr int PLAYER = 0, GONE = 0x9d;
    constexpr uint16_t LEADS_TO_MISSION = 0x100;
    if (ability == GONE)
        return false;
    return hostility == PLAYER || ((toggles & LEADS_TO_MISSION) && !destinationCompleted);
}

float creatureMarkerScale(float viewScale) {
    const float k = viewScale * 0.6f * 80.0f / 120.0f * 16.0f / 8.0f; // 0.8 * viewScale
    return 0.4f / k;
}

float markerPulse(double worldClockMs, bool pulses) {
    return pulses ? (float)((std::cos(worldClockMs * 0.006) + 1.0) * 0.5) : 0.0f;
}

glm::dvec3 pullToSquare(glm::dvec3 offset, double reach) {
    for (int axis = 0; axis < 2; axis++) {
        const double distance = std::fabs((double)(int64_t)offset[axis]); // whole blocks
        if (distance >= reach)
            offset *= reach / distance;
    }
    return offset;
}

// --- The overlay's markers --------------------------------------------------------------------------

float bob(int clockMs) { return std::cos((float)clockMs * 0.005f) * 5.0f; }

Hover hover(const std::vector<Marker> &markers, glm::vec2 pointer, float uiScale,
            const std::function<float(const Marker &)> &labelWidth) {
    Hover h;
    float nearest = -1.0f;
    for (size_t i = 0; i < markers.size(); i++) {
        const Marker &k = markers[i];
        if (!k.questSprite.empty())
            continue;
        const glm::vec2 d = pointer - k.at;
        const float distance2 = d.x * d.x + d.y * d.y;
        const float halfHeight = HOVER_HALF_SIZE * uiScale * k.scale;
        float halfWidth = halfHeight;
        if (!k.label.empty() && labelWidth)
            halfWidth = std::max(halfWidth, labelWidth(k) * 0.5f);
        if (!(std::abs(d.x) < halfWidth && std::abs(d.y) < halfHeight) || (nearest >= 0.0f && !(distance2 < nearest)))
            continue;
        nearest = distance2;
        h.best = (int)i;
        const glm::ivec2 spot(k.block.x, k.block.y);
        if (k.shrineTarget)
            h.shrine = spot;
        if (k.flightTarget)
            h.flight = spot, h.flightToPlayer = !k.flightPoint;
        if (k.reportsHover) {
            if (k.item.category != 0)
                h.item = k.item;
            else if (k.reportsCell)
                h.cell = Cell{k.block.x >> 4, k.block.y >> 4}; // (an arithmetic shift floors)
            h.chosen = spot;
        }
        h.cursor = k.block;
    }
    return h;
}

void enlargeTargets(std::vector<Marker> &markers, const Hover &hovered) {
    for (Marker &k : markers) {
        const glm::ivec2 spot(k.block.x, k.block.y);
        k.enlarged = (hovered.flight && *hovered.flight == spot) || (hovered.shrine && *hovered.shrine == spot) ||
                     (hovered.chosen && *hovered.chosen == spot);
    }
}

} // namespace mapmarks
