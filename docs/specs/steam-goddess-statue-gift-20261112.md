# Steam goddess statue (zone object type 0) and the "gift_of_the_goddess" dialog

Status: source-side behavioural contract, code-free. Date: 2026-11-12.
Scope: the supported Steam release only. Question: where vanilla worlds contain zone (static) objects of type 0, every condition for their "Examine" prompt and use action in the use-prompt routine 0x98180, the dialog pages and give that action would produce, and what prevents repeat gifts. It resolves the disagreement between the held use-prompt contract (path unreachable) and the held success-music contract (path fires in vanilla). Conditional statements are source conditions, not runtime observations. Nothing here is implementation advice.

## 0. Authority, method, exposure

* Analysis image: C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe, SHA-256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244 (re-hashed for this contract), image base 0x140000000. All addresses are RVAs of this image.
* Installed retail executable C:/Program Files (x86)/Steam/steamapps/common/Cube World/cubeworld.exe was hashed only: 5,939,072 bytes, SHA-256 A1EFBBD41EEB8DF08395C4D6511194D416CB70A14054B2A2000B9F5CDBCFA65B (same identity as the held contracts).
* Shipped dictionary consulted as data (keys only, no text reproduced): the dict_en.xml copy inside the decompiled export's game-files folder, SHA-256 3629A31565DD1F92B2199D22BF3B5CEDC741FD0CA83538BAE79B6AECEB4D06DB. The installed copy was not read or hashed (outside the permitted set); identity of the two copies is assumed, not proved.
* Evidence: whole-image instruction listing, per-function instruction listing of the zone generator 0x2FF6F0, per-function presentations, call-reference table, string and string-reference tables of the original export. No executable was run.
* Held contracts read: docs/CLEANROOM_AUTHOR.md; steam-success-music-triggers-and-pickup-rate-20261112.md; use-prompt.md; steam-game-update-complete-phase-schedule-20261112.md. Facts below were re-derived from Steam evidence.
* Infinite-Cubes reconstruction, used as leads only (release/src/cube and release/include/cube files only; no third-party folders searched): its static-object identifier table (type 0 named after the model goddess2.cub), its zone-generator branch for a "shrine of life" building, and its building-type selector. Names "goddess statue", "shrine of life", "flute", "special item" are reconstruction labels; every behavioural statement below was confirmed against Steam code unless marked "lead".
* The coordinating harness injected the repository's top-level instruction text and branch name into this context at start without a tool read (workflow policy only). No engine, test, log, capture or runtime information was read or received.

Terminology: G = Game object; P = local creature (world +0x448); PD = P's player-data block (P+0x1528); DW = the general dialog window (G+0x418), page vector begin/end at DW+0x1E8/DW+0x1F0, pages 0x1B0 bytes; "cell" = 16 × 16 blocks; "region" = 64 × 64 cells (1024 × 1024 blocks).

---

## 1. Headline resolution

The goddess-statue gift is **unreachable in vanilla Steam**, for two independent reasons:

1. The special-item test 0x5E890 reports special item subtype 4 as held for every creature, unconditionally (§3.1). The type-0 case of 0x98180 therefore always returns 0 with no text and no action (§3.2).
2. Even if that test were bypassed, the dictionary key "gift_of_the_goddess" has no entry in the shipped dictionary, so the page builder 0x167C80 adds no page (§4.1). The subsequent end-relative writes that set the flute give would then land on whichever page was already last in DW (or before the start of an empty page vector), not on a goddess page.

Consequently the held use-prompt contract (§3.7.2 note: unreachable) is correct, and the held success-music contract §A.4/§A.5 claim that 0xB591D fires in vanilla through the goddess statue is **wrong**: 0xB591D has no vanilla producer within the census of that contract, exactly like 0xB51D9. The statues themselves do exist in vanilla worlds (§2).

---

## 2. Where type-0 zone objects come from

### 2.1 Building selector: the "shrine of life" site (0x2D53D0)

0x2D53D0 classifies one cell (cell x, cell y as 32-bit integers, plus an area-information record) into a building code. After a long series of earlier branches that may return other codes (dungeon/place lookups via 0x2AA270 and 0x2A9DF0, and place-type branches keyed on the area record's type at +0 with influence +0x1C above 0.5; returns listed throughout the routine before 0x2D77C8), the region-site branch runs:

1. Region (rx, ry) = floor(cell x / 64), floor(cell y / 64) (true floor for negatives) (0x2D77C8–0x2D780D).
2. The site cell of that region is computed by 0x2D3B60 (0x2D781D), see §2.2.
3. Parity test (0x2D7822–0x2D784C): when (rx + ry) is odd (the signed remainder of the sum modulo 2 is nonzero, so negative odd sums count as odd), the region's building is **0x35** (lead: shrine of life) with reach 1. Otherwise the region's building is drawn from an 11-way hash of the site (seed 0x8AE21 + site x + 57 × site y; other building codes, not covered here).
4. When the classified cell equals the site cell in both coordinates, the routine returns the region's building (0x2D7948 onward; the underwater substitution applies only to building 0x15, not to 0x35). Cells within Chebyshev distance 1 of the site, other than the site, return 0 (no building) for reach 1.

So: in a checkerboard half of all regions (those with odd rx + ry), exactly one candidate cell, the region's site, is classified as building 0x35, **unless one of the earlier branches of 0x2D53D0 has already returned a different code for that cell**. Those earlier branches were not enumerated here; how often they pre-empt a site is unresolved.

### 2.2 Site position (0x2D3B60)

For region (rx, ry), with all arithmetic on unsigned 32-bit values wrapping modulo 2^32:
* base = ry × 57 + rx.
* Hash H(seed): v = base + seed; v = v XOR (v shifted left 13); r = ((v × v × 0xEC4D + 0x131071F) × v + 0xD208DD0D) with bit 31 cleared; q = r divided by 21474 (0x53E2), truncating; result = (100000 − q) modulo 62 (unsigned subtraction and unsigned modulo).
* Site cell x = rx × 64 + 1 + H(0x24341); site cell y = ry × 64 + 1 + H(0x165709).

The site therefore lies 1..62 cells into the region on each axis.

### 2.3 Placement in the zone generator (0x2FF6F0, 0x31402F–0x314121)

When the generator's per-cell building code (register value compared at 0x31402F) is 0x35, and a per-cell flag byte (frame byte read into DIL at 0x314024; lead: "the cell lies outside the zone being generated") is zero:
1. A level z is obtained from 0x2D87F0 (arguments: the two cell coordinates and the building code 0x35) (0x31404E).
2. A pedestal is built by 0x0BB400 with offsets (4, 4, z) and extents (8, 8, 2) (call at 0x314083).
3. A zone object record is default-constructed by 0x2F93A0: scale +0x24 = 1.0, state byte +0x28 = 1, user/link +0x30 = 0, item +0x38 empty (type byte 0), required count +0xD8 = 1. Its type dword +0x00 is set to **0** (0x314094).
4. Its orientation byte +0x20 = CRT rand() modulo 4 (one CRT draw, 0x31409B–0x3140AF; values 0..3).
5. Its position +0x08/+0x10/+0x18 = fixed-point conversions (0x0CB990) of (cell block x + 8, cell block y + 8, z + 2), i.e. the horizontal centre of the cell, two blocks above the level (0x3140B5–0x31410D). The two frame operands used for x and y are the cell's block-origin values (consistent with the lead; not separately traced).
6. The record is appended to the zone's object list by 0x1194B0 (0x31411C). Generation then skips the treasure/statue alternatives of the same branch (jump to 0x3146CA).

When the flag byte is nonzero, nothing is placed for this cell (0x31403C).

The zone object therefore is not an item-taking object (+0x38 empty), so in 0x98180 it always reaches the by-type dispatch (§3.2), and it is in the targeting set of 0x8AE50 (type 0 is eligible; use-prompt contract §2.3).

Census limit: this is the only Steam creator of type-0 zone objects confirmed here. The reconstruction also constructs a workstation record whose default type is 0 but discards it unless a workstation type is assigned (lead); the Steam counterpart was not checked. Records arriving from the host replace the client's zone object arrays (phase-schedule phase 36), so client-side statues are the host generator's output.

Side observation (limits apply): 0x2EB7B0 scans cells and, for every cell whose building code is 0x35 (0x2F17AE), appends a record with code 0xC carrying the cell and the level from 0x2D87F0 to a list (0x2F17FA–0x2F18B5). Its consumer (lead: a map marker) was not traced.

---

## 3. Prompt and use-key conditions in 0x98180

### 3.1 What 0x5E890 tests

0x5E890 takes a creature and an integer subtype S and returns a boolean.
* When S equals 4, it returns **true immediately** (0x5E8B1–0x5E8B6), without reading the creature at all. No inventory, region, quick-item or player-data state is consulted. No vanilla (or any) character can lack special item 4 as far as this routine is concerned.
* For any other S: it forms a probe item with type byte 0x18, subtype S, the creature's current region pair (P+0x150 into probe +0x0C, P+0x154 into probe +0x10), and every other field zero (0x5E8BD–0x5E91D). It sums the counts of all inventory stacks of every bag (P+0x9F0..P+0x9F8; stacks 0xA4 bytes, count dword at +0, item at +4) whose item equals the probe under 0x4DF60, and returns true when the sum is at least 1 (0x5E921–0x5E986). 0x4DF60 compares type, subtype, +0x08, both region dwords +0x0C/+0x10, +0x18, byte +0x1C, rarity +0x14, the spirit count +0x9D and that many spirit records; so for S ≠ 4 a held special item counts only when its stored region equals P's **current** region and its other fields are zero. The quick-item slot is not included (unlike 0x50670).

### 3.2 Type-0 case (0x9A59B–0x9A6E5)

Reached only after all earlier exits of 0x98180 decline (held use-prompt contract §3.1–§3.6): no dodge or walk key held and DW not shown; not climbing/climb-eligible; not dead; not carrying; no creature branch returned; the targeted object G+0x1E7C/G+0x1E80/G+0x1E84 resolves (0x2C3150); and it is not an item-taking object (+0x38 nonzero with +0x28 nonzero; never true for a generated statue). The switch on object type then selects case 0:
1. 0x5E890(P, 4) (0x9A5AB). When true, the routine returns 0 (0x9A5B2 jumps to 0x9AD42, which yields 0 and stores the use-key previous state through the common exit). No text is set, no highlight, no icon, no action. Since the test is constant true, this is the only behaviour in Steam.
2. (Dormant.) Otherwise the "info" text of the use button would be set to "Examine" (0x437BE8) and the routine would return 1; only on the use-key rising edge (flag in R15B, 0x9A60F) would it build the dialog of §4 and set the give fields.

Effect in vanilla: aiming at or standing near a goddess statue makes it the targeted object (it beats a nearby creature, and is beaten by any valid ground item, per use-prompt §2.5), the prompt routine returns 0, and 0x8AE50 proceeds to its no-prompt fallbacks (Ring selector branch, targeted ground item, or the equipped vehicle item: use-prompt §4.1–§4.3). The statue is inert to the use key.

---

## 4. Dormant dialog path (what the code would do)

### 4.1 Page building, 0x167C80

Arguments at the call (0x9A649–0x9A678): G+0x50 (speech/dictionary object), key "gift_of_the_goddess" (0x437CA8, the string's only reference), an empty context built by 0x80290, seed 0 (R9D cleared at 0x9A669), destination = DW's page vector (DW+0x1E8), speaker = empty string (0x42FF38).

Behaviour of 0x167C80 for this key:
* Keys starting with '!' (0x21) or '*' (0x2A) take literal-text paths; this key does not.
* The key is looked up in the ordered map at (G+0x50)+0x98 (0x1506A0 lower bound, 0x89470 comparison). That map is filled by the dictionary loader 0x1510F0 from the elements of dict_en.xml (file name 0x44CDD0, its only reference at 0x15BC33) whose element name is "speech" (0x441018) or "name" (0x4363C8), keyed by their id attribute. The shipped dictionary contains 949 speech ids and 858 name ids; **none is "gift_of_the_goddess"** (it has related ids such as "gift_of_strength" and "i_already_granted_you_my_gift", used elsewhere). The lookup therefore fails and 0x167C80 returns without adding or modifying any page.
* For a found key (not the vanilla case): when the entry's variant list (entry +0x40 object, vector at +0x10/+0x18) is non-empty, the variant index is the 32-bit xorshift of the seed (shift left 13, right 17, left 5, each XORed in) modulo the variant count, and the seed advances by 1; with seed 0 the index is 0. A new page is appended when the page vector is empty or the last page holds more than 10 lines; then the variant is rendered by 0x166CB0 (which may itself create further pages; not decoded here). Page count and key order for "gift_of_the_goddess" are therefore **undefined in vanilla**: there is no source text.

### 4.2 The give fields (0x9A693–0x9A6D3)

After 0x167C80 returns, the routine writes, relative to the vector's **end** pointer DW+0x1F0, i.e. into the last 0x1B0-byte page currently in DW:
* page +0xD8 (end − 0xD8): item type byte = 0x18 (0x9A6A1);
* page +0xDC (end − 0xD4): item subtype dword = 4 (0x9A6B6);
* page +0xD4 (end − 0xDC): give count dword = 1 (0x9A6D3).
No other field of the give item is written here; its remaining fields are whatever that page already held (the page constructor's defaults were not traced). Because 0x167C80 adds nothing in vanilla, these writes would hit a pre-existing last page of DW, or, with an empty vector, memory before the vector's start (end equals begin; a null end would fault). The routine then destroys the context (0x82580) and returns 1 (0x9A6E5 to 0x988C1). No interaction record is queued; nothing is stored per statue or per character.

### 4.3 DW page advance (0xB40D0, general-window block after the speech-bubble loop)

Per Game update (phase 64, after local control):
1. **Empty vector**: DW's own node visibility (node at DW+0x178, its visibility array element) is set to 0, and DW+0x204 (page index), DW+0x200 (page timer, binary32), DW+0x208 and the 64-bit DW+0x210 are zeroed.
2. **Non-empty vector**, in this order:
   a. **Page effects**, only when the page timer equals 0.0 exactly and the index is in [0, page count): on the current page, if its give count +0xD4 is nonzero, the give of §4.4; then, if its take count +0x30 is nonzero, remove the take item +0x34 from P's bags (0xA2210). Then the page's list at +0x190 (lore records: insert into PD+0xA8 and possibly a chat "lore increased" line) and list at +0x1A0 (zone keys: unless already reported by 0x5F4C0, insert into PD+0xB8 and PD+0x38, set the note timer to 180000.0 at PD+0xE8 entry +0x24, play sound 0x20 at the camera eye) are processed. A page whose effects run is the one that just became current; because the timer is zeroed only on page change, each page's effects run once per time it becomes current.
   b. Timer += the binary64 interval passed by the Game update (narrowed to binary32). That interval is the Game update's capped frame interval in ms (XMM7, loaded from the stored capped value at 0xAA006 before the call at 0xAA1BC; path coverage of that load not exhaustively proved).
   c. DW's visibility element is set to 1 (shown).
   d. Reveal threshold: L = DW+0x2FC × Σ over the current page's lines of (line length + 1) (0x290430; 0 when the index is out of range). DW is constructed by 0x28FD60 (from 0xFEB26, stored at Interface +0x300 = G+0x418), which sets DW+0x2FC = 40 (0x28FE93). If the timer ≤ L + 100, both edge statics 0x5515B5 and 0x5515B6 are cleared. Otherwise: if (attack key G+0x1A70 is up and static 0x5515B5 is set) or (use key G+0x1A7B is up and static 0x5515B6 is set) — i.e. a release of either key after the threshold — and the index is below the page count, then the index increases by 1, the timer and DW+0x208 are zeroed, and sound 0x7A (when the new index equals the page count) or 0x7B (otherwise) plays at the camera eye G+0x1DC0 with volume 1.0. Then both statics take the current key states.
   e. If the index is now ≥ the page count: the vector is emptied (0x8ADF0), DW is hidden, and index, timer and DW+0x208 are zeroed.
   There is no automatic page timeout: a page stays until a key release after its reveal threshold.

Timing consequence for a hypothetical goddess dialog: the first page's effects run in the same Game update that built the pages (local control precedes phase 64); the last page's give runs on the first update after the player has advanced onto that page. While DW is shown, 0x98180 returns at its suppression test, so the statue cannot be re-used during the dialog.

### 4.4 The give (carry test call 0xB58CE, add 0xB58FB, announce 0xB5918, success-music store 0xB591D, take 0xB5ADB)

On a page whose give count C (+0xD4) is nonzero, with give item I at +0xD8:
1. Carry test 0x50670(P, I). For type 0x18 it fails (no give) when the count of items equal to I (0x4DF60) over all bag stacks plus the quick-item slot (P+0xA0C, count P+0xA08) is above 0. Note that 0x4DF60 compares I's region and other fields, so "already has a flute" is judged against I's exact fields, not against special item 4 alone. If the test fails, nothing below happens (no add, no announce, no timer).
2. Add C × I to P's bags (0x4E810 on P+0x9F0, slot −1).
3. Announce (0x9D6F0 with G, I, C, P).
4. G+0x2050 = 60000 (0xB591D). Not gated by the ability test.
5. When C = 1 and I's type is 0x18: queue the token script "define_item" I, "you_received_the_item" through 0x1694E0 into DW's own page vector with seed rand() modulo 10000 (one CRT draw) and an empty speaker; these pages are appended after the current one, so DW continues with them. Then the ability test (P+0x68 zero, or P+0x6C strictly greater than the binary32 conversion of 0x4F220(P)); if it passes: P+0x6C = 0, P+0x68 = 0x7C (hold pose), P+0x1D8 = copy of I.
For the dormant goddess path, I would be type 0x18 subtype 4 with count 1, so all of step 5 would apply.

---

## 5. Repeat-gift prevention

* The only gate at the statue is 0x5E890(P, 4) in 0x98180. There is no per-statue state (the statue's state byte +0x28, link +0x30 and item +0x38 are not read or written by case 0), no player-data set membership, no cooldown, and no interaction record or host involvement.
* At give time the carry test 0x50670 refuses a second type-0x18 item equal (under 0x4DF60) to one already carried or in the quick slot.
* Nothing else was found. Since 0x5E890(P, 4) is constant true, the question is moot in vanilla: no gift is ever offered.

---

## 6. Constants

| Value | Source | Use |
| --- | --- | --- |
| 0x35 | 0x2D784C, 0x31402F | building code that receives the type-0 object |
| odd (rx + ry) | 0x2D782F–0x2D784A | regions that use building 0x35 |
| 64 | 0x2D77C8–0x2D780D | cells per region axis |
| 57, 0x24341, 0x165709 | 0x2D3B68, 0x2D3B8A, 0x2D3B83 | site hash base multiplier and x/y seeds |
| 0xEC4D, 0x131071F, 0xD208DD0D (subtract 0x2DF722F3), 0x7FFFFFFF | 0x2D3BA5–0x2D3BC2 | site hash polynomial and mask |
| 21474 (0x53E2), 100000, 62, +1 | 0x2D3BC6–0x2D3C38 | site hash scaling and in-region offset |
| 0 | 0x314094 | zone object type of the statue |
| rand() mod 4 | 0x31409B–0x3140AF | orientation byte +0x20 |
| (+8, +8, +2) | 0x3140B5–0x3140DF | object position offsets from cell origin / level |
| pedestal (4, 4, z; 8, 8, 2) | 0x314055–0x314083 | pedestal box |
| 1.0, 1, empty, 1 | 0x2F93A0 | default scale, state byte, item, required count |
| 4 | 0x5E8B1 | subtype reported held unconditionally |
| 0x18 / 4 / 1 | 0x9A6A1 / 0x9A6B6 / 0x9A6D3 | give type, subtype, count (end − 0xD8, − 0xD4, − 0xDC) |
| 0x437BE8 "Examine", 0x437CA8 "gift_of_the_goddess" | strings | dormant prompt and key |
| 40 | 0x28FE93 | DW+0x2FC reveal factor per line character |
| +100 | 0xB40D0 general block | reveal threshold addend |
| 0x7A / 0x7B | 0xB40D0 general block | close / next-page sounds |
| 60000 (0xEA60) | 0xB591D | success-music timer |

## 7. Corrections to held contracts (held files unchanged)

1. steam-success-music-triggers-and-pickup-rate-20261112.md §A.4 (b), last paragraph of §A.4 and the §A.5 row for 0xB591D: the goddess statue path is not reachable in vanilla (0x5E890 returns true for subtype 4 unconditionally, and the dictionary key is absent). 0xB591D has no vanilla producer found; "Vanilla SP" for 0xB591D should read "No producer found", like 0xB51D9. That contract's open item "who advances the dialog page timer +0x200" is answered for DW in §4.3 here.
2. use-prompt.md §3.7.2 note: the conclusion (unreachable) is confirmed; additionally the page builder would produce no page for the key, so the give fields would be written onto an unrelated last page.

## 8. Unresolved

* Which earlier branches of 0x2D53D0 pre-empt a building-0x35 site, and therefore the actual fraction of odd-parity regions that contain a statue.
* The meaning of the generator's per-cell flag byte that suppresses placement (lead: outside the generated zone), the exact identity of the frame operands used as the cell origin, and 0x2D87F0's level computation.
* A complete census of other creators of type-0 zone objects (only 0x31402F–0x31411C confirmed).
* Identity of the dict_en.xml copy read with the installed file; whether any non-dictionary source can populate the map at (G+0x50)+0x98.
* Default fields of a dialog page's give item (page constructor) and therefore 0x4DF60 equality at give time; 0x166CB0 page rendering internals.
* Whether XMM7 at 0xAA1BC holds the capped interval on every path; other writers of DW+0x2FC.
* The consumer of the 0x2EB7B0 code-0xC records.
* No runtime observation confirms any statement here.

FINAL / STABLE / HELD
