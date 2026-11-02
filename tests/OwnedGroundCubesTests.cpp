#include "OwnedGroundCubes.h"
#include <cmath>
#include <cstdio>
#include <limits>

int ownedGroundCubesTests() {
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        if (!ok) {
            ++failures;
            std::printf("FAIL owned-ground cubes: %s\n", message);
        }
    };
    groundscene::Record record;
    record.position = {5 * 65536, 7 * 65536, 11 * 65536};
    record.item[0] = 30; // The effect category is the class-adapted item, not this original one.
    record.owner = 17;
    nativesaves::Item adapted{};
    adapted[0] = 3;
    const glm::vec4 color{.3f, .45f, 1, 1};
    const auto source = ownedground::source(record, adapted, true, 17, 1000.75f, color);
    check(source && source->position == record.position && source->count == 8 && source->clock == 1000 && source->first == color &&
              source->second == color * .9f && source->radius == groundscene::itemScale(adapted) * 10.f,
          "producer retains raw position, adapted scale, truncated clock and all four rarity components");
    check(!ownedground::source(record, adapted, false, 17, 0, color) && !ownedground::source(record, adapted, true, 18, 0, color),
          "personal data and the original owner ID are independent gates");
    record.delayMs = -1;
    check(!ownedground::source(record, adapted, true, 17, 0, color), "negative delay is not an exact-zero landing");
    record.delayMs = -0.f;
    adapted[0] = 26;
    const auto wrapped = ownedground::source(record, adapted, true, 17, -1.f, color);
    check(wrapped && wrapped->clock == UINT32_MAX, "category26 and negative zero admit; the signed clock retains its low32 bits");
    const auto large = ownedground::source(record, adapted, true, 17, 4294967808.f, color);
    check(large && large->clock == 512, "float world clocks above2^32 wrap after signed64 truncation");
    adapted[0] = 10;
    check(!ownedground::source(record, adapted, true, 17, 0, color), "the category range ends at9, with26 as its separate exception");

    ownedground::Source orbit;
    orbit.position = record.position;
    orbit.first = {1, 0, 0, .8f};
    orbit.second = {0, 1, 0, .2f};
    orbit.clock = 1000;
    orbit.radius = 2;
    const glm::ivec3 header{2, 4, 6};
    const auto cubes = ownedground::prepare(orbit, header, {-3 * 65536, 65536});
    check(cubes.size() == 8 && orbit.clock == 1000, "eight immediate draws use a local clock without mutating their source");
    if (cubes.size() == 8) {
        const auto firstCenter = glm::vec3(cubes[0].model * glm::vec4(glm::vec3(header) * .5f, 1));
        const auto nextCenter = glm::vec3(cubes[1].model * glm::vec4(glm::vec3(header) * .5f, 1));
        check(glm::length(firstCenter - glm::vec3(3.618034f, 9.175570f, 13)) < 3e-6f &&
                  glm::length(nextCenter - glm::vec3(2.618034f, 9.902113f, 13.886f)) < 4e-6f,
              "fixed XY offset precedes ten-position orbit spacing, while443 stepping changes rise separately");
        check(cubes[0].material == glm::vec4(.5f, .5f, 0, .5f) && std::abs(cubes[1].material.a - (.8f * .2785f + .2f * .7215f)) < 1e-7f,
              "the rising phase interpolates RGBA without replacing pixel opacity");
        check(std::abs(glm::length(glm::vec3(cubes[0].model[0])) - .15f) < 1e-7f && std::abs(cubes[0].model[0].x - .14433753f) < 2e-7f &&
                  std::abs(cubes[0].model[0].y - .02551264f) < 2e-7f && std::abs(cubes[0].model[0].z + .031871375f) < 2e-7f,
              "the first pulse and local X20/Y15/Z5 degree rotations retain their order");
        const float nextScale = ((std::cos(13.f) * .5f + 1.f) * .1f) * (1.f - .443f * .443f);
        check(std::abs(glm::length(glm::vec3(cubes[1].model[0])) - nextScale) < 1e-7f,
              "the pulse's thirteen-index increment is radians, independent of the443 rise clock");
        const auto wide = ownedground::prepare(orbit, {12, 4, 6}, {-3 * 65536, 65536});
        check(glm::length(glm::vec3(wide[0].model * glm::vec4(6, 2, 3, 1)) - firstCenter) < 4e-6f && wide[0].model[3] != cubes[0].model[3],
              "full allocation header dimensions determine centering even for a sparse owner mesh");
    }
    orbit.clock = 0xffffff00u;
    const auto wrap = ownedground::prepare(orbit, header, {});
    check(wrap.size() == 8 && std::abs(wrap[1].material.g - .0935f) < 1e-7f,
          "unsigned443 stepping wraps before the per-iteration modulo2000");
    orbit.clock = 0;
    const auto zero = ownedground::prepare(orbit, header, {});
    check(zero.size() == 8 && zero[0].model[0] == glm::vec4(0) && zero[0].model[1] == glm::vec4(0) && zero[0].model[2] == glm::vec4(0),
          "a zero-scale first cube remains a draw submission rather than being retired or culled");
    orbit.position[0] = std::numeric_limits<int64_t>::max();
    const auto fixedWrap = ownedground::prepare(orbit, header, {1, 0});
    check(fixedWrap[0].model[3].x == float(std::numeric_limits<int64_t>::min()) * (1.f / 65536.f),
          "render offsets add in low64 before the direct float conversion");
    orbit.count = 0;
    check(ownedground::prepare(orbit, header, {}).empty(), "nonpositive counts produce no iterations");
    std::printf("Native owned-ground cubes: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
