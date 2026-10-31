# Steam InventoryWidget physical grid and scrollbar

Independent native-only supplement to steam-gui-item-icons9 and
remaining-runtime-gaps4.15. Sources:0x27c950,0x27d2b0,0x27fce0,
0x280a40,0x280c80,0x27f7a0,0x280270 and0x280e30, with native
instructions/constants. No engine, tests, exporter/build output, SDK,
owner assets or captures were inspected; no game was executed.

## 1. Distinct indices and dimensions

Collection**widget+0x1e8** points to24-byte backing-tab vector entries;
each tab stores ordered **164-byte physical slots**, with signed32
quantity and the160-byte original item at slot+4. Selected tab is
signed32**+0x2b4**, mode signed32**+0x280**. Do not derive these indices
from a flat presentation list of nonempty items.

Cell width/height are signed32**+0x2a8/+0x2ac**. Constructor0x27c950
defaults both to40 and, when its item-frame template exists at+0x228,
replaces them with trunc32 of that template Widget's transformed queries
0x32da30/0x32ad10. Preserve the live values from that path.

In draw0x27d2b0 and refresh0x280a40, use **local effective** width from
0x32a1d0 and height from0x329e00. Columns C are trunc32 of the float32
quotient (width−10)/(cellWidth+5). Visible rows R are trunc32 of
(height−40)/(cellHeight+5), with float32 subtraction/division and integer
addition of5 before converting the divisor. These are not owner drawing
AABB extents or nonempty-item counts. The local-to-screen basis used for
actual cell drawing is separately described in section3.

Per-tab signed32 first-row values occupy the vector**+0x288/+0x290/+0x298**.
The current hover pair is **+0x270/+0x274**, and the independent click-selected
pair is **+0x278/+0x27c**. Each pair is a canonical **tab/physical-slot**
identity. The available row-offset count**+0x2a4** is one signed32 value
for the current tab, not a number of occupied cells or a maximum offset.
Constructor0x27c950 initializes hover to(−1,−1), selected pair to(0,0),
the row-offset vector empty and available count to1.

## 2. Exact draw interval, holes and hover

Draw0x27d2b0 first updates its region-arrow visibility, then clears byte
**+0x2a0**. A null collection returns **before** resetting hover. For a
nonnull collection, it immediately resets both hover fields to **−1**,
before the selected-tab bound check or cell loop. Thus a reached valid
draw with no hovered cell clears an old hover, but a null-collection
return retains the old pair. A hidden/not-drawn Widget has no implicit
new hover reset from this draw routine. Neither refresh0x280a40 nor
scrollbar0x280c80 supplies a separate hover reset.

For a maintained nonnegative valid tab, first physical slot S is its
stored first-row multiplied by C. The rectangular viewport's exclusive
end V is **S plus R times C**, with native signed32 integer arithmetic.
First compute E as the lesser of V and the tab's physical slot-vector
length. For **mode0 and tabs0/1 only**, scan from **S up to E**, in
increasing physical index. Ignore quantity0 slots for the following
region test. At the **first quantity!=0 slot** whose original item
region+0x0c/+0x10 differs from widget region+0x1f0/+0x1f4, replace E by
that index and stop scanning. The mismatching slot is excluded.

This scan **does not start from physical slot0**, does not search for
the first matching-region start, and does not compact matching items or
holes into new cells. A mismatch before S does not cut this viewport;
later matches after the first mismatch inside it do not resume it.
Other modes/tabs have no such region-end scan. Sources are
**0x27d5a3–0x27d79a**, with the scan's visible starting index.

The icon/navigation rectangular loop still visits **every index in
[S,V)**, row-major, even after E or the physical vector ends. It registers
one navigation point for each such rectangle, including unused tail
rectangles. Only indices **below E** enter the backing-record/hover/icon
branch. That branch tests the screen rectangle and, on a hit, stores
the physical tab/index **before testing quantity**. Therefore a valid
empty hole below E can be hovered and navigated; unused rectangles at
or after E register points but do not acquire a backing hover pair.

After hover handling, ordinary icon submission requires quantity!=0;
mode2/3 instead submit all usable records below E even at quantity0.
There is no category!=0 test at this icon caller. Text has its own
admission in section6. The clicked/selected pair is not rewritten by
merely drawing or scrolling the grid. Existing0x280e30 adopt-hover and
selection-frame rules remain remaining-runtime-gaps4.15.

The native intended layout has positive C and nonnegative row offsets.
This draw path does not clamp a negative offset up to0 or clamp S up to
the matching region. Negative offsets can address before the backing
vector and are outside safe native input; preserve their absence of a
native repair rather than inventing a valid negative-index item.

## 3. Screen rectangles and navigation rounding

Use frame origin O from**0x32cf50(widget)**. Independently compose the
node's parent-world matrix with its root's live local matrix through
0x100fa0/0x173e0. The scalar m used here is the resulting **M00**, with
its sign; it is not abs(M00), a vector length or a second global GUI scale.

Relative to S, column and row are the signed integer remainder/quotient
of (slot−S) by C. Local cell left is **10+(cellWidth+5) times column**;
top is **40+(cellHeight+5) times row**. Perform these integer operations,
convert to float, multiply by m, add the matching O component, truncate
toward zero to signed32, then convert back to float. These separately
snapped screen left/top are the native hover rectangle origin.

Compare cached GUI cursor float XY directly against **left/top inclusive,
right/bottom exclusive**. Right is snapped left plus float(cellWidth)
times m; bottom is snapped top plus float(cellHeight) times m. These
extents/endpoints are **not truncated again**, and there is no extra
inverse-frame or shape-outline hit test in the cell branch. In particular
negative m is not converted into a reordered positive rectangle.

Icon center adds **float(cellHeight divided by2 as signed integer) times
m to both snapped left and snapped top**, even for nonsquare cells.
The navigation-point center separately truncates those resulting float
center components to signed32. This does not replace the float icon
center or continuously compared hover rectangle. Base icon size, hover
factor, category9 factor and animation/shader arguments are unchanged
from steam-gui-item-icons9.

## 4. Refresh counts and scroll limits are separate from the draw end

With a nonnull collection, **0x280a40** first rebuilds its region list
through0x2806a0 and invokes0xb3ed0 on the local Game through widget+0x210.
Its existing canonical sorting effects remain slot-lifecycle8. Then it
calculates C/R as in section1. If **C≤0**, it returns without resizing
the row-offset vector or assigning+0x2a4; the earlier refresh/sort work
has already run. No forced minimum-one-column substitution appears.

For positive C, resize the row-offset vector to the backing tab count:
existing entries survive, new entries are0. A valid current tab with a
**physically empty vector** sets available count A to0 and its row to0,
then returns. This differs from a nonempty vector containing only holes.

For a nonempty physical vector, choose the count input N as follows:

|Mode/tab|N used for scrollbar range|
|---|---|
|mode0, tab0 or1|count **every quantity!=0, region-equal record across the entire physical tab**|
|all other cases|the tab's complete physical slot-vector length, including holes|

The first row does not affect that full-tab count. The regional scan
does not stop at a foreign-region record, measure its matching prefix
span, or count quantity0 holes. This is intentionally **different from
section2's physical viewport interval**. Preserve both rules; neither
can replace the other with a common compacted visible-item list.

For regional mode0 tabs0/1, divide signed32 **N−1 by C**, truncating
toward zero. For the other case, the native nonempty physical length
minus1 is divided by positive C with unsigned64 division before its
result is used as signed32. Under normal finite native vector lengths
both yield the corresponding nonnegative last-row quotient; the regional
**N=0** case specifically uses signed−1/C, not unsigned underflow.
Available count A is **that quotient minus R plus2**, then clamped
upward to at least1. Sources **0x280b89–0x280c3f**. This counts allowed
first-row positions; maximum offset is **A−1**. R is not clamped to0:
a zero/negative visible-row calculation participates in this arithmetic.

Clamp only the current tab's stored row **down to A−1 when larger**;
there is no lower-zero clamp and no simultaneous clamp of other tabs.
A missing/invalid current tab sets A to1 without resetting a particular
row. Negative selected-tab indices are outside the maintained safe
contract; the native signed upper-bound test does not provide a safe
lower-bound repair. A physically nonempty regional tab with no matching
nonzero records still uses the formula and A≥1, not the empty-vector A0
case. For N0 and C>1, trunc(−1/C) is0; for C1 it is−1.

**0x27f7a0/0x280270** each call that refresh first. Next row increments
the current stored offset only when below A−1; previous row decrements
only when above0. A changed offset requests sound0x59, calls selection
frame0x280e30 with adopt-hover0, then scrollbar0x280c80. A boundary
attempt still did its initial refresh, but has no sound or these two
follow-up calls. It does not advance a canonical selected-slot index
to preserve an on-screen cell.

## 5. Scrollbar and layout arithmetic

Scrollbar**0x280c80** requires collection nonnull, thumb node+0x248
nonnull, **A!=0**, and a nonempty row-offset vector. Otherwise it returns
without changing thumb geometry or hiding/deleting it. It does not
recompute A. The helper assumes its current tab can index that vector;
nonempty vector alone is not an independent valid-tab check.

Track length T is **trunc32(local effective height−70.0)**, constant
**0x428c0000 at0x42e7bc**. Thumb height is signed integer **T divided
by A**, truncating toward zero. Thumb X is local effective width minus
float **28.0**, word0x41e00000 at0x46e620. Preserve the thumb Widget's
current **local effective width**, queried before setting its frame.
For **A≤1**, Y is literal float **35.0**, word0x420c0000 at0x441bc0.
For A>1, first calculate signed32 **(T−thumbHeight) times currentRow**,
then signed-divide by A−1, truncating toward zero, then add integer35
and convert to float. Thumb height converts to float only after its
integer division. Submit that X/Y/width/height through **0x32c680**,
position first through0x32b250 with byte0, then size through0x32c6f0
with propagation byte1; do not substitute
a floating continuous percentage or globally scaled screen rectangle.

There is no minimum-one-pixel thumb height, negative-T clamp, extra
round-to-nearest or explicit thumb-visibility assignment here. T smaller
than A can produce height0. A0 skips the helper entirely; a negative A
would pass its nonzero gate and use its signed division/A≤1 branch,
although native refresh's admitted values are nonnegative. A negative
currentRow is likewise not corrected by this layout helper.

Resize/layout**0x27fce0**, when item-frame template+0x228 exists, refreshes
the Widget, recomputes C/R and rebuilds its cell-frame nodes in **column-
outer, row-inner** creation order. Each frame is placed at local
**(10+(cellWidth+5)×column,40+(cellHeight+5)×row)**, with per-instance
root transformation/display made unique. That owner cell-frame creation
order is separate from row-major icon/navigation submission. Its optional
up/down arrow frames are placed at **(width−30,10)** and
**(width−30,height−30)**. Region arrows are placed at **(15,height−25)**
and **(width−60,height−25)**. These constants are verified at
**0x280043–0x280129**, using float10/30/25/15/60 at
0x42e598/0x42e5a8/0x42e5a4/0x42e7a4/0x42e7b8. All width/height terms here are local
effective queries, passed to the normal frame setter; they are not new
physical-pixel placements. The resize hook calls0x280c80 after its other
frame placement, not a second occupied-item row calculation.

## 6. Bounded cell text facts

These are original0x27d2b0 manual text submissions after all grid icons
and the Plasma pipeline restore. They are **local text origins/baselines
to0x269210**, not the snapped screen icon rectangles. Font is the
constructor-selected resource1.dat, size12 with black outline radius3.
The two-pass native text transform is plasma-shapes-and-text7.1.

For a physical usable slot below E, let c/r be its viewport column/row.
The main name origin X is **(cellWidth+5)×c+cellHeight+20**. Its baseline
differs by mode:

|Mode|Name admission|Baseline Y|Draw call|
|---|---|---|---|
|0 inventory|original category!=0, independently of quantity|(cellHeight+5)×r+cellHeight+10|0x27e9f1|
|2 crafting|original category!=0, independently of quantity|(cellHeight+5)×r+cellHeight+13|0x27e701|
|3 shop|original category!=0 and quantity!=0|(cellHeight+5)×r+cellHeight+10|0x27eeaf|

The names use full helper0x1628b0 and word-wrap flag0x10 with float wrap
width **cellWidth−cellHeight−20**, computed in signed32 then converted
to float. Color is **(1,0.25,0.25,1)** if class admission0x1094d0 fails;
otherwise0x10a600(original item,current player region), whose exact
palette is steam-ring-selector-rendering8.1. This does not alter the
original item's category/quantity or the hover interval.

Inventory mode0 appends **` (` plus signed decimal quantity plus`)`**
to its name for categories in0x5f0e0's set{1,10,11,12,13,22,28}, without
an added positive-count condition. Crafting mode2 appends that quantity
suffix only for quantity>0 and tab!=6. Shop mode3 appends it only for
quantity>0. Negative shop quantities can therefore have a name without
a quantity suffix; zero shop quantities have no name while their icon
caller still admits them. These are name-string suffixes, not repeated
icons or newly inferred corner quantity labels.

The auxiliary right-aligned baseline is
**(cellWidth+5)×c+cellWidth+6**, **(cellHeight+5)×r+cellHeight+37**,
flags2. A positive-quantity repair entry (mode2/tab6) submits a white
literal **`+`** there at0x27e39a. Shop mode3 uses its existing tab-specific
buy/sell price result, when nonzero, followed by a space and byte0xae,
in **RGBA(1,0.9,0,1)** at0x27f064. Ordinary mode0's eligible sale-price
line while shop is open uses that same placement/color at0x27ebb7.
Price/business formulas remain loot-shops, not a new grid quantity rule.

Mode1 is not claimed as another text variant by this bounded table.
Repair-specific equipped/availability decorations and every surrounding
heading/footer are not exhaustively respecified here. No owner artwork,
numeric font glyph geometry or replacement template has been inferred.

## 7. Root Widget attachment and callback order

The native custom-wrapper constructor chain is
**0x27c950→0x268a70→0x328ff0**. Base construction writes **Widget
flags+0x150=0**, explicitly at **0x3290c6**; neither0x268a70 nor
0x27c950 sets bit0x200. Do not confuse the constructor's mode argument
stored at+0x280 with Widget flags. The InventoryWidget vtable
**0x46ef78, draw slot+8** points to0x27d2b0.

For the usual interface caller's shallow instance of its owner root,
**0x333890 does clone the source own Widget through slot+0x140**.
Attaching the custom wrapper through0x334410 therefore transfers that
existing root Widget's geometry through0x329c70, deletes/replaces that
root wrapper and attaches the new InventoryWidget. As specified fully
in steam-character-list-input-and-frame-init5, new bind/frame position
and size take the old **raw stored** frame values; new inner bind takes
the old deformed inner position and unclamped inner size; B is copied
and its inverse recomputed. Minimum is derived from new bind size minus
new inner size. This transfer does **not** copy the old Widget flags,
alignment, enabled state or dynamic class. Descendant own Widgets cloned
by0x333890 remain; it does not flatten/remove all child Widget bindings.

Custom construction makes the root transformation/display unique with
0x335830/0x3356d0 while preserving their contents. Frame getters do not
subsequently import another owner binding or fit drawing bounds. The
unit geometry before attachment is not an authoritative replacement for
this transferred geometry. Numeric owner binding/artwork remains data.

In **0x330b20**, the **own Widget's**+0x150 bit0x200 chooses callback
placement: set means its draw hook before child traversal, clear means
after. The constructor path above consequently selects the **after-
children** hook. On the ordinary admitted uncached path, the node's own
shape has already drawn, then its children in child-list order, then
0x27d2b0 draws the immediate3D cell icons and manual text at that point
in the owner scene. A later global icon layer is not this callback order.
The existing visibility/excluded-node/clip/blur/cache rules of the Plasma
traversal still govern reaching the hook; this paragraph does not force
hidden/cached contents to invoke it or infer a missing0x200 assignment
from an owner's desired appearance.
