#pragma once
// Sampled levels and the retained Controls editor are independent of window keys.
// Authority: steam-physical-binding-and-controls-capture-producers-20261109.md.
#include "GameOptions.h"
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <glm/vec2.hpp>

struct GLFWwindow;

namespace physicalinput {
using Keyboard = std::array<uint8_t, 256>;
using Mouse = std::array<uint8_t, 16>;
using Levels = std::array<uint8_t, gameoptions::BINDINGS>;

struct Editor {
    gameoptions::Controls values = gameoptions::defaultControls();
    int32_t waiting = -1, candidate = -1, first = 0;
    bool visible = false;
    void bind(gameoptions::Binding); // Ordinary distinct input; all exact duplicates are cleared.
    void release(int32_t button);
    void beginDraw(bool padSelected); // Called after the draw's point provider.
    bool offerRow(int row, float baseline, glm::vec2 localPoint, float width, bool padSelected);
    std::array<int32_t, 2> rows(float height) const;
    static float rowY(int visibleRow) { return 27.f + 30.f * float(visibleRow); }
    static int visibleRows(float height = 600.f);
};

// Each application wrapper fetches its current receiver graph. A leaf record
// writer retains that editor. No lifetime or cross-thread guarantee is implied.
struct Target {
    const gameoptions::Controls *bindings = nullptr;
    Levels *levels = nullptr;
    Editor *editor = nullptr;
    bool *padSelected = nullptr;
};
using Current = std::function<Target()>;

class Poll {
  public:
    enum Phase { Skipped, Capturing, Normal } phase = Skipped;
    using ReadKeyboard = std::function<void(Keyboard &)>;
    using ReadMouse = std::function<void(Mouse &)>;
    // Current output storage persists, including after a partial/failed provider.
    // Defined first-output zeros are engine policy, not a native first-failure fact.
    Keyboard keyboard{};
    Mouse mouse{};
    void sample(bool admitted, const ReadKeyboard &, const ReadMouse &, const Current &);
    void clearZoom(const Current &); // After motion, before XInput, only in Normal.
    void finish(); // After all normal/capture work, before the ordinary frame.
    glm::vec2 relativeMotion() const;
    const Keyboard &previousKeyboard() const { return oldKeyboard; }
    const Mouse &previousMouse() const { return oldMouse; }
  private:
    Keyboard oldKeyboard{};
    Mouse oldMouse{};
};

// Windows provider uses the public OS DirectInput interface, never the game SDK.
// Standard descriptor fields and referenced GUID bytes were independently matched
// to both original headers and all263 records. Imported behavior and hardware
// availability remain provider boundaries; a missing mouse has a logged fallback.
class Devices {
  public:
    Devices();
    ~Devices();
    Devices(const Devices &) = delete;
    Devices &operator=(const Devices &) = delete;
    bool open(GLFWwindow *, std::string &error);
    void close();
    bool foreground() const;
    std::string description() const;
    void readKeyboard(Keyboard &);
    void readMouse(Mouse &);
    static void inactiveWait();
  private:
    struct State;
    std::unique_ptr<State> state;
};
} // namespace physicalinput
