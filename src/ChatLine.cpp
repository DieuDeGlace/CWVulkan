// The chat's input line. See ChatLine.h.
#include "ChatLine.h"

#include "ScreenItems.h"

std::optional<std::u16string> ChatLine::enter() {
    if (!open) {
        open = true;
        line.clear();
        return std::nullopt;
    }
    open = false;
    std::u16string sent = std::move(line);
    line.clear();
    if (sent.empty())
        return std::nullopt;
    return sent;
}

void ChatLine::cancel() {
    open = false;
    line.clear();
}

void ChatLine::type(const std::string &characters) {
    if (!open)
        return;
    for (unsigned char c : characters)
        if (c >= 32 && c != 127)
            line.push_back((char16_t)c);
}

void ChatLine::backspace() {
    if (open && !line.empty())
        line.pop_back();
}

Hud::Screen ChatLine::screen(glm::vec2 window) const {
    Hud::Screen out;
    if (!open)
        return out;
    // The text as the interface's font draws it (Latin-1; anything else shows as '?'), with a
    // caret after it. (The template keeps a field's own size; its text is set whole.)
    // A long line shows its end, as much as the field holds.
    std::string shown;
    for (char16_t c : line)
        shown.push_back(c < 256 ? (char)c : '?');
    if (shown.size() > SHOWN_CHARACTERS)
        shown.erase(0, shown.size() - SHOWN_CHARACTERS);
    shown.push_back('_');
    out.push_back(screenitems::widget("edit", {FIELD_LEFT, window.y - FIELD_FROM_BOTTOM}, FIELD_SIZE, shown));
    return out;
}
