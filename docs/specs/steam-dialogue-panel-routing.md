# Steam answer dismissal and general dialogue panel routing

Independent clean-room facts from original native RVAs. This supplements
`remaining-runtime-gaps.md` §§5.6–5.7. No engine, tests, exporter or third-party
reconstruction was inspected, and no game was executed.

## 1. Movement dismisses answers by current input state

The frame **0xa5a50**, at **0xab283–0xab3bf**, uses the current mapped movement
states, not creature velocity, displacement, acceleration or a change in
position. This is a **held-state** test, with no previous-bit or press-edge
requirement. A movement input already held when answers become visible can
therefore satisfy it on a later frame.

The complete gates are:

- Skip this entire dismissal block while **both** full interactive map byte
  **G+0x668** and last-used-pad byte **G+0x1958** are nonzero. An ordinary open
  map with keyboard/mouse as the last device does not skip it.
- Otherwise dismissal is requested when **any** current forward/back/left/right
  mapped byte **G+0x1a75/+0x1a76/+0x1a77/+0x1a78** is nonzero **and** chat's
  input-active byte **(G+0x398)+0x1d8 is zero**; or when the filtered analog
  pair's squared length is **strictly greater** than the float32 constant at
  **0x437f74**, bit pattern **0x3c23d70b** (approximately 0.010000001).
  The analog branch is not gated by chat input activity.
- That squared length is computed by **0x2b630** from **G+0x1d6c / +0x1d70**:
  each float32 component is squared and the two float32 products are added.
  The frame calls it at **0xab2c2**, compares at **0xab2c7**, and does not
  dismiss on equality or an unordered comparison.
- On a dismissal request, the frame performs its shared inventory/map/panel
  closing operations. For answers specifically, it checks the answer root
  **(G+0x378)+0x178** with **0x9af00** at **0xab3af**. **Only if that root is
  currently visible** does it clear the answer-page vector at widget+0x1e8
  through **0x8adf0**, call site **0xab3bf**.

Visibility recomputation follows this block at **0xab3d3**. Therefore the
clear decision uses visibility retained before this recomputation. Pending
answers still hidden behind a recipient bubble are not cleared merely by
movement. In the frame that the bubble disappears and answers first become
visible, this earlier dismissal check can still see the previously hidden
root. A held input needs no new edge to dismiss them on a subsequent frame.

## 2. Answer visibility is a UI-routing input, not a movement freeze

The visible answer panel is one of **0x1014f0**'s ordinary UI predicates.
It is not a separate universal player/camera lock. In particular, ordinary
movement **0x9c050** has no early return for answer visibility or for a true
0x1014f0 result. Keyboard movement can continue and produce the dismissal
above. The routine's existing chat, system-menu, GUI-interaction and full-map
with pad gates remain independent. Its 0x1014f0 check at **0x9c67f** participates
in the **movement-triggered dodge-roll** eligibility decision; that is not
a return from movement. Ordinary run/walk state is distinct, as detailed
in §12.

Use the established ordinary-UI controller routing from the controller
specification: a visible answer panel routes the left stick to navigation
and the right stick to UI operations, with the gameplay left analog pair
cleared on that route. This is why a controller navigating answers does not
simply walk through them. Do not replace the held-input dismissal predicate
by a velocity threshold or add an answer-specific keyboard movement freeze.

Mouse look **0x97880** likewise uses ordinary UI routing. With 0x1014f0 true,
it first dispatches GUI pointer movement. If the last-used-pad byte is set,
that branch returns. In the keyboard/mouse case, UI mouse motion does not
normally change camera yaw/pitch; the existing exception allows camera
dragging while physical left mouse-button state is held and **0x33f1e0**
returns no GUI hit/capture. That helper returns the captured GUI object
when present, otherwise performs the root hit test. The answer panel does
not introduce a stronger camera prohibition. Its own rectangle can supply
the GUI hit which prevents that exception. These tests are at the end of
0x97880, around **0x97b82 / 0x97bb1**.

## 3. The specialization feedback uses a fixed screen-space panel

The general dialogue object is **G+0x418**, equivalently interface+0x300
because the interface begins at G+0x118. It is a persistent widget, separate
from both the answer panel G+0x378 and the per-speaker bubble vector G+0x400.
The specialization choice action **0x89cb0** compiles its confirmation pages
into this object's vector **+0x1e8 / +0x1f0 / +0x1f8**. No NPC bubble needs to
be created for this confirmation.

Interface setup **0xf9ac0** copies the owner's **blackwidget** template under
the ordinary game UI root and constructs this object through **0x28fd60**,
call site **0xfeb26**. It supplies the owner's **speech-next** and
**speech-end** shapes as the two indicator templates. The constructor copies
these as children of the panel root, loads **resource1.dat**, uses font size
**12** and outline **3**, and initializes milliseconds per character to
**40** at widget+0x2fc. The interface gives this panel dimensions **320×180**
GUI units through **0x32c9a0**; these are ordinary widget dimensions, not a
world-projected speaker-bubble rectangle. The size call is **0xfeb7a**, using
float32 width 320 from **0x441bd8** and height 180 from **0x42e7d4**.

Placement **0x101bd0**, for interface+0x300, measures its transformed widget
dimensions, centers it horizontally in the window, and positions its bottom
**150 GUI units above the window bottom**. More exactly, its parent reference
is **((window width − measured panel width)/2, window height)** with the
additional local offset **(0, −150 − panel height)**. This uses the same
bottom placement convention as the answer panel; it does not follow an NPC's
head or project the confirmation speaker into screen coordinates.
The dimension queries and placement call are at **0x101efa / 0x101f10 /
0x101f42**; the −150 constant is **0x438040**.

## 4. General-dialogue visibility and continuation indicators

The general-dialogue branch of **0xb40d0** controls visibility from its own
page vector. An empty vector hides the root and resets page index+0x204,
clock+0x200, displayed-character count+0x208 and speaker ID+0x210 to zero.
With pages present it processes current page effects under the clock-zero
guard, increments the elapsed clock, and sets the panel root visible. It
does not wait for the absence of recipient-addressed NPC bubbles; that
waiting rule belongs to answers in G+0x378.

Advancement uses the existing **strict clock > nominal page duration+100 ms**
release gate from §5.7. After the last page, it clears the vector, hides the
root, and resets page index/clock/display count. There is no automatic
speech-bubble timeout for the general panel. Movement dismissal in §1
targets the separate answers and does not clear this general page vector.

Drawing **0x28ffe0** sets its child **speech-next** visible only when the
clock is strictly greater than duration+100 ms and a later page exists.
It sets **speech-end** visible only beyond the same strict threshold on the
last page. Before that threshold neither indicator is shown. The panel
uses the stored colored words and typewriter behavior already specified
for this widget, not ordinary unstyled NPC-bubble lines.

General-dialogue visibility **does not itself make 0x1014f0 true**, as already
established in §4.14. It does hide the ordinary/zoom crosshairs through the
explicit checks in **0x103200**. The frame also has an independent visible
general-dialogue target-facing hook: if its speaker ID resolves, that actor
becomes the target for the existing facing behavior. Neither fact changes
the general panel into a modal answer-choice list or an NPC bubble.

## 5. Exact general-dialogue word layout

Sources are the page draw **0x28ffe0** and its per-word helper **0x2904b0**.
At the start of each valid page draw, signed i32 horizontal cursor+0x2f0,
line-top+0x2f4 and reveal-unit count+0x2f8 are zero. A temporary suppress-gap
flag starts true. The helper visits stored words in page order. An actually
empty stored string returns immediately, leaving these counters and the
suppress-gap flag unchanged.

For nonempty words which reach layout, use the **whole stored word** for
measurement and punctuation decisions, even when only a prefix is revealed.
The native font-bounds query **0x269050** supplies minimum/maximum x; the
word width is their difference truncated toward zero to signed i32. Add
**4** to that advance only when the whole word is exactly one ASCII
apostrophe. The punctuation set for spacing and wrap exemption is exactly
the singleton tokens **period, hyphen, comma, semicolon, exclamation mark,
question mark, colon, apostrophe**. Internal punctuation in a longer string
does not make the entire word a punctuation token.

Before layout, unless the suppress-gap flag is true, add the truncated
font size to the cursor for an ordinary word, or integer-divide that size
by **3** for a punctuation token. Then set suppress-gap true only for a
whole word exactly equal to **hyphen or apostrophe**, and false otherwise.
Thus the first word has no leading gap; a punctuation token normally gets
the smaller gap; the word after a hyphen/apostrophe has no gap.

For a nonpunctuation word only, wrap when **cursor plus word advance is
strictly greater than effective panel width minus three times truncated
font size**. Equality does not wrap. Wrapping sets the cursor to zero and
replaces the integer line-top by the truncation of the float32 sum of its
old value and **truncated font size × 1.350000023841858**. The multiplier is
stored at **0x470258**. A first oversized word can trigger this same wrap;
there is no special exemption for the first word on a line. Punctuation
tokens never trigger this wrap.

Submit the revealed string at panel-relative **x = cursor + truncated font
size**, **y = line-top + twice truncated font size**. After submitting it,
advance the cursor by the **whole-word advance**, not by the revealed
prefix's width. Use the stored word's RGBA and the widget's normal black
outline color, **RGBA (0,0,0,1)**, with its existing outline width 3. This
draw path has no answer-row hover/cyan override.

For the configured size 12 and panel width 320, the concrete values are:

| Quantity | GUI value |
| --- | --- |
|Ordinary inter-word gap|12|
|Punctuation inter-word gap|4|
|Initial text x / baseline y|12 / 24|
|Wrap comparison|cursor + full advance > 284|
|Line-top increment at each wrap|16|
|Text offset after wrap|x 12, baseline y new line-top + 24|

The comparison uses the effective **untransformed frame width**, queried
by **0x32a1d0**, not a window width or a fresh fit-to-text width. General
dialogue keeps its configured panel dimensions; this drawing helper does
not expand the panel vertically or fit it to the current page. The key
layout and submission ranges are **0x2908c0–0x290aa5** and
**0x290b61–0x290be1**. Parent/widget transforms and clipping still follow
the ordinary GUI contract.

## 6. Reveal budget includes one unit after each processed word

At each nonempty word, **0x2904b0** computes a signed integer budget by
truncating the float32 quotient **widget clock+0x200 / milliseconds per
character+0x2fc**. The configured divisor is 40. If the budget is **less
than** the current reveal-unit count+0x2f8, it returns false and the page's
word loop stops, without laying out that word. Equality is allowed through.

Otherwise it performs the full-word layout from §5. If the budget is less
than the reveal count plus the whole word's UTF-16 code-unit length, it
submits only the prefix whose length is **budget minus reveal count**;
otherwise it submits the complete word. It then increases the reveal count
by **submitted UTF-16 code-unit count plus one**. The separator unit is
charged even after the last word. A nonempty word with a zero-length prefix
therefore still performs full-word layout and increases this count by one;
the following word then normally fails the budget test. This is distinct
from an actually empty stored string, which returns before the budget/layout
work and consumes no reveal unit.

For ordinary words `Hi` and `there`, the derived budgets illustrate the
boundary: budget 0 submits an empty prefix of `Hi`; 1 submits `H`; 2 submits
`Hi`; 3 also reaches the second word but submits its empty prefix; 4 submits
`Hi` and `t`. An implementation must not reveal the next word's first
character at budget 3 merely because its text representation contains a
space between the words. These are arithmetic examples, not runtime captures.

Nominal duration **0x290430** independently sums **every stored word's
UTF-16 length plus one**, including actually empty stored words, and
multiplies by milliseconds per character. The drawing skip for empty words
does not remove their unit from nominal duration. Prefix lengths are UTF-16
units, with no grapheme or surrogate-pair boundary correction in this helper.
The draw loop stores its resulting reveal count as float32 at +0x208 after
drawing; it does not use that prior count to decide the current prefix.

## 7. Shake affects displayed glyphs, not layout or reveal speed

The per-word byte flag chooses the shaken draw helper **0x269de0** only
when it is **exactly 1**; other values use ordinary text helper **0x269210**.
Both receive the same revealed prefix, color, baseline and full-word layout.
Shake does not modify the reveal divisor, word advance or wrap measurement.

The shaken helper uses **0x268b80**, which emits one UTF-16 unit at a time
through the font renderer. For zero-based unit index j within that submitted
prefix, its extra vertical offset is the float32 cosine of **j plus
float32(engine time × 0.01)**, multiplied by **font size × 0.1**. Engine time
is the signed i32 millisecond field at GUI engine **+0x194**, converted to
float32; 0.01 and 0.1 are the stored float32 constants at **0x42e51c** and
**0x42e528**. The index starts at zero again for each word/helper invocation.
There is no random draw in this glyph displacement.

More precisely, 0x269de0 first converts the supplied baseline position to
its font-helper coordinate space using the GUI transform's scale. 0x268b80
adds the displacement there, then normal font rendering applies its usual
transform. At unit GUI scale and font size 12 its amplitude is **1.2** in
those coordinates. Horizontal continuation uses each one-unit draw's returned
pen position. This separate-unit emission does not perform a pair-kerning
operation between neighboring units of the shaken string. The normal outline
and fill passes use the same time/index sequence.

There is also a separate voice-pulse read of this flag in **0x28ffe0**.
If integer previous displayed-count/3 differs from integer current reveal
count/3, drawing requests **one sound 0x35** at the local player's position,
volume 1; it does not emit once for every crossed group of three. The base
pitch is 1, or the float32 constant **0.9600000381469727** at **0x470428**
divided by the resolved speaker's float at creature+0x84 when that render-map
entry exists. If the last successfully processed word's flag is exactly 1,
first multiply that base by **1.5 + float32(rand())/32767**. Then, for either
flag case, add **(float32(rand()) × 0.5)/32767** using the next random draw.
These are voice-pitch random draws, separate from the deterministic glyph
displacement. The final current reveal count is saved at widget+0x208 after
this sound/indicator work. An empty-prefix nonempty word can be the last
processed word for this purpose; there is no additional visible-glyph test.

## 8. Continuation indicators retain the owner's template dimensions

Constructor **0x28fd60** copies the supplied **speech-next** and
**speech-end** templates under the dialogue root and makes their shape/draw
data independently mutable. It does not replace them with generated text,
an arbitrary icon or a fixed numeric size. The drawing path changes their
visibility and position, **not their size**. Preserve the copied owner's
geometry, frame dimensions and inherited appearance.

Use the panel's effective untransformed width W (**0x32a1d0**) and height H
(**0x329e00**). Let N be the current **speech-next primitive's width in its
parent-widget coordinate space**, from **0x32da30** on the child's +0x70
primitive. The ordinary parent-relative position setter is **0x32b560**,
as specified in `plasma-format-and-scene.md` §2.7.3.

- **speech-next:** x is **(W − N) × 0.5**. Its y is **H − 50 plus
  5 × cos(float32(engine milliseconds × 0.01))**. The time, multiplier,
  amplitude and offset are from **GUI engine+0x194**, **0x42e51c**,
  **0x42e584**, **0x42e5b0**. The position submission is **0x29030b**.
- **speech-end:** x is also **(W − N) × 0.5**, using the **next** primitive's
  measured width, not an independent measurement of end. Its y is exactly
  **H − 50**, with no cosine displacement. The position submission is
  **0x2903d6**.

These positions are relative to the parent widget's frame; they are not
absolute window coordinates. For the ordinary 320×180 panel, both indicators
are nominally at y **130**, with next oscillating by ±5. Their existing
template width determines x. The visibility conditions in §4 are evaluated
separately from these position updates; neither indicator is shown during
the immature part of a page. The native setup supplies both indicator
templates, and the end-position path reads next's primitive width.

## 9. Last-page release clears in the same update; general feedback is not a UI lock

The general-dialogue branch of **0xb40d0** increments the current elapsed
clock **before** testing the strict maturity condition. If mature and either
Primary-Attack or Use has a stored previous held bit and is now released,
it increments the page index **once**, resets elapsed/displayed count, and
requests the appropriate page sound. Releasing both actions in that update
still advances just one page. The stored action bits are then sampled from
the current controls in that mature branch.

It checks the new index against the page count **later in the same call**.
When this was the last page, it clears the vector, hides the root and resets
index/clock/display count immediately, before returning to the frame. There
is no extra visible empty page, delayed timeout or additional release needed.
The empty-vector branch on the following update also resets speaker ID.
When another page remains, its clock-zero page effects run on the following
update, as already stated in §5.7.

**0x1014f0 does not inspect general DialogWidget G+0x418**, its visibility,
or its nonempty page vector. In interface-relative offsets this object is
**+0x300**; the answer panel is **+0x260**, which the predicate does inspect.
This difference is verified directly in 0x1014f0. A general feedback page
alone therefore does not request ordinary UI pointer routing or freeze
gameplay. Its explicit crosshair/facing and action-release consumers remain
as described above; do not replace that behavior with an invented modal lock.

## 10. Word measurement uses unoutlined ink bounds at the current raster scale

Answer drawing **0x28ee60**, answer sizing **0x28f6e0**, and general
dialogue drawing **0x2904b0** use the same widget text measure **0x269050**.
Relevant call sites are **0x28efcf / 0x28f786 / 0x29055a**. The measure
obtains the GUI engine's **current matrix**, through engine virtual slot
**+0x70**, then calls the font measure **0x352110** at **0x2690f7** with:

| Input | Value supplied by 0x269050 |
| --- | --- |
|Requested font size|Widget float32+0x1b0, normally 12|
|Outline radius|**0**, independently of the widget's visible outline radius 3|
|Character / line spacing|Widget float32+0x1b8 / +0x1bc|
|Wrap width / alignment-and-wrap flags|0 / 0|
|Scalable-font mode|Enabled, byte 1|
|Start index|−1, whole string|
|Low-level measure selector|Byte 0: **glyph-box/ink mode**, not pen-position mode|

The arguments are visible at **0x2690bf–0x2690f7**, and the final selector is
forwarded to **0x350eb0**. In that mode the latter unions the positioned
glyph bitmap boxes, excluding each glyph texture's one-transparent-texel
border. Pen advances and kerning position subsequent glyphs, but the measured
x range is the glyph-box range. It does not substitute the final pen x or
the sum of advances for that range. Use the existing native glyph-box rules
and empty/missing-glyph behavior from `plasma-shapes-and-text.md` §6.7.
The outline radius 3 used for visible text does **not** expand this measured
range; do not add six units to a word's width for its two outline edges.

This is not an identity-raster measurement merely because the requested size
is 12. **0x352110 → 0x354600 → 0x354cf0 / 0x3541a0** uses the supplied current
matrix's two axis lengths for scalable raster sizing. For a normal raster
font, raster Sx and Sy are each the corresponding matrix-axis length times
the requested size, rounded to the nearest 1/64 by the positive-size
**multiply by 64, add 0.5, truncate, divide by 64** sequence, then bounded
to **1..1000**. FreeType uses those raster sizes with the scalable/hinted
glyph path and no FreeType rotation/shear transform. Translation does not
affect these sizes. The exact existing setup and glyph rules are in that
same document §§6.2/6.4.

After measuring in raster coordinates, **0x352110** multiplies each x bound
by **requested size / actual raster Sx** and each y bound by **requested
size / actual raster Sy**. Thus it returns bounds in requested-size layout
units. This normalization does not erase hinted bitmap/pen rounding at the
chosen raster size: a different GUI scale can change the normalized ink
width. It is neither a rotated screen-space AABB nor a forced identity-font
measurement. The matrix input is exactly the engine's active matrix at the
measurement call; 0x269050 does not compose another full widget transform
onto it or force a replacement identity matrix.

The dialogue helpers finally calculate **maximum x minus minimum x first**
in float32 and then truncate that difference to signed i32. They do not
truncate the two bounds separately. The apostrophe's extra 4 and all gaps,
wrap comparisons and text origins remain the later layout operations from
§5 or answer-panel §5.6. A missing font makes 0x269050 return zero for both
bounds, before those later operations.

## 11. Compiling into the general panel preserves existing pages and playback state

The phrase/script compiler **0x1694e0** takes a destination page vector. It
does **not** clear that destination at entry and does not reset the enclosing
widget's clock, current page index, displayed count or speaker ID. Its fresh
local variable-definition context is separate from that destination's
existing pages. The ordinary phrase expansion **0x167c80** and page append
**0x16c310** likewise preserve previously stored pages.

For a successfully resolved ordinary dictionary phrase, 0x167c80 appends
a new page if the destination is empty or its **last page already contains
more than 10 stored words**; otherwise it expands onto that existing last
page. This tests the pre-expansion word count. Explicit script page breaks,
answer IDs, emotes and `%` use their separately specified page rules; none
is an implicit clear of earlier pages. The normal phrase page decision and
append are at **0x16806f–0x168095**.

In particular, specialization action **0x89cb0** clears the **answer** pages
at G+0x378, then calls 0x1694e0 at **0x89f60** with **G+0x418's existing
page vector**. It does not clear/reset the general widget before that call.
If general dialogue was already active, preserve its existing pages and
playback clock/index while applying the ordinary append/page rules. The
usual empty-widget case has already been reset by the general update
consumer; that familiar single-page flow is not evidence of a reset inside
compilation. Other callers may explicitly manage their own destination,
but an arbitrary phrase compile must not acquire a blanket replace/reset
policy from this specialization path.

The specialization call also **does not assign G+0x418's speaker ID+0x210**.
It uses the selected NPC's ID's low 32 bits as the dictionary seed at
0x89f60, while retaining whatever speaker ID the general widget already
held. The compiler neither receives that full ID as a speaker assignment
nor performs a speaker-field write. In the usual previously empty-widget
flow the retained ID is zero, because 0xb40d0's empty-vector branch reset
it. Do not turn the dictionary seed into a new full speaker ID or implicitly
apply the NPC-bubble attachment routine's speaker assignment here. The
selected speaker's full ID still belongs to the answer/action context.

## 12. A visible answer panel gates particular player-control stages

These facts concern a normal live-player update with **0x1014f0 true because
answers are visible**. Existing modal-window, actual chat-input-active,
stun/lock, stamina, cooldown and ability-state requirements remain separate.
Older `player-control.md` paragraphs sometimes call 0x1014f0 “chat typing”;
for these consumers it is the **complete ordinary UI predicate**, which
includes the visible answers. Actual chat input activity uses the separate
byte **(G+0x398)+0x1d8**.

### 12.1 Mapped held states are retained, independently of GUI hit testing

Keyboard/mouse polling in **0x1355a0**, **0x136740–0x1367d7**, clears and
resamples each of the 23 logical binding states. It reads the configured
device/code, converts the sampled keyboard or mouse byte to boolean, and
writes the corresponding **G+0x1a70+index** through **0xa36c0**. This loop
does not consult 0x1014f0 or a pointer hit/capture result before sampling an
attack, skill, jump or dodge binding. The setter itself only validates
index <23 and stores the supplied byte.

Thus visible answers do not zero the primary/strong/class/ultimate/jump/dodge
held bytes at the input boundary. A keyboard-rebound primary attack has
the same later logical-action UI gate as a mouse-bound primary attack.
Conversely, rebinding jump or ultimate to a physical mouse button does not
automatically give it a new “GUI click consumed” gate. Physical GUI mouse
events and logical bound-action states are separate native paths. Pad
buttons remain subject to the already specified gameplay-versus-UI routing
in 0x1355a0; an A mouse-event synthesis is not automatically a mapped primary
held-state write in that UI branch.

### 12.2 The action-stage UI gate is global, including keyboard bindings

The player action pass **0x8ae50** queries 0x1014f0 at **0x8ee23**. A true
result jumps at **0x8ee2a** directly to cleanup at **0x8fa86**, skipping the
remainder of the action handling. This is **not conditional on where the
pointer is, a GUI hit, a captured GUI object, or the physical button source**.
Moving the pointer off the answer panel does not re-enable these skipped
logical actions while the ordinary UI predicate remains true.

| Logical action or state | Effect of answer-visible UI at the relevant native stage |
| --- | --- |
|Primary attack, G+0x1a70|Late normal-attack selection/start and its walk-modifier handling are skipped|
|Strong attack, G+0x1a71|Late charge/start/held/release handling is skipped|
|Class skill, G+0x1a72|Late class-skill starts and the late class-key-release cancellation rules are skipped|
|Standing dodge, G+0x1a7a|Late class-dependent stationary dodge/teleport/leap selection is skipped|
|Moving dodge roll|Separately disallowed by the earlier movement predicate described below|
|Jump, G+0x1a79|Still processed by the earlier movement routine, subject to its ordinary movement/ability requirements|
|Ultimate, G+0x1a73|Still processed by its earlier rising-edge handler, subject to its ordinary skill/usability requirements|
|Ordinary run/walk flag|Still updated by its earlier rule; answer visibility is not an additional run/walk prohibition|

The skipped late region includes the normal/strong released-channel endings
and class-release endings from `player-control.md` §§4.9–4.12. The UI gate
does not itself set current ability to zero, cancel every active action or
clear the sampled held states. Earlier common ability maintenance and later
world/creature simulation remain their own operations.

The late primary previous-bit **0x5515a6** is normally written at **0x8ee53**,
and the late class/dodge previous bits **0x5515a7 / 0x5515a8** at
**0x8fa1f / 0x8fa2d**. The UI jump skips these stores as well. They retain
their prior values while this gate is taken; do not globally resample or
clear all action-edge history merely because the panel is open. A later
eligible update can therefore observe a delayed held/edge condition.

### 12.3 Earlier jump, ultimate and run/walk behavior

**0x8ae50** calls movement **0x9c050** at **0x8bdbb**, before the late UI
gate. In movement, the 0x1014f0 result at **0x9c67f** makes its
**dodge-wanted boolean false**. That boolean would otherwise require held
dodge, ground contact, exactly zero dodge timer and sufficient stamina,
before nonzero intent can launch the moving roll. This boolean is **not a
sprint-enable flag**. Ordinary keyboard intent and the later jump/hold-jump
steps continue when answers alone make the UI predicate true.

The jump checks around **0x9d203 / 0x9d5e2** and its previous-state store
at **0x9d692** remain reachable. The movement routine's class-key-modified
jump rules also remain reachable, including its class-dependent boosted
jump and warrior ability **0x9f** selection where their normal conditions
hold. Consequently it is incorrect to force the class held byte false
globally to implement the late class-skill UI gate.

The ultimate rising-edge test is around **0x8c4fe–0x8c61f**, before 0x8ee23.
It still tests the logical ultimate binding and its own skill/usability
conditions, and updates previous ultimate byte **0x5515a2** at **0x8c61f**.
Visible answers by themselves do not suppress it. For a pad, whether Y
supplies that logical binding is decided by the separate UI routing, not
by this ultimate consumer.

The ordinary run/walk update at **0x8be3d–0x8be7c** is also before the late
UI gate. It normally sets player **+0x128 bit 6** while the Walk binding
**G+0x1a7d** is clear and clears that bit while Walk is held. The existing
airborne ability-0x78 exception instead uses stamina and held Jump. The UI
predicate is not added to either rule. Class-skill starts such as
**0x50 / 0x51 / 0x5f / 0x53** belong to the gated late class-skill handling;
they must not be conflated with that ordinary run/walk flag.

In the enclosing frame, the call to 0x8ae50 is at **0xa9b2c**, before the
movement-dismiss/answer-visibility block at **0xab283**. A movement input
which dismisses a previously visible answer panel later in that frame can
therefore still encounter the answer-visible action gates during the
earlier player pass. Do not retroactively apply the later hidden state to
that earlier action decision.

### 12.4 Where pointer hits actually matter

The physical left-button/no-GUI-hit condition in the opening portion of
0x8ae50 is for **dropping the cursor-held inventory item** while inventory
is visible; it is not the attack-stage UI predicate. The mouse-camera drag
exception in §2 also uses **0x33f1e0**. Neither condition narrows the global
0x8ee23 logical-action gate. Answer selection itself remains the separate
visible-panel release consumer in §5.6, using its last-drawn hovered row.
