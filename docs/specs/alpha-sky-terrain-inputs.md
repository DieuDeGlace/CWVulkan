# Alpha sky and cloud terrain inputs

Independent native-only bounded supplement to `alpha-rendering.md` §3,
`sky-and-clouds.md` part A, and the independent Alpha world-generation regions,
terrain and clouds specifications. No engine, exporter, owner data/captures,
reconstruction or game execution was used. Alpha documents cite virtual
addresses; the client image base is **0x400000**, so VA and RVA are both given
below. W is the Alpha World embedded at **Game+0x2e4**.

## 1. Palette factors are direct Alpha World blend queries

Palette provider **VA0x59d640 / RVA0x19d640** receives the camera's signed64
fixed XYZ at **Game+0x140**, with **65536 units/block**. Its terrain-factor
calls use X/Y divided by 65536 with signed truncation toward zero, then the
32-bit block-coordinate arguments. They are not sampled at the player's
position, drawn actor position, a cloud position or an assumed region center.

| Factor | Direct World routine VA / RVA | Region-cell value being blended | Palette call site VA / RVA |
| --- | --- | --- | --- |
| Warmth Wm | 0x5c4dd0 / 0x1c4dd0 | float at cell+0x10 | 0x59d8f2 / 0x19d8f2 |
| Moisture Mo | 0x5c4800 / 0x1c4800 | float at cell+0x0c, followed by the oasis rule where admitted | 0x59d92d / 0x19d92d |
| Sea share Se | 0x5f0720 / 0x1f0720 | 1 for signed32 cell elevation+0x18 <0, otherwise 0 | 0x59da51 / 0x19da51 |
| Fire share Fi | 0x5ef040 / 0x1ef040 | 1 for cell byte+0x08 equal to 1, otherwise 0 | 0x59dd9d / 0x19dd9d |

These are the noise-displaced, distance-weighted region-cell blends specified
in Alpha worldgen-regions §§3–4. They read the Alpha region-cell pointer grid
at **W+0x4000bc**, not one categorical biome value from a single owner cell.
The weighting, scan order and missing-cell behavior belong to that existing
contract. Its ordinary default warmth/moisture is 0.5 and sea/fire is 0.
The moisture oasis adjustment consults the applicable location record: a
successful dry blend with location type3 can increase moisture. It is not
simply a cached region-wide average.

Warmth and moisture are obtained before the ordered hot/dry and warm/wet
palette modifiers. Sea share is queried **only in the warm/wet branch**,
when **Wm>0.3 and Mo>0.6**. Fire share is queried later for the fire modifiers.
The existing palette contract supplies their exact color operations. Sea share
does not mean the camera is below sea level, the current ground height is
negative, or the effective block is water. It is the weighted negative-cell-
elevation indicator above.

The provider separately checks the **effective block at the full fixed camera
XYZ** through **VA0x42f860 / RVA0x2f860** for the underwater palette override.
That water test is independent of Se. The palette provider makes no direct
base-height H or mountain-mask M query; no extra terrain-height, cliff or
ground-level scalar should be attributed to this palette path. Warmth also
feeds the existing ambient-light rule; it is not a second separately sampled
ambient biome input.

## 2. Direct blends versus loaded-column data

The four direct blend routines in §1 compute from the World region-cell data;
they do not call zone/column lookup to retrieve a stored column's climate.
This qualifies the broad introductory wording in `alpha-worldgen-clouds.md`
about using stored column values: it must not be read as a description of
these direct palette/cloud blend call sites.

Higher-level terrain routines can have their own column lookup. For example,
roughness **VA0x5f9340 / RVA0x1f9340** uses column lookup
**VA0x4347a0 / RVA0x347a0** for its fire contribution and falls back to the
direct fire blend when no column is returned. Nothing here removes such
internal policies from H/M and their dependencies. Passing no explicit zone
is not by itself proof that every nested terrain helper ignores loaded World
columns. The existing terrain/regions contracts remain authoritative for
those helpers and their location-dependent shaping.

## 3. Cloud admission and cloud-position samples are different

The reached cloud portion of frame **VA0x4ac260 / RVA0xac260** uses this
ordered set of World queries:

| Use | Sampling coordinates and direct query | Native call VA / RVA |
| --- | --- | --- |
| Global moisture gate | Local creature **Game+0x8006d0**, physical fixed XY at creature+0x10/+0x18, converted to block coordinates; direct Mo | 0x4b1564 / 0xb1564 |
| Global fire gate | The same local-creature physical XY; direct Fi, reached after the moisture test | 0x4b15b0 / 0xb15b0 |
| Candidate coverage | Cloud cell **indices (X,Y)** themselves treated as block coordinates; direct Mo | 0x4b16b7 / 0xb16b7 |
| Local cloud moisture | Actual staggered cloud anchor **(200X,200Y+100·(X remainder 2))** in blocks; direct Mo | 0x4b1779 / 0xb1779 |
| Cloud ground contribution | Same actual anchor; mountain mask **M, VA0x5effa0 / RVA0x1effa0** | 0x4b17b4 / 0xb17b4 |
| Cloud ground contribution | Same actual anchor; base height **H, VA0x5c5e20 / RVA0x1c5e20** | 0x4b17ee / 0xb17ee |

Global admission requires **Mo>0.01** and **Fi<0.01**, in addition to the
existing sky/cloud suppression flag. The frame's cloud-grid center instead
comes from the **camera**, divided into 200-block cells, with the existing
13×13 window. Thus the global gate is not the camera-factor sample described
by the old alpha-rendering §11.4 summary; `alpha-worldgen-clouds.md` §§2–3
already records the local-player distinction.

The coverage test compares its cell-index noise value with **0.8 times the
cell-index moisture**. This first moisture query is not moved to the physical
cloud anchor. Only after coverage passes is the actual anchor queried, with
local moisture **at least 0.3** required. No per-cloud sea, fire or warmth
query is added to these reached tests.

The height inputs are **H plus 100 times M**, with a nonnegative base clamp,
then the separate existing noise altitude. The two height calls supply no
explicit zone pointer. This is not a query for the highest stored solid block,
the final terrain surface S, the current camera altitude, a structure roof or
a prepared mesh bound. The established cloud placement contract supplies the
remaining noise/scale/orientation operations; they are not duplicated here.

## 4. Precision and limits

The component selector **VA0x4120b0 / RVA0x120b0** and fixed-to-block helper
**VA0x42f570 / RVA0x2f570** in the cloud caller establish signed64 division
by 65536 with low32 block-coordinate delivery. Neither GUI coordinates nor
the renderer's rebased floating origin replace those World sample coordinates.
The direct blend return values are narrowed to float before the caller's
comparisons/color arithmetic. Worldgen-regions specifies the blend's own
float/double staging and negative/out-of-domain behavior.

The cloud specification's still-open exact fixed-point conversion of its
assembled base altitude is not newly resolved by this input audit. Full missing-
region preparation/lifetime, generation-thread scheduling, all nested H/M
cache effects, executable server equivalence and additional rendering modes
are outside scope. No independent numeric oracle was taken. The known direct
World queries and their sampling locations above must be distinguished from
those unclosed paths and from any replacement engine's terrain adapter.
