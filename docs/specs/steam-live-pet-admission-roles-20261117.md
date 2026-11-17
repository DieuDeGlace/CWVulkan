# Steam live pet admission and World roles

FINAL / STABLE / HELD

Static original Steam evidence only. The matching x64 image is
`cubeworld.original.exe.unpacked.exe`, image base `0x140000000`, SHA-256
`7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244`.
All addresses below are RVAs in that image. Role labels describe the proven
object and caller, rather than a name assigned by the original executable.

## The gate and its writers

The first gate of the player-pet pass at `0x2e0db0` is a 64-bit **string
length in bytes** at `World+0x210`. It belongs to the narrow string at
`World+0x200`; the stock strings below use inline storage and capacity is at
`World+0x218`. The comparison at `0x2e0e01` skips the entire first creature
pass when the length is zero. That pass is separate from the later work
beginning at `0x2e13c7`. It does not test a title, play, host or player marker
at this first gate.

- The common World constructor `0x29a130` sets capacity to 15, length to
  zero and the first byte to zero at `0x29a256–0x29a273`. It separately
  initializes the byte at `World+0x358` to zero at `0x29a276` and the local
  player pointer at `World+0x448` to null at `0x29a2eb`.
- The shared initializer `0x2b5f20` writes the seed at `World+0x220` and
  assigns the supplied string to `World+0x200` unless the argument is already
  that member. The assignment at `0x2b5f93` sets the length in the normal
  copy path at `0x0338ea`; an empty input sets zero at `0x0338bb`.
  These are the two observed direct stock call sites for the initializer:
  `0x07f279` for the graphical World and `0x0f1969` for the embedded Host
  World. Both populate the length before their regular updates.
- Startup constructs the graphical World at `0x136163`. The Game constructor
  `0x07ea80` retains its pointer at `Game+0x8`, initializes the requested seed
  at `Game+0x206c` to 123 and writes the five bytes of “world” into the
  requested string at `Game+0x2070` (string setup at `0x07ef6a–0x07efa9`).
  It passes those same values to the initializer at `0x07f26c–0x07f279`:
  this World's seed is 123 and its string length is **5**.
- The embedded Host constructor `0x0f17a0`, called at `0x07eb36`, constructs
  another World at `Host+0x8` (constructor call `0x0f17e1`). Host itself
  starts at `Game+0x6e8`. The Host supplies seed 123 and the six bytes of
  “ world”, including the leading space, at `0x0f194b–0x0f1969`. This
  World has string length **6** and initially has no local player pointer.
  Neither World's initialization is delayed until character selection.

## Which frame reaches the gate

| Stock route | World and expected admission state |
| --- | --- |
| Graphical title and character selection | This is the World at `Game+0x8` (a pointer). The startup string is “world” and its length is 5. The graphical frame `0x0a5a50` checks its requested seed and compares the full requested string to the World's actual values at `0x0a72ce–0x0a72ff`. If both match, it advances that World at `0x0a734c`, with any preceding substeps at `0x0a7328`. Menu/selection visibility affects bit `0x80` of the 16-bit field at `Creature+0x128` just before the check (`0x0a7242–0x0a72c7`). It does not substitute a zero World string length or bypass the pet pass in that block. |
| Ordinary solo play after selecting an existing character | The same graphical World and its existing local creature continue. The selection callback does not reassign the World string or require a host peer; the default matching seed and “world” string still admit the substep. With no connection transition, `World+0x358` stays zero. |
| Embedded Host authority | The separate World at `Host+0x8` (embedded, not a pointer slot) is stepped from `0x0f5520` at `0x0f692d`, with any preceding substeps at `0x0f6917`. The worker loop `0x083e60` runs while `Game+0x2068` is set and steps this Host when the client has no valid peer or is using the special local peer (`0x03e2d0`); it waits when the client has a different valid peer. The Host has a nonempty string even with an empty actor list. Receiving a valid peer actor is separate from establishing that string or stepping the Host. |

In both callers the common World step is `0x2da6c0`, which calls the pet pass
at `0x2da8fb`. When the graphical seed or name differs, that frame skips
the local World step at this site. The alternate selection helper
`0x08a500` writes the *requested* seed and string at `Game+0x206c` and
`Game+0x2070`, plus character-side copies; it does not assign
`World+0x200`. The two frame comparisons (including `0x0ad724–0x0ad745`)
are readers. Separately, the connect handoff `0x03d820` writes
`World+0x358` to 1 at `0x03db2d`; the socket/disconnect cleanup in
`0x0911e0` writes it back to zero on the relevant branches at `0x091249`
and `0x091309`. Those are changes to the mode byte, not writes to the
World's name length.

## How the v11 equipment reaches the local actor

1. The Game constructor opens `Save/characters.db` at `0x07f30c–0x07f32e`.
   The “num” record drives the creation of individual character entries in
   the selection list; those entries themselves are not added to the World
   creature list (`0x07f440–0x07f499`). The constructor separately creates
   the live creature with ID 1, puts it into the graphical World's roster
   and ID map through `0x29d0d0`, and makes the same creature
   `World+0x448` at `0x07f64e–0x07f657`. Its equipped pet slot begins empty
   (`0x04ce80`, `0x04d600`, `0x04d300`). If the “slot” record is present,
   the startup restore loads the selected record into the live creature at
   `0x07f71a` as well.
2. The visible **Start Game** action is menu choice 0 at `0x291210`.
   Its click handler `0x0965e0` opens character selection. The **Select**
   control is bound to `0x09f7c0` at `0x0fcba4–0x0fcbbb`. For an existing
   slot in range, that callback records the selected index, calls the
   character reader at `0x09f856` using the current graphical
   `World+0x448` as destination, and records the selection under “slot”.
   It loads into the creature already registered with that World, without
   substituting the selection-list creature. The **Continue** menu branch
   also calls this reader against the live actor at `0x0966b4`.
3. The standard reader `0x09bb20` looks up the decimal slot key, decompresses
   a found record, and passes the result to the character reader at
   `0x09bcda`. The character reader `0x147800` begins with the four-byte
   version field; the matching writer `0x149760` and its context initializer
   `0x147750` establish standard version 11. On a complete v11 record, eleven
   contiguous equipment records of `0xa0` bytes start at decoded offset
   `0x3e`. The eleventh is at decoded offset `0x67e`; `0x147c3a–0x147c44`
   passes it to the `0x1496d0` reader, which copies all `0xa0` bytes to
   `Creature+0x8b8`. The one-byte type is at decoded offset `0x67e`;
   the four-byte little-endian subtype is at decoded offset `0x682`,
   which lands at `Creature+0x8bc`. These are offsets in the decompressed
   character record, not file offsets. The reader sets `Creature+0x60`
   to zero and clears the runtime pet ID through its reset call
   `0x064a90`. A short equipment read does not copy a partial item.

The Host does not take its actor from the graphical World's `+0x448` or
directly from the character database. The separate peer receive path in
`0x0f4490` validates the player ID against the peer, finds its own Creature or
registers it at `0x0f487d`, then applies received creature fields from
`0x0f488b`. When present, the equipment block in that field update covers
`Creature+0x278` for `0x6e0` bytes (`0x139551–0x139565`, `0x138680`),
including the pet type and subtype. A peer actor admitted this way is in
the Host's creature list regardless of its null local-player slot. File
loading alone does not create that Host actor.

## After the first gate

The first pass iterates the World list at `World+0x40`. The player candidate
branch requires the byte at `Creature+0x60` to be zero and the equipped type
at `Creature+0x8b8` to be `0x14` (`0x2e0e34–0x2e0e45`). Carrying an item in
inventory and having a nonempty World by themselves do not qualify. A valid
candidate can be the graphical local creature or a Host peer creature:
the list traversal does not select candidates through `World+0x448`.

The pass first resolves the current pet ID at `Creature+0x9d8` in the
World's ID map at `+0x450`. If there is no resolving ID, it clears the ID
and scans the same roster for the first creature whose owner ID at
`Creature+0x1a0` matches the candidate's ID at `Creature+0x8`
(`0x2e0e4b–0x2e0edb`). The gate at `0x2e0ee2` then requires the
`World+0x358` byte to be zero for the qualifying player's pet update and
new creation branch. New creation additionally requires the update branch
not to have marked a change, a still-zero pet ID, and an action byte at
`Creature+0x68` outside `0x78`, `0x79`, `0x98`, `0x9c` and `0x9d`
(`0x2e111d–0x2e1145`). The spawn registers in this same World at
`0x2e11c8`, sets the new creature's role byte at `+0x60` to 6, and uses the
item subtype as the new creature's four-byte creature ID at `+0x64`
(`0x2e11e5–0x2e1208`). This branch does not independently gate on a
nonzero subtype.

## Boundaries

The default graphical title is not an empty-name World. An unmodified
placeholder does not qualify because its pet slot is empty, and a live
creature already loaded from a save is subject to the same tests even in
a title frame that advances the World. An ordinary valid v11 selection
with a pet equipped therefore supplies the input to the local simulation
pass as soon as that World steps. This is static reachability; it does not
establish the first visible pet frame, the result for a particular owner's
save, or successful publication for a particular peer.

The term “Host authority” here names the embedded worker and its separate
World. The “server” argument comparison at `0x135fec` alone is not
evidence of a separate executable or headless run with another writer.
This contract does not resolve alternate seeded-world activation,
unresolved indirect writers, connection timing, malformed input, new
character creation, or pet handling outside this first equipped-item pass.
