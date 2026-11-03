# Steam ordinary mouse delivery to the camera

Independent native-only supplement. RVAs refer to the supported Steam image.
This extends the established
[pointer-poll cadence, §7](steam-character-list-input-and-frame-init.md) and
[GUI pointer/capture routing, §8](steam-gui-pointer-capture-routing.md).
Camera response arithmetic remains
[steam-gameplay-camera.md §3.1](steam-gameplay-camera.md).

## 1. Reached delivery boundary

The inspected direct-call path into Game **0x97880** is application loop
**0x1355a0 at 0x13683e**. It runs after queued window-message dispatch, after
the normal physical control-state application, and before that iteration's
**XInputGetState** and controller work. The later frame driver is **0x134420**.
It is one reached call in the normal input block, not one call per movement
message or a callback conditioned on a changed cursor coordinate.

The normal block requires enabled/ready byte **0x536ce8 nonzero**, foreground
window equal to main window **0x551ac8**, and **0x9ae50 false**. The last helper
tests signed32 Controls **(Game+0x658)+0x2e0**: a nonnegative binding-wait index
excludes this normal movement/XInput block. This is the state after queued
messages, rather than a saved pre-message binding-wait condition. The inactive
branch sleeps 16 ms; it is not evidence of a general simulation pause.

Window procedure **0x133c70** handles **WM_MOUSEMOVE, 0x200**, by returning 1,
without another call to 0x97880. Physical button-message handlers can thus run
before this iteration's pointer poll and use its preceding cached pointer.
The native event/capture details remain in the linked routing contract.

## 2. Exact source and units of the two float arguments

The loop evaluates **0x1014f0 at 0x1367e4** to select the argument source.

**UI false:** the source is the first two signed32 components, offsets **0/4**,
of the native mouse-device state read at **0x136672**, which requests **0x10
bytes**. The native mouse data-format descriptor is **0x473c40**, with data
size 16 and flags word 2. These X/Y values are relative device displacement
counts. The bridge loads them at **0x136821/0x136829**. It does not subtract
an OS cursor center or previous cursor point, convert them to GUI units,
multiply by elapsed time, or scale them to client pixels.

**UI true:** the loop calls **GetCursorPos at 0x1367f2**. If fullscreen-state
DWORD **0x551bf0 is zero**, it calls **ScreenToClient at 0x13680d**; otherwise
it keeps the screen coordinates without that conversion. These are absolute
Windows coordinates, client coordinates only in the converted branch. They
have no GUI-scale multiplication in this bridge. The signed32 XY are loaded
at **0x136813/0x136819**.

Both sources undergo signed32-to-float32 conversion at
**0x136831/0x136834**, then become arguments 2 and 3 of 0x97880. The callback
receives these two different coordinate meanings depending on the sampled UI
predicate; it does not always receive a mouse displacement.

There is no inspected success-result branch after the device-state read or
these cursor API calls that suppresses the callback. Numeric contents after
a failed output-producing call are not established here; zero deltas or a
recovery sample must not be inferred from failure.

## 3. Pointer update, repeated predicate reads and stationary motion

The handler reevaluates **0x1014f0 at 0x978bc**. When true, it truncates the
two floats to signed32 and calls Interface **0x101160 at 0x978e3**, which first
invokes GUI pointer movement **0x3418d0**. That helper copies current pointer
floats **engine+0x180/+0x184** to previous **+0x188/+0x18c**, then stores the
new current point before hit/capture/hover/move processing. A reached unchanged
point still performs this update and the Interface timestamp/filter work;
the complete details are already in pointer-poll cadence §7.

Only after that UI processing does **0x978e8** test last-device byte
**Game+0x1958**. Nonzero returns from this UI branch before its mouse-specific
map/customization/camera work. With zero it continues, including the existing
right-button map/customization routes in gameplay-camera §3.1.

The later camera gate reevaluates UI at **0x97b82**. It admits camera response
when UI is false, or when physical left button bit 0 is held and **0x33f1e0**
returns null. An admitted block reevaluates UI again at **0x97bb1**: when true
it uses current-minus-previous cached GUI pointer floats as the displacement;
when false it retains the incoming arguments as displacement. These are live
predicate reads, not one saved mode snapshot across GUI callbacks.

**Stationary relative polls are admitted.** If UI is false and both device
displacements are zero, the normal loop still calls 0x97880, the handler still
enters the camera block, applies the zero deltas, and still clamps retained
pitch target to **0..180 degrees**. This happens before the iteration's XInput
work. The UI-only last-device return does not exclude this branch even when
last-device is pad. There is no intervening nonzero-motion predicate in this
reached producer/handler path. Camera speed, invert-Y, float products, widened
double target updates and clamp comparisons are unchanged from camera §3.1.

## 4. Exact ordinary-UI predicate

**0x1014f0**, called with Interface **Game+0x118**, returns true on the first
true condition below, in this order. A visible root means its own current
primitive visibility value is nonzero, not an ancestor-visible traversal,
pointer hit, active capture, or nonempty widget contents:

| Order | Condition |
| --- | --- |
| 1–5 | Visible roots at Game+0x118, +0x130, +0x170, +0x198, +0x200: title, character selection, character creation, world selection, world creation |
| 6 | Nonzero full-map byte Game+0x668 |
| 7–10 | Visible widget roots, widget+0x178, at Game+0x330 inventory, +0x340 crafting, +0x388 multiplayer, +0x350 shop |
| 11 | Nonzero separate byte Game+0x420 |
| 12–14 | Visible widget roots at Game+0x370 upgrade, +0x368 customization, +0x360 identification |
| 15 | Nonzero chat-active byte (Game+0x398)+0x1d8 |
| 16–19 | Visible widget roots at Game+0x378 answers, +0x358 system menu, +0x658 Controls, +0x650 Options |

Global Plasma-root/F4 visibility is not another term. Ordinary NPC bubbles,
general DialogWidget Game+0x418, and Ring selector Game+0x310 are not terms.
Full map +0x668 is not an NPC conversation flag. The identical enumerated
predicate is also documented in
[steam-npc-speech-rendering.md §1](steam-npc-speech-rendering.md).

## 5. Last-device state and synthetic/warp bounds

Constructor **0x7ea80 at 0x7ec8c** initializes Game+0x1958 to zero. Before
this movement callback, the loop's mapped physical-control pass writes zero
at **0x136798** when a tested mapped physical key/button is active. After
the callback, admitted XInput branches can write one, for example at
**0x1368b4/0x1368e1**. The broader controller assignment conditions remain
remaining-runtime-gaps §4. The movement handler itself reads this byte but
does not clear it merely because movement arguments or GUI displacement are
nonzero. A complete census of unrelated key/button writers is outside scope.

Later controller SetCursorPos navigation does not invoke another 0x97880 in
that iteration or immediately replace the GUI cached pointer. A subsequent
admitted absolute poll can observe its new OS cursor position. A generated
WM_MOUSEMOVE follows the same window-procedure return described in §1; it
does not provide an extra route into the movement callback. No additional
ignore-next-move, warp-discard or coordinate-change exclusion is established
inside the inspected producer/handler bridge.

## 6. Limits

These are the ordinary positive call path and its explicit guards. The direct
call audit is not a proof against external hooks or all indirect callers.
No per-message, per-present or arbitrary cursor-callback cadence is supplied.
Raw-input alternatives, OS event coalescing, device failure/reacquisition,
injected-input provenance and cursor API failure contents remain unproved;
no recovery or synthetic-motion filtering is invented for them. The existing
capture/GUI traversal contracts remain separate from OS focus and this normal
poll admission.
