#pragma once
// Steam scene-light targets/state, independently implemented from
// docs/specs/steam-scene-lighting.md §2. Separate from sky colors and shadows.
#include "SteamSky.h"
#include <optional>

namespace steamlighting {
struct Environment {
    steamsky::Region region;
    bool blockFlag20 = false;
    int structureKind = 0;
    std::optional<float> effect13RemainingMs;
};
struct Colours {
    glm::vec4 front, back, ambient;
};
// Native renderer input is a capped wall-clock double, then rounded to float.
float frameMilliseconds(double elapsedMs);
float dayCoordinate(float timeMs);
float collectorDaylight(float timeMs, float forest);
glm::vec3 sunDirection(float timeMs); // frame direction; upload normalizes it again
Colours targets(float timeMs, const Environment &);
glm::vec4 material(bool underwater);

// Shared WORLD fog binding, separate from the adaptive scene range. The native
// initial GPU value is unproved; value() defines an engine zero-scale fallback.
class FogScale {
  public:
    bool setDistance(float distance);
    float value() const { return boundScale.value_or(0); }
    bool bound() const { return boundScale.has_value(); }

  private:
    std::optional<float> boundScale;
};

class EasedColours {
  public:
    EasedColours() { reset(); }
    void reset(); // native constructor values, including ambient alpha1
    void update(const Colours &target, float elapsedMs);
    const Colours &value() const { return current; }

  private:
    Colours current;
};
} // namespace steamlighting
