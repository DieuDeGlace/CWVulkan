# Steam combat texts and hit lights (Game update phase 68, routine 0xA0750)

Status: FINAL / STABLE / HELD (source-side behavioural contract, code-free).
Date: 2026-11-13.
Scope: the supported Steam release only. The combat-text routine 0xA0750, called once per Game update at 0xAADBD (phase 68 of the held phase-schedule contract), the floating-text records it appends to G+0x20B0, the light effects it appends to G+0x20A0, its other side effects (taming, two attacker fields, a console line), and how the per-frame consumers treat its records. Nothing here is implementation advice.

## 0. Authority, method and exposure

Authority: analysis image `decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe`, 5,757,952 bytes, SHA-256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244 (re-hashed for this contract), image base 0x140000000. The installed retail `cubeworld.exe` was hashed only: 5,939,072 bytes, SHA-256 A1EFBBD41EEB8DF08395C4D6511194D416CB70A14054B2A2000B9F5CDBCFA65B. All addresses are RVAs of the analysis image. No executable was run.

Evidence used: the whole-image instruction listing restricted to the 687 instructions of 0xA0750 (0xA0750–0xA131F; every instruction read; native operands decide wherever the presentation differs), the call-reference table (0xA0750 has exactly one caller, 0xAADBD in 0xA5A50, with G in the first argument register), the per-owner listing of 0xA5A50 around 0xAADBA and 0xAD8E4–0xAD96B, the image's string table, raw reads of constants, strings and import-name thunks from the image, and per-function presentations of the helpers 0x7D2F0, 0x9550, 0x9CD0, 0x648B0, 0x6E650, 0xA1840, 0xB36A0, 0x109330 (also its native listing), 0x109960, 0xE2E00 (argument use only) and 0xCE5F0 (the part that reads the effect list). Constants quoted as floats were read as binary32 from the image.

Reference sources read: docs/CLEANROOM_AUTHOR.md; the five assigned held contracts (complete phase schedule 2026-11-12, progress notices and floating text 2026-11-12, gameplay frame order, creature hit, sound); the original evidence above. The assigned creature-hit contract contains a verification section naming a probe recording; it was read as assigned, contains no implementation, and nothing here depends on it. The Infinite-Cubes reconstruction was not consulted; nothing here rests on it. The harness injected the repository's top-level project instruction text and the branch name into this context at session start without a tool read; it carries workflow policy only. No engine, test, exporter, log, capture, owner asset or runtime information was read or received. No other agent was used.

Terminology follows the held phase-schedule and notices contracts: G is the Game object, W the client World (G+0x08), P the local creature (W+0x448), "fixed" positions are signed 64-bit integers with 65,536 units per block, Z is vertical, screen Y grows downward. "Lookup(id)" means the search of W's ordered identifier-to-creature map at W+0x450 (the routine inlines the tree search, and also calls 0xE720 with W and an identifier, which performs the same lookup); it yields the stored creature pointer, or null when the identifier is absent. "The hit list" is W+0x90; its 0x48-byte records are the damage-number records of the held creature-hit contract §0.3, whose field names are used here:

| Record offset | Meaning (creature-hit §0.3) |
| --- | --- |
| +0x00 | attacker identifier (signed 64) |
| +0x08 | target identifier (signed 64) |
| +0x10 | amount (binary32; negative = healing) |
| +0x14 | critical byte |
| +0x18 | stun duration (signed 32, ms) |
| +0x20/+0x28/+0x30 | fixed position X, Y, Z |
| +0x44 | spirit-flag byte |
| +0x45 | kind byte (0 normal, 1 blocked, 3 immune, 4 dodge reward, 5 mana shield; 6 see §5) |
| +0x46 | "attacker ability 0x1D" byte |

## 1. Corrections to the task wording and to held contracts (held files unchanged)

1. **"TAMED!" is not a floating text.** The only occurrence of that string in the image (narrow, 0x43635C) is written to the process's narrow standard output (the imported `cout`, then the narrow end-of-line manipulator 0x9550, which puts a line feed and flushes) at 0xA0B6D–0xA0B8A. No record is appended to G+0x20B0 or G+0x20A0 by the taming branch (§5).
2. **"K.O." is the stun text**, produced for every visited record whose stun duration is nonzero, including hits that do not involve P (§3). It is not tied to death.
3. **"BLOCK", "MISS" and "ABSORB" carry a trailing line feed.** After the narrow literal is widened into the wide stream, the routine applies the wide end-of-line manipulator 0x7D2F0 (puts the widened '\n', then flushes) through the imported wide manipulator inserter, so the record texts are "BLOCK" + U+000A, "MISS" + U+000A, "ABSORB" + U+000A. Numbers and "K.O." have no line feed.
4. **"MISS" is the text of kind 3** (the creature-hit contract's "immune" kind); there is no separate miss kind. Kind 4 and kind 6 records never produce a text.
5. **Critical hits** have no separate string; they change only the font size (60 instead of 20) and, for non-negative hits on creatures other than P, the drift (−200 instead of −100).
6. The held notices contract §2 says this routine "typically" uses lifetime 1000, font 20 or 60 and flags 1. Precisely: every record it appends has lifetime 1000 and flags 1; font 20, or 60 for critical damage/label texts; "K.O." is always 20.
7. Held schedule phase 72 says the floating texts are projected "and updated" after this routine. Because phase 68 precedes phase 72 in the same Game update, records appended here are **drawn in the same frame at age 0** (unlike the notices of phases 89–92, which first appear one frame later), and the lights appended here are **aged once before they are first rendered** (§7, §8).

## 2. Traversal and the per-record order

0xA0750(G) walks the hit list W+0x90 from its first node to its sentinel, in list order, once. At phase 68 that list holds the records produced by this frame's world batch (phase 25), the records added by local control (phase 54) and the received records forwarded at phase 49 (held schedule §§3.4, 3.7). The routine never removes, reorders or modifies list records. W and W+0x450 are re-read per record; P is re-read from W+0x448 at each use.

**Record gate.** A record is visited only when lookup(target identifier) finds an entry whose creature pointer is nonzero (0xA0827–0xA0877). Otherwise nothing at all happens for that record (no text, no light, no "K.O.", no taming, no attacker update).

For a visited record the following steps run in this order; each is independent of the others' outcomes except where stated:

1. Attacker bookkeeping (§6.1).
2. A spirit-flag lookup with no observable effect (§6.3).
3. Taming (§5).
4. Spirit-weapon lights (§4.1), only when the spirit-flag byte is nonzero.
5. Ability-0x1D light (§4.2), only when the +0x46 byte is nonzero.
6. "K.O." text (§3.2), only when the stun duration is nonzero.
7. Damage number or label text (§3.3), only when P is involved.

So one record can yield up to two floating texts ("K.O." first, then the number/label) and any number of lights (lights first). Appends are always to the **end** of their list, so list order equals creation order. There are no CRT `rand` calls, no sound calls (no call to 0xA03F0, 0xA0420 or an append to W+0xA0) and no chat-widget calls anywhere in 0xA0750.

## 3. Floating texts (list G+0x20B0)

### 3.1 Record layout used

The 104-byte payload layout of the held notices contract §2 applies unchanged (age +0x00, drift +0x04, fixed anchor +0x08/+0x10/+0x18, screen offsets X +0x20 and Y +0x24, RGBA +0x28..+0x34, wide string +0x38, lifetime +0x58, font size +0x5C, flags +0x60). 0xA0750 does not use the constructor 0x80A40; it fills a stack record field by field. Differences from the notices: age 0, lifetime **1000**, flags **1**, anchor derived from the hit record (not from P), drift ±50/±100/−200, no random anchor perturbation. The damage/label record is appended through 0xA1920 (0xA127C); the "K.O." record is inserted at the list end by an inlined insertion through the same node constructor 0x6E7B0 (0xA0F2A–0xA0F6B), with the same list-length check (0x222222222222221) and the same copy-by-value semantics, so the two paths are behaviourally identical appends. The stack record's string is destroyed after each append (0x15C9A0).

### 3.2 "K.O." (0xA0E5B–0xA0F72)

Admission: stun duration (record +0x18, signed 32) is nonzero (any sign). No condition on P, the attacker, the kind, the amount or the critical byte.

| Field | Value |
| --- | --- |
| Text | wide "K.O." (0x436368, four characters, no line feed) |
| Age | 0 |
| Anchor X, Y | record X, Y exactly (each plus the truncation of binary32 0.0, i.e. plus 0) |
| Anchor Z | record Z + 131,072 (binary32 131072.0 at 0x42E5FC truncated toward zero to signed 64: two blocks above the record position) |
| RGBA | (1.0, 1.0, 0.0, 1.0): R, G and A stored as 1.0; the G store is an 8-byte store of 1.0 into +0x2C that also writes **0.0 into B** (+0x30) |
| Drift | −50.0 (0xC2480000) when lookup(target identifier) equals P; otherwise +50.0 (0x42480000). (The stack record first receives −100.0 at 0xA0E65, always overwritten.) |
| Screen offsets +0x20/+0x24 | **not written by this path** (see below) |
| Lifetime | 1000 |
| Font size | 20.0 (0x41A00000), never 60 |
| Flags | 1 |

Screen offsets: the eight bytes at +0x20/+0x24 of the stack record are left as they were. Within one call of 0xA0750 they are 0.0/0.0 once any earlier step of the same call has built a damage/label text (§3.3 stores 0 there) or run the taming branch (§5, whose empty-item construction zero-fills that stack area); for a "K.O." built before either has happened in the call, the offsets are whatever the stack held on entry. The native result in that case is indeterminate (§9 item 1).

### 3.3 Damage number or label (0xA0F77–0xA129A)

**Admission** (all required, tested in this order):

1. P is involved: lookup(attacker identifier) equals P, or else 0xE720(W, target identifier) equals P. (An attacker identifier of 0 or an unknown attacker yields null, which does not equal a non-null P.) Hits between two creatures neither of which is P get **no** number or label.
2. Kind is neither 4 nor 6 (the test subtracts 4 and ignores bit 1 of the 8-bit result).
3. |amount| ≥ 0.1, or kind ≠ 0. |amount| is formed by widening the binary32 amount to binary64, clearing the sign bit and narrowing back (an exact absolute value); 0.1 is binary32 0x3DCCCCCD (0x42E528), compared as binary32. A NaN amount fails the comparison and is then admitted only when kind ≠ 0.

So kind-0 hits below 0.1 in magnitude are silent; kind 1, 3 and 5 always show their label; any other kind (2, 7 and above, if ever produced) shows a number.

**Anchor.** X, Y = record X, Y exactly. Z = record Z + 131,072, then plus a size term: with T = 0xE720(W, target identifier) (non-null by the record gate), the binary32 value T+0x8C is multiplied by 0.5 (0x4AC964), subtracted from 1.0 (0x4AC980), multiplied by 65536.0 (0x42E5F8), truncated toward zero to signed 64 and added (0xA107C–0xA10A8). Thus the text starts two blocks plus (1 − ½·T+0x8C) blocks above the record position. When the target is P (see the table), Z is then reduced by 65,536 (one block).

**Colour and drift** (amount compared with 0 in binary32; the "non-negative" branch is taken when 0 < amount, 0 = amount, or the comparison is unordered):

| Case | RGBA | Drift | Anchor Z adjustment |
| --- | --- | --- | --- |
| amount < 0 (healing), target = P | (0.4, 1.0, 0.5, 1.0) (0x3ECCCCCD, 1.0, 0x3F000000, 1.0) | +100.0 | −65,536 |
| amount < 0, target ≠ P | (0.4, 1.0, 0.5, 1.0) | −100.0 | none |
| amount ≥ 0, target = P | (1.0, 0.3, 0.3, 1.0) (0x3E99999A for G and B) | +100.0 | −65,536 |
| amount ≥ 0, target ≠ P, spirit-flag byte 0 | (1.0, 1.0, 1.0, 1.0) | −100.0, or **−200.0** (0xC3480000) when the critical byte is nonzero | none |
| amount ≥ 0, target ≠ P, spirit-flag byte nonzero | (1.0, 1.0, 0.25, 1.0) (0x3E800000) | as the row above | none |

"Target = P" here is 0xE720(W, target identifier) equal to P (re-read from W+0x448). A hit dealt by P to another creature takes a "target ≠ P" row; a hit on P takes a "target = P" row whoever dealt it; P's self-heal records (attacker = target = P, creature-hit §§2.8–2.9) take the first row.

**Other fields:** age 0; screen offsets X and Y both 0.0 (one 8-byte zero store at 0xA117A); lifetime 1000; flags 1; font size 20.0, or **60.0** (0x42700000) when the critical byte is nonzero, for every row and every kind.

**Text.** A fresh wide string stream is constructed for each text (0x7DB40 with mode 3, default formatting: precision 6, no width, no fill, classic formatting as far as established — §9 item 4). Then, by kind:

| Kind | Text |
| --- | --- |
| 1 | "BLOCK" + U+000A (narrow 0x436374 widened, then manipulator 0x7D2F0) |
| 3 | "MISS" + U+000A (0x43637C) |
| 5 | "ABSORB" + U+000A (0x436384) |
| any other admitted kind | the number below, no prefix, no sign, no line feed |

Number: let v = |amount| (exact, as above), a binary32 value.

* If v ≥ 1.0, or v is NaN: v is truncated toward zero to signed 32 (CVTTSS2SI on the binary32 value) and inserted with the imported wide-stream signed 32-bit inserter (import thunk 0x42C580, mangled name ending "@H@Z"): plain decimal. Examples: 1.0 → "1", 12.9 → "12", 999.99 → "999". Values of 2^31 or more and NaN give the integer-indefinite value and print "-2147483648".
* If v < 1.0: k = v × 10.0 (binary32, 0x42E598), truncated toward zero to signed 32; the result is converted back to binary32 and multiplied by 0.1 (binary32 0x3DCCCCCD); that binary32 value is inserted with the imported wide-stream **float** inserter (import thunk 0x42C750, mangled name ending "@M@Z"), i.e. default (general, six significant digits) formatting. k is 1 … 9 for admitted kind-0 hits (0.1 ≤ v < 1), giving "0.1", "0.2", … "0.9"; a non-label kind with v < 0.1 would print "0". Healing amounts print without a minus sign; the green colour is the only healing indication.

The stream's contents are extracted (0x415A0), assigned to the record text (0x82C00), the temporary released, the record appended (0xA1920), then the stream destroyed.

## 4. Light effects (list G+0x20A0)

### 4.1 Effect record

G+0x20A0 is a doubly linked list (sentinel pointer at G+0x20A0, unsigned 64-bit count at G+0x20A8, maximum 0x3FFFFFFFFFFFFFE before a length error). Each node holds two link pointers followed by a 48-byte payload copied by value (node constructor 0x6E650):

| Offset | Width / type | Meaning |
| --- | --- | --- |
| +0x00/+0x08/+0x10 | 3 × signed 64 | fixed position X, Y, Z |
| +0x18/+0x1C/+0x20 | 3 × binary32 | light colour R, G, B (no alpha field) |
| +0x24 | binary32 | radius (passed unchanged to the renderer's light record) |
| +0x28 | signed 32 | age in ms |
| +0x2C | signed 32 | lifetime in ms |

Both effects of 0xA0750 write all 48 bytes.

### 4.2 Spirit-weapon lights (0xA0B90–0xA0D8A)

Admission: spirit-flag byte (record +0x44) nonzero and lookup(attacker identifier) finds a non-null creature A (no condition on P, on the kind, on the amount or on the target beyond the record gate).

The list of A's weapons in use is built by 0x4FD40 (held creature-hit contract §5.4: zero, one or two weapon items in a defined order). For each weapon in that order, its glow colour is computed by 0x109330 (below); when the colour's fourth component is strictly greater than 0 (it is positive exactly when the weapon has at least one entry whose material byte is ≥ 0x80), one effect is appended:

* position = record X, Y, Z exactly;
* colour = the glow colour's first three components;
* radius = 20.0 (0x41A00000);
* age = 0 (the radius store is an 8-byte store whose upper half zeroes the age);
* lifetime = 400 (0x190).

So a dual-wielding attacker whose two weapons both carry spirits yields two lights per qualifying record. The temporary weapon list is freed afterwards.

**Glow colour 0x109330(item).** Start from (0, 0, 0, 0). For each of the item's spirit entries (count = item byte +0x9D, 4-byte entries from item +0x20, in order) whose material byte is ≥ 0x80, add component-wise (binary32) the colour returned by 0x109960(material, base (1, 1, 1, 1) from 0x42E8C0, s = 1.0). With n such entries and n > 0, multiply each component by (1.0 / n) (one binary32 division of 1.0 by n, then one multiplication per component). For the four spirit materials with s = 1.0, 0x109960 gives base RGB × 1.5 (multiplier 0.5 × s + 1.0 with 0.5 from 0x4329B0, 1.0 from 0x42E8C0) and alpha (s + 1.0) × 1.5 = 3.0:

| Material | Colour per entry (binary32) |
| --- | --- |
| 0x80 | (1.0, 0.5, 0.1) × 1.5 = (1.5, 0.75, 0.15) |
| 0x81 | (0.3, 1.0, 0.5) × 1.5 = (0.45, 1.5, 0.75) |
| 0x82 | (0.3, 0.5, 1.0) × 1.5 = (0.45, 0.75, 1.5) |
| 0x83 | (0.8, 0.8, 1.0) × 1.5 = (1.2, 1.2, 1.5) |
| 0x84 … 0xFF | (0.5, 0.5, 0.5), alpha 1.0 |

(Products as rounded in binary32; the 0.1, 0.3, 0.8 inputs are the binary32 constants the routine uses.) The light colour is the average of the entries' colours; it may exceed 1.0.

### 4.3 Ability-0x1D light (0xA0D8F–0xA0E56)

Admission: record byte +0x46 nonzero and lookup(attacker identifier) finds a non-null creature. One effect is appended through 0xA1840: position = record X, Y, Z; colour (0.3, 0.3, 0.5) (0x3E99999A, 0x3E99999A, 0x3F000000); radius 16.0 (0x41800000); age 0; lifetime 400.

### 4.4 Ageing and use of these effects

* **Ageing (phase 72, 0xB36A0, before the floating-text pass):** every G+0x20A0 effect has age += integer ms (signed 32 addition); an effect whose age is then strictly greater than its lifetime is collected and erased after the traversal. With lifetime 400, an effect survives while its accumulated age ≤ 400.
* **Rendering (phase 95, inside the scene submission 0xE2E00, which receives G+0x20A0 as its sixteenth argument and passes it to 0xCE5F0):** each surviving effect is offered to the renderer's per-frame point-light collection (container at renderer+0x318, through 0xE2BD0) only when the renderer visibility test 0xE08C0 passes for its position. The submitted light has the effect's position, its radius unchanged, a distance value derived from the fixed differences to the renderer's eye (exact arithmetic not part of this contract), and colour = effect RGB × f, where f = 1.0 − (float)age / (float)lifetime, clamped below at 0. Fading is therefore linear in age.
* Consequently, because phase 68 precedes phase 72, the first rendered intensity of a light appended in a frame of interval ms is 1 − ms/400, and a light appended in a frame whose integer ms exceeds 400 is removed before it is ever rendered.

These are the only producers in 0xA0750; other producers of G+0x20A0 (for example phase 48) are outside this contract.

## 5. Taming (0xA0A05–0xA0B8A)

Admission, all required, in this order: the record's target identifier equals P's identifier P+0x08 (direct comparison; P is dereferenced here unconditionally); kind = 6; amount ≥ 1.0 (binary32; NaN fails); P's held-item category byte P+0x1D8 equals 0x15; 0xE720(W, attacker identifier) is non-null (call that creature C).

Effects, in order:

1. If P's extra-item slot P+0x8B8 is not empty (category byte nonzero), it is added to P's inventory by the inventory insertion 0x4E810(P+0x9F0, P+0x8B8, 1, −1).
2. P+0x8B8 is overwritten with an empty item record (all fields zero; copy helper 0x9CD0).
3. P's ability byte P+0x68 = 0.
4. P+0x9D8 (linked-creature identifier) = C's identifier C+0x08.
5. P+0x8B8 category byte = 0x14, and P+0x8BC (the item's 32-bit subtype field) = C+0x64 (C's race code). The other fields of that item stay zero from step 2.
6. 0x648B0(P, P+0x1D8, 1): removes up to one unit of an inventory item matching the held item from P's inventory (tabs P+0x9F0, matching by 0x4DF60, a slot whose count reaches 0 is reset to an empty item); details of the match are bounded by fingerprint only.
7. P's held item P+0x1D8 is overwritten with an empty item record.
8. The narrow line "TAMED!" followed by a line feed is written to the process's standard output (§1 item 1).

There is no floating text, light, sound, chat line or random draw in this branch, and the same record continues with the remaining steps of §2 (kind 6 never yields a damage text; it can still yield "K.O." or lights if its other fields say so). Which producer emits kind-6 records is not established (§9 item 6).

## 6. Other side effects

### 6.1 Attacker bookkeeping (0xA087D–0xA0974)

When lookup(attacker identifier) finds a non-null creature A and the amount is ≥ 0 (binary32 comparison with 0; negative or NaN amounts skip): A+0xBC0 (binary32) += amount, then A+0xBC8 (32 bits) = 0. This applies to every visited record whose attacker resolves, whether or not P is involved. The meaning of these two fields was not established (§9 item 7).

### 6.2 What is not done

No record of W+0x90 is consumed or altered; nothing is appended to any W list; no network, chat, sound, particle (G+0x2090) or random-stream effect occurs.

### 6.3 Dead lookup (0xA097B–0xA0A03)

When the spirit-flag byte is nonzero, the routine looks up the target, compares it with P, and if different performs another tree search for the attacker identifier whose result is never used. No observable effect.

## 7. Ageing, movement, fading and drawing of these floating texts

0xB6240 (phase 72) treats these records exactly as the held notices contract §4 describes for every record (container rebuild, list-order traversal, projection through the stored matrices, no culling other than the in-front test, two TextShapes per record, ageing after drawing, removal when age > lifetime). Applying it to the values above:

* **First appearance:** phase 72 of the **same** Game update, at age 0 (alpha 1, no drift yet), because phase 68 runs before phase 72; projection uses the matrices stored before this frame's camera update (phase 73).
* **Fade:** q = age / 1000 (age before this frame's increment), alpha factor f = 1 − q, linear. A record whose age reaches exactly 1000 is drawn once more with alpha 0 and removed after its next increment; otherwise it is removed once age exceeds 1000.
* **Motion:** pixel position = (projected X + 0, projected Y + q × drift + 0) for damage/label texts (offsets zero), linear in age:
  * damage to others: rises 100 px over 1000 ms (0.1 px/ms), 200 px when critical;
  * damage or healing on P: starts one block lower and **sinks** 100 px (positive drift = downward);
  * healing of others: rises 100 px;
  * "K.O." on P rises 50 px; "K.O." on any other creature sinks 50 px; its offsets are as stated in §3.2.
  The anchor is a fixed world point taken at creation; the text does not follow the creature.
* **Sizes and outline:** font 20 gives outline stroke radius max(20 × 0.125, 3.0) = 3.0; font 60 gives 7.5. Outline colour (0.1 R, 0.1 G, 0.1 B, f) and fill (R, G, B, f): for example a white number has outline (0.1, 0.1, 0.1); red (on P) (0.1, 0.03, 0.03); healing green (0.04, 0.1, 0.05); "K.O." yellow (0.1, 0.1, 0); spirit-tinted (0.1, 0.1, 0.025).
* **Flags:** these records pass flags 1 to the TextShape flag property (+0x2F8), where the notices pass 0. The TextShape layout routine 0x352310 receives that value; its meaning (for example an alignment or anchoring mode) was not established, so the horizontal/vertical placement of the glyph box relative to the computed pixel position for flags 1 versus 0 is unproved (§9 item 2).
* **Trailing line feed** of "BLOCK"/"MISS"/"ABSORB": passed to the TextShape as part of the text; whether it produces an empty second line that changes the drawn box or its vertical placement was not established (§9 item 3).
* Paint order: list order, so a later record (for example the number text after the "K.O." of the same record, or this frame's texts after older ones) paints over earlier ones; all floating texts paint beneath the rest of the GUI (notices §4 item 1).

## 8. Timing summary

* Frame N, phase 68: texts and lights appended (anchors and positions fixed now; no randomness).
* Frame N, phase 72: lights aged by ms (removed if age > 400); texts drawn at age 0 then aged by ms.
* Frame N, phase 95: surviving lights submitted with intensity factor 1 − age/400.
* The hit records themselves are discarded at phase 98 of frame N; nothing of 0xA0750 depends on later frames.

## 9. Unproved list

1. **"K.O." screen offsets on a first K.O. in a call.** When no damage/label text and no taming has run earlier in the same call, the record's +0x20/+0x24 come from stale stack contents left by earlier calls of the Game update. Their actual values (and hence whether such a K.O. is shifted on screen) were not determined; they are 0.0/0.0 in every other case.
2. **Meaning of TextShape flag value 1** (+0x2F8, consumed by 0x352310) versus 0: not analysed.
3. **Rendering of the trailing line feed** in "BLOCK"/"MISS"/"ABSORB" by the text layout: not analysed.
4. **Stream formatting locale**: the wide streams use the default global locale; no locale change was searched for in the whole image (as notices contract §10 item 6). The float inserter's exact formatting path (general format, precision 6) is the library default and was not traced inside the runtime.
5. **Creature +0x8C** (the size term of the damage-text height): meaning and typical values not established.
6. **Kind-6 records and P+0x1D8 category 0x15 / P+0x8B8 category 0x14**: the producer of kind-6 hit records, and the item semantics (feeding item, pet item), were not traced; the numeric rule of §5 is what is proved. The internals of 0x4E810, 0x648B0 and 0x4DF60 are bounded by fingerprint only.
7. **A+0xBC0 / A+0xBC8**: meaning and readers not established.
8. **Light collection limits**: whether 0xE2BD0 or later renderer stages cap, sort or drop point lights (and how the radius 16/20 and colour above 1.0 are used in shading) was not analysed; only the hand-off described in §4.4 is proved. The visibility test 0xE08C0 is used here as a black box.
9. **Identity of record producers**: which list entries exist at phase 68 is taken from the held schedule and creature-hit contracts; the received-record forwarding filter of phase 49 was not re-traced.
10. **Exceptional paths**: a null P (dereferenced unconditionally at 0xA0A10), list-length errors (0xA12AC/0xA12BA raise a length error), and concurrency with threads that do not take the World lock.
11. **No runtime observation** confirms any statement here; all are static.
