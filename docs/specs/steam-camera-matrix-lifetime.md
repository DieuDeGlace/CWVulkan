# Steam Game camera-matrix initialization and lifetime

Independent native-only supplement. Addresses are RVAs in the supported Steam
executable. This note covers ordinary Game construction, the direct frontend
transitions, and the early-speech/late-camera boundary. Matrix formulas remain
in [steam-gameplay-camera.md §7](steam-gameplay-camera.md); fixed-anchor
initialization is separate in
[steam-camera-position-lifetime.md](steam-camera-position-lifetime.md).

## 1. CPU matrices and constructor evidence

Each record is 16 float32 entries, 64 bytes, in column-major storage:

| Game offset | Record |
| --- | --- |
| +0x1ea4 | Unshaken camera-to-world transform; also supplies targeting basis |
| +0x1ee4 | Unshaken full-world-to-view transform |
| +0x1f24 | Shaken, render-origin-relative view transform |
| +0x1f64 | Camera projection |

**No constructor matrix value is established for these records.** Ordinary
startup **0x1355a0** allocates a Game of size **0x2288** and invokes
**0x7ea80 at 0x136331**. The reached Game constructor does not initialize these
four matrices, does not invoke their camera rebuild, and supplies no identity,
zero, or other numeric default for them. Its direct initialization writes the
scalar immediately preceding this region at **0x7ee97**, Game+0x1ea0, then
resumes later member initialization at **0x7ee9e**, Game+0x1fa8. The four
matrices occupy +0x1ea4 through +0x1fa3.

In contrast, the fixed eye/look-at/origin fields are explicitly zeroed, and
the constructor's later angle assignments at **0x7f767–0x7f7a2** establish
90/0/180 displayed and target angles. Neither fact assigns the corresponding
matrices. The successfully completed Game constructor therefore does not give
a canonical numeric pre-camera matrix state. This does not establish particular
heap bytes or claim that newly allocated memory must contain nonzero values.
There is no owner-runtime oracle for those initial bytes in this audit.

## 2. Direct frontend transitions do not prime matrices

The reached direct handlers below do not assign any of the four matrices and
do not call **0xadcd0** to construct a new set:

| Transition | Native route | Relevant separate state change |
| --- | --- | --- |
| Title Start Game | **0x965e0**, entry 0 | Shows character selection |
| Title Continue | **0x965e0**, entry 3 | Loads the local creature and sets both orbit distances to 8 |
| Select existing character | Existing-record branch of **0x9f7c0** | Loads the local creature, resets angles to 90/0/180 and distances to 8 |
| New Character selection | Out-of-range-record branch of **0x9f7c0** | Resets/repositions the local creature, shows creation, resets angles to 90/0/0 and distances to 8 |
| Create Character | **0x9ea10** | Hides creation and saves/refreshes character data |

These direct operations retain whatever matrix contents the Game already has.
If an earlier camera update completed, its matrices survive the transition
until the next update. Resetting angles, changing the physical player position,
loading a save, or toggling frontend visibility does not itself rebuild them.
The reached player/save and anchor distinctions are described in the linked
position-lifetime note and frontend-title §6.1.

The separate **0x9f060** world-creation handler is not the ordinary Create
Character route. No matrix-reset behavior is inferred from its older
angle-reset description. Its unshown frontend status remains
frontend-characters §8.

## 3. First reached camera update assigns all four

The ordinary frame calls **0xadcd0 at 0xaaf02**. After updating angles,
distance, shake, retained look-at and final eye, this routine constructs its
matrices in the order below:

1. The block beginning **0xaec3c** initializes the complete camera-to-world
   matrix at +0x1ea4 to identity and then applies its eye translation and
   displayed-angle rotations.
2. The block beginning **0xaed3a** initializes the complete unshaken view at
   +0x1ee4 to identity, then applies inverse rotations and negative-eye
   translation.
3. The block beginning **0xaee21–0xaee43** updates negative look-at X/Y and
   initializes the complete shaken view at +0x1f24 to identity before its
   shake, inverse rotations and relative-eye translation.
4. Following the ordinary FOV update, **0xa0340 at 0xaf0a4** writes all 16
   entries of projection +0x1f64 from current FOV, viewport width/height,
   near 0.05 and far 1920. It does not update only selected entries of an
   otherwise retained projection.

There is no first-call test, matrix-valid flag, special initial-value priming,
or early return on zero elapsed time in this reached construction. These are
full per-call matrix rebuilds, not interpolation from previous matrix elements.
Zero elapsed time can leave smoothing coefficients zero while the matrix
construction still executes using the then-current camera state. The precise
transforms, projection convention and duration inputs are already specified
in gameplay-camera §§2 and 7; this note does not introduce alternate formulas.

## 4. Early consumers precede the new matrices

The ordinary Game frame calls the retained NPC speech/page consumer
**0xb40d0 at 0xaa1bc**, before camera **0xadcd0 at 0xaaf02**. Its admitted
bubble placement transforms the origin-adjusted world point through
**Game+0x1f24**, then, only for positive view Z, through **Game+0x1f64**, using
**0x2b680**. Admission, homogeneous division and placement remain
[steam-npc-speech-rendering.md §§3 and 10](steam-npc-speech-rendering.md).
An absent or ineligible bubble does not force a matrix read merely because
this frame stage is reached.

Consequently an admitted early bubble normally sees the preceding camera
update's matrices and matching retained origin pair. A direct frontend event
in §2 does not replace them before this phase. Before any completed camera
update, this audit cannot assign canonical numeric matrix inputs to such an
early consumer; it must not be described as guaranteed identity or zero.

Later scene submission **0xe2e00 at 0xad9d4** receives the newly built Game
matrices as inputs. It copies Game+0x1ee4 to renderer+0x35c, Game+0x1f24 to
renderer+0x39c, and Game+0x1f64 to renderer+0x3dc, with first stores at
**0xe2f4b/0xe2f81/0xe2fb7**. This is not a return path that initializes or
restores the Game matrices after rendering. The requested-world mismatch
reset **0xe8a30 at 0xad752** only clears renderer+0x4a4; it supplies no Game
matrix initialization or first-use priming.

## 5. Limits

This establishes the ordinary constructor, direct frontend handlers and reached
camera/scene boundary, not an exhaustive census of external callbacks, alternate
Game factories, arbitrary cloning, window/device recovery or other untraced
writers. Those paths remain unproved. No assumption about initial GPU constant
buffers, map/GUI temporary matrices, allocator clearing, first visible pixels,
or replacement-engine recovery is supplied. In particular, retained previous
camera data after a normal update is established; a numeric native default
before that first update is not.
