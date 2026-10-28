# Source review checkpoint — 2026-10-28

This is a replacement-engine source package for review. It is not a finished-game
release or a claim of pixel/behavioural parity. Older dated development notes may
describe prototypes that have since been replaced; the current README and this
checkpoint describe the intended setup and remaining limits.

## Current handoff items

- Water: implemented the written Steam surface curve, colour/depth passes, full-height
  exposed water faces, wet-ground films and underwater tint. Alpha dynamic models
  follow water. Default-on lilies use owner assets as independent render decoration.
  See [water progress](water-progress.md) for checks and remaining lighting/order gaps.
- HP above maximum: fixed native-player initialization and equipment updates at the
  record boundary. The regression preserves wounds between frames. Updated GPU
  captures show 136/136 where the earlier capture showed 169/136.
- Statue slab: compared 81 columns against owner-exported blocks. All 477 stored
  blocks within the slab height range match; the generator was left unchanged.
- Character selection: reuses the owner's rounded `gui.plx` frame, blended behind
  previews so it does not tint them. Opacity remains a visual estimate; a 1:1 colour
  match and the logo overlay's native light are not established.
- Packaging: a separate allowlisted tree and ZIP include required source tables,
  specifications, config templates, project art and dependency notices. Game data,
  screenshots, recordings, saves, SDK checkouts, private handoffs and binaries are
  excluded. Local research is preserved outside the upload tree.
- Source license: the project owner selected GPL version 3 for original engine and
  extractor source (`GPL-3.0-only`). The full text is in the root LICENSE; existing
  third-party licenses and notices remain separate.

## Validation

The development build and full EngineTests suite passed, including synthetic water,
lily and HP checks, owner-data comparisons and the setup wizard's simulated-install
tests. Steam and Alpha Vulkan captures completed on an RX 6600 with saving disabled;
the updated character-select screen was inspected. These cover the tested cases.
The staged source package built without compiler warnings in a separate CMake output
directory using cached dependency sources, and its full EngineTests run passed.
It contains no dependency build products or game exports. This is a source-completeness
check on the development machine, not a fresh-machine installation test.

No original-game session, native save edit or mod installation was needed for this
checkpoint. Captures are private and excluded from the source archive.

## Work still open

- Water: Steam vertex occlusion/point-light parity, Alpha's far-static-model threshold,
  and wider waterfall, underwater, wet-ground, dusk/night and material comparisons.
- Frontend: exact panel opacity and logo shading require further independent evidence.
- Gameplay edges from the handoff: sound coverage/music fades, ability/AI cases,
  pets, controller sticks, conversation give/take commands and network feat/guest-quest/
  monster-hit details. Recorded comparisons do not prove complete gameplay parity.
- Human/system checks: two Steam accounts and an original-game peer, a real first-run
  wizard session, a fresh-machine install and extended save/load gameplay.
- Publication: complete provenance/rights review; the source-license choice is complete.
  See [PUBLICATION.md](../PUBLICATION.md). Existing third-party licenses remain separate.

Do not upload the whole development checkout. Review the fresh
`build/CubeVulkan-upload-<timestamp>/` tree produced by `tools/package_source.ps1`,
then upload its contents after the publication decisions above.
