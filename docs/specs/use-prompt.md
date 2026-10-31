# The use prompt (use key E)

Behavioural specification of Steam Cube World, written from the decompiled executable and
the binary's constants. Routines are cited by RVA (image base 0x140000000). Offsets are byte
offsets into the record named. Times are milliseconds and distances are blocks. World
positions are 64-bit integers in 1/65536 of a block, so a position difference is multiplied
by 1/65536 (the float at 0x42e510) before it is compared with a distance in blocks. "d²"
always means a squared distance in blocks².

The subject is the routine **0x98180**. Each frame it decides what the use key would do, shows
that on the use key's quick button, and performs the action on the frame the key goes down.
The routine depends on what the controller is targeting, so this spec also covers the target
selection in 0x8ae50. It also covers the fallback logic in 0x8ae50 that runs when 0x98180
declines: ground items and the equipped vehicle item.

Vocabulary is shared with `skills-and-interactions.md` (creature record, interaction
records), `lore-and-artifacts.md` (static object record, Examine), `map-markers.md`
(controller, chardata, map), `conversation-script.md` (the conversation builder 0x2d9480) and
`plasma-format-and-scene.md` (set text 0x3348e0, set colour 0x334a20).

---

## 0. Conventions

### 0.1 Objects

| Name | Reached as | Notes |
|---|---|---|
| controller | `this` of 0x98180 and 0x8ae50 | the game controller |
| world | controller +0x8 | +0x40 creature list; +0x448 local **player**; +0x450 id → creature map (lookup 0xe720); +0x460 zone map (lookup by zone key, 0x16e00 / 0x2c3150) |
| chardata | player +0x1528 | the player's character data (map-markers §0.4) |
| use button | (controller +0x228) → +0x20 | the quick-button scene of the use key. Its text node is named `info` (wide string at 0x436390); a second text node is `count` (0x4363e8) |
| use button icon | controller +0x3e8, item at +0x1f8 | an item (0xa0 bytes) drawn on the use button; an empty item draws nothing |
| dialog window | controller +0x418 | page list at +0x1e8, pages 0x1b0 bytes (lore-and-artifacts §2.3) |
| crafting window | controller +0x340 | station mode (i32) at +0x2b4 |
| shop window | controller +0x350 | mode (i32) at +0x2b4 |
| window C | controller +0x360 | opened by class 0x83 (§3.6.3); identity not traced |
| customization window | controller +0x368 | |
| usable-item selector / Ring Menu | controller +0x310 | owner GUI **node** `selector`; content/use behavior is [slot-lifecycle §9](steam-inventory-slot-lifecycle.md#9-ring-menu-selector-construction-content-and-input), with call/input ordering in [Ring rendering §7](steam-ring-selector-rendering.md#7-frontend-roots-and-same-frame-input-order) |
| inventory grid | controller +0x330 | ordinary InventoryWidget |
| speech bubbles | controller +0x400 .. +0x408 | a vector of bubble pointers; bubble +0x210 = speaker creature id |
| the object at controller +0x10 | | named WorldMap in map-markers. Here it also holds a collection keyed by creature id (§3.3) and a selected id at +0x148; map flight mode is its byte +0x164 |

A **Widget window is visible** when its node at widget+0x178 has nonzero
current Display.visibility. Predicate0x9af00 receives that **node**, reading
node+0x68's visibility array+0xe0 at signed32 index+0xac. The Ring selector
G+0x310 is already a node, so its test uses that pointer directly, without
another+0x178. This own-node visibility test does not mean ordinary inventory
G+0x330 is open. **Opening** sets the tested visibility to1; **toggling**
sets it to1 if it was0, otherwise0.

### 0.2 Creature fields used (in addition to skills-and-interactions §0)

| Offset | Meaning here |
|---|---|
| +0x40, +0x44, +0x48 | the player's movement-intent vector (floats). 0x8ae50 zeroes it unless the player is gliding or in ability 0x79, so it comes from the movement keys |
| +0x5c | movement flags: bit 0 on the ground, bit 1 (also "landed"), bit 2 touching a wall |
| +0x68 / +0x6c | current ability / its elapsed time |
| +0x80 | flags: 0x1000, 0x4000, 0x200000, 0x400000 used below |
| +0x84, +0x88, +0x8c | size factors x, y, z |
| +0x140 | a float the revive sets to −0.5 |
| +0x14c | class byte |
| +0x19c | a byte that hides some creatures from targeting (§2.2) |
| +0x1a0 | for the player: the id of the creature being carried (§3.5) |
| +0x1a8 | the target creature id of the current ability (for creatures: an occupant / partner id, tested in §3.6.9) |
| +0x1b0, +0x1b4 | a zone key, 0x7fffffff when none (§2.6) |
| +0x1d8 | an item the player is holding up or using (ability 0x7c, vehicles) |
| +0x968 | an owner id compared with ground items' reservation |
| +0x990 | a float subtracted from the render height when aiming (§2.2) |
| +0x9a0 | stamina |
| +0x9d8 | the id of the player's active pet |
| +0x9f0 | the inventory (bags) |
| +0xaac | money (i32) |
| +0xab8 | pending interactions list |
| +0xb10, +0xb18, +0xb20 | render (interpolated) position |

**"Ability finished"** below means: the current ability's elapsed time (+0x6c) is strictly
greater than that ability's duration (0x4f220 on the creature). An idle player (ability 0)
counts as finished after its first frame, because the ability time keeps advancing.

**Size range** of a creature C (0x5ec30): if C's ability is 0x5d, it is 0.5 × size x + 40.
Otherwise it is 0.5 × the largest of the three size factors + 5. Constants 0.5 (0x4ac964),
5.0 (0x42e584), 40.0 (0x42e5ac). **Within talk range** means d²(player, C) < (size range)²,
using the logical positions (+0x10..+0x20) of both. **Within use range** means
d²(player, C) < (0.5 × C's size x + 5)².

### 0.3 Static object record (zone list at zone +0x60, 0xe0 bytes each)

| Offset | Meaning |
|---|---|
| +0x00 | type (i32) |
| +0x08, +0x10, +0x18 | position x, y, z |
| +0x20 | flags; bit 0 = rotated a quarter turn (x and y size swapped) |
| +0x24 | scale (float) |
| +0x28 | "active" byte. For doors it means *closed* (§3.7.2). For item-taking objects it means *not yet given* |
| +0x30 | occupant / owner creature id (i64) |
| +0x38 | the item it takes (0xa0 bytes; its type byte is at +0x38) |
| +0xd8 | how many of that item it takes (byte) |

An object's **box size** is 0x13a960: the base size of its type (0x13a520) times its scale.
Its **centre** is its position raised by half the box height.

### 0.4 Ground item record (zone list at zone +0x30, 0xe0 bytes each)

| Offset | Meaning |
|---|---|
| +0x00 | the item (0xa0 bytes: type byte +0, subtype i32 +4, rarity byte +0x14) |
| +0xa0, +0xa8, +0xb0 | position x, y, z |
| +0xc4 | flags; bit 1 (0x2) = no automatic pick-up |
| +0xc8 | a float. Automatic pick-up waits until it is exactly 0 |
| +0xd4 | i32; 0 marks an empty slot |
| +0xd8 | reserved-for owner id (0 = anyone) |

### 0.5 Controller targeting state

| Offset | Meaning | Empty value |
|---|---|---|
| +0x1e58 | **targeted creature** id (i64) | 0 |
| +0x1e68 | **highlighted creature** id: set by the prompt when a creature prompt is shown | 0 |
| +0x1e70, +0x1e74, +0x1e78 | **targeted ground item**: zone x, zone y, index | (0x7fffffff, 0x7fffffff, 0) |
| +0x1e7c, +0x1e80, +0x1e84 | **targeted static object**: zone x, zone y, index | (0x7fffffff, 0x7fffffff, 0) |
| +0x1e88 | byte: the crosshair is over some creature's box this frame | 0 |

A zone key is (zone x, zone y), where zone x = floor(floor(x / 65536) / 64) (0x16e50); the
same applies to y. 0x2c3150 resolves a static object target to its record. It returns nothing
when the zone is not loaded or the index is outside the list.

### 0.6 The quick button and the key

* **Show text T**: set the text of node `info` of the use button to T (0x3348e0, mode 1).
* **Show colour K**: set the colour of node `info` (0x334a20).
* Every frame, before targeting, the game update 0xa5a50:
  * clears the targeting state to the empty values above, and clears +0x1e88;
  * sets the use button's `count` text to "" and its `info` text to "";
  * sets the use button icon to an empty item;
  * sets `info`'s colour to white (1, 1, 1, 1) (0x42e8c0).

  So a prompt that writes nothing shows a blank button.
* The use key is the key-state byte **controller +0x1a7b**. Key-state bytes start at +0x1a70
  in binding order (0x42670): +0x1a70 normal attack, +0x1a71 special attack, +0x1a72 class
  skill, +0x1a73 ultimate skill, +0x1a74 use quick item, +0x1a75..+0x1a78 move forward,
  backward, left, right, +0x1a79 jump, **+0x1a7a dodge**, **+0x1a7b use/interact**, +0x1a7c lamp,
  **+0x1a7d walk**, +0x1a7e call pet, and so on.
* **Pressed** means the key is down this frame and was up in the previous frame. 0x98180 keeps
  its own previous-state byte (global at 0x5515b4) and writes the current state into it on
  every call, whichever way the routine leaves. The fall-through code in 0x8ae50 (§4) keeps a
  separate one (0x5515a3), stored at the reached common continuation after
  the prompt/selector/fallback decision. Early returns before that stage do
  not update it; the exact gates are Ring rendering §7.2.

---

## 1. Per-frame order

1. **0xa5a50** (the game update) resets the button and the targeting state (§0.6). It then
   builds the creature candidate list (§2.1). It calls 0x8ae50 unless one of these holds: the
   world is not ready (world +0x210 is 0), the player's ability is 0x9c or 0x9d (flying with a
   flight master), or the frontend containers at controller +0x118 (title),
   +0x130 (character selection) or +0x170 (character creation) are visible. When
   0x8ae50 is skipped there is no prompt at all.
2. **0x8ae50** performs its existing early handling and internal admission
   checks, chooses targets (§2), then calls **0x98180** (§3). The exact
   focus/chat/ability early exits and enclosing call gate are
   [Ring rendering §7.2](steam-ring-selector-rendering.md#72-ordinary-use-outer-call-and-internal-early-exits).
3. If0x98180 returned0, a shown **Ring selector G+0x310** takes the
   Use-key branch first (§4.1), suppressing ground-item/vehicle fallback
   even without a press or with no candidates. Only a hidden selector
   reaches those later fallback branches (§4.2–§4.3). If the prompt returned1,
   its own continuation applies (§4.4). The independent Quick Item action4
   is later and separate from this prompt/Use decision; its rules and the
   subsequent Ring toggle/close/label/repeat ordering are
   [slot-lifecycle §9.5](steam-inventory-slot-lifecycle.md#95-use-and-quick-item-are-separate-rising-edge-routes)
   and [Ring rendering §7.3](steam-ring-selector-rendering.md#73-ordered-frame-stages).

---

## 2. Target selection (0x8ae50)

### 2.1 Creature candidates (built in 0xa5a50)

Every creature in the world list whose health (+0x180) is above 0 and whose render position
(+0xb10) is within 160 blocks of the player's position (d² ≤ 25600, 0x438008). It must also
pass the renderer's visibility test (0xe08c0 on controller +0x18) for a sphere of radius
0.6 (0x42e708) × the larger of size x and size y. The list is sorted by that distance.

### 2.2 Creature under the crosshair, and the nearby creature

The camera position is controller +0x1dc0 (world fixed point). The camera's forward unit
vector F is the floats at controller +0x1ec4, +0x1ec8, +0x1ecc. The view matrix is at
controller +0x1f24 and the projection matrix at +0x1f64. The render-origin offset added to x
and y before transforming is controller +0x1df0 / +0x1df8.

A candidate C is skipped when any of these holds:

1. C is the player.
2. The player's ability is 0x76 and C is the player's pet (+0x9d8).
3. The player's ability is 0x23 and has not finished, and C's hostility kind is 1 or 7.
4. C is hostility kind 8 with race 0xf4..0xf7 (0x5ee50) and its byte +0x19c is set.
5. C's race is 0x11d, or C's race is 0x12e and its byte +0x19c is set.
6. The player's ability has not finished and is "offensive" (0x5eea0: every ability except
   0x33, 0x34, 0x50, 0x6d, 0x6f..0x73, 0x86, 0x87, 0x8d..0x90, 0x92, 0x95), and C is not
   hostile to the player (hostility test 0x50550).
7. The player's ability has not finished, is 0x86 or 0x87 (0x5eed0), and C is hostile to the
   player.

For the remaining candidates, the **aim point** P is C's render position with z lowered by the
float at C +0x990. If P fails the visibility test 0xe08c0, C is skipped. Otherwise:

* **Nearby creature.** If d²(player position, C's render position) < 16 (0x4aca34), and this
  is the smallest such d² so far, C becomes the nearby creature.
* **Crosshair test.** Build the box centred on P with extents (size x + 1, size y + 1,
  size z + 1) (1.0 at 0x4ac980). Transform each of its 8 corners by the view matrix. Keep only
  corners whose view-space third component is > 0, and project those. The crosshair is
  **inside** the box when the projected corners include one with x < 0, one with y < 0, one
  with x > 0 and one with y > 0. In that case:
  1. Set +0x1e88 = 1.
  2. Test line of sight from the camera to P (0x2b5d90, at most 256 blocks, 0x42e5c4). The
     sight test passes when the two points are closer than 0.01 blocks. It fails when they
     are farther apart than the limit. Otherwise it casts a ray through the blocks (0x2d1d20)
     and passes if nothing is hit before the full distance minus 0.01.
  3. If there is line of sight: use C's **unadjusted raw drawn tuple** D for
     depth/ranking: t = (D − camera) · F. If t ≥ 0, take the distance from D to the
     point camera + t·F (the perpendicular distance to the view ray). If it is the smallest
     so far, the **targeted creature** (+0x1e58) becomes C.

So among the creatures under the crosshair, the one closest to the centre line of the view
wins. There is no limit on its distance from the player here; each prompt has its own range
(§3.6).

#### 2.2.1 Exact fixed endpoint of creature LOS

Stable bounded native-only clarification of **0x8baef**, after the
crosshair-box flag write. Read the candidate's stored **float32 step
offset+0x990** at0x8ba4e. Flip its float sign bit at0x8ba56, using
mask0x80000000 from0x4acb30. Multiply that negated float by
**float65536** at0x8ba6d. Truncate this **float32 product toward zero to
signed64**, instruction0x8ba71. There is no double product, floor,
signed32 narrowing or conversion of the absolute drawn Z to float.

The fixed endpoint copies all three full signed64 drawn components
**+0xb10/+0xb18/+0xb20**. X and Y remain unchanged. Add the converted
signed64 offset to drawn Z at **0x8bab7**, retaining the low64 bits.
For finite representable offset conversions this is also expressible
as drawnZ minus the toward-zero signed64 conversion of
float32(stepOffset times65536). The actual native sequence negates
the float first and then adds its converted offset; an overflow-checked
or wider coordinate expression must not alter its low64 result.

The LOS start is the retained full fixed eye tuple
**Game+0x1dc0/+0x1dc8/+0x1dd0**, not the look-at point or player physical
position. The helper0x2b5d90 receives float maximum**256**,
word0x43800000 at0x42e5c4, and flag**0**, from
**0x8bac1–0x8baef**. It subtracts fixed endpoint/start components in64
bits before its conversions; no float absolute-position subtraction
is introduced here. Its precheck, float norm, three divisions and
L−float0.01 acceptance are the same arithmetic detailed in
[camera §6.4](steam-gameplay-camera.md#64-complete-visibility-guard0x2b5c00-for-the-eight-eye-corners),
but its terrain helper is0x2d1d20 with flags0,0. Therefore this LOS
does **not** test block flag0x40.

Derived example, not a runtime capture: drawnZ655360 and step-offset
word0x3dcccccd (float0.1) produce negative scaled float
**−6553.60009765625**, truncated offset**−6553**, and fixed endpoint
Z**648807**. Replacing this with subtraction in absolute float block
coordinates can produce a different fixed endpoint.

The adjusted endpoint belongs to this LOS call. After it succeeds,
depth and perpendicular ranking use the **unadjusted** drawn tuple,
as corrected above and specified precisely in
[frame-order §8.3](steam-gameplay-frame-order.md#83-creature-depth-is-an-independent-per-pass-minimum).
This supplement does not replace the earlier projection/box arithmetic
with the LOS endpoint-conversion sequence.

### 2.3 Static objects

Scan every loaded zone whose key lies in the rectangle covering the player's x ± 8 blocks and
y ± 8 blocks (inclusive). In each, scan the static object list in order. Index i is the
position in that list.

An object is **eligible** when its item-type byte (+0x38) is not 0, or its type is in this
set (bit masks 0x4000001f8600079 for types below 0x3b and 0x8ff9003fb01f for 0x53..0x82):

> 0x00, 0x03, 0x04, 0x05, 0x06, 0x15, 0x16, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x3a,
> 0x53, 0x54, 0x55, 0x56, 0x57, 0x5f, 0x60, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x73,
> 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x82

For an eligible object O, let d² = d²(player position, O's position). If d² > 16, skip O.
Otherwise let c² = d²(camera, O's centre). Then:

1. **Nearby object.** If d² is smaller than the best nearby-object d² so far (or there is none
   yet), test line of sight from the player's position to O's centre, at most 8 blocks
   (0x4aca30). If there is sight, O becomes the nearby object and the **nearby creature is
   forgotten**.
2. **Crosshair object.** Skip this step if an object or ground item has already been chosen
   under the crosshair with camera distance² ≤ c². Otherwise test line of sight from the
   player's position to O's centre (8 blocks). If there is sight, take O's box. Its x and y
   sizes are swapped when flag bit 0 of +0x20 is set. Its corners are O's position plus
   (a·sx − sx/2, b·sy − sy/2, k·sz) for a, b, k in {0, 1}. Apply the same crosshair test as
   in §2.2. If the crosshair is inside: the targeted creature becomes 0, the **targeted
   object** becomes (zone x, zone y, i), and c² becomes the best camera distance.

A static object under the crosshair therefore beats a creature under the crosshair, provided
it is within 4 blocks of the player and visible from the player.

### 2.4 Ground items

The zone rectangle is the same. Each ground item G in the zone's list (index i) is skipped
unless it is valid (0x2b8140). G is valid when its item type is not 0 and its +0xd4 is not 0.
G is not valid when any of these holds:

* G is of type 0x17 and the player already carries one (0x5e780);
* G is of type 0x1e and its 16-block cell (from its position) is in chardata +0xa8;
* G is of type 0x18 and the player already owns special item *subtype* (0x5e890);
* G is reserved (+0xd8 not 0) for someone other than the player's +0x968.

Let d² = d²(player position, G's position). If d² > 16, skip G. Then:

1. **Automatic pick-up.** If G's type is 0x0c or 0x0e and flag bit 1 of +0xc4 is clear: G is
   never a target. If its +0xc8 is exactly 0, a kind-7 interaction (§5) for G is appended to
   the player's pending list. This is skipped when the list already holds a kind-7 record
   with the same zone x, zone y and index and an equal item (item comparison 0x4df60).
2. **Otherwise** (the 10-block test against 100.0 at 0x42e5bc is always met here):
   1. c² = d²(camera, G's position). If the player may not carry more of G's item
      (0x50670, §4.2), add 16 to both d² and c².
   2. **Nearby item.** If d² is smaller than the best so far, G becomes the nearby ground
      item. The nearby creature **and** the nearby object are forgotten.
   3. **Crosshair item.** This step applies only if c² is smaller than the best camera
      distance so far, which is shared with §2.3. Test line of sight from the camera to G's
      position, at most 200 blocks (0x42e5c0). If there is sight, take the item's model from
      the world's model table (world +0x70, lookup 0x173b10); with no model, skip. Place the
      model's box (sizes at model +0x88, +0x8c, +0x90) as the drop is drawn: translated to
      G's position, turned by the item's drop orientation (0x10a330), scaled, and offset by
      −0.5 (0x42e80c) of its size. Apply the crosshair test of §2.2. If it passes: the
      **targeted ground item** becomes (zone x, zone y, i), the targeted creature becomes 0,
      the targeted object becomes empty, and c² becomes the best camera distance.

#### 2.4.1 Complete direct LOS call inventory inside0x8ae50

There are **four direct0x2b5d90 call sites** in this routine. Every one
supplies flag0, so each uses0x2d1d20 with flags0,0: effective type0/2
is passable and flag0x40 is ignored. This statement is restricted to
these LOS calls. The distinct earlier direct aim ray at0x8b26d supplies
flags0,1 and **does** honor0x40; see
[the positive terrain-ray contract](steam-positive-terrain-ray.md).

| Call RVA | Reached role | Fixed start | Fixed end | Float maximum |
|---|---|---|---|---|
|0x8baef|Creature crosshair LOS|Stored eye Game+0x1dc0 tuple|Candidate drawn tuple with the §2.2.1 Z adjustment|256, constant0x42e5c4|
|0x8cfbb|Nearby static-object candidate replacing the previous nearby object|Current local player's physical tuple Creature+0x10|Object center described below|8, constant0x4aca30|
|0x8d092|Static-object crosshair candidate after the camera-distance gate|Current local player's physical tuple Creature+0x10|Separately rebuilt object center described below|8, constant0x4aca30|
|0x8d923|Ground-item crosshair candidate after the camera-distance gate|Stored eye Game+0x1dc0 tuple|Ground record's full fixed tuple+0xa0/+0xa8/+0xb0|200, constant0x42e5c0|

For each object call, obtain the third float extent H returned by
**0x13a960(object)**. Multiply H by float0.5, then multiply that rounded
float by float65536; truncate toward zero to signed64. Add the result
to the object's full signed64 Z at+0x18, retaining low64 bits. Its full
fixed X/Y at+0x08/+0x10 remain unchanged. This is half-height above the
object origin, with no owner-model occupied-geometry center substitution.
The two independently reached constructions are
**0x8cf27–0x8cf8f** and **0x8cffe–0x8d066**. The0.5 constant is0x4ac964;
the8 limit is loaded at0x8ccb1. These two calls can both occur for the
same object when their separate candidate gates pass.

The ground call uses the position tuple directly, without a model-center,
half-height, drop-rotation, bob or step correction. Its stored eye start
is selected at0x8d7d6 and the200 constant is loaded at0x8d435. Model
lookup and the ground model's crosshair-box construction happen only
after the LOS succeeds; they do not change that already tested endpoint.

The later fallback selection in §2.5 reuses the retained nearby candidates;
it makes **no additional LOS call or fresh terrain ray**. In particular,
the nearby creature and nearby ground-item selection paths do not gain
the crosshair paths' LOS requirement merely because they later become
fallback targets. A nearby object's retained eligibility already includes
its0x8cfbb test. This is not a new general visibility guarantee for every
eventual target. Other routines/callers and nonfinite conversion cases
are outside this bounded call-site audit.

### 2.5 Fallbacks and the dead player

* When the player's health is ≤ 0, the targeted creature becomes 0 and the targeted object
  becomes (0x80000000, 0x80000000, 0). This happens between the object scan and the
  ground-item scan.
* After both scans, if there is no targeted creature and the targeted object is empty:
  * a nearby creature that still exists becomes the targeted creature, unless its hostility
    kind is 1 or 7;
  * a nearby object becomes the targeted object.
* If there is no targeted ground item, the nearby ground item becomes it.

In effect: the crosshair wins. Without a crosshair target, a friendly creature or an object
within 4 blocks is used, but any valid ground item within 4 blocks cancels both of those.

### 2.6 Side effect of targeting a creature

If the targeted creature has hostility kind ≠ 0, and it either has hostility kind ≠ 3 or has
class 0x9d or flag 0x1000, and its zone key (+0x1b0, +0x1b4) is not 0x7fffffff, then:
unless 0x5f4c0 already reports that zone for the player, the zone is added to chardata +0xb8
(marked missions) and +0x38 (discovered zones), and its note timer (chardata +0xe8) is set to
180000 ms (0x482fc800). This is not part of the prompt; it is listed so that it is not lost.

---

## 3. The prompt routine (0x98180)

The routine returns 1 when it has taken the key this frame (even with a blank text) and 0
when it has nothing. The first section that applies ends the routine.

### 3.1 Suppression

Return 0, with no prompt, when the dodge key (+0x1a7a) is held, the walk key (+0x1a7d) is
held, or the dialog window is visible.

### 3.2 Fall and Climb

* **Fall.** If the player's ability is 0x7a (climbing): show "Fall" and return 1. On press the
  ability becomes 0 (the ability time is not touched).
* **Climb.** Shown when all of these hold:
  * the player's ability has finished;
  * the dodge timer (+0x12c) is exactly 0;
  * the ability is not 0x78, 0x76 or 0x79;
  * the length² of the movement-intent vector (+0x40..+0x48) is > 0.1 (0x42e528);
  * bits 0 and 2 of +0x5c are "wall touched, not on the ground" (flags & 5 = 4);
  * stamina (+0x9a0) > 0.

  Show "Climb" and return 1. On press the ability becomes 0x7a.

Separately, at the start of 0x8ae50, a climbing player whose wall bit (4) is clear and who has
bit 0 or bit 1 set is put back to ability 0.

### 3.3 Abilities that complete here (no prompt)

These run before Revive whenever their condition holds. They never show text or end the
routine.

* **Ability 0x96 ended.** The player's ability is 0x96 and its time is greater than its effect
  time (0x4ead0 with −1). Set ability and time to 0. Find the creature whose id is the
  player's +0x1a8. If it exists:
  1. store a copy of it in the controller +0x10 collection, keyed by its id (0x2f4290);
  2. play sound 0x30 at the camera position, volume 1 (0xa0420);
  3. set the collection's selected id (+0x148) to its id;
  4. set controller +0x20c8 to its position in the collection's list (0x93d10).
* **Ability 0x97 ended.** Same condition with 0x97. Set ability and time to 0. Look up the
  player's +0x1a8 in the collection (0x9c000). If found:
  1. remove it (0x2f7ef0);
  2. play sound 0x3c at the listener (0xa03f0);
  3. set the selected id to the entry at index +0x20c8 of the list (wrapped into range,
     0x9bf60), or 0 if the list is empty.

### 3.4 Revive

If the player's health (+0x180) is ≤ 0: show "Revive" and return 1. On press:

1. Revive the player (0x65fb0).
2. Store the player's position as the death position (chardata +0x290..+0x2a0).
3. Set the tombstone timer (chardata +0x28c) to 300000 ms (immediate 0x48927c00).
4. Move the player to the respawn position (0x2c2860).
5. Set the player's +0x140 to −0.5.
6. Set health to the maximum health (0x5fac0).

While dead, the game update also forces the player's ability to 0 each frame.

### 3.5 Throw

If the player's ability is 0x80 (carrying) and the creature whose id is the player's +0x1a0
exists: show "Throw" and return 1. On press the ability becomes 0x81 and its time 0.

### 3.6 The targeted creature

This applies only if the targeted creature (+0x1e58) resolves to a creature C. The checks run
in order. A check that matches shows its text and returns 1. A check that fails passes on to
the next, and after the last one the routine goes on to §3.7.

Unless noted, a matching check also sets the **highlighted creature** (+0x1e68) to C's id.
Pressing **queues a kind-3 interaction** for C (0x98040, §5).

#### 3.6.1 Pick up — "Pick up"

All of these hold:

* at least one of:
  * C has flag 0x4000;
  * C has hostility kind 1 and ability 0x82;
  * C is the player's pet (+0x9d8) and its race is not in the set of 0x5ef80 (§6.3);
* d²(player, C) < 25 (0x42e5a4).

On press: the player's +0x1a0 = C's id, the player's ability = 0x80, time = 0.

#### 3.6.2 Townsperson within talk range

C has hostility kind 3 and is within talk range.

* **Petrified** (flag 0x400000): show "Examine". The highlighted creature is not set. On
  press: build the token script `define_model` *C's race*, `he_is_petrified`. Show it in the
  dialog window (0x1694e0 into the dialog page list) with the seed C's id mod 100000 and an
  empty speaker name. No interaction is queued.
* **Otherwise**: show "Talk". On press, by C's class (+0x14c):

| Class | Action |
|---|---|
| 0x80, 0x81, 0x82, 0x9c | **Shop.** Open the shop window. Reset the interface panel at controller +0x118 to page 0 (0x102db0). Controller +0x1e98 = C's id and +0x1ea0 = the class. Fill the shop (0xb75d0): if the shop window's mode is 1, it lists the controller's list at +0x21b0. Otherwise the stock comes from C: class 0x81 → 0x2acf90, 0x82 → 0x2ac6e0, 0x9c → 0x2acbd0 (given C's id), any other → 0x2acdd0. The window is then refreshed (0x280a40, 0x280c80). No interaction is queued. |
| 0x83 | Open window C (controller +0x360). Reset the panel (0x102db0). No interaction. |
| 0x9a | **Flight master.** Controller +0x668 = 1 (the map is open) and the map's flight mode (map +0x164) = 1. No interaction. |
| anything else | **Conversation**, below. |

**Conversation.**

1. If a speech bubble for C already exists, the press only speeds up the text and nothing is
   queued. Let *shown* be the bubble's revealed count (+0x200, float) and *total* the length
   of its current page (0x290430). If *shown* ≤ *total*, add 100000 (0x43800c) to *shown*;
   otherwise set *shown* to *total*.
2. Otherwise build C's conversation script (0x2d9480, conversation-script §3). If it has any
   lines:
   1. Render it to pages (0x1694e0) with the seed C's id mod 100000 and an empty speaker name.
   2. Create C's speech bubble from the pages (0xa3170). Pages whose +0x20 is set go to the
      dialog window at controller +0x378 instead, and that window's +0x200 is set to C's id.
3. Queue a kind-3 interaction for C, whether or not the script was empty.

#### 3.6.3 Pet — "Pet"

All of these hold:

* C has hostility kind 4 or 1;
* C's race category (0x65750) is 4 (§6.2);
* the player's ability has finished;
* C's ability (+0x68) is 1;
* C is within talk range.

On press: the player's ability = 0xa2, time = 0, and a kind-3 interaction is queued.

#### 3.6.4 Interact — "Interact"

C has hostility kind 4 and is within talk range. Pressing queues kind 3.

#### 3.6.5 Open (three cases) — "Open"

The cases are tested in this order, each within talk range. Each queues kind 3.

1. C's race is 0xb5..0xb8 (0x5ee10).
2. C's race is 0xf5.
3. C has hostility kind 8 and race 0xc2..0xc5 (0x5edb0), or C's race is 0x10f or 0x111.

#### 3.6.6 Use — "Use"

C has flag 0x200000 and is within talk range. Pressing queues kind 3.

#### 3.6.7 Use (shrine) — "Use"

C's race is 0x10c and C is within talk range. On press: the two bytes at controller +0x668 and
+0x669 are both set to 1. That opens the map in shrine-choice mode (map-markers §0.2). No
interaction is queued.

#### 3.6.8 Use (campfire creature) — "Use"

C's race is 0xb9 and C is within **use range**. The press does the same as the static
campfire (§3.7.6, crafting station mode 3). No interaction is queued.

#### 3.6.9 Sleep — "Sleep"

C's race is 0xbb or 0xbc, C is within use range, and C's +0x1a8 is 0 (nobody is using it).
Pressing queues kind 3.

#### 3.6.10 Examine — "Examine"

C has hostility kind 8 and is within use range. Pressing queues kind 3.

### 3.7 The targeted static object

Resolve the targeted object (0x2c3150). If there is none, return 0.

#### 3.7.1 Objects that take an item (checked first)

These are objects whose item-type byte (+0x38) is not 0 and whose active byte (+0x28) is not
0. Their type does not matter. The use button icon becomes that item. The text is the item's
name (0x1628b0). When the count (+0xd8) is 2 or more, the text is *count* + " x " + *name*
(" x " at 0x437c70).

* If the player carries fewer than +0xd8 of the item (0x5e780): also show the colour
  (1.0, 0.3, 0.3, 1.0) (0x4380a0). Return 1; the press does nothing.
* If the player carries enough, on press:
  1. If the object's type is not 9, remove that many of the item from the player (0x648b0).
  2. Play sound 0x3e at the listener (0xa03f0).
  3. Append a kind-5 interaction (§5) carrying the targeted zone x, zone y and index *and a
     copy of the object's item* to the player's list (0x129a0).

  Return 1.

An object whose active byte is 0, or that has no item, goes on to §3.7.2. The full
behaviour, including lore braziers, is in lore-and-artifacts §2.1.

The requirement count at O+0xd8 is **unsigned byte**, zero-extended at
0x99a56/0x99ba2 for both the0x5e780 test and0x648b0 removal call.
For a normal empty inventory, required count0 passes the sum test even
without a matching item. Non-type9 objects still call0x648b0 with0;
object type9 skips removal but still undergoes the sum test. The helper
does not early-return for request0 and can default-clear matching empty
records; its exact full-item, tab/slot traversal and clearing semantics
are steam-inventory-slot-lifecycle §3.2/§3.4. No refresh/sort occurs in
this producer, and the host kind5 consumer does not repeat the removal.

#### 3.7.2 By type

"Queue kind 5" means: append a kind-5 interaction holding only the targeted zone x, zone y
and index (no item) to the player's list. "Mine" means the object's +0x30 equals the
player's id.

| Type(s) | Condition | Text | On press | Returns |
|---|---|---|---|---|
| 0x00 | the player owns special item 4 (0x5e890) | none | — | **0** |
| 0x00 | otherwise | "Examine" | dialog `gift_of_the_goddess` (below) | 1 |
| 0x03, 0x04, 0x05, 0x06, 0x15, 0x54, 0x55, 0x56, 0x57, 0x59, 0x5a, 0x69 (doors) | active byte ≠ 0 | "Open" | queue kind 5 | 1 |
| same | active byte = 0 | "Close" | queue kind 5 | 1 |
| 0x16 (chest) | see §3.7.3 | "Open" or none | §3.7.3 | 1 |
| 0x1c, 0x1e, 0x1f, 0x73 (seats) | not mine | "Sit" (0x437c88) | queue kind 5 | 1 |
| same | mine | none | queue kind 5 (stand up) | 1 |
| 0x20, 0x5f, 0x60 (beds) | not mine | "Sleep" | queue kind 5 | 1 |
| same | mine | none | queue kind 5 | 1 |
| 0x3a | not mine | "Teleport" | nothing | 1 |
| 0x3a | mine | none | nothing | 1 |
| 0x53 | not mine | "Campfire" | crafting station 3 (§3.7.6) | 1 |
| 0x53 | mine | none | nothing | 1 |
| 0x62 | not mine | "Furnace" | crafting station 5, then if the window is now visible refresh it (0x280a40, 0x280c80) | 1 |
| 0x63 | not mine | "Anvil" | crafting station 0 | 1 |
| 0x64 | not mine | "Spinning Wheel" | crafting station 5 | 1 |
| 0x65 | not mine | "Loom" | crafting station 1 | 1 |
| 0x66 | not mine | "Saw" (0x437d20) | crafting station 5 (the mode is set even if it is already 5) | 1 |
| 0x67 | not mine | "Workbench" | crafting station 0 | 1 |
| 0x62..0x67 | mine | none | nothing | 1 |
| 0x68 | not mine | "Customization Bench" | reset the customization window (0x294c50: its +0x210 = −1, +0x37c = 0), then toggle it | 1 |
| 0x68 | mine | none | nothing | 1 |
| 0x76..0x7e, 0x82 (lore objects) | — | "Examine" | find and tell the lore tale (lore-and-artifacts §2.3) | 1 |
| any other type | — | none | — | **0** |

Notes:

* Door types 0x59, 0x5a and 0x69 are not in the targeting set (§2.3). They can only be
  targeted through the item rule, and then §3.7.1 applies instead. So in practice their door
  prompt does not appear.
* Types 0x1b and 0x1d are targetable but fall to "any other type". The routine returns 0, so
  the ground-item and vehicle logic (§4) gets the key.
* **gift_of_the_goddess** (type 0). Build a dialog from the script key `gift_of_the_goddess`
  (0x167c80) into the dialog page list with an empty speaker. On its last page (0x1b0 bytes),
  set the byte at page +0xd8 to 0x18, the int at page +0xdc to 4 and the int at page +0xd4 to
  1. These are offsets −0xd8, −0xd4 and −0xdc from the end of the list. The page probably
  carries an item at +0xd8 (type 0x18, subtype 4, count 1 at +0xd4) as a reward. Per lore-and-artifacts §2.2 this branch is unreachable, because 0x5e890 always
  reports special item 4 as owned.

#### 3.7.3 Chests (type 0x16)

Only when the active byte is not 0:

1. Let (cx, cy) be the chest's 16-block cell (0x137e0 on its x and y).
2. If (cx, cy) is already in chardata +0xa8, stop (no text, return 1).
3. Find the zone (0x16e00 with the targeted zone x, y). In its chest-content map (zone data
   +0x450, keyed by object index) find the targeted index. If there is no entry, or the
   entry's item (entry +0x20) has type 0, stop (no text, return 1).
4. Show "Open". On press:
   1. Insert (cx, cy) into chardata +0xa8 (0x745e0).
   2. Copy the entry's item. If its type is 0x1e, replace it with the concrete item that
      0x10a720 makes from it for the player.
   3. Build the token script `define_item` *item*, then `you_received_the_item` if the
      item's type is 0x17 or 0x18, or its type is 3..9 or 0x1a and its rarity (+0x14) is above
      2. Otherwise the second token is `you_received_an_item`. Show it in the dialog window
      (0x1694e0) with the seed rand() mod 10000 and an empty speaker.
   4. If the player's ability is 0 or has finished: the ability becomes 0x7c (holding the
      item up), time 0, the player's +0x1d8 = the item, and controller +0x2050 = 60000.
   5. If the item's money value (0x109070) is 0, add one of it to the inventory (0x4e810 on
      +0x9f0, count 1, slot −1) and announce it (0x9d6f0). Otherwise add the value to the
      player's money (+0xaac). The value is non-zero only for type 0x1f. For that type it is
      ((h mod 100000) / 100000 + rarity + 1) × 10, where h is a xorshift of (+8 + 0xcc72f):
      shift left 13, then right 17, then left 5, each step xor-ed in.

   No interaction is queued: chests are opened locally.

#### 3.7.4 Lore objects

See lore-and-artifacts §2.2–§2.3. No interaction is queued; lore is credited when the last
page is shown.

#### 3.7.5 Teleport

The prompt "Teleport" is shown but pressing does nothing in this routine.

#### 3.7.6 Crafting stations

"Crafting station *m*" means:

1. Toggle the crafting window.
2. If its mode (+0x2b4) is not *m*, set it to *m* (0x89580, which also resets the window's
   selection and refreshes it).
3. Refresh the crafting lists (0xb2180).

The modes found are: 0 anvil and workbench, 1 loom, 3 campfire (and the race 0xb9 creature),
5 furnace, spinning wheel and saw.

---

## 4. When the prompt routine returns 0 (0x8ae50)

### 4.1 Usable-item selector / Ring Menu open

This checks the **selector G+0x310**, not InventoryWidget G+0x330.
When shown, the use-key rising edge resolves controller+0x20c0 into
the fresh0x93e60 list of nonzero category1 canonical slots, calls
0xb7740 on that live slot, then hides the selector if that list was
nonempty. An empty list leaves it shown. It suppresses the later ground
pickup/vehicle fallback in this no-prompt branch even without a use
edge or with an empty candidate list. This branch is not opened by ordinary
InventoryWidget G+0x330 visibility. Selection, close and independent Quick
Item action4 details are [slot-lifecycle §9](steam-inventory-slot-lifecycle.md#9-ring-menu-selector-construction-content-and-input);
outer admission and same-frame ordering are
[Ring rendering §7](steam-ring-selector-rendering.md#7-frontend-roots-and-same-frame-input-order).

### 4.2 Targeted ground item

After prompt0 with the Ring selector hidden, if a ground item G is targeted,
resolve it (0x2b8fd0); if it is gone, stop. The use button
icon becomes G's item.

* **Reserved**: G's +0xd8 is not 0 and differs from the player's +0x968. Show the colour
  (1.0, 0.1, 0.1, 1.0) (0x438060) and "Reserved". The press does nothing. (Targeting already
  rejects such items, §2.4, so this is a safety net.)
* **Full**: the player may not carry more of it (0x50670), or the bags have no room (0x8a370
  on +0x9f0 with count 1). Show the colour (1.0, 0.3, 0.3, 1.0) and "Full".
* Otherwise show "Pick up".

**May carry more** (0x50670): this is false only for item types 0x01, 0x0b, 0x15 and 0x18
when the count the player already has reaches a cap. The count adds every inventory stack
equal to the item and, if the equipped quick item (+0xa0c) is equal, its count (+0xa08). The
caps are:

| Item | Count that blocks |
|---|---|
| type 0x15 or 0x18 | > 0 |
| type 1 subtype 0x0c or 0x0e | > 4 |
| type 0x0b subtype 0x27..0x29 | > 9 |
| type 0x01, 0x0b, 0x15 or 0x18 otherwise | > 49 |

**On press:**

1. If the player may not carry more: add the chat line "You can't carry more of these
   items.\n" (0x4362f0) in the colour (1.0, 0.2, 0.2, 1.0) (0x438070). Play sound 0x34 at the
   player's position, volume 1 (0xa0420).
2. Else if the bags have room: append a kind-7 interaction (§5) holding a copy of G's item and
   the targeted ground item's zone x, zone y and index (0xa2130).
3. Else, by the bag the item belongs to (0x5f100): 0 → "Your equipment bag is full.\n",
   2 → "Your item bag is full.\n", 3 → "Your ingredient bag is full.\n",
   4 → "Your pet bag is full.\n" (any other value: no message). Each goes in the same colour
   as in step 1. Then play sound 0x34 at the player's position.

### 4.3 Nothing targeted: the vehicle item

This applies after prompt0 with the Ring selector hidden, when no ground
item is targeted and the dialog window is not visible. Chardata
+0x148 holds an item: type byte at +0x148, subtype i32 at +0x14c. It is the equipped vehicle
or special item.

* If its type is not 0: the use button icon becomes it, and the targeted ground item is
  cleared. If (ability 0x78 and subtype 1) or (ability 0x79 and subtype 2), show "Cancel".
  Otherwise show the item's short name. That is the name from 0x16af30 (by type and subtype),
  formatted by 0x16be20 with the style `default` (0x4363f8).
* On press:
  * The ability becomes 0 when any of these holds:
    * ability 0x78 with subtype 1;
    * ability 0x79 with subtype 2;
    * ability 0x76 with subtype 3, while the movement-intent length² is < 0.001 (0x42e514);
    * ability 0x7a while the item's type is 0.
  * Otherwise, if the item's type is not 0: the player's +0x1d8 becomes the item, and the
    ability becomes 0x78 for subtype 1, 0x79 for subtype 2, and 0x89 for anything else.
    Its time becomes 0.

### 4.4 When the prompt routine returned 1

When the **Ring selector node G+0x310** is shown, this continuation skips
both ground-target clearing and the unused lookup below. It tests the
selector, not InventoryWidget G+0x330. With selector hidden, clear the
targeted ground item; a press while alive, with the targeted object's
zone x ≥0 and the dialog window closed, calls0x2c3150. That call's result
is unused, so it has no effect. The later independent Quick Item action4
remains separate as described in §1 and slot-lifecycle9.5.

---

## 5. Interaction records produced

Records are the 0xc8-byte interaction records of skills-and-interactions §2.1. They are
appended to the end of the player's pending list (+0xab8). Fields not listed keep the
defaults of 0x9660: an empty item, zone x/y/index = −1, target id 0, extras 0, +0xc6 = 1.

| Kind | Made by | Item (+0x00) | Zone x, y, index (+0xa0, +0xa4, +0xa8) | Target id (+0xb0) |
|---|---|---|---|---|
| 3 | 0x98040: every creature prompt marked "queues kind 3" | empty | −1, −1, −1 | C's id |
| 5 | static objects (§3.7.1, §3.7.2) | the object's item for §3.7.1; otherwise empty | the targeted object | 0 |
| 7 | ground items (§2.4 automatic, §4.2 press) | a copy of the ground item's item | the ground item's zone and index | 0 |

How the handler 0x2c3710 treats kinds 3, 5 and 7 is outside this spec. In particular, what
kind 3 does for each creature kind (vendors, chests-as-creatures, beds, pets) and what kind 5
does for doors, seats and beds is not traced here.

---

## 6. Data

### 6.1 Constants

| RVA | Value | Use |
|---|---|---|
| 0x42e510 | 1/65536 | position units to blocks |
| 0x42e528 | 0.1 | Climb: movement-intent length² |
| 0x42e514 | 0.001 | vehicle cancel: movement-intent length² |
| 0x42e5a4 | 25.0 | Pick up range² (5 blocks) |
| 0x4ac964 | 0.5 | size range factor; box half-size |
| 0x42e584 | 5.0 | size range addend |
| 0x42e5ac | 40.0 | size range addend for ability 0x5d |
| 0x4aca34 | 16.0 | nearby range² (4 blocks); carry penalty |
| 0x42e5bc | 100.0 | ground item inner test (10 blocks) |
| 0x4aca4c | −1.0 | "no best yet" |
| 0x4aca30 | 8.0 | sight limit, player → object |
| 0x42e5c4 | 256.0 | sight limit, camera → creature |
| 0x42e5c0 | 200.0 | sight limit, camera → ground item |
| 0x4ac980 | 1.0 | added to creature size factors for the aim box |
| 0x42e80c | −0.5 | ground item model box offset |
| 0x438008 | 25600.0 | creature candidate range² (160 blocks) |
| 0x42e708 | 0.6 | creature visibility radius factor |
| 0x43800c | 100000.0 | speech bubble fast-forward |
| 0x4380a0 | (1.0, 0.3, 0.3, 1.0) | "not enough items" and "Full" colour |
| 0x438060 | (1.0, 0.1, 0.1, 1.0) | "Reserved" colour |
| 0x438070 | (1.0, 0.2, 0.2, 1.0) | chat warnings |
| 0x42e8c0 | (1, 1, 1, 1) | default prompt colour (reset each frame) |
| immediate | 300000.0 (0x48927c00) | tombstone timer on revive |
| immediate | 60000 | controller +0x2050 when a chest item is held up |
| immediate | 180000.0 (0x482fc800) | mission note timer (§2.6) |

### 6.2 Strings

| RVA | Text |
|---|---|
| 0x436390 | `info` (node name) |
| 0x4363e8 | `count` (node name) |
| 0x42ff38 | empty string |
| 0x437ba8 | Fall |
| 0x437bb8 | Climb |
| 0x437bc8 | Revive |
| 0x437bd8 | Throw |
| 0x437be8 | Examine |
| 0x437bf8 / 0x437c08 | `define_model` / `he_is_petrified` |
| 0x437c18 | Talk |
| 0x437c28 | Pet |
| 0x437c30 | Interact |
| 0x437c48 | Open |
| 0x437c58 | Use |
| 0x437c60 | Sleep |
| 0x437c70 | " x " |
| 0x437c78 | Close |
| 0x437c88 | Sit |
| 0x437c90 | Teleport |
| 0x437ca8 | `gift_of_the_goddess` |
| 0x437cd0, 0x437ce0, 0x437cf0, 0x437d10, 0x437d20, 0x437d28, 0x437d40, 0x437d68 | Furnace, Anvil, Spinning Wheel, Loom, Saw, Workbench, Customization Bench, Campfire |
| 0x437260 | Pick up |
| 0x437238 / 0x437250 / 0x437350 | Reserved / Full / Cancel |
| 0x436568, 0x436578, 0x436590 | `define_item`, `you_received_the_item`, `you_received_an_item` |
| 0x4362f0, 0x437270, 0x4372b0, 0x4372e0, 0x437320 | the five chat warnings of §4.2 |

### 6.3 Race sets

**Race category 4** (0x65750 returns 4; the "Pet" prompt): 0x13–0x24, 0x32, 0x35, 0x37–0x44,
0x4a, 0x4b, 0x57–0x5d, 0x62–0x64, 0x66–0x6b, 0x74, 0x91–0x93, 0x96–0x9f, 0xa1–0xa4, 0xa8–0xac,
0xae, 0xc8, 0xc9, 0xd1–0xd3, 0xd5, 0xd7, 0xd9–0xdc, 0xe0, 0x10e, 0x115, 0x125–0x127, 0x12f.
The other categories, for reference: 1 = 0x78–0x82, 0x12e; 5 = 0x11, 0x29, 0x2a, 0x60, 0x61,
0x75, 0xa5, 0xa6, 0xad, 0xff, 0x10d, 0x114; 6 = 0x49, 0x52, 0x55, 0x6e, 0xe1, 0xe2, 0xe5;
7 = 0x45–0x48, 0xc0, 0xc1, 0x107, 0x11f, 0x120, 0x122, 0x123. Category 2 is most of
0x83–0x12d (0x83–0x90, 0xaf–0xbe, 0xc2–0xc6, …). Category 3 is the humanoid races 0x00–0x10,
0x12, … and 0x128, 0x130, 0x131. The jump table is at 0x6579c.

**0x5ef80 set** (a pet of these races cannot be picked up): 0x13, 0x14, 0x16, 0x17, 0x19–0x28,
0x3f–0x43, 0x4a, 0x4b, 0x62–0x64, 0x66–0x69, 0x97, 0x9e, 0xa3, 0xa4, 0xa6, 0xad, 0xc8, 0xc9,
0xd1–0xd3, 0xd5, 0xd7, 0x106, 0x107, 0x10e, 0x125–0x127.

---

## Open questions

1. **Window C** (controller +0x360, opened by "Talk" on class 0x83) was not identified. The
   binary has an `AdaptionWidget` class, which is a candidate.
2. **What kind 3 does** for each creature prompt (Interact, Open, Use with flag 0x200000,
   Sleep, Examine on hostility 8, Pet, and plain Talk) is decided by the interaction handler
   0x2c3710, which this spec does not cover. The same holds for kind 5 on doors, seats and
   beds, including the "mine" press that has no text.
3. **Creature ability 1** is required for "Pet". Its meaning (a resting or idle pose?) was not
   checked. Ability 0x82 (with hostility 1, for "Pick up") and the meanings of player
   abilities 0x96, 0x97 and 0xa2 are also inferred from use only (taming/release and
   petting are likely).
4. **Movement-intent vector** (+0x40..+0x48): the spec infers it from how 0x8ae50 clears it and
   from the movement routine 0x9c050. Its exact source and units were not traced.
5. **Speech bubble fast-forward**: a second press only clamps the revealed count. Page
   advance must happen elsewhere (bubble timer or the dialog). Not traced.
6. **Teleport** (type 0x3a) shows a prompt with no action here. The teleport may happen
   elsewhere, for example by proximity. Not traced.
7. **Chest set**: a chest counts as opened when its 16-block cell is in chardata +0xa8. That
   is the set map-markers calls "known places". Two chests in one 16-block cell would share
   one flag.
8. The **drop orientation and scale** used for the ground-item crosshair box (0x10a330 called
   three times, 0x13050, 0x2b470, and the item-type split at types < 3, 3–4 and 0x0c/0x0d)
   are summarised, not specified. A port can use the box the drop is actually drawn with.
9. **Sound ids** 0x30, 0x34 and 0x3c were not matched to file names. The sound-name strings
   are not stored in id order.
10. The **vehicle item** subtypes (1 → 0x78 gliding, 2 → 0x79, 3 with ability 0x76,
    others → 0x89) are given as found. Matching them to hang glider, boat, reins and
    climbing spikes is likely but not confirmed.
