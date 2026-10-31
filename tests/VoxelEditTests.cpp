#include "VoxelEdit.h"
#include <bit>
#include <cmath>
#include <cstdio>

int voxelEditTests() {
    using namespace voxeledit;
    int failures = 0;
    auto check = [&](bool pass, const char *what) {
        if (!pass) {
            std::printf("FAIL voxel edit: %s\n", what);
            ++failures;
        }
    };
    glm::mat4 root(1);
    root[3] = {10, 20, 0, 1};
    const auto at = frame({2, 4, 8}, {1000, 1000}, root, {}, {200, 100}, {});
    check(at.has_value(), "valid full allocation");
    if (at) {
        const auto centre = at->model * glm::vec4(1, 2, 4, 1);
        const auto clip = at->projection * centre;
        check(glm::length(centre - glm::vec4(0, 0, 1, 1)) < .000001f, "model centers the complete header");
        check(std::abs(clip.x / clip.w + .78f) < .000001f && std::abs(clip.y / clip.w - .84f) < .000001f,
              "pivot plus queried dimensions sets the physical center");
        check(at->projection[2][2] == std::bit_cast<float>(0x3f800347u) && at->projection[3][2] == std::bit_cast<float>(0xbdccd20bu),
              "stored lens words");
        const auto resized = frame({2, 4, 8}, {1000, 500}, root, {}, {200, 100}, {});
        check(resized && resized->model == at->model, "model has no window-height normalization");
        root[0][0] = root[1][1] = 2;
        const auto scaled = frame({2, 4, 8}, {1000, 1000}, root, {}, {400, 200}, {});
        check(scaled && scaled->model == at->model, "GUI root scale does not resize the edit model");
        const auto ray = pointerRay(*at, {1000, 1000}, {110, 80});
        check(ray && std::abs(ray->origin.x - 1) < .001f && std::abs(ray->origin.y - 2) < .001f &&
                  std::abs(ray->direction.z - 1) < .000001f,
              "screen center unprojects into the actual object space");
        const auto left = pointerRay(*at, {1000, 1000}, {100, 80});
        check(left && left->direction.x > 0, "negative-aspect screen direction");
    }
    check(!frame({0, 1, 1}, {1000, 1000}, root, {}, {200, 100}, {}), "invalid allocation is not rendered");
    CubModel cub;
    cub.sx = 3;
    cub.sy = cub.sz = 1;
    cub.rgb = {255, 0, 0, 0, 255, 0, 100, 100, 100};
    nativesaves::Item item{};
    const Ray horizontal{{-1, .5f, .5f}, {1, 0, 0}};
    const auto hit = trace(cub, item, horizontal);
    check(hit.hit && hit.cell == glm::ivec3(2, 0, 0) && hit.normal == glm::ivec3(-1, 0, 0) && hit.distance == 3,
          "sprite marker colors are skipped before the opaque voxel");
    item[nativesaves::item::ATTRIBUTE_COUNT] = 1;
    item[0x1d] = uint8_t(-2);
    item[0x20] = 128;
    const auto boundary = trace(cub, item, horizontal);
    check(boundary.hit && boundary.cell.x == -2 && boundary.distance == 0 && boundary.attributeMaterial == 128,
          "exact negative integer starts one cell below mathematical floor");
    const auto carried = trace(cub, item, horizontal, glm::ivec3(254, 0, 0));
    check(carried.hit && carried.cell.x == 2 && !carried.attributeMaterial, "carried coordinates compare as bytes");
    item[0x1d] = 0;
    const auto wrapped = trace(cub, item, {{256.25f, .5f, .5f}, {1, 0, 0}});
    check(wrapped.hit && wrapped.cell.x == 256 && wrapped.attributeMaterial == 128,
          "attribute byte matches can occur outside the base allocation");
    cub.rgb.assign(9, 0);
    const auto expired = trace(cub, {}, {{0, .5f, .5f}, {1, 0, 0}});
    check(!expired.hit && expired.distance == 1000 && expired.cell.x == 1000, "distance1000 stops before testing the next cell");
    std::printf("Voxel edit projection/picking: %s\n", failures ? "FAILED" : "PASS");
    return failures;
}
