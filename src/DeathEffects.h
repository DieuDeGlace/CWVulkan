#pragma once
// How Steam Cube World shows a monster dying, as recorded from the game with the
// extractor's death probe (docs/progression.md, "Deaths and coins"):
//   * at 0 HP the body is gone at once and bursts into its own voxels, which fly
//     out, fall and lie on the ground;
//   * the game removes the creature about a second later (969-1031 ms measured);
//   * (coins are ground items that hop in place, docs/specs/loot-pickup.md; nothing flies.)
// Particle rules checked against the owner's supported Steam build and live
// read-only measurements. See docs/progression.md for provenance and limits.
#include "Mesher.h"
#include <functional>
#include <random>
#include <vector>

class DeathEffects {
  public:
    using Floor = std::function<bool(float x, float y, float z, float &height)>;
    using Solid = std::function<bool(glm::dvec3 point)>;
    // An exposed voxel of the body as it was drawn at the moment of death.
    struct Piece {
        glm::vec3 at;      // native voxel origin, used as the particle anchor
        glm::u8vec3 color;
        float size = 1;    // edge length in blocks
        glm::mat3 basis{1}; // original body-part orientation, relative to size
    };
    // One body part bursts: a random half flies away from that part's centre.
    void burst(const std::vector<Piece> &pieces, glm::vec3 center);
    // Moves the fragments.
    void update(double dt, const Floor &floor, const Solid &solid = {});
    // The fragments as one mesh in world space (empty when there are none).
    void fragmentMesh(Mesh &out) const;
    size_t fragmentCount() const { return fragments.size(); }

    static constexpr int MAX_PIECES = 1000; // native particle-list limit, across deaths
    static constexpr float GRAVITY = 30;   // blocks/s^2
    static constexpr int LIFE_MS = 5000;
    static constexpr int START_DELAY_MS = 1000; // native initial age = -(rand() % 1000)
    // Decoded state also allows comparisons with RecordDeathParticles exports.
    struct Fragment {
        glm::dvec3 at;
        glm::vec3 velocity, angles{0}, spin;
        glm::mat3 basis{1};
        glm::u8vec3 color;
        int64_t ageMS = 0;
        bool resting = false;
    };
    const std::vector<Fragment> &fragmentStates() const { return fragments; }
    static void stepFragment(Fragment &fragment, int elapsedMS, const Solid &solid);
    static glm::mat3 fragmentBasis(const Fragment &fragment);

  private:
    std::vector<Fragment> fragments;
    std::mt19937 random{20261021};
    double fractionMS = 0;
};
