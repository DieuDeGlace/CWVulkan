# Steam Game update: complete phase schedule of one ordinary frame and its outer main-loop iteration

Status: FINAL / STABLE / HELD (source-side behavioural contract, code-free).
Date: 2026-11-12.
Scope: the supported Steam release only. It answers (1) the complete ordered census of one ordinary Game update reached through 0x134420 -> 0xA5A50, (2) the outer main-loop iteration around it, (3) every early exit of the Game update, (4) what could not be proved. Nothing here is implementation advice. Conditional examples are source conditions, not runtime observations.

## 0. Authority, method and exposure

Authority: the analysis image C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe, 5,757,952 bytes, SHA-256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244 (re-checked for this contract), image base 0x140000000. The installed retail executable C:/Program Files (x86)/Steam/steamapps/common/Cube World/cubeworld.exe was hashed only: 5,939,072 bytes, SHA-256 A1EFBBD41EEB8DF08395C4D6511194D416CB70A14054B2A2000B9F5CDBCFA65B, matching the identity recorded in the held host-thread contract. All addresses below are RVAs of the analysis image. No executable was run.

Evidence used: the original export under the sibling decompiled directory — the per-owner instruction listing of 0xA5A50 (6,106 discovered instructions), the whole-image listing for scoped searches, the call-reference table (908 call records owned by 0xA5A50, every one reviewed in call-site order), the function range table, the longer-budget presentation of 0xA5A50, selected per-function presentations of the frame driver 0x134420, the application owner 0x1355A0 and roughly forty direct callees, and raw data reads from the image for constants and initial values. Where the presentation and the native listing differed, native operands decided; the resulting corrections to held contracts are recorded in section 6.

Coverage limit that matters: 0xA5A50 contains four computed jumps (switch dispatches) at 0xA74D6, 0xAA9F8, 0xABDE0 and 0xABEC8 whose case bodies lie in byte ranges the exporter did not decode (0xA74D8–0xA7E6B, 0xAA9FA–0xAAC3F, 0xABDE2–0xABE33, 0xABECA–0xABF81). Those bodies carry no call records. Their behaviour below (region-discovery narration, item-description narration, two music-track selections) is taken from the longer-budget presentation only and is marked "presentation-only".

The Infinite-Cubes reconstruction was not consulted for this contract; nothing here rests on it. Read for this assignment: docs/CLEANROOM_AUTHOR.md, the five assigned held contracts (gameplay frame order; first-frame HUD parent and Interface-update order; world clock units; in-process host thread and world-step scheduling; world-step phase order) and the original evidence above. The coordinating harness injected the repository's top-level project instruction text and the branch name into this context at session start without a tool read; it carries workflow policy only. No engine, test, exporter, log, capture, owner asset or runtime information was read or received. No other agent was used.

## 1. Terminology

G is the Game object (process global at 0x551A80). W is the rendered/client World, pointer at G+0x08. P is W's local creature, pointer at W+0x448. I is the embedded Interface at G+0x118. M is the GUI manager at G+0x20. CL is the network client embedded at G+0xD18. CH is the Chat widget at G+0x398 (its typing byte is CH+0x1D8). "Shown" for a node N means the current element of N's Display visibility property is nonzero (query helper 0x9AF00, setter 0xA3A20), as in the first-frame contract. Roots: title I+0 (G+0x118), character selection I+0x18 (G+0x130), character creation I+0x58 (G+0x170). "Ordinary UI" is the predicate 0x1014F0 on I (wrapper 0x9AE70). The capped frame interval is the binary64 milliseconds value after the 1000 cap (section 2.4); "integer ms" is that value truncated toward zero (native CVTTSD2SI at 0xAAED0).

"World aggregate" is W's sixteen two-word list containers W+0x90..W+0x180 (the outgoing aggregate replaced at frame end). "Received aggregate" is G's sixteen matching containers G+0x1970..G+0x1A60. The descriptive labels used for individual containers (hits, sounds, particles, edits, projectiles, item pickups, loot, notices) are consumer-derived names from what this routine does with them; they are not proved business names. Offsets correspond one-to-one: G+0x1970 to W+0x90, G+0x1980 to W+0xA0, and so on in 0x10 steps.

## 2. The outer main-loop iteration (question 2)

### 2.1 Iteration order

The only message pump is in application owner 0x1355A0. The loop head is 0x136440; the loop tail calls the frame driver at 0x137B8E and jumps back at 0x137B93. Each iteration performs, in this order:

| Step | RVA | Behaviour |
| --- | --- | --- |
| 1 | 0x136440–0x136452 | Quit request: if the byte at **G+0x1D68** is nonzero, post a quit message. |
| 2 | 0x136458–0x1364EB | Message drain. Peek without removal; while a message is pending, retrieve it with the blocking retrieve call. If the retrieve call reports the quit message, run shutdown (close the window, delete the Game first, then renderer, world map, world, GUI, sprites, graphics, release the input devices, swap chain, device and context, network cleanup, platform shutdown) and leave the loop permanently. Otherwise translate, dispatch (window owner 0x133C70, per the held first-frame contract), retest G+0x1D68 and peek again. The drain continues until the queue is empty, so messages posted during dispatch (for example translated characters) are normally retrieved in the same drain. |
| 3 | 0x1364ED | If the ready byte at 0x536CE8 is zero, go directly to step 6. That byte is initialised to 1 in the image's data section and no direct-address writer exists in the listing (computed writes not excluded). |
| 4 | 0x1364FA–0x136625 | Cursor handling. Read the client rectangle and the cursor position. If ordinary UI is false and the cursor is more than 500 pixels from the client centre on either axis and the main window is foreground, clear the latch at 0x551A8C and warp the cursor back to the centre. If ordinary UI is true, then on the first such iteration after the latch was clear, warp the OS cursor to the GUI cursor position (GUI object at 0x551AC0, floats +0x30/+0x184) when they differ, and set the latch. |
| 5 | 0x13662B–0x137B81 | If the ready byte is zero or the main window is not the foreground window, go to step 6. Otherwise poll input (section 2.2). |
| 6 | 0x137B83 | Sleep 16 ms. Reached only by the two skip branches of steps 3 and 5 (not foreground, or ready byte zero); the polled path jumps past it to step 7. |
| 7 | 0x137B8E | Frame driver 0x134420 (section 2.3), on every iteration that did not leave in step 2. |

There is no separate frontend versus gameplay dispatch in the main loop or the frame driver. Title, character selection, character creation and world pages are Interface roots; the same Game update 0xA5A50 runs every frame and gates individual phases on root visibility (section 5). The Interface update, including the late five-root publication of the first-frame contract, is a phase inside the Game update at 0xAB5CE, not a main-loop step.

### 2.2 Input poll (step 5)

Order inside the polled path: keyboard device state (256 bytes) and mouse device state (16 bytes) are read through their device objects' slot +0x48; the binding-capture predicate 0x9AE50 is tested at 0x136630.

* Capture inactive: for each of the 23 bindings, mapper 0xA36C0 applies the bound key (type 0, code at G+0x1BDC+8·i, at most 0xFF) or mouse button (type 1, index below 4); any active keyboard/mouse binding clears the last-device byte G+0x1958. Then the single pointer poll 0x97880 runs. Then XInput user 0 is read. On success, button states route to the mapper, to press owner 0x94900 and release owner 0x965E0 (so page callbacks can run here), set G+0x1958 to one, and with ordinary UI true the pad's B button directly zeroes the visibility elements of the panels at G+0x330, G+0x350, G+0x340, G+0x348, G+0x388, G+0x368, G+0x360, G+0x358, G+0x650 and G+0x658 and clears bytes G+0x668 (map) and G+0x420 (direct element stores, not through 0xA3A20). On XInput failure two pad-reset helpers 0xA38C0 and 0xA3900 run instead.
* Capture active: the poll scans for a newly pressed key (excluding codes 1, 0x1C and 0x9C) or mouse button and records it through 0x8A120 and 0x8A4F0; the ordinary mapping, pointer poll and XInput are not run that iteration.
* Both branches end by copying the current key and mouse state into the previous-state buffers (0x137B04 onward).

No keydown or character owner is called from the poll; those run only from step 2.

### 2.3 Frame driver 0x134420

1. Return immediately if the renderer global 0x551A90 or the Game global 0x551A80 is zero. Otherwise:
2. With 1 ms timer resolution raised and restored, sample the millisecond tick T0 (0x134464).
3. Two clear calls on the device-context object at 0x551A98 (slots +0x190 and +0x1A8; the colour comes from the 16 bytes at 0x42E8C0, the second call passes flags 3 and float 1.0). These precede the Game update, so its later scene submission draws onto freshly cleared targets.
4. Elapsed time: a lazily initialised static timestamp (0x551CB0) holds the previous nanosecond value. Current nanoseconds are built from the performance counter and frequency by quotient/remainder arithmetic; the signed difference is converted to binary64, divided by 1e9 and multiplied by binary64 1000 (0x437FD8). The very first interval is the gap between two consecutive counter reads.
5. Game update 0xA5A50 at 0x134558 with that uncapped binary64 value.
6. Only if the main window is foreground: query the swap chain's fullscreen state (slot +0x58) into 0x551BF0.
7. If the cached display settings differ from G+0x1B88 (fullscreen flag), G+0x1B8C, G+0x1B90, G+0x1B94, G+0x1B98 or G+0x1BC4 (dword comparisons): copy G+0x1B88..G+0x1BD8 into the cache, set the fullscreen state (slot +0x50), call 0x135310 with zero, then resize the target (slot +0x70) with width G+0x1B8C, height G+0x1B90, rate G+0x1B94 over 1, and format value 0x1C.
8. With 1 ms resolution raised: if (tick − T0) is below the dword frame budget G+0x1BBC, sleep for the difference (0x134725); restore the resolution.
9. Present (swap-chain slot +0x40 at 0x13474E) with sync flag "cached G+0x1BC4 nonzero" (0x551C2C).

Correction to the held host-thread contract §4.1 item 3: the frame-limit sleep is **before** the presentation call, not after it; the sleep budget is measured from T0 at driver entry and therefore includes the clears, the whole Game update and any resize work. Slot identities (clear, fullscreen, resize, present) are by slot position only; their interface identity was not separately proved.

### 2.4 Zero-ms frame and the 1000 ms cap

* The cap is applied only inside the Game update (entry, 0xA5A50 prologue): the value used is the input when it is less than or equal to binary64 1000, otherwise 1000. The frame driver itself never caps. Every per-frame consumer in section 3 receives this capped value (as binary64, as binary32 for the drawn-position pass, or as truncated integer ms for the effect agers). Time above 1000 ms is discarded; there is no catch-up.
* At 1000 ms an admitted world batch makes sixty-two binary32 16.0 steps and one binary32 8.0 final step (held world-clock contract). All other phases still run once.
* A zero-ms frame (two equal counter reads) still runs every phase of section 3 in order: one final world step of 0.0 when admitted, the drawn-position pass with 0.0, all list consumers, all edge detections (their previous-state statics are refreshed), the Interface update and scene submission. Timers that subtract the interval (G+0x2050, G+0x204C, the Ring repeat statics, the static at 0x5514F8) are unchanged; the integer-ms agers receive 0. Any fractional interval below 1 ms likewise gives the agers 0 while the world batch and camera receive the fraction.
* The frame-budget sleep and the presentation of section 2.3 are unaffected by the cap.

## 3. Complete ordered census of one Game update 0xA5A50 (question 1)

The routine has one return (0xADBD7, section 4). Phases below are in execution order; "once" means once per Game update. Only the world step 0x2DA6C0 runs per substep; nothing else in this routine is repeated per substep. The World lock W+0x3C8 is held from 0xA6464 to 0xADB23.

### 3.1 Before the World lock

| # | RVA window | Phase | Behaviour, reads/writes | Gates / cadence |
| --- | --- | --- | --- | --- |
| 1 | prologue | Cap | Capped binary64 interval retained for the whole routine. | Once. |
| 2 | 0xA5B13–0xA5B1A | List clear | Free every node of the list at G+0x6C8 and leave it empty (0x3C9C0). Role of that list unresolved. | Once, unconditional. |
| 3 | 0xA5B59–0xA643B | Input-hint labels | When G+0x1958 is zero (last device keyboard/mouse): set the "name" text of the first three entries of the hint-slot vector G+0x228 (entry stride 0x20) to the key names of bindings at G+0x1BFC, G+0x1C34 and G+0x1BF4 (0x4CA10 on G+0x30), with default colours. Otherwise set them to the pad glyph strings "X", "A", "Y" with other colours. Then set the texts of the six key-label nodes of the vector G+0x240 from bindings G+0x1C6C, G+0x1C64, G+0x1C54, G+0x1C74, G+0x1C7C and the literal "F1". | Once, every frame, all pages. |
| 4 | 0xA6440–0xA644D | Cursor latch | If M+0x1A8 has neither of bits 0x1/0x2, clear G+0x1DB0. | Once. |
| 5 | 0xA6455 | T0 diagnostic tick | Millisecond tick for diagnostics only. | Once. |
| 6 | 0xA6464 | Enter W's World lock. | | Once. |

### 3.2 Frame-entry state and frontend handling (under lock)

| # | RVA window | Phase | Behaviour, reads/writes | Gates / cadence |
| --- | --- | --- | --- | --- |
| 7 | 0xA6470–0xA64A0 | Platform-user cache | If P+0x968 is zero, obtain the local platform user identifier through the platform context (call through slot +0x10) and store it at P+0x968. | Only while zero. |
| 8 | 0xA64B8 | Local scratch aggregate constructed (destroyed at 0xADB71); no use between was identified. | | Once. |
| 9 | 0xA64C0–0xA65F0 (no calls) | Frontend player pose and camera | If the title or the character-selection root is shown: P ability byte P+0x68 = 0x9D; P+0x1B0/+0x1B4 = P's physical X/Y block coordinates (signed 64-bit position shifted right 16 with rounding toward zero); P+0x1B8..+0x1C8 = P's physical position; camera yaw G+0x1DA8 += capped ms × 0.003 (binary64 at 0x437FA0); camera pitch G+0x1D98 = 110.0; G+0x1DB8 = 12.0. Otherwise, if P's ability is 0x9D, set it to 0. Character creation alone does not take this branch. | Once. |
| 10 | following, no calls | Night lamp on frontend/travel | If title, selection or creation is shown, or P's ability is 0x9C: set bit 0x0001 of P+0x128 when W's time-of-day W+0x0C is below 18,000,000 or at least 68,400,000 (binary32 at 0x438010/0x438020), else clear it. | Once. |
| 11 | 0xA669C–0xA675C | Lock-wait diagnostic | Sample the tick again; if it exceeds T0 by more than 1 ms, write "lag: " with the wait and the time since the previous such line to standard output. | Diagnostic only. |
| 12 | 0xA6769–0xA67DA | Target clears | G+0x1E58 = 0, G+0x1E68 = 0, the two 12-byte triples at G+0x1E70 and G+0x1E7C = (0x7FFFFFFF, 0x7FFFFFFF, 0), G+0x1E88 = 0 (frame-order contract §2 stage 1). | Once, unconditional. |
| 13 | 0xA67FC–0xA6E91 | Hint-slot contents | Clear the "count" text of hint entry 2; set its "info" text empty when the class special-ability code 0x656E0(P) is zero, else to that ability's localised name, and when P's cooldown record for it (map P+0xB78) has a nonzero float at +0x20, to that cooldown converted to whole seconds ((value + 999) truncated, divided by 1000) with a suffix. Clear entry 3's "info". Reset the item shown by the widgets G+0x3E0 and G+0x3E8 to an empty item record (0x9CD0). Clear entry 1's "count" and "info" texts and colour. | Once. |
| 14 | after 13 | Dead-player ability reset | If P's HP float P+0x180 is ≤ 0, P+0x68 = 0. | Once. |
| 15 | 0xA6EB9–0xA6FC5 | Map zoom index | Rising edge of logical byte G+0x1A85 (previous in static 0x5514F4) decrements G+0x1E04; rising edge of G+0x1A86 (static 0x5514F5) increments it. Each result is clamped below at 0 and to (record count − 1) of the map object G+0x10 (records of 0x68 bytes between map+0x170 and map+0x178). | Every frame; no map-open, chat, menu or focus gate. |
| 16 | 0xA6FD0–0xA6FE9 | Map-closed reset | If G+0x668 (full map open) is zero: G+0x1E38 = 0 and map+0x164 = 0. | Once. |
| 17 | 0xA7004–0xA702F | Snapshots | Save P's gold P+0xAAC, the current-zone counter 0x60EC0(P), P's zone pair P+0x150/+0x154, and P's maximum-HP value 0x5FAC0(P). | Once. |
| 18 | 0xA7043, 0xA7053 | Membership-set clears | Empty W+0x190 and W+0x1A0 (0x8AAD0). These are the two sets the publication routine 0x2E30B0 later reads and clears (world-step contract). | Once, before the world batch. |
| 19 | 0xA705F | Audio object call | Call slot +0x08 of the audio object at G+0x28. Effect unresolved. | Once. |
| 20 | 0xA7070–0xA7130 | Travel tick sound | If P's ability is 0x9C, its timer P+0x6C exceeds 500.0, and the truncated timer divided by 500 differs from the truncated (timer + capped ms) divided by 500, play sound 0x78 at P's physical position (0xA0420) with two CRT random draws shaping its parameters. The timer is read before the world batch advances it. | Once at most. |
| 21 | 0xA7140 | Diagnostic tick. | | |
| 22 | 0xA7162 | P state copy | Copy-construct a 1,760-byte local from P+0x278; no later use was identified in this routine. | Once. |
| 23 | 0xA71DF–0xA723D | Discovery cell test | From P's physical block coordinates, floor-divide by 1024 to a cell (cx, cy); if 0x36160(W, cx, cy) is true round both down to even. Record whether 0x5E430(P, cx, cy) is already true. Query special item 0x22 of P (0x5E3A0). | Once, before the world batch. |
| 24 | 0xA72A0–0xA72C7 | Frontend flag | Set bit 0x0080 of P+0x128 if any of title, selection or creation is shown, else clear it. | Once. |

### 3.3 World batch and its immediate consumers

| # | RVA window | Phase | Behaviour, reads/writes | Gates / cadence |
| --- | --- | --- | --- | --- |
| 25 | 0xA72E1–0xA734C | Client World batch | Admitted only when the dword G+0x206C equals W+0x220 **and** the string comparison 0x6DD20(G+0x2070, W+0x200) returns nonzero (equal). Then binary32 16.0 steps of 0x2DA6C0 while the binary64 remainder exceeds 16.0, then one final step with the remainder narrowed to binary32 (held clock and step-order contracts). | Per substep (the only per-substep work). |
| 26 | 0xA7353–0xA7370 | Mismatch hide | When the batch is not admitted, directly zero the current visibility element of the node at (G+0x380)+0x178 (the HUD-parent mirror target of the first-frame contract). | Only on mismatch. |
| 27 | 0xA7382–0xA7415 | Maximum-HP rescale | If 0x5FAC0(P) now differs from the snapshot of phase 17: write a diagnostic line, multiply P+0x180 by new/old, then clamp it to at most the new maximum (strictly-greater test). | Once at most. |
| 28 | 0xA7464–0xA74D6 (+ undecoded 0xA74D8–0xA7E6B) | Region-discovery narration | If phase 23's test was false and 0x5E430(P, cx, cy) is now true: G+0x2050 = 60000 (0xA7471); classify the cell through 0x115DF0/0x93480; for kinds 1, 0x13, 0x14, 0x15 (only when P+0x128 bit 0x4000 is set), 0x16 and 0x17 build a narration entry ("new_page", "define_land", and a kind-specific description key; kind 0x17 chooses singular or plural by special item 0x22) and queue it through 0x1694E0 with one CRT random draw modulo 10000 to the general DialogWidget's container (G+0x418)+0x1E8. Case bodies presentation-only. | Once at most. |
| 29 | 0xA7E6F–0xA7E80 | Drawn-position pass | 0x2B68E0(W, capped ms as binary32) — frame-order contract §7. Then 0x368230 at 0xA7E89, which returns zero with no effect. | Once, unconditional. |
| 30 | 0xA7E95–0xA7F42 | Map focus | If G+0x1E38 is zero, G+0x1E08..+0x1E18 and G+0x1E20..+0x1E30 = P's physical position; otherwise G+0x1E18 = terrain height at G+0x1E08/+0x1E10 (0x35EA0) × 65536. Then G+0x1E20 is moved toward G+0x1E08 by 0x7D0B0 (fixed-point fractional approach). | Once. |
| 31 | 0xA7F47 | Diagnostic tick. | | |

### 3.4 Application of the received aggregate (data received during the previous frame's client service)

| # | RVA window | Phase | Behaviour, reads/writes | Gates / cadence |
| --- | --- | --- | --- | --- |
| 32 | 0xA7F50–0xA800F | Optional tap | Only when G+0x21F8 is nonnull: append own-player records (record owner equal to P's identifier P+0x08) of W+0x90, W+0x150 and W+0xD0 to G+0x2200, G+0x2210, G+0x2220. The constructor and session teardown store zero; no nonzero direct writer was found. | Normally skipped. |
| 33 | 0xA8038–0xA81DE | Received block edits | If G+0x19A0 is nonempty: diagnostic lines "CUBE CHANGES 1..4"; for each record in order, remove (0x91160) or set with colour (0xA37D0) the block at its X/Y/Z in W according to two flag bytes; append the record to the history (+0xD8) of the zone containing it (floor division by 64; zone lookup 0x16E00) when that zone exists. | Once per record. |
| 34 | 0xA81F0–0xA821B | Zone byte clears | For each zone key in G+0x19E0, clear byte +0xF8 of the existing zone. | Once per record. |
| 35 | 0xA8230–0xA82E2 | Zone array replacement A | For each G+0x19C0 record, empty the existing zone's array at +0x30 and insert the received data. | Once per record. |
| 36 | 0xA8300–0xA83DC | Zone array replacement B | For each G+0x19D0 record, empty the zone's array at +0x60, insert the received data, add the zone to W+0x1A0, then 0x31D470 and 0x31D700 on the zone. | Once per record. |
| 37 | 0xA842F–0xA8445 | G+0x1A30 records | For each record not having both identifiers equal to P's identifier, when its second identifier resolves to a creature, call 0x65390 on that creature with the record payload. Meaning unresolved. | Once per record. |
| 38 | 0xA8469–0xA8483 | Creature map updates | For each G+0x1A20 record whose creature resolves, store its dword value under its key in that creature's map +0xB88 (the map later read for boss detection, phase 84). | Once per record. |
| 39 | 0xA8498 | Source-array publication | 0x2E30B0(W) (world-step contract). | Once, unconditional. |
| 40 | 0xA84C3 | Membership insertion | Insert each G+0x1A00 record's identifier into W+0x20. | Once per record. |
| 41 | 0xA84D0 | Diagnostic tick. | | |
| 42 | 0xA84EE–0xA8622 | Chat record | At most one pending record of the queue G+0x21A0: when its identifier is non-negative and resolves to a creature, append that creature's name line to CH; append the message text; remove the record (0xA0670). Held Chat contract. | Once at most. |
| 43 | 0xA8652–0xA8686 | World text notices | Append every string record of W+0x180 to CH with a fixed colour. | Once per record. |
| 44 | 0xA86DB–0xA88F6 | Splices | Move all nodes of G+0x1980, G+0x1990, G+0x19A0, G+0x19F0, G+0x1A00, G+0x1A10, G+0x1A50, G+0x1A40 to the ends of W+0xA0, W+0xB0, W+0xC0, W+0x110, W+0x120, W+0x130, W+0x170, W+0x160 respectively, so later consumers see received and locally produced records together. | Once. |
| 45 | 0xA88FB–0xA8916 | Static countdown | Subtract the capped ms from the static dword at 0x5514F8, clamping at zero. No other direct reference exists; it starts at zero (zero-initialised data). | Once; no observable effect found. |
| 46 | 0xA8970–0xA8A99 | Own-player W+0x130 notices | For each W+0x130 record owned by P: a floating text (lifetime 3000) at P's position is appended to G+0x20B0 with text from 0x935A0, the same text goes to CH in a fixed colour, and sound 0x1F plays at the camera eye. Record meaning unresolved. | Once per record. |
| 47 | 0xA8AB1–0xA8FA6 | Received hits | For each G+0x1970 record whose target resolves: a target with HP ≤ 0 and type code +0x64 other than 0x11D joins a local death list. If the target is P: kind 1 applies a mana-like cost (P+0x188, P+0x158, P+0x184 and a stdout "mana cost" line); kind 5 decrements the float +0x14 of P's +0x980 records of type 6 (clamped, zeroed at ≤ 0) and appends a W+0x150 record for each; any other kind except 6, while P's HP is positive, reduces P+0x18C by 0.25 when the amount is positive (clamped at 0) and sets P+0x994 = 1.0, raises the stun timer P+0x130 to the record's integer when larger, subtracts the amount from P's HP and on a negative result sets HP to 0, appends a type-0x1B record at P's position to W+0xA0 and adds P to the death list, and finally clamps HP to the maximum. If the target is another creature and the amount is positive and the attacker is not P, set the target's +0x994 = 1.0. | Once per record. |
| 48 | 0xA9084–0xA9641 | Death break-up | For each death-list creature whose byte +0x1500 is nonzero and that passes the renderer visibility test 0xE08C0 with a size-based radius: emit body-part pieces through 0x90390 for its model parts (one part skipped when flag bit 0x400 of +0x80 is set); type code 0x90 also appends an effect to G+0x20A0. | Once per listed creature. |
| 49 | 0xA969D–0xA9725 | Forwarding into W | Append G+0x19B0 records not owned by P to W+0x60 (the live list traversed by 0x2DBA40 in later world steps); append G+0x1970 records not owned by P to W+0x90. | Once per record. |
| 50 | 0xA9745–0xA9766 | Optional hook | Control-flow guard check (no effect); when G+0x21F0 is nonnull, 0x14B160(G+0x21F0, W+0x90). No nonzero direct writer of G+0x21F0 was found. | Normally skipped. |
| 51 | 0xA979B–0xA9882 | Loot | For each W+0x170 identifier that resolves: 0x2D9480 produces a list; when nonempty, queue a narration entry (0x1694E0, whose numeric argument is the creature identifier modulo 100000, not a random draw) and call 0xA3170; then 0x98040(G, creature). | Once per record. |
| 52 | 0xA989A–0xA9979 | Received-aggregate reset | Construct sixteen empty containers and exchange them into G+0x1970..G+0x1A60, then destroy the old contents. | Once, unconditional. |

### 3.5 Targeting, local control and outer player-facing work

| # | RVA window | Phase | Behaviour, reads/writes | Gates / cadence |
| --- | --- | --- | --- | --- |
| 53 | 0xA9985–0xA9ABF | Candidate vector | For each creature of W+0x40 with HP > 0 whose distance from P (0x2B650 of the converted difference) is at most 25600.0 (binary32 0x438008, units of that helper not proved) and that passes 0xE08C0, append (creature, distance); then sort (0x7D720). | Once. |
| 54 | 0xA9AC4–0xA9B2C | Local control | 0x8AE50(G) only when W+0x210 is nonzero, P's ability is neither 0x9C nor 0x9D, and title, selection and creation are hidden (frame-order contract §3). | At most once. |
| 55 | 0xA9B3C–0xA9C32 | Debug placement | Only if the byte at 0x5514CE is nonzero: place P on the terrain and rewrite its orientation vectors. That byte has no direct writer and is zero-initialised. | Normally skipped. |
| 56 | 0xA9C4F–0xA9CE9 | Debug revive/travel | Only if the byte at 0x5514CF is nonzero (same writer status): restore non-positive HP to maximum, P+0x130 = −3000.0, and possibly enter ability 0x9C. | Normally skipped. |
| 57 | 0xA9D22 | Item tooltip | 0xB6B00(G): chooses the hovered/selected item among Ring candidates, the widget at G+0x380, inventory G+0x330, G+0x340, G+0x350, G+0x360 sources and updates the tooltip widget G+0x3C0. Internals bounded by fingerprint only. | Once. |
| 58 | 0xA9D4F–0xA9DD3 | Hint and map-dependent visibility | Each hint entry of G+0x228 is shown iff its "info" text is nonempty and the map is closed; node G+0x2C0 is shown iff the map is closed. | Once. |
| 59 | 0xA9DE6–0xA9E5D | General-dialog speaker override | Frame-order contract §5 (store 0xA9DF6). | Once. |
| 60 | 0xA9E62 | Diagnostic tick. | | |
| 61 | 0xA9E81–0xA9FF6 | Right stick | Frame-order contract §§2 stage 6, 5. Slowdown factor uses 1 − distance/50 floored at 0.3, times 0.4; exponent 3.0. | Once. |
| 62 | 0xAA036–0xAA1A2 | Waypoint distance label | If the node (G+0x380)+0x178 is shown and the map object's marker (map+0x110) is set, the text node G+0x448 shows the truncated value of (length of P-minus-marker difference) × 0.5; otherwise it is emptied. | Once. |
| 63 | 0xAA1B1 | Creature presentation 0xB0680(G, candidates). | | Once. |
| 64 | 0xAA1BC | Dialogue update 0xB40D0(G). | | Once. |
| 65 | 0xAA1FB–0xAA505 | Particle emitters | For each W+0xB0 record, append its count of particles to G+0x2090, each with several CRT random draws for offset, velocity and colour jitter; type 1 adds a further draw; type 5 sets a flag and draws a delay modulo 500. | Once per record. |
| 66 | 0xAA58F–0xAA5F3 | World sounds and shake | For each W+0xA0 record play its sound at its position (0xA0420); for codes 0x54 and 0x55 raise G+0x1E00 to max(current, 1 − distance/2500, floored at 0). | Once per record. |
| 67 | 0xAA628–0xAADA9 (+ undecoded 0xAA9FA–0xAAC3F) | Item pickups | For each W+0x110 record whose creature resolves: decode the item (0x10A720). If it is P: optional quest-map insertion (flag bit 2), per-zone counter increment for one item class, otherwise add to inventory P+0x9F0 (0x4E810) or, for value items, add 0x109070's value to gold P+0xAAC; refresh 0xB3ED0 and the inventory widget G+0x330; for special classes queue "define_item"/"you_received_the_item"/"you_received_an_item" narration (one random draw) with item-specific description (presentation-only switch); then, when P's ability is 0 or its timer exceeds 0x4F220(P), set the hold pose ability 0x78, 0x79 or 0x7C, copy the item to P+0x1D8, zero the timer, and set G+0x2050 = 60000 (0xAAD54). Independently, for P while G+0x340 is shown call 0xB2180; for every resolved creature call 0x9D6F0(G, item, count). | Once per record. |
| 68 | 0xAADBD | Combat texts 0xA0750 | Walks W+0x90 and appends "MISS", "BLOCK", "ABSORB", "TAMED!", "K.O." and damage floating texts / effects to G+0x20B0 / G+0x20A0. Bounded by fingerprint only. | Once. |
| 69 | 0xAADC5 | Reward popup 0xA1320 | If the node G+0x3B8 is hidden and the queue G+0x2250 is nonempty: pop one entry, fill the node's "info" and "xp" texts, clear its "shine" property and play sound 0x21 at the camera eye. | At most one per frame. |
| 70 | 0xAADCA–0xAAEA6 | Zone request list | Empty G+0x2240; only when W+0x358 is nonzero: append every existing zone within ±0x100 blocks of P (or of P+0x1B0/+0x1B4 during ability 0x9C) whose byte +0xF8 is nonzero. | List rebuilt once; filled only when W+0x358 ≠ 0. |
| 71 | 0xAAEBF | Client service 0x3F4F0(CL) | Drain CL's incoming queue (message handler fills G+0x1970..G+0x1A60 and writes W), timeouts, periodic state send (held host-thread contract §4.3). | Once, unconditional. |

### 3.6 Effects, camera, HUD, Interface, music and notices

| # | RVA window | Phase | Behaviour, reads/writes | Gates / cadence |
| --- | --- | --- | --- | --- |
| 72 | 0xAAED0–0xAAEF7 | Integer-ms agers | With the truncated integer ms: 0xB37F0 caps G+0x2090 at 1000 entries and advances particle motion/age; 0xB36A0 ages G+0x20A0 entries (+0x38 += ms) and removes those whose age exceeds +0x3C; 0xB6240 projects the floating texts G+0x20B0 through the matrix values stored at G+0x1F24..G+0x1FA0 as they stand before this frame's camera update (phase 73; whether 0xADCD0 is the writer of those fields was not checked) and updates them; 0xB03D0 runs the hold-progress timer of the panel G+0x348 (G+0x20CC/G+0x20D0, reset when hidden). | Once each. |
| 73 | 0xAAF02 | Camera update 0xADCD0(G) | Binary64 capped ms (frame-order contract §2 stage 8). | Once. |
| 74 | 0xAAF0C | Edit remesh scheduling 0xAF110(G, integer ms) | For each W+0xC0 edit, collects zones of the 3×3 neighbourhood (±13 blocks, floor division by 64) for re-meshing. Bounded by fingerprint. | Once. |
| 75 | 0xAAF1C onward | XP/level snapshot | Save total XP 0x666A0(P) (level P+0x144 plus P+0x148) and level P+0x144. | Once. |
| 76 | 0xAAF58 | W+0x120 records | 0x2C3470(W, record) per record; that routine performs a read-only lookup with no observable effect. | Once per record. |
| 77 | 0xAAF83–0xAB137 | HUD toggles | Only if CH+0x1D8 is zero and title, selection and creation are hidden: rising edges (statics 0x5514FC..0x551501, updated only while this gate passes) of G+0x1A7F → 0x1031E0, G+0x1A81 → 0x102FE0; each node in vector G+0x3C8..G+0x3D0 is shown iff G+0x330 is shown; G+0x1A82 → 0x102F10; G+0x348 shown iff G+0x340 shown; G+0x1A80 → toggle the Ring G+0x310; G+0x1A84 → 0x1030C0; G+0x1A83 → 0x103150. | Once. |
| 78 | 0xAB1A1–0xAB3BF | Map pointer or movement-close | If the map is open and G+0x1958 is nonzero: move the GUI cursor (0x3418D0) from G+0x1D60·G+0x1E48 and (1 − G+0x1E4C)·G+0x1D64, set G+0x1E38 = 1, and pan G+0x1E08 by a stick-derived vector scaled by 2^G+0x1E04 (0x862F0). Otherwise, when any of G+0x1A75..G+0x1A78 is set while CH is not typing, or the left-stick length (G+0x1D6C) exceeds 0.01: close the map (G+0x668 = 0), hide G+0x330, G+0x350, G+0x340, G+0x348, G+0x358, G+0x368, G+0x360, G+0x370, G+0x388, clear G+0x420, and if G+0x378 is shown empty its list (0x8ADF0). | Once. |
| 79 | 0xAB3F3–0xAB468 | G+0x378 panel | Hidden when its list is empty; otherwise shown unless some entry of the vector G+0x400 has +0x218 equal to P's identifier; when shown, 0x28ED90 on it. | Once. |
| 80 | 0xAB477–0xAB4B8 | Ring auto-close | If the Ring is shown and G+0x1A75 or G+0x1A76 is set, or stick Y G+0x1D70 is above 0.25 or below −0.25, hide it. | Once. |
| 81 | 0xAB4F9–0xAB5B6 | Map presentation smoothing | Map closed: when G+0x1D60 > 0, approach G+0x1E48/+0x1E4C toward values derived from G+0x1D60, G+0x1D64 and G+0x1BD0 (constants 0.3, 0.2); G+0x1E50 approaches G+0x1BD0. Map open: approach a near-zero pair and approach G+0x1E50 toward 1.8. | Once. |
| 82 | 0xAB5BB–0xAB5CE | Interface update 0x103200(I) | Including the late five-root publication (first-frame contract §3). | Once, unconditional. |
| 83 | 0xAB5D3–0xAB5DD | If G+0x669 is zero, G+0x21C0 = 0. | | Once. |
| 84 | 0xAB63F–0xAB65B | Boss detection | True when some creature with byte +0x60 equal to 1 has, in its map +0xB88, an entry for P's identifier, and 0x5EEE0 or 0x5EDA0 holds for it. | Once. |
| 85 | 0xAB69B–0xAC137 (+ undecoded 0xABDE2–0xABE33, 0xABECA–0xABF81) | Region labels and music track | Empty G+0x2D0 and G+0x2D8. During ability 0x9C G+0x2D8 shows "Arrival in N" seconds (0x2ADC50). Otherwise G+0x2D0 shows the localised region name and G+0x2D8 the site or location name; when that name differs from the static last name (0x551530) and 0x333ED0 on G+0x2E0 is false, the banner G+0x2E0 is filled ("name", empty "detail", "location" reset). Track choice: main theme at zone (0,0) with a site; shrine track when the cell flag 0x20 is set; otherwise a biome track then beach/ocean overrides by terrain height (< 10, < 0) then a site/faction track (presentation-only switches). Spirit-world item 0x13 labels G+0x2D8 and forces the deadlands track; boss forces the boss track. G+0x2050 and G+0x204C are decreased by the capped ms (clamped at 0); G+0x2050 > 0 forces the success track; a shown title, creation or selection root forces the main theme. | Once. |
| 86 | 0xAC1CD–0xAC3B5 | Music engine | Query the audio object (slot +0x28). When not playing: on a changed track store it and, under the lock G+0x1D38, set the path "Music\\"+track at G+0x2028; set the delay static 0x533934 (300000, or 10000 when the success timer is active); set volume G+0x2048 = G+0x1BA8 × 0.01 under the lock; reset counter 0x551504. When playing: advance 0x551504 by the capped ms, choose the delay rules, and approach G+0x2048 toward the volume target or 0.015 under the lock. | Once. |
| 87 | 0xAC3D7–0xAC734 | Quick-item hint | Clear hint entry 0's "count" and "info". Build the Quick-Item candidate list 0x93E60; when nonempty, wrap the index G+0x20C0 into range, show that item in G+0x3E0, its count when stackable, and its name. | Once. |
| 88 | 0xAC739–0xACE5C | Ring selector | If the Ring G+0x310 is hidden: clear static 0x551554, G+0x1E8C and G+0x1E90. Otherwise fill its texts ("You don't have any usable items." when empty), and with candidates scroll G+0x20C0 by G+0x1A77/G+0x1A78 or stick X with a 200 ms repeat per direction (statics 0x551558/0x55155C, sound 0x58 at the camera eye), approach the rotation G+0x1E8C and the opening scale G+0x1E90 (toward G+0x664 × 200); then show its "brackets" children iff the list is nonempty. | Once. |
| 89 | 0xACE6C–0xAD0E1 | XP notice | If total XP increased since phase 75: floating "+N" text (three random draws) and a CH line "You gain N XP.". | Once at most. |
| 90 | 0xAD112–0xAD2B3 | Gold notice | If P+0xAAC increased since phase 17: floating "+N GOLD" (three random draws). | Once at most. |
| 91 | 0xAD2C3–0xAD4A7 | Zone-counter notice | If 0x60EC0(P) increased since phase 17 and P's zone pair is unchanged since phase 17: floating "+N PLATINUM" (three random draws). | Once at most. |
| 92 | 0xAD4CD–0xAD71F | Level notice | If P+0x144 increased since phase 75: floating "LEVEL UP!" (three random draws), sound 0x1D, CH line "You reached level N.". | Once at most. |
| 93 | 0xAD745–0xAD752 | Renderer world-mismatch flag | When the wanted/loaded test of phase 25 fails (either comparison), zero renderer+0x4A4 (0xE8A30). | Only on mismatch. |
| 94 | 0xAD759–0xAD84C | Render flags and UI sound | Build scene flags: 0x08 and 0x80 from the debug bytes 0x5514C6/0x5514D6 (no direct writers), 0x20 when the GUI root M+0x140 is hidden or when title/creation/selection is shown, 0x10 from 0x9AEB0(M, 0x11), 0x02 when the map is open, 0x40 when the title is shown. Play sound 0x56 at the camera eye when "ordinary UI or Ring shown" becomes true and 0x57 when it becomes false (static 0x551560). | Once. |
| 95 | 0xAD85A–0xAD9D4 | Scene submission | Build the camera matrix block (0x295D00 from G+0x1D80), then 0xE2E00(renderer, eye G+0x1DC0, look-at G+0x1DD8, matrix, flags, view range max(3, G+0x1B9C × 30 / 100), map scale/offsets G+0x1E48/G+0x1E50, map focus G+0x1E20, zoom index G+0x1E04, effect lists G+0x2090/G+0x20A0, target data G+0x1E70, Ring state, and others). | Once. |
| 96 | 0xAD9D9–0xADAC3 | Profiling | Accumulate tick differences between the diagnostic samples into statics 0x551564..0x551590 and reset them after more than 20 frames. No reader that affects behaviour was identified. | Diagnostic. |
| 97 | 0xADAC9–0xADADB | Own-record queueing 0x3CC40(CL, W+0x90) | Only when CL's current-host identifier CL+0x70 passes its validity test: copy records owned by P from W+0x90, W+0xA0, W+0x150 and W+0xD0 into CL's pending lists CL+0x20, CL+0x30, CL+0x40, CL+0x50. Transmission happens in a later client service. | Once. |
| 98 | 0xADAE0–0xADB1A | World-aggregate reset | Replace W+0x90..W+0x180 with a fresh empty aggregate (0x83640) and destroy the old one. | Once, unconditional. |
| 99 | 0xADB23 | Leave W's World lock. | | Once. |
| 100 | 0xADB30–0xADBD7 | Local destruction, security check, return. | | |

### 3.7 Cross-frame consequences that follow from this order

* Network data drained at phase 71 of frame N is applied at phases 33–51 of frame N+1, after frame N+1's world batch and drawn-position pass and before its local control. The received aggregate is cleared at phase 52 of each frame.
* Records produced by the world batch of frame N (W+0x90..W+0x180) are consumed in the same frame (phases 43–51, 65–68, 74, 76), queued for the host at phase 97 only if owned by P, and discarded at phase 98. Received records spliced into W at phase 44 are consumed in the same way; phase 97 queues a record only when its owner field equals P's identifier and only from W+0x90, W+0xA0, W+0x150 and W+0xD0.
* The floating-text projection (phase 72) reads the stored matrix fields before this frame's camera update (phase 73); if the camera update is their writer, the projection uses the previous camera epoch (writer not verified).
* The XP and level notices compare values taken at phase 75 (after the client service and camera) with values at phases 89/92; gold and zone-counter notices compare with phase 17 (before the world batch).

## 4. Early returns inside the Game update (question 3)

0xA5A50 has exactly one return instruction (0xADBD7) and no direct call to a non-returning routine in its call records. Every branch reaches phase 99 and the return. There is therefore no early exit that skips later phases of the Game update. Skips are local branch decisions only: the world batch (phase 25), local control (phase 54), the HUD toggles (phase 77), and the other per-phase gates listed in section 3.

The only paths that leave before the end are: the frame driver not calling the Game update at all (renderer or Game global zero, section 2.3); shutdown in the message drain (no further frame); and exceptional runtime paths. The "list too long" length checks in several callees raise runtime errors that would abort the frame (the World lock would then not be released by this routine); these are not ordinary paths. Early returns inside callees (for example the four exits of 0x8AE50, frame-order contract §4) end only that callee.

## 5. Gate matrix by condition

| Condition | Effect inside one Game update |
| --- | --- |
| Title or selection shown | Frontend pose and orbiting camera (phase 9); lamp by night (10); flag 0x80 (24); local control skipped (54); HUD toggles skipped (77); main-theme track (85); scene flags 0x20 (and 0x40 for title). The world batch still runs when phase 25 admits. |
| Creation shown | As above except phase 9. |
| Map open (G+0x668) | Map-closed reset skipped (16); hint entries and G+0x2C0 hidden (58); with a pad as last device the pointer branch replaces movement-close (78); map smoothing targets (81); scene flag 0x02. Phase 15 is not gated by the map. |
| Chat typing (CH+0x1D8) | HUD toggles skipped (77); key-driven movement-close skipped but stick-driven close still possible (78); 0x8AE50's internal returns (frame-order contract §4). |
| Ordinary UI / retained focus | No direct gate in 0xA5A50 except the UI open/close sound (94); gates live in callees (0x8AE50, 0x9C050, Interface update). |
| Inactive or background window | The poll is skipped and 16 ms sleep is added (section 2.1), but the Game update runs fully. There is no pause. |
| Dead player (P HP ≤ 0) | Ability forced to 0 (14); excluded from the candidate vector (53 applies to all creatures); damage application to P skipped (47). No respawn logic was found in this routine. |
| Ability 0x9C | Travel tick sound (20); local control skipped (54); "Arrival in" label (85); zone-request centre (70). Ability 0x9D: local control skipped. |
| Wanted/loaded world mismatch | World batch skipped (25), mirror node zeroed (26), renderer flag zeroed (93); every other phase runs. |
| Role byte W+0x358 nonzero (any host switch, including to the local host) | Zone request list filled (70); inside the world step, the role-gated groups of the world-step contract. The Game update never ticks the in-process host H; that is the host thread. |
| CL+0x70 invalid | Own-record queueing skipped (97). |

Autosave, save timers, day-clock and death/respawn: no save or autosave call was identified in 0xA5A50's direct callee list. G+0x2050 (60000) and G+0x204C are music timers, not save timers. The day-clock advances only inside the world step (held clock contract); the Game update only reads W+0x0C for the frontend lamp. The in-process host's 60-second "state" save belongs to the host generation thread (held host-thread contract §2.3).

CRT random-number consumption in the Game update itself occurs at phases 20 (two draws), 28 (one), 65 (several per particle), 67 (one per narration), 89–92 (three each), plus whatever the callees draw. The per-callee draw counts were not closed.

## 6. Corrections to held contracts recorded here (held files unchanged)

1. First-frame contract §3 step 1 names the quit-request byte "G+0x3AD". The native operand at 0x136447 (and its retest in the drain) is byte offset **G+0x1D68**. The presentation indexes the Game pointer as an array of eight-byte words (0x3AD × 8 = 0x1D68). The same presentation convention explains its "+0x23" (I at G+0x118) and "+0x32B" (G+0x1958).
2. Host-thread contract §4.1 item 3 places the frame-limit sleep "after presenting". The native call order in 0x134420 is sleep at 0x134725, then the presentation slot call at 0x13474E.
3. Host-thread contract limit L6 (main-loop 16 ms sleep): the sleep is taken when the main window is not foreground or the ready byte 0x536CE8 is zero; that byte is initialised to 1 in the image and has no direct-address writer.

The gameplay frame-order contract's stages remain correct; this document supplies the intervening consumers it does not list. The first-frame contract's stage table (d through k) is consistent with phases 42, 54, 59–64, 72–76, 77–81, 82 and 95.

## 7. What remains unproved (question 4)

* The bodies of the four undecoded switch ranges (phases 28, 67, 85): exact case sets, string keys and any calls are from the longer-budget presentation only; no native instruction coverage exists in the export.
* Semantics of many containers and helpers named here only by observed use: G+0x6C8 list; the audio call at slot +0x08 (19); the 1,760-byte P copy (22); 0x5E430/0x36160/0x93480 discovery and classification; 0x65390 (37); W+0x130 notice records (46); 0x2D9480/0xA3170/0x98040 loot (51); 0xB6B00 tooltip, 0xA0750 combat texts, 0xB37F0/0xB6240/0xB03D0/0xAF110 internals (bounded by fingerprint, not instruction by instruction); 0x28ED90 and the identity of the G+0x378 panel; 0x333ED0; the music engine's audio-object slots; 0x14B160 and the objects G+0x21F0/G+0x21F8.
* Whether the guard byte 0x536CE8, the debug bytes 0x5514C6/0x5514CE/0x5514CF/0x5514D6 and the pointers G+0x21F0/G+0x21F8 can become nonzero through computed addresses, aliases or external injection. Only direct-address writers were searched.
* Which writers of total XP or level lie between phases 75 and 89/92; the snapshot window is established, its producers are not.
* Full internals of 0x8AE50, 0xB0680, 0xB40D0, 0xADCD0, 0x103200, 0xE2E00, 0x3F4F0 and the client message handler beyond the held contracts.
* Interface identity of the clear, fullscreen, resize and present slots; the meaning of format value 0x1C; 0x135310; the GUI cursor object's fields; OS message-queue behaviour (ordering, starvation by a continuous message stream, translated-character generation, repeat timing).
* The units of the candidate distance (25600.0) and the waypoint label (× 0.5); the exact smoothing factors passed in registers to 0x7D0B0, 0x7D050 and 0x16EE0.
* Exceptional paths: callee runtime errors, faults on null widgets or creatures, and behaviour when the World lock is left held by an aborted frame.
* Concurrency: the chunk, generation, map, AI, music and host threads run concurrently; only the World lock scope of this routine is asserted, not freedom from races with threads that do not take it (for example the music thread's own lock G+0x1D38 usage outside these phases).
* Callee-level random draws and their order, which affect the CRT random stream shared by all phases.
* No runtime observation confirms any timing or ordering statement; all are static.
