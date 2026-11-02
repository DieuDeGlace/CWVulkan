# Steam GUI option initialization and standard shape bounds preparation

Independent native-only bounded follow-up to
[blur surfaces](steam-plasma-blur-surfaces.md) and
[surface caches](steam-plasma-surface-caches.md). Sources are native startup
0x134cc0, constructors0x38e100/0x339b70, traversal0x330b20, standard shape
factories/constructors and their virtual tables. No engine, tests, exporter,
owner data or game execution was used.

## 1. Initial option word and actual startup route

The successful graphics startup path **0x134cc0** allocates the concrete
GUI engine and calls **0x38e100 at0x135121**. That constructor calls base
**0x339b70 at0x38e12a**. The base constructor explicitly writes unsigned32
**7 to engine+0x288 at0x339f4e**. All three low bits therefore start set:

| Mask | Reached consequence |
|---|---|
|0x01|Retained by ordinary drawing; temporarily cleared/restored by the low-scale Widget-cache route described below. This audit does not assign a broader quality-option name.|
|0x02|Allows consideration of an own Widget cache with Widget flag0x40; its other selection/dirty/resource gates still apply. Also participates in the cached-Widget rectangle pick guard0x333f60.|
|0x04|Allows blur consideration; live Display flag1, positive radius and traversal admission still apply.|

The concrete constructor does not replace this option word. The reached
startup continues through font-manager setup and concrete initialization
virtual+8 →**0x38fb90**; no replacement of the option word was identified
on this route. Default7 does not force a cache allocation or blur layer
for a node whose own flags/data/admission do not request one.

Mask0x08 starts off. Its separate matrix/counter consequence, absence of a
reached reset and lack of a depth-enable change are specified in
[Plasma depth state §§1–3](steam-plasma-depth-state.md).

## 2. Direct write census and temporary bit0x01 lifetime

The native mapped-function direct-displacement census, with GUI receiver
identity checked, establishes four writes to this GUI field:

* **0x339f4e:** constructor assignment7.
* **0x330cc7:** during selected own-Widget cache redraw preparation,
  clear only bit0x01 when its live cache scale is below1.
* **0x331769 /0x331775:** after that cache's final concrete copy,
  set or clear bit0x01 to restore its saved entry value. These are
  alternative restoration branches, not an unconditional enable.

These cache operations preserve bits0x02/0x04 and the other word bits.
The existing [cache contract §§1/3–4](steam-plasma-surface-caches.md)
specifies selection, live requery boundaries and final-copy timing.
The census found no distinct GUI menu/config setter for cache or blur.
Other native objects and stack locations with the same numeric displacement
are not GUI option writes.

This is bounded evidence from decoded exported native function bodies and
the reached startup/cache paths. It is not a proof against untraced alias,
bulk-copy, external or injected writers, nor a user-facing setting mapping.
It establishes the initial word and the reached ordinary mutations without
assuming that a missing setter means an arbitrary future word is impossible.

## 3. Standard authored shape virtual+0x70 does not change the matrix

Traversal **0x330ff5–0x331009** invokes the own shape's virtual+0x70 with
the current GUI matrix before its projected-radius arithmetic. This is
shape bounds preparation, not a demonstrated transform adjustment for the
three standard serialized shape classes:

| Serialized class | Reached construction and virtual table | Virtual+0x70 |
|---|---|---|
|GenericShape|loader0x3827e0 →factory0x33cb90 →constructor0x37a890; table0x47c460|No-op0x0e8a20.|
|SmoothMeshShape|loader0x383e30 →factory0x33da20 →constructor0x3589a0; table0x47acc8|No-op0x0e8a20.|
|TextShape|loader0x384bf0 →factory0x33da70 →constructor0x367590; table0x47b130|0x368c60: prepares cached text bounds when a font exists and scalable flag0x01 is set.|

Text preparation passes the matrix to **0x352110**, whose font setup
**0x354600 →0x354cf0** reads it for scale-dependent font quantities.
It measures the current string and writes the TextShape's bounds minimum
**+0x300/+0x304** and maximum **+0x308/+0x30c**. It does not write the
supplied matrix. With no font or scalable bit clear, this hook returns
without that refresh. The existing text measurement/font rules remain
[shapes and text §6.7](plasma-shapes-and-text.md).

For these three classes, the blur-radius matrix after the hook is therefore
the same current GUI matrix supplied before it. Bounds can still differ
because the TextShape's cached bounds were refreshed. Resource drawing
objects with short graphics vtables are distinct from these node-owned
serialized shape objects; their vtables must not be substituted here.
Other dynamic classes, later mutations and all invalid/failure paths remain
outside this bounded standard-shape statement.
