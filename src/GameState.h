#pragma once
// Selected Game/Interface-owned values survive the engine's render/world session
// adapter. Authority: steam-game-interface-and-input-lifetime-through-session-transitions-20261110.md.
// This does not replace or certify the complete native Game/World/entity graph.
#include "ChatLine.h"
#include "GameInput.h"
#include "GameMouse.h"
#include "NativeTerms.h"
#include "Spawned.h"

struct GameState {
    std::shared_ptr<SpawnedCreatures::PlayerStorage> player;
    nativeterms::Dictionary terms;
    bool termsAttempted=false; // One fixed-file resource attempt per retained owner.
    // Defined initial action zeros are owned engine policy; the native constructor
    // does not prove initial values for its otherwise unwritten sampled23 bytes.
    GameInput input;
    ChatLine chat;
    gamemouse::State mouse;
};
