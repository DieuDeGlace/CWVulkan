#include "CharacterCard.h"
#include "GameAngles.h"
#include "GameMath.h"
#include "GuiItem.h"
#include "NativeCrt.h"
#include <array>
#include <bit>
#include <cstring>

namespace charactercard {
namespace {
template <class T> T read(const uint8_t *record, size_t offset) {
    T value;
    std::memcpy(&value, record + offset, sizeof value);
    return value;
}
template <class T> void write(uint8_t *record, size_t offset, const T &value) {
    std::memcpy(record + offset, &value, sizeof value);
}
void ease(uint8_t *record, size_t offset, float target, float factor) {
    const float old = read<float>(record, offset);
    write(record, offset, old + (target - old) * factor);
}
glm::mat4 rotation(float degrees, int axis) {
    const float radians = degrees * 0.01745329238474369f;
    const float c = nativecrt::cosf(radians), s = nativecrt::sinf(radians);
    glm::mat4 result(1);
    if (axis == 0) {
        result[1][1] = result[2][2] = c;
        result[1][2] = s;
        result[2][1] = -s;
    } else {
        result[0][0] = result[1][1] = c;
        result[0][1] = s;
        result[1][0] = -s;
    }
    return result;
}
class DrawnOrigin {
  public:
    explicit DrawnOrigin(uint8_t *record) : position(record + 0xb10) {
        std::memcpy(saved.data(), position, saved.size());
        std::memset(position, 0, saved.size());
    }
    ~DrawnOrigin() { std::memcpy(position, saved.data(), saved.size()); }
    DrawnOrigin(const DrawnOrigin &) = delete;
    DrawnOrigin &operator=(const DrawnOrigin &) = delete;

  private:
    uint8_t *position;
    std::array<uint8_t, 24> saved;
};
} // namespace

std::optional<Geometry> geometry(const glm::mat4 &node, glm::vec2 pivot, glm::ivec2 viewport, int32_t zoneRadius) {
    const float reciprocal = 1.f / ((node[0][3] * pivot.x + node[1][3] * pivot.y) + node[3][3]);
    const glm::vec2 projected{((node[0][0] * pivot.x + node[1][0] * pivot.y) + node[3][0]) * reciprocal,
                              ((node[0][1] * pivot.x + node[1][1] * pivot.y) + node[3][1]) * reciprocal};
    Geometry result;
    result.scale = node[0][0];
    result.screen = {projected.x + result.scale * 100.f, projected.y + result.scale * 110.f};
    const float farPlane = float(std::bit_cast<int32_t>(uint32_t(zoneRadius) << 6));
    const auto projection = guiitem::screenProjection(viewport, result.screen, farPlane, -.5f);
    if (!projection)
        return std::nullopt;
    result.projection = *projection;
    result.base[3].z = 17;
    result.base = (result.base * rotation(-95, 0)) * rotation(45, 2);
    if (result.scale != 1)
        for (int column = 0; column < 3; ++column)
            result.base[column] *= result.scale;
    return result;
}

void advance(uint8_t *record, int32_t guiMilliseconds, bool hovered) {
    for (size_t offset : {size_t(0x12c), size_t(0x130), size_t(0x5c)})
        write(record, offset, uint32_t(0));
    write(record, 0x128, uint16_t(0));
    write(record, 0x68, uint8_t(0));
    const float ms = float(guiMilliseconds);
    write(record, 0x99c, read<float>(record, 0x99c) + ms * .0075f);
    const float factor = gamemath::blendFactor(double(guiMilliseconds), .01f);
    const float yaw = read<float>(record, 0xb3c);
    if (hovered) {
        write(record, 0xb3c, yaw - ms * .05f);
        write(record, 0x998, .2f);
    } else {
        write(record, 0xb3c, gameangles::turn(yaw, 0, factor));
        ease(record, 0x998, 0, factor);
    }
    for (int axis = 0; axis < 3; ++axis) {
        ease(record, 0x40 + axis * 4, hovered && axis == 1 ? 8.f : 0.f, factor);
        ease(record, 0x34 + axis * 4, 0, factor);
    }
}

void prepare(uint8_t *record, const Frame &frame, const nativepose::Models &models, const std::function<int()> &random,
             nativepose::Sparks &sparks, std::vector<nativepose::Draw> &draws) {
    advance(record, frame.guiMilliseconds, frame.hovered);
    draws.clear();
    nativepose::Call call;
    call.dt = frame.rendererMilliseconds;
    call.nowMs = frame.systemMilliseconds;
    call.rand = random;
    call.sparks = &sparks;
    const DrawnOrigin restore(record);
    nativepose::pose(record, call, models, &draws);
}
} // namespace charactercard
