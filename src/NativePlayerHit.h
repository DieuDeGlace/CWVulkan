#pragma once
// The client frame's received-hit consumer, distinct from the host's NativeHit.
// Written from docs/specs/remaining-runtime-gaps.md §1.
#include "NativeAI.h"
#include "NativeHit.h"
#include "SoundRules.h"

namespace nativeplayerhit {
struct Result {
    bool deathEffect = false; // strictly negative ordinary subtraction, not exact zero
    std::vector<std::array<uint8_t, 0x30>> buffNotices;
    std::vector<soundrules::RequestRecord> sounds;
};
// Damage is already reduced by the hit producer. This must not apply armor again.
// The frame queues a death candidate only for the returned strict-negative flag.
Result apply(nativeai::Creature &player, const nativehit::Number &number);
} // namespace nativeplayerhit
