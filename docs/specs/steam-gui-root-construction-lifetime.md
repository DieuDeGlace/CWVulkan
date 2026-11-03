# Steam ordinary GUI root construction and lifetime

Independent native-only contract for ordinary Interface startup and the reached
frontend/root lifetime boundary. RVAs refer to the supported Steam executable.
G is Game, I is its embedded Interface **G+0x118**, and E is GUI engine
**G+0x20**. This does not enumerate every derived Widget or assume contents of
uninspected owner documents. Pointer dispatch remains
[steam-gui-pointer-capture-routing.md](steam-gui-pointer-capture-routing.md).

## 1. The engine root exists before Interface population

GUI engine construction **0x339b70** creates a node through **0x33cd30** and
stores it at **E+0x140 at 0x339fe5**. Ordinary Interface initialization
**0xf9ac0** uses that existing node as the parent for its top-level live roots.
It does not replace E+0x140 with the root of gui.plx.

Ordinary draw entry **0x33e170** traverses **E+0x140** through **0x330b20**.
Ordinary uncaptured picking likewise starts at **E+0x140**; a nonnull capture
**E+0x168** takes precedence. Engine registration of an allocated node is
separate from attachment to this tree: **0x32ee60→0x33bc30** registers nodes
even when they have no parent. A registered detached source/template tree is
not thereby another ordinary draw/pick root.

At engine construction, hovered node **E+0x160**, capture **+0x168**, focus
Widget **+0x170**, auxiliary Widget **+0x178**, and down-owner Widget **+0x1b0**
are initialized to null, at **0x339d58–0x339dac**. This is their engine
constructor state, not a proof that arbitrary later initialization callbacks
cannot assign them.

## 2. Document sources and live instances are distinct

In **0xf9ac0**, the gui.plx load **0xfb59f→0x3429a0** targets a newly created
node whose parent argument is null. The subsequent quest-tag.plx load
**0xfb613→0x3429a0** uses that same detached document target. This target is
not E+0x140 or G+0x220. The established PLX loader appends the file root's
children to its supplied target; see
[plasma-format-and-scene.md §1.8](plasma-format-and-scene.md).

Startup finds named nodes through **0x332190**. Some are moved out of that
source hierarchy with **0x3347f0**, including the node named **blackwidget**,
retained at **I+0x310**. Detaching a named template is different from making a
live window visible. Other selected document nodes are explicitly attached to
live parents. The whole source document target is not attached as the ordinary
live GUI root by this startup path.

For the selected live panels below, **0x333890** constructs a new node hierarchy
from a source template under the requested live parent. This is the native
shallow instance route: new node identities and child links, initially shared
shape/Transformation/Display objects, copied node state, and cloned own Widgets
where present. Children flagged 0x04 are not recursively instanced. It is not
a claim of independent copies of all drawing/keyable data; deep-copy route
**0x3308e0** is different. Exact sharing and Widget replacement remain
plasma-format-and-scene §2.5.

A derived panel constructor can attach its own Widget to the instanced root.
The reached attachment helper **0x334410** preserves the old root Widget's
binding through **0x329c70**, destroys that old wrapper through **0x33e0c0**,
and installs the new wrapper. This changes the own Widget, not the node's
identity or every descendant Widget. It does not flatten the authored subtree.
No particular owner matrices, shapes, children, or template flags are asserted
here beyond the native source names and operations.

## 3. Reached parentage and relative insertion order

Node creation **0x33cd30** appends to its nonnull parent through **0x32f640**.
That helper uses the parent's ordered child list and records the new parent
in node+0x40. These selected startup operations occur in this order:

| Native site | Live node or operation | Parent |
| --- | --- | --- |
| 0xf9ba9 | textFX, G+0x670 | E+0x140 |
| 0xf9c51 | lifebars, G+0x678 | E+0x140 |
| 0xf9cf6 | game-UI container, G+0x220 | E+0x140 |
| 0xf9d9b | title container, G+0x118 | E+0x140 |
| 0xf9e58 | character-selection container, G+0x130 | E+0x140 |
| 0xf9f14 | character-creation container, G+0x170 | E+0x140 |
| 0xf9fd0 / 0xfa092 | world-selection/world-creation containers, G+0x198/+0x200 | E+0x140 |
| 0xfb6bb | Construct StartMenuWidget through 0x2910c0 | A fresh child of the title container |
| 0xfc992 | Construct Options through 0x287050 from a blackwidget instance | E+0x140 |
| 0xfca5e | Construct Controls through 0x275a70 from a blackwidget instance | E+0x140 |
| 0xfedbf | Construct mode-0 InventoryWidget through 0x27c950 from a blackwidget instance | G+0x220 |
| 0xffe0d | Construct ChatWidget through 0x26a2e0 on a fresh node | G+0x220 |
| 0x10069d | Detach/reappend the existing inventory root | G+0x220 |

The title/character containers above are created live containers, not the
detached gui.plx source root. Character cards are later generated within the
character-selection container. Options and Controls are separate top-level
instances, while inventory and chat are descendants of game UI. Thus Options
follows the frontend containers, Controls follows Options, and both follow the
game-UI subtree in the engine root's relative child order. This table is a
selected ordered subsequence, not a claim that no other roots are inserted
between its later entries.

Inventory initially precedes crafting within game UI, but the late reappend
moves it after already existing game-UI children, including chat. The system
menu is then constructed after that inventory move. This agrees with
[remaining-runtime-gaps.md §4.17](remaining-runtime-gaps.md)'s registration-order
completion. Ordinary panel show/hide does not itself perform this reappend.

Startup completes root initialization through **0x340d60 at 0x100809** and
**0x333820 at 0x100820**, then builds the character list through **0x107440**.
The latter changes descendants, not E+0x140. These operations do not mean
reloading the source document as the live root on every show.

## 4. Frontend-to-play transitions retain Interface and root

The direct title **Start Game/Continue** routes in **0x965e0**, existing/New
Character selection in **0x9f7c0**, and **Create Character 0x9ea10** operate on
the existing embedded Interface and engine. They do not call ordinary Interface
initialization again or construct/replace E+0x140. Loading/resetting the local
creature is a different lifetime from constructing the Game and its Interface.

Continue hides the title container and retains the GUI tree. Existing-character
selection refreshes lists and inventory collection bindings on that tree.
Create Character can invoke **0x107440**, whose reached rebuild first detaches
the existing Select/Delete controls in order **7,8,9**, at
**0x107475/0x107480/0x10748b**, then clears old card children through
**0x330a80 at 0x107494**. It constructs the saved-character cards and the
null/new card, initializes the hierarchy, and reattaches those same three
controls after the cards. The character-selection container and E+0x140 remain.
This is genuine descendant destruction/rebuilding, not evidence that every
live child survives a character transition. See frontend-characters §2.3.

## 5. Visibility and detachment are not capture/focus deletion

Ordinary hide helper **0xa3a20** only writes the current Display.visibility
value selected through node+0x68. The reached direct visibility stores in the
frontend handlers likewise do not clear engine capture, hovered node, focus,
auxiliary Widget, down-owner or held-button state. F4 separately toggles the
same property on **E+0x140**, not on game UI or each panel; see
[steam-preview-hidden-interface-order.md §1](steam-preview-hidden-interface-order.md).

Detachment **0x3347f0(node,null)** invokes **0x334330** to remove the child-list
link and clear node+0x40. It does not destroy that node or call the engine's
capture/focus cleanup. Reparenting removes the old link and appends a new link;
reparenting to the same parent can therefore move a node to the list end.
Neither operation has a direct capture/focus clearing write.

Accordingly visibility or removal from the root hierarchy must not be described
as an automatic release. The existing picker prioritizes a nonnull raw capture
pointer over root traversal; the precise disabled/visibility and callback rules
remain in the pointer/capture contract. This does not establish indefinite
validity after destruction or authorize treating a stale pointer as live.

## 6. Explicit reached deletion cleanup

The character-child clearing route **0x330a80** snapshots the child list and
calls engine node deletion **0x33dda0** for each child. That helper only acts
when node+0x08 identifies the supplied owning engine. In its reached order:

- If the node equals **E+0x168**, it clears capture and calls **0x3418d0** at
  unchanged current pointer coordinates before continuing deletion. This is
  real pointer processing, not just a null assignment.
- If **E+0x160** still equals that node after that processing, it clears it.
- It snapshots/recursively deletes children, detaches the node from its parent,
  and unregisters its shared Transformation/shape/Display use. Shared objects
  are destroyed on the reached last-user branch rather than every node removal.
- It deletes the node's own Widget through **0x33e0c0** when present, removes
  the node's engine registration, and invokes its destructor.

Own-Widget deletion **0x33e0c0** directly clears matching references at
**E+0x170**, **+0x178**, and **+0x1b0**, then unregisters/destroys that Widget.
This helper does not run the ordinary focus-change event sequence merely
because a matching focus pointer is cleared. It is distinct from explicit
focus assignment **0x349a90**, and from capture release **0x3493b0**, which
clears E+0x168 and processes the unchanged pointer. These operations do not
constitute a general held-button-mask reset.

The capture-clear pointer pass precedes node detachment and can invoke callbacks.
The native dispatchers can retain raw node/Widget pointers across callbacks;
no stable-ID recovery or universal post-callback liveness/re-pick policy is
established. Explicit deletion cleanup is not a guarantee that arbitrary
reentrant deletion, external raw destruction or every derived override is safe.

## 7. Limits

Owner document contents, missing-template/resource failure behavior, complete
final teardown of the detached source tree, and all later writers/reparenting
operations are outside this bounded trace. Whole-engine shutdown and external
callbacks are not inferred to have the same cleanup ordering as 0x33dda0.
Only the reached ordinary construction, direct frontend transitions and
identified hide/detach/deletion helpers are established. Source templates may
share drawing data with live instances; no private-data assumption or inferred
replacement GUI content is supplied.
