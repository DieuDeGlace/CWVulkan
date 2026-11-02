# Steam normal world creature pose invocation

Independent native-only clean-room supplement from0xd3170, scene0xe2e00,
registration0x29d0d0 and the reached packet3 receiver. No implementation,
SDK/reconstruction, owner data/captures or game execution were inspected.
Addresses are RVAs. This fixes the abbreviated radius reading in
creature-pose §12.4 and supplies ordering/current-state facts without
replacing the already specified pose, camera or sphere-test mathematics.
The arithmetic below concerns ordinary finite representable inputs.

## 1. Traversal, registration and packet updates

Normal scene0xe2e00 calls0xd3170 once at0xe3bd1. Inside that invocation,
the creature pass follows the **World+0x40 registration list**, starting
at sentinel.next and following next links until the same sentinel. A node
contains next+0, previous+8 and Creature pointer+0x10. After the current
entry's work the pass reads that node's next link. It does not snapshot a
count, sort the creatures, group them by kind or move the local player
ahead of the list. The local player is visited at its existing position.

This list has no numeric comparison key or key signedness. The separate
**World+0x450 ID tree compares signed64 IDs**, as visible in lookup0xe720;
that tree is not the source of the pose traversal. Neither ascending ID
order nor pointer-address order describes the forward registration walk.
See [World creature visitation](steam-world-creature-visitation.md).

Common registration0x29d0d0 appends a new node **before the sentinel,
after the previous tail**, then updates the ID-tree entry. It does not
prepend or reposition existing registrations by ID. The local-player
registration at0x7f64e uses this same append operation. This does not
establish that its World was empty at that call, or resolve every factory's
creation order; the earlier startup-order limit remains.

Packet3 receiver0x3e450 performs signed64-ID lookup0xe720 at0x3e8de for
the nonlocal record. Only a missing/null result takes the new0x15f8-byte
Creature construction0x4ce80 at0x3e909 and registration at0x3e924.
An already resolved record is updated in place, without unlinking and
re-registering its node. Existing packet3 updates therefore retain its
registration position; new records append in their actual receive-processing
order. This is the client's own list, not an automatic copy of the host's
registration order. Packet6 removal, reconnect/reset, explicit removal and
later recreation are separate lifecycle operations.

For example, given registered A,B, registering C then applying another
ordinary packet3 update to A leaves A,B,C. Drawing a culled subset preserves
that subset's relative list order. This example does not prescribe ID values
or a new spawn/factory order.

## 2. Current scene state precedes the normal actor pass

Earlier Game targeting/death consumers can use retained renderer state.
The later normal actor pass has a different boundary: scene entry0xe2e00
has already installed its current supplied CPU state before0xd3170 runs.

| Phase | Native state/order |
| --- | --- |
| Scene input copy | **Final eye** XYZ from argument2, Game+0x1dc0, is copied to Renderer+0x338/+0x340/+0x348. Independently, origin offsets+0x428/+0x430 negate **look-at X/Y** from argument3, Game+0x1dd8. The three-float angle tuple at+0x350..+0x358 and view+0x39c/projection+0x3dc receive the current inputs. |
| CPU planes |0xdf8f0 at0xe308a extracts the six planes from those copied view/projection matrices, storing them at+0x43c. |
| Adaptive/derived ranges |0xecda0 at0xe3096 updates the current scene range fields, including actor range+0x4a0. This follows plane extraction and precedes the actor traversal. |
| World actor collection |0xd3170 at0xe3bd1 uses these current fields; its pose-gating sphere call is0xe08c0 at0xd5e1d. |
| Later shader uploads |After0xd3170 and the point-light collector, the scene installs the ordinary shader state and uploads view/projection through virtual slots+0x58/+0x60. Their later position does not make the earlier CPU sphere test use retained matrices. |

In the normal Game caller, view+0x39c is supplied from **Game+0x1f24**
and projection+0x3dc from **Game+0x1f64**. The view includes the already
specified camera shake; it is the current scene input rather than a new
unshaken substitute. The fixed final eye, separate look-at-based origin
offsets, planes and+0x4a0 belong to the same current scene call. The sphere's
fixed distance anchor is the **eye**; its plane-coordinate rebasing uses
the **negative look-at** origin, matching that origin-relative view.
The exact argument loads/destinations and distinct cache lifetimes are in
[pose/body coordinate lifetime](steam-body-particle-coordinate-lifetime.md).
The directly reached model/terrain fixed-to-float and GPU upload boundaries
continue in [ordinary WORLD GPU coordinates](steam-world-gpu-coordinates.md).
This corrects the earlier table wording that negated the eye tuple for
the render origin. Existing construction/precision
remains in [gameplay camera](steam-gameplay-camera.md) and
[scene visibility §§1.2–1.3](steam-scene-lighting.md#13-native-cpu-clip-matrix-and-six-plane-equations).

“Retained” here describes the renderer fields before this scene-entry
replacement, normally left by its prior scene call. It is not a second
camera selected by0xd3170, and shader constant-buffer state is not the
source read by0xe08c0. The earlier bubble-projection cache schedule remains
separate as specified in [NPC speech §10](steam-npc-speech-rendering.md).

## 3. Exact normal pose admission and sphere inputs

The admitted call's frame/light/buff material and separate pixel-opacity inputs
are [ordinary WORLD pose material preparation](steam-world-pose-material-preparation.md).

For each registered entry, the normal pose route requires, in order:

1. Draw bit0x01 does not exclude this entry as the local-player pointer;
   draw bit0x80, when set, permits only that local-player pointer.
2. Creature HP float+0x180 is greater than0.
3. The current-scene sphere test0xe08c0 at0xd5e1d succeeds.

The local comparison is pointer equality against World+0x448, not ID
ordering. Bits0x01 and0x80 together admit no creature on this route.
There is no additional containing-zone active-mesh test in this actor
admission. Other sphere calls within0xd3170 have their own consumers;
this section specifies the call that gates0x17ab0.

Let the drawn fixed tuple be Creature+0xb10/+0xb18/+0xb20. For ordinary
races, its sphere center uses that X/Y and adds to Z the signed64 truncation
of **float32(−stepOffset×65536.0f)**, where stepOffset is float+0x990.
The native sequence negates the float by its sign bit, multiplies in
float32, truncates toward zero and adds in low64 arithmetic. It does not
use the raw physical tuple+0x10 as this center.

The radius is determined before the pose call:

* Compute the finite maximum of width+0x84 and depth+0x88, then multiply
  by **0.6f**, word0x3f19999a, rounded to float32.
* Take the maximum of that result and **body height+0x8c**. The native
  height lower-bound selection is0xd5cef–0xd5d03. This is the operation
  omitted by the older abbreviated creature-pose §12.4 reading.

For race **0x12e**, use drawn X/Y and instead add to drawn Z the truncation
of **float32(float32(field+0x17c×0.5f)×65536.0f)**. Its ordinary step-offset
subtraction is not also applied. Add **float32(field+0x17c×1.2f)** to the
height-bounded radius above, with a separate float32 addition. Constants
0.5f and1.2f are words0x3f000000 and0x3f99999a. These operands are read
from the values already stored when the gate runs; the current pose has
not yet recomputed any cache used by this test.

The distance argument is **Renderer+0x4a0**, with no+64 addition. The
shared fixed-distance and all-six-plane test is unchanged; its math is
in the existing scene-visibility contract. This radius is not the
death-effect consumer's half-maximum-extent sphere.

## 4. Flags supplied by the ordinary Game scene call

The normal0xa5a50 caller builds the flags forwarded unchanged to0xd3170:

| Added bit | Reached input condition |
| --- | --- |
| 0x08 | Global byte0x5514c6 nonzero |
| 0x80 | Global byte0x5514d6 nonzero |
| 0x20 | Interface root at (Game+0x20)+0x140 not shown; also added when any frontend root Game+0x118/+0x130/+0x170 is shown |
| 0x10 | Interface0x9aeb0 key0x11 check is true |
| 0x02 | Game+0x668 map-mode byte nonzero |
| 0x40 | Game+0x118 title root shown |

With both globals zero, the HUD root shown, the named frontend roots
hidden, that key check false and map mode off, the built mask is0.
This builder does not add bit0x01. The latter remains a supported
0xd3170 input from another caller, not an assumed ordinary gameplay mask.
The HUD/logo/map/control bits do not add a new normal-pose admission
condition beyond §3. Bit0x08 can add debug geometry before the main
actor sphere gate; it does not supply another0x17ab0 callsite.

## 5. Pose/cache and shared CRT consequences

After the sphere succeeds, the renderer copies ability clock+0x6c into
animation clock+0x08, selects animation state+0x0c from the current ability
and applies the existing nonzero lock-timer override. See creature-pose
§12 for that state mapping. It then invokes0x17ab0 at **0xd67b2** once
for this entry. Color/fade preparation does not introduce a separate
zero-material-alpha rejection before that call.

The pose's state0x61 empty-spark-list initialization therefore consumes
its twelve shared main-thread CRT draws in the surviving registration-list
order. A failed normal gate does not call the pose or initialize/update
its spark list through this route. Model-descriptor collection is the
output of these already executed pose calls; later resource batching
does not reorder their earlier random draws. See
[main CRT §3](steam-main-crt-stream.md#3-main-frame-sharing-and-relevant-phase-order)
and [pose sparks §7](creature-pose-items.md#7-the-sparks-of-state-0x61).

The pose/body cache marker+0x1500 is set after the normal call at0xd67b7.
It is not a per-frame deduplication or latest-frame visibility marker:
its already specified sticky lifetime remains
[transient-light producers §3.1](steam-transient-light-producers.md#31-meaning-and-production-of-creature-byte0x1500).
The later emote/overhead branches in this actor visit do not invoke the
full pose a second time. Multiple queued body parts or overhead models
are not multiple invocations of the state0x61 initialization.

## 6. Other direct calls and explicit scope

The single normal-world callsite is not a global one-pose-per-creature-per-
frame guarantee. Scene0xe2e00 has a separate optional creature-preview
call at0xe4bfb, later than the normal world collection; it temporarily
zeros that creature's drawn XYZ for the call and restores them afterwards.
Widget preview0xdb630 also calls0x17ab0 at0xdb7c0. Those paths retain the
cached animation-state/clock behavior described in creature-pose §12.1;
the world pass's HP/frustum admission must not be presumed to guard them.

This supplement does not establish all outer admission/membership producers
of those preview paths or guarantee that their supplied creature can never
also occur in the world list. It does not give every factory's initial
registration order, every reconnect/ID-collision recovery, or arbitrary
list mutation during rendering. It establishes one ordinary scene's direct
world-list traversal and current inputs, rather than adding a global
per-frame deduplication rule.
