# Netcode, part 3: every packet

Byte layouts of every packet Steam Cube World sends, who sends it, when, and what the receiver
does. Transport, channels and timing are in `netcode-transport.md`; roles and interest in
`netcode-roles.md`. Routines are cited by RVA. All values are little-endian; "i32", "u32",
"i64" are 4- and 8-byte integers, "f32" an IEEE single.

---

## 1. Framing and reading

* A packet is one Steam P2P message (or one loopback buffer). It starts with a **u32 packet
  id**; the rest depends on the id. There is no length prefix, magic, checksum, sequence number
  or version field (except in packets 0 and 24).
* Every packet is sent reliably (Steam send type 2): host → client on channel 0, client → host
  on channel 1 (`netcode-transport.md` §1.4).
* **Reader rule** (every read in 0x3e450, 0xf4490 and the list readers): a read of *n* bytes
  first checks that *n* bytes remain; if not, the read position jumps to the end and the value
  keeps whatever the reader had before (its default). So a short packet never fails: missing
  fields take defaults and later reads see nothing. Extra trailing bytes are ignored.
* **Compressed bodies** (packets 3 and 7): an i32 *L* (the compressed length) then *L* bytes,
  a zlib stream (RFC 1950: two-byte header, deflate data, Adler-32 trailer). The game links
  zlib 1.2.8 ("deflate 1.2.8", "inflate 1.2.8" strings). It compresses with zlib's
  one-call compress at the default level (0x322280 passes −1 to 0x3222a0; output buffer sized
  input × 1.8875 + 12, 0x66c10) and inflates in 128,000-byte chunks until the stream ends
  (0x66dc0 with `inflateInit_` 0x322010). Any valid zlib stream decodes, so a sender need not
  reproduce the game's compressed bytes. A stream that fails to inflate yields an empty or
  partial body, read under the reader rule. A packet 3 with *L* ≤ 0 is ignored.
* Records inside packets are copied raw from memory, padding included. Padding bytes are
  therefore unpredictable on the wire and are never read by the receiver.

## 2. Packet catalogue

| Id | Name used here | Direction | Body after the id | Sent |
|---|---|---|---|---|
| 0 | invitation | host → client | u32 version = 100001 | on invite, then every > 5 s while not joined |
| 1 | join | client → host | — | on connect |
| 2 | leave | either | — | on leave, kick, host shutdown |
| 3 | creature update | either | i32 *L*, zlib(record delta) | host: every tick per visible creature; client: every send tick |
| 4 | your id | host → client | i64 creature id | answer to 1 |
| 6 | end of creature updates | host → client | — | every tick, after the 3s |
| 7 | world update | host → client | i32 *L*, zlib(15 lists) | every tick |
| 8 | time | host → client | i32 day, i32 time of day (ms) | every tick |
| 9 | interaction | client → host | 200-byte interaction record | at most one per send tick |
| 10 | damage number | client → host | 72-byte record | each the player caused |
| 11 | sound request | client → host | 48-byte record | each the player owns |
| 12 | buff-change notice | client → host | 48-byte record | each the player caused |
| 13 | effect object | client → host | 128-byte record | each the player owns |
| 14 | chat | client → host | i32 *n*, *n* UTF-16 code units | at most one per send tick |
| 14 | chat | host → client | i64 speaker's creature id, i32 *n*, *n* UTF-16 units | at most one per tick per client |
| 15 | zone request | client → host | i64 zone key | every > 1 s per zone still needed |
| 24 | version mismatch | client → host | u32 version = 100001 | answer to a 0 with another version |

Ids 5 and 16–23 are never sent or handled. Unknown ids are ignored (the host logs
"MESSAGE??? <id>" when the sender has no connection).

### 2.1 Packet 0: invitation

Sender: 0xf41c0 (on the "invite" click) and the re-invite loop of the host tick 0xf5520.
Body: u32 100001 (constant at 0x43f3ac). 8 bytes in all.

Receiver (client, 0x3e450 case 0), from anyone: version 100001 → notification on the first one
and the invitation set refreshed; anything else → packet 24 back. (`netcode-transport.md` §4.2.)

### 2.2 Packet 1: join

Sender: 0x3d820 (connect). 4 bytes.

Receiver (host, 0xf4490 case 1), with or without a connection:

1. Find the smallest id *k* in 1, 2, …, 10 such that the host world has no creature with id *k*
   (its id map, host+0x458, has no entry or a null one). If none is free, do nothing: the joiner
   gets no answer.
2. If the sender already has a connection record, free that record (its creature, if any, is
   not removed).
3. Create a connection record (0x150 bytes, constructor 0xf16c0): creature id *k* at +0, the
   sender's SteamID at +8, and the per-guest state listed in `netcode-roles.md` §3. Store it in
   the connection map (host+0x568) under the sender's SteamID.
4. Reply packet 4 with *k*.

The id is not reserved until the guest's creature exists (it is created by the guest's first
packet 3), so two joins handled in the same tick may be given the same id.

### 2.3 Packet 2: leave

Senders: the client (0x3d820, to the host it leaves); the host (0xf53b0 kick, 0xf4ff0 drop all,
0xf1f20 shutdown). 4 bytes.

Receivers:
* host (0xf4490 case 2 → 0xf3cf0): remove the sender from the invited set; if it has a
  connection, remove its creature from the host world (0x2d2260) if present, free the record
  and erase the connection.
* client (0x3e450 case 2), from anyone: if the sender is the current host, connect to "me"
  (back to one's own world); in any case remove the sender from the invitation set.

### 2.4 Packet 3: creature update

Body: i32 *L*, then *L* bytes of zlib. Decompressed:

| Offset | Size | Meaning |
|---|---|---|
| 0 | 8 | creature id (i64) |
| 8 | 8 | field mask *M* (u64) |
| 16 | varies | the fields whose bit is set in *M*, in bit order, each as raw bytes (§3) |

**Host → client.** In the host tick (0xf5520), for each guest, one packet 3 per creature in its
interest set (`netcode-roles.md` §4), in **ascending creature id** order (the set is an ordered
map by id), followed by one packet 6. The mask holds every field that differs from the copy
last sent to that guest; a creature new to that guest gets every field (mask with bits 0..48
set). **A packet is sent even when nothing changed** (mask 0: the body is just id and mask).

**Client → host.** In each client send tick (0x3f4f0) the client sends its own player's record
(world+0x448), id first, as a delta against its own last-sent copy (client+0x78), full when the
full-update flag (client+0x9e8) is set; it then clears the flag and stores the record as the new
copy.

**Receiver, client** (0x3e450 case 3):
1. Inflate; read the id. If it is the local player's own id, stop (a player's own record is
   never taken from the network).
2. Note the id in the set of creatures heard since the last packet 6 (client+0x9f0).
3. Find the creature (0xe720). If absent, create one (0x15f8-byte record, constructor 0x4ce80
   with that id) and add it to the world (0x29d0d0).
4. Apply the delta to the record at creature+0x10 (0x138e00, §3).
5. **Smoothing.** If the creature was just created, or its health (+0x180) ≤ 0: no smoothing:
   +0xb58 = 0 (4 bytes), empty its container at +0x9f0 (0x3d7f0), and set the drawn position (+0xb10,
   +0xb18, +0xb20) to the position, the drawn orientation (+0xb34..+0xb3c) to the orientation,
   and the "previous drawn" position (+0xaf8..+0xb08) and orientation (+0xb28..+0xb30) to those.
   Otherwise: +0xb58 = 60.0 and the "previous drawn" position, orientation and a third triple
   (+0xb40..+0xb48 ← +0xb4c..+0xb54, the aim) take the current drawn values. The drawn position
   then eases toward the received one in 0x2b68e0 while +0xb58 > 0 (`netcode-roles.md` §5).

**Receiver, host** (0xf4490 case 3), only from a connected guest: inflate; read the id; **if it
differs from the guest's assigned id, drop the packet**. Otherwise find or create the creature
exactly as the client does, apply the delta, and smooth the same way. There is no validation of
any field: the host takes the guest's record as the guest sends it.

### 2.5 Packet 4: your id

Body: i64 creature id. Receiver (client, 0x3e450 case 4): give the local player that id in the
client world (0x2a2de0) and set the full-update flag, so the next packet 3 carries the whole
record.

### 2.6 Packet 6: end of creature updates

No body. Receiver (client, 0x3e450 case 6): every creature in the client world that is not the
local player and was not heard (by a packet 3) since the previous packet 6 is removed
(0x2d2260); then the heard set is emptied. This is how creatures leave a guest's view and how
dead or despawned creatures disappear.

### 2.7 Packet 7: world update

Body: i32 *L*, zlib. Decompressed: fifteen lists, in this order. Each simple list is an i32
count *n* followed by *n* raw records of the size given; each keyed list is an i32 count *n*
followed by *n* entries of {i64 zone key, i32 count *m*, *m* raw 0xe0-byte records}.

| # | Host list (world+…) | Record size | Contents | Record layout |
|---|---|---|---|---|
| 1 | +0xc0 | 0x1c (28) | **block changes** | the first 28 bytes of the block-change record (`effect-objects.md` §0.5): block x, y, z (i32 ×3), the 6 bytes before, the 6 bytes after, the day (i32) |
| 2 | +0x90 | 0x48 (72) | **damage numbers** | `creature-hit.md` §0.3 |
| 3 | +0xb0 | 0x48 (72) | **particle bursts** | `creature-hit.md` §0.4 |
| 4 | +0xa0 | 0x30 (48) | **sound requests** | `creature-hit.md` §0.4 (owner id at +0x28) |
| 5 | +0xd0 | 0x80 (128) | **effect objects** (projectiles, areas, …) | `effect-objects.md` §0.1 |
| 6 | +0xf0 | keyed, 0xe0 | **static objects** of a zone (full list) | `lore-and-artifacts.md` §1.4 |
| 7 | +0xe0 | keyed, 0xe0 | **ground items** of a zone (full list) | `drops-and-items.md` §0.4 |
| 8 | +0x100 | 8 | **zone keys answered** (to packet 15) | zone key |
| 9 | +0x110 | 0xd0 (208) | **pick-up notices** | `zone-object-interaction.md` §6 |
| 10 | +0x120 | 0x10 (16) | **kill credits** | `creature-hit.md` §0.4 |
| 11 | +0x130 | 0x28 (40) | **feat notices** | i64 creature id, then 32 bytes (not traced, open question) |
| 12 | +0x140 | 0x18 (24) | **aggro notices** | `creature-hit.md` §0.4 |
| 13 | +0x150 | 0x30 (48) | **buff-change notices** | `creature-hit.md` §0.4 |
| 14 | +0x160 | 4 | **sleep notices** | i32 (0 = the world reset; `zone-object-interaction.md` §2.2) |
| 15 | +0x170 | 8 | **calls** | i64 creature id (`creature-ai.md`: a call for help) |

A **zone key** is 8 bytes: the zone x (i32) then the zone y (i32), zones being 64 × 64 blocks.

The host's world keeps these lists as sixteen consecutive {list, count} pairs at world+0x90 ..
+0x18f (the last, +0x180, is never sent). The writers are 0xf0de0 (28-byte), 0xf0e70 (72),
0xf0c90 (48), 0xf0d20 (128), 0xf1120 (a zone's 0xe0 records), 0xf1220 (8), 0xf0f10 (208),
0xf1010 (16), 0xf1090 (40), 0xf0c00 (24), 0xf0b80 (4); the readers 0x39840, 0x39970, 0x39fd0,
0x3a250, 0x39690, 0x39dd0, 0x3a390, 0x3a5a0, 0x39af0, 0x39ce0, 0x3a130, 0x39470, 0x39570,
0x39380, 0x3a6a0 in list order.

Which records each guest receives (all of them, or only those within 256 blocks, plus the
zone states it asked for) is in `netcode-roles.md` §4.2. The packet is sent every tick, also
when every list is empty.

**Receiver** (client, 0x3e450 case 7): append each list to the client's received lists at
game+0x1970 + 0x10 × slot (slot = the host list's index: +0x90 → slot 0 at game+0x1970, …,
+0x170 → slot 14 at game+0x1a50). The game frame 0xa5a50 consumes them (`netcode-roles.md` §8).

### 2.8 Packet 8: time

Body: i32 day (host world+0x08), i32 time of day = trunc32(host world+0x0c), in milliseconds
(a day is 86,400,000 ms). Receiver (client, 0x3e450 case 8): if the day differs from the client
world's, run the day change (0x2c34c0, `native-saves.md` §5.6) first; then set the client
world's day and its time of day to float(the received integer).

### 2.9 Packet 9: interaction

Body: the 200-byte (0xc8) interaction record of `skills-and-interactions.md` §2.1 (item 0xa0,
zone x, zone y, object index, target id, two extra fields, kind, the 16-bit field), raw
(0x3a790).

Sender: 0x3f4f0, in each send tick, takes the **first** pending interaction of the local
player (creature+0xab8) off the list and sends it; at most one per send tick. (On a guest's
world the interaction handler 0x2c3710 does nothing, since the world is a mirror, so the list
only drains through the network.)

Receiver (host, 0xf4490 case 9): read the record (defaults: zone x, y, index −1, the 16-bit
field 1); append it to the end of the pending list of the sender's creature in the host world,
if that creature exists. The host world's interaction handler then performs it (pick-ups,
drops, chests, doors, talking, bombs: `zone-object-interaction.md`, `skills-and-interactions.md`).
Any kind is accepted.

### 2.10 Packets 10, 11, 12, 13: events the guest caused

Each is the packet id followed by one raw record: 10 a damage number (72 bytes), 11 a sound
request (48), 12 a buff-change notice (48), 13 an effect object (128). One packet per record.

Sender: the client, from four queues (client+0x20, +0x30, +0x40, +0x50), all sent in each send
tick in that order (all 10s, then all 11s, then 12s, then 13s) and then emptied. The queues are
filled at the end of each game frame (0x3cc40, while the client has a valid host) from the
client world's own event lists, taking the records **the local player caused**:

* damage numbers whose attacker id (+0x00) is the local player's id → packet 10;
* sound requests whose owner id (+0x28) is the local player's id → packet 11;
* buff-change notices whose source id (+0x00) is the local player's id → packet 12;
* effect objects whose owner id (+0x00) is the local player's id → packet 13.

(The client world's event lists are then cleared, 0xa5a50 end.)

Receivers (host, 0xf4490 cases 10..13, only from connected guests) append each record to a
queue: damage numbers host+0x588 (0xa16f0), sound requests host+0x5b8 (0xa18b0), buff-change
notices host+0x598 (0xa1530), effect objects host+0x5a8 (0xa15a0). The record's defaults if
short: sound pitch and volume 1.0 and owner 0; effect object as its constructor (owner −1,
size and charge 1.0, lifetime 5000, …). What the host does with them is in `netcode-roles.md` §2
(they are applied to the host world and re-broadcast in packet 7).

### 2.11 Packet 14: chat

Client → host body: i32 *n*, then *n* UTF-16LE code units (no terminator). Sent from the
client's outgoing chat queue (client+0x60), at most one line per send tick, only when *n* > 0.

Receiver (host, 0xf4490 case 0xe, connected guests only; *n* ≤ 0 is ignored): read the text
and append {the sender's creature id, the text} to the chat queue (+0x130) of **every**
connection record, the sender's own included.

Host → client body: i64 speaker's creature id, i32 *n*, *n* UTF-16 units. In each host tick,
for each guest, the oldest line of its chat queue (if any) is taken off and sent, after packet
7; at most one line per guest per tick. Receiver (client, case 0xe): if *n* > 0, show the line
with the speaker (0x3cae0). There is no length limit and no filtering.

### 2.12 Packet 15: zone request

Body: i64 zone key (zone x i32, zone y i32).

Sender (0x3f4f0): once more than 1000 ms have passed since the previous round (a static timer),
one packet 15 for each zone in the game's needed-zone list (game+0x2240). That list is rebuilt
every frame, on a mirror world only (0xa5a50 before 0x3f4f0): every loaded zone whose "state
needed" flag (zone+0xf8) is set, among the zones whose index lies between those of the local
player's block x − 256 and x + 256, and y − 256 and y + 256 (zone index = floor(block / 64),
0x1ba800; the player's block position, or, while its ability is 0x9c, the pair at +0x1b0 /
+0x1b4 taken as block coordinates).

Receiver (host, 0xf4490 case 0xf, connected guests only): insert the key into that guest's
requested-zone set (record+0x110); the next distribution (`netcode-roles.md` §4.2) answers it.
The client clears a zone's "state needed" flag when the key comes back in list 8 of a packet 7.

### 2.13 Packet 24: version mismatch

Body: u32 100001 (the client's version, 0x42ef20). Sent by the client (via 0x3f460, channel 1)
in answer to a packet 0 whose version is not 100001 or is missing. Receiver (host, case 0x18,
with or without a connection): remove the sender from the invited set.

---

## 3. The creature record delta

The delta covers bytes of the 0x15f8-byte creature record (layout: `creature-upkeep.md` §1.1,
`creature-physics.md`, `worldgen-creatures.md` §0.3 and the other creature specs). Below,
offsets are **creature record offsets**; on the wire the encoder works on the record from
+0x10 on (0x139620 encodes, 0x138e00 decodes, 0x3b7a0 copies the record from +0x10 into a
0x970-byte snapshot, which ends at creature+0x980).

Bit *b* of the mask set means field *b* follows. Fields appear in increasing bit order, each as
its raw bytes (no padding between fields). Bits 49..63 are never set.

| Bit | Record offset | Bytes | Meaning (see the creature specs) | "Changed" test |
|---|---|---|---|---|
| 0 | +0x10 | 24 | position, i64 ×3 (1/65536 block) | any of the three differs |
| 1 | +0x28 | 12 | orientation, f32 ×3 | any differs (float ≠) |
| 2 | +0x34 | 12 | velocity, f32 ×3 | any differs |
| 3 | +0x40 | 12 | movement intent, f32 ×3 | any differs |
| 4 | +0x4c | 12 | push (knock-back) vector, f32 ×3 | any differs |
| 5 | +0x58 | 4 | look pitch / body tilt, f32 | float ≠ |
| 6 | +0x5c | 4 | contact flags | compared as a float |
| 7 | +0x60 | 1 | hostility kind | byte ≠ |
| 8 | +0x64 | 4 | race | compared as a float |
| 9 | +0x68 | 1 | current ability | byte ≠ |
| 10 | +0x6c | 4 | ability time, f32 | float ≠ |
| 11 | +0x70 | 4 | combo counter | compared as a float |
| 12 | +0x74 | 4 | time since the last landed hit, f32 | float ≠ |
| 13 | +0x78 | 0xb0 | **appearance** block | see below |
| 14 | +0x128 | 2 | state flags, u16 | ≠ |
| 15 | +0x12c | 4 | dodge timer, f32 | float ≠ |
| 16 | +0x130 | 4 | stun timer, f32 | float ≠ |
| 17 | +0x134 | 4 | f32 timer | float ≠ |
| 18 | +0x138 | 4 | f32 timer | float ≠ |
| 19 | +0x13c | 4 | f32 timer | float ≠ |
| 20 | +0x140 | 4 | f32 (recovering toward 0) | float ≠ |
| 21 | +0x144 | 4 | level, i32 | int ≠ |
| 22 | +0x148 | 4 | experience, i32 | int ≠ |
| 23 | +0x14c | 1 | class | byte ≠ |
| 24 | +0x14d | 1 | specialisation | byte ≠ |
| 25 | +0x158 | 4 | charge, f32 | float ≠ |
| 26 | +0x15c | 12 | stance A, f32 ×3 | any differs |
| 27 | +0x168 | 12 | stance B, f32 ×3 | any differs |
| 28 | +0x174 | 12 | aim / carrier offset, f32 ×3 | any differs |
| 29 | +0x180 | 4 | health, f32 | float ≠ |
| 30 | +0x188 | 4 | mana, f32 | float ≠ |
| 31 | +0x18c | 4 | stealth, f32 | float ≠ |
| 32 | +0x190 | 4 | f32 | float ≠ |
| 33 | +0x194 | 4 | ground grip, f32 | float ≠ |
| 34 | +0x198 | 4 | light radius, f32 | float ≠ |
| 35 | +0x19c | 1 | stage byte | byte ≠ |
| 36 | +0x19d | 1 | byte | byte ≠ |
| 37 | +0x1a0 | 8 | owner id, i64 | ≠ |
| 38 | +0x1a8 | 8 | linked creature id (mount, carrier, focus), i64 | ≠ |
| 39 | +0x1b0 | 8 | flight zone pair, i32 ×2 | either differs |
| 40 | +0x150 | 8 | current region, i32 ×2 | either differs |
| 41 | +0x1b8 | 24 | home position, i64 ×3 | any differs |
| 42 | +0x1d8 | 0xa0 | item in use | item equality (`zone-object-interaction.md` §0.7) |
| 43 | +0x278 | 0x6e0 | equipment, 11 items of 0xa0 | any slot unequal (item equality) |
| 44 | +0x958 | 16 | name (zero-terminated) | compared up to the terminator |
| 45 | +0x968 | 8 | reward-owner id, i64 | ≠ |
| 46 | +0x970 | 1 | byte | ≠ |
| 47 | +0x974 | 4 | i32 | ≠ |
| 48 | +0x978 | 8 | i64 | ≠ |

Notes:

* **Not sent**: block power (+0x184), the buff list, inventory, skills, cooldowns, aggro and
  every field past +0x980. Each machine derives those itself or never needs them for others.
* Bits 40 and 39 are out of address order (the region pair at +0x150 is bit 40). The order
  above is the wire order.
* The "changed" test only decides whether a field is sent; when sent, all its bytes go,
  including bytes the test ignores. Float fields compared as floats never count +0.0 and −0.0
  as different and always count a NaN as changed. Fields 6, 8 and 11 hold integers but are
  compared with a float compare, so two integer bit patterns that both read as NaN count as
  changed every time (harmless).
* **Appearance (bit 13)** is "changed" when any of these differ (0x138820, offsets within the
  block): bytes 0–1, bytes 2–4, u32 at 8 (the appearance flags), f32 at 0x0c, 0x10, 0x14, u16
  at 0x18, 0x1a, 0x1c, 0x1e, 0x20, 0x22, 0x24, 0x26 (model numbers), and f32 at 0x28 … 0xac.
  Bytes 5–7 alone never trigger it.
* **Item equality** (bits 42, 43) ignores padding and the attributes beyond the item's count.
* A **full** update (new creature, after packet 4, after connecting, after a world reset) sets
  bits 0..48 regardless of change. The "previous" copy used for a full update is the record
  itself, so nothing else differs.
* The receiver writes each field's bytes straight into its record; an absent field is left
  as it was. Sender and receiver must therefore agree on every previous value: this holds
  because the channel is reliable and ordered and both sides reset together on the events
  above.

---

## 4. Per-tick order on the wire

**Host, to each guest, each tick** (0xf5520): [3] for every creature in that guest's interest
set, ascending id → [6] → [8] → [7] → [14] (if a chat line is queued). Before all of these, in
the same tick, any re-invitations [0] to invited friends who have not joined. A guest whose
creature does not yet exist in the host world (it has not sent its first [3]) gets nothing at
all.

**Client, each send tick** (0x3f4f0, when more than 32 ms have passed since the previous one
and the client has a valid host): [3] → [14] (one line, if any) → [9] (one interaction, if any)
→ all [10] → all [11] → all [12] → all [13] → and, if more than 1000 ms have passed since the
last round, one [15] per needed zone.
