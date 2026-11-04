# Extractor independence workboard

The current priority is to remove the extractor's remaining CWSDK dependency.
The [provenance audit](sdk-license-provenance.md) has not established a grant for
the exact SDK snapshot. Independent replacements use original project code and
the separately authored contracts in `specs/`; SDK headers, implementations,
compiler layout dumps and third-party reconstructions are not implementation
references. This workboard does not grant rights or certify final redistribution.

## Completed adapters

- Loader callback record and forwarded-context validation.
- Primary sprite catalog and ordinary vertex-buffer metadata.
- Loaded-zone enumeration, active terrain buffers and their scoped lock pair.
- Stored Zone terrain and the original CWZ1 codec.
- Proper-name fragment data reads from a verified RVA/length manifest.
- Owned 160-byte item records and eleven-slot equipment capture. Scalar widths,
  attributes, count and padding remain byte-preserving capture data. Native
  semantic item assignment remains a distinct operation. Naming/class-rule
  probes, equipment diagnostics and recording helpers use the owned record.
- Game-to-client-World and World-to-local-creature links, plus the separately
  embedded host-World identity, from the independent native ownership contracts.
- Signed-ID tree lookup in the two recorder consumers. The observed client frame
  guard remains held through their pointer-use scope. A busy/unreadable lookup
  defers the approach recorder; it is not a missing-subject observation.

## In progress and remaining

- The registration-list reader is implemented and tested. Existing SDK list
  consumers still need migration with their complete borrowed-pointer use scopes.
  Registration order, duplicate list values and ID-tree ordering are distinct.
- Creature scalar/appearance/pose fields and dynamic inventory/auxiliary buffers.
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
