// The game's settings files, written from docs/specs/frontend-options.md §1 and §4.1.
#include "GameOptions.h"
#include "CameraMotion.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace gameoptions {

double Options::smoothingConstant() const {
    return cameramotion::angleRate(cameraSmoothness);
}

std::string Options::fpsLimitText() const {
    return minTimeStep == 0 ? std::string("none") : std::to_string(1000 / minTimeStep) + " fps";
}

int Options::zonesDrawn() const {
    constexpr int FEWEST = 4, BELOW_FEWEST = 3;
    const int zones = renderDistance * 30 / 100;
    return zones < FEWEST ? BELOW_FEWEST : zones;
}

float Options::viewDistance() const { return DEFAULT_VIEW_DISTANCE * (float)zonesDrawn() / (float)DEFAULT_ZONES_DRAWN; }

float Options::lookSpeed() const {
    constexpr float DEFAULT_SPEED = 50.0f;
    return (float)cameraSpeed / DEFAULT_SPEED;
}

int Options::shortestFrameMs() const { return std::max(0, minTimeStep); }

namespace {

// The settings in the file's order, each its name and where it lives.
struct Setting {
    const char *name;
    int Options::*integer;
    float Options::*real;
};
const Setting SETTINGS[] = {
    {"fullscreen", &Options::fullscreen, nullptr},
    {"resolutionX", &Options::resolutionX, nullptr},
    {"resolutionY", &Options::resolutionY, nullptr},
    {"refreshRate", &Options::refreshRate, nullptr},
    {"antiAliasing", &Options::antiAliasing, nullptr},
    {"renderDistance", &Options::renderDistance, nullptr},
    {"cpuPerformance", &Options::cpuPerformance, nullptr},
    {"soundVolume", &Options::soundVolume, nullptr},
    {"musicVolume", &Options::musicVolume, nullptr},
    {"cameraSpeed", &Options::cameraSpeed, nullptr},
    {"cameraSmoothness", &Options::cameraSmoothness, nullptr},
    {"language", &Options::language, nullptr},
    {"invertY", &Options::invertY, nullptr},
    {"minTimeStep", &Options::minTimeStep, nullptr},
    {"smallGUI", &Options::smallGUI, nullptr},
    {"vsync", &Options::vsync, nullptr},
    {"showRarity", &Options::showRarity, nullptr},
    {"musicLoop", &Options::musicLoop, nullptr},
    {"guiScale", nullptr, &Options::guiScale},
    {"mapScale", nullptr, &Options::mapScale},
    {"antiAliasingSamples", &Options::antiAliasingSamples, nullptr},
};

bool readText(const fs::path &file, std::string &text) {
    std::ifstream in(file, std::ios::binary);
    if (!in)
        return false;
    std::ostringstream s;
    s << in.rdbuf();
    text = s.str();
    return true;
}

// Written whole to a temporary file first, so a failed write leaves the old file.
bool writeText(const fs::path &file, const std::string &text) {
    fs::path temporary = file;
    temporary += ".tmp";
    {
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        if (!(out << text))
            return false;
    }
    std::error_code ec;
    fs::rename(temporary, file, ec);
    if (ec) {
        fs::copy_file(temporary, file, fs::copy_options::overwrite_existing, ec);
        fs::remove(temporary);
    }
    return !ec;
}

const char *const BINDING_NAMES[BINDINGS] = {
    "ButtonAttack", "ButtonSpecialAttack", "ButtonClassSkill", "ButtonUltimateSkill", "ButtonQuickItem", "ButtonUp",
    "ButtonDown", "ButtonLeft", "ButtonRight", "ButtonJump", "ButtonDodge", "ButtonUse", "ButtonLamp", "ButtonWalk",
    "ButtonCallPet", "ButtonWorldMap", "ButtonRingMenu", "ButtonInventory", "ButtonCraftingMenu", "ButtonSystemMenu",
    "ButtonMultiplayerMenu", "ButtonZoomIn", "ButtonZoomOut"};
const char *const BINDING_LABELS[BINDINGS] = {
    "Normal Attack", "Special Attack", "Class Skill", "Ultimate Skill", "Use Quick Item", "Move Forward",
    "Move Backward", "Move Left", "Move Right", "Jump", "Dodge", "Use/Interact", "Lamp", "Walk", "Call Pet",
    "World Map", "Ring Menu", "Inventory", "Crafting Menu", "System Menu", "Multiplayer Menu", "Zoom In", "Zoom Out"};
constexpr int KEYBOARD = 0, MOUSE = 1;

} // namespace

Options readOptions(const std::string &text) {
    Options o;
    std::istringstream in(text);
    std::string word;
    while (in >> word) {
        for (const Setting &s : SETTINGS) {
            if (word != s.name)
                continue;
            bool ok;
            if (s.real) {
                float v;
                ok = (bool)(in >> v);
                if (ok)
                    o.*(s.real) = v;
            } else {
                int v;
                ok = (bool)(in >> v);
                if (ok)
                    o.*(s.integer) = v;
            }
            if (!ok)
                return o; // the first value that does not read ends the reading
            break;
        }
    }
    return o;
}

bool loadOptions(const fs::path &file, Options &out) {
    std::string text;
    if (!readText(file, text))
        return false;
    out = readOptions(text);
    return true;
}

std::string writeOptions(const Options &o) {
    std::ostringstream out;
    for (const Setting &s : SETTINGS) {
        out << s.name << ' ';
        if (s.real)
            out << o.*(s.real); // the stream's default float format
        else
            out << o.*(s.integer);
        out << '\n';
    }
    return out.str();
}

bool saveOptions(const fs::path &file, const Options &o) { return writeText(file, writeOptions(o)); }

const char *bindingFileName(int action) { return action >= 0 && action < BINDINGS ? BINDING_NAMES[action] : ""; }
const char *bindingLabel(int action) { return action >= 0 && action < BINDINGS ? BINDING_LABELS[action] : ""; }

Controls defaultControls() {
    Controls c;
    auto key = [](int code) { return Binding{code, KEYBOARD}; };
    auto mouse = [](int button) { return Binding{button, MOUSE}; };
    c[ATTACK] = mouse(0), c[SPECIAL_ATTACK] = mouse(1), c[CLASS_SKILL] = key(0x2a), c[ULTIMATE_SKILL] = key(0x13);
    c[QUICK_ITEM] = key(0x10), c[UP] = key(0x11), c[DOWN] = key(0x1f), c[LEFT] = key(0x1e), c[RIGHT] = key(0x20);
    c[JUMP] = key(0x39), c[DODGE] = mouse(2), c[USE] = key(0x12), c[LAMP] = key(0x21), c[WALK] = key(0x1d);
    c[CALL_PET] = key(0x14), c[WORLD_MAP] = key(0x32), c[RING_MENU] = key(0x0f), c[INVENTORY] = key(0x30);
    c[CRAFTING_MENU] = key(0x2e), c[SYSTEM_MENU] = key(0x18), c[MULTIPLAYER_MENU] = key(0x24);
    c[ZOOM_IN] = key(0), c[ZOOM_OUT] = key(0);
    return c;
}

Controls readControls(const std::string &text) {
    Controls c = defaultControls();
    std::istringstream in(text);
    std::string word;
    while (in >> word)
        for (int i = 0; i < BINDINGS; i++)
            if (word == BINDING_NAMES[i]) {
                int code, kind;
                if (!(in >> code >> kind))
                    return c;
                c[(size_t)i] = {code, kind};
                break;
            }
    return c;
}

bool loadControls(const fs::path &file, Controls &out) {
    std::string text;
    if (!readText(file, text))
        return false;
    out = readControls(text);
    return true;
}

std::string writeControls(const Controls &c) {
    std::ostringstream out;
    for (int i = 0; i < BINDINGS; i++)
        out << BINDING_NAMES[i] << ' ' << c[(size_t)i].code << ' ' << c[(size_t)i].kind << '\n';
    return out.str();
}

bool saveControls(const fs::path &file, const Controls &c) { return writeText(file, writeControls(c)); }

std::string bindingName(const Binding &b) {
    if (b.kind == MOUSE)
        return "Mouse " + std::to_string(b.code);
    if (b.code == 0)
        return "Unassigned";
    // The keys of a US layout, by scan code (set 1).
    static const char *const KEYS[0x59] = {
        nullptr, "Escape", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "Backspace", "Tab",
        "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]", "Enter", "Ctrl", "A", "S",
        "D", "F", "G", "H", "J", "K", "L", ";", "'", "`", "Shift", "\\", "Z", "X", "C", "V",
        "B", "N", "M", ",", ".", "/", "Right Shift", "Num *", "Alt", "Space", "Caps Lock", "F1", "F2", "F3", "F4", "F5",
        "F6", "F7", "F8", "F9", "F10", "Num Lock", "Scroll Lock", "Num 7", "Num 8", "Num 9", "Num -", "Num 4", "Num 5",
        "Num 6", "Num +", "Num 1", "Num 2", "Num 3", "Num 0", "Num Del", nullptr, nullptr, nullptr, "F11", "F12"};
    if (b.code > 0 && b.code < 0x59 && KEYS[b.code])
        return KEYS[b.code];
    return "Key " + std::to_string(b.code);
}

void loadAndComplete(const fs::path &folder, Options &options, Controls &controls) {
    options = Options{};
    controls = defaultControls();
    loadOptions(folder / "options.cfg", options);
    loadControls(folder / "controls.cfg", controls);
    saveOptions(folder / "options.cfg", options);
    // (the game writes controls.cfg only when the controls screen's OK applies it; CubeVulkan
    // writes a missing one so it can be edited)
    if (!fs::exists(folder / "controls.cfg"))
        saveControls(folder / "controls.cfg", controls);
}

} // namespace gameoptions
