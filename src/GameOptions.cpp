// The game's settings files, written from docs/specs/frontend-options.md §1 and §4.1.
#include "GameOptions.h"
#include "CameraMotion.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string_view>
#include <cstdint>
#include <utility>

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
    static constexpr int order[] = {0,1,4,5,6,7,8,9,10,11,12,3,2,13,14,15,16,17,20,18,19,21,22};
    for (int i : order)
        out << BINDING_NAMES[i] << ' ' << c[(size_t)i].code << ' ' << c[(size_t)i].kind << '\n';
    return out.str();
}

bool saveControls(const fs::path &file, const Controls &c) { return writeText(file, writeControls(c)); }

std::wstring bindingName(const Binding &binding) {
    const int32_t initialCode = binding.code;
    if (initialCode == 0 && binding.kind == 0)
        return L"Unassigned";
    if (binding.kind == 1) {
        std::wostringstream out;
        const int32_t number = std::bit_cast<int32_t>(uint32_t(binding.code) + 1u);
        out << L"Mouse " << number;
        return out.str();
    }
    // Complete first/detailed constructor dictionary. Missing keys stay unknown;
    // this table is independent of compact menu names and the OS keyboard layout.
    static constexpr std::pair<int32_t, std::wstring_view> names[] = {
        {1, L"Escape"},
        {2, L"1"},
        {3, L"2"},
        {4, L"3"},
        {5, L"4"},
        {6, L"5"},
        {7, L"6"},
        {8, L"7"},
        {9, L"8"},
        {10, L"9"},
        {11, L"0"},
        {12, L"-"},
        {13, L"="},
        {14, L"Backspace"},
        {15, L"Tab"},
        {16, L"Q"},
        {17, L"W"},
        {18, L"E"},
        {19, L"R"},
        {20, L"T"},
        {21, L"Y"},
        {22, L"U"},
        {23, L"I"},
        {24, L"O"},
        {25, L"P"},
        {26, L"["},
        {27, L"]"},
        {28, L"Return"},
        {29, L"Left Control"},
        {30, L"A"},
        {31, L"S"},
        {32, L"D"},
        {33, L"F"},
        {34, L"G"},
        {35, L"H"},
        {36, L"J"},
        {37, L"K"},
        {38, L"L"},
        {39, L";"},
        {40, L"'"},
        {41, L"`"},
        {42, L"Left Shift"},
        {43, L"\\"},
        {44, L"Z"},
        {45, L"X"},
        {46, L"C"},
        {47, L"V"},
        {48, L"B"},
        {49, L"N"},
        {50, L"M"},
        {51, L","},
        {52, L"."},
        {53, L"/"},
        {54, L"Right Shift"},
        {55, L"Numpad *"},
        {56, L"Left Alt"},
        {57, L"Space"},
        {58, L"Caps Lock"},
        {59, L"F1"},
        {60, L"F2"},
        {61, L"F3"},
        {62, L"F4"},
        {63, L"F5"},
        {64, L"F6"},
        {65, L"F7"},
        {66, L"F8"},
        {67, L"F9"},
        {68, L"F10"},
        {69, L"Num Lock"},
        {70, L"Scroll Lock"},
        {71, L"Numpad 7"},
        {72, L"Numpad 8"},
        {73, L"Numpad 9"},
        {74, L"Numpad -"},
        {75, L"Numpad 4"},
        {76, L"Numpad 5"},
        {77, L"Numpad 6"},
        {78, L"Numpad +"},
        {79, L"Numpad 1"},
        {80, L"Numpad 2"},
        {81, L"Numpad 3"},
        {82, L"Numpad 0"},
        {83, L"Numpad ."},
        {86, L"<"},
        {87, L"F11"},
        {88, L"F12"},
        {100, L"F13"},
        {101, L"F14"},
        {102, L"F15"},
        {112, L"Kana"},
        {115, L"C1"},
        {121, L"Convert"},
        {123, L"No Convert"},
        {125, L"Yen"},
        {126, L"C2"},
        {141, L"Numpad ="},
        {144, L"Previous Track"},
        {145, L"@"},
        {146, L":"},
        {147, L"_"},
        {148, L"Kanji"},
        {149, L"Stop"},
        {150, L"AX"},
        {151, L"Unlabeled"},
        {153, L"Next Track"},
        {156, L"Numpad Enter"},
        {157, L"Right Control"},
        {160, L"Mute"},
        {161, L"Calculator"},
        {162, L"Play/Pause"},
        {164, L"Media Stop"},
        {174, L"Volume -"},
        {176, L"Volume +"},
        {178, L"Web Home"},
        {179, L"Numpad ,"},
        {181, L"Numpad /"},
        {183, L"SYSRQ"},
        {184, L"Right Alt"},
        {197, L"Pause"},
        {199, L"Home"},
        {200, L"Up Arrow"},
        {201, L"Page Up"},
        {203, L"Left Arrow"},
        {205, L"Right Arrow"},
        {207, L"End"},
        {208, L"Down Arrow"},
        {209, L"Page Down"},
        {210, L"Insert"},
        {211, L"Delete"},
        {219, L"Windows Left"},
        {220, L"Windows Right"},
        {221, L"App Menu"},
        {222, L"System Power"},
        {223, L"System Sleep"},
        {227, L"System Wake"},
        {229, L"Web Search"},
        {230, L"Web Favorites"},
        {231, L"Web Refresh"},
        {232, L"Web Stop"},
        {233, L"Web Forward"},
        {234, L"Web Back"},
        {235, L"My Computer"},
        {236, L"Mail"},
        {237, L"Media Select"},
    };
    const auto found = std::lower_bound(std::begin(names), std::end(names), initialCode,
        [](const auto &entry, int32_t code) { return entry.first < code; });
    return found != std::end(names) && found->first == initialCode ? std::wstring(found->second) : L"Unknown";
}

std::wstring compactBindingName(const Binding &b) {
    const int32_t key = b.code;
    if (key == 0 && b.kind == 0)
        return L"?";
    if (b.kind == 1) {
        // Construct the provider before rereading the key. Native addition wraps
        // before signed integer insertion; the host stream supplies its locale.
        std::wostringstream out;
        const int32_t number = std::bit_cast<int32_t>(uint32_t(b.code) + 1u);
        out << L"M" << number;
        return out.str();
    }
    // Complete compact constructor population, not the detailed controls-screen
    // names or an inferred OS layout. Sorted immutable storage avoids tree setup.
    static constexpr std::pair<int32_t, std::wstring_view> names[] = {
        {1, L"ESC"},
        {2, L"1"},
        {3, L"2"},
        {4, L"3"},
        {5, L"4"},
        {6, L"5"},
        {7, L"6"},
        {8, L"7"},
        {9, L"8"},
        {10, L"9"},
        {11, L"0"},
        {12, L"-"},
        {13, L"="},
        {14, L"BS"},
        {15, L"TAB"},
        {16, L"Q"},
        {17, L"W"},
        {18, L"E"},
        {19, L"R"},
        {20, L"T"},
        {21, L"Y"},
        {22, L"U"},
        {23, L"I"},
        {24, L"O"},
        {25, L"P"},
        {26, L"["},
        {27, L"]"},
        {28, L"RET"},
        {29, L"LC"},
        {30, L"A"},
        {31, L"S"},
        {32, L"D"},
        {33, L"F"},
        {34, L"G"},
        {35, L"H"},
        {36, L"J"},
        {37, L"K"},
        {38, L"L"},
        {39, L";"},
        {40, L"'"},
        {41, L"`"},
        {42, L"LS"},
        {43, L"\\"},
        {44, L"Z"},
        {45, L"X"},
        {46, L"C"},
        {47, L"V"},
        {48, L"B"},
        {49, L"N"},
        {50, L"M"},
        {51, L","},
        {52, L"."},
        {53, L"/"},
        {54, L"RS"},
        {55, L"N*"},
        {56, L"LA"},
        {57, L"SP"},
        {58, L"CL"},
        {59, L"F1"},
        {60, L"F2"},
        {61, L"F3"},
        {62, L"F4"},
        {63, L"F5"},
        {64, L"F6"},
        {65, L"F7"},
        {66, L"F8"},
        {67, L"F9"},
        {68, L"F10"},
        {69, L"NL"},
        {70, L"SL"},
        {71, L"N7"},
        {72, L"N8"},
        {73, L"N9"},
        {74, L"N-"},
        {75, L"N4"},
        {76, L"N5"},
        {77, L"N6"},
        {78, L"N+"},
        {79, L"N1"},
        {80, L"N2"},
        {81, L"N3"},
        {82, L"N0"},
        {83, L"N."},
        {86, L"<"},
        {87, L"F11"},
        {88, L"F12"},
        {100, L"F13"},
        {101, L"F14"},
        {102, L"F15"},
        {112, L"KN"},
        {115, L"C1"},
        {121, L"CON"},
        {123, L"NCON"},
        {125, L"YEN"},
        {126, L"C2"},
        {141, L"N="},
        {144, L"PT"},
        {145, L"@"},
        {146, L":"},
        {147, L"_"},
        {148, L"KJ"},
        {149, L"ST"},
        {150, L"AX"},
        {151, L"UL"},
        {153, L"NT"},
        {156, L"NE"},
        {157, L"RC"},
        {160, L"MUT"},
        {161, L"CAL"},
        {162, L"PP"},
        {164, L"MS"},
        {174, L"V-"},
        {176, L"V+"},
        {178, L"WH"},
        {179, L"N,"},
        {181, L"N/"},
        {183, L"SRQ"},
        {184, L"RA"},
        {197, L"PAU"},
        {199, L"HOM"},
        {200, L"UA"},
        {201, L"PGU"},
        {203, L"LA"},
        {205, L"RA"},
        {207, L"END"},
        {208, L"DA"},
        {209, L"PGD"},
        {210, L"INS"},
        {211, L"DEL"},
        {219, L"WL"},
        {220, L"WR"},
        {221, L"AM"},
        {222, L"SP"},
        {223, L"SS"},
        {227, L"SW"},
        {229, L"WS"},
        {230, L"WF"},
        {231, L"WR"},
        {232, L"WST"},
        {233, L"WF"},
        {234, L"WB"},
        {235, L"MC"},
        {236, L"MA"},
        {237, L"SEL"},
    };
    const auto found = std::lower_bound(std::begin(names), std::end(names), key,
        [](const auto &entry, int32_t code) { return entry.first < code; });
    return found != std::end(names) && found->first == key ? std::wstring(found->second) : L"?";
}

void loadAndComplete(const fs::path &folder, Options &options, Controls &controls) {
    options = Options{};
    controls = defaultControls();
    loadOptions(folder / "options.cfg", options);
    loadControls(folder / "controls.cfg", controls);
    saveOptions(folder / "options.cfg", options);
    // The constructor persists the current aggregate even for an existing file.
    saveControls(folder / "controls.cfg", controls);
}

} // namespace gameoptions
