# Steam native sprite catalog: bounded registry and data contract

Independent native-only specification. Sources are the original Steam executable,
its native disassembly/decompilation and existing independent prose. No SDK,
engine, exporter, test, reconstruction, owner output/capture or game execution was
used. This establishes an indexed catalog and a limited set of sprite data fields;
it does not establish a complete class, every live sprite or a finished adapter.

## 1. Image identity and verified lookup address

Addresses are RVAs in the analyzed original unpacked AMD64 image
`cubeworld.original.exe.unpacked.exe`, preferred base **0x140000000**, SHA-256
`7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244`.
This identifies the analyzed image, not every game executable or modified build.

The bounds-checked catalog lookup is exactly **RVA 0x17190**, hence preferred
virtual address **0x140017190**. Its native bytes were rechecked independently.
The direct call at **0x1e620f** also targets that exact entry. **0x171900 is a
different address**, inside the mesh routine beginning at 0x170700; it is not
the lookup specified here. Leading-zero formatting must not add a trailing zero
to the lookup RVA.

## 2. Owner and primary registry layout

Ordinary startup separately requests **0x80 bytes** at **0x136105–0x13610a** and
constructs the catalog through **0x172f60 at 0x13611a**. The returned catalog
pointer is supplied to World constructor **0x29a130**, which stores it at
**World+0x70 at 0x29a1ec**. The client World itself is reached through the native
controller's +0x08 pointer, as bounded in
[the tick-context provenance contract](steam-mod-loader-tick-context.md).

Thus the registry is inside a separately allocated catalog reached through a
World pointer field; it is not an inline sprite array inside World. The ordinary
host/client path shares that catalog input, as established in
[World ownership §1](steam-local-world-zone-ownership.md#1-bounded-conclusion-and-object-identities).
That sharing does not make the catalog's resource caches immutable.

| Catalog offset | Established content |
| --- | --- |
| +0x10 | Eight-byte pointer to the primary sprite-pointer array's beginning |
| +0x18 | Eight-byte pointer immediately beyond its live elements |
| +0x20 | Eight-byte pointer immediately beyond its allocated capacity |

Each live element is an **eight-byte sprite pointer**. Native helper **0x1e6640**
derives the element count from the beginning/live-end byte span using an
arithmetic right shift by three. The allocation-end field is a capacity bound,
not an additional live-element count. The registry itself has no separate count
integer in those three fields.

Lookup **0x17190** receives the catalog pointer in RCX and a **signed32 index in
EDX**. It rejects negative indices. Its upper-bound comparison uses the low32
bits of the derived element count as a signed32 value. An admitted index returns
the eight-byte pointer stored at that index; it does not test the sprite's
dimensions, RGB pointer, file-load outcome or GPU readiness. An in-range stored
null consequently remains a null result. This is an ordinary initialized-vector
contract, not validation of arbitrary corrupt pointer spans.

## 3. Construction, population and ordering

Catalog constructor **0x172f60** initializes the primary vector fields to zero,
constructs the directory string **sprites**, and invokes **0x177f50**. At
**0x177f91–0x177f99**, that routine requests **0xe44 = 3652 live entries** through
resize helper **0x1e63d0**. Newly added pointer entries are zero-filled by that
helper. The ordinary startup index range is therefore **0 through 0xe43**.

The loop **0x177f9e–0x178040** visits indices in ascending order, requerying the
current element count through 0x1e6640. Its index and comparison are signed32.
For each index it requests **0xa8 bytes**, invokes sprite constructor
**0x16e6b0 at 0x177ffa**, and writes the resulting pointer into the selected
array slot at **0x17803d**. The explicit allocation-failure branch instead
supplies null. This branch does not establish a supported complete startup with
missing sprite allocations, since later resource-loading calls have their own
admission requirements.

After that allocation loop, the large population routine uses explicit catalog
indices and file paths, calling the shared CUB loader **0x16f320**. Registry order
is therefore **catalog-ID order**. It is not filename order, directory enumeration
order, a count of distinct filenames or a count of successful loads. The complete
3652-slot filename/loading-coverage census is outside this bounded contract.
Verified examples of particular file bindings remain in
[the hair-resource contract](steam-pose-sprite-markers.md) and
[the stone-stool provenance contract](steam-stone-stool.md); their asset contents
were not read for this audit.

One directly reached registry consumer is **0x1e6202–0x1e623a**: it looks up
catalog index 0xdcc through 0x17190 and places that returned sprite pointer into
a separate catalog-side table at +0x68. This is pointer reuse, not evidence that
the primary vector is a list of draw descriptors. Ordinary pose selection also
uses 0x17190, as independently described in the hair contract. Item resolution
through **0x173b10** receives this same catalog owner but has additional category
and cache behavior; it is not equivalent to enumerating the primary vector.

## 4. Established sprite allocation and raw voxel fields

The observed **0xa8-byte allocation request** does not supply a complete C++ class
declaration. Only the following resource fields are assigned meanings here:

| Sprite offset | Established meaning |
| --- | --- |
| +0x68 | Eight-byte pointer to the sprite-owned, tightly packed raw RGB voxel allocation |
| +0x88 | Signed32 full voxel-allocation X count |
| +0x8c | Signed32 full voxel-allocation Y count |
| +0x90 | Signed32 full voxel-allocation Z count |

On the ordinary file-stream route, **0x16f320** reads three four-byte dimension
values in X/Y/Z order, followed by the RGB payload. These are native little-endian
integer counts, not floating scales or occupied-geometry bounds. Copy helper
**0x170030** assigns the dimensions, releases any previous +0x68 allocation, and
copies the new raw RGB bytes into sprite-owned storage. For ordinary positive,
nonoverflowing dimensions, each voxel occupies three consecutive unsigned color
bytes, red then green then blue. **X advances fastest, Y next, Z slowest**.
There is no voxel alpha byte in this allocation.

The copied data retains the raw owner RGB values, including attachment-marker
colors. It has not undergone the CPU vertex color encoding or shader processing
specified in [the voxel vertex input contract](steam-voxel-vertex-inputs.md). Marker
extraction and independent geometry/occlusion decisions are separately bounded
in [the marker contract §§2–3](steam-pose-sprite-markers.md#2-raw-colors-and-attachment-coordinate-extraction).
They do not change the identity of the raw RGB payload.

**No independent per-sprite scale field is established by this slice.** The
dimensions specify voxel counts; their conversion to scene units belongs to the
reached consumer's transform. No unproved field is labeled scale, occupied bounds
or a universal radius. Other sprite members remain outside this field contract.

## 5. Lifetime, readiness and separate resource families

Catalog cleanup **0x1730d0** walks the primary indices forward, using the live
begin/end-derived count and invoking cleanup on nonnull sprite pointers. Sprite
cleanup **0x16e950** releases the raw +0x68 allocation; **0x16e9f0** additionally
releases sprite storage when its deletion argument requests that operation.
Catalog wrapper **0x1734d0** calls catalog cleanup and conditionally releases the
catalog allocation. These paths establish borrowed resource/payload lifetimes,
not permanently valid numerical addresses. Reload through 0x170030 can replace
a payload allocation without changing the outer sprite pointer.

Nonnull primary membership is not a successful-load or readiness predicate.
Constructor 0x16e6b0 starts the raw RGB pointer and dimension fields at zero. A
failed first file-open route in 0x16f320 can return with that empty resource still
occupying its catalog slot. Separately, **0x16eba0** requires nonnull backend
resource pointers at **sprite+0x70/+0x78** and successful readiness results from
both. Lookup 0x17190 does not perform that test. Raw voxel availability, completed
file loading, mesh creation and backend readiness must not be conflated.

The primary vector is **not an exhaustive registry of all live sprites**.
Procedural category **0x17/23** items demonstrably use the separate cache at
**catalog+0x28**, keyed by item ID and modifier. **0x1737b0** creates/caches those
resources without appending them to the primary vector. Their exact cache layout
and generation-stage limits remain in
[the procedural-resource contract §§1–2](steam-procedural-item-resources.md).
Other factories and resource families were not exhaustively audited here.

No synchronization or lifetime extension is supplied by the simple lookup or
pointer enumeration itself. Concurrent construction, reloading, cache insertion,
teardown, later registry replacement/compaction and external mutation are not
closed by this audit. Malformed dimensions, truncated files, overflowing products
and allocation-failure recovery are likewise not given a complete contract.
These are native layout and reached-path facts, **not a runtime-capture oracle,
full-world/content-coverage claim, exhaustive live-resource registry, complete
class layout or proof that an independent capture adapter is finished**.
