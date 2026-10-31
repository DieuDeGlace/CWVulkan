#include "CameraPosition.h"
#include "GameMath.h"
#include "NativeCrt.h"
#include <cmath>

namespace cameraposition {
namespace {
constexpr int64_t BLOCK = 65536, QUARTER = 16384;
terrainray::Cell blockAt(const terrainray::World &world, const Point &point) {
    return world.block(terrainray::blockCoordinate(point[0]), terrainray::blockCoordinate(point[1]), terrainray::blockCoordinate(point[2]));
}
Point corner(const Point &point, int64_t x, int64_t y, int64_t z) {
    return {gamemath::addFixed(point[0], x), gamemath::addFixed(point[1], y), gamemath::addFixed(point[2], z)};
}
int64_t weighted(int64_t old, int64_t desired, int64_t weight) {
    const auto product = gamemath::multiplyFixed(gamemath::subtractFixed(desired, old), weight);
    return gamemath::addFixed(old, product / BLOCK);
}
double boatFactor(double ms) {
    const int count = int(ms * 100.0);
    const double multiplier = double(.05f / 100.0f);
    double result = 0;
    for (int i = 0; i < count; ++i)
        result += ((double(i) + 1) / count - result) * multiplier;
    return result;
}
Point eyeAt(const State &state, const std::array<float, 3> &forward) {
    Point eye;
    for (int axis = 0; axis < 3; ++axis) {
        const float blocks = forward[axis] * state.distance;
        eye[axis] = gamemath::subtractFixed(state.lookAt[axis], int64_t(blocks * float(BLOCK)));
    }
    return eye;
}
} // namespace

Point candidate(Subject &subject, const terrainray::World &world) {
    Point point = subject.physical;
    point[2] = gamemath::subtractFixed(point[2], int64_t(subject.stepOffset * float(BLOCK)));
    float height = 0;
    if (subject.ability == 0x57 || subject.ability == 0x58)
        height = subject.size[0] * .5f;
    else if (subject.ability == 0x78 || !subject.creationLayerShown)
        height = subject.size[2] * .5f + .5f;
    point[2] = gamemath::addFixed(point[2], int64_t(height * float(BLOCK)));
    if (subject.ability == 0x76) {
        if (!subject.mount || !subject.mount->alive) {
            subject.ability = 0;
        } else {
            const float raised = subject.mount->height * .75f + subject.mount->offsetZ;
            point[2] = gamemath::addFixed(point[2], int64_t(raised * float(BLOCK)));
        }
    }
    if (subject.ability == 0x79)
        point[2] = gamemath::addFixed(point[2], 2 * BLOCK);
    for (;;) {
        const auto first = blockAt(world, point);
        if ((first.type & 0xfd) || ((first.flags & 0x40) && !(first.flags & 0x80)))
            break;
        // Native asks the same coordinate twice; unchanged solid terrain does
        // not become an invented upward head sweep.
        if (!(blockAt(world, point).type & 0xfd))
            break;
        point[2] = gamemath::addFixed(point[2], BLOCK);
    }
    for (int64_t x : {-QUARTER, QUARTER})
        for (int64_t y : {-QUARTER, QUARTER})
            for (int64_t z : {-QUARTER, QUARTER})
                if (blockAt(world, corner(point, x, y, z)).type & 0xfd) {
                    point = subject.physical;
                    break; // Only this Z loop ends; subsequent corners use raw P.
                }
    return point;
}

void follow(State &state, const Subject &subject, const Point &desired, double frameMs) {
    const double ms = gamemath::frameMilliseconds(frameMs);
    if (subject.ability == 0x9c) {
        state.lookAt = subject.physical;
    } else if (subject.ability == 0x9d) {
        state.lookAt = subject.physical;
        state.lookAt[2] = gamemath::addFixed(state.lookAt[2], BLOCK);
    } else if (subject.ability == 0x98 || subject.ability == 0x79) {
        const int64_t weight =
            subject.ability == 0x79 ? int64_t(boatFactor(ms) * double(BLOCK)) : int64_t(gamemath::blendFactor(ms, .25f) * float(BLOCK));
        for (int axis = 0; axis < 3; ++axis)
            state.lookAt[axis] = weighted(state.lookAt[axis], desired[axis], weight);
    } else {
        const auto &v = subject.velocity;
        const float speed2 = (v[0] * v[0] + v[1] * v[1]) + v[2] * v[2];
        const float moving = 1.0f - nativecrt::expf(-speed2);
        const float rate = float(double(moving) * .5 + .02500000037252903);
        const int64_t weight = int64_t(gamemath::blendFactor(ms, rate) * float(BLOCK));
        for (int axis = 0; axis < 2; ++axis)
            state.lookAt[axis] = weighted(state.lookAt[axis], desired[axis], weight);
        const float zRate = std::abs(subject.stepOffset) * .005f + .0035f;
        const float factor = gamemath::blendFactor(ms, zRate);
        const float difference = float(gamemath::subtractFixed(desired[2], state.lookAt[2]));
        state.lookAt[2] = gamemath::addFixed(state.lookAt[2], int64_t(difference * factor));
    }
    if (gamemath::fixedDistanceSquared(state.lookAt.data(), subject.physical.data()) > 2500.0f)
        state.lookAt = subject.physical;
}

void update(State &state, Subject &subject, const std::array<float, 3> &forward, double frameMs, const terrainray::World &world) {
    const auto desired = candidate(subject, world);
    follow(state, subject, desired, frameMs);
    state.eye = eyeAt(state, forward);
    if (subject.ability == 0x9c)
        return;
    bool visible = true;
    for (int64_t x : {-QUARTER, QUARTER})
        for (int64_t y : {-QUARTER, QUARTER})
            for (int64_t z : {-QUARTER, QUARTER})
                if (!terrainray::visible(world, corner(state.eye, x, y, z), subject.physical, 200.0f, terrainray::Material::TypeOnly))
                    visible = false;
    if (!visible) {
        const std::array<float, 3> backward{-forward[0], -forward[1], -forward[2]};
        for (int64_t x : {-QUARTER, QUARTER})
            for (int64_t y : {-QUARTER, QUARTER})
                for (int64_t z : {-QUARTER, QUARTER}) {
                    const float distance =
                        terrainray::trace(world, corner(state.lookAt, x, y, z), backward, state.distance, terrainray::Material::TypeOnly);
                    if (distance >= 0)
                        state.distance = distance;
                }
        state.eye = eyeAt(state, forward);
    }
    if (blockAt(world, state.eye).type & 0xfd) {
        state.eye = state.lookAt = subject.physical;
        state.distance = 0;
    }
}
} // namespace cameraposition
