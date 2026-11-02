# Steam pose origins and body-particle coordinate lifetime

Independent native-only clean-room facts from the normal Game/scene/pose
callers, camera0xadcd0 and body helper0x90390. No implementation, SDK/
reconstruction, owner data/captures or game execution were used. Addresses
are RVAs. Fixed coordinates and origins are signed64 with65536 units per
block; additions/negations/subtractions below retain low64 results.

## 1. Final eye and look-at are distinct scene inputs

At the normal scene call **0xad9d4**, Game0xa5a50 supplies:

| Argument | Actual address and native load | Destination/use |
| --- | --- | --- |
| 2, RDX | **Game+0x1dc0**, loaded by0xad90e | Final fixed eye XYZ at+0x1dc0/+0x1dc8/+0x1dd0 |
| 3, R8 | **Game+0x1dd8**, loaded by0xad907 | Smoothed fixed look-at XYZ at+0x1dd8/+0x1de0/+0x1de8 |

Scene0xe2e00 copies **argument2, the final eye**, into Renderer+0x338/
+0x340/+0x348 at0xe2ec5/0xe2ed0/0xe2edb. Separately, it negates
**argument3's look-at X/Y**, storing Renderer+0x428/+0x430 at
0xe2f23/0xe2f31. These offsets are not negations of Renderer+0x338/
+0x340. There is no corresponding Z render-origin shift.

The four relevant roles are consequently:

| Role | Source |
| --- | --- |
| Shared sphere helper's fixed distance anchor | Renderer+0x338/+0x340/+0x348: final eye |
| Plane-query coordinate rebasing | Add Renderer+0x428/+0x430: negative look-at X/Y |
| Shaken render-origin-relative view | Renderer+0x39c copied from Game+0x1f24; constructed for coordinates shifted by negative look-at, with the relative eye translation and shake already specified in gameplay-camera §7.2 |
| Normal body-pose camX/camY | Pointers to Renderer+0x428/+0x430: negative look-at, not negative eye |

Projection+0x3dc, plane extraction and current range timing remain as in
[world pose invocation §2](steam-world-pose-invocation.md#2-current-scene-state-precedes-the-normal-actor-pass).
The final eye's role in the distance test does not change the coordinate
origin required by those planes or the origin-relative view matrix.
This corrects the older §2 wording that tied the render-origin negation
to its ambiguously named “fixed camera” tuple.

## 2. Normal pose arguments and per-actor origin cache

At the normal **0xd67b2 →0x17ab0** call, argument7 is a pointer to
Renderer+0x428, placed at0xd6780/0xd6787; argument8 points to
Renderer+0x430, placed at0xd6774/0xd677b. The arguments point to signed64
fixed values, not float block coordinates.

Common pose preparation copies their dereferenced values into animation
state **A+0x8d0/+0x8d8** at0x17f37/0x17f41. With A at Creature+0xc38,
these are Creature+0x1508/+0x1510. Constructor0x17010 initially stores0
in both fields at0x1711f/0x17126, and initializes the prepared marker0.
The common preparation subsequently sets that marker and records the
supplied origins as already specified in creature-pose §3.

The normal pose builds its part matrices in the supplied render-origin
coordinate frame. The complete part/model and point/center calculations
remain in [creature pose](creature-pose.md) and
[body particles §3](steam-body-voxel-particles.md#3-position-and-outward-velocity).
The origin fields describe the actor's common-preparation invocation;
they are not an automatic alias of the current Renderer or Game fields.

Failure of the normal actor gate means0x17ab0 is not called. That failed
route does not refresh these origin fields or regenerate the cached part
matrices. The prepared marker remains sticky. If no other preparation/
writer reaches the actor, its existing cache survives while the Game's
camera and renderer continue changing. This is cache retention, not a
per-frame rebasing operation performed for culled actors.

## 3. Game origin assignment and phase lifetime

Game constructor0x7ea80 initializes **Game+0x1df0/+0x1df8 to0**.
Normal camera0xadcd0 subsequently computes them from the smoothed
look-at, independently of the actor cache:

0xaee21 loads look-at X at Game+0x1dd8; its low64 negation is stored
at Game+0x1df0 by0xaee32. Look-at Y at+0x1de0 is loaded at0xaee39,
negated and stored at+0x1df8 by0xaee43.

There is no final-eye substitution and no Z-origin member in this pair.
See [gameplay camera §7.2](steam-gameplay-camera.md#72-render-origin-relative-shaken-view)
for the matching view construction.

The relevant normal Game-frame order is:

1. Accepted death candidates take the documented cached-part call sequence
   to0x90390, up to ten calls at0xa9110 through0xa95c4, before the current
   camera update. The existing hair/resource conditions still apply.
2. Ordinary burst/other later frame work and particle updating run under
   their existing contracts.
3. Camera0xadcd0 is called at **0xaaf02**, updating the Game origin pair.
4. The later scene call0xad9d4 receives the eye/look-at addresses in §1;
   its normal world pose pass can then refresh admitted actors' caches.

Thus a body birth reads the Game origin left by the preceding completed
camera update, ordinarily the previous frame's update. The body pass does
not update that pair per actor. Camera update, scene-input installation
and individual actor preparation are three separate last-writer lifetimes.
With the same completed camera output passed to the ordinary scene,
Game+0x1df0/+0x1df8 equal that scene's Renderer+0x428/+0x430; neither
equality is a guarantee that a culled actor's older cache used that origin.

## 4. Direct cached matrices and unused body-origin arguments

The ten accepted caller routes pass the **actual cached matrix addresses**
listed in body particles §1, rather than temporary rebased matrices. For
example the first call passes Creature+0xe30 at0xa90ff. The traced prelude
and part calls contain no fresh pose call, matrix translation repair or
per-actor restoration of Game+0x1df0/+0x1df8.

The caller does read Creature+0x1508/+0x1510 before each call. It converts
each signed64 value to float32, multiplies by float32 1/65536, and passes
these **block-space floats as body-helper arguments7/8**. At the first
call they occupy caller stack+0x30/+0x38, stored at0xa90e6/0xa90e0.
These are different arguments/types from the pose's signed64 pointers.

Crucially, **0x90390 does not read those origin arguments**. Its reached
incoming stack reads are tint pointer argument9 at0x90afb and style
argument10 at0x90872. Native frame addressing confirms no reads of the
argument7/8 slots, including arithmetic or address-taking. Merely passing
the cached origins therefore supplies no body-position correction.

The helper transforms the integer voxel and source center with the supplied
full matrix, using the existing §3 float/homogeneous-divide/fixed conversion
contract. It then subtracts **Game+0x1df0 at0x90b82** and
**Game+0x1df8 at0x90b8e** from transformed fixed X/Y. It does not subtract
the actor's cached origin fields or use the ignored origin floats. Z stays
as transformed. The input cached matrix itself is not rebased by this
process; clearing translation in the particle's retained extra-matrix copy
is not a mutation or correction of the source cached part matrix.

Once constructed, the live record owns the resulting fixed positions;
later Game-origin changes do not retroactively unbase them again. Its
ordinary draw subsequently rebases those fixed positions with that draw's
Renderer+0x428/+0x430, as specified in the shared particle contract.

## 5. Culled-cache consequence and exact bounds

Consider an actor prepared under origin O0. During a later frame, the
body phase still precedes that frame's camera update. The camera then
updates Game origin to O1, and the actor can fail the later normal pose
gate, leaving its matrices/origin cache at O0. On a subsequent body call,
the native path consumes that retained matrix together with the current
Game origin O1; no per-actor origin comparison or compensation is inserted.

**Conditional arithmetic reference, not a native runtime capture:** a
cached matrix point X of0.25 blocks under cached origin−100 blocks
corresponds to100.25 when subtracting that cached origin. If the Game
origin is now−101 blocks, the same quantized matrix point produces
**101.25 blocks** through0x90390. Relative to subtracting O0 from the
same quantized point, the native subtraction differs by O0−O1 in X/Y.
The existing death/prepared/frustum/resource gates still have to admit
the call; culling alone does not create a body event or bypass those gates.

The point/center quantization, outward velocity and all random-draw orders
remain unchanged. This supplement does not add a body producer, refresh
an offscreen pose, restore origins for an actor or normalize the retained
matrix to another coordinate frame. Alternate previews/whole-record copies,
all unrelated camera resets, special per-part retention and external
rebinding/failure paths were not exhaustively audited. The established
normal caller/helper behavior is the direct cached-matrix plus global
negative-look-at path above.
