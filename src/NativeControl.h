#pragma once
// How the local player is steered by the keys, the gamepad stick and the camera
// (docs/specs/player-control.md; "spec 3.6" and the like below name its sections).
//
// Once a frame (`frame`, the game's 0x8ae50):
//   - a climb ends once the player is off the wall, and the way it wants to go (the intent,
//     +0x40) is cleared unless it glides or sails;
//   - the movement routine (`control`, 0x9c050) runs while the player is neither stunned
//     (+0x130) nor locked (+0x134): the movement keys and stick in three modes (walking
//     by the camera's yaw, swimming by the camera's whole matrix, climbing along the wall),
//     sailing's build-up, the glider's steering, lift and dive, the dodge roll, and jumping
//     (the wall jump, the double jump, the warriors' leap, rising in water, holding the jump);
//   - then running or walking, the lamp, the aim point and the look vector, the special
//     skill (specialSkillKey), the pet key, and the attack keys: the normal and the charged
//     attack, the class key's stances and the dodge key's class moves.
//
// The use key and its prompts are useprompt's (src/UsePrompt.h); the camera's aim assist
// for the gamepad is `aimAssist`, and the aim point is `aimPoint`, for the caller's camera.
//
// Everything works on the player's game-layout record (nativepose::CREATURE_SIZE bytes);
// what the game reads from its controller object comes in through `Input`.
#include "NativeAI.h"
#include "NativeBuffs.h"

#include <cstdint>
#include <functional>
#include <vector>

namespace nativecontrol {

// This frame's keys and camera (spec 1.1, 1.2).
struct Input {
    // The movement keys (W, S, A, D by default), jump (Space) and dodge (a mouse button).
    bool forward = false, back = false, left = false, right = false;
    bool jump = false;
    bool dodge = false;
    bool attack = false;       // the normal attack (left mouse)
    bool strongAttack = false; // the strong, charged attack (right mouse)
    bool classSkill = false;   // the class key (left Shift): stances, the double jump, the leap
    bool ultimate = false;     // the special skill (R)
    bool walk = false;         // walk instead of run; the normal attack only as its key goes down
    bool lamp = false;         // toggles the lamp as it goes down
    bool pet = false;          // calls the pet, mounts or leaves it (T)
    float stickX = 0, stickY = 0; // the gamepad's left stick (right and forward positive)
    double cameraYaw = 0;         // degrees; the ground's forward is -row 1 of its turn
    float camera[3][3] = {};      // the camera's rows: A (left), B, C (the view direction)
    bool blocked = false;         // retained focused Widget (engine+0x170), not panel visibility
    bool panelShown = false;      // shown Ring, or full map with pad last-device (movement only)
    bool typing = false;          // the chat line takes the keys
    bool ordinaryUi = false;      // gates late attacks/class/dodge, not movement or ultimate (dialogue spec 12)
    // From the frame's targeting (spec 4.5): the aim point (aimPoint) and the creature the
    // strong attack charges at.
    bool hasAim = false;
    int64_t aimAt[3] = {0, 0, 0};
    int64_t chargeTarget = 0;
};

// The values the game keeps between frames (spec 1.4).
struct State {
    uint8_t lastJump = 0; // the jump key at the end of the last movement call
    int jumpHeld = 0;     // ms the jump key has been held
    int lift = 0;         // ms left in which holding the jump still raises it
    uint8_t lastAttack = 0;
    uint8_t lastSkill = 0; // the class key
    uint8_t lastDodge = 0;
    uint8_t lastLamp = 0;
    uint8_t lastUltimate = 0;
    uint8_t lastPet = 0;
};

// What the controller asks of the world.
struct World {
    std::function<const uint8_t *(int64_t id)> byId; // a creature's record, null if gone
    std::function<bool(int type)> hasBuff;           // the player's buffs (for ability times)
    std::function<bool(int item)> ownsSpecialItem;   // the player owns special item `item` (none: no)
    std::function<void(int sound)> playSound;        // a sound at the camera (none: silent)
    std::vector<nativebuffs::Notice> buffNotes;      // the buffs the special skill key gave
};

// The movement routine (spec 3) on the local player's record; dt in ms.
void control(const Input &in, State &state, uint8_t *player, double dt, const World &world);

// Native frame cut points (steam-gameplay-frame-order.md §§4/6). A false begin
// skips all later stages and their edge stores. Caller inserts assistance before
// ownerLook, object/Use work before pet, and Quick Item before attack.
bool beginFrame(const Input &in, State &state, nativeai::Creature &player, double dt, World &world);
void ownerLook(nativeai::Creature &player, const World &world);
void pet(const Input &in, State &state, nativeai::Creature &player, World &world);
// Compatibility frame for callers without interleaved direct-use work. dt in ms.
void frame(const Input &in, State &state, nativeai::Creature &player, double dt, World &world);

// The attack keys, the class key and the dodge key's moves (spec 4.9 to 4.12), as the
// frame runs them unless the chat line has the keys.
void attack(const Input &in, State &state, nativeai::Creature &player);

// The aim point (spec 4.5 steps 2 and 3).
struct Aim {
    int64_t rayOrigin[3] = {0, 0, 0}; // where the aiming ray starts
    int64_t eye[3] = {0, 0, 0};       // the camera's eye
    float forward[3] = {0, 0, 1};     // the view direction F (one long)
    float wallDistance = -1;          // how far the ray gets through the blocks (< 0: no hit)
    float aimDepth = -1;              // the nearest hostile creature along F (< 0: none)
    bool override = false;            // the aim is not pulled onto a creature
};
void aimPoint(const Aim &aim, const uint8_t *player, int64_t out[3]);

// The gamepad's aim assist (spec 4.6): the yaw the camera turns toward, from the one it
// turned toward before (`turnYaw`), when the stick is pushed and a living creature is
// targeted; otherwise `turnYaw` unchanged.
struct AimAssist {
    const uint8_t *player = nullptr;
    const uint8_t *target = nullptr; // the targeted creature (null: none)
    float stickX = 0, stickY = 0;
    int64_t rayOrigin[3] = {0, 0, 0};
    float forward[3] = {0, 0, 1};
    double cameraYaw = 0, turnYaw = 0; // degrees
    double dt = 0;
};
double aimAssist(const AimAssist &a);

// The special skill on the R key (controls.cfg's button 3, ButtonUltimateSkill), from the
// class (+0x14c) and whether the specialisation (+0x14d) is zero (0x656e0): warriors 0x94
// or 0x95, rangers 0x8f or 0x8e, mages 0x2f or 0x90, rogues 0x6d or 0x8d; any other class 0.
int specialSkill(const uint8_t *creature);
// An ability's key in the game's dictionary (the skill naming key: "SkillHeroicShout"), null if none.
const char *skillKey(int ability);
// 0x50bd0: how long after its use an ability may be used again (ms; 0: no wait).
int cooldown(const nativeai::Creature &creature, int ability);

// 0x29d3c0: the abilities that are buffs on the user itself (among them the rogues' two
// special skills, 0x6d and 0x8d) give that buff and append its notice, then start the
// ability's cooldown. Any other ability only starts its cooldown.
void selfBuff(nativeai::Creature &creature, int ability, std::vector<nativebuffs::Notice> &notices);

// The special skill key as 0x8ae50 reads it on a frame that gets that far: as the key goes
// down, a usable special skill starts (the rogues' two are buffs instead), and the cooldown
// of the ability then running starts.
void specialSkillKey(const Input &in, State &state, nativeai::Creature &player, World &world);

} // namespace nativecontrol
