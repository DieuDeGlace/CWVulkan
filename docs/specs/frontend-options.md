# Frontend: options, controls, interface keys and the pause (system) menu

Follow-up evidence: `remaining-runtime-gaps.md` §4 supplies the XInput mappings,
stick filters, camera arithmetic and UI event edges left open here. Exact GUI
navigation-point ranking remains unresolved in that independent trace.

Behavioural specification of Steam Cube World's settings and its in-game system menu, written
from the decompiled executable and the binary's constants (routines by RVA, image base
0x140000000). Conventions, GUI units and the text helper are those of `frontend-title.md` §0.
"G" is the game object.

---

## 1. `options.cfg`

### 1.1 Storage and life cycle

* The settings are one block of 21 four-byte values. A static copy is built before `main`
  (0x13a9e0, defaults below), **read** from `options.cfg` in the working directory (the game
  folder) by 0x13aa50 and **immediately written back** by 0x13b1d0 (so a missing or partial
  file is completed with defaults at every start). The game object keeps its own copy at
  G+0x1b88 (copied in by 0x7ea80) and the options screen another (§2).
* **Applying** (0x89c10, from the options screen's Apply and OK): the screen's copy replaces
  G's copy, `options.cfg` is rewritten, and the music volume is set to musicVolume × 0.01 at
  once. The F-key changes of §3 also rewrite the file.
* **Format** (0x13b1d0): one line per setting, `name value`, in this order, ints in decimal,
  the two floats in the C++ stream's default float format:

  `fullscreen`, `resolutionX`, `resolutionY`, `refreshRate`, `antiAliasing`,
  `renderDistance`, `cpuPerformance`, `soundVolume`, `musicVolume`, `cameraSpeed`,
  `cameraSmoothness`, `language`, `invertY`, `minTimeStep`, `smallGUI`, `vsync`,
  `showRarity`, `musicLoop`, `guiScale`, `mapScale`, `antiAliasingSamples`.
* **Reading** (0x13aa50): the file is read word by word. A word that is exactly one of the
  names above is followed by reading its value (an int, or a float for `guiScale` and
  `mapScale`); any other word is skipped, so an unknown name's value is skipped on the next
  turn as a word. Reading stops at the end of the file or at the first failed read.

### 1.2 The settings

| # | Name | Type | Default | Range / meaning | On the options screen |
|---|---|---|---|---|---|
| 0 | fullscreen | int | 1 | 0 windowed, 1 fullscreen | Mode |
| 1 | resolutionX | int | 0 | pixels; 0 = not chosen (desktop size) | Resolution |
| 2 | resolutionY | int | 0 | pixels | Resolution |
| 3 | refreshRate | int | 0 | Hz | Refresh Rate |
| 4 | antiAliasing | int | 0 | on/off | — |
| 5 | renderDistance | int | 50 | 5 … 100, step 5; zones drawn = renderDistance × 30 / 100 (integer), but 3 when that is below 4 | Render Distance, shown with "%" |
| 6 | cpuPerformance | int | 1 | 0 REDUCED, 1 FULL | CPU Usage |
| 7 | soundVolume | int | 100 | 0 … 100, step 10 | Sound FX Volume |
| 8 | musicVolume | int | 100 | 0 … 100, step 10; music volume = value × 0.01 | Music Volume |
| 9 | cameraSpeed | int | 50 | 10 … 100, step 5 | Camera Speed |
| 10 | cameraSmoothness | int | 80 | 0 … 100, step 5; camera smoothing constant = 0.005^(value × 0.01) (`frontend-title.md` §3.3) | Camera Smoothness |
| 11 | invertY | int | 0 | 0 No, 1 Yes | Invert Y Axis |
| 12 | language | int | 0 | an index, not clamped by the arrows | Language (shown as the number) |
| 13 | minTimeStep | int | 0 | 0 … 50 ms; frames are at least this long; 0 = no limit | FPS Limit: "none" for 0, else 1000 / value + " fps" (integer division) |
| 14 | smallGUI | int | 0 | on/off | — |
| 15 | vsync | int | 1 | 0 OFF, 1 ON | VSync |
| 16 | showRarity | int | 1 | 1 "as stars", 0 "color coded" | Rarity |
| 17 | guiScale | float | 1.0 | 0.4096 … 3.0517578 (1.25⁻⁴ … 1.25⁵) | — (F2 / F3) |
| 18 | mapScale | float | 1.0 | 0.7513148 … 1.331 (1.1⁻³ … 1.1³) | — (F5 / F6) |
| 19 | musicLoop | int | 0 | 0 OFF, 1 ON | Loop Music |
| 20 | antiAliasingSamples | int | 8 | sample count | — |

The resolution list offered is built at start-up (0x1355a0) from the display modes Windows
enumerates: only 32-bit modes that pass a test mode change, grouped by width × height, each
with its set of refresh rates.

---

## 2. The options screen

An OptionsWidget (constructor 0x287050, draw hook 0x288c50), GUI[0xa7], **450 × 560**,
centred in the window on every resize (0x101bd0). It is opened from the root menu
(`frontend-title.md` §4.2) or the system menu (§5). On construction it copies G's settings
(0x28bf30) and finds the current resolution in the mode list.

### 2.1 Drawing (every frame)

Font size 12, outline 3, outline colour (0, 0, 0, 1), white. Each row's label is drawn
left-aligned at x = 15 and its value centred at x = (widget width − 250) × 0.5 + 240, at the
row's y:

| Row | y | Value text |
|---|---|---|
| Mode | 26 | "Fullscreen" / "Windowed" |
| Resolution | 56 | width + " x " + height of the selected mode |
| Refresh Rate | 86 | refreshRate + " Hz" |
| VSync | 116 | "ON" / "OFF" |
| Render Distance | 146 | renderDistance + "%" |
| CPU Usage | 176 | "FULL" / "REDUCED" |
| Sound FX Volume | 206 | soundVolume + "%" (all percentages: the number, then "%") |
| Music Volume | 236 | musicVolume + "%" |
| Loop Music | 266 | "ON" / "OFF" |
| Camera Speed | 296 | cameraSpeed + "%" |
| Camera Smoothness | 326 | cameraSmoothness + "%" |
| Invert Y Axis | 356 | "Yes" / "No" |
| FPS Limit | 386 | "none" or N + " fps" |
| Language | 416 | language |
| Rarity | 446 | "as stars" / "color coded" |

Each row has a left and a right arrow ("leftbutton", "rightbutton" from `gui.plx`), drawn in
(0, 1, 1, 1) while the mouse is over it and white otherwise.

### 2.2 The arrows (each a small handler, 0x28aef0 … 0x28bf10)

| Row | Left | Right |
|---|---|---|
| Mode | toggle 0/1 | toggle 0/1 |
| Resolution | mode index − 1 (not below 0) | mode index + 1 (not past the last) |
| Refresh Rate | previous rate of this resolution | next rate of this resolution |
| VSync | toggle | toggle |
| Render Distance | − 5, not below 5 | + 5, not above 100 |
| CPU Usage | set 0 | set 1 |
| Sound FX Volume | − 10, not below 0 | + 10, not above 100 |
| Music Volume | − 10, not below 0 | + 10, not above 100 |
| Loop Music | toggle | toggle |
| Camera Speed | − 5, not below 10 | + 5, not above 100 |
| Camera Smoothness | − 5, not below 0 | + 5, not above 100 |
| Invert Y Axis | toggle | toggle |
| FPS Limit | minTimeStep + 1, not above 50 (fewer fps) | minTimeStep − 1, not below 0 |
| Language | − 1 | + 1 |
| Rarity | toggle | toggle |

Changing the resolution sets resolutionX/Y from the mode and moves refreshRate to the
nearest rate that mode offers (0x28bc70 and its mirror 0x28ba90). Arrows only change the
screen's copy.

### 2.3 Buttons

Three buttons of 110 × 15 under the rows: **Apply** (apply and save, §1.1; the screen stays
open), **OK** (apply, save and close), **Cancel** (close without applying). Closing keeps the
screen's copy as it is; it is not reloaded from G when reopened (open question 2).

---

## 3. Interface keys (key-down handler 0x96e70, Windows virtual-key codes)

| Key | Effect |
|---|---|
| F1 | toggle the help sheet (GUI[0x4f], the controls overview) |
| F2 | guiScale ÷ 1.25, not below 0.4096; re-layout (0x101bd0); save |
| F3 | guiScale × 1.25, not above 3.0517578; re-layout; save |
| F4 | toggle the visibility of the whole interface root |
| F5 | mapScale ÷ 1.1, not below 0.7513148; save |
| F6 | mapScale × 1.1, not above 1.331; save |
| Esc | toggles a flag at G+0x420 (open question 4) |
| Enter | in the chat: opens the input line, or sends it (chat commands are not frontend) |

---

## 4. Controls

### 4.1 `controls.cfg`

23 bindings, each two ints: a **code** and a **kind** (0 = keyboard, the code is a keyboard
scan code; 1 = mouse, the code is the button number 0 left, 1 right, 2 middle, 3 fourth). The
block lives at G+0x1bdc. It is filled with defaults (0x41dd0 → 0x43760), then read from
`controls.cfg` (0x42b30), and written by 0x438e0 when the controls screen's OK applies it
(0x89b80).

Format: one line per binding, `Name code kind` (two decimal ints separated by a space).
Reading: a known name followed by two ints sets that binding; unknown names are skipped.

| # | File name | Screen label | Default (code, kind) |
|---|---|---|---|
| 0 | ButtonAttack | Normal Attack | mouse left (0, 1) |
| 1 | ButtonSpecialAttack | Special Attack | mouse right (1, 1) |
| 2 | ButtonClassSkill | Class Skill | Left Shift (0x2a, 0) |
| 3 | ButtonUltimateSkill | Ultimate Skill | R (0x13, 0) |
| 4 | ButtonQuickItem | Use Quick Item | Q (0x10, 0) |
| 5 | ButtonUp | Move Forward | W (0x11, 0) |
| 6 | ButtonDown | Move Backward | S (0x1f, 0) |
| 7 | ButtonLeft | Move Left | A (0x1e, 0) |
| 8 | ButtonRight | Move Right | D (0x20, 0) |
| 9 | ButtonJump | Jump | Space (0x39, 0) |
| 10 | ButtonDodge | Dodge | mouse middle (2, 1) |
| 11 | ButtonUse | Use/Interact | E (0x12, 0) |
| 12 | ButtonLamp | Lamp | F (0x21, 0) |
| 13 | ButtonWalk | Walk | Left Ctrl (0x1d, 0) |
| 14 | ButtonCallPet | Call Pet | T (0x14, 0) |
| 15 | ButtonWorldMap | World Map | M (0x32, 0) |
| 16 | ButtonRingMenu | Ring Menu | Tab (0x0f, 0) |
| 17 | ButtonInventory | Inventory | B (0x30, 0) |
| 18 | ButtonCraftingMenu | Crafting Menu | C (0x2e, 0) |
| 19 | ButtonSystemMenu | System Menu | O (0x18, 0) |
| 20 | ButtonMultiplayerMenu | Multiplayer Menu | J (0x24, 0) |
| 21 | ButtonZoomIn | Zoom In | unassigned (0, 0) |
| 22 | ButtonZoomOut | Zoom Out | unassigned (0, 0) |

(Key letters are the US-layout keys for those scan codes, for orientation.)

**Evaluation** (the main loop, 0x1355a0, each frame, unless the controls screen is waiting for
a key): for each binding, a keyboard binding with a code from 1 to 255 reads that scan code's
state from the keyboard device; a mouse binding with a code below 4 reads that button; code 0
with kind 0 is unassigned. The XInput gamepad adds fixed mappings seen in the same loop:
d-pad up → Zoom In, d-pad down → Zoom Out, Back → Crafting Menu and Inventory, Start → System
Menu (the rest of the pad mapping is open question 5).

### 4.2 The controls screen

A ControlsWidget (constructor 0x275a70, draw hook 0x276200), GUI[0xa8], **500 × 600**,
centred on resize, opened from the system menu. It keeps its own copy of the 23 bindings.

* **Rows** (0x276200): one per binding, 30 units apart starting at y = 27, scrolled by an
  up button, a down button and a scroll thumb (as many rows as fit in the widget height
  minus 120). Font size 12, outline 3. The label (table above) left-aligned at x = 20; the
  key's name centred at x = (width − 250) × 0.5 + 240. The key's name is "Mouse " + number for
  mouse bindings, "Unassigned" for (0, 0), otherwise the key's name (0x4c830).
* Colours: an unassigned row in (0.2, 1, 0.2, 1); the row under the mouse in (0.2, 1, 1, 1)
  (hover band: from 20 above the row's y to 10 below it, and x left of width − 40); the row
  waiting for a key in (1, 0.35, 0.35, 1), with "Press a key" in place of the key's name.
* **Clicking a row** (mouse press handler 0x965e0 → 0x276f30) makes it the waiting row. The
  next key or mouse button pressed becomes that row's binding (open question 6). Every press
  first clears the waiting state (0x2761f0).
* **Buttons** (140 × 15): **Defaults** (the screen's copy becomes the default bindings),
  **OK** (0x276c80: G's bindings = the screen's copy, `controls.cfg` written, screen closed),
  **Cancel** (screen closed).

---

## 5. The system (pause) menu

A SystemWidget (constructor 0x291c70, draw hook 0x291e00), GUI[0x48], **150 × 150**, centred
on resize.

* **Opening**: the System Menu binding (§4.1, default O; Start on a gamepad) toggles it on the
  press edge (0x103150), but only while no front layer is shown and the chat line is not
  open. When it opens, another in-game panel (GUI[0x43]) is hidden. The world is **not**
  paused; the game keeps running behind the menu.
* **Drawing** (0x291e00): font size 13, outline 3, outline colour (0, 0, 0, 1), centred at
  half the width; hovered entry (0.2, 1, 1, 1), others white; hover needs 0 ≤ x < width.

  | Entry | Index | Text y | Hover band | Navigation point y |
  |---|---|---|---|---|
  | Options | 0 | 30 | 10 ≤ y < 40 | 25 |
  | Controls | 1 | 60 | 40 ≤ y < 70 | 55 |
  | Start Menu | 2 | 90 | 70 ≤ y < 100 | 85 |
  | Exit Game | 3 | 120 | 100 ≤ y < 130 | 115 |

* **Clicking** (the draw hook connects 0x292300 to the widget's press event, id 3): the menu
  hides itself, then by the hovered index:
  * **Options**: toggle the options screen (0x103110).
  * **Controls**: toggle the controls screen (0x102ed0).
  * **Start Menu** (0x9fdf0): save the current character (0xa27b0); 0x911e0 (open question
    7); reload every saved character into the in-memory list (0x9bb20 for each index); show
    the **title layer**; clear G+0x420 and the conversation flag G+0x668; hide the in-game
    windows (inventory, crafting, map, character, multiplayer and similar panels). P is not
    reloaded: the character just played starts riding the eagle from where it stands
    (`frontend-title.md` §3.1).
  * **Exit Game**: set the quit flag G+0x1d68 (0x9f660); the main loop ends and the game
    closes (what is saved on the way out belongs to the shutdown path, not traced here).

---

## Verification

* **Files**: start the game with no `options.cfg` / `controls.cfg` and compare the files it
  writes with §1.2 and §4.1; then edit values out of range and check what the screen shows.
* **Screens**: screenshots of the options, controls and system menus at 1920 × 1080 with
  guiScale 1, compared with the row positions above.
* **Camera smoothness**: with cameraSmoothness 0, 50, 100, record the title camera
  (`frontend-title.md` verification) and check the smoothing constant.

## Open questions

1. **Zones from renderDistance.** The "30 / 100, at least 3" rule is how 0xa5a50 derives the
   draw radius it passes to the renderer; whether the world loader uses the same number was
   not traced.
2. **Cancel and reopening.** The screen's copy is only loaded at construction (0x28bf30);
   whether anything reloads it when the screen is reopened was not found. Oracle: change a
   value, Cancel, reopen, screenshot.
3. **antiAliasing, antiAliasingSamples, smallGUI, language**: not on any screen; their
   effects were not traced.
4. **Esc** toggles G+0x420; its reader (it is cleared by Start Menu and when a conversation
   ends) was not traced. Oracle: press Esc in game and on the title, record G+0x420 and
   screenshots.
5. **The full gamepad mapping** (sticks, A/B/X/Y, triggers) in 0x1355a0 was only partly read.
   Oracle: record the binding states (G+0x1a70 + index) while pressing each pad button.
6. **Key capture.** Where the waiting row receives the pressed key (and whether Escape
   cancels) was not found; the press handler and the key-down handler both skip their
   normal work while a row waits. Oracle: rebind a row to a key, a mouse button and Escape,
   then read `controls.cfg`.
7. **0x911e0** (called by Start Menu before reloading characters) was not read; it may stop
   multiplayer hosting.
8. **The key-name function** (0x4c830) and its table for scan codes (for example whether it
   uses the Windows key-name service) were not read. Oracle: screenshot of the controls
   screen after binding unusual keys.
