# Native text result ownership

The extractor uses the separately authored
[generated-text](specs/steam-native-generated-text.md) and
[item-key](specs/steam-item-text-key-lookup.md) contracts for the supported image.
It calls the owner's original generators; it does not bundle their vocabulary,
resource tables, or formatting implementation.

Each call constructs fresh 32-byte destination storage. After normal return,
scoped ownership calls the original wide-string object cleanup, RVA 0x15c9a0,
including on decoder/converter failures and consumer C++ exception unwinding.
This replaces direct buffer deletion, which did not account for the native
aligned-allocation path. Cleanup before a failing generator returns remains
unproved; no valid empty destination or successful rollback is assumed there.

The bounded decoder copies explicit UTF-16 unit counts, with capacity below 8
selecting inline storage and capacity at least 8 selecting the heap pointer.
It verifies the terminator and rechecks the object record. Empty text is valid;
malformed, unreadable, changed or over-budget data publishes no partial value.
The capture budget is 1,048,576 units per result, not a native string maximum.
Metadata rechecks are not allocation pins or atomic character snapshots.

The decoder preserves embedded zero and surrogate units. The existing line
export then requires representable UTF-8 without embedded zero, rejecting bad
input instead of silently truncating or substituting text. This is an export
policy, not a native Unicode guarantee. It does not normalize valid text.

Item names use the initialized inline resource owner at Game+0x50. The native
item-key helper replaces the SDK table walk and returns its own copied string.
It receives the raw signed numeric pair, distinct from the full item-name
formatter's normalized item copy. Both native empty-key fallback and ordinary
item-name fallback are preserved; a completed string is not proof that a
resource lookup found an entry.

For each conversation record, name and key enrichment completes before output
begins. Owned extra text is bounded to 8 MiB per record. Generation, use and
native disposal occur within a scoped client World+0x3c8 interval, mirroring
observed native callers. Only owned strings reach file formatting after release.
This does not prove protection of every text-resource writer or owner shutdown;
the active callback must retain Game and World lifetime. Disk write failures can
still leave partial output, and completed people are marked only after flush.

The name helper can perform runtime static initialization and acquire runtime
locks. Its inspected body has no direct rand/srand calls, and the key lookup
does not select random alternatives. These findings do not erase the separate
conversation generator's documented runtime/RNG effects or establish a complete
transitive synchronization guarantee.

Fixtures cover scope cleanup and consumer exceptions, the failed-construction
limit, inline/heap boundaries, embedded zero, surrogate pairs, allocation capacity
versus text length, terminator and metadata failures, budgets and overflow. UTF-8
checks cover exact BMP/supplementary output and rejected malformed input. They
do not execute the original allocator or qualify a live supported-game session.
Other extractor SDK consumers and final-release acceptance remain open.
