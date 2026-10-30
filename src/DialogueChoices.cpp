#include "DialogueChoices.h"
#include <algorithm>
#include <utility>

namespace dialoguechoice {
namespace {
bool punctuation(const Word &word) {
    return word.text.size() == 1 && std::wstring(L".-,:;!?'").find(word.text[0]) != std::wstring::npos;
}
bool joinsNext(const Word &word) { return word.text == L"-" || word.text == L"'"; }
struct Flow {
    float width = 0;
    int wraps = 0;
    std::vector<glm::vec2> positions;
};
Flow flow(const Row &row, float width, float leading, const Measure &measure) {
    Flow result;
    float cursor = 0;
    for (size_t i = 0; i < row.words.size(); ++i) {
        const auto &word = row.words[i];
        const bool mark = punctuation(word);
        if (i && !joinsNext(row.words[i - 1])) cursor += mark ? 3 : 12;
        const int advance = (int)measure(word.text) + (word.text == L"'" ? 4 : 0);
        if (!mark && cursor + advance > width - 24) {
            cursor = 0;
            ++result.wraps;
        }
        result.positions.push_back({cursor + 20, result.wraps * leading + 25});
        cursor += advance;
        result.width = std::max(result.width, cursor);
    }
    return result;
}
}

void Panel::offer(int64_t speaker, std::vector<Row> rows) {
    clear();
    who = speaker;
    choices = std::move(rows);
}
void Panel::clear() { choices.clear(); hover = -1; shown = false; }
void Panel::updateVisibility(bool hasAddressedBubble) { shown = !choices.empty() && !hasAddressedBubble; }
void Panel::dismissForMovement() { if (shown) clear(); }

Layout Panel::layout(glm::vec2 window, const Measure &measure) {
    Layout out;
    if (!shown || !measure) return out;
    float width = 0, height = 20;
    for (const auto &row : choices) {
        const auto measured = flow(row, size.x, 18, measure);
        width = std::max(width, measured.width);
        height += 32 + 18 * measured.wraps;
    }
    size = {50 + width, height};
    out.size = size;
    out.at = {(window.x - size.x) / 2, window.y - 150 - size.y};
    float top = 0;
    for (size_t i = 0; i < choices.size(); ++i) {
        // Native measurement uses 18 per wrap, while drawing advances by 16.
        const auto drawn = flow(choices[i], size.x, 16, measure);
        const auto measured = flow(choices[i], size.x, 18, measure);
        const float next = top + 32 + 16 * drawn.wraps;
        out.rows.push_back({top, next, 32.f + 18 * measured.wraps});
        out.navigation.push_back({(float)(int)(size.x / 2), (float)(int)((top + next) / 2 + 8)});
        for (size_t w = 0; w < choices[i].words.size(); ++w)
            out.words.push_back({choices[i].words[w], drawn.positions[w] + glm::vec2(0, top), i});
        top = next;
    }
    return out;
}
void Panel::updateHover(const Layout &layout, bool subtreeHit, float localMouseY) {
    hover = -1;
    if (!shown || !subtreeHit) return;
    for (size_t i = 0; i < layout.rows.size(); ++i) {
        const auto &row = layout.rows[i];
        if (localMouseY >= row.top && localMouseY < row.top + row.measuredHeight)
            hover = (int)i; // later measured intervals can overlap and replace it
    }
}
Action Panel::release(bool speakerExists, int coins, float timeMs, uint8_t specialization) {
    Action action;
    if (!shown) return action;
    const std::string id = hover >= 0 && (size_t)hover < choices.size() ? choices[hover].id : "";
    action.speaker = who;
    clear();
    if (!speakerExists) return action;
    if (id == "change_specialization?") {
        action.kind = Action::ChangeSpecialization;
        action.specialization = (uint8_t)(1 - specialization);
    } else if (id == "rest_inn?" && coins > 9 && (timeMs >= 64800000 || timeMs <= 21600000)) {
        action.kind = Action::Rest;
        action.coins = 10;
    }
    return action;
}
} // namespace dialoguechoice
