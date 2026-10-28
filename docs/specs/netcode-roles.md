# Netcode, part 2: roles, authority, interest and worlds

Who simulates what in Steam Cube World's multiplayer, the order of the host tick, which
creatures and events each player receives, and how the host's world is shared. Written from the
decompiled executable; routines by RVA. Transport and the menu are in `netcode-transport.md`,
byte layouts in `netcode-packets.md`.

---

## 1. Roles

Every running game owns a **host** (game+0x6e8, its own world at host+0x08, run by the host
thread 0x83e60) and a **client** (game+0xd18, using the game's main world, game+0x08). The two
worlds are separate objects in the same process.

* The **host world** is authoritative: its remote-client flag (world+0x358) is clear. The
  creature specs call this "authoritative" (`creature-abilities.md` §0, `creature-physics.md`
  §0). The host world has no local player of its own (only the game constructor 0x7ea80 sets a
  world's local-player pointer, world+0x448, and only in the client world): every player,
  including the one sitting at that machine, is a guest connected through the protocol (by
  loopback for the local one).
* The **client world** is always a mirror: connecting (also to one's own host) sets its
  remote-client flag (`netcode-transport.md` §4.3). Its local player (world+0x448) is the
  player at this machine.
* **Single player** is the client connected to its own host by loopback. **Hosting** is the
  same with friends connected to that host too. **Being a guest** is the client connected to a
  friend's host; one's own host then idles (`netcode-transport.md` §2).

So there is exactly one authority per session, the host, and the host is a player's game (there
is no dedicated server in the original game). At most 10 players share a host (creature ids 1
to 10, `netcode-packets.md` §2.2).

## 2. The host tick (0xf5520)

Under the host lock (host+0x5f8), each tick, in this order:

1. **Receive** everything waiting on channel 1, then everything in the loopback queue, through
   the dispatcher 0xf4490 (`netcode-packets.md` §2).
2. **Re-invite** invited friends who have not joined, every > 5 s (`netcode-transport.md` §4.1).
3. **Send to each guest** (connections in ascending SteamID order): if the guest's creature
   exists in the host world, its interest set (§4.1), packets 3, then 6, 8, 7 (the events
   gathered for it since the previous tick, §4.2), and one chat line (14). A guest without a
   creature yet gets nothing.
4. **Gather guests' events**: the four queues filled by packets 10–13 are moved into a fresh
   set of event lists (0xf3fd0): damage numbers (host+0x588), sound requests (host+0x5b8),
   buff-change notices (host+0x598), effect objects (host+0x5a8).
5. Lock the world (world+0x3c8) and **apply the guests' events** (0x29d980):
   1. the sound requests are appended to the world's sound list (world+0xa0);
   2. the damage numbers are appended to world+0x90 and each, in order, is logged ("<damage>
      damage from <attacker> <target>") and **applied** to the host world with the apply
      routine 0x29dcf0 (`creature-hit.md` §3: aggro, knock-back, stun, health, death, drops,
      kill credit); if its damage is > 0 and the target exists, the target's hit flash
      (+0x994) = 1.0;
   3. each buff-change notice gives its buff to the target, if it exists (0x65390, flag 0), and
      is appended to world+0x150;
   4. each effect object is appended to the live effects (world+0x60), where the host world's
      effect step moves it and makes its hits (`effect-objects.md`), and to the outgoing list
      (world+0xd0).
6. **Step the world**: the 16-ms sub-steps of 0x2da6c0 (`netcode-transport.md` §2), then
   0x2b68e0 (with *dt*; among other things the drawn-position smoothing, §5), then 0x2e30b0
   (which turns the zones whose ground items or static objects changed into the outgoing zone
   lists, §4.2).
7. If the world's sleep-notice list (world+0x160, host+0x168) holds a 0 (a world reset by
   sleeping, `zone-object-interaction.md` §2.2): every guest forgets which creatures it knows,
   so each gets full updates of everything in view on the next tick.
8. **Distribute** this tick's world events to each guest (0xf33b0, §4.2).
9. Empty the world's event lists (world+0x90 .. +0x18f, by swapping with empty ones).
10. Unlock the world, then the host.

The events made during tick *N* therefore leave in tick *N* + 1's step 3, about 32 ms later.

## 3. The connection record (0x150 bytes, one per guest)

| Offset | Meaning |
|---|---|
| +0x00 | the guest's creature id (1..10) |
| +0x08 | the guest's SteamID |
| +0x10 .. +0x10f | sixteen {list, count} pairs: the events waiting for this guest, in the same slots as the world's event lists (world+0x90 + the same offset − 0x10) |
| +0x110 | the guest's requested zones (ordered set of zone keys) |
| +0x130 | the guest's chat queue: {speaker's creature id, text} |
| +0x140 | the creatures the guest knows: creature id → the 0x970-byte copy last sent |

## 4. Interest management

### 4.1 Creatures (host tick step 3)

For a guest G whose creature P exists in the host world:

1. Walk the host world's creature list (world+0x40) in its order. Skip G's own creature. For
   each other creature C:
   * If C's hostility kind (+0x60) is not 0 (C is not a player) and C is **too far** from P,
     skip it. "Too far" is the test 0xf3e20 with 270.0 blocks (0x42e7dc): in double precision,
     with the position differences in blocks (difference × 1/65536), |dx| > 270 or |dy| > 270
     or dx² + dy² + dz² > 270². (The z difference has no box test of its own.) **Players are
     always sent, at any distance.**
   * If G does not know C yet (C is not in G's known map): only **one** such newcomer per guest
     per tick is admitted — the first one met in list order — plus any newcomer whose flags
     (+0x80) have bit 0x800 or 0x2000 set (those are always admitted; each admitted newcomer also
     uses up the single slot). Others are skipped this tick and tried again next tick.
   * Otherwise C joins this tick's set, with a copy of its record from +0x10 (0x3b7a0).
2. For each creature of the set, in ascending id order, send a packet 3: a delta against G's
   known copy, or a full update when G did not know it (`netcode-packets.md` §2.4, §3).
3. The set, with its copies, becomes G's known map; then packet 6. The guest removes everything
   not in the set (`netcode-packets.md` §2.6).

So a guest sees every player and every non-player within 270 blocks of itself; creatures
entering view arrive one per tick (about 31 a second), each with a full update; a creature that
leaves view is deleted at the guest and comes back with a full update. Every visible creature
costs one packet 3 per tick even when idle. The rate is the host tick rate, about 31 Hz.

The host world spawns creatures near **every** player (`worldgen-creatures.md` §7.1: within
256 blocks of any creature of hostility 0), so each guest's surroundings are populated on the
host.

### 4.2 Events and zone states (0xf33b0)

For each guest G (ascending SteamID), the tick's world events are appended to G's waiting lists:

| World list | Sent to G |
|---|---|
| block changes (+0xc0) | all |
| damage numbers (+0x90) | all |
| particle bursts (+0xb0) | if G's creature exists: those whose position is within **256 blocks** of it (0xf3e20 with 256.0, 0x42e5c4) |
| sound requests (+0xa0) | if G's creature exists: those within 256 blocks whose owner id (+0x28) is **not** G's creature (G already played its own). Sounds 0x1b and 0x1f are logged ("SoundDie SENT!", "SoundMissionComplete SENT!" / "NOT SENT!") |
| effect objects (+0xd0), zone ground items (+0xe0), zone static objects (+0xf0), zone keys (+0x100), pick-up notices (+0x110), kill credits (+0x120), feat notices (+0x130), aggro notices (+0x140), buff-change notices (+0x150), sleep notices (+0x160), calls (+0x170) | all |

Then G's **requested zones** (packet 15) are answered, each looked up in the host world's zone
map (world+0x460):

1. If the zone is not loaded on the host (absent or its zone object null), nothing: G will ask
   again a second later.
2. If the zone has block changes (its change log zone+0xd8 is not empty), append the whole log
   to G's block changes.
3. If its ground items were ever changed (zone+0xf9), append {zone key, its ground items
   zone+0x30} to G's ground-item lists.
4. If its static objects were ever changed (zone+0xfa), append {zone key, its static objects
   zone+0x60} to G's static-object lists.
5. Append the zone key to G's zone keys (the "answered" list).

Then G's requested set is emptied.

The zone lists of the world (+0xe0, +0xf0) are filled by 0x2e30b0 in step 6: for each zone
whose ground items changed this tick (world+0x190) it sets zone+0xf9 and appends {key, copy of
the ground items}; for each zone whose static objects changed (world+0x1a0) it sets zone+0xfa
and appends {key, copy of the static objects}. **Every change re-sends the zone's whole list to
every guest**, wherever they are.

## 5. What each side simulates

"Owned" (`creature-physics.md` §0): a creature is owned by the world that decides for it — the
host world for every non-player, each player's own client for that player.

| What | Host world | Client world (mirror) |
|---|---|---|
| World generation (terrain, structures, zone contents) | generates its zones | **generates the same zones itself** from the same seed (§7); nothing of the terrain is sent |
| Spawning creatures | yes, near every player (`worldgen-creatures.md` §7.1, "unless world+0x358 is set") | no |
| Creature AI | yes, its AI thread (`docs/creatures.md`: "the AI runs only on the server") | no; its AI records of non-local creatures are removed (`creature-ai.md`: on a remote client every record not the local player's) |
| Physics (0x2bb8e0) | every creature; the owned parts for non-players | every creature between updates; owned parts only for the local player |
| Abilities and attacks (0x2c5250) | decided for non-players ("deciding world") | decided for the local player |
| Upkeep (0x2de4c0) | every creature | every creature (the mirror's values for others are overwritten by updates) |
| Interactions (0x2c3710) | performed for every creature, guests' included (their records arrive as packet 9) | never performed: the local player's are sent to the host |
| Effect objects | moved, collided and applied | moved for display; those of others arrive in packet 7 |
| Day and time of day | advanced | overwritten by packet 8 every tick |

**Drawn positions.** Each received creature update arms a 60 ms smoothing (+0xb58 = 60.0) and
the drawn position (+0xb10..+0xb20) and orientation (+0xb34..+0xb3c) ease toward the received
ones in 0x2b68e0, by a factor from 0xa7a0 (the smoothing factor of `effect-objects.md` §0.6)
with the constants at 0x42e6e8; while +0xb58 ≤ 0 they simply equal the record's. The same
happens to guests' players in the host world.

## 6. Authority

| Matter | Decided by | How it spreads |
|---|---|---|
| A player's own record (position, health, mana, ability, gear, name, level, …) | that player's client | packet 3, taken by the host unchecked, relayed to everyone in packet 3 |
| A player's hit on anything | the attacker's client (its "deciding world"): it makes the damage number | packet 10 → the host applies it (0x29dcf0) and relays it (packet 7 list 2) |
| A non-player's hit on a player | the host | damage number in packet 7 list 2; the **target's client applies it to its own record** (0xa5a50: blocked hits move block power and push, mana-shield hits drain buff 6 and send a buff-change notice, others lower stealth, raise stun, lower health and on death play sound 0x1b) and reports the new health in its next packet 3 |
| A non-player's hit on a non-player, deaths, kill credits | the host | packet 7 (damage numbers, kill credits; health ≤ 0 in packet 3; packet 6 removes it once gone) |
| Buffs a player gives | that player's client | packet 12 → host gives the buff and relays it; others give it on their mirror (0xa5a50: every notice except one whose source and target are both the local player) |
| Buffs a non-player gives | the host | packet 7 list 13 |
| Projectiles and other effect objects of a player | that player's client creates them | packet 13 → the host simulates them (their hits are the host's) and relays them; clients show others' |
| Drops, rewards, loot | the host (the death in 0x29dcf0, `drops-and-items.md`) | the zone's ground items in packet 7 list 7 |
| Picking up | the host: the guest sends interaction kind 7 (packet 9); the host checks distance and item equality (`zone-object-interaction.md` §6) | pick-up notice in packet 7 list 9 (all guests; the picker's client gives itself the item — consumer not traced, open question) and the zone's ground items |
| Dropping an item | the host (interaction kind 8, packet 9) | ground items |
| Chests, doors, objects, artifacts | the host (interaction kind 5 and others) | the zone's static objects in packet 7 list 6 |
| Block changes | the host | packet 7 list 1, and the zone's change log for late comers |
| Sleeping / world reset | the host (interaction on a bed) | sleep notice (list 14), day and time (packet 8); the guests' day change runs their reset 0x2c34c0 |
| Day and time of day | the host | packet 8 |
| Aggro | the host | packet 7 list 12 (clients copy it into their mirrors) |
| Calls for help | the host | packet 7 list 15 |
| Missions, quests, inventory, skills, gold, map knowledge | each player's own character data (characters.db) | not sent; equipment shows through packet 3 bits 42 and 43 |

A guest is trusted completely: its record, its damage numbers, its buffs and its projectiles are
applied as sent. There is no anti-cheat and no ownership check beyond "a guest may only update
the creature whose id it was given".

## 7. Worlds, seeds and switching

* **Every Steam Cube World world is the same**: seed 123, named "world" (the host constructor
  0xf17a0 sets up its world with 123 and that name, 0x2b5f20; the world-select screens that
  could choose another are never shown, `frontend-characters.md` §8). The client world uses the
  same seed. **The seed is never sent**, nor any other check that two players share a world:
  zone keys, static-object indices (interaction records address objects by zone and index) and
  generated creature ids (derived from the zone, `worldgen-creatures.md` §1.1) are assumed to
  match because generation is deterministic.
* **Switching worlds mid-session does not exist** in this build. Switching hosts does: joining
  a friend (`netcode-transport.md` §4.3) removes every creature but one's own from the client
  world, undoes every block change of loaded zones and marks every loaded zone as needing its
  state again (zone+0xf8). From then on the client asks the new host for each nearby zone
  (packet 15, once a second) until answered, and the host sends the block-change log and, for
  zones that ever changed, their full ground items and static objects. Zones that never changed
  keep the client's own generated contents. Going back to one's own host does the same with
  one's own host.
* **How the host's world is shared**: by its creatures (packet 3 within 270 blocks, all
  players), its events (packet 7, filtered as in §4.2), its zone states on request (§4.2) and on
  every change, and its clock (packet 8). Nothing else of the host's world is sent; terrain and
  untouched zone contents come from each player's own generator.
* **Persistence.** Only the host's world keeps the shared state: the world set-up 0x2b5f20 opens
  `Save/world.db` for a world whose remote-client flag is clear (the host world) and whose name
  is not empty. So zone states persist in the **hosting** player's save; a guest's own save is
  untouched while it is a guest (its host idles). Each player's character persists in their own
  `Save/characters.db`. (`native-saves.md` §1.1 words this as "not in server mode (world +0x358
  is 0)"; by this spec's reading the world that saves is the host world. See open question 9 in
  `netcode-compatibility.md`.)
* **Sleeping** (a world reset) on the host resets the host world (0x2c34c0 via the world update,
  `native-saves.md` §5.6), makes every guest receive full updates (§2 step 7) and, through the
  day in packet 8, runs the day change on every guest's mirror. The sleep notice itself also
  reaches the guests (list 14); a 0 in it makes the client reset its sent-record copy to a
  default record and send a full update (0x3f4f0, start).

## 8. What a client does with a world update (0xa5a50)

Packet 7 only appends the received lists to the game's slots (game+0x1970 + 0x10 × slot,
`netcode-packets.md` §2.7). The game frame 0xa5a50 consumes them, before the client world's
step, in this order (a guest's own records came back filtered or not as in §4.2):

1. **Block changes**: for each, if the "after" block's type byte (+0x16) and flag byte (+0x17)
   are both 0, remove the block (0x91160); otherwise set it to the six "after" bytes (+0x12,
   0x1a37d0). Record the change in the zone's change log (zone at floor(x / 64), floor(y / 64);
   0xa1680 into zone+0xd8). (Logged "CUBE CHANGES 1" … "4".)
2. **Zone keys answered**: each loaded zone's "state needed" flag (zone+0xf8) = 0.
3. **Ground items by zone**: each loaded zone's ground items (zone+0x30) are replaced by the
   received list.
4. **Static objects by zone**: each loaded zone's static objects (zone+0x60) are replaced; the
   zone is queued for an object rebuild (world+0x1a0) and refreshed (0x31d470, 0x31d700).
5. **Buff-change notices**: each gives its buff (flag byte +0x28) to the target, if present,
   unless source and target are both the local player (0x65390).
6. **Aggro notices**: the angered creature's aggro entry for the angering one (+0xb88 map) =
   the value.
7. The client world's own zone-change step (0x2e30b0).
8. **Kill credits**: each dead creature id (+0x08) goes into the world's dead set (world+0x20),
   so the mirror never spawns or keeps it.
9. Sound requests, particle bursts, block changes, pick-up notices, kill credits, feat notices,
   calls and sleep notices are appended to the client world's own lists (world+0xa0, +0xb0,
   +0xc0, +0x110, +0x120, +0x130, +0x170, +0x160), where the usual consumers play, show or act
   on them.
10. **Feat notices** for the local player: the floating text "FEAT!" and sound 0x1f.
11. **Damage numbers**, for each whose target exists: a dead target (health ≤ 0, race not
    0x11d) is collected for its death effects; a target that is the local player has the hit
    applied to its own record (§6); for another target, a positive hit not made by the local
    player makes it flash (+0x994 = 1.0).
12. **Effect objects** not owned by the local player are added to the client world's live
    effects (world+0x60).
13. **Damage numbers** not made by the local player are appended to the client world's list
    (world+0x90), to be shown.

The kill credits' own handler (0x2c3470, called per credit later in the frame) only looks the
creature up and changes nothing.

## Errata (from the clean-room implementation)

- §8 step 11, "for each whose target exists": in the game the local player is in the world's creature list, so a hit on the player passes this test. A port that keeps the player apart from the list must count it too, or no monster hit on the player ever applies.
