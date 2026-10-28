#pragma once
// The Steam release's sky, CubeVulkan's optional look ([Render] Style=steam): the palette by
// time of day and region, its easing from frame to frame, the sun's path and where its glow
// sits on the screen, and the dome of star cubes. Written from docs/specs/sky-and-clouds.md
// part B; the section numbers in the comments point there. The palette's light colours (its
// P7..P9) belong to a lighting spec not written yet, so the Steam look's lights still come
// from DayNight.h.
#include <glm/glm.hpp>
#include <vector>

namespace steamsky {

// §B.3: the region values the palette reads at the camera's column.
struct Region {
    float warmth = .5f;  // W (0x2d9b60)
    float dryness = .5f; // D (0x2b5960): high is green and wet
    float ground = 50;   // H, the ground height in blocks above sea level (0x35ea0, mode 0)
    float lava = 0;      // Lv (0x2b9750)
    float forest = 0;    // Fo (0x2d9f30)
    float kind27 = 0;    // Pw: the place's weight where the place is of kind 27
};

// The four colours the sky is drawn with.
struct Palette {
    glm::vec3 sky1{0}; // lower part of the sky gradient
    glm::vec3 sky2{0}; // upper part
    glm::vec3 fog{0};  // near the horizon and below it
    glm::vec3 sun{0};  // the sun's glow
};

// §B.3: the palette for the palette daylight L (the same curve as Alpha's, AlphaLook.h).
Palette palette(float daylight, const Region &region, bool underwater);

// §B.4: the renderer eases the fog and the sun colour towards each frame's palette; sky1 and
// sky2 follow it at once. The first frame (or a reset) takes the palette as it is.
class EasedPalette {
  public:
    void update(const Palette &target, double elapsedMs);
    void reset() { primed = false; }
    const Palette &value() const { return current; }

  private:
    Palette current;
    bool primed = false;
};

// §B.5: the direction towards the sun: below the ground at midnight, in +x at 06:00, high and
// leaning towards -y at noon.
glm::vec3 sunDirection(double timeOfDayMs);

// §B.5: where the sun's glow sits for the sky shader: (screen x, screen y shifted with the
// camera's tilt, width / height, 1 when the sun is in front of the camera, else 0). `view` and
// `projection` are the camera's (projection with Vulkan's flipped y), `gradientShift` the
// tilt shift (AlphaLook.h's fogGradientTranslation).
glm::vec4 sunPosition(glm::vec3 sunDirection, const glm::mat4 &view, const glm::mat4 &projection, float aspect,
                      float gradientShift);

// §B.6: the star value S' (low at night, lower again over forests) and whether stars show.
float starValue(double timeOfDayMs, float forest);
bool starsShown(float starValue);
// Their opacity: fading in as S' goes from 0.4 down to 0.3.
float starOpacity(float starValue);

// §B.6: one star of the dome: a cube.cub model, placed relative to the dome's centre.
struct Star {
    float turnDegrees; // about the vertical axis
    float tiltDegrees; // up from the horizon, about the x axis
    float scale;       // 1..3 blocks per voxel
};
// The stars of the dome, ring by ring (about half the 1275 places hold one).
std::vector<Star> starDome();
// A star's model matrix for a dome of radius 64 R - 10 blocks centred on the origin.
glm::mat4 starMatrix(const Star &star, int radiusUnits);

} // namespace steamsky
