# Steam dead-creature areas, block destruction and retirement

Independent native-only behavioral supplement for **0x2a45e0**. RVAs refer
to the original Steam executable. No implementation, owner exports or game
execution were used. This specifies its reached ordinary finite authority
path, including its deferred retirement; the separate static-object trap
pass is [steam-world-traps.md](steam-world-traps.md).

## 1. Admission, source order and inputs

The World substep calls this function at **0x2da8f3**, after **0x2e17d0** and
**0x2a56c0**, inside the **World+0x358 byte=0** authority branch. Pet prelude
**0x2e0db0** follows it. Its second argument is the same **float32 elapsed
milliseconds** supplied to this substep. There is no positive-elapsed guard
inside this function; an admitted zero substep still reaches it.

It walks **World+0x40's registration list forward**, including local and guest
players in that traversal. Work is admitted only when the entry's current
**float HP+0x180<=0 and kind byte+0x60!=0**. Kind0 entries receive none of the
death-clock, burst, area or retirement work below. The list is not sorted or
pre-collected by HP: each entry's HP is tested when its visit is reached.
Next links are read after each source's work. Its own queued removals occur
only after the full source traversal, not during the source/target loops.
Thus a later registered creature killed by an earlier source can reach its
own death branch in this same pass; an already visited creature waits for a
later pass. This is not a snapshot of the initially dead creatures.

For each admitted dead source, first insert its signed64 **ID+0x08** into
**World+0x20's dead-ID set**, through **0x37a40 at0x2a4749**. Re-inserting an
existing ID does not append a second entry. This precedes all bursts and
target selection. No source-range, renderer, frustum, loaded-zone window,
or owner-resolution admission replaces the HP/kind predicate.

Other source inputs are physical signed64 fixed XYZ at **+0x10/+0x18/+0x20**
(65536 units/block), signed32 race at **+0x64**, float clock **+0x6c**,
float fields **+0x184/+0x188**, signed64 owner/link ID **+0x1a0**, and the
remembered target-ID set at **+0x9b8**. Size inputs below are raw float
**+0x84** and **+0x8c**. This document does not assign new defaults or infer
the producers of those inputs.

## 2. First-clock-zero burst

Only when the source's clock **+0x6c is exactly0.0f**, and its race is
**0x90,0x119 or0x11d**, enter the area branch (**0x2a4752–0x2a4788**).
This is tested before adding this substep's elapsed time. Other dead races
skip directly to §6. A nonzero clock does not repeat this branch merely
because another area or target is nearby.

First append one **0x48-byte particle-burst record to World+0xb0**, through
**0x36ff0 at0x2a4836**, before any block destruction or victim visits:

| Field | All three races | Race0x11d | Races0x90/0x119 |
|---|---|---|---|
| Position+0x00..0x17 | source physical fixed XYZ | same | same |
| Velocity+0x18..0x20 | float(0,0,20) | same | same |
| RGB+0x24..0x2c | — | float(0,0.5,1) | float(1,0.2,0.5) |
| Size+0x34 | float0.25 | same | same |
| Count+0x38 | signed32 20 | same | same |
| Style+0x3c | — | signed32 2 | signed32 1 |
| Spread+0x40 | float3 | same | same |

**The first burst's alpha+0x30 is not initialized to a specified value.**
At **0x2a47bc–0x2a47c4**, the producer reads four bytes from the unassigned
padding of the preceding dead-ID insertion result and copies them into
that field. The insertion helper writes an iterator pointer and a byte
success flag, not those padding bytes. No writer to this source float exists
in the function. Record padding **+0x44..+0x47** is likewise not initialized.
Do not compare these as deterministic initialized producer data, claim
alpha=1, or declare alpha unused from this producer trace. The bounded
ordinary consumer's storage and draw behavior are now established in§2.1.

This burst construction has no direct CRT random draw. Expansion of a burst
by later particle consumers is a separate phase.

### 2.1 Native consumer: stored alpha versus submitted color

The ordinary Game-frame burst expander in **0xa5a50** copies each World+0xb0
record through **0x7fb50 at0xaa1fb**. It then copies **all four color floats**,
burst offsets **+0x24/+0x28/+0x2c/+0x30**, into a newly constructed live
particle's **+0x44/+0x48/+0x4c/+0x50**, through **0x17170 at0xaa460**.
The copy helper is16 bytes, not a three-component RGB copy. Queue insertion
through **0xa21a0 at0xaa505**, **0x6e710** and **0x7f9e0** preserves that
live-particle color. Thus the burst's undefined fourth component can reach
stored particle state; it is not initialized by the copy or queue.

For **style1**, expansion subsequently replaces the entire color through
**0x16ff0 at0xaa493**, then **0x17170 at0xaa4a2**. The replacement's fourth
color input is **float1**, supplied at **0xaa47d** as the fifth ABI argument
(including the destination pointer); its source is
the float1 constant at0x4ac980, loaded at0xa7076. Consequently the first
bursts of races0x90/0x119 do not retain their undefined burst alpha even in
the final queued particle color. This is a native style-specific replacement,
not permission to initialize the outgoing burst field to1.

For **style2**, used by race0x11d's first burst, that replacement is skipped:
the stored live-particle alpha remains the copied indeterminate value.
The reached particle updater **0xb37f0**, called at **0xaaed9**, uses particle
style, age, lifetime, motion, collision and related flags; it does not read
that color-alpha field for those decisions or transfer it into geometry,
RGB, lifetime or another control. It does not turn these particles into
another style. This updater therefore introduces no visible dependency on
the indeterminate fourth component.

The ordinary world render entry **0xe2e00 calls0xe6840 at0xe3ca7** with the
live particle collection. A particle's node includes a16-byte list header,
so live color-alpha **+0x50** is node offset **+0x60** in this draw routine.
At **0xe69b3–0xe6a27**, the draw initially reads it and computes a candidate
fourth material component using frame color/light factors. However,
**0xe6a2c–0xe6a38 explicitly replaces that result with float1 for both
style1 and style2**, before color submission through virtual slot+0x98 at
**0xe6b1b**. Its RGB equations are independent component products; the
fourth component is not mixed into RGB. Admission, resource selection and
geometry likewise do not use that stored color-alpha field.

Therefore the normal reached draw of these actual first-burst styles submits
**material fourth component exactly1**, independently of the outgoing
burst's indeterminate alpha. Style2's raw stored alpha remains unconstrained,
but it does not determine visible particle color/opacity or geometry on this
consumer route. The initial read/multiply still exists; this is a replacement
before submission, not a claim that native execution never reads the bytes.

Do not generalize this result to every particle style: the same draw predicate
retains its computed fourth component for **styles0,4,5,6**. The verified
comparison distinction is between an **uninitialized outgoing burst field**,
the **style-specific stored particle field**, and the **exact1 submitted
component for styles1/2**. No deterministic producer alpha or generic
all-particle alpha fallback follows from this trace.

## 3. Race0x90 terrain destruction

Only race **0x90** enters the terrain loop after the first burst. The other
two races go directly to §4. Visit integer offsets **−8 through+8 inclusive**,
with **X outermost, Y middle, Z innermost**, all ascending. Sample fixed XYZ
is source physical XYZ plus each offset times65536, using low64 additions.

For the sample-radius test, subtract source fixed XYZ using low64 differences;
convert each signed64 difference to float32, multiply each by float32
**1/65536 (word0x37800000 at0x42e510)**, square separately, and add X²+Y²
then Z² in float32. Admit only **strictly<64.0f** (word0x42800000 at0x42e5b4).
This is a strict eight-block sphere of sample offsets, not the whole17³ cube.

Read the six-byte block through **0xe9a0 at0x2a4a05**. Fixed-to-block conversion
is toward-zero signed64 division by65536, narrowing to signed32, followed by
the original-negative/reconstruction-mismatch decrement, independently for
XYZ. It has the same narrowing-before-negative-correction behavior described
in [trap§2](steam-world-traps.md#2-verified-coordinate-conversion-including-out-of-range-negative-inputs), without that
section's final cell/zone division. The block reader's existing missing-zone
default applies; this pass does not load zones to satisfy the query.

Admit a returned block solely when **six-byte block flag byte5 &0x11 is
nonzero**. There is no additional type, RGB, collision-solidity or HP test
for a block. The byte layout is RGB at0..2, unused byte3, type byte4, flags
byte5. For each admitted block, perform these operations in order:

1. Append a debris burst to **World+0xb0**, through **0x36ff0 at0x2a4abb**.
   Position is the raw sample fixed XYZ plus **32768 on each axis**, not
   the queried integer block center reconstructed from its narrowed index.
   The offset is obtained by truncating the double constant32768 at0x42e588
   to signed64 and adding it in low64 arithmetic. Velocity=(0,0,10), size=0.5,
   count=3, style=5, spread=3. RGB is each returned raw block byte converted
   to float32 and multiplied by **1/128**, word0x3c000000 at0x4386e0; alpha=1.
   There is no255 division or RGB clamp: byte255 becomes1.9921875. The
   record's final four padding bytes remain uninitialized.
2. Call **0x2a59f0 at0x2a4afe** with the unshifted raw sample fixed XYZ.
   The shared mutation helper writes water below integer block Z1, otherwise
   empty, provided the zone and stored column extent exist. Its contract is
   in [effect-objects§2.7](effect-objects.md#27-blocks).
3. Append one **0x20-byte block-change record to World+0xc0**, through
   **0x36db0 at0x2a4c14**. Coordinates+0/+4/+8 are the narrowed corrected
   signed32 block XYZ; +0x0c..+0x11 copy the six returned before-bytes;
   +0x12..+0x17 copy the global block at **0x5534a8** (startup empty
   ff ff ff 00 00 00); +0x18 copies World's signed32 day+0x08.
   **+0x1c..+0x1f are unwritten.** The advertised after-bytes are this global
   empty block even where the actual mutation helper wrote water below Z1.
4. Set the per-source accepted-block flag. It is not a mutation-success
   return value or a count of distinct final changes.

After all sample visits, if that flag is set, emit exactly one sound at the
source's then-current physical position: id1, volume1, owner field+0x28=0.
One direct **CRT rand at0x2a4ccc** supplies pitch: convert to float32, multiply
by float32 **0.4 (word0x3ecccccd at0x42e52c)**, divide by32767.0f, then
add0.5f, with separate float32 rounding. Append through **0x370f0 at0x2a4d12**.
Sound padding+0x24..+0x27 is not assigned as a sound field.

Then call **0x2e0710 at0x2a4db2** for the rectangle from source X/Y−8 blocks
to source X/Y+8 blocks. Each fixed offset is added in low64 first; divide
signed64 by65536 toward zero and take low32 for each argument. There is
**no negative fractional floor correction** in these four producer arguments.
The shared helper's entry/support/flag conditions apply; calling it does
not imply unconditional removal of all scenery in that rectangle. Neither
the sound nor this cleanup call occurs when no block passed the flag test.

## 4. Shared first-visit target selection and push

After the optional race0x90 terrain work, all three admitted races visit
the current **World+0x40 registration list forward**, beginning again at its
head (**0x2a4dbb onward**). No target vector, distance sort or frustum list
is substituted. Reject the source itself; then require **0x50550(source,
candidate) nonzero**, candidate **HP>0**, and candidate **float+0x12c exactly0**.
The hostility helper is independently specified in skills-and-interactions§0.
Reject an ID already present in the source's **+0x9b8 remembered-ID set**.

The spatial gate uses physical fixed positions, without drawn offsets or LOS.
For horizontal differences, subtract source−candidate in low64 signed
arithmetic, convert each signed64 result to double, and multiply by double
1/65536 (0x43ef58). Let the float radius be candidate size+0x84 multiplied
by **float0.7 (word0x3f333333 at0x4328ac)**, then add **8.0f**, with separate
float32 operations. Reject if either absolute double difference exceeds
the radius widened to double, or if double X²+Y² exceeds the float32
radius² widened to double. Horizontal equality is admitted.

For vertical separation, convert the low64 signed Z difference to float32,
multiply by float32 1/65536, and take its absolute value. The bound is,
in float32 order: candidate size+0x8c times0.5, plus source size+0x8c,
plus8. Reject **absolute Z>=bound**; vertical equality is excluded.

For an admitted new target, build horizontal push from **candidate−source**
fixed X/Y differences, each signed64→float32 then times float32 1/65536.
Square separately and add X²+Y², then+0, in float32. Only if that sum is
**strictly>float0.01** (word0x3c23d70a at0x42e51c), call sqrtf and multiply
each component by the separately rounded float32 quotient1/sqrtf(sum).
Otherwise retain the unnormalized components. Multiply each horizontal
component by10.0f; push Z=2.5f. This deliberately leaves a small nonzero
horizontal distance unnormalized; it is not a zero-only test.

Next consume one direct **CRT rand at0x2a4fdb**. Critical is true exactly
when signed rand remainder4 is0. Take damage from source **float+0x184**;
when critical, double it by a float32 self-add. Then insert the candidate
ID into the source's remembered-ID set, through **0x37a40 at0x2a5049**.
This occurs **before** the kind8 rejection and the water-bubble owner/damage
checks below. Such later-rejected targets still consumed this draw and
remain remembered. There is no retry or undo of that insertion here.

## 5. Damage versus water-bubble healing

Reject a candidate whose kind is8 after the draw/insertion above.

For source race **0x90 or0x119**, call the shared hit helper **0x29eb60
at0x2a5274**, using source as attacker and the critical-adjusted damage
directly; there is no producer-side positivity gate for this damage.
**Hit type is1**, written at **0x2a529f**, not2. The shared hit consumer retains responsibility for
defense, rejection, health changes, secondary random draws and outputs.

For source race **0x11d**, first multiply the critical-adjusted damage by
source float **+0x188**, in float32, and require the result strictly>0.
Resolve source ID+0x1a0 through **0xe720 at0x2a508a**. Missing owner skips
the target after it was already remembered. When **0x50550(owner,candidate)
is nonzero**, call the same hit helper with **owner as attacker and hit
type2**, written at **0x2a5230**. Otherwise require candidate kind!=7 and HP still>0, then use the
healing-notice path below. Kind7 on this friendly branch receives neither
damage nor healing.

Both actual hit routes pass the same remaining arguments, in the shared
13-argument contract from creature-hit§1.1: same World; selected candidate;
attacker as above; final damage as above; critical byte; **spirit flag1**;
**stun chance+0.0f**; source physical XYZ pointer in the unused position
argument; the push from§4; hit type1/2; ability0; null unused pointer;
**blockable byte1**. The hit return is not used to undo remembered state.

The friendly race0x11d healing path constructs a normal0x48 hit record:
attacker ID=source+0x1a0, target ID=candidate+0x08, damage=the positive
computed amount with its float sign bit toggled, critical as above,
stun/timer fields=0, number position=candidate physical XYZ, push=(0,0,0),
spirit/type/ability bytes at+0x44/+0x45/+0x46=0. Padding **+0x15..+0x17
and+0x47** is not initialized. Append it to **World+0x90 at0x2a5146**.

For this ordinary authority admission, World+0x358=0 also admits the next
branch: append the **same record a second time** to World+0x90 through
**0x36e00 at0x2a51c1**, then call **0x29dcf0 at0x2a5202** to apply it.
The two appends precede the single apply call. Do not collapse them into
one notification or infer a second health application. The function also
has a remote-client/local-owner condition for the second append; direct
client invocation is outside the ordinary caller admission specified here.

## 6. Clock advance, owned-kind6 revival and queued retirement

After all admitted area targets, or immediately for an admitted dead source
that did not enter the area branch, add elapsed milliseconds to its current
**float+0x6c**, storing float32 (**0x2a5308–0x2a5313**). There is no whole-ms
conversion, clamp or clock reset here. It is after the first-clock-zero
test, first burst, terrain changes and target actions. In a zero substep
the clock can remain0 and a surviving source can repeat that first-visit
branch next time; its remembered target set still filters prior targets.

When **kind!=6 or source+0x1a0=0**, the source is queued for retirement once
the updated clock is **>=1000.0f**, word0x447a0000 at0x42e5d4. Append its
borrowed pointer to a temporary list through **0x36c20 at0x2a549c**. Before
continuing the outer source walk, visit all current World+0x40 creatures
forward and erase this source's ID from their **+0xb88 and+0xb98 ID-keyed
containers**, through **0xf180 at0x2a54ee/0x2a54fd**. It does not sort,
remove their remembered+0x9b8 entries, or destroy the queued source yet.

The alternative **kind6 with nonzero+0x1a0** does not fall back to that
1000-ms retirement. Require a nonnull entry for that ID in World+0x450,
updated clock **>=20000.0f** (word0x469c4000 at0x42e5ec), and the resolved
owner's **HP>=0**, including exactly0. Missing owner, null entry or negative
owner HP leaves this corpse registered; there is no timeout deletion in
this branch. When admitted, set source HP to **0x5fac0(source)** at0x2a53e8,
copy owner physical fixed XYZ to source, write signed64 zero to source
**+0x9e0**, and clear source **+0xb88** through **0xb810 at0x2a5472**.
The function does **not** reset clock+0x6c, clear+0xb98/+0x9b8, erase its
dead-ID entry, or copy owner angles/drawn XYZ in this revival branch.
Those unperformed operations must not be added as an inferred respawn.

Only after the outer World+0x40 traversal completes, walk the temporary
retirement list in its append order. For each borrowed pointer, find the
first matching registration-list node, unlink it if found and decrement
**World+0x48**; erase its ID from **World+0x450** through **0x2ac530 at0x2a55f2**;
then invoke the creature's virtual destructor with delete flag1 at0x2a5602.
The producer does not erase the persistent World+0x20 dead-ID set or rebuild
World+0x50 here. Finally free the temporary list nodes/head.

## 7. Output/RNG lifetime and explicit limits

The explicit CRT draws in this helper are the one per accepted-block source
sound, followed by one per newly admitted spatial target, in the visit
order above. No direct srand, per-source seed or stream restoration occurs.
Shared hostility/hit/heal and later particle consumers remain their own
contracts; the direct draw count is not an assertion that all callbacks
consume no random values. In particular, each actual hit callback can
advance the stream before the next target's direct critical draw.

World+0x90 hits, +0xa0 sounds, +0xb0 bursts and+0xc0 block notices are
appended to the existing outgoing lists. They are not cleared per source,
zone or substep here. Host distribution/whole outgoing-container clearing
is specified in [steam-world-daily-reset.md](steam-world-daily-reset.md).

This closes the bounded finite ordinary body and its direct writes. It does
not specify new race/clock/damage-field producers, revise the independently
specified hostility/hit/heal/terrain-helper internals, establish an initialized
value for first-burst alpha (distinct from§2.1's draw replacement), or authorize replacing deletion with HP-only
filtering. Standalone invocation under a remote-client World, exceptional
allocation failures and mutations by untraced external callbacks are outside
this ordinary authority contract.
