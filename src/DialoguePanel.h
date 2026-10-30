#pragma once
// Persistent general dialogue, distinct from NPC bubbles and answer choices.
// Behavior: remaining-runtime-gaps.md 5.7; steam-dialogue-panel-routing.md.
#include "TalkScripts.h"
#include <functional>
#include <optional>

namespace dialoguepanel {
struct Run {
    dialoguewords::Word word;
    glm::vec2 at{0};
};
struct Layout {
    std::vector<Run> runs;
    int revealed = 0;
    uint8_t lastFlag = 0;
};
class General {
  public:
    const std::vector<TalkPlay::Page> &pages() const { return content; }
    void applyCompilation(std::vector<TalkPlay::Page> pages) { content = std::move(pages); }
    bool visible() const { return !content.empty(); }
    float elapsed() const { return clock; }
    int duration() const;
    size_t page() const { return index; }
    int64_t speaker() const { return speakerId; }
    // Update uses the native capped frame duration, not the physics substep.
    std::optional<int> update(float elapsedMs, bool primary, bool use);
    Layout layout(const std::function<float(const std::wstring &)> &measure) const;
    std::optional<float> voice(const Layout &, std::optional<float> speakerWidth, const std::function<int()> &random);
    enum Indicator { Hidden, Next, End };
    Indicator indicator() const;

  private:
    std::vector<TalkPlay::Page> content;
    size_t index = 0;
    float clock = 0, displayed = 0;
    bool previousPrimary = false, previousUse = false;
    int64_t speakerId = 0; // phrase compilation does not assign/reset this field
};
} // namespace dialoguepanel
