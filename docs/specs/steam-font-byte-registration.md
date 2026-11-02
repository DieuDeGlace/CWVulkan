# Steam pre-root document loading and font-cache provenance boundary

Independent native-only bounded follow-up to
[frontend fonts §2](steam-frontend-font-and-logo.md#2-effective-source-cache-directories-and-unresolved-memory-producer).
No engine, tests, exporter, owner files/captures or game execution was used.
This records verified loading order; it does **not** establish a named-font-byte
registration producer or effective resource1.dat/resource2.dat contents.

## 1. Documents precede the title root's explicit font selection

Native GUI initialization **0xf9ac0** reaches the following calls in order:

| Call site | Operation |
|---|---|
|0xfa2b2|Load **icons.plx** through0x3429a0.|
|0xfb59f|Load **gui.plx** through0x3429a0.|
|0xfb613|Load **quest-tag.plx** through0x3429a0.|
|0xfb6bb|Construct title root Widget through0x2910c0.|

The title constructor subsequently requests **resource1.dat** through
0x269130 at0x2911ba. Its request therefore does not precede all document-driven
font activity merely because it is an explicit filename assignment in the
title's own constructor.

## 2. The reached document-to-cache route

The document route **0x3429a0 →0x3809a0** reaches TextShape loader
**0x384bf0 at0x38115a**. That loader accepts the document's
**TextShape.fontName /TextShape.wfontName**, and forces the completed
TextShape's rebuild through virtual+0x08 at **0x384fd3**.

The reached rebuild **0x368b90** resolves the supplied font name through
**0x34d060**, retrying arial.ttf if that TextShape's requested font fails.
The resolver's basename cache is shared through the GUI FontEngine. A
successful document-driven request can therefore populate that cache before
the title's own explicit request. An existing basename entry is returned
before another source load, as specified in the linked font contract.

This is a **data-conditional route**, not an assertion that every loaded
document contains a TextShape or that any particular one requests
resource1.dat/resource2.dat. The owner documents and their font keys were
not inspected. Which names succeed, their cache order within those documents,
and the face bytes behind a successful entry remain unproved here.

## 3. Named-byte producer remains unresolved

The document font-name/rebuild route above is a **font-cache loading** route.
It is not a demonstrated insertion of a key/byte span into FontEngine's
separate named-byte map **+0x10**. No reached resource1.dat/resource2.dat
insertion producer was established by this bounded investigation.

That does not prove global absence of registration, an empty named-byte map
at first load, a disk-only effective face or a particular glyph appearance.
The existing resolver's cache/memory/file precedence and failure bounds
remain unchanged. Closing effective byte provenance still requires a proven
registration caller with key/span source, or separately permitted registry/
cache provenance evidence before the relevant first load.

## 4. Within-document construction and font-request order

The normal PLX reader **0x3809a0** consumes top-level chunks in stream
encounter order. **TextShape, SmoothMeshShape and GenericShape share one
increasing shape-index counter**, starting at0 for that reader; the index
is assigned as its chunk is encountered, before the corresponding loader
returns. It is not a shape-name sort, node traversal or draw order.

For a TextShape, dispatch **0x38115a →0x384bf0** creates that one shape
through **0x33da70 at0x384c87**, parses its properties, then forces its
rebuild through **0x384fd3 →0x368b90** before returning to the next outer
chunk. The constructor itself does not resolve a font before those properties
are parsed. A nested **TextShape.font** property container supplies that
same shape's name/style fields; it does not construct another TextShape.

Consequently increasing encounter-assigned shape-index order reproduces
the **outer document's TextShape first-request subsequence**, provided each
shape is constructed/parsed/rebuilt inline at its position. Creating every
shape first and deferring all font rebuilds is not the reached timing.
Fallback to arial.ttf occurs synchronously within the failed TextShape
request, before the loader advances. The shared cache retains entries from
earlier documents; starting each document with an empty font cache would
change the established multi-document order.

There is an important bounded nested-load case. **0x352df0** routes a
non-TTF/DAT font name to vector-font loader **0x3530b0**. That loader can
synchronously load a separate Plasma document through **0x3429a0 at0x353377**.
Any TextShapes encountered there make their requests during that nested
load, before the outer font request returns. Its shape indices belong to
its own reader, not the outer array. Therefore a flattened outer shape
array alone is not a complete global request schedule for this case.
The actual startup documents/font extensions were not inspected, so this
exception is retained rather than presumed unused from owner values.

Ordinary node references to already loaded shape-table entries are not a
second construction at each reference. The standard GenericShape rebuild
**0x37ad60** only marks that shape changed; it does not recursively construct
its referenced TextShape or resolve another font. The separate TextShape
clone route **0x367ea0** does construct/copy/rebuild a new TextShape when
actually invoked; this section does not insert such a call merely because
a reference exists. Legacy PLA reader0x3448a0 and arbitrary later clone/
custom callbacks remain outside this normal startup PLX ordering statement.
None of these order facts closes the named-byte insertion producer in§3.
