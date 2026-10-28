# Creature pose: held items, extra models, trails and draws (0x17ab0)

Part of the creature pose specification; read `creature-pose.md` first for the conventions,
the record A, the matrix helpers (translate, scale, rotX/Y/Z, rotAxis, point, centring) and
the names used here: the final hand places **Rr**, **Lr**, the hand turns **RTurn**,
**LTurn** (`creature-pose.md` §6), the chest and head places **Cp**, **Hp**, **swing**,
**baseSwing**, **legL**, **bob**, **x9**, **glowAll** and the frames:

* **body frame** = A+0x138 as built this call (`creature-pose.md` §8.2);
* **hands frame** = A+0x178 as built this call (§8.7), except in §4.1 where it is the
  **previous** call's A+0x178 (that section runs before the hands frame is rebuilt);
* **base** = the base matrix of §8.2.

"Rr" and "Lr" are always the final places at that point of the call: eased (§6) and, when
the creature rolls or uses ability 0x78, already raised (§8.2 steps 2 and 3).

**quarter** = float(double(swing × 0.25 × 180) / π), the turn of things carried on the back.

## 1. Overview

What this file covers, in the order the routine does it:

1. race 0x12e's rings (§5.1), before anything else;
2. the sparks of state 0x61 (§7), before the base matrix;
3. the board, raft and eagle (§5.2) and the items held while eating, playing or using
   (§4.1), after the body frame;
4. the piece over the head (§5.3), after the head;
5. after the hands frame and arm parts: items between the hands (§4.2), quivers (§3.3), the
   right weapon then the left weapon (§3.2), the shield on the back (§3.4);
6. the weapon tips and trails (§6);
7. the glove grip (§5.4), after the left hand.

## 2. Shared notation

* **glow(slot)** = the item glow (§8.1) of C's item at that offset.
* **size(item)** = the item size (§8.2).
* model(item), model#(id), usable(slot) as in `creature-pose.md` §1.2. Every placement here
  draws only when its model exists.
* **Draw**(model, M, numbers) and its mirrored and weapon-effect forms are in §9. Unless a
  section says otherwise the tint is the call's tint.

---

## 3. Weapons, quivers and the shield

### 3.1 In the hands or on the back
A weapon is drawn **on the back** when the ability keeps weapons away and the item's kind is
not one of the kinds that stay in hand.

The ability keeps weapons away when it is 0 or 1; or one of 0x16, 0x34, 0x50 … 0x5b, 0x5d,
0x5e; or one of 0x64, 0x6c, 0x75 … 0x7a, 0x7c, 0x7d, 0x80, 0x81, 0x89, 0x8a, 0x92 … 0x94,
0x98, 0x9a … 0x9d, 0xa0 … 0xa3. (These are the masks 0x6fff0000001 over ability − 0x34,
0x34 … 0x5e, and 0xf3d1c060337e0101 over ability − 0x64, 0x64 … 0xa3.)

Kinds that stay in hand: right hand (kind at +0x6dc) 4, 0xc, 0x15; left hand (kind at
+0x63c) 4, 0xc, 0x14, 0x15.

### 3.2 The weapon matrices
**Right weapon** (sprite A+0x118, matrix A+0x2f8). Let m be the sprite, k the right kind.

*In hand:* A+0x2f8 = hands frame; translate(Rr); rotZ(RTurn.z); rotY(RTurn.y);
rotX(RTurn.x); rotAxis(0, 0, 0, 1); rotAxis(0, 0, 1, 0); rotAxis(C+0x58 × 0.5 + 0, 1, 0, 0);
scale(weapon scale, uniform); if k is 0x15: rotX(−45), translate(0, 0, 5). Then the final
offset, starting from (sx × −0.5, sy × −0.5, sz × −0.5):
* if m is model#(0x401): (sx × −0.5 − 1, 0.5, 1);
* else if k is 0xa: z = −10;
* else if k is 0xc: scale(0.6, uniform) now, and y = float(1 − size y as integer);
* else if k is 6: y = 1.5 − sy;
* else if k is 7: y = 0;
* else if k is 8: y = −2, z = −4;
* else if k is not 4: z = −4.

*On the back:* A+0x2f8 = body frame; rotZ(quarter); translate(0, −7.1, 0.1); rotAxis(−45
if k is 0xa, else −135, 0, 1, 0); rotAxis(90, 0, 0, 1); scale(weapon scale, uniform); the
final offset is (sx × −0.5, sy × −0.5, sz × −0.5).

Then translate by the final offset. Glow: 1 if glowAll, else glow(+0x6d8). Fourth number
C+0x994. Draw, then the weapon effect (§9) for the item at +0x6d8. Then the tips (§6.1).

**Left weapon** (sprite A+0x120, matrix A+0x338). Let k be the left kind. Fourth number
w = C+0x994, plus (cos(t × 0.01) + 1) × 0.25 when C+0x158 > 0 (t = A+0x08).

*In hand:* A+0x338 = hands frame; if the state is 0x4d: translate((0, 0, 8) + 0.5 × (Rr +
Lr)); otherwise translate(Lr.x + 0.01, Lr.y + 0.01, Lr.z + 0.01). rotZ(LTurn.z);
rotY(LTurn.y); rotX(LTurn.x); rotAxis(0, 0, 0, 1); rotAxis(0, 0, 1, 0); rotAxis(C+0x58 ×
0.5 + 0, 1, 0, 0); scale(heldScale, uniform). Final offset from (sx × −0.5, sy × −0.5,
sz × −0.5):
* if the sprite is model#(0x401): (1 − sx × 0.5, 0.5, 1);
* else if the state is 0x4d: unchanged;
* else if k is 6: y = 1.5 − sy;
* else if k is 7: y = 0;
* else if k is 0xc: scale(0.6, uniform) now, y = float(1 − size y as integer);
* else if k is 0xd: y = 3;
* else if the **right** kind is 8: y = −2, z = −4;
* else if k is 4: rotY(180) now (offset unchanged);
* else z = −4.

*On the back:* A+0x338 = body frame; translate(0.5, −0.65, 0.5); rotZ(quarter); if k is
0xd: translate(0, −8.2, 0), rotAxis(180, 0, 0, 1). Otherwise: if k is 6 or 7:
translate(0, −6.2, 0), rotAxis(−45, 0, 1, 0); else translate(0, −7.2, 0), rotAxis(−225, 0,
1, 0); then rotAxis(90, 0, 0, 1). scale(heldScale, uniform). Final offset (0.001 − sx ×
0.5, 0.001 − sy × 0.5, 0.001 − sz × 0.5).

Then translate by the final offset. Glow: 1 if glowAll, else glow(+0x638). Fourth number w.
Draw, then the weapon effect for the item at +0x638. Then the tips (§6.1).

The rotations by 0 degrees are real calls (§2a of the main file).

### 3.3 Quivers
For the left item (+0x638) with turn 45, then the right item (+0x6d8) with turn −45: if the
item's category is 3, its kind is 6 or 7 and model#(0x400) exists: M = body frame;
rotZ(quarter); translate(0, −7.1, 0.1); rotAxis(turn, 0, 1, 0); rotAxis(90, 0, 0, 1);
scale(weapon scale + 0.01, uniform); centre. Draw: glow 0, fourth C+0x994. (Not stored.)

### 3.4 The shield on the back
If there is a shield (`creature-pose.md` §4.5): M = body frame; translate(0.65, −0.5, 0.3);
rotZ(quarter); translate(0, −6.2, 0); rotAxis(−45, 0, 1, 0); rotAxis(90, 0, 0, 1);
scale(C+0xb4, uniform); centre. Draw: glow 0, fourth C+0x994. (Not stored.)

---

## 4. Items held in use (the item at +0x1d8)

### 4.1 Eating, playing, busy hands (the previous hands frame)
Each line applies when the state and the clock t (A+0x08) match and model(+0x1d8) exists.
M starts as **the hands frame left by the previous call** (A+0x178 before it is rebuilt).
Draw numbers: alpha, x9, glow(+0x1d8), 0, depth. Tested in this order:

| State and time | Placement |
|---|---|
| 0x4e or 0x4f with 5000 > t, or 0x63 with 300 > t | k = size(item) × 12; translate((0, 0, 8) + 0.5 × (Rr + Lr)); rotZ(t × 0.02); rotY(45); rotX(45); scale(k, uniform); centre. |
| 0x5a, 5000 > t | translate((0, 0, −1) + 0.5 × (Rr + Lr)); scale(0.8, uniform); centre. |
| 0x5b, 5000 > t | translate((7.5, 0, 0) + Lr); scale(0.75, uniform); centre. |
| 0x5e, 5000 > t | if C+0x80 bit 0: rotAxis(legL, 1, 0, 0); translate(Rr); rotZ(RTurn.z); rotY(RTurn.y); rotX(RTurn.x); scale(0.75, uniform); translate(sx × −0.5, sy × −0.5, −sz). |
| 0x5f, 5000 > t | translate((7.5, 0, 0) + Lr); scale(0.85, uniform); centre. |
| 0x5d, 5000 > t | translate(0.5 × (Rr + Lr)); scale(0.75, uniform); centre. |
| 0x62, 1000 > t | translate(Rr); translate(−2.3, 0.3, 0.13); rotZ(the **target** RT.z of this call, before easing); scale(0.75, uniform); centre. |
| 0x5c, 5000 > t | translate((0, −7, 0) + Lr); scale(0.85, uniform); centre. |
| 0x60, 5000 > t | translate(Rr); rotZ(RTurn.z); rotY(RTurn.y); rotX(RTurn.x); scale(0.5, uniform); translate(sx × −0.5, sy × −0.5, −sz). |

"0.5 × (Rr + Lr)" is the sum first, then each component × 0.5; "(a, b, c) + P" adds per
component. The tests use t, while the state's targets (tables) use the ability clock for
these states.

### 4.2 Between the hands (the current hands frame)
After the arm parts:
* Ability 0x54 or 0x55 with float(U) > C+0x6c: M = hands frame; translate(0.5 × (Rr + Lr));
  rotX(LTurn.x); k = size(item) × (hand scale × 12); scale(k, uniform); centre. Draw: glow
  0, fourth C+0x994.
* Ability 0x56: M = hands frame; translate(0.5 × (Rr + Lr)); rotX(LTurn.x); scale(hand
  scale, uniform); translate(sx × −0.5, sy × −0.5, 0). Draw: glow 0, fourth C+0x994.

---

## 5. Extra models

### 5.1 Race 0x12e's rings
Only for race 0x12e, first thing in the call. ring = model#(0xe33); n = trunc32(C+0x17c ×
0.5); s = 3 / float(ring's size z); drop = trunc64((C+0x8c × 0.5) × 65536).

For k = 0 … n − 1: M = identity; translate(float(C+0xb10 + camX) × DOT, float(C+0xb18 +
camY) × DOT, float((C+0xb20 − drop) + k × 0x20000) × DOT); rotZ(float(k × 90) + C+0xb3c);
scale(s, uniform); translate(sx × −0.5, sy × −0.5, 0); draw.

Then one more: z = ((n + n as a 32-bit integer, widened) × 65536 − drop) + C+0xb20;
M = identity; translate(x and y as above, float(z) × DOT); rotZ(float(n × 90) + C+0xb3c);
scale(s × (C+0x17c × 0.5 − float(n)), uniform); translate(sx × −0.5, sy × −0.5, 0); draw.

Ring draws use the tint (1, 1, 1, 1) (not the call's) and the numbers alpha, 0, 0, 0, 0.

### 5.2 Board, raft and eagle (after the body frame)
cs2 = chest scale + chest scale. h(v) = (0, 0, (C+0x8c × v) / C+0x84) + Cp.

| When | Model | Placement |
|---|---|---|
| ability 0x78 | model(+0x1d8) | body frame; translate(h(8)); rotX(−A+0x74); scale(cs2, uniform); centre. |
| ability 0x79 | model(+0x1d8) | body frame; translate(h(−9)); rotX(−A+0x68); rotZ(−A+0x7c); scale(cs2, uniform); translate(1, 2, 0); translate(sx × −0.5, sy × −0.5, 0). |
| ability 0x9c or 0x9d | model#(0xe35) | body frame; translate(h(−10.5)); rotX(−A+0x68); rotZ(−A+0x7c); scale(cs2, uniform); translate(sx × −0.5, sy × −0.5, 0). |

Draw numbers: alpha, x9, 0, 0, depth.

### 5.3 The piece over the head (ability 0x8a)
After the head: model#(0xdeb). M = body frame; translate(Hp.x + A+0x8c, Hp.y + A+0x90,
(Hp.z + A+0x94) − bob × 0.5); translate(0, 0, −3); rotY(−A+0x6c); rotZ(A+0xa0 − A+0x70);
rotY(A+0x9c); translate(0, 0, 3); scale(4, uniform); translate(sx × −0.5, sy × −0.5, 0).
Draw: glow 0, fourth C+0x994.

### 5.4 The glove grip
Right after the left hand is drawn (so not when hide bit 0x10 is set or there is no hand
sprite), if C+0x128 bit 0 is set and model#(0xd0a) exists: M = hands frame; translate(Lr.x,
Lr.y + 2, Lr.z − 2); scale(0.8, uniform); translate(sx × −0.5, 1 − sy × 0.5, float(2 − size
z as integer)). Draw with tint (1, 1, 1, 1) and numbers alpha, 1, 0, C+0x994, depth.

---

## 6. Weapon tips and trails

A holds four trails of 16 points each (entry 0 is the newest):

| Offset | Trail |
|---|---|
| A+0x5bc + 12 i | right weapon tip T(i) |
| A+0x67c + 12 i | right weapon base B(i) |
| A+0x73c + 12 i | left weapon tip LT(i) |
| A+0x7fc + 12 i | left weapon base LB(i) |

### 6.1 Entry 0
After a weapon's matrix M is built (§3.2): base = point(M, (sx × 0.5, sy × 0.5, 0)) and
tip = point(M, (sx × 0.5, sy × 0.5, sz)) go to B(0), T(0) for the right weapon and LB(0),
LT(0) for the left. A missing weapon leaves its entry 0 unchanged.

### 6.2 Relative to the creature
After the shield: back = (float(−(C+0xb10 + A+0x8d0)) × DOT, float(−(C+0xb18 + A+0x8d8)) ×
DOT, 0) (64-bit sums, negated, then converted). LB(0), LT(0), B(0), T(0) each += back.

### 6.3 Moving the trails
The first case that applies (t = A+0x08, c6c = C+0x6c):

1. **Spin**: state 0x4b, t > float(d1) and float(d1 + d2) > t. M = identity;
   translate(float(C+0xb10 + camX) × DOT, float(C+0xb18 + camY) × DOT, float(z) × DOT) with
   z as for the base matrix; rotZ(C+0xb3c). outer = ((t − d1) × 20) / float(d1 + d2);
   inner = outer × 0.5. For i = 0 … 15: a = float(double(i) × π × 0.0625 − π/2); B(i) =
   point(M, inner × (sin(a), cos(a), 0)) + back; T(i) = point(M, outer × (sin(a), cos(a),
   0)) + back. The left trail is not touched.
2. **Swing**: the state is one of 1, 2, 3, 8, 0xa, 0xc, 0xe, 0x1d, 0x1e, 0x1f, 0x20, 0x21,
   0x24, 0x31, 0x32 or 0x41, t > float(d1) and float(T) > t. s = ease(dt, 0.05). For i = 0 …
   14, in that order: B(i+1), T(i+1), LB(i+1), LT(i+1) each eased toward entry i of its
   trail by s (value = s × (target − value) + value, per component; entry i already moved
   this call).
3. **Left only**: the state is 0xf, 0x11 or 0x13, and C+0x6c > float(d1). s = ease(dt,
   0.05). LT(0) = 0 × (LT(0) − LB(0)) + LT(0). For i = 0 … 14: B(i+1) = B(0); T(i+1) =
   B(0); LB(i+1) and LT(i+1) eased toward entry i.
4. **Right only**: the state is 0x10, 0x12, 0x14 or 0x57, C+0x6c > float(d1) and
   float(d1 + d2 + d3 div 2) > C+0x6c. s = ease(dt, 0.075). B(0) = 0.5 × T(0) + 0.5 × B(0);
   T(0) = 0 × (T(0) − B(0)) + T(0). For i = 0 … 14: B(i+1) and T(i+1) eased toward entry i;
   LB(i+1) = LB(0); LT(i+1) = LB(0).
5. **Otherwise**: every entry 1 … 15 of each trail = its entry 0.

Finally: if the right weapon sprite is none, B(1 … 15) = B(0) and T(1 … 15) = T(0); if the
left is none, LB(1 … 15) = LB(0) and LT(1 … 15) = LT(0).

(The "0 ×" terms are kept because they turn an infinite or NaN difference into NaN.)

---

## 7. The sparks of state 0x61

A+0x8e0 is a doubly linked list (A+0x8e0 points at the head node; A+0x8e8 is the element
count). Each node holds a position, three floats, at node+0x10. The step runs after the
hidden parts and before the base matrix, when the state is 0x61 or the list is not empty.

1. If the list is empty (so the state is 0x61): six times, r1 = rand(), r2 = rand(); append
   a node with position ((float(r2) × 20) / 32767 − 10, (float(r1) × 20) / 32767 − 10, 20).
2. Walk the list from the first node, with a counter j that is 1 for the first node, 2 for
   the second, and so on:
   * if the state is 0x61: a = xorshift(j + 0x3d6) mod 100 (unsigned); b = xorshift(j) mod
     100; target = ((float(b) / 100 − 0.5) × C+0x84, (float(a) / 100 − 0.5) × C+0x88,
     ((float(a) × 0.25) / 100 + 0.75) × C+0x88); the position is eased toward the target by
     ease(dt, 0.0025);
   * otherwise: the position += ((normalised position) × 0.05) × dt (per component); then,
     if its length² > 400, the **whole list is cleared** and the walk stops (this node and
     the rest are not drawn).
   * Draw it: model = model#(0xe2f) if the bit 0 of ((trunc32(t) + xorshift(j)) div 200,
     unsigned 32-bit) is set, else model#(0xe30). M = identity; translate(float(x) × DOT,
     float(y) × DOT, float(z) × DOT) with x = (C+0xb10 + trunc64(p.x × 65536)) + camX,
     y = (C+0xb18 + trunc64(p.y × 65536)) + camY, z = ((C+0xb20 + trunc64(p.z × 65536)) −
     trunc64(C+0x990 × 65536)) + trunc64(A+0xc8 × 65536) (p the position);
     rotZ((float(xorshift(j + 0x2667) mod 1000) / 1000) × 360); scale(0.05, uniform);
     centre. Tint (1, 1, 1, 1), numbers 1, 0, 0, 0, 0.

The twelve rand() draws come from the game's shared generator; they happen only when the
list is empty at the start of a call in state 0x61.

---

## 8. Small tables

### 8.1 Item glow (0x10a420)
For an item of category c (+0x00), subtype k (+0x04), material m (+0x1c), the first rule
that applies:

| Rule | Glow |
|---|---|
| c = 0 | 0 |
| m is 1, 0xb, 0xc, 0xd or 0x17 | 1 |
| m is 0x1d or 4 | 0.5 |
| m is 0x21 or 0x23 | 0.25 |
| c = 0x18 and k is 4, 5, 7, 8, 0xa or 0x12 | 1 |
| c = 0x17 | 1 |
| otherwise | 0 |

### 8.2 Item size (0x10a330)
| Rule | Size |
|---|---|
| c = 0xb, k = 0x24 | 0.05 |
| c = 0xb, k = 0 or 0x20 … 0x23 | 0.15 |
| c = 0xc | 0.04 |
| c = 0x18, k = 1 or 2 | 0.15 |
| c = 0xb, k = 0x2a | 0.15 |
| otherwise | 1/14 (0.0714285746) |

### 8.3 Mount offsets (0x65040)
The rider's extra shift by the race of the creature ridden (state 0x3f). x is always 0.

| Race | (y, z) |
|---|---|
| 0x125, 0x126 | (0, −0.4) |
| 0x1a, 0x1e | (0, −0.9) |
| 0x21 | (0, −0.2) |
| 0x4a, 0xc8, 0xc9 | (0, −0.3) |
| 0x4b, 0x117 | (0, 0.1) |
| 0x97 | (0, −0.35) |
| 0xd2 | (−0.3, −0.3) |
| 0xd3 | (0, −0.6) |
| 0xd5 | (0, 0.25) |
| 0xd7, 0x127 | (0, 0.4) |
| 0x106 | (0, −1) |
| 0x107 | (0, −2) |
| 0x10e | (0, −1.6) |
| any other | (0, 0) |

(The values are the floats nearest the decimals: −0.4 is 0xbecccccd, −0.9 0xbf666666,
−0.35 0xbeb33333, −0.6 0xbf19999a, 0.4 0x3ecccccd, −1.6 0xbfcccccd.)

---

## 9. Draw calls

The routine does not draw itself; it calls the renderer. CubeVulkan records these calls
(`nativepose::Draw`) and renders them; they are not compared by the oracle.

* **Draw** (0xee2f0): context (argument 2), model, matrix, pointer to the tint (4 floats),
  then five floats: n1, n2, n3, n4, n5. For the body parts n1 = alpha (argument 6), n2 = x9,
  n3 = the glow, n4 = the fourth number (usually C+0x994), n5 = depth (argument 11).
* **Mirrored draw** (0xee990): the same arguments; used for the right hand and right foot,
  whose matrices have a negative x scale.
* **Weapon spirits** (0xeec50): context, the item (C+0x638 or C+0x6d8), model#(0xa46), the
  weapon matrix, the tint pointer, then 0, alpha, x9, the weapon's glow, the weapon's
  fourth number, depth. Issued right after each weapon's draw. It draws the spirit cubes
  set into the item (§9.1); a weapon without spirits draws nothing more.

### 9.1 The spirit cubes (0xeec50)

Model #0xa46 is `cube.cub`, a single white voxel (1 × 1 × 1). An item record carries up to
32 **spirits** at +0x1d, four bytes each: x, y, z (signed bytes) and a material (unsigned
byte); the count is the byte at +0x9d (`conversation-script.md` lists the same layout).
0xeec50 draws one cube per spirit, in order i = 0 … count − 1; with a count of 0 it draws
nothing.

For spirit i (bytes at +0x1d + 4i):

1. **Matrix**: a copy of the weapon matrix M (the one just used to draw the weapon, in the
   weapon model's own voxel units, before any centring of the cube) whose translation
   column is moved by (x, y, z): for each row r, M'[12+r] = M[12+r] + ((M[r] × x +
   M[4+r] × y) + M[8+r] × z), with x, y, z converted exactly from the signed bytes. The first three columns are unchanged, so the
   cube is one weapon voxel in size and sits with its corner on voxel (x, y, z) of the
   weapon model: it reads as an extra voxel set into the blade.
2. **Colour**: materialColour(material, tint, s) of 0x109960 with s the call's first number
   (always 0 from the pose):

   | Material | Colour (r, g, b, a) |
   |---|---|
   | 1 | (0.7, 0.7, 0.7, 1) × tint |
   | 2 | (0.4, 0.3, 0.2, 1) × tint |
   | 3 | (0.5, 0.5, 0.5, 1) × tint |
   | 5 | (0.1, 0.1, 0.1, 1) × tint |
   | 7 | (0.9, 0.9, 0.9, 1) × tint |
   | 0xb | (1.0, 0.7, 0.2, 1) × tint |
   | 0xc | (0.8, 0.8, 0.85, 1) × tint |
   | 0x80 | (1.0, 0.5, 0.1, s + tint.a) × (0.5s + 1) |
   | 0x81 | (0.3, 1.0, 0.5, s + tint.a) × (0.5s + 1) |
   | 0x82 | (0.3, 0.5, 1.0, s + tint.a) × (0.5s + 1) |
   | 0x83 | (0.8, 0.8, 1.0, s + tint.a) × (0.5s + 1) |
   | any other | (0.5, 0.5, 0.5, 1) × tint |

   "× tint" multiplies each channel by the tint's channel (alpha by tint.a). For the four
   spirit materials the tint's colour is **not** applied, only its alpha; with s = 0 the
   factor (0.5s + 1) is 1. (The same routine colours the hit sparks, `creature-hit.md`
   §2.9, and the spirit materials 0x80 … 0x83 are the ones whose hit effects that section
   lists.)
3. **Draw** (0xee2f0): cube model, M', that colour, then alpha, x9, glow, fourth, depth as
   the weapon was drawn.

So a weapon with spirits shows coloured voxels (orange, green, blue, pale violet for
0x80 … 0x83) at fixed places on it, steady (no animation, no particles, no trail), as long
as the weapon is drawn. A cube drawn white at the weapon's origin is not something the game does:
it would take a spirit with an unlisted material at (0, 0, 0) and a white tint, and then it
would be grey (0.5).

The calls in order, with their numbers when they differ from (alpha, x9, glow, C+0x994,
depth):

| # | What | Kind | Tint, numbers |
|---|---|---|---|
| 1 | race 0x12e rings | draw | (1,1,1,1); alpha, 0, 0, 0, 0 |
| 2 | sparks (state 0x61) | draw | (1,1,1,1); 1, 0, 0, 0, 0 |
| 3 | board / raft / eagle | draw | alpha, x9, 0, 0, depth |
| 4 | items in use (§4.1) | draw | alpha, x9, glow(+0x1d8), 0, depth |
| 5 | chest | draw | glow 1 if glowAll else glow(+0x3b8) |
| 6 | neck item | draw | glow 1 if glowAll else glow(+0x318) |
| 7 | shoulders item | draw | glow 1 if glowAll else glow(+0x598) |
| 8 | tail | draw | glow 0 |
| 9 | head | draw | glow 0 |
| 10 | head piece (0x8a) | draw | glow 0 |
| 11 | hair | draw | hair colour; alpha, 1/0/−1, 0, C+0x994, depth + 0.0001 |
| 12 | head item (+0x278) | draw | glow(+0x278) |
| 13 | left arm part, right arm part | draw ×2 | glow 0 |
| 14 | between the hands (§4.2) | draw | glow 0 |
| 15 | quivers (left item, then right) | draw | glow 0 |
| 16 | right weapon | draw + effect | glow 1 if glowAll else glow(+0x6d8) |
| 17 | left weapon | draw + effect | glow 1 if glowAll else glow(+0x638); fourth w |
| 18 | shield on the back | draw | glow 0 |
| 19 | left hand | draw | glow(+0x4f8) |
| 20 | glove grip | draw | (1,1,1,1); alpha, 1, 0, C+0x994, depth |
| 21 | right hand | mirrored | glow 0 |
| 22 | left wing, right wing | draw ×2 | glow 0 |
| 23 | left foot | draw | glow(+0x458) |
| 24 | right foot | mirrored | glow(+0x458) |

The hair's n2: 1 if C+0x80 bit 10 is set, −1 if bit 22 is set (bit 22 wins), else 0
(`creature-pose.md` §8.5).

---

## Open questions

1. **What the extra models are.** Model ids 0x400 (quiver), 0x401 (an arrow or bolt in
   the hand), 0xa46 (the weapon effect's model), 0xb04, 0xd0a (a glove grip), 0xdeb, 0xe21,
   0xe2f / 0xe30 (spark frames), 0xe33 (ring), 0xe35 (an eagle, used by abilities 0x9c and
   0x9d) are named from how they are placed; only the ids are certain (0xa46 is `cube.cub`,
   §9.1). The renderer routines 0xee2f0 and 0xee990 are not specified here; in particular what the
   five numbers do in the renderer (alpha, an outline or flag value, glow, a fade, depth
   bias) is a reading of how they are filled, not of the renderer.
2. **The previous hands frame.** §4.1 uses A+0x178 as the previous call left it. For the
   first call on a fresh A this is whatever A+0x178 held (zeros on a new record).
3. **Sparks are not covered by the oracle** (the list is outside the compared ranges, and
   draws are not recorded); their arithmetic is from the assembly only. Their rand() draws
   consume the shared generator, which other recorded routines depend on.
4. **The trail scalars written as "0 ×"** (§6.3 cases 3 and 4) are from the assembly; they
   look like a leftover of a lengthening factor that is 0 in this build.
5. **Spirit cubes on screen (§9.1)** are from the assembly only; no recording holds a weapon
   with spirits. Oracle (ZoneExtractor request, TEST character): hook 0xeec50 and log, per
   call, the item's 0xa0 bytes (so +0x1d … +0x9d), the matrix (16 floats at argument 4), the
   tint (argument 5) and the next *count* calls to 0xee2f0 (model pointer, matrix, colour,
   the five numbers). In game: hold a weapon whose +0x9d is above 0 (a looted or crafted
   weapon with spirits; check the byte in the log), stand still, then swing; also log one
   frame with a spirit-less weapon (expect no 0xee2f0 calls from 0xeec50). Compare M' and
   the colours with §9.1, and take a screenshot to see where the cubes sit on the blade.
