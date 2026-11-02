# Steam ordinary interface cache-enable callers

Independent native-only bounded reachability supplement. Sources are
Game/interface initialization0x7ea80/0xf9ac0, the reached ChatWidget
constructor0x26a2e0, base Widget/node construction and copying, and the
ordinary tagged document readers. No engine/tests/exporter, owner
files/data/captures, reconstruction or game execution was used. Existing
specifications remain unchanged.

## 1. Proven fixed game-side enable: ChatWidget

Ordinary Game construction **0x7ea80** initializes its embedded Interface
at **Game+0x118** through0xf8690 and subsequently calls **0xf9ac0** on
that same Interface. Within the reached interface startup:

- **0xffdf7 →0x33cd30** creates the empty node supplied to the chat
  constructor. This is a newly created node, not a copied owner template.
- **0xffe0d →0x26a2e0** constructs ChatWidget on it. The constructor
  calls base Widget0x328ff0 at0x26a347, initializes its chat state/font,
  and **at0x26a440 sets bit0x40 in its own unsigned32 Widget+0x150**,
  preserving the other bits of that flag word.
- **0xffe13** stores that Widget at **Interface+0x280**, equivalently
  **Game+0x398**. Startup subsequently requests its frame through0x32c9a0.

This positive cache-enable write is independent of any authored
Widget.flags property. The receiver is ChatWidget itself, not its node,
the GUI engine or the separate chat editor-active byte. It does not set
node flag0x400. Chat identity/layout remains
[chat presentation §1](steam-chat-feat-presentation.md).

The ordinary GUI engine initially has option+0x288 mask0x02 enabled,
as specified in [GUI options §1](steam-plasma-options-and-shape-bounds.md).
Thus this constructor supplies a concrete requester for the Widget-cache
branch; actual traversal, visibility, retained-surface availability and
dirty/redraw admissions remain
[surface caches §§1–5](steam-plasma-surface-caches.md). Neither this bit
nor the engine option means an unconditional redraw on every frame.

## 2. Library flag propagation is a separate provenance

Fresh base Widget0x328ff0 starts flags+0x150 at0; fresh node0x32ee60
starts flags+0x80 at0. These constructors do not enable the respective
cache bits on every ordinary object.

The following reached library mechanisms can preserve or supply either
cache bit when their actual source/input contains it:

|Library path|Confirmed destination and provenance|
|---|---|
|Shared tagged Widget-property reader0x386560|The `Widget.flags` property is read as a four-byte value and stored as the Widget's flag word+0x150 at0x386be3. The reader does not force bit0x40.|
|Tagged node reader0x382f00|The `Node.flags` value is stored to the newly constructed node+0x80 at0x3836b3. Its separate loader-option operation adds flag0x08, not0x400. The special file-root behavior remains the existing document-loader contract.|
|Actual base Widget copy0x329210|Copies source flag word+0x150 at0x3293a7. This preserves a supplied0x40; it does not create one independently of the source.|
|Shallow node instance0x333890 /deep node copy0x3308e0|Copy the source node flag word+0x80 at0x33391a /0x3309ea. Their source may contain0x400; copying does not prove that a game caller supplied that bit.|

Document loading and instancing are used by ordinary interface startup,
but this audit does not inspect owner documents or assign numeric flags
to any of their templates. These library property/copy paths therefore
establish capability and propagation, rather than another demonstrated
fixed cache-enable value at a specific game widget/node.

The directly reached surface-preparation helper0x32d890 **tests** Widget
flag0x40 and can allocate/destroy its retained surface; it does not enable
that flag. Likewise draw0x330b20 consuming node flag0x400 is not itself
a producer of that flag. The engine option's initial7 supplies none of
the requesting Widget/node bits by itself.

## 3. Bounded remaining reachability limits

The startup/derived-constructor and directly examined ordinary interface
update paths establish the fixed ChatWidget write above. No second
receiver-proved fixed Widget0x40 enable or fixed game-side node0x400
enable was established in this bounded trace. This is **not** a claim
that the game never uses node caching, that an owner document contains
no such flags, or that every possible runtime caller has been excluded.

In particular arbitrary loaded flag words, source flags propagated by
copies, indirect/custom Widget calls, aliased/bulk stores and alternate
initialization/update paths remain outside the closed result. Numeric
0x40/0x400 matches on other receiver types do not establish either
cache-enable producer. No additional named library cache-toggle setter
or demonstrated game caller of one is asserted here.

Cache scalar defaults and byte-gated destruction remain
[cache startup §§1–3](steam-plasma-cache-startup.md); dirty/active/animation
lifetime remains [cache invalidation §§1–6](steam-plasma-cache-invalidation.md).
This supplement adds call provenance, without broadening their setter
census or inventing template flags, reset policy or all-frame redraw.
