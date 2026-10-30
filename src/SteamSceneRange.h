#pragma once
// Native mesh-readiness/triangle-budget range, from steam-scene-lighting.md 2.6/6.
#include "SceneVisibility.h"
#include <functional>
#include <mutex>
#include <optional>

namespace scenerange {
using Point = std::array<int64_t, 3>;
struct Player {
    Point physical{};
    uint8_t ability = 0;
    float abilityMs = 0;
    bool effect13 = false;
    uint8_t blockFlags = 0;
};
struct Frame {
    Point camera{};
    int32_t zoneRadius = 0;
    double elapsedMs = 0;
    Player player;
    // Bounds and camera use the same absolute fixed coordinate system.
    std::function<std::optional<scenevisibility::Bounds>(int64_t x, int64_t y)> zone;
    std::function<bool(const scenevisibility::Bounds &, float radius)> visible;
};
struct Ranges { float scene = 0, models = 0, abilities = 0; };
int32_t triangleCost(size_t opaqueIndices, size_t waterIndices, size_t lavaIndices);
glm::vec3 groundColour(Point physical); // unnormalized native RGB, truncating XY to blocks
std::optional<float> candidate(const Frame &); // unset when an active mesh lacks a native cost

// A worker may finish cache metadata before its new GPU groups become active.
// Geometry readiness is deliberately supplied separately by the frame owner.
class CachePublication {
  public:
    void publish(const scenevisibility::Bounds &bounds) {
        std::lock_guard lock(mutex);
        latest = bounds;
    }
    std::optional<scenevisibility::Bounds> read() const {
        std::lock_guard lock(mutex);
        return latest;
    }
  private:
    mutable std::mutex mutex;
    std::optional<scenevisibility::Bounds> latest;
};

class State {
  public:
    bool update(const Frame &);
    void reset() { ranges.scene = 0; } // native reset leaves derived fields until the next scene call
    const Ranges &value() const { return ranges; }
    float waterOpacity() const { return opacity; }
    glm::vec3 flaggedColour() const { return colour; }
  private:
    Ranges ranges;
    float opacity = 1; // native constructor; scene reset leaves this field alone
    glm::vec3 colour{0};
};
} // namespace scenerange
