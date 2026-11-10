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
#include <mmsystem.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>

namespace {

constexpr int KEYBOARD = 0, MOUSE = 1;
constexpr int MOUSE_BUTTONS = 4;       // 0 left, 1 right, 2 middle, 3 the fourth

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
    static DWORD previousPoll = timeGetTime();
    const DWORD now = timeGetTime();
    pad.elapsedMs = (int32_t)(now - previousPoll);
    previousPoll = now;
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

GamepadButtons GameInput::pollGamepad() { return readGamepad(); }

void GameInput::update(const physicalinput::Poll &poll, const physicalinput::Levels &levels, const GamepadButtons &pad,
                       bool ui, bool mapMode, bool panelEligible) {
    if (poll.phase != physicalinput::Poll::Normal) {
        padFrame.skipPoll();
        edge = {};
        return;
    }
    padFrame.update(pad, ui, mapMode, panelEligible);
    for (int i = 0; i < gameoptions::BINDINGS; ++i) {
        const uint8_t now = padFrame.held[size_t(i)] ? uint8_t(1) : levels[size_t(i)];
        edge[size_t(i)] = now != 0 && down[size_t(i)] == 0;
        down[size_t(i)] = now;
    }
}

std::array<bool, gameoptions::BINDINGS> gamepadControls(const GamepadButtons &pad, bool ui, bool mapMode, bool suppressJump) {
    std::array<bool, gameoptions::BINDINGS> held{};
    held[gameoptions::ZOOM_IN] = pad.dpadUp;
    held[gameoptions::ZOOM_OUT] = pad.dpadDown;
    held[gameoptions::CRAFTING_MENU] = pad.back;
    held[gameoptions::INVENTORY] = pad.back;
    held[gameoptions::SYSTEM_MENU] = pad.start;
    held[gameoptions::ATTACK] = pad.rightShoulder;
    held[gameoptions::SPECIAL_ATTACK] = pad.rightTrigger > 128;
    if (!ui || mapMode) {
        held[gameoptions::LAMP] = pad.dpadRight;
        held[gameoptions::CALL_PET] = pad.leftThumb;
        held[gameoptions::RING_MENU] = pad.rightThumb;
        held[gameoptions::WORLD_MAP] = pad.dpadLeft;
    }
    if (!ui && !mapMode) {
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

void GamepadFrame::skipPoll() {
    dismiss = false;
}

void GamepadFrame::update(const GamepadButtons &pad, bool ui, bool mapMode, bool panelEligible) {
    held = {};
    dismiss = false;
    if (!pad.connected) {
        movement = {}, look = {};
        return;
    }
    if (!pad.b)
        suppressJump = false;
    else if (ui)
        suppressJump = dismiss = true;
    held = gamepadControls(pad, ui, mapMode, suppressJump);
    if (!ui || mapMode) {
        movement = gamepadStick(pad.leftX, pad.leftY);
        look = gamepadStick(pad.rightX, pad.rightY);
    } else {
        movement = {};
        if (!panelEligible)
            look = gamepadStick(pad.rightX, pad.rightY);
        // Panel scrolling preserves the previous turn pair, including on a neutral sample.
    }
}
