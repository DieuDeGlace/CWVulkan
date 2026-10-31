# Multiplayer

CubeVulkan plays multiplayer the way Steam Cube World does, on the game's own wire protocol, so
CubeVulkan players and players of the original game can share a world. The behaviour comes from
the clean-room specs `docs/specs/netcode-transport.md`, `netcode-roles.md`, `netcode-packets.md`
and `netcode-compatibility.md`. The protocol layer has been checked byte for byte against
172,964 packets recorded from the original game (`EngineTests netdecode`).

## How a game is put together

Every running game has two worlds, as the original does:

* **The host world** decides everything that is not a player. It spawns creatures near every
  player, runs their AI, abilities and hits, performs everyone's interactions (pick-ups, chests,
  doors, talking), drops loot, runs the day and time and the sleep reset, and keeps the zone
  states in `Save/world.db`. It has no player of its own: every player in it is a guest's
  creature, including the one at this machine.
* **The client world** is what the player sees and plays. It is always a mirror of a host:
  of a friend's host when the player joined one, otherwise of this machine's own host. Single
  player is simply the client connected to its own host through a loopback, using the same
  packets.

While the player is a guest elsewhere, their own host idles.

### The host tick (about every 32 ms)

The tick runs in the game's order (netcode-roles.md §2):

1. Read the guests' packets: their own records, interactions, hits, sounds, buffs and effects.
2. Re-invite friends who have not joined, every 5 s.
3. Send each guest:
   * every player and every other creature within 270 blocks, as deltas against what it
     already has, with one newcomer a tick;
   * then the end-of-updates marker (a creature left out is removed);
   * then the clock, the world update and one chat line.
4. Apply the guests' hits (the hit routine), buffs and effect objects to the world.
5. Run the world in steps of at most 16 ms, including an admitted zero residual.
   Interpolate and place linked record state once with the full capped tick interval,
   then collect the changed zones' final output lists.
6. After a sleep reset, every guest gets full records again.
7. Hand out this tick's events. Sounds and particles go only to guests within 256 blocks, and
   never a guest's own sounds. Answer zone requests with the block-change log and any changed
   ground items or static objects.
8. Empty the event lists.

An authority reset notice stays in those lists through the entire batch. Each World
substep that sees a zero notice resets nonplayers after body updates and before
active effects. Retained players and queued events survive; a later substep can
recreate and reset residents again. The next tick starts without the old notice.
See the [daily-reset contract](specs/steam-world-daily-reset.md) for the native bounds.

### The client mirror

The client world (netcode-roles.md §5, §8) spawns nothing, runs no AI and performs no
interactions; its player's interactions go to the host one at a time.

* **Creatures** are made, changed and removed by packets3 and6. Existing live updates arm
  native smoothing control60; fresh/dead records snap. The positive value is not a frame
  countdown. The full-frame [native interpolation](specs/steam-creature-drawn-interpolation.md)
  then precedes the separate [linked-pose pass](specs/steam-creature-linked-placement.md).
* **Ability times** follow fields actually present in packet deltas. A received timestamp
  overwrites its field, while an omitted timestamp remains unchanged. The positive-control
  drawn pass adds its specified32-times-blend increment; there is no separate200-ms
  prediction clock or forward-only timestamp clamp.
* **World updates** (packet 7) are applied in the game frame's order: blocks, answered zones,
  ground items, static objects, buffs, aggro, kill credits, the shown lists, feats, damage
  numbers, then effects.
* **A monster's hit on this player** is applied by this player's own client, which reports the
  new health in its next record.
* **The clock** is the host's (packet 8). A new day runs the day change.
* **What this player caused** goes back to the host each frame: its hits, sounds, buff notices
  and effect objects, together with its record.

Each player's character, inventory, gold, experience and quests are their own (in
`Save/characters.db`). Nothing of them is shared: kills give experience to the player whose
kill credit it is, and loot lies on the ground in the host's zones for whoever picks it up.

## Playing together

**Through Steam, the game's way** (the default unless `--offline` or `[Multiplayer] Steam=0`):

* Only immediate Steam friends can connect, and the host has to invite them.
* Press **J** (the controls' "Multiplayer Menu") for the friends list. Hovering over a friend
  shows two buttons:
  * the left one is **invite**, or **uninvite** for a friend you have invited (it also kicks
    them if they have joined);
  * the right one is **join** a friend whose invitation is live, or **leave** to go back to
    your own world.
* A friend's name is followed by " (host)" when you are their guest, and by " (joined)" or
  " (invited)" when they are on your host. "You were invited by ..." shows in orange when an
  invitation arrives. An invitation stays live while the host keeps re-inviting, about every
  5 s.
* From the command line, `--invite friend|steamid` invites and `--join-steam friend|steamid`
  joins.
* Up to 10 players share a host. A guest that hears nothing from its host for 5 s returns to
  its own world. After leaving, a guest needs a new invitation to come back.

**By address over UDP** (CubeVulkan only, with its own reliable delivery):

* Host with `--host [port]` or `[Multiplayer] Host=1`. The port is 27016 unless `Port=` says
  otherwise.
* Join with `--join address[:port]` or `[Multiplayer] Join=address`.
* J lists the players on your host (with uninvite to kick) and the host you are on (with
  leave).

CubeVulkan peers greet each other with packet 25 and may then exchange packet 26 messages
(CubeVulkan extras, for mods). An original-game peer never sees either.

## The dedicated server

`CubeVulkan.exe --server [port]` hosts by address, and `--server steam` hosts through Steam
(its account invites with `--invite`). Following netcode-compatibility.md §4, it is a host with
no client:

* It has no window and no audio, and needs no graphics card: its log says
  `GPU: none (dedicated server)`.
* It runs the host tick as above: spawning near its players, AI, interactions, the clock and the
  sleep reset.
* It keeps the shared world in `Save/world.db` (or `--save-folder`): every minute, and when it
  stops. `--no-save` keeps nothing.
* It logs the number of connected players every 30 s.
* It still needs Steam with the Cube World licence, and the owner's extracted data.

## Testing

* `EngineTests` runs `tests/NetTests.cpp`. It covers:
  * every packet encoded and decoded;
  * the reader rule and the creature delta;
  * a host and guests over the loopback and over UDP;
  * the session timers;
  * the host tick with the client frame on stand-in worlds (interest, removal by omission, event
    filters, zone answers, guests' events, interactions, the clock, the sleep reset, a second
    machine joining and leaving);
  * the mirror's swing timing on a real `SpawnedCreatures` world;
  * the multiplayer menu's rows (`tests/FrontendTests.cpp`).
* `EngineTests netdecode <files>` decodes recordings of the original game's packets
  (ZoneExtractor's `Extract\net\*.bin`) and checks that each encodes back to its bytes.
* For multi-process tests on one machine, run every instance hidden: `--hidden` (a window that
  is never shown) or `--server`. Use test saves and settings (`--save-folder`,
  `--settings-folder`, `--no-save`, `EngineTests makesave`). For example, a host
  `--host 27216 --hidden --play` and a guest `--join 127.0.0.1:27216 --hidden --play`, or a
  `--server 27217` with two hidden guests. The log line `Multiplayer: player N ..., other
  players: ...; hits taken H` shows what each one sees.
* Testing flags: `--net-trace T` logs the mirrored abilities each frame for a second from T s,
  `--test-coins T` has the host lay five coins by its player, and `--net-loss F` drops that
  fraction of UDP packets.

## For mods and engine features

* `Multiplayer` (`src/Multiplayer.h`) owns the transport and both ends, and is driven in three
  calls a frame: `beginFrame`, `consume` and `endFrame`. It also exposes:
  * the menu's rows and buttons (`friends`, `hostButton`, `guestButton`);
  * notices and chat (`takeNotices`, `takeChat`, `say`);
  * the pieces themselves: `hostSide`, `mirrorSide`, `host`, `clientGame` and `net`.
* `HostSide` and `MirrorSide` (`src/NetWorlds.h`) adapt `SpawnedCreatures` to the protocol's
  world interfaces (`src/net/Worlds.h`). Their hooks are `deaths`, `reset`, `dayChanged`,
  `featDone`, `damageShown` and `startedOver`.
* `cwnet::HostGame::extra` and `cwnet::ClientGame::extra` receive CubeVulkan extras (packet 26).
  `HostLink::sendExtra` and `GuestLink::sendExtra` send them, to CubeVulkan peers only.
* `frontend::FriendsMenu` (`src/Frontend.h`) is what the J menu draws from. Anything that fills
  it in can supply the rows.

## Not done yet

* No session with a real second Steam account has been tried. The Steam path is built to the
  spec, but only the loopback and UDP paths have been run.
* Its open questions (netcode-compatibility.md §5.2) remain. These include the feat notice's
  payload, who gives the picked-up item to a guest, what a guest learns from talking to a quest
  giver, and the exact arithmetic of a monster's hit on the local player, which here uses the
  hit routine's apply (0x29dcf0) as a stand-in.
* The menu greys out friends who are not playing Cube World only once the spec traces that flag
  (open question 1). Its panel and button placement is CubeVulkan's own, since the spec gives
  none.
* There is no chat entry line yet. Received lines are shown and logged, and `Multiplayer::say`
  sends one.
