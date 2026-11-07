// Independent gameplay-camera §2, §4 and §7.2, including actual queued Audio consumers.
#include "Audio.h"
#include "CameraMotion.h"
#include "MouseMotion.h"
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
    cameramotion::FieldOfView lens;
    check(lens.radians() == .8726646900177002f, "native lens starts at the exact stored fifty-degree radians");
    lens.update(0x400, 0);
    check(lens.radians() == .8726646900177002f, "zero interval does not prime the narrow lens target");
    lens.update(0x400, 16);
    check(lens.radians() == .8222894668579102f, "narrow lens uses native float easing reference");
    lens.update(0, 16);
    check(lens.radians() == .8238686323165894f, "released lens retains its value and uses the slower native rate");
    cameramotion::FieldOfView longLens, cappedLens, fractionalLens;
    longLens.update(0x400, 2000);
    cappedLens.update(0x400, 1000);
    fractionalLens.update(0x400, .01);
    check(longLens.radians() == .5410485863685608f && longLens.radians() == cappedLens.radians(),
          "lens wall interval caps at one second without snapping to target");
    check(fractionalLens.radians() == .8725948929786682f, "lens retains native fractional-millisecond input");
    cameramotion::FieldOfView unrelatedLens;
    unrelatedLens.update(0x3ff, 16);
    check(unrelatedLens.radians() == .8726646900177002f, "unrelated toggle bits do not select the narrow lens");
    double mousePitch = 90, mouseYaw = 180;
    cameramotion::mouseTargets(mousePitch, mouseYaw, 4, -8, 50, false);
    check(mousePitch == 88 && mouseYaw == 179, "native default mouse sensitivity is quarter-degree displacement");
    cameramotion::mouseTargets(mousePitch, mouseYaw, 0, -8, 50, true);
    check(mousePitch == 90, "invert-Y changes only the vertical target update");
    cameramotion::mouseTargets(mousePitch, mouseYaw, 0, 1000, 50, false);
    check(mousePitch == 180, "mouse clamps the pitch target at the downward pole");
    cameramotion::mouseTargets(mousePitch, mouseYaw, 0, -1000, 50, false);
    check(mousePitch == 0, "mouse permits the complete native upward range");
    mousePitch = 181.25;
    cameramotion::mouseTargets(mousePitch, mouseYaw, 0, 0, 50, false);
    check(mousePitch == 180 && mouseYaw == 179, "stationary relative poll still clamps an out-of-range pitch target");
    mousePitch = -.25;
    cameramotion::mouseTargets(mousePitch, mouseYaw, 0, 0, 0, true);
    check(mousePitch == 0 && mouseYaw == 179, "zero sensitivity and invert do not bypass the stationary pitch clamp");
    mousePitch = 90;
    mouseYaw = 180;
    cameramotion::mouseTargets(mousePitch, mouseYaw, 7, 0, 3, false);
    check(mouseYaw == 180.0 - double(.105f), "mouse product rounds to float before widening into target degrees");
    mouseYaw = 180;
    cameramotion::mouseTargets(mousePitch, mouseYaw, .00001f, 0, 50, false);
    check(mouseYaw < 180 && float(mouseYaw) == 180.f, "double targets retain displacement below a float-degree increment");
    // Exercise the shared production routing with different entry/current ownership.
    mousemotion::State mouse;
    mouse.admitted = true;
    mouse.padSelected = true;
    mouse.pointerTarget = true;
    mouse.mapOpen = mouse.customizationOpen = true;
    mouse.arguments = {4, -8};
    mouse.pointerDelta = {100, 200};
    auto routing = mousemotion::route(mouse);
    check(routing.turnCamera && routing.cameraDelta == mouse.arguments && !routing.panMap && !routing.turnModel,
          "relative motion ignores the UI-only device, capture and button gates");
    mousePitch = 90, mouseYaw = 180;
    cameramotion::mouseTargets(mousePitch, mouseYaw, routing.cameraDelta.x, routing.cameraDelta.y, 50, false);
    check(mousePitch == 88 && mouseYaw == 179, "relative routing delivers device displacement to the existing camera math");
    mouse.interfaceAtEntry = mouse.interfaceOwned = true;
    mouse.padSelected = mouse.pointerTarget = false;
    mouse.buttons = 1;
    mouse.arguments = {1000, 2000};
    mouse.pointerDelta = {4, 8};
    routing = mousemotion::route(mouse);
    check(routing.turnCamera && routing.cameraDelta == mouse.pointerDelta && !routing.panMap && !routing.turnModel,
          "blank-space left dragging works in an ordinary UI and uses the cached pointer delta");
    mousePitch = 90, mouseYaw = 180;
    cameramotion::mouseTargets(mousePitch, mouseYaw, routing.cameraDelta.x, routing.cameraDelta.y, 50, false);
    check(mousePitch == 92 && mouseYaw == 179, "absolute UI coordinates do not become camera displacement");
    mouse.buttons = 3;
    routing = mousemotion::route(mouse);
    check(routing.pointerMoved && routing.panMap && routing.turnModel && routing.turnCamera,
          "map/model right dragging can continue into an admitted left-drag camera update");
    mouse.pointerTarget = true;
    routing = mousemotion::route(mouse);
    check(routing.panMap && routing.turnModel && !routing.turnCamera,
          "capture or a fresh node hit blocks camera dragging without suppressing map/model work");
    mouse.buttons = 2;
    mouse.pointerTarget = false;
    routing = mousemotion::route(mouse);
    check(routing.panMap && routing.turnModel && !routing.turnCamera, "a right button alone never admits UI camera dragging");
    mouse.padSelected = true;
    routing = mousemotion::route(mouse);
    check(!routing.pointerMoved && !routing.panMap && !routing.turnModel && !routing.turnCamera,
          "a pad-selected UI poll performs no mouse-specific work after pointer publication");
    mouse.padSelected = false;
    mouse.pointerDelta = {};
    routing = mousemotion::route(mouse);
    check(!routing.pointerMoved && routing.panMap && routing.turnModel && !routing.turnCamera,
          "a stationary right-drag call still reaches map/model branches");
    mouse.interfaceOwned = false; // A pointer callback changed ownership after entry.
    routing = mousemotion::route(mouse);
    check(routing.turnCamera && routing.cameraDelta == mouse.arguments,
          "late relative ownership uses the supplied values instead of a frozen entry-mode interpretation");
    mouse.interfaceAtEntry = false;
    mouse.arguments = {};
    mousePitch = 181.25;
    routing = mousemotion::route(mouse);
    if (routing.turnCamera)
        cameramotion::mouseTargets(mousePitch, mouseYaw, routing.cameraDelta.x, routing.cameraDelta.y, 50, false);
    check(mousePitch == 180, "a reached stationary relative call still clamps pitch");
    mouse.admitted = false;
    mousePitch = 181.25;
    routing = mousemotion::route(mouse);
    if (routing.turnCamera)
        cameramotion::mouseTargets(mousePitch, mouseYaw, routing.cameraDelta.x, routing.cameraDelta.y, 50, false);
    check(mousePitch == 181.25 && !routing.panMap && !routing.turnModel,
          "a skipped focus/capture poll does not deliver a substitute neutral camera call");

    frontend::OrbitCamera precise;
    precise.set(90, 0, 180, 8);
    precise.targetYaw = 180.0000001;
    precise.smooth(16, 100);
    check(precise.yaw > 180 && precise.yaw < precise.targetYaw && float(precise.yaw) == 180.f,
          "shared gameplay/frontend orbit retains double-degree smoothing below float precision");
    precise.set(90, 0, 179, 8);
    precise.targetYaw = -179;
    precise.smooth(16, 100);
    check(precise.yaw < 179, "displayed camera follows the stored target without shortest-angle wrapping");
    std::printf("Camera motion: %s\n", failures ? "FAILED" : "passed");
    return failures;
}
