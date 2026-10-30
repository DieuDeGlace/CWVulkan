#pragma once
#include "SteamPointLights.h"
#include "NativeRules.h"

namespace steampoints {
// One live world-list entry, from steam-scene-lighting.md sections3.7–3.10.
// Pose anchors are the record's cached render positions, not new geometry.
void collectCreature(const uint8_t *record, const nativerules::HasBuff &, const Frame &, std::vector<Light> &out);
}
