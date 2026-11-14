# Steam type-77 particle and scene zone writer

## Authority and scope

This addendum covers successful type-77 ambient particle insertion, the reached
update and draw paths, and the ordinary scene write to zone offset +0x234.
The installed Steam CubeWorld.exe and the archived cubeworld.original.exe both
have 5,939,072 bytes and SHA-256
A1EFBBD41EEB8DF08395C4D6511194D416CB70A14054B2A2000B9F5CDBCFA65B.
The archived unpacked Steam image has 5,757,952 bytes and SHA-256
7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244.
The unpacked image and its original instruction, function, and string exports
supply the RVAs and data below. The image base is 0x140000000. Offsets are
in bytes from the named object or record. The numeric behavior describes the ordinary finite, in-range path.
The asset identity and render state come from the original executable.

## Admission and complete initialized particle payload

The ambient pass at RVA 0xAF110 looks up an existing zone in the current World.
Its comparison at 0xAF6A8 admits the zone only when its binary32 +0x234 value
is strictly below 4,096. It walks the admitted zone's emitter records in stored
order. At 0xAF6E3–0xAF6EA, the signed 32-bit type at emitter +0x00 selects
the type-77 path.

That path truncates the binary32 current-World time at World +0x440 to a
signed 32-bit millisecond value. It compares the signed integer quotient by
1,000 before and after adding the signed 32-bit frame interval. Both divisions
truncate toward zero. Unequal quotients permit one random draw at 0xAF739.
A draw divisible by 16 admits one particle. Even an interval crossing several
boundaries permits at most one such attempt per emitter per visit. A successful
attempt consumes exactly five more draws, in order for linear Y, linear X,
angular Z, angular Y, angular X. Each becomes binary32, is divided by 32,767,
and has 0.5 subtracted. The resulting linear values are multiplied by 4.0 and
the angular values by 10.0, in binary32. Linear Z is assigned −2.0 directly.
The branch consumes no angle draw and makes no sound request.
Evidence is 0xAF6F0–0xAF8F6, with the five draws at 0xAF772, 0xAF789,
0xAF7C8, 0xAF7DF, and 0xAF7F6.

The initialized values below use offsets from the 184-byte particle payload.
The payload begins 16 bytes into the list node. World positions use signed
64-bit fixed point with 65,536 ticks per block.

| Payload offset | Width and initialized value |
| --- | --- |
| +0x00 | Signed 32-bit particle type 4. |
| +0x04 | Signed 32-bit age of 0 milliseconds. |
| +0x08, +0x10, +0x18 | Signed 64-bit X, Y, Z copied from emitter +0x08, +0x10, +0x18. |
| +0x38, +0x3C, +0x40 | Binary32 X, Y, Z angles, each positive zero. The draw path interprets them in degrees. |
| +0x44, +0x48, +0x4C | Binary32 red, green, blue copied from emitter +0x2C, +0x30, +0x34. |
| +0x50 | Binary32 alpha seed of 1.0. |
| +0x54, +0x58, +0x5C | Binary32 X, Y, Z linear velocities. The two random components occupy X and Y, and Z is −2.0 blocks per second. |
| +0x60, +0x64, +0x68 | Binary32 X, Y, Z angular rates in degrees per second. |
| +0x6C through +0xAB | Sixteen binary32 values forming the additional identity transform. The four entries at +0x6C, +0x80, +0x94, +0xA8 are 1.0. The other twelve are positive zero. |
| +0xAC | Binary32 scale of 0.5. The particle culler also uses this value as its plane margin. |
| +0xB0 | One byte with value 1, selecting axis-stepped block collision in the reached update. |
| +0xB1 | One byte with value 0, allowing the ordinary motion and rotation update when the age is nonnegative. |
| +0xB4 | Signed 32-bit lifetime of 20,000 milliseconds. |

The insertion copies the three eight-byte slots at payload +0x20, +0x28, and
+0x30. The type-77 path supplies no initialized value for those 24 bytes
and no reached type-4 update or draw site assigns them a role. The payload copy
omits bytes +0xB2 and +0xB3. Their contents are unspecified here. The assignment
sites at 0xAF755–0xAF8A0 and the field copy at 0x7F9E0 establish these
distinctions. The additional transform uses image constants at 0x42E630 and
0x42E640 and the scalar assignments in the same branch.

At 0xAF8A4–0xAF8F6, the branch inserts the particle immediately before the
current first entry of the Game list at +0x2090, or before its sentinel when
empty. The helper at 0x6E710 uses the 200-byte node allocation at 0x87200
and the copy at 0x7F9E0. The initial next link targets the old first entry
and the initial previous link targets that entry's old predecessor. The
insertion increments the 64-bit count at Game +0x2098 and links both
neighbors to the new node. Forward traversal encounters consecutive new
particles in reverse spawn order.

## Reached update

The main Game update at 0xA5A50 calls 0xB37F0 at 0xAAED9, calls the
ambient pass at 0xAAF0C, and submits the scene at 0xAD9D4. The newly inserted
particle therefore enters the later draw at age zero and with its initial
angles when it passes draw admission. The earlier particle update has already
run for that frame.

On a later update, 0xB37F0 first trims a list whose count exceeds 1,000 down
to 1,000, taking entries from its tail via 0xA2460. It walks forward and adds
the signed 32-bit frame interval to each retained particle's age. For type 4
with payload byte +0xB1 equal to zero and age at least zero, it converts the
interval to binary32 and multiplies it by 0.001 to get seconds. It adds each
angular rate times that elapsed time to its corresponding angle once. The
type-4 branch bypasses vertical acceleration. The angle update is at
0xB3948–0xB3988 and the selected motion branch starts at 0xB3AF1.

For payload byte +0xB0 equal to one, the update multiplies the three
velocities by the elapsed seconds and computes their binary32 displacement
length. The number of steps is the truncated length plus one, limited to ten.
Each displacement component is multiplied by the binary32 reciprocal of
that step count. The result is scaled by 65,536 and truncated toward zero to
a signed 64-bit position increment. Each step attempts X, then Y, then Z.
After each axis increment it uses the current candidate position, with all
coordinates rounded down to signed 32-bit whole blocks, to query the current
World at 0xE9A0. A returned block type of 0, 2, 7, 8, or 11 accepts the axis
increment on this path. Other byte values cause the update to undo that
increment and clear the corresponding linear velocity. The ordinary
missing-zone block fallback in 0xE9A0 has type 1, established by the data
initialization at 0x1113. A Z-axis collision
clears all three linear velocities and all three angular rates. The rest of
this frame's substeps use the original precomputed increments. A collision
does not itself remove this particle. See 0xB3AFE–0xB3CE8.

The age comparison at 0xB3D94–0xB3D9D schedules removal only when the age
strictly exceeds the lifetime. Age equal to 20,000 survives the comparison.
The scheduled removals are unlinked after this walk at 0xB3E11–0xB3E3D.
The reached type-4 draw uses the stored scale and has no fade or scale
term based on particle age. Trimming and lifetime removals both occur before
new ambient insertions in that frame.

## Draw model, transform, and material

The scene call at 0xAD9D4 passes the same Game particle list to 0xE2E00,
which calls its forward draw traversal 0xE6840 at 0xE3CA7 when scene
flag bit 0x80 is clear. Each type-4 candidate is culled by 0xE08C0 using
its position, its 0.5 scale as a plane margin, and the binary32 scene range
at Scene +0x49C. The range test uses all three camera-relative axes. Each
fixed-point difference is squared in signed 64-bit arithmetic and divided by
65,536, truncating toward zero before the sum. The comparison threshold
is the signed 64-bit truncation of the binary32 range squared and then scaled
by 65,536. Equality passes this step. All six scene planes then test the
particle position plus the 0.5 distance margin. The plane test uses horizontal
origin offsets at Scene +0x428 and +0x430 before conversion to block units
and uses absolute Z. The scene computes those offsets as the negatives of
the Game origin's fixed-point X and Y passed from Game +0x1DD8. A plane rejects a strictly negative result.

Type 4 uses zero-based model table entry 0xA46, or 2,630, as seen at
0xE6A5E–0xE6A77. The ordinary table is reached through World +0x70 and
has its entry array at table +0x10. Registration at 0x19C48B–0x19C4F0,
in 0x177F50, binds the original name cube.cub at image 0x455B50 to that
entry using the direct index helper 0x1734C0 and the voxel resource loader
0x16F320 with the two byte options 1 and 0. The same original table
registers leaf.cub at entry 0xD09 at 0x1D44CB–0x1D4530. The type-4 indexed
draw at 0x16EC60 requires both of the model's buffer wrappers and their
readiness checks. This describes the successfully loaded ordinary model.
The voxel dimensions and vertex colors of the asset data are outside this
source-only audit.

The draw translates to the position in block units with the horizontal scene
origin offsets, applies the stored Z, Y, and X angles in that order, scales
all three model axes by 0.5, and composes with the particle's stored identity
transform. The angle helper at 0x2B160 multiplies degrees by binary32
0.017453292 before its trigonometric calls. The renderer adds the rotated,
scaled negative half-dimensions of the model to the translation, placing
the model's dimensional center at the particle position. These operands and
their consumers are at 0xE6B3E–0xE6DC8.

At 0xE69AA–0xE6A27, red, green, and blue material multipliers are the
particle RGB values times the separate binary32 scene multipliers at +0x530,
+0x534, and +0x538. The material alpha is the particle alpha seed times Scene
+0x53C, multiplied by the greater of binary32 0.2 and the position lighting
byte divided by binary32 255.0. The lighting query at 0xDAB20 leads to
0xDA830 through signed whole-block coordinates rounded down, uses channel
index zero, and returns 255 on a missing zone. The scene sets its four
multipliers to 1.0 and sets red and green to 0.5 when the block type returned
at the camera by 0xEAB0 is 2, at 0xE3341–0xE33B4.

The pass selects renderer mode 2 at 0xE6879. On the ordinary renderer
constructed by 0xC3150, vtable 0x4385E8 selects the mode at 0xC3DC0.
Mode 2 binds solid fill with face culling disabled, counterclockwise front
faces, and depth clipping enabled. It enables depth testing with the
less-than-or-equal function and depth writes. Target zero has blending
disabled and all four color channels writable. The
state descriptors originate at 0xC323B–0xC33C8, and mode 2 uses the
state objects at renderer +0x9E0, +0x9E8, and +0xA08. The model uses the
position-and-color vertex input and the shader pair created at 0xC60D0 and
bound by 0xC5DE0. The original embedded shader reflection identifies
the vertex shader at 0x4387B0 and the pixel shader at 0x43A010.

The draw uploads the RGBA material multiplier through 0xC6F20 to the
vertex Material constant at slot 3. It supplies 0.0 to the vertex White
constant at slot 7 at pass entry, then 0.0 to vertex Glow at slot 9 and
0.0 to pixel Shininess at slot 5 for the type-4 draw. The original
constant setters are 0xC7A20, 0xC6C50, and 0xC73B0, and the binding order
is in 0xC68C0. The draw selects the model using the stored particle type.
The type-77 initializer supplies the tint and alpha seed in the payload.

## Ordinary scene zone scan and write

The ordinary scene phase receives the same frame's published Game camera
position at Game +0x1DC0, +0x1DC8, and +0x1DD0. At 0xE2EC5–0xE2EDB
it copies the signed 64-bit fixed-point coordinates to Scene +0x338,
+0x340, and +0x348. The frame derives the zone scan radius by multiplying
signed 32-bit Game +0x1B9C by 30, dividing by 100 with signed truncation
toward zero, and taking the greater of that quotient and 3. The scene
stores the resulting signed 32-bit radius at +0x438. The call chain is
0xAD863–0xAD8B2, 0xAD996–0xAD9D4, and 0xE2FDB–0xE2FE1.

The writer routine 0xD3170 runs at call site 0xE3BD1. When scene flag
bit 0x80 is set, it bypasses the zone sweep. For the sweep, it rounds each
camera horizontal coordinate down to a signed 32-bit whole-block coordinate,
then rounds the division of that coordinate by 64 down to a signed zone
coordinate. The sweep uses the published scene camera in both the ordinary and flight
cases. Each axis runs from its center zone minus the radius
through its center zone plus the radius, both endpoints included. X increases
in the outer loop. At each X, Y runs through its full increasing range.
A camera one tick west of zero has center X zone −1. A camera exactly at
block −64 also has center X zone −1. See 0xD34DD–0xD364E and
0xD452E–0xD4586.

The active World is the pointer at Scene +0x08. The lookup uses the
existing zone map at World +0x460 with X in the low half and Y in the high
half of the 64-bit key. A miss, a null zone pointer, or a zero 64-bit zone
+0x1D0 count skips the candidate. The count belongs to the list at zone
+0x1C8, initialized empty by 0xC25C0. The geometry path stages entries at
zone +0x1D8 and the ordinary publication path at 0x84D59–0x84DAB
copies that list into the active list through 0x7CC20. The writer clears
zone dword +0x440 and the consecutive ranges +0x240 through +0x33F and
+0x340 through +0x43F before testing a candidate that has an active list.
The relevant sites are 0xD3651–0xD37BA. The main frame holds the current
World's +0x3C8 critical section across the ambient and scene phases
at 0xA6464–0xADB23, through 0x2ABB70 and 0x2B9890. Geometry publication
at 0xEC71B–0xECB28 and active-list publication at 0x84D82–0x84DAB
use that same World critical section.

For the writer's call to 0xE0680, the arguments are the current Scene,
the zone's lower signed 64-bit fixed-point X, Y, Z tuple at +0x1E8,
the upper tuple at +0x200, and the binary32 sum of Scene +0x4A4 and 64.0
blocks. A single +0x4A4 sample is taken at 0xD34CE for the sweep and
reused for each candidate at 0xD37BC–0xD37D5. The 64.0 is added anew
for each call. The ordinary scene setup calls 0xECDA0 before this write
phase and before setting Scene +0x49C. In that frame +0x49C is the
smallest of Scene +0x4A4, binary32 0.2 times the zone radius in blocks,
and 200.0, at 0xE32AB–0xE330D. Scene +0x49C is used by the particle
culler and the subsequent object tests described here.

In 0xE0680 the horizontal range check uses fixed-point midpoints computed
from the two bounds. Each midpoint is half the signed sum rounded toward
zero at the fixed-point tick. The culler subtracts that position from
camera X and Y, squares each signed 64-bit difference, then divides each
square by 65,536 with signed truncation before adding them. The threshold
is the signed 64-bit truncation of the supplied radius squared in binary32
and multiplied by binary32 65,536. The sum must be at most that threshold.
For the subsequent box test it uses plane records 0, 1, 2, 4, and 5 at
Scene +0x43C, each 16 bytes. Each plane uses upper bound components where
the corresponding coefficient is positive and lower components otherwise.
X and Y receive the Scene +0x428 and +0x430 origin offsets. The selected
X, Y, and Z values are converted to binary32 then multiplied by 1 divided
by 65,536 to obtain block units. The test sums
the Y term, X term, Z term, and constant in that order and rejects a
strictly negative plane result. These checks are at 0xE06A9–0xE0868.
The range check itself has no vertical component.

After 0xE0680, the writer checks the 64-bit zone +0x1D0 count again.
A nonzero count and a successful box test allow appending the zone pointer
to the scene collection at +0x300. The writer then subtracts current camera
X, Y, and Z from the zone's separately stored fixed-point center at +0x218,
+0x220, and +0x228. Each signed 64-bit difference becomes binary32 and is
scaled by 1 divided by 65,536. The writer squares the three results in
binary32, adds X and Y, then adds Z, and stores the result at zone +0x234
at 0xD388F. It then tests the stored value against the squared binary32
sum of Scene +0x49C and 64.0 for subsequent object work. Equality fails
that later object threshold. The store has already happened at that point.
The later routine 0xCE5F0 reads +0x234 at 0xCE72F and uses the same
subsequent threshold on the current scene zone collection.

## Provenance and bounds

The binary32 distance at Scene +0x4A4 starts at zero in the scene
constructor 0xCBD10 and has a zero reset at 0xE8A30. The pre-sweep
routine 0xECDA0 updates it from the current camera-centered zone coverage
and geometry work. That routine begins with the squared zone radius in
blocks and can reduce its distance candidate using the centers of zones
whose entries are missing, null, or have zero active-list count. That test
uses the grid center 32 blocks inward from the origin of each of those
64-block zones. Camera-relative distances for that coverage test are
computed in binary64, with the narrower squared result held in binary32.
For its geometry work limit it considers existing zones with an active list, applies
0xE0680 with a binary32 argument of 100,000.0, and gathers the horizontal
center distances within the reduced coverage and the signed work counts
from zone +0x444. It visits these samples by increasing distance and
uses the smaller of the current and preceding squared distances, with
zero in place of a preceding distance for the first sample, if
the accumulated count first reaches 12,288 times the square of the zone
radius. It takes the square root, subtracts binary32 44.8, and floors the
distance candidate at binary32 0.001.

The subsequent +0x4A4 update in 0xECDA0 has a flight path when the player
state byte at +0x68 is 0x9C and the binary32 elapsed state time at player +0x6C
exceeds 1,000. That path blends toward the greater of the distance candidate
and 64.0. On the other reached path, a component different from zero in
0x2AA140's result or a matching player-list entry of type 0x13 can cap the
target at 256.0. The three-component query runs only when both 0x20 and
0x02 are present in byte +0x05 of the block sample from 0xEAB0 at the
player's position. It uses the player's horizontal whole-block coordinates
truncated toward zero. The list query at 0x5E3A0 checks the player's +0x980
list. If the cap is active and the distance
candidate exceeds 256.0, the routine blends toward 256.0. Otherwise it takes
a lower or equal candidate directly and blends toward a higher one. The
blend weights come from 0xA7A0 with the scene interval and a binary32
coefficient of 0.005 in the flight path or 0.0005 in the other paths.
The weight starts at zero in binary64. It takes the truncation of 100 times
the interval converted to binary64 as the sample count. For positive counts
it moves the weight toward successive fractions through 1.0, spaced by
one divided by that count. Each movement adds the difference between the
current target fraction and current weight, multiplied by the coefficient
converted to binary64 and scaled by binary64 0.02. The final weight becomes
binary32. The new scene distance adds the weight times the target-minus-old
distance to the old value in binary32. See 0xED4B4–0xED711 and 0xA7A0.
The resulting stored +0x4A4 is the writer's distance operand before
adding 64.0. The 0xECDA0 coverage check is a separate call at 0xED1FE
and does not publish zone +0x234.

The ordinary zone creation route at 0x2AE0C0 allocates the zone, runs
0x2F9400 and selected generation at 0x2FF6F0, then inserts a pointer in
the World map. The reached zone constructor calls 0xC25C0 on zone +0x1C0.
This constructor creates the two empty lists and directly initializes the
neighboring dword +0x238 and byte +0x23C. No assignment to +0x234 is
established there. The geometry publisher at 0xE9530 writes both bounds
and their fixed-point midpoint into +0x1E8 through +0x22F. Each stored
midpoint component is half the signed sum of the two bounds with truncation
toward zero. It updates +0x230 and clears the +0x23C byte. Its publication does not set +0x234. The active
list copy at 0x842D0 has the two lists as its destinations. The selected
world edit at 0x2DA090 sets +0x23C and stamps +0x238. The ambient setup
at 0xAF110 also writes neighboring fields, and its second sweep reads
+0x234.

Within the traced ordinary setup and scene path, 0xD388F is the only
established store to zone +0x234. Offset matches alone do not identify
stores to this zone object. For example, the store at 0x28FE46 belongs to a
UI object in 0x28FD60. The review leaves the field's contents before the
first accepted scene write unresolved, including possible effects of storage
history or other indirect writers. A zone outside the sweep, failing either
count check, or failing the box test receives no +0x234 store from this
sweep. Since the ambient pass precedes scene submission, it reads the
value already present at the time of its visit. An official name for +0x234, the names of the player-query flags, and
exhaustive coverage of other lifecycle paths remain open.
