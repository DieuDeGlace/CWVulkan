# Steam frontend panel backgrounds

This bounded native-only note covers ordinary saved-character cards, the
character-creation look panel and Options. Sources are the original Steam
executable/decompile, cited by RVA with image base0x140000000, and existing
independent Plasma/frontend specifications. No owner document, asset, capture,
engine or game execution was inspected. The title logo is outside scope.

## 1. Shared source, multiple live panels

The common native construction route is the named **blackwidget** template
from the loaded gui.plx document. Interface construction0xf9ac0 looks that
name up with0x332190 at0xfc5d4 and stores the source node at Interface+0x310
at0xfc5d9. The name literal is at0x441000. The source is detached from the
ordinary live hierarchy; it is used to create live instances. Document-source
and GUI-engine-root ownership are independently described in
[GUI root construction §§1–3](steam-gui-root-construction-lifetime.md).

The word blackwidget is a node name. It is not an instruction to generate a
black rectangle, nor does the lookup supply an RGBA constant or a fixed alpha.
The copied node's drawing, descendants, keyed Display and binding data are
the source of its authored appearance.

| Ordinary frontend part | Reached live construction |
| --- | --- |
| Saved-character selection screen container, GUI[3] | Empty node from0x33cd30 at0xf9e58, stored at Interface+0x18. This container itself has no supplied shape. |
| Each saved-character card and the extra New Character card |0x107440 instances Interface+0x310 beneath GUI[3], calling0x333890 at0x10752e, then constructs CharacterPreviewWidget0x26ea60 at0x107540 on that instance. |
| Character-creation screen container, GUI[0xb] | Empty node from0x33cd30 at0xf9f14, stored at Interface+0x58. This container itself has no supplied shape. |
| Character-creation outer look panel, GUI[0xd] |0x333890 copies Interface+0x310 beneath GUI[0xb] at0xfcd3b; its live node is stored at Interface+0x68 at0xfcd40. |
| Options, GUI[0xa7] |0x333890 copies Interface+0x310 at0xfc96e under the GUI-engine root; OptionsWidget0x287050 is constructed on that instance at0xfc992 and stored at Interface+0x538. |

Thus these are separate live copies of a common authored source, not one
shared translucent overlay painted once behind all three screens. The
saved-character screen has one copied panel per card, including its null
entry; the otherwise empty screen container does not supply a full-list
rectangle. Other controls have their own template art.

## 2. Wrapper attachment and content ordering

The card and Options constructors call BaseWidget0x268a70, which attaches the
new derived Widget to the receiving node and makes the root Transformation
and Display unique when necessary through0x335830/0x3356d0. This preserves
the copied values; it does not replace the Display with white defaults or
replace the source drawing with immediate background geometry. The root-only
Widget-binding transfer and preservation of descendant Widgets are detailed
in [character-list construction](steam-character-list-input-and-frame-init.md)
and [frontend characters §2.3](frontend-characters.md).

Character creation has a separate content node. After copying its outer
blackwidget,0xf9ac0 copies that panel's Transformation and Display through
their virtual copy entries and installs the copies with0x334d30/0x334760.
It constructs CharacterStyleWidget0x26fda0 on a newly created empty node at
0xfce0d. Then0x32f6e0, called at0xfce43, finds nodes in the outer copy whose
name matches the source blackwidget name and appends the style Widget's node
there. The helper reaches node-child append0x32f640 and fits the content
Widget to the matched node's existing effective frame. It does not move the
outer panel's drawing into the new empty style node. Consequently the
outer copied art and inner custom text/color controls remain distinct draw
participants. Any additional same-name descendants are an owner-hierarchy
input, not assumed absent here.

The inspected foreground hooks are CharacterPreviewWidget0x26eba0,
CharacterStyleWidget0x271030 and OptionsWidget0x288c50. They draw their
specified text, portrait or interactive controls. These hooks do not issue
a separate common fixed-RGBA background rectangle. In particular the
CharacterPreviewWidget's creature rendering and subsequent Plasma restore
through0x349c30 are not the background source.

Ordinary node traversal0x330b20 submits an admitted node's own shape before
its own Widget draw callback. Its ordinary child/Widget callback order then
follows the Widget flags, as specified in
[Plasma scene §2.3.1](plasma-format-and-scene.md#231-draw-traversal-0x330b20).
Therefore the copied panel's own shape can precede the custom labels or
portrait without an extra rectangle call in those hooks. Child drawings and
their order remain the actual copied hierarchy's data. Clipping, blur and
caching have their separately specified traversal cuts; this note does not
substitute a generic child-before-parent rule for those cuts.

## 3. Exact color inputs and numerical boundary

The reached native panel constructors and the three foreground hooks do
not establish a universal background RGBA such as black with alpha0.5.
The actual background fill colors, gradient vertices, textures and their
parameters, own Display.fillColor, and any authored changes to those values
are **owner gui.plx data**. Their numerical values have not been read in this
audit. Even whether the copied source uses a single rectangle, several
drawings or texture-assisted geometry cannot be settled by the template name.

The independently verified color combination is nevertheless exact:

* For ordinary shape submission0x3913d0, the BaseColor input is the owning
  node's own live Display.fillColor multiplied componentwise by the
  current engine tint. The ordinary non-text tint is (1,1,1,1).
* The Plasma vertex shader multiplies the geometry's per-vertex RGBA by
  that BaseColor. Interpolation then supplies the pixel color. With no
  object texture and no mask, this is the output RGBA, including alpha.
* A parent's Display.fillColor is **not inherited** by descendants. The
  empty screen container does not contribute a second opacity multiplier
  to every card or label merely by being their parent.
* If the owner shape has texturing or clipping enabled, their additional
  texture-opacity and mask operations are those in
  [Plasma clip layers §§4–5](steam-plasma-clip-layers.md#4-geometry-color-and-texture-operands).
  They cannot be collapsed into an assumed constant panel alpha.

The display loader0x382130 and constructor0x3736c0 establish defaults of
**Display.fillColor=(1,1,1,1), strokeColor=(1,1,1,1), visibility1, clipping0,
blurRadius0 and flags0** when no authored value replaces them. These are
Display defaults, not a claim that blackwidget actually uses them or that
its geometry is white. The template instance carries the source values.
The relevant loader fields are listed in
[Plasma format §1.7.3](plasma-format-and-scene.md#173-display-0x382130-object-0x3736c0).

BaseWidget's separate text-color initialization and calls to0x269160 do
not recolor the background Display. For example0x269160 writes the four
text RGBA fields at BaseWidget+0x1c4..+0x1d0; it does not write node
Display.fillColor. The white/cyan/black-outline constants in the frontend
text hooks must therefore remain text inputs rather than inferred panel
RGBA or panel-opacity producers.

## 4. Display admission, animation and blending

The selection and creation containers are initially hidden in0xf9ac0;
their later ordinary screen switches change live Display.visibility.
Options' copied root is also initially hidden, then shown/hidden through
the same visibility operation0xa3a20. That helper's screen switch does not
establish a timed panel-opacity fade. Frame visibility remains subject to
the GUI root and all ordinary ancestor admissions.

The copied drawing/Display/Transformation may still have authored keyable
values and sequences. Startup traverses the GUI root through0x340d60 and
0x333820; the card builder0x107440 separately initializes the rebuilt card
container with0x333820. Generic enabled custom-Widget initialization does
not automatically play widget:enable. Copied descendant Buttons have their
own initialization/press/leave sequence behavior. These are data-driven
evaluations, not proof of a fixed blackwidget alpha or permission to ignore
its authored current values. The normal per-step update and live-slot rules
are [Plasma animation and Widget initialization](plasma-format-and-scene.md).

On the ordinary Plasma pipeline, construction0x38fb90 and binding0x38e800
enable target0 blending. **RGB uses SRC_ALPHA and INV_SRC_ALPHA, ADD;
alpha uses ONE and INV_SRC_ALPHA, ADD.** All RGBA channels are writable.
There is no extra shader premultiplication before these factors. For an
ordinary untextured/unmasked black panel with a specified source alpha,
the already rendered scene therefore contributes through one minus that
source alpha. The actual source alpha still comes from the inputs in §3,
not the blend-state name. Exact depth/raster, offscreen target and mask
state remain in [Plasma clip layers §6](steam-plasma-clip-layers.md#6-blend-depth-raster-and-unresolved-sampler-inputs)
and [Plasma depth state](steam-plasma-depth-state.md).

## 5. Bounded conclusion and limits

The native evidence establishes the shared template provenance and separate
live copies for these normal panels, the empty screen-container distinction,
the foreground/background traversal relationship and the exact color/blend
inputs. No additional common fixed-color rectangle or universal opacity was
found in these reached construction and foreground draw paths. This is not
a global absence claim about other GUI callbacks or owner-authored children.

The remaining numerical appearance inputs are the actual owner source
drawing colors/textures and its live keyed Display/style state. Their values
cannot be recovered from the native string blackwidget, the panel's requested
frame size, text colors or a screenshot-derived guess. No uninspected owner
geometry, animation duration, fill alpha, sampler value or duplicated template
child is asserted here. This note does not re-audit the logo, all menus or
unrelated WORLD overlays, and does not modify the earlier stable specs.
