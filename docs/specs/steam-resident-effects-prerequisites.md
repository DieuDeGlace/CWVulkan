# Steam resident cell prerequisites and kind9 effect producer

Stable bounded native supplement, 2026-10-31. Sources are original Steam
**0x2e17d0**, its native instructions and constants, the reached record-copy and
distance helpers, and existing independent height/effect specifications. This
completes the two consumer/producer branches left open in
[steam-authoritative-residents.md §7](steam-authoritative-residents.md#7-explicit-remaining-boundary).
No runtime execution or replacement implementation was consulted.

Positions below are signed-64 fixed units, **65536 per block**. Arithmetic
described as float is float32 unless explicitly described as double. Ordinary
finite inputs and structurally valid native containers are the scope.

## 1. Nonempty spawn+0x928 cell prerequisite: layout and phase

The spawn record has an **ordered tree header pointer at +0x928 and 64-bit
element count at +0x930**, not a flat list of cell pairs. The constructor
**0x118a60** creates an empty tree using **0x86ea0** and zeros the count. The
destructor **0x118d30** clears this tree through **0x53d90**. In a value node,
signed-32 **cell X/Y are at +0x1c/+0x20**. The consumer starts at the header's
leftmost node, then visits native in-order successors. It performs no separate
coordinate sort. The insertion producer/comparator for a nonempty instance of
this currently unused spawn field is not newly established; an oracle must
retain its actual tree iteration order rather than reconstruct an order by
guessing.

The gate is reached only after the common nonnull-live-ID/dead-ID/range checks
and the required-defeated count test. It precedes required-stage and
dead-prerequisite-ID admission. A zero count bypasses it at **0x2e2089–0x2e2091**.
On the nonempty route, initialize one temporary byte to **1**, at **0x2e2097**.
That initialization is for the entire cell traversal, not each cell.

## 2. Cell lookup, indexed objects, and the retained-byte quirk

Each cell is a **16-block XY cell coordinate**, not a zone coordinate. To find
its zone, multiply each signed-32 cell component by **16 retaining the low32
bits**, reinterpret the result as signed32, then floor-divide by **64**. There
is no cell-center offset. This is ordinarily floor(cell/4), but replacing the
multiply-before-division sequence changes overflowing coordinates. These
operations are at **0x2e20d0–0x2e2116**.

Look up that zone key in the existing **World+0x460 loaded-zone map**, through
**0x12700 at 0x2e212e**. A missing key or null mapped zone immediately rejects
this spawn. There is no saved-zone fallback, load request, or search of another
zone for this cell.

For a present zone, traverse its **+0x4a8 linked list of signed-32 object
indices**, in list order. This is a selected index list, not the entire object
vector. An index must be nonnegative and smaller than the native object count
derived from **(+0x68 end minus +0x60 begin) / 0xe0**. The signed64 quotient is
narrowed to signed32 for that comparison. Invalid indices are skipped.

For a valid indexed 0xe0-byte object, require **state byte +0x28 nonzero**.
Convert its fixed X/Y at **+8/+0x10** to 16-block cell coordinates: the
fixed-to-block floor/narrowing operation of resident supplement §2, followed by
floor division by **16**. Both resulting signed-32 components must equal the
current prerequisite pair. Z, object type, item payload, user ID, distance and
LOS do not participate. The object's state byte means shut/not-yet-given for
the interaction families described in
[zone-object-interaction.md §0.2](zone-object-interaction.md#02-zone-object-record-zone-0x60-list-0xe0-bytes-each).
Object disappearance is not required; an object with state 0 fails this match.

The following retained-byte behavior is explicit in the native instructions:

* A matching indexed nonzero-state object clears the temporary byte to **0**
  at **0x2e2276**, stops that cell's index traversal, and advances to the next
  prerequisite cell.
* Exhausting a cell's index list without a match tests that byte at
  **0x2e227d**. If it is still 1, immediately admit this gate and continue to
  required-stage processing at **0x2e22ef**. Later cells are not visited.
* If the byte is already 0, an index-list exhaustion instead advances to the
  next cell. The next-cell backedge **0x2e22dc → 0x2e20d0** does **not** restore
  the byte to 1. Exhausting the cell tree rejects the spawn at **0x2e22ce–
  0x2e22d1**. A missing later zone also rejects immediately.

Consequently, for an ordinary nonempty valid tree, admission is controlled by
its **first cell**: its zone must be loaded/nonnull and it must contain **no
matching nonzero-state object in the +0x4a8 index list**. If the first cell does
find such an object, later cells cannot restore admission, although their
lookups/scans are still reached until traversal ends or a zone is absent.
This is not an all-cells-cleared test. Preserve the native quirk.

Examples, assuming the earlier common gates pass:

* First cell has a loaded zone and an empty index list: admit this gate
  immediately, even if a later cell's zone is missing.
* First cell has a matching indexed state 1 object; next cell is loaded and
  empty: reject after the traversal. Clearing the second cell does not help.
* First cell's zone is absent: reject. A later loaded empty cell cannot help.
* A matching-position object is state 0, unlisted, or addressed only by an
  invalid index: it does not block the first-cell admission.

This branch does not change an object, spawn record, World collection, random
state or sound/output list. The generator audit still has no identified
producer that fills this tree. Its **consumer is now specified**; the missing
producer remains an explicit limit, not a reason to invent generated cells.

## 3. Per-zone kind9 request: exact mode0x8a bookkeeping

This branch remains within **0x2e17d0**, with the same authority admission and
unsigned Zone-pointer order as the main resident supplement. Immediately after
each zone's daily refresh, reset two temporary per-zone flags: permission=1,
effect-request=0. Then walk that zone's spawn map in the established signed-ID
key order. The flags are stack-local and are not stored in the zone.

Only these events affect them:

* When the live-ID lookup already finds a **nonnull creature pointer**, and
  the spawn's **full signed32 mode field +0x8bc equals 0x8a**, clear permission.
  This is **0x2e1e12–0x2e1e2a**. It does not inspect the existing creature's
  current ability, HP or range, and does not clear an already-set request.
* For a newly constructed creature whose copied **low mode byte is 0x8a**,
  first perform the CRT draw and altitude increment from resident supplement
  §5. Then, if permission is still 1, set effect-request=1 and permission=0,
  at **0x2e26b5–0x2e26c9**. A new mode0x8a creature still takes its altitude
  draw when permission was already 0.

Rejected dead-ID/far/prerequisite points do not close permission. No other
mode closes it. Thus the first encountered relevant event decides whether the
zone requests an effect: an already-live mode0x8a closes permission without a
request; a successful new mode0x8a sets a request. A later existing live point
does not retract that request. The existing-live route's full32 comparison and
new route's low-byte comparison are distinct for noncanonical mode words.

After completing the spawn map, produce at most **one candidate kind9 effect
for this zone** if requested, at **0x2e28c8**. Suppression below does not undo
the resident creation or altitude draw. There is no saved pending request and
no automatic retry record: the next call starts fresh and its now-live point
can close permission.

## 4. Candidate position and day-derived direction

The effect direction uses **World day signed32 at +8 reinterpreted as uint32**.
Apply the specified 32-bit xorshift sequence: XOR with itself shifted left by
13 bits, then XOR with itself shifted logically right by 17 bits, then XOR with
itself shifted left by 5 bits, retaining 32 bits after each step. Take its **unsigned remainder
10000**. This computation at **0x2e28d5–0x2e28fd** is independent of zone
coordinates and consumes no CRT draw or seed.

Convert the remainder to float and **divide by 10000.0f** (constant
**0x42e5e8**, word0x461c4000). Double that result by a float addition to itself.
Widen to double, multiply by stored double **π at 0x42e758**
(word0x400921fb54442d18), then narrow to float. This is the angle supplied to
native **cosf/sinf**, not double trig. Arithmetic is at **0x2e2901–0x2e2920**.
All requested zones on the same day use the same angle.

For each axis, the zone center block coordinate is the low32 result of
**zone+0x10/+0x14 multiplied by64, then plus32**, interpreted as signed32.
Sign-extend that result to64 and multiply by65536 for its fixed center.

At **0x2e293f**, query **0x35ea0(World, centerBlockX, centerBlockY, mode0)**.
This is the existing ground-height provider from
[worldgen-terrain-height.md](worldgen-terrain-height.md). Add **400.0f**
(**0x42e7e8**, word0x43c80000), multiply by **65536.0f**
(**0x42e5f8**, word0x47800000), and truncate the float result toward zero to
signed64. This is the initial fixed Z; the conversion occurs at **0x2e2951**,
before XY trig/deduplication. It is a query at the zone center, not at the
candidate's displaced XY.

For candidate X, multiply cosf(angle) by **200.0f**, then multiply that float
result by65536.0f, truncate to signed64, and add it to fixed centerX. Candidate
Y uses sinf(angle) through the same two float multiplications and truncation,
then adds fixed centerY. The additions retain low64 bits. Radius200 is
**0x42e5c0**, word0x43480000. These operations are at **0x2e2985–0x2e29b6**;
do not combine the trig offset with an absolute float center before truncation.

## 5. Existing-effect suppression

Before constructing/outputting the candidate, scan **World+0x60 live effect
list** in forward order, including effects appended by earlier zones in this
call. Only **record kind +0x68 == 9** participates. Call the XY distance helper
**0x2a5ac0 at 0x2e29fb** with candidate fixed XY, existing effect fixed XY,
and **200.0f**. Stop and suppress the candidate on the first effect within
that inclusive radius. No owner, age/lifetime, Z, zone, resource or HP test is
made. An old record awaiting removal by the later effect step can suppress it.

The distance helper subtracts fixed signed64 components first, converts the
differences to double, and multiplies by double1/65536. Absolute X/Y must each
be at most200. The squared XY sum is double; radius squared is calculated as
float200×200 before widening. The helper reports outside only when that
squared sum is **strictly greater** than radius², so equality suppresses.
The helper's high return bits are irrelevant; the caller tests the low byte.

If suppressed, no kind9 record is appended and no existing effect is moved,
renewed or modified. The terrain-height and trig work already occurred.

## 6. Constructed record, outgoing copy and lifetime boundary

On an unsuppressed candidate, initialize a **0x80-byte effect record** directly
at **0x2e2a1f–0x2e2b09**. The general effect constructor is not called here.
Its meaningful initialized fields are:

| Offset | Value |
|---|---|
| +0x00 signed64 owner | -1 |
| +0x08/+0x10/+0x18 signed64 position | candidate X/Y/Z from §4 |
| +0x20/+0x28/+0x30 signed64 target | zero |
| +0x38 eight bytes, +0x40 four bytes | zero |
| +0x44/+0x48/+0x4c float velocity | normalized centerward XY ×20, Z as below |
| +0x50 float damage | **not initialized by this producer** |
| +0x54 float contact radius | **80.0**, word0x42a00000 |
| +0x58 float size | **20.0**, word0x41a00000 |
| +0x5c float charge, +0x60 float auxiliary | zero |
| +0x64 flag byte | zero |
| +0x68 signed32 kind | **9** |
| +0x6c hit-type byte | zero |
| +0x70 float age | +0.0ms |
| +0x74 signed32 lifetime | **20000ms** |
| +0x78 signed32 freeze time | zero |

For velocity, subtract candidate fixed XY from fixed center XY as low64
signed differences. Convert each difference to float **before** multiplying by
float1/65536 (**0x42e510**, word0x37800000). Square X and Y separately in
float, add X²+Y² in float, then add +0.0f. Apply sqrtf and compute **1.0f /
length** (1.0f is **0x4ac980**, word0x3f800000). Multiply each block difference
by that reciprocal, then by **20.0f** (**0x42e5a0**, word0x41a00000), storing
velocity X/Y. Z is calculated as **(reciprocal × +0.0f) ×20.0f**, not obtained
from a vertical displacement. Ordinary radius200 candidates have nonzero
length. No extra normalization fallback or length clamp occurs here.

For example, day 0 gives angle 0. The candidate is 200 blocks east of center,
at the specified center-height-plus400 conversion, and the finite ordinary
velocity is (-20,0,0) blocks/s. This is an arithmetic example, not a runtime
capture.

One zero 64-bit store initializes +0x5c/+0x60 together. A separate 64-bit store
writes lifetime 20000 at +0x74 and freeze time 0 at +0x78.
**Damage+0x50 remains unwritten**; do not infer a
zero value from the ordinary constructor's other defaults. Semantic copy
**0x38a40** nevertheless copies that word. It copies defined record fields,
not padding bytes +0x65..67, +0x6d..6f or +0x7c..7f. Damage is not used by
the reached no-contact kind9 update described in effect-objects §1.5; other
render consumers are not newly audited by this producer supplement.

First append the record to the tail of **World+0x60**, updating count+0x68,
through **0x36ce0 at 0x2e2b2b**. Then append an independent copy of the **same
source record** to outgoing **World+0xd0**, updating count+0xd8, through
**0x36ce0 at 0x2e2b73**. Native argument loads explicitly supply the same
source address on both calls. These are value copies, not a shared effect
pointer. There is no sound or actor/item award generated by this branch.

The per-zone temporary request is cleared during candidate processing. The
next zone repeats its own admission/bookkeeping, while seeing the newly
appended live effect for suppression. After **all** selected zones have been
processed, the existing final petrification/list cleanup pass runs once.

The effect later reaches **0x2dba40 in this same World step**, after creature
processing. Its movement, age, kind9 terrain-height smoothing and expiry are
already specified in [effect-objects.md §§1.1–1.5](effect-objects.md), and host
outgoing-container lifetime is in [steam-world-daily-reset.md](steam-world-daily-reset.md).
Updating the live copy does not update the outgoing creation copy's age or
position. This supplement does not add a replacement kind9 mesh/asset or
invent an untraced cell-prerequisite producer.
