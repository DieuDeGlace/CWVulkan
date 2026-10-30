#pragma once
// The identify window (docs/specs/loot-shops.md §5: the window at controller +0x360, its frame
// routine 0x965e0). Talking to a townsperson of class 0x83 opens it beside the inventory. The
// player puts an item from the inventory in its slot; the window shows its price (half the item's
// price, at least 1, 0x277bc0), and its button turns a type-15 item into a random weapon or
// armour piece of the item's rarity, for any class, bound to the item's region
// (gameshop::identify).
//
// The spec gives the rules, not the window's look: its geometry here is CubeVulkan's, built of
// the game's gui.plx templates like the shop windows (a black widget, an item box, a button).
#include "GameShop.h"
#include "InventorySlots.h"
#include "Hud.h"

#include <functional>
#include <string>

namespace identifywindow {

// The window's state: shown or not, and the item in its slot (a copy; the inventory's stack is
// found again by item equality, as its tab and index would point at it).
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

constexpr glm::vec2 WINDOW_SIZE{260, 260}, SLOT_SIZE{60, 60}, BUTTON_SIZE{120, 15};
// The "button" template drawn at BUTTON_SIZE reaches this much lower than its size (its frame's
// margin, measured on a capture); the button takes clicks over all of it.
constexpr float BUTTON_DRAWN_EXTRA = 47;

struct Layout {
    glm::vec2 at{0}, slotAt{0}, buttonAt{0};
    bool inWindow(glm::vec2 p) const;
    bool onSlot(glm::vec2 p) const;
    bool onButton(glm::vec2 p) const;
};
// For a window `window` interface units in size: where the vendor's widget goes, right of the bag.
Layout layout(glm::vec2 window);

// What the window shows.
struct Shown {
    bool slotted = false;
    std::string name, icon; // the slotted item's name and icon key (Hud::addIcon)
    glm::vec4 colour{1};    // its rarity's colour
    int count = 0;
    int price = 0, gold = 0;
    bool ready = false;     // canIdentify
    bool overButton = false, overSlot = false;
};
Hud::Screen screen(const Layout &layout, const Shown &shown);

} // namespace identifywindow
