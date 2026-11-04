# Steam catalog indexed side resources

Independent native-only supplement to the [primary catalog contract](steam-native-sprite-catalog.md). This bounded audit covers the catalog's three side vectors, their ordinary construction/population/cleanup, and the directly reached scene-submission consumers. Only original executable/disassembly/decompilation and finalized independent prose were read. No replacement implementation, SDK, exporter, test, probe, owner capture/package, third-party reconstruction or game execution was used. This is not a complete writer or lifetime census.

## 1. Supported image and owner

All addresses below are RVAs in the original unpacked AMD64 image `cubeworld.original.exe.unpacked.exe`, preferred base **0x140000000**, SHA-256 **7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244**. The binary hash was checked for this audit. These facts do not cover every distribution or modified executable.

The catalog is the separately allocated **0x80-byte** owner reached through **World+0x70**; its backend input is at **catalog+0x08**. The primary sprite vector is at **+0x10/+0x18/+0x20**. Those owner/provenance facts remain bounded by the primary catalog contract. Offsets here are relative to that catalog, not to World, a sprite or a buffer wrapper.

## 2. Three vector wrappers and lookup ABI

Each side member is an inline **24-byte vector wrapper** containing three eight-byte pointers. Each live element is one **eight-byte borrowed sprite pointer**, not an ID, draw descriptor, RGB address or GPU interface.

| Side member | Begin | Live end | Capacity end | Checked getter RVA | Ordinary live length |
| --- | --- | --- | --- | --- | --- |
| +0x38 | +0x38 | +0x40 | +0x48 | **0x1739f0** | **0x64 / 100** |
| +0x50 | +0x50 | +0x58 | +0x60 | **0x1e6330** | **0x86 / 134** |
| +0x68 | +0x68 | +0x70 | +0x78 | **0x1e6300** | **0x86 / 134** |

All three getters receive **RCX = initialized catalog pointer**, **EDX = signed32 side index**, and return the stored pointer in **RAX**, or zero for a rejected index. They reject negative indices; the upper bound is the begin/live-end byte span arithmetically shifted right by three, compared as a 64-bit unsigned count against the sign-extended admitted index. They do not use the capacity end as the live bound. This differs from primary lookup **0x17190**, whose upper comparison uses a signed32 count.

None of the side getters validates the pointed-to sprite, tests its dimensions or raw RGB, binds buffers, extends its lifetime, inserts a value or acquires a lock. An admitted null slot returns null. This contract assumes an ordinary stable initialized wrapper; it is not validation of corrupt pointer spans.

The separate address helper **0x1734c0** takes a vector-wrapper address and an index and returns the address of that eight-byte element. It supplies no bounds check. Population uses that helper to write slots; it must not be confused with a checked catalog getter.

## 3. Construction and ordinary population

Constructor **0x172f60** zeros all nine side-wrapper pointer fields, initializes the separate tree at +0x28/+0x30, and calls the large population routine **0x177f50**. The side-population region follows the primary resource allocations and file-load calls. It performs these operations in order:

| Operation | Native evidence |
| --- | --- |
| Resize +0x38 to 0x64, with null as the fill value | **0x1e2d30–0x1e2d58** |
| Write +0x38's selected aliases | **0x1e2d5d–0x1e42c6** |
| Resize +0x50 to 0x86, with null as the fill value | **0x1e42c9–0x1e42f1** |
| Write +0x50's selected aliases | **0x1e42f6–0x1e6121** |
| Resize +0x68 to 0x86, with null as the fill value | **0x1e6124–0x1e614c** |
| Write +0x68's five selected aliases | **0x1e6151–0x1e6275** |

The resize helper is **0x1e64d0**. Its reached fill helper **0x172f30** copies an eight-byte fill value to each added element. Capacity growth passes through **0x1736c0/0x173630**, retaining existing live elements and replacing the vector's element storage when needed. Consequently an element-address borrow has a different invalidation boundary from the sprite pointer stored there.

Every explicit side-slot assignment in the above region first calls primary lookup **0x17190** for a fixed catalog ID, then stores that returned pointer. The assignments do not clone a sprite, allocate another voxel resource or load another CUB. Ordinary membership is an alias of primary membership, including a possible returned null. The tables are indexed by their consumers' record-type domains, independently of primary catalog IDs. They are not filename-sorted lists or packed lists of only nonnull entries.

The following are complete slot-to-primary-ID bindings for this bounded trailing population region. All numbers in these binding tables are hexadecimal. Each row's IDs correspond in order to the inclusive side-index range in its first column; `null` means that the initial null fill receives no later assignment in this region. These are mapping data, not claims that the corresponding file loads succeeded.

### +0x38 bindings

There are **93 explicit assignments** and **7 unassigned/null slots**. Explicit write order is not monotonically increasing side-index order: for example, 0x10 is written before 0x0f, and 0x32 before 0x2f. Consumers use the resulting index, not the chronological position of a write.

| Side index range | Primary catalog IDs, in side-index order |
| --- | --- |
| 00–0e | cf6, cf7, cf8, cf9, cfa, cfb, cfc, cfd, cfe, cff, d00, d01, d02, d03, d04 |
| 0f–1d | d31, d33, d32, d05, d06, d07, d08, d09, d0a, d0b, d0c, d0d, d0e, d0f, d10 |
| 1e–2e | d30, null, d11, d12, d13, d14, d1b, d1c, d1d, d1e, d1f, d20, d21, d22, d23, d24, d25 |
| 2f–3f | d26, d27, d28, d29, d2a, d2b, d2c, d2d, d2e, d2f, d34, d35, d36, d37, d38, d3a, d39 |
| 40–4b | d3e, d3e, d3e, d3e, c32, c33, c34, c35, d3f, d40, d41, d42 |
| 4c–58 | null, null, null, null, null, d18, d19, d1a, dd1, e28, e29, e2a, e2b |
| 59–63 | null, d43, d44, d45, d46, d47, d48, d49, d4a, d4b, d4c |

In particular, **0x40–0x43 all alias primary ID 0xd3e**. Four occupied side slots therefore do not establish four distinct sprite objects or four distinct payloads.

### +0x50 bindings

There are **131 explicit assignments** and **3 unassigned/null slots: 0x11, 0x1a and 0x61**. Here too, write order is explicit and sometimes nonmonotonic, for example 0x4a–0x52 are written before 0x43–0x49.

| Side index range | Primary catalog IDs, in side-index order |
| --- | --- |
| 00–10 | d4d, e2e, e31, d4e, d4f, d50, d51, d52, d53, ddf, de1, de3, b7a, de5, de6, e32, de0 |
| 11–20 | null, d54, d55, d56, d57, d57, d5b, d5f, d60, null, d62, d64, d63, d65, d66, d67 |
| 21–30 | d68, d6a, d6b, d6c, d6d, d6e, d6f, d70, d71, d69, d72, d73, d74, d75, d76, d77 |
| 31–40 | d78, d79, d7a, d7b, d7c, d7d, d7e, d7f, d80, d81, a4a, d82, d83, d84, d85, d86 |
| 41–50 | d88, d89, d8a, d8b, d8c, d8d, d8e, d8f, d90, c72, c73, c74, c75, c76, c77, c78 |
| 51–60 | c79, c7a, da9, dc5, dc7, dc9, dcb, dcd, e0f, e10, e11, e12, daa, dab, dac, dad |
| 61–70 | null, d91, d92, d96, d97, d93, d94, d95, dba, dd0, dd2, dd3, dd4, dd5, dd6, e19 |
| 71–80 | e1a, e1b, e1c, c7d, d98, afe, af8, af9, afa, afb, afc, afd, aff, dce, d15, d16 |
| 81–85 | d17, e22, dd7, dd8, dd9 |

**Side indices 0x15 and 0x16 both alias primary ID 0xd57.** This does not merge those two record types or their separate transform behavior.

### +0x68 bindings

Only these five indices are explicitly assigned; **all other live indices in 0x00–0x85 remain null** after this ordinary population region.

| Side index | Primary catalog ID | Lookup / slot-address call / final store RVA |
| --- | --- | --- |
| 54 | dc6 | **0x1e615e / 0x1e617c / 0x1e6189** |
| 55 | dc8 | **0x1e6199 / 0x1e61b7 / 0x1e61c4** |
| 56 | dca | **0x1e61d4 / 0x1e61f2 / 0x1e61ff** |
| 57 | dcc | **0x1e620f / 0x1e622d / 0x1e623a** |
| 82 | e23 | **0x1e624a / 0x1e6268 / 0x1e6275** |

## 4. Reached consumer meanings

The audit establishes two record-type domains and an additional resource for selected types. It does not establish universal native class names from field resemblance.

**+0x38: the first scene-record domain.** Routine **0xd3170** walks a reached container of **0x40-byte records**, uses their leading signed32 type as the side index, and calls **0x1739f0 at 0xd39d9**. Type **0x59** takes another branch before this lookup. A null result skips that resource path. A nonnull result supplies the sprite's X/Z allocation dimensions for extent/culling/placement work and reaches scene submission **0xee2f0 at 0xd3f09**, with that same sprite pointer. This establishes scene-model use, rather than a table of per-instance records. As one content anchor, side index **0** aliases primary **0xcf6**: the original string at **0x460e38 is `flowers.cub`**, passed through path construction at **0x1d3be3–0x1d3bfa** and loaded into that primary slot by **0x16f320 at 0x1d3c48**. This one anchor does not classify every entry as vegetation.

**+0x50: the second scene-record domain.** Routine **0xe0ac0** receives a record pointer in RDX and uses the record's leading signed32 type to call **0x1e6330 at 0xe0c0b**. It uses sprite allocation dimensions to form instance transforms and submits the retained sprite to **0xee2f0**. The ordinary main-resource branch tests the returned pointer before dimension use. Types **0x15/0x16** also submit their initially selected resource and then select side index **0x17**, through the getter call at **0xe103b**, for subsequent work. The two former types share primary **0xd57** while **0x17 maps to 0xd5b**. As another content anchor, side index **0** aliases primary **0xd4d**: string **`goddess2.cub` at 0x455bb8**, path construction **0x1d64ab–0x1d64c2**, primary load **0x1d6510**. The second domain is not the numerical primary catalog-ID domain, even when an integer occurs in both.

**+0x68: selected additional components in that second domain.** The same **0xe0ac0** routine calls **0x1e6300 at 0xe1072** for record types **0x54–0x57**, preserving that type as the index. It null-checks the extra resource, uses its dimensions/record state in a separate transform, and submits it at **0xe172a**. Type **0x82** has a corresponding extra-resource lookup at **0xe1773**, also null-checked. The ordinary +0x50 resource is retained separately. Thus the +0x68 values are additional sprite components for selected records, not replacement primary entries or a third procedural-generation mechanism. Side **0x54 → primary 0xdc6** has original string **`cage-door.cub` at 0x462f60**, path construction **0x1df8b0–0x1df8c7**, primary load **0x1df915**. This supports a concrete door component; it does not prove a semantic name for every other component.

Submission helper **0xee2f0** receives the sprite pointer together with instance transform/parameters. Its reached branches store the resource pointer as a grouping key or in queued instance data. Neither a side-vector slot nor that queue entry is another sprite allocation or a CPU vertex array. Complete instance-transform equations, every downstream draw and all record-layout fields are outside this note.

## 5. Cleanup, borrowed lifetime and readiness

Catalog cleanup **0x1730d0** first destroys nonnull sprites in the primary vector, then nonnull values in the separate +0x28 cache. It subsequently frees the side-vector element allocations in **+0x68, +0x50, +0x38 order**, clearing each corresponding pointer triple. It does **not** run sprite cleanup once per side element. This is direct evidence for non-owning alias elements and prevents treating repeated side membership as separately owned resources. The cache's nodes/head and primary-vector storage are also released on that cleanup route. Deleting wrapper **0x1734d0** calls cleanup and conditionally frees the catalog allocation.

The side pointer arrays can therefore still contain aliases during an interval in cleanup after their primary pointees have been destroyed. A nonzero begin pointer or occupied slot during teardown is not permission to dereference it. A caller needs a stable catalog-and-sprite lifetime; the getters provide no ownership increment or lifetime extension. Reallocation invalidates side-element addresses, while raw RGB reload or remeshing can replace a sprite's payload/buffers without changing its outer pointer. Those payload/buffer lifetimes remain in the primary and [buffer-resource contracts](steam-sprite-buffer-resources.md).

Ordinary side population happens after the earlier primary load calls in the same constructor call. This establishes program order, **not successful loading or GPU completion**. Primary constructor **0x16e6b0** begins with zero RGB/dimensions/buffer pointers; a failed first file open can leave an existing empty sprite. The side alias store does not inspect that state. None of the three side getters calls **0x16eba0**. On the ordinary backend, that latter helper's concrete buffer calls bind IA state and return byte 1; they are not GPU-fence checks. Null-slot coverage, sprite existence, raw RGB availability, completed meshing, nonempty geometry, successful binding and GPU readiness remain different facts.

## 6. Actual protection boundary and the separate cache

The audited checked getters are plain bounds-check/load operations with **no lock, atomic snapshot, generation counter, ready flag or ownership acquisition**. The population writes, wrapper growth and cleanup above do not establish a reader-protection protocol for a concurrent observer. The reached scene consumers demonstrate use in the native rendering path; that observation alone does not prove thread exclusivity, immutability or protection against reload/destruction. **No named mutex or world/zone snapshot lock has been proved here to protect the side wrappers, their sprite pointees, raw RGB or GPU wrappers as one lifetime.** A full writer census, reload/replacement scheduling and an externally usable protection scope remain unresolved. No universal capture lock is inferred.

**+0x28/+0x30 remain the existing procedural cache, not a fourth indexed side vector or another newly discovered family.** The finalized [procedural-resource contract §§1–2](steam-procedural-item-resources.md) establishes its tree head/count, signed32 ID/modifier key ordering, eight-byte resource value and category **0x17** lookup/generation route **0x1737b0**. Cache misses allocate resources without appending them to the primary catalog. That lookup publishes the pointer before generation finishes and has no established ready flag/mutex. The alias-only trailing side population audited here does not alter that contract or make the cache safe to enumerate concurrently.

The mapping/count statements in §3 describe this image's ordinary construction path. They do not prove that no other path can mutate a wrapper, that every instance can be reached through these three domains, or that these vectors plus the primary/cache exhaust all live sprites. No runtime oracle, asset-content census, independent adapter completion or whole-world capture coverage is claimed.

## 7. Finalization

**FINAL / STABLE / HELD.** The bounded note was read back in full after writing. No older finalized note was edited. Unproved protection, complete writer coverage and lifetime scheduling remain explicitly unresolved.
