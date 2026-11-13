# Steam Shrine of Life: activation, glow, map teleport and respawn

Status: source-side behavioural contract, code-free. Date: 2026-11-12.
Scope: the supported Steam release only. Question: how Shrines of Life are represented, activated (flute), shown (glow), used on the map (teleport) and used as respawn points. Statements are source conditions read from the original image, not runtime observations. Nothing here is implementation advice.

## 0. Authority, method, exposure

* Analysis image: C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe, SHA-256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244 (hashed for this contract), image base 0x140000000. All addresses are RVAs of this image.
* Installed retail executable C:/Program Files (x86)/Steam/steamapps/common/Cube World/cubeworld.exe was hashed only: 5,939,072 bytes, SHA-256 A1EFBBD41EEB8DF08395C4D6511194D416CB70A14054B2A2000B9F5CDBCFA65B (same identity as the held contracts).
* Evidence: the original export's per-function presentations (including the long-timeout retry presentation of 0x2C5250), whole-image instruction listing, call table, string and string-reference tables; float constants were read directly from the analysis image's bytes. The instruction listing has no entries for 0x2CCA00–0x2CD700 (inside 0x2C5250); for that range the presentation was used and the two distance constants were confirmed by decoding the RIP-relative loads at 0x2CD1F1 / 0x2CD1F9 from raw bytes. No executable was run.
* Held contracts read: docs/CLEANROOM_AUTHOR.md; steam-goddess-statue-gift-20261112.md; use-prompt.md; map-markers.md; steam-item-map-selection.md; steam-game-update-complete-phase-schedule-20261112.md (identified, not needed beyond the others). Facts from them are cited by section and were re-checked where marked.
* Infinite-Cubes reconstruction was used as leads only (release/include/cube and release/src/cube, release/src/gfx files; no third-party folders searched). Names "Shrine of Life", "flute", "Rune Portal" are labels; every behavioural statement below was confirmed against Steam code unless marked "lead".
* The coordinating harness injected the repository's top-level instruction text and branch name into this context at start without a tool read (workflow policy only). No engine, test, log, capture or runtime information was read or received.

Terminology: W = World (the object whose +0x448 is the local creature); G = Game; P = local creature (W+0x448); PD = P's player data (P+0x1528). "cell" = 16 × 16 blocks; "site region" = 64 × 64 cells (1024 blocks); "land" = the 8192-block land cell returned by 0x2ADA20 (map-markers "land-map cell"; the region that region-locked special items use). Fixed point = 1/65536 block. "Known-cell set" = PD+0xA8, the ordered set of (cell x, cell y) pairs that map-markers calls "known places" (also used by chests and lore).

---

## 1. Headline

1. A Shrine of Life is the **type-0 zone object** (model goddess2.cub; reconstruction label "goddess statue") on its pedestal, placed at the site cell of building code **0x35** (dictionary key "ShrineOfLife", 0x16C7E0). The race-0x10C creature is a different object (lead: "RunePortal") that only opens the shrine-choice map (§2.3).
2. Activation is **playing the flute** (special item type 0x18, subtype 4, ability 0x89) within 8 blocks of the statue. The flute is offered as the **Use key fallback** item of 0x8AE50, not by the statue's own prompt. At the 5000 ms boundary of the ability, the statue's 16-block cell is inserted into **PD+0xA8**, every carried flute's rank field rises by 1, and sound **0x6E (flute2.wav)** plays. Nothing is sent to the host and no zone-object state changes.
3. The glow is a **per-character point light**, radius 32, colour (0.25, 0.3, 0.5), on every type-0 statue whose cell is in PD+0xA8.
4. The map shows the `shrine` sprite (place kind 12) only for shrine cells in PD+0xA8. A left click on such a marker in the open full map starts ability **0x75**, which teleports the player (and pet) after **3000 ms** to cell offset (+6, +8) at the cell level. No cost.
5. On **Revive** (pressing Use while dead), the respawn point is the **nearest activated shrine in the player's current land**. If none exists, it is the **nearest Shrine-of-Life site in the same land** within a ±8192-block window, activated or not. If neither exists, it is the player's own cell. Distance is horizontal Euclidean, measured from the player's position.
6. Every new character starts beside a Shrine of Life whose cell is pre-inserted into PD+0xA8. Every character counts as owning the flute (0x5E890 subtype 4 is constant true).

---

## 2. Representation (question 1)

### 2.1 The statue (zone object type 0)

Placement is as held in steam-goddess-statue-gift §2: building selector 0x2D53D0 gives code 0x35 to the site cell of every site region with odd rx + ry (unless earlier branches pre-empt it). Site cell per 0x2D3B60. The zone generator 0x2FF6F0 (0x31402F–0x31411C) places one type-0 record at (cell origin + 8, + 8, level + 2), with active byte +0x28 = 1, and a pedestal (4, 4, level; 8, 8, 2). 0x16C7E0 maps building code 0x35 to dictionary key "ShrineOfLife" (0x44D528); 0x3A gives "ManaWell" and 0x3B gives "ShrineOfFinesse". The statue's own prompt case in 0x98180 always returns 0 (goddess contract §3.2, re-confirmed premise).

The statue's 16-block cell is the site cell itself, because the statue lies at cell origin + 8. The keys that activation, glow, map and respawn use therefore coincide: floor(position / 16) of the statue, and site cell × 16 + 8 for map and respawn.

### 2.2 Start shrine

* 0x2D1100 (start-site finder) draws one CRT angle and one CRT distance in [100000, 199999] site regions. It then walks site regions eastward one at a time, using 0x2D3B60 sites, and accepts only cells whose building is **0x35**, plus several terrain/biome/place tests (land kind 0, 2 or 7 via 0x2B9080, place type 5, two non-positive weight tests, not fully decoded here).
* The new-character setup 0x9F7C0 (call 0x9FB62) stores that cell in P+0x1D0/+0x1D4. It positions the player at (cell x × 16 + 8, (cell y + 2) × 16, terrain height there).
* The character-creation completion 0x9EA10 (insert at 0x9EBA3) inserts (P+0x1D0, P+0x1D4) into **PD+0xA8**. So each new character's start shrine is already activated: it glows, shows on the map, can be teleported to, and is a respawn candidate.

### 2.3 Race 0x10C creature (not the shrine)

* Use prompt: "Use" within talk range. A press sets G+0x668 and G+0x669 to 1, which opens the map in shrine-choice mode (use-prompt §3.6.7; re-confirmed in 0x98180 near the "Use" string 0x437C58). No interaction is queued and nothing is activated.
* Light (0xCE5F0): a race-0x10C creature whose byte +0x19C is nonzero contributes a point light at its drawn position lowered by its step offset +0x990. Radius 20.0 (0x41A00000), colour (0.1, 0.2, 0.4).
* Appearance (0x53E80 case 0x10C): head/body model pair 0xE00/0xE01, flag 0x400.
* Lead: the reconstruction names this race "RunePortal" (models rune-portal.cub / rune-portal-feature.cub). Where Steam spawns it, and what sets +0x19C, were **not traced**. It is a second entry point to the same shrine teleport (§5.3), not a shrine. The category-1 subtype-13 item does the same (steam-item-map-selection §1).

---

## 3. Activation (question 2)

### 3.1 The flute is always "owned"

0x5E890(creature, 4) returns true immediately for every creature (goddess contract §3.1). No inventory item is needed. A new character's inventory was **not** checked for an actual flute stack: the flute is never added at creation in the code examined, and that remains unresolved (§8). The rank increment (§3.4) affects only real flute stacks.

### 3.2 Offering the flute on the Use key (W update 0x2DA6C0, 0x2DAE01–0x2DB18B)

Every W update, PD+0x148 (the "usable vehicle/special item" of use-prompt §4.3) is first cleared. Then, only if all of the following hold:

* the dodge timer P+0x12C is 0.0;
* P's ability has finished (elapsed P+0x6C strictly greater than the 0x4F220 duration);
* P's ability is not 0x7A (climbing).

When those hold: ability 0x78 writes item (0x18, 1) and 0x79 writes (0x18, 2). Then the first matching rule below wins:

1. 0x5E890(P, 7) and a type-1 object is near (0x2B84D0 with flag 0) and (P+0x5C & 3) ≠ 0 → (0x18, 7).
2. 0x5E890(P, 0xD) and a type-9 object is near (flag 0) and (P+0x5C & 3) ≠ 0 → (0x18, 0xD).
3. **0x5E890(P, 4) [always true] and a type-0 object is near with its active byte required (0x2B84D0(W, 0, 1), call 0x2DAF5C) and (P+0x5C & 3) ≠ 0 → (0x18, 4), the flute** (0x2DAF3F–0x2DAF98).
4. Then subtype 8 (types 2 or 0xF), 5 (type 0x10, active), 2 and 1 (not shrine related).

P+0x5C bits 0 and 1 are the on-ground and second contact bit (lead: in water). A player in the air gets no flute.

**Nearness test 0x2B84D0(W, type T, requireActive R)**: P must exist. It scans every loaded 64-block zone overlapping P's x ± 8 and y ± 8 blocks (the zone hash map at W+0x468…+0x490). Each zone's object list (zone+0x60, 0xE0-byte records) is checked in order. It returns true at the first record that meets all three conditions:
* its type equals T;
* its byte +0x28 is nonzero, or R is 0;
* its 3-D distance² to P's position is **≤ 64.0** blocks² (0x42E5B4).

The distance uses each fixed-point difference converted to float and multiplied by 1/65536.

### 3.3 Pressing Use (0x8AE50 fallback; use-prompt §4.3)

The fallback applies only when all of these hold:
* the prompt routine 0x98180 returned 0 (it always does for a targeted statue, and when nothing is targeted);
* the Ring selector is hidden;
* no ground item is targeted;
* the dialog window is not visible.

The Use button then shows the flute icon and the item's short name. On the Use rising edge, P+0x1D8 becomes the item (0x18, 4), P's ability becomes **0x89** and its elapsed time becomes 0. A creature prompt (e.g. a townsperson under the crosshair) or any valid ground item pre-empts the fallback. In practice the key is **E** (key byte G+0x1A7B), as recalled.

### 3.4 The flute ability 0x89 (0x2C5250 case 0x89, local creature only)

Ability 0x89 has total duration **5000 ms** (0x4F220: base 5000, no other parts). The case runs only for the creature equal to W+0x448. Remote players' flute playing never registers anything for the local character. Let t be the elapsed time before the step and dt the step's elapsed ms. Within the case, in this order:

1. **Registration boundary**: the held item is type 0x18 subtype 4 and t < 5000.0 ≤ t + dt (constant 0x42E5E4 = 5000.0):
   * Scan the 3 × 3 zones around P's zone (zone offsets −1..+1 on each axis, x outer, via 0x16E50 / 0x16E00), each zone's object list in order.
   * For each object of **type 0** whose 3-D distance² to P is **< 64.0** (strict; 1/65536 and 64.0 loaded at 0x2CD1F1 / 0x2CD1F9), take its cell (floor(x/16), floor(y/16), 0x137E0).
   * If that cell is **not** already in PD+0xA8: insert it (0x7D390). Then, over every bag of P (P+0x9F0..+0x9F8) and every stack (0xA4 bytes) with count ≠ 0, item type 0x18 and subtype 4, add 1 to the item's dword +0x08 (stack +0x0C). Then append sound **0x6E** at P's position to the W sound list (W+0xA0, via 0xA18B0).
   * If the cell is already present, nothing happens for that statue. Several statues within range in one step are each processed.
   * The active byte +0x28 is **not** tested here, and no object field is written.
   * After this block the case **ends** (jump to the common tail 0x2CFE95), whether or not any statue was found. So the generic end-of-ability item interaction (kind 2, queued for other items at the duration boundary) is **never queued for the flute**.
2. **Start sound**: on a step with t = 0.0 exactly, sound **0x6D** is appended at P's position with P's id as source. The sound table 0xA3A40 maps 0x6D to flute1.wav and 0x6E to flute2.wav.
3. **Music notes**: for held subtypes 4..11 except 9, while t is below the duration and floor(t / 500) differs from floor((t + dt) / 500), one particle record (kind 7, count 1) is queued to W+0xB0 (0x2D0450). Its exact offset and colour fields were not decoded.

**Pose**: the animation selector 0xD3170 sets creature +0xC44 = **0x5A** for ability 0x89 with held item (0x18, 4). Lead: this is the "play flute" animation. Rendering of the held flute model (flute.cub) was not traced.

**Rank**: the item tooltip 0xB7B90 shows, for type 0x18 subtype 4, the text "RANK " followed by (item +0x08) + 1. Each newly activated shrine raises a carried flute's rank by 1. With no carried flute stack, no rank is stored anywhere except implicitly in PD+0xA8.

### 3.5 What is recorded

* Only PD+0xA8 gains the cell. It is the same set that marks opened chests and read lore cells. Persistence with the character save is a lead (not traced here).
* No zone-object state change: +0x28, +0x30 and +0x38 are untouched.
* No interaction record and no host message. Activation is a client-local character fact. Each character has its own set.
* Already activated: the Use offer is still made near a statue with +0x28 ≠ 0, and the flute still plays (5 s, pose, 0x6D, notes), but registration is skipped (no 0x6E, no rank).
* The offer radius (≤ 8, requires +0x28 ≠ 0) and the registration radius (< 8, no +0x28 test) differ at exactly 8 blocks.

---

## 4. The glow (question 3)

Light collector 0xCE5F0, static-object pass (type-0 branch 0xCF41D–0xCF7C1):

* Driving state: the statue's cell (floor of x and y, ÷16) is in the **local** PD+0xA8. The glow is per character, has no animation or fade, and appears from the first frame after registration.
* Visibility: 0xE08C0 on the object position with radius XMM7 = 32.0 (loaded at 0xCE6B3; assumed unchanged at the call 0xCF439) and the renderer's distance W-renderer+0x4A4.
* Light: position = object position raised by (object box height from 0x13A960 + 1.0) blocks (×65536, truncated). Radius **32.0**. Colour **(0.25, 0.3, 0.5)** (0x3E800000, 0x3E99999A, 0x3F000000), a constant blue. Camera distance² is recorded for sorting. The light is appended to renderer+0x318 (0xE2BD0).
* No other shrine-specific effect (particles, model swap, highlight) was found in this pass.

---

## 5. The map (question 4)

### 5.1 Marker creation

The map region builder 0x2EB7B0 (0x2F17AE–0x2F18B5) runs for each site region overlapping a map region. It creates a place record of **kind 12 (0xC)** at block (site x × 16 + 8, site y × 16 + 8) and level z = 0x2D87F0(site cell) when both hold:
* that point lies inside the map region;
* its building is 0x35.

The branch runs only when a map-builder value read through its third parameter is < 0x201. Lead: the map scale must be ≤ 512. Which zoom levels this excludes is unresolved.

### 5.2 Marker display (map-markers §1.3.1, §1.7, §1.8)

* Kind 12 bypasses the zone-discovered test but is skipped unless its 16-block cell is in PD+0xA8. **Only activated shrines appear.**
* Icon: the node `shrine` from gui.plx / quest-tag.plx (G+0x518). Empty label, white label colour, flag A set.
* Scale: (4 ÷ unit) × mapScaleNow × 8, clamped to [0.35, 1.0]. The icon is drawn at 1.25 when it is the hover/shrine target. Shrine icons do not bob.
* Shrine-choice mode (G+0x669): only flag-A records are hit-testable. Their text colour pulses as (0.75 + 0.25·cos(W+0x440 × 0.01), 0.25, 1, 1), and text scale is 1.2 for the target. The label is empty, so whether this colour tints anything visible depends on the icon draw, which is not established.
* The sprite's own artwork and colour (the "blue circle") were not inspected. They are owner asset content.

### 5.3 Teleport (steam-item-map-selection §2, §4)

* Every overlay draw resets the shrine target WorldMap+0x15C. Hovering a flag-A marker while the map is open (G+0x668) and not dragged sets it to the marker's block XY.
* Left mouse-up (0x965E0, acceptance 0x96BA2–0x96D85) requires G+0x668 set, not dragged, a target set, and floor(target ÷ 16) in PD+0xA8. On acceptance: P ability = **0x75**, elapsed 0, P+0x1D0/+0x1D4 = target cell, G+0x668 cleared.
* Shrine-choice mode is **not** required: the ordinary full map (opened by its toggle) also accepts a shrine click. The race-0x10C "Use" and the category-1 subtype-13 item only restrict hover to shrines.
* At the 3000 ms effect boundary of ability 0x75 (0x2C5250 branch 0x2C5886, local creature only), the player's position becomes ((hx × 16) + 6, (hy × 16) + 8, 0x2D87F0(hx, hy, 0)) in blocks. The pet (P+0x9D8), if it resolves, is copied to the same position.
* **Costs: none.** No item, money, cooldown, region limit or confirmation. Any activated shrine on the map, in any land, can be chosen. No teleport-specific sound was found.

---

## 6. Death and revival (question 5)

### 6.1 Revive trigger (0x98180; use-prompt §3.4)

While P's health ≤ 0, the Use button reads "Revive". On the Use press:
1. 0x65FB0 revives P.
2. The death position (PD+0x290..+0x2A0) is set to P's position.
3. The tombstone timer PD+0x28C is set to 300000.
4. P's position is replaced by **0x2C2860(W, out, &P+0x10)** (call 0x987A7, its only caller).
5. P+0x140 is set to −0.5.
6. Health is set to its maximum.

No automatic revive was found in this routine. Whether other paths (e.g. the host) revive is not covered.

### 6.2 Respawn choice 0x2C2860

Let L = P+0x150/+0x154, the player's **current land**. Each W update writes it with 0x2ADA20 of P's block position, or of the zone centre when P stands in a block with flag 0x20 (0x2DA6C0, after the creature-update loop; exact instruction addresses not listed). Distances are horizontal: (candidate centre − P's fixed x/y) × 1/65536 in double precision (0x43EF58), squared and summed. Z is ignored. The best distance starts at −1.0 (0x4725D8). The default result cell is the cell of the passed position, i.e. the death cell.

**Pass 1, activated shrines.** Walk PD+0xA8 in its sort order (x, then y). For each cell c:
* skip c unless 0x2ADA20 of block (c × 16 + 8) equals L in both dwords;
* compute d²; if no best exists yet or d² < best (strict), check that the building at c is 0x35 (0x69C60);
* if it is, c becomes best.

Non-shrine members (chests, lore cells) are rejected by the building test. On ties, the first in set order is kept. If pass 1 found anything, it is the answer.

**Pass 2, any shrine site.** This runs only when pass 1 found nothing:
* Site regions rx run from floor(floor((x − 8192) / 16) / 64) to floor(floor((x + 8192) / 16) / 64), x outer; ry likewise from y. x and y are the passed position in blocks, so the window is about ±8192 blocks (up to 17 × 17 site regions).
* For each site region, compute its site cell. The hash is the same as 0x2D3B60: base 57 × ry + rx, seeds 0x24341 and 0x165709, polynomial 0xEC4D / 0x131071F / 0xD208DD0D, mask 0x7FFFFFFF, ÷21474, (100000 − q) mod 62, +1.
* Apply the same land test, strict nearer test and building-0x35 test.
* Activation is **not** required in this pass.

**Fallback.** When neither pass finds a shrine, the death cell is kept.

**Result position.** For the chosen cell (cx, cy): ((cx × 16) + 6, (cy × 16) + 8, 0x2D87F0(cx, cy, 0)), converted to fixed point. That is 2 blocks west of the statue's centre line, at the building level (the statue stands at level + 2 on the 8 × 8 × 2 pedestal, so this point lies within the pedestal footprint at its base). How collision then resolves that point was not traced.

### 6.3 Consequences

* Activated shrines in other lands are never respawn points, including the start shrine once the player has left its land.
* With at least one activated shrine in the current land, the nearest **activated** shrine wins, even if an unactivated site is nearer.
* Without one, the nearest **existing** shrine site in the land wins, within the ±8192 window.

---

## 7. The player's recollection versus Steam

| Recollection | Steam verdict |
| --- | --- |
| Every character owns the flute from creation | **Confirmed functionally**: 0x5E890(·, 4) is constant true, so the flute is always offered. Whether a visible flute item is in a new inventory is **unresolved** (no creation-time add found in the code examined). |
| Shrines of Life are respawn points | **Confirmed** (0x2C2860 accepts only building-0x35 cells). |
| Discover a shrine, press E, the character plays the flute | **Confirmed**: E (G+0x1A7B) via the 0x8AE50 special-item fallback within 8 blocks while on the ground/contact, ability 0x89, pose 0x5A, flute1.wav, notes. "Discovery" is simply proximity; no separate discovery is needed. |
| The shrine starts to glow, showing it can be teleported to | **Confirmed**: the cell enters PD+0xA8 at the 5000 ms boundary (flute2.wav). That drives both the glow (radius 32, (0.25, 0.3, 0.5)) and teleport eligibility. The glow is per character. |
| A blue circle icon appears on the map at the shrine, clickable to teleport | **Confirmed except the artwork**: the `shrine` sprite appears only for activated shrines, and a left click teleports after 3000 ms (ability 0x75) at no cost, in the ordinary map too. Sprite shape and colour **unverified** (asset). |
| Dying with no activated shrine sends you to the nearest one | **Confirmed with limits**: the nearest Shrine-of-Life site in the same land within about ±8192 blocks; otherwise the death cell. It requires pressing Revive. |
| Otherwise you go to the activated one | **Refined**: the nearest activated shrine **in the current land** (not the last-activated one). Activated shrines in other lands are ignored, and then the pass-2 rule applies. |

---

## 8. Constants

| Value | RVA / source | Use |
| --- | --- | --- |
| 0x35 | 0x16C7E0, 0x2C2A7B, 0x2C2ED5, 0x2D1100, 0x2EB7B0 | Shrine of Life building code |
| 0 | 0x2DAF57, 0x2C5250 case 0x89 | statue zone-object type |
| 0x18 / 4 | 0x2DAF79 / 0x2DAF8E | flute item type / subtype |
| 0x89 | 0x2C5250, 0x4F220 | flute ability; duration 5000 ms |
| 5000.0 | 0x42E5E4 | registration boundary |
| 64.0, 1/65536 | 0x42E5B4, 0x42E510 | ≤ 8 blocks offer (0x2B84D0); < 8 blocks registration |
| ±8 blocks (0x80000) | 0x2B84D0 | offer zone scan window |
| 0x6D / 0x6E | 0x2C5250; 0xA3A40 | flute1.wav at start / flute2.wav on new registration |
| 500 ms | 0x2C5250 | music-note particle period |
| 0x5A | 0xD3170 | flute animation id (+0xC44) |
| +1 to item +0x08 | 0x2C5250; tooltip 0xB7B90 "RANK n+1" | flute rank per new shrine |
| 32.0, (0.25, 0.3, 0.5), +1.0 above box top | 0xCE5F0 (0x42E7AC, 0x4AC980) | statue glow |
| 20.0, (0.1, 0.2, 0.4) | 0xCE5F0 | race-0x10C light when +0x19C ≠ 0 |
| kind 12, (site × 16 + 8) | 0x2EB7B0 | map place |
| 0x75, 3000 ms, (+6, +8, level) | steam-item-map-selection §2/§4 | map teleport |
| ±0x20000000 fixed (8192 blocks) | 0x2C2860 | pass-2 window |
| −1.0 | 0x4725D8 | "no best" |
| 1/65536 (double) | 0x43EF58 | respawn distance scale |
| (+6, +8, 0x2D87F0) | 0x2C2F46–0x2C2F9F | respawn point |
| 300000 | 0x98180 | tombstone timer |

## 9. Unresolved

* Whether a new character's inventory receives a real flute stack. If it does not, the "RANK" increments have no carrier.
* Persistence and synchronisation of PD+0xA8 (character save format, any host copy).
* The 0x2EB7B0 gate value (< 0x201) and which map zoom levels lack shrine markers.
* The `shrine` sprite's appearance, and whether the shrine-choice pulse colour tints the icon.
* Where race-0x10C creatures spawn and what sets their +0x19C.
* Exact music-note particle fields; held-flute model drawing; whether movement or damage can cancel ability 0x89 before the 5000 ms boundary. If cancelled earlier, no registration occurs.
* Behaviour of 0x16E00 for unloaded zones in the registration scan.
* The 0x2D1100 terrain/biome subtests beyond the building, land-kind and place-type checks.
* No runtime observation confirms any statement here.

FINAL / STABLE / HELD
