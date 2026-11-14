#include "NativeParticles.h"
#include "GameMath.h"
#include "SoundRules.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <deque>

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
glm::mat4 product(const glm::mat4 &left, const glm::mat4 &right) {
    glm::mat4 result;
    for (int column = 0; column < 4; ++column)
        for (int row = 0; row < 4; ++row) {
            const float xy = left[0][row] * right[column][0] + left[1][row] * right[column][1];
            const float xyz = xy + left[2][row] * right[column][2];
            result[column][row] = xyz + left[3][row] * right[column][3];
        }
    return result;
}
glm::mat4 rotated(const glm::mat4 &source, float degrees, glm::vec3 axis) {
    constexpr float radians = 0.01745329238474369f;
    const float angle = degrees * radians, c = std::cos(angle), s = std::sin(angle), k = 1 - c;
    glm::mat4 rotation(1);
    rotation[0] = {(axis.x * axis.x) * k + c, (axis.x * axis.y) * k + s * axis.z, (axis.x * axis.z) * k - s * axis.y, 0};
    rotation[1] = {(axis.x * axis.y) * k - s * axis.z, (axis.y * axis.y) * k + c, (axis.y * axis.z) * k + s * axis.x, 0};
    rotation[2] = {(axis.x * axis.z) * k + s * axis.y, (axis.y * axis.z) * k - s * axis.x, (axis.z * axis.z) * k + c, 0};
    return product(source, rotation); // positive unit axes, even for angle0
}
std::array<int64_t, 3> fixedPoint(const glm::mat4 &transform, glm::vec3 point) {
    glm::vec4 projected;
    for (int row = 0; row < 4; ++row) {
        const float xy = point.x * transform[0][row] + point.y * transform[1][row];
        const float xyz = xy + point.z * transform[2][row];
        projected[row] = xyz + transform[3][row];
    }
    const float reciprocal = 1.0f / projected.w;
    std::array<int64_t, 3> result;
    for (int axis = 0; axis < 3; ++axis) {
        const float normalized = projected[axis] * reciprocal;
        result[axis] = int64_t(normalized * 65536.0f);
    }
    return result;
}
} // namespace

bool supportedParticle(const Particle &p) {
    return p.style >= 0 && p.style <= 8;
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

void System::prependBody(const BodyPart &part, const Random &random) {
    const auto dimensions = part.dimensions;
    if (dimensions.x <= 0 || dimensions.y <= 0 || dimensions.z <= 0)
        return;
    const size_t xy = size_t(dimensions.x) * size_t(dimensions.y);
    if (xy > part.rgb.size() / 3 || size_t(dimensions.z) > part.rgb.size() / 3 / xy)
        return; // caller must supply a complete owner allocation
    const auto occupied = [&](glm::ivec3 at) {
        if (glm::any(glm::lessThan(at, glm::ivec3(0))) || glm::any(glm::greaterThanEqual(at, dimensions)))
            return false;
        const size_t index = 3 * ((size_t(at.z) * dimensions.y + at.y) * dimensions.x + at.x);
        const uint8_t r = part.rgb[index], g = part.rgb[index + 1], b = part.rgb[index + 2];
        return (r || g || b) && !(r == 255 && !g && !b) && !(!r && g == 255 && !b) && !(!r && !g && b == 255);
    };
    const std::array<glm::ivec3, 6> neighbors{{{-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}}};
    const auto center = fixedPoint(part.transform, glm::vec3(dimensions) * .5f);
    const auto draw = [&] { return random ? random() : 0; };
    std::deque<Particle> temporary;
    for (int x = 0; x < dimensions.x; ++x)
        for (int y = 0; y < dimensions.y; ++y)
            for (int z = 0; z < dimensions.z; ++z) {
                const glm::ivec3 voxel{x, y, z};
                if (!occupied(voxel))
                    continue;
                bool exposed = false;
                for (const auto &neighbor : neighbors)
                    if (!occupied(voxel + neighbor)) {
                        exposed = true;
                        break;
                    }
                if (!exposed || draw() % 2)
                    continue;
                Particle p;
                p.style = 0;
                p.age = -(draw() % 1000);
                const auto point = fixedPoint(part.transform, glm::vec3(voxel));
                p.position = point;
                for (int axis = 0; axis < 2; ++axis)
                    p.position[axis] = gamemath::subtractFixed(point[axis], part.gameOffset[axis]);
                p.birthPosition = p.position;
                for (int axis = 0; axis < 3; ++axis) {
                    const int64_t difference = gamemath::subtractFixed(point[axis], center[axis]);
                    const int64_t doubled = gamemath::multiplyFixed(difference, 131072) / 65536;
                    p.velocity[axis] = float(doubled) * (1.0f / 65536.0f);
                }
                for (int axis = 2; axis >= 0; --axis) {
                    const float centered = float(draw()) / 32767.0f - .5f;
                    p.spin[axis] = centered * 100.0f;
                }
                const size_t index = 3 * ((size_t(z) * dimensions.y + y) * dimensions.x + x);
                for (int channel = 0; channel < 3; ++channel) {
                    const float color = float(part.rgb[index + channel]) / 255.0f;
                    p.colour[channel] = color * part.tint[channel];
                }
                p.colour.a = part.tint.a * 1.0f;
                p.extra = part.transform;
                p.extra[3].x = p.extra[3].y = p.extra[3].z = 0;
                p.size = 1;
                p.collision = true;
                p.lifetime = 5000;
                if (draw() % 2)
                    temporary.push_front(p);
                else
                    temporary.push_back(p);
            }
    live.insert(live.begin(), temporary.rbegin(), temporary.rend());
}

void System::prependAmbientLeaf(const soundrules::AmbientLeaf &leaf) {
    Particle p;
    p.style = 4;
    p.position = leaf.position;
    p.velocity = {leaf.velocity[0], leaf.velocity[1], leaf.velocity[2]};
    p.spin = {leaf.spin[0], leaf.spin[1], leaf.spin[2]};
    p.colour = {leaf.colour[0], leaf.colour[1], leaf.colour[2], 1};
    p.size = 0.5f;
    p.lifetime = 20000;
    live.insert(live.begin(), p);
}

bool System::step(Particle &p, int32_t milliseconds, const BlockType &block) {
    p.age = std::bit_cast<int32_t>(uint32_t(p.age) + uint32_t(milliseconds));
    if (p.age < 0 || p.frozen || p.style == 3 || !supportedParticle(p))
        return false;
    const float seconds = float(milliseconds) * 0.001f;
    if (p.style == 0 || p.style == 1 || p.style == 2 || p.style == 5 || p.style == 6) {
        const float gravity = seconds * 30.0f;
        p.velocity.z -= gravity;
    }
    if (p.style != 7)
        turn(p, seconds);
    glm::vec3 displacement;
    for (int axis = 0; axis < 3; ++axis)
        displacement[axis] = p.velocity[axis] * seconds;
    const bool descending = p.style == 6 && p.velocity.z < 0;
    if (!p.collision && !descending) {
        for (int axis = 0; axis < 3; ++axis)
            p.position[axis] = gamemath::addFixed(p.position[axis], int64_t(displacement[axis] * 65536.0f));
        turn(p, seconds); // free motion adds its own advance after the style-dependent first turn
        return false;
    }
    const float horizontal = descending ? displacement.y * displacement.y + displacement.x * displacement.x
                                        : displacement.x * displacement.x + displacement.y * displacement.y;
    const float length = std::sqrt(horizontal + displacement.z * displacement.z);
    const int steps = std::min(10, int(length) + 1);
    const float fraction = 1.0f / float(steps);
    std::array<int64_t, 3> delta;
    for (int axis = 0; axis < 3; ++axis)
        delta[axis] = int64_t((displacement[axis] * fraction) * 65536.0f);
    const auto typeAt = [&] {
        return block ? block(gamemath::floorFixedBlock(p.position[0]), gamemath::floorFixedBlock(p.position[1]),
                             gamemath::floorFixedBlock(p.position[2]))
                     : uint8_t(1);
    };
    for (int i = 0; i < steps; ++i) {
        if (descending) {
            for (int axis = 0; axis < 3; ++axis)
                p.position[axis] = gamemath::addFixed(p.position[axis], delta[axis]);
            if (typeAt() != 0)
                return true; // no rollback, freezing or second angular advance
            continue;
        }
        for (int axis = 0; axis < 3; ++axis) {
            p.position[axis] = gamemath::addFixed(p.position[axis], delta[axis]);
            const uint8_t type = typeAt();
            if (type == 0 || type == 2 || (p.style == 4 && (type == 7 || type == 8 || type == 11)))
                continue;
            p.position[axis] = gamemath::subtractFixed(p.position[axis], delta[axis]);
            p.velocity[axis] = 0;
            if (axis == 2)
                p.velocity = p.spin = glm::vec3(0);
        }
    }
    return false;
}

void System::update(int32_t milliseconds, const BlockType &block) {
    if (live.size() > 1000)
        live.resize(1000); // newest tail, after every expansion draw was consumed
    std::array<bool, 1000> retire{};
    for (size_t i = 0; i < live.size(); ++i)
        retire[i] = step(live[i], milliseconds, block);
    size_t index = 0;
    std::erase_if(live, [&](const Particle &p) { return retire[index++] || p.age > p.lifetime; });
}

glm::mat4 matrix(const Particle &p, const std::array<int64_t, 2> &offset, glm::ivec3 dimensions) {
    glm::mat4 result(1);
    result[3] = {float(gamemath::addFixed(p.position[0], offset[0])) * (1.0f / 65536.0f),
                 float(gamemath::addFixed(p.position[1], offset[1])) * (1.0f / 65536.0f), float(p.position[2]) * (1.0f / 65536.0f), 1.0f};
    result = rotated(result, p.angles.z, {0, 0, 1});
    result = rotated(result, p.angles.y, {0, 1, 0});
    result = rotated(result, p.angles.x, {1, 0, 0});
    float size = p.size;
    if (p.style == 7 || p.style == 8) {
        const float ratio = float(p.age) / float(p.lifetime);
        float factor = 1.0f - ratio;
        if (p.style == 7)
            factor *= .03f;
        size = factor * p.size;
    }
    for (int column = 0; column < 3; ++column)
        result[column] *= size;
    result = product(result, p.extra);
    // Full-header centering precedes the engine mesh's X/Y compensation.
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
