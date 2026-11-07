# Steam controller aiming: axis state, camera turn and target tracking

Status: FINAL / STABLE / HELD. Independent source-side contract, 2026-11-07.

## Reference identity and limits

This bounded contract describes the original Steam input admission and stored stick state, right-stick camera consumption, camera-centered creature targeting, aim publication and the identified target-assistance stages. It does not specify every controller button, combat action, interaction target, camera collision or character animation rule.

The authoritative image is `C:\Users\DieuDeGlace\Documents\decompiled cube world steam\binary\cubeworld.original.exe.unpacked.exe`, SHA-256 `7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244`, x64, preferred image base 0x140000000, PE timestamp 0x5D93AB0D, image size 0x58A000. All routine and instruction addresses below are RVAs. Its native decompilation, assembly, call references and referenced binary constants were examined. The packed original and installed Steam executable have SHA-256 `A1EFBBD41EEB8DF08395C4D6511194D416CB70A14054B2A2000B9F5CDBCFA65B`. Alpha is outside scope.

The separately authorized Infinite-Cubes Steam reconstruction served as a discovery aid. Native evidence controls the behavioral findings. No replacement implementation, project specification history, tests, SDKs, owner captures, logs or export packages were consulted. Previously held tooltip documents were not changed or used as evidence.

## Owned axis state and setters

The game object owns two separate pairs of single-precision stick components. The movement pair occupies offsets 0x1D6C and 0x1D70; the turning pair occupies 0x1D74 and 0x1D78. The constructor at RVA 0x7EA80 initializes both pairs to zero. Their setters are RVAs 0xA38C0 and 0xA3900 respectively.

Each setter first stores both supplied components. It then clears the entire pair only when their squared magnitude is strictly below the native threshold with float bits 0x3D23D70B, approximately 0.040000003: nominally a radial dead zone of radius 0.2. Equality is retained. Outside this dead zone the setter does not normalize the pair, subtract a dead-zone offset, rescale its remaining range, or independently clip each component.

The ordinary XInput publisher in RVA 0x1355A0 polls controller index zero. A signed 16-bit stick component is divided by 32768 before publication. Horizontal and vertical components keep their signs. Thus -32768 supplies -1, while 32767 supplies slightly less than +1. A diagonal is not reduced to unit length by these setters.

These stored pairs are independent of the held-control array and the controller-mode indication byte at game offset 0x1958. Calling a stick setter does not itself publish that indication. A successful poll is not, by itself, evidence that the mode indication became controller-active.

## Admission, reset and preservation cases

The following distinctions are original behavior of the polling path at RVA 0x1355A0. “Preserved” means the branch does not write the previously stored axis pair; it does not mean a fresh neutral sample was published.

| Polling situation | Stored movement pair | Stored turning pair | Separate control and mode facts |
| --- | --- | --- | --- |
| Successful ordinary gameplay, or the map-open branch | Refreshed from the left stick through its setter | Refreshed from the right stick through its setter | Held controls have already been cleared/rebuilt from configured keyboard/mouse inputs and then receive controller contributions. The axis setters do not clear controller-mode indication. |
| Successful non-map menu handling, with any of the visible inventory, crafting or vendor panels under the pointer | Explicitly zeroed through its setter | Preserved; this branch does not call the turning setter | Right-stick input is handled as panel scrolling and, for the inventory panel, region navigation. This is separate from held-control publication and mode indication. |
| Successful non-map menu handling without one of those panels under the pointer | Explicitly zeroed | Refreshed from the right stick through its setter | The right stick remains available to the later camera/model consumer. |
| Failed XInput poll, after ordinary input handling was admitted | Explicitly zeroed | Explicitly zeroed | The keyboard/mouse held-control refresh has already occurred. Failure does not add a controller-mode reset; earlier input-dependent mode writes still apply. |
| Foreground/active-input admission fails | Preserved | Preserved | The normal polling/control refresh branch is skipped. That skip itself does not clear held controls or mode indication. |
| Binding-capture diversion is active | Preserved | Preserved | The normal control rebuild and XInput polling branch is bypassed in favor of binding capture. Capture helpers at RVAs 0x8A120 and 0x8A4F0 edit/finish the binding selection; they do not call the axis setters. No axis or mode reset is established from that diversion. |

Normal input admission requires the application's active-input condition and its window to be foreground. The complete lifecycle and initialization of that global active-input condition were not audited. The inspected branch tests it; this contract does not infer its value from window appearance alone.

The menu branch is selected by the native interface-ownership predicate at RVA 0x1014F0 together with the map being closed. Inventory/crafting/vendor diversion tests each panel's own visibility and its node hit test. Merely opening an arbitrary menu does not imply that the turning pair is cleared. During panel diversion, even a neutral right stick leaves its earlier stored turning pair intact.

The diverted right-stick threshold is a strict absolute component magnitude greater than 17378. Vertical scrolling uses an initial 400 ms repeat interval and subsequent 100 ms intervals; horizontal inventory-region changes use the corresponding movement edge. These local UI tests do not republish a neutral turning pair.

In the ordinary gameplay/map branch, the controller-mode indication is set by the inspected axis check only when the absolute left-stick vertical component or right-stick horizontal component exceeds 7849. Left horizontal or right vertical movement alone is not included in that particular mode check, although their axis values are still published. Configured pressed keyboard or mouse controls can clear the indication earlier in the same poll; recognized controller button paths can set it later. This indication therefore must not be substituted for the contents of either stored axis pair.

The caller also applies some component dead-zone edits to its temporary XInput sample after both pairs have already been published. Those later edits do not retroactively change game-owned pairs. No later axis consumer of that edited temporary sample was identified in that branch.

Foreground loss does not imply the frame update has stopped: the outer loop still reaches the frame routine at RVA 0x134420, which calls game update at RVA 0xA5A50 when its existing-object guards admit it. The preserved-axis fact is consequently distinct from any claim that a background frame cannot consume those axes. No new automatic neutralization on foreground loss or binding capture is established here.

## Right-stick consumption and GUI interaction

Within RVA 0xA5A50 the right-stick consumer uses the stored turning pair, not the temporary XInput record. Each component is cubed independently, then multiplied by the stage's camera-turn rate and frame interval. Positive horizontal input subtracts from desired camera yaw; positive vertical input subtracts from desired camera pitch. The controller consumer does not consult the mouse inversion option or mouse camera-speed multiplier, and it does not perform a pitch clamp in this consumption block.

The base turn rate is 0.4. If the frame's creature-in-view indication is set and the current crosshair creature resolves with positive HP, the rate becomes 0.4 multiplied by the greater of 0.3 and one minus that creature's drawn-position distance from the camera divided by 50. The minimum resulting rate is 0.12. The indication and creature identifier have separate producers; they are not one persistent lock record.

When the customization-model panel's own visibility is set, the same cubed horizontal and vertical inputs add to that panel model's yaw and pitch instead of subtracting from the camera angles. Otherwise they alter the desired camera angles. This consumer has no blanket menu-ownership or controller-mode gate. Preserved turn values from a panel-diverted input poll can therefore still reach it; a panel-scroll action is not itself an instruction to clear those values.

The desired camera angles are game-owned double-precision state, with pitch at 0x1D98 and yaw at 0x1DA8. The current camera angles are separate state. A later stage, RVA 0xADCD0, blends current angles toward desired angles and rebuilds the camera basis. Its complete smoothing, focus and collision behavior is outside this contract. The native blend helper at RVA 0xA7A0 is not replaced here with an assumed exponential formula.

Mouse handling at RVA 0x97880 is a separate publisher. In a menu it first updates the interface pointer; if controller-mode indication is set, it returns there. Otherwise it can drag the map or rotate the customization model with the appropriate mouse button. Camera turning is admitted outside menus, or with the left mouse button held over no pointer node. Mouse pitch uses the camera-speed option and inversion option and is clamped between 0 and 180 in that handler. Mouse yaw subtracts horizontal motion. Outside menus that handler does not require controller-mode indication to be clear. These are overlapping publishers of desired camera state, not exclusive ownership implied by the last-device indication.

## Frame target reset and creature selection

RVA 0xA5A50 resets the crosshair creature identifier at game offset 0x1E58 to zero each ordinary update, resets the separate prompt identifier, resets static/ground targets to their absent sentinels, and clears the creature-in-view byte at 0x1E88. The stick pairs and the player's aim vector are not cleared by those target resets. A separately stored highlighted identifier at 0x1E60 is not the crosshair identifier; RVA 0x8AE50 clears it later when execution reaches that stage. No persistent positive highlighted-target producer is established by this bounded work.

The ordinary player-control routine at RVA 0x8AE50 is called from game update only when a world is being played, the player is not in ability 156 or 157, and the start, character-selection and character-creation screens are not visible. This outer admission is not a blanket rejection of all ordinary GUI panels.

The caller builds a candidate list of creatures with positive HP, drawn positions no farther than 160 blocks from the player, and admission by the renderer's sphere-visibility test. It sorts that list by increasing squared distance. The native sorting family begins at RVA 0x7D720; its comparisons, including RVA 0x757F0, use the distance field. No stable creature-ID tie-break is established for equal distances.

RVA 0x8AE50 uses the existing camera basis's forward direction, not a pointer-coordinate ray. Candidate filtering excludes the player, the ridden companion during ability 118, encoded type 285, and type 302 when its interaction state is nonzero. Hostility 8 with encoded type 244 through 247 is also rejected when its interaction state is nonzero, as established by RVA 0x5EE50. During ability 35 before its attack duration ends, hostility values 1 and 7 are rejected.

Timed combat filtering is also applied. When the ability elapsed time is at most its attack duration, the classification at RVA 0x5EEA0 requires damage compatibility at RVA 0x50550; the healing classification at RVA 0x5EED0 instead rejects damage-compatible candidates. The healing classification admits ability IDs 134 and 135. The first classification is false for IDs 51, 52, 80, 109, 111, 112, 113, 114, 115, 134, 135, 141, 142, 143, 144, 146 and 149, and true for the other IDs evaluated by that helper. Full combat-compatibility semantics are outside this target-selection contract.

After those filters, each candidate must pass a second sphere-visibility test using its step-adjusted drawn position. The routine projects the eight corners of a box expanded by one unit in each appearance-size dimension. Only corners with positive view-space depth contribute. It records whether projected corners occur on each side of the horizontal and vertical center axes. All four sides are required to continue as a crosshair candidate. This is a bounding-box straddle test, not a nearest pointer-icon test.

When the four-sided test succeeds, the creature-in-view byte is set before the subsequent line/range test. A failed line/range test therefore does not undo that indication. The tested position is the step-adjusted drawn position, with native range parameter 256 and the candidate supplied as the ignored/target creature to the world test at RVA 0x2B5D90.

For a candidate that passes, the routine projects its drawn position onto the camera-forward ray from the camera position. A projection behind the camera is rejected; zero forward distance is admitted. The crosshair identifier is replaced only by a strictly smaller perpendicular miss distance from that ray. A tie retains the candidate already chosen in iteration order. It does not simply choose the nearest creature in world distance, the largest creature, or the first hostile creature.

A separate forward-distance measure is collected for attack-compatible candidates. Another nearby non-crosshair measure is used for later interaction fallback. These measures must not be conflated with the perpendicular-distance winner.

## Aim-vector publication and target assistance

The initial look point is 100 units forward from the camera focus. A world-ray result, admitted with the native 60-unit query, can shorten that point. Later, if the separate highlighted identifier is absent, an eligible forward attack distance exists, and the player is not in ability 51, a point at that distance from the camera can replace the look point when it lies forward of the player along the viewing direction.

For abilities 31 through 34, an ability-owned target-position record supplies an additional latch: while the previous ability time has not passed the anticipation duration, the current look point is stored there; afterward the stored point is reused. This is a distinct stateful ability exception, not a persistent crosshair creature lock.

Before the normal aim-vector publication, execution can return because a focus widget exists, chat input is active, the gliding-state predicate at RVA 0x5EE70 is active with stamina below 0.001, or ability 125 has elapsed time below 2000 ms. Creature scanning has already occurred before these returns. Thus a freshly selected crosshair identifier does not prove that the player's aim was refreshed in that invocation.

When execution reaches publication, the routine clears player toggle bit 0x0200 and stores the vector from player position to the chosen look point in three single-precision player fields at offsets 0x174, 0x178 and 0x17C. This is a directional displacement, not three Euler angles and not an established unit vector. It does not itself set the face-aim bit 0x0004 in the ordinary publication block.

The identified camera target-assistance branch is gated by a nonzero crosshair creature identifier and movement-pair squared magnitude strictly greater than 0.1. It does not test turning-pair magnitude, controller-mode indication or a newly pressed right-stick input. The target must resolve and have positive HP.

Its predicted target direction begins at target drawn position relative to camera focus, raises the target point by 0.45 times its appearance height, and adds target velocity multiplied by an estimated travel time. Travel time uses player-to-target drawn-position distance divided by 150 for the native ranged-attacking predicate at RVA 0x5ECA0, or 500 otherwise.

With usable horizontal directions, the desired tracking yaw is based on current camera yaw plus the arcsine of the signed cross product of the normalized horizontal camera and predicted-target directions, converted to degrees. A pitch value is calculated when the target direction is nonzero, but this branch publishes only a yaw adjustment. It does not publish that calculated pitch.

The yaw blend uses a time response with rate 0.015 per millisecond, increased to 0.03 when roll time is nonzero, multiplied by exponential attenuation with exponent -20 times the length of the cross product of the normalized full directions. Native exponential calls and their multiplication are visible at RVAs 0x8CAC2 through 0x8CAE5. The resulting weight is supplied to the angular interpolation helper at RVA 0x17840, called at RVA 0x8CAFD; its float result is stored in the double-precision desired-yaw field. No direct player aim-vector write or face-aim-bit write occurs in this assistance branch.

This assistance runs before later static-object and ground-item target replacement. Those later scans can clear the crosshair creature identifier, and a later dead-player check also clears it. If no crosshair creature or static-object target remains, interaction fallback can choose a nearby creature whose hostility is neither 1 nor 7. The full static/ground targeting audit is outside scope. This ordering means the creature used for yaw assistance need not equal the final interaction target.

A timed ability-154 exception later in RVA 0x8AE50 clears face-aim bit 0x0004, then, while the action is within its attack duration and its referenced creature resolves, replaces the aim displacement with that creature's drawn position minus player position and sets the bit again.

After player control, a visible speech panel can independently replace the crosshair identifier with its speaker identifier. If the speaker resolves, game update publishes speaker entity position minus player position to the aim-vector fields and sets face-aim bit 0x0004. The subsequent right-stick slowdown check uses the identifier at that later stage, together with the separately retained creature-in-view indication.

## Established stage order and unresolved boundaries

The original stage order is input publication; frame target reset and candidate collection; player-control selection and any admitted aim/yaw-assistance publication; later interaction replacements; speech override; right-stick camera/model turning and its slowdown; then later camera smoothing and basis rebuilding. The target selection and look-point publication use the camera basis available before that later rebuilding. A new desired-angle write does not directly replace the already used basis within the earlier selection stage.

The evidence establishes game-owned axis and camera state, frame-owned crosshair selection, panel diversion that can preserve turn values, and player-owned aim displacement. It does not establish a persistent controller lock-on mechanism. Character animation and the complete rules that turn the player's rendered body from aim data were not audited; setting desired camera yaw, storing aim displacement and setting a face-aim bit are distinct observable publications.

The broader initialization/reset census, every caller of axis setters, all global active-window-state producers, device calibration outside the native signed samples, every GUI pointer-routing path, network replication authority for aim, and full attack/interaction semantics remain unresolved. None is supplied from reconstruction labels or assumed controller conventions.

FINAL / STABLE / HELD. Read back by the source-side author; further writes are held. Role exposure: no prohibited implementation material was encountered.
