#pragma once
// Optional engine appearance data; Steam's primary-color character record stays unchanged.
#include <glm/gtc/type_precision.hpp>
#include <array>
#include <cstdint>
#include <string>

class Config;
namespace hair {
struct Style {
    bool dual = false;
    glm::u8vec3 second{255};
    float start = .4f, blend = .35f;
    bool operator==(const Style &) const = default;
};
Style normalised(Style);
std::array<uint32_t, 3> cacheKey(Style);
Style fromConfig(const Config &, const std::string &characterName);
} // namespace hair
