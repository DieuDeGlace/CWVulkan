#pragma once
// The game's 23 controls (docs/specs/frontend-options.md §4.1) as gameplay reads them each frame:
// every binding of controls.cfg is looked up on the keyboard (by scan code) or the mouse (by
// button), and the gamepad's fixed mappings are added on top.
//
//   GameInput controls;
//   controls.update(window, gameControls, pressedKeys, pressedButtons, !menuOpen);
//   if (controls.held(gameoptions::UP)) ...
//   if (controls.pressed(gameoptions::INVENTORY)) ...
#include "GameOptions.h"

#include <array>
#include <cstdint>
#include <vector>

struct GLFWwindow;

// XInput user zero, interpreted using remaining-runtime-gaps.md §4.
struct GamepadButtons {
    bool dpadUp = false, dpadDown = false, back = false, start = false;
    bool dpadLeft = false, dpadRight = false, a = false, b = false, x = false, y = false;
    bool leftShoulder = false, rightShoulder = false, leftThumb = false, rightThumb = false;
    uint8_t leftTrigger = 0, rightTrigger = 0;
    int16_t leftX = 0, leftY = 0, rightX = 0, rightY = 0;
    bool connected = false;
    int32_t elapsedMs = 0; // Signed timeGetTime difference between successful polls.
};
struct GamepadStick { float x = 0, y = 0; };
GamepadStick gamepadStick(int16_t x, int16_t y);
std::array<bool, gameoptions::BINDINGS> gamepadControls(const GamepadButtons &pad, bool ui = false,
                                                     bool mapMode = false, bool suppressJump = false);
struct GamepadCamera { double yaw = 0, pitch = 0; }; // degrees to subtract
GamepadCamera gamepadCamera(GamepadStick stick, double elapsedMs, float trackingDistance = -1);

class GamepadFrame {
  public:
    void update(const GamepadButtons &pad, bool ui, bool mapMode, bool panelEligible = false);
    void clear(); // Lost foreground/capture: release inputs without clearing the B latch.
    std::array<bool, gameoptions::BINDINGS> held{};
    GamepadStick movement{}, look{};
    bool dismiss = false;
  private:
    bool suppressJump = false;
};

class GameInput {
  public:
    // Reads every binding. `pressedKeys` and `pressedButtons` are the GLFW key and mouse button
    // presses since the last frame, so a tap shorter than a frame still counts as a press.
    // `enabled` gates keyboard/mouse bindings; fixed pad mappings follow their UI rules.
    void update(GLFWwindow *window, const gameoptions::Controls &controls, const std::vector<int> &pressedKeys,
                const std::vector<int> &pressedButtons, bool enabled, bool ui = false,
                bool mapMode = false, bool capturingBinding = false, bool panelEligible = false,
                const GamepadButtons *sample = nullptr);
    static GamepadButtons pollGamepad(GLFWwindow *window, bool capturingBinding);
    static bool physicalBindingDown(GLFWwindow *window, const gameoptions::Controls &controls);
    const GamepadFrame &gamepad() const { return padFrame; }
    // Whether the control is down now, and whether it went down this frame.
    bool held(int action) const { return action >= 0 && action < gameoptions::BINDINGS && down[(size_t)action]; }
    bool pressed(int action) const { return action >= 0 && action < gameoptions::BINDINGS && edge[(size_t)action]; }
    // A GLFW key as a keyboard binding: its scan code as the game reads it ({0, 0} for a key
    // without one).
    static gameoptions::Binding keyBinding(int glfwKey);
    // Whether a GLFW key is the key of some binding (CubeVulkan's own keys give way to them).
    static bool keyIsBound(const gameoptions::Controls &controls, int glfwKey);
    // Whether a press of GLFW key `glfwKey`, or of GLFW mouse button `button`, is the binding.
    static bool keyMatches(const gameoptions::Binding &b, int glfwKey);
    static bool buttonMatches(const gameoptions::Binding &b, int button);

  private:
    std::array<bool, gameoptions::BINDINGS> down{}, edge{};
    GamepadFrame padFrame;
};
