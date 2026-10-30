# Steam chat and received-feat presentation

Independent native behavioral specification from ChatWidget0x26a2e0/
0x26a930/0x26b670, frame0xa5a50, input handlers0x96e70/0x9f670,
floating-text0xb6240 and their helpers. No engine, tests, exporter, build
or owner captures were inspected; no game was executed. These are actual
consumers/UI. Feat origination remains bounded by steam-feat-production-audit;
this document does not invent an award or interpret its unknown payload.

## 1. Widget and frame geometry

**Game+0x398** is ChatWidget, RTTI at0x54b4a0, primary vtable0x46d1e8.
Creation **0xf9ac0** uses a new empty GUI node through0x33cd30, not a
blackwidget or an owner text-entry box. Constructor0x26a2e0 binds the
ordinary owner font **resource1.dat** and sets its widget flag0x40. There
is no native replacement chat-panel fill/border in this constructor or
manual draw. Parent GUI visibility/clipping still applies.

The initial requested frame is **400×250 GUI units**, through0x32c9a0.
Layout **0x102787–0x1027cc** obtains effective height with0x32ad10 and
width with0x32da30. It places the chat node with **anchor(W−width,H−height)**
and **offset(−20,−20)** through the shared0x334510 placement helper. Use
that helper's current parent/frame transforms, rather than multiplying
already effective dimensions by GUI scale again.

Relevant ChatWidget fields:

| Offset | Data |
|---|---|
| +0x1a8/+0x1b0 | Displayed row-list sentinel /unsigned64 row count |
| +0x1b8 |32-byte UTF-16 editor string; length+0x1c8, capacity+0x1d0 |
| +0x1d8 | Chat editing-active byte, initially0 |
| +0x1e0 | Owner font object |
| +0x1e8/+0x1ec | Signed32 caret index /signed selection displacement, both initially0 |

Rows are ordered lists of words. A word stores a wide string and **three
RGB bytes**, not an independent message timer or alpha. Appending colored
fragments can continue the last row; there is no mandatory row boundary
between the differently colored sender prefix and body.

## 2. Text layout, colors, history and entry presentation

Draw **0x26a930** first removes oldest rows until count≤**trunc32(frame
height/18)**. It draws the remaining oldest row first, with left coordinate
**3**, baseline **19**, and **18** between row baselines. Empty rows also
consume18. History disappears by this height-based removal; this draw has
**no message-age fade, typewriter reveal, arrival bounce or timeout**.
Closing chat editing does not itself delete displayed rows.

Each ordinary stored word is drawn with **resource1.dat, font size12,
outline input3**, no alignment/wrap flags. Fill RGBA is its stored RGB
bytes each converted to float and **divided by255**, alpha1. It draws a
black outline pass followed by colored fill at the same position. A token
that is exactly one U+0020 is not rasterized and advances exactly **5 GUI
units**. Other words advance by the native measured **box maxX−minX**,
not by an extra fixed inter-word gap. Punctuation remains part of a word.

Measure **0x352110** uses the composed node matrix, font12, outline3,
box mode, full text and flags0. Its glyph box includes the outline used
by this query; do not replace it by unoutlined summed pen advances or
force an identity matrix while the draw uses a scaled node. Positions in
the manual font calls use the current matrix's X scale; ordinary GUI
ancestor/mask context is entered before these calls and restored afterward.
The full native font/rounding contract remains plasma-shapes-and-text §§6.7–6.8.

The editor buffer is drawn **at X3, baseline frameHeight−12**, white,
with the same font12/black outline3, independently of whether the editing
byte is set. The usual send route clears that buffer; a route which merely
clears editing need not clear the retained text. There is no separately
drawn entry-background rectangle or selection highlight in0x26a930.

While editing, a **literal vertical pipe** caret is drawn only when
**(signed GUI clock+0x194 /500) bit0 is1**. This is the GUI engine's
global clock, not a timer reset on opening chat. Its baseline is the same
frameHeight−12; X is3 plus the font's prefix pen position at the clamped
caret index, obtained through **0x352310→0x351650**. It uses the same two
black-outline/white-fill calls. Thus cursor position uses a prefix **pen**
query, while word wrapping/advancement uses the **box** width above.

### 2.1 Append tokenizer and wrapping

**0x26b670** accepts UTF-16 text and one RGB-byte color. It splits at
**space U+0020, LF U+000a and CR U+000d**. Nonempty preceding fragments
become tokens, and each actual delimiter becomes its own one-unit token.
There is no dictionary expansion, punctuation split, apostrophe/hyphen
special case or `$colour` parser here. At end-of-string it emits only
the preceding nonempty fragment, not a synthetic delimiter/empty token.

An LF or CR token creates a new empty row and is not appended as a glyph.
They are separate row delimiters: raw CRLF supplied directly here can
create two row transitions. This is distinct from the dictionary XML
import normalization established for speech. Repeated spaces remain
separate5-unit tokens.

Wrap width is **trunc32(frameWidth−30)**. Start from the measured width
of the current final row: spaces5, other existing words' full float box
widths. For each new ordinary token, its incoming width is5 for space,
otherwise **trunc32(box width)**, converted back to float for addition.
The fitting comparison is **strict sum<wrapWidth**; exact equality wraps.

There is one deliberate fragment-gluing exception. If the existing last
row's final stored token is nonempty and is **not exactly space**, the
first otherwise overflowing incoming ordinary token may still be appended;
that allowance is then consumed. It can survive earlier fitting tokens.
This permits separate colored
append calls to finish the same text fragment. It is not permission to
concatenate all subsequent words into an overflowing row.

That allowance belongs to this **one append call**. Explicit CR/LF row
creation **does not clear it or rearm it**. A true ordinary width wrap can
only occur while it is already false and leaves it false. Only accepting
an otherwise overflowing ordinary token consumes a true allowance; fitting
tokens do not. The next append call computes a fresh allowance from its
then-current last row, even if the previous call retained an unused one.

Otherwise an overflowing token creates a new row and is appended there
without splitting a long word; a space that caused that wrap is discarded.
For that discarded space the current append's running width still becomes
**5**, even though the new row contains no stored token. Later tokens in
the same call therefore inherit that5-unit wrap budget cost. A later append
recomputes width from actual stored tokens; it does not retain this phantom
space width.
Explicit CR/LF creates its own row directly. Existing rows are not globally
rewrapped by a later append or by the draw's height trim. An append marks
the widget dirty. No fixed maximum message-character count or elapsed-time
history cap is established by these routines.

## 3. Actual incoming text producers and colors

Frame0xa5a50 processes **at most one** pending received-chat entry from
Game+0x21a0 per visit, at **0xa8530–0xa8610**. If sender ID is nonnegative
and resolves to a creature, append its current converted name plus literal
**`: `**, in **RGB(0,255,255)**. A missing/non-resolving or negative sender
has no invented sender label. Then append the received UTF-16 body plus
**LF**, in **RGB(255,255,255)**, and pop that entry. The sender and body
are separate append calls and therefore preserve the color boundary.
Local host/standalone entered text uses this same queue/consumer. Enter's
ordinary enqueue0x9775d has no guest/host/standalone role branch and does not
directly append ChatWidget text. Thus its local display also respects one
queued record per Game frame; do not add an immediate second UI-only echo.

The world's ordinary notification-string list **World+0x180** is visited
separately, appending each text plusLF in **RGB(255,200,200)** at0xa8679.
The color writer0x4ce70 copies the supplied three bytes; it is not a
rarity lookup. Float-color wrapper0x26bf10 truncates each supplied
component times255 to a byte before using the same append routine.

Received feats use the purple RGB in §4, independently of their floating
text's float RGBA. These consumers do not feed raw chat through the speech
grammar and do not infer colors from words or sender names.

## 4. Received-feat chat, floating text and sound

The event record/delivery/recipient test remains remaining-runtime-gaps
§2 and steam-feat-production-audit. Only records addressed to the local
creature ID create this display. **0x935a0 always returns literal FEAT!**
and ignores the trailing descriptor. Frame consumer **0xa8969–0xa8a9e**
appends **`FEAT!` plusLF** to chat in **RGB(200,100,255)** and plays
**sound0x1f through0xa03f0**, the existing listener/camera sound helper.
No item, XP, gold or mission award is introduced by this display path.

Its floating-text payload is104 bytes before list links, with these
initialized inputs:

| Payload offset | Feat value |
|---|---|
| +0 | Signed32 age0 ms |
| +4 | Float total vertical screen drift−100 |
| +8/+0x10/+0x18 | Local player's physical signed64 fixed XYZ at receipt |
| +0x20/+0x24 | Float screen offsets(+5,−100) |
| +0x28 | Float RGBA **(0.7,0.2,1,1)** |
| +0x38 | Wide string FEAT! |
| +0x58 | Signed32 lifetime3000 ms |
| +0x5c | Float font size30 |
| +0x60 | Text flags0 |

The float color comes from constants0x4328ac/0x42e700 and the1 constants,
verified at0xa8976–0xa898e. Do not substitute the chat byte color or attach
this record to a moving player after receipt. The position is captured.

### 4.1 Floating consumer0xb6240

Every visit clears/rebuilds the transient GUI container **Game+0x670**.
For each retained record, add current render-origin offsets **G+0x1df0/
G+0x1df8** to its fixed X/Y, leave fixed Z, convert completed sums to
float blocks, transform/divide by the retained **view G+0x1f24**, then
by **projection G+0x1f64**. Only positive divided **view Z** enters the
text construction. Screen X is(W/2) times projectedX plusW/2; screen Y
is(−H/2) times projectedY plusH/2. There is no new terrain-occlusion,
actor-HP, distance-fade or screen-edge-clamp test here.

Use old age for **q=float(age)/float(lifetime)**. Place at
**(projectedX+screenOffsetX, projectedY+q times totalDrift+screenOffsetY)**.
For the feat, this starts5 pixels right/100 pixels above the captured
point and drifts another100 pixels upward over3000 ms. The consumer
passes font30 directly and no GUI-scale multiplier in this record path.

It creates two separate TextShapes at that same position, using owner
resource1.dat and record flags. First: transparent fill, stroke RGB
**record RGB times0.1**, alpha **recordAlpha times(1−q)**; outline radius
**max(fontSize times0.125,3)**, giving3.75 for this feat. Second: fill
record RGB and that same faded alpha, default outline radius0. This gives
a dark colored outline behind the bright glyph, not a WORLD shader/glow
model. Pixel/depth/mask behavior is the ordinary GUI text pipeline.

After attempted drawing, including when the point is behind the camera,
add the caller's signed32 elapsed ms to age. Frame0xa5a50 supplies its
ordinary capped wall elapsed truncated to signed32, at0xaaee8. Remove
after this update **only when age>lifetime**, after traversal. At equal
age3000 the record remains and can draw alpha0 before its next increment.
This is a fade of floating text; it does not impose a chat-row fade.

## 5. Editor controls: verified core and remaining dispatch boundary

The core editor uses UTF-16 indices. **0x26b600 has four ordered tests**:
first negative caret becomes0; then caret greater than the string's signed32
length becomes length. Cache that corrected caret. If cachedCaret+selection
is negative, replace **selection with−cachedCaret**, leaving caret unchanged.
Then, if cachedCaret+the possibly corrected selection is greater than length,
replace **caret with length−selection**, leaving selection unchanged. There
is no second caret clamp after that upper correction in this invocation.
For example length10/caret2/selection−5 becomes caret2/selection−2;
length10/caret2/selection20 becomes caret−10/selection20. Do not silently
replace that asymmetric native rule by endpoint clamping.

0x26a8c0 deletes
the selected signed range, or one unit at caret when selection is0,
and clears selection. A negative selection moves caret to its lower
endpoint after deletion. Backspace at a positive caret with no selection
deletes the preceding unit and decrements caret.

Key handler **0x26b300** implements Left/Right, Delete, Backspace, Home,
End; Shift(VK0x10 in the GUI held-key set) extends the signed selection.
Left with Shift decrements caret/increments selection; Right does the
opposite. Home with Shift adds the old caret to selection then sets
caret0; End adds oldCaret−length then sets caret=length. Without Shift
these moves clear selection. Enter itself does nothing in this inner
editor; Game's outer dispatch handles send/open below.

Character insertion **0x26b170** rejects code units<32, replaces a nonzero
selection, inserts one UTF-16 unit at caret, then increments caret. The
actual Game route **0x9f670** requires no other focused GUI editor and the
chat-edit byte set. It narrows its input to the low byte and **sign-extends
that byte before the u16 insertion call**, verified at0x9f67d/0x9f69d.
This is a distinct input boundary from received UTF-16 network text.

Outer keyboard dispatch **0x96e70** admits chat Enter/editing only while
main UI root G+0x220 is visible, no other GUI editor is focused, and the
Controls binding-wait index G+0x658+0x2e0 is negative. Enter when inactive
sets editing1. Enter when active clears editing, processes a nonempty
buffer, then clears the buffer and forwards the key to the inner editor.
Ordinary text queues the local player ID and text through0xa1610 at
**0x9775d** for the existing chat delivery. Its received echo is §3.
Recognized command branches before that ordinary enqueue are §5.1.

The common mouse-down tail **0x94900** clears editing without a buffer
clear.0x1014f0 explicitly includes the chat-edit byte in ordinary UI mode,
and0x103200 marks the widget dirty every update while editing so the
global-clock caret can change. No edit-history Up/Down producer or
selection highlight is present in the core handlers described here.

**Escape in this Game keyboard handler toggles G+0x420**, before entering
the common chat handling; it does not clear editing/buffer. The inner editor
has no Escape action. This native route does not supply a separate chat-cancel
Escape semantic. No clipboard shortcut is present in the core editor, and
the only direct caller of its insertion routine is0x9f670; additional external
platform clipboard injection remains unverified, rather than assumed.

### 5.1 Recognized entered commands

0x96e70 uses a wide input stream and0x6d6f0 to extract the first whitespace-
delimited token, skipping leading whitespace. Comparisons are exact and
case-sensitive. It recognizes these six tokens; remaining arguments do not
become chat. Unrecognized tokens, including unrecognized slash commands,
send the **complete original editor buffer**, retaining its whitespace.

| Token | Local behavior |
|---|---|
| `/check` | Toggle MultiplayerWidget G+0x388 byte+0x1f0, append the literal `multiplayerWidget->checkGame = ` followed by decimal0/1 andLF |
| `/namepet` | Extract the next whitespace-delimited token only; absent name does nothing. Names1..15 UTF-16 units rename the equipped pet item only when its category byte is20; length≥16 appends `Name is too long.` plusLF |
| `/sit` | When current ability clock is **strictly greater** than float(current total duration0x4f220), replace ability+0x68 with0x57 and set clock+0x6c to0 |
| `/wave` | Same duration/clock gate, ability0xa1 |
| `/pet` | Same duration/clock gate, ability0xa2 |
| `/dance` | Same duration/clock gate, ability0xa3 |

Command feedback above uses float RGBA **(1,0.5,0.5,1)** at0x4380f0,
converted by0x26bf10 to **RGB(255,127,127)**. These local commands do not
implicitly post their text to the ordinary chat queue. A recognized motion
command that fails its duration gate is still consumed. No current-HP or
velocity gate is added by these command branches.

The pet item is the local creature's embedded item at+0x8b8. Name writer
**0x10a3c0** stores name length in item+0x9d and each input UTF-16 unit's
**low byte** in the byte at item+0x20+4×index. This command's1..15 limit is
stricter than the helper's own≤32 acceptance. It does not rename the player
or resolve an arbitrary nearby pet; subsequent existing pet-item consumers
provide the visible pet name. Chat buffer clearing occurs after recognized
commands just as after an ordinary send. Other GUI focus/capture gates in
§5 still apply.

These sections are stable for implementation. The only retained input
uncertainty is additional platform clipboard injection outside the traced
editor route; it does not justify invented commands, timeouts or colors.

## 6. `/check` actually gates multiplayer friend-row interaction

MultiplayerWidget constructor **0x279db0** initializes **widget+0x1f0=1**.
The command in §5.1 toggles that byte. Its verified behavioral consumer
is the widget's manual draw **0x27a520**; this is not a dead diagnostic
checkbox and is not the host's world-generation/seed compatibility test.

Friend-list refresh **0xb33f0** enumerates the Steam friends interface with
mask **4**, validates the supplied Steam IDs, and stores ID/name40-byte
records in widget+0x1f8/+0x200. It does **not** use checkGame to remove
records. The draw still includes a valid friend's row when its game is
ineligible; a disabled friend is not filtered out of this underlying list.

Each draw resets **hover index+0x2ec to−1** and **right-action byte+0x2f0
to0**. With checkGame true it queries the local identifier from the
**SteamUtils009 context at0x53cc20, virtual offset0x48**, then for each
friend queries **Steam friends virtual offset0x40** for its current game
information. The row is interaction-eligible only if that query succeeds
and **low24 bits of returned64-bit game ID equal the local returned32-bit
identifier**. It is not a comparison of the friend record's Steam ID,
the player's region or the world seed. With checkGame false those game-
information checks are bypassed and the valid row enters normal hit tests.

For an ineligible row the name text color becomes **RGBA(0.5,0.5,0.5,1)**
from0x43f0f0. It does not enter the branch that records the hovered row or
draws hover-only invite/uninvite actions. Existing host/invited/joined
suffix/state queries still occur, and an existing connection can still
produce its normal join/leave text; merely drawing that text does not make
the row eligible or assign its hover index.

Mouse activation **0x27b0f0** requires the cached hover index to be in the
stored friend vector. It uses+0x2f0 to choose invite/uninvite side versus
join/leave side and invokes the existing networking helpers. It does
**not** independently redo the game query. Therefore `/check` changes
the normal draw's eligibility/color/cache, which determines whether this
activation can reach those helpers. Do not implement it as friend-list
filtering, a world-mismatch packet rejection, or an automatic invitation.

Bounded source result: constructor, command toggle and these draw reads
are the identified writes/consumers for this widget byte. No networking
transport/handshake check of this byte was established. This resolves the
widget-flag part of netcode-compatibility's old question5.2.1; the native
interface slots/returned-data comparison are stated directly without
guessing an additional Steam-version compatibility rule.

### 6.1 Export-safe Steam ABI and output buffer

Native interface constructors **0x3be70 /0x25e910** request exactly
**SteamFriends017 /SteamUtils009**. Identification of the virtual calls
is independently confirmed by the **original native steam_api64.dll**:
its exported **SteamAPI_ISteamFriends_GetFriendGamePlayed** at DLL RVA
**0x53d0** forwards to virtual+0x40; exported
**SteamAPI_ISteamUtils_GetAppID** at DLL RVA **0x5340** forwards to
virtual+0x48. These are DLL-relative provenance addresses, not Game RVAs.
This resolves the local call's API identity as **GetAppID**, separate
from native invitation protocol version100001.

GetAppID receives the interface pointer and returns **unsigned32 AppId**
in EAX. Native compares all32 of this local return against the friend's
masked24-bit game ID; it does not narrow the local value to24 bits.

GetFriendGamePlayed receives **interface pointer, unsigned64 friend SteamID,
output pointer**, in RCX/RDX/R8 under Windows x64. Its return is boolean
in **AL**. The original draw supplies a **24-byte, eight-byte-aligned
output record** at stack+0x38. The **unsigned64 game ID is at offset0**;
the native game reads its low32 at0, masks to24 bits, and uses no other
output field for this eligibility test. The following16 bytes must still
be present for the API's complete output; they are not another string or
array/count header. A byte-packed8-byte result allocation is insufficient.

The draw initializes the game-ID object at0..7 and the final eight-byte
object at16..23 before the call; the middle eight bytes are not used by
this consumer. A wrapper may safely zero all24 bytes. On false return
the row is ineligible and **no output game-ID comparison is performed**.
On true return use the new output from that friend, not a retained value
from a previously successful friend. Caller setup and used-field reads
are verified at **0x27a9a7–0x27aa14**. No layout/name for unused fields
is required by this contract.

## 7. Chat, focused GUI editors and menu visibility

### 7.1 Distinct keyboard and character admission

Keyboard-down **0x96e70** requires all three conditions before its chat
open/send/editor handling: **G+0x220's own live display visibility is
nonzero**, **GUI engine+0x170 focus pointer is null**, and **ControlsWidget
G+0x658's waiting index+0x2e0 is negative**. An open system/options/control
panel is not an additional direct condition. In particular, G+0x420's
system-menu byte is neither a chat-cancel nor a gate here. Global Escape
toggles that byte before these tests, even when a focused editor or hidden
root prevents the later chat branch.

Character route **0x9f670** requires only **null GUI focus pointer** and
**chat editing+0x1d8 nonzero**. It does **not** recheck root visibility,
Controls waiting or menu visibility. If a retained active chat outlives a
root-visibility transition, that route can still insert characters even
though keyboard Enter cannot send/close it until its own gates pass.
Do not fold these routes into one invented modal-blocked predicate.

**0x342090 merely returns engine+0x170.** It does not test whether that
focused object is an edit widget, active or currently visible. The ordinary
producers here are GUI text editors, but any retained non-null pointer
blocks both chat routes. The GUI's separate focused-widget dispatch also
checks widget+0x1a0; Chat's admission does not add that check.

### 7.2 Main game-UI root production

UI update **0x103200** sets G+0x220 visible exactly when these five
frontend root nodes' own visibility values are all0:
**G+0x118, G+0x130, G+0x170, G+0x198, G+0x200** (UI-array indices
0,3,0xb,0x10,0x1d). It otherwise hides G+0x220. This is separate from
the much broader ordinary-UI predicate0x1014f0.

Options **G+0x650/UI[0xa7]**, Controls **G+0x658/UI[0xa8]**, and system
menu byte **G+0x420** are not in that five-root hide test. Opening those
over gameplay does not by itself hide the chat root or prohibit Enter.
A retained visible frontend root can still hide it in a title/frontend
context. Apply the actual root state, not simply 'an options menu is open'
or 'any ordinary UI is open'. Character insertion retains its narrower
gate even when the root is hidden as above.

### 7.3 Platform event ordering and editor Enter

Windows message adapter **0x133c70** forwards **WM_KEYDOWN to GUI0x340df0
first, then Game0x96e70**. WM_CHAR likewise calls **GUI0x340dd0 first,
then Game0x9f670**. GUI dispatch is not a consumed-event return that skips
the Game call. The GUI receives the low16 input bits; the Game route gets
the low8, with the signed-byte insertion behavior already specified in §5.

GUI0x340df0 updates its held-key set and calls the current focused active
widget's key-down callback. Ordinary text editor **0x327e20** handles Enter
by calling **0x349a90(engine,0)** to clear focus. Because that occurs before
Game's subsequent gate test, the **same Enter event can finish an ordinary
GUI edit and then open chat**, or send an already active chat, when the
root/waiting gates permit it. Do not use a pre-GUI focus snapshot to invent
a consumed Enter for Chat. Other focused-widget callbacks retain their
own behavior; this is not a blanket clear-focus-on-any-key rule.

Focus helper0x349a90 marks the old widget dirty and sends the existing blur
event before replacing engine+0x170. Mouse button dispatch0x340ec0/
0x341300/0x3415f0 also clears focus when the hit node's widget differs
from the focus pointer (or there is no hit); ordinary text-editor mouse
handler0x328120 can then focus the clicked editor. Merely having a menu
visible is not equivalent to having an editor focused. A visibility change
does not supply an implicit null return from0x342090.

### 7.4 Binding capture does not finish inside WM_KEYDOWN

ControlsWidget's native vtable **0x46e468** has the no-op0xe8a20 for
key-down/key-up/character offsets0xc0/0xc8/0xd0. Neither GUI keyboard
dispatch nor Game0x96e70 captures a binding and clears waiting before
testing Chat. Game's waiting test is the ordinary gate in §7.1; it is
not a capture-and-early-return block with a later Chat retry.

The actual capture/finish occurs later in **0x1355a0's sampled input
branch**, using rising scan-code states. It assigns through0x8a120→
0x2769a0 for ordinary keys, or skips assignment for Escape/Enter/keypad
Enter, then clears waiting through **0x8a4f0** after the first fresh key.
The Windows message pump has already drained/translated/dispatched its
queued messages earlier in this main-loop iteration. It does not replay
that key's Game keyboard handler after the capture clears waiting.

Consequently, when waiting is active at a normal **WM_KEYDOWN entry**,
that same event does **not** finish binding capture and then open/send
Chat: its waiting gate remains closed during Game dispatch. This differs
from an ordinary focused editor's synchronous Enter blur in §7.3. A later
distinct or auto-repeat WM_KEYDOWN after the sampled branch has cleared
waiting sees the new state and can reach Chat normally; no additional
cross-event 'binding-finishing key consumed' latch was established.
