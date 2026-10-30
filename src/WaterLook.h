#pragma once
// Original water styling guided by the owner's surface and submerged references.
// Appearance is independent of the Alpha/Steam lighting and all world generation.
#include <glm/glm.hpp>
class Config;
namespace waterlook {
enum class Style { Omega, Native };
struct Settings {
    Style style = Style::Omega;
    bool reflections = true;
    int reflectionSteps = 24;
    float underwaterVisibility = 80;
    void read(const Config &);
};
struct Submerged {
    glm::vec3 horizon, zenith, fog, material;
};
Submerged submerged(float sunlight);
// Preserve the chosen displayed haze across the native Steam grading stage.
glm::vec3 steamShaderColour(glm::vec3 displayColour);
glm::vec3 steamDisplayColour(glm::vec3 shaderColour);
} // namespace waterlook
