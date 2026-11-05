# Independent Windows extractor build

The shipping ZoneExtractor target builds without CWSDK. Its source declares
Game, World, Creature and Zone only as opaque pointer types. Owned adapters use
the separately authored native contracts for fields, collections, loader callbacks
and original call boundaries. CMake no longer supplies an SDK include directory,
requires a sibling checkout, or uses the former SDK permissive option. The obsolete
CWSDK_DIR cache setting is retired during configuration.

After building, run `tools/validate_extractor_no_sdk.ps1`. It inspects the actual
Ninja compiler commands and valid header-dependency records for both extractor
translation units, rejecting SDK headers/paths and the former permissive option.
The records are retained under the build directory for review. This checks the
shipping target, rather than requiring an unrelated sibling SDK checkout to be
renamed or deleted. The DLL retains MakeMod, ModPreInitialize, ModMajorVersion and
ModMinorVersion exports. CubeModLoader remains a separate installation.

Stored-zone enumeration and spawn sidecars now use the owned sequence/identity
readers. Collision and drop probes read stored columns under the existing scoped
zone/buffer leases. A per-pass column cache avoids copying a column again for
each queried height. It retains first-read bytes for that pass and refreshes on
the next pass; it is not an atomic whole-World snapshot. Its byte budget uses the
existing capture limit, without asserting a native capacity.

The diagnostic solid-block predicate retains its existing stored-type policy.
Missing zones and unreadable columns are unavailable data, rather than known air.
Incomplete push/drop probes defer changes, and collision logging preserves its
retry position. The deep-water scan gathers type ranges in the same pass and
uses overflow-safe differences before squaring diagnostic distances. None of
these helpers replaces native physics or changes engine terrain generation.

The independent [stat-call audit](specs/steam-native-stat-query-abi.md) also
authenticates the remaining original stat entry points: float returns in XMM0,
byte selection arguments, and the native UI's supported null-secondary critical
query. Some queries allocate temporary native equipment lists; caller-held
protection and complete lifetime remain relevant. Their ABI/label evidence does
not provide a universal pure-query or no-throw guarantee.

Sparse fixtures check negative coordinate mapping, stored-height boundaries,
extreme probe coordinates, ownership failures, unloaded data, cache reuse and
refresh. All five targets and the CPU/owner-data suite qualify the source changes.
Supported-game startup, loaded-world extraction/recovery and broader parity checks
remain open. Temporary unpublished-Zone disposal is still unproved; that oracle
retention is not described as solved by removing the SDK dependency.

Legacy RegionProbe/RegionScout sources remain optional and outside the shipping
CMake target and runtime package. Their SDK setup and the historical license audit
are separate from this build. No additional SDK rights, legal clearance or final
1:1 gameplay acceptance follows from successful compilation.
