# Installed Steam loader: factory-object and module lifetime

Independent native-only supplement to
[the observed loader ABI](steam-mod-loader-capture-abi.md). Only the installed
original loader binary and independent prose were read. No SDK, game engine,
exporter, test, reconstruction, owner output or game execution was used. All
addresses below are **loader RVAs**, not game addresses. This describes observed
machine operations, not a C++ class declaration or an implemented adapter.

## 1. Exact scope and distinct allocations

The inspected CubeModLoader.fip is the same 145408-byte AMD64 binary scoped by
the ABI note, SHA-256
`3b0c333bf733d158211c22918fe2dfa9605bae8eb7fe4f7363bcb6b27e59c71c`.
Its own version exports report 7.3. No compatibility with another binary is
inferred from that version pair alone.

Three things must remain distinct: a loader-allocated module record; the DLL
handle stored at **record+0x28**; and the factory-returned pointer stored at
**record+0x50**. The latter two fields are eight bytes wide. The record's own
cleanup table is not the pointed-to plugin object's table. In particular, the
plugin's reached **slot+0x00 is initialization**, as proved in
[ABI §3](steam-mod-loader-capture-abi.md#3-returned-object-vptr-priorities-and-lifetime);
it must not be reclassified as a destructor by a C++ naming convention.

## 2. Retention, exclusion and pointer-array replacement

The active DLL record-pointer vector has its three eight-byte fields at loader
**0x237f8 / 0x23800 / 0x23808**: beginning, live end and allocation end. Startup
**0x102b0**, including **0x102bc–0x102c3**, initializes these fields to zero.
DLL discovery **0xe090** creates records, loads each path through **0x1380** and
appends record pointers to this vector. The successful LoadLibraryA result is
stored at record+0x28 by **0x13a0**. This operation has no factory-object input.

After export/version validation and configuration processing, **0xe4e5–0xe4ee**
copy the record-pointer sequence into a separately allocated array using
**0x10280**. That helper copies pointer-array bytes; it does not clone records or
factory objects. The copied, complete discovered-record vector is installed at
**0x23810 / 0x23818 / 0x23820** by **0xe548–0xe556**. If its previous array exists,
**0xe4f3–0xe53d**, including the release at **0xe52e**, release only that previous
array allocation and clear its vector fields. There is no per-record traversal
or factory-object release in this replacement.

The active vector is then filtered by **record byte+0x58** at
**0xe595–0xe5cb**. A zero byte removes that pointer from the active sequence by
shifting the later pointer-array entries and shortening the end by eight bytes.
This branch does not invoke record cleanup, free record+0x50 or release the DLL
handle. The complete discovered-record copy was made before filtering, so the
excluded record remains represented there. This is **pre-factory inclusion
filtering**, not a proved runtime object-unregister operation.

For each remaining record, **0xe5e3 / 0xe5e6** invoke pre-initialization and the
factory; **0xe5e9** stores the factory result at +0x50. Initialization then
dereferences that same pointer at **0xe613–0xe61a**. Later event dispatchers use
the active vector, the retained +0x50 pointer and the pointed-to priority bytes.
Their complete reached slot/routing census is in ABI §§4–6. Those loops do not
remove an object as a consequence of a callback return; the specified nonzero
returns stop the relevant event dispatch, rather than becoming a deletion flag.

The factory store itself has no prior-object release: if this pass were reached
again with an already populated +0x50, the store would replace its bits without
first disposing of the old pointer. This conditional observation does not prove
ordinary discovery reentry, a supported reload protocol or a once-only promise.

## 3. Actual cleanup entries and their bounds

**0x1760** handles the active vector. With a nonnull beginning it releases the
pointer-array allocation through **0x18dd0** at **0x179b**, then zeros the three
vector fields at **0x17a3–0x17aa**. A null beginning takes the immediate return
path. It never visits the pointed-to records or reads their +0x50 fields.

**0x17d0** performs the equivalent operation for the complete discovered-record
array at 0x23810: the array release is at **0x180b** and the vector-field clearing
at **0x1813–0x181a**. Again, neither record allocations, factory objects nor DLL
handles are released. The release chain **0x18dd0 → 0x18dc8 → 0x1b010** reaches
the imported CRT free operation; the pointer supplied by these two cleanup
entries is the array allocation, not one of its elements.

A different callable cleanup belongs to the **module record itself**. Constructor
**0x1120** installs the table at **0x1c030**, whose first entry is **0x14d0**.
That entry calls **0x1460**, which releases any heap-backed filename storage and
resets the record's filename state. With a nonzero second argument, **0x14d0**
also releases the record allocation at **0x14e7**. Neither helper reads or frees
record+0x50, invokes its plugin table, releases record+0x28 or clears either of
those two fields. The existence of this record-cleanup entry therefore supplies
no plugin-object destructor contract.

The fixed-reference census found 0x14d0 in that record table and the direct
0x1460 call inside 0x14d0. Its availability through a table is not evidence that
the normal active-vector filter or the two array cleanups invoke it: they do not.
Arbitrary indirect invocation of the record-cleanup entry was not exhaustively
closed.

## 4. Reached loader UI continuation and restart request

Tick dispatcher's first-use setup calls **0x2190 at 0x235d**. That setup constructs
a loader UI controller through **0x103f0 at 0x2268**, passing the address of the
complete discovered-record vector. **0x1045a** retains that vector address at
UI-controller+0x208. Thus this continuation retains access to records excluded
from the current active dispatch vector as well as included records; it is not
another factory-object array.

The selection branch **0x10738–0x1077f** checks a nonnegative index against that
complete vector's current length. For a valid index, **0x10768** toggles the low
bit of the selected record's byte+0x58, **0x10773** calls helper 0x107a0 with the
complete vector, and **0x10778** sets UI-controller+0x1fc to 1. This controller
byte was initialized to zero at **0x1046b**. The reached branch does not change
record+0x50 or remove/reinsert the record in the active event vector. Ordinary
event priority passes do not retest record+0x58, so this byte toggle alone is not
an immediate dispatch-unregister or factory-creation protocol. The complete
configuration-file format and UI input routing are outside this note.

The close branch at **0x106cd** first hides its UI node through **0x17210**. It
then tests the changed byte at **0x106db**. If nonzero, **0x106fc** invokes the
imported **_execvp**, naming the literal **cubeworld.exe** and supplying an
argument-pointer array containing that same literal followed by null. No plugin
cleanup, module-record cleanup or DLL unload precedes that call in this branch.
The native request is executable re-execution, not an observed in-process module
reload. If the call returns, its result is not inspected before the handler
returns; successful process replacement is not guaranteed by the presence of
the call.

## 5. DLL/module teardown, failure and remaining limits

The native import directory has **no FreeLibrary or FreeLibraryAndExitThread
entry**, and the delay-import directory is empty. The bounded GetProcAddress
census found the four required plugin-export names in discovery and the embedded
runtime's SleepConditionVariableCS / WakeAllConditionVariable lookups, not a
resolved module-unload operation. This is an import/reached-resolver result,
not proof against all computed calls, code supplied by plugins or operations
performed by another module.

Loader DllMain **0xf7e0** admits its attach work only for reason 1 at
**0xf7fa–0xf7fd**. Its directly reached non-attach path returns success without
per-plugin or per-record cleanup. Attach itself also calls LoadLibraryA on the
literal **CubeModLoader.fip at 0xf8d5**, retaining the result at **0x23840** by
**0xf8db**. That separate filename-based lookup is not record+0x28 or +0x50.
The exact resolved module identity/reference-count consequences of this lookup
were not closed; it must not be silently converted into a guaranteed loader
pinning or unload policy.

There is nevertheless ordinary **CRT termination processing outside that
DllMain body**. Startup 0x102b0 passes cleanup addresses **0x1760, 0x17d0 and
0x1840**, in that order, to the registration wrapper **0x191c0** at
**0x102d5 / 0x102f3 / 0x10311**. The third entry handles another array
container; it is not a traversal of the factory objects. The registration
wrapper reaches either the CRT atexit registration or the loader's on-exit table
registration through **0x191d8**. This registration does not itself dispose of
anything, and its result is not checked by 0x102b0.

On an admitted process-detach route, the entry wrapper **0x1a3b0**, through
**0x1a491 → 0x1a33c → 0x1a2b8**, reaches CRT termination processing. Its successful
initialized-state path calls **0x194e8 at 0x1a2f8**; that helper reaches on-exit
table execution through **0x1af90** or the CRT _cexit branch. This establishes a
registered-cleanup route, not a new loop disposing of every retained factory
result. Forced termination, failed registration and complete exception/unwind
coverage are not given a cleanup guarantee here. Hook removal and callback
quiescence before external DLL unload were not established.

The audited immediate failure paths are narrower:

* Module-load failure in **0x1380** stores the failed LoadLibraryA result, reports
  the error and calls CRT **exit(1) at 0x13d7**. It has no explicit record or
  plugin release before that exit.
* Missing required exports or unsupported versions reach the fatal discovery
  paths ending in **exit(1) at 0xe9d0**. Fresh-pass export/version checking
  precedes factory creation; these are not graceful skip/unregister operations.
  No per-module release is performed before the fatal call on those paths.
* A null factory result is stored without a fallback. The later initialization
  pass dereferences it; no null-result rejection/disposal protocol is supplied.
  Recovery from a factory, initializer or event callback that throws, faults or
  disposes of its own storage remains outside the established contract.

Within the audited discovery, exclusion, factory, initialization, event, UI and
array-cleanup routes, **no loader-owned release of record+0x50 is established**.
Nor is a dedicated unregister export present among this binary's four named
exports. This bounded result does not promise eternal allocation or prove all
external/reentrant unload paths absent. It establishes no SDK destructor, full
object size, allocator match, ownership transfer or permission to invalidate
object/table/module storage while the retained callbacks can still use it.
