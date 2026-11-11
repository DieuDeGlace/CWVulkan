#pragma once
// Selected Game/Interface-owned values survive the engine's render/world session
// adapter. Authority: steam-game-interface-and-input-lifetime-through-session-transitions-20261110.md.
// This does not replace or certify the complete native Game/World/entity graph.
#include "ChatLine.h"
#include "GameInput.h"
#include "GameMouse.h"
#include "NativeTerms.h"
#include "Spawned.h"
#include "WorldSelection.h"

struct GameState {
    std::shared_ptr<SpawnedCreatures::PlayerStorage> player;
    nativeterms::Dictionary terms;
    bool termsAttempted=false; // One fixed-file resource attempt per retained owner.
    // Defined initial action zeros are owned engine policy; the native constructor
    // does not prove initial values for its otherwise unwritten sampled23 bytes.
    GameInput input;
    ChatLine chat;
    gamemouse::State mouse;
    // Requested World pair (seed, then name). The World's applied pair keeps its
    // start-up value; the update gate on a mismatch is not wired until its visible
    // result is verified (steam-pending-source-selection-and-world-reconfiguration-20261110.md, U1).
    worldselection::Pair requestedWorld;
};
