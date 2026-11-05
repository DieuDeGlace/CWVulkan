# Experimental native generation requests

This is a developer reference-capture path, separate from ordinary loaded-zone
asset export. It invokes the supported owner's native generator in a test session.
The following admission, snapshot and pause rules are CubeVulkan tool policies,
not an implementation of native generation or a universal lifetime guarantee.

Write `Extract/gen.request` with whitespace-separated signed 32-bit coordinate pairs,
one pair per line. The complete request must fit within 4096 bytes and the remaining
32-attempt process budget. Native allocation/construction failures also consume
attempts. Invalid, oversized or over-budget requests remain on disk. An active
batch does not consume or redirect a second request.

Before consuming an admitted request, the extractor publishes an exclusive local
`request_<process>_<tick>_<sequence>.txt` snapshot beside its generation output.
That snapshot contains the copied World address, unsigned primary seed bits,
signed derived generation seed, day counter and every requested coordinate in
order, including duplicates. Its version marker is `CW-GENERATION-SNAPSHOT 1` and
its final marker is `END`. It is a request record, not a completion report or an
automatically replayable `gen.request`. Snapshots and generated game data stay
local and are excluded from source/runtime packages.

The input file stays exclusively open from its bounded read through admission,
so another producer cannot replace it between reading and consumption. Consumption
marks that exact held file for deletion on close. An abandoned read leaves it intact.
The output snapshot is written to a new temporary file, flushed and renamed
without replacing existing files. Snapshot or request-consumption failure pauses
generation for the process and leaves the original request in place. Existing
completed or temporary owner files are not overwritten by snapshot publication.

Before each native generation and after the native generation/refresh phases,
the extractor compares the current copied World address, primary/derived seed
and day with the admitted context. An observed mismatch pauses the batch for
the rest of the process, even if the old values later return. The current
coordinate and remaining queue are retained, with the complete original request
also preserved in the immutable snapshot. Native generation/refresh failures
likewise pause rather than repeatedly calling into a partially changed context.
Ordinary loaded-zone export continues independently.

For a retry, inspect the log and request snapshot, start a fresh authorized test
session in the intended world/day, and copy the desired coordinate rows into a
new `gen.request`. Do not copy the snapshot header/footer into that request.
Already completed coordinates are also present in the original snapshot; select
only the rows you intend to repeat. Larger captures need multiple sessions.

Address equality is not a lifetime pin or a complete World-state identity. These
checks do not create an atomic World snapshot, roll back native effects, qualify
exception cleanup, or make all legacy output files one atomic dataset. Temporary
Zones remain retained: refresh can publish raw Zone references, and the broader
generator escape lifetime remains unproved. The 32-attempt limit is not a fixed
byte-memory bound. Supported-game startup/capture/recovery still needs live
validation for the current DLL.

`EngineTests generationrequest` checks parser bounds, sticky context changes,
full-width snapshot values, exclusive input ownership/consumption and actual
Windows file publication/no-overwrite
behavior with isolated temporary fixtures.
