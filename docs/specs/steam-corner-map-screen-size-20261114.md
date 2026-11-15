# Steam world map: corner map versus full map screen size and placement

## Image identity

- File: cubeworld.original.exe.unpacked.exe (Steam x64 Cube World, unpacked image)
- SHA-256: 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244 (re-hashed during this task; matches)
- Preferred image base 0x140000000. All addresses below are RVAs. Constants were read from the file bytes using the section table (.text RVA 0x1000 at file 0x400; .rdata RVA 0x42C000 at file 0x42A800; .data RVA 0x530000 at file 0x52DC00). Values in .data are their initial file values and could be changed at run time.
- Role: source-side specification author. This document is code-free behavioural prose.

## 1. Call chain and ownership

1. The Game frame routine 0xA5A50 updates two eased map values held in the Game object: the map screen centre and the map "current scale". At 0xAD9D4 it makes the only call to scene submission 0xE2E00.
2. 0xE2E00 (scene object) calls the map pass 0xE64B0 at 0xE41A2. It does this only when the world object (scene+0x08) has a nonzero qword at +0x210 and the flag word does not have bit 0x20 set.
3. 0xE64B0 sets up the map pass. It turns the centre fractions into pixel coordinates, passes the scale through unchanged and calls the map renderer 0x2F44A0 at 0xE65C1. The renderer object is scene+0x10. After the call, 0xE64B0 hands the renderer's view matrix (renderer+0x58) and projection matrix (renderer+0x18) to the graphics backend, through scene virtual slots +0x58 and +0x60 respectively.

The renderer receives exactly the same set of arguments in both states. The only things that differ between the corner state and the full state are:
- the centre fractions;
- the scale;
- the "half-radius" byte, which is the last argument.

### 1.1 Arguments that reach 0x2F44A0 (order as passed by 0xE64B0)

1. The renderer object.
2. The draw target (scene+0x268).
3. A pointer to the map focus position: three signed 64-bit 16.16 fixed-point world coordinates. This is Game+0x1E20.
4. Centre x in pixels: the scene width (signed 32-bit at scene+0x41C) converted to float, multiplied by centre fraction x, then truncated toward zero.
5. Centre y in pixels: the scene height (signed 32-bit at scene+0x420), multiplied by centre fraction y in the same way.
6. Viewport width: the scene width integer.
7. Viewport height: the scene height integer.
8. A pointer to three floats (angles in degrees):
   - the first is the negated scene float at +0x350, clamped by a minimum against -120.0 (0x42E894), so it is never above -120;
   - the second is 0;
   - the third is the negated scene float at +0x358.
   0xE2E00 copies the scene floats +0x350/+0x354/+0x358 from its fourth argument at entry.
9. Zoom level: a signed integer, Game+0x1E04.
10. Scale: a float (see section 3).
11. A truncated integer from the scene float at +0x330. The renderer never reads it.
12. A byte: flag bit 0x10. The renderer never reads it.
13. The half-radius byte: the logical NOT of flag bit 0x02. It is nonzero in the corner state.

The fourth argument of 0xE64B0 (the float Game+0x664, forwarded by 0xE2E00 as its 14th argument) is not used by 0xE64B0.

### 1.2 The flag word (built by 0xA5A50 between 0xAD757 and 0xAD8C0)

- Bit 0x02 is set when the byte at Game+0x668 is nonzero.
- Bit 0x20 suppresses the map pass. It is set unless the window test at 0xAD780 succeeds, and also when any of the windows tested at 0xAD81D, 0xAD82D or 0xAD83D is active.
- Bits 0x08, 0x80, 0x10 and 0x40 do not affect map size.

In this document:
- the **full state** is Game+0x668 nonzero;
- the **corner state** (ordinary play) is Game+0x668 zero.

## 2. Screen-centre target and easing (0xA5A50, 0xAB4BD to 0xAB5B6)

The current centre is two floats at Game+0x1E48 (x) and Game+0x1E4C (y). Each frame 0xA5A50 moves it toward a target.

### 2.1 Frame time and the ease factor

- Frame time: at 0xA5AFC the routine's double argument is clamped to at most 1000.0 (double at 0x437FD8). The result is kept in a stack slot and, at the time of the map ease, in a callee-saved register. Its unit is not proved.
- The pair ease 0x7D050 moves each component by (target - current) multiplied by an ease factor. The single ease 0x16EE0 does the same for one float.
- The ease factor comes from 0xA7A0, called with the frame time and the rate 0.01 (0x42E51C):
  - the step count is the frame time multiplied by 100.0 (double at 0x4ACA18), truncated to an integer;
  - starting from 0, for each step i (from 0 to count-1) the value moves toward (i+1)/count by the fraction rate x 0.02 (double at 0x42E540);
  - the factor is the final value, converted to float. It is 0 when the step count is 0 or less.

### 2.2 Full state

- The target centre is (0.5, 0.6). Both are immediate constants written into a stack pair at 0xAB4C7/0xAB4D1 (0x3F000000 and 0x3F19999A).
- That stack pair is the same slot that held the clamped frame time. The frame time was already in a register by then.
- The centre is eased toward this target.

### 2.3 Corner state

When the signed integer Game+0x1D60 is greater than 0:
- target x = 1.0 - ((float of Game+0x1D64) x 0.3 / (float of Game+0x1D60)) x mapScale;
- target y = 1.0 - mapScale x 0.2 (0.2 from 0x42E700).

mapScale is the float at Game+0x1BD0, the options map scale. The target is written to the same stack pair and eased with the same helper and rate.

When Game+0x1D60 is 0 or less, no centre target is computed and the centre is not eased that frame.

Game+0x1D60 and +0x1D64 are also passed to 0xE2E00 as its fifth and sixth arguments. They appear to be the window width and height, but this was not proved. They are distinct storage from the scene width and height (scene+0x41C/+0x420) that the renderer uses.

### 2.4 Sources of the constants in the corner horizontal term

- **The 1.0** is a callee-saved register. Its only write in 0xA5A50 is at 0xA7076, from 0x4AC980 (1.0).
- **The 0.3** is also a callee-saved register, with three writes on the paths that reach the multiply:
  - 0xA9E78 loads 0.3 from 0x42E704.
  - Inside the loop over the list at world+0xB0 (0xAA1F0 to 0xAA523), the same register temporarily holds a stack-loaded per-element float (stack offset 0xA54 in the frame's local area, filled by 0x7FB50 at 0xAA1FB).
  - Every exit from that loop falls through 0xAA529, which reloads 0.3 from 0x42E704.
  - When the list is empty, the branch at 0xAA1DB jumps to 0xAA543 with the register still holding 0.3 from 0xA9E78.
  - Every conditional or unconditional jump that targets the range 0xAA543 to 0xAB51F comes from inside that range. No path reaches the multiply with the stack-loaded value.

  So the multiplier is always 0.3. This includes ordinary play, which takes the corner branch whether or not the list is empty.
- **The 0.01 reloads**: the reloads of the same register with 0.01 at 0xAB57A and 0xAB598 happen after the multiply (0xAB547). They only supply the ease rate.

### 2.5 Conversion to pixels

0xE64B0 converts the fractions to pixel coordinates against the scene width and height (section 1.1). The renderer then converts them to a normalised-device offset (section 4.2).

Steady-state centres:
- Full: the horizontal centre of the viewport, and an NDC vertical offset of +0.2.
- Corner, at mapScale m with the window and scene sizes equal:
  - pixel x = W - 0.3 x H x m;
  - NDC y = 1 - 0.4 x m. That places the centre 0.2 x H x m from the edge of the viewport that NDC +y maps to.

## 3. The scale value per state

- The current scale is the float at Game+0x1E50.
- The scale target is:
  - full state: 1.8 (0x4328D0), loaded into a stack slot (frame offset -0x78) and passed by pointer;
  - corner state (both sub-branches, including width 0 or less): mapScale itself, passed as a pointer to Game+0x1BD0.
- It is eased by 0x16EE0 with the frame time and rate 0.01.
- 0xA5A50 passes mapScale (Game+0x1BD0) multiplied by the current scale (Game+0x1E50) as the scale argument. 0xE2E00 and 0xE64B0 forward it unchanged.
- Steady state: full = 1.8 x mapScale; corner = mapScale x mapScale (that is, 1.0 at mapScale 1).
- During a transition the current scale passes through intermediate values. The scale argument is always mapScale x current.

### 3.1 Uses of the scale inside the renderer

- **View scale.** The renderer multiplies the scale by 0.6 (0x42E708) and by 80.0 (0x42E5B8), divides by 120.0 (0x42E7C8), multiplies by 16.0 (0x4ACA34), and divides by the float value of the signed integer at 0x5469A4 (initial value 8). The result is s = 0.8 x scale, used as a uniform scale.

  This divisor is the global at 0x5469A4, not the halved per-call radius. So the half-radius byte does not change s.
- **Constant marker size.** Entity and player marker size factors are 0.4 (0x42E52C) divided by s, so markers keep a constant screen size whatever the scale.
- **Route dot spacing.** Route dots are spaced by the integer part of (block factor x 5) / scale (block factor defined in section 5).

## 4. Projection and view inside 0x2F44A0

If the viewport height is below 0.0001 (0x4AC954), the renderer returns without drawing.

### 4.1 Projection (column-major 4x4 at renderer+0x18, elements 0 to 15)

- **Field of view:** f = 1 / tan(0.3926991) (0x43EF54, which is pi/8). That gives f = 2.414214 and a 45-degree vertical field of view.
- **Element 0:** -(f / (width / height)). The sign is applied with the mask at 0x4ACB30.
- **Element 5:** f.
- **Element 10:** 0x3F800347 (about 1.0001).
- **Element 11:** 1.0.
- **Element 14:** 0xBDCCD20B (about -0.1).
- **All other elements:** 0. So clip w equals view z, and the camera looks along +z.

### 4.2 Screen-centre offset

This step is the same for both states.

- ox = 2 x (centre x pixels - 0.5 x width) / width.
- oy = 2 x (centre y pixels - 0.5 x height) / height.
- The renderer builds an identity matrix whose translation column is (ox, oy, -0.1, 1). The -0.1 comes from 0x42E528.
- It left-multiplies the projection by that matrix (product helper 0x173E0, which replaces its first operand by first x second, column-major). Clip x and clip y therefore gain ox x w and oy x w.
- This is a pure NDC shift. The map is centred on the pixel centre without changing its size. The clip z shift does not affect size.

### 4.3 View matrix (renderer+0x58)

The view matrix starts as identity. Each helper multiplies the current matrix on the right:
1. translate by (0, 0, 300.0) (0x42E7E0), helper 0x2B7A0;
2. uniform scale by s on x, y and z, helper 0x2B580;
3. rotate about z by 0 degrees, helper 0x13050;
4. rotate about x by angle 1, helper 0x2B360;
5. rotate about z by angle 3, helper 0x13050.

Both rotation helpers convert degrees with 0.01745329 (0x42E520).

A map-space point is therefore rotated, then scaled by s about the focus point, then pushed 300 units along view +z. Before tilt, the map plane at the focus lies at view distance 300.

### 4.4 No per-state viewport, scissor or clip

Neither 0x2F44A0 nor 0xE64B0 calls any viewport, scissor or clip-rectangle routine. Their callee lists contain only:
- matrix helpers;
- math library calls;
- mesh draw 0xEE2F0;
- container and lookup helpers;
- the renderer lock and unlock (0x2F3D00 and 0x2F4280, a critical section at renderer+0x1A0);
- unresolved scene virtual calls at +0x08, +0x98, +0xC0, +0x80 (argument 20000.0 from 0x42E5EC), +0x58, +0x60 and +0x48.

The renderer adds no extra scale, viewport or projection term in the corner state. The only per-state branches inside 0x2F44A0 are:
- **Drawn radius.** The radius field renderer+0x19C is set to the signed integer at 0x5469A4 (8). It is halved, using signed division by 2, to 4 when the half-radius byte is nonzero (corner).
- **Cursor pick.** The per-chunk cursor-ray pick that writes the hovered world position at renderer+0x100/+0x108/+0x110 and +0x118/+0x11C runs only when the half-radius byte is zero (full). It does not affect drawn size.

## 5. Drawn extent

- **Block factor.** The block factor is the integer part of 2^(zoom) x 4, where 2.0 is at 0x4AC9D0 and 4.0 at 0x4ACA20, and it is at least 1. The zoom is first clamped to [0, number of level entries - 1]. The level entries are a vector of 0x68-byte entries at renderer+0x170/+0x178.
- **Chunk size.** A map chunk covers (signed integer at 0x5469A0, initial 16) x block factor world blocks.
- **Chunk grid.** The focus chunk index is the integer part of focus position / chunk size, on each horizontal axis. Chunks are drawn from focus index - radius to focus index + radius - 1 inclusive, on both axes. That is 2 x radius chunks per axis:
  - full: 16 x 16;
  - corner: 8 x 8.
- **Chunk placement.** Each chunk mesh is placed with a translation only:
  - (chunk origin - focus) x 1/65536 (0x42E510) / block factor;
  - the chunk origin is offset by -3 x block factor blocks on each horizontal axis.

  So in view units, before s is applied, one chunk spans 16 units.
- **Marker clamping.** Markers farther than radius x 16 x block factor blocks from the focus on either horizontal axis are pulled back to that boundary. In the corner state this uses the halved radius.

## 6. Nominal on-screen half-width at mapScale 1

Before tilt and perspective, with the map plane at view distance 300:
- half-width in view units = radius x 16 x 0.8 x scale argument;
- half-width as a fraction of viewport height = f x (half-width in view units) / (2 x 300) = 0.00402369 x half-width in view units.

At mapScale 1 (steady state):

| State | Scale argument | Radius | Half-width in view units | Half-width in viewport heights | Width in viewport heights |
|---|---|---|---|---|---|
| Full | 1.8 | 8 | 184.32 | about 0.7416 | about 1.483 |
| Corner | 1.0 | 4 | 51.2 | about 0.2060 | about 0.412 |

In general:
- full half-width = 0.7416 x mapScale viewport heights;
- corner half-width = 0.2060 x mapScale^2 viewport heights;
- the full/corner ratio at mapScale 1 is 3.6.

"Viewport height" is the scene height at scene+0x420. These are nominal square half-widths about the focus. The real grid is offset from the focus by the focus position within its chunk and by the -3 x block factor origin offset (section 5).

## 7. Order summary

1. 0xA5A50, per frame:
   - clamp the frame time;
   - (unrelated work);
   - branch on Game+0x668;
   - ease the centre (Game+0x1E48/+0x1E4C) toward the state target;
   - ease the current scale (Game+0x1E50) toward 1.8 (full) or mapScale (corner);
   - build the flags;
   - call 0xE2E00 with centre pointer Game+0x1E48, scale mapScale x current scale, and the flags.
2. 0xE2E00: if the world condition holds and flag 0x20 is clear, call 0xE64B0.
3. 0xE64B0:
   - lock the renderer;
   - convert the centre fractions to pixels;
   - call 0x2F44A0 (projection, view, chunk and marker draw);
   - submit the view and projection to the backend;
   - draw the overlays (0xE6E60, 0xE7980);
   - unlock.

## Evidence boundaries

- **Proved from original evidence:**
  - the argument mapping (from the call-site assembly at 0xAD9D4, 0xE41A2 and 0xE65C1, and the frame offsets in the 0xE2E00 and 0x2F44A0 prologues);
  - all constants listed, read from the file bytes;
  - matrix element order and product order (helpers 0x173E0, 0x2B7A0, 0x2B580, 0x2B360, 0x13050);
  - the per-state branches in 0xA5A50 and 0x2F44A0;
  - the register-path proof for the 0.3 horizontal constant.
- **Not proved:**
  - What Game+0x668 means beyond "full state". The full/corner identification rests on the full radius plus cursor pick when it is set, and the halved radius when it is clear; its writer was not traced.
  - That Game+0x1D60/+0x1D64 equal the scene width and height (scene+0x41C/+0x420). The closed-form corner pixel position assumes they do.
  - The unit of the frame time, and so the real-time ease rate.
  - Which pixel row NDC +y maps to: that depends on the unresolved backend virtual calls (scene slots +0x58/+0x60) and was not inspected. Whether the corner map sits at the top or the bottom edge is therefore not proved here.
  - Whether the backend applies its own viewport or scissor (unresolved virtual calls), and whether mesh draw 0xEE2F0 clips anything. Neither was inspected.
  - That chunk mesh vertices span 16 units per chunk. Inferred from the placement translation; the mesh builder was not inspected.
  - Run-time changes to the .data globals 0x5469A0 (16), 0x5469A4 (8) and 0x5469A8 (3).
  - Whether the write of 1.0 at 0xA7076 dominates every path. It is the only write in the routine; dominance was not checked exhaustively.
  - The tilt and perspective effects of the angle -120 degrees or less, and actual pixel measurements against a running game. No runtime check was made.
- **Exposure statement.**
  - Before the task, the harness placed in my context the project file AGENTS.md (role and coordination policy text, with no implementation content), a git branch name, and general environment and tool descriptions. I did not open AGENTS.md myself.
  - Files I read: docs/CLEANROOM_AUTHOR.md, and these original evidence files:
    - decompiled/functions/1402f44a0.c, 1400e64b0.c, 14002b7a0.c, 14002b580.c, 1400173e0.c, 1402f3d00.c, 1402f4280.c, 14007d050.c, 140016ee0.c, 14000a7a0.c, 14002b360.c and 140013050.c;
    - targeted ranges of decompiled/assembly.txt (functions 0x2F44A0, 0xE64B0, 0xE2E00 and 0xA5A50);
    - targeted lines of decompiled/calls.csv;
    - raw bytes and the hash of binary/cubeworld.original.exe.unpacked.exe.
  - A directory listing of the decompiled folder showed file names only.
  - I read no replacement-engine source, tests, tools, logs, project memory, other documents, SDK, reconstruction source or web content. No prohibited material was exposed. No game executable was run; PowerShell was used only to hash the binary and read its bytes.
