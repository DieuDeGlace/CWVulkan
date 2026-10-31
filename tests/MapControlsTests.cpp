#include "MapControls.h"
#include <cstdio>
#include <limits>

int mapControlsTests() {
    using namespace mapcontrol;
    int failures = 0;
    auto check = [&](bool pass, const char *message) {
        if (!pass) {
            std::printf("FAIL map controls: %s\n", message);
            ++failures;
        }
    };
    check(mappedZoom(0, true, true) == 1 && mappedZoom(8, false, true) == 8 && mappedZoom(4, true, false) == 3,
          "mapped zoom clamps each ordered edge independently against all nine layers");
    check(wheelLayer(3, 120) == 2 && wheelLayer(3, -120) == 4 && wheelLayer(8, -1) == 6,
          "wheel messages use direction and their separate explicit ceiling6");
    const Position target{10 * 65536, -10 * 65536, 123}, cursor{20 * 65536, 0, 999};
    check(wheelCenter(target, cursor, true) == Position{15 * 65536, -5 * 65536, 123}, "inward wheel motion halves the fixed XY difference");
    check(wheelCenter(target, cursor, false) == Position{0, -20 * 65536, 123}, "outward wheel motion subtracts the whole fixed difference");
    check(wheelCenter({0, 0, 0}, {std::numeric_limits<int64_t>::max(), 0, 0}, true)[0] == 0,
          "wheel multiplication wraps low64 before signed division toward zero");
    const auto height = terrainHeight({-65535, 65535, 0}, [&](int32_t x, int32_t y) {
        check(x == 0 && y == 0, "terrain sampling converts fixed XY toward zero");
        return -1.25f;
    });
    check(height == Position{-65535, 65535, -81920}, "terrain Z uses the float height's fixed conversion");
    check(ease({0, 0, 0}, {65536, -65536, 0}, .01) == Position{13, -13, 0},
          "center easing quantizes its native coefficient into an integer weight");
    check(ease({0, 0, 0}, {65536, -65536, 0}, .00999999999) == Position{0, 0, 0},
          "center easing retains the original double interval at the hundredth-ms boundary");
    check(mousePan({}, {1, 2}, 3, 0) == Position{8 * 65536, -16 * 65536, 0}, "mouse pan scales GUI delta before float yaw rotation");
    check(padPan({}, {1, -.5f}, 3, 0, 10) == Position{-40 * 65536, 20 * 65536, 0},
          "pad pan applies its separate half-scale and frame interval");
    check(!movementCloses(true, true, true, false, {1, 0}) && !movementCloses(true, false, false, true, {.1f, 0}) &&
              movementCloses(true, false, true, false, {}) && !movementCloses(true, false, true, true, {}) &&
              movementCloses(true, false, false, true, {.11f, 0}),
          "map pad mode bypasses movement-close, and chat gates only its digital term");
    std::printf("Map controls: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
