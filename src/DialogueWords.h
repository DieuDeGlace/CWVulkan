#pragma once
// Dictionary word metadata and append rules: steam-dialogue-word-tokens.md.
#include <cstdint>
#include <set>
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace dialoguewords {
struct Style {
    glm::vec4 colour{1};
    uint8_t flag = 0;
};
struct Word {
    std::wstring text;
    glm::vec4 colour{1};
    uint8_t flag = 0;
};
glm::vec4 itemColour(int type, int subtype, int rarity);
glm::vec4 landColour(int biome);
Style tagged(Style inherited, const std::set<std::string> &tags);
std::wstring wide(const std::string &utf8);

class Builder {
  public:
    std::vector<std::vector<Word>> pages{1};
    // Substitutions split only on whitespace. Literal dictionary text first
    // separates its native ASCII punctuation, preserving the two source kinds.
    void fragment(const std::wstring &, Style);
    void literal(const std::wstring &, Style);
    void nextPage() { pages.emplace_back(); }
};
} // namespace dialoguewords
