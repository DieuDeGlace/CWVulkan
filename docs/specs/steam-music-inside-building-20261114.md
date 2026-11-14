# Steam music choice: the "inside a building" predicate and 0x2aa270

Authority: cubeworld.original.exe.unpacked.exe, 5,757,952 bytes, SHA-256
7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244 (hash re-computed for this
note), image base 0x140000000. All addresses below are RVAs of that image.

What was read: docs/CLEANROOM_AUTHOR.md (role policy); docs/specs/sound.md section 3.3 only, for
the question's context; the decompiled and disassembly listings of the analysis image (functions
0xa5a50, 0x2aa270, 0x9ae80, 0xeab0, 0xe9a0, 0x36160, the static initializer at 0x1100, and the
call/test sites listed in section 3); raw bytes of the image for the jump tables at
0x2aa3fc/0x2aa420 and the constant at 0x4ac964. Infinite-Cubes was consulted only for one lead
(the name of a block flag bit, file include/cube/Block.h and BlockIds.h of its release tree); it
is marked as a lead below. No executable was run. Nothing else in the CubeVulkan repository was
read.

## 1. The predicate and its place in the music choice

The music choice lives in the game update 0xa5a50. In order (instruction RVAs in brackets):

1. The biome B is read from the local player's region +0x150/+0x154 (0x2b9080, at 0xab6fc).
   The local player is the creature at world +0x448, where world is game +0x8.
2. The zone Z is computed from the player's 64-bit fixed-point position fields +0x10 (x) and
   +0x18 (y), each first turned into a block coordinate by dividing by 0x10000 rounding toward
   zero, then passed through 0x9beb0 (floored division by 1024) [0xab70f..0xab73c].
3. The "double zone" byte D = 0x36160(world, Zx, Zy) [0xab74a].
4. The provisional place kind K0 = 0x2b99d0(world, Zx, Zy, D) [0xab75f], a 32-bit value.
5. A call to 0x362a0 for the same zone [0xab784]; it is not part of the predicate.
6. **The predicate** [0xab789..0xab7ba]: the game looks up the block record at the player's
   three-dimensional position, using all three fixed-point fields +0x10 (x), +0x18 (y) and
   +0x20 (z) of the player creature (not the camera, and not a ground column). The lookup is
   0xeab0: each coordinate is floor-divided by 0x10000 (true floor for negatives) to give a block
   coordinate; 0xe9a0 then finds the storage cell keyed by (block x ÷ 64, block y ÷ 64), floored,
   in the map at world +0x460, and either copies the 6-byte block record out of it (via 0xeb90)
   or, when the cell is absent or its +0x18 pointer is null, copies the 6-byte default record
   at 0x54dcb0. The test is 0x9ae80: bit 5 (mask 0x20) of byte +5 of that 6-byte record.
   0x9ae80 has exactly one caller in the image, this site (0xab7b3).
   The default record is filled by the static initializer at 0x1100 with the bytes
   C8 C8 C8 00 01 00, so its byte +5 is 0: a position whose storage is not present is treated
   as "not inside".
7. If the bit is set ("inside"): K = 0x2aa270(world, Zx, Zy, K0, D) [0xab7ce], replacing K0
   outright (including by 0). The place-weight test is skipped entirely, so the 0.5 rule does
   not apply to this K.
8. If the bit is clear: the place weight is computed (0x364e0, then 0x9bee0 with K0 and D)
   [0xab7d8..0xab831], and K = 0 when 0.5 is greater than the weight, else K = K0
   [0xab836..0xab83f]. The 0.5 is the float at 0x4ac964 (value 0.5 in the image), loaded into a
   preserved register at 0xab3c4 and not rewritten before this comparison. The comparison is
   "0.5 above the weight", so a weight of exactly 0.5 keeps K0; an unordered (NaN) weight also
   keeps K0.
9. K (from 7 or 8) is next used for the place-name subtitle (0x166ab0 at 0xabb6f, only when
   K is nonzero); that is HUD text, not music.
10. Title rule [0xabd4b..0xabd69]: zone (0, 0) and K nonzero gives maintheme.ogg.
11. Shrine rule [0xabd6e..0xabdb1]: the game repeats exactly the same lookup (0xeab0 at the
    player's +0x10, +0x18, +0x20) and tests the same mask 0x20 in byte +5 (inline test at
    0xabd9d); if set, shrine.ogg.
12. Only otherwise does the biome / ground-height / place-kind mapping of sound.md rule 3 run.

Consequences that follow from this order:

- The "inside a building" predicate and the shrine test are the same predicate: the same
  record, same position, same bit. sound.md describes the shrine test as a lookup of the
  column at the player's (x, y); in the image it is the three-dimensional block at the
  player's (x, y, z), identical to the predicate.
- Therefore, whenever the 0x2aa270 branch is taken, the result of rules 1 to 3 is either
  maintheme.ogg (zone (0, 0) and a nonzero 0x2aa270 result) or shrine.ogg. The place-kind
  mapping of rule 3 never receives a 0x2aa270 result, and such a frame is never "place
  music" through rule 3. Later rules (spirit world, boss, success, menus) still override as
  in sound.md.
- This holds provided the block record does not change between the two lookups in the same
  frame. The code between them is HUD text work; no world write was observed there, but that
  was not exhaustively proved (see section 4).

Lead (Infinite-Cubes, not confirmed in Steam): byte +5 of the 6-byte block record is the block
flags byte, and mask 0x20 is named there as the block's "dungeon" flag (used for the dungeon's
name and music). The Steam evidence above confirms only the record, offset and mask; whether
the bit means "building", "dungeon" or something broader is a naming question left open.

## 2. What 0x2aa270 takes and returns

Inputs: world (passed but never read), zone x Zx and zone y Zy (32-bit signed), the place kind
K0 from 0x2b99d0 (32-bit), and the double-zone byte D (fifth argument, on the stack; only zero
versus nonzero matters).

Output: a 32-bit place kind. 0 is the "none" value.

Rules:

1. If D is zero, the result is 0.
2. Otherwise Zx and Zy are each rounded down to an even number (floor(v ÷ 2) × 2, true floor for
   negatives): ex, ey.
3. The kind K0 selects (jump tables at 0x2aa3fc and index bytes at 0x2aa420, covering K0 from
   5 to 0x2f):
   - 0x2a, 0x2c, 0x2d, 0x2e, 0x2f: returned unchanged.
   - 5, 9, 0xf: a hash decides (below).
   - everything else (including 0x2b, 0x28, 0x29 and values outside 5..0x2f): 0.
4. The hash, in 32-bit wrapping arithmetic: u = ey × 0x39 + 0x62ad0 + ex; u becomes
   u XOR (u shifted left by 13); v = ((u × u × 0xec4d + 0x131071f) × u + 0xd208dd0d) with bit 31
   cleared. Then h is the high 32 bits of the signed 64-bit product of v and the signed 32-bit
   constant 0x9e5706b3; s is h shifted right arithmetically by 13; q is s plus 1 when s is
   negative, else s. (Note: this is exactly the instruction sequence; it is not the usual
   division-by-constant form, so no divisor is claimed.)
   - K0 = 5: result 0x29 when the low three bits of q are all zero, else 0.
   - K0 = 9: result 0x28 when the low bit of q is zero, else 0.
   - K0 = 0xf: result 0x2b when the low bit of q is zero, else 0.

How the music choice treats it: the result replaces K with no further weight test (section 1,
step 7). A result of 0 makes the title rule fail; the shrine rule then fires (same predicate).
A nonzero result matters for the music only through the title rule at zone (0, 0).

## 3. Other ordinary users

Calls of 0x2aa270 (call site, containing function):
0x678e9 (0x675c0); 0xab7ce (0xa5a50, the music choice); 0x116f1e (0x115df0); 0x11a09b
(0x119a00); 0x12c490 (0x12a6f0); 0x2a83fb (0x2a8290); 0x2a9e85 (0x2a9df0); 0x2ab220 (0x2ab0a0);
0x2b2ac6 (0x2b0400); 0x2d456e (0x2d4290); 0x2d54e1 and 0x2d7639 (0x2d53d0); 0x2f1f32 (0x2eb7b0);
0x2fb979 (0x2fb850); 0x2ffab1 (0x2ff6f0).

The same block-record test (mask 0x20 of byte +5, written as an inline bit test):
- after a fixed-point position lookup through 0xeab0: 0xabd9d (0xa5a50, the shrine rule);
  0x2ad92a (0x2ad8f0); 0x2b2a3b (0x2b0400); 0x2dab3e and 0x2db745 (0x2da6c0); 0x2e565e
  (0x2e3510).
- after a direct block-coordinate lookup through 0xeb90: 0x14328e (0x142f20); 0x144466
  (0x144260); 0x144c24 (0x1448a0); 0x1451d6 (0x144fd0); 0x14571e (0x145600).

Two of these show the same predicate-then-kind shape as the music choice: 0x2b0400 tests the
bit at a position and, when set, derives the zone from that position (floored ÷ 1024) and calls
0x2aa270, then distinguishes results 0x2b and 0x2c; 0x2ad8f0 tests the bit and, when set,
derives the zone from the same position. Their further behavior is not specified here.

## 4. Limits

- The meaning of flag 0x20 (building, dungeon or other interior) is not established from Steam;
  only its location and use are. The "dungeon" name is a reconstruction lead.
- The search for other users of the bit covered only the inline byte-test encoding and the one
  helper 0x9ae80; tests written in another form (shift-and-mask, bit-test instructions on a
  loaded byte, copied flag bytes) were not searched.
- That the block record is unchanged between the predicate lookup (0xab7ab) and the shrine
  lookup (0xabd91) in the same frame was not exhaustively proved; no world write was seen in
  the intervening code.
- The runtime content of the default record at 0x54dcb0 is taken from its static initializer
  at 0x1100; no later write to it was found, but writes through pointers were not ruled out.
- The 64-block storage cell scale in 0xe9a0 is stated as observed; its relation to zones is not
  specified here. The internal behavior of 0xeb90 (lookup inside a cell, including any vertical
  range check) was not analyzed beyond its use of the same default record.
- What 0x362a0 (step 5) computes was not analyzed; it does not feed the predicate or K.
- No runtime capture verified any of this; it is static evidence from the analysis image only.
