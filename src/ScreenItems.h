#pragma once
// Small builders for the items of a Hud::Screen (texts, squares, icons and gui.plx templates in
// interface units), for the in-game windows that are drawn through Hud::drawScreen.
#include "Hud.h"

#include <string>

namespace screenitems {

// A narrow string as the interface's font reads it: Latin-1 (the coin is 0xae).
inline std::wstring latin1(const std::string &s) {
    std::wstring w;
    for (unsigned char c : s)
        w.push_back((wchar_t)c);
    return w;
}

// A text drawn as the game's widgets draw theirs: its outline first, then the fill. `align`:
// 0 left, 1 centred, 2 right.
inline Hud::ScreenItem text(const std::string &s, glm::vec2 at, glm::vec4 colour, int align = 0, float size = 12, float outline = 3) {
    Hud::ScreenItem t;
    t.kind = Hud::ScreenItem::Text;
    t.text = latin1(s), t.at = at, t.fontSize = size, t.outline = outline, t.color = colour, t.align = align;
    return t;
}

inline Hud::ScreenItem square(glm::vec2 at, glm::vec2 size, glm::vec4 colour) {
    Hud::ScreenItem q;
    q.kind = Hud::ScreenItem::Square;
    q.at = at, q.size = size, q.color = colour;
    return q;
}

// An icon given to Hud::addIcon, `width` units wide (0: its own size).
inline Hud::ScreenItem icon(const std::string &key, glm::vec2 at, float width) {
    Hud::ScreenItem i;
    i.kind = Hud::ScreenItem::Icon;
    i.name = key, i.at = at, i.size = glm::vec2(width, width);
    return i;
}

// A gui.plx template placed at `at` and resized to `size` (zero: its own size), its texts set to
// `label` when one is given, tinted.
inline Hud::ScreenItem widget(const std::string &name, glm::vec2 at, glm::vec2 size, const std::string &label = "",
                              glm::vec4 tint = glm::vec4(1)) {
    Hud::ScreenItem w;
    w.kind = Hud::ScreenItem::Template;
    w.name = name, w.at = at, w.size = size, w.text = latin1(label), w.color = tint;
    return w;
}

// The black widget's fill lets the world show through; a window drawing a picture or small
// texts on it is darkened under it first.
inline const glm::vec4 WINDOW_SHADE{0.05f, 0.05f, 0.07f, 0.6f};

inline bool inside(glm::vec2 p, glm::vec2 at, glm::vec2 size) {
    return p.x >= at.x && p.y >= at.y && p.x < at.x + size.x && p.y < at.y + size.y;
}

// The interface's colours for these windows: the panels' titles, prices, refusals and a hovered
// button (the menus' hover colour).
inline const glm::vec4 TITLE_COLOUR{1, 0.8f, 0.5f, 1}, PRICE_COLOUR{1, 0.9f, 0, 1}, REFUSED_COLOUR{1, 0.25f, 0.25f, 1},
    HOVER_COLOUR{0.2f, 1, 1, 1}, HINT_COLOUR{0.6f, 0.6f, 0.7f, 1}, DISABLED_TINT{0.55f, 0.55f, 0.55f, 1};

} // namespace screenitems
