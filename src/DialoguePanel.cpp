#include "DialoguePanel.h"
#include <algorithm>
#include <cstring>

namespace dialoguepanel {
namespace {
bool punctuation(const std::wstring &text) {
    return text.size() == 1 && std::wstring(L".-,:;!?'").find(text[0]) != std::wstring::npos;
}
size_t units(const std::wstring &text) {
    if constexpr (sizeof(wchar_t) == 2) return text.size();
    size_t count = 0;
    for (auto c : text) count += (uint32_t)c > 0xffff ? 2 : 1;
    return count;
}
std::wstring prefix(const std::wstring &text, size_t count) {
    if constexpr (sizeof(wchar_t) == 2) return text.substr(0, count);
    std::wstring out;
    for (auto c : text) {
        if (!count) break;
        const size_t width = (uint32_t)c > 0xffff ? 2 : 1;
        if (count < width) { out += (wchar_t)(0xd800 + (((uint32_t)c - 0x10000) >> 10)); break; }
        out += c;
        count -= width;
    }
    return out;
}
}
int duration(const std::vector<dialoguewords::Word> &words) {
    uint32_t count = 0;
    for (const auto &word : words) count += (uint32_t)units(word.text) + 1;
    const uint32_t bits = count * 40;
    int32_t milliseconds;
    std::memcpy(&milliseconds, &bits, sizeof milliseconds);
    return milliseconds;
}
int General::duration() const {
    return index < content.size() ? dialoguepanel::duration(content[index].words) : 0;
}
std::vector<std::string> General::append(const TalkScript &script, const Speech &speech, std::mt19937 &random,
                                          const TalkNames *names, std::optional<std::array<int32_t, 2>> loreCell) {
    auto compiled = playScript(script, speech, random, names, content);
    if (loreCell && !compiled.pages.empty()) compiled.pages.back().loreCells.push_back(*loreCell);
    content = std::move(compiled.pages);
    return std::move(compiled.unrendered);
}
std::optional<int> General::update(double elapsedMs, bool primary, bool use) {
    if (content.empty()) {
        index = 0;
        clock = displayed = 0;
        speakerId = 0;
        previousPrimary = previousUse = false;
        return {};
    }
    clock = (float)((double)clock + std::min(elapsedMs, 1000.0));
    if (!(clock > duration() + 100)) {
        previousPrimary = previousUse = false;
        return {};
    }
    const bool released = (previousPrimary && !primary) || (previousUse && !use);
    previousPrimary = primary;
    previousUse = use;
    if (!released) return {};
    ++index;
    clock = displayed = 0;
    if (index < content.size()) return 0x7b;
    content.clear();
    index = 0;
    return 0x7a;
}
General::Indicator General::indicator() const {
    if (content.empty() || !(clock > duration() + 100)) return Hidden;
    return index + 1 < content.size() ? Next : End;
}
Layout layout(const std::vector<dialoguewords::Word> &words, float elapsedMs, float frameWidth,
              const std::function<float(const std::wstring &)> &measure) {
    Layout out;
    if (!measure) return out;
    const int budget = (int)(elapsedMs / 40.f);
    int cursor = 0, top = 0;
    bool suppressGap = true;
    for (const auto &word : words) {
        if (word.text.empty()) continue;
        if (budget < out.revealed) break;
        const bool mark = punctuation(word.text);
        if (!suppressGap) cursor += mark ? 4 : 12;
        suppressGap = word.text == L"-" || word.text == L"'";
        const int advance = (int)measure(word.text) + (word.text == L"'" ? 4 : 0);
        if (!mark && cursor + advance > frameWidth - 36) {
            cursor = 0;
            top = (int)((float)top + 12.f * 1.350000023841858f);
        }
        auto shown = word;
        shown.text = prefix(word.text, std::min<size_t>(units(word.text), (size_t)(budget - out.revealed)));
        out.revealed += (int)units(shown.text) + 1;
        out.lastFlag = word.flag;
        out.runs.push_back({std::move(shown), {cursor + 12.f, top + 24.f}});
        cursor += advance;
    }
    return out;
}
Layout General::layout(const std::function<float(const std::wstring &)> &measure) const {
    return index < content.size() ? dialoguepanel::layout(content[index].words, clock, 320, measure) : Layout{};
}
std::optional<float> voice(float &displayed, const Layout &layout, std::optional<float> speakerWidth,
                           const std::function<int()> &random) {
    std::optional<float> pitch;
    if ((int)displayed / 3 != layout.revealed / 3 && random) {
        float value = speakerWidth ? .9600000381469727f / *speakerWidth : 1;
        if (layout.lastFlag == 1) value *= 1.5f + (float)random() / 32767.f;
        value += ((float)random() * .5f) / 32767.f;
        pitch = value;
    }
    displayed = (float)layout.revealed;
    return pitch;
}
std::optional<float> General::voice(const Layout &layout, std::optional<float> speakerWidth,
                                   const std::function<int()> &random) {
    return dialoguepanel::voice(displayed, layout, speakerWidth, random);
}
} // namespace dialoguepanel
