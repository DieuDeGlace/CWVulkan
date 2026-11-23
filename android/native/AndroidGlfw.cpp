// The part of the GLFW API the engine calls, for Android: one full-screen window (the activity's
// ANativeWindow), its touch screen as the mouse, a hardware or system keyboard, and a game
// controller. Keys carry the scan codes GLFW reports on Windows, so key bindings, options.cfg and
// the DirectInput-style keyboard state read the same on both systems.
#include "AndroidWindow.h"

#include <volk.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <android/log.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>
#include <jni.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <deque>
#include <utility>

struct GLFWwindow {
    bool shouldClose = false;
    double cursorX = 0, cursorY = 0;
    int buttons[8] = {};
    int cursorMode = GLFW_CURSOR_NORMAL;
    GLFWkeyfun key = nullptr;
    GLFWcharfun character = nullptr;
    GLFWmousebuttonfun mouseButton = nullptr;
};
struct GLFWmonitor {};

namespace {
android_app *theApp = nullptr;
GLFWwindow theWindow;
GLFWmonitor theMonitor;
GLFWvidmode theMode{};
bool windowCreated = false;
bool resumed = false, focused = false;
unsigned generation = 0;
androidwindow::MessageHook messageHook = nullptr;
uint8_t keyboard[256] = {};
androidwindow::Gamepad pad;

void post(androidwindow::Message::Kind kind, int value = 0) {
    if (messageHook)
        messageHook({kind, value});
}

// GLFW's key for an Android key code (GLFW_KEY_UNKNOWN for one the game does not read).
int glfwKeyOf(int code) {
    if (code >= AKEYCODE_A && code <= AKEYCODE_Z)
        return GLFW_KEY_A + (code - AKEYCODE_A);
    if (code >= AKEYCODE_0 && code <= AKEYCODE_9)
        return GLFW_KEY_0 + (code - AKEYCODE_0);
    if (code >= AKEYCODE_F1 && code <= AKEYCODE_F12)
        return GLFW_KEY_F1 + (code - AKEYCODE_F1);
    if (code >= AKEYCODE_NUMPAD_0 && code <= AKEYCODE_NUMPAD_9)
        return GLFW_KEY_KP_0 + (code - AKEYCODE_NUMPAD_0);
    switch (code) {
    case AKEYCODE_BACK: case AKEYCODE_ESCAPE: return GLFW_KEY_ESCAPE;
    case AKEYCODE_ENTER: return GLFW_KEY_ENTER;
    case AKEYCODE_NUMPAD_ENTER: return GLFW_KEY_KP_ENTER;
    case AKEYCODE_DEL: return GLFW_KEY_BACKSPACE;
    case AKEYCODE_FORWARD_DEL: return GLFW_KEY_DELETE;
    case AKEYCODE_INSERT: return GLFW_KEY_INSERT;
    case AKEYCODE_SPACE: return GLFW_KEY_SPACE;
    case AKEYCODE_TAB: return GLFW_KEY_TAB;
    case AKEYCODE_SHIFT_LEFT: return GLFW_KEY_LEFT_SHIFT;
    case AKEYCODE_SHIFT_RIGHT: return GLFW_KEY_RIGHT_SHIFT;
    case AKEYCODE_CTRL_LEFT: return GLFW_KEY_LEFT_CONTROL;
    case AKEYCODE_CTRL_RIGHT: return GLFW_KEY_RIGHT_CONTROL;
    case AKEYCODE_ALT_LEFT: return GLFW_KEY_LEFT_ALT;
    case AKEYCODE_ALT_RIGHT: return GLFW_KEY_RIGHT_ALT;
    case AKEYCODE_CAPS_LOCK: return GLFW_KEY_CAPS_LOCK;
    case AKEYCODE_DPAD_LEFT: return GLFW_KEY_LEFT;
    case AKEYCODE_DPAD_RIGHT: return GLFW_KEY_RIGHT;
    case AKEYCODE_DPAD_UP: return GLFW_KEY_UP;
    case AKEYCODE_DPAD_DOWN: return GLFW_KEY_DOWN;
    case AKEYCODE_MOVE_HOME: return GLFW_KEY_HOME;
    case AKEYCODE_MOVE_END: return GLFW_KEY_END;
    case AKEYCODE_PAGE_UP: return GLFW_KEY_PAGE_UP;
    case AKEYCODE_PAGE_DOWN: return GLFW_KEY_PAGE_DOWN;
    case AKEYCODE_MINUS: return GLFW_KEY_MINUS;
    case AKEYCODE_EQUALS: return GLFW_KEY_EQUAL;
    case AKEYCODE_LEFT_BRACKET: return GLFW_KEY_LEFT_BRACKET;
    case AKEYCODE_RIGHT_BRACKET: return GLFW_KEY_RIGHT_BRACKET;
    case AKEYCODE_BACKSLASH: return GLFW_KEY_BACKSLASH;
    case AKEYCODE_SEMICOLON: return GLFW_KEY_SEMICOLON;
    case AKEYCODE_APOSTROPHE: return GLFW_KEY_APOSTROPHE;
    case AKEYCODE_GRAVE: return GLFW_KEY_GRAVE_ACCENT;
    case AKEYCODE_COMMA: return GLFW_KEY_COMMA;
    case AKEYCODE_PERIOD: return GLFW_KEY_PERIOD;
    case AKEYCODE_SLASH: return GLFW_KEY_SLASH;
    case AKEYCODE_NUMPAD_ADD: return GLFW_KEY_KP_ADD;
    case AKEYCODE_NUMPAD_SUBTRACT: return GLFW_KEY_KP_SUBTRACT;
    case AKEYCODE_NUMPAD_MULTIPLY: return GLFW_KEY_KP_MULTIPLY;
    case AKEYCODE_NUMPAD_DIVIDE: return GLFW_KEY_KP_DIVIDE;
    case AKEYCODE_NUMPAD_DOT: return GLFW_KEY_KP_DECIMAL;
    }
    return GLFW_KEY_UNKNOWN;
}

// The scan code GLFW reports for a key on Windows (an extended key's 0xe0 prefix as 0x100).
int scancodeOf(int key) {
    static const char rowQ[] = "QWERTYUIOP", rowA[] = "ASDFGHJKL", rowZ[] = "ZXCVBNM";
    if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9) return 0x02 + (key - GLFW_KEY_1);
    if (key == GLFW_KEY_0) return 0x0B;
    for (int i = 0; rowQ[i]; i++) if (key == rowQ[i]) return 0x10 + i;
    for (int i = 0; rowA[i]; i++) if (key == rowA[i]) return 0x1E + i;
    for (int i = 0; rowZ[i]; i++) if (key == rowZ[i]) return 0x2C + i;
    if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F10) return 0x3B + (key - GLFW_KEY_F1);
    switch (key) {
    case GLFW_KEY_ESCAPE: return 0x01;
    case GLFW_KEY_MINUS: return 0x0C;
    case GLFW_KEY_EQUAL: return 0x0D;
    case GLFW_KEY_BACKSPACE: return 0x0E;
    case GLFW_KEY_TAB: return 0x0F;
    case GLFW_KEY_LEFT_BRACKET: return 0x1A;
    case GLFW_KEY_RIGHT_BRACKET: return 0x1B;
    case GLFW_KEY_ENTER: return 0x1C;
    case GLFW_KEY_LEFT_CONTROL: return 0x1D;
    case GLFW_KEY_SEMICOLON: return 0x27;
    case GLFW_KEY_APOSTROPHE: return 0x28;
    case GLFW_KEY_GRAVE_ACCENT: return 0x29;
    case GLFW_KEY_LEFT_SHIFT: return 0x2A;
    case GLFW_KEY_BACKSLASH: return 0x2B;
    case GLFW_KEY_COMMA: return 0x33;
    case GLFW_KEY_PERIOD: return 0x34;
    case GLFW_KEY_SLASH: return 0x35;
    case GLFW_KEY_RIGHT_SHIFT: return 0x36;
    case GLFW_KEY_KP_MULTIPLY: return 0x37;
    case GLFW_KEY_LEFT_ALT: return 0x38;
    case GLFW_KEY_SPACE: return 0x39;
    case GLFW_KEY_CAPS_LOCK: return 0x3A;
    case GLFW_KEY_PAUSE: return 0x45;
    case GLFW_KEY_SCROLL_LOCK: return 0x46;
    case GLFW_KEY_KP_7: return 0x47;
    case GLFW_KEY_KP_8: return 0x48;
    case GLFW_KEY_KP_9: return 0x49;
    case GLFW_KEY_KP_SUBTRACT: return 0x4A;
    case GLFW_KEY_KP_4: return 0x4B;
    case GLFW_KEY_KP_5: return 0x4C;
    case GLFW_KEY_KP_6: return 0x4D;
    case GLFW_KEY_KP_ADD: return 0x4E;
    case GLFW_KEY_KP_1: return 0x4F;
    case GLFW_KEY_KP_2: return 0x50;
    case GLFW_KEY_KP_3: return 0x51;
    case GLFW_KEY_KP_0: return 0x52;
    case GLFW_KEY_KP_DECIMAL: return 0x53;
    case GLFW_KEY_F11: return 0x57;
    case GLFW_KEY_F12: return 0x58;
    case GLFW_KEY_KP_ENTER: return 0x11C;
    case GLFW_KEY_RIGHT_CONTROL: return 0x11D;
    case GLFW_KEY_KP_DIVIDE: return 0x135;
    case GLFW_KEY_PRINT_SCREEN: return 0x137;
    case GLFW_KEY_RIGHT_ALT: return 0x138;
    case GLFW_KEY_NUM_LOCK: return 0x145;
    case GLFW_KEY_HOME: return 0x147;
    case GLFW_KEY_UP: return 0x148;
    case GLFW_KEY_PAGE_UP: return 0x149;
    case GLFW_KEY_LEFT: return 0x14B;
    case GLFW_KEY_RIGHT: return 0x14D;
    case GLFW_KEY_END: return 0x14F;
    case GLFW_KEY_DOWN: return 0x150;
    case GLFW_KEY_PAGE_DOWN: return 0x151;
    case GLFW_KEY_INSERT: return 0x152;
    case GLFW_KEY_DELETE: return 0x153;
    }
    return -1;
}

// The Windows virtual-key code of a key, for the window procedure's WM_KEYDOWN / WM_KEYUP.
int virtualKeyOf(int key) {
    if ((key >= GLFW_KEY_A && key <= GLFW_KEY_Z) || (key >= GLFW_KEY_0 && key <= GLFW_KEY_9))
        return key; // the same ASCII codes
    if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F12)
        return 0x70 + (key - GLFW_KEY_F1);
    if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_9)
        return 0x60 + (key - GLFW_KEY_KP_0);
    switch (key) {
    case GLFW_KEY_BACKSPACE: return 0x08;
    case GLFW_KEY_TAB: return 0x09;
    case GLFW_KEY_ENTER: case GLFW_KEY_KP_ENTER: return 0x0D;
    case GLFW_KEY_LEFT_SHIFT: case GLFW_KEY_RIGHT_SHIFT: return 0x10;
    case GLFW_KEY_LEFT_CONTROL: case GLFW_KEY_RIGHT_CONTROL: return 0x11;
    case GLFW_KEY_LEFT_ALT: case GLFW_KEY_RIGHT_ALT: return 0x12;
    case GLFW_KEY_CAPS_LOCK: return 0x14;
    case GLFW_KEY_ESCAPE: return 0x1B;
    case GLFW_KEY_SPACE: return 0x20;
    case GLFW_KEY_PAGE_UP: return 0x21;
    case GLFW_KEY_PAGE_DOWN: return 0x22;
    case GLFW_KEY_END: return 0x23;
    case GLFW_KEY_HOME: return 0x24;
    case GLFW_KEY_LEFT: return 0x25;
    case GLFW_KEY_UP: return 0x26;
    case GLFW_KEY_RIGHT: return 0x27;
    case GLFW_KEY_DOWN: return 0x28;
    case GLFW_KEY_INSERT: return 0x2D;
    case GLFW_KEY_DELETE: return 0x2E;
    case GLFW_KEY_KP_MULTIPLY: return 0x6A;
    case GLFW_KEY_KP_ADD: return 0x6B;
    case GLFW_KEY_KP_SUBTRACT: return 0x6D;
    case GLFW_KEY_KP_DECIMAL: return 0x6E;
    case GLFW_KEY_KP_DIVIDE: return 0x6F;
    case GLFW_KEY_SEMICOLON: return 0xBA;
    case GLFW_KEY_EQUAL: return 0xBB;
    case GLFW_KEY_COMMA: return 0xBC;
    case GLFW_KEY_MINUS: return 0xBD;
    case GLFW_KEY_PERIOD: return 0xBE;
    case GLFW_KEY_SLASH: return 0xBF;
    case GLFW_KEY_GRAVE_ACCENT: return 0xC0;
    case GLFW_KEY_LEFT_BRACKET: return 0xDB;
    case GLFW_KEY_BACKSLASH: return 0xDC;
    case GLFW_KEY_RIGHT_BRACKET: return 0xDD;
    case GLFW_KEY_APOSTROPHE: return 0xDE;
    }
    return 0;
}

// The character a key types on a US layout, or 0 (the NDK has no key-character map).
char typedCharacter(int code, bool shift) {
    if (code >= AKEYCODE_A && code <= AKEYCODE_Z)
        return char((shift ? 'A' : 'a') + code - AKEYCODE_A);
    if (code >= AKEYCODE_0 && code <= AKEYCODE_9)
        return shift ? ")!@#$%^&*("[code - AKEYCODE_0] : char('0' + code - AKEYCODE_0);
    switch (code) {
    case AKEYCODE_SPACE: return ' ';
    case AKEYCODE_PERIOD: return shift ? '>' : '.';
    case AKEYCODE_COMMA: return shift ? '<' : ',';
    case AKEYCODE_MINUS: return shift ? '_' : '-';
    case AKEYCODE_EQUALS: return shift ? '+' : '=';
    case AKEYCODE_SLASH: return shift ? '?' : '/';
    case AKEYCODE_BACKSLASH: return shift ? '|' : '\\';
    case AKEYCODE_SEMICOLON: return shift ? ':' : ';';
    case AKEYCODE_APOSTROPHE: return shift ? '"' : '\'';
    case AKEYCODE_LEFT_BRACKET: return shift ? '{' : '[';
    case AKEYCODE_RIGHT_BRACKET: return shift ? '}' : ']';
    case AKEYCODE_GRAVE: return shift ? '~' : '`';
    case AKEYCODE_AT: return '@';
    case AKEYCODE_POUND: return '#';
    case AKEYCODE_STAR: return '*';
    case AKEYCODE_PLUS: return '+';
    }
    return 0;
}

int gameCodeOf(int scancode) { return scancode < 0 ? -1 : (scancode & 0x100) ? ((scancode & 0x7f) | 0x80) : (scancode & 0xff); }

int modifiers() {
    auto held = [](int key) { const int c = gameCodeOf(scancodeOf(key)); return c >= 0 && keyboard[c]; };
    return (held(GLFW_KEY_LEFT_SHIFT) || held(GLFW_KEY_RIGHT_SHIFT) ? GLFW_MOD_SHIFT : 0) |
           (held(GLFW_KEY_LEFT_CONTROL) || held(GLFW_KEY_RIGHT_CONTROL) ? GLFW_MOD_CONTROL : 0) |
           (held(GLFW_KEY_LEFT_ALT) || held(GLFW_KEY_RIGHT_ALT) ? GLFW_MOD_ALT : 0);
}

void keyEvent(int key, bool down, bool repeat) {
    const int scancode = scancodeOf(key), code = gameCodeOf(scancode);
    if (code >= 0)
        keyboard[code] = down ? 0x80 : 0;
    if (const int vk = virtualKeyOf(key))
        post(down ? androidwindow::Message::KeyDown : androidwindow::Message::KeyUp, vk);
    if (theWindow.key)
        theWindow.key(&theWindow, key, scancode, down ? (repeat ? GLFW_REPEAT : GLFW_PRESS) : GLFW_RELEASE, modifiers());
}

void buttonEvent(int button, bool down) {
    if (button < 0 || button >= 8 || (theWindow.buttons[button] != 0) == down)
        return;
    theWindow.buttons[button] = down ? GLFW_PRESS : GLFW_RELEASE;
    using M = androidwindow::Message;
    static const M::Kind downs[3] = {M::LeftDown, M::RightDown, M::MiddleDown}, ups[3] = {M::LeftUp, M::RightUp, M::MiddleUp};
    if (button < 3)
        post(down ? downs[button] : ups[button]);
    if (theWindow.mouseButton)
        theWindow.mouseButton(&theWindow, button, down ? GLFW_PRESS : GLFW_RELEASE, modifiers());
}

// A finger is the mouse. A mouse rests over a button before it clicks, and the interface finds
// the pointed widget once per frame, so a finger's press waits a poll after it arrives and its
// release a poll after the press. A second finger holds the right button (it drags the map).
// While the game holds the cursor (GLFW_CURSOR_DISABLED, the camera), a drag moves it instead.
struct Touch {
    int pointer = -1, second = -1;
    float lastX = 0, lastY = 0;
    struct Pending { int button; bool down; };
    std::deque<Pending> pending; // applied one per poll
} touch;

void releaseEverything() {
    for (int c = 0; c < 256; c++)
        keyboard[c] = 0;
    for (int b = 0; b < 8; b++)
        buttonEvent(b, false);
    touch = {};
    pad = {};
}

int32_t onMotion(const AInputEvent *event) {
    const int32_t source = AInputEvent_getSource(event);
    if ((source & AINPUT_SOURCE_JOYSTICK) == AINPUT_SOURCE_JOYSTICK) {
        auto axis = [&](int a) { return AMotionEvent_getAxisValue(event, a, 0); };
        auto stick = [](float v) { return int16_t(std::clamp(v, -1.0f, 1.0f) * 32767.0f); };
        auto trigger = [](float v) { return uint8_t(std::clamp(v, 0.0f, 1.0f) * 255.0f); };
        pad.connected = true;
        pad.leftX = stick(axis(AMOTION_EVENT_AXIS_X));
        pad.leftY = stick(-axis(AMOTION_EVENT_AXIS_Y)); // XInput's Y points up
        pad.rightX = stick(axis(AMOTION_EVENT_AXIS_Z));
        pad.rightY = stick(-axis(AMOTION_EVENT_AXIS_RZ));
        pad.leftTrigger = trigger(std::max(axis(AMOTION_EVENT_AXIS_LTRIGGER), axis(AMOTION_EVENT_AXIS_BRAKE)));
        pad.rightTrigger = trigger(std::max(axis(AMOTION_EVENT_AXIS_RTRIGGER), axis(AMOTION_EVENT_AXIS_GAS)));
        const float hatX = axis(AMOTION_EVENT_AXIS_HAT_X), hatY = axis(AMOTION_EVENT_AXIS_HAT_Y);
        pad.buttons &= ~0x000F;
        pad.buttons |= (hatY < -0.5f ? 0x1 : 0) | (hatY > 0.5f ? 0x2 : 0) | (hatX < -0.5f ? 0x4 : 0) | (hatX > 0.5f ? 0x8 : 0);
        return 1;
    }
    if (!(source & AINPUT_SOURCE_CLASS_POINTER))
        return 0;
    const int32_t action = AMotionEvent_getAction(event);
    const size_t index = size_t((action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
    const size_t count = AMotionEvent_getPointerCount(event);
    const bool held = theWindow.cursorMode == GLFW_CURSOR_DISABLED;
    switch (action & AMOTION_EVENT_ACTION_MASK) {
    case AMOTION_EVENT_ACTION_DOWN:
    case AMOTION_EVENT_ACTION_POINTER_DOWN: {
        if (index >= count)
            break;
        const int id = AMotionEvent_getPointerId(event, index);
        if (touch.pointer < 0) {
            touch.pointer = id;
            touch.lastX = AMotionEvent_getX(event, index), touch.lastY = AMotionEvent_getY(event, index);
            if (!held) {
                theWindow.cursorX = touch.lastX, theWindow.cursorY = touch.lastY;
                touch.pending.push_back({GLFW_MOUSE_BUTTON_LEFT, true});
            }
        } else if (touch.second < 0) {
            touch.second = id;
            touch.pending.push_back({GLFW_MOUSE_BUTTON_RIGHT, true});
        }
        break;
    }
    case AMOTION_EVENT_ACTION_MOVE:
        for (size_t i = 0; i < count; i++)
            if (AMotionEvent_getPointerId(event, i) == touch.pointer) {
                const float x = AMotionEvent_getX(event, i), y = AMotionEvent_getY(event, i);
                if (held)
                    theWindow.cursorX += x - touch.lastX, theWindow.cursorY += y - touch.lastY;
                else
                    theWindow.cursorX = x, theWindow.cursorY = y;
                touch.lastX = x, touch.lastY = y;
            }
        break;
    case AMOTION_EVENT_ACTION_UP:
    case AMOTION_EVENT_ACTION_POINTER_UP: {
        if (index >= count)
            break;
        const int id = AMotionEvent_getPointerId(event, index);
        if (id == touch.pointer) {
            touch.pointer = -1;
            touch.pending.push_back({GLFW_MOUSE_BUTTON_LEFT, false});
        } else if (id == touch.second) {
            touch.second = -1;
            touch.pending.push_back({GLFW_MOUSE_BUTTON_RIGHT, false});
        }
        break;
    }
    case AMOTION_EVENT_ACTION_CANCEL:
        touch.pending.push_back({GLFW_MOUSE_BUTTON_LEFT, false});
        touch.pending.push_back({GLFW_MOUSE_BUTTON_RIGHT, false});
        touch.pointer = touch.second = -1;
        break;
    case AMOTION_EVENT_ACTION_SCROLL:
        post(androidwindow::Message::Wheel, int(AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_VSCROLL, 0) * 120.0f));
        break;
    }
    return 1;
}

// A controller's buttons in XInput's bits.
uint16_t padBit(int code) {
    switch (code) {
    case AKEYCODE_BUTTON_A: return 0x1000;
    case AKEYCODE_BUTTON_B: return 0x2000;
    case AKEYCODE_BUTTON_X: return 0x4000;
    case AKEYCODE_BUTTON_Y: return 0x8000;
    case AKEYCODE_BUTTON_L1: return 0x0100;
    case AKEYCODE_BUTTON_R1: return 0x0200;
    case AKEYCODE_BUTTON_THUMBL: return 0x0040;
    case AKEYCODE_BUTTON_THUMBR: return 0x0080;
    case AKEYCODE_BUTTON_START: return 0x0010;
    case AKEYCODE_BUTTON_SELECT: return 0x0020;
    }
    return 0;
}

int32_t onKey(const AInputEvent *event) {
    const int code = AKeyEvent_getKeyCode(event), action = AKeyEvent_getAction(event);
    if (action != AKEY_EVENT_ACTION_DOWN && action != AKEY_EVENT_ACTION_UP)
        return 0;
    const bool down = action == AKEY_EVENT_ACTION_DOWN;
    if (const uint16_t bit = padBit(code)) {
        pad.connected = true;
        pad.buttons = down ? uint16_t(pad.buttons | bit) : uint16_t(pad.buttons & ~bit);
        return 1;
    }
    if (code == AKEYCODE_BUTTON_L2 || code == AKEYCODE_BUTTON_R2) {
        (code == AKEYCODE_BUTTON_L2 ? pad.leftTrigger : pad.rightTrigger) = down ? 255 : 0;
        return 1;
    }
    const int key = glfwKeyOf(code);
    if (key == GLFW_KEY_UNKNOWN)
        return code == AKEYCODE_VOLUME_UP || code == AKEYCODE_VOLUME_DOWN ? 0 : 1;
    keyEvent(key, down, down && AKeyEvent_getRepeatCount(event) > 0);
    if (down) {
        const bool shift = (AKeyEvent_getMetaState(event) & AMETA_SHIFT_ON) != 0;
        if (const char c = typedCharacter(code, shift)) {
            post(androidwindow::Message::Character, (unsigned char)c);
            if (theWindow.character)
                theWindow.character(&theWindow, (unsigned char)c);
        } else if (key == GLFW_KEY_ENTER || key == GLFW_KEY_BACKSPACE || key == GLFW_KEY_ESCAPE || key == GLFW_KEY_TAB) {
            post(androidwindow::Message::Character, virtualKeyOf(key)); // Windows sends these as WM_CHAR too
        }
    }
    return 1;
}

int32_t onInput(android_app *, AInputEvent *event) {
    switch (AInputEvent_getType(event)) {
    case AINPUT_EVENT_TYPE_MOTION: return onMotion(event);
    case AINPUT_EVENT_TYPE_KEY: return onKey(event);
    }
    return 0;
}

void onCommand(android_app *app, int32_t command) {
    switch (command) {
    case APP_CMD_INIT_WINDOW:
        if (app->window) {
            generation++;
            theMode.width = ANativeWindow_getWidth(app->window);
            theMode.height = ANativeWindow_getHeight(app->window);
        }
        break;
    case APP_CMD_TERM_WINDOW:
    case APP_CMD_LOST_FOCUS:
        releaseEverything();
        focused = command != APP_CMD_LOST_FOCUS && focused;
        break;
    case APP_CMD_GAINED_FOCUS: focused = true; break;
    case APP_CMD_RESUME: resumed = true; break;
    case APP_CMD_PAUSE: resumed = false; break;
    case APP_CMD_DESTROY: theWindow.shouldClose = true; break;
    }
}

bool visible() { return theApp && theApp->window && resumed; }

// Handles every waiting event; with `wait`, also blocks until the app is visible again.
void pump(bool wait) {
    if (!theApp)
        return;
    for (;;) {
        const bool block = wait && !visible() && !theApp->destroyRequested;
        android_poll_source *source = nullptr;
        const int result = ALooper_pollOnce(block ? -1 : 0, nullptr, nullptr, reinterpret_cast<void **>(&source));
        if (source)
            source->process(theApp, source);
        if (theApp->destroyRequested)
            theWindow.shouldClose = true;
        if (result == ALOOPER_POLL_TIMEOUT || result == ALOOPER_POLL_ERROR) {
            if (!block)
                break;
        }
        if (!source && !block)
            break;
    }
}
} // namespace

namespace androidwindow {
void setMessageHook(MessageHook hook) { messageHook = hook; }
void readKeyboard(uint8_t (&state)[256]) { std::memcpy(state, keyboard, sizeof keyboard); }
Gamepad gamepad() { return pad; }
unsigned windowGeneration() { return generation; }
int virtualKey(int glfwKey) { return virtualKeyOf(glfwKey); }
android_app *app() { return theApp; }
void attach(android_app *app) {
    theApp = app;
    app->onAppCmd = onCommand;
    app->onInputEvent = onInput;
}
std::string filesDirectory() {
    return theApp && theApp->activity->externalDataPath ? theApp->activity->externalDataPath : ".";
}

// Calls a method of CubeVulkanActivity from this thread.
template <class F> auto withActivity(F call) -> decltype(call((JNIEnv *)nullptr, jobject(), jclass())) {
    using Result = decltype(call((JNIEnv *)nullptr, jobject(), jclass()));
    if (!theApp)
        return Result();
    JavaVM *vm = theApp->activity->vm;
    JNIEnv *env = nullptr;
    const bool attach = vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) == JNI_EDETACHED;
    if (attach && vm->AttachCurrentThread(&env, nullptr) != JNI_OK)
        return Result();
    const jobject activity = theApp->activity->clazz;
    Result result = call(env, activity, env->GetObjectClass(activity));
    if (env->ExceptionCheck())
        env->ExceptionClear();
    if (attach)
        vm->DetachCurrentThread();
    return result;
}
bool callWithText(const char *method, const std::string &text) {
    return withActivity([&](JNIEnv *env, jobject activity, jclass type) {
        const jmethodID id = env->GetMethodID(type, method, "(Ljava/lang/String;)V");
        if (!id)
            return false;
        const jstring s = env->NewStringUTF(text.c_str());
        env->CallVoidMethod(activity, id, s);
        env->DeleteLocalRef(s);
        return true;
    });
}
bool shownError = false;
void showError(const std::string &message) { shownError = callWithText("showError", message) || shownError; }
bool errorShown() { return shownError; }
void requestGameFolder(const std::string &message) { callWithText("requestGameFolder", message); }
bool takeImportFinished() {
    return withActivity([](JNIEnv *env, jobject activity, jclass type) {
        const jmethodID id = env->GetMethodID(type, "takeImportFinished", "()Z");
        return id && env->CallBooleanMethod(activity, id);
    });
}
bool waitEvents(int milliseconds) {
    if (!theApp)
        return false;
    android_poll_source *source = nullptr;
    if (ALooper_pollOnce(milliseconds, nullptr, nullptr, reinterpret_cast<void **>(&source)) >= 0 && source)
        source->process(theApp, source);
    return !theApp->destroyRequested;
}

// The system keyboard, through the activity's InputMethodManager.
void showKeyboard(bool show) {
    if (!theApp)
        return;
    JavaVM *vm = theApp->activity->vm;
    JNIEnv *env = nullptr;
    const bool attached = vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) == JNI_EDETACHED;
    if (attached && vm->AttachCurrentThread(&env, nullptr) != JNI_OK)
        return;
    const jobject activity = theApp->activity->clazz;
    const jclass activityClass = env->GetObjectClass(activity);
    const jstring name = env->NewStringUTF("input_method");
    const jobject service = env->CallObjectMethod(
        activity, env->GetMethodID(activityClass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;"), name);
    const jobject window = env->CallObjectMethod(activity, env->GetMethodID(activityClass, "getWindow", "()Landroid/view/Window;"));
    const jobject view = window ? env->CallObjectMethod(window, env->GetMethodID(env->GetObjectClass(window), "getDecorView",
                                                                                  "()Landroid/view/View;"))
                                : nullptr;
    if (service && view) {
        const jclass serviceClass = env->GetObjectClass(service);
        if (show) {
            env->CallBooleanMethod(service, env->GetMethodID(serviceClass, "showSoftInput", "(Landroid/view/View;I)Z"), view, 0);
        } else {
            const jobject token = env->CallObjectMethod(view, env->GetMethodID(env->GetObjectClass(view), "getWindowToken",
                                                                                "()Landroid/os/IBinder;"));
            env->CallBooleanMethod(service, env->GetMethodID(serviceClass, "hideSoftInputFromWindow", "(Landroid/os/IBinder;I)Z"),
                                   token, 0);
        }
    }
    if (env->ExceptionCheck())
        env->ExceptionClear();
    if (attached)
        vm->DetachCurrentThread();
}
} // namespace androidwindow

// --- GLFW ---------------------------------------------------------------------------------------

int glfwInit(void) {
    pump(true); // until the activity has a window to draw in
    return theApp && theApp->window ? GLFW_TRUE : GLFW_FALSE;
}
void glfwTerminate(void) { windowCreated = false; }
void glfwWindowHint(int, int) {}
GLFWwindow *glfwCreateWindow(int, int, const char *, GLFWmonitor *, GLFWwindow *) {
    if (windowCreated || !theApp || !theApp->window)
        return nullptr;
    windowCreated = true;
    theWindow = {};
    theWindow.shouldClose = theApp->destroyRequested != 0;
    return &theWindow;
}
void glfwDestroyWindow(GLFWwindow *) { windowCreated = false; }
int glfwWindowShouldClose(GLFWwindow *w) { return w && w->shouldClose; }
void glfwSetWindowShouldClose(GLFWwindow *w, int value) {
    if (w)
        w->shouldClose = value != 0;
}
void glfwSetWindowTitle(GLFWwindow *, const char *) {}
void glfwGetWindowSize(GLFWwindow *, int *width, int *height) {
    const bool shown = theApp && theApp->window;
    if (width) *width = shown ? ANativeWindow_getWidth(theApp->window) : 0;
    if (height) *height = shown ? ANativeWindow_getHeight(theApp->window) : 0;
}
void glfwGetFramebufferSize(GLFWwindow *w, int *width, int *height) { glfwGetWindowSize(w, width, height); }
GLFWmonitor *glfwGetPrimaryMonitor(void) { return &theMonitor; }
const GLFWvidmode *glfwGetVideoMode(GLFWmonitor *) {
    if (theApp && theApp->window)
        theMode.width = ANativeWindow_getWidth(theApp->window), theMode.height = ANativeWindow_getHeight(theApp->window);
    theMode.redBits = theMode.greenBits = theMode.blueBits = 8;
    theMode.refreshRate = 60;
    return &theMode;
}
const GLFWvidmode *glfwGetVideoModes(GLFWmonitor *monitor, int *count) {
    if (count) *count = 1; // the screen's own mode only
    return glfwGetVideoMode(monitor);
}
void glfwSetWindowMonitor(GLFWwindow *, GLFWmonitor *, int, int, int, int, int) {}
void glfwPollEvents(void) {
    // Release presses whose time has come, one per poll (see Touch).
    if (!touch.pending.empty()) {
        const auto next = touch.pending.front();
        touch.pending.pop_front();
        buttonEvent(next.button, next.down);
    }
    // While the app is off screen the game waits here, as a minimized window waits for messages.
    pump(true);
}
int glfwGetInputMode(GLFWwindow *w, int mode) { return mode == GLFW_CURSOR ? w->cursorMode : GLFW_FALSE; }
void glfwSetInputMode(GLFWwindow *w, int mode, int value) {
    if (mode == GLFW_CURSOR)
        w->cursorMode = value;
}
int glfwRawMouseMotionSupported(void) { return GLFW_FALSE; }
int glfwGetKeyScancode(int key) { return scancodeOf(key); }
int glfwGetMouseButton(GLFWwindow *w, int button) { return button >= 0 && button < 8 ? w->buttons[button] : GLFW_RELEASE; }
void glfwGetCursorPos(GLFWwindow *w, double *x, double *y) {
    if (x) *x = w->cursorX;
    if (y) *y = w->cursorY;
}
void glfwSetCursorPos(GLFWwindow *w, double x, double y) { w->cursorX = x, w->cursorY = y; }
GLFWkeyfun glfwSetKeyCallback(GLFWwindow *w, GLFWkeyfun f) { return std::exchange(w->key, f); }
GLFWcharfun glfwSetCharCallback(GLFWwindow *w, GLFWcharfun f) { return std::exchange(w->character, f); }
GLFWmousebuttonfun glfwSetMouseButtonCallback(GLFWwindow *w, GLFWmousebuttonfun f) { return std::exchange(w->mouseButton, f); }

const char **glfwGetRequiredInstanceExtensions(uint32_t *count) {
    static const char *names[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME};
    *count = 2;
    return names;
}
VkResult glfwCreateWindowSurface(VkInstance instance, GLFWwindow *, const VkAllocationCallbacks *allocator, VkSurfaceKHR *surface) {
    if (!theApp || !theApp->window)
        return VK_ERROR_NATIVE_WINDOW_IN_USE_KHR;
    VkAndroidSurfaceCreateInfoKHR info{VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR};
    info.window = theApp->window;
    return vkCreateAndroidSurfaceKHR(instance, &info, allocator, surface);
}
