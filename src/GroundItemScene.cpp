#include "GroundItemScene.h"
#include "GameMath.h"
#include "GroundItems.h"
#include "worldgen/Noise.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace groundscene {
namespace {
int32_t subtype(const nativesaves::Item &item) {
    int32_t value;
    std::memcpy(&value, item.data() + 4, sizeof value);
    return value;
}
void translate(glm::mat4 &m, glm::vec3 p) {
    for (int row = 0; row < 4; ++row)
        m[3][row] = ((p.x * m[0][row] + p.y * m[1][row]) + p.z * m[2][row]) + m[3][row];
}
void scale(glm::mat4 &m, float value) {
    if (value != 1)
        for (int column = 0; column < 3; ++column)
            for (int row = 0; row < 4; ++row)
                m[column][row] *= value;
}
void rotate(glm::mat4 &m, int axis, float degrees) {
    const float radians = degrees * .01745329238474369f;
    const float c = std::cos(radians), s = std::sin(radians);
    const int a = (axis + 1) % 3, b = (axis + 2) % 3;
    for (int row = 0; row < 4; ++row) {
        const float p = m[a][row], q = m[b][row];
        m[a][row] = s * q + p * c;
        m[b][row] = c * q - p * s;
    }
}
} // namespace

Record decode(const nativesaves::Record &bytes) {
    Record result;
    const auto copy = [&](auto &value, size_t offset) { std::memcpy(&value, bytes.data() + offset, sizeof value); };
    using namespace grounditems::ground;
    copy(result.item, ITEM);
    copy(result.position, POSITION);
    copy(result.angle, ANGLE);
    copy(result.delayMs, DELAY);
    copy(result.timerMs, TIMER);
    copy(result.quantity, COUNT);
    copy(result.owner, OWNER);
    return result;
}

float itemScale(const nativesaves::Item &item) {
    const int category = item[0], kind = subtype(item);
    if (category == 11 && kind == 36)
        return .05f;
    if ((category == 11 && (kind == 0 || (kind >= 32 && kind <= 35) || kind == 42)) || (category == 24 && (kind == 1 || kind == 2)))
        return .15f;
    return category == 12 ? .04f : .0714285746216774f;
}

scenevisibility::Sphere bounds(const Input &input, const nativesaves::Item &item) {
    return {input.position, float(std::max(input.dimensions.x, input.dimensions.z)) * itemScale(item)};
}

Prepared prepare(const Input &input, const nativesaves::Item &item) {
    Prepared out;
    out.position = input.position;
    const int category = item[0], kind = subtype(item);
    const float size = itemScale(item);
    const glm::vec3 dimensions(input.dimensions), half = dimensions * .5f;
    out.sphere = bounds(input, item);
    out.material.a = (category == 11 && (kind == 14 || kind == 19)) || category == 19 ? 1.f : float(input.light) / 255.f;
    out.opacityScale = category == 11 && (kind == 37 || kind == 39) ? .6000000238418579f : 1.f;
    if (input.delayMs > 0) {
        const float h = (input.delayMs - 250.f) / 250.f;
        const float hop = (1.f - h * h) * 5.f;
        out.position[2] = gamemath::addFixed(out.position[2], worldgen::trunc64(double(hop * 65536.f)));
    }
    scale(out.model, size);
    int style = category;
    if (category == 22 && (kind == 63 || kind == 64))
        style = kind == 63 ? 3 : 4;
    if (input.originalCategory == 30)
        style = 30;
    const bool pulse = style == 12 || style == 13 || style == 14 || style == 23 || style == 24 || style == 29 || style == 30;
    if (style == 3 || style == 4 || style == 12 || style == 13) {
        rotate(out.model, 2, style == 12 || style == 13 ? input.angle + input.worldMs * .2f : input.angle);
        translate(out.model, {0, 0, half.x});
        rotate(out.model, 1, 90);
        translate(out.model, -half);
    } else if (pulse) {
        rotate(out.model, 2, input.angle + input.worldMs * .1f);
        translate(out.model, {0, 0, dimensions.x * .8500000238418579f});
        if (style == 14) {
            rotate(out.model, 1, 45);
            rotate(out.model, 0, 45);
        }
        translate(out.model, -half);
    } else if (category == 11 && kind == 14) {
        rotate(out.model, 2, input.worldMs * .019999999552965164f);
        translate(out.model, {0, 0, dimensions.z});
        rotate(out.model, 0, 45);
        rotate(out.model, 1, 45);
        scale(out.model, 5);
        translate(out.model, -half);
    } else {
        rotate(out.model, 2, input.angle);
        translate(out.model, {-half.x, -half.y, 0});
    }
    if (input.delayMs != 0) {
        translate(out.model, half);
        rotate(out.model, 0, input.delayMs);
        rotate(out.model, 1, input.delayMs * .699999988079071f);
        translate(out.model, -half);
    }
    if (pulse) {
        const float doubled = input.worldMs + input.worldMs;
        const float degrees = doubled * .20000000298023224f;
        const float radians = float(double(degrees) * 3.14159265358979323846 / 180.0);
        const float wave = .5f * std::cos(radians) + .5f;
        out.material *= 1.f + wave * wave;
    }
    if (input.selected)
        out.material += glm::vec4(.25f);
    return out;
}
} // namespace groundscene
