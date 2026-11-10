#pragma once
// Selected Game/Interface-owned values survive the engine's render/world session
// adapter. Authority: steam-game-interface-and-input-lifetime-through-session-transitions-20261110.md.
// This does not replace or certify the complete native Game/World/entity graph.
#include "ChatLine.h"
#include "GameInput.h"
#include "GameMouse.h"

struct GameState {
    // Defined initial action zeros are owned engine policy; the native constructor
    // does not prove initial values for its otherwise unwritten sampled23 bytes.
    GameInput input;
    ChatLine chat;
    gamemouse::State mouse;
};
