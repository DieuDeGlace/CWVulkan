# Steam Game update phase 69 (reward popup 0xA1320) and music order relative to present

Status: source-side behavioural contract, code-free. Date: 2026-11-12.
Scope: the supported Steam release only. Answers (1) every writer of the queue G+0x2250, (2) the popup node G+0x3B8 (construction, texts, "shine", show/hide), (3) the parameters of sound 0x21, (4) the music update's order relative to the renderer present and the timers G+0x2050 / G+0x204C. Conditional statements are source conditions, not runtime observations. Nothing here is implementation advice.

## 0. Authority, method, exposure

* Analysis image: C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe, SHA-256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244, image base 0x140000000. All addresses are RVAs of this image.
* Installed retail executable C:/Program Files (x86)/Steam/steamapps/common/Cube World/cubeworld.exe was hashed only: 5,939,072 bytes, SHA-256 A1EFBBD41EEB8DF08395C4D6511194D416CB70A14054B2A2000B9F5CDBCFA65B (same identity as the held phase-schedule contract).
* Evidence: the whole-image instruction listing, the call-reference table, the per-function presentation, string and string-reference tables of the original export, raw constant reads from the analysis image, and a string census (no structural decode) of the shipped GUI asset quest-tag.plx under the decompiled release's game-files directory (SHA-256 EC56CDF49173A822EF035C3C4D46B13D6E7E63F59244D38309332696FEA2EAE7). No executable was run.
* Held contracts read: docs/CLEANROOM_AUTHOR.md, the complete-phase-schedule contract (2026-11-12) and the sound contract (sound.md, sections 1.3–2.1 only). The first-frame HUD contract was not needed.
* Infinite-Cubes reconstruction: consulted as leads only (its game header, game source around the popup, interface source lines naming the node, and its audio interface declaration). Every fact below was confirmed against Steam evidence unless marked "lead only".
* The coordinating harness injected the repository's top-level instruction text and branch name into this context at start without a tool read (workflow policy only). A scratch directory listing showed other sessions' file names; none of those files was opened. No engine, test, log, capture or runtime information was read or received.

Terminology follows the phase-schedule contract: G = Game object (global 0x551A80); I = embedded Interface at G+0x118; P = local creature; "shown" = the current element of a node's Display visibility property is nonzero.

## 1. Queue G+0x2250: writers (question 1)

### 1.1 Container and entry layout

* G+0x2250 is a doubly linked list head pointer with its element count at G+0x2258. The Game constructor 0x7EA80 creates its empty sentinel at 0x7F1C2–0x7F1E2 (node allocator 0x872C0, node size 0x58 bytes = two link words + a 0x48-byte entry).
* Entry layout (from the copy routine 0x808F0 and the entry destructor 0x82760, both used by 0xA1320):
  * +0x00: one 32-bit integer (with 4 bytes padding). 0xA1320 copies it but never reads it.
  * +0x08: wide string (32-byte small-buffer string object) — the "info" text.
  * +0x28: wide string (32-byte string object) — the "xp" text.
* The popup consumes from the front (head's next node).

### 1.2 Census of every reference

Every instruction in the image whose operand is G+0x2248..G+0x2258 by any base register, plus the presentation's word-indexed form (index 0x44A/0x44B), was enumerated:

| Routine | Access | Meaning |
| --- | --- | --- |
| 0x7EA80 (Game constructor) | 0x7F1C2–0x7F1E2 | Creates the empty list. |
| 0x818D0 (Game destructor; it also joins threads and closes the socket) | 0x81D01–0x81D77 | Frees every remaining entry (0x82760 then free) and the sentinel; zeroes the count. |
| 0xA1320 (reward popup) | 0xA1373–0xA13B8 | Tests the count, copies the front entry, unlinks it, decrements the count, destroys it. |

The remaining hits are unrelated: 0x17AB0 and 0x53E80 address their own stack frames (RBP-relative locals), 0x177F50 and 0x2C5250 address RSP-relative locals, 0x2FF6F0 writes a stack dword, and 0x173B10 returns element 0x44A of an unrelated 8-byte-element vector (bounds-checked index read, not G).

Supporting checks: the entry copy routine 0x808F0 has exactly one caller (0xA1320 at 0xA1393); the node allocator 0x872C0 has exactly one caller (the constructor); a stand-alone list-clear routine of this element type, 0x81170, has no direct caller in the call table (role unresolved; possibly reached by a computed or unwind path).

### 1.3 Result

* **No writer exists.** Nothing in the analysed image pushes an entry into G+0x2250 through a direct offset; no mission/quest completion, boss kill, artifact, level-up or network path feeds it. The queue is created empty, only ever popped, and freed at destruction.
* Consequently, in vanilla single-player Steam play (and in multiplayer client play) the queue is always empty; phase 69 performs only its visibility read and count test each frame and never fills, animates or sounds the popup. The node G+0x3B8 is never made visible through this path.
* Reward-like events that do exist route elsewhere and do not touch G+0x2250: XP, gold, platinum and level notices (schedule phases 89–92, floating texts and chat lines), narration entries through 0x1694E0, and the "success" music timer G+0x2050 (section 4.3).
* Lead only (consistent with the above): the reconstruction labels this list as a mission-notice queue that is never filled.
* Limit: writers that reach the list through a pointer computed from another base (for example G+0x2240 plus 0x10 in a register) or through external injection were not excluded beyond the operand census and the single-caller facts above.

## 2. The popup node G+0x3B8 (question 2)

### 2.1 Construction

* Built in the Interface constructor 0xF9AC0 at 0xFEA2D–0xFEA7F, stored at I+0x2A0 (= G+0x3B8):
  1. The node is found by name **"missiontag-white"** (wide string at 0x4413D0) by depth-first name search 0x332190 in the Interface's loaded GUI container. That container (created at 0xFB524) receives two GUI files loaded at 0xFB59F and 0xFB613: **"gui.plx"** (0x440AA8) and **"quest-tag.plx"** (0x440AB8), both with load flags 0x20. The name "missiontag-white" occurs in quest-tag.plx.
  2. Its parent is set (0x3347F0) to the node at I+0x108 (= G+0x220).
  3. Its visibility is set to zero (0xA3A20 with 0) — hidden at construction.
* No widget, font, colour or texture of the popup is created or set in code. Its appearance — child widgets, their layout, fonts, colours, textures, and the "shine" animation's keys and duration — is authored data inside quest-tag.plx.
* What the asset census shows (string table only; the PlasmaGraphics binary structure was not decoded): the file contains tags "missiontag-white", "missiontag-green", "missiontag-blue", "missiontag-purple", "missiontag-yellow", "smallmissiontag", "smallquesttag", "questtag", "questbar"; an animation name "shine" immediately follows "missiontag-white"; text nodes named "info" carry the placeholder text "you defeated the saurian"; nodes named "xp" carry the placeholder "xP +10" (these appear after "missiontag-yellow" in file order); text resources reference "resource1.dat" (font resource name). **Unresolved:** which subtree each "info"/"xp" node belongs to, all geometry, colours, textures, fonts, and the "shine" key data and duration. These require decoding quest-tag.plx's structure.

### 2.2 Layout

* Interface layout routine 0x101BD0 (arguments: Interface, width, height; called from 0x96E70 and 0xA2720) places the node at 0x10247D–0x102492 through 0x334700 with four binary32 values: first pair (width/2, height) — width halved by signed integer division toward zero, then converted to binary32; height converted to binary32 — and second pair (0.0, −300.0; 0xC3960000 read from 0x42E8B4). The first pair is the screen-space anchor (bottom centre); the second pair is passed to 0x334510 alongside it. Exact semantics of the second pair (offset versus pivot) were not traced; lead only: the reconstruction treats it as an offset of −300 in y, i.e. 300 pixels above the bottom edge.

### 2.3 What 0xA1320 does when it pops (inert in vanilla)

Gate (0xA1350–0xA137B): the current element of the node's Display visibility (property object at node+0x68, element array +0xE0 indexed by +0xAC) is zero **and** the count G+0x2258 is nonzero. Then, in order:

1. Copy the front entry, unlink and destroy the list node, decrement the count.
2. Set text "info" (0x436390, 4 characters) to the entry's +0x08 string, recursive flag 1 (0x3348E0 at 0xA1409).
3. Set text "xp" (0x43639C, 2 characters) to the entry's +0x28 string, recursive flag 1 (0xA1462).
4. Start the animation named **"shine"** (0x4363A8) at time 0 (0x3342A0 at 0xA14B7).
5. Play sound 0x21 at the camera eye (section 3).
6. Destroy the local copy.

Text formatting: the strings are copied verbatim; 0xA1320 adds no prefix, number formatting or colour. Since no producer exists, there is no Steam-defined format for either string (unresolved by construction).

Text-set semantics (0x3348E0 / 0x334C50): a depth-first search from the popup for nodes whose name equals the key, skipping nodes whose flag bit 0x4 at +0x80 is set; a matching node does not search its own children for further name matches, a non-matching node recurses. With flag 1, the text is written into the matched node and all its unflagged descendants that are text widgets (widget type 3 at +0x70 of the +0x58 object), only when it differs. If no node of that name exists, nothing happens.

### 2.4 The "shine" property: it is an animation start, not a clear

Correction to the schedule's wording "clears its shine property": 0x3342A0 with value 0 **starts the animation named "shine" from time 0**. For each of the node's three animation tracks (+0x58, +0x60, +0x68) that exists, 0x369030 stores the name and, for every animated property of that track that has a key sequence named "shine", restarts it at time 0 (0x34AF80) and sets the track's playing byte (+0x48) to 1. It then recurses into every child node. The 0 is a start time, not a value.

### 2.5 Show, animate, hide

* Visibility is driven only by the animation: near the end of the Interface update 0x103200 (0x106B7C–0x106B92), every frame, the node's visibility is set to "some animation is playing" (0x333ED0: any of its three tracks has the playing byte set, or any child without flag bit 0x4 is recursively playing).
* Within one Game update the order is: phase 69 (pop, start "shine") → phase 82 Interface update (node becomes shown because the animation is playing) → scene/GUI drawing. The node stays shown while the "shine" animation plays and becomes hidden at the first Interface update after it ends; on the following frame's phase 69 the next entry could be popped (at most one pop per frame).
* No input dismisses it, no timer in code hides it, and no code fade exists; any fade, scale or movement and its duration are the "shine" key data of quest-tag.plx (unresolved, section 2.1). The routine that advances GUI animation time and clears the playing byte was not traced.
* No branch in the traced tail of 0x103200 skips 0x106B7C; early returns of 0x103200 as a whole were not exhaustively audited.

## 3. Sound 0x21 (question 3)

Call at 0xA14D7–0xA14F9: play routine 0xA0420 with number 0x21, position = the camera eye G+0x1DC0..G+0x1DD0 (three signed 64-bit fixed-point coordinates, 1/65536 block units), volume factor binary32 1.0 (0x3F800000 from 0x4AC980, in the fourth argument register) and pitch binary32 1.0 (same constant, fifth argument). The caller also stores a byte 1 as a sixth argument; 0xA0420 never reads it (it passes its own 0 to the engine).

In 0xA0420 (consistent with held sound.md §2):

* File: number 0x21 maps (0xA3A40) to **"Sounds\\new-quest.wav"**.
* Distance gate: d² = sum of squared (P − eye) per axis × 1/65536 (binary32 1.52587891e-05 at 0x42E510); dropped if d² > 4096.0 (0x438000). At the eye d² = 0, so it always passes.
* Attenuation a = 1.0 − d² × 0.000244140625 (0x437F64) = 1.0 at the eye.
* Volume passed to the engine = binary32(soundVolume integer G+0x1BA4) × (1.0 × a) × 0.01 (0x42E51C), i.e. soundVolume × 0.01 at the eye, computed in binary32 in that order (volume factor × attenuation first).
* Pan: the position is transformed by the 4×4 matrix at G+0x1EE4..G+0x1F20 (lateral = elements 0, 4, 8, 12; w = elements 3, 7, 11, 15), pan = (1/w) × lateral × −0.1 (0x42E7F8), clamped below to −1.0 (0x4ACA4C) then above to 1.0. The camera update 0xADCD0 builds this matrix from identity, rotations and a translation by the negated eye, so at the eye the lateral component is nominally zero (binary32 rounding of large absolute coordinates may leave a small residue). Held sound.md states the engine ignores the pan. Note: phase 69 runs before this frame's camera update (phase 73), so both the eye and the matrix are the previous frame's, mutually consistent.
* Pitch / frequency ratio: 1.0, passed unchanged.
* 0x21 is outside 0x6D..0x77, so the music duck timer G+0x204C is not touched.
* Listener: the camera eye G+0x1DC0 (the same position the sound is placed at).

## 4. Music order relative to present (question 4)

### 4.1 Frame placement

* The whole music logic (schedule phases 85 and 86) is inside the Game update 0xA5A50, which the frame driver 0x134420 calls at 0x134558. In the frame driver the order is: target clears → Game update (including music phases) → fullscreen query and optional resize → frame-budget sleep (0x134725) → present (swap-chain slot +0x40 at 0x13474E). Therefore the music update of frame N always runs **before** frame N's present and before the frame-limit sleep.
* Inside the Game update: phase 82 Interface update → 83 → 84 boss detection → 85 region labels / track choice / timers → 86 music engine → 87 quick-item hint … → 95 scene submission. Music state is thus written before scene submission of the same frame.
* The music engine only sets requests (track path G+0x2028 and volume G+0x2048 under the lock G+0x1D38); actual streaming is consumed by another routine (0x83FB0 reads G+0x2048; thread identity not proved here), so audible timing is asynchronous to present.

### 4.2 Timers inside phase 85 (0xAC0BC–0xAC1B6), exact order

The capped frame interval (binary64 ms, kept in a register from the routine's cap) is used.

1. (Before this point, presentation-only switch bodies choose a track. At 0xAC087 a string comparison 0x6DDF0 on the chosen track, whose second operand was not traced, sets the delay static 0x533934 to 100 when it reports a difference; then, when boss detection holds, the track becomes "boss.ogg" (0x4368D0).)
2. G+0x2050 ← truncate-toward-zero(binary64(G+0x2050) − ms); negative results become 0.
3. G+0x204C ← same rule.
4. If G+0x2050 > 0: track ← "success.ogg" (0x4368E0); if it differs from the current track, delay static ← 500.
5. If the title (I+0), character-creation (I+0x58) or character-selection (I+0x18) root is shown: track ← "maintheme.ogg" (0x436750), clear a local "boss-or-similar" flag; if it differs from the current track, delay ← 500.
6. A local "long hold" flag is that local flag when G+0x1BD4 is zero, otherwise false.

Both timers decrease once per Game update, before the music engine reads them and before present; at a zero-ms frame they are unchanged.

### 4.3 Writers of the timers

* G+0x2050 is set to 60000 (0xEA60) by: region discovery (0xA7471, phase 28), local item pickup hold pose (0xAAD54, phase 67), 0x98180 (called from local control 0x8AE50 at 0x8DF69; sets hold-pose ability 0x7C and copies an item to P+0x1D8) at 0x9A205, and the dialogue update 0xB40D0 twice (0xB51D9, 0xB591D) right after an item is added to P's inventory (0x4E810) and notified (0x9D6F0). The constructor zeroes it (0x7EF39).
* G+0x204C is raised to at least 6000 by 0xA0420 whenever sounds 0x6D..0x77 play ("flute1/2", "harp1/2", "fiddle1", "whistle1/2", "bell1/2", "lute1", "nightingale1" .wav). No other writer was found.

### 4.4 Music engine, phase 86 (0xAC1C6–0xAC3C2)

Query the audio object (slot +0x28, "is playing", identity by slot position only).

* Playing: counter static 0x551504 ← truncate(binary64(counter) + ms). If the chosen track differs from the current one and byte static 0x533938 is set: delay ← counter + 500 when boss or success timer active; otherwise 30000 when counter < 25000, else counter + 5000; clear the byte. If the track is unchanged: delay ← 300000 when the long-hold flag is set, else counter + 300000, and set the byte. Then under lock G+0x1D38 approach G+0x2048 (0x16EE0 with factor helper 0xA7A0 and ms) toward: 0.015 (0x437F78) at rate 0.005 (0x42E518) when G+0x204C > 0; else 0.0 at rate 0.005 (boss or success) or 0.0004 (0x437F68) when counter > delay; else binary32(G+0x1BA8) × 0.01 at rate 0.001 (0x42E514).
* Not playing: if the track changed, store it as current and, under the lock, set G+0x2028 = "Music\\" (0x4368EC) + track. Then delay ← 10000 when G+0x2050 > 0 else 300000; under the lock G+0x2048 ← binary32(G+0x1BA8) × 0.01; counter ← 0; byte ← 1.
* Initial values: delay static 300000, byte static 1 (data section); G+0x2048 initialised to 1.0 by the constructor (0x7EF2E).

## 5. Unresolved

* Whether any computed-address, injected or out-of-image writer feeds G+0x2250 (none in the operand census).
* The role and reachability of the unreferenced list-clear routine 0x81170.
* The full structure of quest-tag.plx: which children of "missiontag-white" exist ("info", "xp"), their geometry, fonts (only "resource1.dat" is named), colours, textures, and the "shine" animation's tracks, keys and duration.
* The semantics of the second coordinate pair (0.0, −300.0) in the popup placement.
* The GUI routine that advances animation time and clears the playing byte.
* Audio-object slot identities (+0x28, +0x38, +0x40) beyond slot position, and the music consumer thread.
* The exact form of the approach factor 0xA7A0 (shared helper, not restated here).
* No runtime observation confirms any timing or ordering statement.

FINAL / STABLE / HELD
