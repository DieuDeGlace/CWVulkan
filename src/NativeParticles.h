#pragma once
// Ordinary burst styles1/2/5, from docs/specs/steam-particle-bursts.md.
// Owner geometry and rendering are separate from this fixed-coordinate simulation.
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <vector>
#include <glm/glm.hpp>

namespace nativeparticles {
using Burst = std::array<uint8_t, 0x48>;
using Random = std::function<int()>;
using BlockType = std::function<uint8_t(int32_t, int32_t, int32_t)>;

struct Particle {
    int32_t style = 0, age = 0;
    std::array<int64_t, 3> position{};
    glm::vec3 angles{0}, velocity{0}, spin{0};
    glm::vec4 colour{1};
    glm::mat4 extra{1};
    // The body producer initializes this second tuple; ordinary bursts do not.
    std::optional<std::array<int64_t, 3>> birthPosition;
    float size = 0;
    bool collision = false, frozen = false;
    int32_t lifetime = 4000;
};

bool supportedParticle(const Particle &);

struct BodyPart {
    glm::ivec3 dimensions{0};
    std::span<const uint8_t> rgb;
    glm::mat4 transform{1}; // prepared corner-based part matrix, before engine Z shift
    glm::vec4 tint{1};
    std::array<int64_t, 2> gameOffset{};
};

class System {
  public:
    // Every style expands and consumes its shared draws before the live cap.
    // Motion/draw covers1/2/5 and collision-on body0; other styles retain records.
    void append(std::span<const Burst>, const Random &);
    // One prepared part, in native X/Y/Z sampling and randomized deque order.
    // Body and ordinary particles share the same later cap/update/draw lifetime.
    void prependBody(const BodyPart &, const Random &);
    void update(int32_t milliseconds, const BlockType &);
    const std::vector<Particle> &states() const { return live; }
    void clear() { live.clear(); }
    static void step(Particle &, int32_t milliseconds, const BlockType &);

  private:
    std::vector<Particle> live;
};

// Native corner-based WORLD matrix, full owner allocation dimensions. Offsets are
// fixed scene X/Y translations applied before float conversion; Z has no offset.
glm::mat4 matrix(const Particle &, const std::array<int64_t, 2> &offset, glm::ivec3 dimensions);
} // namespace nativeparticles
