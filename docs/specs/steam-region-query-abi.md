# Steam region-query ABI and oracle provenance

Focused independent clean-room specification from the owner's original
unpacked Steam executable, native argument loads, output stores and callers.
No engine, test, exporter or mod-DLL source was inspected. No original game
was executed. RVAs are relative to that executable's actual loaded module
base; the analysis image base 0x140000000 is not a promise about runtime ASLR.
This document specifies the call boundary; geographic formulas remain in
`worldgen-terrain.md` §§2.3–2.6 and the landmark sections referenced there.

## 1. Original 0x2ada20 entry and return contract

The observed Windows x64 register contract is:

| Location at function entry | Value |
| --- | --- |
| RCX | World object pointer |
| RDX | Caller-supplied writable output-pair pointer |
| R8D | Signed 32-bit whole-block X coordinate |
| R9D | Signed 32-bit whole-block Y coordinate |
| RAX at return | The same output-pair pointer originally supplied in RDX |

The output occupies exactly **8 bytes**, with signed 32-bit **region X at +0** and
signed 32-bit **region Y at +4**, in little-endian order. These are region-cell
indices, with nominal 8192-block cells. They are not a biome enum, fixed-point
position, landmark block center, two floats or two signed 64-bit values.

The native entry saves R9D and R8D at **0x2ada3e/0x2ada41**, saves the output
pointer at **0x2ada44**, and the world pointer at **0x2ada47**. The coordinates
are read from the low 32 bits with signed interpretation. They are not read
from stack arguments, XMM registers or a pointer passed in R8. There is no
additional unaccounted-for return-buffer argument shifting this observed
register layout.

Calling this entry with an explicit output pointer and ignoring its returned
pointer is compatible with that boundary: a void return declaration does
not by itself explain incorrect output. Conversely, treating the returned
RAX as a packed coordinate pair, or replacing the explicit-output boundary
with a compiler-selected by-value structure-return convention, is not the
observed native call contract.

The caller must provide a non-null writable 8-byte output. The routine does
not return an alias of a world-owned/current-player region buffer. A private
output pair avoids accidental modification of unrelated live world fields.
Input coordinates are by value; native callers can reuse a former coordinate
pair's storage as the output only because their X/Y argument values have
already been loaded separately.

## 2. Landmark-aware meaning, not a player/camera query

Original 0x2ada20 derives its 1024-block landmark-cell coordinates by floor
division of the supplied block X/Y, including negatives. It queries the
big-cell flag through **0x36160** and place kind through **0x2b99d0**.
For a nonzero place kind it obtains that supplied cell's landmark point
through **0x362a0** and the warped squared distance from that point to the
supplied block coordinate through **0x364e0**.

Its landmark radius is **250 blocks** for a small place or **500** for a
big one. Only when the squared distance is strictly below radius squared
and **(1−distanceSquared/radiusSquared)² ≥0.25** does it select the landmark
point as the final query coordinate. Otherwise it retains the original X/Y.
The threshold is float 0.25 at **0x4ac95c**; these tests and arithmetic are
the native float32 operations. This is the existing terrain §2.6 rule,
now explicitly tied to the callable ABI.

At **0x2adb51**, it calls 0x2ad4d0 with RCX=the original world pointer,
RDX=the original output pointer, and R8D/R9D=the chosen block coordinate
(either the supplied point or its selected landmark center). It then
returns the original output pointer at **0x2adb56**.

These entry paths do not fetch the current player, renderer, camera, or a
global current-region pointer. Landmark selection is derived from the
supplied X/Y. A caller that wants the player's region must itself supply
coordinates derived from that player. Original 0x2ada20 does not silently
replace arbitrary caller coordinates by the player's position.

## 3. Original 0x2ad4d0 uses the same output-pair ABI

The base region query has **the same RCX/RDX/R8D/R9D contract and output
layout** as §1. It returns that same output pointer in RAX. Its entry saves
R9D as signed Y at **0x2ad4ed**, the output pointer at **0x2ad4f0**, and R8D
as signed X at **0x2ad4f3**. The original World parameter is not dereferenced
by this base query; retaining it in the declared call boundary preserves
the observed register positions.

The routine performs the point warp and nearest jittered 8192-block region
selection from `worldgen-terrain.md` §§2.3–2.4, without 0x2ada20's landmark
substitution. Its candidate traversal is X outer/Y inner; only a strictly
closer candidate replaces a prior result, so the first candidate wins ties.
It uses the supplied point, not a player/camera location.

At **0x2ad72d** it writes the full output pair to **(−1,−1)** before scanning
candidates. At **0x2ad896** a selected result writes both 32-bit coordinates
as one 8-byte store through the originally saved output pointer. At
**0x2ad8d1** it returns that pointer. Thus a normal call does not merely
return an internal pointer while leaving the caller's pair untouched.

0x2ad4d0 and 0x2ada20 are different legitimate reference fields: the former
is the raw warped nearest-region query, the latter is landmark-aware. They
can disagree around landmark influence areas without an ABI error. Label
which field a record captures; do not substitute one for the other merely
to obtain a passing comparison.

## 4. Related helpers with different argument meanings/order

**0x2ad8f0** is the fixed-position form. Its entry is RCX=World,
RDX=output 8-byte region pair, **R8=pointer to 24 bytes containing signed 64-bit
fixed X/Y/Z at offsets 0/8/16**, and it returns the output pointer in RAX.
The argument loads are at **0x2ad907–0x2ad915**. It reads all three fixed
coordinates, queries the effective block there through 0xeab0, and tests
block flags bit 0x20. It converts fixed XY to block coordinates **toward zero**.
With bit 0x20 clear, it forwards those block coordinates to 0x2ada20. With
the bit set, it derives the landmark cell/point first and calls 0x2ada20 at
that landmark point. The two forwarding calls are **0x2ad9d4/0x2ada01**.
Its effective-block dependence is additional world-state input; it is not
an interchangeable spelling of the four-register whole-block query.

**0x358e0**, the region-neighborhood query, instead takes
**RCX=World, EDX=signed block X, R8D=signed block Y, R9=output pointer**.
Its output is **0x58 bytes**, not 8: nine biome IDs at +0x00, nine squared
distances at +0x24, original block X/Y at +0x48/+0x4c, nearest distance at
+0x50, and nearest biome at +0x54. It does not put a region-coordinate pair
in its first 8 bytes. Supplying an 8-byte output would be too small.

**0x2b9080** maps a region index pair to a scalar biome: RCX=World,
EDX=region X, R8D=region Y, with the integer result in EAX. Its inputs are
already region indices, not block coordinates, and it has no output-pair
pointer. Repeated biome values do not imply repeated region identities.

## 5. Bounded exporter reference and modified-data provenance

For an ABI/provenance record, preserve the original module identity/base,
queried RVA, exact signed block X/Y, supplied world/output pointer identity,
the 8 output bytes and returned output-pointer value. Use separate private
output pairs for raw 0x2ad4d0 and landmark-aware 0x2ada20 results. If an internal
native forwarding observation is available, label the coordinate selected
at 0x2adb51 separately from the caller's original coordinate. No such new
runtime observation was performed for this specification.

The implementer reported that the current original-game capture has
**BigRegions.dll enabled with Scale=2**, and logged anchor
**(−128445739,−15874741)**. This is **reported capture provenance**, not a
mod implementation inspected here. That capture was corrected to 2476
distinct region pairs over 3823 requests, rather than constant output.
Neither Scale nor that anchor is an extra argument in the original entry
ABI above. A modified geographic result must retain its modification/scale/
anchor labels; it cannot establish an unmodified original-function oracle
just because its entry RVA and C++ call signature match.

No conclusion about a particular hook, coordinate remapping formula or
runtime patch mechanism follows from this original ABI trace. The verified
result is that the explicit World/output/X/Y boundary is correct for the
original function and that no automatic player/camera substitution exists
there. Runtime modifications and the original geographic reference remain
separate evidence, without weakening or inventing vanilla expected pairs.
