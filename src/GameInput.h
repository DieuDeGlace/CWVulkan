#pragma once
// The game's 23 controls (docs/specs/frontend-options.md §4.1) as gameplay reads them each frame:
// every binding of controls.cfg is looked up on the keyboard (by scan code) or the mouse (by
// button), and the gamepad's fixed mappings are added on top.
//
//   GameInput controls;
//   controls.update(poll, levels, pad);
//   if (controls.held(gameoptions::UP)) ...
//   if (controls.pressed(gameoptions::INVENTORY)) ...
#include "GameOptions.h"
#include "PhysicalInput.h"

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
    void skipPoll(); // Foreground/capture bypass retains sampled controls, both stick pairs and the B latch.
    std::array<bool, gameoptions::BINDINGS> held{};
    GamepadStick movement{}, look{};
    bool dismiss = false;
  private:
    bool suppressJump = false;
};

class GameInput {
  public:
    // The application already sampled devices before motion/XInput. Consumers see
    // final levels, never fabricated pulses from short window-message taps.
    void update(const physicalinput::Poll &, const physicalinput::Levels &, const GamepadButtons &,
                bool ui = false, bool mapMode = false, bool panelEligible = false);
    // Publish admitted physical bytes before synchronous pointer callbacks.
    // update() later merges the pad while preserving the previous completed
    // sample for compatibility edges. Standalone update() remains supported.
    void publishPhysical(const physicalinput::Poll &, const physicalinput::Levels &);
    static GamepadButtons pollGamepad();
    const GamepadFrame &gamepad() const { return padFrame; }
    // Whether the control is down now, and whether it went down this frame.
    bool held(int action) const { return action >= 0 && action < gameoptions::BINDINGS && down[(size_t)action]; }
    bool pressed(int action) const { return action >= 0 && action < gameoptions::BINDINGS && edge[(size_t)action]; }
    uint8_t level(int action) const { return action >= 0 && action < gameoptions::BINDINGS ? down[(size_t)action] : 0; }
    // A GLFW key as a keyboard binding: its scan code as the game reads it ({0, 0} for a key
    // without one).
    static gameoptions::Binding keyBinding(int glfwKey);
    // Whether a GLFW key is the key of some binding (CubeVulkan's own keys give way to them).
    static bool keyIsBound(const gameoptions::Controls &controls, int glfwKey);
    // Whether a press of GLFW key `glfwKey`, or of GLFW mouse button `button`, is the binding.
    static bool keyMatches(const gameoptions::Binding &b, int glfwKey);
    static bool buttonMatches(const gameoptions::Binding &b, int button);

  private:
    std::array<uint8_t, gameoptions::BINDINGS> down{};
    std::array<bool, gameoptions::BINDINGS> edge{}; // Compatibility consumers; Native owners retain their own latches.
    std::array<uint8_t, gameoptions::BINDINGS> beforePhysical{};
    bool physicalPublished = false;
    GamepadFrame padFrame;
};
