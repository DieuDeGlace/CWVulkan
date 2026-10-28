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
#include <vector>

struct GLFWwindow;

class GameInput {
  public:
    // Reads every binding. `pressedKeys` and `pressedButtons` are the GLFW key and mouse button
    // presses since the last frame, so a tap shorter than a frame still counts as a press.
    // With `enabled` false (a menu has the keys) every control reads as up.
    void update(GLFWwindow *window, const gameoptions::Controls &controls, const std::vector<int> &pressedKeys,
                const std::vector<int> &pressedButtons, bool enabled);
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
};

// The gamepad's fixed mappings (frontend-options.md §4.1, read in 0x1355a0): which controls the
// d-pad and the Back and Start buttons press. The rest of the pad is open question 5.
struct GamepadButtons {
    bool dpadUp = false, dpadDown = false, back = false, start = false;
};
// The controls those buttons hold: d-pad up Zoom In, d-pad down Zoom Out, Back the Crafting Menu
// and the Inventory, Start the System Menu.
std::array<bool, gameoptions::BINDINGS> gamepadControls(const GamepadButtons &pad);
