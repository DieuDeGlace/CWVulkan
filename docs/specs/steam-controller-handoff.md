# Steam controller handoff: identification, map history, aim and region ties

Independent behavioral prose from the native executable/decompile/assembly and
the earlier independent controller specifications. No engine, tests, exporter,
generated build files or owner captures were inspected, and no game was run.
Addresses are RVAs. This makes the remaining-runtime-gaps §§4.17–4.19 handoff
self-contained and completes the previously unspecified region-sort tie order.

## 1. Identification GUI navigation points and activation

Complete verified template construction, placement, labels, slot/icon
and visual states are in **steam-identification-ui.md**.

The identification widget is **G+0x360**, constructed through0x277060.
Draw **0x277250** registers **two points every time the widget draws**,
even when **0x277c00 finds no valid slotted item**. In order they are:

| Point | Widget-local coordinates before placement |
|---|---|
| Identify | trunc32(W/3), trunc32(H−20) |
| Goodbye! | trunc32((W times2)/3), trunc32(H−20) |

W/H are the current **local effective frame width/height**, from
0x32a1d0/0x329e00. Multiplication/division/subtraction is float32 before
each truncation toward zero. The points are not the geometric centers of
the two footer halves and are not conditional on affordability. Both go
through the same **0xf97e0** widget-placement registration helper as the
other manual GUI points: it applies the current widget/root placement
and composed transform, projects with its homogeneous reciprocal, then
truncates each final screen coordinate to signed32 before appending.
Use the shared transform contract, not a second viewport-scale guess.
The exact manual-point transform, footer mouse-local hybrid and text
basis are distinguished in **steam-identification-ui §3.2**. In
particular the point transform is node-world plus pivot, not the
widget's full frame matrix containing bindInverse/framePos.

Their global order is after crafting/details/shop registration and
before answers/inventory in the native GUI tree; interface equipment
points precede all these draw registrations. See remaining-runtime-gaps
§4.17 for the complete tree order. A hidden/skipped ancestor prevents the
draw, but there is no additional navigation-point omission merely because
Identify is currently inoperable. An A release over an unusable point
still cannot identify an item.

Pointer action tests are separate from the point positions. In the
widget's local coordinates, **0x277d70** requires a resolved nonzero-count
category15 item in its selected inventory tab/index, plus strict footer
coordinates **0<X<W/2 and H−30<Y<H**. Goodbye test **0x277cd0** uses
**W/2<X<W and H−30<Y<H**, without the valid-item prerequisite. Edges,
including the exact midline, do not satisfy either strict test.

The ordinary press/release route remains **down0x94900**, which records
whether identification was visible, followed by **up0x965e0 with button0**, which
commits the identification only with the native shown/visibility-snapshot/gate-word,
affordability, valid-slot and category15 gates. Goodbye's successful
release hides both identification and inventory. The controller A's
mouse-down/up synthesis must retain these phases; registration of a
point is not a direct call to a business action. Price, generated result,
inventory consumption and RNG order remain loot-shops §5.
The exact snapshot polarity, unchanged constructor-set+0x1f0 word and
live tab/index binding are clarified in **steam-identification-ui §6**;
the older “button was pressed” interpretation is not its native contract.

## 2. Interactive-map shoulders: list, edges and resulting state

Full-map byte **G+0x668** means an ordinary interactive map is open;
it is independent of NPC Talk or flight/shrine modes. The shoulders index
the local character's **region-history linked list at personal+0x118**,
with64-bit count+0x120. A node has next/previous links+0/+8 and signed32
region XY at+0x10/+0x14. These are not flight destinations or quest IDs.
The UI's selected node is **G+0x6e0**, interface+0x5c8.

History producer **0x2da6c0 /0x2d0610** records the player's current
generated region, except during abilities0x9c/0x9d. A matching first
entry leaves the list alone. Otherwise remove an older matching entry
if present and insert the current region at the front. Thus entries are
unique and most-recent-first; there is no fixed-length trim in that pass.
When the full-map region-label panel changes from hidden to shown,
**0x103200** selects its first node and refreshes the label/arrows. This
initialization does not itself perform the shoulder recenter operation.

In a successful XInput poll inside interactive map mode, **left shoulder
mask0x0100 down edge** calls **0x101860** first: follow next toward older
history unless that next node is the sentinel. **Right shoulder0x0200
down edge** then calls **0x101790**: follow previous toward newer history
unless already on the first node. There is **no wrap or held repeat**;
simultaneous fresh shoulders run left then right. The handler does not
require its corresponding visible arrow to be enabled, and still refreshes
and recenters when an edge hits a history boundary.

Previous-edge bytes **0x551cdd /0x551cde** initialize0 and update from
the current shoulders **only in that map branch**. Gameplay, ordinary
UI, failed polls and Controls capture leave them unchanged. Entering or
reopening the map does not separately reset them. A shoulder held on entry
can trigger when its retained bit is0. Each invoked edge marks the last
device as pad. Right shoulder also retains its separate held Normal-Attack
mapping under the native input routing contract.

After a possible selection change, **0x1081b0** refreshes the generated
region name. Its left/older arrow is shown only when a next node exists;
the right/newer arrow is shown only off the first node. **0x2b9210**
computes the selected region's deterministic block point, using the
existing region-position generator: each axis is region coordinate
times8192, plus1024, plus its salted region hash modulo6144; salts are
0xe29e/0x70532. It is not a region bounding-box midpoint.

**0xa3390** sets target map center **G+0x1e08/+0x1e10/+0x1e18**: X/Y
are those block integers times65536; Z is **0x35ea0(blockX,blockY,mode0)**
times65536, truncated to signed64. It sets manual-center byte+0x1e38=1.
The shoulder handler raises signed32 zoom **G+0x1e04 to at least4**.
It preserves a larger zoom and leaves drawn center+0x1e20 to ordinary
smoothing. It does not start travel or alter flight/shrine state.

The handlers have no explicit empty-history fallback. Normal history
production supplies an entry; an artificially sentinel-only list does
not establish a meaningful destination. Do not invent a region0 shortcut
as an observed native behavior for that malformed/preinitialization case.

## 3. Gameplay aim integration has two distinct effects

Sources: **0x8ae50**, **0xa5a50**, **0x17840**, **0xadcd0**. Each frame
clears **target creature ID G+0x1e58** and projected-box byte **+0x1e88**.
The latter means an eligible creature's projected box spans the crosshair.
It is set **before line of sight**, so can be true without selecting a
target. Candidate filters, projected corners, line of sight and strict
perpendicular-distance ranking remain use-prompt §§2.1–2.2. Equal ranking
keeps the earlier candidate. This state is rebuilt, not a latched lock-on.

### 3.1 Movement yaw assistance

This occurs within0x8ae50 after creature selection and its earlier
movement/ability/ultimate stage, before static-object/ground-item targeting
and nearby fallbacks. **Late primary/strong/class/standing-dodge handling
is later**, after direct-use work and its ordinary-UI gate; “attack/skill
handling” here must not move that late region ahead of assistance.
Whole-frame admission, including the **world-step batch before0x8ae50**,
is [gameplay frame order](steam-gameplay-frame-order.md). Require
current nonzero creature target to resolve with **HP>0**, and filtered
**LEFT movement stick G+0x1d6c/+0x1d70 squared length strictly greater
than float32(0.1)**. It adds no right-stick, held-aim, last-device,
projected-box-byte or separate UI/map gate. Existing admission to the
player-input routine and its target filters still apply.

Direction D begins at **ray origin G+0x1dd8/+0x1de0/+0x1de8** and points
to target **drawn fixed XYZ+0xb10/+0xb18/+0xb20**, converting differences
to blocks. Add **0.45 times target body Z+0x8c** vertically. Add target
velocity **+0x34/+0x38/+0x3c times lead seconds**, where lead is the
distance from the player's physical position to the target's drawn
position divided by **150 if0x5eca0 is true, otherwise500**. This uses the
existing ranged predicate, not a per-weapon projectile-speed lookup.

Use existing camera forward **F=G+0x1ec4/+0x1ec8/+0x1ecc**. For nonzero
horizontal vectors, desired yaw is **displayed yaw G+0x1d90**, plus
**asin(normalized Fx times Dy minus normalized Fy times Dx) times180/pi**.
The analogous pitch calculation does not update pitch target here.
For nonzero full F/D, normalize them and let m be their cross-product
length. Blend is **(1−exp(−rate times dt)) times exp(−20 times m)**,
dt in milliseconds, rate float **0.014999999664723873** when player
float+0x12c is zero, otherwise **0.029999999329447746**.

Apply0x17840 to **old yaw target G+0x1da8 narrowed to float**, desired
yaw and that blend. Store its float result widened to double at+0x1da8.
Do not directly overwrite displayed yaw or recast/rotate the ray here.
The desired angle uses displayed yaw; the old blended angle is the
separate target. A later object/item target does not undo this adjustment.

The helper0x17840 uses sine/cosine direction pairs of float angles in
degrees. With their double dot product at least **−0.8999999761581421**,
return old angle plus blend times **asin(clamp(sin(new−old),−1,1))
times180/pi**, retaining native float rounding. Below that dot threshold
return the new angle directly. It is not a generic linear-angle or
atan2 shortest-angle interpolation.

### 3.2 Manual right-stick slowdown

Later in0xa5a50, use the **then-current** target ID. Require+0x1e88
nonzero and a resolved living target. Distance starts at **camera eye
G+0x1dc0**, not player position or ray origin. Multiply the ordinary
cubic right-stick look rate by **max(1−distance/50,float32(0.3))**.
Movement is not required. Conversely movement assistance can run without
this slowdown because it has different conditions.

Object/item targeting and nearby fallbacks can replace/clear the target
without clearing+0x1e88. A visible **general DialogWidget G+0x418**
also replaces+0x1e58 by its stored speaker ID+0x210 before this slowdown,
without replacing the projected-box byte. A general page with retained
speaker ID0 therefore does not become an invented NPC aim lock.

Order is target rebuild/creature test, movement yaw help, object/item and
fallback targeting, general-widget speaker override, manual stick update,
then ordinary displayed-camera smoothing0xadcd0. Assist/manual elapsed
uses the **double frame milliseconds capped at1000**, not the UI successful-poll
timer. These local camera adjustments send no attack/quest/network event.

## 4. Inventory region-distance tie order, including more than32 choices

**0x2806a0** takes each nonzero-count item region from the currently selected
ordinary inventory tab and always adds the player's current region.
Its intermediate set deduplicates pairs in **signed lexicographic X/Y**
order. The final sort compares only **Chebyshev distance to current player
region**: maximum of absolute X and Y differences. There is no coordinate
or insertion-ID secondary key. Coordinate subtraction wraps modulo2^32;
absolute-value negation is also32-bit and the maximum comparison is
signed. Thus the exceptional word0x80000000 remains signed INT_MIN after
the native absolute-value operation. Ordinary valid region coordinates
use their usual nonnegative distances. This source is not an unsigned-coordinate
lexicographic set or a Euclidean-distance ranking.

The inventory's selected pair **+0x1f0/+0x1f4** persists across ordinary
region-vector rebuilds and tab changes. Opening inventory through0x102fe0
initializes it to the player's current region. The rebuilt vector is at
**+0x1f8/+0x200**. Visible region filtering applies to inventory tabs0/1;
the controller can still cycle this shared selection on the other tabs.

Sort **0x27c420**, called with initial budget equal to entry count,
has these verified branches. Entries are whole8-byte coordinate pairs.
Subranges of **32 or fewer** use stable insertion **0x27b780**, moving
an entry left only across strictly greater keys. Thus small-input ties
preserve the intermediate lexicographic order. Above32, the following
partition swaps determine order instead of a synthetic tie-break.

### 4.1 Pivot sample order

Partition **0x27bd00** takes middle index **floor(N/2)** and last index
N−1. Sample helper **0x27b6a0** uses one ordered triple (first,middle,last)
for **N≤41**. For **N≥42**, let s=floor(N/8). In this order, order triples
**(0,s,2s)**, **(middle−s,middle,middle+s)**,
**(last−2s,last−s,last)**, then their median positions **(s,middle,last−s)**.
These are actual in-place sample rearrangements, not detached key samples.

Triple ordering **0x27bb60** first exchanges its first/middle entries
only if middle is strictly smaller. It then exchanges middle/last only
if last is strictly smaller; after that latter exchange it again tests
middle against first and exchanges only if strictly smaller. Equal-key
pairs do not exchange in that individual comparison. The resulting
middle entry supplies the pivot key.

### 4.2 Three-way partition movement

The partition first expands a contiguous equal-pivot band around middle,
leftward and then rightward. It then searches from that band toward the
right and toward the left. Right search skips strictly greater entries,
gathers equal entries at the band's upper end by whole-entry exchange,
and stops on a smaller entry or the end. Left search skips strictly
smaller entries, gathers equals at the lower end by whole-entry exchange,
and stops on a greater entry or the beginning. Right search occurs before
the corresponding left search on each iteration.

With a greater entry available on the left and a smaller entry on the
right, exchange them and continue the searches. When both searches reach
their array boundaries, return the equal band; it is excluded from further
sorting. When only one search has exhausted its side, the band rotates
to create room, in these exact orders:

- **Left side exhausted:** if the equal band's upper end differs from
  the stopped right-search entry, exchange the band's first entry with
  its upper-end entry. Then exchange that same first-band entry with
  the stopped right-search entry. Advance both band bounds and right
  search by one entry.
- **Right side exhausted:** move left search and the lower band bound
  one entry left. If those positions differ, exchange their entries.
  Move the upper band bound one entry left and exchange its entry with
  the new lower-bound entry.

These rotations are what can permute coordinate ties. No later stable
pass restores their original order. Remaining lower/higher partitions
are sorted independently: recurse on the shorter; on equal sizes recurse
on the right, then continue on the left. Each partition changes the
budget to **half=floor(oldBudget/2), then half+floor(half/2)**. This is
the two integer truncations, not an unqualified floor(0.75 times budget).

### 4.3 Heap fallback and equal-child choice

If a subrange still exceeds32 and budget is below1, use heap creation
**0x27b9a0**, then extraction **0x27c360**. Build a maximum heap by
processing parents **floor(N/2)−1 down through0**. Save that parent's
whole entry, move the larger child up repeatedly to the hole, choosing
the **right child on equal keys**. A final left-only child is moved up
when present. Then insert the saved entry upward, moving parents down
only while their key is strictly smaller; equal parent keys stop it.
During heap **creation**, upward insertion is bounded by the **original
parent index being repaired**, not root0. During **extraction**, repair
starts at root0 and that is its upward limit. This distinction is explicit
in0x27b9a0's top-index argument to0x27c230 and the corresponding finite-float
repair in0xc8b50/0xca310; the held entry must not escape its build subheap.

Extraction moves the current root to the final unextracted position,
saves that position's prior entry, shrinks the heap by one, and repairs
from root with the same larger-child/right-on-equal descent and strict
upward insertion. Continue until one entry remains. Repair helpers are
**0x27c0f0 /0x27c230**. Neither heap phase imposes a coordinate tie-break.

### 4.4 Small independently derived tie oracle

This is a **static arithmetic derivation, not a native runtime capture**.
Take player region(0,0), all32 distinct region pairs on the square shell
**max(abs(x),abs(y))=4**, plus the mandatory current-region pair(0,0).
Sort the33 input pairs signed lexicographically and call their positions
0 through32. The current-region pair is input16; input0 is(−4,−4).

Native output is the original inputs in this order:
**16, then1 through15, then0, then17 through32**.
The triple first/middle/last swaps the zero-distance input16 with input0;
the remaining radius4 values become one equal band and need no further
sort. Thus the first shell choice is **(−4,−3)**, while stable distance
sorting would have retained(−4,−4) first. This provides a bounded fixture
that detects replacing the native larger-input sort with a stable sort.

## 5. Scope

The identification points/actions, history shoulder edges, two camera
effects and complete region sort behavior above are stable. Underlying
target-box/line-of-sight arithmetic and shared GUI placement already have
independent specifications cited above. No additional controller
binding, empty-history destination or source event is inferred here.
