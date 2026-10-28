# Netcode, part 1: transport, sessions and the multiplayer menu

Behavioural specification of how Steam Cube World connects players, written from the
decompiled executable and the binary's constants. Routines are cited by RVA (image base
0x140000000). The other parts are:

* `netcode-roles.md`: who simulates what, authority, interest management, worlds and seeds;
* `netcode-packets.md`: every packet, byte for byte, with the creature delta and the world
  update;
* `netcode-compatibility.md`: comparison with CubeVulkan's protocol, what interoperating with
  original-game players takes, and the open questions with their oracles.

Vocabulary used in all four parts:

* **SteamID**: a 64-bit Steam account id.
* **Host**: the game's built-in server object (the binary's RTTI calls it `Host`). Every running
  game owns one, at game+0x6e8, with its own world at host+0x08 (so host+0x10 is that world's
  day, host+0x14 its time of day, host+0x48 its creature list, host+0x98 its event lists and
  host+0x458 its id → creature map).
* **Client**: the game's network client object (RTTI `Client`), at game+0xd18. Its world is
  the game's main world, game+0x08.
* **Guest**: a player whose client is connected to someone else's host. **Single player** is a
  client connected to its own host.
* A **valid SteamID** passes the test 0x3be00 (repeated inline in 0x3f4f0, 0x9f6c0, 0xb33f0
  and 0x27a520). With H the high 32-bit word and L the low word: the universe (H >> 24) is
  1..4; the account type T = (H >> 20) & 0xf is 1..10; and T = 1 (individual) needs L ≠ 0 and
  instance (H & 0xfffff) < 5, T = 7 needs L ≠ 0 and instance 0, T = 3 needs L ≠ 0.
* All multi-byte values on the wire are little-endian.

---

## 1. Steam API use

### 1.1 Interfaces

The executable imports only `SteamAPI_Init`, `SteamAPI_Shutdown`, `SteamAPI_RunCallbacks`,
`SteamAPI_RegisterCallback`, `SteamAPI_UnregisterCallback`, `SteamAPI_GetHSteamUser`,
`SteamInternal_ContextInit` and `SteamInternal_FindOrCreateUserInterface` from
`steam_api64.dll`. Every interface is reached through a cached context
(`SteamInternal_ContextInit`) whose initialiser asks for one versioned interface:

| Context | Initialiser | Interface | Methods used (vtable byte offset → method of that interface version) |
|---|---|---|---|
| 0x5314f0 | 0x3bea0 | `SteamNetworking005` | +0x00 SendP2PPacket, +0x08 IsP2PPacketAvailable, +0x10 ReadP2PPacket, +0x18 AcceptP2PSessionWithUser |
| 0x5314d8 | 0x3be70 | `SteamFriends017` | +0x18 GetFriendCount, +0x20 GetFriendByIndex, +0x38 GetFriendPersonaName, +0x40 GetFriendGamePlayed |
| 0x5314c0 | 0x3bed0 | `SteamUser020` | +0x10 GetSteamID |
| 0x53cc20 | 0x25e910 | `SteamUtils009` | +0x48 (slot 9; GetAppID in that interface's layout, see open questions) |
| 0x537270 | 0x134070 | `SteamNetworkingUtils002` | +0x30, called once at start-up under the log line "InitRelayNetworkAccess" (0x1355a0) |

Nothing else of Steam is used for multiplayer: **no lobbies (no SteamMatchmaking), no rich
presence, no game-launch or overlay invites, no "join game" from the Steam friends list, no
CloseP2PSessionWithUser, no session-failure callback**. The only registered callback is
`P2PSessionRequest_t` (id 1202 = 0x4b2), registered by the game constructor 0x7ea80 at
game+0x1960 (RTTI `CCallbackInternal_onP2PSessionRequest`), unregistered by the game
destructor 0x818d0.

### 1.2 Start-up

The main routine 0x1355a0 calls `SteamAPI_Init` (logging "SteamAPI_Init", then "STEAM API
INITIALIZED SUCCESSFULLY" or "STEAM API INIT FAILED"; on failure it returns and the game does
not start), then the networking-utilities call above, then `WSAStartup` (a left-over: no socket
is ever created, see §6). `SteamAPI_Shutdown` is called when the main routine ends.

The client constructor 0x3aa10 stores the player's own SteamID (GetSteamID) in a global
(0x54ddc8); the host constructor 0xf17a0 stores the same value in another (0x5516f8). Both are
"me" below.

### 1.3 Accepting sessions (0x9f6c0)

On every `P2PSessionRequest_t` (remote SteamID R), the game walks its immediate friends
(GetFriendCount with flag 4, GetFriendByIndex 0 .. count−1; a count of −1 counts as 0). If some
friend is a valid SteamID equal to R, it calls AcceptP2PSessionWithUser(R). Otherwise
the request is not answered (Steam then drops it). **Only immediate Steam friends can exchange
packets with a game**, in either direction.

### 1.4 Sending and channels

Every packet is sent with SendP2PPacket, send type **2 (k_EP2PSendReliable)**, and one of two
channels:

| Direction | Channel | Senders |
|---|---|---|
| host → client | **0** | 0xf4f50 (general), 0xf41c0 and the re-invite loop in 0xf5520 (invitations), 0xf53b0, 0xf4ff0, 0xf1f20 (leave notices) |
| client → host | **1** | 0x3f460 (general), 0x3f4f0 (creature updates), 0x3d820 (join and leave) |

Steam's reliable mode keeps each channel's packets in order and retransmits them, so the game
has no sequence numbers, acknowledgements or resends of its own.

**Loopback.** When the destination SteamID is "me", nothing goes through Steam:

* client → own host: the buffer is copied (0x3a970) into the host's local queue at host+0x620
  (lock at host+0x5d0) by 0xf32c0;
* host → own client: the host calls its local connection object at host+0x5c8 (RTTI
  `Host::LocalClientConnection<Client>`), which hands the buffer to the client.

The bytes are identical either way, so single player runs the very same protocol.

### 1.5 Receiving

* **Client** (thread started by the client constructor, body 0x3c920 → 0x3bae0): while the
  client's run flag (client+0x08) is set: `SteamAPI_RunCallbacks`; then while
  IsP2PPacketAvailable on **channel 0**: ReadP2PPacket into a buffer of the announced size,
  and on success append (sender SteamID, buffer) to the client's inbox at client+0xa40 (lock at
  client+0xa18), `Sleep(0)` between packets; then `Sleep(16)`. This is the only place
  `SteamAPI_RunCallbacks` is called, so session requests are answered from this thread.
* **Host**: at the start of each host tick (0xf5520) it reads every packet waiting on
  **channel 1**, handing each to the host dispatcher 0xf4490, then empties its local queue
  (host+0x620) through the same dispatcher. A receive phase longer than 8 ms is logged as
  "Host receive <ms> <any>".
* The client handles its inbox once per frame, at the start of 0x3f4f0, through the client
  dispatcher 0x3e450.

---

## 2. Threads and rates

| Loop | Where | Rate |
|---|---|---|
| Host tick | the host thread 0x83e60 calls 0xf5520(host, elapsed ms) | about every 32 ms (see below) |
| Client network tick | 0x3f4f0, called once per rendered frame from the game frame 0xa5a50 | sends at most once per 32 ms |
| Client receive thread | 0x3bae0 | polls about every 16 ms |

**Host thread (0x83e60).** While the game's run flag (game+0x2068) is set:

* If the client is currently a guest of another player (0x3e2d0: the client's host SteamID is a
  valid SteamID and is not "me"), the host does not tick: `Sleep(33)`.
* Otherwise: elapsed = now − time of the previous tick start (timeGetTime, ms); run the host
  tick with it; if that took less than 32 ms, sleep for the rest of the 32 ms.

So a game's own host idles while its player is a guest elsewhere, and resumes when it comes
back. When the run flag drops, the host thread disconnects every guest (0xf4ff0, §4.4).

**Host tick time-step (0xf5520).** *dt* = min(elapsed, 1000). The world advances in sub-steps
of 16 ms (constant 0x4aca34): if *dt* > 16, it runs ⌊(*dt* − 17) / 16⌋ + 1 world steps
(0x2da6c0) of 16.0 ms and then one step of the remainder (*dt* − 16 × that count); otherwise one
step of *dt*. The order of everything else in the tick is in `netcode-roles.md` §2.

---

## 3. The multiplayer menu

### 3.1 Opening it

The key "Multiplayer Menu" (controls.cfg `ButtonMultiplayerMenu`, default J;
`frontend-options.md`, the controls table, row 20) and the system menu's entry (layer index 4,
"multiplayer-menu" with its background, built in 0xf9ac0; its handler 0x1030c0) open the
multiplayer widget (RTTI `MultiplayerWidget`). Opening it rebuilds its friend list (0xb33f0):

1. Clear the list (widget+0x1f8 .. +0x200, rows of 0x28 bytes: SteamID at +0, the persona name
   as a wide string at +8).
2. For i = 0 .. GetFriendCount(4) − 1: GetFriendByIndex(i, 4); if it is a valid individual
   SteamID, append (id, GetFriendPersonaName(id) converted from UTF-8).
3. Refresh the widget (0x27b490).

The list is not refreshed while the widget stays open.

### 3.2 What each row shows (0x27a520)

The widget draws the title "Friends" and, for each row whose id is a valid SteamID:

* the friend's name, followed by
  * " (host)" when this player is currently that friend's guest (the client's host is that id,
    0x3e310), then
  * " (joined)" when that friend is connected to this player's host (0xf4170: present in the
    host's connections, host+0x568), else " (invited)" when this player's host has invited them
    (0xf4350: present in the host's invited set, host+0x578);
* row colours change with these states (constants at 0x46ee20, 0x46ee30, 0x46ee40, 0x4380f0,
  0x4381c0, 0x46d670; cosmetic);
* two buttons when the mouse is over the row:
  * left: "invite", or "uninvite" when the friend is invited (that is, in the invited set);
  * right: "join", or "leave" when this player is that friend's guest. The left button is drawn
    only while the row is hovered; the right one while hovered, and also always when the friend
    has a live invitation for this player (0x3e320).
* When the widget's flag +0x1f0 is set, buttons are offered only for friends whose
  GetFriendGamePlayed reports a game whose low 24 bits equal this game's app id (SteamUtils
  +0x48, read once per draw); other rows are greyed. Where +0x1f0 is set was not traced (open
  question).

### 3.3 Clicking (0x27b0f0)

On a click on row *i* (index kept at widget+0x2ec; widget+0x2f0 says which button):

* **Left button (host side):** if the friend is in the invited set or connected (0xf4350 or
  0xf4170): **uninvite / kick** (0xf53b0, §4.5). Otherwise **invite** (0xf41c0, §4.1).
* **Right button (guest side):** if this player is that friend's guest: **leave** (0x3dbf0 =
  connect to "me", §4.3). Otherwise, if the friend's invitation is live (0x3e320): **join**
  (0x3d820 with the friend's SteamID, §4.3). Otherwise nothing.

There is no other way to join or host: no address entry, no server browser, no command line.

---

## 4. Connection life cycle

### 4.1 Invitation (host side, 0xf41c0)

1. Send packet **0** (`netcode-packets.md` §2.1: id 0 and version 100001) to the friend,
   channel 0, reliable.
2. Under the host lock (host+0x5f8): find or insert the friend in the **invited set**
   (host+0x578; ordered by SteamID; each entry holds the SteamID and a time) and set its time to
   now (timeGetTime).

**Re-invitation.** Every host tick (0xf5520, after the receive phase), for each invited friend
that is **not** connected (not in host+0x568) and whose time is more than 5000 ms old: set its
time to now and send packet 0 again. A connected friend stays in the invited set but is not
re-invited.

### 4.2 Receiving an invitation (client side, 0x3e450 case 0)

A packet 0 is accepted from anyone (not only the current host; §5).

* If its version is **100001** (0x186a1): if the sender has no live invitation yet (0x3e320),
  show the notification "You were invited by " + GetFriendPersonaName(sender) (UTF-8 converted;
  colour {1.0, 0.5, 0.2, 1.0} at 0x42ffa0; shown through the game's message list 0x26bf10).
  Then insert or refresh the sender in the client's **invitation set** (client+0xa00, SteamID →
  time) with time = now.
* Otherwise (a different version, or a packet too short to hold one): reply packet **24**
  (0x18) with the client's version 100001 (`netcode-packets.md` §2.13) to the sender, channel 1.
  The host that receives it removes the client from its invited set (0xf4490 case 0x18,
  erase by key 0x3dce0) and so stops re-inviting. Nothing is shown to either player.

**Expiry.** In each client tick (0x3f4f0), while the host is heard from (§4.6), every
invitation whose time is more than 6000 ms old is logged "INVITATION TIMEOUT" and removed. With
the 5-second re-invitation, an invitation stays live as long as the inviting host keeps running
and keeps the friend in its invited set.

### 4.3 Joining and leaving (client side, 0x3d820 "connect to X")

Connect-to-X is used for join (X = the friend), leave (X = me, via 0x3dbf0), the host's leave
notice and the host-silence timeout (X = me), and the client destructor 0x3b100 (X = me). It
does, in order:

1. If the current host id (client+0x70) is a valid SteamID: send packet **2**
   (leave) to it (through the loopback when it is "me", else channel 1), and remove that id
   from the invitation set.
2. Reset the client's sent-record snapshot (client+0x78) to a freshly constructed default
   creature record (0x4d600) and set the **full-update flag** (client+0x9e8 = 1). Clear the
   current host id (client+0x70 = 0, both words). Clear the set of creatures heard this frame
   (client+0x9f0) and the four outgoing event queues (client+0x20, +0x30, +0x40, +0x50).
3. In the client's world: remove every creature except the local player (0x2d2c00) and undo every block change of
   every loaded zone, marking each zone as needing its state from the host again (0x2da090: zone
   +0xf8 = 1; "undo N changes" logged).
4. If the game has a host (client+0xa10 ≠ 0): that host drops all its guests (0xf4ff0, §4.4).
5. Send packet **1** (join) to X (loopback when X is "me", else channel 1).
6. Last-heard time (client+0x9ec) = now; current host id = X; the world's remote-client flag
   (world+0x358) = 1.

So joining a friend first leaves one's own host (which also kicks one's own guests), and
leaving goes back to one's own host. The client world is always a mirror (remote-client flag
set), also in single player. Note that step 4 runs on every connect, including the return to
"me": coming back from a friend's game also clears one's own host of guests (it has none then).

### 4.4 Dropping all guests (host side, 0xf4ff0)

Under the host lock: for every connection whose SteamID is not "me", send packet **2** to it
(channel 0, reliable, 4 bytes); free every connection record; empty the connection map
(host+0x568), the invited set (host+0x578) and the four queues of events received from guests
(host+0x588, +0x598, +0x5a8, +0x5b8; `netcode-roles.md` §2). The host destructor 0xf1f20
(logging "[HOST] waiting for generation thread...") does the same sends before freeing.

### 4.5 Kicking one player (host side, 0xf53b0, "uninvite")

Under the host lock: remove the SteamID from the invited set; if it has a connection: if its
creature exists in the host's world, remove that creature (0x2d2260); free the record and erase
the connection. After the lock: unless the id is "me", send it packet **2** (channel 0).

A guest's own packet 2 does the same on the host (0xf4490 case 2 → 0xf3cf0), without the reply.

### 4.6 Timeouts and reconnection

* **Client:** if more than 5000 ms pass (the test is now − last-heard ≥ 5001) without any packet
  from the current host, the client connects to "me" (§4.3): it returns to its own world. Any
  packet from the current host refreshes last-heard (0x3e450, before dispatch); so does joining.
  Since a running host sends every tick (§2), silence means the host is gone or unreachable.
* **Host:** there is **no** timeout. A guest that vanishes without sending packet 2 keeps its
  connection, its id and its creature (frozen at its last update) until the host player kicks
  it or the host shuts down.
* There is **no automatic reconnection**. A guest that leaves for any reason loses its
  invitation (§4.3 step 1; a packet 2 from a host also removes it, 0x3e450 case 2). On the host,
  a guest whose packet 2 arrived is removed from both the connections and the invited set
  (0xf3cf0), so it is not re-invited; one whose packet 2 never arrived stays "connected", and
  connected friends are not re-invited either. Either way the host player has to uninvite
  and invite again before that friend can rejoin (open question 3 in
  `netcode-compatibility.md`).

### 4.7 Version check

The only version check is the invitation's: the constant **100001** (host side 0x43f3ac, reply
0x42ef20, compared as an immediate in 0x3e450). Join (packet 1) carries no version and the host
does not check that the joiner was invited: any friend whose session Steam accepted can send
packet 1 and be given an id.

---

## 5. Who may send what

* The **client** dispatcher (0x3e450) reads the packet id; if the sender is not the current host
  and the id is neither 0 nor 2, the packet is dropped. Packets 0 and 2 are accepted from
  anyone (an invitation, or "forget my invitation").
* The **host** dispatcher (0xf4490) looks the sender up in its connections; if absent it logs
  "MESSAGE??? <id>" but still handles ids 1, 2 and 24; every other id needs a connection and is
  otherwise ignored. Unknown ids are ignored by both sides (no reply, no disconnect).

---

## 6. Left-overs that are not multiplayer

* A socket client: the game object has a socket handle (game+0x21f8), threads (game+0x1ff8,
  +0x2008) and a "Disconnected." message (0x911e0), and `WSAStartup` is called, but the
  executable imports no `socket`, `connect`, `send` or `recv`; the handle is never set, so this
  path is dead. 0xa5a50's copying of the local player's events into game+0x2200 / +0x2210 /
  +0x2220 depends on that handle and never runs.
* The world-select screens with "Multiplayer Worlds..." (0x9ffa0, names starting `online_`) are
  never shown (`frontend-characters.md` §8).
