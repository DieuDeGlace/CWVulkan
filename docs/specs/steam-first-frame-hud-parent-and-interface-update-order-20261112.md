# First-frame HUD-parent value and Interface-update order — supported Steam original

Independent, bounded source-side behavioral contract. It answers: the value of the HUD-parent selected integer reached through G+0x220 at construction and across page transitions, every writer found, the order of message delivery relative to the Interface update's late five-root publication within one main-loop iteration, the Game frame phases that bear on that order, and the conditions that skip or change the publication. Conditional examples are source conditions, not runtime observations.

## 1. Authority, method and terminology

Authority: the original image C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe, 5,757,952 bytes, SHA256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244 (identity re-checked for this contract). All addresses are RVAs relative to image base 0x140000000. The original exports under the sibling decompiled directory were used: assembly.txt, calls.csv, function_ranges.csv, strings.csv, selected functions/*.c presentations and the longer-budget retry-180-phase1 presentation of 0xA5A50. Native operands decided every claim where the C presentation was ambiguous; raw image bytes were compared for the HUD publication window 0x1066C7–0x106744, the keydown gate load at 0x97057, the Interface-update call at 0xAB5CE, the I+0x108 publication store at 0xF9CFB, the Display default-value store at 0x373711 and the property index clear at 0x34A471. All matched the listing. The Infinite-Cubes tree was not consulted; nothing here rests on it. No executable was run.

Terminology follows the cleared contracts. G is the Game, I the embedded Interface at G+0x118, M the GUI manager at G+0x20, C the Chat widget at G+0x398, Q the Controls widget at G+0x658. G+0x220, equivalently I+0x108, holds the eight-byte pointer to the unnamed HUD-parent node P. "Selected integer" of a node N means the four-byte element of the visibility property of N's Display object (the object at N+0x68): the element array pointer is at Display+0xE0 and the signed four-byte current-element index is at Display+0xAC. Query helper 0x9AF00 reports whether that current element is nonzero; setter 0xA3A20 stores a normalized zero or one into the current element. Neither helper inspects ancestors.

The five frontend roots tested by the publication are I+0 (title, G+0x118), I+0x18 (character selection, G+0x130), I+0x58 (character creation, G+0x170), I+0x80 (G+0x198) and I+0xE8 (G+0x200). As new leads, not as screen names: I+0x80 is the root handed to the world-list builder 0x108450 and tested by the world-list drag branch of 0x101160; I+0xE8 is the parent of the button whose caption is the UTF-16 literal "Create World" at 0x441210 and whose event three is bound at 0xFD82F to Game target 0x9F060, which hides I+0xE8.

## 2. The HUD-parent selected integer: values and writers

### 2.1 Creation and initial value

Interface setup 0xF9AC0 (called once from the Game constructor 0x7EA80 at 0x7F4D4) creates P through node factory 0x33CD30 at 0xF9CF6, with the manager root M+0x140 as parent, an empty name and null transform/shape/display arguments, and stores the result at I+0x108 at 0xF9CFB. Because the display argument is null, the factory constructs a fresh Display through 0x3736C0. That constructor builds its "visibility" property at Display+0x70 through base property constructor 0x34A420, which clears the eight bytes at property+0x38 (so the index at Display+0xAC starts at zero), then fills the property's element vector at Display+0xE0 with two four-byte elements both equal to one (source value stored at 0x373711, count two).

Setup stores nothing into P's selected integer. Its other references to I+0x108 (0xFB969, 0xFB9CF, 0xFCE88 onward through 0x1006B9) only pass P as a parent or reparent target. The 24 setup calls to 0xA3A20 target other nodes (I+0x40, +0x48, +0xD0, +0xD8, +0x140, +0x170, +0x1D0, +0x1E8–+0x1F8, +0x2A0 and widget-owned nodes of I+0x218–+0x278 and +0x538/+0x540), never I+0x108. The remaining Game-constructor callees after setup (0x9BB20, 0x9AFB0, 0x107690) do not reference I+0x108 or G+0x220.

Therefore, when the constructor returns and the process Game global at 0x551A80 is published, P's selected integer is **one**. In the same setup the separate roots receive explicit values: I+0 one (0xF9DF5), I+0x18 zero (0xF9EB4), I+0x58 zero (0xF9F70), I+0x80 zero (0xFA032), I+0xE8 zero (0xFA0F4). Correction/clarification of the cleared Chat contract's sentence "The first is initially selected one, the other four initially zero": the explicit one belongs to I+0; P separately starts at one only through the Display default, not through a setup store. Under either reading, P's value before the first publication is one.

### 2.2 Writer census

| Writer | Effect on P's selected integer |
| --- | --- |
| Display constructor default (0x3736C0, via 0x33CD30 in setup) | Both stored elements one; current index zero; observed value one. |
| Late five-root publication in Interface update 0x103200, store call at 0x10671B | Writes normalized one when all five roots report zero, otherwise zero. |

No other writer was found. Evidence: the only direct-displacement loads of G+0x220 as a Game field in the listing are the keydown Chat gate at 0x97057 (a read); every other "+0x220" operand belongs to a stack frame, World (W+0x220), an entity or widget coordinate triple, a string record, or I+0x220 (equivalently G+0x338) inside Interface routines. The only I+0x108 loads in Interface routines are setup's parent arguments and 0x103200 at 0x106714 (the publication) and 0x10672E (the re-read for the mirror below). Direct callers of 0xA3A20 are only 0xA5A50 (18 sites), 0xF9AC0 (24) and 0x103200 (13); none of the Game-update sites uses G+0x220. The Game constructor, Game destructor 0x818D0, release owner 0x965E0, press owner 0x94900, Select 0x9F7C0, creation confirmation 0x9EA10, world creation 0x9F060, return-to-title 0x9FDF0, keydown 0x96E70 (key 0x73 toggles M+0x140, P's parent, not P), Interface layout 0x101BD0, the five HUD toggles called from Game update, and the main loop contain no store to P's selected integer.

Consequently P's value is not reset at session/world entry or at return to the title/frontend. Those transitions change only the root values (section 2.3); P follows at the next traversal of the publication.

### 2.3 Value at each transition

| Moment | P's selected integer |
| --- | --- |
| Game construction complete, before the first Game update | One (default). |
| After the first Game update | Zero, because I+0 is one from setup (unless something changed I+0 first). |
| Title "Continue" path (0x965E0 prefix, title selection value three) hides I+0 | Unchanged by the callback; the next publication writes one if I+0x18, +0x58, +0x80 and +0xE8 are all zero. |
| Title selection value zero (hide I+0, show I+0x18) | Unchanged; next publication still writes zero. |
| Select 0x9F7C0 hides I+0x18 (store 0x9F7F3); for a new slot it shows I+0x58 (0x9FA03) | Unchanged; next publication writes one only if all five roots are then zero. |
| Creation confirmation 0x9EA10 hides I+0x58 (0x9EA4C) | Unchanged; next publication writes one under the same condition. |
| World creation 0x9F060 hides I+0xE8 (0x9F0A7) | Unchanged; next publication reflects the remaining roots. |
| Return to title 0x9FDF0 shows I+0 (0x9FE66) | Unchanged by the callback (it retains one if it was one); next publication writes zero. |

Every one of these page stores is made in a callback or owner reached from message delivery or input polling (section 3), never from inside the publication. The selected inline root-store census above is not complete for I+0x80 and I+0xE8; any other writer of those roots affects P only through the next publication.

## 3. Order within one main-loop iteration

The only message pump in the image is in application owner 0x1355A0 (sole PeekMessage/GetMessage/TranslateMessage/DispatchMessage call sites 0x13646B–0x1364E2). The Game is constructed at 0x136331 and published to 0x551A80 before the first iteration; the window is shown and focused after that. Each iteration then performs, in order:

1. **Quit test.** A nonzero byte G+0x3AD posts a quit message.
2. **Message drain.** While a message is pending: retrieve it; on the quit message, run shutdown (Game deleting slot first) and leave the loop; otherwise translate it, dispatch it to window owner 0x133C70, and retest G+0x3AD. Translation of a key-down posts its character message into the same queue, so the character is normally retrieved later in the same drain. For identifiers 0x100/0x101/0x102 the window owner calls the GUI owner first and then the current-Game owner (cleared Chat contract). Button messages reach press owner 0x94900 and release owner 0x965E0 (whose GUI release dispatches registered page callbacks such as 0x9F7C0, 0x9EA10, 0x9F060 and the menu route to 0x9FDF0); the wheel reaches 0x97CC0; pointer-move messages are ignored.
3. **Input poll** (only when ready byte 0x536CE8 is nonzero and the main window is foreground; otherwise sleep 16 ms). Device states are sampled, the binding-capture predicate 0x9AE50 is tested, mapped actions are applied, the single pointer poll 0x97880 runs, then XInput. Controller buttons can call 0x965E0/0x94900 directly here, so page callbacks can also run in this phase. No keydown/character owner is called from this phase.
4. **Frame driver 0x134420**, called on every iteration including the inactive sleep path (0x137B8E). It returns at once if the renderer global 0x551A90 or the Game global 0x551A80 is zero. Otherwise it issues two clear calls on the object at 0x551A98, measures elapsed time (nanoseconds divided by 1e9, times the stored double 1000 at 0x437FD8; the previous sample is a lazily initialized static, so the very first measured interval is just the gap between two consecutive counter reads), calls Game update 0xA5A50 at 0x134558, handles fullscreen/resize changes, sleeps for the configured frame limit G+0x1BBC if time remains, and finally makes the presentation call (slot +0x40 of the object at 0x551AA0, sync flag from 0x551C2C). The presentation identity is by call position and slot only.

Within Game update the relevant ordered phases are:

| Order | Call site / window | Relevant reads and writes |
| --- | --- | --- |
| a | 0xA6769 onward | Frame-entry target clears (frame-order contract §2.1). |
| b | 0xA7328 / 0xA734C | World step batch (when wanted/loaded world match). |
| c | 0xA7E80 | Drawn-position pass. |
| d | 0xA84D9–0xA8626 | At most one pending Chat record consumed into C's rows (Chat contract). |
| e | 0xA9B2C | Local control 0x8AE50, gated on title/selection/creation roots hidden (live own-node reads), not on P. |
| f | 0xA9DF6–0xAA1BC | Speaker override, right stick, creature presentation 0xB0680 at 0xAA1B1, dialogue 0xB40D0. |
| g | 0xAAED9–0xAAF58 | Helpers 0xB37F0, 0xB36A0, 0xB6240, 0xB03D0, camera update 0xADCD0 at 0xAAF02, 0xAF110, per-object 0x2C3470. |
| h | 0xAAFCC–0xAB137 | Only if C's typing byte is zero and title, selection and creation roots are hidden: rising-edge HUD toggles 0x1031E0 (I+0x550 byte), 0x102FE0, 0x102F10, 0x1030C0, 0x103150 and the G+0x310 Ring toggle. None writes P or any of the five roots. |
| i | 0xAB1A1–0xAB4xx | Full-map pointer branch or the movement-close block (clears G+0x668 and G+0x420 and hides several panels, not the roots or P); G+0x378 panel visibility; G+0x310 auto-close. |
| j | 0xAB5CE | Interface update 0x103200 on I with the float frame milliseconds. |
| k | 0xAD9D4 | Scene submission 0xE2E00. |

The cleared frame-order contract's stages remain correct; this table adds stages h–j, which that contract does not list, between its camera update and scene submission. No correction to its stated order is needed.

Within Interface update 0x103200 (single return at 0x106C18; no jump crosses the publication window; no switch dispatch):

1. Entry: binding-capture test 0x9AE50, then the I+0x330 node's visibility (ordinary-UI result, or zero under binding capture) and position, ordinary-UI-dependent visibility of I+0x560, I+0x551 reset when I+0x550 is zero, I+0x338 hidden.
2. Typing marker 0x1033EF–0x103405 (sets C's dirty byte when typing is nonzero).
3. Page-specific work, including the character-list and world-list passes keyed on I+0x18 and I+0x80, and many HUD widget updates. None of it writes P or the five roots.
4. 0x106685–0x1066C2: visibility of node I+0x1E0 from its own test, ordinary UI and a player flag.
5. **Late publication 0x1066C7–0x10671B**: query I+0, I+0x18, I+0x58, I+0x80, I+0xE8 in that order, stopping at the first nonzero; write zero to P if any was nonzero, one if all five were zero.
6. Mirror 0x106720–0x106740: re-read P and write the same normalized value to the node at widget+0x178 of I+0x268 (G+0x380).
7. Remaining work keyed on I+0x550, hit-counter text, two auto-visibility nodes, then manager update 0x349CF0 at 0x106BA7.

## 4. Readers of P before the first publication of a frame

Only one consumer reads P anywhere: Game keydown 0x96E70 at 0x97057, as the first of its three Chat gates, after its raw-key store and its special-key prework. It reads the then-current element on every delivered key-down. It runs only in drain phase 2, which precedes that iteration's Game update. Every keydown is therefore evaluated against the value published by the **previous** iteration's Game update, or against the constructor default before the first Game update.

Not readers: GUI keydown/keyup/character owners; Game keyup 0x97870; Game character 0x9F670 (focus and typing byte only); press 0x94900, release 0x965E0 and their GUI dispatch; wheel 0x97CC0; pointer poll 0x97880/0x101160; the XInput branch; Game update before 0xAB5CE; and Interface update before step 5. After the publication, only the mirror reads P in the same frame.

Observed values for the requested first frames:

* **First iteration after Game construction.** Keydowns retrieved in the first drain see one. With the constructor states M+0x170 null (manager base 0x339B70 clears it), Q+0x2E0 minus one (Controls constructor) and C+0x1D8 zero, all three gates pass. The first publication then writes zero, given I+0 still one.
* **First iteration in which a gameplay entry callback runs.** Keydowns earlier or later in that drain see the old value, normally zero from the frontend. The same iteration's publication writes one if all five roots are then zero, and keydowns from the next iteration see one.
* **First iteration in which return-to-title runs.** Keydowns in that drain, before or after the callback, see the old value, normally one. The same iteration's publication writes zero. Since the title store happens during the drain, a key-down after the callback in the same drain is evaluated against one while the title root is already shown.

## 5. Conditions that skip or change the publication

* **Skipped:** only when the frame driver returns early because renderer global 0x551A90 or Game global 0x551A80 is zero, when shutdown has begun (the drain leaves the loop on the quit message before another frame), or when an earlier exceptional or nonreturning path aborts the Game or Interface update. P then keeps its last value, and keydown keeps reading it.
* **Not gating:** an inactive or background window (the sleep path still reaches the frame driver); zero or tiny elapsed time (the publication does not use it); binding capture (Q+0x2E0 at least zero); retained focus; Chat typing; ordinary UI 0x1014F0; G+0x420 menu byte; G+0x668 map; the I+0x550 byte; any hidden page; M+0x140 or other ancestors' values. None of these is tested around the publication.
* **Value rules:** each root query treats any nonzero current element as shown, including negative values. The result written is strictly zero or one. Only the current element at the current index is read or written. Null roots, a null P or a null Display are not tested; they would fault rather than skip.
* **Same-frame changes:** a root change made in the drain or poll phase of the same iteration is seen by that iteration's publication. Stages h and i and steps 1–4 of the Interface update do not write the five roots. Root changes made after step 5 (for example by manager update 0x349CF0's node traversal, if any property evaluation alters a root) are seen only by the next iteration's publication.

## 6. Independent conditional examples

These use chosen field values and normal provider returns. They are not captures.

1. *Initial value.* Fresh construction, no input. Before the first Game update P reads one; the first publication sees I+0 one at its first query and writes zero; the I+0x268 mirror node also receives zero.
2. *Keydown before the first publication.* The first drain holds a key-down 0x0D, its translated character 0x0D, then a key-down 0x41. The first keydown passes gate 1 (one), gate 2 (null focus) and gate 3 (minus one), and sets C+0x1D8 to one with the draft unchanged. The character 0x0D is admitted by null focus and nonzero typing but rejected as a control unit after index normalization. The key-down 0x41 also sees one and, with typing now nonzero, reaches Chat's key-edit owner (dirty and normalize only). Its character 0x41 inserts U+0041. The first publication then writes zero. In later title-screen iterations Enter key-downs fail gate 1, so typing stays one, while characters keep inserting into the draft until a completed pointer-press postlude or another reached writer clears typing.
3. *Same, but a second key-down 0x0D follows in the first drain.* It also sees one; typing is nonzero, so it clears typing, submits the current draft (the ordinary branch queues it to G+0x21A0 if nonempty), clears the draft and normalizes.
4. *Entry, then late update.* P is zero, the title is shown and the other roots are zero. One drain holds the release completing title value three, then a key-down 0x0D. The prefix hides I+0 before GUI release. The key-down sees zero and fails gate 1. In the same iteration the publication finds all five roots zero and writes one. A key-down 0x0D in the next iteration passes gate 1.
5. *Page change ordered before a key in the same drain, return direction.* P is one in gameplay. A menu callback reaches 0x9FDF0 (title shown), then a key-down 0x0D arrives with null focus and Q+0x2E0 negative. The key-down sees one and opens typing over the title page. That iteration's publication writes zero, and later Enter key-downs fail gate 1.
6. *Partial page change.* Value zero hides I+0 but shows I+0x18. The publication's first query is zero and its second is nonzero, so it writes zero; P stays zero.
7. *Skipped frame.* With renderer global zero, a page change leaves P unchanged indefinitely. Keydowns keep reading the last published value.
8. *Background window.* With the main window not foreground, the poll is skipped and the loop sleeps 16 ms, but the frame driver still publishes. P tracks root changes made by delivered messages.

## 7. What remains unproved

* Whether the Display visibility index (Display+0xAC) of P or of a root can ever leave zero. No simple store to such an index was found in the GUI range, but wider stores, sequence/animation property evaluation and computed writes were not closed. P's second stored element (also one at construction) is never written by 0xA3A20.
* Whether manager update 0x349CF0 or node advance/seek property evaluation can rewrite visibility elements of P or of the roots. Their property-evaluation internals were not closed.
* A complete writer census for the five roots, especially I+0x80 and I+0xE8. Only the selected inline stores and owners named above were identified. The full flow between world list, world creation, character selection and gameplay is not claimed.
* Universal absence of other writers to P: computed addresses, aliases, bulk copies, exception paths and uninspected indirect callers are not excluded.
* OS behavior: whether and when key messages are queued before the window is shown or focused, how input during loading is delivered, generation of translated characters and repeat timing. Only handling of retrieved messages is stated.
* Callback reentry and thread interleaving: the Game's worker threads were not inspected for GUI writes. Steam overlay or other injected hooks are outside scope.
* The presentation call's import identity, the frame-limit and fullscreen helper internals, and the meaning of ready byte 0x536CE8.
* The I+0x268 mirror target's screen role, and the gameplay meaning of the nodes changed in stages h–i.
* The pointer-press postlude reachability on the title page, and the complete set of typing-byte writers beyond those named in the cleared Chat contract.

## 8. Exposure statement

Read for this contract: docs/CLEANROOM_AUTHOR.md; the three assigned cleared contracts (Chat ingress, gameplay frame order, character-list input); and filename-selected original evidence (the executable, assembly listing, call and range metadata, string table, selected C presentations including the retry Game update). The coordinating harness injected the repository's top-level project instruction text and the branch name into this context at session start, without any tool read; it carries workflow policy and no engine, test, exporter or runtime content. Nothing else in the CubeVulkan repository was read, no engine or runtime information was received, and no other agent was used.
