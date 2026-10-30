# Creature abilities, part 2: shots, casts and the other branches (0x2c5250)

Second half of the specification of the ability step 0x2c5250. Part 1
(`creature-abilities.md`) holds the conventions and names used here (§0: t, a, T, M, D,
"passes", "reaches", "n-ms boundary", the deciding world, m, w, p, V, set A and set B, the
producers and their default fields), the switch and the full case list (§2), the common tail
(§3) and the melee branch (§4). Every branch below ends in that tail.

Where a branch creates effect objects, the record's contents are specified in
`effect-objects.md` §3 and are only referred to here; this document adds what that one leaves
out (who runs the branch, the other sounds and particles, mana and stamina, buffs) and corrects
it where noted.

---

## 1. Staff and wand shots, volleys, the meteor: 0x26–0x30, 0x7b (0x2cf5fa)

Only in the deciding world. In order:

1. **Charging hum.** If t ≤ float(T − 200) and a 200-ms boundary: a sound at C, id 0x27,
   pitch rand × 0.25 / 32767 + 1.0, owner C.
2. **Shot moment** S = T; for a ∈ 0x2e..0x30, with third = M / 3: e = trunc32((dt + t) −
   float(T)); S = T + (e − e mod third) (C remainder); if S < T, S = T.
3. **Mana.** If C passes T and the aim's squared length (0x2b650: (x² + y²) + z²) > 0: mana =
   mana − mana cost(a) (no floor).
4. A shot is made only if t < float(T + M), C passes S (as a float), and the aim's squared
   length > 0. Otherwise the branch ends.
5. The shot: `effect-objects.md` §3.3 steps 5–7 (one rand for its damage; the record goes to
   W+0x60; then if +0x158 > 0, mana = mana − +0x158 with a floor at 0, and +0x158 = 0; then,
   if its flag F is set, consume buff 9; then the copy to W+0xd0; then the cast sound, volume
   0.9 (0x42e534), owner 0, built on the 0x80270 defaults).

rand order: hum, damage.

## 2. Arrows, boomerang, homing bolt: 0x17, 0x18, 0x1b, 0x6c, 0xa0 (0x2ceb05)

Only in the deciding world. As `effect-objects.md` §3.1, where "t crosses X" means "C passes
X" in the sense of part 1: stamina at t = 0.0; mana and the charge release when C passes T;
a shot when C passes T, or for 0x1b with +0xba8 > 0 when C passes T + 300 (the boomerang's
return throw); consume buff 0x0a when the shot's flag is set; the shot's sound (owner C).
Stamina costs: 0x18 none, 0x17 none, 0x1b none, 0x6c 0.25, 0xa0 1.0 (`stat-rules.md` §6.4).

rand order: damage, sound.

## 3. Arrow barrage: 0x38 (0x2cf1ea)

Only in the deciding world. As `effect-objects.md` §3.2: when C passes T, release the charge
(+0xba8 = +0x158; mana = mana − +0x158 without a floor; +0x158 = 0) and consume buff 0x0a;
then every (M / 4)-ms boundary while float(T) ≤ t < float(T + M), if the aim's squared length
is above 0, a sound at C (id 0x16, pitch rand × 0.5 / 32767 + 1.0, owner C) and an arrow.

rand order per arrow: sound, damage.

## 4. Area casts: 0x62, 0x63 (0x2c9ffc)

For **every** C (no gate): if t < float(T) and a 200-ms boundary: hum (id 0x27, pitch rand ×
0.25 / 32767 + 1.0, owner 0). Then, only in the deciding world, when C passes T: a sound at C
(id 0x29, pitch rand × 0.25 / 32767 + 1.0, owner 0) and the area of `effect-objects.md` §3.4
(hit type 1 for 0x62, 2 for 0x63, the healing area).

## 5. Charging moves: 0x19, 0x1a, 0x1c, 0x25, 0x3c, 0x40, 0x41, 0x93 (0x2cad1d)

Only in the deciding world: on a 200-ms boundary, a sound at C, id 0x3a, pitch +0x158 × 0.5 +
1.0, owner C. No rand. (The charge +0x158 itself is built by the physics and the AI.)

## 6. Healing an ally: 0x1d (0x2cae0d)

Only in the deciding world:

1. +0x158 = dt × 0.0002 (0x472174) + +0x158; if 1.0 < +0x158, +0x158 = 1.0.
2. Only on a 200-ms boundary:
   1. A particle burst at C's position + dots(aim) per axis: velocity (0, 0, 10.0), size 0.15
      (0x42e6fc), count 6, style 2, spread 3.0; colour by C+0x970: 4 → (1.0, 0.4, 0.3), 5 →
      (0.2, 1.0, 0.4), other → (0.2, 0.6, 1.0); alpha unset.
   2. X = the creature with id +0x1a8. Stop unless X exists and 0 < X's health.
   3. A damage number {from C, to X, amount = maximum health of X × −0.1 (0x42e7f8), position
      X's} appended to W+0x90.
   4. If authoritative, or both C and X are local: the **same number is appended a second
      time**, then applied (0x29dcf0). (On a client watching others only the first copy is
      made.)

No rand.

## 7. Ray spells: 0x68–0x6b (0x2cb299)

Only in the deciding world. "held" = a ∈ {0x69, 0x6a}.

1. **Mana.** If C passes T and held: mana = mana − mana cost(a) (0.3, or 0 with buff 9); then
   consume buff 9.
2. If mana is negative (≤ 0 and ≠ 0): mana = 0 and the branch ends.
3. The branch ends unless float(T) ≤ t + dt and t < float(T + M).
4. **Sounds.** If a ∈ {0x68, 0x6b} and C passes T: a sound at C, id 0x2b, pitch rand × 0.1 /
   32767 + 1.0, owner 0. Otherwise, if held and a 200-ms boundary: a sound at C, id 0x27,
   pitch rand × 0.25 / 32767 + 1.0, owner 0.
5. **When it strikes.** If C passes T, strike (step 6). Otherwise, if held, with u =
   t − float(T): strike if trunc32(u) / 200 ≠ trunc32(u + dt) / 200 and float(T) < t + dt
   (re-read). Otherwise the branch ends.
6. **The strike.**
   1. n = the aim; if (x² + y²) + z² > 0.0, normalise it (0x127f0).
   2. O = the shot point 0x654a0(C) (`effect-objects.md` §3.6). d = 0x179b0(aim) (q = 1 /
      sqrtf((x² + y²) + z²), each component × q). L = 0x2d1d20(W, O, d, 200.0, mode 0, honour
      1) (`effect-objects.md` §2.5), the distance to the first block, at most 200.
   3. Empty set A, then set B.
   4. **Blocks.** If L < 200.0: E = O + dots(d' × L) per axis, with d' = 0x179b0(aim) again
      and the product (d'.x × L), then × 65536, truncated. For ox, oy, oz each −4 … 4 (x
      outer, z inner), Q = E + offsets × 65536: if the squared distance in blocks of (Q − E)
      < 16.0, break the block at Q exactly as part 1 §4.4 step 2 (debris burst, removal, block
      change). If any broke: a sound **at E**, id 0x1a, pitch rand × 0.4 / 32767 + 0.5, owner
      0; and 0x2e0710(W, trunc-block(E.x − 0x40000), trunc-block(E.y − 0x40000),
      trunc-block(E.x + 0x40000), trunc-block(E.y + 0x40000)).
   5. **Burning ground** (class 3, specialisation 1 only): an area at the ray's end,
      `effect-objects.md` §3.6 ("at the beam's end").
   6. **Creatures.** If L > 0: step = dots(n) per axis; i = 0; P = O. Repeat: for each X in
      W's list, in order:
      * skip X if X is C, not (0 < X's health), X is in set A, or X's dodge timer ≠ 0.0;
      * rX = X+0x84 × 0.5 + 1.0; skip unless the flat distance test of (P − X) passes with
        r = rX and |blocks(P.z − X.z)| (via double) < X+0x8c × 0.5 + 1.0;
      * insert X into set A;
      * r1 = rand; c = crit(X, true); critical = r1 / 32767 < c;
      * (a knock-back direction is computed from C to X and then not used);
      * damage = ((rand × 0.05 / 32767) + 1.0) × (m × p), m and p asked before the draw; if
        critical, damage = damage + damage;
      * if C and X are hostile: hit request (target X, damage, critical, spirit flag = held,
        stun chance = 0.1 if held else 0.0, position P, push (0, 0, 0), hit type = 1 for
        0x68/0x69, 2 for 0x6a/0x6b, but 7 whenever C's specialisation is 2; ability 0;
        0; blockable 1); if it landed and a = 0x68 and the proc roll 0x5e820(C, 0.25)
        succeeds: give C {9, 0, 30000.0, 0} (refresh 0) with its notice and a sound at C, id
        0x32, pitch 1.0, owner 0; then, for class 3 specialisation 1 when C passes T,
        burning ground at P (`effect-objects.md` §3.6 "at a struck enemy"); struckAny =
        true.

      Then P = P + step, i = i + 1, and repeat while float(i) < L. After the loop: if
      struckAny, +0x70 = +0x70 + 1, 0x603f0(C) (no effect), +0x74 = 0.
   7. **No one touched.** If set A is empty: a particle burst at C's position + dots(d'' × L)
      (d'' = 0x179b0(aim)): velocity (n.x × −1.0, n.y × −1.0, n.z × −1.0), colour (1.0, 0.2,
      0.5, alpha unset), size 0.1, count 5, style 1, spread 3.0; then drop the combo (part 1
      §3.1). (Friendly creatures in the ray count as touched.)

rand order: sound (step 4), block sound (step 6.4), then per touched creature: critical,
damage, draws inside the hit, the 0x68 proc.

Correction to `effect-objects.md` §3.6: the branch also tests for abilities 0x74 and 0x1d
(a 10-block reach, the charge, a different damage), but the switch never sends 0x74 or 0x1d
here (they go to the melee branch and §6), so those paths are unreachable.

## 8. The channelled beam: 0x88 (0x2c97c1)

1. For every C: if mana < mana cost(0x88) (0.03125) and t < float(T + M): +0x6c = float(T + M)
   (the beam ends now).
2. The rest only in the deciding world, and only if t ≤ float(T + M) (t re-read).
3. On a 200-ms boundary: a sound at C, id 0x27, pitch rand × 0.25 / 32767 + 1.0, owner 0.
4. Pulse period Q = trunc32(float(T + M) / 20.0). The rest only on a Q-ms boundary of t and
   only if mana ≥ mana cost (NaN fails).
5. mana = mana − mana cost. Empty set A (it stays empty: nothing below inserts into it).
6. L = 0x2d1d20(W, C's position, 0x179b0(aim), 5.0, mode 0, honour 1).
7. For i = 0 … 5: d = 0x179b0(aim); f = (L × float(i)) / 5.0; P = C's position + dots(d × f)
   per axis (d.x × f, then × 65536); R = (float(i) / 5.0) × 5.0 + 1.0. For each X in W's list:
   if C and X are hostile, X's hostility kind is not 8, X is not in set A, and R × R > the
   squared distance in blocks of (X − P):
   1. a sound at C, id 0x2a, pitch 1.0, owner 0;
   2. push = 0x179b0(aim) × 8.0 per component;
   3. r1 = rand; base = ((m × p) × t) / float(T + M) (m for 0x88, t read now); critical =
      r1 / 32767 < crit(X, true); r2 = rand; damage = (1.25 − r2 × 0.5 / 32767) × base (not
      doubled when critical);
   4. hit request (target X, damage, critical, spirit flag 1, stun chance 0.05, position P,
      push, hit type 1, ability 0, 0, blockable 1). Its result is not used.

A creature may be hit at several of the six points of one pulse.

## 9. Using an item; the healing skill: 0x54, 0x55, 0x82 (0x2c5337)

Only in the deciding world. Let k = the item-in-use type (+0x1d8).

**Not food and not 0x82** (k ≠ 1 and a ≠ 0x82): only when t = 0.0 exactly:
1. A sound at C, id 0x2f, pitch 1.0, owner 0.
2. If k = 0x1c (a potion): by its subtype (+0x1dc) the buff {type, strength, 600000.0
   (0x4725bc), source 0} — 1 → (0x17, 1.2), 2 → (0x18, 1.2), 3 → (0x19, 1.2), 4 → (0x1a, 0),
   5 → (0x1b, 0), 6 → (0x1c, 0), any other → (0x16, 1.2); bytes +1..+3 and +0xc..+0xf of the
   buff are unset. For subtype 5, first 0x2d2b40(W, C, 0x1d): if C has a buff of type 0x1d
   (first match) its duration becomes 0.0 and a notice {C, C, that buff, flag 0} is appended.
   Then give C the buff with the **refresh flag set** and append its notice (flag 0).

**Food (k = 1) or the healing skill (a = 0x82):**
1. U = the item's use time 0x108fa0 (`creature-physics.md` §7.5). For 0x82, U = D instead,
   and when C reaches 50.0: a particle burst at C's position (velocity (0, 0, 5.0), colour
   (0.6, 0.5, 0.3, 1.0), size 0.4, count 0x0f, style 0, spread 3.0), then a sound at C, id
   0x30, pitch rand × 0.1 / 32767 + 0.8, owner 0.
2. Only if t < float(U) and a 200-ms boundary:
   1. share = 200.0 / float(U); if C's hostility kind ≠ 0, share = share × 0.5.
   2. heal = maximum health of C × share.
   3. If heal > 0: a damage number {from −1 (all bits set), to C, −heal (sign bit flipped),
      position C's}; health = heal + health; if health > maximum health (asked again),
      health = maximum health (asked a third time).
   4. On a 400-ms boundary: a sound at C, id 0x2f, pitch 1.0, owner 0.

## 10. Healing from mana: 0x23 (0x2c8ed0) and 0x86, 0x87 (0x2c9290)

### 10.1 Ability 0x23
1. For every C: if t < float(T) and a 200-ms boundary: hum (0x27, pitch rand × 0.25 / 32767 +
   1.0, owner 0).
2. In the deciding world, only if float(T) < t ≤ float(T + M):
   1. Q = trunc32(float(M) × 0.125). Continue only if trunc32((dt + t) − float(T)) / Q ≠
      trunc32(t − float(T)) / Q.
   2. Continue only if mana ≥ mana cost (0.0625); mana = mana − mana cost.
   3. X = the creature with id +0x1a8, or C itself if none (no health test).
   4. r = rand; amount = (p × −2.0) × (1.25 − r × 0.5 / 32767) (p asked after the draw;
      0x42e60c). Damage number {from C, to X, amount, position X's}.
   5. A sound at C, id 0x2c, pitch 1.0, owner 0.
   6. If authoritative or both C and X are local: apply it (0x29dcf0).

### 10.2 Abilities 0x86 (around the aim) and 0x87 (around C)
1. For every C: if mana cost (0.03125) > mana and t < float(T + M): +0x6c = float(T + M).
2. In the deciding world, only if t ≤ float(T + M):
   1. On a 200-ms boundary: hum (0x27, pitch rand × 0.25 / 32767 + 1.0, owner 0).
   2. Q = trunc32(float(T + M) × 0.0625); continue only on a Q-ms boundary of t.
   3. Continue only if mana ≥ mana cost; mana = mana − mana cost.
   4. Centre = C's position, plus dots(aim) per axis for 0x86.
   5. For each X in W's list (C included): if C and X are not hostile and X's hostility kind
      is not 8 and 25.0 (0x42e5a4) > the squared distance in blocks of (X − centre): r =
      rand; amount = (p × −0.5) × (1.25 − r × 0.5 / 32767); damage number {from C, to X,
      amount, position X's}; a sound at C, id 0x2c, pitch 1.0, owner 0; if authoritative or
      both C and X are local, apply it.

## 11. Strikes on a focus target: 0x8c (0x2c8b16)

In the deciding world, only if float(T) < t ≤ float(T + M):
1. Q = trunc32(float(M) × 0.0625); continue only if trunc32((dt + t) − float(T)) / Q ≠
   trunc32(t − float(T)) / Q.
2. Continue only if mana ≥ mana cost (0 for 0x8c; mana is not reduced).
3. F = the creature with id +0x1a8; stop if none (no health test).
4. damage = (1.25 − rand × 0.5 / 32767) × (m × w) (m and w asked before the draw);
   critical = rand / 32767 < crit(F, false) (second draw); if critical, double it.
5. Hit request (target F, damage, critical, spirit flag 1, stun chance 0.0, position C's,
   push (0, 0, 0), hit type 0, ability 0x8c, 0, blockable 1).
6. If it landed: +0x70 = +0x70 + 1, 0x603f0(C), +0x74 = 0.

(The circling around F is in the upkeep, `creature-upkeep.md` step 4.)

## 12. Skills that start something

* **0x70, 0x71, 0x73** (0x2c9ea6, 0x2c9dfb, 0x2c9f51): in the deciding world, when t = 0.0:
  0x29d3c0(W, C, key) with key 0x0c, 1 and 6 respectively — these keys are not skill ids that
  routine gives buffs for, so it only sets C's cooldown entry for that key to float(0x50bd0(C,
  key)) = 0.0 (`skills-and-interactions.md` §1.5, §1.7); then a sound at C, id 0x32 (0x70),
  0x60 (0x71) or 0x5f (0x73), pitch 1.0, owner 0. (Their buffs are given by the R press,
  not here.)
* **0x8e** (0x2c8d3a): in the deciding world, when t = 0.0: the ranger's area,
  `effect-objects.md` §3.5.
* **0x8f, 0x90, 0x95** (0x2cab73, 0x2cac73, 0x2cabe3): in the deciding world, queue an
  interaction of kind 11, 12 or 13 on C (`skills-and-interactions.md` §1.8): 0x8f when C's
  flag 0x800 (+0x80) is clear and t = 0.0; 0x90 when t < float(T) and trunc32(t) / (T / 8) ≠
  trunc32(t + dt) / (T / 8) (T / 8 by C division; 0 would divide by zero); 0x95 when C passes
  T.

## 13. Movement abilities

### 13.1 Dashes and rolls: 0x31, 0x32, 0x52 (0x2c5a1b)
In the deciding world: if t = 0.0, stamina = stamina − stamina cost(a) (0x32, 0x52: 0.25;
0x31: 0), floor 0. Then if t ≤ 0.0 and 0.0 < t + dt: a sound at C, id 0x0e, pitch rand × 0.25
/ 32767 + 1.0, owner C.

### 13.2 Blink: 0x33 (0x2ca321)
In the deciding world, when C passes T:
1. mana = mana − mana cost (1.0); if 0 > mana, mana = 0.
2. s = (x² + y²) + z² of the aim; stop unless s > 0. len = sqrtf(s) (recomputed the same
   way). d = 0x179b0(aim). L = 0x2d1d20(W, C's position, d, len, mode 0, honour 1).
3. E = C's position + trunc64(((L × d.x) × 65536)) per axis (L × component first).
4. pass = C has a buff of type 0x13. best = 400.0 (0x42e7e8), B = E.
5. For ox = −4 … 4 (outer), oy = −4 … 4, oz = −2 … 6 (inner): Q = E + (ox, oy, oz) × 65536.
   If the overlap test 0x2b77c0(W, Q, C's sizes +0x84, pass, honour 1) says free
   (`creature-physics.md` §7.1): d² = squared distance in blocks of (Q − E); if best > d²,
   best = d² and B = Q.
6. If 400.0 > best: C's position = B; then repeat: if 0x2b77c0 at (B.x, B.y, z − 65536) says
   blocked, stop; read the block at (x, y, z − dots(C+0x8c × 0.5) − 65536) (0xeab0); if its
   type byte is 2 (water), stop; otherwise z = z − 65536. Finally +0xbb0 = blocks(z).
No rand.

### 13.3 Full stealth: 0x34 (0x2cf174)
In the deciding world, when t = 0.0: +0x18c = 1.0; stamina = stamina − stamina cost (1.0),
floor 0.

### 13.4 Map destination-cell relocation: 0x75 (0x2c5886)
Only for the local player, when C passes T: with hx = +0x1d0, hy = +0x1d4 (i32): position =
(((hx × 16) + 6) × 65536, ((hy × 16) + 8) × 65536, 0); then z = 0x2d87f0(W, hx, hy, 0) ×
65536 (a cell height from the world generator, not specified here). If C's mount/pet
(+0x9d8) exists, its position becomes C's.

### 13.5 Travel along a curve: 0x98 (0x2cd9bb)
Only for the local player, and only if t > 2000.0 (0x42e5d8). S = +0x1b8 (start, dots), E =
+0xae0 (destination, dots).
1. If 64.0 > the squared length in blocks of (C's position − E): ability = 0; stop.
2. u = (t − 2000.0) / 20000.0 (0x42e5ec).
3. Control points in blocks, relative to S: P0 = (0, 0, 0); P1 = (0, 0, blocks(0 +
   ((E.z + 20 × 65536) − S.z))) (0x29c0b0 then 0x29c110; the zero is dots(0.0)); P2 =
   blocks(E − S) with z + 20.0; P3 = blocks(E − S).
4. De Casteljau with u, each lerp "A + (B − A) × u" computed per component as ((B − A) × u)
   + A: A1 = lerp(P0, P1), B1 = lerp(P1, P2), C1 = lerp(P2, P3), D1 = lerp(A1, B1), E1 =
   lerp(B1, C1).
5. Target G = S + dots(D1) + dots((E1 − D1) × u) (64-bit adds, in that order).
6. f = the smoothing factor 0xa7a0(double dt, 0.01) (`creature-upkeep.md` §4.9). Position =
   position + dots(blocks(G − position) × f) per axis.
7. +0xb10 = G; velocity, movement intent and knock-back (+0x34..+0x54) = 0; +0xbb0 =
   blocks(z).

### 13.6 Eagle flight: 0x9c (0x2cdfd9)
Only for the local player. tb = trunc-block. ground(x, y) = 0x8a140(W, x, y), the land's
height at a block column (world generator, not specified here); terrain(x, y) = 0x35ea0(W,
x, y, 0) (`effect-objects.md` §2.8).

1. S = +0x1b8 (x, y); D = (+0x1b0 × 65536, +0x1b4 × 65536). S.z = dots(max(ground(tb(S.x),
   tb(S.y)), 0) + 200.0); D.z = dots(max(ground(tb(D.x), tb(D.y)), 0) + 200.0) (maxss: a NaN
   height gives 0).
2. N = 0x2adc50(start = (tb(S.x), tb(S.y)), end = (tb(D.x), tb(D.y))): dx, dy as 32-bit
   differences (start − end), N = trunc32(sqrt(double(dx)² + double(dy)²) × 2.5) (double),
   clamped to 10000 … 30000.
3. **Arrival** (t ≥ float(N)): ability = 0; C.x = D.x, C.y = D.y (z unchanged). In inventory
   tab 1 (+0x9f0, second tab), the first slot with count > 0 and an item of type 0x18
   subtype 1 (the hang glider): ability = 0x78 and the item is copied to +0x1d8. If none:
   ability = 0x78, +0x1d8 = 0x18, +0x1dc = 1 (the rest of the item unchanged). t = 0. The
   landing marker of `effect-objects.md` §3.7 and its sound (id 0x79, pitch 1.0, owner 0).
4. **Under way** (t < float(N)), in double: q = double(t) / double(N − 1); q = q × q; g =
   1.0 − (1.0 − q) × (1.0 − q).
   1. Point K = S + scale(D − S, trunc64(g × 65536.0)), with scale(v, f) = (v × f) divided by
      65536 toward zero, per axis. If g > 0.5: K = D + scale(S − D, trunc64((1.0 − g) ×
      65536.0)).
   2. h1 = ground(tb(K.x), tb(K.y)) + 200.0; h2 = terrain(tb(C.x), tb(C.y)) + 50.0; h = h1 if
      h1 > h2, else h2; h = max(h, 200.0) (maxss); K.z = dots(h). If 0x2b8050 (below): K.z =
      500 × 65536.
   3. C.x = K.x, C.y = K.y. rise = K.z − C.z; C.z = C.z + trunc64(float(rise) × 0xa7a0(dt,
      0.005)).
   4. If g > 0.9 (the double of float 0.9, 0x472200): fall = trunc64(double(D.z − C.z) × (g −
      0.8999999761581421)); C.z = C.z + trunc64(float(fall) / 0.1).
   5. +0xb10 = C's position; knock-back = 0; +0xbb0 = blocks(C.z).
5. **Always**: v = (blocks(D.x − S.x), blocks(D.y − S.y), 0) in double (0x295cc0); r = 1.0 /
   sqrt(x² + y² + z²); v = v × r per component; v = v × 10.0; velocity = v rounded to float.
   With α = t × 0.001 (t re-read; on arrival it is 0): movement intent = (5.0 × cosf(α) +
   velocity.x, 5.0 × sinf(α) + velocity.y, 5.0 × 0.0 + velocity.z).

**0x2b8050 (far flight)**: true when the local player's ability is 0x9c, sqrt((blocks of its
x − D.x)² + (… y − D.y)²) > 1000.0 and the same from +0x1b8 to D > 16384.0 (all double).

### 13.7 Rising on the eagle: 0x9d (0x2cde09)
Only for the local player:
1. h1 = 0x8a140(W, tb(x), tb(y)); h2 = 0x35ea0(W, tb(x), tb(y), 0) + 50.0; H = max(h1, h2)
   (maxss) + 150.0; f = 0xa7a0(dt, 0.001); H = max(H, 150.0).
2. z = z + trunc64(float(dots(H) − z) × f).
3. velocity = (10.0, 5.0, 0.0); knock-back = 0; +0xbb0 = blocks(z).
4. α = t × 0.001: movement intent = (5.0 × cosf(α) + 10.0, 5.0 × sinf(α) + 5.0, 5.0 × 0.0 +
   0.0).

## 14. Summons, pets and others

### 14.1 Summoning: 0x65, 0x66 (0x2ca803)
In the deciding world, when C passes T:
1. Walk W's list: count creatures whose owner (+0x1a0) is C's id and whose flag 0x800 is set;
   track the smallest id, starting from 0x80000000000003e8. If the list was not empty and the
   count is above 2, stop. New id = smallest − 1.
2. Construct a creature record with that id (0x4ce80). Hostility kind 1 if C's is 1 or 7,
   else 4. Level = C's level.
3. Race = 0x25; if C's race list (+0xbf0 … +0xbf8, i32 each) is not empty, race = the
   entry at (rand as a 64-bit value) modulo the entry count (unsigned).
4. By C's summon mode (+0xbe8): 1 → copy C's race, its +0x278 block (0x3b6b0), its
   appearance (+0x78, 0x7e780) and class; 2 → the same from the creature with id +0x9e0 (or
   C if none); otherwise → the race above, the race's default looks (0x53e80) and the
   equipment generator 0x511e0 (hostility kind, race, level, flags, +0x970, the class and
   specialisation bytes, +0x278, +0xbd0, +0xbe8, +0xbf0, +0x9f0; not specified here).
5. Flags: clear 0x200, set 0x800. Level = C's again. Health = its maximum. Position and
   +0x1b8 = C's position. +0x970 = C's. AI flags +0xbcc = 0xb0 for 0x65, 0x1000 for 0x66.
   Owner = C's id. Add to the world (0x29d0d0).

rand: one, only if the race list is not empty.

### 14.2 Taming: 0x7e (0x2ce833)
Only if authoritative, when C reaches T:
1. X = the creature with id +0x9e0 (the last one it fought). Stop unless X exists, X's
   item in use (+0x1d8) has type 0x15 and its subtype equals C's race.
2. A sound at C, id 0x2f, pitch rand × 0.2 / 32767 + 0.9, owner 0.
3. s = the strength of the **last** buff of type 7 in C's list (0 if none). +0x70 = +0x70 +
   1; +0x74 = 0. s = s + 0.2.
4. If s ≥ 1.0: s = 1.0; insert C's id into the dead set (W+0x20); new id = (smallest id in
   W's list, from 0x80000000000003e8) − 1; remove C's old id from the id map (0x2ac530); C's
   id = new; enter it (0x29bf70); hostility kind = 6; appearance block = the defaults of
   0x4d1f0 (0x7e780); 0x2b67b0(W, X's id, C) (makes C X's pet; not specified here).
5. A damage number (0x29a100 defaults) {from C (its id now), to X, s, kind byte +0x45 = 6,
   position C's}.
6. Give-and-tell C {type 7, strength s, duration 5000.0, source = C's owner +0x1a0}, refresh 1.

#### 14.2.1 What the world does in step 4 (CubeVulkan's markDead and renumbered)

Step 4 changes three world structures, in this order, all inline in 0x2c5250:

1. **Dead set** (W+0x20, an ordered set of i64; 0x7d3f0 inserts, nothing if present): the old
   id goes in. The game never takes an id out of this set during play. Its readers:
   * the resident pass (`worldgen-creatures.md` §7.1 step 2): a spawn point whose id is in the
     set is never made into a creature again, so the tamed creature's spawn point stays empty;
   * spawn-point prerequisites (same §7.1: every id in +0x938 must be dead), so taming counts
     as a kill for them;
   * the upkeep's orphaned summons and zone-cleared flag (`creature-upkeep.md` §1c, §1d).
   (On a remote client the dead ids arrive from the host: 0xa5a50 inserts each id of the
   controller's received list +0x1a00 into W+0x20.)
2. **Id map** (W+0x450, id → creature): the entry under the old id is erased (0x2ac530; erase
   by key, nothing if absent); C's id (+0x08) becomes the new id (smallest id in W's creature
   list W+0x40, starting from 0x80000000000003e8, minus 1); the entry for the new id is found
   or made (0x29bf70 → 0x2992e0) and set to C. C stays at the same place in the creature list.
3. Nothing else is renamed. Anything that holds the old id (another creature's target +0x9e0,
   an owner field, a group set, the AI map keyed by id) now finds no creature: the AI record
   under the old id is dropped at the next AI pass (`creature-ai.md` §2.4) and a new one is made
   under the new id. The damage number of step 5 already carries the new id.

### 14.3 A pet with its owner: 0x80 (0x2cca01)
For every C: if +0x1a0 equals C's own id: +0x1a0 = 0, ability = 0. Then only when C reaches
50.0: if authoritative, the creature with id +0x1a8, if its ability is 0x82, gets ability 0;
then, in the deciding world, a sound at C, id 0x30, pitch rand × 0.1 / 32767 + 1.0, owner C.

### 14.4 Throwing a creature: 0x81 (0x2ce6a7)
For every C: if +0x1a8 equals C's own id: +0x1a8 = 0, ability = 0. If t > 2000.0, ability = 0.
Then only when C reaches 50.0:
1. In the deciding world: a sound at C, id 0x0f, pitch rand × 0.1 / 32767 + 2.0, owner C.
2. If authoritative and the creature X with id +0x1a8 exists: α = float((double(yaw) × π) /
   180.0 + π/2); X's knock-back = (10.0 × cosf(α) + C's velocity x, 10.0 × sinf(α) + C's
   velocity y, 5.0 + C's velocity z) (sinf is called before cosf).

### 14.5 Calling up a race-0x12e creature: 0x9a (0x2ccb1a)
Only if authoritative:
1. If 200.0 < t < float(D) and a 400-ms boundary: Y = the yaw basis of C's yaw; o = Y × (0,
   C+0x88 × 0.6, 0.1 − C+0x8c × 0.5) (0xa5960: x = (o.y × Y[4] + o.x × Y[0]) + o.z × Y[8],
   y = (o.x × Y[1] + o.y × Y[5]) + o.z × Y[9], z = (o.x × Y[2] + o.y × Y[6]) + o.z ×
   Y[10]); a particle burst at C's position + dots(o): velocity (0, 0, 5.0), colour (0.6,
   0.5, 0.3, 1.0), size 0.2, count 5, style 0, spread 3.0; then a sound at C, id 0x30, pitch
   rand × 0.1 / 32767 + 0.5, owner 0.
2. If t > 1800.0 (0x47259c): O = the creature with id +0x1a0; if O exists, its race is
   0x12e and its +0x19c is 0: O+0x19c = 1, O's ability = 0x99, O's time = 0. (The upkeep then
   raises it, `creature-upkeep.md` step 1e.)

### 14.6 Self-kill: 0x85 (0x2c59b9)
Only for the local player, when C passes T: health = 0.

## 15. Sounds only

* **9** (0x2c5b29), in the deciding world: when C passes T, a sound at C, id 0x10, pitch
  rand × 0.25 / 32767 + 1.0, owner C; then on a 200-ms boundary a sound at C, id 0x3a, pitch
  +0x158 × 0.5 + 1.0, owner C.
* **0x4a** (0x2cc920), only if authoritative, when C reaches T: a sound at C, id 0x2e, pitch
  rand × 0.25 / 32767 + 0.6, owner 0.
* **0x53** (0x2cb17b), in the deciding world, on a 500-ms boundary: a sound at C, id 0x27,
  pitch rand × 0.25 / 32767 + 0.5, **volume** rand × 0.25 / 32767 + 0.5 (two draws, pitch
  first), owner 0.

## 16. Using a special item: 0x89 (0x2cce4c)

Only for the local player. k = type of +0x1d8, s = its subtype. The ability lasts 5000 ms;
"the use moment" is C reaching 5000.0 (0x42e5e4). zone(x) = floor-block(x) / 64 by floor
(0x16e50); cell(x) = floor-block(x) / 16 by floor (0x137e0). A zone's object list is at
zone+0x60 … +0x68, 0xe0 bytes per object: kind i32 +0, position i64 ×3 +8, a state byte
+0x28.

1. **k = 0x18, s = 7, at the use moment:** for ox = −1 … 1 (outer), oy = −1 … 1: the zone
   (zone(C.x) + ox, zone(C.y) + oy) (0x16e00). For each object of kind 1 whose squared
   distance in blocks to C is below 64.0: set +0xae0..+0xaf0 to 0; for cx = cell(object x) −
   10 … + 10 (outer), cy = cell(object y) − 10 … + 10: if the cell object 0x69c60(W, cx, cy)
   is 0x3c: +0xae0 = (cx × 16 + 24) × 65536, +0xae8 = (cy × 16 + 24) × 65536, +0xaf0 =
   0x2d87f0(W, cx, cy, 0) × 65536; ability = 0x98; t = 0; +0x1b8 = C's position (the last
   match wins). After the cell search: a sound at C, id 0x73, pitch 1.0, owner 0; and this
   zone's remaining objects are skipped. Then the branch ends.
2. **k = 0x18, s = 4, at the use moment:** the same nine zones; for each object of type 0
   (zone object +0x00) within 64.0 (squared), in list order: if its cell pair (cell(x),
   cell(y)) is already in the local player's used-cell set ((local player +0x1528)+0xa8,
   found with 0x93730), nothing happens for that object. Otherwise (CubeVulkan's
   `discover`): insert the pair (0x7d390); then, in every inventory tab of the **local
   player** (W+0x448, here the same creature as C), every slot with a non-zero count and an
   item of type 0x18 and id 4 gets +1 on the i32 at slot +0xc (the item's modifier, +8);
   then a sound at C, id 0x6e, pitch 1.0, owner 0. So the counter and the sound happen once
   per newly found cell, and an object whose cell was found before (by this item, an opened
   reward chest or a read lore cell, which use the same set) gives nothing. The set is the
   one saved with the character and drawn by the map (`lore-and-artifacts.md` §1.1, §4.4,
   §4.5; `map-markers.md` §0.4, where +0xa8 is called "known places"). Then the branch ends.
   Evidence: 0x2c5250 at the item branch, the insert and the inventory walk sit inside the
   "find returned the end" test.
3. **When t = 0.0:** a sound at C, owner C: k = 0x18 with s = 4, 5, 6, 7, 8, 10, 11 → id
   0x6d, 0x6f, 0x71, 0x72, 0x74, 0x76, 0x77, pitch 1.0; any other item → id 0x3c, pitch rand
   × 0.1 / 32767 + 1.0.
4. **k = 0x18, s in 4 … 11 but not 9, t < 5000.0, on a 500-ms boundary:** a particle burst at
   C's position + dots(Y × (0, C+0x88 × 0.6, C+0x8c × 0.0)) (Y = yaw basis, product as
   §14.5): velocity (0, 0, 1.0), colour (1, 1, 1, 1), size 1.0, count 1, style 7, spread 0.5.
5. **k = 0x18, s = 0x0d, when C reaches 200.0:** the nine zones; each object of kind 9 with
   a non-zero state byte, closer than 64.0 (squared distance in blocks, (x² + y²) + z²):
   0x2d3a10(W, object, 0) (the zone-object action); if its state byte changed, insert the
   zone into W+0x1a0 (0x7d360). Then the branch ends.
6. **At the use moment** (any item not ended above): queue an interaction of kind 2 carrying a
   copy of the item (0x9660, kind byte +0xc4 = 2, 0x9cd0, appended with 0x129a0) on C. If
   k = 0x18 and s = 8: a sound at C, id 0x75, pitch 1.0, owner 0; give-and-tell C {type
   0x13, 0, 60000.0 (0x441bec), source 0} with refresh 1; and the same to C's mount (+0x9d8)
   if it exists.

---

## 17. Constants of part 2

| RVA | Value | Use |
|---|---|---|
| 0x472174 | 0.0002 | 0x1d charge rate |
| 0x42e6fc | 0.15 | 0x1d burst size |
| 0x42e7f8 | −0.1 | 0x1d heal share |
| 0x42e60c | −2.0 | 0x23 heal factor |
| 0x42e80c | −0.5 | 0x86/0x87 heal factor |
| 0x42e5a4 | 25.0 | 0x86/0x87 radius² |
| 0x4328a4 | 0.0625 | pulse periods (0x86, 0x87, 0x8c) |
| 0x42eec0 | 0.125 | 0x23 period |
| 0x42e5a0 | 20.0 | beam pulses per duration; 0x98 lift |
| 0x42e524 | 0.05 | beam stun chance; ray damage spread |
| 0x4aca30 | 8.0 | beam push |
| 0x4aca34 | 16.0 | ray block radius² |
| 0x42e7e8 | 400.0 | blink search |
| 0x42e5b4 | 64.0 | 0x98 arrival, 0x89 object radius² |
| 0x42e5d8 | 2000.0 | 0x98 delay, 0x81 limit |
| 0x42e5ec | 20000.0 | 0x98 travel time |
| 0x4725bc | 600000.0 | potion buff length |
| 0x441bec | 60000.0 | 0x89 buff 0x13 length |
| 0x42e5e4 | 5000.0 | 0x89 use moment, taming buff length |
| 0x47259c | 1800.0 | 0x9a raise time |
| 0x42e5b0 | 50.0 | 0x80/0x81/0x82 moment, terrain margin |
| 0x42e938 | 150.0 | 0x9d height |
| 0x42e5c0 | 200.0 | flight height, 0x9a start, ray reach |
| 0x42e514 | 0.001 | weaving rate, 0x9d smoothing |
| 0x42e518 | 0.005 | flight smoothing |
| 0x42e51c | 0.01 | 0x98 smoothing |
| 0x472200 | 0.8999999761581421 (double) | flight's last tenth |
| 0x4ac9a8 / 0x4ac9b8 | 0.5 / 1.0 (double) | flight easing |
| 0x4ac9f8 | 10.0 (double) | flight speed |
| 0x472208 | 2.5 (double) | flight time per block |
| 0x437fd8 / 0x4723c0 | 1000.0 / 16384.0 (double) | far flight |
| 0x42e758 / 0x42e778 / 0x42e740 | π / 180.0 / π/2 (double) | 0x81 throw angle |
| 0x42e598 | 10.0 | throw speed, 0x1d burst velocity |
| 0x42e708 | 0.6 | widths, colours, growl pitch |
| 0x42e530 | 0.8 | 0x82 sound pitch |

---

## 18. Open questions (part 2)

1. **Unspecified callees.** 0x2d87f0 (a cell's height, used by 0x75 and 0x89), 0x8a140 (the
   land's height), 0x69c60 (cell object), 0x511e0 (equipment for a summon), 0x4ce80, 0x53e80,
   0x3b6b0, 0x7e780 (copies and defaults), 0x29d0d0 (adding a creature, partly in
   `skills-and-interactions.md` §2.3), 0x2b67b0 (making a pet), 0x2ac530 / 0x29bf70 (the id
   map), 0x2d3a10 (zone-object action), interaction kind 2 in 0x2c3710. Each needs its own
   spec before these branches can be bit-exact.
2. **Item 0x18's subtypes** (4–11, 0x0d) are named only by behaviour; which special items they
   are, and what the structure at the local player's +0x1528 is, were not established.
3. **+0x1d0 / +0x1d4** (0x75's destination cell) have a verified producer:
   accepted map-marker left release0x965e0 writes the floored16-block
   target cell, then starts0x75. See steam-item-map-selection.md §2 for
   admission/order. This does not establish every other possible writer.
4. **0x9d sets the velocity to (10, 5, 0)** whatever the heading; this looks like a
   placeholder in the game but is what it does.
5. **0x1d appends its heal number twice** on an authoritative world (once before the test,
   once after). Whether the game shows two numbers is unknown; both are appended.
6. **Taming inserts the tamed creature's old id into the dead set**, presumably so others
   forget it; the consequences are outside this step.
7. **The summon branch computes a flag** (hostility 7 or +0x80 bit 0x8000) and never uses it.
8. **Division by zero**: 0x90's T / 8, 0x8c's and 0x86/0x87's periods, 0x38's M / 4, the
   volleys' M / 3 and the beam's period can be 0 for very short durations; the game would
   fault.
9. **No recording yet** of 0x23, 0x33, 0x34, 0x53, 0x54, 0x55, 0x62, 0x63, 0x65, 0x66, 0x70,
   0x71, 0x73, 0x7e, 0x80, 0x81, 0x86, 0x87, 0x8c, 0x9a, or of any local-player branch.
10. **Who runs 0x62/0x63's and 0x23's hum** is "every creature" while the rest of those
    branches is gated; on a client this makes remote creatures hum but not cast. Kept as is.
