#pragma once
#include "SteamPointLights.h"

// Visual responses to consumed hit records, independently specified in
// docs/specs/steam-transient-light-producers.md. These do not apply damage.
namespace steampoints {
glm::vec4 spiritGlow(const uint8_t *item);

struct DeathCandidate {
    uintptr_t identity = 0; // source actor identity, ordered as an unsigned native pointer
    const uint8_t *record = nullptr; // valid throughout this synchronous batch
};

class TransientLights {
public:
    using Lookup = std::function<const uint8_t *(int64_t)>;
    void hit(const std::array<uint8_t, 0x48> &number, const Lookup &lookup);
    void deaths(std::vector<DeathCandidate>, const std::function<bool(const Point &, float, float)> &visible);
    void append(TransientSource source) { sources.push_back(std::move(source)); }
    // Called once per wall frame, after births and before scene collection.
    void advance(double elapsedMs);
    const std::vector<TransientSource> &values() const { return sources; }
private:
    std::vector<TransientSource> sources;
};
} // namespace steampoints
