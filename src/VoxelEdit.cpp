#include "VoxelEdit.h"
#include "worldgen/Noise.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <bit>
#include <cmath>

namespace voxeledit {
std::optional<Frame> frame(glm::ivec3 allocation, glm::ivec2 viewport, const glm::mat4 &root, glm::vec2 pivot, glm::vec2 size,
                           glm::vec3 euler) {
    if (viewport.x <= 0 || viewport.y <= 0 || allocation.x <= 0 || allocation.y <= 0 || allocation.z <= 0)
        return std::nullopt;
    Frame out;
    out.model = glm::translate(glm::mat4(1), {0, 0, 1});
    for (int axis = 0; axis < 3; ++axis) {
        glm::vec3 direction(0);
        direction[axis] = 1;
        out.model = glm::rotate(out.model, glm::radians(euler[axis]), direction);
    }
    out.model = glm::scale(out.model, glm::vec3(.35f));
    out.model = glm::scale(out.model, glm::vec3(1.0f / (float)std::max({allocation.x, allocation.y, allocation.z})));
    out.model = glm::translate(out.model, glm::vec3(allocation) * -.5f);
    const auto point = root * glm::vec4(pivot, 0, 1);
    const float reciprocal = 1.0f / point.w;
    const glm::vec2 centre{point.x * reciprocal + size.x * .5f, point.y * reciprocal + size.y * .6f};
    const float width = (float)viewport.x, height = (float)viewport.y;
    const float focal = 1.0f / std::tan(.39269909262657166f);
    glm::mat4 lens(0);
    lens[0][0] = -focal / (width / height);
    lens[1][1] = focal;
    lens[2][2] = std::bit_cast<float>(0x3f800347u);
    lens[3][2] = std::bit_cast<float>(0xbdccd20bu);
    lens[2][3] = 1;
    float x = (centre.x - width * .5f) / width;
    x += x;
    const float y = ((centre.y - height * .5f) / height) * -2;
    out.projection = glm::translate(glm::mat4(1), {x, y, 0}) * lens;
    return out;
}

std::optional<Ray> pointerRay(const Frame &frame, glm::ivec2 viewport, glm::vec2 pointer) {
    if (viewport.x <= 0 || viewport.y <= 0)
        return std::nullopt;
    const auto inverse = glm::inverse(frame.projection * frame.model);
    float x = pointer.x / (float)viewport.x, y = pointer.y / (float)viewport.y;
    x = (x + x) - 1;
    y = 1 - (y + y);
    auto point = [&](float depth) {
        const auto p = inverse * glm::vec4(x, y, depth, 1);
        return glm::vec3(p) * (1.0f / p.w);
    };
    Ray ray;
    ray.origin = point(0);
    const auto delta = point(1) - ray.origin;
    const float length = std::sqrt((delta.x * delta.x + delta.y * delta.y) + delta.z * delta.z);
    if (!(length > 0) || !std::isfinite(length))
        return std::nullopt;
    ray.direction = delta * (1.0f / length);
    return ray;
}

Pick trace(const CubModel &cub, const nativesaves::Item &item, const Ray &ray, std::optional<glm::ivec3> carried) {
    Pick out;
    for (int axis = 0; axis < 3; ++axis) {
        out.cell[axis] = worldgen::trunc32(ray.origin[axis]);
        if (ray.origin[axis] < 0)
            --out.cell[axis];
    }
    const size_t count = std::min<size_t>(item[nativesaves::item::ATTRIBUTE_COUNT], 32);
    auto sameBytes = [](glm::ivec3 a, glm::ivec3 b) {
        return (uint8_t)a.x == (uint8_t)b.x && (uint8_t)a.y == (uint8_t)b.y && (uint8_t)a.z == (uint8_t)b.z;
    };
    while (out.distance < 1000) {
        const auto cell = out.cell;
        if (cell.x >= 0 && cell.y >= 0 && cell.z >= 0 && cell.x < cub.sx && cell.y < cub.sy && cell.z < cub.sz) {
            const size_t at = 3 * (((size_t)cell.z * cub.sy + cell.y) * cub.sx + cell.x);
            if (at + 2 < cub.rgb.size()) {
                const int r = cub.rgb[at], g = cub.rgb[at + 1], b = cub.rgb[at + 2];
                const bool empty = (r == 0 && g == 0 && b == 0) || (r == 255 && g == 0 && b == 0) || (r == 0 && g == 255 && b == 0) ||
                                   (r == 0 && g == 0 && b == 255);
                out.hit = !empty;
            }
        }
        for (size_t i = 0; i < count; ++i) {
            const size_t at = 0x1d + 4 * i;
            const glm::ivec3 attribute{item[at], item[at + 1], item[at + 2]};
            if ((!carried || !sameBytes(attribute, *carried)) && sameBytes(attribute, cell)) {
                out.hit = true;
                out.attributeMaterial = item[at + 3];
            }
        }
        if (out.hit)
            return out;
        const auto position = ray.origin + ray.direction * out.distance;
        float nearest = 10;
        int axis = 0;
        for (int candidate = 0; candidate < 3; ++candidate) {
            const float direction = ray.direction[candidate];
            if (direction == 0)
                continue;
            const float boundary = (float)(out.cell[candidate] + (direction > 0 ? 1 : 0));
            const float distance = (boundary - position[candidate]) / direction;
            if (distance < nearest) {
                nearest = distance;
                axis = candidate;
            }
        }
        out.cell[axis] += ray.direction[axis] > 0 ? 1 : -1;
        out.normal = glm::ivec3(0);
        out.normal[axis] = ray.direction[axis] > 0 ? -1 : 1;
        out.distance += nearest;
    }
    return out;
}
} // namespace voxeledit
