# Frontend: the title scene, the root menu, music and screen changes

Behavioural specification of what Steam Cube World shows from start-up until the player is in
the world: the live scene behind the menus, the root ("start") menu, the music and the way the
game moves between its front screens. Written from the decompiled executable and the binary's
constants; routines are cited by RVA (image base 0x140000000). The character screens are in
`frontend-characters.md`, the options, controls and pause screens in `frontend-options.md`.

The interface library is Plasma (`plasma-format-and-scene.md`, `plasma-shapes-and-text.md`);
this document uses its vocabulary (node, live `Display.visibility`, widget, frame).

**The game has no separate title scene.** The scene behind the menus is the ordinary world,
simulated and drawn by the ordinary game loop, with the local player riding an eagle high
above the ground and the gameplay camera slowly circling it. Only a handful of things differ
while a front screen is open; they are listed in §3. An engine therefore builds the title by
running its world with those differences, not by replaying anything.

---

## 0. Conventions

* "G" is the game object (constructor 0x7ea80, per-frame routine 0xa5a50); "W" the world
  (G+0x8); "P" the local player's creature record (W+0x448). Offsets are given only so a
  reader can cross-check a routine.
* **GUI units.** All interface positions and sizes below are in the interface's own units:
  pixels of the window divided by the GUI scale (option `guiScale`, default 1.0,
  `frontend-options.md` §1). The window size in pixels is G+0x1d60 (width) and G+0x1d64
  (height).
* **Shown / hidden** means the live `Display.visibility` of a node (1 or 0). Every front
  screen is a node with no shape of its own (a "layer"); hiding it hides everything under it.
  Showing and hiding are immediate: the front screens have no fade or slide.
* Times are milliseconds. *dt* is the frame's elapsed time in milliseconds as a double,
  capped at 1000 (0xa5a50, first lines).
* Angles are degrees unless stated.

### 0.1 Immediate-mode text

The menus in this document (and the options and controls screens) draw their text every frame
from the widget's draw hook, through a small text helper shared by all "BaseWidget"s
(0x269xxx):

* a font size (0x2691b0), an outline width (0x2691f0), a text colour (0x269160), an
  outline colour (0x2691c0) and an alignment (0x269190) are set, then
* 0x269210 draws a string at (x, y) in the widget's frame (x, y are passed in that order;
  y is the line's reference height used by every caller below). When the outline width is
  above 0 the string is drawn twice, first with the outline, then filled.

Alignment values seen: 0 = left-aligned at x, 1 = centred on x, 2 = right-aligned at x,
5 = centred both ways (used once, §characters). The font is `resource1.dat` (Plasma's
ScalableFont); it has capital letters only, so mixed-case strings appear in capitals.

### 0.2 Navigation points

Each menu entry also registers a point (0xf97e0: the widget's frame origin plus (x, y),
converted to window pixels) in a per-frame list at GUI+0x5b0. The list is rebuilt every frame;
it appears to be what a gamepad cursor snaps between (open question 6).

---

## 1. What exists before the first frame

The game object's constructor (0x7ea80) prepares, in this order, everything the title needs:

1. **The title world.** The world is initialised (0x2b5f20) with **seed 123** and the name
   **"world"** (G+0x206c and G+0x2070 hold the wanted seed and name; W+0x220 and W+0x200 the
   loaded ones). World initialisation sets the time of day to **10:00** (36 000 000 ms,
   W+0xc) and opens `Save/world.db`. The world is generated around the player as usual.
2. Sounds are preloaded (0xa3a40 for ids 0…124; the files are `Sounds\*.wav`).
3. **Characters** are read from `Save/characters.db` (`frontend-characters.md` §1).
4. The interface is built (0xf9ac0): `gui.plx` and `quest-tag.plx` are loaded, then every
   screen node and widget is created. The **title layer is shown**; the character-select,
   character-creation, world-select and world-creation layers are hidden.
5. **A default player** is created: a fresh creature record (0x4ce80) named "Wollay", race
   0x101, class 1 (warrior), level 1, health at its maximum (0x5fac0), position (0, 0, 0)
   (also copied to +0xb10), +0x1b8..+0x1c8 = the world's default point (0x2d3db0: x = y =
   block 33 562 624.5, i.e. 0x20020000000 + 0x8000 in world units, z = the terrain height
   there), orientation and movement flags zero, hostility 0, two iron swords in the weapon
   slots (§frontend-characters 4.1 describes the same reset), its knowledge record
   (0x7ff20) and the race's default looks (0x53e80, no looks record). It is added to the
   world and becomes P. The map knowledge is refreshed (0x2e0b50).
6. **The last character is put on stage.** If `characters.db` has a "slot" entry, its value
   becomes the selected character index (G+0x2264) and that character is loaded **into P**
   (0x9bb20: P becomes a copy of the saved character, including its position, gear, looks and
   knowledge). Then the map knowledge is refreshed again, the default recipes are ensured
   (0x9afb0 with 0, `frontend-characters.md` §5.6), the GUI's character lists are refreshed
   (0x107690) and the inventory screens are rebuilt (0xb2180).
7. **The camera** starts at pitch 90, roll 0, yaw 180 (targets and current values the same;
   §3.3) and distance 8 (target and current).

So the title shows the last-played character (or Wollay when there is none), standing where it
was saved, but in the seed-123 world named "world" (the character's own world is chosen only
through the world screens, `frontend-characters.md` §8).

---

## 2. The front screens

| Layer (GUI index) | Node | Shown at start | Contents |
|---|---|---|---|
| title | GUI[0], G+0x118 | yes | the root-menu node GUI[1] with the root-menu widget GUI[2] (§4) |
| character select | GUI[3], G+0x130 | no | the character list, Select/Create, delete (`frontend-characters.md` §2) |
| character creation | GUI[0xb], G+0x170 | no | the look panel, the name field, Create Character |
| world select | GUI[0x10], G+0x198 | no | the world list, Select, delete, Multiplayer Worlds… |
| world creation | GUI[0x1d], G+0x200 | no | name and seed fields, Create World |

A **Back** button (GUI[0x68], a `gui.plx` "button", 200 × 40, label "Back", click handler
0x9ddb0) is shown whenever one of the last four layers is shown (0x103200).

Two predicates drive the scene differences:

* **Riding** (used for the eagle and the camera): the title layer or the character-select
  layer is shown.
* **Front** (used for the lantern, the hidden HUD, the music and the menu sounds): the title,
  character-select or character-creation layer is shown.

---

## 3. The title scene

### 3.1 The player on the eagle (every frame, in 0xa5a50 before the world step)

* While **riding**: P's current ability (+0x68) is set to **0x9d**, the "rising on the eagle"
  ability; P's +0x1b0 / +0x1b4 are set to P's block x and y (position ÷ 65536, rounded toward
  zero) and +0x1b8..+0x1c8 to P's position (these are the start and destination of an eagle
  flight; on the title they only follow P); the camera targets are set as in §3.3.
* While not riding, if P's ability is 0x9d it is set to 0 (P then falls and lands normally).
* Ability 0x9d is specified in `creature-abilities-ranged.md` §13.7: P climbs (smoothly) to at
  least 150 blocks above the higher of the land and the terrain, its velocity is (10, 5, 0)
  blocks per second and its movement intent circles with period 2π s. **This is the
  "flyover": P drifts north-east at about 11.2 blocks per second at a constant height above
  the ground, from wherever it was standing.** There is no scripted route.
* The eagle itself is the model drawn under any creature whose ability is 0x9c or 0x9d:
  `sprites/eagle.cub` (sprite slot 0xe35), placed as in `creature-pose-items.md` §5.2. The
  rider's pose for 0x9d comes from `creature-pose.md`. The rider wears its own gear and looks.
* P's state flags (+0x128): bit 0x80 is set while **front** and cleared otherwise; bit 0x1
  (the lantern) is, while **front** (and also during an eagle flight, ability 0x9c), set when
  the time of day is before 05:00 (18 000 000 ms) or at/after 19:00 (68 400 000 ms) and
  cleared otherwise. (Otherwise bit 0x1 is the player's own lantern toggle.) What bit 0x80
  does is open question 2.
* If P's health is 0 or below, its ability is set to 0.

The rest of the frame is ordinary: the world is stepped (0x2da6c0, in steps, only while the
loaded world's seed and name equal the wanted ones), P's physics, abilities and upkeep run, the
other creatures live. Time of day therefore runs from 10:00 at the normal rate.

### 3.2 What the renderer is told

The world is drawn by 0xe2e00. Two flags of its last argument matter here (0xa5a50, just
before the call):

* **0x20** ("no HUD", also set for other reasons): set while **front**. The HUD (life bars,
  hot bar, …) is not drawn.
* **0x40** ("logo"): set while the **title layer** is shown. The logo is drawn (§3.4).

Sky, sun, fog, clouds and lighting are the ordinary ones for the current time of day and
place (`alpha-rendering.md` for CubeVulkan's default look, its "Steam differences" section for
the Steam look). Nothing about them is title-specific.

### 3.3 The camera (0xadcd0, every frame)

The gameplay camera is an orbit camera with three target angles and current angles (G+0x1d98,
+0x1da0, +0x1da8 targets: pitch, roll, yaw; G+0x1d80, +0x1d88, +0x1d90 current), a target
distance G+0x1db8 and a current distance G+0x1db4, all in degrees and blocks.

**Title targets.** While **riding**, every frame before the camera update:

* yaw target = yaw target + *dt* × 0.003 (double): **3 degrees per second**, never wrapped;
* pitch target = 110;
* distance target = 12.

Roll is not touched (it stays 0). Mouse or stick look input, scaled by the camera-speed option,
is also added to the targets every frame unless the world map is open (0xa5a50, after the
camera block); on the title this input is normally zero (open question 3).

**Smoothing.** With *k* = 0.005 raised to (cameraSmoothness × 0.01) (option, default 80;
`frontend-options.md`), each current angle moves toward its target by the factor
s = smooth(*dt*, *k*) (0xa7a0, specified in `creature-upkeep.md` §4.9): current = current +
(target − current) × s, in double. The current distance does the same in single precision.

**Look-at point.** For ability 0x9d the look-at point (G+0x1dd8..+0x1de8) is P's position with
z raised by one block (65536 units), taken directly with no smoothing. (Other abilities use
other rules in 0xadcd0; they are gameplay, not title, behaviour.) If the look-at point ends
more than 50 blocks (squared distance 2500) from P, it snaps to P's position.

**Eye.** With p, r, y the current pitch, roll and yaw in radians (degrees × 0.0174533), the
forward vector is

  f = (cos y · sin r · cos p + sin y · sin p,  sin y · sin r · cos p − cos y · sin p,  cos p · cos r)

and the eye is look-at − distance × 65536 × f (each axis truncated to an integer). With roll 0
this is: pitch is measured from straight up (90 = level, 110 = looking 20° down), yaw 0 puts
the eye north of the target (+y) looking south, yaw 180 south of it looking north, and the eye
turns counter-clockwise seen from above as yaw grows. The game then checks the eight corners
of a small box around the eye against solid blocks and, if any is inside, shortens the
distance by casting from the look-at point (0x2a0d40); this keeps the eye out of terrain and
is part of the gameplay camera.

### 3.4 The logo (0xe2e00, flag 0x40)

The logo is the model `sprites/cubeworld.cub` (sprite slot **0xc31**), drawn as a screen-space
model by the overlay routine 0xee5a0. With W, H the window size in pixels, *g* the GUI scale
and φ = 2π × ((wall-clock milliseconds mod 10000) × 0.0001) (`timeGetTime`, so the motion
does not depend on the game's clock):

* **centre**: x = W × 0.5, y = H × (0.25 + 0.05 × cos φ) (screen pixels, y down): the logo
  bobs 5 % of the screen height up and down every 10 seconds;
* **rotation**: about x by 20 − 5 × cos φ degrees, then about y by 180, then about z by 0
  (applied in that order to the model's points, after centring the model on its own middle:
  the model is first moved by −½ of its size along each axis);
* **scale**: *g* × 0.011 × 1024 ÷ H (one uniform factor; see open question 1 for how the
  overlay maps it to pixels);
* **colour**: (1, 1, 1, 1).

The logo is not drawn while any other front screen is open.

---

## 4. The root menu

The root menu is a StartMenuWidget (constructor 0x2910c0, draw hook 0x291210), GUI[2],
attached to the node GUI[1] inside the title layer, sized **200 × 100**. On every window
resize (0x101bd0) the node is placed at x = (W − w) × 0.5, y = (1.6 × H − h) × 0.5 (W, H the
window in GUI units, w, h the widget's size): horizontally centred, its middle at 80 % of the
screen height.

### 4.1 Drawing (0x291210, every frame)

Text: font size 18, outline width 4, outline colour (0, 0, 0, 1), alignment 1 (centred on
x = half the widget's width). An entry is **hovered** when the mouse, taken into the widget's
frame (0x32b180), has 0 ≤ x < widget width and y in the entry's band. The hovered entry is
drawn in (0.2, 1, 1, 1) and the others in (1, 1, 1, 1). The hovered index is stored (−1 when
none) for the click.

| Entry | Index | Shown | Text y | Hover band (y) | Navigation point y |
|---|---|---|---|---|---|
| Start Game | 0 | always | 30 | 10 ≤ y < 40 | 25 |
| Continue | 3 | only if the selected index G+0x2264 names an existing character | 80 | 60 ≤ y < 90 | 75 |
| Options | 1 | always | c | c − 20 ≤ y < c + 10 | c − 5 |
| Exit | 2 | always | c + 50 | c + 30 ≤ y < c + 60 | c + 45 |

c = 130 when Continue is shown, else 80.

Then, in font size 12, outline 3, white:

* "Copyright (c) 2019 Picroma e.K." left-aligned at the window point (10, H − 15)
  (window pixels converted into the widget's frame);
* the version "1.0.0-1" right-aligned at the window point (W − 10, H − 15).

### 4.2 Clicking (0x965e0, a press of mouse button 0)

Only while the title layer and the root-menu node are both shown, and only for the hovered
entry:

* **Start Game** (0): hide the title layer, show the character-select layer.
* **Options** (1): show the options widget (GUI[0xa7]); the title layer stays shown under it.
* **Exit** (2): set the quit flag (G+0x1d68); the main loop ends and the game closes.
* **Continue** (3): load the selected character into P (0x9bb20), hide the title layer, set
  the camera distance target and current value to 8, refresh the map knowledge (0x2e0b50).
  The player is then simply in the world. The load resets and rewrites the whole record,
  position, velocity and ability included; see §6.1 (erratum of 2026-10-28).

---

## 5. Music and menu sounds (0xa5a50)

### 5.1 Music

Each frame the game picks one track name, in this order of precedence (later wins): the
biome / place / faction track, the spirit-world track, the boss track, the success jingle, and
last, while **front**, **`maintheme.ogg`**. So on every front screen the main theme plays,
whatever is around P. When the chosen name differs from the playing one, a short switch delay
(500 ms) is set. The file is `Music\maintheme.ogg` in the game folder.

When the music stream is idle and the chosen name differs from the last one, the stream is
replaced by `Music\` + name and its volume set to musicVolume × 0.01 (musicVolume 0…100,
`frontend-options.md` §1). While a track plays, its volume is eased toward musicVolume × 0.01.
The rest of the scheduler (pauses between tracks, the effect of `musicLoop`, fades) is
gameplay behaviour and is left to open question 7.

### 5.2 Menu sounds

Each frame, "a menu is open" = 0x1014f0 says some screen is open (this includes every front
layer, the options and controls screens and the in-game menus) or the GUI[0x40] panel is
shown. When this changes from closed to open the sound **0x56 `menu-open2.wav`** plays, and
from open to closed **0x57 `menu-close2.wav`**, at the camera position, volume 1. The
character-creation arrows play **0x58 `menu-select.wav`** (`frontend-characters.md` §5).
Hovering the root-menu entries plays nothing.

---

## 6. Transitions

| From | Action | Result |
|---|---|---|
| start-up | — | title layer shown; P rides; logo; main theme |
| title | Start Game | character select shown (P keeps riding, logo hidden) |
| title | Continue | all front layers hidden, P loaded from the selected slot, camera distance 8 |
| title | Options | options screen over the title |
| title | Exit | the game closes |
| character select | Back | title shown |
| character select | Select (existing) | `frontend-characters.md` §3: in the world |
| character select | Create (the "New character" entry) | creation shown, P reset and moved to a start spot, camera 8 / pitch 90 / yaw 0 |
| creation | Back | character select shown |
| creation | Create Character | creation hidden, help sheet shown, in the world |
| world select | Back | character select shown |
| world creation | Back | world select shown |
| in the world | system menu → Start Menu | progress saved, characters reloaded, title shown (`frontend-options.md` §5) |

Back is one handler (0x9ddb0) that checks, in this order: world creation shown → hide it, show
world select; else world select shown → hide it, show character select; else creation shown →
hide it, show character select; else character select shown → hide it, show title.

When P leaves a front screen the eagle ability is dropped (§3.1), so a character selected
while riding falls from its saved position only if that position is in the air; normally the
loaded record puts it back where it was saved.

> **Erratum (2026-10-28).** The paragraph above and §4.2 are right but too short: they read as
> if leaving the title only ends ability 0x9d, and an engine built that way drops the player out
> of the sky from wherever the flyover had carried it. What puts the player back on the ground is
> the **reload of the whole record** by 0x9bb20, which happens *before* the eagle is dropped and
> replaces the flyover's position, velocity and ability. §6.1 states it in full.

### 6.1 What Continue and Select do to the player

Continue (0x965e0, entry 3) and Select on an existing character (0x9f7c0,
`frontend-characters.md` §3) do the same thing to P; they differ only in the screen and camera
bookkeeping listed at the end.

1. **The record is reloaded from the save, in place, during the click** (0x9bb20 with the
   selected index G+0x2264 for Continue, the list cursor for Select; under the game lock and the
   world lock). 0x9bb20 inflates blob `<index>` of `characters.db` and runs the reader 0x147800
   (`native-saves.md` §3), which first **resets** P and then reads the saved fields:
   * reset (`native-saves.md` §3.2): 0x64a90 empties P's lists and maps and zeroes its timers;
     the whole main block from +0x10 is overwritten with a freshly constructed creature's
     (0x4d300 → 0x3b7a0). So the **velocity, acceleration and knock-back are zero, the current
     ability (+0x68) is 0 and its clock (+0x6c) is 0, the physics flags are the defaults, and the
     eagle-flight fields +0x1b0..+0x1c8 that the title kept rewriting are cleared**; hostility
     is 0;
   * then the saved **position** (+0x10..+0x20, 64-bit, exactly as saved), health, level,
     experience, class, gear, inventory, knowledge, home cell, buffs … are read in;
   * the saved **orientation** (+0x28..+0x30) is read and then **set to 0** (yaw 0, pitch 0,
     roll 0): the character always faces yaw 0 after loading;
   * not saved, so at their defaults: mana, stamina, block power, cooldowns, abilities, the
     cursor item, the death position (`native-saves.md` §3.3, last paragraph).
   Then 0x9bb20 makes sure the recipe {1, 7} is known. There is no velocity or flight state to
   clear afterwards: none survives the reset.
2. **No safe-ground search.** Nothing tests or moves the loaded position: no ground probe, no
   raise, no teleport to a town. The player is wherever the save says, which is where it stood
   when last saved: the 60-second autosave and the save on "Start Menu" run only while no front
   screen is open (`native-saves.md` §4.5), so a save never records a flyover position.
3. **The front layer is hidden** (Continue: the title layer; Select: the character-select
   layer). From the next frame P is no longer **riding** (§2), so 0xa5a50 stops forcing ability
   0x9d; the "if 0x9d set to 0" branch of §3.1 has nothing to do, the reload already made it 0.
   The lantern bit and bit 0x80 of +0x128 return to the player's own (§3.1).
4. **Zones are not waited for.** The game does not hold P while the zones around the restored
   position load (the flyover may have carried the world's loaded area hundreds of blocks away:
   11.2 blocks a second). It does not need to: a block looked up in a missing zone is **solid**
   (`creature-physics.md` §2.3), so P stands on (or inside) the not-yet-loaded ground and cannot
   fall; the unstick step 0x2da1e0 does nothing while any column of P's box is unloaded
   (`creature-physics.md` §7.1), and once the zone arrives it moves P sideways by 1–2 blocks, or up
   by 1 block per step, if P overlaps a solid block. The zone thread follows P's position
   (how it re-centres is `frontend-title.md` open question 5).
5. The map knowledge is refreshed (0x2e0b50).

Bookkeeping that differs:

| | Continue (0x965e0) | Select (0x9f7c0) |
|---|---|---|
| index used | G+0x2264 (already the `slot` read at start-up) | the list cursor, stored into G+0x2264 |
| `slot` written | no | yes |
| GUI lists (0x107690), inventory screens (0xb2180) | not refreshed | refreshed |
| camera | distance 8 (target and current); **angles kept** from the title's circling | distance 8, pitch 90, roll 0, yaw 180 (targets and current) |

So the first frame in play shows the saved character standing at its saved spot, facing yaw 0,
motionless, with the camera at distance 8.

**For an engine:** on Continue or Select, (1) rebuild the player's record from the save, the
position included, with every non-saved field at its constructor default (velocity, ability,
ability time, flight fields, physics flags); (2) set yaw to 0; (3) only then let the frame logic
see that no riding layer is shown. Ending 0x9d on the flyover record is not a substitute. While
the restored position's zones are missing, treat their blocks as solid rather than as air.

---

## 7. What an engine needs from the game's files

* Models: `sprites/cubeworld.cub` (logo), `sprites/eagle.cub` (the mount). The sprite slots
  quoted (0xc31, 0xe35) are the game's numbering of its sprite list (ZoneExtractor writes the
  slot → file table).
* Font: `resource1.dat` (Plasma ScalableFont). Interface templates: `gui.plx`
  ("button", "button2", "smallbutton", "edit", "leftbutton", "rightbutton", …).
* Music: `Music\maintheme.ogg`. Sounds: `Sounds\menu-open2.wav`, `menu-close2.wav`,
  `menu-select.wav`.
* Saves: `Save/characters.db`, `Save/world.db`; settings: `options.cfg`, `controls.cfg`.

---

## Verification

Recordings to compare with (all with the ZoneExtractor DLL; "oracle request" means a new
`Extract\oracle.request` command that dumps the named fields once per tick for N ticks):

* **Camera on the title**: G+0x1d80..+0x1db8, the look-at and eye (G+0x1dc0..+0x1de8), *dt*,
  and the cameraSmoothness option, for 600 ticks from start-up. Expect yaw target + 0.003 ×
  *dt* per tick, pitch target 110, distance target 12, and the eye formula of §3.3.
* **The rider**: P's +0x10..+0x20, +0x34..+0x48, +0x68, +0x6c, +0x128 per tick on the title;
  expect ability 0x9d, velocity (10, 5, 0), lantern bit by time of day.
* **The logo**: the existing draw recorder (`MenuProbe` / the draw recording of ZoneExtractor)
  with the world matrix of the draw of slot 0xc31 at two window sizes; check centre, the
  10-second bob and the angles.
* **Start-up state**: W+0xc and W+0x220 right after start (expect 10:00 and 123), P's name and
  race with and without a `characters.db`.

## Open questions

1. **Logo scale in pixels.** 0xee5a0 receives *g* × 0.011 × 1024 ÷ H; how the overlay pass
   projects these screen-space models (and so the logo's size in pixels) was not traced.
   (Since traced in `hud-money.md` §3: a 50° perspective camera with the model one unit
   away, which gives ≈ 12.08 × *g* pixels per logo voxel; still to be checked on screen.)
   Oracle: the logo's drawn bounding box at 1280 × 720 and 1920 × 1080 with guiScale 1.
2. **P+0x128 bit 0x80** (set while front): its reader was not traced (candidates: the HUD,
   the nameplate, targeting). Oracle: compare a front-screen frame with the bit forced clear.
3. **Look input on the title.** G+0x1d74 / +0x1d78 feed the camera targets every frame; whether
   mouse motion with the cursor shown can turn the title camera was not checked. Oracle:
   record G+0x1d74, +0x1d78 while moving the mouse on the title.
4. **Version string.** The digits come from constants in 0x291210 (1, 0, 0 and 1, with "." and
   "-"); whether a build number elsewhere replaces them was not checked. Oracle: screenshot.
5. **The world loader.** How the zone thread reacts when the wanted seed differs from the
   loaded one (0x2d2c00, 0x3d820) is outside this document.
6. **Navigation points** (§0.2) are assumed to serve a gamepad pointer; their reader was not
   traced. Oracle: move a gamepad stick on the title and record the cursor.
7. **Music scheduler.** The gaps between tracks, the fade rates (0.001, 0.005 and 0.015 per
   step appear in 0xa5a50) and what `musicLoop` changes were not specified. Oracle: record the
   music stream name and volume (G+0x2028, G+0x2048) per tick across a title → world →
   title round trip, with musicLoop 0 and 1.
