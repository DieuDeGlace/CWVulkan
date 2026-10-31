# Player control by the keys (0x9c050, and its frame in 0x8ae50)

Behavioural specification of how Steam Cube World turns the local player's keys, gamepad
stick and camera into movement and actions, written from the decompiled executable, its
assembly and the binary's constants. Routines are cited by RVA (image base 0x140000000).
Offsets are byte offsets into the record named. Times are milliseconds, distances blocks,
world positions 64-bit integers in 1/65536 of a block, angles degrees unless stated.

The heart is the movement routine **0x9c050** (game controller, *dt*), which the per-frame
player-input routine **0x8ae50** (game controller, *dt*, creature candidate list) calls once
per frame. 0x8ae50 also maps the attack, class-skill and dodge keys to abilities; that part is
specified here too (§4). Parts covered elsewhere and only referenced:

* the special skill on R and its cooldown: `skills-and-interactions.md` §1;
* the crosshair's creature, object and item targets, the use key E and its prompts, the quick
  item key: `use-prompt.md` §2–§4;
* what the interactions queued here do once the world handles them (kinds 5, 8, 10):
  `zone-object-interaction.md`;
* the ability starts 0x60460 and 0x60720 and the move pickers 0x5fc40 / 0x60120:
  `creature-ai.md` (the "next hit", "charge move", "start a hit", "start a charge" rules);
* the smoothing factor 0xa7a0: `creature-upkeep.md` §4.9; the turn 0x17840:
  `creature-physics.md` §7.3; the yaw basis 0x13050: `creature-abilities.md` §4.1; ability
  durations 0x4f220 and 0x4ead0: `stat-rules.md` §7.1 and `creature-abilities.md` §0;
  usable check 0x507b0: `skills-and-interactions.md` §1.6; skill level 0x65690 is always
  1000 (`creature-upkeep.md`).

---

## 0. Conventions

* Single-precision IEEE arithmetic, each operation rounded before the next, in the order
  written (as in `creature-upkeep.md` §0). Where a step says "in double", the operands are
  widened to double, combined in double and the result narrowed to float once.
* "× 0" terms are written out where the game computes them: they matter for signed zeros and
  NaN and must be kept.
* "Truncate" converts toward zero (32-bit unless stated).
* A comparison is written as the game tests it. "x > 0" is false for NaN.
* *dt* is the frame's elapsed time in ms as a double.
* **duration** means 0x4f220 (player) as a float; "the ability has finished" means
  +0x6c > duration.
* The **intent** is the player's wanted-direction vector +0x40, +0x44, +0x48 (x, y, z).
* "Gliding" is 0x5ee70: ability 0x78, bit 0 of +0x5c clear (not on the ground), stun
  +0x130 ≤ 0 and dodge timer +0x12c ≤ 0.

---

## 1. Inputs

### 1.1 Key bytes and their default bindings

The input layer writes one held-state byte per button at controller +0x1a70 + index (see
`skills-and-interactions.md` §1.1 and `use-prompt.md` §0.6). The defaults come from 0x43760
(device 0 is the keyboard with a scan code, device 1 a mouse button):

| Byte | Button | Default | Read here |
|---|---|---|---|
| +0x1a70 | normal attack | mouse button 0 | §4.9 |
| +0x1a71 | strong (charged) attack | mouse button 1 | §4.4, §4.9 |
| +0x1a72 | class skill | left Shift (0x2a) | §3.10, §4.9–§4.11 |
| +0x1a73 | ultimate skill | R (0x13) | `skills-and-interactions.md` §1 |
| +0x1a74 | quick item | Q (0x10) | §4.8 |
| +0x1a75 | forward | W (0x11) | §3.1, §3.3, §3.6 |
| +0x1a76 | back | S (0x1f) | §3.1, §3.6 |
| +0x1a77 | left | A (0x1e) | §3.1, §3.6 |
| +0x1a78 | right | D (0x20) | §3.1, §3.6 |
| +0x1a79 | jump | Space (0x39) | §3.3, §3.10, §3.11, §4.2 |
| +0x1a7a | dodge | mouse button 2 | §3.9, §4.11 |
| +0x1a7b | use | E (0x12) | `use-prompt.md` |
| +0x1a7c | lamp | F (0x21) | §4.3 |
| +0x1a7d | walk | left Ctrl (0x1d) | §4.2, §4.9 |
| +0x1a7e | call pet / ride | T (0x14) | §4.8 |

The gamepad's left stick is the pair of floats at controller **+0x1d6c** (x, right
positive) and **+0x1d70** (y, forward positive).

### 1.2 Game controller fields

| Offset | Type | Use |
|---|---|---|
| +0x08 | pointer | the world; world +0x448 is the local player's creature record |
| +0x18 | pointer | the renderer (visibility test 0xe08c0) |
| +0x20 | pointer | the window root: 0x342090 on it returns a modal window or null; its +0x1a8 bit 0 is the left mouse button |
| +0x118 | object | the chat line; 0x1014f0 on it is true while it takes the keys ("typing") |
| +0x310 | pointer | a panel (its +0x68 widget; the int at (widget +0xe0)[widget +0xac] is non-zero while shown) |
| +0x330 | pointer | a window (+0x178 → +0x68, same visibility int; probably the inventory) |
| +0x398 | pointer | an object whose byte +0x1d8 is non-zero while it holds the keys |
| +0x668, +0x1958 | bytes | when both are non-zero the movement routine ignores the keys |
| +0x1d80 | double | camera pitch, degrees from straight up |
| +0x1d88 | double | a second camera angle (read, unused here) |
| +0x1d90 | double | **camera yaw**, degrees |
| +0x1da8 | double | the yaw the camera is turning toward (written by the aim assist, §4.6) |
| +0x1dc0, +0x1dc8, +0x1dd0 | i64 ×3 | the camera's eye position |
| +0x1dd8, +0x1de0, +0x1de8 | i64 ×3 | the origin of the aiming ray |
| +0x1df0, +0x1df8 | i64 ×2 | render-origin offset (targeting) |
| +0x1ea4, +0x1ea8, +0x1eac | f32 ×3 | camera matrix row **A** (sideways, pointing left) |
| +0x1eb4, +0x1eb8, +0x1ebc | f32 ×3 | camera matrix row **B** |
| +0x1ec4, +0x1ec8, +0x1ecc | f32 ×3 | camera matrix row **C**, the view direction **F** |
| +0x1e58 | i64 | the targeted creature (`use-prompt.md` §0.5) |
| +0x1e60 | i64 | an aim override: while non-zero the aim point is not pulled onto a creature (§4.5); 0x8ae50 sets it to 0 every frame after the special-skill check |
| +0x20c0 | i32 | the quick-item selection |

### 1.3 Player record fields (record size 0x15f8)

| Offset | Type | Use |
|---|---|---|
| +0x08 | i64 | id |
| +0x10, +0x18, +0x20 | i64 ×3 | position |
| +0x2c | f32 | roll (tilts the glider, §3.2) |
| +0x30 | f32 | yaw (wall jump turns it, §3.10) |
| +0x34, +0x38, +0x3c | f32 ×3 | velocity; +0x3c is vertical (up positive) |
| +0x40, +0x44, +0x48 | f32 ×3 | **intent** |
| +0x4c, +0x50, +0x54 | f32 ×3 | push vector (set by wall jumps and the mage-class blink, cleared by jumps) |
| +0x5c | u32 | movement flags: bit 0 on the ground, bit 1 in water, bit 2 (4) touching a climbable wall, bit 12 (0x1000) wall contact for a wall jump |
| +0x68 | u8 | current ability |
| +0x6c | f32 | ability time |
| +0x80 | u32 | (of the mount) bit 1 (2): a flying mount |
| +0x128 | u16 | state flags: bit 0 lamp, bit 2 (4) looking at the owner, bit 6 (0x40) running, bit 9 (0x200) cleared each frame |
| +0x12c | f32 | dodge timer |
| +0x130 | f32 | stun timer |
| +0x134 | f32 | a lock timer (also gates the movement routine, §2) |
| +0x14c, +0x14d | u8 ×2 | class, specialisation |
| +0x158 | f32 | how far a charged move has charged |
| +0x174, +0x178, +0x17c | f32 ×3 | **look vector**: from the player to the aim point, blocks |
| +0x18c | f32 | stealth |
| +0x1a0 | i64 | owner id |
| +0x1a8 | i64 | linked creature: the mount while riding (0x80), the charge's target |
| +0x1d8 | u8 | type byte of the held-up item record |
| +0x638, +0x63c | u8, u32 | item in the second hand slot: type, subtype |
| +0x6d8, +0x6dc | u8, u32 | item in the weapon slot: type, subtype |
| +0x9a0 | f32 | stamina (0..1) |
| +0x9ac, +0x9b0, +0x9b4 | f32 ×3 | **wall normal N** |
| +0x9d8 | i64 | pet id |
| +0x9e8 | f32 | how long the warriors' charge lasts |
| +0xa08, +0xa0c | i32, item | the item held on the mouse cursor: count, item |
| +0xab8 | list | pending interactions (`skills-and-interactions.md` §2.1) |
| +0xae0, +0xae8, +0xaf0 | i64 ×3 | a locked aim point (beams, §4.5) |
| +0xb10, +0xb18, +0xb20 | i64 ×3 | render position |
| +0xb78 | map | cooldowns |
| +0xbb4 | f32 | a value that scales the glider's lift (§3.3) |

### 1.4 The game's static values

Held between frames in the executable's data; all start at 0.

| RVA | Type | Meaning |
|---|---|---|
| 0x551594 | u8 | jump key at the end of the last movement call |
| 0x551598 | i32 | how long the jump key has been held, ms |
| 0x55159c | i32 | **lift time**: ms left in which holding the jump raises the jump |
| 0x5515a0 | u8 | left mouse button at the last frame |
| 0x5515a1 | u8 | lamp key at the last frame |
| 0x5515a2 | u8 | R at the last examination (`skills-and-interactions.md` §1.4) |
| 0x5515a3 | u8 | use key (`use-prompt.md`) |
| 0x5515a4 | u8 | pet key |
| 0x5515a5 | u8 | quick-item key |
| 0x5515a6 | u8 | normal-attack key |
| 0x5515a7 | u8 | class-skill key |
| 0x5515a8 | u8 | dodge key |
| 0x5515ac, 0x5515b0 | u32 | system time (timeGetTime) when the class key went down, for classes 2/4 of specialisation 1 and for class 3; written, never read |

Each "last" byte is written only on frames that reach its step; on frames that return early
it keeps its value, so an edge can be seen late.

---

## 2. The frame (0x8ae50), in order

The controller's update 0xa5a50 calls 0x8ae50 under the conditions of
`skills-and-interactions.md` §1.3. 0x8ae50 then does, in this order:

1. **Dropping the cursor item.** When the left mouse button (window root +0x1a8 bit 0) is
   down this frame and was not at the last frame (0x5515a0), the cursor count +0xa08 is not
   0, the window at controller +0x330 is shown, and 0x33f1e0 reports nothing under the mouse: queue a
   fresh interaction of kind 8 holding a copy of the cursor item +0xa0c (zone x, zone y and
   object index −1, target 0, +0xc6 = 1) on the player's list; then count − 1; if the count
   is now below 1, count = 0, the cursor item's type byte = 0 and its +0x04 (subtype) = 0.
   Then 0x5515a0 = the button bit.
2. **Leaving a wall.** If the ability is 0x7a (climbing), +0x5c bit 2 is clear, and bit 0 or
   bit 1 is set: ability = 0.
3. **Targeting.** The aim ray, the creature under the crosshair, the nearby creature and the
   two values kept for §4.5: the **aim depth** and the **charge target**
   (`use-prompt.md` §2.2; the two extra values are given in §4.5).
4. **Clearing the intent.** Unless gliding or the ability is 0x79 (sailing), the intent is
   set to (0, 0, 0).
5. **The movement routine.** If the stun timer +0x130 ≤ 0 and the lock timer **+0x134** ≤ 0
   (each "not > 0"; a NaN timer also skips), call 0x9c050 (§3). The dodge timer +0x12c does
   not gate this call.
6. Everything in §4.

---

## 3. The movement routine (0x9c050)

Let **P** be the player record. The routine keeps a step size **s**, starting at 80.0.

### 3.1 Sailing (ability 0x79), or a cleared intent

If P is gliding, go to §3.2. Otherwise:

* If P's ability is 0x79 (sailing): s = 5.0.
  1. If |intent|² < 225.0, none of the four movement keys is held and stick x² + stick y²
     < 0.01: intent = (0, 0, 0).
  2. If |intent|² > 1600.0: k = 1.0 / sqrt(|intent|²); intent.y = k × intent.y, intent.x =
     k × intent.x, intent.z = k × intent.z; then each component × 40.0. (The boat's wanted
     speed builds up by the keys and is capped at 40.)
* Any other ability: intent = (0, 0, 0).

(|v|² is always (x × x + y × y) + z × z.) Then go to §3.3.

### 3.2 Gliding: steering by roll, and running out of stamina

1. Let (vx, vy) = velocity x, y. L = (vx × vx + vy × vy) + 0.0. If L > 0:
   1. k = 1.0 / sqrt(L); dx = vx × k; dy = vy × k; t = (k × 0) × 0.
   2. Side vector: a = dx − t; b = t − dy; c = dy × 0 − dx × 0.
   3. e = 0xa7a0(*dt*, 0.001).
   4. intent.x = (dx × 10.0 − intent.x) × e + intent.x; intent.y = (dy × 10.0 − intent.y)
      × e + intent.y.
   5. q = roll +0x2c. intent.y = intent.y − (a × q) × 0.05; intent.x = intent.x − (b × q) ×
      0.05; intent.z = intent.z − (c × q) × 0.05.
2. If 0.001 > stamina (false for NaN): clear +0x128 bit 6 (running); e =
   0xa7a0(*dt*, 0.002); intent.z = e × (0 − intent.z) + intent.z; and **return from the
   routine at once** (none of §3.4–§3.12 runs, the static values are not updated).

### 3.3 Gliding: lift and dive

Only while gliding (tested again). Let **g** = min(1.0, max(0.0, +0xbb4 × 0.001 − 0.5))
and **fall** = max(−velocity z, 0.0) (the sink speed; the sign flip is a bit flip).

* **Lift:** if the jump key is held, stamina > 0 and velocity z ≤ 5.0: intent.z = in
  double, (g × 0.002) × *dt* × fall + intent.z.
* **Otherwise (dive):** let f = the stick's y; if the forward key is held, f = 1.0. If the
  forward key is held or f > 0: intent.z = in double, ((g × f) × 0.0015) × *dt* × fall +
  intent.z. Then, in either case, e = 0xa7a0(*dt*, 0.002) and intent.z = e × (0 −
  intent.z) + intent.z.

Then, if velocity z > 0 and intent.z > 12.0: intent.z = 12.0.

(While gliding, "lift" and "dive" both push intent z up in proportion to how fast the glider
sinks; the dive eases intent z back toward 0. The movement code elsewhere turns intent z into
the glider's pitch.)

### 3.4 When the keys are ignored

Return at once (static values not updated) if any of these holds: 0x342090 on the window
root returns a window; the byte +0x1d8 of the object at controller +0x398 is set; the panel
at controller +0x310 is shown; controller +0x668 and +0x1958 are both non-zero.

### 3.5 Step size, the yaw basis and the dodge test

1. If gliding: s = float(*dt* × 4.0) (in double).
2. **Yaw basis Y** = 0x13050 applied to the identity with the camera yaw (controller +0x1d90,
   narrowed to float). Its rows are R0 = (cos, sin, 0), R1 = (−sin, cos, 0), R2 = (0, 0, 1)
   (exact entries as 0x13050 computes them). The ground "forward" is −R1, "right" is −R0.
3. **Dodge wanted** (decided now, before the keys move the intent): true when all hold: the
   chat line is not typing, the dodge key is held, P is on the ground (+0x5c bit 0), the
   dodge timer +0x12c equals 0 exactly, and stamina ≥ 0.25.

### 3.6 The movement keys

Three modes, chosen per key at the moment the key is handled (abilities and flags read anew):

* **Climbing:** ability 0x7a and +0x5c bit 2 set. Uses the wall normal N and **k = s × 0.2**.
* **Swimming:** +0x5c bit 1 set and the ability is not 0x79. Uses the camera rows A, B, C.
* **Walking/flying:** otherwise, but skipped entirely (no change) when P is in ability 0x78
  with bit 0 clear and stamina ≤ 0 (a glider out of stamina cannot steer). Uses Y.

The keys are handled in this order: forward, back, right, left. Each line below is applied to
each axis i (x, y, z) in that order, using that axis's entries (R0ᵢ, Aᵢ, …):

| Key | Climbing | Swimming | Walking |
|---|---|---|---|
| forward | z only: intent.z = s × 0.2 + intent.z | intentᵢ = ((Bᵢ × 0 + Aᵢ × 0) + Cᵢ) × s + intentᵢ | intentᵢ = ((R0ᵢ × 0 − R1ᵢ × 1.0) + R2ᵢ × 0) × s + intentᵢ |
| back | z only: intent.z = intent.z − s × 0.2 | intentᵢ = ((Bᵢ × 0 + Aᵢ × 0) − Cᵢ × 1.0) × s + intentᵢ | intentᵢ = intentᵢ − ((R0ᵢ × 0 − R1ᵢ × 1.0) + R2ᵢ × 0) × s |
| right | intent.x −= (N.y − N.z × 0) × k; intent.y −= (N.z × 0 − N.x) × k; intent.z −= (N.x × 0 − N.y × 0) × k | intentᵢ = ((Bᵢ × 0 − Aᵢ × 1.0) + Cᵢ × 0) × s + intentᵢ | intentᵢ = intentᵢ − ((R1ᵢ × 0 + R0ᵢ) + R2ᵢ × 0) × s |
| left | the same three terms added instead of subtracted | intentᵢ = ((Bᵢ × 0 + Aᵢ) + Cᵢ × 0) × s + intentᵢ | intentᵢ = ((R1ᵢ × 0 + R0ᵢ) + R2ᵢ × 0) × s + intentᵢ |

(In the climbing rows "x −= v × k" means x = x − v × k. N.z × 0 is computed once per key and
used in both the x and y terms.)

So on the ground W walks toward where the camera looks, A and D strafe; swimming follows the
whole camera matrix (looking down and pressing W dives); on a wall W and S climb up and down
and A and D move along the wall.

### 3.7 The stick

Only when stick x² + stick y² ≠ 0. Let sx = −stick x.

* **Climbing:** k = s × 0.2 (this k is the step from now on in this block). intent.x =
  (N.y × sx − N.z × 0) × k + intent.x; intent.y = (N.z × 0 − N.x × sx) × k + intent.y;
  intent.z = (N.x × 0 − N.y × 0) × k + intent.z; then intent.z = k × stick y + intent.z.
* **In water** (+0x5c bit 1 set; unlike the keys, this holds also while sailing): for each
  axis, wᵢ = (sx × Aᵢ + Bᵢ × 0) + stick y × Cᵢ; then intent.z = w.z × s + intent.z,
  intent.y = w.y × s + intent.y, intent.x = w.x × s + intent.x.
* **Otherwise**, unless P is in ability 0x78 with bit 0 clear and stamina ≤ 0: with sy =
  −stick y, wᵢ = (sy × R1ᵢ + sx × R0ᵢ) + R2ᵢ × 0 (for y the two first products are added in
  the order sx × R0.y + sy × R1.y); then z, y, x as in the water case.

### 3.8 Ability 0x36

If P's ability is 0x36 it becomes 0.

### 3.9 The dodge roll

If the dodge was wanted (§3.5), |intent|² > 0, stamina ≥ 0.25 and +0x12c ≤ 0:

1. Ability = 0.
2. (x, y) = intent x, y. k = 1.0 / sqrt((x × x + y × y) + 0); x = x × k; y = y × k;
   t = k × 0.
3. Velocity x = x × 15.0 + 0; velocity z = t × 15.0 + 5.0; velocity y = y × 15.0 + 0.
4. Dodge timer +0x12c = 600.0; stamina = stamina − 0.25.

The roll goes the way the keys (or stick) point, flat, 15 blocks/s with a small hop.

### 3.10 Jumping

1. **Lift allowance:** L = 100. If P's ability is 0x80 (riding), the mount (0xe720 on
   +0x1a8) exists and its +0x80 bit 1 is set (a flying mount): L = 500.
2. **Skip:** if the jump key is not held, or P's ability is 0x33 and +0x6c ≤ duration (the
   teleport is still running): the held time 0x551598 = 0; go to §3.11.
3. **Wall jump:** if P's ability is 0x7a and the jump key was not held at the last call
   (0x551594 = 0): ability = 0; then, if +0x5c bit 12 (0x1000) is set:
   * velocity = (N.x × 15.0, N.y × 15.0, N.z × 15.0) (written y, x, z);
   * intent = the same three values; push vector +0x4c.. = the same three values;
   * dodge timer +0x12c = 600.0;
   * yaw +0x30 = 0x17840(+0x30, +0x30 + 180.0, 1.0) (turn to face away from the wall);
   * stamina = stamina − 25.0; if stamina ≤ 0, stamina = 0. (25.0 against a 0..1
     stamina: a wall jump always empties it.)
4. Held time = truncate(double(held time) + *dt*).
5. If the held time > 0:
   1. **Take-off:** if held time < 200, (P is on the ground or +0x12c ≠ 0) and velocity z ≤
      0:
      * velocity z = 5.0 if P's ability is 0x53, or is 0x86 or 0x87 with +0x6c < duration;
        else 10.0.
      * If the class-skill key is held:
        - if stamina ≥ 0.25 and (class 4 with specialisation 1, or class 2 with
          specialisation 1): +0x12c = 600.0, velocity z = 20.0, stamina − 0.25 (the double
          jump);
        - then, if P's ability is not 0x9f, stamina ≥ 0.25 and class 1: ability = 0x9f, +0x6c
          = 0, velocity z = 20.0, stamina − 0.25 (the warriors' leap).
      * Lift time 0x55159c = L. Push vector = (0, 0, 0).
   2. **Rising in water:** if +0x5c bit 1 is set and velocity z < 10.0: lift time = L;
      velocity z = in double, *dt* × 0.001 + velocity z.

(The take-off window is the first 200 ms of a press; the ground test accepts a rolling
player too, so one can jump out of a roll.)

### 3.11 Holding the jump

If the lift time > 0 and the jump key is held, P's ability is not 0x53, P's ability is not
(0x86 or 0x87 with +0x6c < duration), and velocity z < 10.0: velocity z = in double, *dt* ×
0.08 + velocity z; then if velocity z ≥ 10.0 (or is NaN), velocity z = 10.0.

### 3.12 End of the routine

Lift time = truncate(double(lift time) − *dt*); if negative, 0. Then 0x551594 = the jump
key.

---

## 4. After the movement routine (0x8ae50)

### 4.1 Early returns

0x8ae50 returns here, doing nothing more this frame, when any of these holds: a modal
window (0x342090); the +0x398 object's byte +0x1d8; P gliding with stamina < 0.001; P in
ability 0x7d with +0x6c < 2000.0.

### 4.2 Running or walking (+0x128 bit 6)

* If P's ability is 0x78 and P is off the ground: bit 6 is set when stamina > 0 and the jump
  key is held, cleared otherwise.
* Otherwise bit 6 is set unless the walk key is held (then cleared).

### 4.3 The lamp

When the lamp key is down and was not at the last frame (0x5515a1), toggle +0x128 bit 0.
Then 0x5515a1 = the lamp key.

### 4.4 A long-finished ability ends

Unless P's ability is one of 0x98, 0x9c, 0x9d, 0x57, 0x76, 0x80, 0x79, 0x78, 0x7a, 0x51, 0x5f,
0x53, 0x92, 0x56, 0x58, 0x09, 0x24, 0x50, 0x1d, 0x1e, 0x25 (states that last: riding,
sailing, gliding, climbing, stances, channels), the strong-attack key is not held, the
ability has finished and +0x6c > 10000.0: ability = 0.

### 4.5 The weapon's two moves, the aim point and the look vector

1. **The two moves** (kept for §4.12). Let w = the weapon slot's subtype +0x6dc (its type is
   not tested) and spec = +0x14d.
   * w = 10: first 0x21 (spec 1) or 0x1f; second 0x22 or 0x20.
   * w = 12: first 0x2b or 0x29; second 0x2c or 0x26.
   * w = 11: first 0x6b or 0x68; second 0x6a or 0x69.
   * otherwise first = 0x5fc40(P, off); second: w = 5 → 6; w = 3 → 0x12; w = 4 → 0x15; w = 6
     or the second slot's subtype +0x63c = 6 → 0x1a; w = 7 or +0x63c = 7 → 0x19; w = 8 →
     0x1c; two-handed (0x5e9b0) → 0x3c; +0x63c = 0xd → 9; else 0x40.
2. **Aim point.** Start with the ray origin (controller +0x1dd8..) + F ×100.0 blocks.
   The call0x8b26d→0x2d1d20 uses **limit60** and flags0/1. A return d≥0 replaces the
   point by ray origin+F×d, multiplying float F by d then65536 before signed64 truncation.
   A clear ray reaching the helper's positive limit returns60, not a negative no-hit
   sentinel; thus it ordinarily selects60 rather than the initial100. F here is
   (B×0+A×0)+C, per axis. This clarifies the older fallback shorthand; exact producer,
   origins and finite references are [gameplay frame order §8](steam-gameplay-frame-order.md).
3. **Snap to a creature.** The targeting pass (§2 step3) separately keeps the **aim depth**:
   among creatures under the crosshair with line of sight, the smallest
   t = (creature's raw drawn position − stored eye) ·
   F, taken only when t ≥ 0, whose hostility to P (0x50550) is "hostile" (or "not hostile" while P's ability is 0x86
   or 0x87). If controller +0x1e60 = 0, the aim depth is ≥ 0 and P's ability is not 0x33:
   Q = eye (+0x1dc0..) + F×aim depth; if (Q−P's position)·F>0, the aim point=Q.
   This depth need not belong to selected creature+0x1e58 and is not clipped by the
   terrain scalar. Game+0x1e60's nonzero producer remains unverified; preserve its
   read-before-clear behavior without inventing a selected-target/charge-target writer.
4. If P's ability is 0x8c, 0x4f220 is called and its result dropped.
5. **Beams lock their aim.** If P's ability is 0x1f, 0x20, 0x21 or 0x22: if
   double(0x4ead0(P, −1)) < double(+0x6c) − *dt* (its first part ended before this frame),
   the aim point = the stored point +0xae0..; otherwise the aim point is stored there.
6. Clear +0x128 bit 9 (0x200). **Look vector** +0x174, +0x178, +0x17c = (aim point − P's
   position) × 1/65536, per axis.
7. The special skill key: `skills-and-interactions.md` §1.4. Then controller +0x1e60 = 0.

The **charge target** (used in §4.9) is chosen in the same targeting pass: walking the
candidates in order, a creature becomes it when its hostility kind (+0x60) is 1 or 7 and no
charge target has been chosen yet, or when its squared distance from P (integer squares of
the position differences, each shifted right by 16, summed, × 1/65536) is below the best
so far (this second test does not look at the kind).

### 4.6 Gamepad aim assist (camera)

When the targeted creature T (+0x1e58) exists with health > 0 and stick x² + stick y² > 0.1:

1. D = (T's render position − ray origin) × 1/65536; D.z = D.z + T +0x8c × 0.45.
2. speed = 150.0 if 0x5eca0(P) (class 3, classes 0x95..0x98, races 0x75 and 0x56, or a
   ranged weapon: weapon slot type 3 with subtype 6, 7, 8, 10, 11 or 12, or the second slot
   with 6, 7, 8, 10, 11), else 500.0. lead = |P's position − T's render position| / speed.
   D = D + T's velocity × lead.
3. yaw′ = the camera yaw. If (D.x² + D.y²) + 0 > 0 and (F.x² + F.y²) + 0 > 0: normalise
   (D.x, D.y, 0) and (F.x, F.y, 0) (0x127f0); yaw′ = float(double(asin(F̂.x × D̂.y − F̂.y ×
   D̂.x)) / π × 180.0 + double(yaw)).
4. (A pitch toward D, acos of its normalised z, is computed the same way and not used.)
5. If |D|² > 0 and |F|² > 0: normalise both; m = |F̂ × D̂| (sqrtf of the squared cross
   product); r = 0.015 if +0x12c = 0, else 0.03; f = (1 − expf(float(−(r × *dt*)) )) ×
   expf(m × −20.0). Controller +0x1da8 = double(0x17840(float(+0x1da8), yaw′, f)).

The camera drifts toward the target's lead point, faster the closer it already looks at it
and a little faster while rolling.

### 4.7 Looking at the owner (ability 0x9a)

If P's ability is 0x9a: clear +0x128 bit 2; if +0x6c < duration and the owner (0xe720 on
+0x1a0) exists: look vector = (owner's render position − P's position) × 1/65536; set bit 2.

### 4.8 Use, quick item, pet and riding

1. Targets of objects and ground items, and the use key: `use-prompt.md`.
2. **Pet key.** When it goes down (0x5515a4 was 0) and P's ability has finished:
   * If P's ability is 0x76 (riding the pet): ability = 0.
   * Else, if the pet (0xe720 on +0x9d8) exists, its squared distance from P (positions,
     blocks) is < 36.0 and P owns special item 3 (0x5e890(P, 3)): ability = 0x76 (mount it).
   * Otherwise: +0x1d8 = 0x21 (hold up the call item), ability = 0x9b, +0x6c = 0, play sound
     0x7c locally at the camera eye (0xa0420), and queue a fresh interaction of kind 10 on P
     (`zone-object-interaction.md` §8).
   Then 0x5515a4 = the pet key.
3. **Quick-item key.** When it goes down (0x5515a5): the quick slots are listed (0x93e60); if
   any, slot (+0x20c0 wrapped into the list) is used through 0xb7740 (queues kind 1,
   `skills-and-interactions.md` §2.3) and a widget's visibility int is cleared. Then
   0x5515a5 = the key.

### 4.9 The attack keys

If the chat line is typing (0x1014f0), the rest of 0x8ae50 (§4.9–§4.12) is skipped and
0x5515a6..0x5515a8 keep their values.

1. **Normal attack wanted** = the attack key, except false when the walk key is held and the
   attack key was held at the last frame (0x5515a6): holding walk turns the attack into one
   hit per press. Then 0x5515a6 = the attack key.
2. **Strong attack key held:** if 0x60720(P, force = the class-skill key) succeeds, +0x1a8 =
   the charge target (§4.5). Go to §4.10.
3. **Otherwise, a charged move goes off as the key is let go**, by P's ability:

| Ability | Becomes |
|---|---|
| 0x09 | if +0x158 < 0.01: +0x158 = 0 and ability 0; else 0x74, time 0 |
| 0x19 | 0x18, time 0 |
| 0x1a | 0x38, time 0 |
| 0x1c | 0x1b, time 0 |
| 0x24 | 0, time 0 |
| 0x25 | 0x26, time 0 |
| 0x3c | if +0x158 < 0.01: as for 0x09's first case; else specialisation ≠ 1: 0x3d with time = float(0x4ead0(P, −1)); specialisation 1: 0x3e, time 0 |
| 0x40 | if +0x158 < 0.01: as above; else 0x0c with time = float(0x4ead0(P, −1)) |
| 0x41 | 0x11 with time = float(0x4ead0(P, −1)) |
| 0x93 | 0x94, time 0 |
| any other | if a normal attack is wanted: 0x60460(P, force = the class-skill key); if it succeeds and the ability is now 0x37, the **charge leap** below |

**Charge leap (ability 0x37):** velocity = (look.x + look.x, look.y + look.y, look.z +
look.z); +0x9e8 = (sqrtf(|velocity|²) / 50.0) × 1000.0; if |velocity|² > 2500.0, normalise
it (0x127f0) and multiply each component by 50.0; then velocity z = 15.0.

### 4.10 Releasing the class-skill key

If the class-skill key is not held: ability 0x50 with |intent|² > 0 → 0; 0x51 → 0; 0x5f
with +0x6c > 1000.0 → 0; 0x53 → 0; 0x92 → 0.

### 4.11 Class-skill and dodge-key moves (neither attack key held)

Only when neither the attack key nor the strong-attack key is held. "Finished" is P's
ability having finished (§0). In this order:

1. Class 4 or class 2, specialisation 0, class key held: if 0x65690(P, 0x50) > 0, finished,
   on the ground and ability ≠ 0x32: ability = 0x50 (time kept).
2. Class 4 with specialisation 1, or class 2 with specialisation 1 and ability ≠ 0x78, class
   key held: (stamp 0x5515ac if the key just went down;) if finished, ability ≠ 0x32 and
   0x65690(P, 0x51) ≠ 0: ability = 0x51.
3. Class 1, class key held, stamina ≥ 0.25: if finished and ability ≠ 0x5f: ability = 0x5f,
   time 0.
4. Class 3: (timeGetTime is called;) if the class key is held: (stamp 0x5515b0 if it just went
   down;) if finished, +0x12c = 0 and ability ≠ 0x53: ability = 0x53; velocity z = the
   larger of velocity z and −0.2 (−0.2 when velocity z is NaN); push vector = (0, 0, 0).
5. **Dodge-key moves** (a standing dodge: the dodge key down this frame and not at the last,
   0x5515a8, and |intent|² < 0.01). Each needs its class (and specialisation), usable
   0x507b0(P, move, 1) and a non-zero skill level:

| Class | Move | Usable test | Skill test | Then |
|---|---|---|---|---|
| 3 | 0x33 | 0x33 | 0x33 | ability 0x33, time 0, +0x12c = 0 |
| 1 | 0x37 | 0x37 | 0x37 | ability 0x37, time 0, +0x12c = 0, then the charge leap of §4.9 (its length by 0x9baf0) |
| 2, spec 1 | 0x16 | 0x16 | 0x16 | ability 0x16, time 0, +0x12c = 0 |
| 4, spec 0 | 0xa0 | 0xa0 | **0x16** | ability 0xa0, time 0, +0x12c = 0 |
| 2, spec 0 | 0x34 | 0x34 | 0x34 | ability 0x34, time 0, +0x12c = 0, then the blink below |

**Blink (0x34):** if |intent|² ≤ 0, push vector = look vector × −1.0 (x, z, y in that
order); else push vector = intent. +0x12c = 600.0; push z = 0; if |push|² > 0: normalise it,
push x and y × 20.0, push z = 15.0, velocity x = push x + velocity x, velocity y = push y +
velocity y.

Then 0x5515a7 = the class key and 0x5515a8 = the dodge key (whatever the attack keys).

### 4.12 Ending held moves

If the strong-attack key is not held, the second move of §4.5 is 0x1d and P's ability is
0x1d: ability = 0. If the attack key is not held, the first move is 0x1e and the ability is
0x1e: ability = 0.

---

## 5. Sitting, riding and sailing

* **Sitting and sleeping** are not keys of their own: the use key on a seat or bed queues an
  interaction of kind 5 (`use-prompt.md` §3.7.2), and the world seats the player
  (`zone-object-interaction.md` §2.4). Taking a seat ends sneaking (ability 0x50) and
  clears the intent.
* **Riding** is ability 0x80 (a mount, linked by +0x1a8; holding the jump lifts for 500 ms on
  a flying mount, §3.10) or ability 0x76 (the pet, §4.8). Both are lasting abilities (§4.4).
* **Sailing** is ability 0x79: the intent is kept between frames, grows by 5 per key per frame
  and is capped at 40 (§3.1).
* **Running** is the default (+0x128 bit 6); the walk key walks (§4.2).

---

## 6. Constants

| RVA | Value | Use |
|---|---|---|
| 0x42e5b8 | 80.0 | initial step s |
| 0x42e584 | 5.0 | sailing step; glide lift limit on velocity z; take-off for 0x53/0x86/0x87; dodge hop |
| 0x4ac980 | 1.0 | |
| 0x42e514 | 0.001 | glide stamina floor; 0x9c050 smoothing rate; +0xbb4 scale |
| 0x437ff4 | 225.0 | sailing: below this |intent|² with no input, stop |
| 0x42e51c | 0.01 | stick dead zone (sailing); charge empty; standing-dodge limit |
| 0x43294c | 1600.0 | sailing cap² |
| 0x42e5ac | 40.0 | sailing cap |
| 0x42e598 | 10.0 | glide forward factor; jump speed; hold-jump cap |
| 0x42e524 | 0.05 | glide roll steering |
| 0x42e6d0 | 0.002 | glide lift rate; glide smoothing rate |
| 0x42e6c8 | 0.0015 | glide dive rate |
| 0x4ac964 | 0.5 | +0xbb4 offset |
| 0x42e59c | 12.0 | glide intent z cap |
| 0x42e8e8 | 4.0 (double) | glide step per ms |
| 0x4ac95c | 0.25 | dodge, double jump and leap stamina; warrior stance stamina |
| 0x42e700 | 0.2 | climbing step factor |
| 0x42e7a4 | 15.0 | dodge speed; wall jump speed |
| 0x42e7d4 | 180.0 | wall jump turn |
| 0x42e5a4 | 25.0 | wall jump stamina cost |
| immediate | 600.0 (0x44160000) | dodge timer set by dodge, wall jump, double jump, blink |
| immediate | 100, 500, 200 (ms) | lift allowance, flying mount, take-off window |
| immediate | 5.0, 10.0, 20.0 (0x40a00000, 0x41200000, 0x41a00000) | take-off speeds |
| 0x437f90 | 0.001 (double) | rising in water |
| 0x437fb8 | 0.08 (double) | holding the jump |
| 0x42e520 | 0.0174532924 | degrees to radians (0x13050) |
| 0x42e5d8 | 2000.0 | ability 0x7d input lock |
| 0x42e5e8 | 10000.0 | long-finished ability |
| 0x42e5bc | 100.0 | default aim distance |
| 0x42e5f8 | 65536.0 | blocks to position units |
| 0x42e510 | 1/65536 | position units to blocks |
| 0x42e5b0 | 50.0 | charge leap speed |
| 0x42e5d4 | 1000.0 | charge leap time scale; warrior stance limit |
| 0x437ffc | 2500.0 | charge leap speed² cap |
| 0x42e800 | −0.2 | mage float: lowest velocity z |
| 0x42e5a0 | 20.0 | blink speed |
| 0x4aca4c | −1.0 | blink backwards; "none" marks |
| 0x437fe8 | 36.0 | pet mounting range² |
| 0x42e528 | 0.1 | aim assist stick threshold |
| 0x437f84 | 0.45 | aim assist height factor |
| 0x42e938 | 150.0 | aim assist projectile speed (casters, ranged) |
| 0x42e5c8 | 500.0 | aim assist speed (others) |
| 0x42e758 | π (double) | |
| 0x42e778 | 180.0 (double) | |
| 0x437f78 | 0.015 | aim assist rate |
| 0x42e6ec | 0.03 | aim assist rate while rolling |
| 0x42e628 | −20.0 | aim assist falloff |
| 0x42e5c4 | 256.0 | line-of-sight limit (targeting) |

---

## 7. How it is checked

* `EngineTests controloracle <file>`: `Extract/oracle/control.bin` (CCT1), recorded calls of
  0x9c050 with the keys (+0x1a70..+0x1a7f), stick, camera yaw and rows, gates, the three
  statics and the player record before and after. Covers §3 bit for bit.
* `EngineTests attackoracle <file>`: `Extract/oracle/attack.bin` (CAT1), recorded 0x60460 and
  0x60720 calls with the class-key flag (the starts of §4.9 and §4.11).
* The special skill key has unit checks in `tests/EngineTests.cpp` (specialSkill, cooldown,
  specialSkillKey, selfBuff).
* §2 and §4.1–§4.12 apart from the ability starts have no oracle yet.

---

## Open questions

1. **+0x134 or +0x12c.** 0x8ae50 calls 0x9c050 only when +0x130 ≤ 0 and **+0x134** ≤ 0
   (instruction at 0x8bdaa). The dodge timer +0x12c does not gate it; CubeVulkan's current
   port gates on +0x12c. What sets +0x134 (named a stamina-regeneration lock in
   `creature-upkeep.md`) for the player was not traced.
2. **+0xbb4** scales the glider's lift (g = clamp(+0xbb4/1000 − 0.5, 0, 1)); which skill or
   stat it holds was not traced.
3. **Abilities named only by number:** 0x36 (cleared every call), 0x86/0x87 (lower jump),
   0x8c, 0x9a, 0x9b, 0x7d, 0x57/0x58, 0x92. Their meaning comes from other specs or is
   unknown.
4. **0x5e890(P, 3)**, the special item that allows riding the pet, and **0x33f1e0** (nothing
   under the mouse) were not specified.
5. **0xa0420's last two arguments** for the pet call sound (a float register and the byte 1)
   were not resolved; the float is probably the volume 1.0.
6. **The aim assist's pitch** (§4.7 step 4) is computed and discarded; whether another routine
   reads it was not checked. The angles at controller +0x1d80 / +0x1d88 are named from this
   use only.
7. **§4.12 looks dead:** with the move lists of §4.5 the second move can never be 0x1d and the
   first is 0x1e only if 0x5fc40 returns it, which `creature-ai.md` says it does not.
8. **The gamepad:** which XInput controls write the stick floats and the key bytes (beyond the
   Y button for R) was not traced; the input routine is 0x1355a0.
9. **Mouse button numbers** (0, 1, 2 for attack, strong attack, dodge) are the binding codes;
   that 2 is the middle button is assumed.
10. **The charge-target rule** (§4.5, last paragraph) lets a later creature of any hostility
    replace the first hostile one when it is nearer; it reads like a slip in the game but is
    what the code does.
