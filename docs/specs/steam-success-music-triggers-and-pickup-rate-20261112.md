# Steam success-music timer G+0x2050 triggers, and ground-coin pickup request rate

Status: source-side behavioural contract, code-free. Date: 2026-11-12.
Scope: the supported Steam release only. Question A: which game events reach each writer of the success-music timer G+0x2050 (value 60000). Question B: how many pickup interactions (kind 7) the client produces and sends when several coins are in reach, how it chooses and re-requests them, and how the host consumes a player's queued interactions. Conditional statements are source conditions, not runtime observations. Nothing here is implementation advice.

## 0. Authority, method, exposure

* Analysis image: C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe, SHA-256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244, image base 0x140000000. All addresses are RVAs of this image.
* Installed retail executable C:/Program Files (x86)/Steam/steamapps/common/Cube World/cubeworld.exe was hashed only: 5,939,072 bytes, SHA-256 A1EFBBD41EEB8DF08395C4D6511194D416CB70A14054B2A2000B9F5CDBCFA65B (same identity as the held contracts).
* Evidence: the whole-image instruction listing, the call-reference table, the per-function presentations, the string and string-reference tables of the original export, and raw binary32 constant reads from the analysis image. No executable was run.
* Held contracts read: docs/CLEANROOM_AUTHOR.md; the complete-phase-schedule contract (2026-11-12); the reward-popup-and-music-order contract (2026-11-12); loot-pickup.md; zone-object-interaction.md. Note: the two older held files contain a short "Errata (from the clean-room implementation)" section (loot-pickup.md) and one remark about the current port's handling of a field (zone-object-interaction.md, open question 9). They were read as part of the assigned files; none of that wording was used, and every fact below was re-derived from Steam evidence.
* Infinite-Cubes reconstruction: consulted as leads only (its game update around the discovery narration, its mission-tracking routine and mission/item/static-object identifier tables, and its static-object prompt for the goddess statue). Names such as "mission", "supplier gnome", "flute", "reward chest", "artifact", "special item", "glyph" are reconstruction labels and are marked "lead"; every behavioural statement was confirmed against Steam code.
* The coordinating harness injected the repository's top-level instruction text and branch name into this context at start without a tool read (workflow policy only). No engine, test, log, capture or runtime information was read or received.

Terminology as in the phase-schedule contract: G = Game object; W = the client World (G+0x08); P = local creature (W+0x448); PD = P's player-data block (P+0x1528); "phase N" = the numbered phase of the Game update 0xA5A50. The ability-duration helper 0x4F220 takes P and returns an integer duration (ms) for P's current ability; it is not decoded here. "Ability test" below always means: P's ability byte P+0x68 is zero, or P's ability timer P+0x6C (binary32 ms) is strictly greater than the binary32 conversion of 0x4F220(P) (comparison is unordered-false, so a NaN timer fails).

---

## A. Writers of G+0x2050

Four direct writers store 60000 (immediate 0xEA60): 0xA7471, 0xAAD54, 0x9A205, 0xB51D9, 0xB591D (the dialogue update has two). The Game constructor zeroes it (0x7EF39, held music contract). The music phase decrements it (held music contract §4.2). No other direct writer was found.

### A.1 Phase 28, region discovery (0xA7471)

**Cell (phase 23, instructions ending at 0xA723D; 0x36160 call at 0xA71DF).** Before the world batch, from P's position (P+0x10, P+0x18; 64-bit, 1/65536 block):
1. Each coordinate is converted to whole blocks by dividing by 65536 with truncation toward zero (a bias of 0xFFFF is added to negative values before an arithmetic shift by 16), then narrowed to 32 bits.
2. Each block coordinate is floor-divided by 1024 (true floor for negatives), giving (cx, cy).
3. 0x36160(W, cx, cy) is the world's "large cell" predicate: it hashes the pair (floor(cx/2), floor(cy/2)) and, when a hash bit is set, additionally requires 0x36210(W, cx). When it reports true, both cx and cy are rounded down to even (floor(c/2) × 2, true floor for negatives). Otherwise they are kept.
4. The result "already" = 0x5E430(P, cx, cy) is stored (0xA7224) before the batch.

**What 0x5E430 tests.** It returns false when PD is null; otherwise true when the pair (cx, cy) is a member of either of two ordered sets in PD: first the set at PD+0xC8, then the set at PD+0xD8 (pairs of 32-bit integers, compared lexicographically). Lead names: PD+0xC8 "solved missions", PD+0xD8 "finished missions". Every Steam inserter that adds to PD+0xC8 also adds the same key to PD+0xD8; some inserters add to PD+0xD8 only. So the test is "this 1024-block cell (or its even-rounded 2×2 large cell) is recorded as completed for this character, in either strength".

**Who fills these sets between phase 23 and phase 28.** At the end of every world step 0x2DA6C0 (0x2DB9E8) the routine 0x2E1750 runs when W+0x448 is non-null: it builds the region object for P's current region pair P+0x150/P+0x154 (0x115DF0) and calls 0x131620 on it. 0x131620 does nothing while P's ability is 0x9C. Otherwise, in this order, it inserts cells (each cell computed from the stated position by the same block conversion, floor division by 1024 and large-cell even rounding as above; "kind" is the region object's classification 0x93480 of that cell):
1. For every creature in W's creature list whose flags +0x80 have bit 0x2000, whose state +0x128 has bit 0x100, whose byte +0x19C is nonzero, and whose position is within squared distance 400.0 (0x42E7E8; binary32, per-axis delta × 1/65536) of P: cell from that creature's stored origin (+0x1B8, +0x1C0). Kind 0xF: insert into PD+0xD8. Kind 1 (lead: supplier gnome): insert into PD+0xD8 and PD+0xC8.
2. For the zones covering P ± 0x140000 (20 blocks) in x and y: every zone object of type 0x15 or 0x16 that takes an item (+0x38 nonzero) and whose state byte +0x28 is zero (given / opened), within squared distance 400.0 of P: cell from the object position; kind 0xF inserts into PD+0xD8. Every object of type 0x7E with state +0x28 zero (lead: brazier lit): if the kind is 0x16, insert into PD+0xD8 and PD+0xC8.
3. For each of the 7 × 7 zones around P's zone (zone = floor(block/64); offsets −3..+3 in x outer, y inner): when the zone exists, its id list (+0x498, count +0x4A0) is non-empty, and every id in it is a member of W's set W+0x20 (lead: dead creatures): cell from **P's own position**; kind 0xF, 0x10, 0x11 or 0x12 inserts into PD+0xD8 only; any other kind (including 0) inserts into both sets.
4. For every (cell, kind) record of the region object (its map at +0x10): kind 0xE when P holds the cell's artifact item (0x119660 then 0x5E780 with count 1); kinds 2..5 when P holds special item kind + 0xC; kind 6, 7, 8, 9, 0xA, 0xB, 0xC, 0xD when P holds special item 1, 2, 0x12, 3, 5, 7, 8, 9 respectively (0x5E890): insert that cell into both sets. Kind 0x12: when P's inventory count of the cell's item (0x11A6C0, 0x119950) exceeds 9, insert into PD+0xD8 only.
5. It then recounts kinds 0x13..0x17 to maintain P+0x128 bits 0x8000, 0x800 and 0x4000 and the two buff entries 0x21/0x22 (not otherwise relevant here).

The only other inserters of these sets are the character-data loader 0x147800 (reached from 0x9BB20) and nothing else by direct offset (the saver 0x149760 only reads them). Limit: inserts through a pointer computed from another base were not excluded beyond this census.

**Phase 28 test (0xA7454–0xA7471).** When "already" was false and 0x5E430(P, cx, cy) — the **same** (cx, cy) from phase 23, not recomputed from P's new position — is now true, G+0x2050 = 60000 at 0xA7471. This store is unconditional once the transition is seen: it precedes the classification (0x115DF0 for P's region, then 0x93480 on (cx, cy)) and happens for every kind, including kinds whose narration case is empty. The narration cases (kinds 1, 0x13, 0x14, 0x15 only with P+0x128 bit 0x4000, 0x16, 0x17; held schedule) follow.

Consequences: the timer is set by phase 28 exactly when a mission-completion record for the cell P stood in at the start of the frame is created during this frame's world batch (by 0x131620 in any world substep). Completions of other cells (for example section 4 above for a remote cell) do not trigger it. Fires in vanilla single-player: yes; 0x131620 runs on the client world for the local player in ordinary play.

### A.2 Phase 67, received item hold pose (0xAAD54)

Reached per pickup notice in W+0x110 whose picker resolves (0xAA628), after the item A = class-adapted notice item (0x10A720 with the picker, 0xAA647), only when the picker is P (0xAA650). The per-player check (notice flags bit 0x2 with a block key already in PD's map) can skip the whole notice first (0xAA6EC), in which case nothing below happens.

Receipt branch gate (0xAA840–0xAA857): the **notice's** raw item type byte is 0x1E (lead: generated treasure, later adapted to gear or a gold bag), **or** A's type is 0x17 or 0x18 (leads: artifact, special item), **or** A's type is 0x1D (lead: glyph). Any other item (coins 0x0C, mana cubes, ordinary gear or consumables not wrapped as 0x1E) skips the branch.

Inside the branch, after the narration is queued (one CRT draw modulo 10000, 0xAAC5B), the ability test is applied (0xAACB7–0xAACD1). If it fails, the branch ends without touching the pose or G+0x2050. If it passes:
* A's type 0x18 and subtype 2 (lead: boat): when P+0x5C has bit 0x2 set, ability ← 0x79, held item ← A, timer ← 0; when bit 0x2 is clear, the pose, held item and timer are **not** changed, but control still reaches the 60000 store.
* A's type 0x18 and subtype 1 (lead: hang glider): ability ← 0x78, held item P+0x1D8 ← A, timer ← 0.
* Every other receipt item (type 0x18 with another subtype, 0x17, 0x1D, or a 0x1E-wrapped item whatever it adapted to): ability ← 0x7C, held item ← A, timer ← 0.
* Then G+0x2050 = 60000 (0xAAD54).

Correction to loot-pickup.md §4 step 3.5 ("in every case … 60000"): the store is skipped when the ability test fails. Fires in vanilla single-player: yes, on picking up any treasure (0x1E), artifact, special item or glyph ground item while P is free; the notice arrives from the host through the client service like any pickup.

### A.3 0x98180 from local control (0x9A205): opening a reward chest

0x98180 is the use-prompt routine called by local control 0x8AE50 at 0x8DF69 on every pass of local control. Its "act" flag is a rising edge of the interact/use control byte G+0x1A7B (byte nonzero and the remembered previous value, static 0x5515B4, zero). It returns early, before any target handling, when G+0x1A7A or G+0x1A7D is nonzero or the general dialog node ((G+0x418)+0x178) is shown. Creature targets are handled before zone objects; the zone-object part runs only when no earlier creature branch returned.

The 60000 write is in the zone-object case for object **type 0x16** (lead: reward chest), at the object targeted by G+0x1E7C/G+0x1E80 (zone) and G+0x1E84 (index), resolved by 0x2C3150. Conditions in order:
1. The object does not take an item (an object with +0x38 nonzero and state +0x28 nonzero is handled earlier by the hand-in path, which queues a kind-5 record) and its state byte +0x28 is nonzero (0x99E99).
2. The cell key (0x137E0 of the object's x, 0x137E0 of its y) is **not** in PD's set PD+0xA8 (lead: opened chests) (0x99EF6–0x99EFE).
3. The zone exists and its content map +0x450 has an entry for the object index whose item type is nonzero (0x99F12–0x99F4B).
4. The prompt "Open" is shown. Only on the act edge (0x99FA8) does the rest run.

Then: the cell key is inserted into PD+0xA8; the content item is copied (the entry stays); a type 0x1E content is class-adapted for P (0x10A720); narration "define_item" + "you_received_the_item" (types 0x17, 0x18, or gear 3..9/0x1A with rarity +0x14 above 2) or "you_received_an_item", queued with one CRT draw modulo 10000. Then the ability test (0x9A1A2–0x9A1C7): only if it passes, ability ← 0x7C, timer ← 0, held item ← the item, and G+0x2050 = 60000 (0x9A205). Finally, regardless of the test: when the item's money value 0x109070 is nonzero, gold P+0xAAC gains it; otherwise the item is added to the inventory with count 1 (0x4E810) and announced (0x9D6F0, count 1). No interaction record is queued for this case; the opening is client-local and per character through PD+0xA8 (the zone object's state byte is not changed here).

Fires in vanilla single-player: yes, when the player presses use on an unopened type-0x16 chest whose zone content map holds an item. Limit: where the client world's zone content map +0x450 is filled was not traced here.

### A.4 Dialogue update 0xB40D0 (0xB51D9 and 0xB591D): page gives

Both writes are on the "give" of a dialog page (0x1B0-byte page; give count +0xD4, give item +0xD8; take count +0x30, take item +0x34), executed on the update where the container's current page (index +0x204) is valid and the container's page timer +0x200 equals 0.0 exactly. In both: if the give count is nonzero and P may carry the item (0x50670): add count × item (0x4E810), announce (0x9D6F0), then G+0x2050 = 60000 — this store is **not** gated by the ability test. When the count is 1 and the item type is 0x18, a receipt narration is queued (one CRT draw modulo 10000) and, if the ability test passes, ability ← 0x7C, timer ← 0, held item ← the item. If P cannot carry the item, nothing is given and the timer is untouched.

* 0xB51D9: the per-speaker speech-bubble containers (the G+0x400 list; pages at +0x1E8/+0x1F0). Order there: take, then give.
* 0xB591D: the general dialog window (G+0x418; pages at +0x1E8/+0x1F0). Order there: give, then take.

**Which dialogue outcomes set a give.** Census of every writer of a page's give fields (direct and end-relative offsets): (a) the integer-script page builder 0x168120, operations 8 (give the inline item × 1) and 9 (give n coins: type 0x0C, material 0x0B); its only caller is 0xB40D0 itself (0xB5283) with the script "item, phrase 0x85", which contains neither operation; the string-token page builder 0x1694E0 has no give/take operation and writes no give field; (b) 0x98180 zone-object case **type 0** (lead: goddess statue; prompt "Examine"; only when P does not already hold special item 4, 0x5E890): on the use-key edge it builds the "gift_of_the_goddess" pages into the dialog window (0x167C80) and sets the last page's give to item type 0x18, subtype 4 (lead: flute), count 1 (0x9A6A1–0x9A6D3).

Therefore in vanilla Steam the only traced dialogue give is the goddess statue's flute: when that last page becomes current in the dialog window, 0xB591D sets G+0x2050 = 60000 (and the flute is added, announced, narrated and held up if the ability test passes). Quest rewards, shop purchases and NPC gifts do not use this path (purchases and crafting go elsewhere and do not write G+0x2050). 0xB51D9 has no traced producer and is inert in vanilla as far as the census reaches. Fires in vanilla single-player: 0xB591D yes (goddess statue, once per character until special item 4 is held); 0xB51D9 not reached.

### A.5 Summary

| Writer | Event | Gated by ability test | Vanilla SP |
| --- | --- | --- | --- |
| 0xA7471 | Mission completion recorded this frame for the cell P occupied at frame start | No | Yes |
| 0xAAD54 | Local pickup notice of 0x1E (raw), or adapted 0x17/0x18/0x1D | Yes (boat without P+0x5C bit 0x2 still stores) | Yes |
| 0x9A205 | Use on an unopened type-0x16 chest with stored content | Yes | Yes |
| 0xB591D | Dialog-window page give (only producer: goddess statue → flute) | No | Yes |
| 0xB51D9 | Speech-bubble page give | No | No producer found |

---

## B. Ground-coin walk-over pickup rate

### B.1 Client: choosing and queuing (local control 0x8AE50, 0x8D4DE–0x8D733)

Once per pass of local control (at most once per frame, phase 54 and 0x8AE50's own early returns), the client scans ground items:
1. Zones: x from floor((P.x − 0x80000 as blocks)/64) to floor((P.x + 0x80000)/64), i.e. P ± 8 blocks (0x16E50 zone of position); for each, y over the same range in y. x is the outer loop and y the inner loop, both ascending. Missing zones are skipped.
2. Ground items of each zone in index order (0xE0-byte records, zone +0x30..+0x38).
3. An item must pass the original-record validity helper 0x2B8140.
4. Squared distance: per axis, (P position − item position) as a 64-bit integer converted to binary32 and multiplied by binary32 1/65536 (0x42E510, 0x37800000); summed as (dx² + dy²) + dz². The item is out of reach when this exceeds binary32 16.0 (0x4ACA34, 0x41800000), i.e. 4 blocks; the comparison is unordered-false, so a NaN sum counts as in reach.
5. Auto-pickup class: item type byte 0x0C or 0x0E (type − 12 with bit 0x2 masked is zero; leads: coin, mana cube), and ground flags +0xC4 bit 0x2 clear. Such an item whose pickup delay +0xC8 is not exactly 0.0 (or is NaN) is neither queued nor targeted this pass. Other items go to the use-key targeting path (squared reach 100.0, 0x42E5BC), not covered here.
6. For each qualifying item, a kind-7 interaction record is formed: item = copy of the raw ground item (not class-adapted), zone x/y at +0xA0/+0xA4, item index at +0xA8, kind byte 7 at +0xC4, +0xC6 = 1, other fields zero/−1 as constructed.
7. Duplicate suppression: P's interaction list (P+0xAB8, count P+0xAC0) is walked; if any pending record has kind 7, the same three dwords (zone x, zone y, index) and an equal item (0x4DF60), the new record is dropped. Otherwise it is appended at the **end** of the list.

There is no per-frame or per-update cap: every in-reach, landed coin not already pending is queued in the same pass, in zone/index order. There is no client-side prediction: the ground item is not removed or marked locally.

### B.2 Client: sending (client service 0x3F4F0, phase 71)

* The client world does not consume its own list: the interaction handler 0x2C3710 returns immediately when W+0x358 is set, without clearing the list (entry test of the byte at W+0x358 in 0x2C3710; the final clear is also skipped while it is set). The client connect routine 0x3D820 sets that byte (0x3DB2D). (W+0x358 being set for the single-player client is consistent with the in-process host path below; its exact setting moment for single-player was not otherwise traced.)
* Each call of the client service reads timeGetTime. When now − C+0x9EC (time of last receipt) exceeds 5000 ms (unsigned 32-bit, test against 0x1388), it takes the reconnect path 0x3D820 and sends nothing else this call (0x3F70F–0x3F726).
* Otherwise, when now − static 0x54DE00 exceeds 32 ms (unsigned 32-bit, strictly greater than 0x20; 0x3F87B–0x3F884) the static is set to now (0x3F88A) and, subject to a validity test of the destination identifier at C+0x70/C+0x74, a **send tick** runs: the state packet (id 3) is sent; then, if P's interaction list is non-empty, **exactly one** record — the front (oldest) — is copied, unlinked, freed, the count decremented, and sent as packet id 9 (0x3F4F0 presentation lines for the pop; packet built with 0x3A790, dispatched by 0x3F460). When the destination equals the in-process identifier (static 0x54DDC8), packets go to the in-process host object (C+0xA10) instead of the Steam networking interface; this is the single-player path.
* So the client sends at most one interaction per send tick, and send ticks are at least 33 ms apart (and no more often than once per Game update, since the service runs once per frame). With N coins queued at once, the last is sent about N send ticks later.

### B.3 Client: re-requests

The pending-list check of B.1 step 7 only compares against records still in the list. Once a record has been popped and sent, the same coin (still present in the client's zone data) qualifies again on the next pass and is appended again at the end of the list. Re-queuing continues every frame until the client's ground-item array for that zone is replaced by host data (held schedule phase 35, records G+0x19C0 replacing zone +0x30) or the coin otherwise leaves the scan conditions. No timer, counter or limit governs re-requests besides the 32 ms send spacing and the one-record-per-tick drain.

### B.4 Host: receiving and processing

* Host message handler 0xF4490, case 9 (0xF49D6–0xF4AAF): when the sender is a known connection, the record is decoded (0xF0A10) and, if the sender's creature exists, appended at the end of that creature's interaction list (+0xAB8). No count limit, no duplicate check.
* Host world step: the interaction handler 0x2C3710 runs once per world step for each living creature (held zone-object-interaction contract §1). On the host the remote flag is clear, so it walks the **whole** list front to back, handling every record by kind, and then empties the list (0x3C9C0), handled or not. All interactions queued for a player since the previous step are therefore processed in that step, in arrival order.
* Kind 7 on the host (held zone-object-interaction §6, consistent with Steam): range test squared distance ≤ 16.0 against the host's copy of the player's position; item equality against the ground item at the index; on success one CRT draw (pickup sound pitch), a pickup notice to W+0x110, and, unless ground flag bit 0x2 is set, the ground item is emptied and its zone marked changed (W+0x190). A later duplicate request for the same index fails the item-equality test (the slot is empty) and has no effect, no draw and no notice.
* Limit: the cadence of host world steps relative to packet arrival belongs to the held in-process host-thread contract and was not re-traced here.

### B.5 Constants (B)

| Value | Source | Use |
| --- | --- | --- |
| 16.0 (0x41800000) | 0x4ACA34 | auto-pickup reach² (client) and kind-7 reach² (host) |
| 1/65536 (0x37800000) | 0x42E510 | position units to blocks |
| 0x80000 | immediate | zone scan half-width, 8 blocks |
| 0.0 | register | pickup delay must equal 0.0 |
| 0x20 (32 ms, strictly greater) | 0x3F881 | send-tick spacing |
| 0x1388 (5000 ms, strictly greater → reconnect) | 0x3F70F | receive timeout gate |
| 1 | — | interactions popped per send tick |
| 100.0 (0x42C80000) | 0x42E5BC | non-auto item targeting reach² (context only) |

---

## C. Unresolved

* 0x4F220 internals (duration per ability) and 0x93480/0x115DF0 classification internals; mission kind meanings are leads only.
* The source of the client zone content map +0x450 used by the type-0x16 chest path.
* Whether a computed-address writer fills a speech-bubble page's give fields (would activate 0xB51D9); none found.
* Who advances the dialog page timer +0x200 away from 0.0 (governs single execution of a page give).
* The destination-identifier validity test in 0x3F4F0 for the in-process identifier (inferred to pass, since single-player state traffic must flow).
* Host step cadence versus packet arrival (held host-thread contract).
* No runtime observation confirms any statement here.

FINAL / STABLE / HELD
