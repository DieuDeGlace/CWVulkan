# Loot: ground items, the drop hop, pick-up and receiving items (0x2e3510, 0xd3170, 0xa5a50, 0xa0750, 0xb40d0)

Behavioural specification of Steam Cube World, written from the decompiled executable, its
assembly and the binary's constants. Routines are cited by RVA (image base 0x140000000).
The ground item record (0xe0 bytes, zone list +0x30) is `drops-and-items.md` §0.4; how it is
targeted and auto-picked is `use-prompt.md` §2.4; the server side of picking up (interaction
kind 7, the pick-up notice) is `zone-object-interaction.md` §6; the class adaptation of
treasure items is `player-item-adaptation.md`; the inventory add is `worldgen-creatures.md`
§4.5. Positions are 64-bit integers in 1/65536 of a block. "rand" is the C runtime's
`rand()`.

This file answers what happens to an item between being laid on the ground and being in the
player's inventory, and how items given without a ground item (conversation pages, taming)
arrive.

---

## 1. A ground item's life (the world update 0x2e3510, ground-item part)

For every loaded zone, for every ground item G in its list (+0x30), in order, each world
update with the elapsed time dt (ms, float):

1. **Wall items.** If G's type is 0x11: four block tests snap it against a wall. In order:
   the block at (G's x, G's y − 1 block); (x, y + 1 block); (x − 1 block, y); (x + 1 block, y),
   all at G's ground height (+0xb0). Each that is solid (block type with bit 1 masked off is
   not 0) moves G against it and sets its angle (+0xc0):

   | Solid neighbour | New coordinate | Angle |
   |---|---|---|
   | y − 1 | y = tb(y) × 65536 + d | 180.0 |
   | y + 1 | y = (tb(y) + 1) × 65536 − d | 0.0 |
   | x − 1 | x = tb(x) × 65536 + d | 90.0 |
   | x + 1 | x = (tb(x) + 1) × 65536 − d | 270.0 |

   where d = trunc(13107.2) = 13107 (0x4725ac, 0.2 block) and tb is the truncated block
   (position / 65536 rounded toward zero, `drops-and-items.md` §0.3). Later tests override
   earlier ones; the tests read the coordinates as already moved.
2. **Timer +0xcc.** If G +0xcc (i32) is above 0: it becomes max(trunc(float(+0xcc) − dt), 0).
   (Laid items start with 0; its writer is not among the loot routines.)
3. **Pick-up delay +0xc8.** If G +0xc8 (float) is above 0: subtract dt. If it is now ≤ 0, set
   it to exactly 0 and:
   * unless the world is a server (+0x358 set): a sound request at G's position, id 0x3d when
     G's type is 12 (a coin), else 0x3e; pitch 1.0 + float(rand) × 0.1 / 32767.0; volume 1.0
     (`sound.md` §5.2). One rand.
   * if there is a local player, G's type is 3..9 or 0x1a and G is reserved for that player
     (+0xd8 equals its +0x968): a second sound request, id 0x5d, pitch 1.0, volume 1.0.

Nothing in this update moves an item toward a player; ground items never fly.

## 2. Drawing a ground item: the drop hop (0xd3170)

Each frame, for every zone within 3 zones of the local player and each ground item G with
+0xcc = 0:

1. The item drawn is the class-adapted one (0x10a720 with the local player); its model comes
   from the model table (`item-models.md`). Without a model it is skipped, as is an item that
   fails the visibility test (0x2b8140, `use-prompt.md` §2.4) or the view test.
2. **The hop.** While +0xc8 > 0, the drawn position is raised by
   trunc((1.0 − h²) × 5.0 × 65536) with h = (+0xc8 − 250.0) / 250.0 (0x43293c, 0x42e584):
   a parabola that is 0 at 500 ms left, 5 blocks at 250 ms and 0 again at 0.
   Laid items start with 500..799 ms (`drops-and-items.md` §1 step 2), so for the first
   0..299 ms h is above 1 and the item is drawn **below** its ground point (down to about
   19 blocks at 799 ms), then it rises out, peaks 5 blocks up and lands (open question 1).
3. While +0xc8 ≠ 0 the model is also turned about its centre by +0xc8 × 0.7 (0x4328ac;
   degrees, the axis chosen by the model's orientation code) — it tumbles while it hops.
4. Types 12, 13 and 14 and the special types 0x17, 0x18, 0x1d, 0x1e get extra motion from the
   world clock (world +0x440, through cosine terms of 2 × clock × 0.2 degrees); types 3 and 4
   lie on their side. The exact transforms are not specified here (open question 7).
5. Once +0xc8 is 0, gear (types 3..9, 0x1a) reserved for the local player, while world +0x210
   is set, adds an effect at its position (0x1400cb910 into the list at controller +0xc0, with
   values 8 and 0.1 and a strength of 10 × the item's draw scale) — presumably the glow that
   marks one's own drop (open question 7).

## 3. Picking up

* **Automatically**: coins (type 12) and type 14 without flag bit 1, within 4 blocks of the
  player, once their delay is exactly 0 (`use-prompt.md` §2.4 step 1).
* **With the use key**: any other valid ground item within reach (`use-prompt.md` §2.4, §4.2).

Both queue interaction kind 7, which the authoritative world handles: a pick-up sound, a
**pick-up notice** in world +0x110 (picker id, the item, the count, the position, the flags)
and, unless flag bit 1 is set, the ground item is emptied
(`zone-object-interaction.md` §6).

So the measured "coins arrive 0.59–0.89 s after the death" (`progression.md`) is the
500..799 ms delay plus the frame and interaction delay: a coin hops in place and is taken as
soon as its delay ends while the player is within 4 blocks. A coin farther away stays on the
ground.

## 4. Receiving a picked-up item (the frame routine 0xa5a50 at 0xaa608)

Every peer walks the pick-up notices of world +0x110 in order (after its particle and dialog
updates). For each notice:

1. P = the creature with the notice's picker id; if none, skip the notice.
2. A = the class-adapted item (0x10a720 on the notice's item with P), which turns a type-30
   treasure into the concrete item for P's class (`player-item-adaptation.md`).
3. If P is the local player:
   1. **Per-player items.** If the notice's flags have bit 1 (the ground item stays for
      others, e.g. role items of `worldgen-creatures-roles.md` §2): key = the block of the
      notice's position (floor of x, y and z in blocks, 0x93870). If the key is already in the
      player's map +0x88 (player data, `native-saves.md` item 19), **skip the rest of this
      notice**, including the announcement. Otherwise insert the key. So each per-player item
      can be taken once per character.
   2. **Into the inventory**, one of:
      * A is type 12 with material 13: the player's per-region counter (player data +0xf8, a
        map keyed by region) for the player's current region (+0x150, +0x154) gains the
        notice's count;
      * A has a money value (0x109070, non-zero only for type 0x1f, `use-prompt.md` §3.7.3):
        gold (+0xaac) gains that value (the count is ignored);
      * otherwise: add count × A to the inventory (tab by category). Coins (type 12, material
        11) go to the gold counter this way and type 14 to the second counter
        (`worldgen-creatures.md` §4.5 step 5).
   3. Refresh (0xb3ed0, then the inventory windows).
   4. If the **notice's** item (before adaptation) is type 0x1e: insert the 16-block cell of
      the notice's position (0x137e0 on x and y) into the used-cell set (player data +0xa8),
      as opening a chest does.
   5. **Receipt dialog**, when the notice's item is type 0x1e or A's type is 0x17, 0x18 or
      0x1d: a token script `define_item` A, then `you_received_the_item` when A is type 0x17
      or 0x18, or A is gear (3..9, 0x1a) with rarity above 2; otherwise
      `you_received_an_item`. For type 0x18 it continues with `new_page`, `define_land`
      (the biome 0x2ff5c0 and land seed 0x9afa0 of A's region) and a text chosen by A's
      subtype (1..18, jump table at 0xadc0c; not specified here). It is shown in the dialog
      window with seed rand mod 10000 (one rand). Then, if the player's ability is 0 or has
      run past its duration: ability 0x78 for type 0x18 subtype 1; for subtype 2, 0x79 only
      when the player's flags +0x5c have bit 1 (else no ability); otherwise 0x7c (holding it
      up); the held item (+0x1d8) = A, ability time 0. In every case controller +0x2050 =
      60000.
   6. If the crafting window is shown, rebuild the crafting lists (`loot-crafting.md` §2).
4. For **every** notice whose picker exists (local or not, unless skipped in step 3.1):
   announce A, the count and the picker (0x9d6f0, a chat line).

## 5. Items given without a ground item

### 5.1 Conversation pages (0xb40d0, page records of 0x1b0 bytes)

A dialog page carries an item to give (+0xd4 count, +0xd8 item) and an item to take (+0x30
count, +0x34 item). When the page becomes current (`lore-and-artifacts.md` §4.1):

* **Give**, if the count is not 0 and the player may carry it (0x50670): add count × item to
  the inventory (tab by category), announce it, controller +0x2050 = 60000. If the count is 1
  and the item is type 0x18: a dialog `define_item` item, `you_received_the_item` (seed rand
  mod 10000) and the hold-up (ability 0x7c, held item = it) under the same ability test as
  §4 step 3.5. If the player cannot carry it, nothing is given.
* **Take**, if the count is not 0: remove count of that item from the inventory (0xa2210):
  coins (type 12, material 11) from the gold counter (not below 0); otherwise from the first
  stacks, in tab and slot order, that are the same item, emptying stacks as needed.

The dialog window runs give, then take; speech bubbles over creatures run take, then give.
The page builder (0x168120) fills them from three script operations: op 8 gives the last
defined item ×1, op 9 gives n coins (type 12, material 11), op 10 takes the last defined item
×n. Which scripts emit these operations was not found (open question 4).

### 5.2 Taming (0xa0750, on the world's damage-number list +0x90)

The authoritative side of taming is `creature-abilities-ranged.md` §14.2: it renames the tamed
creature, makes it the player's pet and appends a damage number of kind 6 from the tamed
creature to the player with strength s. Every peer then walks the damage numbers; for one
where:

* both ids exist, its value (+0x20) is ≥ 0;
* the "to" id (+0x18) is the local player, the kind byte (+0x55) is 6 and the value is ≥ 1.0;
* the player holds pet food (held item +0x1d8 of type 0x15) and the "from" creature (+0x10)
  exists:

the local player:

1. if its pet slot (+0x8b8) is not empty, the pet item goes into the inventory (count 1);
2. its pet slot becomes an item of **type 0x14**, subtype = the tamed creature's race (+0x64),
   every other field 0; its ability (+0x68) = 0; its pet id (+0x9d8) = the tamed creature's id;
3. one of the held item is removed from the inventory (0x648b0), and the held item is cleared;
4. "TAMED!" goes to the console.

Pet food is item type 0x15 whose subtype is the race it tames (`drops-and-items.md` §3.3);
it comes from death drops (the fixed item, 1 in 8 × 1 in 5), the general shop
(`loot-shops.md` §2.4) and nowhere else in the routines read here. The pet item itself drops
from a dead creature's pet slot 1 in 50 (`drops-and-items.md` §2.1 step 3). Calling the pet is
interaction kind 10 (`zone-object-interaction.md` §8); the pet creature is made by 0x2b67b0
(`worldgen-creatures.md` §7.4).

---

## 6. Constants

| RVA | Value | Use |
|---|---|---|
| 0x4725ac | 13107.2 | wall item offset (0.2 block) |
| 0x43293c | 250.0 | half the hop time (ms) |
| 0x42e584 | 5.0 | hop height (blocks) |
| 0x4328ac | 0.7 | tumble per ms of delay left |
| 0x42e528 | 0.1 | landing sound pitch spread |
| 0x42e5f0 | 32767.0 | rand normaliser |
| immediate | 0x3d, 0x3e, 0x5d | landing sounds: coin, item, own gear |
| immediate | 60000 | controller +0x2050 after receiving |
| 0x4ac980 | 1.0 | taming threshold |

---

## 7. Verification

* **ground.request** (new): every tick for N seconds, dump each ground item of the zones
  around the player (position, +0xc0, +0xc4, +0xc8, +0xcc, +0xd8, the item) and the player's
  position. Kill a creature from more than 4 blocks away and from close by: coins must stay
  put in the first case and vanish when +0xc8 reaches 0 in the second; the drawn hop can be
  checked against a frame capture at known +0xc8 values.
* **pickup.request** (new): hook the notice walk in 0xa5a50: for each notice the picker, the
  item and its adaptation, the count, the flags, and the player's gold, inventory stacks,
  +0x88 and +0xf8 before and after.
* Taming: the existing `hit.request`/`watch.request` recordings with a pet-food tame; compare
  the pet slot, pet id and inventory.

## 8. Open questions

1. **The hop below ground.** As read, an item whose delay starts above 500 ms is drawn under
   its ground point first. Confirm with a capture (it may be hidden by the terrain, or the
   height may be clamped elsewhere).
2. **Type 12 material 13** (counted per region in player data +0xf8) — what item it is, and
   who reads the counter. It answers `native-saves.md`'s open question on +0xf8 in part.
3. **The +0x88 map** of per-player items keyed by block, as read here, replaces "meaning not
   established" in `native-saves.md` item 19. Confirm with a save taken before and after
   picking up a role item.
4. **Script operations 8, 9, 10** (give item, give coins, take item): no emitter was found
   among the conversation builders read so far.
5. **The type-0x18 receipt texts** (jump table at 0xadc0c by subtype) are not specified.
6. **Timer +0xcc** of ground items counts down but nothing here sets it; items with +0xcc > 0
   are not drawn.
7. **Drawing details**: the clock-driven motion of coins and special items, the lying poses,
   and the effect added for one's own gear (§2 steps 4–5). A frame capture beside a
   ground.request dump would settle them.

## Errata (from the clean-room implementation)

- §5.2: the damage-number offsets +0x10/+0x18/+0x20/+0x55 are 0x10 too high (they look like list-node offsets). The 0x48-byte record has them at +0x00/+0x08/+0x10/+0x45, as creature-hit.md §0.3 gives. Writing at +0x55 overruns the record (MSVC C4789).
- §4 step 3.1: the +0x88 set is keyed by a 3-D block here, while native-saves item 19 stores 12-byte (pair key, 4-byte value) entries. To reconcile in the decompile.
