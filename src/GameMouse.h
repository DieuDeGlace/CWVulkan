#pragma once
// Ordered Game mouse boundaries from the complete independent mouse-owner and
// clock/snapshot/widget contracts. Scene/item/scalar providers belong to the Game.
#include <cstdint>
#include <functional>

namespace gamemouse {
struct Process {
    uint32_t pressedAt = 0; // Ordinary PE zero fill; no Game-constructor reset.
    uint8_t widgetAtPress = 0;
};
struct State {
    uint32_t cachedY = 0; // Per-Interface constructor word; preserve press-copy bits.
};

// The caller supplies one normally live receiver graph. A copied set of ports
// retains that Game across GUI callbacks; each provider fetches its current fields.
// This is not a lease for arbitrary node/actor deletion or cross-thread mutation.
struct Target {
    State *state = nullptr;
    std::function<uint32_t()> guiY;
    std::function<void(int32_t)> guiPress, guiRelease;
    std::function<bool()> widgetSelected, mapShown;
    std::function<bool(bool first)> mapArrow;
    std::function<void(int32_t)> coordinatePress;
    std::function<uint32_t()> clock;
    std::function<int64_t()> cursorHeight;
    std::function<void()> selectTarget, resetMovement;
    std::function<float()> maximumHealth;
    std::function<void(int32_t)> inventoryPress, secondaryPress;
    std::function<void(float)> adjustHealth;
    std::function<void()> closeChat, hideSelector;

    std::function<void(int32_t)> releasePrefix, selectedTextRelease, widgetRelease,
                                  coordinateRelease, itemRelease, controlsRelease;
};
using Current = std::function<Target()>;

// False is one of the two post-GUI map-arrow returns; GUI/snapshot work remains.
bool press(Process &, const Current &, int32_t button);
void release(Process &, const Current &, int32_t button);
} // namespace gamemouse
