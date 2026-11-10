#include "GameMouse.h"

namespace gamemouse {
bool press(Process &process, const Current &current, int32_t button) {
    const Target game = current(); // Press retains the entry Game, unlike keyboard.
    game.state->cachedY = game.guiY();
    if (uint32_t(button) <= 2) game.guiPress(button);
    process.widgetAtPress = game.widgetSelected() ? 1 : 0;
    if (game.mapShown()) {
        if (game.mapArrow(true))
            return false;
        if (game.mapArrow(false))
            return false;
    }
    game.coordinatePress(button);
    const uint32_t now = game.clock();
    const uint32_t elapsed = now - process.pressedAt; // Read after clock/provider reentry.
    process.pressedAt = now;
    if (elapsed < 200 && game.mapShown() && game.cursorHeight() > 0)
        game.selectTarget();
    game.resetMovement();
    const float before = game.maximumHealth();
    if (uint32_t(button) <= 2) game.inventoryPress(button);
    if (button == 0)
        game.secondaryPress(button);
    game.adjustHealth(before);
    game.closeChat();
    game.hideSelector();
    return true;
}

void release(Process &process, const Current &current, int32_t button) {
    const Target game = current();
    game.releasePrefix(button);
    if (uint32_t(button) <= 2) game.guiRelease(button);
    game.selectedTextRelease(button);
    if (game.widgetSelected() && process.widgetAtPress != 0 && button == 0)
        game.widgetRelease(button);
    game.coordinateRelease(button);
    game.itemRelease(button);
    game.controlsRelease(button);
}
} // namespace gamemouse
