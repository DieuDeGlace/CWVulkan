#include "DeathEffects.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

void DeathEffects::burst(const std::vector<Piece> &pieces, glm::vec3 center) {
    if (pieces.empty())
        return;
    std::uniform_int_distribution<int> nativeRand(0, 32767);
    std::vector<Fragment> emitted;
    for (const Piece &p : pieces) {
        if (!(p.size > 0) || !std::isfinite(p.size) || !std::isfinite(glm::determinant(p.basis)) ||
            std::abs(glm::determinant(p.basis)) < 1e-9f ||
            !std::isfinite(p.at.x) || !std::isfinite(p.at.y) || !std::isfinite(p.at.z))
            continue;
        // The native burst visits exposed voxels and keeps a random half. It
        // does not enlarge a sampled voxel or add an invented upward impulse.
        if (nativeRand(random) % 2)
            continue;
        Fragment f;
        f.at = p.at;
        f.color = p.color;
        f.basis = p.basis * p.size;
        f.ageMS = -(nativeRand(random) % START_DELAY_MS);
        // Native sprite positions and centres are quantized to 1/65536 block.
        for (int axis = 0; axis < 3; axis++) {
            double at = std::trunc(double(p.at[axis]) * 65536.0);
            double middle = std::trunc(double(center[axis]) * 65536.0);
            f.at[axis] = at / 65536.0;
            f.velocity[axis] = float((at - middle) * 2.0 / 65536.0);
            f.spin[axis] = (nativeRand(random) / 32767.f - .5f) * 100.f;
        }
        emitted.push_back(f);
    }
    // Newly created body particles precede older ones in the native list.
    fragments.insert(fragments.begin(), emitted.rbegin(), emitted.rend());
    if (fragments.size() > MAX_PIECES)
        fragments.resize(MAX_PIECES);
}

void DeathEffects::stepFragment(Fragment &f, int elapsedMS, const Solid &solid) {
    if (elapsedMS <= 0)
        return;
    const float seconds = elapsedMS * .001f;
    f.ageMS += elapsedMS;
    if (f.ageMS < 0 || f.resting || f.ageMS > LIFE_MS)
        return;
    f.velocity.z -= GRAVITY * seconds;
    f.angles += f.spin * seconds;
    glm::vec3 move = f.velocity * seconds;
    int steps = std::min(10, int(glm::length(move)) + 1);
    move /= float(steps);
    for (int step = 0; step < steps; step++) {
        for (int axis = 0; axis < 3; axis++) {
            double shift = std::trunc(move[axis] * 65536.f) / 65536.0;
            f.at[axis] += shift;
            if (!solid || !solid(f.at))
                continue;
            f.at[axis] -= shift;
            f.velocity[axis] = 0;
            if (axis == 2) {
                f.velocity = glm::vec3(0);
                f.spin = glm::vec3(0);
                f.resting = true;
            }
        }
    }
}

void DeathEffects::update(double dt, const Floor &floor, const Solid &solid) {
    if (!std::isfinite(dt) || dt <= 0)
        return;
    fractionMS += std::min(dt, 60.0) * 1000;
    int elapsedMS = (int)std::floor(fractionMS);
    fractionMS -= elapsedMS;
    Solid collision = solid;
    if (!collision && floor)
        collision = [&](glm::dvec3 at) {
            float ground;
            return floor((float)at.x, (float)at.y, (float)at.z + 1, ground) && at.z < ground;
        };
    for (auto &f : fragments)
        stepFragment(f, elapsedMS, collision);
    fragments.erase(std::remove_if(fragments.begin(), fragments.end(),
                                   [](const Fragment &f) { return f.ageMS > LIFE_MS; }),
                    fragments.end());
}

glm::mat3 DeathEffects::fragmentBasis(const Fragment &f) {
    glm::mat4 spin(1);
    spin = glm::rotate(spin, glm::radians(f.angles.z), glm::vec3(0, 0, 1));
    spin = glm::rotate(spin, glm::radians(f.angles.y), glm::vec3(0, 1, 0));
    spin = glm::rotate(spin, glm::radians(f.angles.x), glm::vec3(1, 0, 0));
    // Native draw composes T * Rz * Ry * Rx * original body-part basis.
    // Applying spin on the right instead rotates in the part's local axes.
    return glm::mat3(spin) * f.basis;
}

void DeathEffects::fragmentMesh(Mesh &out) const {
    out.vertices.clear();
    out.indices.clear();
    out.min = glm::vec3(1e30f), out.max = glm::vec3(-1e30f);
    for (const auto &f : fragments) {
        glm::mat3 basis = fragmentBasis(f);
        glm::mat3 normals = glm::transpose(glm::inverse(basis));
        bool reflected = glm::determinant(basis) < 0;
        for (int face = 0; face < 6; face++) {
            uint32_t base = (uint32_t)out.vertices.size();
            glm::vec3 n = glm::normalize(normals * glm::vec3(FACE_NORMALS[face][0], FACE_NORMALS[face][1], FACE_NORMALS[face][2]));
            auto pack = [](float v) { return (uint8_t)std::lround(std::clamp(v * .5f + .5f, 0.f, 1.f) * 255); };
            for (int k = 0; k < 4; k++) {
                const int *c = FACE_CORNERS[face][k];
                glm::vec3 p = glm::vec3(f.at) + basis * (glm::vec3(c[0], c[1], c[2]) - .5f);
                out.vertices.push_back({p.x, p.y, p.z, f.color.r, f.color.g, f.color.b, 255,
                                        pack(n.x), PACKED_NORMAL, pack(n.y), pack(n.z)});
                out.min = glm::min(out.min, p);
                out.max = glm::max(out.max, p);
            }
            if (reflected)
                out.indices.insert(out.indices.end(), {base, base + 2, base + 1, base, base + 3, base + 2});
            else
                out.indices.insert(out.indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
        }
    }
}
