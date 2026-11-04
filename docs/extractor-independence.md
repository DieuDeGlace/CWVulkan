# Extractor independence workboard

The current priority is to remove the extractor's remaining CWSDK dependency.
The [provenance audit](sdk-license-provenance.md) has not established a grant for
the exact SDK snapshot. Independent replacements use original project code and
the separately authored contracts in `specs/`; SDK headers, implementations,
compiler layout dumps and third-party reconstructions are not implementation
references. Native-only authors use the standalone
[author policy](CLEANROOM_AUTHOR.md), which contains no implementation history.
This workboard does not grant rights or certify final redistribution.

## Completed adapters

- Loader callback record and forwarded-context validation.
- Primary sprite catalog and ordinary vertex-buffer metadata. Calibration context
  discovery, calibration naming and animation/snapshot slot maps now use guarded
  primary reads. An unavailable map defers recording and preserves previous
  snapshots/calibration rather than publishing unknown slot IDs. Other catalog
  side records and tour/graphics consumers remain in the remaining-view scope.
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
  for negative coordinates. Other scalar consumers remain to be migrated.
- Game-to-client-World and World-to-local-creature links, plus the separately
  embedded host-World identity, from the independent native ownership contracts.
- Client registration capture for creature recordings, approach selection,
  townsperson visits, combat selection/health/resource recording and client AI
  traces. Capture resolves the separate client World from Game,
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

- Remaining SDK registration-list consumers need migration with their complete
  borrowed-pointer use scopes. Registration order, duplicate list values and
  ID-tree ordering are distinct. The independent
  [host/AI protection audit](specs/steam-world-actor-protection.md) establishes
  host-before-World locking for the named host phases and separate AI/worker
  scopes; neither guard alone covers all named host writers. The client capture
  helper must not be reused as a host capture or generic quiescence API.
- Remaining creature scalar consumers, appearance/pose fields and dynamic
  inventory/auxiliary buffers.
- Remaining World fields, collections, region/material sidecars and record views.
- Remaining Game renderer/camera/Speech fields and native vector/matrix aliases.
- Server-update list locations and payload decoding.
- Remove the SDK include/configuration after the final consumer is replaced;
  compile the extractor without the sibling SDK checkout.
- Run supported-game startup, title and loaded-world extraction comparisons;
  verify owner files are restored, then qualify packaging and wider acceptance.

The current CMake extractor target still requires CWSDK. No completed slice above
is described as full SDK independence. The registration/container checks detect
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
