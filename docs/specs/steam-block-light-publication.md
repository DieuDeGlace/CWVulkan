# Steam stored block-light publication and lifetime

Independent native-only clean-room facts. RVAs refer to the Steam executable.
This covers channel0 queries, the ordinary terrain mesh job and directly reached
invalidation paths. No game execution, owner captures or implementation sources
were used. The solver's existing mathematical contract and its water-mesh
consequences remain in [voxel inputs §7.5](steam-voxel-vertex-inputs.md#75-water-optimization-observes-post-solver-terrain-red-bytes).

## 1. Storage and completed-solve writeback

The relevant persistent storage is the loaded zone's ordinary six-byte block
vector, not a renderer-owned second gameplay-light volume. A zone's columns begin
at+0x518, have stride0x58 and use localX×64+localY. Each column stores signed32
baseZ at+0x3c and its block-vector begin/end at+0x40/+0x48. The six bytes are
red, green, blue, auxiliary light, type and flags. The auxiliary light is byte3;
the flags are byte5. Sources: column lookup0xdfb50, effective reader0xeb90 and
light reader0xda830.

Mesh job0xe9530 captures a3×3 table of actual Zone pointers through0xcbfc0.
The table records center zoneXY, then nine pointers in X-major/Y-minor order
for center−1 through center+1 on each axis. Lookup0xdfb50 uses these pointers
to return the original zone's column; it does not return a copied voxel column.

At0xe96cb the job constructs a separate temporary light volume through0xcb9c0,
then calls solver0x2a3260 at0xe96f2. The ordinary target rectangle is the
zone's64×64 block columns, with an8-column halo for the temporary volume.
Each temporary column entry has stride0x20: baseZ at+0 and a byte-vector
begin/end/capacity at+8/+0x10/+0x18. Its allocation length follows the
corresponding stored column's block count. These are scalar light arrays;
constructing them does not clone the columns' six-byte block records.

After completing propagation,0x2a3260 copies results back to the target
rectangle. It visits increasing X, increasing Y within X and increasing stored
block index within each column. There is a Sleep(0) yield before processing
each X's columns. Missing columns are skipped. Every existing stored record
whose raw type is0 or2 receives the completed temporary light byte as its new
red value. The other five bytes retain the values read from that record.
The six-byte put uses0xa36e0; the final-loop call is0x2a3d95. This is a
record write, not an atomic publication of the whole target light volume.
Ordinary block readers also read this changed record; no subsequent Game-side
copy makes its red visible. Effective-reader substitutions still take precedence
over a stored record as described in §3.

Only the target zone's stored type0/2 records are changed by this copy.
Neighboring halo columns are not published back by this target job. Solid and
other non0/2 records retain their original red, green and blue. No column is
extended, no implicit above/below-column block is allocated, and no green,
blue, auxiliary-light, type or flags value is newly calculated by this copy.
Temporary samples outside the padded XY domain or above its stored vertical
range are255; below its base they are0. These defaults do not cause additional
physical block allocation.

The copy finishes before0x2a3260 returns. The mesh job then samples this solved
temporary volume into model lighting fields and constructs terrain geometry.
Thus gameplay storage already contains the new red values while later CPU
geometry work and GPU-resource publication are still pending. The model/vertex
samples and gameplay query are related consumers of the solve, but are not
interchangeable: the gameplay query has the type, auxiliary-byte and minimum5
rules below.

## 2. Lock and publication boundaries

The neighborhood capture is inside World+0x3f0's critical section:
enter0x2abb80 at0xe962e, capture0xcbfc0 at0xe9684 and leave0x2b98a0 at0xe968e.
That lock has already been released when the temporary volume is constructed
and the solver is called. Neither the solver's final copy nor the reached
in-range0xa36e0 put enters that critical section.

The later mesh installation uses a different critical section, World+0x3c8:
enter0x2abb70 at0xec71b and leave0x2b9890 at0xecb28. This protects the late
mesh-cache/pending-group installation described in
[scene lighting §1](steam-scene-lighting.md#1-cached-zone-mesh-fields-readiness-and-capture-coherence).
It is not the stored-red publication boundary: those writes happened earlier.
Worker0x842d0's subsequent pending-to-active GPU-group installation does not
perform another gameplay-light copy.

Readers0xda830/0xdab20 enter neither of these locks. They do not test active
mesh-group count, pending-group count, dirty byte+0x23c or a light-ready/version
field. The captured neighborhood contains borrowed live pointers, rather than
an independently immutable voxel snapshot. This establishes the local lock
boundaries and ordering; it is not a complete audit of all outer thread/zone
lifetime coordination and does not establish a whole-zone atomic read snapshot.

## 3. Exact channel0 query

Fixed-position entry0xdab20 obtains signed32 blockXYZ and calls0xda830.
For each signed64 fixed coordinate, it divides by65536 toward zero and narrows
the quotient to signed32. For a negative original coordinate, it decrements
that narrowed result when multiplying it back by65536 does not reconstruct
the original fixed value. The decrement wraps in32 bits. This matters outside
the signed32 block range as well as at ordinary negative fractional blocks.
There is no additional16-block-cell conversion in this entry.

Reader0xda830 floors signed32 blockX/Y to64-block zone coordinates and looks
up World+0x460. A missing map entry or null Zone returns255 immediately.
For a present zone it obtains the effective six-byte record through0xeb90.
While the effective type is10,12 or19, it increases blockZ by1 and reads again.
For the final record:

| Effective type | Channel0 result |
| --- | --- |
| 0 or2 | Maximum of red byte0,5 and auxiliary light byte3 |
| Any other type | Auxiliary light byte3 |

The native type test and byte3 selection are visible at0xdaac4–0xdaaf2;
the record's byte3 is explicitly distinct from flags byte5. The result is an
integer in0..255. No time-of-day factor, renderer color or float normalization
is applied by this reader.

The effective reader's usual column rules still apply. Below the stored base
it supplies the global solid record; its auxiliary byte is0, so the light query
returns0. Above the stored end it supplies canonical air for Z>0 and canonical
water otherwise; both have red255 and therefore yield255. A stored type0 block
at Z<=0 whose flags do not contain0x02 is also replaced by canonical water
before the query examines its red. Consequently a solved red byte in such a
raw air record need not be the value observed through the effective reader.
Stored type0 with flags0x02 and stored type2 retain their stored-red behavior.
These rules come from0xeb90, not an additional light-cache fallback.

Upward retries repeat the zone lookup. If a zone becomes unavailable during
that retry loop, the native path uses the six-byte global record at0x551678
rather than the entry's immediate255 return. Its runtime initialization is
not resolved by this supplement; the ordinary stable-loaded-map query above
does not depend on that concurrent-unavailability case.

## 4. Physical edits and retained light state

The physical setters0x91160/0xa36e0 and0x31c1c0 write ordinary block records.
They do not themselves solve lighting, append history or mark a mesh dirty.
Their ownership distinctions are specified in
[block changes](steam-block-change-lifecycle.md) and
[the full setter](steam-full-block-setter.md).

A later Game notice pass0xaf110, called at0xaaf0c, requests affected loaded
zone meshes by writing dirty byte+0x23c and its+0x238 timestamp. The dirty
write is0xaf38c and the timestamp write0xaf393. This path does not clear or
replace the zone's stored red bytes. Pending/active mesh changes likewise
are not a validity predicate of0xda830.

Therefore an unmodified record retains its previous solved red until another
ordinary write or a subsequent solver copy replaces it. A physically replaced
record immediately exposes its newly supplied bytes through the effective
reader, including canonical water substitution where applicable; it does not
wait for remeshing. There is no query-side rule that darkens a dirty zone or
falls back to a separate old/new light volume.

The solve's red writeback produces no World+0xc0 block-change notices and no
Zone+0xd8 saved-history entries. Its persistence is the lifetime of the actual
loaded column data. This trace establishes no independent expiration clock or
serialization of a separate completed-light volume. Regeneration/history replay
and mesh solving remain distinct stages.

## 5. Gameplay and particle source identity

Creature upkeep0x2de4c0 calls0xdab20 at0x2e00d3 with channel0 and the
creature's physical fixed position. See
[creature upkeep §4.7](creature-upkeep.md#47-blocks-and-light).
Lit particle rendering0xe6840 calls the same entry at0xe6999 with channel0,
the particle's fixed position and the renderer's World from+8. Particle
normalization and the0.2 floor occur later in that renderer; they do not
change the stored block-light byte. See [particle rendering](steam-particle-bursts.md).

Both routes consequently read this ordinary loaded-column source through the
same accessor. They do not sample the terrain GPU vertex buffer, a model's
cached+0x28 lighting field or the pending mesh-group list. The identity is
per passed World: this does not equate the host embedded World's columns with
the separate client World's columns or establish a cross-World light transfer.

## 6. Scope and correction

This completes the bounded normal channel0 storage/publication contract.
The exact global0x551678 initialization and complete outer worker/streaming
lifetime synchronization remain untraced here. Other channel indices and all
possible unrelated physical-block writers are outside this supplement.

This corrects the older creature-upkeep §4.7 references to byte5 as the
auxiliary light source. Byte5 remains the flags byte; the queried auxiliary
light source is byte3. The earlier description of byte3 as unused in
effect-objects is correspondingly limited by this verified consumer.
