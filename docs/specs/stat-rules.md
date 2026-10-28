# Creature and item stat rules

Behavioural specification of Steam Cube World, written from the decompiled executable, its
assembly and the binary's constants. Routines are cited by RVA (image base 0x140000000);
constants by the RVA of the value in the binary. Offsets are byte offsets into the record
named. Times are milliseconds.

This document collects every routine that turns a creature record (level, class,
specialisation, race, flags, worn items, buffs) or an item record into a number, and the
small tests built on them. Several of these were already summarised inside other specs
(`creature-upkeep.md` §4, `creature-hit.md` §5, `effect-objects.md` §2,
`skills-and-interactions.md` §0 and §1.6); this document is the complete statement and,
where they disagree, was re-checked against the assembly (see "Corrections" at the end).

Shared vocabulary (buff records, **give buff** 0x65390) is defined in
`skills-and-interactions.md` §0.

---

## 0. Conventions

### 0.1 Arithmetic

* All arithmetic is IEEE single precision (SSE scalar) unless a step says "double". Every
  product and sum is rounded to float after each operation, in the order written. "a × b × c"
  means (a × b) × c. Sums over slots are made one slot at a time into a running float, in the
  order of the table given.
* "Doubled" means x + x (the game adds the value to itself; identical to × 2.0).
* "float(n)" converts a signed 32-bit integer exactly as a C cast does (round to nearest).
  "trunc(x)" converts a float to a signed 32-bit integer toward zero; a value that is NaN or
  outside the 32-bit range gives −2147483648 (the hardware's "integer indefinite").
* "u mod n" on an unsigned 32-bit value is the unsigned remainder.
* **xorshift(v)** on an unsigned 32-bit v: v = v XOR (v shifted left 13); v = v XOR (v shifted
  right 17, logical); v = v XOR (v shifted left 5). All shifts drop bits beyond 32.
* **powf** is the C runtime's single-precision power. It is used as 2^x (item power, price)
  and 2^4 (defence against crits). **pow** (double) is used once (§10.2).
* "First buff of type t": walk the creature's buff list from its front; the first entry whose
  type byte equals t. "Every buff of type t": all such entries, front to back.

### 0.2 The creature record (fields read here)

| Offset | Type | Meaning |
|---|---|---|
| +0x60 | u8 | hostility kind; 0 = player. "Player" below always means kind 0. |
| +0x64 | i32 | race |
| +0x68 | u8 | current ability id |
| +0x6c | f32 | time spent in the current ability (ms) |
| +0x70 | i32 | combo count |
| +0x80 | u32 | creature flags; bits 4, 9, 15, 17, 18, 22, 26 are read here |
| +0x128 | u8 | more flags; bit 5 (0x20) is read by the hostility test |
| +0x130 | f32 | stun timer (stunned while > 0) |
| +0x134 | f32 | stamina-regeneration lock (see `creature-upkeep.md`) |
| +0x144 | i32 | level |
| +0x148 | i32 | experience |
| +0x14c | u8 | class (1, 2, 3, 4 for the four player classes) |
| +0x14d | u8 | specialisation |
| +0x150, +0x154 | i32, i32 | the region the creature is in |
| +0x188 | f32 | mana, 0..1 |
| +0x18c | f32 | stealth, 0..1 (`creature-upkeep.md` step 19) |
| +0x1d8 | item | the item currently being used (consumables) |
| +0x278 + k × 0xa0 | item | eleven equipment slots, k = 0..10 (table §0.4) |
| +0x980 | list | buff list; each entry 0x18 bytes: type (u8) at +0, strength (f32) at +4 |
| +0x9a0 | f32 | stamina |
| +0xb5c … +0xb74 | 7 × i32 | skill points (§9) |
| +0xb78 | map | cooldowns: ordered map from ability id (i32) to remaining time (f32) |
| +0xba8 | f32 | a duration factor used only by ability 0x94 |

There is **no maximum-mana routine**: mana is a fraction capped at 1.0 wherever it is
raised (see `creature-upkeep.md`). Likewise there is no separate health-regeneration stat;
the sheet's "REGENERATION" is the stamina factor (§6.2).

### 0.3 The item record (0xa0 bytes)

| Offset | Type | Meaning |
|---|---|---|
| +0x00 | u8 | type (category) |
| +0x04 | i32 | subtype (id within the type); read as unsigned where compared with 23 |
| +0x08 | u32 | modifier (the item's seed) |
| +0x0c, +0x10 | i32, i32 | the item's region |
| +0x14 | u8 | rarity |
| +0x18 | i32 | for type 2 only: the wrapped item's type in its low byte |
| +0x1c | u8 | material |
| +0x1d | 32 × 4 bytes | attribute entries (three signed bytes, one value byte) |
| +0x9d | u8 | attribute count ("upgrades") |

**Two-handed**: type 3 and subtype (unsigned) ≤ 23 and subtype in {5, 6, 7, 8, 10, 11, 15, 16,
17, 18, 23} (bit mask 0x878de0). Used throughout; written "2H" below.

### 0.4 Equipment slots

A slot "counts" only when the item in it has the slot's required type; otherwise the slot is
treated as empty for that routine.

| Slot | Offset | Required type |
|---|---|---|
| W1 (first weapon) | +0x638 | 3 |
| W2 (second weapon) | +0x6d8 | 3 |
| chest | +0x3b8 | 4 |
| feet | +0x458 | 6 |
| hands | +0x4f8 | 5 |
| shoulder | +0x598 | 7 |
| neck | +0x318 | 8 |
| ring 1 | +0x778 | 9 |
| ring 2 | +0x818 | 9 |

(+0x278 and +0x8b8, the other two slots, are read by none of these routines.)

The **nine-slot order** used by critical chance, haste and the stamina factor is W1, W2,
chest, feet, hands, shoulder, neck, ring 1, ring 2. The **six-slot order** of maximum health is
W1, W2, chest, feet, hands, shoulder. The **armour order** is chest, feet, hands, shoulder.

### 0.5 May use (0x50640, with 0x1094d0)

A creature **may use** an item when it is not a player (kind ≠ 0), or when the creature's
class may use the item (§1.3). 0x50640 is exactly this test. Wherever this document says
"usable", it means "the slot counts (§0.4) and the creature may use the item".

---

## 1. The item's own level and power

### 1.1 Reference region (inside 0x109c50)

The region an item is judged against is the creature's (+0x150, +0x154) when the creature's
hostility kind is 0, 4 or 6; for any other kind it is the item's own region (+0x0c, +0x10),
which makes the item always "at home" for monsters and NPCs.

### 1.2 Item level (0x109720), given an item and a reference region (x, y)

1. If the item's type is 3..9 or 0x1a, **and** its modifier mod ((rarity + 1) × 10) is 0
   (unsigned 32-bit), **and** max(|x − item x|, |y − item y|) < 2 (Chebyshev distance on the
   region coordinates, 32-bit signed differences): the level is the rarity.
2. Otherwise: the level is the rarity if x = item x and y = item y; else −1.

So an item counts at full rarity in its own region; items whose modifier is a multiple of
(rarity + 1) × 10 also count fully in the eight neighbouring regions; everywhere else the
level is −1.

### 1.3 Class may use (0x1094d0), given an item and a class byte c

* **Type 2:** take a copy of the item whose type is the low byte of +0x18 and whose +0x18 is
  0 (subtype, modifier, region, rarity, material and attributes copied). If that type is
  again 2, the answer is **yes**; otherwise the answer is this test applied to the copy.
* **Type 3 (weapons), by subtype:** 3, 4, 5 → c = 4; 6, 7, 8, 14 → c = 2; 9 → yes; 10, 11, 12
  → c = 3; 0, 1, 2, 13, 15, 16, 17 → c = 1; 18 and above → yes.
* **Types 4..7 (armour), by material:** 1, 4, 5, 11, 19, 23, 29 → c = 1; 20, 27, 31, 33 →
  c = 2; 24, 26, 32, 35 → c = 3; 28, 30, 34 → c = 4; any other → yes.
* **Any other type:** yes.

### 1.4 Item power (0x109c50), given an item, a creature and an upgrade count u

1. *lvl* = item level (§1.2) against the reference region of §1.1.
2. *h* = xorshift(modifier); *r* = h mod 1000.
3. *d* = 32 if the item is 2H, else 16.
4. exponent = (float(u) × 0.5 / float(d)) + (float(*lvl*) + float(*r*) / 2000.0).
   Evaluate: A = float(u) × 0.5, A = A / float(d); B = float(*r*) / 2000.0, B = float(*lvl*) + B;
   exponent = A + B.
5. Power = powf(2.0, exponent).

u is 0 for every caller except the weapon damage of §2.1, which passes the item's attribute
count (+0x9d, unsigned). An item at home has power 2^(rarity + r/2000), between 2^rarity and
just under 2^(rarity + 0.5); away from home 2^(−1 + r/2000).

Constants: 2.0 (0x4ac9d0), 0.5 (0x4ac964), 2000.0 (0x42e5d8).

---

## 2. Per-item stat values

Every routine here returns 0.0 for item types it does not cover. "P" is the item power of
§1.4 (u = 0 unless stated). Each takes the creature only to pass it to §1.4.

**Roll** used by health, armour, resistance and stamina regeneration, with a salt K:
*v* = xorshift(modifier + K) (the addition wraps at 32 bits); *roll* = float(*v* mod 100000)
× 0.25 / 100000.0 + 1.0 (a value in [1.0, 1.25)). Evaluated as ((float × 0.25) / 100000.0) +
1.0.

### 2.1 Weapon damage (0x10ad90) and base damage (0x108f20)

Type 3 only. Let P = item power with u = the attribute count (+0x9d) for 0x10ad90, u = 0 for
0x108f20.

| Subtype | Weapon damage |
|---|---|
| 3, 4 | P doubled |
| 5, 13 | P × 4.0 |
| 11, and the creature is not a player | P × 6.0 (0x10ad90 only) |
| any other 2H subtype (and 11 for players; 11 always in 0x108f20) | P × 8.0 |
| anything else (0, 1, 2, 9, 12, 14, 19–22, ≥ 24) | P × 4.0 |

The **upgrade bonus** shown in the tooltip (0x10ad50) is 0x10ad90 − 0x108f20.

Constants: 4.0 (0x4aca20), 6.0 (0x4aca24), 8.0 (0x4aca30).

### 2.2 Item health (0x1097b0)

Types 3..7.
1. *f* = roll(K = 0x2673) × 10.0.
2. Type 4: *f* doubled; then, if the subtype is 1, doubled again.
3. 2H: *f* doubled.
4. *m* = 2.0 for material 1; 1.5 for 0x1b; 1.75 for 0x1c; else 1.0.
5. Result = (P × *f*) × *m*.

Constants: 0.25 (0x4ac95c), 100000.0 (0x43800c), 1.0 (0x4ac980), 10.0 (0x42e598), 2.0
(0x4ac9d0), 1.5 (0x42e548), 1.75 (0x4328cc).

### 2.3 Item armour (0x108d50) and item resistance (0x10a1e0)

Types 4..7.
1. *f* = roll(K = 0x18008 for armour, 0x1a6d for resistance).
2. Type 4: *f* doubled; if the subtype is 1, doubled again. (The routines also test 2H, which
   cannot apply to these types.)
3. *f* = *f* × the material factor:

| Material | Armour | Resistance |
|---|---|---|
| 1 | 1.2 | 0.85 |
| 4 | 1.25 | 0.85 |
| 6 | 0.25 | 0.25 |
| 0x0b | 1.125 | — |
| 0x13 | 0.8 | — |
| 0x14 | 0.8 | 0.85 |
| 0x18, 0x1a, 0x20 | 0.5 | — |
| 0x1b, 0x1c, 0x1e, 0x1f | 0.8 | 0.75 |
| 0x1d | 1.15 | 0.85 |
| 0x21 | 0.9 | 0.75 |
| 0x22 | — | 0.75 |
| 0x23 | 0.6 | — |

"—" and any material not listed: no multiplication (not even by 1.0).

4. Result = P × *f*.

Constants: 1.2 (0x42e718), 1.25 (0x42e71c), 0.25 (0x4ac95c), 1.125 (0x441c74), 0.8
(0x42e530), 0.5 (0x4ac964), 1.15 (0x4328b4), 0.9 (0x42e534), 0.6 (0x42e708), 0.85
(0x42e714), 0.75 (0x42e710).

### 2.4 Item critical chance (0x1090f0) and item haste (0x10a490)

Types 3..9.
1. *a* = 0.02 if the item is 2H or its type is 4, 8 or 9; else 0.01.
2. Type 4 with subtype 1: *a* doubled.
3. *q* = float(modifier mod 21) / 20.0.
   * Critical: *b* = 1.0 − *q*.
   * Haste: *b* = *q*.
4. Material adjustments (at most one applies):

| Material | Critical | Haste |
|---|---|---|
| 4, 0x21, 0x22, 0x23 | — | *b* = *b* × 0.9 |
| 6 | *a* = *a* × 0.5 | *a* = *a* × 0.5 |
| 0x0b | *b* = *b* + 1.0 | — |
| 0x0c | — | *b* = *b* + 1.0 |
| 0x1d | — | *b* = *b* + 0.5 |

5. *v* = (((P × 0.0625) × *a*) × *b*) × 3.0.
6. Result = *v* if *v* ≥ 0.001, else 0.0 (a NaN *v* gives 0.0).

Constants: 0.01 (0x42e51c), 0.02 (0x42e6e8), 20.0 (0x42e5a0), 0.9 (0x42e534), 0.0625
(0x4328a4), 3.0 (0x42e580), 0.001 (0x42e514).

### 2.5 Item stamina regeneration (0x109f30)

Types 4..9 and 0x1a (weapons give 0).
1. *f* = roll(K = 0x157) × 0.01. Type 4: *f* doubled (no extra doubling for subtype 1).
2. *g* = float(((modifier × 8) wrapped to 32 bits) mod 21) / 20.0.
3. Material: 0x1b or 0x1f: *g* + 0.5; 0x1c or 0x1e: *g* + 1.0; 0x1d: *g* + 0.1; 0x21: *g* +
   0.4; 0x22: *g* + 0.75.
4. *t* = (P × *f*) × *g* (float); *v* = float(double(*t*) × 1.5) (the product is formed in
   double and rounded once).
5. Result = *v* if *v* ≥ 0.001, else 0.0.

Constants: 0.01 (0x42e51c), 0.1 (0x42e528), 0.4 (0x42e52c), 0.75 (0x42e710), 1.5 as a
double (0x441c78).

### 2.6 Consumables

* **Heal amount (0x109460).** Type 1 only: subtypes 1, 2, 4, 5, 6, 7, 8, 12 → 200.0;
  10, 11 → 100.0; anything else (and other types) → 0.0. The tooltip shows "Restores health
  over … seconds" when this is above a threshold it computes elsewhere.
* **Use time (0x108fa0)**, an integer number of ms, for the item at +0x1d8 and in the
  tooltip. Type 0x1c → 500. Type 1: subtype 3 → 500; subtypes 1 and 12 →
  trunc((1.5 − float(rarity) × 0.5 × 0.25) × 5000.0); subtype 2 → the same with 6000.0; any
  other subtype → the same with 10000.0. Evaluate *k* = (float(rarity) × 0.5) × 0.25, then
  (1.5 − *k*) × constant, then truncate. Other types → 0.
* **Demon damage (0x109290).** 1.0 for type 0x18 subtype 12, else 0.0. 0x510f0 sums it over
  the second inventory tab (one entry per occupied slot, not per unit) when the creature has
  at least two tabs; otherwise 0.0. Used by the hit routine (`creature-hit.md` §1).

Constants: 200.0 (0x42e5c0), 100.0 (0x42e5bc), 1.5 (0x42e548), 5000.0 (0x42e5e4), 6000.0
(0x441c80), 10000.0 (0x42e5e8).

### 2.7 Item price (0x109d30) and sell price (0x10a390)

Price, an integer:
1. *base* by type: 3 → 5.0; 4 → 4.0; 5, 6, 9 → 2.0; 7, 8 → 3.0; 11 with subtype 0x20..0x23 →
   10.0; 0x16 → 50.0; anything else → 0.2.
2. 2H: *base* doubled.
3. *f* = *base* × 5.0.
4. If (type 3..9 or 0x1a) and modifier mod ((rarity + 1) × 10) == 0: *f* doubled.
5. Price = max(1, trunc(powf(2.0, float(rarity)) × *f*)).

Sell price = trunc(max(1.0, sqrtf(float(price)) × 0.5)) — if the product is ≤ 1.0 the
result is 1.

Constants: 0.2 (0x42e700), 5.0 (0x42e584), 4.0 (0x4aca20), 2.0 (0x4ac9d0), 3.0
(0x42e580), 10.0 (0x42e598), 50.0 (0x42e5b0).

---

## 3. Weapon lists (the "selectors")

Attack power and spell power add up a list of weapons. The list depends on a selector byte.

### 3.1 Selector 0: weapons in use (0x4fd40)

"Usable" is §0.5 (type 3 in the slot, and not a player or the class may use it). The list is
built in this order and the first rule that applies is used:

1. W2 is usable with subtype 6 or 7 → [W2].
2. Else W1 is usable with subtype 6 or 7 → [W1].
3. Else by the current ability (+0x68):
   * 3, 5, 8, 9, 0x0b, 0x13, 0x28, 0x2a → [W1] if usable, else [].
   * 0x0c, 0x0d, 0x11, 0x12, 0x15, 0x37, 0x5f, 0x64, 0x6c, 0x8c, 0x94, 0x9e → W1 if usable,
     then W2 if usable.
   * 0x29, 0x2b → [W2] if usable, else [].
   * any other → W2 if usable; then W1 if W1 is type 3 with subtype 12 and the creature may
     use it (0x50640).

### 3.2 Selector 1: both weapons (0x52ce0)

W2 if its type is 3, then W1 if its type is 3. No class test. The character sheet uses this
selector.

---

## 4. Offence

### 4.1 Attack power (0x4fa70), given a creature and a selector

1. *s* = 5.0; then for each weapon of the selector's list (§3), in list order: *s* = *s* +
   weapon damage (§2.1, 0x10ad90).
2. *s* = (stealth (+0x18c) × 0.2 + 1.0) × *s*.
3. By the current ability: 0x60 → *s* × 50.0; 0x64 → *s* doubled; 0x67 → *s* × 5.0; 0x6c →
   *s* × 3.0.
4. Flags (+0x80): bit 9 → × 1.5; bit 17 → × 1.2; bit 18 → doubled (in this order).
5. First buff of type 0x17: × its strength.
6. First buff of type 0x21: × (strength × 0.1 + 1.0).
7. Bit 26 → × 1.1.

Constants: 5.0 (0x42e584), 0.2 (0x42e700), 1.0 (0x4ac980), 50.0 (0x42e5b0), 3.0
(0x42e580), 1.5 (0x42e548), 1.2 (0x42e718), 0.1 (0x42e528), 1.1 (0x42e538).

### 4.2 Spell power (0x65c70), given a creature and a selector

The character sheet labels this "SPELL POWER" (shown for class 3).
1. *s* = 5.0; add the weapon damage of each weapon in the selector's list, in order.
2. Bit 9 → × 1.5; bit 17 → × 1.2; bit 18 → × 4.0.
3. First buff of type 0x17: × its strength.
4. Bit 26 → × 1.1.

(No stealth factor, no ability factor, no type-0x21 buff.) Constant 4.0 at 0x4aca20.

### 4.3 Ability damage factor (0x50f20), given an ability id

6, 0x1b → 0.5; 0x16 → 0.1; 0x1f, 0x21 → 0.4; 0x20, 0x22 → 0.6; 0x26, 0x2c, 0x3a, 0x3b, 0x45,
0xa0 → 2.0; 0x2f, 0x69, 0x6a → 0.75; 0x35 → 3.0; 0x38 → 0.25; 0x49 → 0.2; anything else →
1.0. (Used by the projectile and effect damage of `effect-objects.md` §3.)

### 4.4 Critical chance (0x50d30), given an attacker A, a target T (or none) and a flag "magic"

1. *c* = A's stealth × 0.5 + 0.0.
2. If A's current ability is 0x8c and float(duration(A)) (§7.1) > A's ability time (+0x6c):
   *c* = *c* + 0.5. (Duration is only computed for ability 0x8c.)
3. For each slot in the nine-slot order (§0.4) that counts: *c* = *c* + item critical chance
   (§2.4).
4. If T is present: *c* = *c* − (magic ? magic crit defence(T) : crit defence(T)) (§5.4).
5. Return *c* (not clamped).

The sheet calls this with no target and magic = "uses mana" (§8.3) and shows *c* × 100 as a
percentage. Damage callers draw rand first and compare (`effect-objects.md` §1.7.8).

### 4.5 Haste / attack-speed factor (0x66460)

1. *s* = 1.0.
2. Let *T* = combo threshold (§4.6) and *c* = min(float(combo +0x70) / float(*T*), 1.0) — the
   minimum taken as "the quotient if it is below 1.0, else 1.0" (NaN gives 1.0). Each case
   below computes its own *c* (and calls §4.6 itself).
   * Class 1, specialisation 0: *s* = *c* × 0.5 + 1.0.
   * Class 3, specialisation 1: *s* = *s* + *c*.
   * Class 2, specialisation 0: *s* = *s* + *c* × 0.5.
3. For each slot in the nine-slot order that counts: *s* = *s* + item haste (§2.4).
4. If the creature has any buff of type 0x0c: *s* doubled.
5. Bit 9 → × 0.75; bit 18 → × 0.6; bit 26 → × 1.1.

The sheet shows "HASTE" as (*s* − 1.0) × 100 %.

Constants: 0.5 (0x4ac964), 0.75 (0x42e710), 0.6 (0x42e708), 1.1 (0x42e538).

### 4.6 Combo threshold (0x5f9e0), an integer

If W2 holds a type-3 item, by its subtype: 3, 4, 10 → 50; 6 → 60; 8 → 80; 11, 12 → 20; 15,
16, 17 → 15; any other → 30. Otherwise 15 if bit 4 of +0x80 is set, else 30.

### 4.7 Proc roll (0x5e820), given a creature and a chance *p*

If W2 is type 3: subtypes 0, 1, 2, 12 → *p* × 0.5; subtypes 8, 10 → *p* × 0.3 (0x42e704);
others unchanged. Then draw one rand; the result is float(rand) / 32767.0 < *p*.

---

## 5. Defence

### 5.1 Maximum health (0x5fac0)

1. *h* = 125.0 if class 1 and specialisation 1, else 100.0.
2. For each slot in the six-slot order that counts: *h* = *h* + item health (§2.2). (No
   class test.)
3. Bit 9 → × 10.0; bit 17 → × 1.5; bit 18 → × 15.0.
4. First buff of type 0x16: × its strength.
5. Every buff of type 0x20, in list order: × its strength.
6. Bit 26 → × 1.1.

The sheet shows trunc(*h*).

Constants: 100.0 (0x42e5bc), 125.0 (0x432938), 10.0 (0x42e598), 1.5 (0x42e548), 15.0
(0x42e7a4), 1.1 (0x42e538).

### 5.2 Armour (0x4f080) and resistance (0x64d90)

1. *a* = 0.0. For each slot in the armour order (chest, feet, hands, shoulder) that is usable
   (§0.5): *a* = *a* + item armour (§2.3) — or item resistance for 0x64d90.
2. If bit 9 **or** bit 17 of +0x80 is set: × 1.2 (once).
3. Bit 18: doubled.
4. First buff of type 0x0e: × 0.25.
5. First buff of type 0x18 (armour) or 0x19 (resistance): × its strength.

No bit-26 factor.

### 5.3 Block value (0x50370)

1. *b* = 5.0.
2. If W1 is type 3 with subtype 13 and the creature may use it:
   *b* = weapon damage(W2) × 6.0 + 5.0 for class 1 specialisation 1, else weapon damage(W2) ×
   4.0 + 5.0. W2 is passed to §2.1 without a slot or class test (a non-weapon gives 0). Go to
   step 4.
3. Otherwise: if W1 is usable, *b* = weapon damage(W1) + *b*; then if W2 is usable, *b* = *b*
   + weapon damage(W2). (If W1 was a shield the creature may not use, this path runs and
   skips it.)
4. Bit 9 → doubled; bit 17 → × 1.2; bit 18 → × 4.0; bit 26 → × 1.1.

### 5.4 Defence against critical hits (0x50ea0, 0x5f740)

* crit defence(T) (0x50ea0) = (armour(T) / powf(2.0, 4.0)) × 0.05; then × 0.25 if T has a
  buff of type 0x0e.
* magic crit defence(T) (0x5f740): the same with resistance(T).

powf(2, 4) is 16.0 exactly. Note that the 0x0e buff is applied twice (once inside armour or
resistance, once here). Constant 0.05 at 0x42e524.

---

## 6. Mana and stamina factors

### 6.1 Mana factor (0x5f8d0) — sheet "MANA GENERATION"

1.0 − strength × 0.1 for the first buff of type 0x22; 1.0 without one. Multiplies mana gains
in the upkeep and in effects. The sheet shows it × 100 %.

### 6.2 Stamina factor (0x647b0) — sheet "REGENERATION"

1.0 plus item stamina regeneration (§2.5) for each slot in the nine-slot order that counts.
Multiplies stamina regeneration in the upkeep. Shown × 100 %.

### 6.3 Mana cost (0x5f7c0), given a creature and an ability id

* 0x20, 0x22, 0x26, 0x2c, 0x2e, 0x30, 0x69, 0x6a: 0.0 if the creature has a buff of type 9,
  else 0.3 (0x42e704).
* 0x23 → 0.0625; 0x33 → 1.0; 0x61 → 0.5; 0x86, 0x87, 0x88 → 0.03125 (0x4328a0).
* Anything else → 0.0.

### 6.4 Stamina cost (0x65ea0), given a creature and an ability id

0.0 if the creature has a buff of type 13. Otherwise: 0x16 → 0.5; 0x32, 0x52, 0x6c → 0.25;
0x34, 0x37, 0xa0 → 1.0; anything else → 0.0.

---

## 7. Ability durations

### 7.1 Duration (0x4f220) = effect part + middle part + recovery part

Each part is an integer. A part marked "/S" is trunc(constant / S) with S the haste factor of
§4.5 (computed afresh for each part; constants are floats, the division is single
precision). The three parts are also available separately: 0x4ead0 (effect), 0x660d0
(middle), 0x5e9f0 (recovery); each takes an ability id, or a negative value meaning the
current ability (+0x68). 0x4f220 itself always uses the current ability byte. Ids above 0xa3
(possible only through the separate routines) fall into the "any other" rows.

**Effect part (0x4ead0):**

| Abilities | Value |
|---|---|
| 0, 1, 9, 0x0c, 0x34, 0x38, 0x3d–0x3f, 0x50–0x52, 0x5f, 0x6e, 0x74, 0x76, 0x78–0x7a, 0x92, 0x95, 0x9b, 0x9e, 0x9f | 0 |
| 2, 0x0a, 0x1b, 0x45–0x47, 0x4a–0x4f | 300/S |
| 3–8, 0x0b, 0x0f, 0x12–0x16 | 100/S |
| 0x18–0x1a, 0x1c, 0x25, 0x3c, 0x40, 0x41, 0x93 | 50/S |
| 0x1d, 0x32, 0x71, 0x8c, 0x8e, 0x8f | 100 |
| 0x1f, 0x21, 0x3a, 0x3b, 0x67 | 800/S |
| 0x20, 0x22, 0x23 | 1600/S |
| 0x26, 0x2c, 0x62 | 600/S if W2's subtype (+0x6dc, read without a type test) is 12, else 1200/S |
| 0x27, 0x2d, 0x42, 0x43, 0x68, 0x6b | 500/S |
| 0x2e, 0x30 | 1200/S |
| 0x37 | 400 |
| 0x48, 0x49 | 200 |
| 0x60 | 5000/S |
| 0x64, 0x69, 0x6a | 1000/S |
| 0x6c | 100 if the creature has a buff of type 13, else 200/S |
| 0x0d, 0x0e, 0x10, 0x11, 0x28–0x2b, 0x44, 0x94 | 200/S |
| 0x75, 0xa1 | 3000 |
| 0x85 | 60000 |
| 0x86–0x88 | 2000/S |
| 0x89, 0xa2, 0xa3 | 5000 |
| 0x8b | 10 |
| 0x90 | 1500/S |
| 0x98 | 120000 |
| 0x9a | 2000 |
| any other | 400/S |

**Middle part (0x660d0):**

| Abilities | Value |
|---|---|
| 0, 1, 9, 0x19, 0x1a, 0x1c, 0x33, 0x3c, 0x40, 0x50–0x52, 0x76, 0x78–0x7a, 0x86–0x89, 0x90, 0x92, 0x9a, 0xa1–0xa3 | 0 |
| 2, 3, 0x0a, 0x13, 0x14, 0x44 | 200/S |
| 4, 5, 0x26, 0x2c, 0x3f | 100/S |
| 6, 0x0d, 0x11, 0x12, 0x15, 0x16, 0x42, 0x43 | 400/S |
| 7, 8 | 150/S |
| 0x0b | 200 |
| 0x0e–0x10 | 250/S |
| 0x17, 0x18 | 50/S |
| 0x1b, 0x23 | 1200/S |
| 0x1d, 0x69, 0x6a | 2000 |
| 0x1f, 0x21, 0x4a, 0x67 | 600 |
| 0x20, 0x22 | 1200 |
| 0x2e, 0x30, 0x38, 0x68, 0x6b | 500/S |
| 0x2f | 1000/S |
| 0x34, 0x6c | 250 if the creature has a buff of type 13, else 500 |
| 0x4d–0x4f | 500 |
| 0x37 | 100 |
| 0x45, 0x46, 0x9b, 0x9f | 1000 |
| 0x48, 0x49, 0x82 | 3000 |
| 0x5f | 20000 |
| 0x64 | 6000 |
| 0x8b | 5000 |
| 0x8c | 5000/S |
| 0x94 | trunc(+0xba8 × 1000.0) |
| any other | 300/S |

**Recovery part (0x5e9f0):**

| Abilities | Value |
|---|---|
| 1, 9, 0x19, 0x1a, 0x1c, 0x1d, 0x32, 0x3c, 0x40, 0x50–0x52, 0x5f, 0x76, 0x78–0x7a, 0x89, 0x92, 0x9a, 0x9b, 0x9f, 0xa1–0xa3 | 0 |
| 0, 0x34 | 100 |
| 4–6, 0x3a, 0x3b, 0x3f | 300/S |
| 8, 0x0f, 0x13, 0x42, 0x43 | 200/S |
| 0x0b | 600 |
| 0x0c, 0x17, 0x1b, 0x1f–0x23, 0x26–0x2e, 0x30, 0x3d, 0x3e, 0x44, 0x68–0x6b, 0x74, 0x86–0x88, 0x90, 0x9e | 100/S |
| 0x10 | 400/S |
| 0x18 | 10/S |
| 0x37 | 400 |
| 0x45, 0x46, 0x67 | 800/S |
| 0x48, 0x49 | 200 |
| 0x6c | 50 if the creature has a buff of type 13, else 100 |
| 0x8b | 10 |
| any other | 500/S |

The sum is a 32-bit integer; callers compare float(sum) with the ability time +0x6c.

Constants: 50 (0x42e5b0), 100 (0x42e5bc), 200 (0x42e5c0), 250 (0x43293c), 300 (0x42e7e0),
400 (0x42e7e8), 500 (0x42e5c8), 600 (0x42e7ec), 800 (0x432940), 1000 (0x42e5d4), 1200
(0x432944), 1500 (0x432948), 1600 (0x43294c), 2000 (0x42e5d8), 5000 (0x42e5e4), 150
(0x42e938), 10 (0x42e598); fixed values are immediates.

### 7.2 Short timers

* Stun duration 0x5fc20: 3000 for a non-player, 1500 for a player (an integer; the hit
  routine's stun roll, `creature-hit.md` §1).
* Skill level 0x65690: always 1000.

---

## 8. Tests and predicates

### 8.1 Usable (0x507b0), given a creature C and an ability id S (32-bit)

In this order; the first rule that decides wins.
1. If S equals C's current ability (the byte, zero-extended) and float(duration(C)) > C's
   ability time: **no**.
2. If C's stun timer > 0 and S ≠ 0x71: **no**.
3. If S = 0x50: **no** if C's current ability is 0x50; else **no** if float(duration(C)) >
   C's ability time (the current ability is still running).
4. If C's current ability is 0x32: **no**.
5. If mana cost(C, S) (§6.3) > C's mana: **no**.
6. If stamina cost(C, S) (§6.4) > C's stamina (+0x9a0): **no**.
7. Look S up in the cooldown map (+0xb78). If an entry exists and its value is not exactly
   0.0 (NaN counts as not 0.0): **no**.
8. If S = 0x1d: usable exactly when C's mana > 0.0. Otherwise **yes**.

Comparisons are ordered: any comparison with a NaN operand is false (so a NaN time or mana
never blocks in steps 1, 3, 5, 6).

### 8.2 Hostile — "can fight" (0x50550), given creatures A and B

Hostile when all hold:
1. A and B are different records.
2. Neither has bit 22 of +0x80.
3. Either kind (+0x60) of A or B is 7; **or** all of: neither class is 0xa1, neither race is
   0x12f, neither kind is 8, and at least one of:
   * A's kind is 1 and (B's kind is not 1, or bit 15 of their +0x80 differs);
   * B's kind is 1 and (A's kind is not 1, or bit 15 differs);
   * A or B has bit 5 of +0x128.

Otherwise friendly.

### 8.3 Smaller predicates

* **Can be damaged (0x50520):** kind 0, 1, 3 or 6; or kind 7 with race not in 0x8c..0x8e.
* **Uses mana (0x5edd0):** class 3, 0x97 or 0x98; or race 0x75 or 0x56; or W2 is type 3 with
  subtype 10, 11 or 12.
* **Ranged (0x5eca0):** class 3, 0x95, 0x96, 0x97 or 0x98; race 0x75 or 0x56; W2 type 3 with
  subtype 10..12; or W2 or W1 type 3 with subtype 6, 7, 8, 10 or 11.
* **Holds a 2H weapon (0x5e9b0):** W2 is 2H, or W1 is 2H.
* **Guarding (0x5ed30):** false if +0x134 > 0. True when the ability is 9; or 0x5f while
  float(duration) > time; or 0x6e; or 0x3c or 0x40 for class 1 specialisation 1. Else false.
* **First buff of type t (0x5e3a0):** the first matching entry, or none.
* **Class skill (0x656e0), the ability on the "Ultimate" button:** class 1: specialisation 0
  → 0x94, else 0x95; class 2: 0 → 0x8f, else 0x8e; class 3: 0 → 0x2f, else 0x90; class 4:
  0 → 0x6d, else 0x8d; any other class → 0. ("Else" is any non-zero specialisation.)

### 8.4 Race group (0x65750), given a race id

| Group | Races |
|---|---|
| 1 | 0x78–0x82, 0x12e |
| 2 | 0x83–0x90, 0xaf–0xbe, 0xc2–0xc6, 0xcc–0xcf, 0xd4, 0xd6, 0xdd–0xdf, 0xe7–0xeb, 0xee–0xfb, 0x106, 0x108–0x10c, 0x10f–0x113, 0x116–0x11e, 0x121, 0x124, 0x129–0x12d |
| 3 | 0x00–0x10, 0x12, 0x2b–0x2e, 0x30, 0x31, 0x33, 0x34, 0x36, 0x4c–0x51, 0x53, 0x54, 0x5f, 0x94, 0x95, 0xa7, 0xbf, 0xc7, 0xca, 0xcb, 0xd0, 0xd8, 0xe3, 0xe4, 0xe6, 0xec, 0xed, 0xfc–0xfe, 0x100, 0x103–0x105, 0x128, 0x130, 0x131 |
| 4 | 0x13–0x24, 0x32, 0x35, 0x37–0x44, 0x4a, 0x4b, 0x57–0x5d, 0x62–0x64, 0x66–0x6b, 0x74, 0x91–0x93, 0x96–0x9f, 0xa1–0xa4, 0xa8–0xac, 0xae, 0xc8, 0xc9, 0xd1–0xd3, 0xd5, 0xd7, 0xd9–0xdc, 0xe0, 0x10e, 0x115, 0x125–0x127, 0x12f |
| 5 | 0x11, 0x29, 0x2a, 0x60, 0x61, 0x75, 0xa5, 0xa6, 0xad, 0xff, 0x10d, 0x114 |
| 6 | 0x49, 0x52, 0x55, 0x6e, 0xe1, 0xe2, 0xe5 |
| 7 | 0x45–0x48, 0xc0, 0xc1, 0x107, 0x11f, 0x120, 0x122, 0x123 |
| 0 | any other |

(The hit routine treats group 6 specially; demon damage of §2.6 applies there.)

---

## 9. Skills (the seven skill counters)

Let *p* be the skill points: for a player the i32 at the offset below; for anyone else
level × 10 (32-bit). Let *k* = 1.0 − 1.0 / (float(*p*) / 20.0 + 1.0) (evaluated as written:
divide, add, reciprocal-style division, subtract).

| Routine | Offset | Sheet label | Value |
|---|---|---|---|
| 0x50b60 | +0xb5c | Climbing Speed | 1.0 if bit 9 or bit 18 of +0x80; else (*k* + 1.0) × 0.5 |
| 0x663f0 | +0xb60 | Swimming Speed | 1.0 if bit 9 or bit 18; else (*k* + 1.0) × 0.5 |
| 0x51190 | +0xb64 | Diving Skill | 0.0 for a non-player; else (1.0 − *k*) × 0.1 |
| 0x652d0 | +0xb68 | Riding Speed | *k* × 1.4 + 1.1 |
| 0x5e3d0 | +0xb6c | Hang Gliding Speed | *k* × 1.9 + 1.1 |
| 0x65330 | +0xb70 | Sailing Speed | *k* × 1.9 + 1.1 |
| 0x5f6e0 | +0xb74 | Light Radius | *k* × 12.0 + 8.0 |

The sheet shows the speeds × 100 rounded with "%", diving as a stamina-per-second figure,
and light radius in metres. Constants: 20.0 (0x42e5a0), 0.5 (0x4ac964), 0.1 (0x42e528),
1.4 (0x4328bc), 1.9 (0x4328d4), 1.1 (0x42e538), 12.0 (0x42e59c), 8.0 (0x4aca30).

---

## 10. Level curves

### 10.1 Saturating curves of a level-like integer *n*

All share *j* = 1.0 − 1.0 / ((float(*n*) − 1.0) / 50.0 + 1.0):

| Routine | Value | Used by |
|---|---|---|
| 0x61150 | *j* × 0.5 + 0.5 | interactions (clone aura strength) |
| 0x65460 | trunc(*j* × 15000.0 + 15000.0) | interactions (clone aura duration) |
| 0x656a0 | *j* × 0.4 + 0.8 | upkeep (called with 1000 from 0x65690) |
| 0x504e0 | *j* × 1.8 + 1.2 | ability update while gliding (called with +0x1e0) |

Constants: 50.0 (0x42e5b0), 15000.0 (0x432950), 0.4 (0x42e52c), 0.8 (0x42e530), 1.8
(0x4328d0), 1.2 (0x42e718).

### 10.2 Experience for the next level (0x5fa80), given the level *L*

trunc(pow(2.0, double(float(*L* − 1) × 0.5)) × 10.0), in double: the exponent is formed in
float (*L* − 1 as a 32-bit integer, converted, × 0.5), widened, raised with the double pow,
multiplied by 10.0 in double, truncated. The HUD shows experience (+0x148) against it.
Constants 2.0 and 10.0 as doubles at 0x4ac9d8 and 0x4ac9f8.

### 10.3 Per-level factor (0x61000)

(*x* − 1.0) × 0.1 + 1.0 for a float *x*; used for coins and rewards (`creature-hit.md` §4.4,
`drops-and-items.md`).

---

## 11. Where these are shown: the character sheet (0x273380)

For reference (display only): "LEVEL" = +0x144; "Weapon Level" / "Armor Level" draw stars
from the item level (§1.2) of W1, W2 and of shoulder, chest, hands, feet against the
creature's region; "HIT POINTS" = trunc(maximum health); "SPELL POWER" (class 3) or
"ATTACK POWER" = 0x65c70 / 0x4fa70 with selector 1; "ARMOR", "RESISTANCE"; "CRITICAL" =
critical chance × 100 (no target); "HASTE" = (haste − 1) × 100; "REGENERATION" = stamina
factor × 100; "MANA GENERATION" = mana factor × 100; then the seven skills (§9) with their
point counts. The number formatting routine 0x1a2740 is not specified.

The item tooltip (0xb7b90) shows, for the hovered item and the local player: item level
stars, the classes that may use it (§1.3 with classes 1..4), weapon damage and the upgrade
bonus (§2.1), armour, resistance, health, haste, critical, stamina regeneration (§2.2–§2.5),
heal amount and use time (§2.6), and demon damage.

---

## Verification

Existing oracles in `tests/EngineTests.cpp` that can check these rules (recordings live in
`Extract/oracle/` unless noted):

* **`EngineTests playerstats <stats.bin>`** (CST1, ZoneExtractor's stats.request): each
  sample is a whole creature record, its buff list (0x18 bytes each) and ten floats computed
  by the game: maximum health, attack power (selector 1), spell power (selector 1), armour,
  resistance, critical chance (no target), attack power (selector 0), spell power
  (selector 0), haste, stamina factor. Checks §1, §2.1–§2.5, §3, §4.1, §4.2, §4.4, §4.5,
  §5.1, §5.2, §6.2 exactly (bit-for-bit), including the buff-dependent branches when the
  sample carries those buffs. The test currently compares seven of the ten; selector 1 and
  the stamina factor are printed but not compared and can be added.
* **`EngineTests statsoracle`** (`Extract/characters/*/gear.txt` gear surveys): the player's
  totals with one slot emptied, restored or swapped, plus each item's own health. Checks
  maximum health, attack power, spell power, armour, resistance and critical chance to a
  small tolerance, and item health (§2.2). The survey records category, id, modifier, region,
  rarity, the +0x18 value and material per item, so it exercises the material tables and the
  region rule (the harness places the player in the region most items come from).
* **`EngineTests rulesoracle <anim-*.bin>`** (animation recordings): checks duration 0x4f220,
  its three parts 0x4ead0, 0x660d0, 0x5e9f0 (§7.1), use time 0x108fa0 (§2.6) and "may use"
  0x50640 (§0.5) for all nine slots on every recorded creature, and counts records whose
  health equals maximum health (§5.1) per race. Only buff 0x0e is recorded there; abilities
  that depend on buffs 0x0c or 0x0d cannot be checked exactly.
* **`EngineTests upkeeporacle`** (CWU3/CWU4, `creature-upkeep.md` §7): exercises maximum
  health, haste through the durations, the stamina factor and the mana factor inside the
  upkeep.
* **Hit recordings** (`hit.bin`, CHT1, `creature-hit.md` §6): exercise armour, resistance,
  block value (§5.3), maximum health, the combo threshold, guarding, ranged, demon damage and
  the race group through 0x29eb60.
* **Effect-object recordings** (`effect-objects.md` §Verification): exercise critical chance
  with a target (§4.4, §5.4), the proc roll, attack power and spell power with selector 0, the
  ability damage factor and haste.

Not covered by any recording: prices (§2.7), heal amount (§2.6), the skill curves (§9), the
level curves (§10), the class skill table, and the usable test (§8.1) directly (it is only
reached through the interaction handler). The character sheet and tooltip could be scraped
for §9 and §2.7.

---

## Corrections to earlier specs

* `creature-hit.md` §5.3 gives material 6 an armour and resistance factor of 0.5. The game
  uses 0.25 (0x4ac95c) for both (checked against 0x108d50's jump table and 0x10a1e0).
* `creature-hit.md` §5.5 calls 0x10ad90 "blockItem"; it is the general weapon damage used by
  attack power, spell power and the block value alike (§2.1).
* Not an error, but easy to misread: `creature-upkeep.md` §4.1 describes item power with the
  upgrade term fixed at zero, which holds for every routine there; weapon damage (§2.1) passes
  the attribute count instead (§1.4).

---

## Open questions

1. **powf / pow exactness.** Bit-exact item power needs the C runtime's powf for 2^x with a
   non-integer x; whether a portable exp2f reproduces it for every exponent in [−1, 7) was not
   checked. Experience uses the double pow with a half-integer exponent.
2. **Meanings of the flag bits** of +0x80 (4, 9, 15, 17, 18, 22, 26) and of +0x128 bit 5 are
   not established; the rules above use them only as numbers. Bits 9 and 18 behave like
   "elite/boss" multipliers (×10 and ×15 health), bit 26 like a small global bonus.
3. **Buff type meanings** (9, 0x0c, 0x0d, 0x0e, 0x16, 0x17, 0x18, 0x19, 0x20, 0x21, 0x22) are
   named here only by effect.
4. **Region upkeep.** Who writes +0x150/+0x154 (and whether it is the zone's region or the
   region under the camera) was not traced; the item level depends on it.
5. **Item types 0x1a and 0x1c** appear in the level rule, stamina regeneration and use time
   without being identified.
6. **Material and subtype names** are given as numbers only; the class table of §1.3 implies
   which weapon subtypes belong to which class.
7. **The heal-amount threshold** in the tooltip and the routine that turns a consumable's
   heal amount and use time into healing over time were not traced.
8. **0x510f0's inventory tab** is the second tab (index 1); that this tab holds the special
   items (pets, artifacts) is inferred, not confirmed.
9. **Display rounding** (0x1a2740 and the sheet's roundf use) is unspecified; it matters only
   for matching the UI text.
10. **Division by zero.** Haste can reach 0.0 only with absurd inputs; then every "/S" part
    becomes trunc(±∞) = −2147483648 per the hardware. An implementation should reproduce
    this rather than trap.
