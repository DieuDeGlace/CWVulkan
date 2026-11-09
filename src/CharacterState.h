#pragma once
// Shared finite Entity operations. Reset-alone and decoder defaults are distinct.
// See the held creation and selected-character default-core contracts.
#include "NativeAI.h"
#include "NativeSaves.h"
#include "worldgen/Residents.h"

namespace characterstate {
void reset(worldgen::CreatureRecord &, nativeai::Creature &);
void copyItem(uint8_t *destination, const uint8_t *source);
void copyAppearance(uint8_t *destination, const uint8_t *source);
void copyCore(uint8_t *destination, const uint8_t *source);
// Overlays actor-owned save fields only; CharacterData/inventory/history stay separate.
void capture(const uint8_t *record, const nativeai::Creature &, nativesaves::Character &);
}
