#pragma once
// Received-feat display only: steam-chat-feat-presentation.md section 4.
// Origination and the descriptor's meaning are deliberately outside this store.
#include "SceneVisibility.h"
#include <array>
#include <cstdint>
#include <optional>
#include <vector>
namespace feattext {
using Point = std::array<int64_t, 3>;
constexpr float FONT_SIZE = 30, OUTLINE = 3.75f;
constexpr int SOUND = 0x1f;
struct Label {
    glm::vec2 at{};
    glm::vec4 fill{}, outline{};
};
class Store {
  public:
    void received(Point physical) { entries.push_back({physical, 0}); }
    std::vector<Label> visit(const std::optional<scenevisibility::Projection> &, glm::vec2 viewport, int32_t elapsedMs);
    size_t size() const { return entries.size(); }

  private:
    struct Entry {
        Point physical;
        int32_t age;
    };
    std::vector<Entry> entries;
};
} // namespace feattext
