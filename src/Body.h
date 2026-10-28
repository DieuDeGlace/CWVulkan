#pragma once
// The player's body against the world's blocks, as the game handles it: a square
// footprint half its hitbox scale either side of the centre (0.96 wide for every
// recorded player). In the game a one-block step is climbed exactly when the body's
// edge, 0.475-0.483 from the centre, reaches it (measured 2026-10-22 with the
// extractor's collision log, docs/movement.md). Walls stop that edge, and any part
// of the footprint over a floor holds the body up. The body's height is not
// measured yet: walls are tested at two heights above step height, as before.
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <functional>

// Walking speed as the game changes it (collision log, 2026-10-22, a level-1 human
// warrior; three identical starts): with a direction held, dv/dt = 82.1 - 2.615 v
// along it, capped at 11.57 blocks/s (RMS 0.0035 over 11 ticks); with none, the
// speed decays at 5.31/s (x0.9204 a tick). The
// game steps this in ticks of about 1/64 s, so update() does too.
struct Walk {
    static constexpr float MAX_SPEED = 11.57f, ACCEL = 82.1f, DRAG_MOVING = 2.615f, DRAG_IDLE = 5.31f;
    static constexpr double TICK = 1 / 64.0;
    glm::vec2 velocity{0};
    // `direction`: unit length, or zero for no input. `maxSpeed` for other gaits.
    void update(glm::vec2 direction, double dt, float maxSpeed = MAX_SPEED) {
        while (dt > 1e-9) {
            float step = (float)std::min(dt, TICK);
            dt -= step;
            if (direction != glm::vec2(0)) {
                velocity += (ACCEL * direction - DRAG_MOVING * velocity) * step;
                float speed = glm::length(velocity);
                if (speed > maxSpeed)
                    velocity *= maxSpeed / speed;
            } else
                velocity *= std::exp(-DRAG_IDLE * step);
        }
    }
};

struct Body {
    float halfWidth = .48f;
    explicit Body(float hitboxScale = .96f) : halfWidth(std::clamp(hitboxScale, .1f, 4.f) * .5f) {}

    // Centre and four corners of the footprint.
    template <class F> void footprint(F &&f) const {
        f(0.f, 0.f);
        for (float dx : {-halfWidth, halfWidth})
            for (float dy : {-halfWidth, halfWidth})
                f(dx, dy);
    }
    // Would the body (above step height) be inside a block with its feet at (x, y, z)?
    bool blocked(const std::function<bool(float, float, float)> &solid, float x, float y, float z) const {
        bool hit = false;
        footprint([&](float dx, float dy) {
            hit = hit || solid(x + dx, y + dy, z + 1.2f) || solid(x + dx, y + dy, z + 1.9f);
        });
        return hit;
    }
    // The highest floor under any part of the footprint, from height `fromZ` down
    // (`floor` gives one column's floor); `fallback` when none is known.
    float floorUnder(const std::function<bool(float, float, float, float &)> &floor, float x, float y, float fromZ,
                     float fallback) const {
        float best = -1e9f;
        footprint([&](float dx, float dy) {
            float h;
            if (floor(x + dx, y + dy, fromZ, h))
                best = std::max(best, h);
        });
        return best > -1e9f ? best : fallback;
    }
};
