#pragma once
// Identification state and transformed footer geometry, from steam-identification-ui.md.
// The owner blackwidget/itemframe tree is instantiated and drawn by HudIdentification.cpp.
#include "GameShop.h"
#include "InventorySlots.h"
#include <glm/glm.hpp>
#include <array>

#include <functional>
#include <string>

namespace identifywindow {

// Native selection is a live pair; item/slotted are presentation scratch. Legacy
// recording sessions can still supply a copied item with put().
struct Window {
    bool open = false;
    bool slotted = false;
    inventoryslots::Index selection;
    bool visibleAtDown = false;
    int actionGate = 1;
    // Native selection retains indices across close/reopen and resolves current content, never an identity match.
    void bind(inventoryslots::Index index) { selection = index; }
    bool resolve(const inventoryslots::Inventory &);
    bool acceptsRelease(int button) const { return open && visibleAtDown && button == 0; }
    gameshop::Item item{};
    void show() { open = true, slotted = false; }
    void close() { open = false, slotted = false; }
    void put(const gameshop::Item &it) { item = it, slotted = true; }
    void takeOut() { slotted = false; }
};

// The price the window shows (0x277bc0): half the slotted item's price, at least 1; 0 with an
// empty slot.
int price(const Window &w);

// Whether the button does something when pressed (0x965e0's tests besides the press itself): an
// item in the slot whose stack is not empty (`count`), of type 0x0f, and its price within `gold`.
bool canIdentify(const Window &w, int count, int gold);

// One press of the button when canIdentify holds (0x965e0 steps 1-4): what is paid and what is
// made. The caller takes the gold, takes one from the slotted stack (emptying the slot when none
// is left), adds the result to the inventory, plays SOUND_IDENTIFIED and announces it.
struct Outcome {
    int paid = 0;
    gameshop::Item made{};
};
Outcome identifyOnce(const Window &w, const std::function<int()> &rand);

// --- The window on screen -------------------------------------------------------------------

constexpr glm::vec2 WINDOW_SIZE{350, 285};
struct Layout {
    bool valid = false;
    glm::vec2 size{0}, slotSize{0};
    glm::mat4 frameToScreen{1}, nodeToScreen{1}, textToScreen{1}, slotToScreen{1};
    glm::vec2 local(glm::vec2 screen) const;
    bool inWindow(glm::vec2 screen) const;
    bool onSlot(glm::vec2 screen) const;
    bool onButton(glm::vec2 screen, bool resolved = true) const;
    bool onGoodbye(glm::vec2 screen) const;
    std::array<glm::vec2, 2> navigation() const;
};

// What the window shows.
struct Shown {
    bool slotted = false;
    std::string name, icon, regionDescription; // the slotted item's name and icon key (Hud::addIcon)
    glm::vec4 colour{1}, regionColour{1};      // its rarity's colour
    int count = 0;
    int price = 0, gold = 0;
    bool ready = false;     // canIdentify
    bool overButton = false, overSlot = false, overGoodbye = false;
};

} // namespace identifywindow
