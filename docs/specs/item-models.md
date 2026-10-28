# Item models (0x173b10)

Behavioural specification of how Steam Cube World picks the voxel model it draws for an item,
written from the decompiled executable, its assembly and the binary's constants. Routines are
cited by RVA (image base 0x140000000). The game's model table itself (which model index each
item gets) is game data: it is not given here. CubeVulkan reads it from the owner's install
through the extraction tool (§4). This spec gives the rules the lookup follows and the layout
of the extracted file.

---

## 0. Inputs

### 0.1 The item (0xa0 bytes, as `drops-and-items.md` §0.1)

Only five fields decide the model:

| Offset | Field | Note |
|---|---|---|
| +0x00 | **category** (the item type, u8) | |
| +0x04 | **id** (the subtype, i32, compared unsigned where ranges are tested) | |
| +0x08 | **modifier** (u32) | used only through unsigned remainders |
| +0x14 | **rarity** (u8) | |
| +0x1c | **material** (u8; compared as a signed byte, which matters only for values ≥ 0x80) | |

For category 0x16 the field +0x18 also matters, through the normalising step (§1).

### 0.2 The model store

The routine is called as 0x173b10 (store, item) and returns a model pointer, or null for "no
model". The store (one per world, e.g. world +0x70) holds:

| Offset | Use |
|---|---|
| +0x08 | the device handle new models are made with |
| +0x10, +0x18 | the **model list**: begin and end of an array of model pointers (8 bytes each). A model's **index** is its position in this list; the list is filled from the game's model files at start-up |
| +0x28 | the **on-demand cache** for category 0x17 (§3) |

"Model *k*" below means: if 0 ≤ *k* < the list's length, the pointer at index *k*, else null
(0x17190 and the inline copies of its test). Every result of the lookup is either such an
index or null, except category 0x17.

---

## 1. Normalising the item (0x1092b0)

The routine first copies the item (0x35400, field by field) and adjusts the copy:

| Category, id | Becomes |
|---|---|
| 0x16, id 0x3f | category 3, id = the item's +0x18 |
| 0x16, id 0x40 | category 4, id 0 |
| 0x16, id 0x41 | category 8, id 0 |
| 0x16, id 0x42 | category 9, id 0 |
| anything else | unchanged |

(Category 0x16 ids 0x3f..0x42 stand for a weapon, chest, neck or ring piece; for 0x3f the
weapon's kind is kept in +0x18.) Material, rarity and modifier are kept. Everything below
uses the normalised copy.

---

## 2. The decision (0x173b10)

### 2.1 Shape

0x173b10 is a fixed decision tree. The first branch is on the category; inside it, depending
on the category, on the id, the material, or both; at the leaves the index is a constant,
or a constant plus a rarity term, plus a modifier term. Categories 1 and 3 go through helper
routines that return the index (0x1738f0 and 0x1e66d0). Some branches share leaves with
other categories (the decompiled tree jumps between them), which the extracted table
captures without special handling.

### 2.2 The rarity term

Where rarity is used, it is **min(rarity, 4)** (a byte, so never negative): rarities 4, 5, 6
… all give the same model. Two exceptions:

* category 0x0f uses min(max(rarity − 1, 0), 3): rarities 0 and 1 share a model, and 4 and up
  share another;
* category 1 with id 1 adds the **raw** rarity byte, unclamped (0x1738f0): rarities above 4
  give other indices (other models, or none past the end of the list).

### 2.3 The modifier term

Where the modifier is used, it is the unsigned remainder of the modifier by one of: **2, 3,
4, 5, 6, 10, 11** (2 and 4 as the low bit and the low two bits). A row of five rarities is
usually multiplied by (modifier mod 5), (mod 6) or (mod 11) to pick among variants. All of
them repeat with a period of **660** (the least common multiple), so (category, id, material,
min(rarity, 4), modifier mod 660) decides the model, apart from §2.2's exception.

### 2.4 What each category depends on

"One" means a single model whatever the fields; "none" means null.

| Category | Depends on |
|---|---|
| 0, and above 0x21 | none |
| 1 | id (a few ids have their own model, any other id one shared model); id 1 also the raw rarity |
| 2 | one |
| 3 (weapons, 0x1e66d0) | id; for most ids rarity and modifier mod 11 (mod 6 for one id), and for some ids the material (a few materials have their own model); ids 0x16 and 0x17: none |
| 4 | material (for one material also the id); rarity and modifier mod 5 for most materials; id 1: none |
| 5, 6, 7 | material, rarity, modifier mod 5 (the id is not read) |
| 8, 9 | whether the material is 12, rarity, modifier mod 5 |
| 0x0a | one |
| 0x0b | id, then material (one id compares materials 0x81, 0x82, 0x83) |
| 0x0c | material |
| 0x0d, 0x0e, 0x10 | one |
| 0x0f | rarity (§2.2) |
| 0x11, 0x12 | modifier mod 4 |
| 0x13 | whether the id is 1; modifier mod 3 |
| 0x14 | whether the id is 0x18, 0x19 or another |
| 0x15 | id (ids up to 0x127 are compared; an unlisted id gets one shared model) |
| 0x16 | id; for some ids also modifier mod 2, 3, 4, 5, 6 or 10 |
| 0x17 | made on demand from id and modifier (§3) |
| 0x18 | id |
| 0x19, 0x1b, 0x1d, 0x1f, 0x20, 0x21 | one |
| 0x1a, 0x1c | id |
| 0x1e | none |

---

## 3. Category 0x17: models made on demand (0x1737b0, 0x2b880)

Category 0x17 items have no model in the list. 0x173b10 calls 0x1737b0 (store, id,
modifier):

1. **Cache.** The store's cache (+0x28) is an ordered map keyed by the pair (id, modifier),
   compared as two signed 32-bit integers, id first (0x172e50 finds or inserts the key). The
   value is a model pointer (node +0x28), null when new.
2. If the entry already holds a model, return it.
3. Otherwise make a new model object (0xa8 bytes, 0x16e6b0 with the store's device and flag
   1), store it in the entry, and fill it with the generator 0x2b880 (generator state, model,
   id, modifier). Return the model. Models are never removed from the cache.

**The generator 0x2b880** works on a small state object (discarded afterwards) holding the
model, the id, the modifier and a 32-bit **counter** (+0x30):

1. **Seed.** v = modifier × 57 + id (32-bit wrap); v = v xor (v shifted left 13); h = ((v ×
   v × 60493 + 19990303) × v + 0xd208dd0d) with the top bit cleared (all unsigned 32-bit
   wrap); counter = 100000 − h / 21474 (unsigned division).
2. **Size.** The model is set to 24 × 24 × 24 voxels (0x16ff70).
3. **Colours.** Four colours of three channels each are drawn. One draw from counter value c
   is: x = c xor (c shifted left 13); x = x xor (x shifted right 17); x = x xor (x shifted
   left 5) (unsigned); the channel = (x mod 250) + 10. Each colour takes c = counter: its
   first channel from c + 2, its second from c + 1, its third from c; then counter + 3. The
   twelve channels are stored as floats.
4. **Shape.** By the id: 0 → 0x30790, 1 → 0x2bf90, 2 → 0x2e490, 3 → 0x2ffd0, 4 → four more
   draws and 0x2daf0 around the model's centre, 5 → 0x2cae0, 6 → 0x2f450, any other id → a
   colour of three draws (each (x xor x shifted left 5) & 0x7f) and 0x2fc60 around the
   centre. These shape generators are not specified here.
5. **Finish.** 0x170700 (model, 0) builds the model for drawing.

The model depends only on (id, modifier), and is the same every time for the same pair.

---

## 4. The extracted table (Extract/oracle/item-models.bin)

### 4.1 How it is made

`tools/ZoneExtractor` runs inside the owner's game. When the file `Extract\itemmodels.request`
exists and the extractor has seen the model store, it deletes the request and writes the
table:

* It numbers the store's models by their **first** index in the model list (a model listed
  at several indices is named by the lowest).
* For each category 0..0x2f except 0x17, each id 0..127 and each material 0..63, it asks
  0x173b10 about an item with only those fields set (+0x18 = 0, all else zero):
  * at rarity 0 for every modifier 0..329, and at modifier 0 for every rarity 0..4;
  * if neither varies, the key has **kind 0** (one model); if only the rarity varies,
    **kind 1** (five); if only the modifier varies, **kind 2** (330); if both vary, it asks
    again for all 5 × 330 pairs, **kind 3**.
  * A key for which every answer is "none" is not written.
* Identical tables are written once and shared by index.

### 4.2 Layout (little-endian)

| Part | Content |
|---|---|
| magic | the 4 bytes `CWI2` |
| table count | u32 |
| each table | i32 kind (0..3), then its model numbers as i32 (−1 = none): 1 for kind 0; 5 for kind 1 (rarity 0..4); 330 for kind 2 (modifier mod 330); 5 × 330 for kind 3, rarity-major (index = rarity × 330 + modifier mod 330) |
| key count | u32 |
| each key | 6 bytes: u8 category, u8 id, u8 material, u8 padding (0), u16 table index |

Keys are in increasing order of (category, id, material). Category 0x17 is never present.

### 4.3 Looking an item up

To get the model number of an item:

1. Normalise it as in §1 (category 0x16 ids 0x3f..0x42).
2. If the category is 0x17, use §3 instead.
3. Find the key (category, id, material). None: no model.
4. Pick by the table's kind with r = min(rarity, 4) and m = modifier mod 330.

### 4.4 Limits of the table

What the extraction does not capture, in the game's own terms:

* **Ids above 127 and materials above 63.** Category 0x15 compares ids up to 0x127 and gives
  every other id a shared model, so its ids 128 and up are missing; category 0x0b compares
  materials 0x81..0x83. Categories whose answer ignores the id (or material) also give
  models for any id (or material) out of range.
* **Modifier period.** The table indexes the modifier mod 330, but categories 0x11, 0x12 and
  0x16 use modifier mod 4, which does not divide 330: for modifiers of 330 and up, (modifier
  mod 330) mod 4 can differ from modifier mod 4. A period of 660 would be exact.
* **Raw rarity** of category 1 id 1 above 4 (§2.2).
* **+0x18** of category 0x16 id 0x3f: the table was made with +0x18 = 0; without step 1 of
  §4.3 every such item shows the same model.

---

## 5. How it is checked

* `EngineTests rulesoracle <files>`: for each recorded creature record, the nine worn and
  held item slots are looked up in the table (`src/NativeItems.cpp`) and the model's size
  (from `Extract/sprites.txt`) is compared with the size of the model the game drew for that
  slot (0x173b10's answer, recorded). All 117,000 recorded slots match
  (`docs/animation.md`). This checks the table and the lookup, not §3.
* Nothing checks §3 (category 0x17) yet.

---

## Open questions

1. **Which items are category 0x17**, and the four shape generators (0x30790, 0x2bf90,
   0x2e490, 0x2ffd0), 0x2daf0, 0x2cae0, 0x2f450, 0x2fc60 and the finishing step 0x170700, are
   not specified. A clean port of §3 needs them, or a recording of the models the game makes.
2. **Model list order.** The index space is the order the game loads its model files in;
   CubeVulkan already relies on that order (`sprites.txt`), but the loader itself was not
   specified here.
3. **The modifier period** (§4.4): whether any category 0x11, 0x12 or 0x16 item in the game
   actually has a modifier of 330 or more was not checked. If so, the extractor should scan
   0..659.
4. **Rarities above 4** occur only through `drops-and-items.md` §1.3; whether category 1 id 1
   ever gets one was not checked.
5. **Signed material bytes**: the tree compares the material as a signed byte. Nothing found
   gives an item a material of 0x80 or more except the three values in category 0x0b; their
   source is unknown.
