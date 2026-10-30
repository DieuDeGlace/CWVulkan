# Steam stone-stool mapping, size and missing owner geometry

Independent clean-room facts from the original native executable, its
catalog/size-table data and a read-only filename check of the owner's current
installation. No engine, tests, exporter or third-party reconstruction was
inspected. No game was executed and no asset was created or modified.

## 1. Native name and exact mapping

**`stone-stool` is a native asset name, not an invented placeholder.** The
original executable contains **`stone-stool.cub` at RVA 0x461620**. Model-store
constructor **0x172f60** supplies the directory **`sprites`** to catalog
loader **0x177f50**. Its filename construction at **0x1d6e83–0x1d6e9a** uses
that exact filename. The load at **0x1d6ec3–0x1d6ee8** targets catalog index
**0x0d62 (3426)** through the ordinary CUB loader **0x16f320**.

The native assignment at **0x1e48b6–0x1e48ee** installs catalog 0x0d62 in
the **static-object model table at model-store+0x50**, for **object type
0x1b (27)**. The runtime lookup is **0x1e6330**. This is a static-object ID,
not zone scenery model ID 0x1b and not a special procedural-geometry marker.

Adjacent mappings are independently assigned and must not be substituted
merely because their files exist:

| Static-object type | Catalog index | Exact native filename | Filename source | Table assignment |
| --- | --- | --- | --- | --- |
|0x1b (27)|0x0d62 (3426)|stone-stool.cub|0x461620; use 0x1d6e83|0x1e48b6–0x1e48ee|
|0x1c (28)|0x0d64 (3428)|stool.cub|0x461648; use 0x1d6f73|0x1e48f1–0x1e4929|
|0x1d (29)|0x0d63 (3427)|sandstone-stool.cub|0x461630; use 0x1d6efb|0x1e492c–0x1e4964|

## 2. Exact native physical size; no provisional extent is needed

**0x13a520** maps type 0x1b to its actual default size **(1,1,1) blocks**.
This is verified both from the routine's cases and the original two-level
dispatch data: type 0x1b selects branch index 32 from **0x13a8d0**, which
selects **0x13a834** through **0x13a84c**. The selected branch writes three
float 1.0 values. The default here is native behavior, not an unknown size
estimated from a missing asset.

**0x13a960** multiplies each of those three extents by the static-object
record's **float scale+0x24**. For scale s, the physical extents are therefore
**(s,s,s)**. **0x13a470** supplies visual factor **1** for type 0x1b. For
comparison, ordinary stool type 0x1c has physical extents **(1,1,0.5)** before
record scale; sandstone-stool type 0x1d uses the same (1,1,1) default as stone.
The distinct ordinary-stool Z extent is not evidence for changing type 0x1b.

## 3. Generic static-object pose, using the actual CUB dimensions

Object renderer **0xe0ac0** looks up the static-object table using 0x1e6330
and handles type 0x1b in the generic asset-backed path. It has no stool-specific
deformation, hinge, slide, sway, or animation-clock transform. Its state/time
fields do not supply an additional pose for this type.

Let the actual successfully loaded CUB's voxel dimensions be Vx,Vy,Vz.
For this type, the uniform voxel scale is **s/Vx**, because the native generic
rule is max(scaled physical X,scaled physical Y)/Vx times visual factor 1.
Subtract **Vx/2 and Vy/2** from local voxel X/Y, with **no voxel-Z centering**.
Apply uniform scale and the static Z rotation, then translate to the record's
signed64 fixed position **+0x08/+0x10/+0x18**, expressed in blocks. The rotation
uses the **full unsigned turn byte+0x20 times 90 degrees**, with the existing
float radians conversion from `worldgen-parity-gaps.md` §4.1. The object
position is the model's horizontally centered bottom, not its geometric center.

Consequently physical extent s in Z does not independently rescale the CUB
height to s: its rendered height would be **s×Vz/Vx**. The missing file's Vx,
Vy,Vz cannot be inferred from the physical size table or from another stool.

## 4. Current owner-file absence and what remains unresolved

On **2026-10-30**, the exact owner path
**`D:\SteamLibrary\steamapps\common\Cube World\sprites\stone-stool.cub`**
does not exist. The directory does contain `sandstone-stool.cub`, `stool.cub`,
`stool-white.cub`, `stool02.cub` and `stool03.cub`. This is a statement about
this installation's files, not proof that every distribution omits the asset.

This stock catalog load uses the filesystem path: the loader's archive-pointer
slot is set to zero at **0x1780ad** and is passed unchanged to the stone-stool
load. The ordinary CUB reader **0x16f320** reads the file's three 32-bit dimensions
and its RGB voxel data before calling the mesh-construction routine 0x170030.
Its file-open failure path **0x16f51c–0x16f55e** returns without that construction.
It does **not** substitute another filename, construct a primitive stool or
generate replacement voxels. Its separate archive-miss path likewise returns.

The catalog allocates sprite objects through **0x16e6b0** before loading.
Their dimensions at **+0x88/+0x8c/+0x90** start at zero; geometry vectors start
empty and resource pointers **+0x70/+0x78** start null. A failed first file
load therefore supplies no initialized stool voxel geometry or useful positive
dimensions, even though the catalog/table slot itself can remain non-null.
No new fallback geometry follows from this specification.

The native name, type mapping, physical extents, scale factor and pose rule
are fully verified. The remaining unresolved data is the **actual owner
stone-stool CUB payload and its dimensions**, if a valid copy exists. Resolving
that requires the owner's exact asset or a labeled native catalog capture
showing a successfully populated 0x0d62 model, its dimensions and geometry
payload. Merely observing a non-null catalog pointer is insufficient.

A data check can therefore classify type 27's physical size as verified while
retaining the missing-owner-model warning. Renaming it to another asset,
silently drawing another stool, or synthesizing a shape would not establish
native geometry parity. No additional game launch is needed to establish the
already verified mapping and physical sizing facts above.
