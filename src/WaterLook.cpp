#include "WaterLook.h"
#include "Config.h"
#include "Palette.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <glm/gtc/constants.hpp>
namespace waterlook {
void Settings::read(const Config &config) {
    auto styleName = config.text("Water", "Style", "Omega");
    std::transform(styleName.begin(), styleName.end(), styleName.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    style = Style::Omega;
    if (styleName == "native")
        style = Style::Native;
    else if (styleName == "revised")
        style = Style::Revised;
    reflections = config.flag("Water", "Reflections", true);
    reflectionSteps = (int)config.number("Water", "ReflectionSteps", 24, 8, 48);
    underwaterVisibility = (float)config.number("Water", "UnderwaterVisibility", 80, 8, 256);
}
float Settings::visibility() const {
    return style == Style::Revised ? std::min(underwaterVisibility, 48.f) : underwaterVisibility;
}
glm::vec2 waveOffsets(glm::dvec2 origin, double seconds) {
    const double turn = glm::two_pi<double>();
    return {std::remainder(.65 * origin.x + .31 * origin.y + .80 * seconds, turn),
            std::remainder(-.27 * origin.x + .72 * origin.y - .65 * seconds, turn)};
}
Submerged submerged(float sunlight, Style style) {
    const float brightness = .2f + .8f * std::clamp(sunlight, 0.f, 1.f);
    if (style == Style::Revised)
        return {glm::vec3(.02f, .45f, .82f) * brightness, glm::vec3(0, .35f, .72f) * brightness,
                glm::vec3(0, .42f, .79f) * brightness, glm::vec3(.38f, .82f, 1.f)};
    return {glm::vec3(.01f, .08f, .48f) * brightness, glm::vec3(.005f, .025f, .20f) * brightness, glm::vec3(.01f, .07f, .43f) * brightness,
            glm::vec3(.38f, .82f, 1.f)};
}
glm::vec3 steamShaderColour(glm::vec3 displayColour) {
    return palette::albedo(displayColour * 255.f, false) / 255.f;
}
glm::vec3 steamDisplayColour(glm::vec3 shaderColour) {
    auto colour = glm::clamp(shaderColour, 0.f, 1.f);
    const float grey = glm::dot(colour, glm::vec3(.299f, .587f, .114f)) * .35f;
    colour.g = colour.g * .65f + grey;
    colour.b = colour.b * .65f + grey;
    return colour * colour * (3.f - 2.f * colour);
}
} // namespace waterlook
