// The game's controls read each frame, from docs/specs/frontend-options.md §4.1.
#include "GameInput.h"
#include "NativeCrt.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <Xinput.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <map>

namespace {

constexpr int KEYBOARD = 0, MOUSE = 1;
constexpr int HIGHEST_SCAN_CODE = 255; // a keyboard binding's code is 1..255
constexpr int MOUSE_BUTTONS = 4;       // 0 left, 1 right, 2 middle, 3 the fourth

// The GLFW key of each scan code the game can bind, made once GLFW knows the keyboard.
const std::map<int, int> &keysByCode() {
    static std::map<int, int> keys;
    static bool made = false;
    if (!made) {
        for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; key++) {
            const gameoptions::Binding b = GameInput::keyBinding(key);
            if (b.code > 0)
                keys.emplace(b.code, key);
        }
        made = !keys.empty();
    }
    return keys;
}

bool bindingDown(GLFWwindow *window, const gameoptions::Binding &b) {
    if (b.kind == MOUSE)
        return b.code >= 0 && b.code < MOUSE_BUTTONS && glfwGetMouseButton(window, b.code) == GLFW_PRESS;
    if (b.kind != KEYBOARD || b.code < 1 || b.code > HIGHEST_SCAN_CODE)
        return false; // (code 0 on the keyboard is unassigned)
    const auto &keys = keysByCode();
    const auto key = keys.find(b.code);
    return key != keys.end() && glfwGetKey(window, key->second) == GLFW_PRESS;
}

GamepadButtons readGamepad() {
    GamepadButtons pad;
    using GetState = DWORD(WINAPI *)(DWORD, XINPUT_STATE *);
    static const GetState getState = [] {
        // Use the OS runtime; never load an XInput DLL from the game directory.
        for (const wchar_t *name : {L"xinput1_4.dll", L"xinput9_1_0.dll", L"xinput1_3.dll"})
            if (HMODULE module = LoadLibraryExW(name, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32))
                if (auto entry = GetProcAddress(module, "XInputGetState"))
                    return reinterpret_cast<GetState>(entry);
        return GetState(nullptr);
    }();
    XINPUT_STATE state{};
    if (!getState || getState(0, &state) != ERROR_SUCCESS)
        return pad;
    pad.connected = true;
    const auto &p = state.Gamepad;
    auto has = [&](WORD mask) { return (p.wButtons & mask) != 0; };
    pad.dpadUp = has(XINPUT_GAMEPAD_DPAD_UP), pad.dpadDown = has(XINPUT_GAMEPAD_DPAD_DOWN);
    pad.dpadLeft = has(XINPUT_GAMEPAD_DPAD_LEFT), pad.dpadRight = has(XINPUT_GAMEPAD_DPAD_RIGHT);
    pad.back = has(XINPUT_GAMEPAD_BACK), pad.start = has(XINPUT_GAMEPAD_START);
    pad.a = has(XINPUT_GAMEPAD_A), pad.b = has(XINPUT_GAMEPAD_B);
    pad.x = has(XINPUT_GAMEPAD_X), pad.y = has(XINPUT_GAMEPAD_Y);
    pad.leftShoulder = has(XINPUT_GAMEPAD_LEFT_SHOULDER), pad.rightShoulder = has(XINPUT_GAMEPAD_RIGHT_SHOULDER);
    pad.leftThumb = has(XINPUT_GAMEPAD_LEFT_THUMB), pad.rightThumb = has(XINPUT_GAMEPAD_RIGHT_THUMB);
    pad.leftTrigger = p.bLeftTrigger, pad.rightTrigger = p.bRightTrigger;
    pad.leftX = p.sThumbLX, pad.leftY = p.sThumbLY, pad.rightX = p.sThumbRX, pad.rightY = p.sThumbRY;
    return pad;
}

} // namespace

gameoptions::Binding GameInput::keyBinding(int glfwKey) {
    const int scancode = glfwGetKeyScancode(glfwKey);
    if (scancode <= 0)
        return {0, KEYBOARD};
    // Windows gives an extended key (the right-hand Ctrl, the arrows) its 0xe0 prefix as 0x100;
    // the game's codes mark them with 0x80.
    constexpr int EXTENDED = 0x100, GAME_EXTENDED = 0x80;
    const int code = (scancode & EXTENDED) ? ((scancode & 0x7f) | GAME_EXTENDED) : (scancode & 0xff);
    return {code, KEYBOARD};
}

bool GameInput::keyMatches(const gameoptions::Binding &b, int glfwKey) {
    return b.kind == KEYBOARD && b.code != 0 && keyBinding(glfwKey) == b;
}

bool GameInput::buttonMatches(const gameoptions::Binding &b, int button) {
    return b.kind == MOUSE && b.code == button && button >= 0 && button < MOUSE_BUTTONS;
}

bool GameInput::keyIsBound(const gameoptions::Controls &controls, int glfwKey) {
    return std::any_of(controls.begin(), controls.end(), [&](const gameoptions::Binding &b) { return keyMatches(b, glfwKey); });
}

void GameInput::update(GLFWwindow *window, const gameoptions::Controls &controls, const std::vector<int> &pressedKeys,
                       const std::vector<int> &pressedButtons, bool enabled, bool ui, bool conversation, bool capturingBinding) {
    const bool focused = glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;
    if (focused && !capturingBinding)
        padFrame.update(readGamepad(), ui, conversation);
    else
        padFrame.clear();
    for (int i = 0; i < gameoptions::BINDINGS; i++) {
        const gameoptions::Binding &b = controls[(size_t)i];
        const bool zoom = i == gameoptions::ZOOM_IN || i == gameoptions::ZOOM_OUT;
        bool now = (focused && enabled && !zoom && bindingDown(window, b)) || padFrame.held[(size_t)i];
        bool tapped = false;
        if (focused && enabled && !zoom) {
            for (int key : pressedKeys)
                tapped = tapped || keyMatches(b, key);
            for (int button : pressedButtons)
                tapped = tapped || buttonMatches(b, button);
        }
        edge[(size_t)i] = (now && !down[(size_t)i]) || (tapped && !down[(size_t)i]);
        down[(size_t)i] = now;
    }
}

std::array<bool, gameoptions::BINDINGS> gamepadControls(const GamepadButtons &pad, bool ui, bool conversation, bool suppressJump) {
    std::array<bool, gameoptions::BINDINGS> held{};
    held[gameoptions::ZOOM_IN] = pad.dpadUp;
    held[gameoptions::ZOOM_OUT] = pad.dpadDown;
    held[gameoptions::CRAFTING_MENU] = pad.back;
    held[gameoptions::INVENTORY] = pad.back;
    held[gameoptions::SYSTEM_MENU] = pad.start;
    held[gameoptions::ATTACK] = pad.rightShoulder;
    held[gameoptions::SPECIAL_ATTACK] = pad.rightTrigger > 128;
    if (!ui || conversation) {
        held[gameoptions::LAMP] = pad.dpadRight;
        held[gameoptions::CALL_PET] = pad.leftThumb;
        held[gameoptions::RING_MENU] = pad.rightThumb;
        held[gameoptions::WORLD_MAP] = pad.dpadLeft;
    }
    if (!ui && !conversation) {
        held[gameoptions::ULTIMATE_SKILL] = pad.y;
        held[gameoptions::JUMP] = pad.b && !suppressJump;
        held[gameoptions::DODGE] = pad.leftShoulder;
        held[gameoptions::CLASS_SKILL] = pad.leftTrigger > 128;
        held[gameoptions::QUICK_ITEM] = pad.x;
        held[gameoptions::USE] = pad.a;
    }
    return held;
}

GamepadStick gamepadStick(int16_t x, int16_t y) {
    const float halfX = (float)x * (1.0f / 65536.0f), halfY = (float)y * (1.0f / 65536.0f);
    GamepadStick stick{halfX + halfX, halfY + halfY};
    if (stick.x * stick.x + stick.y * stick.y < 0.04000000283122063f)
        return {};
    return stick;
}

GamepadCamera gamepadCamera(GamepadStick stick, double elapsedMs, float trackingDistance) {
    float rate = 0.4f;
    if (trackingDistance >= 0)
        rate *= std::max(1.0f - trackingDistance / 50.0f, 0.3f);
    const double ms = std::min(elapsedMs, 1000.0);
    return {(double)(nativecrt::powf(stick.x, 3.0f) * rate) * ms,
            (double)(nativecrt::powf(stick.y, 3.0f) * rate) * ms};
}

void GamepadFrame::clear() {
    held = {}, movement = {}, look = {};
    dismiss = false;
}

void GamepadFrame::update(const GamepadButtons &pad, bool ui, bool conversation) {
    const GamepadStick previousLook = look;
    clear();
    if (!pad.connected)
        return;
    if (!pad.b)
        suppressJump = false;
    else if (ui)
        suppressJump = dismiss = true;
    held = gamepadControls(pad, ui, conversation, suppressJump);
    if (!ui || conversation) {
        movement = gamepadStick(pad.leftX, pad.leftY);
        look = gamepadStick(pad.rightX, pad.rightY);
    } else
        look = previousLook; // The ordinary UI branch does not write a fresh right pair.
}
