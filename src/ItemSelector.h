#pragma once
// Ring Menu state and canonical candidate identity (steam-inventory-slot-lifecycle, section9).
#include "InventorySlots.h"
#include <cstdint>
#include <optional>
#include <vector>
#include <glm/vec4.hpp>

namespace itemselector {
std::vector<inventoryslots::Index> candidates(const inventoryslots::Inventory &);
size_t wrappedIndex(int32_t runningIndex, size_t candidateCount);
glm::vec4 candidateColour(const nativesaves::Item &); // category1 candidates, including subtype13's fixed cyan
struct Input {
    bool toggle = false, allowToggle = true, forward = false, back = false;
    bool left = false, right = false, fullMap = false;
    float stickX = 0, stickY = 0;
};
struct Update {
    std::optional<size_t> captionIndex; // resolved before this update's left/right changes
    int selectionSounds = 0;
};
// The repeat arm/counters are process operands, independent of one selector's
// index and animation. Standalone callers can keep the local compatibility state.
struct Repeat { uint8_t armed=0; int32_t left=0,right=0; };
class Selector {
  public:
    bool shown = false;
    int32_t index = 0;
    float angle = 0, radius = 0;
    Update frame(const Input &, size_t candidateCount, double elapsedMilliseconds, float guiScale);
    Update frame(const Input &, size_t candidateCount, double elapsedMilliseconds, float guiScale, Repeat &);
    void hide() { shown = false; } // hidden-state animation reset belongs to the frame update

  private:
    bool previousToggle = false;
    Repeat localRepeat;
};
class SoundEdges {
  public:
    int sample(bool ordinaryUi, bool selectorShown);

  private:
    bool previous = false;
};
} // namespace itemselector
