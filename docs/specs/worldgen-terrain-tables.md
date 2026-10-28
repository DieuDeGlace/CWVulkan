# World generator, terrain: tables

Part A, fourth file. Values read from the binary (`cubeworld.original.exe.unpacked.exe`) or, for
tables filled at start-up, computed as their initialisers do.

## 1. Noise tables (0x2e0c0)

**PERM** (256 bytes at 0x470ed0; P[i] = PERM[i AND 255] for i = 0..511, M[i] = P[i] mod 12;
filled by 0x2b7690 into 0x54de10 and 0x54e010):

```
151 160 137  91  90  15 131  13 201  95  96  53 194 233   7 225
140  36 103  30  69 142   8  99  37 240  21  10  23 190   6 148
247 120 234  75   0  26 197  62  94 252 219 203 117  35  11  32
 57 177  33  88 237 149  56  87 174  20 125 136 171 168  68 175
 74 165  71 134 139  48  27 166  77 146 158 231  83 111 229 122
 60 211 133 230 220 105  92  41  55  46 245  40 244 102 143  54
 65  25  63 161   1 216  80  73 209  76 132 187 208  89  18 169
200 196 135 130 116 188 159  86 164 100 109 198 173 186   3  64
 52 217 226 250 124 123   5 202  38 147 118 126 255  82  85 212
207 206  59 227  47  16  58  17 182 189  28  42 223 183 170 213
119 248 152   2  44 154 163  70 221 153 101 155 167  43 172   9
129  22  39 253  19  98 108 110  79 113 224 232 178 185 112 104
218 246  97 228 251  34 242 193 238 210 144  12 191 179 162 241
 81  51 145 235 249  14 239 107  49 192 214  31 181 199 106 157
184  84 204 176 115 121  50  45 127   4 150 254 138 236 205  93
222 114  67  29  24  72 243 141 128 195  78  66 215  61 156 180
```

**Gradients** (0x530a40, 3 doubles each; only x and y used):

| i | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| gx | 1 | −1 | 1 | −1 | 1 | −1 | 1 | −1 | 0 | 0 | 0 | 0 |
| gy | 1 | 1 | −1 | −1 | 0 | 0 | 0 | 0 | 1 | −1 | 1 | −1 |

Constants: input scale 0.32 (double at 0x42e8e0); 0.5 (0x4ac9a8); 1.0 (0x4ac9b8); 2.0 (0x4ac9d8);
output × 70.0 (0x42e8f8); F2 (0x54dd28) = (sqrt(3) − 1) × 0.5 = 0.3660254037844386;
G2 (0x54dd38) = (3 − sqrt(3)) / 6 = 0.21132486540518713.

## 2. Biomes (0x2b9080)

v = R(K(rx, ry) + 0xb3f58) mod 50 (unsigned); the origin's 3 × 3 regions are always 2.

| v | 0 | 1–4 | 5–7 | 8–9 | 10–11 | 12–13 | 14 | 15–16 | 17–18 | 19–22 | 23–25 | 26–27 | 28–29 | 30–31 | 32–49 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| biome | 1 | 2 | 0 | 9 | 10 | 4 | 13 | 11 | 6 | 3 | 8 | 7 | 12 | 5 | 1 |

## 3. Small-cell landmark kinds (0x2b99d0, not big)

m = (q / 2) rem 24 (terrain §3.4). h(b) = plain base height at cellPoint(cx, cy, b); "own" =
bigCell(cx, cy); biome(b) = biome of 0x2ad4d0 at that point. shore(h, f) = 36 if h > −10, else f.

| m | Kind |
|---|---|
| 0 | 7 |
| 1 | 17 |
| 2 | 21 if biome(false) = 13, else 20 |
| 3 | 21 |
| 4 | 22 |
| 5 | 1 |
| 6 | 9 |
| 7 | h(false) ≥ 0 → 18; else shore(h, 0) |
| 8 | h(own) ≥ 0 → (14 if biome(own) = 13, else 13); else shore(h, 0) |
| 9 | h(false) ≥ 0 → 14; else shore(h, 0) |
| 10 | 2 |
| 11 | 16 |
| 12 | 29 |
| 13 | 4 |
| 14 | h(false) ≥ 0 → 30; else shore(h, 23) |
| 15 | b = biome(own); b outside 3..6 or b = 4 → 32; else h(false) ≥ 0 → 33, else shore(h, 23) |
| 16 | 32 |
| 17 | h(false) ≥ 0 → 31; else shore(h, 23) |
| 18 | 34 |
| 19 | 15 |
| 20 | 9 |
| 21 | 38 |
| 22 | 37 |
| 23 | 39 |

(The earlier step "q even and h(own) < 10 → 0" comes first.)

## 4. Big-cell landmark kinds (0x2b99d0, big)

(cx, cy) evened; sevenths test → 5; else m = R(K(cx, cy) + 0x2acfe1) (signed) rem 19. h(true) is at
the big point; "own" = bigCell of the evened cell (true in practice).

| m | Kind |
|---|---|
| 0 | 10 |
| 1 | b = biome(own): 4 or 10 → 46; 9 → 45; else 8 |
| 2 | 24 |
| 3 | 15 |
| 4 | 42 |
| 5 | 44 |
| 6 | 47 |
| 7 | 19 |
| 8 | 21 |
| 9 | h(true) < 10 → 0; else 14 |
| 10 | h(own) ≥ 10 → (14 if biome(own) = 13, else 13); else 0 |
| 11 | 9 |
| 12 | b = biome(own): b outside 3..6 → 25; else h(true) ≥ 0 → 33, else shore(h, 23) |
| 13 | h(true) < 50 → 0; else 28 |
| 14 | 29 |
| 15 | h(true) ≥ 0 → 30; else shore(h, 23) |
| 16 | h(true) ≥ 0 → 31; else shore(h, 23) |
| 17 | 26 |
| 18 | 27 |

## 5. Noise calls

N(sx·X + ox, sy·Y + oy); all scales and offsets are doubles. Listed by routine.

| Routine | Use | sx | ox | sy | oy | Multiplier after |
|---|---|---|---|---|---|---|
| 0x358e0 / 0x2ad4d0 | warp x | 0.0003 | 8432 | 0.0006 | 9438 | × 0.125 × 8192 |
| | warp y | 0.0006 | 567 | 0.0003 | 1223 | × 0.125 × 8192 |
| 0x364e0 / 0x2baf90 | landmark warp x | 0.005 | 18843 | 0.01 | 9432 | × 32 |
| | landmark warp y | 0.01 | 299 | 0.005 | 90650 | × 32 |
| 0x2a0840 | lake | 0.0015 | 6667 | 0.0015 | 33322 | + 1 |
| | relief | 0.001 | 247412 | 0.001 | 8128321 | × 20 |
| 0x2b5960 | dryness | 0.0002 | 6654 | 0.0002 | 98883 | × 0.35 |
| 0x2d9b60 | warmth | 0.0002 | 4234234 | 0.0002 | 55543 | × 0.35 |
| 0x2d4730 | road a | 0.0025 | 2346 | 0.005 | 64565 | × 128 |
| | road a' | 0.005 | 4234 | 0.01 | 5434545 | × 32 |
| | road b | 0.005 | 23432 | 0.0025 | 2342344 | × 128 |
| | road b' | 0.01 | 54356 | 0.005 | 7677 | × 32 (later) |
| | village wear | 0.01 | 0 | 0.01 | 0 | |
| 0x2bb310 | amount | 0.0015 | 565 | 0.0015 | 8877 | |
| | m1 | 0.001 | 34234 | 0.001 | 3245 | |
| | m2 | 0.0015 | 545 | 0.0015 | 654665 | |
| | ridge | 0.006 | 55545 | 0.006 | 6566 | × 0.2 |
| | peaks (kinds 10, 19) | 0.025 | 84333 | 0.025 | 98111 | + 1 |
| 0x2d36f0 | vegetation 1 | 0.01 | 647646 | 0.01 | 84373 | |
| | vegetation 2 | 0.02 | 43234 | 0.02 | 5345345 | × 0.2 (double) |
| | vegetation 3 | 0.05 | 77878 | 0.05 | 9888 | × 0.1 (double) |
| 0x2b4450 | rolling | 0.005 | 3363 | 0.005 | 7874 | + 1, × 25 |
| | rolling detail | 0.015 | 4447657 | 0.015 | 87878 | |
| | terrace 1 | 0.015 | 234234 | 0.015 | 12233 | |
| | terrace 2 | 0.01 | 10943 | 0.01 | 98433 | |
| | step 1 | 0.005 | 7743 | 0.005 | 98834 | |
| | step 2 | 0.005 | 6777 | 0.005 | 8887 | |
| | step 3 | 0.005 | 2133 | 0.005 | 444 | |
| | pit wall | 0.02 | 98453466 | 0.02 | 8743 | × 25 + 30 |
| | peaks | 0.01 | 8757 | 0.01 | 9087 | × 0.25 + 0.75 |
| | river bed | 0.025 | 35 | 0.025 | 8933 | + 1, × 0.5 |
| | cliff T (also zone cliff pass) | 0.01 | 7474 | 0.01 | 98984 | + 1, × 20 |
| | cliff slope (also zone) | 0.02 | 8744 | 0.02 | 904 | × 0.025 + 0.06 |
| 0x2b5670 | grain 1 | 0.08 | 432 | 0.08 | 432 | × 0.05 (double) |
| | grain 2 | 0.04 | 432 | 0.04 | 432 | |
| 0x2a2c50 | cliffs a | 0.1 | 7007 | 0.1 | 6000 | × 0.1 |
| | cliffs a2 | 0.01 | 1838 | 0.01 | 88233 | |
| | cliffs b | 0.01 | 43255 | 0.01 | 6678 | × 0.25 |
| 0x2b7730 | shore | 0.02 | 7743 | 0.02 | 8977 | + 1, × 10 |
| 0x2d8e00 | lava | 0.1 / 0.05 | 0 | 0.1 / 0.05 | 0 | sum × 50 + 50 |
| | poison | 0.06 | 0 | 0.06 | 0 | × 20 + 120 |
| 0x2b40b0 | grass a | 0.005 | 343 | 0.005 | 9233 | + 1, × 0.5 |
| | grass b | 0.02 | 6654 | 0.02 | 778 | + 1, × 0.5 |
| 0x2ab410 | ground t | 0.015 | 1883 | 0.015 | 98984 | + 1, × 0.5 |
| | ground k | 0.01 | 431 | 0.01 | 873 | × 30 + 30 |
| | sand | 0.03 | world+0x330 | 0.03 | world+0x334 | + 1, × 0.5 × 70 |
| forest / kind-27 tints | p | 0.01 | 0 | 0.01 | 0 | + 1, × 20 / 30 / 100 |
| | q | 0.006 | 4343 | 0.006 | 983 | + 1, × 20 / 30 / 50 / 100 |
| 0x2d3150 | rock 1 | (x_in = Y × 0.1 + 98984, y_in = z × 0.4) | | | | |
| | rock 2 | (x_in = X × 0.1, y_in = z × 0.4) | | | | |
| | lava rock s | 0.04 | 8773 | 0.04 | 99808 | × 0.5 |
| | lava rock s2 | 0.02 | 0 | 0.02 | 0 | |
| | lava rock n | 0.1 | 0 | 0.1 | 0 | + 1 |
| 0x2b4320 | snow | 0.05 | 8411122 | 0.05 | 884646 | × 0.05 + 0.2 |

Channel noises are in the height file §5.1.

## 6. Kind masks

| Mask | Kinds | Used by |
|---|---|---|
| 0x1010220000 | 17, 21, 28, 36 | highlands flattening and place shapes skip these |
| 0x1006100 | 8, 13, 14, 24 | the place's own height blended by smooth(2w) |
| 0x3c02e6400 | 10, 13, 14, 17, 18, 19, 21, 30, 31, 32, 33 | roads cleared |
| 0x10102a0400 | 10, 17, 19, 21, 28, 36 | water level stays at base |

## 7. Fixed blocks (static initialisers 0x10e0–0x1120, 0x5ee0–0x5f20)

| Name | Address | r g b glow type flags |
|---|---|---|
| air | 0x54dcb8 | 255 255 255 0 0 0 |
| solid grey (outside the zone / below base) | 0x54dcb0 | 200 200 200 0 1 0 |
| sea water (lookup) | 0x54dcc0 | 255 255 255 0 2 0 |
| sea water (store) | 0x553528 | 255 255 255 0 2 0 |
| air (store) | 0x553520 | 255 255 255 0 0 0 |
