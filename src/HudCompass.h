#pragma once
// Owner-backed HUD compass; steam-hud-model-overlays.md §§1–3/6.
#include "GuiItem.h"

class Assets;
class Renderer;

namespace hudcompass {
std::optional<guiitem::Request> request(uint32_t mesh, glm::ivec3 allocation, glm::ivec2 viewport, float farPlane, float guiScale,
                                        float displayedYawDegrees);
class Compass {
  public:
    void queue(Renderer &, const Assets &, std::vector<guiitem::Request> &, float guiScale, float displayedYawDegrees);

  private:
    bool tried = false;
    uint32_t mesh = 0;
    glm::ivec3 allocation{0};
};
} // namespace hudcompass
