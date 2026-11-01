#include "NativeParticles.h"
#include "GameMath.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>

namespace nativeparticles {
namespace {
template <class T> T field(const Burst &record, size_t offset) {
    T value;
    std::memcpy(&value, record.data() + offset, sizeof value);
    return value;
}
void turn(Particle &p, float seconds) {
    for (int axis = 0; axis < 3; ++axis) {
        const float delta = p.spin[axis] * seconds;
        p.angles[axis] += delta;
    }
}
} // namespace

bool supportedStyle(int32_t style) {
    return style == 1 || style == 2 || style == 5;
}

void System::append(std::span<const Burst> bursts, const Random &random) {
    for (const Burst &burst : bursts) {
        const int32_t style = field<int32_t>(burst, 0x3c), count = field<int32_t>(burst, 0x38);
        if (count <= 0)
            continue;
        const float spread = field<float>(burst, 0x40) * 2.0f;
        const auto draw = [&] { return float(random ? random() : 0); };
        for (int32_t i = 0; i < count; ++i) {
            Particle p;
            p.style = style;
            for (int axis = 2; axis >= 0; --axis) {
                float jitter = draw() / 32767.0f;
                if (axis != 2)
                    jitter -= 0.5f;
                const int64_t delta = int64_t(jitter * 65536.0f);
                p.position[axis] = gamemath::addFixed(field<int64_t>(burst, size_t(axis) * 8), delta);
            }
            for (int axis = 2; axis >= 0; --axis) {
                const float centered = draw() / 32767.0f - 0.5f;
                const float jitter = centered * spread;
                p.velocity[axis] = field<float>(burst, 0x18 + size_t(axis) * 4) + jitter;
            }
            for (int axis = 2; axis >= 0; --axis) {
                const float degrees = draw() * 360.0f;
                p.angles[axis] = degrees / 32767.0f;
            }
            for (int axis = 2; axis >= 0; --axis) {
                const float centered = draw() / 32767.0f - 0.5f;
                p.spin[axis] = centered * 100.0f;
            }
            for (int channel = 0; channel < 4; ++channel)
                p.colour[channel] = field<float>(burst, 0x24 + size_t(channel) * 4);
            if (style == 1)
                p.colour = {1.0f, draw() / 32767.0f, 0.0f, 1.0f};
            if (style == 5) {
                p.collision = true;
                p.age = random ? random() % 500 : 0;
            }
            p.size = field<float>(burst, 0x34);
            live.push_back(p);
        }
    }
}

void System::step(Particle &p, int32_t milliseconds, const BlockType &block) {
    p.age = std::bit_cast<int32_t>(uint32_t(p.age) + uint32_t(milliseconds));
    if (p.age < 0 || p.frozen || !supportedStyle(p.style))
        return;
    const float seconds = float(milliseconds) * 0.001f;
    const float gravity = seconds * 30.0f;
    p.velocity.z -= gravity;
    turn(p, seconds);
    glm::vec3 displacement;
    for (int axis = 0; axis < 3; ++axis)
        displacement[axis] = p.velocity[axis] * seconds;
    if (!p.collision) {
        for (int axis = 0; axis < 3; ++axis)
            p.position[axis] = gamemath::addFixed(p.position[axis], int64_t(displacement[axis] * 65536.0f));
        turn(p, seconds); // ordinary collision-off particles really advance twice
        return;
    }
    const float horizontal = displacement.x * displacement.x + displacement.y * displacement.y;
    const float length = std::sqrt(horizontal + displacement.z * displacement.z);
    const int steps = std::min(10, int(length) + 1);
    const float fraction = 1.0f / float(steps);
    std::array<int64_t, 3> delta;
    for (int axis = 0; axis < 3; ++axis)
        delta[axis] = int64_t((displacement[axis] * fraction) * 65536.0f);
    for (int i = 0; i < steps; ++i)
        for (int axis = 0; axis < 3; ++axis) {
            p.position[axis] = gamemath::addFixed(p.position[axis], delta[axis]);
            const uint8_t type = block ? block(gamemath::floorFixedBlock(p.position[0]), gamemath::floorFixedBlock(p.position[1]),
                                               gamemath::floorFixedBlock(p.position[2]))
                                       : 1;
            if (type == 0 || type == 2)
                continue;
            p.position[axis] = gamemath::subtractFixed(p.position[axis], delta[axis]);
            p.velocity[axis] = 0;
            if (axis == 2)
                p.velocity = p.spin = glm::vec3(0);
        }
}

void System::update(int32_t milliseconds, const BlockType &block) {
    if (live.size() > 1000)
        live.resize(1000); // newest tail, after every expansion draw was consumed
    for (Particle &p : live)
        step(p, milliseconds, block);
    std::erase_if(live, [](const Particle &p) { return p.age > p.lifetime; });
}

glm::mat4 matrix(const Particle &p, const std::array<int64_t, 2> &offset, glm::ivec3 dimensions) {
    glm::mat4 result(1);
    result[3] = {float(gamemath::addFixed(p.position[0], offset[0])) * (1.0f / 65536.0f),
                 float(gamemath::addFixed(p.position[1], offset[1])) * (1.0f / 65536.0f), float(p.position[2]) * (1.0f / 65536.0f), 1.0f};
    constexpr float radians = 0.01745329238474369f;
    result = glm::rotate(result, p.angles.z * radians, glm::vec3(0, 0, 1));
    result = glm::rotate(result, p.angles.y * radians, glm::vec3(0, 1, 0));
    result = glm::rotate(result, p.angles.x * radians, glm::vec3(1, 0, 0));
    for (int column = 0; column < 3; ++column)
        result[column] *= p.size;
    // Burst expansion supplies identity for the extra matrix. Keep full-header
    // centering separate from the engine mesh's later X/Y centering compensation.
    const glm::vec3 half = glm::vec3(dimensions) * -0.5f;
    for (int row = 0; row < 4; ++row) {
        const float x = result[0][row] * half.x, y = result[1][row] * half.y, z = result[2][row] * half.z;
        const float xy = row == 0 ? y + x : x + y;
        const float xyz = xy + z;
        result[3][row] += xyz;
    }
    return result;
}
} // namespace nativeparticles
