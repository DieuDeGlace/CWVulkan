# Creature pose: the targets of every pose state (0x17ab0)

Part of the creature pose specification; read `creature-pose.md` first for the
conventions, the record and the names used here. This file lists what the routine does for
each value of the pose state A+0x0c (its jump table at 0x2af94, 0x66 entries for states
1 … 0x66). Each state's entry runs once per call, after gliding (`creature-pose.md` §4.6)
and before the common steps (§5.4).

## How to read the tables

**Targets** (all start each call at 0, except as gliding set them):

| Name | Field it eases | Name | Field it eases |
|---|---|---|---|
| RT | right hand turn A+0x44 | LT | left hand turn A+0x38 |
| RS | right hand shift A+0x2c | LS | left hand shift A+0x20 |
| BT | body turn A+0x68 | WA | whole-body angles A+0x74 |
| BS | body shift A+0x80 | HS | head shift A+0x8c |
| HA | head angles A+0x98 | TA.x, TA.z | tail angles A+0xa4, A+0xac |
| ST | stance A+0xb0 | WS | whole-body shift A+0xc0 |
| KS | foot kick A+0xcc | DR | drop A+0xd0 |

**Working values** a state may change (`creature-pose.md` §4): the rest places **Rr**, **Lr**
(hands), **Rm**, **Lm** (arm parts), **Fr**, **Fl** (feet), **Tp** (tail), **Hp** (head);
**Cp** is the chest place (read only); **splay**, **turnOut**, **heldScale**, **rate**.

**Time values**: t = A+0x08 (the state clock); c6c = C+0x6c (the ability clock); d1, d2, d3,
T (the current ability's wind-up, middle, recovery and total duration, integers), U (the
held item's use time). "X > t" with X an integer expression means float(X) > t; the
integer expression is computed first ("d1 + d2 div 2" is the integer d1 + (d2 div 2)).
Rows are tested top to bottom and only the first that holds applies.

**Notation.** "RT = (a, b, c)" sets all three components; "RT.x = a" one component;
"RS.y += a" adds a to the current value. Assignments are done in the order written; a later
one sees the earlier. "(u, v, w)" with expressions are evaluated x, then y, then z. "k ×
(a, b, c) + kk × (d, e, f)" is component-wise: (k × a + kk × d, …). sin and cos are the
UCRT's sinf and cosf of the float argument. "Rate r" sets the easing rate; rows without a
rate keep the default 0.01. "Nothing" means no target is set (rate 0.01).

**Shared blocks** used by several states:

* **Grip** (both hands on one weapon): LS = RS; LT = RT; Lr = Rr; Lr.x += −0.2;
  Lr.y += −0.2; Lr.z += −1.
* **One-weapon grip G** (only when one weapon, `creature-pose.md` §3): Rr.y += 2;
  Rr.z += 3; Lr = Rr; Lr.x += −3; Lr.z += −3; RT.x = 0; RT.y = 20; Lm.y += 10;
  Lm.z += −4; LT = RT.
* **Stand-up S**: BT.z = 20; RT = (−120, −90, 30); LT = (−80, 40, 60); LS.y += −4;
  LS.z += 2; then G when one weapon. **S′** is the same except that LT = (0, 0, 90) when the
  left item kind is 0xd.
* **Ended**: ST = (0, 0, 0) and the state A+0x0c = 0. (Then §5.4 step 2 sets t = 0.)
* **Circle**(b): BT.z = 0; WA.z = 0; RT = (60, −20, 20); u = t × 0.01; RS = (0, sin(u) + 2,
  cos(u) + 4); LT = (60, 20, −20); v = float(double(t × 0.01) + π/2); LS = (0, sin(v) + 2,
  cos(v) + 4); BS.z += sin(t × b) × 0.5 (when b is given).
* **Bob**: WS.z = float(double(cos(t × 0.003)) × 0.2 + 0.800000011920929); KS = −30;
  Fr.y += 4; Fl.y += 4.

A "sprite" change writes A's sprite slot: **right weapon** A+0x118, **left weapon**
A+0x120, head A+0xd8, hands A+0xf8, feet A+0x110, tail A+0x108. "#0x401" is model#(0x401).
"Category 3, kind not 0x16/0x17" tests the item's category byte = 3 and its kind minus
0x16 > 1 (unsigned).

---

## States 1 – 9

### State 1 — a charged blow (raised over 500 ms, brought down)
Let **two** = holdsTwoHander.

| When | Sets |
|---|---|
| 500 > t | k = min(1, t / 500). If two: RS.y = 0; RS.x = −Rr.x; RT.x = k × 120; RS.z = 14; LS.y = 0; LS.z = 12; LS.x = 2 − LS.x. Else: RS = (1, 0, 14); LS = (−1, 0, 14); LT.x = −100; RT.x = −100. Then KS = k × −50; BT.x = k × −40; WA.x = BT.x; if the ability is 0x32: WA.x = WA.x − min(360, (t / 500) × 360). Finally **C+0x998 = 0**. |
| float(T) > t | BT.x = −20; RT.x = −70; WA.x = (1 − min(1, ((t − 500) / 500) × 2)) × −60. If two: RS.x = −Rr.x; RS.y = 4; RS.z = 0; LS.y = 4; LS.z = 0; LS.x = 2 − LS.x. Else: RS = (1, 3, 8); LS = (−1, 3, 8); LT.x = −150; RT.x = −150. Rate 0.1. |
| two | Rr.x += −8; Rr.y += 4; Lr = Rr; Lr.x += −2; Lr.z += −1; Lm.y += 10; Lm.z += −4; RT.z = 20; RT.x = −60. |
| otherwise | RS = (1, 3, 2); LS = (−1, 3, 2); LT.x = −250; RT.x = −250. |

Afterwards in every row: if the left item's category is 3 and its kind is 0xd, LT = (−250,
−90, 0). ("× 2" in the second row is x + x.) Writing C+0x998 = 0 affects the later reads of
the stride in this call: the shoulders item height and the wing flap (`creature-pose.md`
§8.3, §8.9).

### State 2 — a two-handed swing
| When | Sets |
|---|---|
| d1 > t | k = min(1, (t / d1) × 1.5); BT.x = 10; BT.y = 10; WA.x = k × 10 + 5; BT.z = −60; RT = k × (60, 0, 60) + (1 − k) × (90, 0, 90); RS = (0, 0, k × 14); LS = RS; Lr = Rr; Lm.y += 10; Lr.x += 2; Lm.z += −4. Rate 0.01. |
| T > t | k = min(1, (t − d1) / d2); WA.x = −10; BT.y = 10; BT.z = 60; RT = (−70, 0, −60); s = sin(float(double(k) × π × 0.5)) × 2; RS = (0, s, 0); LS = RS; Lr = Rr; Lr.x += 2; Lr.y += −1. Rate 0.04. |
| otherwise | **Ended** (state only: ST not touched); Rr.y += 2; Rr.z += 3; Lr = Rr; Lr.x += −3; Lr.z += −3; RT.x = 0; RT.y = 20; Lm.y += 10; Lm.z += −4. |

Afterwards: LT = RT.

### State 3 — a sweep with the right arm
| When | Sets |
|---|---|
| d1 > t | WA.z = 0; RT = (−90, 0, 0); RS = (−5, 8, 0). Rate 0.005. |
| d1 + d2 div 2 > t | WA = (10, 0, −10); BT = (30, 0, −30); RT = (0, 0, 90); RS = (−8, −4, 20). Rate 0.005. |
| T > t | WA.z = −30; BT = (0, 20, −60); RT = (70, 0, 90); RS = (5, −2, 0). Rate 0.02. |
| otherwise | The state A+0x0c = 0; Lr = Rr; Lr.x += −2; Lr.z += −1; RT.y = 90; Lm.y += 10; Lm.z += −4; BT.z = −80. |

In the first three rows, afterwards: LS = RS; Lr = Rr; Lr.x += 2; Lm.y += 10; Lm.z += −4.
In every row, finally: LT = RT.

### State 4 — a lunge
h = d1 div 2.

| When | Sets |
|---|---|
| float(h) > t | k = min(1, t / float(h)); WA.x = k × 5; BT.y = k × 10; BT.z = k × −20; RT = k × (60, 60, 60) + (1 − k) × (0, 120, 0); Lr = Rr. Rate 0.005. |
| class 1 and specialisation 1 | RS = (2, 7, 4); RT = (0, −60, 0); LS = RS; Lr = Rr. |
| otherwise | k = min(1, ((t − float(h)) / float(h)) × 1.5); WA.x = 0; BT = (20, 0, 0); RT = k × (30, 0, 0) + (1 − k) × (90, 0, 90); Rr.z += cos(t × 0.5) × 0.5; RS = (−6, 0, k × 16); LS = RS; Lr = Rr. Rate 0.005. |

Afterwards: Lr.x += 2; Lm.y += 10; Lm.z += −4; LT = RT.

### State 5 — both hands up in front, bobbing
RT = (−90, 0, −40); LT = (−90, 0, 40); Rr.z += cos(t × 0.5) × 0.5; Lr.z += cos(t × 0.5) ×
0.5; RS = (1, −7, 0); LS = (−1, −7, 0); BT.z = −40. Rate 0.005.

### State 6 — weapon at the ready
| When | Sets |
|---|---|
| holdsTwoHander | RT = (−40, 60, −90); RS = (−5, 8, 4); BT.z = −55; BT.x = 10; BT.y = 30; **Grip**. |
| specialisation 1 | RT = (−50, −60, 40); LT = (−90, 90, −50); Rr.z += cos(t × 0.5) × 0.5; Lr.z += cos(t × 0.5) × 0.5; RS = (−3, 7, 3); LS = (−2, 2, 0); BT.z = −20. Rate 0.005. |
| otherwise | RT = (−90, 0, −40); LT = (−90, 90, −40); Rr.z += cos(t × 0.5) × 0.5; Lr.z += cos(t × 0.5) × 0.5; RS = (1, −7, 0); LS = (−1, 0, 1); BT.z = −40. Rate 0.005. |

### State 7 — changing stance
h = T div 2.

| When | Sets |
|---|---|
| float(h) > t | ST = the stance at C+0x168. Rate 0.005. |
| float(T) > t | u = (t − float(h)) / float(h); k = min(1, ((u × u) × u) × 15); ST = (1 − k) × C+0x168 + k × C+0x15c. Rate 0.03. |
| otherwise | ST = C+0x15c. |

Afterwards: RT = (0, 90, 0); if holdsTwoHander: Lr = Rr; Lr.x += 2; Lm.y += 10; Lm.z += −4.

### State 8 — drawing a weapon (by the clock t)
With a left weapon sprite:

| When | Sets |
|---|---|
| 100 > t | BT.z = 80; LT = (0, 0, 90); RT = (−45, 45, 0). |
| 500 > t | BT.z = −90; LT = (0, 60, 90); LS.z += 5; RT = (−90, 0, 45). |
| otherwise | BT.z = −30; LT = (0, 100, 60); RT = (−45, 45, 0). |

Without:

| When | Sets |
|---|---|
| 100 > t | BT.z = −80; RT = (0, 0, 90). |
| 500 > t | RT = (0, 60, −90); BT.z = 90; LS.x += 2; RS.x += 2; RS.y += 3; RT.z += 5; RS.z += 3; LS.y += 3; LS.z += 3. |
| otherwise | Rr.z += 3; Rr.y += 2; Lr = Rr; Lr.x += −3; Lr.z += −3; RT.x = 0; Lm.y += 10; Lm.z += −4; RT.y = 20. |

then (without a left weapon, every row): Lr = Rr; Lr.x += −3; Lr.z += −3; LT = RT.

### State 9 — a weapon from the back
LT = (0, 0, 90). If d1 > t: BT.z = 80; RT = (−45, 45, 0); and if the ability is 0x32:
WA.z = (t / −300) × 360. Otherwise: BT.z = −90; RT = (−90, 0, 45).

---

## States 0xa – 0x1f

### State 0xa — a weapon from the back, swung, then back to standing
| When | Sets |
|---|---|
| float(d1) ≤ t and float(T) > t | BT.z = −90; LT = (0, 0, 90); RT = (−90, 0, 45); Lr.x += sin((t − 600) × 0.1). Rate 0.03. |
| float(d1) ≤ t | **Ended**; unless the race is 0x60 or 0x57: **S′**. |
| otherwise | as state 9. |

### State 0xb
Unless C+0x80 bit 2 (value 4) is set: BT.z = 60; RT = (0, −45, −60); LT = (−45, −45, 0).

### States 0xf and 0x10 — a weapon swing (0x10 forehand, 0xf backhand)
First: ST = C+0x15c. Then if ((C+0xad8 div 2) mod 3), C's signed remainder, equals 1 for
0x10 or 2 for 0xf (every third swing spins): WA.z = clamp01((t − d1) / d2) × −360 and rate
0.03; otherwise WA.z = 0 (rate 0.01).

| When | Sets |
|---|---|
| d1 > t, 0x10 | BT.z = 90; RT = (0, −90, 0); LT = (−90, 0, 90); RS.y += 6; RS.z += 4. |
| d1 > t, 0xf | BT.z = 40; LT = (0, −90, 30); LS.y += −4; LS.z += 4; RT = (0, −90, −120); RS.y += −6. |
| T > t, 0x10 | BT.z = 20; LT = (−90, −60, 90); LS.y += −4; LS.z += 4; RT = (0, −90, −220); RS.y += −6; Fl.y += −4; WS.x = 0.1; Fr.y += 4; turnOut. |
| T > t, 0xf | WS.x = −0.1; BT.z = −90; LT = (−90, −90, −30); RT = (0, −90, −120); RS.y += −6. |
| otherwise | **Ended**; unless the race is 0x60 or 0x57: **S**. |

Afterwards: if the left item kind is 0xd, LT = (0, 0, 90). The rate is the one chosen
first.

### States 0x11 and 0x12 — a blow (0x11 forehand, 0x12 backhand)
| When | Sets |
|---|---|
| d1 > t, 0x12 | BT.z = −40; LT = (−90, −90, −30); RT = (−90, 0, 20); RS.y += −6. |
| d1 > t, 0x11 | BT.z = 40; RT = (−90, 90, 30); LT = (−90, 0, −20); LS.y += −6. |
| T > t, 0x12 | BT.z = 90; LT = (0, 0, 40) if the left item kind is 0xd, else (−40, 0, −30); LS = (−3, 0, 4); RT = (−90, −90, −90); RS = (3, −1, 0); turnOut. Rate 0.05. |
| T > t, 0x11 | BT.z = −90; RT = (−40, 0, 30); RS = (3, 0, 4); LT = (−90, 90, 90); LS = (−3, −1, 0); turnOut. Rate 0.05. |
| otherwise | **Ended**; unless the race is 0x60 or 0x57: **S′** for 0x12, **S** for 0x11. |

### States 0x13 and 0x14
| When | Sets |
|---|---|
| t > d1 and float(T) > t, 0x14 | BT.z = 45; WA.z = 45; RT = (0, −90, −90); RS = (8, 0, 4); LT = (0, −90, −30); LS = (−1, −4, −1); HA.x = −10; BS.y = 3. |
| t > d1 and float(T) > t, 0x13 | BT.z = −20; WA.z = −20; LT = (0, 90, 40); LS = (0, 8, 4); RT = (0, 90, 30); RS = (1, −4, −1); HA.x = −10; BS.y = 3. |
| otherwise | **Circle**(0.01) for 0x14, **Circle**(0.005) for 0x13. |

Rate 0.03 in every row.

### State 0x15 — drawing and loosing a bow
"The bow is in the right hand" when the right item kind is 6 or 7; otherwise it is taken to
be in the left hand.

Bow in the left hand:
1. If the left item kind is 7: LT.y = −90.
2. If float(T × 2) > t:
   * left kind 6: LS.x += −7; LS.y += 5; otherwise LS.x += −3; LS.y += 2;
   * LS.z += 4; RS.y += 3; LT.z = 70; RT.z = 70; RS.x += −10; RS.z += 3; BT.z += −70;
   * if float(T div 2) > t: LS.z += −2; LT.x = −5; RS = (−5, 1, 0); BT.z = −50; LT.z = 70;
     the right weapon sprite = none;
   * otherwise, if the left item is category 3, kind not 0x16/0x17: right weapon sprite =
     #0x401.
3. Otherwise: LS.x += −1; LT.x = −30; RT.x = −30; RT.y = 45; if the left kind is 7:
   LT.z = 50; if the left item is category 3, kind not 0x16/0x17: right weapon sprite =
   #0x401.

Bow in the right hand:
1. If the right kind is 7: RT.y = −90.
2. If float(T × 2) > t:
   * right kind 6: RS.x += 7; RS.y += 5; otherwise RS.y += 2; RS.x += 3;
   * RS.z += 4; LS.y += 3; BT.z = 70; RT.z = −70; LT.z = −70; LS.z += 3; LS.x += 10;
   * if float(T div 2) > t: RS.z += −2; RT.x = −5; LS = (5, 1, 0); BT.z = 50; RT.z = −70;
     the left weapon sprite = none;
   * otherwise, if the right item is category 3, kind not 0x16/0x17: left weapon sprite =
     #0x401.
3. Otherwise: RS.x += 1; RT.x = −30; LT.x = −30; LT.y = −45; if the right item is category
   3, kind not 0x16/0x17: left weapon sprite = #0x401.

### State 0x16 — aiming
Bow in the right hand (right kind 6 or 7):
RS.y += 3; LS.x += 10; RS.x += 4; RT.y = −90; BT.z = 60; RS.z += 2; RT.z = −60;
LT.z = −60; LS.y += 2; LS.z += 1; if the right item is category 3, kind not 0x16/0x17: left
weapon sprite = #0x401. If 300 > t: RS.z += −1; RT.x += 20; RT.y = −70.

Otherwise:
LS.y += 3; LS.x += −4; RS.x += −10; LT.y = −90; RS.y += 2; LS.z += 2; LT.z = 60;
RT.z = 60; RS.z += 1; BT.z = −60; if the left item is category 3, kind not 0x16/0x17: right
weapon sprite = #0x401. If 300 > t: LS.z += −1; LT.x += 20; LT.y = −110.

### States 0x17, 0x1a, 0x1c — hands together in front, circling
**Hands circling** (H): RT = (0, −45, −60); BT.z = −30; LT = (−45, −45, 0); RS.y += 4;
LS.y += 4; RS.x += sin(t × 0.04) × 0.5; RS.z += cos(t × 0.05) × 0.5; LS.x += sin(t ×
0.04) × 0.5; LS.z += cos(t × 0.05) × 0.5.

| State | When | Sets |
|---|---|---|
| 0x1c | always | H |
| 0x17 | d1 > t | H |
| 0x17 | otherwise | BT.z = 60; RT = (0, −45, −60); LT = (−45, −45, 0). |
| 0x1a | d1 > t | H |
| 0x1a | float(T) > t | BT.z = 90; RT = (−90, 0, −90); LT = (−45, −45, 0); RS = (3, 0, 1). Rate 0.03. |
| 0x1a | otherwise | RT = (−60, 0, 0); RS = (3, 0, 1). |

### States 0x18 and 0x19 — circling, then a push to the side (0x19 left, 0x18 right)
| When | Sets |
|---|---|
| d1 > t | RS.y += 4; LS.y += 4; RS.x += sin(t × 0.04) × 0.5; RS.z += cos(t × 0.05) × 0.5; LS.x += sin(t × 0.04) × 0.5; BT.z = −20 for 0x19, 20 for 0x18; LS.z += cos(t × 0.05) × 0.5. |
| T > t, 0x19 | turnOut; WS.x = −0.5; BT.z = 60; WA.z = 30; RT = (0, −45, −60); LT = (−45, −45, 0); RS = (2, 0, 0). |
| T > t, 0x18 | turnOut; BT.z = −60; WS.x = 0.5; WA.z = −30; LT = (0, 45, 60); RT = (−45, 45, 0); LS = (−2, 0, 0). |
| otherwise | nothing |

### State 0x1b — eating or drinking
**Bob**.

### State 0x1d — a spinning swing
| When | Sets |
|---|---|
| d1 > t | BT.z = −80; WA.z = 0; RT = (−90, 90, −90); LT = (20, 90, 90). |
| d1 + d2 > t | BT.z = −90; WA.z = ((t − d1) × 360) / d2; LT = (0, 100, 90); RT = (0, 90, 0). Rate 0.2. |
| otherwise | BT.z = 80; WA.z = 0; LT = (90, 90, 270); RT = (0, 90, 90). |

### State 0x1e — arms crossed, then flung wide
| When | Sets |
|---|---|
| d1 > t | RT = (−80, −90, 60); LT = (−80, 90, −60); RS.y += 3; RS.x += −5; LS.x += 5. |
| T > t | BT = (0, 0, −30); LS.y += −5; RS.y += −5; LT = (−90, 90, 110); RT = (−90, −90, −110); WS.y = −0.5. Rate 0.04. |
| otherwise | **Ended**; **S** (for every race). |

### State 0x1f — a two-handed overhead swing
| When | Sets |
|---|---|
| d1 > t | k = t / d1; RT = (−80, 90, −90); RS = (−10, 8, 8); BT.z = −45; WA.z = k × −20; WS.x = k × 0; if (C+0xad8 div 2) mod 3 = 2 and d1 + d2 div 2 > t: WA.z = WA.z + k × 360. Rate 0.04. |
| d1 + d2 + d3 > t | k = min(1, ((t − d1) × 1.2) / d2); kk = 1 − k; WS.x = kk × 0 − k × 0.8; RT = kk × (−90, 90, 48) + k × (−110, 60, 0); RS = kk × (−10, 8, 8) + k × (−12, 0, −1); HA.x = −10; BT.z = k × 90 − kk × 45; WA.z = k × 30; splay = k × 10. Rate 0.05. |
| otherwise | RS = (2, 0, 0); RT = (0, 0, 0); BT.z = −30. Rate 0.03. |

Afterwards: **Grip**.

---

## States 0x20 – 0x3f

### States 0x20, 0x31, 0x32 — two-handed swings
Let **first** = the state is 0x31. The blow row lasts while (first: d1 + d2 + d3; else: T)
> t.

| When | State | Sets |
|---|---|---|
| d1 > t | 0x31 | k = t / d1; RT = (−40, 60, −90); RS = (−5, 8, 4); BT.x = 10; BT.z = −55; BT.y = 30; WA.z = k × −20; WS.x = k × 0. |
| d1 > t | 0x20 | k = t / d1; RT = (0, −90, 0); RS = (−8, 6, 6); WS.x = −0; HA.x = −10; HA.z = 30; BT.z = 45; WA.z = 20. Rate 0.05. |
| d1 > t | 0x32 | k = t / d1; RT = (30, −60, 0); RS = (−14, 5, 5); WS.x = −0; HA.x = −10; HA.z = 30; BT.z = 45; WA.z = 20. |
| blow | 0x31 | k = min(1, ((t − d1) × 1.2) / d2); kk = 1 − k; splay = k; WS.x = kk × 0 − k × 0.8; RT = k × (−60, 60, 60) + kk × (−60, 60, 60); RS = k × (−12, 4, 2) + kk × (−10, 8, 4); BT.x = −20; HA.x = −10; BT.z = k × 50 − kk × 45; WA.z = k × 30; splay = splay × 10. Rate 0.05. |
| blow | 0x20, 0x32 | k and kk as above; HA.z = −30; WS.x = kk × −0 + k × 0.8; RT = k × (0, −90, −130) + kk × (0, −90, 20); RS = k × (0, 2, 0) + kk × (−8, 6, 6); BT.z = kk × 45 − k × 45; WA.x = k × −20; WA.z = k × −30; splay = k × 10. Rate 0.05. |
| otherwise | all | RS = (2, 0, 0); RT = (0, 0, 0); BT.z = −30. Rate 0.03. |

In the d1 > t rows, before the rate: if (C+0xad8 div 2) mod 3 = 2 and d1 + d2 div 2 > t:
WA.z = WA.z + k × 360 for 0x31, WA.z = WA.z − k × 360 for 0x20 and 0x32.
Afterwards in every row: **Grip**.

### State 0x21 — a two-handed swing from the side
| When | Sets |
|---|---|
| d1 > t | k = t / d1; RS = (2, 2, 14); RT = (50, 0, 0); BT.z = −45; WA.z = k × −20; WS.x = k × 0.5. Rate 0.04. |
| d1 + d2 + d3 > t | k = min(1, ((t − d1) + (t − d1)) / d2); kk = 1 − k; WA.x = kk × 20 − k × 10; RT = kk × (90, 0, 45) + k × (−90, 0, −45); RS = kk × (2, 2, 14) + k × (0, 9, 0); BT.z = k × 45 − kk × 45. Rate 0.05. |
| otherwise | RS = (2, 0, 0); RT = (0, 0, 0); BT.z = −30. Rate 0.03. |

Afterwards: **Grip**.

### State 0x22 — a two-handed thrust
| When | Sets |
|---|---|
| d1 > t | RS = (4, 2, 8); RT = (−90, −180, 90); BT.z = −90; WS.y = −1. Rate 0.02. |
| d1 + d2 > t | RS = (6, 0, 0); WS.y = 0.5; BT.z = 90; RT = (−90, 0, −90); turnOut. Rate 0.05. |
| otherwise | RS = (2, 0, 0); RT = (0, 0, 0); BT.z = −30. Rate 0.03. |

Afterwards: **Grip**.

### State 0x23
RT = (−90, −90, 80); BT.z = −80.

### State 0x24 — a two-handed blow from above
| When | Sets |
|---|---|
| d1 > t | RT = (−90, 0, 0); LT = (−90, 0, 40); RS = (1, −7, 0); LS = (−1, −7, 0); BT.z = −40; WS.y = −0.5; turnOut. Rate 0.005. |
| T > t | RT = (−90, −60, −30); LT = (−90, 60, −50); RS = (3, 5, 2); LS = (6, 8, 2); BT.z = 40; turnOut. Rate 0.05. |
| otherwise | **Ended**; unless the race is 0x60 or 0x57: **S**. |

### State 0x26 — a punch
| When | Sets |
|---|---|
| d1 > t | RT = (−90, −90, 80); BT.z = −80. |
| T > t | RT = (−90, 0, −90); Rr.x += 4; Fl.y += −4; BT.z = 90; Fr.y += 4; turnOut. Rate 0.05. |
| otherwise | The state A+0x0c = 0; Rr.y += 4; Rr.x += −8; Lm.y += 10; Lm.z += −4; RT.x = −60; RT.z = 20; BT.z = −20. |

Afterwards in every row: Lr = Rr; Lm.y += 10; Lr.x += 2; Lm.z += −4; LT = RT. (So in the
last row Lm.y gets + 10 twice, as (Lm.y + 10) + 10, and Lm.z gets −4 twice.)

### State 0x27 — a head butt to the side
| When | Sets |
|---|---|
| d1 + d2 div 2 > t | HA = (−30, 10, 90); BT = (0, 0, 60); HS = (−3, −2, 0). |
| T > t | HA = (30, −10, −30); BT = (10, 0, −20); HS = (3, 0, 2). |

### State 0x28 — a head butt forward
| When | Sets |
|---|---|
| d1 + d2 div 2 > t | HA = (−70, 0, 0); BT = (−10, 0, 0); HS = (0, −3, −1); LT.x = 20; RT.x = 20. |
| T > t | HA = (50, 0, 0); TA.x = −10; TA.z = 0; BT = (20, 0, 0); BS = (0, −3, 1); HS = (0, 0, 2); LT.x = −20; RT.x = −20. |

### State 0x29 — a tail sweep
| When | Sets |
|---|---|
| d1 > t | HA.z = −60; TA.z = −70; TA.x = 0; WA.z = 90; HA.x = 0; HA.y = 10. Rate 0.002. |
| T > t | WA.z = 200; TA.z = 70; TA.x = 0; HA = (0, −20, 60); HS = (−3, 0, −2). |

### State 0x2a — curling into a ball and back
**Gather**(k, H): Hp = k × ((H) − Hp) + Hp; Lr = k × (Cp − Lr) + Lr; Rr = k × (Cp − Rr)
+ Rr; Fl = k × (Cp − Fl) + Fl; Fr = k × (Cp − Fr) + Fr; Tp = k × (((0, 4, 4) + Cp) − Tp)
+ Tp. Each "k × (P − Q) + Q" is per component; H = (Cp.x − 0, Cp.y − 0, Cp.z − 2).

| When | Sets |
|---|---|
| d1 > t | Gather(t / d1, H). |
| d1 + d2 > t | the head, hands, feet and tail sprites = none; BS = (0, 0, −3). |
| T > t | Gather(1 − ((t − d1) − d2) / d3, H). |
| otherwise | nothing |

### State 0x2b
| When | Sets |
|---|---|
| d1 + d2 div 2 > t | HA.y = 10; HA.z = 30; BT = (−10, 0, 0); HS = (−3, 0, −1); LT.x = 20; RT.x = 20; HA.x = −10. Rate 0.005. |
| T > t | HA.x = 50; HA.y = 30; HA.z = 0; if d1 + d2 > t: HA.y = −30; RT.x = −60; TA.x = −20; TA.z = 0; BT = (30, 0, 0); BS = (0, −1, 2); HS = (0, 0, 2); LT.x = −20. Rate 0.005. |
| otherwise | nothing |

### State 0x2c
| When | Sets |
|---|---|
| d1 + d2 div 2 > t | BT.x = 50; HS.y = −6; TA.x = −50; TA.z = 0; HA = (−20, 0, 0); BT.y = 0; BT.z = 0; BS = (0, −2, 2); HS.x = 0; HS.z = −2; LT.x = 20; RT.x = 30. |
| T > t | BT = (0, 0, 0); HS = (0, 0, 1); LT.x = 0; RT.x = 0; HA.y = 0; HA.z = 0; HA.x = −10. Rate 0.02. |
| otherwise | nothing |

### States 0x2d and 0x50 — a bite or peck in four beats
**Open** O: TA.x = 30; TA.z = 10; BS.x = 0; BT.y = 0; BT.z = 0; BT.x = −20; HA.x = 10;
HA.y = 0; HS = (0, 1, 1); LT.x = 20; RT.x = 20; LS.z = 2; RS.z = 2.
**Lean** L: TA.x = −30; TA.z = −10; HS = (0, −2, 2); BS = (0, 1, 1); BT.z = 0; HA = (−30,
0, 0).

| When | Sets |
|---|---|
| d1 > t | HA.z = 0; BS.y = −1; BS.z = 0; O. |
| d1 + d2 div 3 > t | BT.x = 50; TA.x = −30; TA.z = 0; HA = (−30, 0, 0); BT.y = 0; BT.z = 0; BS = (0, 0, 1); HS = (0, −4, 2); LT.x = 60; RT.x = 70. |
| d1 + (d2 + d2) div 3 > t | BT.x = 40; BT.y = 0; LT.x = 70; RT.x = −30; RS = (0, 3, −2); L. |
| d1 + d2 > t | BT.x = 10; BT.y = 0; LT.x = −30; RT.x = −40; LS = (0, 3, −2); L. |
| T > t | HA.z = −10; BS.y = 1; BS.z = 0; O. |
| otherwise | nothing |

### State 0x2e — a somersault forward
| When | Sets |
|---|---|
| d1 > t | TA.x = −60; BT.x = −10; HA.x = 10. |
| d1 + d2 > t | LT.x = 80; RT.x = 80; HS.x = 0; HS.y = −3; HA.x = −60; WA.x = ((t − d1) / d2) × −360; HS.z = 0. |
| T > t | BT.x = 20; HA.x = −10; HS = (0, −2, 0). |

### State 0x2f — a roll to one side
| When | Sets |
|---|---|
| d1 > t | BT.z = −10; BT.y = 30; LT.y = −30; RT.y = −30; HA.z = 60. |
| d1 + d2 > t | LT.y = 80; RT.y = 80; WA.y = ((t − d1) / d2) × −360. |
| T > t | BT.y = −30; LT.y = 30; RT.y = 30. |

### State 0x30 — a roll to the other side
| When | Sets |
|---|---|
| d1 > t | HA.z = −60; BT.z = 10; BT.y = −30; LT.y = 30; RT.y = 30. |
| d1 + d2 > t | LT.y = −80; RT.y = −80; WA.y = ((t − d1) / d2) × 360. |
| T > t | BT.y = 30; LT.y = −30; RT.y = −30. |

### State 0x33 — arms up, a turn, then hands circling
First, always: RT = (60, −20, 20); RS = (0, 2, 4); LT = (60, 20, −20); LS = (0, 2, 4).

| When | Sets |
|---|---|
| d1 > t | BT.z = 50; LT.x = −90; LT.z = 90; RT.x = −90; LS = (−4, −4, −2); RS = (0, 4, 1). |
| d1 + d2 > t | LT.x = −90; LT.z = 90; RT.x = −90; k = clamp01(((t − d1) / d2) × 1.5); WA.z = k × −270; WA.y = k × −40; splay = k × 40; BS.z += 4; LS = (−4, −4, −2); RS = (0, 4, 1); HA.z = −60; HA.x = −30. Rate 0.03. |
| otherwise | **Circle**(0.005). |

### States 0x34 and 0x35 — using an item
State 0x35 first: WS.z = −0.14; BT.x = 10; LS = (0, 4, 0); RS = (0, 4, 0); HS = (0, 2, −2).
Then for both, if float(U) > c6c: HA.y = 0; HA.z = 0; BT = (10, 0, 0); WA = (10, 0, 0);
LS = (1, 7, 8); HA.x = cos(t × 0.02) × 5 + 45; RS = (−1, 7, 8); LT = (100, 0, 0);
RT = (100, 0, 0).

### State 0x36
BT = (−30, 0, 0); LS = (4, 6, 6); RS = (−4, 6, 6); BS.y = 3.

### State 0x37 — swimming
If the intent's length² ((ix × ix + iy × iy) + iz × iz) > 0: LS.y += 4; RS.y += 4;
WA.x = 20; BT.x = −10; HA.x = −20; LT.x = −20; RT.x = −20; if C+0x80 bit 0 is clear:
Fr.y += 4; Fl.y += 4; Fl.z += 3; Fr.z += 3; KS = −40.
Otherwise nothing, but see `creature-pose.md` §5.3 (K4).

### States 0x38 and 0x39 — sitting
| When | Sets |
|---|---|
| C+0x80 bit 0 set | WA = (0, 90, 0); WS.z = (C+0x84 − C+0x8c) × 0.5. |
| 0x38 | WA = (−90, 0, 0); HS = (0, −1, 0); HA.x = 20; LS = (0, 0, 3); RS = (0, 0, 3); WS.z = (C+0x88 − C+0x8c) × 0.5 − 0.15. |
| 0x39 | WA = (90, 0, 0); WS.z = (C+0x88 − C+0x8c) × 0.5 − 0.1. |

### State 0x3a
WA = (90, 0, 0); Fl.y += 4; Fr.y += 4; LS = (0, −4, 11); RS = (0, −4, 11);
WS.z = (C+0x88 − C+0x8c) × 0.5.

### States 0x3b and 0x3c — hanging and climbing
| State | Sets |
|---|---|
| 0x3b | KS = −30; Lr = (−1, −3, −3); Rr = (1, −3, −3); WS.z = 0.3; Fr.y += 4; Fl.y += 4. |
| 0x3c | KS = −30; WA.x = 178; Lr = (−4, 0, 12); Rr = (4, 0, 12); Fr.y += 4; Fl.y += 4; BT.y = cos(t × 0.005) × 10. |

### State 0x3d — arms raised
If the ability is not 0x76: WA.x += 20; BT.x += 30. Then LS = (−2, 0, 0); RS = (2, 0, 0);
LT = (−10, −10, 90); RT = (−10, 10, −90).

### State 0x3e — lying down
HS.x = 0; then:

| When | Sets |
|---|---|
| C+0x80 bit 0 set | HS.y += 1; BT.x = 30; LS = (−1, −7, 0); Fl.z += 2; Fr.z += 2; RS = (1, −7, 0); LT.x = −100; RT.x = −100; HS.z = −4. |
| otherwise | k = ((C+0xe8 + 5) / 140 + 0.075) × C+0x8c; HS.y = 2; HS.z = −2; BT.x = 10; LS = (0, 4, 0); WS.z = −k; RS = (0, 4, 0). |

(HS.y is −1 here when gliding set it, else 0.)

### States 0x3f and 0x40 — riding (0x3f a creature, 0x40 a boat or similar)
| State | Sets |
|---|---|
| 0x3f, riding creature M (argument 10) | WS.z = WS.z + (M+0x8c × 0.75 + liftL × 0.3); WS.y += −0.25; o = the mount offset of M's race (creature-pose-items.md §8.3); WS.x += o.x; WS.y += o.y; WS.z += o.z; if M's race is 0x16 or 0x17: WS.z += −0.75; **DR = −WS.z**. Then HS.y = 2; HS.z = −2. |
| 0x3f, no creature | HS.y = 2; HS.z = −2. |
| 0x40 | WS.z += 0.85 if C+0x5c bit 0 is set, else 1.25; WA.z = −50; HS.y = 1; HS.z = 0; HA = (0, 0, 30). |

Afterwards in both: RT.z = −60; HS.x = 0; RS = (−4, 6, −3); LS = (4, 6, −3); BT.x = 20;
LT = (60, 90, 60); RT.x = 60; RT.y = −90.

---

## States 0x41 – 0x66

### State 0x41 — a shield bash
| When | Sets |
|---|---|
| T > t | BT.z = 0; LT = (0, −90, 0); RT = (0, 90, 0). |
| otherwise | BT.z = 80; WA.z = 0; LT = (90, 90, 270); RT = (0, 90, 90). |

### States 0x42, 0x43, 0x44 — stirring, a push, arms flung up
"Stirring" holds for state 0x44 always, and for 0x42 and 0x43 while d1 > t.

| When | Sets |
|---|---|
| stirring | BT.x = 20; HA.x = 20; u = t × 0.005; LS = (cos(u) × 3 − 1, 5, sin(u) × 5 + 5); v = float(double(t × 0.005) + π); RS = (cos(v) × 3 + 1, 5, sin(v) × 5 + 5). |
| d1 + d2 > t | BT.x = −20; BT.z = 10; LS = (2, 4, 0); RS = (−2, 6, 0); LT = (0, 45, 0); RT = (0, −45, 0). |
| T > t | LS = (−3, −6, 0); RS = (3, −6, 0); LT = (−90, 0, 130); RT = (−90, 0, −130). Rate 0.03. |
| otherwise | nothing |

State 0x43 only, afterwards: if float(T) > t: **Bob**. (Its rate is 0.03 in the third row,
0.01 otherwise, as for the others.)

### State 0x45 — a flurry of alternating blows
| When | Sets |
|---|---|
| not (d1 + d2 + 400 > t) | nothing |
| d1 ≥ t does not hold and (n div 500) mod 2 ≠ 1, with n = trunc32(t) − d1 + 100 | Rate 0.03. If the lowest bit of (n div 1000) is set: LS = (2, −2, 12); RS = (0, 0, −5); BT = (−10, 0, 20); WA = (−5, 0, 10). Otherwise: LS = (0, 0, −5); RS = (−2, −2, 12); BT = (−10, 0, −20); WA = (−5, 0, −10). |
| otherwise | LS = (2, −2, 12); RS = (−2, −2, 12); BT.x = 20; WA.x = 10. |

(mod is C's signed remainder; the lowest bit is that of the two's-complement integer.)

### State 0x47 — by the ability clock c6c
| When | Sets |
|---|---|
| d1 > c6c | BT.z = 60; WA.z = 60; WA.x = −25; RS = (−6, −3, 0); LS = (−1, −4, −1); HA.x = −10. |
| T > c6c | BT.z = −30; WA.z = −30; WA.x = 10; RS = (3, −4, 0); LS = (0, 2, −1). |
| otherwise | nothing |

### State 0x48 — a throw
If C+0xba8 > 0:

| When | Sets |
|---|---|
| d1 > t | BT.z = −90; RS = (2, −2, 1); RT = (0, 10, 0); left weapon sprite = right weapon sprite. |
| d1 + 300 > t | BT.z = 10; LS = (−2, −2, 1); LT = (0, −10, 0); left weapon sprite = right weapon sprite; right weapon sprite = none. |
| T > t | BT.z = 90; both weapon sprites = none. |

then rate 0.03, and if t > d1 and float(T) > t: WA.z = ((t − d1) × 360) / 200; if WA.z >
700, WA.z = 700.

Otherwise (C+0xba8 not > 0): if d1 > t: BT.z = −90; RS = (2, −2, 1); RT = (0, 10, 0).
Else if float(T) > t: BT.z = 90; right weapon sprite = none.

### State 0x49 — holding something out
BT.z = C+0x158 × −90; RS = (2, −2, 1); RT = (0, 10, 0); Rr.z += cos(t × 0.5) × 0.5;
Lr.z += cos(t × 0.5) × 0.5; if C+0x158 > 0: left weapon sprite = right weapon sprite.

### State 0x4a — arms out in front, then apart
| When | Sets |
|---|---|
| d1 > t | BT.x = −20; LT = (−90, 0, 0); RT = (−90, 0, 0); RS = (1, −2, 0); LS = (−1, −2, 0). |
| otherwise | LT = (0, 45, 0); BT.x = 0; RT = (0, −45, 0); RS = (−2, 5, 0); LS = (2, 6, 0). |

### State 0x4b — a whirl
| When | Sets |
|---|---|
| d1 + d2 > t | WA.z = (t × −360) / float(d1 + d2); RT = (−90, −90, −90); RS = (4, 3, 0); BT.z = (t × −60) / float(d1 + d2); LT = (0, 0, 90). Rate 0.03. |
| otherwise | LT = (0, 0, 90); RT = (−60, −60, −60). |

### State 0x4c
If d1 > t: WA.x = −30. Else if d1 + d2 > t: WA.x = 120.

### State 0x4d — swinging model 0xb04 (by the ability clock c6c)
| When | Sets |
|---|---|
| d1 > c6c | RS = (−3, −2, 10); LS = (3, −2, 10); left weapon sprite = model#(0xb04); heldScale = heldScale × 1.5; BT.x = 40; WA.x = 10. |
| d1 + d2 > c6c | RS = (0, 5, 0); LS = (0, 5, 0); BT.x = −20; WA.x = −40. |
| otherwise | RS = (2, −1, 0); LS = (−2, −1, 0). |

### States 0x4e and 0x4f — eating and drinking
| When | Sets |
|---|---|
| 200 > t | BT.x = −20; HA.x = −10; LS = (4, 5, 0); RS = (−4, 5, 0). |
| 5000 > t | BT.x = 20; HA.x = 30; LS = (0, 0, 16); RS = (0, 0, 16); then, for 0x4f only, **Bob**. |
| otherwise | nothing |

(The held item is drawn while 5000 > t: creature-pose-items.md §4.1.)

### State 0x51
BT.x = 20; LS = (0, 0, 12); RS = (0, 0, 12).

### State 0x52
If 1000 > t: BT.x = −30; LS = (0, 8, 3); RS = (0, 8, 3).

### State 0x54
WS.z = C+0x8c × −0.65.

### State 0x55
| When | Sets |
|---|---|
| T > t | BT.x = −30; RT = (−90, 0, −60); LT = (−90, 0, 60); RS = (3, −3, 1); LS = (−3, −3, 1). |
| otherwise | RT = (−60, 0, 0); RS = (3, 0, 1). |

### State 0x56 — beating arms
If t > d1: s = sin(float(double(((t − d1) / d2) × 32) × π)); BT.z = −50 − s × 40;
RS.x = s × 8 − 3; RS.y = 7 − s × 5; LS.x = −5 − s × 8; LS.y = 5 − s × 5; WA.z = −20 − s ×
10; if the right item kind is not 4: RT.x = −90 and LT.x = −90; RT.z = −BT.z; LT.z = −BT.z;
WA.x = −50; rate 0.02. Otherwise nothing.

### States 0x57 and 0x58
"Circling" holds for 0x58 always, and for 0x57 while d1 > t.

| When | Sets |
|---|---|
| circling | BT.z = −60; WA.z = 0; RT = (60, 40, 20); u = t × 0.01; RS = (0, sin(u) + 2, cos(u)) (no + 4); Rr.z += cos(t × 0.5) × 0.5; LT = (60, 20, −20); v = float(double(t × 0.01) + π/2); LS = (0, sin(v) + 2, cos(v) + 4); BS.z += sin(t × 0.01) × 0.5. |
| T > t | BT.z = 45; WA.z = 45; RT = (0, −60, −90); RS = (8, 0, 4); LT = (0, −90, −30); LS = (−1, −4, −1); HA.x = −10; BS.y = 3; turnOut. |
| otherwise | **Circle** (without the BS.z term). |

Rate: 0.01 for 0x58; 0.03 for every row of 0x57.

### State 0x59
| When | Sets |
|---|---|
| d1 + d2 div 2 > t | BT = (−10, 0, 0); LS = (−3, 2, 0); RS = (3, 2, 0); HA.y = 0; HA.z = 0; HA.x = −10. Rate 0.005. |
| T > t | HA = (50, 0, 0); BT = (30, 0, 0); LS = (−4, 3, 5); RS = (4, 3, 5); LT.y = −10; RT.y = 10. Rate 0.005. |
| otherwise | nothing |

### States 0x5a – 0x60 and 0x62 — swaying while busy
Each applies only while 5000 > c6c (state 0x62: while 1000 > c6c); otherwise nothing.

| State | Sets |
|---|---|
| 0x5a | HA = (0, cos(t × 0.003) × 10, 0); u = t × 0.002; BT = (cos(u) × 5, sin(u) × 5, 0); WA = (0, 0, 0); LS = (12, 5, 5); RS = (7, 5, 5). |
| 0x5d | HA = (0, cos(t × 0.003) × 10, 0); z = sin(t × 0.003) × 15; u = t × 0.002; BT = (cos(u) × 5, sin(u) × 5, z); WA = (0, 0, 0); LS = (4, 9, 3.5); RS = (−4, 13, 3.5). |
| 0x5b | (common C) LS = (−5, 7, 4); RS = (cos(u) × 2 − 7, sin(u) × 2 + 10, 3). |
| 0x5e | (common C) RS = (cos(u) × 2 − 5, sin(u) × 2 + 10, 5); RT.y = sin(t × 0.01) × 40. |
| 0x5f | (common C) LS = (−10, 6, 1); RS = (cos(u) − 10, 9, sin(u) × 2 + 1). |
| 0x60 | (common C) RS = (−5, 10, 6). |
| 0x5c | HA = (0, cos(t × 0.002) × 5, 0); BT = (cos(t × 0.0015) × 2, sin(t × 0.002) × 3, sin(t × 0.0015) × 15); WA = (0, 0, 0); LS = (7, 21, 3); RS = (cos(t × 0.0075) × 0.5 + 5, 11, 4). |
| 0x62 | HA = (0, cos(t × 0.003) × 10, 0); BT = (10, 0, 20); RT.z = −20; WA = (0, 0, 0); LS = (−3, −2, 0); RS = (0, 8, 2.5). |

**Common C** (0x5b, 0x5e, 0x5f, 0x60), done first: HA = (0, cos(t × 0.002) × 5, 0);
BT = (cos(t × 0.0015) × 2, sin(t × 0.002) × 10, 0); WA = (0, 0, 0); u = t × 0.0075.

### State 0x61 — leaning back, then the head thrown back
| When | Sets |
|---|---|
| t > 2000 | u = t × 0.01; RS = (1, 0, cos(u) × 5); LS = (−1, 0, sin(t × 0.01) × 5); w = t × 0.003; HA = (−30, cos(w) × 5, sin(w) × 10); BT = (−15, sin(t × 0.002) × 10, 0). |
| otherwise | HA = (40, cos(t × 0.003) × 5, sin(t × 0.005) × 10); BT = (10, sin(t × 0.005) × 10, 0); RS = (−4, 2, 14). |

The sparks of this state: creature-pose-items.md §7.

### State 0x63 — playing an instrument
If 2000 > t: BS.z += −3; BS.y += 2; BT.x = −40; HA.x = −30; BT.z = cos(t × 0.01) × 5;
LS = (4, 8, cos(t × 0.02) × 2 + 6); RS = (−4, 8, cos(float(double(t × 0.02) + π)) × 2 + 6).

### States 0x64, 0x65, 0x66 — swaying while the ability lasts
Only while float(T) > t; otherwise nothing.

| State | Sets |
|---|---|
| 0x64 | BT = (10, sin(t × 0.0075) × 10, 0); RS = (sin(float((double(BT.y × 2) × π) / 180)) × 8, 2, 15). |
| 0x65 | BT = (cos(t × 0.003) × 5 − 25, sin(t × 0.005) × 6, 0); BS.y = 5; RS = (sin(t × 0.01) × 4, 6, 10); RT.x = 20. |
| 0x66 | y = sin(t × 0.005) × 30; z = sin(t × 0.002) × 10; BT = (cos(t × 0.003) × 10, y, z); BS.x = sin(float((double(y) × π) / 180)) × 5; RS = (sin(t × 0.01) × 4 + 2, 6, 5); LS = (sin(t × 0.01) × 4 − 2, 6, 5); KS = cos(t × 0.0045) × 4 − 4; WS.z = 1 − cos(float((double(KS) × π) / 180)). |

### States without an entry
0x0c, 0x0d, 0x0e, 0x25, 0x46, 0x53, 0 and any value above 0x66: nothing.
