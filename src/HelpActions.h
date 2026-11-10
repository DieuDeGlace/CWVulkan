#pragma once
// Selected Help draw body, independent of GUI traversal/font/matrix providers.
// Authority: selected Help row, root/text-space and annotation-flow prose in docs/specs.
#include "NativeAI.h"
#include <functional>
#include <string>
#include <glm/glm.hpp>

namespace helpactions {
struct Sources {
    std::function<uint8_t()> modifier;
    std::function<nativeai::Creature *()> actor;
    std::function<std::wstring(int)> binding;
    std::function<std::wstring(int32_t)> description;
    // Application binds the existing combat queries. Separate callbacks keep
    // full-width results and fresh receivers visible at their reached phases.
    std::function<int32_t(nativeai::Creature *,uint8_t)> primary, secondary;
};
// Returns the reached text helper flow, not a glyph bounding box or pixel position.
using Draw=std::function<glm::vec2(const std::wstring &,glm::vec2,glm::vec4)>;
void draw(const Sources &,const Draw &);
} // namespace helpactions
