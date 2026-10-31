#include "GuiItem.h"
#include "NativePose.h"
#include <bit>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace guiitem {
Placement spritePlacement(glm::vec2 origin, glm::vec2 size, int32_t clock) {
    Placement at;
    at.screen = origin + size * 0.5f;
    at.euler = {225, 0, phase(clock)};
    at.size = size.x * 0.0006f;
    at.darkPass = true;
    return at;
}

float phase(int32_t guiClockMs) {
    return ((float)(guiClockMs % 10000) * 0.0001f) * 360.0f;
}
std::optional<Geometry> geometry(int category, glm::ivec3 allocation, glm::ivec2 viewport, const Placement &at) {
    if (viewport.x <= 0 || viewport.y <= 0 || allocation.x <= 0 || allocation.y <= 0 || allocation.z <= 0)
        return std::nullopt;
    int32_t divisor = std::max({allocation.x, allocation.y, allocation.z});
    if (allocation.z <= 3)
        divisor += 4;
    if (category == 4)
        divisor = (int32_t)((float)divisor * 1.5f);
    Geometry out;
    out.model = glm::translate(glm::mat4(1), {0, 0, 1});
    auto rotate = [&](float degrees, glm::vec3 axis) { out.model = glm::rotate(out.model, glm::radians(degrees), axis); };
    rotate(at.euler.x, {1, 0, 0});
    rotate(at.euler.y, {0, 1, 0});
    rotate(at.euler.z, {0, 0, 1});
    if (category == 9) {
        rotate(90, {1, 0, 0});
        rotate(-90, {0, 0, 1});
    }
    // Keep the native float operations separate; use the full header, not occupied bounds.
    out.model = glm::scale(out.model, glm::vec3(at.size));
    out.model = glm::scale(out.model, glm::vec3(1024.0f / (float)viewport.y));
    out.model = glm::scale(out.model, glm::vec3(1.0f / (float)divisor));
    out.model = glm::translate(out.model, glm::vec3(allocation) * -0.5f);
    const float width = (float)viewport.x, height = (float)viewport.y;
    const float h = 1.0f / std::tan(0.39269909262657166f);
    glm::mat4 lens(0);
    lens[0][0] = h / -(width / height);
    lens[1][1] = h;
    lens[2][2] = 1.000100016593933f;
    lens[3][2] = -0.10001000016927719f;
    lens[2][3] = 1;
    float x = (at.screen.x - width * 0.5f) / width;
    x = x + x;
    const float y = ((at.screen.y - height * 0.5f) / height) * -2.0f;
    out.projection = glm::translate(glm::mat4(1), glm::vec3(x, y, at.screenDepth)) * lens;
    if (at.darkPass) {
        const float offset = -0.1f * at.size;
        out.darkView = glm::translate(glm::mat4(1), glm::vec3(offset, offset, 0));
        out.depthOffset = 0.1f;
    }
    return out;
}
float shininess(const nativesaves::Item &item) {
    const int category = item[nativesaves::item::TYPE], material = item[nativesaves::item::MATERIAL];
    if (category == 0)
        return 0;
    switch (material) {
    case 1:
    case 11:
    case 12:
    case 13:
    case 23:
        return 1;
    case 4:
    case 29:
        return 0.5f;
    case 33:
    case 35:
        return 0.25f;
    }
    int32_t subtype;
    std::memcpy(&subtype, item.data() + nativesaves::item::SUBTYPE, sizeof subtype);
    const bool shinySpecial =
        category == 24 && (subtype == 4 || subtype == 5 || subtype == 7 || subtype == 8 || subtype == 10 || subtype == 18);
    return category == 23 || shinySpecial ? 1.0f : 0.0f;
}
glm::vec3 lightDirection() {
    const float a = 1.0f / std::sqrt(2.1599998474121094f);
    glm::vec3 direction(a, a * 0.4f, -a);
    const float sum = (direction.x * direction.x + direction.y * direction.y) + direction.z * direction.z;
    return direction * (1.0f / std::sqrt(sum));
}
ShaderState overlayState() {
    ShaderState state;
    const float a = 1.0f / std::sqrt(3.0f);
    const glm::vec3 direction(a, a, -a);
    const float sum = (direction.x * direction.x + direction.y * direction.y) + direction.z * direction.z;
    state.direction = direction * (1.0f / std::sqrt(sum));
    return state;
}
void appendDraws(const Request &request, glm::ivec2 viewport, ShaderState &retained, std::vector<Draw> &out) {
    if (!request.mesh)
        return; // unavailable owner sprite
    const auto transform = geometry(request.item[nativesaves::item::TYPE], request.allocation, viewport, request.at);
    if (!transform)
        return;
    Draw draw;
    draw.mesh = request.mesh;
    draw.model = transform->model;
    draw.projection = transform->projection;
    if (request.at.darkPass) {
        draw.view = transform->darkView;
        draw.material = {0, 0, 0, 1};
        draw.state = retained; // black draw has not reset the preceding WORLD effects/palette
        out.push_back(draw);
    }
    retained = ShaderState{};
    retained.direction = lightDirection();
    retained.shininess = shininess(request.item);
    draw.view = glm::mat4(1);
    draw.material = glm::vec4(1);
    draw.state = retained;
    draw.depthOffset = transform->depthOffset;
    out.push_back(draw);
    if (request.cubeMesh)
        for (const auto &cube : spiritCubes(request.item, transform->model)) {
            draw.mesh = request.cubeMesh;
            draw.model = cube.model;
            draw.material = cube.material;
            out.push_back(draw); // same main-pass view, projection, effects and depth offset
        }
}

std::vector<SpiritCube> spiritCubes(const nativesaves::Item &item, const glm::mat4 &weaponModel) {
    std::vector<SpiritCube> cubes;
    if (item[nativesaves::item::TYPE] != 3)
        return cubes;
    constexpr size_t ATTRIBUTE_BEGIN = 0x1d;
    constexpr size_t CAPACITY = (nativesaves::item::ATTRIBUTE_COUNT - ATTRIBUTE_BEGIN) / 4;
    // A malformed count cannot make the replacement read beyond the item's allocation.
    const size_t count = std::min<size_t>(item[nativesaves::item::ATTRIBUTE_COUNT], CAPACITY);
    cubes.reserve(count);
    const float white[4] = {1, 1, 1, 1};
    for (size_t i = 0; i < count; i++) {
        const size_t at = ATTRIBUTE_BEGIN + 4 * i;
        const float x = (float)std::bit_cast<int8_t>(item[at]), y = (float)std::bit_cast<int8_t>(item[at + 1]),
                    z = (float)std::bit_cast<int8_t>(item[at + 2]);
        SpiritCube cube;
        cube.model = weaponModel; // every attribute starts from the original weapon matrix
        const auto dx = weaponModel[0] * x, dy = weaponModel[1] * y, dz = weaponModel[2] * z;
        cube.model[3] = ((dx + dy) + dz) + weaponModel[3];
        const auto colour = nativepose::materialColour(item[at + 3], white, 0);
        cube.material = {colour[0], colour[1], colour[2], colour[3]};
        cubes.push_back(cube);
    }
    return cubes;
}

} // namespace guiitem
