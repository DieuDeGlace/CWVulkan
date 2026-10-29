# Netcode, part 4: CubeVulkan's protocol, interoperating, and open questions

Follow-up evidence: `remaining-runtime-gaps.md` §§1–3 supplies incoming local-player
hit rules, the feat display consumer and client-local guest dialogue effects.
Feat production and payload meaning remain unresolved; do not invent an award packet.

This part compares Steam Cube World's multiplayer (`netcode-transport.md`, `netcode-roles.md`,
`netcode-packets.md`) with CubeVulkan's current protocol (`docs/multiplayer.md`, `src/Net.h`),
says what each of two goals would take, and lists what is still unknown with an oracle for each.
It describes behaviour and requirements only; how to build it is the implementer's call.

The user's decisions that frame it (2026-10-27):
* aim for **wire compatibility** with original-game players if it is feasible; if it turns out
  to be a wall rather than a moderate climb, prefer CubeVulkan's own protocol made to behave
  like the game's;
* **dedicated servers stay possible** as a non-vanilla extra (today's `--server`, by address or
  through Steam), and may use CubeVulkan's own protocol.

---

## 1. The two protocols side by side

| Matter | Steam Cube World | CubeVulkan today |
|---|---|---|
| Transport | Steam P2P (`SteamNetworking005`), reliable, channel 0 down / 1 up | Steam P2P channel 7 unreliable, or UDP by address; own reliability layer for some messages |
| Who may connect | immediate Steam friends only; the host must invite; the guest clicks "join" in the multiplayer menu | anyone by address; by Steam friend name or SteamID from the command line or ini |
| Discovery | the in-game friends list (multiplayer menu, key J) and the invitation packet | rich presence `cubevulkan_host`, log listing |
| Handshake | invitation [0, 100001] → join [1] → id [4]; version mismatch → [24] | join, id, "same world" announcement every 2 s |
| Players | ≤ 10 (ids 1..10) | unlimited ids |
| Single player | the same protocol through a loopback to a built-in host | no network |
| Authority | the host world runs every non-player; each player's client runs its own player and decides its own hits | the host runs monsters; clients send hits as `HIT`; the host works out monster hits on clients (`HURT`) |
| Player state | the game's creature record, 49 fields, delta-coded against the last sent copy, zlib, ~31 Hz | a text line of position, heading, recorded move, health, ~20 Hz; looks once a second as sprite names |
| Creatures | each creature's record, deltas, 270-block view, one newcomer per tick, removal by omission (packet 6) | position, facing, move and health within 120 blocks, ~10 Hz |
| Events | 15 lists of game records each tick (hits, sounds, particles, projectiles, buffs, aggro, block changes, zone states, pick-ups, kill credits, feats, sleep, calls), 256-block filter for sounds and particles | `KILL`, `LOOT`, `LOOTS`, `TAKE`, `GIVE`, `HIT`, `HURT`, `QUEST` |
| Loot | the host's ground items in the zone, picked up through an interaction checked by the host | numbered pouches, first asker wins |
| XP, coins, quests | each player's own; nothing shared beyond the world's events | XP and coins shared per kill, quests copied to every player (CubeVulkan's own inventions) |
| World identity | never checked; seed 123 for everyone | "same extracted world and creature count" check |
| Zone state for late comers | requested per zone (packet 15), answered with the change log and changed lists | late joiners see earlier deaths as gone |
| Time of day | the host's, every tick | not shared |
| Chat | packet 14, UTF-16, relayed to all including the speaker | none |
| Timeouts | guest: 5 s without the host → back to its own world; host: none | 5 s both ways; 6 s host silence → own monsters |
| Dedicated server | none | `--server` by address or Steam |

## 2. Goal (a): the game's behaviour on CubeVulkan's own wire format

To behave like the game while keeping its own format, CubeVulkan needs these behaviours (each
specified in the other parts), whatever the encoding:

1. **One authority.** The host's world decides every non-player: spawning near every player,
   AI, abilities, hits on players, deaths, drops, chests, doors, block changes, day and time.
   Guests keep a mirror: no spawning, no AI, no interactions performed, creatures replaced by
   the host's.
2. **Players own themselves.** Each player sends its whole game-layout record (the 49 fields of
   `netcode-packets.md` §3) and decides its own attacks; the host takes the record as sent and
   applies the player's damage numbers, buffs and projectiles to its world, then relays them.
   Monster hits on a player are applied by that player's own client to its own record.
3. **Interactions travel.** A guest's interactions (pick up, drop, use, talk, bombs, pet call)
   go to the host one at a time and are performed there; results come back as zone states and
   notices.
4. **Interest.** Creatures within 270 blocks plus all players; one newcomer per guest per tick,
   each with a full record; removal by omission. Sounds and particles within 256 blocks, minus
   a guest's own sounds; every other event to everyone.
5. **Zone states.** A guest asks for each nearby zone flagged "state needed" once a second
   until answered; the host answers with the block-change log and, for zones whose ground items
   or static objects ever changed, the whole list; any later change re-sends the zone's whole
   list to everyone. Joining clears the mirror's block changes and re-flags its zones.
6. **Time.** The host's day and time every tick; a day change runs the world-reset day logic on
   guests.
7. **Rates.** A host tick about every 32 ms with 16-ms world sub-steps; guests send every ≥ 32 ms
   and zone requests every second.
8. **Join and leave.** Friends only, by invitation; ≤ 10 players; going to a friend's game
   leaves one's own host (which drops its own guests); leaving or 5 s of host silence returns
   to one's own world; a kick or a leave removes the player's creature on the host.
9. **Chat.** One line per tick each way, relayed to everyone including the speaker.

And it should **drop** what the game does not do: shared XP and coins per kill, copied quests,
numbered loot pouches with first-asker-wins, the "same world" check, the 120-block / 10 Hz
creature feed, CubeVulkan's own `HURT` armour calculation on the host (the game lets the
target's client apply the hit), and rich presence as the way to find a host (the game uses the
friends list and invitations).

Reliability: the game relies on Steam's reliable ordered channels for everything; deltas only
work on such a channel. Over UDP by address CubeVulkan would need ordered reliable delivery for
at least creature deltas (or send full records), as its current `RV` layer does for a subset.

## 3. Goal (b): playing with original-game players on the wire

### 3.1 What it takes

Everything in §2, plus byte-exact packets:

* **Steam.** Run as app 1128000 (CubeVulkan already sets `SteamAppId` and uses the game's
  `steam_api64.dll`), so the original game's menu offers its buttons for a CubeVulkan friend
  (the "same app" test, `netcode-transport.md` §3.2). Accept P2P sessions only from immediate
  friends, as the game does. Use `SteamNetworking005` with send type 2 (reliable), channel 0
  host → client and channel 1 client → host. Never send channel 7 to an original-game peer: it
  never reads that channel, so the data would pile up in its Steam queue.
* **Handshake.** As host: invitation packet 0 with 100001 to each invited friend, again every
  > 5 s until they join; answer 1 with 4 (lowest free id 1..10); treat 24 as "stop inviting";
  answer nothing else at join time. As guest: accept 0 with 100001 (show the invitation, keep it
  6 s from the last one), answer any other version with 24, send 1 to join, wait for 4.
* **Packets.** The ids, layouts, list order and record sizes of `netcode-packets.md`; zlib
  streams for 3 and 7 (any valid zlib stream is accepted, so even stored, uncompressed deflate
  blocks would do; CubeVulkan already has an inflater, `src/Inflate.*`).
* **Records in the game's layouts.** The creature record (0x15f8 bytes, the 49 fields), the
  damage number, sound request, particle burst, effect object, buff-change notice, aggro notice,
  kill credit, pick-up notice, block change, interaction, ground item and static object
  records, as the other specs give them. CubeVulkan's native modules already keep live
  creatures, the native player and these event records in the game's layouts (`src/Spawned.*`,
  `NativeUpdate.h`, `NativeEffects.*`, `NativeInteract.*`, `ZoneStates.*`), which is what
  makes this goal reachable.
* **Same world, bit for bit where the wire assumes it.** Seed 123 and the game's generator with
  vanilla region size (the 1.5× toggle off), because interactions name static objects by zone
  and index, and zones that never changed are not sent: a guest shows its own generated static
  objects and ground items for them, and a host must interpret a guest's object indices against
  its own generation.
* **Mirror mode.** A CubeVulkan guest must run its world as the game's client world does
  (`netcode-roles.md` §5, §8): no spawning, no AI, no performed interactions, creatures created
  and removed by packets 3 and 6, events applied as in §8, its own player's hits applied from
  the host's damage numbers.
* **Host mode.** A CubeVulkan host must present its own player to guests as a creature with an
  id from 1..10, run the host tick of `netcode-roles.md` §2 (including applying guests' damage
  numbers with the apply routine, giving their buffs, simulating their projectiles, performing
  their interactions) and distribute exactly as §4 there.

### 3.2 Feasibility

**Verdict: a moderate climb, not a wall.** The protocol is small and simple: 16 packet ids, no
encryption, no authentication beyond Steam friendship, no clock synchronisation, no
prediction, fixed little-endian records, one 64-bit field-mask delta, zlib, and Steam doing all
reliability and ordering. Every record that travels is a record CubeVulkan's clean-room modules
already hold in the game's layout. The work is mostly architecture (a mirror mode for guests and
a loopback-style host for the local player) rather than reverse engineering.

What blocks interoperability today, and what resolves each:

| # | Blocker | What resolves it |
|---|---|---|
| B1 | No recording of real traffic: every layout here is read from code, not yet seen on the wire | The packet recorder of §5.1, in single player first (it captures almost the whole protocol through the loopback) and then in a two-player session |
| B2 | CubeVulkan has one world; it has no mirror mode and no host/guest split | Implement the client world rules of `netcode-roles.md` §5 and §8 behind CubeVulkan's existing remote-client checks (the native modules already honour "authoritative" and "local") |
| B3 | Guests and hosts must generate identical static objects and ground items for untouched zones | CubeVulkan's generator already matches the oracle zones draw for draw (`docs/CLEANROOM.md`; its remaining gaps are creature spawn points, which a guest never generates, and one shopper); compare a recorded host's zone answers (packet 7 lists 6 and 7) with CubeVulkan's own generation of the same zones |
| B4 | A few consumers are not traced: the pick-up notice's (who gives the picked item), the feat notice's payload, what a guest learns from talking to a quest giver, and the client's arithmetic for a monster's hit on its own player | Spec each from the decompile (0xa5a50, 0x94900 and the conversation path) and check them against the recordings of §5.1 |
| B5 | Testing needs a second Steam account that is a friend and owns the game | A friend's account, or a second owned account on a second PC; nothing on one PC can stand in for the Steam friend gate |
| B6 | CubeVulkan's current Steam layer sends unreliable packets on channel 7 and accepts any session | Reliable send type and channels 0/1 for vanilla peers; friend-only acceptance (`src/SteamSession.*`) |

Nothing found makes it impossible: there is no anti-tamper check of peers, no hash of game
data, and the only version is the constant 100001 (the game has had no update that changed it).

### 3.3 Living with both

* **Telling peers apart.** Both sides ignore unknown packet ids (`netcode-transport.md` §5), so
  a CubeVulkan peer can announce itself with a packet id the game never uses (25 or above) and
  switch to extras only when the other side answers in kind. The game prints "MESSAGE???" for
  an unknown id from a non-connected sender and does nothing else.
* **Extras** (more than 10 players, larger view distance, the 1.5× region size, mods) only
  between CubeVulkan peers; any of them makes a session unsafe for original-game players, so a
  host with extras on should not invite them.

## 4. Dedicated servers

The game has none, but its host already needs no local player: the host world never has one
(`netcode-roles.md` §1), and the host's own player is just a guest on the loopback. A dedicated
server is therefore the game's host without the client, running these host-side rules:

1. The host tick of `netcode-roles.md` §2 at the game's rate: receive, re-invite (Steam only),
   per-guest creature updates and packets 6, 8, 7, 14; gather and apply guests' damage numbers
   (the apply routine 0x29dcf0), buffs (give buff), sounds (relay) and projectiles (simulate);
   the 16-ms world sub-steps; the zone-change lists; the sleep reset's "forget known creatures";
   distribution; clearing the event lists.
2. Ids, joins and leaves (`netcode-packets.md` §2.2, §2.3); no host-side timeout in the game (a
   server may add one as an extra).
3. Interest (`netcode-roles.md` §4.1) and event filtering and zone-request answers (§4.2).
4. The authoritative world: spawning near every player (`worldgen-creatures.md` §7.1), AI with
   only remote players as players, abilities, upkeep, effects, drops and rewards, interactions
   performed for guests (`zone-object-interaction.md`, `skills-and-interactions.md`), day and
   time, the sleep reset (`native-saves.md` §5.6).
5. **Saving the shared world**: in the game the host world is the one that keeps
   `Save/world.db` (`netcode-roles.md` §7), so a server that follows the game keeps the zone
   states; today's `--server` saves nothing.

Transport, per the user's decision:
* **By address**: CubeVulkan's own protocol, server and players all CubeVulkan.
* **Through Steam**: the server logs in as a Steam account. Original-game players can join it
  only if that account is their immediate friend and the server invites them (packet 0); they
  then click "join" in their multiplayer menu. With that, a CubeVulkan server speaking the game's
  protocol could host original-game players too (a dedicated "bot" account). Without it, Steam
  hosting stays CubeVulkan-only.

## 5. Open questions and oracles

### 5.1 The packet recorder (the main oracle)

A ZoneExtractor hook that logs every packet, as received, on the machine it runs on:

* **Hook points**: the host dispatcher 0xf4490 (host, sender SteamID, packet buffer) and the
  client dispatcher 0x3e450 (client, sender SteamID, buffer). Between them they see every packet
  both sides receive, Steam and loopback alike. The buffer is a growable byte array: begin
  pointer at +0, end at +8, a read position (i32) at +0x18 (0 on entry). Optionally the send
  side too: the `SteamNetworking005` object's SendP2PPacket (vtable slot 0: SteamID, data,
  length, send type, channel) and the loopback 0xf32c0 (client → own host).
* **Each record**: timeGetTime (ms); "host" or "client"; the peer SteamID (or a per-session
  number, for privacy); the channel when known; the length; the raw bytes. For ids 3 and 7 also
  the inflated body (inflate with zlib). For id 3 the receiver's record of that creature after
  the delta (0x15f8 bytes, or its first 0x980) helps check the field table.
* **Sessions to record**:
  1. **Single player** (no second account needed): spawn, walk to a town and out past 270
     blocks, fight, get hit, die, pick up, drop, open a chest, use a door, sleep in a bed, chat,
     talk to a quest giver, finish a mission. This captures packets 1, 3, 4, 6, 7, 8, 9–15 and
     the interest behaviour.
  2. **Two players, two PCs, two friend accounts**, both recording: invite, invitation shown,
     join, walk apart beyond 270 blocks, fight the same monster, both pick up loot, chest,
     sleep, chat both ways, leave, rejoin without a new invite (open question 3), kick, host
     quits, guest's network cut for 6 s.
  3. A version mismatch (a hooked game that answers its own invitation with another version,
     or sends packet 0 with 100002 to a friend) to see packet 24 and the host's reaction.

### 5.2 Questions

1. **The SteamUtils call (+0x48) and the menu's "same game" flag.** Is slot 9 of
   `SteamUtils009` GetAppID, and where is the multiplayer widget's +0x1f0 set? Oracle: log the
   returned value and +0x1f0 with the menu open; a screenshot of the menu with one friend in
   Cube World and one in another game.
2. **The networking-utilities call at start-up (+0x30 of `SteamNetworkingUtils002`).** Which
   method it is (the log calls it InitRelayNetworkAccess). Oracle: a log of the call through
   the hook, or Steam's own log at start-up.
3. **Rejoining.** After a guest times out or is cut off, can it rejoin without the host
   uninviting and inviting again? By the code it cannot (its invitation is gone, and the host
   neither re-invites a connected friend nor times one out). Oracle: two-player session 2 above.
4. **The feat notice** (packet 7 list 11, 0x28 bytes): what the 32 bytes after the creature id
   are and who makes them. Oracle: single-player recording while finishing a mission; then a
   spec of the producer.
5. **The pick-up notice's consumer**: which routine gives the picked item to the picker, and
   whether it runs only for the local player. Oracle: recording a pick-up as guest, with the
   guest's inventory before and after.
6. **Talking and quests for a guest**: the host performs the guest's "talk" interaction (kind
   3); what, if anything, reaches the guest (a mission given, a reward), and through which list.
   Oracle: two-player session, the guest talks to a quest giver and finishes the mission.
7. **"State needed" on new zones**: which routine sets zone+0xf8 for a zone newly made in a
   mirror world (0x2da090 sets it only on connecting). Oracle: packet 15s while walking into
   new land (single player is enough).
8. **A monster's hit on the local player** (0xa5a50, the damage-number pass): the exact
   arithmetic (block power, mana, mana shield, stealth constant 0x4ac95c, stun, maximum health)
   deserves its own spec section. Oracle: recorded damage numbers and the player's record before
   and after, in single player.
9. **Which world saves.** By 0x2b5f20, `Save/world.db` is opened by the world whose remote flag
   is clear (the host's); `native-saves.md` §1.1 calls that "not in server mode". Reconcile the
   wording, and check that a guest's own world.db is untouched. Oracle: file times of
   `Save/world.db` on both PCs during session 2.
10. **Stored-block zlib.** Whether the game accepts a zlib stream with stored (uncompressed)
    blocks in packets 3 and 7 (zlib's inflate does; the game's wrapper 0x66dc0 should). Oracle:
    a CubeVulkan peer sending such packets to an original-game peer in a test session.
11. **Large world updates.** A zone with many ground items re-sends its whole list to everyone
    on each change; whether packet 7 ever comes near Steam's reliable-message limit (about
    1 MB). Oracle: the largest *L* seen in the recordings (a town after many drops).
12. **Two joins in one tick** get the same id (the id is only taken when the creature appears).
    Whether that ever shows in practice. Oracle: two guests clicking join at the same moment.
13. **Smoothing details**: the easing factor 0xa7a0 with the constants at 0x42e6e8 (0.02, 0.03)
    and what counts +0xb58 down. Oracle: recorded drawn positions of a remote creature between
    updates (`watch.request` on +0xb10 in the client world).
