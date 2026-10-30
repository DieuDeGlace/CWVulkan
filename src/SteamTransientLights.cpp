#include "SteamTransientLights.h"
#include "NativeRules.h"
#include <algorithm>
#include <cstring>

namespace steampoints {
glm::vec4 spiritGlow(const uint8_t *item) {
    if (!item) return glm::vec4(0);
    glm::vec4 sum(0);
    int included = 0;
    // A valid 0xa0-byte item holds at most 32 four-byte spirit entries.
    const int count = std::min<int>(item[0x9d], 32);
    for (int i = 0; i < count; ++i) {
        const uint8_t material = item[0x20 + 4 * i];
        if (material < 0x80) continue;
        glm::vec4 colour;
        switch (material) {
        case 0x80: colour = {glm::vec3(1,.5f,.1f) * 1.5f,3}; break;
        case 0x81: colour = {glm::vec3(.3f,1,.5f) * 1.5f,3}; break;
        case 0x82: colour = {glm::vec3(.3f,.5f,1) * 1.5f,3}; break;
        case 0x83: colour = {glm::vec3(.8f,.8f,1) * 1.5f,3}; break;
        default: colour = {.5f,.5f,.5f,1}; break;
        }
        sum += colour;
        ++included;
    }
    return included ? sum * (1.f / (float)included) : glm::vec4(0);
}

void TransientLights::hit(const std::array<uint8_t, 0x48> &number, const Lookup &lookup) {
    if (!lookup) return;
    int64_t attackerId, targetId;
    std::memcpy(&attackerId, number.data(), 8);
    std::memcpy(&targetId, number.data() + 8, 8);
    if (!lookup(targetId)) return;
    const uint8_t *attacker = lookup(attackerId);
    if (!attacker) return;
    Point position;
    std::memcpy(position.data(), number.data() + 0x20, sizeof position);
    if (number[0x44]) {
        for (const auto *weapon : nativerules::selectedWeapons(attacker)) {
            const auto colour = spiritGlow(weapon);
            if (colour.w > 0)
                append({position, glm::vec3(colour), 20, 0, 400});
        }
    }
    if (number[0x46])
        append({position, {.3f,.3f,.5f}, 16, 0, 400});
}

void TransientLights::advance(double elapsedMs) {
    const int32_t step = (int32_t)std::min(elapsedMs, 1000.0);
    for (auto &source : sources) {
        uint32_t bits;
        std::memcpy(&bits, &source.elapsedMs, sizeof bits);
        bits += (uint32_t)step;
        std::memcpy(&source.elapsedMs, &bits, sizeof bits);
    }
    std::erase_if(sources, [](const TransientSource &source) { return source.elapsedMs > source.lifetimeMs; });
}
} // namespace steampoints
