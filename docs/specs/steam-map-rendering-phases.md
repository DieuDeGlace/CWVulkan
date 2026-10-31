# Steam map camera, marker and pointer-text phases

Independent native-only timing supplement to
[world-map rendering](worldmap-rendering.md), [map markers](map-markers.md),
[world-map records](worldmap-model.md),
[Preview map/flight/lore](steam-preview-map-location-lore.md) and
[hidden-interface order](steam-preview-hidden-interface-order.md).
Sources:0xa5a50,0x103200,0xe2e00,0xe64b0,0x2f44a0,0x2829c0,
0x83c00,0x83c60,0x6d640,0x9bad0,0x938b0,0xc4af0,0x33e170,
0x330b20 and native constants/instructions. No implementation, tests,
exporter/build, SDK, owner assets/captures or game execution were used.

## 1. Ordered ordinary frame stages

These are CPU calculation/queue/submission stages of one reached
native frame, not a claim of GPU completion at a particular CPU call.

| Order | Stage and relevant RVA |
|---|---|
|1|Window-message drain and ordinary pointer/controller polling, before Game frame; hidden-interface §5 supplies those entry points.|
|2|Early Game update sets/eases **drawn map centre G+0x1e20/+0x1e28/+0x1e30**, using target centre+0x1e08 and panning state+0x1e38. Smoothing call **0xa7f42→0x7d0b0**.|
|3|Ordinary use **0xa9b2c**, then Preview source selection **0xa9d22**. Preview samples retained outputs of the most recent reached producing draws.|
|4|Game's pointer-distance string update, formatter **0xaa154→0x938b0**, described in §5 below.|
|5|Gameplay camera update **0xaaf02→0xadcd0**; later map/panel toggles and drag-target work. Map toggle **0xaafcc** follows source selection and pointer text.|
|6|Late map screen-centre easing **0xab4f9/0xab591→0x7d050** and scale easing **0xab5b6→0x16ee0**, then Interface **0xab5ce→0x103200**.|
|7|Assemble scene flags and current displayed-camera angles; call scene render **0xad9d4→0xe2e00**.|
|8|If admitted, map pass **0xe41a2→0xe64b0**, whose content/camera call is **0xe65c1→0x2f44a0**. Current map matrices, 3D content and terrain pick are calculated here.|
|9|That same map pass submits opaque then faded map queues at **0xe67df→0xe6e60**, **0xe67f1→0xe7980**.|
|10|Later renderer GUI virtual+0x10 at **0xe4f35** dispatches **0xc4af0→0x33e170→0x330b20**. An admitted MapOverlay hook0x2829c0 collects/projects its current flat markers; Preview's later hook follows it.|

Worldmap-rendering §§4.4–4.5 retain the centre/easing formulas.
When not panning, early Game update sets both target and drawn centre
to the player's fixed position; otherwise it refreshes target height
and eases drawn centre. The later drag stage can change the target
after this reached smoothing call. No second centre smoothing call is
introduced before current map rendering. Screen-centre fractions and
scale are separately updated late, after the earlier source/text work.

Thus Preview selection and pointer-distance text **do not sample a
fresh current-frame 3D map pick**. The camera/projection and marker
screen positions are produced later. A normal preceding draw may make
their retained inputs one frame old; skipped draws can make them older.

## 2. When the map matrices and 3D positions are produced

Scene entry0xe2e00 copies its current angle triple to renderer
+0x350/+0x354/+0x358. The map pass uses those **current displayed
world-camera angles**, not newly selected map-marker facing or the
earlier Preview's coordinates. Map pitch/yaw, handedness, projection,
depth shift and view scale remain worldmap-rendering §4.2.

In **0xe64b0**, screen centre is calculated from the current eased
fraction pair and current window dimensions. Each **float32 dimension
times fraction is truncated to signed32 pixels** before passing it to
0x2f44a0. Current centre, zoom and view-scale inputs are the ordinary
Game fields documented in worldmap-rendering4.1. These are not an
additional transform reconstructed by the later Overlay.

**0x2f44a0 writes projection at WorldMap+0x18 and view at+0x58 before
queuing its map content.** It also writes drawn radius+0x19c and
increments its map draw counter. Region instances, 3D creature/player
markers, tombstone and route content are transformed/queued using this
call's camera. Map-markers Part2 and worldmap-rendering §§4/6 retain
their per-family admissions, fixed/world coordinate inputs and order.
The player-label map+0x128 is populated with current clamped world
positions here, for the later flat overlay; it is not the flat marker
screen-position array.

The wrapper then binds the map lighting/material and **those same
stored view/projection matrices**, and submits the opaque/faded queues
before returning. Current flat GUI markers therefore follow current
map submission, rather than being projected early against the preceding
camera and later painted over a new 3D view.

Current camera does not mean every hover-dependent3D input is fresh.
The flight-target+0x150/+0x154 reads in0x2f44a0's route selection and
0xe64b0's map-highlight input still see the retained target before
the later Overlay reset/hover writer. This timing supplement does not
replace their curve/material formulas with a newly picked flat marker.

The height guard is inside0x2f44a0: after cursor/label reset, a height
below float0.0001 returns before rebuilding matrices/radius. A skipped
3D pass likewise does not manufacture a fresh map camera. No new
camera calculation is performed by MapOverlay as a fallback.

## 3. 3D-pass admission, F4 and independent Overlay admission

Map pass0xe64b0 is reached from0xe2e00 exactly under its existing gate:
**World+0x210 is nonnull AND scene flags bit0x20 is clear**.
This is the same path for minimap and full map. Neither full-map byte
G+0x668 nor map drag G+0x1db0 is an extra on/off admission for that pass.
Full-map state changes camera placement/scale and a passed mode flag;
it does not choose an independent second map renderer.

The scene-bit producer in **0xa5a50** is important:

* Query the **global GUI scene root's own live visibility** at
  **0xad780**. Hidden sets bit0x20. This is the root toggled by F4,
  not the game-UI child G+0x220.
* Own shown title G+0x118, character-creation G+0x170 or
  character-selection G+0x130 also sets bit0x20; queries are
  **0xad81d/0xad82d/0xad83d**.

Consequently **F4 suppresses the 3D map pass as well as the GUI root
traversal**. Its hidden interval does not update the map matrices,
3D pick or Overlay hover through either of those draw paths. Game
source selection, pointer-string work, centre/easing updates and
Interface bindings retain their separate normal execution.

This is more precise than treating every hidden Widget/ancestor as a
3D map gate. MapOverlay's own visibility is assigned from G+0x220 late
in0x103200; its GUI admission is ordinary0x330b20 ancestor visibility
and node flag0x4. Neither its own shown value nor that generic draw-disable
flag is queried by the3D-pass gate. For example, a global node flag0x4
with live visibility still1 can suppress GUI traversal without producing
the F4 visibility-derived bit0x20. Other existing scene gates still apply.
See hidden-interface §§1–3 for the hierarchy and unchanged own-state
queries; do not collapse those predicates into one broad UI-hidden bit.

The separate **half-radius flag, scene bit0x10**, is passed to
0x2f44a0 and halves radius8 to4. Its producer queries GUI engine key
code0x11 through0x9aeb0. This flag is **independent of minimap versus
full map**. It disables the terrain ray pick, not all 3D map drawing
or the Overlay's full-map marker-hover pass.

### 3.1 Code0x11 is retained message state, not a physical aggregate

**0x9aeb0** tests membership in GUI engine+0x198's set of **unsigned16
key codes**; the set node stores its code at+0x1a. Window handler
**0x133c70** forwards WM_KEYDOWN0x100 to **0x340df0** and WM_KEYUP0x101
to **0x340e60**, passing the message key's low16 bits unchanged.
The down handler inserts that code before forwarding to a focused
Widget; the up handler erases that code before forwarding. Repeated
down inserts do not count as multiple holds.

For the ordinary Control messages delivered with code **0x11**, this
is one retained membership state: either physical Control down inserts
the same code, and either Control up removes it. The handlers do not
inspect the message's scan/extended-key information to retain left/right
counts, and0x9aeb0 does not poll an aggregate physical-Control state.
There is no side-specific0xa2/0xa3-to0x11 normalization in this path;
the supplied code is the set key. Thus releasing one Control can clear
the queried0x11 even while the other physical Control remains held.
This statement covers the verified message/set route, not a new
focus-loss or device-state reconciliation policy.

## 4. Overlay collection, projection and cursor overwrite order

World positions/semantic record contents of region place/mission/land/
flight lists come from the cached **0x2eb7b0** generation described
in worldmap-model; they are not regenerated because the camera moves.
During its own reached **0x2829c0** draw, Overlay fetches admitted
cached records and **then** projects them into its temporary flat
marker list. It uses **WorldMap+0x58/+0x18**, the current Game centre
and zoom fields, and the map's last written radius+0x19c. Projection
visibility/screen arithmetic is map-markers0.6–0.7. Its player-label
positions come from map+0x128 populated by the preceding3D map call.

Under the normal admitted3D-map/Overlay path these are the **same
frame's map matrices**. If only3D map calculation was skipped while
Overlay still reaches its independent GUI hook, Overlay uses retained
matrices/radius/label data; it does not rebuild them for itself.

Collection, sorting, hover and painting happen within this one Overlay
hook. Flat markers are not a screen-position list built by Preview
selection0xa9d22 or by the3D creature-marker collector. Their existing
depth sort, hit distances and drawing order remain map-markers §§1.5–1.8.

After collection/sorting, Overlay resets its own embedded item and
lore-cell outputs. **Full-map G+0x668 nonzero and drag+0x1db0 zero**
admit its marker hover selection; restricted mode+0x669 further filters
it. These gates skip hover copying, **not collection/projection or the
reached reset**. A visible minimap/closed-full-map or dragged Overlay
therefore still collects, projects, paints and clears its hover outputs.
A hidden/skipped Overlay performs none of that work. Preview's later
borrowed-item versus copied-scalar distinction remains the Preview
map/hidden-interface supplements.

### 4.1 The shared map cursor has two later draw-stage writers

At entry **0x2f44a0** resets fixed cursor XYZ at+0x100/+0x108/+0x110
to **(INT32_MAX times65536, INT32_MAX times65536, −65536)** and clears
player-label map+0x128. The Y constant is0x473378. With half-radius
flag clear, current GUI engine cached pointer floats **+0x180/+0x184**
are unprojected through the new map matrices for the region-model ray
pick. A successful pick updates fixed cursor XYZ and hovered land
+0x118/+0x11c. The existing forced-land override+0x120/+0x124 retains
its reached handling; it is a land selection, not a new XYZ-validity test.

**Correction to map-markers2.1's combined-reset shorthand:**
0x2f44a0 does **not** reset+0x118/+0x11c at entry. Before a current
pick/forced override, that pair can retain the prior Overlay result.
The3D hovered-land highlight consumes the value present during its
current3D queue-building stage.

**Later Overlay0x2829c0 resets+0x118/+0x11c**, flight/shrine target
pairs and its own hover outputs, but does **not reset cursor XYZ**.
A newly nearest admitted marker hit then writes its signed32 block
XYZ shifted to fixed into+0x100/+0x108/+0x110, and replaces hovered
land+0x118 with0x2ada20 at that marker. This overwrite is after current
3D map/highlight submission. With no admitted marker hit, XYZ retains
the current terrain pick or its invalid-Z reset; the land pair has
still undergone Overlay's later reset.

Thus the next update's pointer text can see a marker-snapped cursor
or a terrain-picked cursor, according to the **last reached writer**.
Dragging blocks the Overlay marker overwrite but does not itself block
the3D terrain pick. Both picking/hover use the GUI engine cached pointer
at their respective calls; the distance-text updater does not read
current pointer XY or cast a fresh ray.

## 5. Exact pointer-distance string sampling

This is the Game-frame block ending in **0xaa154→0x938b0**, before
late camera/map-toggle/easing/render work. Its admission is:

* Overlay root+0x178 reports **own shown** through0x9af00; and
* signed64 fixed cursor **Z at map+0x110 is at least0**, tested by
  **0x83c60(cursorZ,0)**.

Otherwise assign the empty string to cursor scene node **G+0x448**
through0x334c50. There is no full-map, drag, ancestor-visible or
flight/lore-target test in this string block. F4 can therefore leave
its calculation running on retained data while suppressing the root
that would paint the text. Cursor-node show/style rules remain the
ordinary interface/owner-scene contract, separate from string sampling.
This own-visibility query also precedes0x103200's later reassignment
of Overlay visibility; it is not a snapshot taken after that assignment.

For a reached nonempty string, subtract **map cursor fixed XY from
current physical player fixed XY** as signed64 differences, via0x83c00.
**0x6d640** converts each difference directly to float32, then multiplies
by float32 **2^-16** at0x42e510. **0x9bad0** squares X/Y in float32,
adds those squares in float32, and takes sqrtf. Promote that float norm
to double, multiply by double0.5 at0x4ac9a8, truncate to signed32
metres and format through **0x938b0**. It has the same m/km thresholds
and decimal-digit rules as Preview map supplement4.2's0x28c9c0.
This cursor string has **no literal`Distance: ` prefix**.

This is horizontal distance; Z only supplies admission. It differs
from Preview flight distance's all-double norm and from a fresh
current-frame ray-to-player distance. Player position is current at
this update, while cursor data comes from its last completed producing
draw. The later current3D pick and Overlay snap first affect a later
reached string update; there is no post-Overlay resampling here.

## 6. Bounded checkpoint

Phase order, camera/Overlay reuse, F4/half-radius/drag distinctions,
cursor writer order and distance sampling above are verified. Existing
map-generation, projection/mesh/shader and marker-family contracts are
linked rather than replaced. Pre-first-producing-draw cursor storage,
exceptional interruption inside draws and owner-specific node/cache
settings are not inferred into new initialization or visibility rules.
