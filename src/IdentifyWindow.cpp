// The identify window (docs/specs/loot-shops.md §5). See IdentifyWindow.h.
#include "IdentifyWindow.h"

#include "ScreenItems.h"

#include <algorithm>

namespace identifywindow {

int price(const Window &w) { return w.slotted ? gameshop::identifyPrice(w.item) : 0; }

bool canIdentify(const Window &w, int count, int gold) {
    return w.slotted && count > 0 && w.item[0] == gameshop::UNIDENTIFIED && price(w) <= gold;
}

Outcome identifyOnce(const Window &w, const std::function<int()> &rand) {
    Outcome o;
    o.paid = price(w);
    o.made = gameshop::identify(w.item, rand);
    return o;
}

namespace {
// Placed by CubeVulkan (the spec gives no geometry): right of the bag, as the vendor's widget
// (the bag is 400 units wide, its top 340 units above the middle); the slot in the middle near
// the top, the button at the bottom.
constexpr float BESIDE_BAG_X = 210, ABOVE_MIDDLE = 340, MARGIN = 10;
constexpr float SLOT_TOP = 45, BUTTON_FROM_BOTTOM = BUTTON_SIZE.y + BUTTON_DRAWN_EXTRA + 12;
} // namespace

Layout layout(glm::vec2 window) {
    Layout l;
    l.at = glm::vec2(window.x * 0.5f + BESIDE_BAG_X, window.y * 0.5f - ABOVE_MIDDLE);
    l.at.x = std::min(l.at.x, window.x - WINDOW_SIZE.x - MARGIN);
    l.at.y = std::max(l.at.y, MARGIN);
    l.slotAt = l.at + glm::vec2((WINDOW_SIZE.x - SLOT_SIZE.x) * 0.5f, SLOT_TOP);
    l.buttonAt = l.at + glm::vec2((WINDOW_SIZE.x - BUTTON_SIZE.x) * 0.5f, WINDOW_SIZE.y - BUTTON_FROM_BOTTOM);
    return l;
}

bool Layout::inWindow(glm::vec2 p) const { return screenitems::inside(p, at, WINDOW_SIZE); }
bool Layout::onSlot(glm::vec2 p) const { return screenitems::inside(p, slotAt, SLOT_SIZE); }
bool Layout::onButton(glm::vec2 p) const { return screenitems::inside(p, buttonAt, BUTTON_SIZE + glm::vec2(0, BUTTON_DRAWN_EXTRA)); }

Hud::Screen screen(const Layout &l, const Shown &s) {
    using namespace screenitems;
    Hud::Screen out;
    const float middle = l.at.x + WINDOW_SIZE.x * 0.5f;
    out.push_back(square(l.at + glm::vec2(4), WINDOW_SIZE - glm::vec2(8), WINDOW_SHADE));
    out.push_back(widget("blackwidget", l.at, WINDOW_SIZE));
    out.push_back(text("Identify", l.at + glm::vec2(15, 25), TITLE_COLOUR));
    out.push_back(widget("itembox", l.slotAt, SLOT_SIZE, "", s.overSlot ? glm::vec4(0.75f, 1, 1, 1) : glm::vec4(1)));
    float y = l.slotAt.y + SLOT_SIZE.y + 22;
    if (s.slotted) {
        if (!s.icon.empty())
            out.push_back(icon(s.icon, l.slotAt + glm::vec2(6), SLOT_SIZE.x - 12));
        if (s.count > 1)
            out.push_back(text(std::to_string(s.count), l.slotAt + SLOT_SIZE - glm::vec2(4, 5), glm::vec4(1), 2));
        out.push_back(text(s.name, {middle, y}, s.colour, 1));
        y += 22;
        out.push_back(text("Price: " + std::to_string(s.price) + " \xae", {middle, y}, s.price <= s.gold ? PRICE_COLOUR : REFUSED_COLOUR, 1));
    } else
        out.push_back(text("Put an item here", {middle, y}, HINT_COLOUR, 1));
    const glm::vec4 tint = !s.ready ? DISABLED_TINT : s.overButton ? HOVER_COLOUR : glm::vec4(1);
    out.push_back(widget("button", l.buttonAt, BUTTON_SIZE, "Identify", tint));
    return out;
}

} // namespace identifywindow
