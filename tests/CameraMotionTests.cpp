// Independent gameplay-camera §2, §4 and §7.2, including actual queued Audio consumers.
#include "Audio.h"
#include "CameraMotion.h"
#include "Frontend.h"
#include "SoundRules.h"
#include <cmath>
#include <cstdio>
#include <limits>

int cameraMotionTests() {
    int failures = 0;
    auto check = [&](bool pass, const char *what) {
        if (!pass) {
            std::printf("FAIL Camera motion: %s\n", what);
            ++failures;
        }
    };
    auto close = [](float a, float b) { return std::abs(a - b) < 0.000001f; };
    using namespace soundrules;
    const int64_t player[3] = {int64_t(1) << 60, -(int64_t(1) << 59), 100 * 65536};
    struct Falloff {
        int blocks;
        float candidate, after16ms;
    };
    const Falloff cases[] = {{0, 1.0f, 0.9240515232086182f},
                             {10, 0.9599999785423279f, 0.8870894312858582f},
                             {25, 0.75f, 0.6930386424064636f},
                             {49, 0.03960001468658447f, 0.036592453718185425f},
                             {50, 0.0f, 0.0f},
                             {51, 0.0f, 0.0f}};
    for (const auto &test : cases) {
        const int64_t at[3] = {player[0] + int64_t(test.blocks) * 65536, player[1], player[2]};
        const float d2 = distanceSquared(at, player), candidate = explosionShake(d2);
        check(d2 == float(test.blocks * test.blocks), "fixed distance retains large-origin precision");
        check(candidate == test.candidate, "quadratic explosion falloff matches independent float references");
        check(cameramotion::decayShake(candidate, 16) == test.after16ms, "same-frame decay matches independent float references");
    }
    const int64_t high[3] = {std::numeric_limits<int64_t>::max(), 0, 0};
    const int64_t low[3] = {std::numeric_limits<int64_t>::min(), 0, 0};
    check(distanceSquared(high, low) == 2.3283064365386963e-10f, "fixed subtraction wraps before float conversion");
    const int64_t offAxis[3] = {player[0] + 3 * 65536, player[1] - 4 * 65536, player[2] + 12 * 65536};
    check(distanceSquared(offAxis, player) == 169 && explosionShake(169) == 0.9323999881744385f &&
              cameramotion::decayShake(explosionShake(169), 16) == 0.8615856170654297f,
          "XYZ squared-distance sum and decay");

    Audio audio; // No device, assets or game required.
    audio.setVolumes(0, 0);
    const int64_t listener[3] = {player[0] + 1000 * 65536, player[1], player[2]};
    audio.setListener(listener);
    Request notice;
    notice.number = soundid::EXPLOSION;
    for (int axis = 0; axis < 3; ++axis)
        notice.position[axis] = player[axis];
    notice.position[0] += 25 * 65536;
    audio.playRequests({makeRequest(notice)}, player);
    check(audio.cameraShake() == 0.75f, "inaudible muted notice raises shake from physical player, not listener");
    notice.number = soundid::EXPLOSION_2;
    notice.position[0] = player[0] + 10 * 65536;
    audio.playRequests({makeRequest(notice), makeRequest(notice)}, player);
    check(close(audio.cameraShake(), 0.96f), "repeated explosions take maximum rather than sum");
    notice.number = soundid::DUST;
    notice.position[0] = player[0];
    audio.playRequests({makeRequest(notice)}, player);
    check(close(audio.cameraShake(), 0.96f), "other sound IDs do not create camera shake");
    audio.setCameraShake(0.99f);
    notice.number = soundid::EXPLOSION;
    notice.position[0] = player[0] + 51 * 65536;
    audio.playRequests({makeRequest(notice)}, player);
    check(audio.cameraShake() == 0.99f, "far explosion does not clear retained shake");
    notice.position[0] = player[0];
    notice.number = soundid::EXPLOSION;
    audio.playRequests({makeRequest(notice)}, player);
    check(audio.cameraShake() == 1.0f, "closer explosion can raise retained shake");

    frontend::OrbitCamera slow, fast;
    slow.targetPitch = fast.targetPitch = 110;
    slow.targetDistance = fast.targetDistance = 12;
    slow.smooth(0.01, 0);
    fast.smooth(0.01, 100);
    check(slow.distance == 8.0003204345703125f && slow.distance == fast.distance, "orbit rate is independent of angle smoothness");
    check(slow.pitch != fast.pitch, "angle option still controls angle smoothing");
    check(cameramotion::distance(8, 12, 16) == 8.245577812194824f, "orbit matches independent 16 ms reference");
    check(cameramotion::distance(8, 12, 0) == 8, "zero elapsed time preserves orbit distance");
    check(cameramotion::distance(8, 12, 2000) == cameramotion::distance(8, 12, 1000), "camera wall interval caps at 1000 ms");
    check(cameramotion::distance(8, 12, 1000) != cameramotion::distance(8, 12, 100), "camera does not use simulation's 100 ms cap");
    check(cameramotion::decayShake(1, 0.01) == 0.9998999834060669f, "shake has independent 0.005 decay");
    // Native pitch is measured from straight up. A/B/C remain defined at the poles,
    // and nonzero pitch/roll must supply a real up column rather than world +Z.
    const auto horizontal = cameramotion::basis(90, 0, 0);
    check(horizontal[0] == glm::vec3(1, 0, 0), "horizontal left basis points +X");
    check(horizontal[1].z == 1 && horizontal[2].y == -1 && horizontal[2].z == -4.371138828673793e-8f,
          "horizontal basis uses native float radians/trig");
    const auto upward = cameramotion::basis(0, 0, 0);
    check(upward == glm::mat3(1), "straight-up camera basis is finite identity");
    const auto tilted = cameramotion::basis(37, 23, -51);
    for (int column = 0; column < 3; ++column) {
        check(close(glm::dot(tilted[column], tilted[column]), 1), "tilted camera axes retain unit length");
        check(close(glm::dot(tilted[column], tilted[(column + 1) % 3]), 0), "tilted camera axes remain orthogonal");
    }
    check(tilted[1] != glm::vec3(0, 0, 1), "tilted camera up is not replaced with world vertical");
    check(cameramotion::basis(37.00000000001, 23.00000000001, -51.00000000001) == tilted, "displayed double angles narrow before trig");
    const auto zero = cameramotion::viewShake(0, 1), phase = cameramotion::viewShake(100, 1);
    check(zero.x == 0.5f && zero.y == 0.5f, "view shake is half-magnitude in both axes at clock zero");
    check(close(phase.x, -0.49499624967575073f) && close(phase.y, -0.32682180404663086f),
          "independent cosine phases use world milliseconds");
    check(cameramotion::viewShake(100, 0) == glm::vec2(0), "no shake leaves render view unchanged");
    std::printf("Camera motion: %s\n", failures ? "FAILED" : "passed");
    return failures;
}
