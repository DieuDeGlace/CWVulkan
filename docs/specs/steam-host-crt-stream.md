# Steam host CRT initialization and thread lifetime

Stable bounded native audit, 2026-10-31. Sources are original host/Game
construction, native thread launch/entry instructions, **0x83e60 → 0xf5520**,
and the specified World time/resident/pet prelude. No game execution or
external runtime/SDK implementation was inspected. This is not an exhaustive
audit of all game random consumers or all later virtual callbacks.

## 1. Generator identity and scope

The reached **rand/srand imports are API-MS-WIN-CRT-UTILITY-L1-1-0.DLL**.
Their established generator contract is the executing thread's persistent
32-bit CRT stream, as in [worldgen-creatures.md §0.1](worldgen-creatures.md)
and [conversation-script.md §2.5](conversation-script.md#25-the-c-runtimes-rand):
each draw advances the unsigned32 state by multiplier214013 and increment
2531011 modulo2³², then returns bits16..30. An explicit srand replaces that
state. State is not selected by the World pointer supplied to a game helper.

Distinguish three verified execution contexts below: the constructing thread,
the host's service/zone-generation worker, and the separate host simulation
worker. Shared World content does not itself identify which CRT stream a
call changes.

## 2. Host and embedded World construction

In the verified Game construction route **0x7ea80**, the host is at
**Game+0x6e8**. **0xf17a0 at 0x7eb36** constructs it; its embedded World begins
at **host+8**, distinct from the Game's separate World pointer at Game+8.
The World constructor is **0x29a130 at 0xf17e1**.

There is one real shared constructor argument worth preserving precisely:
the Game caller supplies the 64-bit value at its existing **World+0x70**;
0xf17a0 forwards that incoming second argument to0x29a130, which stores it
at the embedded **World+0x70**. The field's broader semantics are not newly
identified here. This establishes that one value copy, not a whole-World
copy or an evidenced CRT-state handoff.

Still on the **constructing thread**, **0xf17a0 calls0x2b5f20 at 0xf1969**
with public seed **123** and its constructor-supplied world name. At
**0x2b5f9a**, that initializer explicitly calls **srand(publicSeed)**. Its
content initialization, fully specified in
[worldgen-terrain-zone.md §1](worldgen-terrain-zone.md#1-world-values-and-the-zone-seed-0x2b5f20),
then shuffles64 town-layout lists and makes77 more draws for stored World
parameters. In particular:

* **World+0x220** stores the public seed.
* **World+0x244** is the derived zone-generation seed.
* The other documented scalar/layout fields contain derived content inputs.

Those stored values are not a snapshot of the CRT's current private state
after initialization. For example, the existing seed123 content contract
gives derived zone seed15376; neither123 nor15376 establishes the future
simulation worker's initial stream.

Later within the same Game constructor, **0x7f279** calls0x2b5f20 for the
separate Game World using Game+0x206c, initialized to123 in this route.
This is another explicit seed on the **constructing thread**. It does not
demonstrate a seed on a worker that will be launched later.

The World constructor itself has no direct rand/srand call in its resolved
native call list. Its other constructor helpers are not used here to infer
an otherwise unknown worker seed; the explicit world initializer above is
the established content-seed producer.

## 3. Two distinct worker launches

### 3.1 Service/zone-generation worker created by the host constructor

After embedded-World initialization, **0xf17a0 at 0xf19da** uses **0x7d560**
to allocate an **8-byte payload holding the Host pointer**. It invokes the
thread launcher **0xf0150 at 0xf19e8**, whose **0xf0210** call uses imported
**MSVCP140.DLL::_Thrd_start**.

The native startup wrapper goes through **0x3c7d0**, the callback thunk
**0xf3070 → 0xf3220**, and its mutex/condition handshake, then calls
**0xf2340 at 0xf3292**. That is a different worker from the simulation route
below. On the service worker, requested-zone processing calls
**0x2ae0c0(embedded World, zoneX, zoneY) at 0xf2cbe**.

A **new** zone's0x2ae0c0 path explicitly seeds that worker's CRT stream with
wrapped32 **zoneY×1024 + World+0x244 + zoneX**, then generates the zone under
the existing generation contract. An already loaded nonnull zone returns
before that seed. See worldgen-terrain-zone §2 and
[steam-zone-prerequisite-indices.md §5](steam-zone-prerequisite-indices.md#5-loaded-zone-lifetime-and-absence-of-automatic-rebuilding).

These operations change the **service worker's** stream. They can publish
shared zone content, but there is no game-side transfer of its final CRT
state into the separate simulation worker identified here.

### 3.2 Simulation worker that runs0xf5520

The Game constructor separately packages an **8-byte Game pointer** through
**0x7d560 at 0x7f86c**, then launches **0x75d10 at 0x7f879**. That launcher
calls imported **_Thrd_start at 0x75dd0** with **0x3c7d0** as the native
startup entry. Its native callback-table entry at **0x437ec8** resolves
through thunk **0x87bb0 → 0x88ee0**.

**0x88ee0** moves the payload pointer, signals the startup condition under
its mutex, and calls **0x83e60 at 0x88f52**. The payload contains only the
Game pointer. The inspected packaging, launch, handshake and0x83e60 entry
contain **no game-side srand, current CRT-state read, or RNG-state copy**.
The embedded World remains shared by address; no new World is constructed
by this entry.

The external _Thrd_start/CRT thread initialization is outside the supplied
game implementation examined here. A numeric fresh-thread CRT state is
therefore **unverified**. The conversation-script example “after srand(1)”
is a test of an **explicit seed**, not evidence that this worker starts at1.
Do not infer worker seed123 from host construction, a per-zone seed from
service-worker generation, or a seed from the wall-clock read below.

## 4. Loop admission, continuation and World substeps

**0x83e60** reads timeGetTime at **0x83e6d**, continues while
**Game+0x2068 is nonzero**, and calls the tick only on iterations where
**0x3e2d0(Game+0xd18) returns0**. The semantic meaning of that gate is not
newly identified here. Its alternate branch sleeps; it is not an evidenced
seed/reset operation.

On an admitted iteration, the DWORD clock difference is passed in EDX to
**0xf5520(Game+0x6e8) at 0x83ec6**. These timestamps determine the interval;
there is no srand(timeGetTime) on this entry path. The host tick retains the
integer-ms cap/splitting already specified in
[steam-gameplay-camera.md §7.2.3](steam-gameplay-camera.md) and
[steam-world-daily-reset.md](steam-world-daily-reset.md): successive16-ms
World calls, then a final residual call. An admitted **0-ms host interval**
still reaches that final World call.

**0xf5520 has no direct rand/srand call.** Its child operations can consume
or reset the executing thread's stream. The native body does not save and
restore CRT state at the tick boundary or between World substeps. Thus the
stream entering a later substep is the continuation left by earlier reached
operations on this thread, unless an explicit seed producer intervenes.
The once-per-host-tick drawn-position pass does not establish a fresh stream.

On leaving the run loop,0x83e60 calls **0xf4ff0 at 0x83f10** before its
wrapper returns. There is no game-side CRT-state export back into the World
or constructing thread on the inspected entry/exit route. This audit does
not specify the external runtime's thread teardown or subsequent fresh-thread
initial numeric state.

## 5. Known prelude seed producers, in actual order

Each admitted **0x2da6c0** World call follows this relevant order:

| Phase and caller RVA | Established stream effect |
|---|---|
| **0x2e2e80 at 0x2da728**, then0x2adfe0 at0x2da730 | Time/day and auxiliary work. No direct CRT seed in these bodies. The specified midnight and queued-kind0 actor/archive reset pairs do not save/restore a CRT stream. Resetting World records is not an initial-stream reload. |
| **0x2e3510 at 0x2da73c** | Arena/prelude work precedes residents. It has direct rand sites **0x2e49de/0x2e5d32**, not a direct srand. Reached work can advance the incoming stream before resident admission. This is not a complete inventory of its child random consumers. |
| Authority path: **0x2e17d0 at 0x2da8df** | Collected zones are processed in unsigned Zone-pointer order. Before each zone's point traversal, **0x31c540 at 0x2e1d7c** can explicitly reseed, as below. New mode0x8a residents draw at **0x2e2686**. |
| **0x2a56c0 at 0x2da8e7**,0x2a45e0 at0x2da8f3 | Other authority prelude lies between residents and pets. Its broader random-consumer behavior is not newly exhausted by this audit; it must not be assumed to rewind the resident stream. |
| **0x2e0db0 at 0x2da8fb** | Pet pass. Its reached pet-setup calls at **0x2e1067/0x2e12bd** invoke0x2b67b0, which explicitly reseeds as below. Ordinary visits do not imply that those setup paths ran. |

**Daily refresh:** if zone+0x4f8 already equals the day stored at **World+0x08**, **0x31c540
returns without reseeding**. Otherwise **0x31c72f** calls srand with
wrapped32 **R(H(zoneX,zoneY)+0x4373e1)+day**, then consumes its specified
shopping/camp/helper randomness. There is no restoration of the pre-refresh
state. A fresh zone's day stamp starts at-1; the generation-end versus later
World-touch refresh distinction is already in worldgen-creatures-zone §1.
An already refreshed zone cannot be treated as if it reseeded on every visit.

**Pet setup:** **0x2b67b0 at 0x2b67d1** calls srand with the target creature's
**unsigned32 race at+0x64**, not its ID, owner ID, World seed or day. It then
runs the existing no-record/wild0 appearance and gear setup, leaving that
advanced CRT stream in place. See worldgen-creatures §7.4 and
[remaining-runtime-gaps.md §7](remaining-runtime-gaps.md) for setup admission
and the other field writes. Reconfiguration/creation can therefore change
the continuation observed by later operations and subsequent substeps.

These explicit seeds occur on the thread that reaches them. In the83e60
host path that is the simulation worker; on a separate caller the same helper
affects that caller's stream. There is no per-World random-state parameter or
caller-state restoration in these specified seed producers.

## 6. Precise remaining unknown

The **numeric CRT state supplied to the fresh simulation thread before its
first game-side explicit seed remains unknown** from these game exports.
The relevant implementation boundary is the imported thread startup and CRT
private state, not a missing World field value. Establishing a particular
fresh-thread default would require separate evidence from that runtime or a
bounded native observation; neither was inspected/executed here.

The state at the first resident rand is not automatically equal to that
unknown entry state: earlier tick callbacks, arena/prelude work and reached
explicit refresh seeds can intervene. Network callbacks, all later World
initialization callers, every skill/effect/loot random consumer, and all
other workers are **not exhaustively audited**. The known constructor,
launch, selected prelude and thread distinctions above must not be expanded
into a claim that there are no other stream users or seed producers.

## 7. Official public documentation: conditional default, launch limit retained

Bounded follow-up, 2026-10-31: only Microsoft public documentation was read;
no SDK headers, runtime implementation source/binary, or original process was
inspected. The native entry audit above remains unchanged.

Microsoft's current **srand** reference explicitly describes seeding the
current thread and documents that rand before a prior srand follows the
same sequence as an explicit seed1. Its archived Visual Studio2015 reference
states the same contract. This establishes the documented **rand-before-srand
sequence equivalence**, rather than a new game-side seed call.
[Microsoft srand reference](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/srand?view=msvc-170),
[Visual Studio2015 srand reference](https://learn.microsoft.com/en-us/previous-versions/f0d4wb4t%28v%3Dvs.140%29).

The public threading documents establish that the CRT thread-creation
functions initialize certain runtime variables, and that the callable
std::thread constructor starts a new thread of execution. The reviewed pages
do **not** specify how this executable's imported
**MSVCP140.DLL::_Thrd_start** initializes, copies or advances rand state before
the native callback. They therefore do not close the exact launch-path gap.
[Microsoft multithreading with C and Win32](https://learn.microsoft.com/en-us/cpp/parallel/multithreading-with-c-and-win32),
[Microsoft thread constructor reference](https://learn.microsoft.com/en-us/cpp/standard-library/thread-class?view=msvc-170),
[Microsoft beginthread/beginthreadex reference](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/beginthread-beginthreadex?view=msvc-170).

**Conditional inference only:** if the executing thread reaches its first
relevant rand with an otherwise untouched, unseeded generator, the documented
API behavior is the seed1 sequence. That condition has not been established
for the exact imported startup chain before0x83e60, or for the first later
resident draw. A preceding startup seed or draw would change the observed
position in that sequence. Sequence equivalence also does not expose a
private numeric TLS field or its storage layout.

Consequently the documentation is **insufficient to replace §6's unknown
with a proven numeric state1 at this game's worker entry**. Neither
constructing-thread seed123 nor the public default-sequence statement alone
proves the worker callback's incoming state. The precise remaining evidence
gap is this runtime launch's per-thread RNG initialization/advancement before
the native callback; the bounded review does not claim that no further
Microsoft documentation exists.
