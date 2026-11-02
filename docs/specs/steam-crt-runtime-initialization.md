# Steam CRT default, startup inputs and bounded reset coverage

Stable bounded follow-up, 2026-11-01. This supplements
[main-thread ownership](steam-main-crt-stream.md) and
[host-thread entry limits](steam-host-crt-stream.md). Sources are original
game code, its import references and one locally available runtime binary.
No original process, saves, captures, SDK or runtime implementation source
were inspected or executed.

## 1. Runtime provenance and meaning of the default

The game imports `rand` and `srand` from
`API-MS-WIN-CRT-UTILITY-L1-1-0.DLL`. The separately inspected runtime is
`C:\Windows\System32\ucrtbase.dll`, file version
**10.0.19041.3636 (WinBuild.160101.0800)**, length **1046080 bytes**, SHA-256
**3c60056371f82e4744185b6f2fa0c69042b1e78804685944132974dd13f3b6d9**.
Every RVA in §2 is relative to that runtime module, not the game image.

This identifies an inspected implementation, not a captured loader binding.
The actual runtime module/version selected for the game's API-set imports
has not been observed. A fresh record in this inspected implementation
starts with numeric state **1**. That does not establish state1 at Game
construction, a worker callback, the first frame or the first particle:
those entry points can follow draws or explicit seeds.

## 2. Current-context storage, initialization and update

Runtime `rand` at **0x20a60** and `srand` at **0x20d10** both obtain their
record through **0x12fe0**, at **0x20a64 / 0x20d18** respectively. The
state is the record's **unsigned32 field +0x28**. Neither operation takes
a World, Game, Zone or creature pointer.

The accessor obtains its context through **0x13a40**. That wrapper resolves
`FlsGetValue`; its embedded name is at **0xbc918**, with lookup at
**0x13aaf–0x13ab9**. Its unavailable-function fallback at **0x4c59e** calls
imported `TlsGetValue`. This is runtime-owned current-context storage:
Windows fiber-local storage when available, thread-local storage on the
fallback. It is not selected from a World field.

Within that context, the accessor combines a base with the runtime's
separate state selector, using stride **0x3c8**, at **0x13019–0x1302e**.
Lazy creation allocates **two records of0x3c8 bytes** at
**0x13056–0x13060**. Initialization **0x163a4** visits both records;
**0x163f4**, specifically **0x16406 / 0x1640f**, writes numeric1 to each
record's +0x28. This establishes the numeric initialized field, not merely
equivalence of the returned sequence.

A draw replaces unsigned32 state with **214013 × state + 2531011**, with
low32 wrap, then returns bits16..30 of that replacement. `srand` stores
its unsigned32 seed to the selected +0x28 at **0x20d1d**. Neither restores
the previous state. An untouched state1 therefore advances to **2745024**
and returns **41** on its first draw.

The ordinary calls in main CRT §1 share their executing context's record.
Changing Worlds does not itself select a different CRT stream. Workers
execute in their own contexts; sharing a Game/catalog pointer or publishing
generated records does not copy the constructing thread's state into them.
A runtime fiber/domain switch is distinct from World replacement; none has
been established on these bounded ordinary Game paths.

## 3. Construction seeds and saved-data admission

Both known World initializations explicitly seed123 on the constructing
window thread: Game **0x7ea80 → 0xf17a0 at0x7eb36 → 0x2b5f20 at0xf1969**,
and Game's later **0x2b5f20 at0x7f279**. The seed instruction is
**0x2b5f9a**. The second replaces the state left by the first and intervening
work. Its content-phase reference remains **1037 draws**, ending at
**0x4deb0e3e**, in main CRT §2. That is a subphase boundary.

The later startup path has these verified inputs and order:

1. The character database key **`num`** is queried at **0x7f3ec**; the
   embedded three-byte key is at game RVA **0x436070**. A present value
   containing at least four bytes and a positive signed32 count causes
   cached characters to be loaded at indices **0 through count−1,
   ascending**. The call is **0x9bb20 at0x7f488**. Missing, shorter or
   nonpositive count cases do not run that loop.
2. After interface construction, the new local avatar's appearance is
   prepared through **0x53e80 at0x7f642**, with its third argument null.
   That prologue consumes **five CRT draws**, at **0x53f21, 0x53f3b,
   0x53f5a, 0x53f88, 0x53fae**. The constructor's race0x101 branch adds
   no further imported draw. These are advances, not a seed operation.
3. The key **`slot`** is queried at **0x7f6bf**; its embedded four-byte key
   is at **0x4360d0**. Key presence admits the selected-character branch.
   With at least four bytes, its signed32 value replaces Game+0x2264;
   a shorter present value retains that field. This branch calls
   **0x9bb20 at0x7f71a** with the field, then reaches crafting rebuild
   **0xb2180 at0x7f74d**. Admission is key presence, not proof that the
   character load succeeded.

Character loading reaches deserializer **0x147800 at0x9bccf**, which calls
appearance **0x53e80 at0x147d59** using the character's actual customization
pointer at +0x1528. It does not force the third argument null. Therefore
reaching the loader does not justify charging five null-descriptor draws
for every cached/selected character. The bounded follow-up now proves
**zero draws and zero seeds** for a nonnull customization descriptor with
people index0..7, either sex bit, and arbitrary saved face/hair/RGB values:
see [playable appearance CRT](steam-playable-appearance-crt.md). Appearance
outside that qualified subset remains outside this supplement. The loader's
resolved direct-call descendants contain no imported seed operation.

Crafting's seeds remain main CRT §4: only after outer inventory begin/end
+0x9f0/+0x9f8 differ, seed at **0xb23ce** from the live region pair
+0x150/+0x154; increasing admitted groups0..4 can each reseed at
**0xb244d**. Reaching the `slot` branch proves neither the inventory gate
nor a group gate passed. Saved selection, loaded region and admissions are
inputs, not an owner-independent numeric seed.

**Arithmetic reference only:** five draws starting immediately from
0x4deb0e3e return **12519, 19516, 22542, 23322, 24224**, leaving state
**1587584925 (0x5ea09f9d)**. This labels those five draws, not proof that
roster loading, selection restoration, crafting or every intervening
callback left the constructor at that state.

## 4. Bounded reset and replacement coverage

The native resolved import references contain **50 direct srand call sites
in27 game routines**. Within the resolved direct-call descendants of Game
constructor **0x7ea80**, seed-producing routines are **0x2b5f20** and
**0xb2180**; within host constructor **0xf17a0**, it is **0x2b5f20**.
This covers resolved direct calls, not every indirect callback or calls
made inside another loaded module.

World construction **0x29a130** and its resolved direct-call descendants
contain neither imported rand nor srand. Constructing the containers and
running content initializer0x2b5f20 are separate operations; the latter is
the demonstrated explicit seed producer.

The only resolved direct callers of **0x2b5f20** are the two constructor
sites above. The inspected client transition **0x3d820** does not call it.
That transition's actor reset **0x2d2c00 at0x3da3c** and loaded-state/history
reset **0x2da090 at0x3da4c** are not CRT resets. Resolved direct descendants
of that transition, of **0x2da090**, and of daily reset **0x2d2eb0 /
0x2c34c0** contain no imported seed operation. Their state effects remain
in [the runtime reset contract](remaining-runtime-gaps.md).

Replacing a character through **0x9bb20** does not inherently reseed;
its callers' explicit crafting/starter-kit work is separate. Starter-kit,
crafting and stock seeds remain in main CRT §4. Later daily zone refresh,
pet setup and new-zone generation reset the executing thread under their
own gates, in main CRT §§4–5 and host CRT §§4–5. These operations do not
recover or restore saved per-World CRT state.

## 5. Remaining limits

Established: this inspected runtime's fresh state1, current-context storage,
both construction seed routes, startup database branch inputs and the
bounded direct-call reset coverage. Still unproved: actual loaded runtime
identity, exact state at window/Game entry, a universal first-frame state,
appearance outside the qualified playable subset, optional callback draws,
and state supplied by imported
launch machinery before each worker callback. No numeric state is inferred
from a clock, World seed field, character ID or worker creator.

Host CRT §7's public-documentation review remains valid for its scope.
This later binary evidence closes the inspected runtime's **fresh-record
default**; it does not prove state1 at **0x83e60** entry or close that
launch/advancement and module-binding gap.
