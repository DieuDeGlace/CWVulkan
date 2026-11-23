#pragma once
// What the Android window adds to the GLFW subset of AndroidGlfw.cpp: the window messages the
// Windows build reads from its window procedure, the keyboard as DirectInput reads it, the game
// controller as XInput reads it, and the app's folders.
#include <array>
#include <cstdint>
#include <string>
#include <vector>

struct android_app;

namespace androidwindow {

// The window procedure's messages (main.cpp's interfaceWindowProc on Windows).
struct Message {
    enum Kind { KeyDown, KeyUp, Character, LeftDown, LeftUp, RightDown, RightUp, MiddleDown, MiddleUp, Wheel } kind;
    int value = 0; // a Windows virtual-key code, a UTF-16 unit, or the wheel's delta
};
using MessageHook = void (*)(const Message &);
void setMessageHook(MessageHook hook);

// The keyboard as DirectInput's 256-byte state: 0x80 at each held key's game code (the scan
// code, with 0x80 added for an extended key).
void readKeyboard(uint8_t (&state)[256]);

// The first game controller in XInput's layout; connected is false without one.
struct Gamepad {
    bool connected = false;
    uint16_t buttons = 0; // XINPUT_GAMEPAD_* bits
    uint8_t leftTrigger = 0, rightTrigger = 0;
    int16_t leftX = 0, leftY = 0, rightX = 0, rightY = 0;
};
Gamepad gamepad(); // a connected controller and the on-screen controls together

// The on-screen controls to draw this frame (TouchControls.cpp): discs in screen pixels with the
// opacity of their face and rim, and a label (null for none).
struct TouchShape {
    float x, y, radius, face, rim;
    const wchar_t *label;
};
std::vector<TouchShape> touchShapes();

// Whether a new ANativeWindow has replaced the one the surface was made for: Android takes the
// window away whenever the app leaves the screen. Counts up with each new window.
unsigned windowGeneration();

// The Windows virtual-key code of a GLFW key (MapVirtualKey's answer on Windows), or 0.
int virtualKey(int glfwKey);

// Shows or hides the system keyboard (for a focused text field).
void showKeyboard(bool show);

// The activity's dialogs (CubeVulkanActivity.java). showError's dialog closes the app.
void showError(const std::string &message);
bool errorShown();
void requestGameFolder(const std::string &message);
bool takeImportFinished();
// Handles waiting events for up to milliseconds; false once Android ends the activity.
bool waitEvents(int milliseconds);

// The app's own folder (Android/data/<package>/files): CubeVulkan.ini, the log, Save/ and the
// settings. The owner's game data is looked for in "Cube World" inside it unless the INI says.
std::string filesDirectory();

android_app *app();
void attach(android_app *app); // from android_main, before the game starts

} // namespace androidwindow
