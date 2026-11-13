#include "FeatText.h"
#include <algorithm>
#include <bit>
namespace feattext {
namespace {
glm::vec3 dividedPoint(glm::vec3 point, const glm::mat4 &matrix) {
    glm::vec4 out;
    for (int lane = 0; lane < 4; ++lane) {
        float value = point.y * matrix[1][lane];
        value = value + point.x * matrix[0][lane];
        value = value + point.z * matrix[2][lane];
        out[lane] = value + matrix[3][lane];
    }
    const float inverseW = 1.f / out.w;
    return {out.x * inverseW, out.y * inverseW, out.z * inverseW};
}
std::optional<glm::vec2> project(Point physical, const scenevisibility::Projection &projection, glm::vec2 viewport) {
    glm::vec3 relative;
    for (int axis = 0; axis < 3; ++axis) {
        const int64_t shifted =
            axis == 2 ? physical[axis] : std::bit_cast<int64_t>((uint64_t)physical[axis] + (uint64_t)projection.offset[axis]);
        relative[axis] = (float)shifted * (1.f / 65536.f);
    }
    const auto view = dividedPoint(relative, projection.view);
    if (!(view.z > 0))
        return {};
    const auto clip = dividedPoint(view, projection.perspective);
    const glm::vec2 half = viewport * .5f;
    return glm::vec2(half.x * clip.x + half.x, -half.y * clip.y + half.y);
}
} // namespace
Record feat(Point physical) {
    Record record;
    record.anchor = physical, record.text = L"FEAT!", record.colour = {.7f, .2f, 1, 1}, record.fontSize = FONT_SIZE;
    return record;
}
std::vector<Label> Store::visit(const std::optional<scenevisibility::Projection> &projection, glm::vec2 viewport, int32_t elapsedMs) {
    std::vector<Label> labels;
    labels.reserve(entries.size());
    for (auto &entry : entries) {
        const Record &record = entry.record;
        if (projection)
            if (const auto at = project(record.anchor, *projection, viewport)) {
                const float progress = (float)entry.age / (float)record.lifetime;
                const float alpha = record.colour.a * (1 - progress);
                const glm::vec3 colour(record.colour);
                const float y = at->y + progress * record.drift;
                labels.push_back({glm::vec2(at->x + record.offsetX, y + record.offsetY), glm::vec4(colour, alpha), glm::vec4(colour * .1f, alpha),
                                  record.text, record.fontSize, outlineRadius(record.fontSize), record.flags});
            }
        entry.age = std::bit_cast<int32_t>((uint32_t)entry.age + (uint32_t)elapsedMs);
    }
    std::erase_if(entries, [](const Entry &entry) { return entry.age > entry.record.lifetime; });
    return labels;
}
} // namespace feattext
