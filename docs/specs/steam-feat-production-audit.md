# Steam feat records: producer audit and supported behavior

The complete received-event display, chat colors, floating-text projection/
age/fade and sound are now independently specified in
**steam-chat-feat-presentation.md §§3–4**. That presentation does not resolve
the producer/payload boundary documented below.

Independent clean-room audit of the original native executable, its exported
call references, assembly and constant data. No engine, tests, exporter,
third-party reconstruction or original game execution was used. Analysis
image SHA-256:
**7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244**.
All addresses below are RVAs in that image. This supplements
`remaining-runtime-gaps.md` §2 with a further audit, not a newly observed award.

## 1. Result and implementation boundary

**No native gameplay feat-award producer, award trigger, award authority or
semantic payload definition was identified.** The recognized network-node
construction graph contains only empty-list creation, copying existing
records and decoding received records. The separate numeric-dialogue
descriptor route does not generate a network feat and has no identified
ordinary-gameplay producer of its descriptor opcode.

The verified positive behavior is transporting and displaying a supplied
opaque record. It does not establish an item/XP/coin reward or a mission-
completion notification. There is no evidence here for a new quest-award
packet, an extra event on mission completion, a feat-kind enumeration or
specific generated descriptor values. The exact scope and remaining limit
of the static absence evidence are stated in §5.

## 2. Complete recognized network-node construction paths

The feat list uses a **56-byte allocation**, comprising two 8-byte list links
and a 40-byte record. Its allocator **0x3c4d0** initializes the links; it does
not initialize a recipient or award descriptor. The original direct call
references and their assembly argument setup classify all six callers:

| Caller / call site | What it creates |
| --- | --- |
|0x36a90 /0x36aa2|A node whose record fields are copied from an existing source record|
|0x37080 /0x37092|The same occupied-field copy, in the other list instantiation|
|0x3ac90 /0x3ae07|An empty event-bundle feat-list sentinel; element count is zero|
|0x7e130 /0x7e147|An empty standalone feat-list sentinel; element count is zero|
|0xf1540 /0xf156c|An empty destination sentinel, followed by copying an existing source list|
|0xf5520 /0xf5edc|An empty temporary event-list sentinel, alongside the other temporary event lists|

**0x36a90** has one direct caller: range copy **0x38540 at 0x38593**.
That range copy is called only by list-range insertion **0x39180**, at
**0x391b2/0x391d7**. Its two recognized callers are packet reception/transfer
**0x3e450 at 0x3f26c** and the frame's incoming-list transfer
**0xa5a50 at 0xa885c**. These paths append existing records; they do not choose
an award or fill a new descriptor from mission state.

**0x37080** has two direct callers: received-list decoder
**0x3a130 at 0x3a1ec**, and range copy **0xefd00 at 0xefd53**. The decoder reads
a signed 32-bit count and 40 received bytes for each record, then inserts a copy.
The range copy's three direct callers are **0xf1540 at 0xf1586**,
**0xf33b0 at 0xf374d**, and **0x14b160 at 0x14b553**. The latter two distribute
existing event-bundle contents to peer/client queues. Neither fills payload
fields or applies a feat-specific award predicate.

The world/frame boundary is explicit: frame0xa5a50 copies the incoming
**G+0x1a10** list into **world+0x130** through 0x39180. A network source record
is therefore sufficient to explain a nonempty world list. Seeing a record
there would not by itself locate a native gameplay award producer.

This pass also checked the other recognized constant 56-byte allocation sites
rather than treating allocation size alone as record identity. The other
two-link list allocators **0x3c230** and **0x26a640** serve string-bearing
records: respectively an ID plus wide string, and a wide string plus three
color bytes. Their record constructors **0x36d30 /0x26a150** demonstrate
different payloads. The other inspected 56-byte node allocators use a
three-link tree header, not this feat-list record. An allocation-size match
does not establish another feat producer.

No stored full 64-bit code pointer to 0x36a90,0x37080,0x3c4d0,0x38540,
0xefd00,0x168120,0x15ce50 or 0x16c0a0 was found in the analysis image. Assembly
references to the relevant constructor/decoder entries supplied no additional
address-taking path beyond the recognized direct calls.

## 3. Payload construction that is actually observable

The 40-byte network record begins with the **recipient creature ID**, an 8-byte
value. Its trailing 32 bytes are an opaque descriptor. Record copy routines
**0x36a90/0x37080** read and write these occupied widths:

| Record offset | Occupied width / established role |
| --- | --- |
|+0x00|8 bytes: recipient creature ID|
|+0x08,+0x09|One byte each; semantic meanings not established|
|+0x0c,+0x10|Four bytes each; semantic meanings not established|
|+0x18|Eight bytes; semantic meaning not established|
|+0x20,+0x24|Four bytes each; semantic meanings not established|

The two gaps **+0x0a..+0x0b and +0x14..+0x17** are not copied by these field-wise
constructors. A wire record still occupies 40 bytes. Decode/transport of a raw
record does not prove that all its padding bytes were initialized by an award
producer, nor do those gaps encode an identified string or additional field.

The only more specific descriptor constructor traced here is an **input
decoder**, **0x15ce50**, for numeric dialogue opcode 11. It requires eight
remaining signed 32-bit operands before writing any descriptor field. Relative
to the 32-byte descriptor, it stores operand 1's low byte at+0, operand 2's low
byte at+1, operand 3's four bytes at+4, operand 4's four bytes at+8, and
operands 7/8 at+0x18/+0x1c. For +0x10 it sign-extends operand 5, shifts its 64-bit
bits left 32, and bitwise-ORs them with **sign-extended operand 6**; there is no
unsigned 32-bit mask on the latter. Thus a negative operand 6 fills the upper 32
result bits with ones. The native loads/composition are
**0x15ceef–0x15cefe**. This fact supplies exact accepted-input construction;
it supplies no gameplay names for those fields. The decoder does not write
descriptor padding +2..+3 or+0x0c..+0x0f.

## 4. Numeric dialogue does not supply the missing award route

In numeric compiler **0x168120**, opcode 11 invokes0x15ce50 at
**0x168829**, then appends the descriptor through **0x16c0a0 at 0x168868**
to the last dialogue page's **+0x178** descriptor list. If necessary it first
creates a page. The append copies the descriptor through **0x6e550** into
a separate 48-byte node. It does not construct the 40-byte network record,
add a recipient ID, or append to world+0x130.

The page constructor **0x15c380** makes this descriptor list empty; page
copying preserves it, and destructor **0x82630** empties/frees it. The page
activation routine **0xb40d0** consumes the item actions and the discovered-
cell/mission-mark lists at+0x190/+0x1a0, but **does not consume page+0x178**
to award or transmit a feat. This closes the proposed page-to-network route
even for a descriptor supplied to that accepted numeric opcode.

There is one recognized direct call to numeric compiler 0x168120:
**0xb40d0 at 0xb5283**. Its source stream contains opcode 5, the five operands
of the already-received category 24 item, and speech 0x85
(`you_received_the_item`). It contains **no opcode 11**. Item operands are
consumed as operands; an operand with numerical value 11 does not turn into
a descriptor opcode. No stored full 64-bit pointer or additional assembly
address-taking reference to that compiler was identified. Consequently this
known reachable source does not originate descriptors in ordinary gameplay.
Ordinary string dialogue compilation through 0x1694e0 supplies no equivalent
descriptor/feat token.

## 5. Delivery, consumption and the precise unresolved boundary

An existing record can pass through the generic event distribution and
packet 7 list 11 machinery. Generic host/client delivery is not evidence that
the host has a specific feat-award rule. No award authority, recipient-
selection rule or mission predicate can be assigned without a producer.

Frame 0xa5a50's consumer compares the record's first 8-byte creature ID to the
local player. For a match it displays **FEAT!**, creates the existing 3000-ms
floating text, writes the chat message and plays sound 0x1f.
**0x935a0**, called at **0xa8a04**, ignores the descriptor and always formats
that same literal. It does not decode the unknown fields, grant inventory,
add XP/coins, or complete a quest. See `remaining-runtime-gaps.md` §2 for
the exact visual presentation constants.

**Verified absence within the traced boundary:** no feat-node creation path
above introduces a gameplay descriptor; absent an existing source or a
received network record, those empty-list/copy paths cannot originate an
element. The identified numeric-dialogue caller never emits opcode 11, and
even that opcode's stored page descriptor has no award consumer in 0xb40d0.

**Limit:** this is a complete classification of the recognized construction
and candidate dialogue paths, not a formal proof against every possible
unrecognized indirect or inlined memory writer in the executable. The
evidence is consistent with dormant/unfinished feat origination, but that
label remains an inference. No runtime award record or originating call
stack has been captured in this audit, so payload meanings remain unknown.

If stronger evidence is needed, the useful bounded oracle is the **first
non-sentinel insertion into world+0x130** or the corresponding outgoing
event-bundle feat list in an unmodified supported game: capture its call
stack/RVAs, 40 raw bytes, recipient ID, source-list identity if copied, and
host/guest plus modification provenance. A received-list copy is not an
origination observation; trace its source backward. The six allocator
callers above and record-copy sites 0x36a90/0x37080 distinguish those cases.
No game launch was performed or is required for the static findings here.
