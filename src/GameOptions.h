#pragma once
// The game's settings files in its own format (docs/specs/frontend-options.md §1, §4.1):
// options.cfg (21 settings, "name value" per line) and controls.cfg (23 bindings, "Name code
// kind" per line). CubeVulkan keeps both in its own folder beside the engine.
#include <array>
#include <filesystem>
#include <string>

namespace gameoptions {

// options.cfg (§1.2), with the game's defaults.
struct Options {
    int fullscreen = 1;
    int resolutionX = 0, resolutionY = 0; // 0: not chosen (the desktop's size)
    int refreshRate = 0;
    int antiAliasing = 0;
    int renderDistance = 50;   // 5..100, step 5
    int cpuPerformance = 1;    // 0 REDUCED, 1 FULL
    int soundVolume = 100;     // 0..100, step 10
    int musicVolume = 100;     // 0..100, step 10
    int cameraSpeed = 50;      // 10..100, step 5
    int cameraSmoothness = 80; // 0..100, step 5
    int language = 0;
    int invertY = 0;
    int minTimeStep = 0;       // 0..50 ms; 0 = no frame limit
    int smallGUI = 0;
    int vsync = 1;
    int showRarity = 1;        // 1 "as stars", 0 "color coded"
    int musicLoop = 0;
    float guiScale = 1.0f;     // 1.25^-4 .. 1.25^5 (F2 / F3)
    float mapScale = 1.0f;     // 1.1^-3 .. 1.1^3 (F5 / F6)
    int antiAliasingSamples = 8;

    // The camera's smoothing constant: 0.005 ^ (cameraSmoothness x 0.01).
    double smoothingConstant() const;
    // The FPS limit as the screen shows it: "none" for 0, else 1000 / value + " fps".
    std::string fpsLimitText() const;

    // --- What the settings do when applied ---
    // The zones the game draws around the camera: renderDistance x 30 / 100 (integer), but 3
    // when that is below 4 (0xa5a50).
    int zonesDrawn() const;
    // CubeVulkan's view distance in blocks ([Graphics] ViewDistance) for those zones. The default
    // render distance (50 %, 15 zones) gives 400 blocks, CubeVulkan's own default; the others
    // scale with the zone count (3 zones 80 blocks, 30 zones 800).
    float viewDistance() const;
    // How much the camera turns for the mouse's movement, relative to the default speed (50):
    // cameraSpeed / 50. (The spec says the look input is scaled by the option, not by how much.)
    float lookSpeed() const;
    // The shortest frame in milliseconds (minTimeStep, 0 for none).
    int shortestFrameMs() const;
};
constexpr int DEFAULT_ZONES_DRAWN = 15;  // renderDistance 50
constexpr float DEFAULT_VIEW_DISTANCE = 400.0f;
constexpr float GUI_SCALE_MIN = 0.4096f, GUI_SCALE_MAX = 3.0517578f;
constexpr float MAP_SCALE_MIN = 0.7513148f, MAP_SCALE_MAX = 1.331f;

// Reads options.cfg word by word (0x13aa50): a known name is followed by its value, anything
// else is skipped; reading stops at the end or at the first value that does not read. A missing
// file leaves the defaults.
Options readOptions(const std::string &text);
bool loadOptions(const std::filesystem::path &file, Options &out);
// The file as 0x13b1d0 writes it: one line per setting, in the game's order.
std::string writeOptions(const Options &o);
bool saveOptions(const std::filesystem::path &file, const Options &o);

// controls.cfg (§4.1).
struct Binding {
    int code = 0; // a keyboard scan code, or a mouse button (0 left, 1 right, 2 middle, 3 fourth)
    int kind = 0; // 0 keyboard, 1 mouse
    bool operator==(const Binding &) const = default;
};
constexpr int BINDINGS = 23;
enum Action {
    ATTACK, SPECIAL_ATTACK, CLASS_SKILL, ULTIMATE_SKILL, QUICK_ITEM, UP, DOWN, LEFT, RIGHT, JUMP, DODGE, USE, LAMP,
    WALK, CALL_PET, WORLD_MAP, RING_MENU, INVENTORY, CRAFTING_MENU, SYSTEM_MENU, MULTIPLAYER_MENU, ZOOM_IN, ZOOM_OUT
};
using Controls = std::array<Binding, BINDINGS>;
const char *bindingFileName(int action);  // "ButtonAttack", ...
const char *bindingLabel(int action);     // "Normal Attack", ...
Controls defaultControls();
Controls readControls(const std::string &text);
bool loadControls(const std::filesystem::path &file, Controls &out);
std::string writeControls(const Controls &c);
bool saveControls(const std::filesystem::path &file, const Controls &c);
// The name the controls screen shows for a binding: "Mouse " + number, "Unassigned" for (0, 0),
// otherwise the key's name (open question 8: CubeVulkan names scan codes with its own table).
std::string bindingName(const Binding &b);

// Opens the pair at start-up the way the game does: read, then written straight back, so a
// missing or partial file is completed with defaults.
void loadAndComplete(const std::filesystem::path &folder, Options &options, Controls &controls);

} // namespace gameoptions
