# Steam source prerequisite summaries — final held contract

This is a bounded original-Steam-only account of the rebuild of source+0x4D8 and source+0x490 in RVA 0x2E3510 and their reached prerequisite consumers in RVAs 0x2E17D0 and 0x2E3510. “Template” below means the source catalog value that the inspected admission route copies into a separately allocated resident object. “Identity membership” means the exact World+0x20 lookup described below. These are operational field descriptions, not an assertion of a broader death, quest, monster, door or reward label.

The supported image is binary/cubeworld.original.exe.unpacked.exe under C:/Users/DieuDeGlace/Documents/decompiled cube world steam. Its base is 0x140000000, timestamp 0x5D93AB0D, and verified SHA-256 is 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. All addresses below are RVAs in that image.

Filenames were identified before scoped reads. Principal original files are decompiled/functions/1402e3510.c, 1402e17d0.c, 1402da6c0.c and 14031c540.c. Selected original helpers include 1401473e0.c, 140146b10.c, 140115500.c, 140093730.c, 140286af0.c, 140016e00.c, 140016e50.c, 140073f40.c, 1400509e0.c, 14029d0d0.c, 1400f3e20.c and 1402d3a10.c. Exact native decisions and literal widths were checked in the individually selected original decompiled/assembly.txt listing and image. Earlier held documents were not used as behavior evidence and remain immutable.

## Owners, catalogs and different identity lanes

The loaded-source map is World+0x460. This is distinct from the source object's own catalog at source+0x460. The latter is traversed as tree entries containing a 64-bit identity at node+0x20 and a template pointer at node+0x28. Source+0x480 is a separate forward list of 64-bit identities, with each visited identity at its list node+0x10. Neither collection is the admitted resident list at World+0x40 or resident lookup map at World+0x450.

The template has a coordinate-key collection at +0x900 with native count at +0x908. Its visited key is the pair of 32-bit values at tree node+0x1C and +0x20. The rebuild does not read a source key's stored value as a weight. The template also has a separate 32-bit scalar prerequisite at +0x920, a coordinate prerequisite collection at +0x928 with count +0x930, an explicit identity prerequisite list at +0x938 with count +0x940, and a separate 32-bit prerequisite value at +0x948.

The source's rebuilt destination at +0x4D8 is a tree from a pair of signed 32-bit keys to a signed 32-bit counter. The count of its entries is at source+0x4E0. The separate source+0x490 value is a signed 32-bit scalar counter. The source+0x4C8 table read by the later keyed consumer supplies a required count for the object's coordinate key; it is not the rebuilt +0x4D8 destination.

There are distinct identities even within the template route. Rebuild membership uses the identity stored in the source catalog entry. The resident-admission checks use the identity stored at template+0x38. The template's +0x40 value is later copied to resident+0x1A0. No equality among these lanes is assumed merely from their positions or ordinary use.

## Source selection and rebuild order in 0x2E3510

RVA 0x2E3510 constructs a fresh temporary set of source pointers. It traverses World+0x40 in forward list order and admits actors whose byte+0x60 is zero into the selection route. For each such actor, its signed 64-bit fixed-point X and Y positions at +0x10 and +0x18 are converted to block coordinates with the original negative-fraction correction and 32-bit narrowing, then to signed floor-divided 64-block zone coordinates.

The source scan includes the center zone minus three through center plus three on both axes. X is the outer ascending loop and Y the inner ascending loop; both endpoints are inclusive. For ordinary representable coordinates this is a seven-by-seven square. The scan performs loaded-map lookups and inserts only existing nonnull source pointers into the temporary set, at the reached insertion call 0x2E3DBA. Missing entries and null payloads contribute no source. These inspected lookups do not create or enqueue a missing source.

The temporary set uses helper 0x73F40 with pointer-value ordering. Its comparisons are unsigned 64-bit comparisons and an already present pointer is not inserted again. A source selected by several actors, or encountered at more than one coordinate with the same pointer, is therefore rebuilt once in this set traversal. The subsequent source order is the set's ascending pointer-value order, not the earlier coordinate-scan or actor order.

For each selected source, the entire +0x4D8 tree is cleared first through 0x509E0 at call site 0x2E4473. That helper frees its nodes and resets its header and native entry count. The source catalog at +0x460 is then traversed in its existing forward tree order.

A catalog template contributes only if its native +0x908 key count is nonzero and the catalog entry's 64-bit identity is present in World+0x20. The membership comparisons are signed 64-bit comparisons. The key-count admission is checked at 0x2E4494 and the membership result controls entry into key traversal at 0x2E44E4. The selected code dereferences the catalog's template pointer; it does not add a null-template guard here. Invalid catalog payloads are outside the ordinary-data contract.

For an admitted template, every visited +0x900 key is looked up or inserted in source+0x4D8 through helper 0x1473E0 at call site 0x2E450E. New destination nodes receive that eight-byte key and an initial 32-bit counter of zero through node constructor 0x146B10. The returned counter is incremented once at 0x2E451A. An existing destination key receives another increment. No weight is taken from the template key entry's value.

The destination-key lookup and insertion use signed lexicographic comparison of the first 32-bit component followed by the second, as established by helpers 0x115500 and 0x1473E0. Template keys are visited in their source tree's forward order. There is no additional sort, reset per template, or deduplication of increments beyond reusing one destination node for an equal key.

After all catalog entries for this source have been processed, the source's +0x490 scalar is reset to zero at 0x2E45D1. The routine then walks source+0x480 in forward list order. Each list identity found in World+0x20 increments +0x490 once at 0x2E4632. This second pass does not require a catalog entry or a nonempty template key collection.

Consequently, +0x4D8 and +0x490 summarize different input collections. The first counts admitted template key visits. The second counts admitted identity-list visits. Neither rebuild consults World+0x450, a resident's health, or the number of objects admitted into World+0x40. World+0x20 is read as membership and is not changed by these selected counter steps.

Both counters use native 32-bit increments, without a proved saturation or overflow guard. The consumers below also add 32-bit values and use signed comparisons. No useful gameplay meaning is assigned to overflowed counts.

An empty selected catalog leaves +0x4D8 empty after its clear. An empty +0x480 list leaves +0x490 zero after its independent reset. Two admitted templates visiting the same coordinate key increment the one destination counter twice. Repeated equal identities in +0x480 are counted once for each visited list entry when that identity belongs to World+0x20. This describes the consumer of such entries; the legitimacy or deduplication of all upstream producers was not established.

A source not selected into the temporary set is not cleared by this rebuild pass. No freshness timestamp is tested by the summary consumers below. Their loaded-source preference can therefore select a value whose last rebuild was outside the presently inspected pass; no assumption that every loaded source has just been rebuilt is supplied.

## Ordinary order relative to resident admission

The inspected ordinary World step 0x2DA6C0 calls 0x2E3510 at 0x2DA73C. Later, after a test of World byte+0x358 at 0x2DA8CE, its zero case calls 0x2E17D0 at 0x2DA8DF. Thus the selected rebuild and the keyed consumer within 0x2E3510 precede this later resident-admission route in the ordinary call. The rebuild inside 0x2E3510 itself is not enclosed by a World+0x358 zero test.

RVA 0x2E17D0 also tests World+0x358 itself at 0x2E1856 and reaches its inspected admission body only when zero. It independently gathers existing nonnull loaded sources around the World+0x40 actors whose byte+0x60 is zero. Here the inclusive coordinate bounds are the actor's fixed-point position minus 0x1000000 through position plus 0x1000000 on each axis, converted to source zones. That is a 256-block extent each way, or the center zone minus four through plus four for ordinary representable positions. The source pointers are again deduplicated by pointer value.

Before traversing a selected source's template catalog, 0x2E17D0 calls source-preparation helper 0x31C540 at 0x2E1D7C. The inspected prefix of that helper compares the owning World's day with source+0x4F8 and can remove and free previously owned templates from the source catalog when the stamp changes. Its complete catalog-generation behavior was not audited. In particular, this contract does not assume that the catalog remains unchanged between the earlier summary rebuild and this later preparation call.

For each reached catalog template, 0x2E17D0 first looks up template+0x38 in World+0x450. A present nonnull resident causes the creation route for that template to be skipped, without a health predicate at this admission. If no such resident is present, the same identity is looked up in World+0x20; membership also skips creation. A null resident-map payload is treated like an absent resident for this decision.

The remaining candidate must be within the inspected proximity admission of at least one actor gathered in the earlier byte+0x60-zero list. Helper 0xF3E20 at call site 0x2E1E9F uses the template position and actor position with radius float 256, read from image RVA 0x42E5C4. It first bounds horizontal component magnitudes and then accepts the three-dimensional squared distance at or below the radius boundary. The fixed-point-to-block scale used there is the double at image RVA 0x43EF58, equal to one divided by 65536. A candidate outside every such actor test does not reach the scalar prerequisite below.

These are candidate admissions. Passing them or passing a single summary threshold is not by itself proof that a resident will be created, because further reached prerequisite gates follow.

## Scalar prerequisite in 0x2E17D0

A zero 32-bit value at template+0x920 bypasses the scalar-summary scan. A nonzero value starts a zero 32-bit sum and scans the candidate source's signed zone X minus eight through X plus eight, with Y minus eight through Y plus eight inside it. Both bounds are inclusive; the ordinary scan contains seventeen by seventeen coordinate pairs and visits X outside Y.

For each coordinate, the original loaded-map lookup is at 0x2E1F38. A nonnull loaded source supplies its +0x490 scalar directly to the 32-bit sum at 0x2E1F4F. It does so even when that scalar is zero. Retained state is not also added for that coordinate.

An absent map entry or null loaded pointer instead triggers a coordinate lookup in the retained map at World+0x10. If that retained record exists, its signed 32-bit value at retained node+0x80 is added at 0x2E202F. Missing retained state contributes nothing. The branch does not install a retained record, generate a source or queue missing work.

At 0x2E2079 the accumulated 32-bit value is compared with template+0x920 using a signed comparison. A sum below the required value rejects this candidate at this gate; equality passes to the next gate. There is no extra positive-value admission for a nonzero negative requirement and no clamp of a read loaded or retained scalar. Bounds, loop increments, the accumulator and comparison use their inspected native signed 32-bit widths; overflow and extreme-coordinate behavior are not assigned additional semantic guarantees.

## Other reached prerequisite admissions in 0x2E17D0

The coordinate prerequisite collection at template+0x928, with count +0x930, uses a different route from either summary. Empty count bypasses this gate. For each visited coordinate key, the routine derives its 64-block source coordinate from the signed 32-bit key components multiplied by sixteen, and queries the loaded-source map at 0x2E212E. An absent or null loaded source exits this prerequisite route without taking a retained-state fallback.

For a present source it walks source+0x4A8's object-index list. A signed index must be nonnegative and below the current element count of the source+0x60 array of 0xE0-byte records. A candidate indexed record must have nonzero byte+0x28 and its X/Y position must floor to the visited sixteen-block coordinate key. These are direct index, state-byte and position predicates; no business object name is inferred.

The native collection control uses one flag initialized before the whole coordinate-key traversal at 0x2E2097. A matching enabled indexed record clears it at 0x2E2276. The end-of-object-scan test at 0x2E227D advances to later template gates when the flag is still set. That route can stop key traversal early. The flag is not reset at the next key: after a match has cleared it, later keys do not restore it through this inspected loop. Exhausting the coordinate collection takes the candidate-skip route at 0x2E22D1. This exact reached behavior is not rewritten as an assumed all-keys or any-key business rule.

The separate 32-bit prerequisite at template+0x948 bypasses its gate when zero. Otherwise, the routine scans World+0x40 for an actor whose byte+0x14C equals 0xA1 and whose unsigned byte+0x19C, widened for comparison, equals that required value. The inspected match admits continuation. Its producing gameplay stage and wider role are outside this contract.

The explicit identity list at template+0x938 is checked against World+0x20 when its count at +0x940 is nonzero. Every visited list identity must be present; the first absent identity skips the candidate. An empty list passes this particular check. Repeated identities merely repeat the same membership requirement; the path does not require multiple resident instances or increment a counter for them.

When the complete reached admission succeeds, the route constructs a separate resident object and eventually registers it through 0x29D0D0 at 0x2E283A. That helper appends the resident pointer to World+0x40 and assigns it in World+0x450 under the resident's own +0x8 identity. The construction, attributes and later gameplay of that resident were not otherwise audited.

## Keyed summary prerequisite in 0x2E3510

All selected-source +0x4D8 and +0x490 rebuilds complete before the later selected-source record-consumer traversal in this same 0x2E3510 invocation. That traversal reaches 0xE0-byte records in source+0x60. The scoped keyed prerequisite is admitted only when World+0x358 is zero, the record's 32-bit type is 8 or 10, and its state byte+0x28 is nonzero.

The record's X and Y fixed-point positions at +0x8 and +0x10 are converted to signed sixteen-block coordinate keys with the original negative-coordinate correction and narrowing. The source's +0x4C8 table is queried for that pair through 0x93730 at 0x2E5352. No matching required-count entry skips this prerequisite-success route. A found entry supplies its signed 32-bit required count from node+0x24.

The routine then zeroes a 32-bit sum and scans source-zone coordinates covering the record position minus 0x4000000 through position plus 0x4000000 on both horizontal axes, with inclusive bounds. That fixed-point extent is 1024 blocks each way; for ordinary representable positions the resulting zone rectangle is thirty-three by thirty-three. X is the outer ascending loop and Y the inner one.

For each coordinate, 0x16E00 at call site 0x2E53E9 returns an existing loaded source pointer or zero. A nonnull pointer selects that source's +0x4D8 tree, queried for the same record coordinate key at 0x2E5408. A matching entry contributes its signed 32-bit counter. A missing key contributes nothing and does not trigger retained fallback while the source pointer is nonnull.

Only an absent or null loaded source takes the retained route. The coordinate lookup in World+0x10 is through 0x286AF0 at 0x2E543A. A found retained record has its +0x70 tree queried for the same key through 0x93730 at 0x2E5458. A matching retained key contributes its 32-bit counter; a missing retained record or key contributes nothing. These helpers perform lookups without creating a record or a source.

The accumulated 32-bit result is compared with the required count at 0x2E54A3 using a signed comparison. Equality or a larger sum reaches 0x2D3A10 at 0x2E54B6 with the supplied state value zero. The reached helper writes record byte+0x28 to that value when it differs. If the saved state byte changed, the parent path inserts the source into World+0x1A0 at 0x2E54E4. The helper's other event/timer effects and the below-threshold alternative interactions are outside this prerequisite-summary contract.

No extra positive-value check is imposed on the required-count entry. The sum is built by native 32-bit additions without a proved saturation guard. A zero or absent loaded contribution is not combined with an older retained value for that same coordinate.

## Limits and held status

The loaded-versus-retained rules differ from source creation: the two summary scans prefer a nonnull loaded source and consult retained state only for an absent or null source. The coordinate-index prerequisite in 0x2E17D0 uses loaded sources only. None of those inspected lookup branches supplies source generation, prefetch or a missing-work queue.

This audit establishes the selected rebuild and reached consumers. It does not establish every producer of source+0x460 templates, source+0x480 identities, template key collections, source+0x4C8 requirements or World+0x20 membership; all constructor, restoration, alternate caller and indirect summary writers are not closed. It does not infer a business label from adjacency, numeric type identifiers or set membership.

The ordinary call order is proved, but no universal synchronization or source-pointer lifetime guarantee is supplied. Concurrent changes to templates, identity membership, loaded-map entries or retained data, source-preparation generation details, exceptional paths and additional callers remain unresolved. The selected routines and branches do not themselves establish that every summary read observes an immediately preceding rebuild of that source.

FINAL / STABLE / HELD after complete final readback. This is a new bounded original-only contract. Every earlier held file remains unchanged. Exposure status: clean; only the standalone author policy and individually selected original Steam evidence were read. Further writes stop after readback.
