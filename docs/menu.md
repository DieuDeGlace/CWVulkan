# The frontend

CubeVulkan builds the game's front screens itself, from `docs/specs/frontend-title.md`,
`frontend-characters.md` and `frontend-options.md`. Nothing is recorded or replayed: the old CWM1 capture
(`Extract/ui/menu/native.cwm`, `tools/MenuProbe.cpp`, `tools/MenuCapture.cpp`, `tools/ConvertMenuCapture.py`,
`src/NativeMenu.*`) is retired.

Normal start-up opens the title. `--play` goes straight into gameplay.

## Code

- `src/Frontend.h`, `src/Frontend.cpp`: the screens' rules and layout, which need no window:
  - the title camera;
  - the logo's placement;
  - the root and system menus' lines and hover bands;
  - the list's places;
  - the look state;
  - the hair grid;
  - the starting kit and default recipes;
  - the name rule;
  - the options and controls screens.

  `tests/FrontendTests.cpp` checks them.
- `src/FrontendRun.cpp`: runs them over the world, the renderer and the interface. `frontend::run` runs the
  title, character select and creation. `frontend::InGameMenus` runs the system menu and the screens it opens
  over the running game, a frame at a time, drawn with the game's own interface.
- `src/GameInput.*`: the 23 bindings of `controls.cfg` read each frame (keyboard scan codes, mouse buttons) and
  the gamepad's fixed mappings. Gameplay reads every control through it.
- `src/HudFrontend.cpp` (`Hud::drawScreen`): draws a screen. It draws:
  - the widgets' two-pass texts in `resource1.dat`;
  - squares and icons;
  - `gui.plx` templates (`button`, `button2`, `smallbutton`, `edit`, `leftbutton`, `rightbutton`, `upbutton`,
    `downbutton`, `blackwidget`) placed and sized by the Plasma scene.
- `src/GameOptions.*`: `options.cfg` and `controls.cfg` in the game's format, in CubeVulkan's folder. They
  are read at start and written straight back, so a missing or partial file is completed with the defaults.
  `--settings-folder DIR` reads and writes them in another folder (testing).

## The title

The title is the ordinary world (the extracted or generated world, seed 123), with the world's time from
`Save/world.db` (10:00 for a new world).

- **The player.** The last character played (`slot` of `Save/characters.db`) is put on stage, or a default
  "Wollay". It is the game's own native player, and every frame it gets:
  - the eagle-rising ability 0x9d;
  - its flight point and start;
  - the front bit 0x80 of its toggles;
  - the lantern bit by the hour.

  The game's creature update makes it climb and drift north-east on the eagle (`sprites/eagle.cub`, drawn by
  the pose code). Without the game's creatures, a stand-in point drifts the same way.
- **The camera.** The camera is the spec's orbit camera:
  - the yaw target turns 3° a second;
  - the pitch target is 110 and the distance target 12;
  - it is smoothed by `cameraSmoothness`;
  - its look-at point is one block above the player;
  - it is kept out of solid blocks.
- **The logo.** `sprites/cubeworld.cub` is drawn through the renderer's screen-space path (`NATIVE_MENU`) as a
  3D voxel model.
  - Its centre is at (W/2, H × (0.25 + 0.05 cos φ)), with a 10-second wall-clock period.
  - It is tilted 20 − 5 cos φ about x, then turned 180° about y.
  - Its scale is `guiScale` × 0.011 × 1024 / H.
  - It is seen in perspective (a 60° field of view, the middle at the depth where a voxel has that scale), so the
    sides of its voxels show. Its faces are shaded by their direction: the faces toward the viewer lit, the
    sides and the faces turned down darker.
- **The root menu.** It is 200 × 100, at x = (W − 200) / 2 and y = (1.6 H − 100) / 2 (interface units).
  - Its entries are Start Game, Continue (only when `slot` names a character), Options and Exit. They have the
    spec's text heights and hover bands, in font size 18 with a 4-unit outline, hovered (0.2, 1, 1).
  - The copyright line and "1.0.0-1" are in size 12 at the bottom corners.
- **Sounds.** The main theme plays (`Audio::menuMusic`). `menu-open2` plays when the front screens open,
  `menu-close2` when they close, and `menu-select` on the creation arrows.

## Character select and creation

**Character select** lists the characters of `Save/characters.db` and "New character".

- Each entry requests 600 × 220 at (300, 50 + 230 i), keeping the copied artwork's binding
  when its custom widget replaces the template widget. The list centers horizontally and fits
  vertically using the native ordered damping and integer height calculations.
  Left-button dragging uses filtered pointer movement; release resumes fitting and momentum.
  Pointer polling and frame decay retain their separate clocks. Mouse-wheel scrolling is not
  added to this list.
- An entry shows:
  - the character's creature (its class's recording wearing its look), drawn at (100, 110) in the entry, turned
    (−95°, 0°, 45°). While the mouse is over the entry it turns 50° a second and walks on the spot; otherwise it
    eases back to rest;
  - the name in (0.25, 1, 1);
  - "LV n Race Class";
  - "Specialization: …" in (0.5, 0.5, 1).
- The entry under the mouse is selected. It has these buttons:
  - **Select / Create** (`button2`, requested 200 × 30), at the bottom right;
  - **the delete toggle** (`smallbutton`, 20 × 20);
  - **Delete Character** (`button`, 300 × 30), shown right of the entry after the toggle, with no
    confirmation, as in the game.
- Select loads that character and plays. `slot` is written.

These are size requests, subject to the owner's widget minimum and bind constraints. Select/Create
sits 20 physical pixels inside the selected card's right and bottom frame edges, using the current effective
button dimensions. The delete toggle also uses its effective width; Delete Character sits 20 pixels
right of the card. The owner transforms are preserved. Picking uses the copied shapes, inverse
widget deformation and clipping; registered control descendants stay passive, and later controls
win overlapping picks. The three actions retain mouse-release activation and hide while left is held.
Ordinary Buttons initialize their owner release/leave sequences; enter/leave animate from the current
instance values. Size requests are not reapplied every draw. Full GUI capture, cached mask/shine
composition, preview creatures and wider frontend rendering comparisons remain open.

**Character creation** resets the player to a level 1 warrior at the start spot. The camera is at distance 8,
pitch 90, yaw 0.

- **The look panel** is 300 × 330, at the bottom left:
  - five rows with arrows: race (8), gender, class (4), face and haircut (wrapped by the race's counts);
  - the 16 × 7 hair grid. Hovering a cell previews its colour on the player; a click commits it.
- Every change rebuilds:
  - the looks record;
  - the appearance, through the looks routine (`worldgen::makeLooks` with the record's choices);
  - the class's starting kit, with `srand` of the Steam account's low 32 bits and the default recipes.
- **The name field** (`edit`) takes 2 to 15 UTF-16 units. **Create Character** appears with a valid name. It
  writes the character with `num`/`slot`, shows the help sheet, and then the game starts.

## Ring Menu and selected Quick Item

In a native-save session, Ring Menu (Tab by default, or the right-stick button) opens
the owner's `selector` subtree. It lists every nonzero category1 stack in canonical
tab/slot order, including bombs and portal stones. Left/right controls and the left
stick change the retained selection after a neutral input. Forward/back movement
closes the selector. The ordinary gameplay camera remains active.

Quick Item (Q) uses that selection while the selector is shown or hidden. Use (E)
uses it when the selector is shown and no earlier use prompt takes the key.
Both rebuild candidates after preceding item mutations. The portal-stone path
opens the shrine chooser without consuming the item. The quick-button image uses
its original item record through SpriteWidget's3D callback.

Ring models use a separate queued WORLD path before Plasma, with the native50°
overlay projection, retained depth, full allocation centering, resource grouping,
world-clock spin and no added shadow duplicate or spirit-cube pass. Direct widget
icons retain their distinct45° projection and GUI clock. Owner fonts, models and
selector artwork remain external game data.

The independent contracts are [selector input](specs/steam-inventory-slot-lifecycle.md#9-ring-menu-selector-construction-content-and-input)
and [queued rendering](specs/steam-ring-selector-rendering.md). A read-only capture
can be requested with `--play --character slot-0 --no-save --ring-menu --capture ring.png --run-seconds 4`.

## Options, controls, help and the system menu

- **Options.** The options screen (450 × 560) has the spec's 15 rows, values and arrows. Apply and OK write
  `options.cfg` and put the options into effect. Cancel closes it. What each option does:
  - **Mode, Resolution, Refresh Rate**: full screen on the main display at the chosen mode (the desktop's when
    none is chosen), or a window of the chosen size (CubeVulkan's `[Graphics] Width` and `Height` when none is).
    Captures, benchmarks and `--run-seconds` runs keep a window.
  - **VSync**: the swapchain's present mode, taken up when the swapchain is next made.
  - **Render Distance**: the zones drawn are renderDistance × 30 / 100, but 3 below 4. CubeVulkan's view
    distance is 400 blocks at the default 50 % (15 zones) and scales with the zone count (80 to 800 blocks).
    `--set Graphics.ViewDistance=` still overrides it.
  - **Sound FX and Music Volume, Loop Music**: the audio's volumes and loop, at start and when applied.
  - **Camera Speed**: the mouse's turn, scaled by cameraSpeed / 50 (the default keeps CubeVulkan's own speed).
  - **Camera Smoothness**: the gameplay camera's angles and distance follow the mouse smoothed by
    0.005 ^ (cameraSmoothness / 100), as the title camera does.
  - **Invert Y Axis**: the mouse's vertical look turned over.
  - **FPS Limit**: a frame lasts at least minTimeStep milliseconds.
  - **CPU Usage** REDUCED: the spec does not say what the game changes; CubeVulkan gives up the processor for a
    millisecond each frame.
  - Menu.ini's own toggles (shadows, TAA and SMAA, the progression mode) are read from the INI at start as
    before; F7 to F10 still toggle the renderer's.
- **Controls.** The controls screen (500 × 600) has the 23 bindings.
  - A row is 30 units apart and 16 show, scrolled by the up and down buttons.
  - Colours: unassigned (0.2, 1, 0.2), hovered (0.2, 1, 1), waiting for a key (1, 0.35, 0.35) with
    "Press a key".
  - Defaults, OK (writes `controls.cfg`) and Cancel.
- **The keys.**
  - F1 shows the help sheet.
  - F2 and F3 change the interface's scale, F5 and F6 the map's; both are saved.
  - F4 hides the interface.
- **The system menu** (150 × 150; Options, Controls, Start Menu, Exit Game) opens with the System Menu binding
  (O by default; Start on a gamepad) or Escape, once the inventory and the map are closed. The world keeps
  running behind it, as in the game; while it (or a screen it opened) is up, it has the keys and the pointer.
  Start Menu saves the character and returns to the title; Exit Game quits.
- **The controls in play.** Every binding of `controls.cfg` drives gameplay: the movement keys, jump, dodge,
  walk, the attacks, the class and ultimate skills, the quick item, use, the lamp, calling the pet, the world
  map, the inventory, the crafting menu, the system menu and zoom in and out. The gamepad adds the spec's fixed
  mappings: d-pad up Zoom In, d-pad down Zoom Out, Back the Crafting Menu and the Inventory, Start the System
  Menu. Right-stick press opens Ring Menu, and the Multiplayer Menu binding opens the connected-friends screen.
  Remaining controller comparisons are tracked in the roadmap. CubeVulkan's own testing keys moved off the game's: F11 flies,
  Page Up and Page Down change the hour, P is the tap skill, 1 swaps the weapon; any of them gives way to a
  binding on the same key.

## Optional character-creation hair controls

Character creation has an additional dual-tone hair panel. Enable it, choose a second
color from the ordinary palette, and adjust Start and Blend in five-percent steps.
Start places the transition below the roots; Blend controls its width. A zero blend
makes a sharp split. Hovering a color previews it until the pointer leaves; clicking
commits it. These extra controls appear only during creation, and adjusting them does
not regenerate the character's starting equipment.

The gradient uses the owner's loaded hair model and preserves its shaded voxels and
skin colors. The saved style follows the character through selection, local gameplay
and the map avatar. It is stored in CubeVulkan's existing per-profile notes, alongside
the native character row; the native appearance record keeps its original format.
Existing second-color INI settings can supply initial values without a mod DLL.

This is a user-requested engine extension, rather than a claimed Steam UI feature.
The original game has only one hair color in its creature record; synchronization of
the extra style to remote players is not implemented. No hair models, palettes or
other original artwork are included in either distribution package.

## Weapon customization

Native-save sessions use an owner `blackwidget` rebound as a VoxelWidget with an
800 × 800 requested size. Placement, hit testing and the after-children callback
use the actual owner hierarchy. Equipped
thumbnails, the raw editable model, attributes and material palette use the shared
WORLD renderer. Only editable weapon resources retain their raw voxel volume;
ordinary icons continue sharing the mesh cache. No models are distributed.

Bench opening resets selection/carry but preserves hand and rotation. The draw
updates the cached pick/palette state used by later clicks. Picking retains full
signed coordinates; byte placement and full-coordinate removal remain distinct.
Native down handling rejoins customization after ordinary item actions, then runs
the shared health/chat/Ring completion. Wheel messages are cleared before each
gameplay poll so a delivered event is not replayed on idle frames.

Palette information uses the native source priority and hidden crafting-panel
anchor. Inventory/equipment can take precedence, and an empty shown formula panel
blocks the palette fallback. Broader cached inventory state, GUI capture/layers,
full information comparison/map/ground sources and controller aim remain open;
these bounded paths are not a claim of complete native input or pixel parity.

Native item information is positioned using the preceding draw's stored frame.
Its root and children draw before the own callback submits the icon/text and
resizes the widget. That size affects later queries and the following background;
hidden/skipped draws retain it. Recording-only bitmap previews keep their existing
measurement pass. Full category text, comparison deltas and live source bindings
remain separate work.

`--open-upgrade 0` opens this pane for read-only gameplay captures; `--pointer X Y`
also drives its hover/picking. Recording-only sessions keep the compatibility
editor. `EngineTests makesave DIR <recording folder> loot` creates a separate test
profile with generated equipment/materials; use a new directory, not an owner save.

## Inventory wheel input

Inventory wheel input targets the grid under the pointer, in inventory, shop,
crafting priority. Each physical wheel message attempts one row according to its
sign; deltas are not saved up between frames or redirected to the bag merely
because it is open. Controller and button row inputs retain their existing paths.
The full native widget refresh/scrollbar lifecycle remains a roadmap item.

## Placed by CubeVulkan where the specs leave it open

These are stand-ins, to be replaced when the specs or a capture give them:

- **Positions:**
  - the Back button at (20, 20);
  - the look panel's arrows at x 105 and 255, its values centred at 190;
  - the options arrows 100 left and right of the value;
  - the Apply/OK/Cancel and Defaults/OK/Cancel buttons along the widget's bottom;
  - the name field and Create Character counting their negative y from the bottom (the spec's open
    question 4).
- **Templates.** Buttons do not shrink below their `gui.plx` design size, so the spec's 110 × 15 and
  140 × 15 buttons draw larger. A button's clipped highlight is left out: the overlay has no clipping layers.
- **Backgrounds.** The options, controls, look panel and help sheet use `blackwidget`; the list entries use a
  dark square. The spec does not name their templates.
- **The help sheet's layout.** It lists the bindings in two columns.
- **The logo's overlay projection and lighting.** These are the spec's open question 1. The scale is taken as
  clip units of the window height, so the logo is 0.011 × 1024 / 2 pixels a voxel, about 370 pixels wide at any
  window size. The screen x runs right to left (Direct3D's view), because drawn the other way the logo reads
  mirrored. A fixed front light shades it.
- **The start spot.** The spec's search (frontend-characters.md §4.2): an angle and a radius of 100,000 to
  132,767 cells drawn with `rand()`, then x walked a cell at a time, each area's landmark cell tested: object
  0x35, the ground at (16 x + 8, 16 y + 40) not below 0, the region's biome 0, 2 or 7, the cell's place a town
  (kind 5, read as "the zone kind") and flat ground (at most 4 blocks over 16 in x and y). The danger measures
  (open question 3) are not tested. A world kept as extracted zones has ground only around its home: there the
  search starts at the home with no radius and takes only extracted cells; when none passes, the home's cell
  stands in.
- **The list's creatures**: the scale (80 interface units a block), the ease rate toward the walk and back,
  the walking speed (the intent's 8 blocks a second) and the entry's dark square (behind the text only, so the
  creature is not darkened) are CubeVulkan's.
- **The logo's projection and lighting** (above) are the spec's open question 1.
- **Escape** goes back (it also opens the system menu in play). The game's own Escape flag is open
  question 4.
- **Camera speed's scale** (cameraSpeed / 50) and the Zoom In / Zoom Out rate (twice as near or far a second)
  are CubeVulkan's.

## Captures

`CubeVulkan.exe --menu [--menu-page characters|create|options|controls] --capture FILE.png --run-seconds N`
makes a screenshot. Capture runs open the saves read-only and write nothing. `--save-folder DIR` points at
another Save folder and `--settings-folder DIR` at other settings files. `--pointer X Y` holds the mouse there
(the character list's hover). In play, `--open-system-menu` shows the system menu from the start.
`EngineTests makesave DIR <recording folder>` writes a test `characters.db` there, made by the writer from a
recorded character's snapshot.

In play, `--open-map Z` starts the ordinary full map at zoom Z; `--open-shrines Z` starts
its restricted shrine-choice mode for captures. Use `--capture FILE.png --run-seconds N`
and an explicit `--save-folder DIR --character slot-0` to load character0 from that test save read-only. Marker selection still
requires the character's actual known shrine cells; the capture option adds no discoveries.
