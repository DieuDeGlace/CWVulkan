# Steam main-thread CRT stream and particle ownership

Independent native-only clean-room supplement. Sources are the original
main/window loop0x1355a0, Game construction0x7ea80, ordinary frame0xa5a50,
named worker entries, direct imported calls and the existing independent
producer specifications. No game execution, implementation, SDK/runtime
implementation or owner data/captures were inspected. Addresses are RVAs.
This extends [host CRT lifetime](steam-host-crt-stream.md), rather than
replacing its explicit fresh-thread initialization limit.

Later qualified runtime-binary evidence, startup `num`/`slot` admission and
bounded reset coverage are in
[CRT runtime initialization](steam-crt-runtime-initialization.md). Its proven
fresh-record state1 is distinct from this Game's first-frame state.

## 1. State transition and what owns the state

The named paths use the same imported rand/srand pair from
API-MS-WIN-CRT-UTILITY-L1-1-0.DLL. The established32-bit CRT contract in
host CRT §1 applies: a draw replaces unsigned32 state S with
214013×S+2531011 modulo2³², then returns bits16..30 of the new state,
an integer0..32767. An explicit srand replaces S with its unsigned32
argument. The returned15-bit number is not the full next state.

The owner is the executing thread. A Game, World, creature, particle style,
zone, callback object or frame does not select a private copy of this state.
Calls on one thread continue the state left by its preceding imported calls,
including intervening seed calls. The deterministic coordinate selector R
used by generation is distinct: calculating R does not itself draw from this
CRT stream; passing its result to srand explicitly resets the CRT stream.

Here “main thread” means the thread running0x1355a0's ordinary window/Game
loop. It directly constructs the Game through0x7ea80 at0x136331, and calls
frame wrapper0x134420 at0x137b8e. That wrapper directly calls0xa5a50 at
0x134558. These are synchronous calls on the constructing/window thread,
not a handoff to the separate host simulation worker.

## 2. Construction, known seeds and the numeric default boundary

The constructing thread explicitly seeds during both World initializations:

| Construction route | Seed effect |
| --- | --- |
| Game0x7ea80 → host constructor0xf17a0 at0x7eb36 → World initializer0x2b5f20 at0xf1969 | srand123 at0x2b5f9a, then the documented content-initialization draws |
| Later in Game construction,0x2b5f20 at0x7f279 for the separate Game World | srand with Game+0x206c, initialized123 on this route, then the same content-initialization procedure |
| Later crafting rebuild0xb2180 at0x7f74d | Conditional seed effects in §4; reaching the helper alone does not prove its collection gate passed |

Host construction's seed occurs on the constructing thread before its service
worker runs its own content work. It does not initialize that worker's CRT
state. Similarly, stored World public seed+0x220 and derived zone seed+0x244
are content inputs, not saved private CRT state. See host CRT §§2–3.

**Arithmetic reference, not a runtime capture:** apply the established
[World initialization contract](worldgen-terrain-zone.md#1-world-values-and-the-zone-seed-0x2b5f20)
to explicit seed123:64 shuffles of16 entries followed by77 parameter draws.
For this seed the shuffle rejection tests reject no draws. The whole specified
phase consumes1037 draws. The first parameter/derived zone seed is15376;
the last parameter draw is19947. Immediately after that last draw the full
state is **1307250238 (0x4deb0e3e)**; its next draw would return12519 if
no intervening call changes the stream. This labels that precise subphase,
not Game-constructor return or first-frame entry.

The numeric state before the first explicit construction seed is not
established here. The API's documented seed1-sequence behavior for a genuinely
unseeded generator, discussed in host CRT §7, does not prove that condition
at this executable's main entry. Nor has this bounded audit counted every
constructor/helper draw and conditional reseed through first-frame entry.
Consequently neither1,123 nor15376 is asserted as the first ordinary frame's
private state. No wall-clock-derived initial seed is evidenced by these
construction/frame-entry paths.

## 3. Main-frame sharing and relevant phase order

The following reached work uses the same executing main-thread stream:

| Reached phase | Evidence and stream relationship |
| --- | --- |
| Game's client-World batch |0xa5a50 calls0x2da6c0 at0xa7328/0xa734c. These substeps run on the main thread, including the admitted final zero residual; they do not become host-thread calls because the helper is named World update. Only actually admitted mode-dependent child paths consume/reseed. |
| Death/body voxel births | Calls0x90390 at0xa9110 through0xa95c4. The per-voxel gate, delay, spin and insertion draws therefore continue this same main-thread stream. There is no body/style/actor-specific seed or saved-state restoration. |
| Ordinary burst expansion | Later inline expansion in0xa5a50, including the documented0xaa493/0xaa4b7 special draws. Its12 base draws, and style1/5 extra draw where applicable, continue the stream left by earlier reached main-frame work. No per-burst seed is supplied. |
| Later conditional crafting refresh |0xb2180 at0xaad8f can reset the same stream after those births. This is not an unconditional every-frame srand. |
| Particle updater |0xb37f0 at0xaaed9 performs the specified age/motion/retirement work without adding CRT draws. |
| Normal scene and pose sparks |0xe2e00 at0xad9d4 calls world renderer0xd3170 at0xe3bd1, which calls pose0x17ab0 at0xd67b2. These synchronous scene calls remain on the main thread. |

This is a list of ordered relevant cut points, not a claim that nothing else
runs or draws random numbers between them. Existing whole-frame admission and
ordering remain in [gameplay frame order](steam-gameplay-frame-order.md).
The main frame has no imported srand of its own at entry and does not save/
restore the CRT stream at a frame boundary or between its World substeps.

Pose0x17ab0's imported rand sites are0x25792 and0x257ad. They supply the
six pairs used only when the state0x61 spark list is initially empty. See
[pose-item sparks §7](creature-pose-items.md#7-the-sparks-of-state-0x61).
An ordinary spark-list update does not consume those twelve draws again.
There is no direct srand in this pose routine. The normal scene's spark
initialization is later than this frame's body/burst births; its continuation
can therefore affect later same-thread operations and subsequent frames.
It is not an independent particle expander or actor-owned RNG stream.

Pose's other direct render callers,0xdb630 at0xdb7c0 and0xe2e00 at0xe4bfb,
also do not make a new stream merely by selecting a preview/title draw path.
This supplement establishes the normal world chain above; it does not count
every optional preview call or promise one pose call globally per frame.

The burst record's established0x48-byte payload carries positions, velocity,
RGBA, size/count/style/spread, not private CRT state. If its producer ran on
another thread, that can affect its supplied values; it does not transfer
that producer's stream into the main-thread expansion. The same distinction
applies to received death/event data and main-thread body births.

## 4. Explicit reached main-thread reseeds beyond content initialization

These helpers affect whichever thread reaches them; the named Game/UI
callers reach them on the main/window execution path. Their output-list
content and per-producer draws remain in the linked specifications.

| Helper / imported seed site | Proven seed input and gate |
| --- | --- |
| Starter kit0x913b0 /0x913f6 | Low32 of the live local creature's+0x968 value, documented as its Steam account ID after API assignment. The actual owner value remains dynamic. See frontend-characters §5.5. |
| Crafting rebuild0xb2180 /0xb23ce | Only if the local creature's outer inventory begin/end at+0x9f0/+0x9f8 differ. This means it has inventory tabs, not that a nonzero-quantity item was found. Seed with R of wrapped32(regionX+57×regionY+0xf5f9), using the live pair+0x150/+0x154. |
| Same helper /0xb244d | Visits group indices0..4 in increasing order. For each whose0x2b4260 admission is nonzero, reseed with that base plus the group index, in wrapped unsigned32 arithmetic, before the group's recipe draws. |
| Armour stock0x2ac6e0 /0x2ac77b | Low32 shopkeeper ID + day×13 +0x10ec, wrapped32 |
| Weapon stock0x2acf90 /0x2ad02b | Low32 shopkeeper ID + day×13 +0x156b6, wrapped32 |
| Tier/general stock0x2acbd0 /0x2acc06 and0x2acdd0 /0x2ace01 | Low32 shopkeeper ID + day×13, wrapped32 |

R in the crafting rows is the already specified scalar selector0x31a720;
the region pair and group index, rather than the World public seed or elapsed
time, determine those explicit seed values. See
[crafting lists §2](loot-crafting.md#2-the-crafting-lists-0xb2180),
[stock routines §2](loot-shops.md#2-the-four-stock-routines) and
[starting kit](frontend-characters.md#55-the-starting-kit-0x913b0).
These helpers leave their advanced/reset stream in place; there is no
restoration to the incoming state. A helper's seed alone is not its return
state because the reached content draws follow it.

The late0xaad8f call's nearest frame gate requires the resolved creature to
equal World+0x448 and the crafting root at (Game+0x340)+0x178 to pass0x9af00.
Other verified crafting rebuild callers include construction, character load
and ordinary inventory/shop actions; the full event list remains in
[inventory refresh events](steam-inventory-refresh-events.md).
Showing/hiding a broad UI flag is not itself a replacement srand contract.

World-prelude resets are likewise caller-thread operations. The already
verified daily refresh0x31c540 seeds only when its day stamp differs; pet
setup0x2b67b0 seeds with unsigned32 race+0x64 only when that setup is reached.
Their exact seeds/admission and surrounding World-call order remain in
[host CRT §5](steam-host-crt-stream.md#5-known-prelude-seed-producers-in-actual-order).
The daily actor/archive reset is not a restoration of a saved CRT stream.
The named mode-gated prelude routes do not establish a main-thread reseed
merely because the same World-update entry also has main-thread callers.
Other callers of a shared helper affect their own actual executing thread.

## 5. Separate workers and generation context

| Execution context | Native boundary and consequence |
| --- | --- |
| Host simulation | Constructor launch0x75d10 at0x7f879 reaches0x83e60 →0xf5520. Its World batches, their resident/trap/pet randomness and later host interpolation run on that worker's stream, separately from the main frame. See host CRT §§3.2–5. |
| AI | Launch0x75e80 at0x7f916 uses callback table0x437ee8; reached wrapper0x88f80 calls0x83f20 at0x88ff2. That loop calls0x13830 on Game+0x768. Its rand calls belong to the AI worker, not main/world-render draws. The direct loop/wrapper has no srand; exact incoming numeric state and all possible child reseeds remain unestablished. |
| Host service/generation |0xf17a0 launches its separate service worker0xf2340. It calls0x2ae0c0 at0xf2cbe, affecting that worker's stream. See host CRT §3.1. |
| Client generation coordinator | Launch0x762d0 at0x7f80f, callback table0x437eb8 →0x87bf0 →0x89160, reaches0x84f70 at0x891d2. This is another worker, not the main frame. It creates a PPL scheduler with concurrency limits1..4 and invokes generation dispatch0x7d5c0, including delegated work. |
| Mesh coordinator | Launch0x76160 at0x7f7c2, table0x437ea8 →0x87be0 →0x890c0, reaches0x842d0 at0x89132. This separate worker is not evidence that ordinary actor pose/particle births execute there. |

The Game-pointer payloads do not contain a copied CRT state. Synchronizing or
publishing World records does not merge these threads' RNG states. The
fresh-thread numeric-state limit is the same one retained in host CRT §7;
constructor seed123 is not a proven worker-entry seed.

For an actual new-zone call,0x2ae0c0 explicitly seeds at0x2ae13d with
wrapped32(zoneY×1024+World+0x244+zoneX). An already loaded nonnull zone
returns before this seed. This affects the thread actually executing that
call, including generation's delegated work; it is not a state attached to
the resulting Zone. Dispatch0x7d5c0's direct call is0x7d618, while0x858e0
also reaches0x2ae0c0 at0x85994/0x85c27. This supplement does not assign a
stable runtime worker ID or private per-task stream to every PPL job.

## 6. Exact remaining bounds

This establishes the main-thread sharing and named explicit reset routes;
it is not an exhaustive all-game rand/srand consumer audit. In particular,
numeric main-entry/first-frame state, all optional UI/callback draw counts,
all later AI-child reseeds and exact delegated-task worker assignment remain
untraced. Hash/noise selectors and other locally seeded generators are
separate from these imported CRT calls. Unknown initial states are not
replaced here by clock, frame-count, World-pointer, actor-ID or seed1 guesses.
