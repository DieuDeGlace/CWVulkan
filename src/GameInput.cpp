// The game's controls read each frame, from docs/specs/frontend-options.md §4.1.
#include "GameInput.h"

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
    for (int joystick = GLFW_JOYSTICK_1; joystick <= GLFW_JOYSTICK_LAST; joystick++) {
        GLFWgamepadstate state;
        if (!glfwJoystickIsGamepad(joystick) || !glfwGetGamepadState(joystick, &state))
            continue;
        pad.dpadUp = pad.dpadUp || state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP] == GLFW_PRESS;
        pad.dpadDown = pad.dpadDown || state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN] == GLFW_PRESS;
        pad.back = pad.back || state.buttons[GLFW_GAMEPAD_BUTTON_BACK] == GLFW_PRESS;
        pad.start = pad.start || state.buttons[GLFW_GAMEPAD_BUTTON_START] == GLFW_PRESS;
    }
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
                       const std::vector<int> &pressedButtons, bool enabled) {
    const std::array<bool, gameoptions::BINDINGS> pad = gamepadControls(readGamepad());
    for (int i = 0; i < gameoptions::BINDINGS; i++) {
        const gameoptions::Binding &b = controls[(size_t)i];
        bool now = enabled && (bindingDown(window, b) || pad[(size_t)i]);
        bool tapped = false;
        if (enabled) {
            for (int key : pressedKeys)
                tapped = tapped || keyMatches(b, key);
            for (int button : pressedButtons)
                tapped = tapped || buttonMatches(b, button);
        }
        edge[(size_t)i] = (now && !down[(size_t)i]) || (tapped && !down[(size_t)i]);
        down[(size_t)i] = now;
    }
}

std::array<bool, gameoptions::BINDINGS> gamepadControls(const GamepadButtons &pad) {
    std::array<bool, gameoptions::BINDINGS> held{};
    held[gameoptions::ZOOM_IN] = pad.dpadUp;
    held[gameoptions::ZOOM_OUT] = pad.dpadDown;
    held[gameoptions::CRAFTING_MENU] = pad.back;
    held[gameoptions::INVENTORY] = pad.back;
    held[gameoptions::SYSTEM_MENU] = pad.start;
    return held;
}
