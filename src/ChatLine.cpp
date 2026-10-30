#include "ChatLine.h"
#include <algorithm>
#include <cstdlib>
#include <utility>

std::optional<std::u16string> ChatLine::enter() {
    if (!open) {
        open = true;
        return std::nullopt;
    }
    open = false;
    auto sent = std::exchange(line, {});
    caret = selection = 0;
    return sent.empty() ? std::nullopt : std::optional<std::u16string>(std::move(sent));
}

void ChatLine::clampSelection() {
    const int length = (int)line.size();
    caret = std::clamp(caret, 0, length);
    const int before = caret;
    if (before + selection < 0) selection = -before;
    if (before + selection > length) caret = length - selection;
}

void ChatLine::eraseSelection() {
    clampSelection();
    const int first = std::min(caret, caret + selection);
    line.erase((size_t)first, (size_t)std::abs(selection));
    caret = first;
    selection = 0;
}

void ChatLine::type(const std::string &lowBytes) {
    if (!open) return;
    for (unsigned char byte : lowBytes) {
        // Game sign-extends its low byte before inserting a UTF-16 code unit.
        // Received network strings bypass this input-only narrowing.
        const auto unit = (char16_t)(byte < 128 ? (int)byte : (int)byte - 256);
        if (unit < 32) continue;
        if (selection) eraseSelection();
        clampSelection();
        line.insert((size_t)caret++, 1, unit);
    }
}

void ChatLine::key(Key key, bool shift) {
    if (!open) return;
    clampSelection();
    if (key == Key::Delete || key == Key::Backspace) {
        if (selection) eraseSelection();
        else if (key == Key::Delete) line.erase((size_t)caret, 1);
        else if (caret > 0) line.erase((size_t)--caret, 1);
        return;
    }
    int next = caret;
    switch (key) {
    case Key::Left: --next; break;
    case Key::Right: ++next; break;
    case Key::Home: next = 0; break;
    case Key::End: next = (int)line.size(); break;
    default: break;
    }
    if (shift) selection += caret - next;
    else selection = 0;
    caret = next;
    clampSelection();
}

void ChatLine::append(const std::u16string &text, Colour colour, float frameWidth, const Measure &measure) {
    if (history.empty()) history.emplace_back();
    const auto width = [&](const std::wstring &word) { return word == L" " ? 5.0f : measure(word); };
    float used = 0;
    for (const auto &word : history.back()) used += width(word.text);
    bool glue = !history.back().empty() && !history.back().back().text.empty() && history.back().back().text != L" ";
    const int limit = (int)(frameWidth - 30);
    auto appendToken = [&](std::wstring token) {
        if (token == L"\r" || token == L"\n") {
            history.emplace_back();
            used = 0;
            return; // Explicit row breaks do not consume the glue allowance.
        }
        const float incomingWidth = (float)(int)width(token);
        if (!(used + incomingWidth < (float)limit)) {
            if (glue) glue = false;
            else {
                history.emplace_back();
                used = incomingWidth;
                // A discarded wrapping space still counts as five until a new
                // append recomputes the actual stored row width.
                if (token == L" ") return;
                history.back().push_back({std::move(token), colour});
                return;
            }
        }
        used += incomingWidth;
        history.back().push_back({std::move(token), colour});
    };
    std::wstring token;
    for (char16_t unit : text) {
        if (unit == u' ' || unit == u'\n' || unit == u'\r') {
            if (!token.empty()) appendToken(std::exchange(token, {}));
            appendToken(std::wstring(1, (wchar_t)unit));
        } else token.push_back((wchar_t)unit);
    }
    if (!token.empty()) appendToken(std::move(token));
}

void ChatLine::trim(float frameHeight) {
    const int keep = std::max(0, (int)(frameHeight / 18));
    while (history.size() > (size_t)keep) history.pop_front();
}

void ChatLine::receive(std::optional<std::u16string> sender, std::u16string body) {
    incoming.push_back({std::move(sender), std::move(body)});
}

void ChatLine::receive(int64_t sender, std::u16string body) {
    incoming.push_back({{}, std::move(body), sender, true});
}

void ChatLine::consumeIncoming(float frameWidth, const Measure &measure, const Name &name) {
    if (incoming.empty()) return;
    auto entry = std::move(incoming.front());
    incoming.pop_front();
    if (entry.resolve) entry.sender = entry.id >= 0 && name ? name(entry.id) : std::nullopt;
    if (entry.sender) append(*entry.sender + u": ", {0, 255, 255}, frameWidth, measure);
    append(entry.body + u"\n", {255, 255, 255}, frameWidth, measure);
}
