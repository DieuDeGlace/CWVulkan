# Alpha cloud altitude and fixed placement

This bounded supplement resolves the altitude-conversion question in the
independent Alpha cloud-placement specification. Evidence is the ordinary
Alpha client frame VA0x4ac260 (RVA0xac260), specifically its reached cloud
placement path VA0x4b17b9–0x4b1a42 (RVA0xb17b9–0xb1a42), and the directly
called helpers below. The executable image base is VA0x400000. No engine,
owner resource data, captures or runtime execution were inspected.

## 1. Inputs and existing admission

The grid, coverage/moisture/fire gates and physical sampling anchors remain
as independently described in the sister project's alpha-worldgen-clouds.md
§§1–3 and [alpha-sky-terrain-inputs.md §3](alpha-sky-terrain-inputs.md).
X and Y below are the signed32 cloud-cell indices, not the physical block
coordinates supplied to the height queries. The physical anchor has block X
equal to200X and block Y equal to100 times the sum of2Y and the signed
remainder of X divided by2. Its initial fixed Z is zero.

The local moisture admission has already passed when the altitude work below
runs. M is the mountain-mask query VA0x5effa0 (RVA0x1effa0); H is the base-height
query VA0x5c5e20 (RVA0x1c5e20). Both use that physical anchor and no explicit
Zone argument, as established in the sky-terrain input note. This is neither
the final surface nor a stored highest-solid-block query.

## 2. Base height: float arithmetic, fixed conversion, then clamp

The M result is stored to float32 immediately after its call, at VA0x4b17b9
(RVA0xb17b9). The H result is likewise stored to float32 at VA0x4b17f3
(RVA0xb17f3). The caller multiplies that narrowed M by float100 and rounds the
product to float32, then adds that product to the narrowed H with a separate
float32 result. There is no double multiply/add or fused operation in this
assembly of the base height. Evidence: VA0x4b17f7–0x4b1814
(RVA0xb17f7–0xb1814); float100 is word0x42c80000 at VA0x6fd0b4
(RVA0x2fd0b4).

The resulting base height is passed to scalar conversion VA0x42c580
(RVA0x2c580), called at VA0x4b1819 (RVA0xb1819). Its input is float32. It
multiplies by **float65536**, rounds that product to float32, and only then
converts the product to **signed64 by truncation toward zero**, using
VA0x68d946 (RVA0x28d946). The result occupies eight bytes. The multiplier is
word0x47800000 at VA0x6fce7c (RVA0x2fce7c); the helper's multiply/store and
conversion are at VA0x42c583–0x42c5a2 (RVA0x2c583–0x2c5a2).

The nonnegative clamp is applied **after that fixed conversion**. The signed64
fixed base Z is compared with zero through VA0x42c860 (RVA0x2c860), called at
VA0x4b1824 (RVA0xb1824). A strictly negative fixed result is replaced by
signed64 zero through VA0x412080 (RVA0x12080), called at VA0x4b1833
(RVA0xb1833). A zero result is retained. Thus a small negative float that
truncates to fixed zero does not enter the negative branch. This is not an
earlier float clamp followed by an unspecified conversion.

## 3. Noise altitude is converted separately and added only to Z

The altitude noise input is the pair17X,19Y. Those integer products are formed
with low32 arithmetic, interpreted as signed32, and converted exactly to
double for the noise call VA0x4c0ef0 (RVA0xc0ef0), called at VA0x4b186e
(RVA0xb186e). Evidence for the coordinate staging is VA0x4b1838–0x4b186e
(RVA0xb1838–0xb186e). This noise query is distinct from the coverage and scale
noise queries.

The returned noise is stored to float32 at VA0x4b1873 (RVA0xb1873). That value
is multiplied by float50, rounded to float32, then added to float170 with a
separate float32 result. The two constants are word0x42480000 at VA0x6fcd7c
(RVA0x2fcd7c) and word0x432a0000 at VA0x6fd598 (RVA0x2fd598). The operations
and stored argument occur at VA0x4b1877–0x4b1897 (RVA0xb1877–0xb1897).

This noise altitude goes through the **same scalar conversion** VA0x42c580,
called at VA0x4b189c (RVA0xb189c): float32 multiply by65536, float32 product,
then signed64 truncation toward zero. It becomes a separate eight-byte fixed
offset. It is not retained as a float or added to the float base first.

At VA0x4b18ad (RVA0xb18ad), scalar helper VA0x42c9b0 (RVA0x2c9b0) adds that
fixed offset to the already converted/clamped fixed base Z. It adds the full
64-bit values with carry from the low32 half; there is no intervening narrowing
of Z to signed32. This is a scalar Z addition, not a three-component noise
vector and not a displacement of X or Y. There is no second nonnegative clamp
after this addition in the reached path.

Accordingly, the observable staging consists of two separate float-to-fixed
quantizations, with the clamp between them and the final addition in fixed
space. A single conversion of the combined real-number altitude does not
describe this contract. The earlier cloud specification's assumed conversion
and its description of a separate fixed-point vector are superseded by these
scalar/conversion facts.

## 4. Reached render translation and header centering

The assembled cloud position remains a signed64 fixed XYZ tuple. For the
render translation the caller constructs a second tuple using Game+0x1d8 and
Game+0x1e0 as its two stored signed64 horizontal components, and literal fixed
zero for Z. Helper VA0x42c800 (RVA0x2c800), called at VA0x4b18fe
(RVA0xb18fe), **adds** the two tuples component by component with 64-bit carry.
It does not subtract those stored Game components or first narrow the positions
to float. Construction and the call are at VA0x4b18be–0x4b18fe
(RVA0xb18be–0xb18fe). This note specifies the use of those stored render-origin
components; their upstream camera lifetime is not re-audited here.

The summed tuple is converted by VA0x42c4a0 (RVA0x2c4a0), called at VA0x4b190b
(RVA0xb190b). Each signed64 component is first rounded to float32 and then
multiplied by float1/65536 with a float32 result. There is no double division
before this first float narrowing. The conversion's three component paths are
VA0x42c4ae–0x42c4f7 (RVA0x2c4ae–0x2c4f7). The resulting block-space floating
translation is applied to the initially identity cloud matrix through
VA0x424990 (RVA0x24990), called at VA0x4b191f (RVA0xb191f).

The subsequently reached uniform scale and Z rotation are the separate,
already specified scale-noise and cell-angle operations. After those, the
caller reads the selected cloud resource's **signed32 header words +0x48 and
+0x44**, through VA0x424820 and VA0x424810 (RVA0x24820 and0x24810), respectively.
Each header value is converted to float32 and multiplied by float−0.5,
word0xbf000000 at VA0x6fce8c (RVA0x2fce8c). The local centering displacement
uses the +0x44 value for X, +0x48 for Y, and literal zero for Z. It is passed
through the existing matrix basis by VA0x424a60 (RVA0x24a60), called at
VA0x4b1a13 (RVA0xb1a13). Header access and displacement staging are at
VA0x4b19c2–0x4b1a13 (RVA0xb19c2–0xb1a13).

These are the full resource header dimensions used by the ordinary resource
convention, not occupied-vertex bounds or a separately loaded pivot. The
centering uses half the dimensions, not half of dimensions minus one. No
header Z dimension is used for centering in this reached cloud path. Thus
vertices are centered in local XY before the scale/rotation and floating
render translation take effect; local Z retains its resource base. This agrees
with the independently established full-header convention in
[alpha-plant-record-consumption.md §2](alpha-plant-record-consumption.md#2-full-header-xy-centering-with-z-at-the-base).
The independently identified owner resource remains cloud02.cub, catalog2568;
this audit did not inspect its bytes or substitute dimensions from an asset.

## 5. Limits

The conversions above describe finite ordinary cloud inputs whose fixed
products fit signed64. NaN, infinity, out-of-range float-to-integer results
and arbitrary changed floating-point control modes are not assigned recovery
semantics here. The height/noise queries' internal arithmetic remains governed
by their existing independent contracts. No server equivalence, cache
preparation/lifetime, additional draw modes, resource contents or new runtime
oracle is asserted. The exact clamp/quantization and reached header reads close
the bounded placement questions without closing those other domains.
