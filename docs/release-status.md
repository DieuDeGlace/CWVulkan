# Source review checkpoint — 2026-10-31

This is a replacement-engine source package for review. It is not a finished-game
release or a claim of pixel/behavioural parity. Older dated development notes may
describe prototypes that have since been replaced; the current README and this
checkpoint describe the intended setup and remaining limits.

## Current handoff items

- Native item details now use an independent text plan for the specified category
  rows, stars/region/realm text, artifact/rank/pet effects, healing/demon/consumable
  text, exact float rounding and separately rounded comparison deltas. Equipped
  comparisons preserve original address identity, candidate/Shift order, smaller
  fonts and two late resizes. Inline suffixes follow the real glyph pen; fractional
  fonts share the owner atlas. Tests reproduced missing glyphs and stale standalone
  GUI scale before correcting both. All five targets, the full CPU/owner suite,
  40 world/20 GUI Vulkan cases, water and Alpha checks pass. A private read-only
  gameplay captures at two GUI scales display the selected weapon and its original
  equipped comparison, with unchanged fixture hashes and matching validated binaries.
  Earlier selector/source sampling, live map/ground/Ring bindings and broader
  GUI/input/camera and system checks remain release gates. No owner assets ship.

- Native item-preview selection now positions from stored geometry before its
  later draw. The owner background/children precede icon/text and the text-derived
  resize; a hidden draw retains the size, and growth/shrinking affects the next
  background. Four owner-tree lifecycle regressions fail against the previous
  implementation and pass with this correction. All five targets, the full
  CPU/owner suite, 40 world/20 GUI Vulkan cases, water and Alpha checks pass.
  Ring source selection also retains
  its independent ground-origin flag, rather than forcing it to describe the
  selected canonical item. The later formatter/comparison implementation is described
  above; live source producers and timing still prevent complete PreviewWidget parity.

- Native-save customization now uses the actual owner editor root and shared
  WORLD models: equipped thumbnails, raw base/attributes/candidate and material
  palette, with native picking, cached clicks, rotation and bench/movement gates.
  Ordinary item downs rejoin the editor before shared health/chat/Ring completion.
  Full-coordinate pickup/removal and byte placement retain their distinct rules.
  Palette information follows native source priority and the hidden crafting
  anchor; text and image use a separate class-adapted copy. Gameplay wheel events
  are cleared before each poll, preventing idle-frame replay. Focused regressions,
  all five targets, the full CPU/owner suite, 40 world/20 GUI Vulkan cases, water and
  Alpha checks pass. Private read-only owner-resource captures exercise the pane
  and palette tooltip; they are not original-game pixel comparisons. Broader
  cached inventory/input state, GUI capture/map controls, preview source timing
  and map/ground bindings, controller aim and system checks remain release gates.

- Physical inventory wheel messages now retain their event pointer and sign,
  selecting one owner-tree hit in inventory/shop/crafting priority. Shop and recipe
  grids can receive wheel input while the bag is visible elsewhere. Opposing
  messages remain separate; fractional or large deltas do not accumulate into
  different row counts. Idle input passes leave the stored row alone. Synthetic
  priority/sign cases cover this routing. Cached widget range/thumb state, precise
  refresh callbacks, first binding and broader native input comparisons remain open.
  All five targets build; the full CPU/owner suite,40 world and20 GUI Vulkan cases,
  shared water and Alpha ordering pass.

- Alpha shared water now draws its sorted fading-scenery queue after the sky and
  before the refraction snapshot. Previously, transparent scenery could write
  depth before its sky background and leave dark silhouettes. Native water keeps
  its after-water queue, and CPU fade values and depth writes are unchanged.
  New Vulkan regressions failed before the change and pass afterward: dry partial
  fades match between water modes, and beyond-endpoint fades retain the sky.
  Native admission/cache traversal and broader original-game comparisons remain open.

- Character creation now includes optional dual-tone hair controls: second-color
  preview/selection, enable, transition start and blend. The native palette and
  runtime-loaded hair models are reused without a mod DLL. Per-character engine
  notes retain the style through local selection, play and map rendering, leaving
  the native appearance record unchanged. Synthetic tests cover gradient/skin
  handling, hover/commit, bounds, cache identity and metadata round trips. Hidden
  read-only creation captures exercise Steam and Alpha lighting. Remote-player
  synchronization of this extra appearance remains open; no owner assets ship.
  All five targets build; the full CPU/owner suite,40 world and20 GUI Vulkan cases,
  shared water and Alpha ordering pass. The captures verify the new controls and
  hair; broader background/camera and original-game visual comparisons remain open.

- Customization's native edit projection, object-space pointer ray and voxel/attribute
  traversal now have independent engine implementations and synthetic regressions.
  Checks include full header centering, scale independence, stored lens words,
  exact negative-integer cells, marker exclusion and wrapped attribute coordinates.
  The GUI renderer accepts the raw edit matrices/material with its specified WORLD
  palette and retained state. All five targets, the full CPU/owner suite,40 world
  and20 GUI Vulkan cases, shared water and Alpha ordering pass. The editor root,
  presentation and event continuation are now integrated above; these backend
  checks alone do not establish native pixel parity.

- Native customization palettes now read every canonical tab directly: the selected
  weapon's exact base-material item precedes four spirit-material aggregates.
  Signed counts wrap and may remain negative; aggregation retains its distinct
  full-equality and field-only rules. Clicks consume the preceding draw's palette;
  the later draw rebuilds its entries. Synthetic regressions cover ordering,
  extra tabs, inactive attributes, heterogeneous spirits, cancellation and overflow.
  All five targets, the full CPU/owner suite,40 world/16 GUI Vulkan cases and
  water/Alpha checks pass. The later owner-root integration is described above;
  these palette checks alone do not establish complete customization parity.

- Newly loaded inventory models now share one pending atlas rebuild before drawing,
  instead of rebuilding and uploading the font/icon atlas for every new item.
  Explicit layout updates also clear the pending work. A late-resource owner-tree
  regression, all five targets, the full CPU/owner suite,40 world/16 GUI Vulkan
  cases and water/Alpha checks pass. A hidden read-only recipe capture from the
  optimized binary renders the result/ingredient normally and leaves the save
  unchanged. This is a resource scheduling change, with the same item draw rules.

- Formula detail results and ingredients now use original-record WORLD images;
  hovered item information uses a class-adapted copy. Their separate callback,
  pivot/frame geometry, rotation, dark-pass and depth rules follow the independent
  specification. Missing result resources skip the detail hook without advancing
  its local angle, and hidden draws do not accumulate catch-up. GUI timing now
  narrows each capped elapsed step to float then signed milliseconds and wraps
  its accumulated total across title/game frames. Ingredient hover/navigation
  share the same geometry. The full CPU/owner suite, all five targets,40 world
  and16 GUI Vulkan cases, shared water and Alpha ordering pass. Other icon callers,
  GUI layers and persistent widget refresh state remain open. Hidden, read-only
  native-save captures at GUI scale1 render the recipe result/ingredient and hover
  preview, stop normally and leave the fixture unchanged. Capture executables
  match the validated build; these are not original-game pixel comparisons.

- Inventory, shop and recipe cells now render their original records through the
  shared WORLD item helper during the owner's InventoryWidget callback, after its
  child frames. Physical inventory slots are retained rather than packed; holes,
  per-viewport region cutoffs and full-tab scrollbar counts remain distinct.
  Screen snapping truncates toward zero, hover uses half-open rectangles, and
  icons retain native signed scale, category sizing, GUI spin and spirit data.
  Labels use native class/color and mode-specific quantity admission. Resource
  loads are shared within a session. Formula details, hovered previews and other
  callers still need conversion; full GUI layers and refresh timing remain open.
  All five targets build; the full CPU/owner suite, 40 world and 16 GUI Vulkan
  cases, shared water and Alpha ordering pass. A hidden, read-only native-save
  capture renders the owner grid model and stops normally with the fixture
  unchanged. These checks do not establish complete native visual parity.

- Ring Menu now uses canonical category1 slots, retained signed selection and native
  neutral/repeat/animation state. Selected Use precedes independent Quick Item; the later
  toggle and selection update preserve same-frame ordering and pre-repeat labels.
  Bombs and portal stones use the existing shared item helper, including borrowed shrine
  selection without portal depletion. The owner selector artwork, Display-based arrow
  feedback and selected Quick Item3D SpriteWidget are connected. Ring items use their
  distinct queued50-degree WORLD path, full-header dimensions, world-clock spin and
  retained depth before Plasma, without direct-widget black/spirit passes. Combined UI
  open/close sounds are sampled once after visibility changes. CPU/owner-tree checks,
  all five targets,40 world and16 GUI Vulkan image cases, shared water and Alpha ordering
  pass. Original-game/hardware comparisons and other widget callers remain open.
  Location-banner aging now uses capped UI elapsed time rather than simulation substeps.
  Read-only native-save Ring captures at GUI scales1/0.4096 render three owner items;
  the final capture executable matches the validated build and the fixture save is unchanged.

- Native equipment slots now render original item records through the shared3D WORLD
  helper at each SpriteWidget's owner-tree callback. The copied binding, full-header
  sizing, signed GUI spin, dark/main passes and spirit attributes remain intact;
  child labels/frame art follow the item. Hover does not enlarge this caller.
  Record-less legacy sessions retain compatibility images. Inventory grids,
  crafting and other item-preview callers still need conversion; selected Quick Item
  is covered by the later Ring Menu integration above.
  Headless owner-tree checks cover record preservation, draw order, empty/missing
  resources and visibility. All five targets, the full CPU/owner-data suite,
  40 world and13 GUI Vulkan image checks, shared water and Alpha ordering pass.

- Character-menu controls now initialize and animate the owner's ordinary Button sequences
  instead of showing the saved gray idle pose. Select uses live effective frames and20 physical
  pixels of padding. Card binding is transferred from the copied template; horizontal centering,
  ordered vertical fitting, filtered left-drag and momentum follow the independent specification.
  Pointer polling and frame decay retain separate clocks. CPU state/owner-data checks cover
  these paths. Cached GUI masking/shine and preview creature/world rendering remain open;
  these changes are not a claim of complete menu parity.

- Identification now renders the original selected item through the shared 3D WORLD
  shader path, using owner-extracted CUB geometry and full header dimensions. Native
  projection, rotation, main/dark passes, shininess and weapon spirit cubes are
  integrated with immediate UI ordering and retained depth. CPU checks and 13 actual
  Vulkan image checks pass, including TAA/SMAA, shadows and both lighting presets.
  Read-only native-save captures at GUI scales1/1.5 render the selected owner sprite
  and stop normally with the fixture save unchanged.
  Other widget callers still need conversion from bitmap previews; full inherited
  callback state, offscreen GUI layers and original-game visual comparison remain open.

- Shrine-choice map mode now follows item and creature use, restricts hover to shrine
  targets and accepts the preceding actual draw's target on eligible left release.
  It checks known cells with negative-coordinate floor division, starts the delayed
  native return ability, and preserves item quantity. Deferred mode/source cleanup,
  float world-clock pulses and GUI-scale label sizing are integrated. CPU/owner-data
  checks pass; full original-game/controller comparison remains a release gate.
- Character Select/Create and delete-control placement now uses the owner's effective widget
  frames after resizing, rather than treating the requested sizes as final dimensions. This fixes
  Select extending below its character card. Owner-shape picking replaces the old rectangular
  checks. Owner-data regressions cover UI scales 1 and 1.5; broader menu visual parity remains open.
- Native-save gameplay now uses canonical inventory tabs, equipment and counters.
  Screens derive live slot views and the save bridge writes those actual records.
  Direct equipment/use/drop, pickups, quick items, crafting, cube upgrades, shops,
  identification, pet food and daily reset use their specified distinct operations.
  Shop regressions check transaction data, separate
  sale nodes, whole-node buyback attempts and ordered refresh callbacks. Health
  adjustment stores its rescale before the second maximum-health callback/cap.
  All five build targets, focused inventory/save/loot-window checks and the full
  CPU/owner-data suite pass. A hidden read-only native-character run rendered
  the inventory and stopped normally. Native identification uses the owner's
  blackwidget/itemframe tree, native text/footer positions, live selection and
  release gates. Transformed edge/navigation tests and the CPU/owner suite pass;
  read-only captures at GUI scales1/1.5 exercise the owner artwork and typography.
  The remaining shared3D icon callers still use bitmap compatibility previews; full
  map-panel/capture behavior and extended owner validation remain open. Ring Menu
  and selected Quick Item are covered by the later integration above.
- Native adaptive scene range is integrated: actual unmerged face costs/readiness,
  option-derived window, native equal-key sort, budget threshold, easing and player
  state feed distinct terrain/model/actor/light ranges. Pending cache metadata is
  separate from active GPU geometry and is reused coherently for culling and ground
  lights. Missing legacy cost evidence is not replaced by a guessed triangle count.
  All12 sampled clean native min/max/center/radius and weighted costs match; the full CPU/owner-data suite and
  40 actual Vulkan image checks pass. Native radius15 also completed a hidden no-save
  settled-world capture (sampled private peak1,687MiB). Camera, inherited shader/pass
  inputs, readiness-opacity consumption and wider native visuals remain open.

- NPC speech/emotes now use owner templates and models, with per-speaker playback,
  repeat-Use advancement, exact capped clocks, cached projection and native emote
  pose/depth/culling rules. Lore and receipt scripts append to the general panel;
  final-page lore activation preserves its distinct sound rule. Category24 item
  descriptions receive the item's region context and verified description phrase.
  The full CPU/owner-data suite, 33 actual Vulkan image checks and a hidden no-save
  live speech capture pass. `tools/validate_engine.ps1` runs the full suite followed
  by actual `steamshader` rendering; `steamvoxel` is a CPU data test. These checks
  do not establish complete native visual parity. The other implementation/system
  checks below still need work.
- Saved recipe records now reach the crafting UI through the native progress bridge.
  Missing startup defaults use full item identity; unlisted records remain preserved,
  and crafted results retain formula and all128 upgrade bytes. Full CPU/owner-data
  tests pass, including lifecycle/default-merge and attribute/padding regressions.
  This batch changes no shader path; Vulkan image checks remain the33 passing cases
  from the preceding dialogue checkpoint. No book receipt or decoder-only opcode
  was turned into an invented recipe award.

- Live lighting now reads personal used-cell state, kind3/4 projectiles and
  glowing ground items from gameplay. It preserves native ownership, timer,
  quantity, coordinate conversion and fade rules. Model and terrain light ranges
  are separate. Hit flashes now use consumed local/received hit records, current
  spirit weapons and native fixed positions, first-frame aging and expiration.
  CPU regressions and 28 Vulkan cases cover these sources, including zero-fade
  slots, personal state updates, projectile filtering/removal and owned/delayed/
  picked-up items. The race0x90 death flash is also integrated, using the persistent
  pose-preparation marker, drawn position, cached previous renderer visibility,
  fixed 100-block range and native candidate ordering/800-ms lifetime. Death births
  precede hit-display flashes. Further CPU/GPU checks cover these gates and cached
  visibility across pending origin changes. Live player/NPC records and buffs now
  supply carried/toggled lamps, race/state effects, casting and equipment spirit
  glow, with native anchor/rotation/truncation and source ordering. The casting
  sphere range has its separate native zone-window calculation and 256 cap.
  Native mesh-budget/adaptive range inputs are now connected as described above.
  Camera state, shader/pass inheritance and broader native visual comparisons remain
  open. Source-branch tests do not prove complete frame-input parity.

- Isolated reference capture now runs automatically after game-session
  initialization and refreshes cached light snapshots. The real-install run
  captured both auxiliary lists in 1,590 zones; all 8 model and 9 static lights
  match the engine reader, including mesh-time light samples. All 3,823 terrain
  reference points across 25 fields and 4,847 block samples match. The two region
  failures came from the earlier BigRegions-modified reference. The full suite
  passes, and original Save, Mods and ReShade were restored byte-for-byte.
  These sampled comparisons do not establish full visual parity.

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
  adaptive inputs are integrated above; native camera-state matching remains open.
  A versioned owner-export record captures coherent bounds/counts/triangle cost
  for future comparisons. Synthetic/file-format regressions and no-save Vulkan
  captures pass. The later12-cache oracle above matches fresh native boxes/costs;
  no complete pixel match is claimed.
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
  owner-data comparisons. String-script answer activation, inn payments, persisted
  specialization changes and the general feedback panel are now integrated, with
  native word colors, row picking, navigation points and distinct UI action gates.
  Focused tests include the owner's fonts/templates at unit and 1.5 GUI scale.
  Broader GUI controller coverage and the other roadmap
  gaps remain open. The subsequent equipped-pet update pass
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
  previews so it does not tint them. The owner's alpha is retained without the earlier extra
  0.6 multiplier; a full composition match and the logo overlay's native light are not established.
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
  newly observed item-action producers and wider gameplay comparisons,
  and network feat production. See the [audited roadmap](roadmap.md). Recorded
  comparisons do not prove complete gameplay parity.
- Human/system checks: two Steam accounts and an original-game peer, a real first-run
  wizard session, a fresh-machine install and extended save/load gameplay.
- Publication: complete provenance/rights review; the source-license choice is complete.
  See [PUBLICATION.md](../PUBLICATION.md). Existing third-party licenses remain separate.

Do not upload the whole development checkout. Review the fresh
`build/CubeVulkan-upload-<timestamp>/` tree produced by `tools/package_source.ps1`,
then upload its contents after the publication decisions above.
