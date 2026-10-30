# Steam palette provider ABI and bounded decoded oracle

Independent clean-room recorder contract from the owner's native functions,
assembly and data layout. No engine/test/exporter sources were read and no
original game was run. This document specifies calling/observation inputs;
the independent implementation formulas remain in `steam-scene-lighting.md`
§§2/5 and `sky-and-clouds.md` §B.3. Addresses are RVAs in the inspected image.

## 1. Callable provider interface

**0x2b0400** has9 arguments, returns **void**, and writes seven distinct RGBA
outputs. It receives no renderer pointer, frame duration or simulation delta.
Use the ordinary Windows x64 ABI; all arguments here are pointers, not
floating values passed in XMM registers. At **function entry**, stack offsets
below include the8-byte return address. A normal compiled caller supplies
the ABI's shadow space/alignment; these offsets aren't a custom calling scheme.

| Argument | Entry location | Value/type |
| --- | --- | --- |
|1|RCX|Pointer to fully constructed native World |
|2|RDX|Pointer to three consecutive signed64 camera XYZ,65536 units/block |
|3|R8|Pointer to16 writable bytes: sky1 float32 RGBA |
|4|R9|Pointer to16 writable bytes: sky2 float32 RGBA |
|5|Stack+0x28|Pointer to16 writable bytes: fog float32 RGBA |
|6|Stack+0x30|Pointer to16 writable bytes: sun float32 RGBA |
|7|Stack+0x38|Pointer to16 writable bytes: front float32 RGBA |
|8|Stack+0x40|Pointer to16 writable bytes: back float32 RGBA |
|9|Stack+0x48|Pointer to16 writable bytes: ambient float32 RGBA |

Allocate seven separate16-byte output buffers, preferably16-byte aligned,
and an8-byte-aligned24-byte camera buffer for a query. Preserve the input
camera as signed64 integers; a pointer to three floats/doubles isn't compatible.
The return register has no RGBA-result contract. The output order deliberately
keeps **fog fifth/front seventh**; the earlier swapped label was corrected in
the lighting spec. Colors can exceed1 or have alpha other than1; preserve all
four floats and their raw IEEE bits without clamping them for serialization.

This is a CPU provider. A correctly initialized World is required even when
only its time-dependent outputs are of interest: its region/height/block
helpers read native world structures. The provider's player-dependent branches
explicitly allow **World+0x448=null**, producing no far-flight/effect13 override;
a fake partially initialized Creature is not a substitute for that null case.

## 2. Native execution context and lifetime

The preferred oracle observes the **existing** call from0xe2e00. It runs in
the game-frame context while World's critical section **+0x3c8** is held:
0xa5a50 enters it via0x2abb70 before update/render and leaves through0x2b9890
afterward. The provider and its effective-block lookup do not acquire a
replacement lifetime/map lock of their own. In particular0xe9a0 directly
looks up World+0x460. This trace establishes the normal frame-call context;
it does **not** establish arbitrary concurrent worker-thread calls as safe.

For a direct query use a live, fully initialized native World in its normal
owning-thread phase, or the already established native-world construction
path of an owner harness. No new fake-World layout/construction recipe is
specified here. A live pointer read from a global is not by itself a lifetime
pin. Keep World, any nonnull local Creature and loaded-zone structures alive
for the entire call and copy outputs before returning their buffers to reuse.
An instruction-site observer must preserve all live registers/SIMD values and
flags; ordinary function nonvolatile preservation alone is insufficient.

Loaded/constructed native runtime data matters. For example static initializer
**0x1100–0x111d** sets the six-byte missing-zone block at0x54dcb0 to
**(200,200,200,0,1,0)**. Its bytes in the unpacked file are zero before runtime
initialization. Treating an unpacked image's file data as a fully initialized
running world would change effective-block behavior. A query needn't invent
loaded terrain merely to avoid a miss: the initialized native lookup has an
explicit fallback. Preserve whether the camera's owning zone was actually
present so that fallback observations are labeled.

## 3. Obtaining current decoded World and camera data

Supported by the native call path and field assignments:

- **Live Game global:** read the pointer at module base+0x551a80. For a valid
  Game, its World pointer is **Game+8**, renderer pointer **Game+0x18**, and
  its current camera fixed XYZ are signed64 at **Game+0x1dc0/+0x1dc8/+0x1dd0**.
- **Renderer entry observation:** at entry to0xe2e00, RCX is renderer, RDX
  points to the current fixed camera triple. Renderer+8 is World. This call
  copies the triple to renderer **+0x338/+0x340/+0x348** before the provider.
- **Provider entry observation:** RCX and RDX already give the exact World
  and camera arguments, so no global rediscovery is necessary.

Cross-link frame IDs and decoded values rather than requiring native pointer
identity in replay. For a same-frame observation the renderer's World should
agree with the provider's World. **The local-player position is not the camera**.
Game+0x1dd8/+0x1de0 supply the render-origin-related XY used with opposite sign
at renderer+0x428/+0x430; don't mistake that pair for camera XYZ. Matrices aren't
needed to evaluate provider targets, though the frame oracle may retain them
for separate visibility work.

All three camera coordinates are signed64 fixed; world time at **World+0x0c**
is float32 milliseconds-of-day. Preserve that actual float, not a normalized
0..1 day fraction or wall-clock time. Also retain public world seed+0x220,
actual zone seed+0x244 and terrain values+0x330/+0x334 as provenance.

## 4. Existing-call observation points and outputs

The call instruction is **0xe33fa**; its return point **0xe33ff**, before scene
easing, is the stable target-capture point. Renderer is in **R14**. In this
caller only, outputs at return are frame-RBP-relative:

| Output | Caller buffer |
| --- | --- |
|Sky1|RBP+0xd0 |
|Sky2|RBP+0xc0 |
|Fog|RBP+0x58 |
|Sun|RBP+0x68 |
|Front|RBP+0x28 |
|Back|RBP+0x38 |
|Ambient|RBP+0x48 |

Copy all16 bytes per output immediately. These caller-stack offsets don't
apply to an arbitrary direct query or to provider-entry RSP. Immediately
**before** the call at0xe33fa, its fifth through ninth pointer arguments sit
at current stack+0x20/+0x28/+0x30/+0x38/+0x40; this is8 less than the entry
offsets in §1. At provider entry they are as in §1.

For a frame-transition oracle, copy the previous renderer RGBA immediately
before this call and copy updated RGBA at **0xe3966**, after all transitions
and the sun-direction construction but before its next renderer call.
At0xe3966 R14 remains renderer and the normalized sun float3 is at frame
RBP+0/+4/+8. Renderer fields are sky1+0x4ac,sky2+0x4bc,fog+0x4cc,sun+0x4dc,
interior override+0x4ec,ambient+0x4fc,front+0x50c,back+0x51c,material+0x530.
The **+0x4ec value is an override input, not an eighth provider output**.

The renderer's float frameMS is+0x330. It can also be observed as argument7
at0xe2e00 entry, float32 at entry stack+0x38. For the separate FOV precision
case,0xadcd0 entry receives Game in RCX and the capped **double** frameMS in
XMM1. The producer/cap and first-frame details are in the lighting spec §5;
do not replace a target-only oracle with values already eased using that time.

For an independently replayed **interior frame override**, also preserve the
local player's physical fixed XYZ and effective block there, plus the computed
+0x4ec. Its producer tests the **player block's flags0x02 and0x20**, not merely
the camera block used by the palette provider. The player's block/noise inputs
and camera's block/region inputs are separate even within one frame (§2.6 of
the lighting spec). Capturing+0x4ec alone is sufficient to check its downstream
easing, while the player data is needed to check its independent production.

The scalar renderer+0x52c is **not a provider input**. It still has its base
day value at0xe33ff/0xe3966 and is multiplied by the regional factor later.
If recording it for the collector, capture the post-multiplication value
at **0xe3bb4** and label its phase. This avoids comparing a base-day scalar
with a regional value used by the point-light decisions.

## 5. Independent input record for each target sample

A bounded target case should preserve the following decoded values, with
presence/validity rather than fabricated zeroes for unused optional fields:

1. Image fingerprint/schema and frame ID; source role (`existing_frame_call`
   or `direct_native_query`); World seed provenance; camera fixed XYZ; native
   World time float32; seven target float4s in the named order.
2. **Truncated camera block XY**, toward zero, used by0x358e0,0x35ce0 and
   mode0 height0x35ea0. Preserve their actual native output records or results.
   These are distinct from the floor-rounded XYZ used by the effective block
   getter. Negative sub-block coordinates are useful explicit test cases.
3. The effective **camera block**, six unsigned bytes R,G,B,glow,type,flags,
   obtained by0xeab0(World,out6,cameraFixedX,cameraFixedY,cameraFixedZ). This
   helper returns out6 and internally floors all three fixed coordinates.
   Its type2 test and flags-byte bit0x20 are actual palette branch inputs.
4. Local-player presence. If present, effect13 presence and float remaining
   duration at effect+8, obtained through0x5e3a0(Creature,0x13). When no effect
   exists, remaining duration has no valid native value to compare.
5. For the far-flight branch: ability byte Creature+0x68. When it is0x9c,
   also preserve physical fixed XY+0x10/+0x18, signed32 block anchor
   XY+0x1b0/+0x1b4 and fixed XY+0x1b8/+0x1c0. Retain0x2b8050's bool result
   as an optional observed label; these are not camera coordinates.
6. For the interior structure-color choice, preserve big-structure result
   from0x2aa270 and the exact place kind,big flag and place-cell arguments
   used for it. Its cell coordinates come from floor-dividing the already
   truncated camera blocks by1024. Keep the pre-call coordinates even though
   that helper internally rounds a big place to even.

For region verification, **0x358e0(World,blockX,blockY,out88)** fills a0x58-byte
neighbourhood:9 signed32 biome IDs at+0,9 float squared distances at+0x24,
signed32 input XY at+0x48/+0x4c,nearest float squared distance+0x50 and nearest
signed32 biome+0x54. Its array order is X outer/Y inner. A96-byte scratch
allocation in a caller doesn't make its last8 bytes meaningful output.

**0x35ce0(World,blockX,blockY,out32)** fills the camera-place record: signed32
kind+0,byte big+4,signed32 variant+8,radius+0x0c,center block XY+0x10/+0x14,
float distance²+0x18 and weight+0x1c. Padding+5..7 isn't semantic data. This
helper always writes the center XY; radius is meaningful only when kind
nonzero. Kind0 has distance/weight0. This differs from the initial zone-place
helper0x31a5d0, whose distance/weight fields are not completed at all.
Capture only defined fields as equality inputs, while optional raw bytes may
remain diagnostics. Record mode0 ground height as its returned float32.

Warmth,dryness,lava and forest values may be included as **native derived
diagnostics** from the helpers named in the lighting spec. They are valuable
for localization but shouldn't replace raw camera/time/seed/block/player
inputs in the oracle. For the target equation replay, preserve original
dryness and the independently returned ground height, not only the already
sea-attenuated dryness.

For such optional native diagnostic queries, the compact ABI is:
**0x2d9b60/0x2b9750/0x2d9f30(neighbourhoodPointer,placeKind signed32,
placeWeight float32)→float32**, and **0x2b5960(neighbourhoodPointer)→float32**.
These helpers have no World first argument; the third mixed-signature float
is passed in XMM2 and the float result returns in XMM0. The mode0 height query
is **0x35ea0(World,blockX signed32,blockY signed32,mode signed32)→float32**.
Its mode is0 here. Do not use a decompiler's missing/void floating-return
annotation as the callable ABI.

## 6. Bounded collection and unresolved scope

An initial explicit limit such as256 provider samples and one copy per selected
frame is sufficient for a compact oracle; the limit is recorder policy, not
a game constant. Include ordinary day/night, interior flag0x20 with both
structure-color branches, water,effect13 and far-flight labels when naturally
available. Mark uncovered branches. Store the7×4 outputs in fixed order with
IEEE bits and decimal conveniences; preserve alpha and values above1.

This contract establishes the callable ABI, observed normal synchronization
context and decoded inputs. It does not establish a safe arbitrary remote-
thread invocation recipe, a substitute native World constructor or immunity
to world teardown during an unpinned read. Those are concrete untraced
integration requirements if an exporter chooses that route. Observing the
existing frame call avoids inventing such a protocol. No oracle execution
or original-game launch was performed for this document.
