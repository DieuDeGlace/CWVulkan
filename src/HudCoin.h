#pragma once
// Queued HUD coin/shadow: hud-money.md §2 and steam-world-pass-controls.md §4.
// Geometry and packed owner colors use the ordinary WORLD shader, as the title logo does.
#include "GuiItem.h"

class Assets;
class Renderer;

namespace hudcoin {
inline constexpr const char *COIN_SPRITE = "gold-coin"; // owner catalog0xae1
float turnDegrees(double worldClockMs);
std::vector<guiitem::Request> coinRequests(uint32_t mesh, glm::ivec3 allocation, float farPlane, float guiScale, float height,
                                           double worldClockMs);

class Coin {
  public:
    // Appends the shadow, then the coin before Plasma; missing owner content draws nothing.
    void queue(Renderer &, const Assets &, std::vector<guiitem::Request> &, float height, float guiScale, double worldClockMs);

  private:
    bool tried = false;
    uint32_t mesh = 0;
    glm::ivec3 allocation{0};
};
} // namespace hudcoin
