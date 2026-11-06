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
A failed write of the day file or any fresh or refreshed zone artifact also
pauses the batch. The coordinate stays pending, and the completion count advances
only after both outputs close successfully. Treat files left by a failed attempt
as incomplete. Ordinary loaded-zone export continues independently.

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

For dungeon reference work, `EngineTests castles` reports engine-predicted
castle cells in the selected zone directory. Set `CW_ZONES_DIR` to choose a
captured `seed_<n>` or `gen_seed_<n>` directory. Add `--targets` to search a
bounded area around the origin for structure 42/detail 11 and structure
44/detail 12. This produces candidate coordinates without starting the
original game or writing a request. See the [dungeon
oracle](specs/steam-dungeon-oracle.md) for the native decision and block evidence
needed to confirm the labels.

Use `EngineTests castles --structure <40..47>` to find a candidate interior
for one dungeon structure. `EngineTests castles --floor-targets` uses the
zone coordinates in the selected capture directory to find candidate edge,
corner and room-increment cases. It prints a request that fits the extractor's
per-process zone limit and labels additional picks for a later process. These
commands only predict targets. Compare a fresh Steam batch before treating
the floor or the structure as verified.

`EngineTests castles --guard-targets` selects candidates for a kind-8 gate
guard and the fortress floor cases 0, 2, 3 and 4. Floor 0 means no qualifying
floor cell was found. The candidate search checks the cell's floor code.
Actual guard spawns also need a suitable spot, which this search does not
verify. Seed 123 has candidates for a gate and floors 2, 3 and 4 in the
bounded search. Floor 0 has no candidate in that search.

Fresh ordinary generation exports can include an optional `0xe8` chunk in
`.rec`. It contains the signed X/Y/Z block keys tracked by Steam's object
footprint pass. CubeVulkan uses these keys when a later object rebuild
clears footprint flags. Legacy captures without the chunk retain the
stored flags because their ownership is unknown. The fresh exporter reads
the set at the synchronous generator boundary. Routine live and refreshed
zone exports omit the chunk because their mutation exclusion is unproved.
See the [restoration and occupancy contract](specs/steam-zone-restored-object-occupancy.md).

## Labeled dungeon captures

`CubeVulkanDataCheck.exe --setup-capture --dungeon-targets <targets-file> --game "<Cube World folder>"`
prepares an isolated dungeon session using the same Save, Mods and ReShade
journal as the normal setup. Add `--dry-run` to inspect the preparation
without changing the installation. The setup refuses to replace an existing
generation request and queues only the dungeon request for this mode. Load a
test character in Steam Cube World, wait for completion, close the game, and
run `--setup-restore` against the same game folder.

`Extract/gen-dungeon.request` opts into the dungeon diagnostic. Supply up to
16 signed **16-block cell** coordinate pairs, one pair per line. The extractor
includes each target's 3×3 neighborhood and deduplicates cells and 64-block zones.
The process-wide retained-Zone attempt limit still applies. This mode also caps
expanded stored block data at 256 MiB per batch. The request stays in place when
admission fails. Ordinary `gen.request` takes precedence when both exist.

For the two seed-123 candidates, the target lines are `194 715` and `-862 154`.
These coordinates come from the engine's prediction and require native
confirmation. The batch writes to a new `dungeon_<process>_<tick>_<sequence>`
directory beneath the session's reference root. It retains the original target
list, the resolved zone request and the capture day. Each zone contains a
freshly generated `.cwz` file and a `.cwd` label file. The exporter copies
stored blocks before querying the helpers and skips the daily refresh in this
mode. In kind-8 zones, batch versions 2 and later also write a `.cwg` file with
the ordered guard and activation coordinates for each spawn ID, including
empty trees. The extractor reads these records from the retained fresh
zone while generation is quiescent and exports only scalar keys. A
malformed or oversized tree fails the batch. Version 3 and later also read the
bounded spawn inventory and records each dungeon key stack's quantity,
ID, region, modifier, realm, rarity and material. Version 4 adds a
`.cgt` file in every zone for its gate threshold map. The file preserves
explicit zero entries and halo keys owned by that generating zone.
`complete.txt` appears only after the whole batch succeeds.

The label file records the loaded PE timestamp and image size, public seed
bits, derived seed, day, sand parameters, the mod-file classification at
admission and selected coordinates. It keeps raw and rounded place coordinates,
big flag, place and structure kind, building type, all four castle bytes, raw
top and final floor. These are helper-query results, with stage tags distinct
from the fresh stored block payload. The format does not claim instruction-site
room, floor-branch or decorative observations. The vanilla-candidate label
comes from the mod-file inventory and does not attest active modules.

Run `EngineTests wgdungeon <batch-directory> <owner-Extract/oracle-directory>`.
It requires a complete batch and consistent world context, then compares the
helper labels and exact stored block sequences, including nonempty column bases
and lengths. The target-floor summary derives edge order and smoothing from
the captured top values, then reports the difference from the queried floor.
At a corner it also reports the alternate edge's smoothed top. The summary
does not record internal room flags, floor indices or dome decisions. Set
`CW_DUNGEON_DIFF=1` to print every differing stored block
in columns with the same base. It reports modified or unclassified captures as diagnostic results
with a failing exit code. Versions 2 and later require matching kind-8 spawn
IDs and both coordinate sets. Version 3 adds dungeon key stacks. Version 4
also compares every gate threshold and reports missing, extra, and changed
entries. Earlier captures still compare their original fields. No captured
files belong in the source or release.

### Direct castle ceiling checks

Put `CW-DUNGEON-DOME 1` on the first line of the dungeon target file before
the coordinate pairs. The normal setup command accepts it. This opts into
batch version 5 and one thread-local execution breakpoint at Steam's
verified final-ceiling continuation. Each `.cdo` records the signed layer,
local column, dome flag and final integer ceiling for explicitly targeted
cells in their owning zones. Earlier or later block writes remain separate
from this observation. The comparator reports dome and control columns and
rejects an empty comparison. Plain coordinate files use version 4.

### Live gate transitions

Place `Extract/gate-live.request` containing one line
`CW-DUNGEON-GATE-LIVE 1` before starting an isolated Steam session. Only
this request enables the setter observer. Enter a dungeon and play through
its gates. The recorder copies up to 64 actual nonzero-state calls from
the two verified routes, including the resulting state and timer after
Steam's setter returns. Close the game before reviewing the file.

Run `EngineTests wggatelive <gate-live-file>` to replay the recorded state
and timer through the engine setter. It reports the coverage of the count
and proximity routes separately. This observes the route Steam selected.
The engine's eligibility predicates have their own runtime tests.

Use `EngineTests castles --guard-targets <block-x> <block-y>` to look for
a count-route entrance near a test character's current world position.
The search checks that the engine generates a type-8 door and a positive
threshold at the selected cell. It prints the approach coordinates and
distance. Steam gameplay is needed to confirm the transition.

`EngineTests generationrequest` checks parser bounds, sticky context changes,
full-width snapshot values, exclusive input ownership/consumption and actual
Windows file publication/no-overwrite
behavior with isolated temporary fixtures.
