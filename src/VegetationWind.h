#pragma once
// How the game moves its plants in the wind (docs/specs/vegetation-wind.md).
//
// No shader moves anything and no terrain block ever moves (tree crowns stand still). A zone
// model sways when bit 2 (value 4) of its record's flags is set: each frame its world matrix
// gets a shear that leaves the base where it is and moves the top sideways, by up to 0.2 block
// along y and 0.16 block along x whatever the plant's size. The only inputs are the world's
// millisecond clock and a per-model phase (its place in the zone's model list); there is no
// wind direction, gust or distance fade. The Steam release also swings models flagged 0x10 or
// 0x20 from their top, like a hanging lantern (§4.4).
#include <glm/glm.hpp>
#include <cstdint>

namespace vegetationwind {

// Bits of a zone model record's flags (+0x38).
namespace modelflag {
constexpr int32_t OWN_LIGHT = 0x1;   // shines, and ignores the light where it stands
constexpr int32_t SWAYS = 0x4;       // sways in the wind (§3.1)
constexpr int32_t SWINGS_Y = 0x10;   // Steam: swings about the y axis from its top (§4.4)
constexpr int32_t SWINGS_X = 0x20;   // Steam: the same about the x axis
} // namespace modelflag

// Steam shares its sway between 12 phase groups by list position; Alpha uses the position itself
// (§4.2, §4.3). Either looks the same.
constexpr int STEAM_PHASE_GROUPS = 12;
int phaseIndex(int listPosition, bool steamLook);

// The sideways shift of a swaying model's top (z = H), in blocks, at world time `tMs` for phase
// index `phase` (§4.1). A vertex at height z voxels moves by this times z / H.
glm::vec2 topShift(double tMs, int phase);

// Steam's hinged swing (§4.4): the angle in degrees for the model at list position n.
float swingDegrees(double tMs, int listPosition);

// Where a zone model is and what it is.
struct Placement {
    glm::vec3 position{0};   // its base (blocks)
    float yawRadians = 0;    // about the vertical axis
    float size = 1;          // blocks per voxel
    float heightVoxels = 0;  // H, the model's height in voxels
    int32_t flags = 0;       // the record's flags (+0x38)
    int listPosition = 1;    // 1-based place in the zone's model list
};

// The model's world matrix at world time `tMs`, for a mesh whose voxels are centred on x and y
// with z from 0 at the base: turned by the yaw, [Steam: swung], swayed, scaled, placed (§4).
glm::mat4 modelMatrix(const Placement &p, double tMs, bool steamLook);

} // namespace vegetationwind
