#pragma once
#include "alphagen/Zone.h"
namespace alphagen {
// Base-field overview for sky/map consumers: H, not the final voxel surface S.
// Collision and character placement must read the composed voxel columns.
struct TerrainSample {
    float warmth = .5f, moisture = .5f, sea = 0, fire = 0;
    float height = 0, mountain = 0;
    Block colour;
};
} // namespace alphagen
