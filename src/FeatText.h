#pragma once
// Floating texts of the Game's textFX list: steam-chat-feat-presentation.md section 4 and
// steam-progress-notices-and-floating-text-20261112.md sections 2-4. Origination of the feat
// descriptor is deliberately outside this store.
#include "SceneVisibility.h"
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
namespace feattext {
using Point = std::array<int64_t, 3>;
constexpr float FONT_SIZE = 30, OUTLINE = 3.75f; // the feat record
constexpr int SOUND = 0x1f;
constexpr int32_t LIFETIME = 3000;
// One record (progress-notices section 2): age starts at 0, drift -100 px, X offset +5 px.
// The combat texts (steam-combat-floating-texts-20261113.md section 3) use offsets 0, their
// own drift and lifetime 1000.
struct Record {
    Point anchor{};
    std::wstring text;
    glm::vec4 colour{};
    float offsetX = 5, offsetY = -100, drift = -100, fontSize = 16;
    int32_t lifetime = LIFETIME;
    // Record +0x60, given to both TextShapes' flags (progress notices section 4): 0 here, 1
    // (centre each line, plasma-shapes-and-text.md section 4.2) for the combat texts.
    int32_t flags = 0;
};
// Outline stroke radius of a record's font (section 4 step 7): font/8, at least 3.
inline float outlineRadius(float fontSize) {
    const float scaled = fontSize * 0.125f;
    return scaled > 3.0f ? scaled : 3.0f;
}
Record feat(Point physical);
struct Label {
    glm::vec2 at{};
    glm::vec4 fill{}, outline{};
    std::wstring text = L"FEAT!";
    float fontSize = FONT_SIZE, outlineRadius = OUTLINE;
    int32_t flags = 0;
};
class Store {
  public:
    void received(Point physical) { append(feat(physical)); }
    void append(Record record) { entries.push_back({std::move(record), 0}); }
    std::vector<Label> visit(const std::optional<scenevisibility::Projection> &, glm::vec2 viewport, int32_t elapsedMs);
    size_t size() const { return entries.size(); }

  private:
    struct Entry {
        Record record;
        int32_t age;
    };
    std::vector<Entry> entries;
};
} // namespace feattext
