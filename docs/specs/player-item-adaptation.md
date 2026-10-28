# Player: class-adapted treasure items (0x10a720)

Behavioural specification of the routine that turns a generic treasure item (item type 30)
into the concrete item the local player sees and receives, chosen for the player's class.
Written from the decompiled executable (routines by RVA, image base 0x140000000). Item fields
are those of `drops-and-items.md`: type (byte +0), subtype (u32 +4), modifier (u32 +8), region
(two i32, +0xc and +0x10), rarity (byte +0x14), formula / second id (u32 +0x18), material
(byte +0x1c), attributes from +0x20, attribute count (byte +0x9d); an item is 0xa0 bytes.

**X(v)** is the 32-bit xorshift step used throughout the game (`conversation-script.md` §2.1):
v ^= v << 13; v ^= v >> 17; v ^= v << 5, on unsigned 32-bit values. "mod" is unsigned. All
additions to the seed wrap at 32 bits.

---

## 1. Interface

0x10a720(item I, output O, creature C) writes O and returns it. C is the creature the item is
being shown to or given to (the local player at every call site); only C's class byte (+0x14c)
is read.

Callers (all treat the result as the item to show or to hand over): 0x98180 (interaction
with an item), 0xa5a50 (the frame routine), 0xb6b00, 0xd3170 (drawing items in the world) and
0x28c290 (the item preview widget). So a type-30 treasure looks, in the world and in its
preview, like the item the player will get.

## 2. The output item

When the routine builds a new item, it starts from an item that is **all zero** except as
stated below (bytes +0x1 … +0x3 and +0x9e … +0x9f are left unset, open question 1). "Keep
region, rarity, modifier" means: O's region = I's region, O's rarity = I's rarity,
O's modifier = s (below). O's subtype is 0 unless stated.

## 3. The rule

1. If I's type is not 30: O = I (an exact copy). Done.
2. Let s = I's modifier and r = X(s) mod 7.
3. **Gold.** If I's subtype is 1: O's type = 31, O's rarity = I's rarity, O's modifier = s.
   Everything else stays zero, **region included**. Done. (r is computed but unused.)
4. **Ring or amulet.** If r = 5, O's type = 9; if r = 6, O's type = 8. Then O's material =
   12 when bit 0 of X(s + 1) is 0, else 11; keep region, rarity, modifier. Done.
5. Otherwise the class decides (C+0x14c). For a class other than 1 … 4 the routine returns
   **without writing O** (O keeps whatever the caller put there). Done.
6. **Weapon** (r = 0). O's type = 3; keep region, rarity, modifier. Let t = X(s + 1) mod 3.

   | Class | Material | t = 0 | t = 1 | t = 2 |
   |---|---|---|---|---|
   | 1 warrior | 1 | subtype X(s + 2) mod 3 | subtype 15 + X(s + 2) mod 3 | subtype 13 |
   | 2 ranger | 2 | 6 | 7 | 8 |
   | 3 mage | 2 (t = 0, 1); see below for t = 2 | 10 | 11 | 12 |
   | 4 rogue | 1 | 3 | 4 | 5 |

   For a mage with t = 2 the material is 12 when bit 0 of X(s + 2) is 0, else 11.
7. **Armour** (r = 1 … 4). O's type = 4 + (X(s + 1) mod 4); m = X(s + 2) mod 3; O's material
   by class and m; keep region, rarity, modifier.

   | Class | m = 0 | m = 1 | m = 2 |
   |---|---|---|---|
   | 1 warrior | 1 | 29 | 4 |
   | 2 ranger | 27 | 31 | 33 |
   | 3 mage | 26 | 32 | 35 |
   | 4 rogue | 28 | 30 | 34 |

Notes for an implementer:

* In steps 4, 6 (mage) and 7 the game computes the bit or the residue mod 4 from the first two
  xorshift lines only; the third line (v ^= v << 5) never changes the low five bits, so using
  the full X gives the same result.
* The warrior's "15 + …" case is computed by the game as v + (5 − v / 3) × 3 with unsigned
  32-bit arithmetic, which equals 15 + v mod 3.
* The attributes, formula and attribute count of I are never carried over; O has none.

---

## Verification

There is no recording of this routine yet. Oracle: an `Extract\oracle.request` command in
ZoneExtractor that calls 0x10a720 on scripted inputs (type 30, subtypes 0 and 1, a few
hundred modifiers, each class 0 … 5, and a non-30 item) with a pre-filled output buffer, and
writes input, class, the buffer before and the 0xa0 bytes after. Compare all 0xa0 bytes,
masking bytes +0x1 … +0x3 and +0x9e … +0x9f.

## Open questions

1. **Unset bytes.** The output is built on the stack with bytes +0x1 … +0x3 and
   +0x9e … +0x9f not written, then copied whole (0x35400) into O; they hold stack garbage.
   An implementation should zero them; recordings should mask them.
2. **Classes outside 1 … 4** leave O unwritten; what the callers then show was not checked
   (in normal play the player always has a class 1 … 4).
3. **Item names** (type 8 amulet, 9 ring, 31 coins, 4 chest, 5 hands, 6 feet, 7 shoulders;
   materials 11 gold and 12 silver; weapon subtypes as in `frontend-characters.md` §5.5) are
   for orientation only; only the numbers are certain.
