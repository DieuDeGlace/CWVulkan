# Steam gameplay camera

Independent behavioral specification from native code/assembly and constants,
with prior independent specifications as cross-references. No engine, tests,
exporter, build files or owner captures were inspected and no game was run.
Addresses are RVAs. This is the gameplay camera, not only title flyover.

## 1. State and units

G is Game; P is its world+0x448 local creature. Positions below are signed64
fixed coordinates, **65536 units/block**. These are distinct storage:

| Game offset | Type | Meaning |
|---|---|---|
| +0x1d80,+0x1d88,+0x1d90 | double each | Displayed pitch, roll, yaw in degrees |
| +0x1d98,+0x1da0,+0x1da8 | double each | Target pitch, roll, yaw in degrees |
| +0x1db4 | float32 | Current orbit distance, blocks; also shortened by camera obstruction |
| +0x1db8 | float32 | Requested orbit distance, blocks; obstruction does not replace this target |
| +0x1dc0,+0x1dc8,+0x1dd0 | signed64 each | Final camera eye XYZ |
| +0x1dd8,+0x1de0,+0x1de8 | signed64 each | Smoothed camera look-at/aim-origin XYZ |
| +0x1df0,+0x1df8 | signed64 each | Negative look-at X/Y; render-origin translations |
| +0x1e00 | float32 | Camera shake magnitude, eased toward0 |
| +0x1ea4 |16 float32 | Camera/world transform, unshaken |
| +0x1ee4 |16 float32 | Full world-to-view transform, unshaken |
| +0x1f24 |16 float32 | Render-origin-relative view transform with shake |
| +0x1f64 |16 float32 | Projection matrix |

The basic orbit convention has **pitch0 looking straight up, pitch90
horizontal, pitch180 straight down**; yaw0 looks toward−Y, yaw90 toward+X.
Roll is a real separate rotation, even though the traced ordinary producers
normally leave its target0. Do not derive displayed angles by copying target
each input poll or add assistance directly to a displayed angle.

## 2. Exact angle, distance and shake smoothing

Camera update **0xadcd0** receives the frame's **capped double wall interval
in milliseconds**, cap1000, passed at **0xaaf02** without a float conversion.
It is not simulation step/remainder time or an XInput-successful-poll interval.
The producer/cap/focus behavior is steam-scene-lighting §5.

Use native coefficient **0xa7a0(t,k)**: count=trunc32(t times100 double),
initial accumulator0, and double multiplier **double(float k) times0.02**.
For each sequential integer i from0 through count−1, advance the accumulator
toward **(double(i)+1)/double(count)** by that multiplier, preserving double
operation order, then return it narrowed to float32. For count≤0 return0.
This is the existing creature-upkeep §4.9 coefficient, not an exponential.

In this order,0xadcd0:

1. Converts signed32 **cameraSmoothness G+0x1bb0** to float and multiplies
   by float0.01. Computes float **powf(0.005,that exponent)** and calls0xa7a0
   with that float k and the retained double interval. Widens the returned
   float coefficient to double. Each displayed angle becomes its old double
   plus **(target double−old double) times that widened coefficient**.
   There is no wrap/shortest-angle reduction in this smoothing step.
2. Eases current float distance toward requested float distance with
   **0xa7a0(t,float0.004)**, independent of cameraSmoothness. Subtraction,
   coefficient multiplication and addition are separate float operations.
3. Eases shake float+0x1e00 toward0 with **0xa7a0(t,float0.005)**, using
   that same float subtraction/multiply/add pattern.

**Correction to frontend-title §3.3:** its sentence saying current distance
uses the angle option's coefficient is inaccurate. The native independent
distance rate is **0.004 at0x437f70**, called at **0xade0c**. Angle option
conversion/powf/call is **0xadd54–0xaddae**. Camera speed and invertY do not
alter this smoothing coefficient; those are mouse-input properties.

## 3. Manual input, assistance, zoom and reset producers

### 3.1 Mouse displacement

**0x97880** receives mouse movement. With ordinary UI predicate0x1014f0
false, its two float arguments are the displacement. With the predicate
true, first update interface pointer processing **0x101160** using the
arguments truncated to signed32. If last-device byte **G+0x1958** is then
nonzero, return from this handler before the mouse-specific camera paths.

For ordinary UI mouse processing, current/previous GUI pointer floats are
engine **+0x180/+0x184** and **+0x188/+0x18c**. Nonzero displacement sets
map-drag byte **G+0x1db0=1**. Full map+0x668 with physical right button
(engine+0x1a8 bit1) pans the map. Visible weapon-customization+0x368 with
that physical right button rotates its preview angles instead. Those
routes retain the separate established UI/map contracts.

The actual camera-look block is eligible when **ordinary UI is false**,
or when **physical left button bit0 is held and0x33f1e0 returns null**.
That query returns the GUI capture/point-hit result: a captured widget
prevents the null result; otherwise it searches the GUI tree at the current
pointer. This is not a test of the rebound Normal-Attack action. In the UI
case this block uses the current-minus-previous GUI pointer displacement,
not the callback's absolute coordinates. Thus an answer panel alone does
not justify a blanket frozen gameplay camera, but hitting/capturing GUI
under that physical-left route matters.

Convert **cameraSpeed signed32 G+0x1bac** to float, multiply by stored
float0.005, then by each displacement float. Widen each result to double.
Yaw target **−=horizontal result**. With invertY **G+0x1bb4=0**, pitch
target **+=vertical result**; otherwise subtract it. Clamp pitch target
to **0..180 double** in this mouse block. No elapsed time multiplies a
mouse displacement, and it does not update displayed angles or roll.
Default cameraSpeed50 means0.25 degrees per input displacement unit.

### 3.2 Frame right stick and movement assistance

The successful-poll XInput router provides filtered right stick at
**G+0x1d74/+0x1d78**, and left movement at+0x1d6c/+0x1d70 under the
existing remaining-runtime-gaps §4 routing rules. In0xa5a50 the gameplay
right-stick camera update is a **frame pass**, using the retained values
and capped double frame ms. It is separate from UI successful-poll timers.

Start float rate **0.4 degrees/ms**. With per-frame projected-creature-box
byte+0x1e88 nonzero and final creature target+0x1e58 resolving withHP>0,
multiply rate by **max(1−d/50,float0.3)**, where d is the float Euclidean
distance in blocks from the **previous/currently stored eye+0x1dc0** to
the target's drawn+0xb10 XYZ. Each fixed difference converts to float and
multiplies1/65536 before squared sums/sqrt. This rate does not use
cameraSpeed or invertY.

When weapon customization+0x368 is hidden, subtract **double(powf(rightX,3)
times rate) times doubleMS** from yaw target, and the corresponding rightY
term from pitch target. The cube and its multiplication by rate are float
before widening. This block applies **no pitch clamp**. When customization
is visible, the same horizontal/vertical quantities instead add into its
float model angles+0x1f0/+0x1e8; camera targets are unchanged by this pass.

Before this manual pass, the local input/targeting **0x8ae50** may change
yaw target through movement assistance. Its full verified vector, lead,
threshold and interpolation contract is **steam-controller-handoff §3.1**.
It requires filtered left-stick squared length>float0.1 and a living
creature target; no right-stick deflection is required. It computes desired
yaw from displayed yaw, but blends the **separate target** through0x17840.
Static object/item targeting and general-widget speaker override can later
change the final target ID before the distinct manual slowdown above.

Per-frame order is ordinary target rebuild/assistance, subsequent target
replacement, manual right-stick target changes, queued sound effects,
and **0xadcd0** smoothing/position/matrix construction. Mouse callback
changes are independent displacement events, not replayed as additional
per-frame deltas. The title/character-selection riding override earlier
in0xa5a50 adds doubleMS times0.003 to yaw target and sets pitch110 and
distance12; it does not replace the gameplay update implementation.

### 3.3 Orbit zoom is Walk plus wheel; mapped Zoom is map zoom

Wheel handler **0x97cc0** first routes a hit visible inventory, shop,
crafting, G+0x658, or G+0x388 panel to that panel's row scrolling, in that
order. Only its fallback reaches distance/map zoom. With mapped **Walk
action13 held at G+0x1a7d**, subtract **float32(2 times signed wheel count)**
from requested distance+0x1db8, then clamp float to **0..50 blocks**.
The twice-count integer operation precedes float conversion. It does not
directly set current distance. Walk normally binds LeftCtrl but the mapped
action, rather than hardcoded Ctrl, is what this test reads.

Without Walk, wheel changes the separate map zoom index+0x1e04; its usual
0..6 bounds and cursor-centered map recentering remain the map contract.
Mapped Zoom In/Out actions21/22, including gameplay D-pad up/down, likewise
change **map zoom+0x1e04**, not orbit distance. Their0xa5a50 down-edge bytes
**0x5514f4/0x5514f5** update each frame, with lower/upper bounds using the
map's zoom-layer count. They are not held-repeat orbit-zoom controls.

### 3.4 Initial and explicit reset behavior

| Native event/source | Displayed and target angles | Current/requested distance |
|---|---|---|
| Early Game construction0x7ea80 | All three initially0 | Both8 |
| Completed startup stage0x7f767–0x7f7a2 | Both pitch90,roll0,yaw180 | Both8 |
| Title Continue release0x965e0 | **Keep existing angles/targets** | Both8 |
| Select existing character0x9f7c0 | Both pitch90,roll0,yaw180 | Both8 |
| Select New Character0x9f7c0 | Both pitch90,roll0,yaw0 | Both8 |
| World creation0x9f060 | Both pitch90,roll0,yaw180 | Both8 |

These resets do not directly copy a saved yaw into camera yaw. Character
record restoration and local facing reset remain frontend-title §6.1.
They also do not separately copy a new eye/look-at anchor in the traced
angle/distance assignments. The next camera update handles the position
and its50-block safety snap. No ordinary mouse/manual-stick/assistance
path writes the roll target. Do not add pitch clamping to all producers
or wrap yaw merely because the mouse block has its own pitch bounds.

## 4. Shake producer and frame timing

The ordinary producer is the frame's queued **World+0xa0 sound list**,
processed by0xa5a50 before0xadcd0. For sound IDs **0x54/0x55 only**, take
the notice's fixed XYZ minus the player's **physical XYZ**, convert each
difference to float blocks and form float squared distance d². Candidate
magnitude is **max(0,1−d²/2500)**. Raise G+0x1e00 only when candidate is
strictly greater than its existing float magnitude. This is based on
squared distance, not d/50, and does not sum multiple events.

The sound notice payload is node+0x10 XYZ and node+0x28 sound ID in this
list. Sound playback is attempted first; the magnitude decision does not
depend on playback success, configured sound volume or audibility return.
The camera then performs its §2 decay in the **same frame** before using
the magnitude in its render-view matrix. Generic sounds or arbitrary
particle families do not create camera shake through this producer.
Native source **0xaa594–0xaa5f3**, distance-square helper0x2b650.

## 5. Physical look-at candidate, posture and anchor smoothing

### 5.1 Candidate position

**0xadcd0** begins from P's **physical signed64 XYZ+0x10/+0x18/+0x20**,
not drawn+0xb10, interpolation/extrapolation of that drawn record, or an
actor model's occupied AABB. Subtract from Z the **truncated signed64
float product P.stepOffset+0x990 times65536**. StepOffset is the existing
physics/riding step-smoothing offset in blocks, not camera velocity.
Then add one separately truncated float product of the applicable
height in blocks times65536:

| Condition, in priority order | Height added |
|---|---|
| Ability0x57 or0x58 | float32(P.bodySizeX+0x84 times0.5) |
| Ability0x78, or character-creation layerG+0x170 hidden | float32(P.bodySizeZ+0x8c times0.5 plus0.5) |
| Otherwise, creation layer shown |0 |

For ability **0x76 riding**, resolve mount ID **P+0x9d8** in the world's
creature map. Missing/null/dead mount **sets P's ability0**, without
restarting this candidate construction. A living mount additionally raises
the candidate by **trunc64(float32(mount.bodySizeZ times0.75 plus mount
offsetZ) times65536)**. OffsetZ is the mount-race **0x65040** table,
creature-upkeep §4.8, using the mount's race+0x64. For ability **0x79**,
add another exactly **2 fixed blocks(0x20000)** to candidate Z.

### 5.2 Candidate block probes

The native initial loop calls effective world-block accessor **0xe9a0**
at the candidate's floored block coordinates, narrowing them to signed32.
Its first stop test is **type byte AND0xfd nonzero**, or **flags byte
bit0x40 set and bit0x80 clear**. If neither applies, it queries the same
coordinate again; a second type AND0xfd nonzero advances candidate Z by
one fixed block and repeats, otherwise stops. Native assembly confirms
the same coordinates for these two queries. For an unchanged world read,
this does not raise a candidate out of an ordinary solid block; do not
replace it with an invented “while solid, raise head” routine.

Next it tests a cube of8 candidate offsets **±0.25 block per axis**,
X outer/Y middle/Z inner, using effective0xe9a0 and its **type AND0xfd**
test only. A failing corner resets candidate XYZ to P's **raw physical
position**, dropping the step/posture offsets. It breaks the current
inner Z loop but continues the remaining nested tests with that updated
candidate. This test treats water2/air0 as clear; it is not a collision
shape sweep against object/CUB sprite geometry.

### 5.3 Ability-specific retained look-at update

Apply this after candidate probes; these use P's then-current ability:

| Ability | Stored look-at update |
|---|---|
|0x9c | Copy raw physical XYZ directly |
|0x9d | Copy raw physical X/Y, raw physical Z plus1 fixed block directly |
|0x98 | Smooth all3 candidate components with0xa7a0(doubleMS,float0.25), using the fixed-weight operation below |
|0x79 | Smooth all3 candidate components with the double-return helper0x8a1b0(doubleMS,float0.05), below |
| Other | Velocity-dependent X/Y coefficient, separate step-dependent Z coefficient, below |

**Fixed-weight operation:** for0x98, multiply the returned float factor
by float65536 and truncate to signed64 weight. For0x79, multiply the
returned **double** factor by double65536 and truncate to signed64.
For each component, multiply its signed64 candidate-minus-old delta by
that weight, then add the product **divided by65536 with truncation toward
zero** to old. This is signed integer multiply/divide, preserving native
low64-bit multiplication; it is not float interpolation of a world position.

Helper **0x8a1b0** is not void despite the abbreviated decompiler signature.
Assembly returns a **double** accumulator. Its count is trunc32(doubleMS
times100); its sequential targets and accumulator recurrence are those
of §2, but its per-step multiplier is **float32(k/100)** widened to double,
and its final accumulator **does not narrow to float**. For boat input
k=float0.05 that float division precedes the recurrence. Preserve this
distinction from0xa7a0's double(k) times0.02 and float return.

In the ordinary branch, form float **speed²=(vx²+vy²)+vz²** using P's
velocity+0x34/+0x38/+0x3c. Compute float **w=1−expf(−speed²)**; widen w
to double, multiply by double0.5 and add double
**0.02500000037252903**, then narrow to float k. X/Y use0xa7a0(t,k)
and the fixed-weight operation above. Thus stationary k=float0.025,
approaching float0.525 at high speed; no cameraSmoothness option enters.

For ordinary Z only, compute float **k=abs(P.stepOffset) times0.005
plus0.0035**, call0xa7a0 with retained doubleMS, convert the signed64
candidate-minus-old Z difference to float, multiply it by the returned
float factor, **truncate the product to signed64 and add to old Z**.
This Z branch does not use the quantized fixed weight that X/Y use.

After every ability branch, compute float squared distance from retained
look-at to P's raw physical position, using fixed differences converted
to float then multiplied1/65536. **Strictly greater than2500** snaps all
look-at components to raw physical XYZ. This50-block safety rule does
not preserve a head-height offset or wait for graphical zones to load.

## 6. Orbit eye and terrain obstruction

### 6.1 Forward vector and initial eye

Narrow each displayed double angle to float before multiplying by stored
float radians/degree **0.01745329238474369** and taking cosf/sinf. With
p/r/y denoting those pitch/roll/yaw radians, the camera forward is:

- X: **cos(y)sin(r)cos(p)+sin(y)sin(p)**.
- Y: **sin(y)sin(r)cos(p)−cos(y)sin(p)**.
- Z: **cos(p)cos(r)**.

These are the independent simplified component relationships; native
executes separate float multiplies/adds from its rotated basis, without
a final normalization. Initial eye copies retained look-at, then subtracts
**trunc64(float32(float32(forward component times current distance)
times65536))** independently on each axis. Current distance has already
received its §2 ease toward target for this frame.

### 6.2 Guard rays, shortened distance and final solid fallback

Ability **0x9c skips** the following eye obstruction and final eye-block
tests. Other abilities first check8 **eye±0.25** corners in X/Y/Z nested
order, each to the player's **raw physical XYZ**, using visibility helper
**0x2b5c00**. Evaluate all8 even after one fails. Its distance limit is200;
it rejects beyond that limit through0xf3e20, otherwise normalizes the
float direction and tests whether its effective-terrain ray reaches at
least **float(distance−0.01)**. For squared separation below float0.0001,
it succeeds without a ray. Distance200 precheck uses double block deltas;
normalization/ray distance arithmetic is float.

If all8 succeed, retain the eye without a separate anchor-to-eye sweep.
If any fails, cast8 rays from **look-at±0.25** in the same nested order,
direction **negative forward**, through **0x2a0d40**. Each ray receives
the **then-current** float distance; any nonnegative return replaces it
immediately before the next ray. Therefore later rays use the already
shortened maximum. Forward is not renormalized here. After all8, rebuild
eye from the retained look-at and this final current distance. This
rebuild multiplies **distance times component**, then65536, before each
signed64 truncation. Requested distance+0x1db8 remains unchanged.

Finally query **floor(final eye XYZ)** via0xe9a0. If type AND0xfd is
nonzero, set **both eye and retained look-at to raw physical XYZ** and
current distance to0. No actor/CUB/object collision trace supplements
these terrain tests. Water2 is clear in their material predicate.

The camera supplies a legacy water-mode boolean determined from physical
Z and physics flags:1 when **P.fixedZ>0x20000 or(P.physicsFlags+0x5c
AND0x12)=2**, otherwise0. It also supplies trailing flags to these helpers. The actual
0x2a0d40 terrain traversal does not read those trailing options: its
obstructing material decision is the effective **type AND0xfd** predicate.
Do not make water solid in this camera traversal from the argument's name.

### 6.3 Exact terrain ray stepping used here

0x2a0d40 starts traveled float distance0 and block indices from
**floor(fixed/65536) narrowed to signed32**, helper0xfd80. A missing
zone or initial nonwater/nonair block stops at distance0. It reads
effective blocks with0xeb90. While current block is clear, it permits
at most **501 boundary steps** and calculates the next boundary distance:

For each axis, current fixed ray point is original fixed component plus
**trunc64(float32(float32(direction times traveled) times65536))**.
Axes with float direction² below **0.000001** are ignored. For an
eligible positive direction, boundary is (signed block index+1) times65536;
for a nonpositive direction it is block index times65536. Convert the
signed64 boundary-minus-current-point difference to float, divide by
the direction float, **truncate that quotient to signed64**, convert
back to float and multiply by1/65536. This is the axis candidate step.

Begin with step10 and axisX. Replace only on a **strictly smaller**
candidate, visiting X thenY thenZ, so axis ties retain the earlier axis.
Increment/decrement that chosen block coordinate according to direction's
sign, add the chosen float step to traveled, and continue from that block.
Cross-zone lookup occurs as those signed32 block coordinates change.
Stop at an obstructing block, missing zone,501 steps, or traveled reaching
maximum. Return **the smaller of traveled and maximum**, with no safety
skin subtraction. Boundary hits can produce a zero step before the index
moves; they are not an excuse to add an epsilon to eye distance.

## 7. Matrices, precision, shake and projection

### 7.1 Unshaken camera/world matrix and its inverse

Matrix convention is **column vectors, column-major storage**, as in the
existing matrix specifications. The matrix at **G+0x1ea4** is constructed
from identity with translation to the eye, then rotations **Z(yaw),
Y(roll), X(pitch)** appended on the right. Thus it is the camera-to-world
transform **T(eye) Rz(yaw) Ry(roll) Rx(pitch)**. Eye translation converts
each full signed64 fixed component to float before multiplying1/65536.
Its third basis column **+0x1ec4/+0x1ec8/+0x1ecc** is forward, used by
ordinary targeting. The first column points left in screen terms because
native projection later has negative X aspect. At pitch90/roll0/yaw0,
its basis is left(+X), up(+Z), forward(−Y).

The matrix **G+0x1ee4** is rebuilt from identity by appending rotations
**X(−pitch),Y(−roll),Z(−yaw)** and then translation by negative eye.
Negation occurs on the double angle before narrowing to float, and on
the signed64 fixed eye before float conversion. This is its unshaken
world-to-view inverse. All trig/matrix arithmetic is float; displayed
angle storage and target interpolation remain double. Do not replace
the unshaken basis used for targeting with a view-shake direction.

### 7.2 Render-origin-relative shaken view

Set signed64 **G+0x1df0=−look-at X**, **+0x1df8=−look-at Y**. There is
no corresponding Z origin shift. The renderer receives fixed world X/Y
with these matching offsets applied before conversion. Matrix at
**G+0x1f24** is for those shifted coordinates, not full world positions.

Start identity. From float **World+0x440 clock ms**, compute float
**sx=cosf(clock times0.03) times(shake times0.5)** and
**sy=cosf(clock times0.04) times(shake times0.5)**. Install this(sx,sy,0)
translation in view space. Then append inverse angle rotations in the
same X/Y/Z order as §7.1. Finally append translation whose components
are **(look-at X−eye X), (look-at Y−eye Y), −eye Z**, with each signed64
difference formed **before float conversion and1/65536 multiplication**.
This preserves local precision that the full-world float inverse lacks.

Thus the shaken render-view is **T(sx,sy,0) Rx(−pitch) Ry(−roll)
Rz(−yaw) T(relative negative eye)**. Shake does not change stored fixed
eye/look-at, angle state, or the unshaken forward/basis. Do not apply the
look-at origin shift again through a full-world eye translation.

### 7.3 FOV and native clip convention

Global float **0x533930** starts at **0.8726646900177002 radians(50°)**.
At the end of0xadcd0, if P's **u16 flags+0x128 bit0x400** is clear,
ease it toward that50° target with0xa7a0(doubleMS,float0.002). If set,
ease toward **0.5235987901687622 radians(30°)** with rate float0.01.
Update is old float plus **coefficient times(target−old)**, all float.
The matrix uses the newly eased FOV, not its target. This pass does not
change requested orbit distance from the FOV flag.

Projection **0xa0340**, stored at **G+0x1f64**, takes near **0.05**,
far **1920**, and **aspect=−float(viewport width)/float(viewport height)**,
from signed32 G+0x1d60/+0x1d64. With h=float(1/tanf(float(FOV times0.5))),
nonzero row,column entries are **P00=h/aspect, P11=h, P22=f/(f−n),
P23=−(n times f)/(f−n), P32=1**. All other entries are0, includingP33.
The denominator/multiplications/divisions are float operations. View
forward Z is **positive**, clip W=view Z, and GPU depth is **0..1**.
There is no native projection Y flip. The CPU six-plane conventions and
its half-near mismatch remain steam-scene-lighting §1.3; do not feed a
Vulkan-flipped matrix to that native-equivalent CPU test unchanged.

## 8. Stable scope and remaining upstream boundary

The per-frame camera update, normal input/reset producers, physical anchor,
terrain collision, matrix/shake/FOV arithmetic above are stable. The
**upstream producer of local creature flags+0x128 bit0x400** has not been
established in this pass. The native camera consumes that bit exactly as
specified; this is not evidence for inventing a new scope key, attack
gesture or camera-distance trigger. Owner-data camera pose/offset rules
not listed here are not substituted for this camera's explicit fields.
0xadcd0 assumes a valid local-player record supplied by its caller. It
does not impose a playerHP, menu-visible or movement gate on its ordinary
per-frame smoothing. The special creation/riding conditions above are
explicit exceptions inside that continuing camera update.

### 8.1 Bounded producer audit and verified imports/clear

A follow-up native-text audit checked narrow writes to both **creature+0x128**
and its high byte+0x129, plus the equivalent **main-record+0x118** writes.
The generic mask setter **0xa38a0** really accepts a16-bit mask and a
set/clear byte on a main-record pointer. Its sole verified native caller
**0xa9e5d** passes **mask4**, not0x400. It is not evidence for a dormant
scope input automatically calling that helper. No verified local input,
ability, item-use or worldgen producer setting bit0x400 was found. This
is a bounded static result, not a claim that arbitrary imported flags or
modified builds can never contain it.

Concrete whole-word sources exist. Creature main-record copy **0x3b7a0**
copies its16-bit+0x118 field unchanged; **0x138e00 at0x139172** reads
the whole u16 for network delta field14 when its presence bit is set,
without a per-bit rejection of0x400. These transport/copy paths preserve
an already supplied bit; they do not specify a new user action to create
it. Main-record constructor **0x4d600 at0x4d651** initializes the word0.
The ordinary player-save loading path resets through the existing fresh
record constructor and does not have a dedicated saved FOV-bit field.

There is a verified automatic **clear** in the ordinary upkeep pass
**0x2de4c0 at0x2dfe88–0x2dfe93**. Compute the float squared norm of
**acceleration+0x40/+0x44/+0x48**, in XYZ order. If it is **strictly>0**,
clear bit0x400 before the subsequent ability interruption/stealth logic.
It is not a velocity-magnitude, physical displacement, mouse movement or
camera-angle test. The same pass reads the retained bit for its existing
stealth accumulator, as described in creature-upkeep §4. This behavioral
meaning does not establish the missing local set trigger.

Until a concrete setter is independently found or a labeled original
runtime input demonstrates one, implement the imported/retained bit's
camera and upkeep consumers without fabricating a scope/sneak key binding.
