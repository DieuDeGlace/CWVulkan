# Alpha environment-model material light

This note describes the ordinary environment-record path, including natural
small plants, from the Alpha client terrain mesher to its environment-model
drawer. Evidence is the original Alpha client executable/decompile, image base
VA `0x400000`. Each VA below has its RVA in parentheses. No engine implementation,
owner resource data, capture or runtime execution was used. Other model families
and alternate drawers are outside this bounded contract.

## 1. Field identity and construction boundary

The environment record is 0x40 bytes. Its signed64 fixed XYZ begin at +0x08,
+0x10 and +0x18, with 65536 fixed units per block. The light field at **+0x28 is
float32**, in byte-light units rather than normalized units. It is not a signed
integer field. This is established by the float store in the mesher at
VA0x4a0e37–0x4a0e49 (RVA0xa0e37–0xa0e49), and the float load/division in the
drawer at VA0x4be6f5–0x4be707 (RVA0xbe6f5–0xbe707).

The common record constructor VA0x58e9f0 (RVA0x18e9f0) initializes RGB +0x2c,
+0x30 and +0x34 to float1, and flags +0x38 to integer2. It does not initialize
+0x28. Thus this constructor does not give a freshly generated record a default
light of zero, one or 255. The small-plant construction routes described in
[alpha-plant-record-consumption.md](alpha-plant-record-consumption.md) do not
establish an initialized light value before their list append. This is a
generation-versus-meshing distinction, not permission to reinterpret arbitrary
uninitialized bits as an intended lighting value.

The reached list append for those plants targets the source Zone's list at
**+0x04**, for example VA0x5ec2e2–0x5ec2ec and VA0x5ec582–0x5ec586
(RVA0x1ec2e2–0x1ec2ec, 0x1ec582–0x1ec586). This is also the list read by the
mesher below. The +0x24 source-list label in older vegetation prose does not
describe this reached environment-record list. A list node's payload begins at
node+0x08, so record+0x28 is node+0x30; these offsets must not be conflated.

## 2. Mesh-time origin sampling and precision

The terrain mesher VA0x49d910 (RVA0x9d910) acquires the World critical section at
World+0x8000d8 through VA0x59c480 (RVA0x19c480), called at VA0x4a0cd2
(RVA0xa0cd2). That helper is a lock acquisition, not a light solver. The mesher
then resolves the loaded source Zone with VA0x434d10 (RVA0x34d10), called at
VA0x4a0ce4 (RVA0xa0ce4), and walks Zone+0x04 forward in stored list order. A
missing Zone or an empty list supplies no environment records for this pass.

Only records belonging to the mesh's horizontal cache coordinate pair are
sampled/copied. Each fixed horizontal component is converted from signed64 to
double, multiplied in double by exactly 0.03125, truncated to signed64, then
divided as a signed64 integer by 65536 toward zero. The low32 result is compared
with the corresponding signed32 cache coordinate. Y is evaluated before X;
both must match. This is the reached 32-block horizontal membership calculation,
not a floor operation substituted for the two truncations. Evidence:
VA0x4a0d05–0x4a0db4 (RVA0xa0d05–0xa0db4); double constant at VA0x702a60
(RVA0x302a60).

For a matching record, the actual light query uses the **unmodified record
origin**, not the resource center, a raised sphere center, a deformed vertex or
the camera. Each signed64 fixed XYZ is divided by integer65536 toward zero;
the low32 quotient is passed as that block coordinate. No float position is
used for this division. Thus negative fractional Z is truncated toward zero,
not floored. The effective-block query is VA0x42f7e0 (RVA0x2f7e0), with the
optional Zone argument null. Construction of its three coordinates and the
call are at VA0x4a0dba–0x4a0dff (RVA0xa0dba–0xa0dff).

The effective block has four bytes. Its material type is byte3 masked by0x1f.
The selected light is:

| Effective type at the origin | Selected byte-light value |
| --- | --- |
| 13 | 255 |
| 0 or 2 | First block byte, raised to5 if below5 |
| Any other type | 0 |

These decisions occur at VA0x4a0e04–0x4a0e34 (RVA0xa0e04–0xa0e34). No RGB
luminance or average of nearby samples is involved. The selected integer0–255
is converted directly to float32 and stored into the **source Zone list
record's +0x28**, before that record is copied. All these integer values are
exact in float32. There is no division by255 at this producer.

This is an effective-block read. VA0x42f7e0 uses the loaded column when present;
missing/below-column reads use its default solid block, above-column reads use
default air for positive Z and water otherwise, and stored type0 below Z1 is
replaced by effective water unless flag0x40 exempts it. The material/red bytes
seen after those substitutions govern the table above. The origin sampler
does not bypass these substitutions to inspect a raw stored RGB tuple.

The first byte of air/water is the existing World block-light storage described
in [alpha-rendering.md §9.2](alpha-rendering.md#92-light-values-in-empty-and-water-blocks-0x59a0e0).
Its solver is VA0x59a0e0 (RVA0x19a0e0); the mesher has a reached call at
VA0x49db84 (RVA0x9db84), earlier than the model sampling. This note does not
replace that solver or turn the lock helper into a solve. The model sample
reads the then-current effective World block, rather than an independently
invented model light or a floating mesh-corner accumulator.

## 3. Publication and retained sample lifetime

After writing source+0x28, the mesher copies the record into its temporary
environment list through VA0x5c03f0 (RVA0x1c03f0), called at VA0x4a0e59
(RVA0xa0e59). The semantic record copy VA0x459ab0 (RVA0x59ab0) preserves
the +0x28 float bits. Matching source records keep their original relative
order. Records outside the selected horizontal membership are neither sampled
nor included by this loop. There is no flags-mask0x01 exemption from sampling in
this producer, even though the later own-light draw ignores the sample.

The World lock is released through VA0x601ea0 (RVA0x201ea0). Later, while holding
the cache critical section +0x250, the mesher assigns this temporary list to
the cache's **pending environment list +0x248**. The assignment call is at
VA0x4a11c2 (RVA0xa11c2), using VA0x4570a0 (RVA0x570a0). This does not directly
replace the renderer's active environment list.

The independently established ordinary frame transfer is
[alpha-far-static-ordering.md §6.1](alpha-far-static-ordering.md#61-stored-window-and-readiness-transfer):
VA0x4ac58a–0x4ac652 (RVA0xac58a–0xac652) transfers pending environment records
**+0x248 to active +0x240**, in stored order, alongside the admitted pending
mesh groups under the cache lock. It requires the opaque dependency-key
count to be zero and the pending mesh-group count +0x14 to be nonzero. Otherwise
the prior active records remain. The assignment does not itself erase the
pending environment source list.

Consequently, the light consumed by an active environment model is a retained
mesh-time sample. A change to World block red does not itself make this draw
query the origin again. The sampling/copy and pending-to-active transfer are
distinct phases, and even the raw Zone record can have a newer +0x28 than an
older active copy. This note establishes those phase cuts; it does not establish
every producer of cache rebuild requests or a universal refresh interval.

## 4. Draw material, frame contribution and own-light

The ordinary near and deferred environment routes both call VA0x4bd160
(RVA0xbd160): near at VA0x4b2801 (RVA0xb2801), deferred at VA0x4ba4fe
(RVA0xba4fe). Their caller-supplied RGBA is the ordinary frame material, not
another per-model block query. Its RGB is normally (1,1,1), or (0.4,0.5,1)
underwater; alpha is the sunlight-strength S of
[alpha-rendering.md §2.2 and §4](alpha-rendering.md#22-sunlight-strength-s-0x4ac3e1).
Frame material construction is at VA0x4ad68f–0x4ad726
(RVA0xad68f–0xad726). The near call passes that material buffer at
VA0x4b27c8–0x4b27cf (RVA0xb27c8–0xb27cf); the deferred call supplies the same
frame input.

The drawer first tests **record flags +0x38 mask0x01**, the bounded meaning of
OWN_LIGHT here. Its material assembly copies the caller material's three RGB
floats unchanged, then chooses alpha using that flag result. It does not
multiply those RGB fields by environment-record RGB +0x2c..+0x34:

| Flag0x01 | Submitted material alpha |
| --- | --- |
| Clear | Record light +0x28 divided by float255, then multiplied by caller material alpha S |
| Set | Literal float1; both record light and caller material alpha are bypassed |

The clear branch uses an actual float32 division followed by a separate
float32 multiplication, with division constant255 word0x437f0000 at
VA0x6fce64 (RVA0x2fce64). It does not multiply by a pre-rounded reciprocal.
There is no clamp or integer conversion in this draw branch. For the normal
sampled record, +0x28 is already one of the exact floats0..255. Undefined,
corrupted or externally supplied values do not acquire a safe native default
from this consumer. The set branch supplies word0x3f800000.

The test, RGB copies and alpha assembly are at VA0x4be6b6–0x4be707
(RVA0xbe6b6–0xbe707); the completed RGBA reaches the material setter
VA0x448280 (RVA0x48280) at VA0x4be71e (RVA0xbe71e), before the resource draw
VA0x4e6df0 (RVA0x1e6df0), called at VA0x4be725 (RVA0xbe725).

This alpha is a WORLD **material-light input**, not the separate distance-fade
opacity sent to VA0x447fb0 (RVA0x47fb0). OWN_LIGHT does not select Glow or bypass
the entire WORLD shader, palette, source vertex color or caller RGB. The
shader's handling of material alpha and its independent ambient term remain
as specified in [alpha-rendering.md §5](alpha-rendering.md#5-the-world-vertex-shader-0x6fd928).
The ordinary plant flag results2/4/6 documented in
[alpha-plant-record-consumption.md §4.1](alpha-plant-record-consumption.md#41-actual-flags-produced-by-the-checked-small-plant-branches)
all have this mask clear, including the reed's overwrite-to4 case.

This precisely qualifies the older wording in the sister project's independent
vegetation-wind.md §4.5
about multiplying by the light where the model stands: the origin sample is
the cached record float; the other alpha operand at draw time is **frame S**,
not a second current origin sample. Alpha-rendering §11.3's frame-material
summary is likewise the starting material, with this environment-specific
alpha replacement applied before upload.

## 5. Explicit limits

This contract covers the reached ordinary environment/natural-plant path and
its direct pending/active publication. It does not establish all factories or
later writers of every environment field, every cache invalidation caller,
adjacent-zone ownership, global cache population/extent, or concurrent changes
outside the reached lock scopes. Special effect model0x3c and alternate
map/UI/object/creature drawers are not generalized from this material path.
Record tint may have consumers elsewhere; the RGB-copy fact in §4 is confined
to this drawer. No new raw-generation light oracle, first-mesh scheduling
guarantee, or safe value for uninitialized +0x28 is asserted.
