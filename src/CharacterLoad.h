#pragma once
// Complete native load boundary from the selected-character load/reset/core-copy contracts.
// See steam-selected-character-default-core-copy-20261108.md for omitted widths.
// The caller owns the World guard; native pointer containers use the owned side model.
#include "NativeAI.h"
#include "NativeSaves.h"
#include "worldgen/Residents.h"

namespace characterload {
void apply(worldgen::CreatureRecord &, nativeai::Creature &, const nativesaves::Character &);
}
