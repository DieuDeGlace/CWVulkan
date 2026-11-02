# Steam ChatWidget retained-cache lifecycle

Independent native-only bounded contract for ChatWidget0x26a2e0 and its
ordinary interface/input callers. No engine/tests/exporter, owner
files/data/captures, reconstruction or game execution was used. Existing
specifications are unchanged. Text layout remains
[chat presentation §§1–3](steam-chat-feat-presentation.md); generic traversal
and cache predicates remain [cache invalidation](steam-plasma-cache-invalidation.md)
and [surface caches](steam-plasma-surface-caches.md).

## 1. Fresh construction and actual virtual hooks

Ordinary startup0xf9ac0 constructs this Widget at0xffe0d on a newly
created empty node; its stored address is Interface+0x280/Game+0x398.
The reached call provenance is [cache callers §1](steam-plasma-cache-callers.md).

Constructor **0x26a2e0** first calls base Widget0x328ff0 at0x26a347.
The base sets flags+0x150=0, dirty+0x15c=0 and surface+0x58=null, with
cache scale+0x40=1, offsets+0x44/+0x48=0 and signed divisor+0x4c=1.
The derived constructor installs primary vtable **0x46d1e8**, initializes
row/editor state including editing+0x1d8=0 and caret/selection0, loads
resource1.dat into+0x1e0, then **at0x26a440 ORs flag0x40 into+0x150**.
For this fresh ordinary construction its final flag word is0x40;
flag0x200 is clear. The OR operation itself preserves other flag bits.
It does not allocate the cache surface or resize the frame at that point;
the startup's later0x32c9a0 supplies the requested frame.

|Primary virtual slot|Reached target and behavior|
|---|---|
|+0xd8, ordinary per-step own-Widget hook|0xe8a20, no-op|
|+0x08, own drawing hook|0x26a930, manual history/editor/caret drawing|
|+0x30, post-cache-redraw hook|0xe8a20, no-op|
|+0x50, own resize hook|0xe8a20, no-op|

The per-step and post-cache hooks do not inspect elapsed time, editing,
focus, row/text equality or fade, and write no Widget/keyable/node dirty
field. They do not change geometry/cache scalars or issue model work.
No ChatWidget-specific per-step hook forces every ordinary GUI step dirty.

## 2. Actual repeated editing invalidation is in the interface caller

Ordinary Interface **0x103200**, at **0x1033ff**, sets
**ChatWidget+0x15c=1 whenever ChatWidget+0x1d8 is nonzero**. A zero editing
byte skips that write; it does not clear an already pending dirty byte.
The reached branch does not compare old text, caret position or blink
phase, and has no additional local visibility, ancestor-visibility,
Widget-enabled, focus-pointer or positive-dt test.

This write occurs before the same interface call's later
**0x106ba7 →0x349cf0** ordinary GUI step. The generic advance can then
turn the pending Widget dirty indication into node+0x84 and propagate it
upward; its final clear consumes Widget+0x15c while retaining node+0x84
for drawing. Generic visibility/flag0x04/force admissions still apply.
A hidden/excluded GUI branch is not made drawable by this dirty write.

Consequently editing chat is marked on every reached interface update,
including one whose later signed32 GUI step is0 or whose caret-blink
phase has not changed. This is **not** a property of a standalone call
to0x349cf0: that generic step invokes ChatWidget's no-op+0xd8 and does
not synthesize the earlier Interface write.

Chat editing+0x1d8 is distinct from active Widget engine+0x170. The
ordinary Enter/character admission tests that active getter for null;
the Enter opening branch sets the editing byte, without assigning chat
as the engine's active Widget. The generic own-Widget active comparison
can independently mark a node when it actually matches, but it is not
the source of the repeated ordinary editing invalidation above.

## 3. Text/input dirty producers and equality boundaries

These are explicit reached writes in addition to generic layout/focus/
surface-preparation producers:

|Reached action|Dirty behavior|
|---|---|
|History append0x26b670, normal completion **0x26bea8**|Always sets Widget+0x15c=1 after processing the supplied tokens. There is no old/new content equality test. Successful empty input also reaches this write, whether or not it adds a visible token; an initially empty history can acquire its initial row. Allocation/length-error paths are outside this successful-return statement.|
|Editor short-key handler0x26b300, **0x26b315**|Calls caret/selection clamp0x26b600, then unconditionally sets Widget+0x15c=1 **before** key-specific branches. A cursor key already at its boundary, a key with no editor action, or another no-content-change route still has that write when this handler is reached.|
|Enter closing path in Game0x96e70, **0x970be**|Marks Widget+0x15c=1 before setting editing0 and before subsequent ordinary send/buffer clearing. This includes closing an empty entry. Its later editor short-key call also retains the handler's ordinary dirty write.|
|Enter opening path in0x96e70|Sets editing1; this opening branch itself does not directly mark+0x15c. The next reached interface editing branch supplies the repeated indication.|
|Character insertion0x26b170|Its clamp/selection deletion/string insertion/caret advance have no direct+0x15c, attached-keyable+0x49 or node+0x84 write. The ordinary character caller0x9f670 admits it while editing is nonzero and no active Widget is present; the subsequent interface editing write supplies invalidation.|

Helper0x26b600 only clamps caret/selection; deletion helper0x26a8c0
clamps and attempts selection deletion, or one following code unit when
selection is0, under its existing bounds. Neither helper itself
marks those dirty fields. Their caller's explicit write or ordinary
editing-frame path must remain distinct from the mutation operation.
These input paths do not resize the Widget or rewrite its cache scalars.
Existing complete key/send/token behavior is not replaced by this table.

## 4. Draw-owned mutation, ordering and cache reuse

The ordinary constructor leaves flag0x200 clear, so traversal0x330b20
calls ChatWidget's own draw+0x08 **after its reached child traversal**.
On a Widget-cache redraw this drawing is inside the retained surface's
redraw interval. The post-cache+0x30 call is reached after that own draw
and before ending the cache target; ChatWidget's target is the no-op
above. Final retained-surface composition follows the generic contract.
On a cache hit the traversal skips the own draw and post-cache hook,
along with the node's shape/children; it performs the retained copy.

Manual draw **0x26a930** can remove oldest history rows to fit current
queried height and, while the editing/caret-blink branch is entered,
clamp caret/selection. It queries frame height and existing composed
geometry for its text positions; it does **not** set frame size/position,
Widget cache scale/offset/divisor, Widget+0x15c, node+0x84 or attached
keyable+0x49. Its row pruning is not followed by a second dirty write.

It issues the existing GUI font/context calls, not queued/direct WORLD
model work. The raw GUI clock determines the literal pipe's blink only
when this draw actually runs. There is no history age/fade/reveal update
here, no per-step text fading, and no post-cache time-based invalidation.
Closing editor state does not by itself age out or periodically redraw
the retained history image.

Thus nonediting unchanged history can reuse its cache between actual
dirty producers. An admitted editing update redraws even when the text
and blink phase are equal, because of§2's unconditional editing write.
Entering the own draw does not leave a ChatWidget-specific dirty request
for another frame; generic surface allocation/resize and unrelated
callbacks/layout retain their separate effects and timing.

## 5. Limits

These facts cover the reached ordinary ChatWidget class and explicit
interface/input calls. General ancestor admission, shared keyable changes,
focus loss, external/custom mutation and generic cache resource failures
remain their own contracts. This is not a census of every writer of the
editor-active byte or arbitrary direct history/string mutations. No dirty
snapshot/content comparison, generic elapsed-time redraw or guessed fade
was added, and no existing specification was edited.
