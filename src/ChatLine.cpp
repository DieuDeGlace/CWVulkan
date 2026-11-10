#include "ChatLine.h"
#include <algorithm>
#include <cstdlib>
#include <utility>

std::optional<std::u16string> ChatLine::enter() {
    std::optional<std::u16string> sent;
    keyDown(0x0D,false,[&](const std::u16string &text){sent=text;});
    return sent;
}

void ChatLine::clampSelection() {
    const int length=(int)line.size();
    if (caret<0) caret=0;
    if (caret>length) caret=length;
    if (int64_t(caret)+selection<0) selection=-caret;
    if (int64_t(caret)+selection>length) caret=length-selection;
}

void ChatLine::eraseSelection() {
    clampSelection();
    if (caret>(int)line.size()) return;
    const int first=selection<0 ? caret+selection : caret;
    const int count=selection==0 ? 1 : std::abs(selection);
    line.erase((size_t)first,(size_t)count);
    if (selection<0) caret+=selection;
    selection=0;
}

void ChatLine::type(const std::string &lowBytes) {
    if (!open) return;
    for (unsigned char byte : lowBytes) {
        clampSelection(); // precedes even a rejected control unit
        const auto unit=(char16_t)(byte<128 ? int(byte) : int(byte)-256);
        if (unit<32) continue;
        if (selection) eraseSelection();
        clampSelection();
        line.insert((size_t)caret++,1,unit);
    }
}

void ChatLine::editKey(uint8_t key,bool shift) {
    switch (key) {
    case 0x25: // Left: an edge preserves the prior selection.
        if (caret>0) {--caret;if (shift) ++selection;else selection=0;}
        break;
    case 0x27:
        if (caret<(int)line.size()) {++caret;if (shift) --selection;else selection=0;}
        break;
    case 0x24:
        if (shift) selection+=caret;else selection=0;
        caret=0;
        break;
    case 0x23:
        if (shift) selection+=caret-(int)line.size();else selection=0;
        caret=(int)line.size();
        break;
    case 0x2E:
        eraseSelection();
        break;
    case 0x08:
        if (selection) eraseSelection();
        else if (caret>0) {line.erase((size_t)(caret-1),1);--caret;}
        break;
    default: break;
    }
}

void ChatLine::keyDown(uint8_t key,bool shift,const std::function<void(const std::u16string &)> &submit) {
    if (!open) {
        if (key==0x0D) open=true;
        return;
    }
    if (key==0x0D) {
        ++redrawRevision;
        open=false;
        if (!line.empty() && submit) {
            const auto text=line; // safe engine-owned argument across a synchronous callback
            submit(text);
        }
        line.clear(); // after the callback, including a reentrant text edit
    }
    clampSelection();
    ++redrawRevision;
    editKey(key,shift);
}

void ChatLine::key(Key key,bool shift) {
    clampSelection();
    ++redrawRevision;
    if (!open) return;
    const uint8_t code=key==Key::Left ? 0x25 : key==Key::Right ? 0x27 : key==Key::Home ? 0x24 :
                       key==Key::End ? 0x23 : key==Key::Delete ? 0x2E : 0x08;
    editKey(code,shift);
}

void ChatLine::append(const std::u16string &text, Colour colour, float frameWidth, const Measure &measure) {
    if (history.empty())
        history.emplace_back();
    const auto width = [&](const std::wstring &word) { return word == L" " ? 5.0f : measure(word); };
    float used = 0;
    for (const auto &word : history.back())
        used += width(word.text);
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
            if (glue)
                glue = false;
            else {
                history.emplace_back();
                used = incomingWidth;
                // A discarded wrapping space still counts as five until a new
                // append recomputes the actual stored row width.
                if (token == L" ")
                    return;
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
            if (!token.empty())
                appendToken(std::exchange(token, {}));
            appendToken(std::wstring(1, (wchar_t)unit));
        } else
            token.push_back((wchar_t)unit);
    }
    if (!token.empty())
        appendToken(std::move(token));
    ++redrawRevision; // Successful empty append still invalidates the retained Widget surface.
}

void ChatLine::trim(float frameHeight) {
    const int keep = std::max(0, (int)(frameHeight / 18));
    while (history.size() > (size_t)keep)
        history.pop_front();
}

void ChatLine::receive(std::optional<std::u16string> sender, std::u16string body) {
    incoming.push_back({std::move(sender), std::move(body)});
}

void ChatLine::receive(int64_t sender, std::u16string body) {
    incoming.push_back({{}, std::move(body), sender, true});
}

void ChatLine::consumeIncoming(float frameWidth, const Measure &measure, const NameResolver &name) {
    if (incoming.empty())
        return;
    auto entry = std::move(incoming.front());
    incoming.pop_front();
    if (entry.resolve)
        entry.sender = entry.id >= 0 && name ? name(entry.id) : std::nullopt;
    if (entry.sender)
        append(*entry.sender + u": ", {0, 255, 255}, frameWidth, measure);
    append(entry.body + u"\n", {255, 255, 255}, frameWidth, measure);
}
