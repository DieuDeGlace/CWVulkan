# Camera, World values and region-role cleanup

The camera accessor uses the field table in
[Steam gameplay camera section 1](specs/steam-gameplay-camera.md#1-state-and-units).
Displayed and target angles are separate double-precision triples; current and
requested distances are separate float values. Capture helpers preserve their
existing target-before-display writes. They do not implement ordinary camera
input, smoothing, clamps or angle wrapping. SDK camera-member accesses are removed.

The [World and coordinate contract](specs/steam-world-metadata-and-coordinate-abi.md)
separates the primary 32-bit seed at +0x220 from the derived generation value at
+0x244 and the unrelated pointer at +0x70. It also distinguishes signed32 day
from float time. The accessor preserves unsigned seed bits and exposes a signed32
label for the engine's path/cache convention. New capture paths use that explicit
label; these helpers do not rename existing directories or invoke seed setup.

The recording-only noon helper writes the host and client in separate observed
guard scopes. It reports an incomplete update when either scope is unavailable.
It does not hold both Worlds at once or promise a cross-world transaction. The
primitive setter changes only the time float, leaving the day and simulation
clock alone. Owner lifetime and complete writer protection remain caller concerns.

The eight-byte CoordinatePair replaces the SDK IntVector2 at native region-query,
Zone-constructor and region-role-constructor boundaries. Its two signed32 values
retain their order and are passed through an explicit pointer. No by-value return
or eight-byte component convention is substituted for the original call ABI.

Region-role construction previously retained its native map/vector allocations.
The [cleanup audit](specs/steam-temporary-zone-and-role-lifetimes.md) establishes
destination-only disposal at RVA 0x82450. A successful result now retains its
caller-owned storage through consumption, invokes that original cleanup, then
releases the caller buffer. Consumer C++ exceptions follow the same scoped path.
If native construction fails, no valid empty result or partial cleanup is assumed:
its partial storage is retained and further role capture is disabled for the
process. This bounds repeated attempts; it does not establish recovery from an
arbitrary native fault. An original-game restart begins a new process.

Generated unpublished Zones remain a separate unresolved lifetime question.
The original Zone destructor exists, but the bounded audit did not prove absence
of references retained by every generation descendant. Their temporary cleanup
has not been enabled on that evidence alone. The experimental generation oracle
still retains those Zones, so this is open capture work, not a complete
lifetime guarantee or final-release qualification.

Fixtures check unaligned reads, double versus float precision, separate targets,
signed zero, neighboring map-drag bytes, seed bit preservation, signed coordinate
order, time-only writes and normal/exceptional role-result disposal. Build and
CPU checks complement those fixtures; supported-game execution and remaining
World/Zone view replacement are separate requirements.
