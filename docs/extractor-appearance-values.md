# Captured appearance values

The appearance accessor follows the separately authored
[native record contract](specs/steam-creature-appearance-record.md). It reads
values from an existing Creature; it does not rebuild models or copy a native
look-rebuild algorithm. Callers must retain actor lifetime and applicable guards.
Individual reads do not establish an atomic snapshot or model-resource ownership.

Three ordered tint bytes are captured at Creature+0x7a. The following bytes have
no established color meaning. The legacy snapshot's fourth color token is now
the format's opaque value 255; native padding is not read as alpha. The engine
reader consumes the first three tokens, as before.

The three physical extent inputs at +0x84 are distinct from eight model-transform
scalars at +0xa0. Neither universal full-size/half-size units nor a uniform render
scale convention follows from those extents. The snapshot adds an explicit
`spatial_extent_inputs` line. Its older `scales graphics/hitbox/physics` line and
the creature list's `scale` field retain their existing values for reader
compatibility. These legacy labels do not establish native semantics.

Eight model IDs remain signed 16-bit values, including absent -1 and other
negative values. The accessor also preserves seven pose-control floats and six
placement triples without imposing universal anatomical names or Euler angles.
The scalar at +0xc0 is exposed separately with its meaning unresolved. Existing
snapshot labels for model slots, scalars, controls and triples are compatibility
labels, not a newly verified native taxonomy.

Character snapshots, creature descriptions, death diagnostics and collision-log
headers now read these values without SDK appearance or FloatVector3 overlays.
Documented primitive field consumers also use owned scalar views. Diagnostic
coordinate differences subtract unsigned magnitudes before conversion to double,
avoiding signed overflow while retaining nearby differences at signed64 extrema.
This arithmetic is project diagnostic policy, not a new native gameplay claim.

Synthetic byte fixtures check an unaligned record, tint versus poisoned padding,
high flag bits, signed model IDs, constructor scale bit patterns, distinct extent
inputs, negative zero, all placement triples and unchanged neighboring state.
Scalar fixtures check pitch/bank widths and signed64 distance boundaries. The
five build targets and CPU/owner-data validation are required separately. These
checks do not replace supported-game validation or complete SDK independence.
