#include "HairStyle.h"
#include "Config.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdio>

namespace hair {
Style normalised(Style style) {
    style.start = std::isfinite(style.start) ? std::clamp(style.start, 0.0f, 1.0f) : .4f;
    style.blend = std::isfinite(style.blend) ? std::clamp(style.blend, 0.0f, 1.0f) : .35f;
    return style;
}
std::array<uint32_t, 3> cacheKey(Style style) {
    style = normalised(style);
    if (!style.dual)
        return {};
    return {0x1000000u | ((uint32_t)style.second.r << 16) | ((uint32_t)style.second.g << 8) | style.second.b,
            std::bit_cast<uint32_t>(style.start), std::bit_cast<uint32_t>(style.blend)};
}
Style fromConfig(const Config &config, const std::string &name) {
    Style style;
    style.start = (float)config.number("DualHair", "StartPercent", 40, 0, 100) / 100;
    style.blend = (float)config.number("DualHair", "BlendPercent", 35, 0, 100) / 100;
    std::string spec = config.text("Hair", "SecondColor");
    if (spec.empty() && !name.empty())
        spec = config.text("Characters", name);
    if (!config.flag("DualHair", "Enabled", true) || spec.empty())
        return normalised(style);
    int r, g, b, start = -1, blend = -1;
    unsigned hex;
    if (std::sscanf(spec.c_str(), "%d , %d , %d , %d , %d", &r, &g, &b, &start, &blend) >= 3) {
        style.dual = true;
        style.second = glm::u8vec3(std::clamp(r, 0, 255), std::clamp(g, 0, 255), std::clamp(b, 0, 255));
    } else if (std::sscanf(spec.c_str(), " #%6x , %d , %d", &hex, &start, &blend) >= 1) {
        style.dual = true;
        style.second = glm::u8vec3(hex >> 16, (hex >> 8) & 255, hex & 255);
    }
    if (start >= 0 && blend >= 0) {
        style.start = std::min(start, 100) / 100.0f;
        style.blend = std::min(blend, 100) / 100.0f;
    }
    return normalised(style);
}
} // namespace hair
