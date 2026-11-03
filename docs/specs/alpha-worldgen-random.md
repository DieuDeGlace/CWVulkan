# Alpha world generation: random numbers, seeds and the noise kernel

Behavioural specification of every source of randomness the Cube World **Alpha** world
generator uses: the C runtime random-number generator, the points where the generator
re-seeds it, the per-world seed offsets, and the single lattice-noise kernel that every
terrain, climate, colour, vegetation and cloud formula is built on. Written from the Alpha
client `Cube.exe` (SHA-256 `84a7a132…30bf`), its assembly and the constants in the binary.
Companion documents: [overview](alpha-worldgen-overview.md), [regions](alpha-worldgen-regions.md),
[terrain](alpha-worldgen-terrain.md), [vegetation](alpha-worldgen-vegetation.md),
[clouds](alpha-worldgen-clouds.md).

Routines are cited by their address in `Cube.exe` (image base 0x400000) unless marked
**server** (`Server.exe`, also base 0x400000; the two address spaces are unrelated).

---

## 0. Conventions

* **f32(x)** means "rounded to IEEE single precision". **Float** arithmetic means every
  intermediate is rounded to single precision (SSE `…SS` instructions); **double** means
  IEEE double (SSE `…SD`). Where a formula mixes the two, the text says where the rounding
  happens. A reimplementation must reproduce these roundings to be bit-exact.
* **trunc(x)** is conversion to a 32-bit signed integer by truncation towards zero (the
  `CVTT…2SI` instructions). An out-of-range value gives 0x80000000 (−2147483648).
* Integer arithmetic is 32-bit two's complement with wrap-around unless stated.
* **x mod n** for a possibly negative x is the C remainder (sign follows x). **floor_div**
  is division rounding towards −∞ (the generator writes it as "add n−1 if negative, then
  shift"), e.g. floor_div(−1, 16384) = −1.
* "smooth(t)" is 3t² − 2t³. "sat(t)" clamps to [0, 1].
* Every noise argument in the generator is a double; every noise result is a float.

---

## 1. The C runtime generator

### 1.1 Algorithm

All random draws use `rand()` / `srand()` imported from **MSVCR110.DLL** (Visual C++ 2012
runtime; import slots 0x6fc468 = `rand`, 0x6fc4b4 = `srand`). This is the classic linear
congruential generator:

* state ← state · 214013 + 2531011 (mod 2³²);
* the result is (state >> 16) & 0x7FFF, an integer in 0..32767;
* `srand(s)` sets state = s (s taken as an unsigned 32-bit value).

The state is **per thread** in this runtime. The generator only ever draws after it has
itself called `srand` for the object it is building (§2), so a reimplementation may give
the generator one private state, provided it re-seeds at exactly the points of §2 and draws
exactly as specified. Draws made by other systems on other threads (stars, particles, AI)
never interleave with generator draws once the generator has re-seeded. (Open question 1:
which thread runs zone generation.)

### 1.2 Draw idioms

The generator uses these forms; "draw" below always means one call:

| Written here | Meaning |
|---|---|
| draw % n | one draw, C remainder by n (draws are non-negative, so plain modulo) |
| draw & 1, draw & 3 … | one draw, low bits (the code's signed fix-ups never trigger) |
| u(a) = draw · a / 32767 | one draw scaled, precision stated per use |

### 1.3 Test vector

`srand(123)` then three draws give 440, 19053, 23075 (state after the first draw
= 123·214013 + 2531011 = 28854610, 28854610 >> 16 = 440). The default world seed of a fresh
world object is 123 (0x7b, pushed at 0x58ee17 by the world constructor 0x58eb00).

---

## 2. Seeds and re-seeding points

### 2.1 The world seed

The world seed is a 32-bit integer S stored at world+0x800164 (the world object W is the
game object G + 0x2e4).

* **Client.** At start-up G+0x800a50 is set to `timeGetTime()` (0x45a72d) and the world is
  initialised with it (0x45a741). When the server's world information arrives, 0x46f620
  stores the server's seed in G+0x800a50 (with the world name at G+0x800a54) and the
  streaming routine 0x46a8a0 re-initialises the world with it (0x46afab) if it differs.
  So in multiplayer the client always generates from the **server's** seed.
* **Server.** The server has the same world class (initialiser at **server** 0x4d83a0,
  identical structure); it is called with 123 by its constructor (**server** 0x4c888d) and
  with the loaded/configured seed from **server** 0x549a9e and 0x549e67. Where the server
  reads its seed from was not traced (open question 2).

### 2.2 World initialisation: the seed offsets (0x5a52e0)

`srand(S)`, then exactly **77** draws, each taken modulo 100000 (which leaves the draw
unchanged, since draws are below 32768), stored as 32-bit integers in this order:

| Order | Count | Offset in W | Name used in these specs |
|---|---:|---|---|
| 1 | 1 | +0x800188 | **A**, the world's base offset |
| 2 | 4 | +0x800168 … +0x800174 | O168, O16c, O170, O174 |
| 3 | 20 | +0x80018c … +0x8001d8 | O18c … O1d8 (terrain height) |
| 4 | 2 | +0x8001dc, +0x8001e0 | O1dc, O1e0 (mountain mask) |
| 5 | 2 | +0x8001e4, +0x8001e8 | (not used by the routines specified here) |
| 6 | 2 | +0x8001ec, +0x8001f0 | (not used by the routines specified here) |
| 7 | 2 | +0x8001f4, +0x8001f8 | (not used by the routines specified here) |
| 8 | 8 | +0x8001fc … +0x800218 | O1fc … O218 (link channels) |
| 9 | 6 | +0x80021c … +0x800230 | O22c, O230 used (rock colour) |
| 10 | 6 | +0x800234 … +0x800248 | (not used by the routines specified here) |
| 11 | 10 | +0x80024c … +0x800270 | O24c … O270 (cliff colour) |
| 12 | 4 | +0x800274 … +0x800280 | O274 … O280 (sand colours) |
| 13 | 2 | +0x800284, +0x800288 | (not used by the routines specified here) |
| 14 | 4 | +0x800178 … +0x800184 | O178, O17c used (ravines) |
| 15 | 2 | +0x80028c, +0x800290 | (not used by the routines specified here) |
| 16 | 2 | +0x800294, +0x800298 | (not used by the routines specified here) |

Note that group 14 (at +0x800178) is drawn *after* groups 1–13 although it lies earlier in
memory; the draw order is the one in the table. "Not used here" offsets may be used by
structure, dungeon or creature code (follow-up specs).

The initialiser also clears all region and zone grids, sets the home position
W+0x8000f0 = W+0x8000f4 = 8396928.0 (float bits 0x4b002080, in blocks) and W+0x8000f8 = 0,
and (single player, when a save name is set) reads the stored time of day from the world
save `Save/world_<name>…` (0x5a5730..0x5a57db).

**Test vector (S = 123):** A = 440; O168..O174 = 19053, 23075, 13104, 32363;
O18c..O1d8 = 3265, 30749, 32678, 9760, 28064, 20429, 750, 4480, 3863, 30429, 18586, 30758,
31595, 31567, 24630, 16059, 2734, 703, 3966, 12383; O1dc, O1e0 = 18065, 28392;
O1e4, O1e8 = 29902, 26443; O1ec, O1f0 = 16505, 11114; O1f4, O1f8 = 11737, 10421;
O274..O280 = 26554, 15326, 23548, 10611; O178, O17c = 25656, 17603.

### 2.3 Re-seeding points inside generation

| Routine | Seed | When |
|---|---|---|
| region cell (0x5d7a70) | rx + 0x108a + 1024·ry + 3A | first time cell (rx, ry) is needed |
| region (0x5da280) | A + 1024·ry + rx | after its 5×5 region cells exist, before its own draws |
| zone (0x5e4850) | A + 65536·zy + zx | after its 3×3 regions exist, before its own draws |

All sums are 32-bit with wrap-around. (rx, ry) are region indices, (zx, zy) zone indices
(see the overview). Sub-generators called from the zone generator (houses, dungeons, …)
were not checked for further `srand` calls (open question 3).

---

## 3. The noise kernel N(x, y) (0x4c0ef0)

One function, cosine-interpolated value noise on the integer lattice. Every other formula
in these specs is written in terms of **N(x, y)**. Server counterpart: **server** 0x4d5d30
(same constants).

### 3.1 Lattice value

For a 32-bit integer n:

1. n ← n XOR (n << 13) (32-bit);
2. h = ((n·n·60493 + 19990303)·n + 0xD208DD0D) AND 0x7FFFFFFF, all 32-bit wrap-around
   (60493 = 0xEC4D, 19990303 = 0x131071F; the code subtracts 0x2DF722F3, which is the same
   modulo 2³²);
3. value(n) = 1 − h · 2⁻³⁰, in double. Range (−1, 1].

### 3.2 Interpolation

Inputs x, y are doubles.

1. ix = trunc(x), iy = trunc(y) (**towards zero**, not floor).
2. Corner indices (32-bit): a = ix + 57·iy, b = (ix + 1) + 57·iy, c = ix + 57·(iy + 1),
   d = (ix + 1) + 57·(iy + 1).
3. fx = (x − ix)·π, fy = (y − iy)·π in double, with π the double constant 3.1415927
   (0x702b48; note: not the full-precision π).
4. wx = (1 − cos fx)·0.5, wy = (1 − cos fy)·0.5, cos in double (CRT `cos`).
5. N = [value(a)·(1 − wx) + value(b)·wx]·(1 − wy) + [value(c)·(1 − wx) + value(d)·wx]·wy,
   all in double, then rounded to float.

Consequences: N is in (−1, 1]; it is not symmetric around 0; at integer points N equals
the lattice value; for negative non-integer coordinates the fraction is negative but the
cosine is even, so the cell between −1 and 0 is a mirror of the cell between 0 and 1
(both blend corners 0 and 1 of that axis). The world only uses positive block coordinates,
but some formulas subtract, so the implementer must keep truncation semantics.

### 3.3 Test vectors (computed from the formulas above)

| x | y | N(x, y) |
|---|---|---|
| 0 | 0 | −0.28179097175598145 (lattice value of 0: h = 0x5208DD0D) |
| 0.5 | 0 | −0.4591194987297058 |
| 0 | 0.5 | −0.43308860063552856 |
| 1.25 | 2.75 | 0.14833128452301025 |
| −0.3 | 0 | −0.3548884093761444 (mirrored cell) |
| 100.5 | 200.25 | 0.39360496401786804 |
| 4198.464 | 3423 | 0.5244963765144348 |

The last row is the x-displacement noise of §3 of the regions spec at the home position.

---

## 4. Shared helper shapes

These recur and are named here once:

* **unit(n)** = (n + 1)·0.5 in float: maps N to (0, 1].
* **ridge** forms 1 − |a + b·k|: the absolute value is taken by converting the float sum to
  double, clearing the sign bit, and rounding back (exact, so it equals the float |…|).
* **bump(f)** = max(1 − f, 0)² for a normalised squared distance f (§8 of the regions spec).
* **ease(t)** = 1 − (1 − t²)² and its variants; each use is written out in full.

---

## 5. Checking against the running game

* The noise kernel can be checked in isolation by calling 0x4c0ef0 from a 32-bit mod DLL
  (cdecl, two doubles on the stack, float result in ST0) at the test inputs above.
* The seed offsets can be read from the world object after world initialisation
  (W = G+0x2e4; offsets in §2.2) and compared with §2.2 for the session's seed (the seed is
  at W+0x800164).
* `rand` state is per thread; a hook on MSVCR110 `srand` logging (thread id, seed) during
  zone loading shows the re-seeding sequence of §2.3 and answers open question 1.

---

## 6. Open questions

1. **Generation thread.** Zone generation is called from the streaming routine 0x46a8a0,
   which is wrapped (0x46dba0) and appears to be started through `_beginthread` at
   0x450e70 (thread entry 0x450e00 invoking a stored function object). If generation runs
   on its own thread, its `rand` state is private, which these specs assume.
2. **Server seed source** (configuration, save file, or time) was not traced.
3. **Nested re-seeding.** Whether house, dungeon or creature builders called from the zone
   generator call `srand` themselves was not checked; if they do, every later draw in the
   zone depends on it.
4. The meaning of the offsets marked "not used here" belongs to the follow-up specs.

See also: question 3 — in the direct call graph, the only `srand` calls reachable from the zone generator are 0x5d7a70, 0x5da280 and 0x5e4850 themselves (dungeon, town, tree and camp builders never re-seed; virtual calls not covered), see [alpha-worldgen-caves.md](alpha-worldgen-caves.md) §6.1.
