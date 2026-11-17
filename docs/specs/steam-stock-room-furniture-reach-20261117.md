# Steam stock room furniture selector reach

Original-only source-side contract. The scope is the stock room pass in the
generator at RVA 0x2FF6F0 and the selector namespaces it actually dispatches
for dungeon, fortress, and building rooms.

## Supported image and evidence

The local x86-64 Steam executable is 5,939,072 bytes. The stock
`game-files/cubeworld.exe` and `binary/cubeworld.original.exe` copies both
hash to SHA-256
`a1efbbd41eeb8df08395c4d6511194d416cb70a14054b2a2000b9f5cdbcfa65b`.
The analyzed image is `binary/cubeworld.original.exe.unpacked.exe`,
5,757,952 bytes, SHA-256
`7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244`.
Its preferred image base is 0x140000000. All addresses below are RVAs in
that unpacked image.

The original function exports establish the data flow, with the assembly
export and unpacked PE bytes checked for the room gate, constructor and
consumers. The building-style branch table at 0x67F18, wall dispatch table
at 0x6C2C4 and wall-object dispatch table at 0x6AEE4 were read from the PE.
Direct reference coverage comes from the original call index, the original
assembly export and an independent scan for relative call and jump targets
in executable PE bytes. The checks agreed at the scoped instructions.

## Scope of the stock producer

The generator's furniture gate begins at 0x3124E8. It constructs a fresh
room placement context at 0x312670 through 0x675C0, then immediately makes
the single four-side call at 0x3126A8 through 0x69860. Cleanup releases the
context's lists at 0x312DB2 through 0x2F9830. These are the three normal
uses of that context's stack address in the generator. The later painting
and room operations in this span use other context addresses.

The furniture gate runs inside the floor loop. The loop also checks
nonnegative floor eligibility through 0x2B8B10 at 0x307B9B before reaching
this region. The gate works on a parcel whose two local indices are each
in the inclusive range 0 through 3, checked at 0x302616–0x302632. It skips
a negative floor
for the local site IDs 0x27, 0x56, 0x5E and 0x5F from 0x69C60. The mask is
at 0x308C67–0x308C86 and the floor check at 0x3124F2–0x3124FB. The call
to 0x2D82C0 at 0x312516, with floor argument zero, must return greater
than −32. The second byte of the room classification returned by 0x2A8290
at 0x30264C governs the next gate. Values 2 through 5 suppress all negative
floors. All nonnegative floors require the positive result flag from
0x25F540 evaluated at 0x3050A4. With any other classification byte, a
negative floor that passes the earlier gates reaches the constructor
without the positive-result check. The site ID, floor and classification
are repeatable inputs to these decisions. They are separate from the CRT
draws in the placement pass.

The constructor receives the parcel coordinates, signed floor index and
placement colors. It initializes the room view at the parcel origin in
16-cell units with a 16 by 16 horizontal extent. The signed floor index
is stored at room offset 0x138. Each list contains four-byte selector
values in append order. A list's three eight-byte fields hold its begin,
end and capacity-end pointers. At 0x67630–0x6768E all five lists start
empty.

| List | Room offsets for begin, end and capacity end | Reader |
| --- | --- | --- |
| Wall pieces | 0xC0, 0xC8, 0xD0 | 0x6BEF0 |
| Center pieces | 0xD8, 0xE0, 0xE8 | 0x696B0 |
| Alternate wall pieces | 0xF0, 0xF8, 0x100 | 0x6BEF0 |
| Wall objects | 0x108, 0x110, 0x118 | 0x6A9A0 |
| Ceiling objects | 0x120, 0x128, 0x130 | 0x68510 |

A normal append in 0x675C0 is either explicit or routed through 0x12A80.
The helper copies one supplied four-byte element and advances the end by
four bytes. 0xA640 can grow the vector and preserve the preceding items.
The constructor's constant operands or initialized local arguments supply
the values in the tables below. Appending a placed object to the generator
uses separate output records, for example at 0x6AEA1 and 0x68756 through
0x69B30, and leaves the selector lists intact. The room constructor has no
direct CRT random call for population. Its append operations are driven by
the native classifiers identified below.

### Nonnegative floors

The choice at 0x67B70–0x67F02 uses the local site ID from 0x69C60 at the
original parcel coordinates. A nonnegative floor includes floor zero.

| Site ID | Wall pieces | Center pieces | Wall objects | Ceiling objects |
| --- | --- | --- | --- | --- |
| 0x23 through 0x26 | 2, 4 | Empty | 4, 1 | Empty |
| 0x2D | 0 | Empty | 4, 1 | 1 |
| Any other site ID | Empty | Empty | Empty | Empty |

The 0x2D branch begins at 0x67B97. The 0x23 through 0x26 branch begins at
0x67D86. The alternate wall list stays empty on both and on the default
exit.

### Negative floors

The constructor queries the four-byte classification via 0x2A8290 at
0x677A2. Byte positions are counted from the low-address byte as byte 0.
When byte 2 equals 2, the native path at 0x677A7–0x6784F appends wall
pieces 2 then 5 only when byte 1 equals zero. It appends wall object 0
only when byte 0 equals 1. The center list is the single entry 4 in this
branch. The ceiling and alternate wall lists stay empty.

When byte 2 differs from 2, the constructor uses the region coordinates
from floor division of the two parcel coordinates by 64. It obtains the
region flag through 0x36160, the region classification through 0x2B99D0
and the resulting building-style ID through 0x2AA270. The calls are at
0x678BA, 0x678CF and 0x678E9. These are the building-style outcomes at
table RVA 0x67F18 and the shared tail at 0x67AD2–0x67B66.

| Building-style ID | Wall pieces if byte 1 is zero | Additional center entry | Wall objects before the shared tail | Ceiling objects |
| --- | --- | --- | --- | --- |
| 0x28 | 4, 3, 5 | Empty | 4, 3 | Empty |
| 0x29 | 6, 7, 0, 5 | Empty | 4 | Empty |
| 0x2A | Empty | Empty | 4 | Empty |
| 0x2B | 1 | Empty | 0 if byte 0 is 1 | 0 |
| 0x2C | Empty | 0 | Empty | 0 |
| 0x2D | 4, 1, 5 | Empty | Empty | Empty |
| 0x2E | 4, 2, 5 | Empty | Empty | 0 |
| 0x2F | 4, 0, 5 | Empty | Empty | Empty |
| Other style | Empty | Empty | Empty | Empty |

Every row in this table ends with center entry 4. Thus 0x2C yields center
entries 0 then 4 and every other row yields only 4. When byte 0 is 1,
the shared tail appends wall object 1 after the wall-object entries in the
table. For example, style 0x2B with byte 0 equal to 1 yields wall objects
0 then 1. A nonzero byte 1 makes the wall-piece list empty in all table
rows. The alternate wall list stays empty throughout.

## Four-side pass and interior traversal

At 0x312676–0x3126A8 the native caller passes the same inset as both local
horizontal starts. Each span is twice the difference between 8 and the
inset. The vertical base is the generated floor height. The vertical
argument is the difference between the generated top reference and the
base, minus 2. The four-side routine adds that argument to the base once
to recover its exclusive upper wall height. Native calls may already
have been gated out before reaching this pass.

There is one CRT draw for each side, in native direction order 0, 2, 1,
3, at 0x6987D, 0x698FC, 0x6995B and 0x699C4. An even result calls the wall
piece selector and an odd result calls the wall-object routine. Each side's
work finishes before the next side's draw. The midpoint uses the room
start plus half the corresponding signed span rounded toward zero. The
far cell is the start plus the span minus one.

| Direction | Wall-piece anchor on even draw | Wall-object anchor on odd draw | Wall-piece mirror byte |
| --- | --- | --- | --- |
| 0 | x midpoint, near y | same | 0 |
| 2 | x midpoint minus one, far y | x midpoint, far y | 1 |
| 1 | near x, y midpoint | same | 0 |
| 3 | far x, y midpoint minus one | far x, y midpoint | 1 |

Then the pass visits the half-open horizontal rectangle in increasing x,
and increasing y within each x, at 0x69A19–0x69B0A. A point whose two
signed remainders on division by five are both 3 gets one CRT gate draw at
0x69A68. A result divisible by five invokes 0x68510 at 0x69A95 with the
upper height. A point whose two signed remainders are both 1 gets one
gate draw at 0x69ABC. A result divisible by five invokes 0x696B0 at
0x69AEE with the upper height and floor base. Other points get zero gate
draws. The gate draw still occurs when the called routine's list is
empty. Child draws are completed before the next eligible point.

### Wall pieces

0x6BEF0 returns immediately for an empty wall list. With a nonempty list
it first reads the face-oriented clearance strip. The local lateral
positions run from −2 through +2, one at a time, with local depth −1 and
heights from base plus one up to the exclusive upper height. At each
lateral position the heights are traversed in increasing order. The block
must have at least one set bit under mask 0xFD in byte 4 and a set bit
under mask 0x02 in byte 5. Any failed strip cell returns before selection.
The face view is set up via 0xBAA80 at 0x6BF82 and read via 0xBC340 at
0x6BFB5.

At 0x6BFE0–0x6C017 the original anchor column is tested in the parent
room view from base upwards. A candidate height must be below the upper
height, have all bits clear under mask 0xFD in block byte 4, and have
either bit 0x40 clear or bit 0x80 set in block byte 5. The first passing
height is the placement base. No accepted candidate means zero wall-list
draws. At 0x6C02F the accepted path consumes one CRT draw and indexes
the primary wall list by its remainder on division by the element count.
A one-entry list still consumes this draw.

The optional alternate check tests the list at offset 0xF0 at 0x6C046.
With entries it would take an extra draw at 0x6C056, then replace the
primary pick with an alternate-list pick drawn at 0x6C08A only when the
extra draw is divisible by five. In the covered normal room lifetime the
alternate list is empty and both of these draws are skipped. The wall
handler receives the accepted anchor and height, the original exclusive
upper height, direction and mirror byte. It is invoked before the next
side draw.

| Wall selector | Placement target RVA | Stock population at 0x675C0 |
| --- | --- | --- |
| 0 | 0x6A600 | Populated |
| 1 | 0x6B5C0 | Populated |
| 2 | 0x6A370 | Populated |
| 3 | 0x6A7A0 | Populated |
| 4 | 0x6A140 | Populated |
| 5 | 0x69CE0 | Populated |
| 6 | 0x6B7F0 | Populated |
| 7 | 0x6BB20 | Populated |
| 8 | 0x6B270 | No stock producer found |
| 9 | 0x6AF00 | No stock producer found |
| 10 | 0x6C2F0 | No stock producer found |

Values outside 0 through 10 reach the selector's default exit after
selection. These values have no placement dispatch in this namespace.
Of the stock wall-piece bodies, 5 and 7 have direct CRT calls. For 5,
0x69CE0 takes one parity draw per complete five-cell interval above the
accepted base within the supplied span. On an even draw it takes another
draw. When that second result is divisible by three it takes four more,
in order for two positional choices, rotation and variant. For 7,
0x6BB20 takes a height draw, then a divisibility-by-three draw. A nonzero
remainder takes a second divisibility-by-three draw, whose nonzero
remainder leads to two more draws. The direct sites are 0x69EA8,
0x69EF3, 0x69F5E, 0x69F76, 0x6A002 and 0x6A02D for 5, and 0x6BB8F,
0x6BDA4, 0x6BDF9, 0x6BE1D and 0x6BE72 for 7.

### Center pieces

0x696B0 returns immediately if its list is empty. Otherwise it consumes
one CRT draw at 0x696F8 and selects the element at the remainder on
division by the list length. There is a draw even for the common
single-entry list holding 4.

| Center selector | Placement target RVA | Stock population at 0x675C0 |
| --- | --- | --- |
| 0 | 0x690B0 | Style 0x2C on a negative floor with classification byte 2 other than 2 |
| 1 | 0x682A0 | No stock producer found |
| 2 | 0x68370 | No stock producer found |
| 3 | 0x68790 | No stock producer found |
| 4 | 0x68B00 | Default on negative floors, including after 0 in style 0x2C |

Other values have no placement dispatch after the selector draw.
Stock center target 0 makes two immediate CRT draws at 0x69133 and
0x69160 before geometry placement. Stock center target 4 first tests
whether its draw at 0x68B2F is divisible by eight. A nonzero remainder
ends this attempt. A zero remainder takes two further draws at 0x68BA6
and 0x68BD4 for independent x then y offsets, each the remainder on
division by five less 2. Geometry placement follows those draws.

### Wall objects and ceiling objects

The wall-object routine at 0x6A9A0 checks its own list at offset 0x108
before any draw. It takes a parity draw at 0x6A9E6 if populated. On even
parity it takes a draw at 0x6AA54 and selects by remainder on division by
the list count. Stock object 0 starts at the base. Stock objects 1, 3 and
4 take a further draw at 0x6AA81 for a starting height equal to base
plus 2 plus the remainder on division by four. The supported wall-cell
checks can stop placement after these draws. After validation, object 0
takes two more draws at 0x6AD13 and 0x6AD35 for object kind
0x55 through 0x58 and size. Validated objects 1, 3 and 4 use kinds 0x43,
0x3E and 0x3F with no further direct CRT draw. The native dispatch
also recognizes wall objects 2 and 5. Neither appears in the stock
population of this list.

The ceiling-object routine at 0x68510 checks its own list at offset
0x120 before any draw. It draws at 0x68553 on a populated list and
continues only on even parity. It probes down from the provided upper
height, then draws at 0x685C7 to select by remainder on division by the
list count. Entry 1 requires a further draw divisible by 20 at
0x685E3. The yaw draw at 0x68687 follows that check for entry 1 and
follows selection directly for entry 0. Entry 0 also takes the size
draw at 0x68705. Emitted object kinds are 0x48 for entry 0 and 0x49 for
entry 1. These object selectors belong to their respective lists.
In particular, ceiling object 1 and wall object 1 do not populate
center-piece selector 1.

## Checkpoints and limits

The following expected values are obtained by applying the original
branch constants and selection arithmetic to the stated inputs. They
are local expectations with a supplied draw sequence.

- A nonnegative floor in site 0x2D gives wall pieces [0], wall objects
  [4, 1] and ceiling objects [1], with both center and alternate wall
  empty. A valid even-side wall attempt still consumes one selection
  draw to reach 0x6A600. A successful remainder-1 interior gate at the
  same floor reaches an empty center list and consumes no selection draw.
- For a negative floor with classification bytes 0, 1 and 2 equal to
  1, 0 and 2 respectively, the wall list is [2, 5], center is [4],
  wall objects are [0], and ceiling and alternate wall are empty.
  Its wall selection draw with result 32767 reaches 5, after the wall
  strip and anchor both pass.
- For a negative floor with classification byte 2 other than 2 and
  resulting building style 0x2C, the center list is [0, 4]. At an
  eligible center point, CRT gate result 10 invokes the selector,
  result 17 picks 4 and result 7 fails the divisible-by-eight
  placement gate. Those three draws are consumed at that point and
  no center placement is emitted.
- With inset 2, the pass visits local cells 2 through 13 on each axis.
  The even-side anchors in draw order are (8, 2), (7, 13), (2, 8) and
  (13, 7). The odd-side anchors are (8, 2), (8, 13), (2, 8) and (13, 8).
  The interior contributes nine remainder-3 gate draws and four
  remainder-1 gate draws regardless of the results, before tallying
  any draws in invoked child routines.

The sole observed direct constructor and four-side calls are the
generator sites above. The four direct entries into 0x6BEF0 are the
side calls at 0x698F7, 0x69956, 0x699BF and 0x69A14. The sole direct
entry into 0x696B0 is 0x69AEE. The wall 8–10 bodies have sole observed
direct entries at 0x6C22F, 0x6C256 and 0x6C27D. The center 1–3 bodies
have sole observed direct entries at 0x6979F, 0x69784 and 0x69769.
The exported code had no other direct branch to these entries. The
matching literal references found in PE data were runtime unwind or
exception metadata and one unaligned byte match, with zero absolute
64-bit pointers to the scoped entries found in the scanned sections.

For the inspected normal stock lifetime, the observed wall-piece
producers supply exactly the range 0–7 across all branches and the
center-piece producers supply only 0 and 4. Wall 8–10 and center 1–3
are real dormant dispatch choices in this path. Their handlers consume
zero CRT draws on the covered stock path because the list population
cannot select them. Failure at the gate or the wall clearance scan
has its own earlier exit, and all reachable child draws retain their
positions in the stream. The selection arithmetic and branch evidence
cover these namespaces. They leave other direct decoration passes,
different native images, runtime patches and externally altered lists
outside the claim.

The placement counts above are the direct CRT calls of the identified
scoped routines. They do not establish an absolute CRT draw offset
for every room from world startup or a whole-world geometry fixture.
The parcel creation wrapper seeds CRT at 0x2AE13D on its cache-miss
path before the generator call at 0x2AE179. The generator also has
a parcel-coordinate reseed at 0x30293F before this region on its main
path, plus other draw sites in its room generation. Start any local
decision comparison with the CRT state already in effect at the
furniture pass. The producer coverage is a static trace. The population tables give the
ordered lists for the stated branch inputs when the constructor is
reached. World-coordinate reach of each input combination is unresolved.
The exclusion of wall 8–10 and center 1–3 holds for every constructor
outcome. Evidence is confined to the assigned original exports and PE images.

Author exposure consisted of the workspace entry instruction, isolated
assignment, source-side author policy, individually identified original
exports and the assigned local game binaries. Injected instructions
exposed zero additional project context. This handoff contains
original-only behavioral prose.
