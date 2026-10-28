#pragma once
// The game's creature pose (0x17ab0, docs/specs/creature-pose*.md): from what a creature is
// doing it eases the pose fields of its animation state (creature+0xc38) toward the pose of
// that action, builds the 18 part matrices kept in that state, moves the weapon trails and
// lists the models to draw. CubeVulkan keeps a game-layout creature record per actor and runs
// this on it, so every race, class and ability is posed the way the game poses it.
#include "NativeRules.h"

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

namespace nativepose {

constexpr size_t CREATURE_SIZE = 0x15f8;
constexpr size_t ANIM = 0xc38; // the animation state inside the creature record

// A model as the pose reads it: its size in voxels (the game's model +0x88, +0x8c, +0x90).
struct Model {
    int32_t size[3] = {0, 0, 0};
    const void *user = nullptr; // the caller's own model
};

// Where the pose gets its models: the world's model table by id (0x17190) and an item's
// model (0x173b10). The item is a pointer into the creature record.
struct Models {
    std::function<const Model *(int id)> byId;
    std::function<const Model *(const uint8_t *item)> forItem;
};

// The kinds of draw call (Draw::kind).
constexpr int PLAIN_DRAW = 0, WEAPON_EFFECT_DRAW = 1, MIRRORED_DRAW = 2;

// One model drawn: the model, its matrix (column-major, the game's), the tint and the draw
// call's five numbers (alpha, an outline value, glow, a fade value, depth).
struct Draw {
    const Model *model = nullptr;
    float matrix[16];
    float tint[4];
    float a = 0, b = 0, c = 0, d = 0, e = 0;
    // 0: a plain draw; 1: a weapon's effect (0xeec50) for the item at `item`, with model
    // 0xa46; 2: a model drawn mirrored (0xee990: the right hand and foot).
    int kind = 0;
    const uint8_t *item = nullptr;
};

// The sparks of pose state 0x61: their places relative to the creature, in blocks. The game
// keeps them in a list inside the animation state; here the caller keeps them between calls.
using Sparks = std::vector<std::array<float, 3>>;

// The call's other arguments (0xd3170 passes them).
struct Call {
    float dt = 0;                     // milliseconds since the last frame
    float tint[4] = {1, 1, 1, 1};     // the draw tint
    float alpha = 1;                  // passed to the draws
    int64_t cameraX = 0, cameraY = 0; // the view's origin, in 1/65536 block
    uint32_t hide = 0;                // parts not drawn (1 head, 2 hair, 4 shoulders, 8 chest, 0x10/0x20 hands, 0x40 weapons, 0x80/0x100 feet)
    const uint8_t *mount = nullptr;   // the creature ridden (its record, game layout), if any
    float depth = 0;                  // passed to the draws as their last number
    float scale = 1;                  // applied to the body
    uint32_t nowMs = 0;               // timeGetTime(), for the races whose size beats
    nativerules::HasBuff hasBuff;     // the creature's buffs (none given: no buffs)
    std::function<int()> rand;        // the game's rand(), for the sparks (none: no new sparks)
    Sparks *sparks = nullptr;         // the creature's sparks (none: no sparks)
};

// Poses the creature record `creature` (CREATURE_SIZE bytes, game layout) as 0x17ab0 does;
// the draws go to `draws` when given.
void pose(uint8_t *creature, const Call &call, const Models &models, std::vector<Draw> *draws);

// The easing factor of 0xa7a0: how far a value moves toward its target over a frame of `dt`
// ms at `rate` (gamemath::blendFactor).
float ease(double dt, float rate);

// --- The pose state (creature-pose.md §12, src/NativePoseState.cpp) --------------------------

// The pose state the world's draw gives a creature from its current ability, the items three
// abilities depend on and its lock timer (§12.2, §12.1 step 3).
int poseStateFor(const uint8_t *creature);
// What the world's draw (0xd3170) does right before posing a creature it draws (one that is
// alive and in view): the state clock (anim +0x08) becomes the ability clock (+0x6c), bit for
// bit, and the state (anim +0x0c) poseStateFor(creature). A creature not drawn keeps both.
void setPoseState(uint8_t *creature);

// --- Weapon spirits (creature-pose-items.md §9.1, src/NativePoseSpirits.cpp) ------------------

// The spirit cubes a weapon effect draw (kind WEAPON_EFFECT_DRAW) stands for: one plain draw of
// its model (cube.cub) per spirit set into the item, at the weapon matrix moved to the spirit's
// voxel, coloured by the spirit's material. None for an item without spirits.
void spiritDraws(const Draw &effect, std::vector<Draw> &out);
// The colour routine 0x109960: a material's colour under `tint`, `strength` being the effect
// call's first number (0 from the pose).
std::array<float, 4> materialColour(int material, const float tint[4], float strength = 0);

} // namespace nativepose
