// The identify window (docs/specs/loot-shops.md Â§5). See IdentifyWindow.h.
#include "IdentifyWindow.h"

#include <glm/gtc/matrix_inverse.hpp>

#include <algorithm>

namespace identifywindow {

bool Window::resolve(const inventoryslots::Inventory &inventory) {
    const auto *stack = inventory.slot(selection);
    slotted = stack && stack->count != 0 && stack->item[0] == gameshop::UNIDENTIFIED;
    if (slotted)
        item = stack->item; // presentation scratch; selection remains the live pair
    return slotted;
}

int price(const Window &w) { return w.slotted ? gameshop::identifyPrice(w.item) : 0; }

bool canIdentify(const Window &w, int count, int gold) {
    return w.slotted && w.actionGate == 1 && count != 0 && w.item[0] == gameshop::UNIDENTIFIED && price(w) <= gold;
}

Outcome identifyOnce(const Window &w, const std::function<int()> &rand) {
    Outcome o;
    o.paid = price(w);
    o.made = gameshop::identify(w.item, rand);
    return o;
}

namespace {
glm::vec2 project(const glm::mat4 &matrix, glm::vec2 point) {
    const auto p = matrix * glm::vec4(point, 0, 1);
    return glm::vec2(p) / p.w;
}
} // namespace
glm::vec2 Layout::local(glm::vec2 screen) const {
    // Native manual footer picking uses the frame origin but the inverse node's linear part.
    const auto delta = screen - project(frameToScreen, {0, 0});
    const auto inverse = glm::inverse(nodeToScreen);
    return {delta.x * inverse[0][0] + delta.y * inverse[1][0], inverse[0][1] * delta.x + inverse[1][1] * delta.y};
}
bool Layout::inWindow(glm::vec2 screen) const {
    if (!valid)
        return false;
    const auto p = local(screen);
    return p.x >= 0 && p.y >= 0 && p.x < size.x && p.y < size.y;
}
bool Layout::onSlot(glm::vec2 screen) const {
    if (!valid)
        return false;
    const auto p = project(glm::inverse(slotToScreen), screen);
    return p.x >= 0 && p.y >= 0 && p.x < slotSize.x && p.y < slotSize.y;
}
bool Layout::onButton(glm::vec2 screen, bool resolved) const {
    if (!valid || !resolved)
        return false;
    const auto p = local(screen);
    return p.x > 0 && p.x < size.x * .5f && p.y > size.y - 30 && p.y < size.y;
}
bool Layout::onGoodbye(glm::vec2 screen) const {
    if (!valid)
        return false;
    const auto p = local(screen);
    return p.x > size.x * .5f && p.x < size.x && p.y > size.y - 30 && p.y < size.y;
}
std::array<glm::vec2, 2> Layout::navigation() const {
    const float y = (float)(int32_t)(size.y - 20);
    return {glm::vec2(glm::ivec2(project(textToScreen, {(float)(int32_t)(size.x / 3), y}))),
            glm::vec2(glm::ivec2(project(textToScreen, {(float)(int32_t)((size.x * 2) / 3), y})))};
}
} // namespace identifywindow
