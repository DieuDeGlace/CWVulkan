#include "SceneVisibility.h"
#include "SteamScenery.h"
#include "Extracted.h"
#include "AlphaLook.h"
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <cstdio>

int sceneVisibilityTests() {
    int failures = 0;
    auto check = [&](bool result, const char *message) {
        if (!result) { std::printf("FAIL scene visibility: %s\n", message); ++failures; }
    };
    auto flat = scenevisibility::build(-2, -1,
        [](int64_t, int64_t) { return std::optional<scenevisibility::Column>({0, 1}); },
        [](int64_t, int64_t, int64_t z) { return uint8_t(z < 1 ? 1 : 0); });
    check(flat.minimum == std::array<int64_t, 3>{-128LL * 65536, -64LL * 65536, 0} &&
          flat.maximum == std::array<int64_t, 3>{-63LL * 65536, 65536, 2LL * 65536}, "native 65-block bounds and height clamp");
    check(flat.center[0] == -95LL * 65536 - 32768 && flat.center[1] == -31LL * 65536 - 32768,
          "negative fixed-coordinate midpoint");
    check(std::abs(flat.radius - std::sqrt(32.5f * 32.5f * 2 + 1) * 1.5f) < 0.0001f,
          "cached radius uses inflated half diagonal");
    check(!flat.meshed && flat.plannedBands == 1, "planned bands do not assert published geometry");
    auto resetUpper = scenevisibility::build(0, 0,
        [](int64_t x, int64_t y) -> std::optional<scenevisibility::Column> {
            if (x == 0 && y == 63) return scenevisibility::Column{0, 1};
            if (x == 0 && y == 64) return scenevisibility::Column{0, 500};
            return {};
        },
        [](int64_t x, int64_t y, int64_t z) { return uint8_t(z < 0 || (x == 0 && y == 63 && z == 0) ? 1 : 0); });
    check(resetUpper.lowest == 0 && resetUpper.upper == 1, "first exposed voxel overwrites earlier north-end raise");
    auto laterUpper = scenevisibility::build(0, 0,
        [](int64_t x, int64_t y) -> std::optional<scenevisibility::Column> {
            if (x == 0 && y == 0) return scenevisibility::Column{0, 1};
            if (x == 0 && y == 63) return scenevisibility::Column{0, 0};
            if (x == 0 && y == 64) return scenevisibility::Column{0, 500};
            return {};
        },
        [](int64_t x, int64_t y, int64_t z) { return uint8_t(z < 0 || (x == 0 && y == 0 && z == 0) ? 1 : 0); });
    check(laterUpper.upper == 500 && laterUpper.plannedBands == 3 && laterUpper.maximum[2] == 501LL * 65536,
          "later north-end raise and 250-layer band plan survive");
    auto water = [](bool stone) {
        return scenevisibility::build(0, 0,
            [](int64_t, int64_t) { return std::optional<scenevisibility::Column>({10, 11}); },
            [=](int64_t x, int64_t y, int64_t z) { return uint8_t(stone && x == 0 && y == 0 && z == 10 ? 1 : 2); });
    };
    check(water(false).lowest == 0 && water(true).lowest == 10,
          "stone against water exposes a face; water against water does not");

    scenevisibility::Bounds box;
    box.minimum = {-65536, -65536, -65536};
    box.maximum = {65536, 65536, 65536};
    box.meshed = true;
    scenevisibility::Planes planes;
    planes.fill({0, 0, 0, 1});
    planes[3].w = -1;
    check(scenevisibility::visible(box, {}, planes, 100, 100), "native zone box skips only upper screen plane");
    planes[5].w = -1;
    check(!scenevisibility::visible(box, {}, planes, 100, 100), "far plane still rejects the box");
    planes.fill({0, 0, 0, 1});
    planes[0] = {1, 0, 0, -2};
    check(!scenevisibility::visible(box, {}, planes, 100, 100), "positive corner outside plane is rejected");
    planes.fill({0, 0, 0, 1});
    box.center = {64LL * 65536, 0, 0};
    check(!scenevisibility::visible(box, {}, planes, 100, 0), "stored center gate is strict and does not recenter the box");
    box.center = {};
    box.minimum[0] = box.maximum[0] = 64LL * 65536;
    check(scenevisibility::visible(box, {}, planes, 0, 100), "early horizontal range retains equality");
    box.minimum[0] += 1; box.maximum[0] += 1;
    check(!scenevisibility::visible(box, {}, planes, 0, 100), "early horizontal range rejects one fixed tick beyond edge");
    box.meshed = false;
    check(!scenevisibility::visible(box, {}, planes, 100, 100), "unpublished geometry fails the readiness gate");
    check(steamscenery::earlyWithin({200, 200, 0}, {}, 0), "each squared delta is divided before summing");
    check(!steamscenery::earlyWithin({65537, 0, 0}, {}, 1), "early squared range uses fixed arithmetic");
    const auto identity = scenevisibility::planes(glm::mat4(1));
    check(identity[3] == glm::vec4(0, -1, 0, 1) && identity[4] == glm::vec4(0, 0, 1, 1),
          "native W-minus-Y and W-plus-Z plane extraction");
    planes.fill({0, 0, 0, 1});
    planes[0] = {100, 0, 0, -2};
    check(!scenevisibility::sphere({}, 1, {}, planes, 100), "native sphere adds radius to raw, unnormalized plane");
    planes[0].w = -1;
    check(scenevisibility::sphere({}, 1, {}, planes, 100), "sphere accepts exact raw-plane boundary");
    planes[3].w = -2;
    check(!scenevisibility::sphere({}, 1, {}, planes, 100), "sphere tests upper screen plane too");
    const glm::vec3 eye{3, 4, 5}, target{1, -3, 2}, up{0, 0, 1};
    const auto nativeClip = glm::perspectiveLH_ZO(.9f, -1.6f, .05f, 1920.0f) * glm::lookAtLH(eye, target, up);
    const auto engineClip = glm::perspectiveRH_ZO(.9f, 1.6f, .05f, 1920.0f) * glm::lookAtRH(eye, target, up);
    float difference = 0;
    for (int column = 0; column < 4; ++column)
        for (int row = 0; row < 4; ++row)
            difference = std::max(difference, std::abs(nativeClip[column][row] - engineClip[column][row]));
    check(difference < 0.00001f, "negative-aspect forward-Z native clip matches unflipped RH engine clip");

    // Real CWZ columns check the fast snapshot adapter and native game-Z convention.
    namespace fs = std::filesystem;
    const auto root = fs::temp_directory_path() / ("CubeVulkan-scene-bounds-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directory(root);
    { std::ofstream(root / "manifest.txt") << "seed=123\nhomeX=-65\nhomeY=-1\nhomeZ=7\n"; }
    auto writeZone = [&](int x, int y) {
        std::ofstream out(root / ("z_" + std::to_string(x) + "_" + std::to_string(y) + ".cwz"), std::ios::binary);
        out.write("CWZ1", 4);
        const int32_t header[]{x, y, 0, 0};
        out.write((const char *)header, sizeof header);
        for (int i = 0; i < 4096; ++i) {
            const int32_t base = 10;
            const uint32_t runs = 1;
            const uint16_t count = 1;
            const uint8_t block[]{50, 60, 70, 0, 1, 0};
            out.write((const char *)&base, 4); out.write((const char *)&runs, 4);
            out.write((const char *)&count, 2); out.write((const char *)block, 6);
        }
        check(bool(out), "CWZ fixture written");
    };
    writeZone(-2, -1);
    ExtractedWorld extracted;
    std::string error;
    check(extracted.open(root, error) && !extracted.meshBounds(0, 0), "missing neighbors do not invent native bounds");
    for (int x = -3; x <= -1; ++x)
        for (int y = -2; y <= 0; ++y)
            writeZone(x, y);
    check(extracted.open(root, error), "complete neighborhood opens");
    const auto loaded = extracted.meshBounds(0, 0);
    check(loaded && loaded->minimum == std::array<int64_t, 3>{-128LL * 65536, -64LL * 65536, 10LL * 65536} &&
          loaded->maximum == std::array<int64_t, 3>{-63LL * 65536, 65536, 12LL * 65536},
          "column snapshot preserves absolute fixed XY and unshifted native Z");
    const auto previousStyle = renderStyle();
    setRenderStyle(RenderStyle::Steam);
    Mesh solid, liquid;
    std::optional<scenevisibility::Bounds> meshBounds;
    check(extracted.mesh(0, 0, solid, liquid, &meshBounds) && loaded && meshBounds &&
          loaded->minimum == meshBounds->minimum && loaded->maximum == meshBounds->maximum && !meshBounds->meshed,
          "mesher shares its exact column snapshot and leaves publication to the owner");
    setRenderStyle(previousStyle);
    fs::remove_all(root);
    std::printf("Scene visibility: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
