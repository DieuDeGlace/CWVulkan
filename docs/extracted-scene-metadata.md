# Extracted scene metadata

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
