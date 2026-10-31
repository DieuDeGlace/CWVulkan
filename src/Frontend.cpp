// The front screens' rules and layout, written from docs/specs/frontend-title.md,
// frontend-characters.md and frontend-options.md. Running them is FrontendRun.cpp.
#include "Frontend.h"

#include "GameInput.h"
#include "InventorySlots.h"
#include "NativeInteract.h"
#include "GameMath.h"
#include "TalkGen.h"
#include "worldgen/Spawns.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace frontend {

namespace {
constexpr double DEGREES = 0.0174533; // the game's degrees-to-radians factor
}

// --- The camera ------------------------------------------------------------------------------

void OrbitCamera::set(double p, double r, double y, float d) {
    pitch = targetPitch = p;
    roll = targetRoll = r;
    yaw = targetYaw = y;
    distance = targetDistance = d;
}

void OrbitCamera::titleTargets(double dtMs) {
    constexpr double TURN_PER_MS = 0.003; // 3 degrees a second
    constexpr double TITLE_PITCH = 110;
    constexpr float TITLE_DISTANCE = 12;
    targetYaw += dtMs * TURN_PER_MS;
    targetPitch = TITLE_PITCH;
    targetDistance = TITLE_DISTANCE;
}

void OrbitCamera::smooth(double dtMs, int cameraSmoothness) {
    const float rate = (float)std::pow(0.005, cameraSmoothness * 0.01);
    const double s = gamemath::blendFactor(dtMs, rate);
    pitch += (targetPitch - pitch) * s;
    roll += (targetRoll - roll) * s;
    yaw += (targetYaw - yaw) * s;
    distance += (targetDistance - distance) * (float)s;
}

glm::dvec3 OrbitCamera::forward() const {
    const double p = pitch * DEGREES, r = roll * DEGREES, y = yaw * DEGREES;
    return {std::cos(y) * std::sin(r) * std::cos(p) + std::sin(y) * std::sin(p),
            std::sin(y) * std::sin(r) * std::cos(p) - std::cos(y) * std::sin(p), std::cos(p) * std::cos(r)};
}

glm::dvec3 OrbitCamera::eye(glm::dvec3 lookAt) const { return lookAt - (double)distance * forward(); }

// --- The logo --------------------------------------------------------------------------------

LogoPlacement logoPlacement(float width, float height, float guiScale, uint32_t wallClockMs) {
    constexpr uint32_t PERIOD_MS = 10000;
    const float phase = 6.2831853f * ((float)(wallClockMs % PERIOD_MS) * 0.0001f);
    LogoPlacement p;
    p.centre = {width * 0.5f, height * (0.25f + 0.05f * std::cos(phase))};
    p.tiltDegrees = 20.0f - 5.0f * std::cos(phase);
    p.scale = height > 0 ? guiScale * 0.011f * 1024.0f / height : 0.0f;
    return p;
}

glm::mat4 logoClip(const LogoPlacement &p, glm::vec3 size, float width, float height) {
    // The model is centred on its middle and turned about x by the tilt. The spec's further half
    // turn about y, seen through Direct3D's left-handed view (the viewer on the -z side), turns
    // the model's +z faces toward the viewer and its tops up toward it; the game's screen shows
    // the logo's +z side (the face with the bevel's yellow highlights and dark edges) reading
    // left to right, so its lens mirrors x as the map's does (worldmap-rendering.md §4.2). The
    // half turn and the mirror together are a flip of z, after the tilt.
    const glm::mat4 tilt = glm::rotate(glm::mat4(1), glm::radians(p.tiltDegrees), glm::vec3(1, 0, 0));
    const glm::mat4 centred = glm::translate(glm::mat4(1), -size * 0.5f);
    const glm::mat4 flip = glm::scale(glm::mat4(1), glm::vec3(1, 1, -1));
    // A perspective view, so the voxels' sides show away from the middle and as the model bobs.
    // How the overlay pass projects screen-space models is open (frontend-title.md, open
    // question 1): CubeVulkan takes the field of view of its world camera (60 degrees) and puts
    // the model's middle at the depth where a voxel there is `scale` clip units of the window's
    // height, so the logo keeps its size in pixels as the window grows (0.011 x 1024 / 2 pixels a
    // voxel), its middle at the centre.
    constexpr float FIELD_OF_VIEW_DEGREES = 60.0f;
    const float focal = 1.0f / std::tan(glm::radians(FIELD_OF_VIEW_DEGREES) * 0.5f);
    const float aspect = height > 0 ? width / height : 1.0f;
    const float depth = p.scale > 0 ? focal / p.scale : 1.0f; // voxels from the eye to the middle
    const float nearest = depth * 0.5f, farthest = depth * 1.5f;
    const float depthScale = farthest / (farthest - nearest);
    const glm::vec2 centre(p.centre.x / std::max(1.0f, width) * 2.0f - 1.0f, p.centre.y / std::max(1.0f, height) * 2.0f - 1.0f);
    // Clip space from (x right, y up, z away from the viewer, the middle at z 0): w is the
    // distance from the eye, depth + z.
    glm::mat4 project(0.0f);
    project[0][0] = focal / aspect;
    project[1][1] = -focal; // (the window's y runs down)
    project[2][0] = centre.x, project[2][1] = centre.y, project[2][2] = depthScale, project[2][3] = 1.0f;
    project[3][0] = centre.x * depth, project[3][1] = centre.y * depth;
    project[3][2] = depthScale * depth - depthScale * nearest;
    project[3][3] = depth;
    return project * flip * tilt * centred;
}

float screenModelShade(int axis, int sign, int facing) {
    constexpr float FACING = 1.0f, UP = 0.8f, SIDE = 0.55f, DOWN = 0.45f;
    if (axis == 2)
        return sign == facing ? FACING : SIDE; // (the faces away from the viewer are not seen)
    if (axis == 1)
        return sign > 0 ? UP : DOWN;
    return SIDE;
}

// --- Menus -----------------------------------------------------------------------------------

std::vector<MenuLine> rootLines(bool continueShown) {
    std::vector<MenuLine> lines = {{START_GAME, 30, 10, 40, 25}};
    if (continueShown)
        lines.push_back({CONTINUE, 80, 60, 90, 75});
    const float c = continueShown ? 130.0f : 80.0f;
    lines.push_back({OPTIONS, c, c - 20, c + 10, c - 5});
    lines.push_back({EXIT, c + 50, c + 30, c + 60, c + 45});
    return lines;
}

glm::vec2 rootMenuPosition(glm::vec2 window) {
    return {(window.x - ROOT_MENU_SIZE.x) * 0.5f, (1.6f * window.y - ROOT_MENU_SIZE.y) * 0.5f};
}

int hoveredLine(const std::vector<MenuLine> &lines, glm::vec2 mouse, float widgetWidth) {
    if (mouse.x < 0 || mouse.x >= widgetWidth)
        return -1;
    for (const MenuLine &l : lines)
        if (mouse.y >= l.bandTop && mouse.y < l.bandBottom)
            return l.index;
    return -1;
}

std::vector<MenuLine> systemLines() {
    return {{SYSTEM_OPTIONS, 30, 10, 40, 25},
            {SYSTEM_CONTROLS, 60, 40, 70, 55},
            {SYSTEM_START_MENU, 90, 70, 100, 85},
            {SYSTEM_EXIT, 120, 100, 130, 115}};
}

// --- Character select ------------------------------------------------------------------------

glm::vec2 entryPosition(int i) { return {300.0f, 50.0f + 230.0f * (float)i}; }

CharacterButtons characterButtons(glm::vec2 cardAt, glm::vec2 cardSize, glm::vec2 selectSize, glm::vec2 toggleSize) {
    const float right = cardSize.x + cardAt.x, bottom = cardSize.y + cardAt.y;
    return {
        {(right - selectSize.x) - 20, (bottom - selectSize.y) - 20}, {(right - toggleSize.x) - 20, cardAt.y + 20}, {right + 20, cardAt.y}};
}

void fitCharacterFrames(std::vector<CharacterFrame> &cards, glm::ivec2 window, float milliseconds, float scroll) {
    if (cards.empty())
        return;
    const int32_t total = (int32_t)((cards.back().at.y + cards.back().size.y) - cards.front().at.y);
    float upper = 10, lower = (float)(window.y - 10) - cards.back().size.y;
    if (total < window.y - 20) {
        upper = (float)(window.y / 2 - total / 2);
        lower = (float)(window.y / 2 + total / 2) - cards.back().size.y;
    }
    auto shift = [&](float delta) {
        for (auto &card : cards) {
            card.at.x = (float)(window.x / 2) - card.size.x * 0.5f;
            card.at.y += delta;
        }
    };
    if (cards.front().at.y > upper) {
        const float old = cards.front().at.y;
        const float next = (upper - old) * gamemath::blendFactor((double)milliseconds, 0.01f) + old;
        shift(next - cards.front().at.y);
    }
    if (cards.front().at.y < upper && cards.back().at.y < lower) {
        const float old = cards.back().at.y;
        const float next = (lower - old) * gamemath::blendFactor((double)milliseconds, 0.01f) + old;
        shift(next - cards.back().at.y);
    }
    shift((scroll * 0.001f) * milliseconds);
}

void CharacterScroll::poll(int32_t y, uint32_t clockMs, bool shown, bool held, int windowWidth, std::vector<CharacterFrame> &cards) {
    const uint32_t elapsedBits = clockMs - sampledAt;
    int32_t elapsed;
    std::memcpy(&elapsed, &elapsedBits, sizeof(elapsed));
    sampledAt = clockMs;
    const float previous = pointerY;
    pointerY = (float)y;
    const float old = filteredY;
    filteredY = (pointerY - old) * gamemath::blendFactor((double)elapsed, 0.01f) + old;
    if (!shown || !held)
        return;
    for (auto &card : cards) {
        card.at.x = (float)(windowWidth / 2) - card.size.x * 0.5f;
        card.at.y += filteredY - old;
    }
    const float impulse = (pointerY - previous) * (float)elapsed;
    velocity = (impulse + impulse) + velocity;
}

void CharacterScroll::frame(float milliseconds, bool shown, bool held, glm::ivec2 window, std::vector<CharacterFrame> &cards) {
    if (!shown) {
        velocity = 0;
        return;
    }
    if (!held)
        fitCharacterFrames(cards, window, milliseconds, velocity);
    // Decay follows displacement, including frames held or with no cards.
    velocity = (0.0f - velocity) * gamemath::blendFactor((double)milliseconds, 0.005f) + velocity;
}

float listHeight(int entries) { return entries > 0 ? 230.0f * (float)entries - 10.0f : 0.0f; }

float centredListTop(int entries, float windowHeight) { return windowHeight * 0.5f - listHeight(entries) * 0.5f; }

const char *classWord(int cls) {
    static const char *const WORDS[5] = {"", "Warrior", "Ranger", "Mage", "Rogue"};
    return cls >= 1 && cls <= 4 ? WORDS[cls] : "";
}

const char *specializationWord(int cls, int spec) {
    static const char *const WORDS[5][2] = {{"", ""},
                                            {"Berserker", "Guardian"},
                                            {"Sniper", "Scout"},
                                            {"Fire Mage", "Water Mage"},
                                            {"Assassin", "Ninja"}};
    return cls >= 1 && cls <= 4 && (spec == 0 || spec == 1) ? WORDS[cls][spec] : "";
}

std::string entryLine(int level, int32_t race, int cls) {
    return "LV " + std::to_string(level) + " " + nativeprogress::raceWord(race) + " " + classWord(cls);
}

// --- Character creation ----------------------------------------------------------------------

glm::vec3 hairCellColour(int column, int row) {
    static const glm::vec3 BASE[HAIR_COLUMNS] = {
        {1, 0, 0},    {1, 0.25f, 0}, {1, 0.5f, 0}, {1, 1, 0},     {0.5f, 1, 0},  {0, 1, 0},     {0, 1, 0.5f}, {0, 1, 1},
        {0, 0.5f, 1}, {0, 0.25f, 1}, {0, 0, 1},    {0.25f, 0, 1}, {0.5f, 0, 1},  {0.75f, 0, 1}, {1, 0, 1},    {0.5f, 0.5f, 0.5f}};
    const glm::vec3 b = BASE[std::clamp(column, 0, HAIR_COLUMNS - 1)];
    if (row < 3)
        return b * (0.2f + 0.25f * (float)row);
    if (row == 3)
        return b;
    return b + (glm::vec3(1) - b) * (0.25f * (float)(row - 3));
}

bool hairCellAt(glm::vec2 p, int &column, int &row) {
    const glm::vec2 inGrid = p - HAIR_GRID;
    if (inGrid.x < 0 || inGrid.y < 0)
        return false;
    column = (int)(inGrid.x / HAIR_CELL), row = (int)(inGrid.y / HAIR_CELL);
    return column < HAIR_COLUMNS && row < HAIR_ROWS;
}

namespace {
// Faces and haircuts by race id (frontend-characters.md §5.4).
struct Counts {
    int32_t race;
    int faces, haircuts;
};
constexpr Counts COUNTS[] = {{0, 4, 10}, {1, 6, 11},  {2, 6, 15}, {3, 6, 9},  {4, 5, 6},  {5, 5, 6},  {7, 2, 6},  {8, 5, 6},
                             {9, 5, 3},  {10, 5, 6}, {11, 5, 8}, {12, 5, 4}, {13, 5, 5}, {14, 4, 4}, {15, 6, 6}, {16, 6, 6}};
const Counts *countsOf(int32_t race) {
    for (const Counts &c : COUNTS)
        if (c.race == race)
            return &c;
    return nullptr;
}
int wrapped(int value, int count) { return value < 0 ? count - 1 : value >= count ? 0 : value; }
} // namespace

int facesOf(int32_t race) {
    return countsOf(race) ? countsOf(race)->faces : 1;
}
int haircutsOf(int32_t race) {
    return countsOf(race) ? countsOf(race)->haircuts : 1;
}

void HairEditor::reset(hair::Style style) {
    committed = preview = hair::normalised(style);
}
void HairEditor::arrow(int row, int direction) {
    if (row == 0)
        committed.dual = !committed.dual;
    else if (row == 1 || row == 2) {
        float &value = row == 1 ? committed.start : committed.blend;
        const int percent = (int)std::lround(value * 100);
        value = (float)std::clamp(percent + (direction < 0 ? -5 : 5), 0, 100) / 100;
    }
    preview = committed;
}
void HairEditor::hover(std::optional<glm::u8vec3> color) {
    preview = committed;
    if (preview.dual && color)
        preview.second = *color;
}
void HairEditor::choose(glm::u8vec3 color) {
    committed.dual = true;
    committed.second = color;
    preview = committed;
}
bool HairEditor::press(glm::vec2 panelPoint, glm::vec2 arrowSize) {
    auto hits = [&](glm::vec2 at) {
        const auto p = panelPoint - at;
        return p.x >= 0 && p.y >= 0 && p.x < arrowSize.x && p.y < arrowSize.y;
    };
    for (int row = 0; row < 3; ++row) {
        const float y = 55.0f + 30.0f * row - arrowSize.y * .5f - 5;
        const int direction = hits({SECOND_HAIR_LEFT_ARROW_X, y}) ? -1 : hits({SECOND_HAIR_RIGHT_ARROW_X, y}) ? 1 : 0;
        if (direction) {
            arrow(row, direction);
            return true;
        }
    }
    int column, row;
    if (!secondHairCellAt(panelPoint, column, row))
        return false;
    choose(glm::u8vec3(hairCellColour(column, row) * 255.0f));
    return true;
}
glm::vec2 secondHairPanelAt(glm::vec2 window) {
    return {std::max(0.0f, window.x - LOOK_PANEL_SIZE.x - 10), 20};
}
bool secondHairCellAt(glm::vec2 point, int &column, int &row) {
    return hairCellAt(point - SECOND_HAIR_GRID + HAIR_GRID, column, row);
}

void Look::reset() {
    race = cls = sex = face = haircut = 0;
    committed = {255, 200, 50};
    preview = {255, 255, 255};
}

void Look::arrow(int row, int direction) {
    switch (row) {
    case ROW_RACE: race = wrapped(race + direction, 8); break;
    case ROW_GENDER: sex = 1 - sex; break;
    case ROW_CLASS: cls = wrapped(cls + direction, 4); break;
    case ROW_FACE: face += direction; break;
    case ROW_HAIRCUT: haircut += direction; break;
    default: break;
    }
}

int32_t Look::raceId() const { return nativesaves::raceOf(race, sex); }

void Look::wrap() {
    face = wrapped(face, facesOf(raceId()));
    haircut = wrapped(haircut, haircutsOf(raceId()));
}

nativesaves::Looks Look::record() const {
    nativesaves::Looks l;
    l.set(race, sex, face, haircut, preview);
    return l;
}

std::string Look::value(int row) const {
    static const char *const RACES[8] = {"Human", "Elf", "Dwarf", "Orc", "Goblin", "Lizard", "Undead", "Frogman"};
    switch (row) {
    case ROW_RACE: return RACES[std::clamp(race, 0, 7)];
    case ROW_GENDER: return sex ? "Female" : "Male";
    case ROW_CLASS: return classWord(cls + 1);
    case ROW_FACE: return "Face " + std::to_string(face + 1);
    case ROW_HAIRCUT: return "Haircut " + std::to_string(haircut + 1);
    default: return "";
    }
}

namespace {
namespace field = nativesaves::item;
constexpr int WEAPON = 3, CHEST = 4, FOOD = 1;
constexpr int SLOT_CHEST = 2, SLOT_LEFT = 6, SLOT_RIGHT = 7;

void stamp(nativesaves::Item &it, nativesaves::Cell region, uint32_t modifier) {
    std::memcpy(&it[field::REGION], region.data(), 8);
    std::memcpy(&it[field::MODIFIER], &modifier, 4);
}

// The class's kit (§5.5 step 3): right weapon, left weapon, chest material and the inventory.
struct Kit {
    int right, rightMaterial, left, leftMaterial, chestMaterial;
    struct Carried {
        int count, subtype, material;
    } carried[2];
};
constexpr Kit KITS[4] = {
    {17, 2, -1, 0, 1, {{2, 0, 1}, {1, 13, 1}}},   // warrior
    {6, 2, -1, 0, 27, {{1, 7, 2}, {1, 8, 2}}},    // ranger
    {10, 2, -1, 0, 26, {{2, 12, 11}, {1, 11, 2}}}, // mage
    {3, 1, 3, 1, 28, {{2, 4, 1}, {1, 5, 1}}},     // rogue
};
} // namespace

void giveStartingKit(nativesaves::Character &c, uint32_t account, nativesaves::Cell region) {
    talkgen::CRand rand;
    rand.seed(account);
    c.equipment = {};
    inventoryslots::Inventory inventory;
    inventory.initializeKitTabs();
    if (c.classType >= 1 && c.classType <= 4) {
        const Kit &k = KITS[c.classType - 1];
        c.equipment[SLOT_RIGHT] = nativesaves::makeItem(WEAPON, k.right, k.rightMaterial);
        if (k.left >= 0)
            c.equipment[SLOT_LEFT] = nativesaves::makeItem(WEAPON, k.left, k.leftMaterial);
        c.equipment[SLOT_CHEST] = nativesaves::makeItem(CHEST, 0, k.chestMaterial);
        for (const Kit::Carried &carried : k.carried)
            for (int n = 0; n < carried.count; n++)
                inventory.add(nativesaves::makeItem(WEAPON, carried.subtype, carried.material), 1);
    }
    for (int n = 0; n < 5; n++)
        inventory.add(nativesaves::makeItem(FOOD, 1, 0), 1);
    // The random parts: each worn weapon and the chest, then the carried gear in order.
    for (int slot : {SLOT_LEFT, SLOT_RIGHT, SLOT_CHEST})
        if (c.equipment[(size_t)slot][field::TYPE] != 0)
            stamp(c.equipment[(size_t)slot], region, (uint32_t)rand.next());
    c.inventory = std::move(inventory.tabs);
    for (auto &tab : c.inventory)
        for (auto &stack : tab) {
            const int type = stack.item[field::TYPE];
            if ((type >= 3 && type <= 9) || type == 0x1a)
                stamp(stack.item, region, (uint32_t)rand.next());
        }
    c.recipes.clear();
    addDefaultRecipes(c, 0);
}

void addDefaultRecipes(nativesaves::Character &c, int argument) {
    auto add = [&](int type, int subtype, int material) {
        nativesaves::Item it = nativesaves::makeItem(type, subtype, material);
        it[field::RARITY] = (uint8_t)argument;
        for (const auto &known : c.recipes)
            if (nativeinteract::sameItem(known.data(), it.data())) return;
        c.recipes.push_back(it);
    };
    for (int subtype : {1, 2, 4, 10, 8, 11, 7})
        add(1, subtype, 0);
    if (argument != 0)
        return;
    for (int subtype = 0; subtype <= 6; subtype++)
        add(0x1c, subtype, 0);
    add(0xb, 0x1a, 0);
    add(0xb, 0x16, 0);
    if (c.classType == 4)
        add(0xb, 9, 0x1c);
    if (c.classType == 2)
        add(0xb, 9, 0x1b);
    if (c.classType == 3) {
        add(0xb, 9, 0x1a);
        add(0xb, 9, 0x1a); // (asked twice by the game; the second changes nothing)
    }
    for (int material : {1, 2, 11, 12})
        add(0xb, 10, material);
}

bool nameAccepted(const std::string &utf8) {
    size_t units = 0;
    for (size_t i = 0; i < utf8.size();) {
        const unsigned char c = (unsigned char)utf8[i];
        const size_t length = c < 0x80 ? 1 : c < 0xe0 ? 2 : c < 0xf0 ? 3 : 4;
        units += length == 4 ? 2 : 1; // (a character beyond 0xffff is two UTF-16 units)
        i += length;
    }
    return units >= 2 && units <= 15;
}

nativesaves::Character newCharacter(nativesaves::Cell homeCell, const std::array<int64_t, 3> &position) {
    nativesaves::Character c;
    c.level = 1;
    c.classType = 1;
    c.equipment[SLOT_LEFT] = c.equipment[SLOT_RIGHT] = nativesaves::makeItem(WEAPON, 0, 1); // iron swords
    c.equipment[SLOT_CHEST][field::MATERIAL] = 1;
    c.homeCell = homeCell;
    c.position = position;
    return c;
}

// --- The start spot ----------------------------------------------------------------------------

namespace {
constexpr int32_t CELL_BLOCKS = 16, AREA_CELLS = 64;
constexpr int32_t TEST_X = 8, TEST_Y = 40;       // the block of a cell the ground tests read
constexpr int32_t STAND_X = 8, STAND_Y = 32;      // the block a new character stands on
constexpr float FLAT_RISE = 4.0f;                 // most height difference over 16 blocks
constexpr int STARTING_BIOMES[] = {0, 2, 7};

int32_t floorDivide(int32_t a, int32_t b) { return a / b - ((a % b != 0) && ((a < 0) != (b < 0))); }
} // namespace

bool startCellFits(const StartSpotWorld &w, nativesaves::Cell cell) {
    if (w.cellObject(cell[0], cell[1]) != START_CELL_OBJECT)
        return false;
    const int32_t x = CELL_BLOCKS * cell[0] + TEST_X, y = CELL_BLOCKS * cell[1] + TEST_Y;
    const float here = w.ground(x, y);
    if (here < 0.0f)
        return false;
    const int biome = w.biome(x, y);
    if (std::find(std::begin(STARTING_BIOMES), std::end(STARTING_BIOMES), biome) == std::end(STARTING_BIOMES))
        return false;
    if (w.placeKind(cell[0], cell[1]) != START_PLACE_KIND)
        return false;
    if (std::abs(w.ground(x + CELL_BLOCKS, y) - here) > FLAT_RISE || std::abs(w.ground(x, y + CELL_BLOCKS) - here) > FLAT_RISE)
        return false;
    return !w.playable || w.playable(cell);
}

bool findStartCell(const StartSpotWorld &w, const std::function<int()> &rand, nativesaves::Cell origin, int radiusFrom, int radiusSpread,
                   int maxSteps, nativesaves::Cell &out) {
    constexpr double RAND_MAX_VALUE = 32767.0;
    const double angle = (double)rand() * 6.283185307179586 / RAND_MAX_VALUE;
    const int radius = radiusSpread > 0 ? rand() % radiusSpread + radiusFrom : radiusFrom;
    int32_t x = origin[0] + (int32_t)(radius * std::cos(angle));
    const int32_t y = origin[1] + (int32_t)(radius * std::sin(angle));
    // Walking a cell at a time asks each area 64 times; the answer is the same, so an area
    // already turned down is not tested again.
    bool tested = false;
    nativesaves::Cell lastArea{};
    for (int step = 0; step < maxSteps; step++, x++) {
        const nativesaves::Cell area{floorDivide(x, AREA_CELLS), floorDivide(y, AREA_CELLS)};
        if (tested && area == lastArea)
            continue;
        tested = true, lastArea = area;
        const nativesaves::Cell cell = w.areaCell(area[0], area[1]);
        if (startCellFits(w, cell)) {
            out = cell;
            return true;
        }
    }
    return false;
}

std::array<int32_t, 2> startBlock(nativesaves::Cell cell) { return {CELL_BLOCKS * cell[0] + STAND_X, CELL_BLOCKS * cell[1] + STAND_Y}; }

// --- Options and controls --------------------------------------------------------------------

void OptionsScreen::open(const gameoptions::Options &current, std::vector<DisplayMode> available) {
    values = current;
    modes = std::move(available);
    mode = modes.empty() ? 0 : (int)modes.size() - 1;
    for (size_t i = 0; i < modes.size(); i++)
        if (modes[i].width == values.resolutionX && modes[i].height == values.resolutionY)
            mode = (int)i;
}

const char *OptionsScreen::label(int row) {
    static const char *const LABELS[OPTION_ROWS] = {"Mode",         "Resolution",    "Refresh Rate",  "VSync",
                                                    "Render Distance", "CPU Usage",  "Sound FX Volume", "Music Volume",
                                                    "Loop Music",   "Camera Speed",  "Camera Smoothness", "Invert Y Axis",
                                                    "FPS Limit",    "Language",      "Rarity"};
    return row >= 0 && row < OPTION_ROWS ? LABELS[row] : "";
}

std::string OptionsScreen::value(int row) const {
    const gameoptions::Options &o = values;
    auto percent = [](int v) { return std::to_string(v) + "%"; };
    auto onOff = [](int v) { return std::string(v ? "ON" : "OFF"); };
    switch (row) {
    case OPT_MODE: return o.fullscreen ? "Fullscreen" : "Windowed";
    case OPT_RESOLUTION:
        return modes.empty() ? std::to_string(o.resolutionX) + " x " + std::to_string(o.resolutionY)
                             : std::to_string(modes[(size_t)mode].width) + " x " + std::to_string(modes[(size_t)mode].height);
    case OPT_REFRESH: return std::to_string(o.refreshRate) + " Hz";
    case OPT_VSYNC: return onOff(o.vsync);
    case OPT_RENDER_DISTANCE: return percent(o.renderDistance);
    case OPT_CPU: return o.cpuPerformance ? "FULL" : "REDUCED";
    case OPT_SOUND: return percent(o.soundVolume);
    case OPT_MUSIC: return percent(o.musicVolume);
    case OPT_LOOP: return onOff(o.musicLoop);
    case OPT_CAMERA_SPEED: return percent(o.cameraSpeed);
    case OPT_CAMERA_SMOOTHNESS: return percent(o.cameraSmoothness);
    case OPT_INVERT_Y: return o.invertY ? "Yes" : "No";
    case OPT_FPS: return o.fpsLimitText();
    case OPT_LANGUAGE: return std::to_string(o.language);
    case OPT_RARITY: return o.showRarity ? "as stars" : "color coded";
    default: return "";
    }
}

void OptionsScreen::arrow(int row, int direction) {
    gameoptions::Options &o = values;
    const bool left = direction < 0;
    auto step = [&](int &v, int by, int lo, int hi) { v = std::clamp(v + (left ? -by : by), lo, hi); };
    auto nearestRate = [&]() {
        if (modes.empty() || modes[(size_t)mode].rates.empty())
            return;
        const auto &rates = modes[(size_t)mode].rates;
        int best = rates.front();
        for (int r : rates)
            if (std::abs(r - o.refreshRate) < std::abs(best - o.refreshRate))
                best = r;
        o.refreshRate = best;
    };
    switch (row) {
    case OPT_MODE: o.fullscreen = 1 - o.fullscreen; break;
    case OPT_RESOLUTION:
        if (modes.empty())
            break;
        mode = std::clamp(mode + (left ? -1 : 1), 0, (int)modes.size() - 1);
        o.resolutionX = modes[(size_t)mode].width, o.resolutionY = modes[(size_t)mode].height;
        nearestRate();
        break;
    case OPT_REFRESH: {
        if (modes.empty() || modes[(size_t)mode].rates.empty())
            break;
        const auto &rates = modes[(size_t)mode].rates;
        auto at = std::find(rates.begin(), rates.end(), o.refreshRate);
        int i = at == rates.end() ? 0 : (int)(at - rates.begin());
        i = std::clamp(i + (left ? -1 : 1), 0, (int)rates.size() - 1);
        o.refreshRate = rates[(size_t)i];
        break;
    }
    case OPT_VSYNC: o.vsync = 1 - o.vsync; break;
    case OPT_RENDER_DISTANCE: step(o.renderDistance, 5, 5, 100); break;
    case OPT_CPU: o.cpuPerformance = left ? 0 : 1; break;
    case OPT_SOUND: step(o.soundVolume, 10, 0, 100); break;
    case OPT_MUSIC: step(o.musicVolume, 10, 0, 100); break;
    case OPT_LOOP: o.musicLoop = 1 - o.musicLoop; break;
    case OPT_CAMERA_SPEED: step(o.cameraSpeed, 5, 10, 100); break;
    case OPT_CAMERA_SMOOTHNESS: step(o.cameraSmoothness, 5, 0, 100); break;
    case OPT_INVERT_Y: o.invertY = 1 - o.invertY; break;
    case OPT_FPS: o.minTimeStep = std::clamp(o.minTimeStep + (left ? 1 : -1), 0, 50); break; // left: fewer fps
    case OPT_LANGUAGE: o.language += left ? -1 : 1; break;
    case OPT_RARITY: o.showRarity = 1 - o.showRarity; break;
    default: break;
    }
}

gameoptions::Binding keyBinding(int glfwKey) { return GameInput::keyBinding(glfwKey); }

bool keyMatches(const gameoptions::Binding &b, int glfwKey) { return GameInput::keyMatches(b, glfwKey); }

// --- The multiplayer menu (netcode-transport.md §3.2) --------------------------------------------

const char *hostButtonLabel(const FriendRow &row) { return row.invited || row.joined ? "uninvite" : "invite"; }

const char *guestButtonLabel(const FriendRow &row) { return row.hostOfMine ? "leave" : "join"; }

std::string friendRowText(const FriendRow &row) {
    std::string text = row.name;
    if (row.hostOfMine)
        text += " (host)";
    if (row.joined)
        text += " (joined)";
    else if (row.invited)
        text += " (invited)";
    return text;
}

glm::vec3 enteredFeet(const nativesaves::Character &c, const nativeprogress::Frame &frame, glm::vec3 home) {
    if (c.position == std::array<int64_t, 3>{0, 0, 0})
        return home;
    return nativeprogress::localFeet(c.position, frame);
}

} // namespace frontend
