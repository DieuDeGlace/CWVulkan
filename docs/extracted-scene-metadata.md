# Extracted scene metadata

## Static-object reward maps

`CWR1` may contain the project-owned chunk `0xffff0002`, with stride164 and
count equal to the number of reward-map entries. Each record contains a
little-endian signed32 physical object index, followed by the full160-byte item.
Records are in strictly increasing signed-key order. A supplied zero-count chunk
is a known-empty map. A missing, malformed or duplicate chunk is unknown metadata;
it is never evidence that a chest has no reward. Category0 entries remain entries.

These values are separate from the required item/count embedded in the224-byte
object record. The engine retains the immutable reward map with its loaded source
zone and shares it across block/light snapshots. Interaction reads do no file I/O.
Both client receipt and authority lookup consume these full item values; they do
not rebuild them from a category, rarity or object type. Older files need a fresh
owner capture to supply this map. Generated zones retain their generated map.

The extractor uses the independently specified
[native reward-map layout](specs/steam-static-object-reward-map-capture.md). It reads
memory without invoking native tree methods, checks traversal/count/parent/nil/key
consistency and repeats the observed wrapper/node checks. An unavailable or visibly
changing map omits the chunk. Those checks do not establish an atomic snapshot or
prove a particular lock freezes every map writer; valid quiescent input remains
the capture contract. Sentinel payload bytes are never exported as an item.

`CWR1` continues to store its existing record lists. New owner captures may also
contain a project-defined record tagged `0xffff0001`, with stride112 and count1.
Older readers skip this unknown tag; older files without it remain supported.
This record is evidence from a loaded native mesh cache, not a generation field.

All numeric fields are little endian. The payload is112 bytes:

| Offset | Type | Meaning |
| --- | --- | --- |
|0|u32|Format version1|
|4|u32|Capture phase1: native world mesh-cache critical section held|
|8|u64|Active mesh-group count|
|16|u64|Pending mesh-group count|
|24|i64×3|Native fixed-coordinate minimum XYZ|
|48|i64×3|Native fixed-coordinate maximum XYZ|
|72|i64×3|Native fixed-coordinate center XYZ|
|96|float32|Native cached radius in blocks|
|100|u8|Native mesh-dirty byte|
|101|3 bytes|Reserved, currently zero|
|104|i32|Native weighted triangle cost (water triangle count has double weight)|
|108|4 bytes|Reserved, currently zero|

The exporter reads these fields only while its live-zone snapshot holds both
the native zone-map/lifetime lock and the mesh-cache lock at world+0x3c8.
It omits the record when both group counts are zero: that state cannot
distinguish an unwritten constructor cache from a completed empty mesh.
Fresh-generation oracle exports do not assert this locked capture phase.

Active and pending counts are different. New bounds can briefly accompany an
older active mesh list; neither the dirty byte nor zero counts are an invented
initialization flag. Keep this phase information when comparing computed bounds.
The independent native field and publication facts are in
[steam-scene-lighting.md](specs/steam-scene-lighting.md).

The engine rejects unknown versions/phases, truncated or duplicate records,
inverted bounds, nonfinite/negative radii and horizontal bounds belonging to a
different zone. Missing metadata is an unavailable oracle, not proof of a match.
It does not change stored blocks, assets, user saves or the original game code.

The runtime computes its own mesh-prepass bounds from the exact immutable3×3
terrain snapshot used by its mesh job. Only XY receive the render-origin shift;
native CPU culling keeps game Z. Geometry readiness is set after actual mesh
publication, not inferred from a planned band count. Missing neighbors leave the
native box unavailable and retain the previous conservative rendering fallback.
The captured record is a comparison oracle, not authority over later block edits.
Current camera/distance settings remain configurable; native adaptive range and
camera-state integration are separate work. No fresh owner cached-box comparison
is claimed by the synthetic codec/column/plane regression tests.

## Cached column material inputs

The optional `z_X_Y.cwm` sidecar accompanies a matching `CWZ1` block file.
It contains owner data needed by the full block setter's downward interior fill.
All values are little endian; the exact file length is 98,324 bytes:

| Offset | Type | Meaning |
| --- | --- | --- |
|0|4 bytes|`CWM1`|
|4|i32|Absolute zone X|
|8|i32|Absolute zone Y|
|12|i32|World sand-noise X offset|
|16|i32|World sand-noise Y offset|
|20|4096 records, 24 bytes each|Column inputs in local-X outer/local-Y inner order|

Each column record contains six float32 values in order: highlands, warmth,
dryness, lava-region weight, forest-region weight and kind27 weight. These are
cached column values, not sampled colors or regenerated geometry. The independent
field and setter facts are in [steam-full-block-setter.md](specs/steam-full-block-setter.md).
Coordinates, exact length and finite inputs are validated before use. Missing
sidecars remain compatible with block-only exports; downward fills requiring an
interior gap cannot be faithfully supplied without their cached inputs. Generated
worlds retain their own original material inputs and do not require sidecars.
The file stays local to the owner and is excluded from source/runtime packages.
