# Steam World object clocks and trap-hit tail

Stable bounded native contract, 2026-10-31. Sources are original **0x2e3510**,
its native instructions/constants, **0x137e0**, **0x127f0**, and the already
independently specified state-change/hit helpers. This covers selected-zone
identity/order, per-object clock/personal reset, and the automatic/hit tail
for **object types0x11,0x12,0x13**. It does not claim completeness for the
other object families in this large prelude. No game execution or replacement
implementation was consulted.

## 1. Scheduling and actual selected-zone identity/order

The World step **0x2da6c0 calls0x2e3510 at 0x2da73c**, after the time prelude
**0x2e2e80 at0x2da728** and auxiliary rebuild at0x2da730. This is per World
substep, including an admitted zero interval; it is not a rendering callback.
The argument is **float32 milliseconds**. See the existing World/frame-order
and [daily-reset](steam-world-daily-reset.md) contracts for admission/splitting.

During its shared collection phase,0x2e3510 visits the current **World+0x40
registration list in forward order**. Every entry with **kind byte+0x60=0**
contributes, with no HP, local-ID, visibility or camera-distance test. It
appends borrowed pointers to its temporary player list through **0x36c20
at0x2e3c48**. Before testing each entry's kind, the same traversal clears
creature state bits0x2002 at+0x128 through mask0xdffd. This applies to
**every registration-list entry**, including nonzero kinds, with no HP
gate; only player-list insertion and zone contribution require kind0.

For each contributing creature, convert its **physical fixed X/Y at+0x10/
+0x18** by the exact conversion in §2, then floor-divide its resulting signed32
block coordinate by64. Take the inclusive rectangle **center zone X/Y±3**.
Visit X ascending in the outer loop and Y ascending in the inner loop. Only
successful lookups with a nonnull pointer in **World+0x460 loaded-zone map**
enter the collection. No missing zone is loaded by this collection.

Insert the borrowed **Zone pointer** through **0x73f40 at0x2e3dba**. This is
an ordered set whose comparison is **unsigned64 pointer value**, not an XY
set. After collection, actual zone visitation is its in-order traversal:
**ascending unsigned Zone-pointer order**. Overlap/aliased keys give one visit
per identical pointer. Player order and XY lookup order must not substitute
for that final pointer order. With no contributing player the set is empty.

For each selected zone, existing mission/tracked/ground-item work precedes
the object vector. Visit **current Zone+0x60 begin through +0x68 end**, stride
**0xe0**, in vector order. This is not the generation backup at+0x78 and not
the selected object-index list at+0x4a8. Static-object membership is not
filtered by state or by the trap types before the clock addition below.

## 2. Verified coordinate conversion, including out-of-range negative inputs

**0x137e0**, called for object X/Y by the personal-state reset below, performs
the same narrow-block-first operation as
[steam-scene-lighting.md §3.5.1](steam-scene-lighting.md):

1. Divide signed64 fixed by65536 toward zero, retaining a signed64 quotient.
2. Narrow that quotient to its low32 bits, interpreted as a signed32 **block**.
3. For a negative original fixed input, compare the original with the
   sign-extended **narrowed** block multiplied by65536. If unequal, decrement
   the block with32-bit wrapping.
4. Floor-divide that resulting signed32 block by16 to produce the signed32
   cell component.

The native narrowing/reconstruction comparison is explicit at **0x137f0–
0x13800**. The last floor division is **0x13813–0x1382e**. There is no float
conversion and no full64 cell floor before narrowing.

The **player selected-zone path in0x2e3510** inlines exactly steps1–3, then
floor-divides the signed32 block by**64**, rather than16. Only afterward are
the ±3 zone bounds formed. It does not add/subtract a physical192-block
offset before the narrowing stage.

For a concrete exact fixed input **-2⁴⁸**, division gives block quotient
-2³², narrowing gives0, and negative reconstruction differs from the original:
the corrected block is-1. The personal cell and player center-zone conversion
both therefore give **-1**. Full64 flooring would give different values.
For **+2⁴⁸**, narrowing gives0 with no negative correction, hence cell/zone0.
These are arithmetic examples from the native integer sequence.

The personal-state key is two signed32 components, X then Y, in **8 bytes**;
its set comparison is signed X/Y lexicographic. See the lighting contract for
the matching node field layout and negative-wrap examples.

## 3. Per-object order: clock, personal state, then automatic/hit tail

For **each individual object**, first add the step float milliseconds to its
stored **float32 timer+0x2c**, at **0x2e4b15–0x2e4b1b**. There is no cap,
completion reset, positive-dt gate, state gate or authority gate on that
addition. Do not update all objects' clocks in a separate pass before their
other work. The addition alone does not mark the zone dirty.

Immediately afterward, when **World+0x448 local-player pointer is nonnull**
and object type is **0 or0x16**, derive the cell key from its fixed X/Y at
**+8/+0x10 through0x137e0** and look in the local player's personal set
**player+0x1528 → personal+0xa8**. Desired state is **1 if absent,0 if present**.
If that differs from current state byte+0x28, write that byte and set the
**entire timer float to+0.0**, at **0x2e4b8a**. This is direct assignment,
without the ordinary swing-reversal helper or its sound. It has no authority
gate. Other object types skip this personal reset.

Other reached object-family handling lies between this phase and the final
tail. The tail reads current state/timer after those operations. The
following automatic trap state cycle is **authority only**:
**World+0x358 must be0 at0x2e59a4**. Types other than0x11/0x12/0x13 leave this
tail. Clients still performed the earlier clock/personal-state work.

## 4. Automatic state cycle for0x11/0x12/0x13

If the current object timer is **strictly greater than2000.0f** (constant
**0x42e5d8**, word0x44fa0000), at **0x2e59d1–0x2e59db**:

* A nonzero state requests **state0**, through **0x2d3a10 at0x2e59f3**.
* State0 scans the current World+0x40 list forward for **some kind0 creature**
  within the strict nominal20-block XYZ distance test below. There is no HP
  test. Stop at its first success and request **state1** through **0x2d3a10
  at0x2e5a93**. If none qualifies, keep state0.

Both distances in this document use physical points: object fixed XYZ at
**+8/+0x10/+0x18**, creature fixed XYZ at **+0x10/+0x18/+0x20**. For each
axis, subtract as low64 signed arithmetic, square retaining low64 bits,
interpret the product as signed64, and divide by65536 **toward zero**. Add
the three quotients as low64 arithmetic, X then Y then Z. Compare the final
signed64 sum **strictly less than0x1900000** for activation (400×65536).
There is no float sphere, object extent, drawn position, LOS or frustum test.
The per-axis quotient staging must not become a single division of a summed
square. Overflowing products retain the native low64/signed behavior.

The shared **0x2d3a10** writes the desired low state byte and sets timer to
the float **max(0,1000−old timer)**, preserving its native subtraction and
comparison; see [zone-object-interaction.md §3](zone-object-interaction.md).
For this >2000 input the resulting finite timer is0. If a change occurred,
insert the **current Zone pointer into World+0x1a0** dirty-object-zone set
before continuing. It is a deduplicated pointer set, not an immediate object
erase. Read state again after the callback; state0 skips all hit production.

That callback's sound precedes any trap hit: types0x11/0x12 have sound0x38/
0x37 only for requested state1; type0x13 uses the ordinary default0x39.
Those state-change sounds use object position, pitch1, volume1, owner0 under
the existing helper contract. This producer adds no random chance to the
automatic state transition. At timer≤2000 there is no automatic transition,
but the hit gates below are still evaluated.

## 5. Exact hit timing and the World clock input

Hits require the **post-transition state byte to be nonzero**.

For **type0x11 or0x12**, take current **World+0x440 float32 milliseconds T**.
This stored clock was already incremented by the earlier0x2e2e80 in the same
World step; do not substitute the pre-step clock. First calculate
**float32(T+dt)**, truncate to signed32 with the native SSE conversion, then
divide that signed32 by**200 toward zero**. Separately truncate current T
to signed32 and divide by200 toward zero. Produce the hit traversal only
when these two integer buckets differ. The native future-bucket calculation
precedes the current-bucket calculation at **0x2e5ae0–0x2e5b23**.

This is a forward test from the already updated clock, not an old-to-current
crossing test, not division in float, and not a modulo equality. For finite
T=199.75 and dt=0.5, the buckets are0 and1 and it triggers. An admitted dt0
does not trigger these periodic types. Signed32 conversion is truncation
toward zero, not rounding/flooring; unrepresentable/NaN SSE conversions have
the hardware indefinite0x80000000 result under masked exceptions, not a
saturating or64-bit conversion.

For **type0x13**, the gate is instead **timer+0x2c exactly0.0f**, tested at
**0x2e5b2f–0x2e5b3f**, after addition/any state transition. A positive interval
usually advances an existing zero timer before this test; a fresh automatic
state1 transition can reset it to0 in this same object visit. A dt0 visit can
leave an already-zero active timer at0 and run the traversal again. No
separate consumed-hit latch is written.

## 6. Victim visitation, nominal5-block admission, push and damage

On a trigger, walk the **current full World+0x40 list**, forward from its
head at **0x2e5b45**. This is not the temporary player list or the live-ID
tree. The producer excludes **only kind byte+0x60==1**; it has no HP, player,
pet, owner, immunity or local-ID predicate. Those concerns belong to the hit
callback. Next is read after each reached hit/sound operation; this producer
does not erase a creature or take a separate victim snapshot.

Use the same low64/per-axis signed-division arithmetic of §4, with strict
threshold **0x190000 =25×65536** instead. This is the nominal5-block XYZ
admission at **0x2e5b64–0x2e5bcf**. At exactly5 blocks on one axis with the
others zero, reject. Per-axis integer truncation, rather than an ideal float
radius, is authoritative near the surface.

For an admitted target, construct push from **target minus object** fixed
X/Y, each difference converted to float32 then multiplied by **1/65536.0f**
(**0x42e510**, word0x37800000). The native reloads at **0x2e55cb/0x2e5792**
precede this tail; unrelated earlier scalar uses are not its scale input.
Set Z initially0. If float32(X²+Y²+0) is strictly positive, normalize with
**0x127f0 at0x2e5c30**: float square/sum, sqrtf, float reciprocal1/length,
then separate component multiplications. Otherwise retain zero XY. Multiply
XY by **20.0f** (0x42e5a0, word0x41a00000), and set Z to**5.0f**.
For type0x13, double X and Y by **adding each component to itself**, and set
Z to**10.0f**. These writes are at **0x2e5bd5–0x2e5c85**.

Compute **maxHP(target) through0x5fac0 at0x2e5c92**, then multiply by float
**0.1** (0x42e528, word0x3dcccccd). For type0x13 multiply that result again
by float**5.0** (0x42e584, word0x40a00000). Preserve these separate float
roundings instead of replacing them with one multiplication by0.5.

## 7. Exact hit invocation and state ownership

At **0x2e5cf5**, invoke the shared **0x29eb60** contract:

| Argument | Trap producer value |
|---|---|
| world | the same World supplied to0x2e3510 |
| target | current admitted World-list creature pointer |
| attacker | **null** |
| pre-defence damage | §6 maxHP fraction |
| critical byte | 0 |
| spirit byte | 0 |
| stun chance float | +0.0 |
| argument8, unused by shared hit contract | pointer to object's fixed XYZ |
| push | pointer to §6's three floats |
| hit type | **1 for object0x11, otherwise0** |
| ability | 0 |
| argument12, unused by shared hit contract | null |
| blockable byte | **1** |

Native argument loads at **0x2e5ca9–0x2e5cf5** establish all values, including
the World and null attacker. The caller ignores the return value. The
producer does **not** directly subtract HP, assign stun, synthesize a hit
notice, or initialize one to zeros: shared [creature-hit.md](creature-hit.md)
owns defence, health routing, reaction, random work and its output records.
The trap itself is not disarmed, removed or given another cooldown by this
hit traversal.

## 8. Impact sound, direct CRT order and unconstrained bytes

After each reached hit callback, type0x11 proceeds to the next creature with
**no direct trap-impact sound draw**. Types0x12/0x13 instead copy the target's
then-current physical XYZ, consume **one CRT rand at0x2e5d32**, and queue
**sound1** through **0xa18b0 at0x2e5d76**. This happens irrespective of the
hit callback's return value. Use float **rand×0.3 /32767 +1**, with a rounding
after each multiply/divide/add, volume1 and owner0. The coefficient is
**0x42e704**, word0x3e99999a; see
[steam-sound-event-addenda.md §2](steam-sound-event-addenda.md#2-trap-impact-pitch).

The direct order is **hit callback, then this sound draw**; nested random
work of0x29eb60 runs before the pitch draw. Do not describe the full operation
as exactly one total CRT draw, invent an extra admission chance, or consume a
pitch draw for a filtered target/type0x11. There is no srand or stream
save/restore in this hit tail.

The sound record is0x30 bytes: position+0/+8/+0x10, ID+0x18=1, pitch+0x1c,
volume+0x20=1, owner+0x28=0. **Bytes+0x24..+0x27 are unwritten/unconstrained**.
No general zero initializer is evidenced. Likewise the shared0x48-byte hit
notice's **+0x15..+0x17 and+0x47 padding remain unconstrained** under its
existing producer contract. Any callback particle/other notice follows that
record's own initialized-field contract; this caller adds no blanket clearing.

Direct hit/sound outputs use World+0x90/+0xa0 via the shared callbacks. Their
host lifetime is the existing outgoing-container contract: retained across
the World substep batch until distribution/clear, not cleared per object or
per selected zone. See [steam-world-daily-reset.md](steam-world-daily-reset.md).
There is no direct trap effect-object or terrain-block producer in this tail;
any additional hit/death consequence is the reached callback's behavior.

## 9. Explicit boundary

This closes the selected-zone traversal and reached0x11/0x12/0x13 tail with
its actual clock/personal-state ordering. Generation/interaction/network
sources of the input object state, the other earlier object-family callbacks,
arena-stage work and the separate **0x2a45e0** area pass are not made complete
by this document. Existing shared hit/state/record contracts remain required;
their callbacks must not be replaced by generic damage/sound-only actions.
