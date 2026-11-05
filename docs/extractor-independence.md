# Extractor independence workboard

The shipping Windows extractor now builds without CWSDK. Its native data and
loader boundaries use original project adapters and separately authored contracts.
The compiler-command and header-dependency audit checks the actual target for SDK
paths, headers and its former permissive option. Run
`tools/validate_extractor_no_sdk.ps1` after building. The optional
`CUBEVULKAN_EXTRACTOR_ONLY=ON` CMake configuration builds this DLL before any
renderer/dependency fetching is configured. Audit it with
`tools/validate_extractor_no_sdk.ps1 -BuildDirectory build-extractor`.

The published source snapshot also passed a fresh-directory extractor build with
cached pinned dependency sources; all 38 project headers were confined to that
frozen tree, with no CWSDK cache entry. A separate fresh extractor-only build
passed without FetchContent settings, dependency directories or renderer setup.
Required DLL exports and the actual compiler/header audit passed in both builds.
This is source/build qualification, not supported-game live acceptance.

The [historical SDK audit](sdk-license-provenance.md) still establishes no grant
for the old snapshot; optional legacy RegionProbe/RegionScout sources are not part
of this target. This build result is not legal clearance or live 1:1 acceptance.
Native-only authors continue to use [the standalone policy](CLEANROOM_AUTHOR.md),
without reading implementation, SDK, probes or other project prose.

## Completed adapters

- Loader callback record and forwarded-context validation.
- Primary sprite catalog and ordinary vertex-buffer metadata. Calibration context
  discovery, calibration naming and animation/snapshot slot maps now use guarded
  primary reads. An unavailable map defers recording and preserves previous
  snapshots/calibration rather than publishing unknown slot IDs. Other catalog
  owner/primary lookup in the calibration tour no longer uses SDK members.
- Three indexed side-resource vectors use the independent
  [catalog side-resource contract](specs/steam-catalog-indexed-side-resources.md).
  Group export and tour lookup preserve index order, nulls and aliases, read live
  elements rather than capacity, and reject bounded malformed/unreadable/changed
  metadata. The obsolete procedural-cache type-map walk is removed. The legacy
  group-file labels remain compatible with engine readers. Ordinary native
  mappings/consumers/cleanup are documented; complete writer coverage and a
  concurrent lifetime/protection protocol remain unproved. Rechecks are not pins.
- Ordinary catalog/backend/immediate-context lookup uses independently specified
  guarded fields and backend table identity, with changed-link/context rechecks.
  The main context global is guarded too. SDK graphics members are no longer used;
  no AddRef, COM call or readiness guarantee is implied by the borrowed metadata.
- Loaded-zone enumeration, active terrain buffers and their scoped lock pair.
- Stored Zone terrain and the original CWZ1 codec.
- Proper-name fragment data reads from a verified RVA/length manifest.
- Owned 160-byte item records and eleven-slot equipment capture. Scalar widths,
  attributes, count and padding remain byte-preserving capture data. Native
  semantic item assignment remains a distinct operation. Naming/class-rule
  probes, equipment diagnostics and recording helpers use the owned record.
- Independent creature scalar views for ID, fixed position, velocity, level,
  class/specialization, health, mana and the full sixteen-byte name. Recorder home,
  sweep/tour, specialization, mana and level helpers use these views. Synthetic
  records check signed extrema, negative zero, untouched bytes and the separate
  block-power/mana fields. Block-to-fixed conversion uses defined unsigned scaling
  for negative coordinates. Recorded death/collision positions also use owned
  fixed values, and zero-velocity helpers use the same scalar writer. The
  `LongVector3` alias is removed. Additional diagnostics now use documented
  ID, hostility, name, position, level and health fields; diagnostic coordinate
  differences avoid signed overflow and retain nearby signed64-edge deltas.
- Appearance reads now use the independent
  [record contract](specs/steam-creature-appearance-record.md), retaining distinct
  physical extents, eight signed model IDs, eight model scalars, controls and
  placement inputs. No native padding is treated as color alpha. Legacy text
  labels remain compatible; uncertain native meanings stay explicit. SDK
  appearance and FloatVector3 overlays are removed. See
  [captured appearance values](extractor-appearance-values.md).
- Independent animation-state/sprite/matrix reads replace SDK animation members
  and the `Matrix4` alias. Capture reads twelve sprite pointers and eighteen part
  matrices; the older fourteen/thirty-three CWA2 wire shape retains explicit
  absent/zero padding instead of copying adjacent native state. Raw float-clock
  bits and frame metadata survive. See [recording compatibility](extractor-animation-recordings.md).
- Ordered inventory tabs/164-byte slots now use owned raw values, retaining
  empties, signed quantities, item bytes and padding. Gear surveys use bounded
  captured slots and defer/skip unavailable/changed inputs. Loot diagnostics
  preserve their valid baseline on failed capture and read signed gold directly.
- Conversation generation uses the independently observed client actor scope,
  original result disposal, bounded owned token decoding and temporary-day RAII.
  Formatting uses copied data after release; completed exports are marked only
  after output succeeds. Native runtime/RNG side effects are explicit. See
  [owned buffer capture](extractor-owned-buffers.md) for limits and checks.
- Native proper/item/key text uses scoped owning wide-string results and the
  original object cleanup, including native aligned-allocation handling. Bounded
  UTF16 decoding and strict line UTF8 conversion precede output; native key lookup
  replaces the SDK resource-table walk. Enrichment mirrors an observed client
  actor-guard interval, with file writes after release. GameWString and SDK Speech
  accesses are removed. See [text ownership](extractor-generated-text.md);
  complete resource-writer protection and failing-construction cleanup are not
  established.
- Simple native event queues and Creature effect lists now use bounded owned
  copies under their independently observed client/host guard scopes. Game
  received queues remain distinct from produced World queues. Explicit field
  decoders retain raw gaps and unsigned16 pickup quantity. ServerUpdates,
  entity_data and buffs SDK usages are removed; unresolved scalar labels and
  constructor padding are no longer interpreted as known diagnostic fields.
  Axis-only velocity writes and bounded townsperson names retain other bytes.
  See [event/effect capture](extractor-event-and-effect-capture.md).
- Ordinary object allocation spans now have a separate original-only contract.
  Creature trace readers share the verified0x15f8 storage constant. Unverified
  SDK-container counters, pointer-shape scans and arbitrary-entry walks are
  removed from death diagnostics; documented native captures remain. The SDK
  Renderer-member read is removed. See [allocation boundaries](extractor-allocation-boundaries.md).
- Native Game camera and World scalar views replace SDK camera/seed/time
  members. CoordinatePair replaces SDK IntVector2 at the verified call
  boundaries. Successfully constructed region-role results now use original
  destination-only disposal before caller storage release; a native construction
  failure quarantines its partial storage and disables further role capture.
  See [camera/World values](extractor-camera-world-values.md). Safe disposal of
  generated unpublished Zones remains unproved and their oracle retention is open.
- Remaining Zone enumeration, spawn-sidecar iteration and stored-column
  diagnostics now use the owned sequence/identity/column readers. Per-pass caches
  avoid repeated column copies; unavailable data defers probes instead of becoming
  known air. Game/World/Creature/Zone names are opaque pointer declarations;
  SDK headers, include paths and target gating are removed. Independent stat-query
  ABI/label evidence also replaces the historical SDK-address reference.
- Game-to-client-World and World-to-local-creature links, plus the separately
  embedded host-World identity, from the independent native ownership contracts.
- Client registration capture for creature recordings, approach selection,
  townsperson visits, combat selection/health/resource recording, client AI
  traces, hunt counts, weakest-monster selection and death/spawn recording.
  Failed death capture leaves watches intact instead of falsely retiring them. Capture resolves the separate client World from Game,
  acquires its frame guard, rechecks that identity and retains the guard through
  every borrowed-pointer use. Busy, malformed or changed captures defer the pass;
  only a successfully read empty list can represent an empty observation.
- Recording-only target-health helpers capture client and host registration
  sequences in separate scopes, preserving every matching duplicate-ID entry.
  Host capture nonblockingly takes the outer host guard before the World guard;
  reverse member destruction releases World before host. Both remain held through
  health access. An unavailable capture reports incomplete/deferred work rather
  than silently treating it as an empty actor set. Actual Win32 tests cover each
  guard's contention, acquisition order, held use scope and constructor unwinding.
  This excludes the named cooperating native phases; callback owner lifetimes
  remain required, with no general worker quiescence or whole-record transaction.
- Signed-ID tree lookup in the two recorder consumers. The observed client frame
  guard remains held through their pointer-use scope. A busy/unreadable lookup
  defers the approach recorder; it is not a missing-subject observation.

## In progress and remaining

- Direct SDK registration-list consumers have been replaced. Other World
  collections and raw diagnostic paths still require their own views/protection
  audit. Registration order, duplicate list values and ID-tree ordering remain
  distinct. The independent
  [host/AI protection audit](specs/steam-world-actor-protection.md) establishes
  host-before-World locking for the named host phases and separate AI/worker
  scopes; neither guard alone covers all named host writers. The client capture
  helper must not be reused as a host capture or generic quiescence API.
- Temporary generator lifetime remains unresolved. The independent
  [refresh lifetime audit](specs/steam-zone-refresh-reference-lifetimes.md) proves
  that World+0x1a0 can retain the actual Zone pointer until later native consumer
  dereferences and tree clearing; the Zone destructor does not detach it. Keep
  that storage. Context-tagged request snapshots and sticky mismatch/failure
  pausing now preserve admitted work; see [generation request policy](extractor-generation-requests.md).
  These checks are not lifetime pins or atomic whole-World capture.
  Experimental gen.request batches now admit at most 32 native
  allocation attempts over the extractor process, including failed attempts.
  Empty, malformed, oversized and over-budget requests remain on disk, and active
  batches cannot be redirected by a second request. This is project policy, not a
  native memory limit or proof of all generator reference lifetimes. Split larger
  capture requests into smaller sessions; ordinary loaded-zone export is unchanged.
  The separate [generator escape audit](specs/steam-zone-generator-escape-audit.md)
  distinguishes its complete bounded normal-flow callsite inventory from unproved
  callee scopes and runtime/exception/lifetime boundaries; it does not authorize
  cleanup of arbitrary generated temporaries.
  Explicit unsigned 32 seed arithmetic preserves the native modular equation at
  signed-coordinate extremes.
- Complete creature/controller lifetime and transitive writer audits remain
  separate from the replaced core, appearance, pose, inventory and effect views.
- Keyed event-list and owning-string event behavior remain outside the simple
  capture adapter; broader native event-production comparisons remain open.
- Keep the SDK-free build/dependency audit in validation and qualify its DLL
  exports, supported-game startup, extraction and recovery paths.
- The independent [startup ordering audit](specs/steam-world-metadata-startup-readiness.md)
  establishes ordinary client/host World setup before completed Game publication,
  including title-menu frames. World construction/nonnull pointers alone are not
  readiness predicates; later transitions and arbitrary observations stay unproved.
- Run supported-game startup, title and loaded-world extraction comparisons;
  verify owner files are restored, then qualify packaging and wider acceptance.

The current CMake extractor target uses no SDK headers or include paths. The
registration/container checks detect
bounded malformed or changed metadata; they do not retain allocation lifetimes
or provide atomic whole-creature snapshots. The observed guard excludes only
cooperating writers, and its constructor/destructor require a live World.

Synthetic byte records, sparse node layouts, sentinel exclusions, signed-ID
extremes, malformed/change/budget cases and actual Win32 contention/unwinding
checks exercise these new views. All five targets and the full CPU/owner suite
pass. They are not new native live execution or output-parity acceptance.

After this dependency work, the requested order is multiplayer compatibility and
persistence, inventory/controller UI state, then remaining gameplay/rendering
gaps and final validation. See the [full checklist](roadmap.md).
