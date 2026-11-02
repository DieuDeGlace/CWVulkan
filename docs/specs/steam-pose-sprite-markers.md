# Steam ordinary pose sprites: hair RGB and attachment markers

Independent native-only bounded follow-up to the pose material contract.
Sources: catalog construction/loading **0x177f50**, sprite constructor
**0x16e6b0**, file loader **0x16f320**, voxel copy/marker extraction
**0x170030**, geometry filter **0x16f230**, mesher **0x170700**, corner
color **0x170530**, occlusion **0x171db0**, catalog lookup **0x17190**.
No engine, tests, fixtures, extractor, SDK, reconstruction, owner assets or
captures were inspected, and no game was run.

## 1. Reached ordinary hair loading and shared ownership

The ordinary catalog loop allocates individual **0xa8-byte sprite objects**
and constructs each through **0x16e6b0 at0x177ffa**, with its third argument0.
This initializes sprite **+0x9a=0**, additive RGB bytes **+0xa0..+0xa2=0**,
and corner-occlusion radius **+0x9c=3**. These are separate from marker mode
at+0x99. The loader supplies that mode explicitly for each file.

All **105** reached playable-family hair filename loads in this catalog use
**marker mode0** and final loader argument0, which requests immediate meshing:
goblin12, human24, elf21, orc12, frogman9, lizard6, dwarf9, undead12.
These are individual entries, not a claim that their catalog IDs form one
continuous range. Their load calls span **0x17dcf8–0x1857c8**.
For a concrete provenance anchor, native string **0x44fe40** is
`human-hair-m01.cub`; **0x17ecc3–0x17ece8** selects catalog ID **0x5b3** and
loads it with these two zero arguments. Filenames identify owner assets;
their voxel contents were not inspected.

The exact verified catalog sets are below. Every ID in each inclusive row
was checked, with no intervening ID omitted. Within a row, increasing IDs
map one-to-one to increasing **two-digit NN** in the filename pattern;
the suffix bounds are inclusive. The last column gives the first and last
native0x16f320 callsites for that row in0x177f50.

| Catalog IDs | Owner filename pattern | NN | Load callsites |
| --- | --- | --- | --- |
| 0x9c..0xa1 | goblin-hair-mNN.cub | 01..06 | 0x17dcf8..0x17df50 |
| 0xa7..0xac | goblin-hair-fNN.cub | 01..06 | 0x17e220..0x17e478 |
| 0x5b3..0x5c1 | human-hair-mNN.cub | 01..15 | 0x17ece8..0x17f378 |
| 0x5c8..0x5d0 | human-hair-fNN.cub | 01..09 | 0x17f6c0..0x17fa80 |
| 0x5d1..0x5da | elf-hair-mNN.cub | 01..10 | 0x17faf8..0x17ff30 |
| 0x5db..0x5e5 | elf-hair-fNN.cub | 01..11 | 0x17ffa8..0x180458 |
| 0x5f3..0x5fa | orc-hair-mNN.cub | 01..08 | 0x181880..0x181bc8 |
| 0x5fd..0x600 | orc-hair-fNN.cub | 01..04 | 0x181c40..0x181da8 |
| 0x606..0x60a | frogman-hair-mNN.cub | 01..05 | 0x182078..0x182258 |
| 0x60f..0x612 | frogman-hair-fNN.cub | 01..04 | 0x1824b0..0x182618 |
| 0xb0..0xb5 | lizard-hair-mNN.cub | 01..06 | 0x183860..0x183ab8 |
| 0x194..0x196 | dwarf-hair-mNN.cub | 01..03 | 0x184148..0x184238 |
| 0x19c..0x1a1 | dwarf-hair-fNN.cub | 01..06 | 0x184508..0x184760 |
| 0x1ab..0x1b0 | undead-hair-mNN.cub | 01..06 | 0x184fd0..0x185228 |
| 0x1b7..0x1bc | undead-hair-fNN.cub | 01..06 | 0x185570..0x1857c8 |

This table's mode0 conclusion is limited to these105 catalog entries and
their reached load calls. It does not classify arbitrary mod files, other
catalog IDs, filename substrings or an unlisted factory by resemblance.

Normal pose resource selection uses the catalog pointer returned by
**0x17190**, a bounds-checked ID lookup, for the hair sprite at animation
instance+0xe0. It does not make a per-character recolored copy on this reached
selection route. The separately computed hair color remains the per-draw
material in [creature pose §8.5](creature-pose.md#85-the-hair), delivered as
[WORLD material RGBA](steam-world-pass-controls.md#21-pose-material-delivery-readiness-and-explicit-winding).

## 2. Raw colors and attachment-coordinate extraction

**0x16f320** stores its fourth argument as sprite+0x99. It reads the three
signed32 dimensions and the RGB payload, then passes them to **0x170030**.
That helper copies the raw three-byte RGB values into sprite+0x68 without
palette substitution, whitening, halving or gamma conversion. Storage order
is **3×((Z×sizeY+Y)×sizeX+X)**, with dimensions at+0x88/+0x8c/+0x90.

The copy helper resets the ends of all three attachment vectors to their
existing begins. For marker mode0, scan **X outside, Y middle, Z inside**,
in increasing order, and append each exact matching voxel coordinate to:

| Exact raw RGB | Vector begin/end/capacity | Element |
| --- | --- | --- |
| (255,0,0) | +8/+0x10/+0x18 | signed32 voxel X,Y,Z,12 bytes |
| (0,255,0) | +0x20/+0x28/+0x30 | same |
| (0,0,255) | +0x38/+0x40/+0x48 | same |

These are unshifted integer voxel indices, not centered positions or fixed
world coordinates. Every occurrence is retained in its vector's scan order.
Extraction does **not** replace these raw RGB values with black or white.
Nonzero marker mode skips extraction after resetting the vector ends.
Near colors such as(254,0,0) are not attachment markers under these tests.

## 3. Three independent occupancy decisions

The hidden marker's raw storage and geometric treatment must be distinguished:

- **Geometry ownership:** mesher **0x170700**, filter call **0x170808 →0x16f230**,
  omits raw black(0,0,0). When sprite+0x99 is0 it additionally omits the exact
  pure red/green/blue triplets above. Those marker cells emit no cube faces.
  With nonzero mode they can own ordinary colored geometry.
- **Adjacent-face occupancy:** the mesher's separate six neighbor tests use
  raw black as empty. They do not invoke the marker filter for that neighbor.
  An in-bounds hidden nonblack marker therefore blocks the neighboring ordinary
  voxel's face toward it, even though the marker owns no geometry itself.
- **Corner-occlusion occupancy:** **0x171db0** likewise treats in-bounds raw
  nonblack marker voxels as occupied. Their weights can darken a neighboring
  corner. The existing [CUB color/V contract §3](steam-voxel-vertex-inputs.md#3-ordinary-cub-sprite-color-and-v)
  supplies its sampling/weighting and out-of-bounds rules.

Thus suppressing a marker's faces does not globally turn its stored voxel
into air for the other two decisions. Marker extraction and all three
occupancy decisions are independent of the later hair material tint.

## 4. Visible hair RGB and limits

For a geometry-owning voxel, **0x170700** normalizes its original RGB bytes
and passes that color through **0x170530** and the already specified CPU
color/occlusion encoder. The constructor's separate+0x9a=0 setting applies
the established final half-RGB/fourth-component scaling before packing;
marker mode+0x99 is not that scaling flag. The reached file-copy/mesh path
does not first replace surviving hair RGB with white, greyscale or the
character's chosen hair color. The shared CPU shading and quantization still
precede the queued material's shader composition.

This is not a filename-based rule for every resource containing the word
"hair": for example the native `mammoth-hair.cub` load, catalog ID0xca7,
uses nonzero marker mode. The complete catalog's other model families,
procedural/dynamic factories, later unrelated resource mutations and missing
or malformed asset recovery were not exhaustively audited here. No claim
is made about whether a particular owner's playable hair voxels are white,
colored or contain any markers; that is asset data, not a replacement palette.
