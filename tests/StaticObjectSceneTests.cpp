#include "StaticObjectScene.h"
#include "SteamScenery.h"
#include <cmath>
#include <cstdio>
#include <limits>

int staticObjectSceneTests() {
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        if (!ok) {
            ++failures;
            std::printf("FAIL static-object scene: %s\n", message);
        }
    };
    const staticobjectscene::Point foot{11, -22, 33};
    const auto door = staticobjectscene::sphere(3, 1, foot);
    check(door.centre == staticobjectscene::Point{11, -22, 33 + 98304} && door.radius == 4,
          "type3 uses half of the native3-block height and full extent plus1 radius");
    const auto small = staticobjectscene::sphere(3, .5f, foot);
    check(small.centre[2] == 33 + 49152 && small.radius == 2.5f, "record scale applies before center/radius construction");
    const auto wrapped = staticobjectscene::sphere(3, 1, {0, 0, std::numeric_limits<int64_t>::max()});
    check(wrapped.centre[2] == std::numeric_limits<int64_t>::min() + 98303, "fixed half-height addition preserves low64 bits");
    check(staticobjectscene::playerZone({-1, -64 * 65536 - 1, 0}) == std::array<int32_t, 2>{-1, -2},
          "physical fixed coordinates retain negative zone boundaries");
    check(staticobjectscene::playerZone({-(int64_t(1) << 48), int64_t(1) << 48, 0}) == std::array<int32_t, 2>{-1, 0},
          "player-zone conversion narrows the block quotient before reconstruction");
    const staticobjectscene::Point player{-1, -1, 0};
    check(staticobjectscene::inPlayerSquare(player, 2, -4) && !staticobjectscene::inPlayerSquare(player, 3, -1),
          "containing-zone square is inclusive at3 and independent of a camera position");
    scenevisibility::View view;
    check(view.visible({{100 * 65536, 0, 0}, 20}, 100) && !view.visible({{101 * 65536, 0, 0}, 20}, 100),
          "range equality passes while sphere radius does not extend the distance limit");
    view.planes[5] = {1, 0, 0, -5};
    check(view.visible({{4 * 65536, 0, 0}, 1}, 100) && !view.visible({{3 * 65536, 0, 0}, 1}, 100),
          "the sixth plane admits zero and rejects a strictly negative result");
    check(std::abs(steamscenery::fade(95 * 95, 100) - .5f) < 1e-5f && steamscenery::fade(100 * 100, 100) == 0,
          "fade uses the same limit without adding64");
    std::printf("Native static-object scene: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
