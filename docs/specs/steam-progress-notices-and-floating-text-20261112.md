# Steam progress notices (feat, XP, gold, platinum, level) and their floating texts

Status: FINAL / STABLE / HELD (source-side behavioural contract, code-free).
Date: 2026-11-12.
Scope: the supported Steam release only. Phases 17, 46, 72, 75 and 89–92 of the Game update 0xA5A50 (numbering of the held phase-schedule contract), the floating-text list G+0x20B0, its per-frame consumer 0xB6240, the chat (CH) lines and sounds of these notices, the compared quantities and their producers. Nothing here is implementation advice.

## 0. Authority, method and exposure

Authority: analysis image `decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe`, 5,757,952 bytes, SHA-256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244 (re-hashed for this contract), image base 0x140000000. The installed retail `cubeworld.exe` was hashed only: 5,939,072 bytes, SHA-256 A1EFBBD41EEB8DF08395C4D6511194D416CB70A14054B2A2000B9F5CDBCFA65B. All addresses are RVAs of the analysis image. No executable was run.

Evidence used: the per-owner instruction listing of 0xA5A50 (native operands decide wherever presentation and listing differ), the whole-image instruction listing, the call-reference table, per-function presentations of the helpers named below, the image's string table, and raw constant reads from the image's data sections (floats, doubles, strings, import names). Constants quoted as floats were read as binary32 from the image.

Reference sources read: docs/CLEANROOM_AUTHOR.md; the four assigned held contracts (complete phase schedule, gameplay frame order, sound, chat/feat presentation); the original evidence above. The Infinite-Cubes reconstruction was consulted as leads only (its Game update notice block, its total-XP and level helpers, and its textFX drawing block). Every fact stated below as Steam behaviour was confirmed against the Steam image; where the reconstruction agrees it adds nothing. Reconstruction-only items (names, not rules) are marked as such in §8.3 and §10. The harness injected the repository's top-level project instruction text and the branch name into this context at session start without a tool read; it carries workflow policy only. No engine, test, exporter, log, capture, owner asset or runtime information was read or received. No other agent was used.

Terminology follows the held phase-schedule contract: G is the Game object, W the client World (G+0x08), P the local creature (W+0x448), CH the Chat widget (G+0x398), "capped ms" the frame interval after the 1000 cap. "Fixed" positions are signed 64-bit integers with 65,536 units per block. Screen Y grows downward.

## 1. Corrections to the task wording and to held contracts (held files unchanged)

1. The XP floating text is **"+N XP"**, not "+N": phase 89 inserts the narrow string " XP" (0x43695C) after the number (0xACFC8–0xACFCF).
2. The level chat line is **"You reached level N!"** followed by a line feed, not "…level N.": the terminator is the narrow string "!" (0x4369B4, inserted at 0xAD6A5–0xAD6AC), then the wide-stream end-of-line manipulator 0x7D2F0 (inserts L'\n' and flushes).
3. The XP chat line is "You gain N XP." followed by a line feed (the same manipulator, 0xAD079–0xAD080). The held schedule's quoted lines omit the line feed.
4. Sound-contract §5.1 calls 0x1F "the mission-complete banner"; the native call site (0xA8A84–0xA8A8C) is the phase-46 W+0x130 notice whose text is the literal "FEAT!" (0x935A0), as the held chat/feat contract states. Only the label differs; the number and routine agree.

## 2. Floating-text record layout (list G+0x20B0)

G+0x20B0 is a doubly linked list (sentinel pointer at G+0x20B0, unsigned 64-bit count at G+0x20B8). Each node holds two link pointers followed by a 104-byte payload, copied in by value on append (0xA1920 → node constructor 0x6E7B0). Payload layout, offsets from the payload start:

| Offset | Width / type | Meaning |
| --- | --- | --- |
| +0x00 | signed 32 | Age in ms. Not set by the constructor; every creator below stores 0. |
| +0x04 | binary32 | Total screen drift over the lifetime, pixels (negative = upward). Constructor −100.0. |
| +0x08/+0x10/+0x18 | 3 × signed 64 | Fixed world anchor X, Y, Z. |
| +0x20 | binary32 | Screen X offset, pixels. |
| +0x24 | binary32 | Screen Y offset, pixels. |
| +0x28..+0x34 | 4 × binary32 | RGBA colour, components in 0..1. |
| +0x38 | wide string (32-byte small-buffer string; length +0x48, capacity +0x50, constructor capacity 7) | Text. |
| +0x58 | signed 32 | Lifetime in ms. Constructor 1000. |
| +0x5C | binary32 | Font pixel size. |
| +0x60 | signed 32 | Text-shape flags passed to the TextShape. Constructor 0. |

Constructor 0x80A40 sets only +0x04, the empty string, +0x58 and +0x60. The record is destroyed (string released) by 0x82810 after the copy has been appended.

Other creator: the combat-text routine 0xA0750 (phase 68) appends records of this same 104-byte layout to the same list (append helper 0xA1920 at 0xA127C for damage/MISS/BLOCK/ABSORB texts, and an inlined list insertion through the same node constructor 0x6E7B0 for "K.O."). It builds them field by field without 0x80A40, typically with lifetime 1000, font 20 or 60 and flags 1. Its contents are outside this contract.

## 3. Creators

### 3.1 Common properties of phases 46 and 89–92

* Each creator builds one record on the stack, appends a copy to the **end** of G+0x20B0, then destroys the stack record. There is no deduplication, no merging with an existing record and no list cap other than the library's maximum-length check (0x222222222222221 elements, which raises a length error).
* Age +0x00 = 0 (phase 46 stores R12D, which is 0 on every path from the routine's entry at 0xA5AF0 and 0xA81E4; phases 89–92 store ESI, which is 0 on every path from 0xACDA4/0xACDBD). Drift +0x04 = −100.0. Screen X offset +0x20 = +5.0. Lifetime +0x58 = 3000. Flags +0x60 = 0 (constructor value; no creator writes it).
* P is re-read from W+0x448 at every use; no creator caches it across phases.
* The anchor is P's physical fixed position P+0x10/+0x18/+0x20 **at creation time**, copied by value. The record does not follow P afterwards.
* Text strings are built with a wide string stream (constructed by 0x7DB40). Narrow literals are widened character by character through the stream (0x6D8E0); numbers are inserted with the imported wide-stream signed 32-bit integer inserter (import at 0x42C580, mangled name ends "@H@Z", i.e. `int`), so default stream formatting applies: decimal, no width, no padding, no explicit plus sign other than the literal "+", minus sign only if the value is negative. The stream's locale was not changed by these routines (see §10).

### 3.2 The three random draws (phases 89, 90, 91, 92)

Each of phases 89–92 makes exactly three calls to the CRT `rand` (import at 0x42CD28), in this order, immediately after storing the colour and before any other observable effect of that phase. Let r1, r2, r3 be the three results (0..32767). All arithmetic is binary32, one rounding per operation:

| Draw | Formula (binary32) | Range | Becomes |
| --- | --- | --- | --- |
| r1 (first) | ((float)r1 + (float)r1) / 32767.0 − 1.0 | −1 … +1 | offset **Y** component |
| r2 (second) | ((float)r2 + (float)r2) / 32767.0 − 1.0 | −1 … +1 | offset **X** component |
| r3 (third) | (float)r3 / 32767.0 | 0 … +1 | offset **Z** component |

(The divisor 32767.0 is binary32 0x46FFFE00 at 0x42E5F0, held in XMM14; 1.0 is 0x3F800000 at 0x4AC980, held in XMM8; both are loaded once at the routine's entry, 0xA706D/0xA7076, and never rewritten.) The resulting block-unit vector (X, Y, Z) is converted to fixed by 0x6D240: each component is multiplied in binary32 by 65536.0 (0x42E5F8) and truncated toward zero to signed 64 (CVTTSS2SI). That fixed offset is added component-wise (signed 64) to P+0x10/+0x18/+0x20 by 0x83C30, and the sum is the anchor. Thus the anchor lies up to one block either side of P horizontally and 0 to 1 block above P (Z is vertical). The draws perturb only the anchor; colour, offsets, size and lifetime are not randomised. Phase 46 makes no random draw.

The draws come from the single CRT stream shared with every other `rand` caller of the process (held schedule §5).

### 3.3 Per-creator fields

| Field | Phase 46 (0xA8969–0xA8A99) | Phase 89 XP (0xACE7E–0xAD0E1) | Phase 90 gold (0xAD10B–0xAD2B3) | Phase 91 platinum (0xAD2FB–0xAD4A7) | Phase 92 level (0xAD4C6–0xAD71F) |
| --- | --- | --- | --- | --- | --- |
| Admission | W+0x130 record whose first payload qword equals P+0x08 (P's identifier); once per such record | total XP now > total XP at phase 75 (signed 32) | P+0xAAC now > value at phase 17 (signed 32) | 0x60EC0(P) now > value at phase 17 (signed 32) **and** P+0x150/+0x154 both equal their phase-17 values | P+0x144 now > value at phase 75 (signed 32) |
| Text | "FEAT!" (0x935A0) | "+" N " XP" | "+" N " GOLD" | "+" N " PLATINUM" | "LEVEL UP!" |
| N | — | new total XP − snapshot (signed 32 subtraction) | new gold − snapshot | new counter − snapshot | — (no number in the floating text) |
| RGBA stored | (0.7, 0.2, 1.0, 1.0) from 0x4328AC, 0x42E700, 1.0, 1.0 | (0.65, 0.4, 1.0, 1.0) from 0x42E70C, XMM11 = 0.4 (0x42E52C), 1.0, 1.0 | (1.0, 0.9, 0.5, 1.0): 1.0, XMM9 = 0.9 (0x42E534), XMM10 = 0.5 (0x4AC964), 1.0 | (0.8, 0.9, 0.9, 1.0): 0x42E530, XMM9, XMM9, 1.0 | (1.0, 0.9, 0.5, 1.0) as gold |
| Anchor | P+0x10/+0x18/+0x20 exactly | P position + random offset (§3.2) | same | same | same |
| Screen offset (X, Y) | (+5, −100) | (+5, −100) | (+5, −100) | (+5, −100) | (+5, **−150**) (0x438040) |
| Drift | −100 | −100 | −100 | −100 | −100 |
| Font pixel size | **30** | 16 | 16 | 16 | 16 |
| Lifetime / age / flags | 3000 / 0 / 0 | same | same | same | same |
| Chat line | "FEAT!" + LF, RGB(200, 100, 255) | "You gain N XP." + LF, RGB(150, 128, 255) | none | none | "You reached level N!" + LF, RGB(255, 220, 128), N = new level |
| Sound | 0x1F via 0xA03F0 after the chat line | none | none | none | 0x1D via 0xA03F0 (§6) |

Register provenance of the colour components (each value checked on every control-flow path into the phase): XMM11 = 0.4 is loaded at 0xA9E6B and reloaded with the same constant at 0xAA532, with no path skipping 0xA9E6B; XMM10 = 0.5 is loaded at 0xAB21B on the map-pointer branch and at 0xAB3C4 on the other branch of phase 78, with no path into 0xAB3C5–0xAD4E2 that skips both; XMM9 = 0.9 is loaded at 0xAD0F7, which every path to phases 90–92 passes (the phase-89 skip jumps to 0xAD0E6).

Order of observable effects inside each phase:

* Phase 46: build record → append → chat line → sound 0x1F.
* Phase 89: three draws → build and append record → chat line.
* Phases 90, 91: three draws → build and append record.
* Phase 92: three draws → build record and text → **sound 0x1D** → append record → chat line.

Phases 89–92 run unconditionally in that order once per Game update, each at most once per frame. Several may fire in one frame; each then consumes its own three draws, in phase order.

## 4. Per-frame ageing, projection and drawing (0xB6240, phase 72)

0xB6240(G, ms) is called once per Game update at 0xAAEED with the capped frame interval truncated toward zero to signed 32 (held schedule phase 72). It runs before this frame's camera update (phase 73) and before phases 89–92, so a record appended in frame N is first drawn in frame N+1 with age 0. The held chat/feat contract §§4.1–4.4 already specifies the projection arithmetic, the textFX container and the TextShape path for the feat record; this section re-verifies and generalises it for every record.

1. **Container rebuild.** On entry, every child of the GUI container G+0x670 ("textFX") is removed (0x330A80). textFX is the first child of the GUI scene root (held chat contract §4.3), so all floating texts paint before lifebars, before the whole game-UI subtree (including CH) and before the frontend, options, controls and cursor roots; later siblings paint over it.
2. **Traversal.** Records are visited in list order (oldest first).
3. **Projection.** X and Y: record fixed X/Y plus the signed 64 values G+0x1DF0/G+0x1DF8; Z unchanged. Each completed sum is converted to binary32 and multiplied by 1/65536 (0x42E510). The point is transformed by the 4×4 matrix at G+0x1F24 and divided by its W through one reciprocal; it is drawn only if the divided third component is strictly positive (a NaN comparison fails). The result is transformed by the matrix at G+0x1F64 and divided by its W. Screen X = (W_px × 0.5) × projX + W_px × 0.5; screen Y = (−H_px × 0.5) × projY + H_px × 0.5, with W_px/H_px the signed 32 values at GUI engine +0x1C4/+0x1C8. Exact operation order: held chat contract §4.2.
4. **No other culling.** There is no screen-edge clamp, off-screen test, terrain occlusion, distance fade or size scaling with distance. A point behind the camera (failed test) draws nothing this frame but still ages.
5. **Fade parameter.** q = (float)age / (float)lifetime, using the age **before** this frame's increment; f = 1 − q.
6. **Placement.** Pixel position = (screenX + offsetX, screenY + q × drift + offsetY). For phases 89–91 the text starts 5 px right of and 100 px above the projected anchor and rises another 100 px over 3000 ms (about 0.0333 px per ms, linear, no velocity or gravity). Level: starts 150 px above, also rising 100 px. Feat: as XP but font 30.
7. **Two TextShapes per record**, created fresh each frame by 0x33DA70 (a new TextShape constructed by 0x367590 with the record text), attached under textFX by 0x33CD30 and positioned by 0x334510 at the same pixel position:
   * Outline shape, created first: fill colour ("colors") = record RGBA × 0, i.e. transparent; stroke colour ("strokeColors") = (0.1 R, 0.1 G, 0.1 B, A × f); stroke radius (TextShape +0x2BC) = max(fontSize × 0.125, 3.0) (0x42EEC0, 0x42E580; strictly, 3.0 is used when fontSize × 0.125 ≤ 3.0); font size (+0x2B8) = record font size; font name (+0x2C8) "resource1.dat"; flags (+0x2F8) = record +0x60.
   * Fill shape, created second: fill = (R, G, B, A × f); font size, font name and flags as above; stroke radius and stroke colour stay at constructor defaults (radius 0, stroke colour (0, 0, 0, 1) from 0x42E660).
   Each shape is then refreshed through its virtual slot +0x08 with argument 1.
   For phases 89–92 (font 16) the outline radius is therefore 3.0; for the feat (font 30) 3.75. Example drawn colours at age 0: XP fill (0.65, 0.4, 1, 1) with outline (0.065, 0.04, 0.1, 1); gold/level fill (1, 0.9, 0.5, 1) with outline (0.1, 0.09, 0.05, 1); platinum fill (0.8, 0.9, 0.9, 1) with outline (0.08, 0.09, 0.09, 1).
   Paint order inside textFX is attachment order: record k outline, record k fill, record k+1 outline, record k+1 fill, … — a younger record paints over an older one.
   The pixel-font raster path and rounding are held chat contract §4.4 and the plasma text contracts.
8. **Ageing.** After the attempted drawing of each record, age += ms (signed 32 addition).
9. **Removal.** A record whose age is now **strictly greater** than its lifetime is collected into a temporary list during traversal and erased from G+0x20B0 after the traversal (0x933E0 per node). With lifetime 3000, a record whose age equals 3000 exactly survives one more frame and is drawn with alpha 0.
10. **Gates.** 0xB6240 runs every Game update regardless of title/selection/creation roots, map, chat typing, inactive window or GUI visibility (held schedule §5; held chat contract §4.3). Hiding the GUI scene root hides textFX but does not pause ageing.

There is no scale animation, no per-record velocity field and no alpha other than the linear 1 − q.

## 5. The chat lines

All chat lines in this contract go through ChatWidget append 0x26B670(CH, wide text, three RGB bytes) (held chat contract §2.1 for tokenising, row creation and wrapping; the colour bytes are stored and drawn as byte/255 with alpha 1). The colour argument is built by 0x4CE70, which copies three bytes.

| Phase | Exact wide text passed | RGB bytes | Built at |
| --- | --- | --- | --- |
| 46 | "FEAT!" followed by U+000A (concatenation 0x6E070 with the wide literal at 0x4361F0) | (200, 100, 255) | 0xA8A37–0xA8A72 |
| 89 | "You gain " + N + " XP." + U+000A (stream reset to empty with 0xA58A0 first; wide literals 0x436978 and 0x436968; N as in §3.3; LF from end-of-line manipulator 0x7D2F0) | (150, 128, 255) | 0xAD015–0xAD0BA |
| 92 | "You reached level " + N + "!" + U+000A (wide literal 0x4369B8, N = P+0x144 as read in phase 92, narrow "!" 0x4369B4, LF from 0x7D2F0) | (255, 220, 128) | 0xAD650–0xAD6F8 |

The trailing LF ends the row; per the held chat tokeniser it creates a new empty row after the line. Phases 90 and 91 append nothing to chat.

## 6. Sounds

Phase 92 calls 0xA03F0(G, 0x1D) at 0xAD60A; 0xA03F0 is 0xA0420(G, number, position = listener G+0x1DC0 (the camera eye), volume 1.0, pitch 1.0, final flag 1) (presentation of 0xA03F0; constants 0x4AC980). Per the held sound contract §2 the position equals the listener, so distance attenuation is 1 and the final volume is soundVolume × 0.01 at pitch 1. It plays once per level notice, before the record is appended and before the chat line. Phase 46 plays 0x1F the same way (0xA8A84–0xA8A8C), once per admitted record. Phases 89, 90 and 91 contain no call to 0xA03F0, 0xA0420 or any other sound routine.

## 7. Compared quantities

### 7.1 Snapshots

| Phase | RVA | Stored (stack slot) |
| --- | --- | --- |
| 17 | 0xA6FFB–0xA7029 | gold P+0xAAC (signed 32) first; then 0x60EC0(P) (signed 32); then the zone pair P+0x150, P+0x154 (two signed 32) |
| 75 | 0xAAF1C–0xAAF35 | total XP 0x666A0(P) (signed 32); then level P+0x144 (signed 32) |

All comparisons are signed 32 "greater than" (the skip branches are JLE). The difference N is a 32-bit wrapping subtraction printed as signed; if a value moved across more than 2^31 − 1 the printed N would be negative after the literal "+" (no ordinary producer does this).

### 7.2 Total XP 0x666A0

Total XP = P+0x148 + Σ over k = 1 … L − 1 of trunc(10 × 2^((k − 1) × 0.5)), where L = P+0x144 (signed 32). If L ≤ 1 the sum is empty and the result is P+0x148. Each term: the exponent (k − 1) × 0.5 is formed in binary32 (k − 1 converted to float, times 0.5 from 0x4AC964), widened to binary64, raised as 2.0^exponent (base 0x4AC9D8 = 2.0) in binary64, multiplied by 10.0 (0x4AC9F8), truncated toward zero to signed 32 and accumulated with 32-bit wrapping addition. The image evaluates pairs of terms with a two-lane vector power routine (0x393480, multiplying by −10.0 and subtracting, which is the same truncation) and any remaining term with the scalar CRT power routine (0x39548C); see §10 for the equivalence limit. First terms: 10, 14, 20, 28, 40, 56, 80, 113, 160, 226 (so L = 2 adds 10, L = 3 adds 24, L = 4 adds 44). P+0x148 is a signed 32 field.

### 7.3 Gold P+0xAAC

A signed 32 field of the creature. Increases are compared at phase 90 against phase 17.

### 7.4 The "platinum" quantity 0x60EC0(P)

0x60EC0 returns 0 when P+0x1528 (P's player-data block) is null. Otherwise it looks up, in the ordered map at (P+0x1528)+0xF8 whose key is a pair of signed 32 (compared lexicographically, first then second) and whose value is a 32-bit count at node +0x24, the key equal to P's **current** zone pair (P+0x150, P+0x154); it returns that count, or 0 when the key is absent. It is therefore a per-zone counter of the zone P is in.

The only gameplay writer is the item-pickup phase 67 (0xAA721–0xAA782): for a pickup record resolving to P whose decoded item has category byte +0x00 = 0x0C and material byte +0x1C = 0x0D, the record's 16-bit count (+0xB8) is added to the map entry for P's current zone pair (inserted with 0 if absent, 0x83B20). Category 0x0C is the coin category and material 0x0D is the one the material-name routine 0x16B8A0 maps to "MaterialPlatinum" (its table is indexed by material − 1; index 0x0C → "MaterialPlatinum"). Hence the notice says PLATINUM: the counter counts platinum coins collected **per zone**, not gold. Such coins are not added to the inventory or to gold. The value at node +0x24 is read as a signed 32 for the comparison.

The phase-91 zone condition: both P+0x150 and P+0x154 now must equal the pair saved at phase 17 (0x83B00 compares the two dwords). If P changed zone during the frame, no platinum notice is produced even if coins were collected, because the before and after counters belong to different zones. P+0x150/+0x154 are signed 32.

### 7.5 Level P+0x144

A signed 32 field. For P it is recomputed by 0x66850 (§8.3).

## 8. Producers and whether they fall in a comparison window

Windows (held schedule §3.7): gold and platinum compare phase 17 with phases 90/91; XP and level compare phase 75 with phases 89/92. An increase made outside its window, including anything done during window-message dispatch or polled input before the Game update (main-loop steps 2 and 5, held schedule §2.1), is silently absorbed by the next snapshot and never produces a notice. Writers were found by a whole-image search for direct-offset accesses; §10 lists what this does not exclude.

### 8.1 Gold P+0xAAC

| Writer | Effect | When it runs | Notice? |
| --- | --- | --- | --- |
| Phase 67 item pickups, 0xAA786–0xAA7A2 | For a pickup record resolving to P whose item value 0x109070 is nonzero (only category 0x1F has a value; formula in 0x109070), add that value | phase 67 (local and received pickup records spliced at phase 44) | yes, at phase 90 of the same frame |
| 0x98180 at 0x9A20F–0x9A236 | Same rule (value of a category-0x1F item added to gold, otherwise the item goes to the inventory) for an item taken through the local use prompt | inside local control 0x8AE50, phase 54 (only caller 0x8DF69) | yes |
| 0x94900 at 0x95132 and 0x95C78 | Adds 0x10A390 of an item (shop/inventory sell actions) | mouse-down handler, called only from the window owner 0x133C70 (message drain) and the polled input of 0x1355A0 (main-loop steps 2 and 5) | no |
| 0x94900 at 0x94CC1, 0x95ECD, 0x96118; 0x965E0 at 0x969BF; 0x89CB0 at 0x89FFF (−10, via 0x965E0) | Subtractions (purchases, fees) | same input dispatch | never an increase |
| Load 0x147800 at 0x147E77, with clamp at 0x147EB4–0x147ECA | Gold read from saved data; a value that is negative as signed 32 is replaced by 0x540BE400 (1,410,065,408) | character load (callers 0x9BB20 from start-up 0x7EA80, input handler 0x965E0, GUI callbacks 0x9F7C0 and 0x9FDF0) | not established (§10) |
| 0x9EA10 at 0x9EE2A and 0xA27B0 at 0xA290C | Copy gold from one creature to another when the local creature is rebuilt | GUI callbacks (0x9EA10 registered at 0xFCD14, 0x9F7C0 at 0xFCBA4 in UI construction 0xF9AC0; 0x9FDF0 reached from 0x29234C) and 0x84F70 | not established (§10) |
| 0x2E17D0 at 0x2E25AE | Initial gold of a newly allocated spawned creature | world step (phase 25) | not P |
| 0x4CE80 at 0x4CFB1 | Constructor zero | creature construction | not an increase |

The world step (phase 25), loot (phase 51), dialogue (phase 64), client service (phase 71) and Interface update (phase 82) contain no direct writer of +0xAAC.

### 8.2 Platinum counter (P+0x1528)+0xF8

| Writer | When | Notice? |
| --- | --- | --- |
| Phase 67 (§7.4) | phase 67 | yes, at phase 91, if the zone pair is unchanged since phase 17 |
| Load 0x147800 (map rebuilt at 0x148B51) | character load | not established (§10) |
| 0x9EA10 copies the whole player-data block to a new creature | GUI callback | not established (§10) |

No subtraction or other increment of this map was found by the search pattern of §10 item 1. Save routine 0x149760 reads it (0x14A7CE).

### 8.3 Level P+0x144

| Writer | Effect | When | Notice? |
| --- | --- | --- | --- |
| 0x66850, called at 0x2DA90C in every world substep 0x2DA6C0 when W+0x448 is non-null | Clears seven per-type counters at P+0xB5C..+0xB74, sets level 1, then, only when P's inventory has more than five tabs (P+0x9F0 vector of 24-byte tabs), adds 1 per item of category 0x17 in tab index 5 and counts it into the per-type counter selected by 0x108EF0 when that index is below 7 | phase 25, before the phase-75 snapshot | **no** (absorbed by phase 75 of the same frame) |
| 0x9EA10 at 0x9EB4C, 0x9F7C0 at 0x9FA91, 0x7EA80 at 0x7F5B5 | Set 1 | GUI callbacks / start-up | not an increase in ordinary use |
| 0x147800 at 0x147A9B | Loaded value | character load | not established (§10) |
| 0x2C3710 at 0x2C4A2D/0x2C4B81, 0x2E0DB0 at 0x2E10D2 | Copy an owner creature's level to another creature (pets, summons) | world step | target is a different creature, not P (by construction of those routines; see §10) |
| 0x2E17D0 at 0x2E251E, 0x511E0 | Levels of newly spawned / generated non-player creatures | world step, generation | not P |
| 0x2DA6C0 is also called by 0xF5520 (a routine carrying the "Host receive " diagnostic, reached through 0x83E60/0x88EE0) | on the World object it is given | not traced | not shown to be the client W; see §10 |

Category 0x17 being the artifact category, and "level = 1 + number of artifacts", are reconstruction leads; the native rule as stated above (count of category-0x17 items in tab 5) is Steam-confirmed.

### 8.4 XP field P+0x148

No instruction in the image that adds to, subtracts from or otherwise increments P+0x148 was found. Its writers are whole-value stores only: load 0x147800 (0x147ACE), entity-data copies through 0x3B7A0 (which copies a creature's data from creature +0x10 onward; its +0x138 is creature +0x148; used by load, by 0x9EA10/0xA27B0 rebuilds, by spawn/pet routines and to fill the network client's previous-state buffer CL+0x78 — the last does not write P), and generation of non-player creatures (0x511E0). Writers at +0x148 in other routines (0x10B90, 0x44B40, 0x805D0, 0xC6CE0, 0x64C00) were not shown to target a creature; 0x64C00 is a reset of a different object type.

### 8.5 Can XP or level change in ordinary unmodded play?

* Level: **yes** — it is recomputed every world substep from the inventory (§8.3), so collecting or losing a category-0x17 item changes it, and total XP changes with it. But that recomputation happens only in phase 25, before the phase-75 snapshot, and no identified writer changes P+0x144 between phases 75 and 92. Therefore, through every producer identified here, the **"LEVEL UP!" notice, sound 0x1D and "You reached level N!" line are never produced** in ordinary play.
* XP field P+0x148: no identified gameplay producer changes it after load. Total XP changes only when the level changes, i.e. only in phase 25. Therefore the **"+N XP" notice and "You gain N XP." line are never produced** through the identified producers.
* Gold: phase-67 pickups and use-prompt pickups (phase 54) of value items produce "+N GOLD"; sells and other interface gold changes do not.
* Platinum: phase-67 pickups of platinum coins produce "+N PLATINUM" when the zone pair did not change during the frame.
* Feat (phase 46): produced for each W+0x130 record owned by P; its production is outside this contract (held chat/feat contract §4).

The two "never produced" conclusions hold for the producers identified here; they are subject to unproved items 1–3 of §10.

## 9. Draw-order and timing summary

* Frame N: phase 72 draws and ages existing records → phases 89–92 append new records (anchors fixed now) → frame N+1 phase 72 draws them at age 0 (alpha 1), then age += ms.
* The level sound plays in frame N; its floating text first appears in frame N+1; its chat line is appended in frame N.
* The projection matrices used at phase 72 are those stored before this frame's camera update (held schedule §3.7).

## 10. Unproved list

1. **Indirect writers.** Producers were found by searching direct `[register + offset]` accesses for +0xAAC, +0x144, +0x148 and the +0x1528 → +0xF8 chain. Writes through computed addresses, block copies (memcpy-style), serialisation through generic buffers or other base offsets were not exhaustively excluded. In particular, whether the client message handler (phase 71) can overwrite any of P's fields from received data was not traced; no direct writer was found there.
2. **Rebuild/load timing.** The GUI callbacks 0x9EA10, 0x9F7C0, 0x9FDF0 and the routine 0x84F70 (reached through 0x89160), which reload or rebuild the local creature and copy gold, level and the platinum map, were not traced to their dispatch point (window-message GUI dispatch versus the Interface update at phase 82). If any runs inside a window, the comparison would be between the old creature's snapshot and the new creature's values (P is re-read at each use) and could produce a notice. Not established.
3. **Pet/summon level copies and other world-step callers.** That the targets of 0x2C3710 and 0x2E0DB0 can never be P was inferred from those routines copying from an owner to a separate creature; not proved for every path. The World object passed to 0x2DA6C0 by 0xF5520, and the thread that runs it, were not traced; if it were the client W during phases 75–92 it would be a level writer inside the level window.
4. **Writers at +0x148 in 0x10B90, 0x44B40, 0x805D0, 0xC6CE0** were classified as not P by object shape only.
5. **Vector versus scalar power.** Whether the two-lane routine 0x393480 returns bit-identical results to the scalar CRT power for every exponent used was not proved. Odd multiples of 0.5 give products far from integers and even ones are exact powers of two, so truncation is not expected to differ, but this is an argument, not a proof.
6. **Number formatting locale.** The streams use the default global locale; no locale change was searched for in the whole image. Decimal without grouping is the classic-locale behaviour.
7. **Semantic names.** "Platinum coin" (category 0x0C + material 0x0D), "value item" (category 0x1F), "artifact" (category 0x17), "feat" (W+0x130) and "sell" (0x10A390) are names from strings, consumers or reconstruction leads; the numeric rules are what is proved.
8. **0x108EF0** (index into the seven per-type counters) and the meaning of those counters were not analysed.
9. **G+0x1DF0/G+0x1DF8** are used as render-origin offsets added to X/Y; their writer was not re-traced for this contract (held chat contract §4.1).
10. **Text rendering internals** beyond the property writes listed in §4 (glyph rasterisation, stroke algorithm, rounding) are deferred to the held chat contract §4.4 and the plasma text contracts; not re-verified here.
11. **Exceptional paths**: list-length errors, null P, null P+0x1528 other than in 0x60EC0, and concurrency with threads that do not take the World lock.
12. **No runtime observation** confirms any statement here; all are static.
