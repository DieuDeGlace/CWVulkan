# Map markers: the overlay's places and missions, the world map's creatures and tombstone

Behavioural specification of Steam Cube World, written from the decompiled executable and the
binary's constants. Routines are cited by RVA (image base 0x140000000). Offsets are byte
offsets into the record named. World positions are signed 64-bit integers in 1/65536 of a
block. Integer place positions are in whole blocks. Times are milliseconds.

Two drawing routines are covered:

* **The map overlay's draw**, 0x2829c0. It is the draw entry of the class `MapOverlayWidget`
  (vtable 0x46f408, second slot). It draws every flat marker over the map: an icon from the
  interface scene, a label, and an optional note. Part 1 covers it.
* **The world map's draw**, 0x2f44a0 (class `WorldMap`). It draws the map's 3D content
  through the map camera. Part 2 covers only its creature markers, its tombstone, and the
  list of player positions it hands to the overlay.

---

## 0. Objects, fields and terms

### 0.1 Who is who

| Name used here | How the overlay reaches it | What it is |
|---|---|---|
| **overlay** | its own `this` | the MapOverlayWidget |
| **window** | overlay +0x8 | the interface window. +0x180/+0x184: mouse x, y in pixels. +0x194: the interface's millisecond clock (int). +0x1c4/+0x1c8: window width, height in pixels. +0x48: the font system |
| **controller** | overlay +0x1a8 | the game controller (the object whose +0x118 is passed to the sprite loader 0xf9ac0, see 0.3) |
| **world** | controller +0x8 | the world. +0x40: creature list. +0x70: model table. +0x440: float32 elapsed world-update ms accumulator, advanced by0x2e2e80; distinct from calendar/time-of-day+0x0c (see §1.8.1). +0x448: the local player |
| **map** | controller +0x10 | the WorldMap object (Part 2) |
| **chardata** | local player +0x1528 | the player's character data (0.4) |

### 0.2 Controller fields used

| Offset | Meaning |
|---|---|
| +0x50 | the name generator, used for place, mission and land names |
| +0x664 | **guiScale**, copied from options.cfg `guiScale` (+0x1bcc) at start-up |
| +0x668 | the map is open and interactive (byte) |
| +0x669 | **shrine-choice mode** (byte). The game sets +0x668 and +0x669 together, for example from category1/subtype13 item use (0xb7740). That item queues interaction **kind1**, not kind13; its complete continuation is steam-item-map-selection.md. |
| +0x1bc8 | options.cfg `showRarity` (int; the options block starts at +0x1b88, see the writer 0x13b1d0) |
| +0x1bd0 | options.cfg `mapScale` (float) |
| +0x1db0 | the map is being dragged (byte) |
| +0x1e04 | the map zoom level **L** (int) |
| +0x1e20, +0x1e28, +0x1e30 | the map's centre (world position, 64-bit fixed point) |
| +0x1e50 | **mapScaleNow**: eased each frame toward options `mapScale` (+0x1bd0), rate 0.01 (game update 0xa5a50, easing helper 0x16ee0) |
| +0x1e58 | id of the creature the player is aiming at or looking at (chosen in 0x8ae50) |

### 0.3 Interface sprites

The loader 0xf9ac0 runs on `controller + 0x118`. It loads `gui.plx` and `quest-tag.plx` into one
interface scene. It then finds named nodes in that scene and stores pointers to them. The
field for the n-th pointer is at controller offset `0x118 + 8·n`. The sprites used here:

| Controller offset | Node name | | Controller offset | Node name |
|---|---|---|---|---|
| +0x460 | `enemy` | | +0x518 | `shrine` |
| +0x468 | `monster` | | +0x520 | `spirit` |
| +0x470 | `crystal` | | +0x528 | `bell` |
| +0x478 | `brazier` | | +0x530 | `harp` |
| +0x480 | `manapump` | | +0x538 | `whistle` |
| +0x488 | `npc` | | +0x540 | `solved` |
| +0x4a0 | `star` | | +0x548 | `gnome` |
| +0x4a8 | `cross` | | +0x550 | `hangglider` |
| +0x4b0 | `flight-point` | | +0x558 | `boat` |
| +0x4c0 | `home` | | +0x560 | `climbingspikes` |
| +0x4d8 | `armor-shop` | | +0x568 | `reins` |
| +0x4e0 | `weapon-shop` | | +0x570 | `destination` |
| +0x4e8 | `item-shop` | | +0x578…+0x590 | `crafting1` … `crafting4` |
| +0x4f0 | `identifier` | | +0x598 | `flight-master` |
| +0x4f8 | `smithy` | | +0x5a0 | `guild-hall` |
| +0x500 | `carpenters-shop` | | +0x5a8 | `flower` |
| +0x508 | `tailors-shop` | | +0x5b0 | `key` |
| +0x510 | `inn` | | +0x5b8 | `treasure` |
| | | | +0x5c0, +0x5c8 | `witch`, `demonportal` |

Three names come from wide strings in the data section: `npc` (0x440b78), `inn` (0x440d00) and
`key` (0x440e90). `unknown-flight-point` (+0x4b8), `object`, `static`, `mapposition` and
`direction` are loaded but not used by these routines.

### 0.4 Character data (chardata) fields used

All the sets and maps below are keyed by a pair of integers (x, y) unless noted.

| Offset | Content | Key |
|---|---|---|
| +0x28 | known flight points | block position |
| +0x38 | **discovered zones** | zone (0.5) |
| +0x58 | discovered lands | land cell = block ÷ 8192, floored |
| +0xa8 | **known places** (the used-cell set of lore-and-artifacts.md §1.1: opened chests, read lore, discovered cells) | 16-block cell = block ÷ 16, floored |
| +0xb8 | **marked missions** | zone |
| +0xc8 | a second mission set | zone |
| +0xd8 | a third mission set | zone |
| +0xe8 | **mission note timers**: zone → remaining ms (float, stored at node +0x24) | zone |
| +0x108 | star lists: a 4096-block cell → a list of block positions | cell = block ÷ 4096 |
| +0x128 | lore: a land id (unsigned int) → a collection of found lore pieces. Its element count is at node +0x30 | land id |
| +0x28c | **tombstone timer** (float ms) | |
| +0x290, +0x298, +0x2a0 | **death position** (world position, 64-bit fixed point) | |

Two predicates are used throughout:

* **Mission known** (0x5f4c0): the zone is in +0xb8 or in +0xc8.
* **Mission completed** (0x5e430): the zone is in +0xc8 or in +0xd8.

Both return false when the player has no character data.

### 0.5 Zones and the double-zone snap

A place's **zone** is (floor(x ÷ 1024), floor(y ÷ 1024)) of its block position. The world-gen
test 0x36160 marks some zones as the corner of a 2×2 double zone. When it holds, both zone
coordinates are rounded down to the nearest even number. Every zone key in this spec is
snapped this way. The world update builds the discovered-zone set the same way.

### 0.6 Map unit, regions, and the view square

* **unit** (blocks per map unit) = max(1, truncate(2.0^L × 4.0)). The constants are 0x4ac9d0
  (2.0) and 0x4aca20 (4.0). Levels 0, 1, 2, 3, 4, 5 give units 4, 8, 16, 32, 64, 128.
* A map region is 16 map units wide (int 0x542020 = 16 in the overlay, and 0x5469a0 = 16 in the
  world map). A region is therefore 16·unit blocks wide.
* **radius** = map +0x19c, written by the world-map draw: 8 (int 0x5469a4), or half that for
  the small corner map.
* The overlay visits regions (rx, ry) with rx from cx − radius to cx + radius − 1, and ry the
  same way. Here cx = (centre x in whole blocks) ÷ (16·unit). Both divisions truncate toward
  zero; neither floors.
* A region record is fetched by 0x286c60 (map, rx, ry, L). If the region is not built, it is
  skipped. A region record holds four lists: +0x18 places, +0x38 missions, +0x48 lands,
  +0x58 flight points.

### 0.7 Projection to the screen (overlay)

For a block position P:

1. Take the offset P − centre in fixed point. Multiply it by the integer
   truncate((1.0 ÷ unit) × 65536.0) and shift right by 16. Multiply by 1/65536 (0x42e510). The
   result is in map units.
2. Transform the result by the map's view matrix (map +0x58). If the view-space third
   component is ≤ 0, the point is not shown.
3. Transform by the map's projection matrix (map +0x18) and divide by w, giving (nx, ny, nz).
   The point is shown only if −1 ≤ nx ≤ 1 and −1 ≤ ny ≤ 1. The bounds are 0x4aca4c and
   0x4ac980.
4. Screen x = W·0.5·nx + W·0.5, and screen y = −H·0.5·ny + H·0.5, using the window's W and H.
   The marker's **depth** is nz·0.5.

---

## Part 1. The overlay's place and mission markers (0x2829c0)

### 1.1 The marker record

The draw first collects records, then sorts them, then runs a hover pass, then draws.

| Offset | Content |
|---|---|
| +0x00 | label text (wide string) |
| +0x20 | note text (wide string; empty, or "NEW") |
| +0x40 | scale |
| +0x44…+0x50 | label colour r, g, b, a |
| +0x54, +0x58 | screen x, y (pixels) |
| +0x5c | depth |
| +0x60, +0x64, +0x68 | the marker's block position x, y, z (hover and identity key) |
| +0x70 | icon sprite (or none) |
| +0x78 | overlay sprite drawn over the icon (or none) |
| +0x80 | flag A: shrine-choice target |
| +0x81 | flag B: flight target |
| +0x82 | flag C: hover reports this marker |
| +0x83 | flag D: hover reports its 16-block cell |
| +0x84 | flag E: "is a flight point" (used by flag B) |
| +0x88 | pointer to a hover-info block (or none) |

All flags start at 0. Unless stated otherwise, the colour is white (1, 1, 1, 1; the vector at
0x42e8c0), and the texts, icons and pointer are empty.

### 1.2 Frame start

Before collecting, the draw does the following:

* The map fields +0x150, +0x15c (the flight target and the shrine target) and +0x118 (the
  hovered land cell) are each set to (0x7fffffff, 0x7fffffff), meaning none.
* After collecting and sorting, the overlay's hover outputs are reset: the hover-info block at
  overlay +0x1b0 is zeroed, and the hovered NPC cell at overlay +0x250 is set to
  (0x7fffffff, 0x7fffffff).

### 1.3 Collection, per region (regions in the order of 0.6, x outer, y inner)

#### 1.3.1 Places (region list +0x18)

Each entry has its kind at +0x10 and its block position at +0x14, +0x18, +0x1c.

1. Compute the place's zone (0.5).
2. **Whether it may show.**
   * Kinds 12, 14 and 15 always pass this step.
   * Any other kind passes when the zone-kind routine 0x2b99d0, given the zone and its
     double flag, returns 0.
   * Otherwise it passes only when the zone is in the discovered zones (+0x38).
   * A place that fails is skipped.
3. Project the place (0.7). Skip it if it is not shown.
4. **Kinds 12, 14 and 15** also need their 16-block cell to be in the known places (+0xa8).
   Otherwise they are skipped.
5. **Scale** = (4.0 ÷ unit) × mapScaleNow. Multiply by 8.0 (0x4aca30) for kind 12, and by 4.0
   for kind 14. Clamp the result to at most 1.0 and at least 0.35 (0x4328a8). Places are never
   hidden by scale.
6. The label is empty and the colour is white.
7. Icon by kind:

| Kind | Icon | Kind | Icon |
|---|---|---|---|
| 2 | `inn` | 9 | `tailors-shop` |
| 3 | `item-shop` | 10 | `flight-master` |
| 4 | `armor-shop` | 11 | `guild-hall` |
| 5 | `weapon-shop` | 12 | `shrine` (also sets flag A) |
| 6 | `identifier` | 14 | `npc` (also sets flags C and D) |
| 7 | `smithy` | 15 | `treasure` |
| 8 | `carpenters-shop` | any other | `home` |

#### 1.3.2 Missions (region list +0x38)

The entry fields:

| Offset | Content |
|---|---|
| +0x10 | a name selector passed to the name routine |
| +0x14, +0x18, +0x1c | block position |
| +0x20 | mission kind |
| +0x24 | mission level (0…4) |
| +0x28 | hover-info block; it counts only when its first byte is non-zero |
| +0x40 | lore land id |

Steps:

1. Project the mission (0.7). Skip it if it is not shown.
2. Compute its zone (0.5).
3. **Name shown** is true when unit < 65 (zoom levels 0–4) and the zone is in the discovered
   zones (+0x38).
4. **Known** is the "mission known" predicate (0.4) on the zone.
5. **Lore promotion.** This happens when the mission kind is 14 and the mission is not known.
   The lore map (+0x128) is looked up by the land id at +0x40; a missing entry is created
   empty (0x7bd70). If the entry's element count exceeds 9 (ten or more lore pieces, which is
   100 % lore), then:
   * known becomes true;
   * the zone is added to the marked missions (+0xb8);
   * the note timer for the zone (+0xe8) is set to **18 000 ms**. This is the immediate
     0x468ca000; a missing entry is created first (0x7c1a0).
6. If neither "name shown" nor "known" holds, the mission adds no marker.
7. **Colour.** White if not known. If known, the level colour from 0x64680, given the level
   at +0x24 (table below).
8. **Scale.** s = ((128.0 ÷ unit) × mapScaleNow) ÷ 2.5. The constants are 0x46f584 and
   0x42e760.
   * If s < 0.3 (0x42e704), the mission adds no marker.
   * Otherwise the scale is s clamped to at least 0.66 (0x46f580) and at most 1.0.
   * With mapScaleNow = 1, the cut falls between unit 128 (s = 0.4) and unit 256 (s = 0.2).
9. **Label.** Empty unless the name is shown. When it is shown:
   * The label is the zone's generated name. The name routine 0x166ab0 runs on the name
     generator (controller +0x50) with the zone, the double flag and the value at +0x10.
   * Then, if the mission is known **and** `showRarity` is non-zero: a newline (wide string
     0x4361f0), followed by (level + 1) copies of the character U+00B1 (wide string
     0x437630). The game font draws U+00B1 as a star. Level 0 gives 1 star and level 4 gives
     5 stars.
10. **Note.** The note "NEW" (wide string 0x46f578) is attached when all of these hold:
    * the zone has an entry in the note timers (+0xe8);
    * truncate(window clock ÷ 600) is odd;
    * the mission is not completed.

    The note is therefore visible for 600 ms out of every 1200 ms, for as long as the timer
    entry lives (1.6).
11. **Icon.** Only when the kind is not 0 **and** the mission is known:

| Kind | Icon | Kind | Icon |
|---|---|---|---|
| 1 | `gnome` | 13 | `spirit` |
| 2 | `crafting1` | 14 | `destination` (also sets flag C) |
| 3 | `crafting2` | 16, 17 | `monster` |
| 4 | `crafting3` | 18 | `flower` |
| 5 | `crafting4` | 19 | `witch` |
| 6 | `hangglider` | 20 | `demonportal` |
| 7 | `boat` | 21 | `crystal` |
| 8 | `climbingspikes` | 22 | `brazier` |
| 9 | `reins` | 23 | `manapump` |
| 10 | `harp` | 24 | `key` |
| 11 | `whistle` | any other (15, 25+) | `enemy` |
| 12 | `bell` | | |

12. **Completed overlay.** When the kind is not 0 and the mission is completed, the overlay
    sprite is `solved`. This does not depend on "known".
13. The hover-info pointer is set to the entry's +0x28 when that byte is non-zero.
14. The mission's 16-block cell is also added to a temporary set that lives only for this
    frame. Its only later use is a lookup whose result is ignored, so it has no visible
    effect.

**Mission level colours** (0x64680; the level is clamped to −1…4, and anything below −1 is
treated as −1):

| Level | r, g, b, a | | Level | r, g, b, a |
|---|---|---|---|---|
| −1 | 0.7, 0.7, 0.7, 1 | | 2 | 0.3, 0.45, 1, 1 |
| 0 | 1, 1, 1, 1 | | 3 | 0.55, 0.2, 1, 1 |
| 1 | 0, 1, 0, 1 | | 4 | 1, 1, 0, 1 |

#### 1.3.3 Land names (region list +0x48)

These are included because they share the record list and the draw. Each entry has its land
kind at +0x10 and its block position at +0x14…+0x1c.

1. Project the entry (0.7). Its land cell (block ÷ 8192, floored) must be in the discovered
   lands (+0x58).
2. **Colour** by land kind (0x16b000):
   * 0 and 2: (0.5, 1, 0.25)
   * 1: (0.4, 0.65, 1)
   * 3: (0.5, 0.9, 0.1)
   * 4: (0.2, 0.8, 0.2)
   * 5: (0.4, 0.3, 0.9)
   * 6: (0.6, 0.7, 1)
   * 7: (0.7, 0.7, 0.7)
   * 8: (0.3, 0.7, 1)
   * 9: (1, 0.9, 0.2)
   * 10: (0.9, 1, 0.2)
   * 11: (0.7, 0.9, 1)
   * 12: (0.5, 0.1, 1)
   * 13: (0.7, 0.2, 0.2)
   * otherwise: white.

   Alpha is always 1.
3. **Scale** = ((512.0 ÷ unit) × mapScaleNow) ÷ 2.5 (0x46f588), clamped to 0.66…1.0. Land
   names are never hidden by scale.
4. **Label** = the land's generated name. It is built by 0x16be20 from three inputs: the
   word list for the land kind (0x16b210), the category word "named", and a number string
   (0x611d0). The number is 100000 minus a cell hash divided by 21474. The hash is the
   game's usual integer noise hash of (57·cell y + cell x), masked to 31 bits.
5. No icon and no flags.

#### 1.3.4 Flight points (region list +0x58)

These are collected only while the map's flight mode byte (map +0x164) is set.

1. Project the entry (0.7).
2. **Scale** = ((64.0 ÷ unit) × mapScaleNow) ÷ 2.5 (0x42e5b4). The point is hidden below 0.3.
   Otherwise the scale is clamped to 0.66…1.0.
3. The label is empty and the colour white.
4. **Icon** is `flight-point` if the block position (x, y) is in the known flight points
   (+0x28). Otherwise it is `cross`.
5. Flags B and C, and flag E, are set.

### 1.4 Collection after all regions

#### 1.4.1 Other players' name labels (map list +0x128)

The world-map draw (2.4) fills this map with creature id → clamped world position.

For each entry:

1. Look the creature up by id (0x0e720). Skip it if it is missing or its current ability
   (+0x68) is 0x9D.
2. Project the stored position (0.7).
3. **Scale** = ((64.0 ÷ unit) × mapScaleNow) ÷ 2.5, clamped to 0.66…1.0. Never hidden.
4. The label is the creature's name (+0x958). The colour is white.
5. The marker's block position is the creature's position (+0x10…+0x20) ÷ 65536. If the
   creature's ability is 0x9C (flying to a flight destination), the marker's block position
   is instead its destination (+0x1b0, +0x1b4). Its z is then the terrain height there
   (0x35ea0), stored as a block value. Only the hover identity changes; the label stays where
   the stored position projects.
6. Flag C is set. In flight mode (map +0x164), flag B is also set and the icon is
   `flight-point`.

#### 1.4.2 Stars (chardata +0x108)

1. The visited block square runs from (cx − radius)·16·unit to (cx + radius)·16·unit in each
   axis; the lower bound is included and the upper bound excluded.
2. For every 4096-block cell overlapping that square, look up the cell's list in +0x108.
3. A listed position (block x, y, z at entry +0x14…+0x1c) inside the square is projected (0.7).
4. **Scale** = ((64.0 ÷ unit) × mapScaleNow) ÷ 2.5. It is hidden below 0.3; otherwise clamped
   to 0.66…1.0.
5. The icon is `star`, the label empty, the colour white, and flag C is set.

### 1.5 Sorting

All records are sorted by depth, **largest first**, so far markers are drawn first and near
ones on top (sort 0x281ec0; it is not stable). The hover pass and the draw both walk this
order.

### 1.6 Note timers (chardata +0xe8)

* **Ticking.** Every world update (0x2da6c0), when there is a local player, each entry's value
  is reduced by the frame's elapsed ms. Entries that are still > 0 are kept; all others are
  removed (the map is rebuilt from the survivors).
* **Set when a townsperson marks a place** (game update 0x8ae50). After choosing the creature
  the player is aiming at (controller +0x1e58), the game looks it up. All of these must hold:
  * it exists;
  * its hostility kind (+0x60) is not 0;
  * its hostility kind is not 3, unless its class byte (+0x14c) is 0x9D or its flags (+0x80)
    have bit 0x1000;
  * its destination zone (+0x1b0, +0x1b4) is not 0x7fffffff;
  * that zone is **not** yet "mission known".

  If so, the zone is added to the marked missions (+0xb8) and to the discovered zones (+0x38),
  and its note timer is set to **180 000 ms** (immediate 0x482fc800). Because the timer is set
  only for a zone that was not yet known, this happens once per zone.
* **Set by the lore case** (1.3.2 step 5): **18 000 ms**.

### 1.7 Hover

The hover pass runs only while the map is open (controller +0x668) and not being dragged
(+0x1db0 = 0).

* **S** is the overlay node's on-screen scale. It is the x scale of the node's accumulated
  transform (0x100fa0 on overlay +0x178), which in practice is the interface scale.
* The pointer is the window's mouse position.
* **best** starts at −1.

For each record, in sorted order:

1. In shrine-choice mode (+0x669), only records with flag A are considered.
2. dx = mouse x − marker x, and dy = mouse y − marker y. Let d² = dx² + dy².
3. Half-height h = 25 (0x42e5a4) × S × scale. Half-width w starts as h.
   * If the label is not empty, measure it (0x352110) at font size truncate(scale × 12 × S)
     with outline truncate(scale × 3 × S).
   * Then w = max(w, (right − left) × 0.5).
4. The record is hit when |dx| < w, |dy| < h, and either best < 0 or d² < best. On a hit,
   best = d², and the following happen in this order:
   * Flag A: map +0x15c (shrine target) = the marker's (block x, block y).
   * Flag B: map +0x150 (flight target) = the marker's (x, y), and map +0x158 = (flag E is 0).
   * Flag C: if the hover-info pointer is set and its first byte is non-zero, that block is
     copied into overlay +0x1b0 (0x9cd0). Otherwise, if flag D is set, overlay +0x250 = the
     marker's 16-block cell (floor(x ÷ 16), floor(y ÷ 16)). In either case the marker's (x, y)
     is remembered as the **hover-chosen** position.
   * Always: map +0x100, +0x108, +0x110 = the marker's block position in fixed point, and map
     +0x118 = the land-map cell of (x, y) as returned by 0x2ada20.

Because outputs are written at every new best, a later and closer hit overwrites only the
fields its own flags write. Fields written by an earlier hit stay. That is the original
behaviour.

### 1.8 Drawing (per record, in sorted order)

Fonts and text come from `resource1.dat` (wide string 0x436340). Let **gs** = guiScale
(controller +0x664) and **t** = the window's signed32 clock. The shrine pulse instead
uses the distinct float world elapsed clock; the exact types/order are §1.8.1.

1. **Text scale and colour.** The text scale ts = the record's scale and the colour = the
   record's colour. In shrine-choice mode, a record with flag A instead gets:
   * colour = (0.75 + 0.25·cos(world clock × 0.01), 0.25, 1, 1), using constants 0x42e710,
     0x4ac95c and 0x42e51c;
   * ts = 1.2 (0x42e718) if the marker's (x, y) equals the shrine target (map +0x15c), else
     ts = 1.0.
2. **Icon** (if any).
   * **Icon scale** = the record's scale, or **1.25** (0x42e71c) when the marker's (x, y)
     equals the flight target (map +0x150), the shrine target (map +0x15c), or the
     hover-chosen position.
   * **Placement.** The node's transform is reset to identity and scaled by the icon scale.
     Its position, in interface units, is (x ÷ gs, y ÷ gs − 10 − 5) minus the node's own pivot.
     The constants are 0x42e598 = 10 and 0x42e584 = 5.
   * **Bob.** If the record has neither flag A nor an overlay sprite, cos(t × 0.005) × 5 is
     added to the y (0x42e518). The icon of a completed mission and shrine icons do **not**
     bob.
   * The node is drawn. The label's y is then moved down by 10 × gs pixels (truncated to an
     integer).
3. **Overlay sprite** (the completed mark `solved`). It is drawn at the record's scale (never
   enlarged by hover). Its position is (x ÷ gs, y ÷ gs − 15 + cos(t × 0.005) × 5), so it
   **always** bobs.
4. **Label.** Font size truncate(ts × 12) (0x42e59c), centred on x at the label's y. The
   label is drawn in two passes:
   * Pass one: fill white (0x42e8c0), outline colour = (r × 0.3, g × 0.3, b × 0.3, 1) of the
     colour from step 1, outline width truncate(ts × 3) (0x42e580).
   * Pass two: fill = the colour from step 1, outline width 0.
   * The result is coloured text with a darker outline. Both passes also pass the numbers 0
     and 3.0 to the text routine (0x34d360); their meaning was not established.
5. **Note** (only if the note text is non-empty). Same x, size and outline width as the label,
   at y = the label's y − 18 × gs (0x437fe0). Two passes:
   * Pass one: fill white, outline (0.6, 0.25, 0, 1) (0x46f590).
   * Pass two: fill (1, 0.8, 0.1, 1) (0x46f5a0), outline width 0.
   * The note is yellow text with an orange-brown outline. It blinks as described in 1.3.2
     step 10.

### 1.8.1 Shrine pulse clock versus GUI bob clock

The flag-A shrine-choice **text-color pulse** at **0x286002–0x286047**
reads **World+0x440 directly as float32 milliseconds**, through
overlay+0x1a8→Game+0x8→World. It is not the GUI/window clock and
is not a signed integer reinterpreted or converted to float. The
phase input is the float32 product of that float with constant
**0x3c23d70a** (approximately0.01, data RVA0x42e51c). Pass that
float to **cosf**, call0x286016; multiply the returned float by
**0.25f**, then add **0.75f**, with a float32 result after each
operation. Green is0.25f, blue/alpha1.0f. There is no integer
truncation, time modulo, GUI-scale factor or intermediate double
phase calculation in this consumer.

That world field is an **elapsed world-update accumulator**, not the
calendar/time-of-day field. **0x2e2e80** advances+0x440 by adding its
float32 elapsed-ms argument to the existing float32 field and storing
the float32 sum. The same routine advances calendar/time-of-day
**World+0x0c separately**. Do not substitute that calendar value or
a separately accumulated GUI/render elapsed clock for the pulse.
This statement specifies the read and its direct producer; it does
not infer an additional pause/reset policy at the overlay consumer.

The ordinary icon bob and solved-overlay bob are the distinct calls
**0x286298/0x286455**. Each reads **GUI engine/window+0x194 as
signed32**, converts that integer to float32 first (signed
CVTDQ2PS), then multiplies by float **0x3ba3d70a** (approximately0.005,
data0x42e518), and calls cosf. Its returned float is multiplied by
**5.0f** before adding to the relevant Y value. There is no unsigned
conversion or pre-conversion modulo. Negative signed clock words
therefore remain negative inputs to the conversion. The eligibility
for these two bobs remains §1.8: ordinary icons need neither flag A
nor an overlay sprite; the solved overlay itself always uses its bob.

---

## Part 2. The world map's creature markers and tombstone (0x2f44a0)

### 2.1 Context and order of drawing in 0x2f44a0

1. Reset the map cursor fields (+0x100…+0x110, +0x118) and clear the player-label map
   (+0x128).
2. Draw the terrain regions. In the same pass, draw the explored-cell cubes of each region.
3. Draw the hovered-land highlight cubes (only when +0x118 is set).
4. Collect the creature marker records and the tombstone record (2.2, 2.3), and write the
   player labels (2.4).
5. Sort the records by depth, largest first (0x2e9f50, comparing record +0xd8). Far markers
   are drawn first.
6. Draw each record (2.5).
7. Draw the flight route's dotted line. It is not covered here.

The marker scale depends on **viewScale**, the draw's 10th argument (the map view's scale).
Define k = viewScale × 0.6 × 80 ÷ 120 × 16 ÷ 8, which equals 0.8 × viewScale. The constants
are 0x42e708, 0x42e5b8, 0x42e7c8, 0x4aca34 and 0x5469a4.

### 2.2 Which creatures get a marker

Walk the world's creature list (world +0x40) in order. A creature gets a record when its
ability (+0x68) is not 0x9D **and** one of the following holds:

* its hostility kind (+0x60) is 0, meaning a player. This includes the local player and other
  players;
* its state flags (+0x128, 16-bit) have bit **0x100**, **and** the mission at its destination
  zone (+0x1b0, +0x1b4) is **not completed** (0x5e430). These are the creatures leading the
  character to a mission. Their hostility is not checked.

The record (0xe0 bytes, pushed by 0x2f43f0):

| Offset | Content |
|---|---|
| +0x00…+0x10 | position: the creature's displayed position (+0xb10, +0xb18, +0xb20), not +0x10 |
| +0x18 | turn in degrees (+0xb3c) |
| +0x1c | race (+0x64) |
| +0x20 | hostility kind (+0x60) |
| +0x24…+0xd3 | a copy of the appearance block (creature +0x78, 0xb0 bytes; 0x7e780) |
| +0xd4 | **scale** = 0.4 ÷ k (0x42e52c), which is 0.5 ÷ viewScale |
| +0xd8 | depth: the dot product of (position − centre) ÷ unit with the third column of the map's view matrix |

Appearance fields read later, with offsets relative to the appearance start:

* +0x02, +0x03, +0x04: hair colour r, g, b (bytes);
* +0x18: head model;
* +0x1a: hair model;
* +0x20: chest model (16-bit model indices).

### 2.3 The tombstone

**Record.** When the local player exists and the tombstone timer (chardata +0x28c) is > 0, a
record is added with:

* position = the death position (chardata +0x290…+0x2a0);
* turn 0;
* race **−1**;
* a default appearance (0x4d1f0);
* scale 0.4 ÷ k;
* depth as above.

**Set on revival** (0x98180). This runs while the local player's health is ≤ 0 and the
"Revive" button is pressed:

1. The player is revived (0x65fb0).
2. The death position is set to the player's current position, which is where they died.
3. The timer is set to **300 000 ms**, five minutes (immediate 0x48927c00).
4. The player is then moved to the respawn point (0x2c2860).

**Countdown** (world update 0x2da6c0). It runs each update while all of these hold:

* there is a local player;
* the player's current ability is not 0x98, 0x9C or 0x9D;
* world +0x210 is set.

Each such update:

1. The timer is reduced by the elapsed ms.
2. It is clamped at 0.
3. If the player's squared distance in blocks to the death position is **< 400** (0x42e7e8),
   meaning within 20 blocks, the timer is set to 0.

The tombstone vanishes when the player returns to it or after five minutes.

**Model.** `tombstone.cub`, model index 0xde7 in the world's model table (registered in
0x177f50). If the table has fewer than 0xde8 entries, there is no model and nothing is
drawn.

### 2.4 Player labels for the overlay

In the same walk, every creature meeting all three conditions below has its position written
into the map's label map (+0x128, keyed by creature id; 0x2ea370):

* its hostility kind is 0;
* it is not the local player;
* its ability is not 0x9D.

The position used is +0x10…+0x20, **after** the square clamp of 2.5 step 1. The overlay reads
this map (1.4.1).

### 2.5 Drawing each record

1. **Clamp to the shown square.**
   * Let R = 16 × unit × radius blocks.
   * With the offset d = position − centre, let |dx| be the truncated whole-block x
     difference. If |dx| ≥ R, scale the whole offset (x, y and z) by R ÷ |dx|.
   * Then, with the new offset, do the same for y.
   * The marker is pulled onto the edge of the square along the line to the centre.
2. **Models.**
   * Race −1: one model, `tombstone.cub`, colour white.
   * Otherwise, the first model is the head model. If the head index is negative, out of the
     table, or its slot is empty, the chest model is used instead. Its colour is white.
   * The second model is the hair model, with colour (hair r, g, b ÷ 255, alpha 1)
     (0x42e7d8 = 255).
   * An index outside the table, or an empty slot, draws nothing for that model.
3. **Pulse.**
   * For race −1, or any hostility kind other than 0: pulse = (cos(world clock × 0.006) + 1)
     × 0.5 (0x43ef34). It ranges 0…1, with a period of about 1047 ms.
   * For players the pulse is 0.
4. **Transform** for each model, in map units relative to the map's centre:
   1. translate by the clamped offset ÷ unit;
   2. scale uniformly by the record's scale;
   3. turn about the vertical axis by the record's turn, in degrees (0x13050);
   4. translate by −0.5 × the model's size (model +0x88, +0x8c, +0x90; constant 0x42e80c).

   The model is centred on the point.
5. **Outline and model.** For each model in order (head or chest, then hair):
   * First draw 26 copies, one for each offset (i, j, k) with i, j, k ∈ {−1, 0, 1} except
     (0, 0, 0). Each copy is offset by (i, j, k) model voxels, has colour (0, 0, 0, 0) and
     opacity 1, and passes the **pulse** in the model routine's highlight slot.
   * Then draw the model itself, with its colour, opacity 1 and highlight 0.
   * Model routine: 0x0ee2f0. Its 5th argument is the opacity; ≥ 1 means the opaque queue.
     The 8th argument is the highlight slot. The 9th is a layer value.
6. **Layer.** The 9th argument starts at 2.0 for the first (farthest) record and grows by 1.0
   per record. The outline copies use the layer value and the model uses layer + 0.5, so a
   marker's outline lies under its own models and nearer markers lie over farther ones.

The local player is drawn through this path (hostility kind 0), with head, hair, a black
outline and no pulse.

---

## 3. Constants

| RVA | Value | Use |
|---|---|---|
| 0x4ac9d0 / 0x4aca20 | 2.0 / 4.0 | unit = 2^L × 4 |
| 0x542020, 0x5469a0 | 16 (int) | map units per region |
| 0x5469a4 | 8 (int) | region radius |
| 0x42e510 | 1/65536 | fixed point to blocks |
| 0x4ac9b8, 0x42e590 | 1.0, 65536.0 (double) | offset scaling |
| 0x4aca4c, 0x4ac980 | −1.0, 1.0 | on-screen bounds; clamp top |
| 0x4ac964 | 0.5 | screen mapping; half label width; depth |
| 0x4aca30 | 8.0 | shrine place scale factor |
| 0x4328a8 | 0.35 | place minimum scale |
| 0x46f584, 0x46f588, 0x42e5b4 | 128, 512, 64 | mission / land / other numerators |
| 0x42e760 | 2.5 | scale divisor |
| 0x42e704 | 0.3 | hide-below scale; label outline darkening |
| 0x46f580 | 0.66 | minimum scale (missions, lands, flight, players, stars) |
| 0x42e5a4 | 25.0 | hover half-size per scale |
| 0x42e59c / 0x42e580 | 12.0 / 3.0 | font size / outline width per scale |
| 0x42e598 / 0x42e584 | 10.0 / 5.0 | icon rise and label drop / extra rise and bob amplitude |
| 0x42e518 | 0.005 | bob rate (cos of window ms × 0.005) |
| 0x42e71c | 1.25 | hovered icon scale |
| 0x42e718 | 1.2 | hovered shrine text scale |
| 0x42e51c, 0x4ac95c, 0x42e710 | 0.01, 0.25, 0.75 | shrine-mode pulse colour |
| 0x437fe0 | 18.0 | note offset above the label (× guiScale) |
| 600 (immediate) | ms | note blink half-period |
| 0x42e8c0 | (1, 1, 1, 1) | white |
| 0x46f590 | (0.6, 0.25, 0, 1) | note outline |
| 0x46f5a0 | (1, 0.8, 0.1, 1) | note fill |
| 0x46f578 | "NEW" | note text |
| 0x4361f0 / 0x437630 | "\n" / U+00B1 | rarity line and star |
| 0x436340 | "resource1.dat" | font |
| 0x468ca000 (immediate) | 18 000.0 | lore note timer |
| 0x482fc800 (immediate) | 180 000.0 | townsperson note timer |
| 0x48927c00 (immediate) | 300 000.0 | tombstone timer |
| 0x42e7e8 | 400.0 | tombstone clear distance squared (20 blocks) |
| 0x42e708, 0x42e5b8, 0x42e7c8, 0x4aca34 | 0.6, 80, 120, 16 | k = 0.8 × viewScale |
| 0x42e52c | 0.4 | marker scale numerator |
| 0x42e80c | −0.5 | model centring |
| 0x43ef34 | 0.006 | creature/tombstone pulse rate |
| 0x42e7d8 | 255.0 | hair colour bytes to floats |
| 0x42e520 | π/180 | degrees to radians |

---

## Open questions

1. **Text colour slots.** The text routine (0x34d360 → 0x34fa60 → 0x34fe40) takes three colours
   and an outline width. This spec reads the first colour as the fill and the second as the
   outline, which matches how the passes are built (the outlined pass is overdrawn by a fill
   pass with no outline). This was not confirmed in the text shader. Two more numeric
   arguments (0 and 3.0) and the label's vertical anchor (top or baseline at the given y)
   were not established either.
2. **Model routine slots.** In 0x0ee2f0, the 8th argument is read as a highlight or glow
   amount and the 9th as a layer or depth bias. Both readings come from how the callers use
   them (pulses and increasing per-marker values). The shader that consumes the queued values
   was not traced.
3. **viewScale.** The world map draw's 10th argument comes from a parameter of the big render
   routine 0x0e2e00 (through 0x0e64b0). Where that value is computed was not traced.
4. **Mission sets.** What +0xb8, +0xc8 and +0xd8 each mean is not established; only the two
   predicates built from them are. +0xc8 appears in both.
5. **Flight-point icon.** The flight-point icon reads "`flight-point` when the position is in
   chardata +0x28, otherwise `cross`". Separately, the click handler in 0x094900 appears to
   allow a flight only when the same test is false. The sense of that click test should be
   checked before the flight UI is ported; the drawing rule above is as found.
6. **Creature fields.** The meanings of creature ability 0x9D (skipped everywhere) and 0x9C
   (flying to +0x1b0/+0x1b4), and of hostility kind 3 in the townsperson rule, are inferred
   from use.
7. **Creature +0x1b0.** For townspeople, +0x1b0/+0x1b4 is used as a zone key. For a flying
   player, the same field is a block position. Both uses are as found.
8. **Sort ties.** The record sort is not stable, so the order of equal-depth markers is
   unspecified.
