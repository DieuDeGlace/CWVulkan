# Steam saved playable appearance: CRT consumption

Stable bounded native-only audit, 2026-11-01. Sources are **0x53e80**,
its dispatch tables/instructions, reached arithmetic helpers and saved
character call **0x147d59**. No game, implementation, SDK, owner data or
captures were inspected or executed. This closes the playable saved-input
appearance limit in [CRT initialization §3](steam-crt-runtime-initialization.md#3-construction-seeds-and-saved-data-admission).

## 1. Qualified input and exact result

For a **nonnull readable customization descriptor whose people index is
0..7**,0x53e80 consumes **zero rand draws and zero srand calls** across
both sex choices, every saved face/hair index and every saved RGB byte.
The executing CRT state is unchanged by this appearance call. This
includes the reached saved-character caller's fourth argument0.

| Input | Layout/meaning |
| --- | --- |
| Argument1 | Pointer to the creature's32-bit race field, normally creature+0x64 |
| Argument2 | Appearance output, normally creature+0x78 |
| Argument3 | Nonnull customization descriptor, supplied from creature+0x1528 by0x147800 at0x147d59 |
| Descriptor+0x00 | Signed32 people index0..7 |
| Descriptor+0x04 | Sex byte; only bit0 selects the two routes |
| Descriptor+0x08 / +0x0c | Signed32 face / haircut indices |
| Descriptor+0x10/+0x11/+0x12 | Unsigned8 red/green/blue |

The nonnull branch at **0x53fdb–0x54053** selects and writes race before
the appearance switch. A valid people index overrides the incoming race;
an unrelated saved incoming race therefore does not select an NPC random
branch afterward. Other sex-byte bits do not cause another route.

| People index | People | Race for sex bit0=0 / bit0=1 | Native appearance entries | Draws / seeds, both routes |
| --- | --- | --- | --- | --- |
| 0 | Human | 2 / 3 | 0x5412a / 0x54192 | 0 / 0 |
| 1 | Elf | 0 / 1 | 0x540c0 / 0x5435e | 0 / 0 |
| 2 | Dwarf | 9 / 10 | 0x54c93 / 0x54cfc | 0 / 0 |
| 3 | Orc | 11 / 12 | 0x54db1 / 0x54e43 | 0 / 0 |
| 4 | Goblin | 4 / 5 | 0x543fd / 0x5445a | 0 / 0 |
| 5 | Lizardman | 7 / 8 | 0x545a2 / 0x5460e | 0 / 0 |
| 6 | Undead | 15 / 16 | 0x556f7 / 0x55754 | 0 / 0 |
| 7 | Frogman | 13 / 14 | 0x54216 / 0x54285 | 0 / 0 |

The body-dispatch table is at **0x5decc**. Its reached playable branches,
shared joins including **0x5dda0 / 0x5dda5**, and common arithmetic
postprocessing contain no reached random draw or seed. The reached helper
calls do not introduce one. In particular **people index6** selects
Undead15/16 and never enters the randomized NPC **race6** branch.

## 2. Face, hair and color conditions

Face/hair selections use the saved signed indices and the fixed per-race
remainder/count arithmetic. They do not draw a new face/hair value. Zero,
negative and out-of-UI-range signed32 values do not act as random-choice
sentinels. They retain the native arithmetic/narrowing behavior; no extra
validity clamp or random fallback is supplied here.

For the seven people other than Frogman, descriptor RGB is copied into
appearance **+2/+3/+4**. Both Frogman routes instead write **(255,255,255)**
there, independently of saved RGB. No RGB value requests random coloring.
Selected head/hair identifiers are written as16-bit appearance fields
**+0x18 / +0x1a**; the existing fixed bases/counts remain in
[frontend characters §5.4](frontend-characters.md#54-applying-the-look-0x272d90)
and worldgen-creatures-tables T7. This supplement changes no resource data.

Changing face, hair or color changes no CRT consumption count. The
appearance portion of each qualified roster/selected-character load
contributes zero draws and zero seeds. Subsequent caller work such as
crafting rebuild or starting-kit generation retains its separate contract.

## 3. Null-descriptor boundary

When argument3 is null, the different prologue performs **five draws in
order**, at **0x53f21, 0x53f3b, 0x53f5a, 0x53f88, 0x53fae**. Those draws
initialize temporary color/face/hair inputs; the following race branch
can ignore or replace those inputs. The five-draw prologue does not run
for any qualified nonnull descriptor above.

The constructor's null-descriptor call at0x7f642 uses incoming race0x101,
whose following branch adds no further draw: total5 for that call. Other
null/NPC race routes can add their own draws and are outside this audit.

A nonnull descriptor with people index outside0..7 does not receive the
race override at0x54053. Its incoming race can reach a different NPC branch.
No zero-consumption guarantee is made for that case, a null/unreadable
descriptor or callers outside this qualified playable subset. No whole-
startup, worker-entry or universal first-frame state is inferred here.
