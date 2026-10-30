# Source review checkpoint — 2026-10-29

This is a replacement-engine source package for review. It is not a finished-game
release or a claim of pixel/behavioural parity. Older dated development notes may
describe prototypes that have since been replaced; the current README and this
checkpoint describe the intended setup and remaining limits.

## Current handoff items

- Pyro regional difficulty now uses the first entrant's level while a region has
  occupants. The host shares it in multiplayer; single player uses the same path.
  The last departure/disconnect clears it, so re-entry chooses a fresh level.
  Permanent discovery difficulty locks are ignored. Existing biome/id variation
  and gear-level records remain. Real two-session UDP tests cover publication,
  retained occupancy, last departure and refreshed re-entry.
- Steam model/beam visibility now uses the mesh job's native column/face prepass,
  published-geometry readiness, five-plane containing-zone test, stored center,
  exact early fixed-distance arithmetic and raw six-plane model-sphere test.
  Native CPU coordinates retain unshifted game Z. Missing neighbor data retains a
  conservative fallback. The camera lens/distance settings remain configurable;
  native adaptive range and camera-state matching still need implementation.
  A versioned owner-export record captures coherent bounds/counts/triangle cost
  for future comparisons. Synthetic/file-format regressions and no-save Vulkan
  captures pass; no fresh native cached-box oracle or pixel match is claimed.
- Native Steam front/back/ambient RGBA targets and state smoothing now follow
  the separate author's completed specification. Steam world RGB uses the native
  targets, with optional moon additions kept separate. Tests cover ordered regional/
  structure blends, effect13, low/high day interpolation, constructor values and
  exact native coefficient words. The frame input uses the verified1000-ms wall-time
  cap before float conversion; rendering reads the current client clock/buff state.
  Day/night no-save GPU captures pass. Native ambient alpha is retained in CPU state,
  but its shader saturation/composition stage is still pending. Point registration,
  sky/fog special overrides and remaining shader/color behavior still need work.
- Runtime follow-up: incoming local-player hit rules, native music smoothing,
  gameplay XInput mapping/sticks/camera, page-scoped dialogue marks and daily world/
  character reset behavior now follow the independent runtime-gap specification.
  The complete EngineTests run passed, including the new focused regressions and
  owner-data comparisons. GUI controller navigation, dialogue answer activation
  and the other roadmap gaps remain open. The subsequent equipped-pet update pass
  also builds and passes the full suite, with focused creation, naming, health,
  inheritance, authority, travel and detachment regressions.
- Follow-up timing: music receives double-precision elapsed frame time through the
  audio API with the native 1000-ms cap. The title root activates on mouse release,
  correcting the older spec's description of the 0x965e0 input callback.
- Enemy lifecycle: configurable Timed/Instance/Permanent modes. Timed defaults to
  300 seconds; Instance resets on spawn-zone unload for all players. Temporary
  deaths do not enter permanent saves, and mirrors handle repeated deaths.
  Synthetic and owner-data generated-world integration checks cover these paths.

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
- Gameplay edges: sound coverage and feat/chat presentation, ability/AI cases,
  integrated pet combat/appearance comparisons, controller GUI/aim tracking,
  conversation choices/emotes/numeric item actions,
  and network feat production. See the [audited roadmap](roadmap.md). Recorded
  comparisons do not prove complete gameplay parity.
- Human/system checks: two Steam accounts and an original-game peer, a real first-run
  wizard session, a fresh-machine install and extended save/load gameplay.
- Publication: complete provenance/rights review; the source-license choice is complete.
  See [PUBLICATION.md](../PUBLICATION.md). Existing third-party licenses remain separate.

Do not upload the whole development checkout. Review the fresh
`build/CubeVulkan-upload-<timestamp>/` tree produced by `tools/package_source.ps1`,
then upload its contents after the publication decisions above.
