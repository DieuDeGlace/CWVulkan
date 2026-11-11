# Steam pending source selection and world reconfiguration

Status: FINAL / STABLE / HELD (source-side behavioural contract, code-free).
Date: 2026-11-10.
Scope: the supported Steam image only. Everything below is a description of
the original program's observable behaviour. Nothing here is implementation
advice.

Shorthand used for owning objects (each attribution was checked against the
original evidence; corrections are stated where the task shorthand was wrong):

- G = the Game object (one instance, 0x2288 bytes, created once by the program
  main routine at RVA 0x1355A0 and stored in a global).
- W = the World object the Game points to through the pointer at G+0x08. This
  is "the live World". It is created once by the main routine and handed to the
  Game constructor.
- E = the local player entity, reached through the pointer at W+0x448.
- D = CORRECTED ATTRIBUTION. D is not a frontend or character-database owner.
  D is a 0x300-byte heap block owned by the local entity and reached through
  the pointer at E+0x1528. It is the per-character persistent block that is
  written into and read back from the character save blob. The object that
  "publishes" a selection is the Game itself (routine at RVA 0x8A500), which
  writes the selection both into D and into G.
- C = the network client object embedded in the Game at G+0xD18.
- H = the in-process host object embedded in the Game at G+0x6E8. H embeds its
  own, separate World instance at H+0x08 (that is G+0x6F0). That second World
  is never the live World W.

All offsets are hexadecimal. "dword" = 32 bits, "qword" = 64 bits.
"Text object" = the 32-byte narrow-character string object used throughout the
image: 16 bytes of inline storage or a heap pointer at +0x00, a 64-bit length
at +0x10, a 64-bit capacity at +0x18 (capacity 15 means inline storage).

---------------------------------------------------------------------------

## 0. Headline finding (read this first)

In the supported image the pending selection is RECORDED but is NEVER APPLIED
to the live World by re-running the World seeding routine.

- The World seeding routine at RVA 0x2B5F20 is the only code in the image that
  stores the World's applied seed (W+0x220) and applied name (W+0x200).
- That routine has exactly two call sites in the whole image, and both are in
  constructors that run once at program start: the Game constructor (seed 123,
  name "world", on the live World) and the host-object constructor (seed 123,
  name " world" with a leading space, on the host's own embedded World).
- A selection made later (routine at RVA 0x8A500) only stores the requested
  seed and name into D and into G, removes every creature except the local
  player from the live World, and returns.
- Afterwards the per-frame Game update merely COMPARES the requested pair
  (G+0x206C, G+0x2070) with the applied pair (W+0x220, W+0x200) at two places
  and takes a different branch on inequality (section 6). It never copies one
  to the other and never calls the seeding routine.

This was established three independent ways: (1) the call-reference table of
the analysis database; (2) a raw byte scan of the entire unpacked image for
every relative reference and every absolute pointer to RVA 0x2B5F20; (3) a raw
byte scan of the code section for every instruction form that could store to
displacement 0x220 or reference displacement 0x206C. The reconstruction lead
agrees (it also has only the two start-up call sites).

Consequently "world reconfiguration on selection" in this image consists of
the stores and the creature purge described in section 4, plus the comparison
gates in section 6. What the running game looks like when the two pairs differ
is a runtime question this author cannot observe; see UNRESOLVED item U1.

---------------------------------------------------------------------------

## 1. The six fields

### 1.1 G+0x206C — requested seed

- Width and type: dword. Signed 32-bit integer (it originates from a signed
  integer text extraction and is printed with the signed-integer stream
  inserter; all uses in this contract are equality tests or plain copies, so
  signedness never changes a result here).
- Role: PENDING / REQUESTED.
- Writers (complete list for the image):
  - Game constructor, store of the constant 123 (0x7B) at RVA 0x7EF6A.
  - Selection routine, store of its seed argument at RVA 0x8A56C.
- Readers (complete list for the image):
  - Game constructor at RVA 0x7F26F: read back and passed as the seed argument
    of the seeding routine.
  - Game update at RVA 0xA72E1 and at RVA 0xAD72E: equality comparison against
    W+0x220.

### 1.2 G+0x2070 — requested name

- Width and type: 32-byte text object (length at G+0x2080, capacity at
  G+0x2088).
- Role: PENDING / REQUESTED.
- Writers: Game constructor (initialised empty, then assigned the 5 characters
  "world" at RVA 0x7EF99..0x7EFA9); selection routine (whole-string assignment
  at RVA 0x8A581, skipped only when the source text object is the very same
  object by address). Destroyed by the Game destructor at RVA 0x82068.
- Readers: Game constructor at RVA 0x7F26C (passed by address as the name
  argument of the seeding routine); Game update at RVA 0xA72F1 and RVA 0xAD73E
  (content comparison against W+0x200).
- Other instructions in the image that use displacement 0x2070 address
  unrelated stack frames or unrelated objects; they are not this field.

### 1.3 W+0x220 — applied seed

- Width and type: dword, same signed 32-bit value domain.
- Role: CURRENT / APPLIED.
- Writer (only one in the image): the seeding routine, RVA 0x2B5F76.
- Readers: world-list entry save routine at RVA 0xA2C5A (equality with a list
  entry's seed); Game update at RVA 0xA72D2 and RVA 0xAD728. World generation
  does not read this field directly: it reads the derived number tables that
  the seeding routine fills (section 2.1, step 6).

### 1.4 W+0x200 — applied name

- Width and type: 32-byte text object (length at W+0x210, capacity W+0x218).
- Role: CURRENT / APPLIED.
- Writers: World constructor initialises it empty (RVA 0x29A256); the seeding
  routine assigns it at RVA 0x2B5F93 (skipped only when the argument is the
  same object by address). No other writer exists.
- Readers confirmed in original evidence: world-list entry save routine at
  RVA 0xA2C47; Game update at RVA 0xA72EA and RVA 0xAD737.
- Unconfirmed reconstruction observation: the reconstruction additionally
  tests "applied name is empty" in several world and map routines. Those
  length-only reads (W+0x210) were not individually located in the original
  and are not asserted here as Steam facts.

### 1.5 D+0x2C8 — last-world seed stored with the character

- Width and type: dword, same signed 32-bit value.
- Role: a persisted RECORD of the last requested selection. It is neither the
  pending value the Game update tests nor the applied value.
- Writers: selection routine at RVA 0x8A532 (first store of that routine);
  character-blob reader at RVA 0x148842 (inside the routine at RVA 0x147800);
  the D copy routine at RVA 0x7FBB0 copies it from another D (RVA 0x7FEA7).
- Readers: character-blob writer at RVA 0x14A56E (inside the routine at RVA
  0x149760); the D copy routine.
- No generation, simulation or comparison code reads this field.

### 1.6 D+0x2D0 — last-world name stored with the character

- Width and type: 32-byte text object (length at D+0x2E0, capacity D+0x2E8).
  The D block ends with three further dwords at D+0x2F0, D+0x2F4, D+0x2F8 and
  is 0x300 bytes in total.
- Role: persisted RECORD, as 1.5.
- Writers: D constructor at RVA 0x7FF20 initialises it empty (RVA 0x80234);
  selection routine assigns it at RVA 0x8A560 (skipped only when source and
  destination are the same object); the D copy routine; the character-blob
  reader (the string that follows the seed in the blob).
- Readers: character-blob writer at RVA 0x14A574; the D copy routine.

### 1.7 Do not merge

- (G+0x206C, G+0x2070) is the requested pair. Layout: seed first, name 4 bytes
  later.
- (W+0x220, W+0x200) is the applied pair. Layout: name first, seed 0x20 later.
- (D+0x2C8, D+0x2D0) is a save-file record. Layout: seed first, name 8 bytes
  later.
- The world-list entry (section 5.1) is a fourth, separate holder: name text
  object at entry+0x10, signed dword seed at entry+0x30.

---------------------------------------------------------------------------

## 2. Routine at RVA 0x2B5F20 — World seeding (complete owner)

Range 0x2B5F20..0x2B67AF.

Inputs: W (first argument); seed, a dword taken from the low 32 bits of the
second argument; address of a name text object (third argument).
Output: none. No early return exists; every call runs steps 1 to 8 and then
evaluates the guard of step 9. No lock is taken or released inside the routine
and no role or connection state is tested other than the byte at W+0x358 in
step 9.

Order of effects:

1. Close the World's key/value store: the store object lives at W+0x3B8 and
   its open handle is the qword at W+0x3C0. If the handle is non-zero the
   store is closed through the database library and the handle is set to 0
   (helper at RVA 0x66BE0).
2. Store the seed, dword, at W+0x220.
3. Assign the whole argument name to the text object at W+0x200, unless the
   argument is that same object.
4. Seed the C runtime pseudo-random generator with the seed (the 32-bit value
   is passed unchanged). Then write the line "SRAND initialize" followed by a
   line end to the process standard output stream.
5. Repeat exactly 64 times:
   a. Create a temporary sequence of 16 dwords, all zero.
   b. Set its first ten elements to 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 in that
      order. Elements 10 to 15 stay 0.
   c. Randomly permute all 16 elements in place with the helper at RVA
      0x298D40, which draws from the C runtime generator. The permutation
      walks position i from 1 up to 15. For each i, with n = i + 1, it obtains
      one generator value v (a 15-bit value, 0..32767), accepts it when the
      integer quotient v / n is smaller than the integer quotient 32767 / n,
      or when 32767 modulo n equals n - 1, and otherwise discards it and
      draws again; on acceptance it swaps element i with element (v modulo
      n). Exactly one generator value is consumed per attempt.
   d. Append a copy of the 16-element sequence to the sequence-of-sequences
      held at W+0x4A0 (begin, end, capacity pointers at W+0x4A0, W+0x4A8,
      W+0x4B0; each element is a 0x18-byte begin/end/capacity triple).
      Nothing is removed from this container first, so every call adds 64
      entries.
   e. Free the temporary.
6. Fill the World's derived number tables. Each value is one C runtime
   generator value reduced modulo 100000 (the generator's range is 0..32767,
   so the reduction never changes the value). Exactly 77 values are drawn, in
   this order, each stored as a dword in ascending address order within its
   group:
   1 value at W+0x244; 4 values at W+0x224; 20 values at W+0x248; 2 at
   W+0x298; 2 at W+0x2A0; 2 at W+0x2A8; 2 at W+0x2B0; 8 at W+0x2B8; 6 at
   W+0x2D8; 6 at W+0x2F0; 10 at W+0x308; 4 at W+0x330; 2 at W+0x340; 4 at
   W+0x234; 2 at W+0x348; 2 at W+0x350.
   Together these cover every dword from W+0x224 to W+0x354 exactly once.
7. Empty two sequences without releasing their storage: the end pointer at
   W+0x390 is set equal to the begin pointer at W+0x388, and the end pointer
   at W+0x3A8 is set equal to the begin pointer at W+0x3A0.
8. Reset the 0x38-byte world-state block at W+0x08 to a freshly constructed
   default: its two leading dwords (W+0x08, W+0x0C) are overwritten and its
   three ordered associative containers (head pointer and count pairs at
   W+0x10/W+0x18, W+0x20/W+0x28, W+0x30/W+0x38) are emptied, their previous
   nodes being released. Then the dword at W+0x08 is set to 1 and the dword
   at W+0x0C is set to the constant 0x4C095440 (as a single-precision float
   this is exactly 36000000.0).
9. Guard: only when the byte at W+0x358 is 0 AND the argument name's length
   is not 0:
   a. Open the store at W+0x3B8 on the fixed relative path "Save/world.db"
      (the path does not depend on the seed or the name). The open helper at
      RVA 0x67070 first closes any open handle, asks the database library to
      open the file, and on success issues the statement text
      "CREATE TABLE blobs(key TEXT PRIMARY KEY, value BLOB);" ignoring that
      statement's result. A failed open leaves the handle as the library left
      it and is not reported; the seeding routine ignores the helper's
      result either way.
   b. Call the state loader at RVA 0x2B98B0: it asks the store for the blob
      stored under the 5-character key "state"; when such a blob exists it is
      decoded into the world-state block at W+0x08 by the decoder at RVA
      0x148E90; when it does not exist the block keeps the defaults of step 8.
   When the guard fails (byte at W+0x358 non-zero, or empty name) the store
   stays closed and the defaults of step 8 stand.

Fields this routine never touches: the creature collections, the zone table,
the pointer at W+0x448, the byte at W+0x358, any lock, G, D, E.

---------------------------------------------------------------------------

## 3. Routines at RVA 0x2D2C00 and RVA 0x2DA090 (complete owners)

### 3.1 RVA 0x2D2C00 — remove every creature except the local player

Range 0x2D2C00..0x2D2EAE. Input: W. Output: none. No guard, no early return,
no lock taken inside, no role test. (An overflow of a temporary list raises
the standard "list<T> too long" length error; this is not reachable with
realistic counts.)

Order of effects:

1. Walk the World's creature list (doubly linked; head pointer at W+0x40,
   element count at W+0x48; each list node holds a creature pointer at
   node+0x10) from first to last. Remember, in order, every node whose
   creature pointer differs from the pointer stored at W+0x448.
2. For each remembered node, in the same order: if its creature pointer is
   non-null, destroy and free that creature through the first entry of the
   creature's virtual table with the flag value 1; unlink the node from the
   list; decrement the count at W+0x48; free the node.
3. Empty the creature lookup table (ordered map; head pointer at W+0x450,
   count at W+0x458): release every node and reset the head to the empty
   state with count 0.
4. Rebuild that lookup table from whatever remains in the creature list: for
   each remaining list node, in list order, take its creature, use the signed
   64-bit identifier stored at creature+0x08 as key, find or insert the key,
   and store the creature pointer as the mapped value. Nothing in this step
   special-cases the local player: the table ends up with exactly the
   survivors of step 2.
5. Empty a second list of the World (head pointer at W+0x50, count at W+0x58):
   every node is freed and the count becomes 0. The payloads the nodes refer
   to are not destroyed by this routine.
6. Free the temporary list.

Not touched: the pointer at W+0x448 itself and the entity it refers to; D;
the zone table at W+0x460; every seeding output of section 2; the store at
W+0x3B8; the byte at W+0x358.

### 3.2 RVA 0x2DA090 — undo recorded block changes in every loaded zone

Range 0x2DA090..0x2DA1DD. Input: W. Output: none. No lock taken inside, no
role test. Returns immediately when the zone table has no entries.

The zone table is the keyed container at W+0x460; its entries are chained
from the head pointer at W+0x468; each entry holds a zone pointer at
entry+0x18.

For every entry, in chain order:

1. If the zone pointer is null, skip the entry.
2. Set the byte at zone+0xF8 to 1 (this happens for every non-null zone, even
   one with no recorded changes).
3. If the 64-bit count at zone+0xE0 is 0, go to the next entry.
4. Otherwise walk the zone's change list (doubly linked; head pointer at
   zone+0xD8, count at zone+0xE0) from the LAST element back to the FIRST.
   Each element carries three signed dwords at +0x10, +0x14, +0x18 (block
   coordinates) and a block record starting at +0x1C. For each element call
   the single-block write routine at RVA 0x31C1C0 with the zone, the three
   coordinates in that order, and the address of the element's block record.
5. Write to the process standard output: the text "undo ", the change count
   as an unsigned 64-bit decimal, the text " changes", and a line end.
6. Empty the change list: all elements are freed and the count at zone+0xE0
   becomes 0.
7. Set the byte at zone+0x23C to 1 and store the current multimedia
   millisecond tick, as a dword, at zone+0x238.

The single-block write routine (RVA 0x31C1C0) does nothing when the horizontal
coordinates lie outside the 64-by-64 column area of the zone (the zone's grid
position is the pair of dwords at zone+0x10 and zone+0x14, in units of 64
blocks). Its remaining internals are a provider boundary (limit L4).

---------------------------------------------------------------------------

## 4. Routine at RVA 0x8A500 — record a selection (complete owner)

Range 0x8A500..0x8A5A6. This is the only routine that records a pending
selection after start-up.

Inputs: G; seed (dword, low 32 bits of the second argument); address of a
name text object. Output: none. No guard of any kind: no role or connection
test, no test of the byte at W+0x358, no null test on W, E or D, no comparison
with the current values. No early return.

Order of effects (W is re-read from G+0x08 before each use):

1. Enter the World lock: the critical section at W+0x3C8.
2. Follow W+0x448 to E, then E+0x1528 to D. Store the seed, dword, at
   D+0x2C8.
3. Follow the same two pointers again. Assign the whole name to the text
   object at D+0x2D0, unless the argument is that same object.
4. Store the seed, dword, at G+0x206C.
5. Assign the whole name to the text object at G+0x2070, unless the argument
   is that same object.
6. Call the routine of section 3.1 on W.
7. Leave the critical section at W+0x3C8 and return.

The seed is copied bit-for-bit; there is no conversion, clamping or hashing.
The same argument value is stored twice (D first, then G). The name is copied
by value twice (D first, then G). Nothing is written to W+0x220 or W+0x200.
The routine of section 3.2 is not called on this path, the zone table is not
touched, no number table is regenerated and the store at W+0x3B8 is neither
closed nor reopened.

---------------------------------------------------------------------------

## 5. Callers

### 5.1 Callers of the selection routine (RVA 0x8A500) — ordinary frontend

There are exactly two; both are interface-event handlers registered by the
interface construction routine at RVA 0xF9AC0 through the registration helper
at RVA 0xF8410 with event kind 3 and the Game as receiver.

World-list entry layout used by both (0x38-byte heap object): a pointer at
+0x08 (optional attached preview object), the name text object at +0x10, the
signed dword seed at +0x30, a further dword at +0x34. The Game keeps the
entries as a sequence of pointers at G+0x1790 (begin) / G+0x1798 (end) and the
current entry index as a signed dword at G+0x1770.

A. Call at RVA 0x9F53D, inside the "create world" handler (range
   0x9F060..0x9F5FE; registered at RVA 0xFD82F). Reached when the creation
   page is confirmed. Sequence: hide the creation page; reset camera targets;
   write the new entry count under the key "num" to the Game's second
   key/value store (G+0x2278); read the text of two edit fields; allocate a
   new entry; extract a signed decimal integer from the seed field's text
   into the entry's seed; store the narrowed name field text as the entry's
   name; append the entry to the list and make it current (G+0x1770 becomes
   the last index); persist the entry (routine at RVA 0xA2BE0, which itself
   enters and leaves the World lock); THEN call the selection routine with
   the entry's seed and the address of the entry's name; finally run an
   interface refresh on the object at G+0x118.
   Argument sources: seed = dword at entry+0x30; name = entry+0x10.

B. Tail jump at RVA 0x9FDC6, inside the "select world" handler (range
   0x9FD10..0x9FDEB; registered at RVA 0xFCFB0). Sequence: hide the world
   selection page; reset camera targets; if the signed current index
   (G+0x1770) is less than the number of list entries, jump to the selection
   routine with that entry's seed and name; otherwise show the creation page
   and return without selecting. Only the upper bound is tested: a negative
   index is not rejected by this handler.

There is no start-up-only, network-only or unreachable caller of the
selection routine.

### 5.2 Callers of the seeding routine (RVA 0x2B5F20) — start-up only

A. RVA 0x7F279, in the Game constructor (RVA 0x7EA80), which the main routine
   calls once at RVA 0x136331. Arguments: W from G+0x08; seed = the dword
   just read back from G+0x206C, which the constructor set to 123; name =
   address of G+0x2070, which the constructor set to "world". At this point
   the World constructor has left the byte at W+0x358 at 0 and the name is
   not empty, so step 9 of section 2 opens "Save/world.db" and loads the
   "state" blob if present. The World lock is not held around this call.

B. RVA 0xF1969, in the host-object constructor (RVA 0xF17A0), which the Game
   constructor calls at RVA 0x7EB36, i.e. BEFORE call A. Target: the host's
   own embedded World at H+0x08, freshly constructed there. Arguments: the
   constant seed 123 and a temporary 6-character name " world" (space, then
   "world"). Because that World's byte at +0x358 is also 0, it too opens
   "Save/world.db" and loads "state".

No ordinary gameplay path, frontend path or network path calls the seeding
routine. A third rel32-shaped byte pattern that happens to resolve to
RVA 0x2D2C00 at RVA 0x2D23B6 is the immediate operand of an unrelated
instruction, not a reference.

### 5.3 Callers of RVA 0x2D2C00

- RVA 0x8A58A: the selection routine (section 4), ordinary frontend path.
- RVA 0x3DA3C: the host-switch routine (section 5.5), network path.

### 5.4 Caller of RVA 0x2DA090

- RVA 0x3DA4C only: the host-switch routine, immediately after its call of
  RVA 0x2D2C00. It is never reached from the selection path.

### 5.5 The host-switch routine at RVA 0x3D820 and its call sites — network

Range 0x3D820..0x3DB64 (complete owner). Inputs: C; a 64-bit peer identifier
"new host". It does not read or write G+0x206C, G+0x2070, W+0x220, W+0x200 or
D, and it does not enter the World lock itself. Order of effects:

1. If the current host identifier at C+0x70 passes the identifier validity
   predicate (RVA 0x3BE00): build a 4-byte message holding the dword 2. If
   the current host equals the local peer identifier (global at RVA
   0x54DDC8), hand the message directly to the in-process host through the
   pointer at C+0xA10 when that pointer is non-null; otherwise send it to the
   current host through the platform networking provider (send kind 2,
   channel 1). Then remove the current host from the known-peer set at
   C+0xA00.
2. Reset the client's received-state block at C+0x78 to defaults; set the
   byte at C+0x9E8 to 1; clear the identifier at C+0x70 to zero; empty the
   container at C+0x9F0 and the four lists headed at C+0x20, C+0x30, C+0x50,
   C+0x40 (in that order).
3. Call RVA 0x2D2C00 on W (W = pointer at G+0x08, G = pointer at C+0xA50).
4. Call RVA 0x2DA090 on W.
5. If the in-process host pointer at C+0xA10 is non-null, call the host reset
   routine at RVA 0xF4FF0.
6. Build a 4-byte message holding the dword 1 and deliver it to the new host
   by the same rule as step 1 (direct hand-over when the new host is the
   local peer and the in-process host pointer is non-null; provider send
   otherwise; nothing when the new host is local and the pointer is null).
7. Store the current millisecond tick, dword, at C+0x9EC; store the new host
   identifier, qword, at C+0x70; set the byte at W+0x358 to 1.

The pointer at C+0xA10 is cleared by the client constructor and then set by
the Game constructor (store at RVA 0x7F843) to the address of H.

Call sites and conditions:

- RVA 0x3F721, client per-frame service routine (RVA 0x3F4F0), called from
  the Game update at RVA 0xAAEBF. After draining queued incoming messages it
  compares the current millisecond tick with the dword at C+0x9EC; when more
  than 5000 ms have elapsed (unsigned comparison) it switches to the LOCAL
  peer identifier and returns. No connection-state test precedes this
  comparison in the listing. C+0x9EC is refreshed whenever a message from the
  current host is processed (RVA 0x3E4E0) and at the end of every switch.
- RVA 0x3E752, client message handler (RVA 0x3E450): on message kind 2
  received from the current host, switch to the local peer identifier, then
  drop the sender from the known-peer set.
- RVA 0x27B180 (through the two-instruction forwarding stub at RVA 0x3DBF0
  that supplies the local peer identifier) and RVA 0x27B1B0, both in the
  multiplayer-list activation handler at RVA 0x27B0F0: with the list in its
  "join" mode and a valid selected row, activating the row of the current
  host switches to the local peer identifier; activating another row whose
  identifier is in the known-peer set switches to that identifier.
- RVA 0x3B130, client destructor (RVA 0x3B100): switch to the local peer
  identifier during teardown (Game destruction).

These are network-role paths, not world-selection paths.

---------------------------------------------------------------------------

## 6. How the pending pair is consumed after it is recorded

Only the Game update routine (RVA 0xA5A50) reads the pending pair after
start-up. It enters the World lock at RVA 0xA6464 and leaves it at RVA
0xADB23; both comparison sites lie inside that locked span.

Site 1 (RVA 0xA72CE..0xA7377):

- Condition "match": the dword at G+0x206C equals the dword at W+0x220, AND
  the text at G+0x2070 equals the text at W+0x200 (same length and same
  bytes; helper at RVA 0x6DD20). The seed is compared first; the names are
  compared only when the seeds are equal.
- On match: advance the World simulation for this frame's elapsed time. While
  the remaining time exceeds 16.0 ms, run one world step of exactly 16.0 ms
  and subtract 16.0; then run one final step with the remainder. (World step
  routine at RVA 0x2DA6C0.)
- On mismatch: no world step runs this frame, and one in-game overlay node
  (reached from G+0x380, then +0x178) is forced invisible.

Site 2 (RVA 0xAD724..0xAD757), later in the same frame:

- Condition "mismatch": seeds differ, OR (seeds equal and names differ;
  helper at RVA 0x6DDF0).
- On mismatch: the dword at +0x4A4 of the object the Game points to through
  G+0x18 is set to 0 (helper at RVA 0xE8A30) before the frame is drawn.
- On match: nothing.

Neither site writes any of the six fields.

---------------------------------------------------------------------------

## 7. Seed and source values: widths, copies, conversions, store order

- A selection consists of one signed 32-bit seed and one narrow text name.
  There is no separate "source kind" enumeration in the fields examined: the
  name is the only other component.
- Origin on the create path: the seed is extracted from the wide text of an
  edit field as a signed decimal integer by the standard stream extraction
  for a signed 32-bit integer; the name is the other edit field's wide text
  converted to narrow text (routine at RVA 0x354C30).
- Origin on the select path: both are read from the existing list entry.
- Copies made by the selection routine, in order: seed to D+0x2C8; name to
  D+0x2D0; seed to G+0x206C; name to G+0x2070. All four are value copies of
  the routine's arguments. No width change, no sign change, no arithmetic.
- Start-up copies made by the Game constructor, in order: constant 123 to
  G+0x206C; "world" to G+0x2070; later G+0x206C to W+0x220 and G+0x2070 to
  W+0x200 (through the seeding routine, seed store first, name second), then
  the generator seeding and the derived tables.
- No path copies D to G, D to W, or G to W after start-up.
- The persisted character blob carries the D pair (seed as 4 bytes, then the
  name); reading a character blob overwrites the D pair with the saved one.

---------------------------------------------------------------------------

## 8. Local entity identity and World collections across a selection

Across the selection routine (section 4):

- The pointer at W+0x448 is not written. The same entity object E is
  retained; it is never destroyed, reallocated or re-registered by this path.
- The pointer at E+0x1528 is not written; the same D block is retained and
  only its two last-world fields are overwritten.
- In order: (1) every other creature in the creature list (W+0x40) is
  destroyed and unlinked, the local player's node keeps its place; (2) the
  creature lookup table (W+0x450) is emptied; (3) it is rebuilt from the
  surviving list nodes, keyed by the 64-bit identifier at creature+0x08;
  (4) the list at W+0x50 is emptied.
- Left untouched: the zone table (W+0x460) and every zone's change list and
  flags; the 64-entry-per-call permutation container (W+0x4A0); the derived
  number tables (W+0x224..W+0x354); the two sequences at W+0x388 and W+0x3A0;
  the world-state block (W+0x08..W+0x3F); the store at W+0x3B8 and its
  handle; the byte at W+0x358; the applied pair W+0x220 and W+0x200; all of
  E's own state other than through D.

Across a host switch (section 5.5) the same creature purge happens, then
additionally every loaded zone has its recorded block changes undone and its
three markers set (section 3.2), and the byte at W+0x358 becomes 1. The
entity object at W+0x448 is likewise retained.

Across the seeding routine (start-up only): the creature collections and the
zone table are not touched at all; see section 2 for what is rebuilt.

---------------------------------------------------------------------------

## 9. Roles, guards, locks, threads, provider boundaries

- World lock: one critical section at W+0x3C8 (initialised by the World
  constructor). Entered by the tiny routine at RVA 0x2ABB70, left by the tiny
  routine at RVA 0x2B9890.
  - Selection routine: holds it from before its first store until after the
    creature purge (all four stores and the purge are inside).
  - Game update: holds it around both comparison sites and around the client
    service call, hence around any host switch started from the client
    service routine or the message handler.
  - Seeding routine: not held by either constructor caller.
  - Host switch started from the multiplayer-list handler or from the client
    destructor: the lock state at that moment is not closed (limit L2).
- Client lock: a separate critical section at C+0xA18 protects only the
  queue of incoming messages; it is released before messages are handled and
  before any host switch.
- Role / connection byte W+0x358: 0 after World construction; set to 1 by
  every host switch (including a switch to the local peer); set back to 0 by
  the session teardown routine at RVA 0x911E0 when the Game's session flag
  byte at G+0x21E8 was set (that routine writes it once outside and once
  inside the World lock). Within this contract it gates exactly one thing:
  step 9 of the seeding routine (store open and state load). The selection
  routine and RVA 0x2D2C00 / 0x2DA090 ignore it.
- No routine in this contract tests any other role, permission or
  connection state before acting.
- Threads: the two seeding calls run on the thread executing the main
  routine, during Game construction. The Game update runs on that same
  thread (the main routine's loop). The thread on which interface-event
  handlers (and therefore the selection routine) run was not closed
  (limit L2); because the World critical section is re-enterable by its
  owner, the selection routine's own locking is correct whether or not the
  caller already holds the lock.
- Standard output: the seeding routine and the zone-undo routine write
  diagnostic lines to the process standard output stream as described.

Explicit limits (provider boundaries not fully closed):

- L1. Database library: the open, statement-execution and close entry points
  (RVA 0x1F0BB0, 0x1EB7C0, 0x1EB6C0) were identified only by how they are
  used and by the statement text; their internals and failure modes were not
  read. The blob reader at RVA 0x670F0 was not read beyond its use.
- L2. Interface event dispatch (registration helper RVA 0xF8410, event kind
  3): the triggering input, the dispatching thread and the lock state at
  dispatch were not closed.
- L3. World-state decoder (RVA 0x148E90) and the layout/meaning of the three
  containers in the block at W+0x08 were not closed; only "reset to default,
  then optionally decoded from the state blob" is asserted.
- L4. Single-block write (RVA 0x31C1C0): only its argument order and its
  out-of-zone no-op were closed; its column growth and filler behaviour were
  not.
- L5. In-process host: the reset routine (RVA 0xF4FF0), the direct message
  hand-over (RVA 0xF32C0), the host's thread and its own World were not
  closed beyond the facts stated.
- L6. Platform networking provider: only "first virtual entry is used to
  send a buffer to a peer with send kind 2 on channel 1" is asserted.
- L7. World-list persistence (RVA 0xA2BE0): only its position before the
  selection call, its own World locking and its comparison against the
  applied pair were read (first 262 instructions).

---------------------------------------------------------------------------

## 10. UNRESOLVED

- U1. What the running game does, visibly, while the requested pair differs
  from the applied pair (seed other than 123 or name other than "world"), and
  whether any mechanism outside the supported image's own code changes that.
  Static evidence shows only section 6: no world step and a hidden overlay on
  mismatch, and no code that ever makes the applied pair follow the requested
  pair. This is the most important open point and needs owner-side runtime
  verification, which is outside this author's role.
- U2. The value left in a new list entry's seed when the seed field's text is
  not a valid integer (the entry's seed is not initialised before the
  extraction; the result depends on the runtime library's failure rule).
- U3. Whether the D constructor (RVA 0x7FF20) initialises D+0x2C8. No
  explicit dword store to that offset exists in it; whether a block clear
  covers it was not closed.
- U4. Paths that replace the D block as a whole (the character start handler
  at RVA 0x9F7C0 destroys D and allocates a new one in its "no valid
  character" branch; the routine at RVA 0xA27B0 installs a copy of D on
  another entity) and the exact ordering of a character-blob load relative to
  a selection were read only as far as needed to identify D; their full
  contracts are not part of this document.
- U5. The identity of the second list emptied by RVA 0x2D2C00 (W+0x50) and
  the meaning of the zone markers at zone+0xF8, zone+0x23C, zone+0x238.
- U6. Length-only readers of the applied name (W+0x210) were not enumerated
  in the original (see 1.4).
- U7. Byte-for-byte equivalence between the unpacked analysis image and the
  code the packed retail file executes is inferred from identical headers
  and section tables, not proven (see manifest).
- U8. Every limit L1 to L7 above.

---------------------------------------------------------------------------

## 11. Evidence manifest

Supported image:

| Item | Value |
| --- | --- |
| Retail file | cubeworld.exe, Steam release, 64-bit |
| Retail size | 5,939,072 bytes |
| Retail SHA-256 | A1EFBBD41EEB8DF08395C4D6511194D416CB70A14054B2A2000B9F5CDBCFA65B |
| Retail copies checked | installed Steam copy and the two archived copies: identical hash |
| Analysis image | unpacked derivative of the same file |
| Analysis size | 5,757,952 bytes |
| Analysis SHA-256 | 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244 |
| Link timestamp (both) | 0x5D93AB0D |
| Preferred base (both) | 0x140000000 |
| Code section (both) | RVA 0x1000, virtual size 0x42A33B |

The retail file's code section is stored encrypted on disk and the file has
one extra trailing loader section; all other section headers are identical to
the analysis image. Every RVA in this document refers to the analysis image.
The author did not run the game or any executable; all evidence is static
(instruction listing, call-reference table, string and symbol tables of the
analysis database, and read-only byte inspection of the two image files).

Complete owners (every instruction read):

| RVA range | Role | Instructions |
| --- | --- | --- |
| 0x2B5F20..0x2B67AF | World seeding | 536 |
| 0x2DA090..0x2DA1DD | zone change undo | 75 |
| 0x2D2C00..0x2D2EAE | non-local creature purge | 180 |
| 0x8A500..0x8A5A6 | selection recording | 39 |
| 0x3D820..0x3DB64 | host switch | 191 |
| 0x9F060..0x9F5FE | create-world handler | 313 |
| 0x9FD10..0x9FDEB | select-world handler | 37 |
| 0x9F7C0..0x9FD02 | character start handler | 276 |
| 0xF17A0..0xF1A40 | host-object constructor | 160 |
| 0x911E0..0x913A4 | session teardown | 92 |
| 0x2B98B0..0x2B99CB | state loader | 69 |
| 0x298D40..0x298E35 | permutation helper | 73 |
| 0x31C1C0..0x31C462 | single-block write | 168 |
| 0x27B0F0..0x27B222 | multiplayer-list activation | 69 |
| small helpers (0x2ABB70, 0x2B9890, 0x66BE0, 0x67070, 0x6DD20, 0x6DDF0, 0xE8A30, 0x3BE00, 0x3D6C0, 0x3DBF0, 0x3E310, 0x3E320, 0x29C790, 0x29BE90) | locks, store open/close, text compare, validity, container helpers | about 290 |

Totals for complete owners: 28 routines, about 2,570 instructions, about
9,000 code bytes.

Bounded partial windows (read only inside the stated bounds):

| RVA window | Owner | Instructions (approx.) |
| --- | --- | --- |
| 0x7EA80..0x7F400, plus calls and stores up to 0x7F843 | Game constructor | 470 + filtered |
| 0xA7140..0xA74D6, 0xAADC2..0xAAED0, 0xAD684..0xADB3D | Game update | 520 |
| 0xA2BE0..0xA301E | world-list entry save | 262 |
| 0x3E450..0x3E7A0 (dispatch and kind-2 branch) | client message handler | 130 |
| 0x3F4F0..0x3F760 and 0x40380..0x4042D | client service routine | 190 |
| 0x136200..0x1364C0 | main routine | 150 |
| 0xFCB60..0xFCBF0, 0xFCF70..0xFD000, 0xFD7F0..0xFD880 | interface construction | 90 |
| 0xF4FF0 first 60, 0xF32C0 first 30 | host reset, host hand-over | 90 |
| 0x7FE60..0x7FF10, 0x80200..0x80260 | D copy, D constructor tails | 60 |
| 0x148800..0x148880, 0x14A520..0x14A5C0 | character blob reader/writer | 70 |
| 0x29A130..0x29AF45 (filtered for the offsets named here) | World constructor | filtered |
| 0x3B100..0x3B160, 0x2AE0C0..0x2AE1A0, 0x2B67B0..0x2B686A | client destructor head, zone creation head, a neighbouring generator-seeding routine | 120 |

Totals for partial windows: 12 owners, about 2,150 instructions.

Whole-image checks performed: call-reference table lookups for every routine
named as a caller; a raw scan of every section of the analysis image for
relative and absolute references to RVA 0x2B5F20, 0x2DA090, 0x2D2C00,
0x8A500, 0x9FD10, 0x9F060, 0x9F7C0, 0x3D820, 0x3DBF0, 0x27B0F0, 0x3B100 and
0x911E0; a raw scan of the code section for the displacement 0x206C and for
store instructions with displacement 0x220; listing searches for every
access to displacements 0x206C, 0x2070, 0x220, 0x200, 0x2C8, 0x2D0, 0x358,
0x1528, 0xA10, 0x9EC, 0x21E8, 0x21F8.

Reconstruction lead (authorised 2026-11-07, lead only): five short fragments
of two reconstruction-owned source files of the 1.0.0-1 x64 reconstruction
were consulted after the original evidence had been read, to cross-check the
headline finding. Mismatches between reconstruction and original evidence:
NONE found for the facts stated in this document (same two start-up seeding
call sites, same selection order, same two comparison sites). One
reconstruction observation is recorded as unconfirmed (section 1.4). No
reconstruction names, expressions or structure are reproduced here.

Prohibited-material exposure: none.
