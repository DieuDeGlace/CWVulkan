# Native allocation boundaries and diagnostic cleanup

The independent [allocation audit](specs/steam-native-object-allocation-spans.md)
confirms the ordinary supported-image allocations: Creature 0x15f8, Game 0x2288,
standalone World 0x548 and Renderer 0xa48 bytes. The Creature size already used
by the trace format was correct. Capture code now shares that independently
verified storage constant instead of repeating the literal.

These bounds describe live object storage, not a complete initialized-byte map
or a serialization/clone format. Creature's inline pose state ends before the
separate-owner pointer at +0x1528; its lists and optional 0x300-byte child have
their own lifetimes. Copying the containing bytes does not transfer or duplicate
ownership. No numeric allocator-alignment guarantee was established by the
bounded audit, and no such assertion is added to the reader.

The old death probe walked unnamed SDK collections and scanned pointer-shaped
data to guess further objects. Those heuristic container counters, pointer walks
and deferred arbitrary-entry dumps are removed. Their layouts and meanings were
not independently established. The cleanup also removes the SDK Renderer member
read and the unused zone-lookup/vector-size helpers from this diagnostic path.

Documented native event queues, actor scalar/pose records, new-actor observations
and body-particle sampling remain. Particle-node reads now occur within the
existing observed client actor-guard interval. The guard still depends on live
owners and cooperating native writers; it does not provide universal quiescence.
This cleanup changes extraction diagnostics, without changing engine gameplay,
generated terrain, stored game assets or Steam ownership checks.

All build targets and the CPU/owner-data suite verify the source cleanup. They
do not establish live extraction parity or complete SDK independence. Remaining
Game camera, World/Zone and coordinate views still require replacement before
the extractor can build without the external SDK.
