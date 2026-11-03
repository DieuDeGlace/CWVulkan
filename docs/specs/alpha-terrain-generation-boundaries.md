# Alpha terrain generation: established contract and parity boundaries

Independent bounded assessment of the Alpha terrain specifications. No engine,
tests, extractor, owner exports/captures, reconstruction, mod implementation or
game execution was used. The basis is the independent `AlphaVulkan/docs/specs/`
documents `alpha-worldgen-overview.md`, `alpha-worldgen-random.md`,
`alpha-worldgen-regions.md`, and `alpha-worldgen-terrain.md`, plus their already
written `alpha-worldgen-locations.md` and `alpha-worldgen-caves.md` completions.
Only narrow original Alpha native lookup and client/server arithmetic checks
were added here. Vegetation and structure algorithms are outside this audit.

The earlier Alpha documents cite virtual addresses. Both executables have image
base **0x400000**; this note labels VA and RVA explicitly. Client Cube.exe and
Server.exe have separate address spaces. Native source identities are those
of the existing specifications: client SHA-256 beginning **84a7a132**, ending
**30bf**; server beginning **1ffbcbce**, ending **4fb1**.

## 1. The existing contract goes beyond base height

The four entry documents are not merely a noise/height sketch. They describe
seed offsets, region cells and climate, shaping fields, base height, a padded
height grid, and an ordered zone surface pass including colors, ravines and
lake fill. Their distinct products must not be treated as interchangeable:

| Product/stage | Established native role | Boundary |
| --- | --- | --- |
| Base height H | Client **VA0x5c5e20 / RVA0x1c5e20** returns a float height in blocks, after its location-dependent shaping | Not the final stored surface height |
| Zone base-height grid | Zone generator **VA0x5e4850 / RVA0x1e4850** evaluates a **257×257** grid, including the far row/column | Provides adjacent-height inputs for the 256×256 surface columns |
| Surface-complete columns | The surface pass derives mountain/ridge contribution, a separate float surface S and truncates it to signed32 zs; writes layers at **zs and zs+1**, then ravine/lake branches | Intermediate terrain, before later rock/road/terrace/dungeon/building/tree passes |
| Full generated zone | Later voxel-changing passes, stored zone modifications and block-light work precede generator completion/publication | Includes more than this terrain-only audit |

Climate and surface column loops visit X outside Y. The grid's padding is part
of the input to the strict slope comparisons, not optional interpolation of
a 256×256 height image. The surface S includes terms not present in H. Ravines
can remove a vertical interval and leave an overhang; lake fill can change
both stored blocks and water occupancy. Consequently equality of H, or even
of one top-height integer, cannot establish column-layout or surface parity.

These facts are already specified in Alpha terrain §§4–5. The client surface
boundary **VA0x5e6b58 / RVA0x1e6b58**, where the height-grid cleanup/post-pass
transition begins, is a useful distinct provenance label. It must not be
reported as full-generation output.

## 2. Read the existing completions before treating old questions as gaps

The overview and original regions/terrain open-question lists are historical.
They are not a current completeness census:

- **Locations:** `alpha-worldgen-locations.md` completes region preparation
  **VA0x5da280 / RVA0x1da280**, its location records and per-zone table, and
  the type-10 segment helper. Location-free height alone therefore is not the
  current written scope. Region preparation publishes its region pointer only
  after preparation: its internal height samples see no locations in that
  region. Publishing a partly prepared plan would change those evaluations.
- **Rock additions:** `alpha-worldgen-caves.md` explicitly corrects the earlier
  interpretation of **VA0x5cbe00 / RVA0x1cbe00** and
  **VA0x5cb600 / RVA0x1cb600**. These add rock formations; they are not ellipsoid
  cave carvers. The type-6 candidate grid is 3×3 with offsets 42,127,212, and
  type11 adds a large mound. An implementation based only on the old overview's
  cavity terminology would produce the opposite block mutation.
- That completion also specifies the rock, road and terrace range previously
  left as an outline, and completes the initial ravine spawn branch. It does
  not make every later structure's data-dependent random consumption complete.

Remaining explicit limits relevant to a strict surface/height claim include
location planner sort ties and last-bit location-record height precision
(`locations` §14), full generator/client-server equivalence, and edge behavior
of the whole padded-grid generation path. The later dungeon draw count and
subsequent outlined voxel-changing passes remain separate from a complete
initial surface pass. Their effects cannot be dismissed merely because
vegetation/structures are excluded from the current implementation scope.

## 3. Inputs, coordinates and bounded native checks

Alpha's supported storage domain is an absolute, nonnegative grid:

| Domain | Range/size |
| --- | --- |
| Block X/Y | **0 through 16777215**, inclusive |
| Zone X/Y | **0 through 65535**, inclusive; **256×256** columns |
| Region X/Y | **0 through 1023**, inclusive; **64×64** zones |
| Location subcells | **2048×2048** blocks; 8×8 records per region |
| Entity/location position | Signed64 fixed, **65536 units/block** |

This assessment independently checked the native guards. Client column lookup
**VA0x4347a0 / RVA0x347a0** rejects signed32 X/Y below zero or at least
0x1000000. With an explicitly supplied zone it also rejects columns outside
that zone's half-open 256-column bounds. Zone lookup
**VA0x434d10 / RVA0x34d10** rejects indices outside 0..65535; region lookup
**VA0x434a90 / RVA0x34a90** rejects outside 0..1023. These paths return no
record; they do not wrap negative coordinates to the opposite world edge.

Effective block lookup **VA0x42f7e0 / RVA0x2f7e0** uses its below-column fallback
for a missing/out-of-domain column. Above stored columns it chooses implicit
air for Z>0 and water for Z≤0; a stored air-type block at Z≤0 becomes effective
water unless its dry-air bit 0x40 is set. Raw stored bytes and effective reads
therefore are different comparison products. Alpha blocks are **four bytes**:
RGB and a combined low-five-bit type/high-three-bit flags byte. Column records
are 32 bytes with signed32 bottom/count and a block-array pointer, as specified
in Alpha terrain §1. Steam record layouts must not be substituted for these.

The known home coordinate is **(8396928,8396928)** blocks, in region (512,512).
Matching a world requires its actual seed/derived offsets, name-dependent
region-plan inputs, coordinate convention and preparation state. The temporary
client time-based seed precedes receipt of the server seed; it is not evidence
for selecting a different terrain family. Empty versus nonempty world name
changes the home-region plan in the existing location contract.

The legal storage domain does not prove that evaluating the height function
outside it is rejected, periodic, or equivalent to translating a signed Steam
world. In particular the outermost zone's padded grid evaluates the far edge
at coordinate 16777216. The complete generation behavior there was not closed
by these lookup checks. Interior zone seams, absolute world edges and arbitrary
negative-coordinate extension require different labeled checks.

## 4. Precision and client/server scope

The Alpha noise contract uses double arguments/interpolation and a float
result. Its lattice-coordinate conversion truncates toward zero, including
negative fractional noise arguments; replacing it with floor changes that
kernel. Terrain and climate contain explicit float intermediate boundaries,
double noise-coordinate products, integer wrapping and truncation. The stated
precision must not be replaced wholesale by either double or float arithmetic.
Region displacement also has float staging that loses sub-block resolution
at large absolute coordinates; translating the world origin is not established
as a numerically neutral operation.

A bounded comparison of the original client/server binaries checked:

| Function | Client VA / RVA | Server VA / RVA | Checked body size |
| --- | --- | --- | --- |
| Noise | 0x4c0ef0 / 0xc0ef0 | 0x4d5d30 / 0xd5d30 | 522 bytes each |
| Base height | 0x5c5e20 / 0x1c5e20 | 0x4f9b70 / 0xf9b70 | 4874 bytes each |

Their decoded ordered operations/register-addressing roles and referenced
embedded operand values match after accounting for image-specific addresses.
No different direct float/double staging was found in these two pairs. This
is stronger than merely noticing equal function sizes, but is **not** proof
that every relocated callee, CRT environment, region plan or complete zone
generator has identical behavior. The server zone counterpart is
**VA0x518630 / RVA0x118630**; its full surface/post-pass body was not compared
here. No original game was run and no client/server runtime oracle was taken.

## 5. Precise evidence still needed for a full parity claim

Existing algebra-derived vectors help validate the written contracts but are
not native runtime measurements. A bounded owner-authorized comparison needs
provenance sufficient to distinguish these products:

- Executable identity/client versus server; seed and all derived offsets;
  world name, absolute zone/column coordinates, and region-preparation state.
- **Height-only:** H's float word at selected columns, including region and
  location boundaries, fractional/truncation thresholds, and large coordinates.
- **Surface-complete:** columns at the 0x5e6b58 boundary: bottom, count and
  ordered four-byte block records, with climate/location inputs. Include paired
  columns across a zone seam, ravine openings/overhangs, coast and lake cases.
- **Full generation:** a separately labeled generator-return record, stating
  whether saved edits and light recomputation ran. It cannot stand in for the
  earlier surface snapshot: later voxel additions/removals, including trees,
  affect the apparent highest solid block.
- World-edge checks at X/Y 0 and 16777215, plus the neighboring padded-height
  samples; the lookup guards alone do not answer full edge-generation parity.

Unspecified structure/dungeon or cross-zone mutation order must remain a
separate limitation rather than be hidden by comparing only H or a chosen
terrain-only stage. No oracle capture or execution is requested/performed by
this note; these are the remaining labeled evidence requirements.

## 6. TerrainVariance compatibility boundary

The author's TerrainVariance wrapper was not inspected in this bounded audit;
its actual hook, supported executable, timing and write operations remain
unproved here. No claim about its current implementation is made.

Conditionally, **displacing existing Steam columns after their generation**
preserves the source generator's region/material/column choices unless further
operations replace them. Such a transformation alone does not execute Alpha's
region planning, climate fields, surface S, ravine intervals, dry-air/sea rules
or rock/road/terrace passes. Even agreement with sampled Alpha H would not
establish Alpha column or cave/overhang parity. This follows from the separate
products in §1, not from an inspection of the mod.

Closing the wrapper-specific boundary requires its actual native hook target,
input coordinate/seed/name mapping, invocation stage, columns written and
retained, and whether effective/raw water, stored changes and lighting are
rebuilt. No replacement asset, guessed cave algorithm, mod edit or engine
implementation is supplied. Choosing Alpha as the default while retaining
Steam as a separate mode is the requested product policy, not evidence that
the two native generation families or their coordinate domains coincide.
