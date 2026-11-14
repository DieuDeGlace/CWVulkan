# Steam ambient emitter admission and order

## Authority and scope

This contract covers the ordinary Windows x64 Steam client frame that calls the
ambient emitter pass at RVA 0xAF110. The installed CubeWorld.exe and the archived
packed original are each 5,939,072 bytes with SHA-256
A1EFBBD41EEB8DF08395C4D6511194D416CB70A14054B2A2000B9F5CDBCFA65B.
The matching unpacked analysis image is 5,757,952 bytes with SHA-256
7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244.
All routine and site addresses below are RVAs in that unpacked image. The
evidence is the original instruction and routine exports, cross-checked against
imports and constants from the unpacked image. Field offsets name their owning
object. X and Y are the first and second horizontal axes of the packed world
zone key.

## Main frame and clocks

The main loop in 0x1355A0 calls the frame wrapper 0x134420. The wrapper clears
the render and depth targets, measures the performance-counter interval in
double-precision milliseconds, and calls Game update 0xA5A50 at site 0x134558.
The Game update caps the ordinary finite interval at 1,000 ms. The ambient pass
gets that capped frame interval converted once to a signed 32-bit integer by
truncation toward zero at 0xAAED0. The camera update receives the double interval.

On the ordinary world-update path, Game update advances the current World
earlier through 0x2DA6C0. It uses 16 ms simulation steps while the remaining
interval exceeds 16 ms and then submits the remainder as binary32. There is a
separate world-identity check around that simulation work. At the ambient call
site the sequence is 0xB37F0, 0xB36A0, 0xB6240, 0xB03D0, camera update 0xADCD0,
then ambient pass 0xAF110. The last two call sites are 0xAAF02 and 0xAAF0C. The
camera update has already written the three signed 64-bit fixed-point camera
coordinates at Game +0x1DC0, +0x1DC8 and +0x1DD0 and the view transform beginning
at Game +0x1EE4. Those are the listener inputs that the positional sound helper
0xA0420 reads.

The pass runs once at this point in the traced Game update. It follows the
update of existing particles in 0xB37F0. The soundtrack selection and music
request or volume publication occur later in 0xA5A50, including the Music
path publication under Game +0x1D38. A separate music worker at 0x83FB0
consumes the path and volume under that critical section. The frame order
establishes the order of main-thread decisions and requests. Playback timing
belongs to the audio backend and worker. Game update subsequently calls the
scene renderer 0xE2E00 at 0xAD9D4. The draw uses the same published camera inputs.
After Game update returns, the frame wrapper handles its display checks and
frame pacing, then calls Present on the DXGI swap chain at 0x13474E. Swap-chain creation
for that object is at 0x134CC0. Game update acquires the current World's
critical section +0x3C8 at 0xA6464 and releases it at 0xADB23. Both ambient
traversals and the later scene call execute within that outer scope.

The source World is the current pointer at Game +0x08. World +0x440 holds a
binary32 elapsed millisecond accumulator. The World constructor at 0x29A130
initializes it to zero at 0x29A2E4. World clock update 0x2E2E80 adds each
binary32 simulation step to it at 0x2E2F8B. For a timed emitter, 0xAF110 reads
the current accumulator and truncates it to a signed 32-bit integer before
adding any phase or the signed integer frame interval. Its two time endpoints
are the current integer sample with that emitter's phase, and that same
phased sample plus the frame interval. A trigger is a difference in the signed
integer quotients of those endpoints by the branch's interval. Divisions
truncate toward zero and additions use the native 32-bit width. A change
across several buckets still allows one chance draw for that record. A zero
frame interval cannot trigger a timed branch.

World +0x0C is the separate binary32 time-of-day sample in game milliseconds.
In the World clock update it advances by ten times the simulation interval,
or by a factor of 100 under the alternate actor-state condition checked there.
If the 64-bit field at World +0x210 is zero, that routine sets the day sample
to 32,400,000 ms. The day rollover routine subtracts 86,400,000 ms when the
sample is strictly above that value. Emitter branch selection reads the
current day sample. Only World +0x440 and the integer frame interval enter the
emitter cadence calculations. The pass also samples WinMM timeGetTime at entry
for the separate zone writes described below.

## Preliminary zone writes

Before choosing the emitter scan center, 0xAF110 walks the current World's
list at +0xC0. The two block coordinates in each list node are signed
32-bit values at node +0x10 and +0x14. This is a list whose entries the
selected simulation path 0x2DA6C0 also associates with zone change records.
For each entry the pass takes the combinations of displacement minus 13,
zero and plus 13 blocks on each horizontal axis. Each displaced coordinate
is floor-divided by 64 to make a zone coordinate. In the temporary ordered
set, equal pairs coalesce and the traversal is ascending signed X, then
ascending signed Y within X. That ordering and uniqueness are corroborated
by 0x74440.

For every unique key, the pass looks up an existing zone in the current World.
If the zone exists and its byte at +0x23C is zero, the pass sets that byte
to one and writes the entry timeGetTime sample to the 32-bit slot at zone
+0x238. Each such write in the call uses the same sample. A nonzero byte leaves
the timestamp as stored. No random draw occurs in this preliminary pass.
The resulting key set can include zones outside the later 3-by-3 scan. The
emitter scan walks its neighborhood even when the preliminary list is empty.

The selected zone rendering-data publication in 0xE9530 clears zone +0x23C
at 0xECB1B. The zone-work selector at 0x842D0 reads that byte in a condition
that also uses zone +0x444. The undo path at 0x2DA090 sets the byte and writes
another timeGetTime sample at 0x2DA1A4 and 0x2DA1B1. These selected reads and
writes establish the byte and timestamp as distinct fields from the float
gate at +0x234.

## Emitter scan center, lookup and gate

The player pointer is at World +0x448. For the ordinary non-flight center,
the X and Y inputs are signed 64-bit player coordinates at +0x10 and +0x18
with 65,536 fixed-point ticks per block. The conversion takes the whole block
by rounding down, including for negative fractional positions, and uses a
signed 32-bit block intermediate. A second downward division by 64 produces
each zone index. A negative position one fixed-point tick below zero belongs
to zone minus one. An exact boundary stays in the zone beginning there.

Player state byte +0x68 equal to 0x9C selects the flight center. It uses the
signed 32-bit destination block coordinates at player +0x1B0 and +0x1B4.
They go directly through downward division by 64. The flight status and
destination use are corroborated by the arrival display in 0xA5A50 and the
flight-point display in 0x28CC30. This changes the neighborhood center.
Positional playback still reads the published camera coordinates and view.

The emitter traversal visits nine keys geometrically. X runs from one below
the center through one above, in ascending order. For each X, Y runs from one
below through one above, in ascending order. The exact packed key has X in
the low 32 bits and Y in the high 32. Both passes use the existing World
zone map at +0x460. The lookup uses 64-bit FNV-1a on the eight little-endian
key bytes, with offset basis 0xCBF29CE484222325 and prime 0x100000001B3, then
uses the 64-bit mask at World +0x490. It checks exact key equality in the
selected bucket. The bucket table is at World +0x478 and the missing sentinel
at +0x468. This order comes from the two coordinate loops in 0xAF110 and is
independent of bucket order. The pass skips absent keys and null zone pointers.
Its lookups never request creation.

At 0xAF6A8, admission requires the existing zone's binary32 value at +0x234 to
be strictly below 4,096. Equality and unordered values are excluded. The test
reads the stored slot before enumerating records or doing any emitter draws.
There is no admission test on zone +0x23C, zone +0x238, the zone's render-list
size, or a newly calculated emitter distance in this loop.

## Selected provenance of zone +0x234

For an ordinary generated zone, 0x2AE0C0 allocates it, calls 0x2F9400 to
construct it, runs 0x2FF6F0 and publishes the pointer to the World zone map.
The selected publication takes World +0x3F0 followed by +0x3C8. At 0x2F95DA
the constructor calls 0xC25C0 on the zone subobject starting at +0x1C0.
That constructor initializes its own +0x78 and +0x7C, corresponding to zone
+0x238 and +0x23C. Its +0x74, corresponding to zone +0x234, has no store
in the constructor. The allocation at 0x2AE148 is ordinary operator new,
with the allocation body at 0x392BAC calling malloc. The selected path
therefore establishes no numeric fresh-allocation default for the ambient
gate. The selected ordinary rendering-data publication at 0xE9530 writes the
signed 64-bit zone center at +0x218, +0x220 and +0x228, the separate binary32
bound at +0x230, and clears +0x23C. Those stores do not initialize +0x234.

The ordinary admitted scene-zone path in 0xD3170 is a verified writer. After
scene entry 0xE2E00 publishes the same camera position at scene +0x338, +0x340
and +0x348, 0xD3170 examines its scene zone range. The selected path requires
the scene flag bit 0x80 clear, an existing zone with a nonzero 64-bit slot
at zone +0x1D0, and success from the zone bounds culler 0xE0680. It then
appends the zone to the scene's current zone collection at +0x300. At
0xD388F it stores the sum of the squares of three zone-center-to-camera
differences in zone +0x234. Each signed 64-bit difference is converted to
binary32 and scaled by one over 65,536 before squaring. The sum adds the
Y square to the X square, then the Z square. At 0xD38A9 and 0xCE72F the
scene uses a different threshold. It adds 64 to the binary32 value at
scene +0x49C, squares the sum, and requires zone +0x234 strictly below it. The 0xCE5F0 reader walks the
scene's current zone collection.

That scene writer runs after the ambient scan for the same frame. The
ambient scan therefore observes whatever value already resides in the slot
at its call point. A previously admitted scene zone can carry the most recent
scene-written sample, including across a later frame in which it misses
the scene writer's admission. The selected evidence establishes the
comparison and that writer, with no inferred canonical name for +0x234,
no assertion about the value of its bytes before the first established
write, and no exhaustive inventory of indirect or exceptional writers.

## Records and interval phasing

An admitted zone supplies the consecutive records between its begin pointer
at +0xA8 and end pointer at +0xB0. The stride is 64 bytes. The stored 32-bit
type at record +0x00 is the effective type used here. The signed 64-bit
fixed-point X, Y and Z are at record +0x08, +0x10 and +0x18. The records
are visited in increasing address order. The signed 32-bit ordinal
starts at one separately in each zone and counts every record, including
types with no action in that frame.

The selected ordinary procedural producer at 0x3191B0 is called by 0x2FF6F0
at 0x317136. At 0x31A553 it empties the ambient range, then copies records
of types 31 and 77 through 80 from the zone's +0x18 to +0x20 record range
into it, in source order. The append at 0x31A59B uses 0x69B30. The other
type branches below are present in the reader. The selected ordinary
producer does not place those other types in this vector.

Type 77 uses an unshifted 1,000 ms comparison. The spatial cadence for
types 78 through 80 and for the cricket cases converts X to whole blocks
by truncation toward zero. It adds 333 blocks, or 0x14D0000 fixed-point
ticks, to signed 64-bit Y before converting Y the same way. The converted
X and Y are signed 32-bit for the interval arithmetic. Each coordinate
contributes its own signed remainder, using divisor 1,000 for types 78
through 80 and divisor 5,000 for crickets. The remainders sum to the phase.
The signed quotient bucket is 1,000 ms for types 78 through 80 and
5,000 ms for crickets.

Type 31 first multiplies both signed 64-bit fixed-point coordinates by
13 and applies the same Y addition after multiplying Y. The converted
signed coordinates contribute signed remainders by 1,000, including when
negative. The bucket size for type 31 is 500 ms. The fixed-point conversion
for cadence truncates toward zero, whereas center-zone selection rounds
down. The phase uses the horizontal coordinates only. The sound request
retains the original three fixed-point coordinates.

## Branches and exact draw order

The draws below are successive calls to the imported Windows CRT rand on
the calling thread. The pass performs no local reseed. Other calls on the
main thread, including earlier particle or flight work and later frame
effects, can consume the same stream. Admission, time window, ordinal and
bucket tests consume zero draws. A failed chance test consumes its one
chance draw only. The successful paths perform all their stated draws
before submitting the effect or advancing to the next emitter.

- Type 77 is eligible at any time of day. On a 1,000 ms boundary test
  success, draw once and require divisibility by 16. On chance success,
  draw for horizontal velocity Y, then X, followed by angular-rate Z,
  Y and X. Each of these five draws is converted to binary32, divided by
  32,767, shifted down by 0.5 and then multiplied by 4 for velocity or
  10 for angular rate. Vertical velocity is minus 2. The pass creates a
  type-4 particle at the record position, taking its three color values
  from record +0x2C, +0x30 and +0x34. It starts at age zero, with an opaque
  color alpha, scale 0.5 and a 20,000 ms lifetime setting. The insertion
  at 0xAF8C0 puts it before the old first entry of the Game list at
  +0x2090. It has already missed this frame's 0xB37F0 particle advance and
  may be drawn later in this frame by the ordinary scene particle pass at
  0xE6840, subject to its render gates. This branch makes no sound request.
  The six draw sites in order are 0xAF739, 0xAF772, 0xAF789, 0xAF7C8,
  0xAF7DF and 0xAF7F6.

- Type 31 is eligible at any time of day. On the phased 500 ms boundary
  test success, draw once and require divisibility by 4. Then draw pitch,
  then the sample variant. Multiply the pitch draw by binary32 0.4,
  divide by 32,767 and add binary32 0.8. Use the variant's remainder by
  3 to select sound ID 104, 105 or 106 in that order. Request gain 0.1.
  The draw sites are 0xAFA22, 0xAFA3E and 0xAFA60.

- Daylight is strictly after 21,600,000 and strictly before 75,600,000
  game milliseconds, or between 06:00 and 21:00. Types 78, 79 and 80
  take their daylight branches in that window. After the phased
  1,000 ms boundary test, the chance draw must be divisible by 10.
  A success draws pitch and then submits the respective sound ID
  97, 98 or 99 at request gain 0.1. Multiply the pitch draw by
  binary32 0.2, divide by 32,767 and add binary32 0.9. The paired
  chance and pitch sites are 0xAFBBB and 0xAFBE4, 0xAFCFC and
  0xAFD25, then 0xAFE3D and 0xAFE66 for the three types respectively.

- Outside that daylight window, including exactly 06:00 and 21:00,
  type 78 uses the same phased 1,000 ms boundary test and one chance
  draw divisible by 10. On success it draws pitch, then draws the
  variant. Pitch uses binary32 0.2 and a base of 0.5 with the same
  divisor 32,767. The variant remainder by 2 selects sound ID 102
  or 103 in that order, at request gain 0.1. Types 79 and 80 produce
  no sound outside their daylight window. The type 78 draw sites on
  this path are 0xAFF7E, 0xAFFA7 and 0xAFFC3.

- The cricket cases require a day sample strictly after 43,200,000
  and strictly before 86,400,000 game milliseconds, and an ordinal
  divisible by 20. These conditions admit types 4 through 8 for
  sound ID 100, or types 0, 1, 11 and 12 for sound ID 101. Both
  groups require a phased 5,000 ms boundary change, then a chance
  draw divisible by 70. On success they draw pitch once with the
  same multiply, divide and add order as the daylight bird pitches.
  Request gain is 0.05. The chance and pitch sites are 0xB0274 and
  0xB029A for sound 100, and 0xB0144 and 0xB016E for sound 101.
  The cricket window overlaps the daylight window, and these types
  use the cricket conditions in either part of that overlap.

Unmatched types have no output. A fixed sound ID consumes only the chance and pitch draws. Positional audio requests are immediate and serial
within the record traversal, through the call at 0xB02D0. At 0xA3A40 the
sample names under Sounds are bird1.wav through bird3.wav for IDs 97
through 99, cricket1.wav and cricket2.wav for 100 and 101, owl1.wav and
owl2.wav for 102 and 103, and river1.wav through river3.wav for 104 through
106.

## Playback boundary

The shared helper 0xA0420 receives the original emitter position, sound
ID, request gain, pitch and a cleared request flag. It uses the freshly
published Game camera position to take three signed 64-bit differences,
converts each to binary32 world units and sums their squares. It rejects
squared distances above 4,096 before resolving a sample. At or below
4,096 it multiplies the request gain by one minus squared distance
divided by 4,096, then by the signed 32-bit sound volume setting at Game
+0x1BA4 converted to binary32, then by 0.01. It also computes the
forwarded pan from minus 0.1 times the view-transformed horizontal
coordinate divided by the homogeneous coordinate, clamped to minus one
through one for finite inputs. The helper's music-ducking ID range is
109 through 119, outside the IDs issued by this pass.

The ordinary XAudio2 effect wrapper 0x343C0 is selected through the audio
object created by 0x328C0 and initialized by 0x34270. It requires positive
gain and sample data in order to create the voice. It forwards the
requested pitch to the voice and uses the computed gain. The wrapper
does not use the forwarded pan or request flag. At exactly 64 world
units the positional helper can reach sample resolution, with zero gain
and consequently no playback in this wrapper. Admission and the record's
draws have already occurred before the positional helper or backend can
reject playback.

The scope is the traced ordinary frame, selected generated-zone and scene
paths, nominal game coordinates and finite time values in the native
widths. It establishes no whole-world random seed schedule, asset
availability or mixer output, exhaustive zone writer closure, or
runtime playback measurements. No reconstruction source was needed.
