# Steam transient scene-light producers and lifetime

Independent clean-room specification from the original native executable,
its assembly/call references and constants. No engine, tests, exporter or
third-party reconstruction was inspected; no original game was executed.
RVAs use the supported native analysis image. World positions below are
signed 64-bit fixed coordinates, 65536 units per block. This completes the input
side of `steam-scene-lighting.md` §3.6.

## 1. Persistent list, record and scene handoff

The caller-supplied list is **Game+0x20a0**, with its unsigned 64-bit element count
at **Game+0x20a8**. It persists across frames; the scene does not rebuild or
clear it after each collection. The payload is 48 bytes, after a 16-byte linked-
list node header:

| Payload offset | Type / meaning |
| --- | --- |
|+0,+8,+0x10|Three signed 64-bit fixed world-position components|
|+0x18,+0x1c,+0x20|Float32 RGB|
|+0x24|Float32 radius, blocks|
|+0x28|Signed 32-bit elapsed milliseconds|
|+0x2c|Signed 32-bit lifetime milliseconds|

Append helper **0xa1840** uses record-copy constructor **0x6e650**, which
copies all these fields, appends at the tail and increments the element
count. One hit branch performs the same append directly. No merging by
position, attacker or color occurs in these appends.

At **0xad8e4**, frame 0xa5a50 obtains the address of Game+0x20a0, and at
**0xad966** places it in **argument 16** of scene 0xe2e00, called at 0xad9d4.
The scene retains that supplied address and forwards it to light collector
**0xce5f0 at 0xe3bdd**, after the zone/model collection 0xd3170. This is the
list identified as the frame-transient list in the existing collector spec;
it is distinct from **Game+0x2090**, passed separately for rendered particles.

## 2. Hit flashes: source records and common gate

**0xa0750**, called from the frame at **0xaadbd**, walks the world's damage-
number list **world+0x90** in list order. Use the 0x48-byte record described in
`creature-hit.md` §0.3. Both light branches below require the event's **target
ID+8** to resolve in world+0x450 to a non-null creature. Each branch also
requires the **attacker ID+0** to resolve to a non-null creature.

The light point in both branches is copied directly from the damage record's
**fixed position+0x20/+0x28/+0x30**, with no height offset, origin adjustment,
float position conversion or replacement by the target's current position.
These light branches have **no additional positive-damage, hit-kind,
critical-byte, HP, local-player, day, distance or frustum gate**. Other nearby
statistics/taming/damage-text branches in 0xa0750 have their own conditions;
their conditions must not be incorrectly imposed on these light appends.

### 2.1 Spirit-flag flashes

Require the damage record's **byte+0x44 nonzero**. This is the spirit flag,
not its critical byte+0x14. Obtain the attacker's **current in-use weapon
list through 0x4fd40**, in the order specified by `stat-rules.md` §3.1.
This selector uses the attacker's state at display consumption; the damage
record does not carry a copied weapon list.

For **each selected weapon separately**, call **0x109330** to obtain RGBA.
Only when its **W component is strictly positive**, append one light with:

- RGB equal to that result's RGB, without multiplying by W or damage.
- Radius **20 blocks**, elapsed **0 ms**, lifetime **400 ms**.
- The unshifted damage-record fixed position described above.

The color function and its ordered average over spirit-material entries are
already specified in `steam-scene-lighting.md` §3.10. It can yield RGB above 1;
do not clamp it to byte colors or average the two weapons together. Weapons
whose result W is not positive do not append. There is one light per admitted
weapon, not one per individual spirit entry.

Sources: weapon selection **0xa0c37**, color call **0xa0c5b**, initialization
**0xa0c71–0xa0ccd**, and direct node append **0xa0cd7–0xa0d22**. The combined
radius/elapsed store at 0xa0c92 initializes both radius 20 and elapsed 0.

### 2.2 Ability-flag flash

Independently require the damage record's **byte+0x46 nonzero**, whose native
hit producer sets it when the attacking ability is **0x1d**. After successful
attacker resolution, append one light with **RGB (0.3,0.3,0.5)**, radius
**16 blocks**, elapsed **0 ms**, lifetime **400 ms**, at the same unshifted
damage-record position. The0.3 values are the stored float32 value
**0.300000011920928955078125**, word 0x3e99999a.

This branch follows the spirit-flag branch and does not require that flag.
When both flags qualify, weapon-color lights append first, then this light.
Sources: flag test **0xa0d8f**, through append **0xa0e56**. Its radius/elapsed
initialization likewise writes elapsed 0. Neither hit-light branch draws RNG.

## 3. Race 0x90 death-effects flash

This is a **death-effects** source, not a continuously emitting light on
every living creature of that race. In frame 0xa5a50, the received-hit pass
collects a deduplicated temporary set of creatures needing death effects:
resolved hit targets with **HP≤0 and race not 0x11d**, and a local-player
health subtraction that becomes **strictly negative**. These are the existing
death-collection rules in `remaining-runtime-gaps.md` §1. The light-producing
pass is **0xa9030–0xa963a**. The temporary set deduplicates by the creature
pointer and visits ascending **unsigned pointer values**, not hit-arrival or
creature-ID order: insertion 0x7d330 uses0x742e0's unsigned comparisons, and
iteration advances through 0x282660. This pointer order can vary across runs;
a runtime oracle must retain the observed source order when comparing ties.

For a candidate, require creature **byte+0x1500 nonzero**. Then call
**0xe08c0 at 0xa9084** at the creature's **drawn fixed position
+0xb10/+0xb18/+0xb20**, with sphere radius
**0.5×max(body extents+0x84,+0x88,+0x8c)** and fixed render range
**100 blocks** (float word 0x42c80000 at 0x42e5bc). This tests the renderer state
present at that producer call, before the later camera/scene setup. It is
not a substitution of the eventual scene's adaptive range or final planes.

After that gate, the frame emits its body/attachment model fragments through
0x90390. **Only when the candidate's signed 32-bit race+0x64 equals0x90**, it then
appends one explicit light with:

- Position equal to the same **drawn fixed position**, with no height offset.
- RGB **(1,0.8,0.5)**, where 0.8 is float32
  **0.800000011920928955078125**, word 0x3f4ccccd at 0x42e530.
- Radius **16 blocks**, elapsed **0 ms**, lifetime **800 ms**.

Sources: race test **0xa95c9**, position/color/radius initialization
**0xa95d2–0xa9611**, elapsed/lifetime stores **0xa9616/0xa961d**, and append
**0xa9635**. It is one light per admitted death candidate, after the fragment
calls, rather than one light per fragment or sampled particle. The light
creation adds no RNG draw, and does not require a nonzero fragment count.

## 4. Age, removal and first-frame ordering

All identified births in §§2–3 occur **before** the frame calls lifetime
updater **0xb36a0 at 0xaaee3**. Its step is a **signed 32-bit integer conversion
toward zero of the capped double frame milliseconds**, performed at
**0xaaed0**. This is the wall-frame interval capped at 1000 ms described in
`steam-scene-lighting.md` §5.1, not world time-of-day or a world substep and
not the already rounded renderer float argument. Each call discards the
fractional millisecond; this updater has no fractional carry accumulator.

0xb36a0 traverses the entire persistent light list in order. It adds the
integer step directly to every record's **signed 32-bit elapsed+0x28**, with 32-bit
wraparound. Only when the updated elapsed is **strictly greater than** that
record's signed 32-bit lifetime+0x2c does it collect the node for deletion.
The increment/comparison are **0xb3703–0xb370c**. It finishes this scan first,
then unlinks/frees the collected nodes in traversal order and decrements the
list count. Equality survives: age 400/lifetime 400 remains present, while
age 401/lifetime 400 is removed. No movement, color mutation, radius mutation,
clock reset, upper age cap or source-creature lookup occurs in this update.

Thus newly created age 0 lights already receive the **current frame's integer
step before their first scene collection**. For example, a 400-ms hit light
created during a 1000-ms capped resume frame is deleted before collection.
A normal 16-ms frame first collects it at age 16. The light's fixed position is
a snapshot and does not follow a later-moving creature or particle.

The scene then applies the existing §3.6 visibility test and linear lifetime
fade **max(0,1−float(elapsed)/float(lifetime))** while collecting survivors.
Age equal to lifetime therefore survives storage but contributes zero RGB;
the collector does not add a special zero-color omission for this list.
These producers initialize positive lifetimes 400/800, so no new zero-lifetime
convention is needed to wire them.

## 5. Source-family boundary

The identified appends to this list are exactly the two hit branches in
0xa0750 and the race 0x90 death-effects append in 0xa5a50. The lifetime and
scene-handoff paths only update/read it. These are local **visual responses
to existing hit/death data**, not a new network light packet or a second
simulation-side damage effect. The particle list is separate: neither
rendering an arbitrary particle nor consuming a world particle-burst record
automatically creates a transient light under this contract.

The separate direct collector of live world projectile/effect kinds 3/4
remains `steam-scene-lighting.md` §3.6's first source. It must not be replaced
by, or counted a second time as, the persistent transient list specified here.
