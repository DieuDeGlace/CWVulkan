#pragma once
// Saved-character Widget callback, from steam-character-card-preview.md.
#include "NativePose.h"
#include <glm/glm.hpp>
#include <optional>

namespace charactercard {
struct Frame {
    int32_t guiMilliseconds = 0;
    float rendererMilliseconds = 0;
    bool hovered = false;
    uint32_t systemMilliseconds = 0;
};
struct Geometry {
    glm::vec2 screen{0};
    float scale = 1;
    glm::mat4 base{1}, projection{1};
};
// The live node matrix already contains GUI scale and native layout/binding.
std::optional<Geometry> geometry(const glm::mat4 &node, glm::vec2 pivot, glm::ivec2 viewport, int32_t zoneRadius);
void advance(uint8_t *record, int32_t guiMilliseconds, bool hovered);
// Mutates the borrowed record's animation cache; only drawn XYZ is restored.
void prepare(uint8_t *record, const Frame &, const nativepose::Models &, const std::function<int()> &random, nativepose::Sparks &,
             std::vector<nativepose::Draw> &);
} // namespace charactercard
