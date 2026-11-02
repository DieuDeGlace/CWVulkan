#include "OwnedGroundCubes.h"
#include "GameMath.h"
#include "worldgen/Noise.h"
#include <cmath>

namespace ownedground {
namespace {
constexpr double PI = 3.141592653589793;

void rotate(glm::mat4 &matrix, int axis, float degrees) {
    const float radians = degrees * .01745329238474369f;
    const float c = std::cos(radians), s = std::sin(radians);
    const int a = (axis + 1) % 3, b = (axis + 2) % 3;
    for (int row = 0; row < 4; ++row) {
        const float p = matrix[a][row], q = matrix[b][row];
        matrix[a][row] = s * q + p * c;
        matrix[b][row] = c * q - p * s;
    }
}
} // namespace

std::optional<Source> source(const groundscene::Record &record, const nativesaves::Item &adapted, bool personalData, int64_t localOwner,
                             float worldMs, glm::vec4 rarityColor) {
    const int category = adapted[0];
    if (record.delayMs != 0 || !((category >= 3 && category <= 9) || category == 26) || !personalData || record.owner != localOwner)
        return {};
    Source result;
    result.position = record.position;
    result.first = rarityColor;
    result.second = rarityColor * .8999999761581421f;
    result.clock = uint32_t(worldgen::trunc64(double(worldMs)));
    result.radius = groundscene::itemScale(adapted) * 10.f;
    return result;
}

std::vector<Cube> prepare(const Source &source, glm::ivec3 header, std::array<int64_t, 2> renderOffset) {
    std::vector<Cube> cubes;
    if (source.count <= 0)
        return cubes;
    cubes.reserve(size_t(source.count));
    const float pulse = float(source.clock % 500) / 500.f;
    const double phase = double(pulse + pulse) * PI;
    const float orbit = float(source.clock % 10000) * .00009999999747378752f;
    glm::vec3 base;
    for (int axis = 0; axis < 3; ++axis) {
        const int64_t fixed = axis < 2 ? gamemath::addFixed(source.position[axis], renderOffset[axis]) : source.position[axis];
        base[axis] = float(fixed) * (1.f / 65536.f);
    }
    const glm::vec3 center = glm::vec3(header) * -.5f;
    uint32_t clock = source.clock;
    for (int32_t index = 0; index < source.count; ++index, clock += 443u) {
        const float a = float(clock % 2000) / 2000.f;
        const float q = (a - .5f) + (a - .5f);
        const float cosine = std::cos(float(phase + double(13 * index)));
        const float scale = ((cosine * .5f + 1.f) * source.size) * (1.f - q * q);
        const float turn = float(index) / 10.f + orbit;
        const float angle = float(double(turn + turn) * PI);
        glm::vec3 position = base;
        position.x += std::cos(angle) * source.radius;
        position.y += std::sin(angle) * source.radius;
        const float rise = a * source.radius;
        position.z += rise + rise;
        Cube cube;
        const glm::vec4 second = source.second * a;
        const glm::vec4 first = source.first * (1.f - a);
        cube.material = first + second;
        rotate(cube.model, 0, a * 40.f);
        rotate(cube.model, 1, a * 30.f);
        rotate(cube.model, 2, a * 10.f);
        for (int column = 0; column < 3; ++column)
            cube.model[column] *= scale;
        // Preserve the native float accumulation order, including the X row's Y/X/Z order.
        cube.model[3].x = ((position.x + cube.model[1].x * center.y) + cube.model[0].x * center.x) + cube.model[2].x * center.z;
        cube.model[3].y = ((position.y + cube.model[0].y * center.x) + cube.model[1].y * center.y) + cube.model[2].y * center.z;
        cube.model[3].z = ((position.z + cube.model[0].z * center.x) + cube.model[1].z * center.y) + cube.model[2].z * center.z;
        cubes.push_back(cube);
    }
    return cubes;
}
} // namespace ownedground
