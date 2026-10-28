#pragma once
// The game's front screens, built by the engine itself (docs/specs/frontend-title.md,
// frontend-characters.md, frontend-options.md): the title is the ordinary world, seen by the
// gameplay camera circling the local player as it rises on the eagle, with the cubeworld.cub
// logo and the root menu over it; then character select and creation, deleting, the options and
// controls screens and the in-game system menu.
//
// The first half of this header is the screens' rules and layout, which need no window (the
// tests check them); the second half runs them with the world, the renderer and the interface.
#include "DayNight.h"
#include "GameOptions.h"
#include "Hud.h"
#include "NativeProgress.h"
#include "NativeSaves.h"
#include "Renderer.h"
#include "Saves.h"

#include <array>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class Assets;
class Audio;
class Config;
class ExtractedWorld;
class SpawnedCreatures;
class SteamSession;
class World;
struct GLFWwindow;

// What the window gave since the last frame (filled by main's GLFW callbacks).
struct MenuInput {
    double scroll = 0;
    std::vector<int> pressed, buttons, released;
    std::string typed; // text typed this frame (character creation's name)
    void clear() { scroll = 0; pressed.clear(); buttons.clear(); released.clear(); typed.clear(); }
};

namespace frontend {

// --- The camera (frontend-title.md §3.3) ------------------------------------------------------

// The gameplay camera's angles (degrees) and distance (blocks): targets and current values.
struct OrbitCamera {
    double pitch = 90, roll = 0, yaw = 180;
    double targetPitch = 90, targetRoll = 0, targetYaw = 180;
    float distance = 8, targetDistance = 8;
    // Targets and current values at once.
    void set(double pitch, double roll, double yaw, float distance);
    // While riding: the yaw target turns 3 degrees a second (never wrapped), the pitch target is
    // 110 and the distance target 12.
    void titleTargets(double dtMs);
    // Each current value moves toward its target by smooth(dt, 0.005 ^ (smoothness / 100)).
    void smooth(double dtMs, int cameraSmoothness);
    // The forward vector f of the current angles.
    glm::dvec3 forward() const;
    // The eye: the look-at point less distance x f (blocks).
    glm::dvec3 eye(glm::dvec3 lookAt) const;
};

// --- The logo (frontend-title.md §3.4) -------------------------------------------------------

// The logo's placement for a window of W x H pixels, the GUI scale and the wall clock: its
// centre (pixels, y down), its tilt about x (degrees) and its scale (the overlay's units).
struct LogoPlacement {
    glm::vec2 centre{0};
    float tiltDegrees = 0;
    float scale = 0;
};
LogoPlacement logoPlacement(float width, float height, float guiScale, uint32_t wallClockMs);
// The clip transform the renderer's screen-space path takes (DrawItem::model with NATIVE_MENU)
// for a model of `size` voxels placed so.
glm::mat4 logoClip(const LogoPlacement &p, glm::vec3 size, float width, float height);
// The shade a screen-space model's face is baked with, by its normal's axis (0 x, 1 y, 2 z) and
// sign: faces toward the viewer (z of sign `facing`: -1 for the list's creatures, +1 for the
// logo) lit, turned up (+y) a little less, sides and those turned down darker. (CubeVulkan's:
// the overlay pass's lighting is open question 1.)
float screenModelShade(int axis, int sign, int facing = -1);

// --- The root menu (frontend-title.md §4) ----------------------------------------------------

constexpr glm::vec2 ROOT_MENU_SIZE{200, 100};
enum RootEntry { START_GAME = 0, OPTIONS = 1, EXIT = 2, CONTINUE = 3 };
struct MenuLine {
    int index;           // the entry
    float textY;         // its text's y
    float bandTop, bandBottom; // the hover band
    float navigationY;   // its navigation point's y
};
std::vector<MenuLine> rootLines(bool continueShown);
// Where the root menu's node goes (GUI units) in a window of `window` GUI units.
glm::vec2 rootMenuPosition(glm::vec2 window);
// The entry under the mouse (in the widget's frame), -1 for none.
int hoveredLine(const std::vector<MenuLine> &lines, glm::vec2 mouse, float widgetWidth);

// --- The system menu (frontend-options.md §5) ------------------------------------------------

constexpr glm::vec2 SYSTEM_MENU_SIZE{150, 150};
enum SystemEntry { SYSTEM_OPTIONS = 0, SYSTEM_CONTROLS = 1, SYSTEM_START_MENU = 2, SYSTEM_EXIT = 3 };
std::vector<MenuLine> systemLines();

// --- Character select (frontend-characters.md §2) -------------------------------------------

constexpr glm::vec2 ENTRY_SIZE{600, 220};
// Entry i's place in the list's space.
glm::vec2 entryPosition(int i);
// The list's total height for n entries (the characters and "New character").
float listHeight(int entries);
// The list's top when it fits the window (centred), or the range it is kept in otherwise.
float centredListTop(int entries, float windowHeight);
// The specialisation's word (Berserker, Guardian, Sniper, Scout, Fire Mage, Water Mage,
// Assassin, Ninja) and the class's (Warrior, Ranger, Mage, Rogue).
const char *specializationWord(int cls, int spec);
const char *classWord(int cls);
// "LV 7 Human Warrior" (the race word left out for races without one).
std::string entryLine(int level, int32_t race, int cls);

// --- Character creation (frontend-characters.md §5) ------------------------------------------

constexpr glm::vec2 LOOK_PANEL_SIZE{300, 330};
enum LookRow { ROW_RACE, ROW_GENDER, ROW_CLASS, ROW_FACE, ROW_HAIRCUT, LOOK_ROWS };
// The look panel's row heights (y of each label), and the hair colour title's.
constexpr float LOOK_ROW_Y[LOOK_ROWS] = {27, 57, 87, 117, 147};
constexpr float HAIR_TITLE_Y = 182;
// The hair-colour grid: 16 columns by 7 rows of 17-unit cells from (13, 197).
constexpr glm::vec2 HAIR_GRID{13, 197};
constexpr int HAIR_COLUMNS = 16, HAIR_ROWS = 7;
constexpr float HAIR_CELL = 17;
glm::vec3 hairCellColour(int column, int row);
// The grid cell under a point of the panel; false off the grid.
bool hairCellAt(glm::vec2 panelPoint, int &column, int &row);

struct Look {
    int race = 0, cls = 0, sex = 0, face = 0, haircut = 0; // R (0..7), C (0..3), S, F, K
    std::array<uint8_t, 3> committed{255, 200, 50}, preview{255, 255, 255};
    void reset();
    // An arrow (−1 left, +1 right) on a row (§5.3).
    void arrow(int row, int direction);
    int32_t raceId() const; // from R and S
    // Step 3 of applying: F and K wrapped by the race's faces and haircuts.
    void wrap();
    // The looks record (player data +0x00..+0x13) with the preview colour.
    nativesaves::Looks record() const;
    std::string value(int row) const; // the panel's value text
};
int facesOf(int32_t race);
int haircutsOf(int32_t race);

// The starting kit (0x913b0): equipment, inventory and recipes for the class, the random parts
// from srand(the account's low 32 bits); `region` is where the player stands.
void giveStartingKit(nativesaves::Character &c, uint32_t account, nativesaves::Cell region);
// The default recipes (0x9afb0) with its argument.
void addDefaultRecipes(nativesaves::Character &c, int argument);
// The name field's rule: 2 to 15 UTF-16 code units.
bool nameAccepted(const std::string &utf8);
// A character as the New character entry resets it (§4.1): level 1, health its maximum (set by
// the caller), iron swords, class 1, with the home cell and position of the start spot.
nativesaves::Character newCharacter(nativesaves::Cell homeCell, const std::array<int64_t, 3> &position);
// Where Continue or Select puts a character (frontend-title.md §6.1): exactly where it was saved
// (local feet, a block under the record), with no search for ground. A character never placed
// (position 0, 0, 0; CubeVulkan's stand-in for a world kept as extracted zones) starts at `home`.
glm::vec3 enteredFeet(const nativesaves::Character &c, const nativeprogress::Frame &frame, glm::vec3 home);

// --- The start spot (frontend-characters.md §4.2, 0x2d1100) ----------------------------------

// What the start spot's tests ask of the world, by 16-block cell and by block.
struct StartSpotWorld {
    std::function<nativesaves::Cell(int32_t qx, int32_t qy)> areaCell; // the area's landmark cell (0x2d3b60)
    std::function<int(int32_t cx, int32_t cy)> cellObject;             // 0x69c60
    std::function<int(int32_t cx, int32_t cy)> placeKind;              // the kind of the cell's place (its zone's)
    std::function<int(int32_t x, int32_t y)> biome;                    // the region's biome at a block (0x2b9080)
    std::function<float(int32_t x, int32_t y)> ground;                 // the terrain height at a block (0x35ea0)
    std::function<bool(nativesaves::Cell cell)> playable;              // CubeVulkan has the cell's ground (true by default)
};
constexpr int START_CELL_OBJECT = 0x35, START_PLACE_KIND = 5;
constexpr int START_RADIUS_FROM = 100000, START_RADIUS_SPREAD = 100000; // cells
// Whether a cell passes the start spot's tests: its object is 0x35, the ground at block
// (16 x + 8, 16 y + 40) is not below 0, the region's biome there is 0, 2 or 7, its place is a
// town (kind 5) and the ground is flat (at most 4 blocks of height over 16 blocks in x and in y).
// The danger measures of the spec's open question 3 are not tested.
bool startCellFits(const StartSpotWorld &world, nativesaves::Cell cell);
// The start cell: an angle and a radius drawn with `rand` (0..32767), starting at
// `origin` + (r cos a, r sin a) cells and walking x forward a cell at a time, each time asking for
// the area's landmark cell, until one fits. False when none fits within `maxSteps` steps.
bool findStartCell(const StartSpotWorld &world, const std::function<int()> &rand, nativesaves::Cell origin, int radiusFrom,
                   int radiusSpread, int maxSteps, nativesaves::Cell &out);
// The block a new character stands on in its start cell (§4.1 step 5): (16 cx + 8, 16 cy + 32).
std::array<int32_t, 2> startBlock(nativesaves::Cell cell);

// --- Options and controls (frontend-options.md §2, §4.2) -------------------------------------

constexpr glm::vec2 OPTIONS_SIZE{450, 560};
constexpr glm::vec2 CONTROLS_SIZE{500, 600};
enum OptionRow {
    OPT_MODE, OPT_RESOLUTION, OPT_REFRESH, OPT_VSYNC, OPT_RENDER_DISTANCE, OPT_CPU, OPT_SOUND, OPT_MUSIC, OPT_LOOP,
    OPT_CAMERA_SPEED, OPT_CAMERA_SMOOTHNESS, OPT_INVERT_Y, OPT_FPS, OPT_LANGUAGE, OPT_RARITY, OPTION_ROWS
};
struct DisplayMode {
    int width = 0, height = 0;
    std::vector<int> rates;
};
struct OptionsScreen {
    gameoptions::Options values;
    std::vector<DisplayMode> modes;
    int mode = 0;
    void open(const gameoptions::Options &current, std::vector<DisplayMode> available);
    void arrow(int row, int direction);
    static const char *label(int row);
    static float rowY(int row) { return 26.0f + 30.0f * (float)row; }
    std::string value(int row) const;
};
struct ControlsScreen {
    gameoptions::Controls values = gameoptions::defaultControls();
    int waiting = -1, first = 0;
    static float rowY(int visibleRow) { return 27.0f + 30.0f * (float)visibleRow; }
    static int visibleRows() { return (int)((CONTROLS_SIZE.y - 120.0f) / 30.0f); }
};
// A GLFW key's binding (the scan code as the game reads it), and whether a key is a binding's.
gameoptions::Binding keyBinding(int glfwKey);
bool keyMatches(const gameoptions::Binding &b, int glfwKey);

// --- The multiplayer menu (netcode-transport.md §3) -----------------------------------------------

// One friend as the menu lists it: the name, then " (host)" when this player is that friend's
// guest, then " (joined)" when the friend is connected to this player's host, else " (invited)".
struct FriendRow {
    uint64_t id = 0;
    std::string name;
    bool hostOfMine = false; // this player is that friend's guest
    bool joined = false;     // connected to this player's host
    bool invited = false;    // in this player's host's invited set
    bool invitation = false; // that friend's invitation to this player is live
};
// What the menu needs from the game's multiplayer; the game (or a mod) fills it in.
struct FriendsMenu {
    // The friends with their states. The menu takes the list when it opens (it is not rebuilt
    // while it stays open, §3.1) and refreshes the states of those rows every frame.
    std::function<std::vector<FriendRow>()> list;
    // The row's two buttons (§3.3): left invite / uninvite (kick), right join / leave.
    std::function<void(uint64_t)> hostButton, guestButton;
    // Shown when the list is empty (how to play together in this set-up).
    std::string emptyText = "No friends to show";
};
// The menu's row buttons (§3.2): the left one says "invite" or, for an invited friend, "uninvite";
// the right one "join" or, while this player is that friend's guest, "leave".
const char *hostButtonLabel(const FriendRow &row);
const char *guestButtonLabel(const FriendRow &row);
// The row's text: the name and its state words.
std::string friendRowText(const FriendRow &row);

// --- Running the screens ---------------------------------------------------------------------

struct Services {
    GLFWwindow *window = nullptr;
    MenuInput *input = nullptr;
    const Assets *assets = nullptr;
    Renderer *renderer = nullptr;
    Hud *hud = nullptr;                   // the interface to draw with (the game's own in play); null: one of the screens' own
    World *world = nullptr;
    ExtractedWorld *extracted = nullptr;  // null without an extracted or generated world
    SpawnedCreatures *spawned = nullptr;  // the game's creatures with the native player, if any
    SteamSession *steam = nullptr;
    Audio *audio = nullptr;
    FriendsMenu *friends = nullptr;       // the multiplayer menu's friends (null: no multiplayer menu)
    std::filesystem::path root;           // CubeVulkan's folder (options.cfg, controls.cfg, Save/)
    nativesaves::CharacterStore *store = nullptr;
    const std::vector<CharacterEntry> *recorded = nullptr;
    const nativeprogress::SpriteTable *sprites = nullptr;
    gameoptions::Options *options = nullptr;
    gameoptions::Controls *controls = nullptr;
    DayNightSettings dayNight;
    float minutesPerSecond = 1;
    // The world's time of day (hours): Save/world.db's when there is one, else the new world's 10:00.
    float startHour = 10;
    std::function<void(const std::string &)> log;
    // Puts options the options screen applied into effect (the window, the view distance, the
    // frame limit...); the screens set the volumes themselves.
    std::function<void(const gameoptions::Options &)> applyOptions;
    // The start spot: the world the search asks, and where it starts (the world's origin, 0 0).
    StartSpotWorld startSpot;
    nativesaves::Cell startOrigin{0, 0};
    bool farStart = true; // the spec's far-away radius; false: the search starts at the origin itself
};

struct Request {
    std::string page = "title"; // "title", "characters", "create", "options", "controls"
    std::string capture;        // a screenshot of the page, then quit
    double captureAfter = 0;
    bool readOnly = false;       // a capture run: nothing is written
    glm::dvec2 pointer{-1, -1};  // testing: the mouse held there (window pixels), for captures
    double clickAfter = -1;      // testing: a left click at the pointer this many seconds in (the
                                 // world loaded); the capture is then the game's, after the screens
};

struct Result {
    enum Action { Quit, Play, Resume, Title, Stay } action = Quit;
    int slot = -1;     // the character to play
    bool created = false; // made in character creation (its help sheet was shown)
    // Play: the player was rebuilt from the save and stands at `feet` (local blocks, CubeVulkan's
    // feet a block under the record), facing yaw 0 (frontend-title.md §6.1). The game starts
    // there, without searching for ground.
    bool placed = false;
    glm::vec3 feet{0};
    std::string error;
};

// The title, character select and creation, over the running world.
Result run(Services &s, const Request &request);

class Session;
// The screens over the game while it plays (frontend-options.md §3, §5): the system menu and the
// options, controls and help screens, and the interface keys F1 to F6. The world keeps running
// behind them; gameplay leaves the keys to them while one is open.
//
//   frontend::InGameMenus menus(services, request);
//   menus.start(error);
//   each frame: Result::Action a = menus.frame(dtMs); ... menus.draw(overlay);
class InGameMenus {
  public:
    InGameMenus(Services &s, const Request &request);
    ~InGameMenus();
    bool start(std::string &error);
    // The System Menu binding (or Escape): shows the menu, or hides it when shown.
    void toggleSystemMenu();
    // The Multiplayer Menu binding (default J): shows the friends list, or hides it when shown.
    void toggleMultiplayerMenu();
    // Whether a screen is open, which then has the keys and the pointer.
    bool open() const;
    // One frame: the interface keys always; the keys and clicks of this frame while a screen is
    // open (those are taken from the input). Stay, Title (Start Menu) or Quit (Exit Game).
    Result::Action frame(double dtMs);
    // The open screens, appended to the frame's interface.
    void draw(std::vector<OverlayVertex> &out);
    // F4 hides the whole interface.
    bool interfaceShown() const;

  private:
    std::unique_ptr<Session> session;
};

} // namespace frontend
