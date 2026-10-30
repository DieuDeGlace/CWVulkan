#pragma once
#include "DialogueWords.h"
// Answer-panel state and layout from remaining-runtime-gaps.md section 5.6.
// Text comes from the owner's dictionary; this module supplies no dialogue art.
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace dialoguechoice {
using Word = dialoguewords::Word;
struct Row {
    std::string id;
    std::vector<Word> words;
};
struct PlacedWord {
    Word word;
    glm::vec2 at{0};
    size_t row = 0;
};
struct RowBounds {
    float top = 0, next = 0, measuredHeight = 32;
};
struct Layout {
    glm::vec2 at{0}, size{0};
    std::vector<PlacedWord> words;
    std::vector<RowBounds> rows;
    std::vector<glm::vec2> navigation;
};
struct Action {
    enum Kind { None, ChangeSpecialization, Rest } kind = None;
    int64_t speaker = 0;
    uint8_t specialization = 0;
    int coins = 0; // local debit; the host does not charge again
};
using Measure = std::function<float(const std::wstring &)>;

class Panel {
  public:
    void offer(int64_t speaker, std::vector<Row> rows);
    void clear();
    bool empty() const { return choices.empty(); }
    bool visible() const { return shown; }
    int hovered() const { return hover; }
    int64_t speaker() const { return who; }
    void updateVisibility(bool hasAddressedBubble);
    void dismissForMovement();
    Layout layout(glm::vec2 window, const Measure &measure);
    void updateHover(const Layout &, bool subtreeHit, float localMouseY);
    // Called after normal GUI dispatch on left-button release. Even an outside
    // release consumes all visible answers; labels never select an action.
    Action release(bool speakerExists, int coins, float timeMs, uint8_t specialization);

  private:
    std::vector<Row> choices;
    int64_t who = 0;
    glm::vec2 size{300, 200};
    bool shown = false;
    int hover = -1;
};
} // namespace dialoguechoice
