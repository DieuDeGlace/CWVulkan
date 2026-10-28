# World generator, part B: tables

Appendix to `worldgen-structures.md`. Every table here is the game's own data, read from the
switch routines and constants of the Steam build (image base 0x140000000). "Type" is a cell
object type (§T1) unless the table says "object type" (a static object, zone +0x60) or "model
type" (zone +0x18).

---

## T1. Cell object types

What the builders make of each cell object type (`worldgen-structures.md` §6 says where each
comes from). The words are descriptions of the geometry, not the game's names.

| Type | What is built | Builder |
|---|---|---|
| 0 | nothing | — |
| 1 | common tree | 0x264670 |
| 2 | fir | 0x263210 |
| 3 | birch | 0x2626e0 |
| 4 | willow | 0x263ea0 |
| 5 | palm | 0x265e70 |
| 6 | common tree on a round stone pedestal, four benches | inline (0x3155ec) |
| 7 | palm on a round stone pedestal, four benches | inline |
| 8 | giant common tree (50 high, three times as wide), at the cell's centre | 0x264670 |
| 9 | rock | 0x146450 |
| 10 | rock hill (highlands) or earth hill | 0x145fa0 / 0x145eb0 |
| 11, 12 | rock hill (64 high; 40 / 80 wide), at the cell's centre | 0x145fa0 |
| 13, 14 | rock hill as 11 / 12, plus a building (13: roof kind 3; 14: a plinth with an item) | 0x145fa0 and the building builder |
| 15 | hill with a canyon | 0x145b30 / 0x145cf0 |
| 16 | building with a plinth and an item | building builder |
| 17, 18 | no structure; a crate or chest cell (§C3 of the contents file) | — |
| 19 | stump | 0x266ad0 |
| 20, 21 | fallen log (20 hollow one time in three; 21 always hollow) | 0x2651f0 / 0x265840 |
| 22–26 | town and village houses (roof kind 1, 26 detached) | building builder |
| 27–33 | crop fields | 0x305115 |
| 34 | farmyard: fences and a few things | 0x305115 |
| 35–39 | village buildings (roof kind 4; 39 a 2×2 special) | building builder |
| 40–43 | towers (roof kind 2) | building builder |
| 45–48 | fortress walls, gate and corner towers (roof kind 5) | building builder |
| 49–51, 53 | small stone buildings (roof kind 3) | building builder |
| 52 | wizard's tower | 0x3039df |
| 54, 55 | boulder (on a rock hill when under water) | 0x146560 |
| 56, 57, 61 | stone pieces of rock and tomb places (57, 61: corner posts) | building builder |
| 60 | mound with a hilltop building | 0x146090 and building builder |
| 62 | sunken chest | 0x3158c6 |
| 63–65 | wooden platforms (place landmarks) | building builder |
| 66 | water tower | 0x302ce7 |
| 67 | aqueduct | 0x303575 |
| 68 | cistern | 0x303244 |
| 69, 70 | round tower platform (70 on a spiral ramp) | 0x3029aa |
| 71–75, 76–80, 81–85 | the five-cell rows of big structures 43, 42 and 47 | building builder |
| 86 | 2×2 building of big structures 41 and 44 (roof kind 6) | building builder |
| 87–89 | kind-22 buildings (roof kind 8; 89 has a door) | building builder |
| 90, 91 | town hall pair (roof kind 9; 91 the hall) | building builder |
| 92 | town building with an object 0x75 (117) beside it | building builder |
| 93 | no structure (inside rock places and arenas) | — |
| 94, 95 | 2×2 buildings of big structures 45 and 46 | building builder |
| 96 | arena corner towers | building builder |
| 97 | arena master's dais (kind-24 arenas; its creature is part C's) | 0x303315 |
| 98, 99 | camp cells of place kinds 34 and 35 (§C5 of the contents file) | — |
| 100 | area landmark building (roof kind 11) | building builder |

A type is a **building** when its floor height (T3) is above 0 (0x31ab90).

## T2. Roof kind (0x25fff0)

| Roof kind | Types |
|---|---|
| 1 | 22, 23, 24, 26 |
| 2 | 40, 41, 42, 43 |
| 3 | 13, 49, 50, 51, 53, 60, 62, 69, 70, 71, 72, 73, 74, 75 |
| 4 | 35, 36, 37, 38, 39 |
| 5 | 45, 46, 47, 48 |
| 6 | 86, 94 |
| 7 | 76, 77, 78, 79, 80 |
| 8 | 87, 88, 89 |
| 9 | 90, 91 |
| 10 | 95 |
| 11 | 81, 82, 83, 84, 85, 100 |
| 0 | every other type |

## T3. Floor height (0x25f540)

| Floor height | Types |
|---|---|
| 8 | 22, 23, 24, 25, 26, 87, 88, 89 |
| 10 | 16, 35, 36, 37, 38, 39, 49, 50, 51, 53, 57, 61, 63, 64, 65, 71–85, 90, 91, 96 |
| 15 | 13, 40, 43, 60, 86, 94, 95, 100 |
| 25 | 45, 46, 47 |
| 0 | every other type (not a building) |

## T4. Building height (0x25f6a0)

| Height | Types |
|---|---|
| 10 | 16, 35, 36, 37, 39, 49, 50, 51, 53, 57, 61, 63, 64, 65, 71–80, 82–85, 90 |
| 15 | 13, 60, 86, 94, 95, 100 |
| 16 | 23 |
| 18 | 69 |
| 20 | 38 |
| 25 | 40, 41, 42, 45, 46, 70 |
| 30 | 91 |
| 35 | 81 |
| 60 | 43, 47, 96 |
| 8 | every other type |

## T5. Most floors (0x25f5f0)

| Floors | Types |
|---|---|
| 1 | 13, 22, 24, 25, 26, 35, 36, 37, 39, 45, 46, 49, 50, 51, 53, 55, 58, 59, 60, 71–91, 94, 95, 100 |
| 2 | 23, 38, 40, 47 |
| 3 | 43, 96 |
| 0 | every other type |

## T6. Reach of a cell object (0x25f790)

How many cells beyond its own an object may build into.

| Reach | Types |
|---|---|
| 2 | 1–15, 52, 53, 60, 66, 69, 70 |
| 1 | 16, 22, 23, 24, 25, 35–43, 45, 46, 47, 49, 50, 51, 54, 55, 57, 61, 62, 71–91, 95, 96, 100 |
| 0 | every other type, and 0 |

## T7. Building tests by type

| Routine | True for |
|---|---|
| keeps its height, 0x25e9a0 | type 52: bit 0 of the two-step xorshift of (style + 0x2367) (see §0 of the main file); otherwise 13, 22–25, 35–40, 43, 47, 49, 53, 60, 71–91, 95, 96, 100 (masks 0x1f803c02000 for types below 41 and 0x231fffff0020451 from 43) |
| has battlements, 0x25e940 | type 52: bit 0 of the complement of the two-step xorshift of (style + 0x2ee3); otherwise 40–47 except 44, and every roof-kind-9 type |
| joins a tower, 0x25eb60 (a, b) | a and b both in 40–47 but not 44, and building height (T4) of a ≤ that of b |
| roofed, 0x25ea00 | false for 13, 16, 25, 26, 49, 50, 51, 53, 60 (mask 0x102e000006012000 below 61), 86, 94, 95 and roof kind 9; true otherwise |
| attached house, 0x31ac20 | roof kind 1 except 26; roof kinds 8, 2, 7; and type 25 |
| water line, 0x25ec30 | roof kind 3 and not 13, 51, 53, 60 (mask 0x1028000000002000 below 61), and bit 0 of the two-step xorshift of (style + 0xd55) |
| grassy floor, 0x25ebc0 | roof kind 3 and bit 0 of the two-step xorshift of (style + 0x13c55) |
| has a door, 0x31abb0 | 24, 36, 37, 46, 71, 76, 81, 89, 92, 100 (mask 0x120210800000403 from 36) |
| house, 0x2b8030 | 22–26 and 92 |
| crate cell, 0x2b4410 | 14, 15, 17, 18, 21, 60, 62 (mask 0x500000000026c000 below 63), 100 |
| lore cell, 0x2b42e0 | 13, 37, 55, 69, 70 (mask 0x300040001000001 from 13) |
| fence-free neighbour, 0x31a7c0 | not 27–34 and floor height (T3) ≤ 0 |
| light object, 0x31d470 | object types 0, 16, 63, 64, 65, 66, 83, 112, 113, 114, 126 (mask 0x800e00000010000f from 63) |
| region re-stamped item, 0x31ac70 | item categories 2–9, 15, 22, 24, 26, 30 (mask 0x454083fc) |

## T8. Static object sizes (0x13a520)

Width (x), depth (y), height (z) in blocks at scale 1; 0x13a960 multiplies each by the
object's scale (+0x24), x, y, z in that order. An odd turn swaps width and depth where the
users say so.

| Size | Object types |
|---|---|
| 2, 2, 4 | 0 |
| 4, 4, 4 | 1, 94 |
| 2, 2, 3 | 2 |
| 2, 1, 3 | 3, 128, 129 |
| 4, 1, 4 | 4, 5 |
| 2, 1, 2 | 6, 127 |
| 2, 1, 4 | 7 |
| 1, 0.1, 1 | 8–16 |
| 3, 2, 1 | 21, 22 |
| 3, 3, 1 | 24 |
| 1, 1, 0.5 | 28 |
| 3, 0.4, 0.4 | 30, 31 |
| 1.65, 2.1, 1 | 32 |
| 2, 2, 2 | 37, 38, 100, 101 |
| 2, 2, 1 | 39, 102, 103, 104 |
| 3, 1, 4.5 | 42, 45–56 |
| 4, 1, 1 | 44 |
| 2, 1, 1 | 60, 61, 62, 67–73 |
| 2, 2, 8 | 63, 64, 66 |
| 1, 1, 4 | 65 |
| 2, 2, 0.5 | 83 |
| 3, 3, 4 | 84, 85, 86, 87 |
| 3, 3, 2 | 88, 93, 116, 126 |
| 1, 1.5, 1 | 89 |
| 2, 2, 0.25 | 95, 96 |
| 3, 3, 3 | 98 |
| 1.5, 1.5, 1 | 99 |
| 1.5, 1.5, 2 | 118–124 |
| 3, 1, 5 | 125 |
| 2, 3, 3 | 130 |
| 4, 4, 2.5 | 131, 132, 133 |
| 1, 1, 1 | every other type |

The literals are single-precision: 0.1 is 0x3dcccccd, 1.65 and 2.1 are the nearest floats.

## T9. The building culture (0x25ea50)

From the place's style s (main file §5) and the region's biome b (worldgen-terrain.md, the
biome routine 0x2b9080). Let v = (s + 0x2317) as 32 bits, w = v xor (v << 13), and
u = (w >> 17) xor w (a two-step xorshift).

| Biome b | Culture |
|---|---|
| 3, 6 | 3 − (u & 1): 3 when bit 0 of u is clear, 2 when set |
| 4 | 6 when bit 0 of u is set, else 5 |
| 5, 12 | 4 |
| 7 | 1 |
| 9 | 5 |
| 10 | 6 |
| any other | by u & 3: 0 → 0, 1 → 2, 2 → 1, 3 → 3 |

## T10. Town layouts (world +0x4a0)

Made once per world by the world set-up 0x2b5f9a, before the world's random numbers
(worldgen-terrain.md describes that set-up and its draws): srand(world seed), then 64 rounds.
Each round fills a 16-entry list with 2, 3, …, 11 in entries 0–9 and 0 in entries 10–15,
then shuffles it with the C++ library shuffle as MSVC builds it: for index i from 1 to 15, a
draw n = i + 1, r = rand() repeated while r / n ≥ 0x7fff / n and 0x7fff mod n ≠ n − 1 (the
"uniform from rand" rejection), then entries i and r mod n are swapped when they differ.
Round k's list is layout k. Entry (row j, column i) = list[j·4 + i].

## T11. Wall colours by type (0x2600f0)

Inputs: type t, cell (cx, cy), style s, building culture c (T9), and "xs(n)", the full
three-step xorshift of n (main file §0). A **style colour** SC(k, m, a, r, g, b, n) is
K + xs(s + r) mod n, K + xs(s + g) mod n, K + xs(s + b) mod n with K = xs(s + k) mod m + a, all
unsigned 32-bit, then converted to float.

| Type | Colour (r, g, b) |
|---|---|
| 45–47 | SC(0xd6a, 150, 100, 0x18148, 0x108a, 0xbf99, 50) |
| 16 | 140, 140, 140 |
| 25 | timber colour (T13) |
| 52 | SC(0x2c4, 150, 100, 0x14cb, 0x7b, 0xdf, 50) |
| 53 | 240, 250, 260 |
| 55 | 200, 200, 200 |
| 57, 61 | 135, 130, 140 |
| 63, 64, 65 | 160, 80, 10 |
| 66, 67, 68 | 128, 128, 140 |
| 94 | 260, 250, 130 |
| 95 | 180, 170, 90 |
| 96, 97 | 180, 170, 120 |

Otherwise by roof kind (T2):

| Roof kind | Colour |
|---|---|
| 1 | type 26: SC(0x2c4, 100, 100, 0x14cb, 0x7b, 0xdf, 30). Otherwise by culture: 2 → patchwork (below); 3, 6 → timber colour; 4 → SC(0x2c4, 40, 40, 0x14cb, 0x7b, 0xdf, 30); 5 → (xs(s + 0x237b) mod 30 + 250, xs(s + 0x13c3) mod 30 + 240, xs(s + 0x2245) mod 30 + 230); others → SC(0x2c4, 100, 100, 0x14cb, 0x7b, 0xdf, 30) |
| 3 | SC(0x2c4, 150, 100, 0x14cb, 0x7b, 0xdf, 50) |
| 4 | 135, 130, 140 |
| 6 | 200, 200, 200 |
| 7 | 200, 100, 20 |
| 8 | 80, 80, 80 |
| 9 | SC(0x2c4, 100, 100, 0x14cb, 0x7b, 0xdf, 30) |
| 11 | 180, 170, 90 |
| other | 240, 240, 240 |

**Patchwork** (culture 2): h = rnd(H(⌊cx/3⌋, ⌊cy/3⌋)) (floor division). s′ = h / 3 when
h mod 3 = 0, else s. a = xs(s′ + 0x2c4) & 0xff. Then, in float: g = (xs(s′ + 0x7b) mod 200 + 50)
× 0.5, r = a × 0.5, b = (255 − a) × 0.5; the colour is (r + 120, g + 120, b + 120).

## T12. Trim colours (0x25ece0)

"wall" is T11 for the same type; "mossy(t)" is wall(t) × 0.25 then, component by component,
(22.5, 60, 22.5) + that (the addition 0x172b0 adds the constant first); "aged" is wall × 0.75
then + (15, 30, 15).

| Case | Colour |
|---|---|
| types 45–47 | by xs(s + 0x38e2b) mod 3: 1 → mossy(type); 2 → SC(0x2385, 150, 100, 0x89a, 0x158, 0x933, 50); 0 → wall |
| roof kind 1 or 9 | by culture: 2, 3 → wall × 0.8; 4 → SC(0x11240, 40, 40, 0x18d0, 0x7b, 0x14a, 30); 5 → (xs(s + 0x15a9c) mod 30 + 250, xs(s + 0x17845) mod 30 + 240, xs(s + 0xf8b0) mod 30 + 230); 6 → aged; others → SC(0x1a925, 100, 100, 0x18d0, 0x15a9e, 0x127, 30) |
| roof kind 3 | by xs(s + 0x9167) mod 3: 1 → aged; 2 → SC(0x1a0b, 150, 100, 0x18a93, 0x7fc, 0x1538, 50); 0 → wall |
| type 52 | by xs(s + 0x9167) mod 3: 1 → mossy(52); 2 → SC(0x1a0b, 150, 100, 0x18a93, 0x7fc, 0x1538, 50); 0 → wall |
| otherwise | wall |

## T13. Timber colour (0x260a00)

By building culture: 4 → (xs(s + 0x20b3) mod 10 + 130, xs(s + 0x291) mod 10 + 120,
xs(s + 0x110e) mod 10 + 110); 6 → (… mod 50 + 150, … mod 30 + 120, … mod 30 + 90); any
other → (… mod 50 + 130, … mod 30 + 100, … mod 30 + 50), with the same three offsets.

## T14. Roof colours (0x25f840)

"three(mr, ar, mg, ag, mb, ab)" is (xs(s + 0x20b3) mod mr + ar, xs(s + 0x291) mod mg + ag,
xs(s + 0x110e) mod mb + ab).

| Case | Colour |
|---|---|
| 25 | timber colour |
| 47 | 50, 50, 220 |
| 52 | (xs(s + 0xf8d) mod 200 + 50, xs(s + 0xd0d) mod 200 + 50, xs(s + 0x16c9) mod 200 + 50) |
| 55 | 200, 200, 200 |
| 63–65 | 160, 80, 10 |
| 94 | 260, 250, 130 |
| 95 | 180, 170, 90 |
| 96, 97 | 120, 110, 90 |
| roof kind 1 or 9 | by culture: 3 → timber × 0.7 (0x3f333333); 4 → three(20, 50, 20, 50, 20, 50); 5 → wall colour; 6 → three(50, 200, 100, 100, 30, 50); others by xs(s + 0x1bfb) mod 3: 1 → three(20, 100, 20, 100, 100, 150), 2 → three(50, 130, 30, 100, 30, 50), 0 → three(20, 50, 20, 50, 20, 50) |
| roof kind 3 or 6 | wall colour |
| roof kind 4 | 135, 130, 140 |
| roof kind 8 | 60, 40, 10 |
| roof kind 11 | 180, 170, 90 |
| otherwise | 50, 50, 50 |

## T15. Castle colours

By the castle cell's style byte (0x2a8290's third byte, `worldgen-structures-buildings.md` §9)
and the zone's big structure B (main file §5).

| | style 2 | B 40 | 41 | 42 | 43 | 44 | 45 | 46 | 47 | other |
|---|---|---|---|---|---|---|---|---|---|---|
| first colour, 0x2fec70 | 255, 255, 30 | 120, 100, 180 | 100, 160, 90 | 250, 200, 80 | 250, 280, 300 | 200, 250, 250 | 270, 250, 100 | 250, 230, 100 | 130, 100, 120 | 100, 160, 200 |
| second colour, 0x2feb30 | 255, 150, 100 | 60, 180, 160 | 100, 180, 160 | 129, 255, 255 | 180, 200, 260 | 50, 160, 40 | 270, 200, 100 | 200, 230, 100 | 130, 160, 160 | 129, 255, 255 |

Light colour of castle room kinds (inline): 9 → 10, 10, 10; 10 → 20, 100, 100;
11 → 260, 100, 10; 12 → 50, 150, 20; other kinds have none.

Room depth (0x2feaf0): kind 9 → 40, 10 → 20, 11 and 12 → 8, others 0. Room rise (0x2aa250):
20 when the style byte is 1, else 16.
