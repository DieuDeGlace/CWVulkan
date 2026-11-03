# Steam frontend font-byte registry: bounded producer audit

This native-only supplement preserves all earlier font notes. It examines the
ordinary GUI constructor, font resolver/loader, nearby typed map operations
and reached pre-title document route in the original Steam executable. RVAs
use image base0x140000000. No engine, tests, exporter, owner file/capture,
reconstruction or game execution was inspected. A resource1.dat/resource2.dat
named-byte insertion producer is **still not established** by this bounded
evidence; the exact limits below are part of the result.

## 1. Fresh construction does not import a supplied byte map

Ordinary GUI-engine construction0x339b70 allocates a FontEngine and calls
constructor0x34c410 at0x339f86, then stores the returned pointer at
GUI-engine+0x48. The constructor establishes these distinct members:

| FontEngine member | Reached initial state |
| --- | --- |
|+0x08|Reference to the constructing GUI engine.|
|+0x10/+0x18|Named-byte map head and count; a fresh sentinel and count0.|
|+0x20/+0x28|Cached-font map head and count; another fresh sentinel and count0.|
|+0x30|Font-library handle initialized by0x395630.|
|+0x38/+0x40|Fresh empty directory list.|

The named-map sentinel allocation is0x34c700, called at0x34c453. It allocates
0x58 bytes, points its tree links back to itself and marks the sentinel.
The separate cached-font sentinel comes from0x34c6a0 at0x34c46e. Neither
operation supplies resource-name byte vectors. No copying, adoption or
transfer of a caller-supplied named-byte map occurs in this constructor.

The GUI engine also creates a node named fonts under its separate scene
bookkeeping. A scene node with that name is not the FontEngine named-byte
map. Its construction does not prove registration of any face bytes.
Likewise the ordinary startup directory registration through0x34c940
appends c:\windows\fonts; it does not provide resource1.dat/resource2.dat
byte entries.

These are construction-time facts. They do not prove that the named map is
still empty when a later document, callback or frontend Widget first loads
a font.

## 2. What the reached resolver actually inserts and reads

Resolver0x34d060 constructs a basename cache key, searches **FontEngine+0x20**
and returns an already cached pointer before another load. On a cache miss
its reached map allocation/insertion calls0x34bbc0/0x34be60 operate on
**+0x20**, then store the newly created font pointer in that cache entry.
The nearby generic tree insertion0x34c150 reached through0x34be60 is not
evidence of a named-byte insertion merely because it lies near FontEngine
code. Its receiver on this resolver route is the cached-font map.

The subsequent face-load call0x352df0 receives **FontEngine+0x10** as the
optional named-byte map. In the TTF/DAT route it searches this map by the
requested wide-string name. It consumes an entry only when the entry exists
and its byte-vector begin differs from end. The consumed layout remains:

| Named-map entry field | Meaning on the reached loader path |
| --- | --- |
| Node+0x20|Wide-string name key.|
| Node+0x40|Byte-vector begin.|
| Node+0x48|Byte-vector end.|
| Node+0x50|Byte-vector allocation end, used by cleanup.|

This loader is a lookup/consumer, not the missing registration producer.
The memory-face route0x353c80 passes the stored begin pointer and the
32-bit length derived from end-minus-begin to0x397720. A missing or empty
named entry falls through to the file-face route0x353860→0x397550.
Directory retries pass no named map. The established precedence and failure
behavior remain in [frontend font source §2](steam-frontend-font-and-logo.md#2-effective-source-cache-directories-and-unresolved-memory-producer).

Once the basename is cached successfully, a later named-map value does not
cause this resolver's cache-hit branch to reload the face. Conversely an
earlier document-driven request can determine the cached pointer before
the title's explicit request. This note does not assert that a registration
change actually occurs on either side of that cache boundary.

## 3. Generic document resources are a different object map

The document resource route0x344290→0x33cca0 does not by itself populate
FontEngine+0x10. In0x33cca0 the receiver is the GUI engine; the routine
constructs a resource object through0x36b6d0 and inserts its **object pointer**
into an entry in **GUI-engine+0x130**, using0x338410. Its value at node+0x40
is that resource-object pointer, not the named font map's begin/end byte
vector. Equal-looking names, node offsets or a0x58-byte allocation do not
identify these two typed stores as one registry.

This bounded trace does not establish a later bridge copying such resource
payloads into the FontEngine map. It neither invents that bridge nor claims
that no bridge could exist through an untraced callback or alias.

The already verified startup order remains unchanged:0xf9ac0 loads
icons.plx at0xfa2b2, gui.plx at0xfb59f and quest-tag.plx at0xfb613 before
constructing the title Widget at0xfb6bb. TextShape construction/rebuild can
resolve fonts synchronously during those loads; vector-font loading can
reach a nested document load. Those reached first-request/cache cut points
are in [pre-root font ordering §§1–4](steam-font-byte-registration.md).
The documents' actual font names and byte payloads were not inspected here.

## 4. Verified cleanup ownership, unresolved input ownership

FontEngine destruction0x34c540 first destroys the cached font objects, then
cleans up its remaining library/list/maps. The named map is cleared through
0x34caa0, called at0x34c63b; its node cleanup0x34c7c0 frees a nonnull
byte-vector allocation, clears begin/end/allocation-end and destroys the
entry key/node. Thus that cleanup treats the stored vector buffer as owned
storage. It is not merely a borrowed span ignored at destruction.

This does **not** establish whether an original registration caller would
copy supplied bytes, move a vector, allocate/decode a source or adopt a
buffer. No such caller/source was proved. It also does not establish all
replacement/erase operations, aliasing, synchronization or face validity
after an arbitrary external mutation of the registry.

## 5. Exact remaining production boundary

Within the bounded constructor/startup/font-resolution/document routes above,
there is no newly proved call that inserts resource1.dat or resource2.dat
bytes into FontEngine+0x10. The following remain unknown: the registration
caller if any, the source of its name and byte allocation, registration
timing relative to the earliest document-driven request, and any transfer
through a later alias, callback or external caller not closed by this trace.

The confirmed fresh constructor and the absence of a proved producer in
these paths do not establish global runtime absence, an empty pre-first-load
registry, file-only loading or a numeric byte-source identity. A request for
resource1.dat establishes a resolver key; it does not establish effective
face bytes, glyph coverage, capitalization or equality with another font.
No such values were inferred, and no earlier spec was edited.
