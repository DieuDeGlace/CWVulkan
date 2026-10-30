#include "DialogueWords.h"
#include <algorithm>
#include <utility>
#include <map>
#include <sstream>

namespace dialoguewords {
glm::vec4 itemColour(int type, int subtype, int rarity) {
    if (type == 0x17 || type == 0x18) return {1,.6f,.25f,1};
    if (type == 0x15) return {.7f,1,.25f,1};
    if (type == 0) return {1,1,1,1};
    if (type == 0x16 || (type == 1 && subtype == 13)) return {.5f,1,1,1};
    if (type == 0x0d) return {.5f,.1f,1,1};
    static const glm::vec4 palette[] = {{.7f,.7f,.7f,1}, {1,1,1,1}, {0,1,0,1},
        {.3f,.45f,1,1}, {.55f,.2f,1,1}, {1,1,0,1}};
    return palette[std::clamp(rarity, -1, 4) + 1];
}
glm::vec4 landColour(int biome) {
    static const glm::vec4 colours[] = {{.5f,1,.25f,1}, {.4f,.65f,1,1}, {.5f,1,.25f,1}, {.5f,.9f,.1f,1},
        {.2f,.8f,.2f,1}, {.4f,.3f,.9f,1}, {.6f,.7f,1,1}, {.7f,.7f,.7f,1}, {.3f,.7f,1,1},
        {1,.9f,.2f,1}, {.9f,1,.2f,1}, {.7f,.9f,1,1}, {.5f,.1f,1,1}, {.7f,.2f,.2f,1}};
    return biome >= 0 && biome < 14 ? colours[biome] : glm::vec4(1);
}
Style tagged(Style style, const std::set<std::string> &tags) {
    static const std::map<std::string, glm::vec4> colours = {
        {"creature", {1,.3f,.3f,1}}, {"name", {1,.3f,.3f,1}},
        {"object", {1,.5f,.3f,1}}, {"zone", {.6f,.5f,1,1}},
        {"stress", {0,1,1,1}}, {"realm", {1,.9f,.5f,1}}, {"special", {1,.9f,.5f,1}},
        {"number", {.3f,1,.3f,1}}, {"green", {0,1,0,1}}, {"blue", {.3f,.45f,1,1}},
        {"purple", {.55f,.2f,1,1}}, {"yellow", {1,1,0,1}}
    };
    for (const auto &tag : tags) {
        if (tag == "shake") style.flag = 1;
        if (const auto found = colours.find(tag); found != colours.end()) style.colour = found->second;
    }
    return style;
}
std::wstring wide(const std::string &utf8) {
    std::wstring out;
    for (size_t at = 0; at < utf8.size();) {
        const auto first = (uint8_t)utf8[at];
        uint32_t value = 0, minimum = 0;
        size_t count = 0;
        if (first < 0x80) value = first, count = 1;
        else if ((first & 0xe0) == 0xc0) value = first & 0x1f, minimum = 0x80, count = 2;
        else if ((first & 0xf0) == 0xe0) value = first & 0x0f, minimum = 0x800, count = 3;
        else if ((first & 0xf8) == 0xf0) value = first & 7, minimum = 0x10000, count = 4;
        bool valid = count && count <= utf8.size() - at;
        for (size_t n = 1; valid && n < count; ++n) {
            const auto byte = (uint8_t)utf8[at + n];
            valid = (byte & 0xc0) == 0x80;
            value = (value << 6) | (byte & 0x3f);
        }
        valid = valid && value >= minimum && value <= 0x10ffff && !(value >= 0xd800 && value <= 0xdfff);
        if (!valid) {
            out += L'\ufffd';
            ++at;
            continue;
        }
        at += count;
        if constexpr (sizeof(wchar_t) == 2) {
            if (value > 0xffff) {
                value -= 0x10000;
                out += (wchar_t)(0xd800 + (value >> 10));
                out += (wchar_t)(0xdc00 + (value & 0x3ff));
                continue;
            }
        }
        out += (wchar_t)value;
    }
    return out;
}
void Builder::fragment(const std::wstring &text, Style style) {
    std::wistringstream stream(text);
    auto &words = pages.back();
    while (stream.good()) {
        std::wstring token;
        stream >> token;
        // The native helper tests the stream before extraction, including the
        // empty token from an empty or trailing-whitespace fragment.
        if (!words.empty() && words.back().text == L"=") {
            words.pop_back();
            if (!words.empty()) words.back().text += token;
        } else if (!words.empty() && (token == L"'" ||
                   (!words.back().text.empty() && words.back().text.back() == L'\''))) {
            words.back().text += token;
        } else {
            words.push_back({std::move(token), style.colour, style.flag});
        }
    }
}
void Builder::literal(const std::wstring &text, Style style) {
    std::wstring run;
    auto flush = [&] { if (!run.empty()) { fragment(run, style); run.clear(); } };
    for (wchar_t c : text) {
        const bool mark = std::wstring(L"!'(),-./:;=?").find(c) != std::wstring::npos;
        if (c == L' ' || c == L'\n' || c == L'\t' || c == L'\0' || mark) {
            flush();
            if (mark) fragment(std::wstring(1, c), style);
            if (c == L'\0') break;
        } else run += c;
    }
    flush();
}
} // namespace dialoguewords
