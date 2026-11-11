# Steam in-process host thread and world step scheduling

Status: FINAL / STABLE / HELD (source-side behavioural contract, code-free).
Date: 2026-11-11.
Scope: the supported Steam image only (identity in section 12). Everything
below describes the original program's observable behaviour. Nothing here is
implementation advice.

Companion contracts (read-only context, reused shorthand):
"Steam pending source selection and world reconfiguration" (2026-11-10) and
"Steam world list pages and world entry persistence" (2026-11-11).

Shorthand:

- G = the Game object. W = the live World (pointer at G+0x08).
- C = the network client embedded at G+0xD18.
- H = the in-process host embedded at G+0x6E8. HW = the host's own World,
  embedded at H+0x08 (= G+0x6F0). HW is never W.
- A = the creature-AI controller embedded in a World at World+0x78. Its first
  qword is a back pointer to its World; a keyed container of per-creature AI
  records starts at A+0x08 (World constructor RVA 0x29A130, sub-object
  constructor RVA 0x9590). The AI controller of HW is therefore at H+0x80 =
  G+0x768.
- "Local id" = the local platform user identifier. Two global copies exist:
  the client compares against the global at RVA 0x54DDC8, the host against
  the global at RVA 0x5516F8 (the host constructor fills the latter from the
  platform user interface).
- "Millisecond tick" = the multimedia timer (timeGetTime), a wrapping
  unsigned 32-bit millisecond counter. All elapsed-time tests below are
  unsigned 32-bit differences unless stated otherwise.
- "World lock" = the critical section at World+0x3C8 (enter helper RVA
  0x2ABB70, leave helper RVA 0x2B9890). "Zone lock" = the second critical
  section of a World at World+0x3F0 (enter helper RVA 0x2ABB80, leave helper
  RVA 0x2B98A0).
- All offsets hexadecimal; dword = 32 bits, qword = 64 bits.

---------------------------------------------------------------------------

## 0. Headline findings (read first)

F1. The in-process host is driven by a dedicated Game thread (the "host
thread", entry RVA 0x83E60) that calls the host tick (RVA 0xF5520) on a
32 ms cadence. The host tick advances HW in 16 ms slices with the same slicing
rule the Game update uses for W.

F2. Creature AI runs on its own Game thread (the "ai thread", entry RVA
0x83F20) on a 40 ms cadence, and ONLY for HW (controller at G+0x768). The
live World's AI controller (W+0x78) is never ticked: the AI routine (RVA
0x13830) has exactly one caller in the image.

F3. W never runs AI. W does run its own world step (RVA 0x2DA6C0) every frame
on the main thread, including the per-creature movement routine for every
creature it holds; several authoritative parts of that step are skipped when
the role byte W+0x358 is non-zero (it is 1 after any host switch, including
the switch to the local host). Host results reach W only as messages: the host
delivers messages for the local peer synchronously into the client's incoming
queue (C+0xA40, under the client lock C+0xA18); the Game update drains that
queue once per frame, after W's world step, while holding W's World lock.

F4. A host switch (RVA 0x3D820) never pauses, stops, restarts or joins any
thread. It resets H's peer and queue state and empties HW of creatures (RVA
0xF4FF0) under the host's connection lock, without taking HW's World lock.
The host thread itself idles (33 ms sleeps, no tick) whenever the client's
current host is valid and not the local id; it resumes ticking as soon as the
current host is the local id again (or invalid), with its first elapsed time
capped at 1000 ms.

F5. Eight threads are created in total: one inside the host constructor (the
host "generation" thread for HW), one inside the client constructor (the
client receive thread), and six by the Game constructor (chunk, generation,
host, map, ai, music). Two further Game thread slots ("receive", "send" at
G+0x1FF8 and G+0x2008) are zeroed and joined-if-joinable but are never started
in the supported image.

---------------------------------------------------------------------------

## 1. Thread inventory

Every thread is a C++ standard-library thread (runtime thread-start entry,
launched through the common start stub RVA 0x3C7D0, which calls the first
entry of a per-thread launch object's table and then signals thread-exit
completion). Each creator waits, on a runtime condition variable, until the
new thread has taken ownership of its argument (the trampoline sets a
"started" byte and signals before calling the entry). The argument of every
Game thread is a heap box holding G (RVA 0x7D560); the host generation
thread's box holds H; the client receive thread's box holds C. No thread sets
an affinity mask. Only the chunk and the Game generation threads change their
priority (section 1.2).

Global run flags:

- G+0x2068 (byte): Game run flag. Set to 1 by the Game constructor (RVA
  0x7EF63) before any Game thread starts; set to 0 only by the Game
  destructor (RVA 0x8191C). All six Game threads loop while it is non-zero
  and test it once per iteration (at the bottom of the loop, and once before
  the first iteration).
- H+0x550 (byte): host run flag. Set to 1 by the host constructor (RVA
  0xF17E7); set to 0 only by the host destructor (RVA 0xF1F45).
- C+0x08 (byte): client run flag. Set to 1 by the client constructor (RVA
  0x3AA36); set to 0 by the client destructor (RVA 0x3B1E3).

### 1.1 Table

Order of creation equals order of rows (the host generation thread is created
first, during the host constructor that the Game constructor calls at RVA
0x7EB36; the client receive thread second, from the client constructor called
at RVA 0x7EB46; the six Game threads last, at the end of the Game
constructor).

| # | Name (from the Game destructor's wait messages) | Created by (call site) | Launch routine | Trampoline | Entry | Handle slot | Run flag |
| --- | --- | --- | --- | --- | --- | --- | --- |
| T1 | host generation thread | host constructor RVA 0xF17A0, call at RVA 0xF19E8 | RVA 0xF0150 | RVA 0xF3220 (via table stub RVA 0xF3070) | RVA 0xF2340 | H+0x558 (handle qword) / H+0x560 (id dword) | H+0x550 |
| T2 | client receive thread | client constructor RVA 0x3AA10, call at RVA 0x3ABD3 | RVA 0x38800 | RVA 0x3C920 (via stub RVA 0x3C7F0) | RVA 0x3BAE0 | C+0x10 / C+0x18 | C+0x08 |
| T3 | "chunk thread" | Game constructor RVA 0x7EA80, call at RVA 0x7F7C2 | RVA 0x76160 | RVA 0x890C0 (stub RVA 0x87BE0) | RVA 0x842D0 | G+0x1FB8 / G+0x1FC0 | G+0x2068 |
| T4 | "generation thread" (live World) | Game constructor, call at RVA 0x7F80F | RVA 0x762D0 | RVA 0x89160 (stub RVA 0x87BF0) | RVA 0x84F70 | G+0x1FA8 / G+0x1FB0 | G+0x2068 |
| T5 | "host thread" | Game constructor, call at RVA 0x7F879 | RVA 0x75D10 | RVA 0x88EE0 (stub RVA 0x87BB0) | RVA 0x83E60 | G+0x2018 / G+0x2020 | G+0x2068 |
| T6 | "map thread" | Game constructor, call at RVA 0x7F8C6 | RVA 0x75BA0 | RVA 0x88E40 (stub RVA 0x87BA0) | RVA 0x83C70 | G+0x1FC8 / G+0x1FD0 | G+0x2068 |
| T7 | "ai thread" | Game constructor, call at RVA 0x7F916 | RVA 0x75E80 | RVA 0x88F80 (stub RVA 0x87BC0) | RVA 0x83F20 | G+0x1FD8 / G+0x1FE0 | G+0x2068 |
| T8 | "music thread" | Game constructor, call at RVA 0x7F96A | RVA 0x75FF0 | RVA 0x89020 (stub RVA 0x87BD0) | RVA 0x83FB0 | G+0x1FE8 / G+0x1FF0 | G+0x2068 |

Between T4 and T5 the Game constructor stores the address of H into C+0xA10
(G+0x1728, RVA 0x7F843) and installs the host's local-client connection
(H+0x5C8, RVA 0x7D140, section 4.3). So C+0xA10 and H+0x5C8 are valid before
the host thread T5 starts but after T1, T2, T3, T4 are already running.

Dormant slots: G+0x1FF8/G+0x2000 ("receive thread") and G+0x2008/G+0x2010
("send thread") are cleared by the Game constructor (RVA 0x7EEE4..0x7EEF9).
No code in the image stores a started thread into them (whole-image listing
search for every access to these displacements). Their joins are guarded by
"id is non-zero" and therefore never execute.

### 1.2 Priorities and the per-thread scheduler

- T4 (Game generation thread) sets its own priority to "below normal" (-1) at
  start (RVA 0x84FD4), then creates a Concurrency Runtime scheduler policy
  with concurrency limits 1..4, context priority -1, and makes it the
  current-thread scheduler (RVA 0x84FDE..0x8500F).
- T3 (chunk thread) and T4, on every iteration while holding W's World lock,
  compare the dword at G+0x1BA0 with the value they last applied (initially
  1). When it changed, each thread sets its own priority to 0 (normal) when
  the new value is non-zero or -1 (below normal) when it is zero, and
  installs a new current-thread Concurrency Runtime scheduler with
  concurrency limits 1..8 (value non-zero) or 1..4 (value zero) and the same
  context priority. (T3: RVA 0x84496..0x8450A; T4: RVA 0x85068..0x850D9.)
- No other thread calls a priority or affinity function. The main thread is
  never re-prioritised.

### 1.3 Ownership

- T1 owns HW's zone streaming and HW's periodic state save (section 2.3).
- T5 owns the host protocol and HW's simulation step (section 2).
- T7 owns HW's creature AI (section 3).
- T2 owns only the reception of platform packets for C (channel 0) into
  C+0xA40.
- T3 and T4 serve the live World W (meshing and zone generation around the
  local player); T6 serves the map object at G+0x10 using a snapshot taken
  under W's World lock; T8 serves the audio object at G+0x28 and touches
  neither W nor H (it uses only the critical section at G+0x1D38).
- The main thread runs the Game update (RVA 0xA5A50) and therefore W's world
  step and the consumption of C's incoming queue (section 4).

### 1.4 Teardown order

Game destructor (RVA 0x818D0), in order:

1. Release the object at G+0x21F0 if present; run the session teardown (RVA
   0x911E0). That routine's two joins target the dormant slots and are
   skipped.
2. Clear G+0x2068.
3. For each Game thread in this order, print a wait line to standard output
   and join it if its id is non-zero (join helper RVA 0x9AF20, which raises
   the runtime "no such process"/"resource deadlock" errors when the thread
   is not joinable or is the calling thread, then clears the slot): music
   (G+0x1FE8; after this join the audio object's table entry +0x20 is
   called), ai (G+0x1FD8), generation (G+0x1FA8), chunk (G+0x1FB8), host
   (G+0x2018, message "waiting for host thread..."), map (G+0x1FC8), receive
   (G+0x1FF8, dormant), send (G+0x2008, dormant).
4. Later (RVA 0x8222A) the host destructor RVA 0xF1F20 runs: clear H+0x550,
   print "[HOST] waiting for generation thread...", join T1 (H+0x558), delete
   the critical sections H+0x5D0 and H+0x5F8, send a 4-byte message holding
   the dword 2 to every peer in H+0x568 whose id is not the local id
   (platform send kind 2, channel 0), destroy the 0x150-byte peer objects,
   free all host containers, then destroy HW.

The client destructor (RVA 0x3B100), run as part of Game destruction, first
switches to the local id (RVA 0x3D820), then deletes the client lock C+0xA18,
then frees the buffers still queued in C+0xA40, then clears C+0x08, then joins
T2. The lock is therefore deleted before T2 has been told to stop.

When the host thread T5 leaves its loop it tail-calls the host reset (RVA
0xF4FF0) on its own thread, i.e. while the Game destructor waits in the join
of step 3.

---------------------------------------------------------------------------

## 2. The host

### 2.1 Host thread loop (T5, entry RVA 0x83E60, complete owner)

State: one dword "previous start" P, initialised to the millisecond tick on
entry.

While G+0x2068 is non-zero, each iteration:

1. Remote-host test (RVA 0x3E2D0 on C): true exactly when the identifier at
   C+0x70 passes the identifier validity predicate (RVA 0x3BE00) and differs
   from the client's local id. No lock is taken for this read.
2. If true: sleep 33 ms; P is NOT updated; go to step 6.
3. Otherwise: S = millisecond tick; call the host tick RVA 0xF5520 with H and
   the elapsed time S - P (dword).
4. E = millisecond tick - S. If E < 32 (unsigned): sleep (32 - E) ms. If
   E >= 32: no sleep.
5. P = S.
6. Loop test on G+0x2068.

After the loop: tail-call the host reset RVA 0xF4FF0 on H (section 6.1).

Consequences: the nominal tick period is 32 ms measured start-to-start; a tick
that takes 32 ms or longer is followed immediately by the next one; after an
idle period (step 2) the first tick receives the whole idle span as its
elapsed time, which the tick caps at 1000 ms. There is no catch-up loop at
this level: falling behind only enlarges the next elapsed time.

The host tick runs regardless of whether any remote guest is connected; with
an empty peer map the per-peer phases simply do nothing. Neither the host
thread nor the host tick reads the role byte W+0x358 (of either World).

### 2.2 Host tick (RVA 0xF5520), per-call order

Inputs: H; elapsed milliseconds D (signed dword). Output: none. Bounded
reading: steps 1-6 and 8-14 and the head of step 7 were read instruction by
instruction; the per-peer message construction of step 7 was read by
structure (section 12).

1. Cap: D = 1000 if D > 1000 (signed comparison; a negative D is passed on
   unchanged).
2. Enter the host connection lock H+0x5F8. It is held until step 14.
3. Platform receive: R0 = millisecond tick. While the platform networking
   provider reports an available packet on channel 1: read it (size and
   sender id) into a fresh buffer and, when the read succeeds, hand it to the
   host message handler RVA 0xF4490 (H, sender id, buffer). Then
   R1 = millisecond tick; when R1 - R0 > 8 ms, write "Host receive ",
   the elapsed value, " | ", a boolean that is 1 when at least one packet was
   available, and a line end to standard output.
4. Hand-over queue: enter the queue lock H+0x5D0; move every entry of the
   list headed at H+0x620 (count at H+0x628) into a local list and empty the
   host list; leave H+0x5D0. Then, for each moved entry in queue order, call
   the host message handler with the entry's sender id (+0x10) and buffer
   (+0x18), and free the buffer. Entries are produced by the hand-over
   routine RVA 0xF32C0 (section 4.3); for the in-process client the sender id
   is the client's local id.
5. Store the millisecond tick into the global at RVA 0x551730 (lazily
   initialised once per process).
6. Pending identifiers: for each entry of the ordered set H+0x578 (64-bit id
   at entry+0x1C, last-contact tick at entry+0x24) that has no entry in the
   peer map H+0x568: when now - last > 5000 ms, set last = now and send an
   8-byte message (dword 0, dword 100001 = 0x186A1) to that id through the
   platform provider (send kind 2, channel 0). (Reconstruction lead, not
   confirmed: these are outstanding invitations.)
7. Per-peer phase, for every peer of the map H+0x568 (64-bit id key at
   node+0x20, 0x150-byte peer object at node+0x28): the peer's creature is
   looked up by the 64-bit identifier at peer+0x00 in HW's creature lookup
   table (H+0x458 = HW+0x450). If it is not found or null, the peer is
   skipped. Otherwise:
   a. Visibility: for every creature of HW's creature list (H+0x48 =
      HW+0x40) other than the peer's own creature: when its byte +0x60 is
      non-zero, it is excluded when the distance helper RVA 0xF3E20 (creature
      position +0x10, peer creature position +0x10, constant 270.0) reports
      true. A creature not yet in the peer's known set (peer+0x140) is
      admitted only if either this peer's one-per-tick allowance is still
      available (the allowance is then consumed) or the creature's dword
      +0x80 has any of the bits 0x2800 set. Already-known creatures are
      always admitted.
   b. Messages to the peer are sent in this fixed order of leading kind
      dwords: one kind 3 message per newly visible creature (64-bit id
      followed by a serialised creature state), then kind 6, kind 8, kind 7
      (an aggregated update built from the peer's pending containers), and
      kind 14 (0x0E). Every send goes through the host send routine RVA
      0xF4F50 (section 4.3). The payload meanings are UNRESOLVED here.
   c. The peer's known set is replaced by the new visible set.
   A temporary removal list is constructed before this phase (RVA 0xF5A8A)
   and, after it, every entry is passed to the peer removal routine RVA
   0xF3CF0 (call at RVA 0xF6897). No insertion into that list exists in the
   tick's body and its address is passed to no callee, so in the supported
   image it is always empty and that removal call never executes.
8. HW's World lock is NOT held during steps 3 to 7. The lookups of steps 7a
   and 7c read HW's creature list and lookup table under H+0x5F8 only.
9. Collect guest-produced pending data: a fresh packet object is built
   (RVA 0x3AC90) and RVA 0xF3FD0 moves into it, in this order, the contents
   of H+0x588, H+0x5B8, H+0x598 and H+0x5A8, emptying each host container.
10. Enter HW's World lock (H+0x08+0x3C8).
11. Apply the collected packet to HW (RVA 0x29D980).
12. Step HW: let N = D. When N > 16: run ((N - 17) div 16) + 1 world steps of
    exactly 16.0 ms (RVA 0x2DA6C0, float argument from RVA 0x4ACA34), then
    one final step with the integer remainder N - 16 * that count, which lies
    in 1..16. When N <= 16 (including 0): one step with N. All arithmetic is
    integer milliseconds converted to float only at the call. Then call RVA
    0x2B68E0 on HW with the whole D as float, then RVA 0x2E30B0 on HW.
13. Post-step distribution: for every record of the list at H+0x168
    (HW+0x160) whose dword +0x10 is 0, every peer's known set (peer+0x140) is
    cleared; then RVA 0xF33B0 distributes HW's outgoing block at H+0x98
    (HW+0x90) into the peers' pending containers, and the block is replaced
    by a freshly constructed empty one.
14. Leave HW's World lock; leave H+0x5F8.

Observable timing consequence: data produced by HW's step in one tick is
distributed into peer-pending state at step 13 and transmitted to peers during
step 7 of the following tick. Guest input received in steps 3-4 is applied to
HW in step 11 of the same tick (for collected containers) or directly by the
message handler (handler internals not closed, limit L3).

Step size and maximum: 16 ms slices, at most 1000 ms of simulation per tick
(at most 62 full slices plus one remainder slice). Clock source:
timeGetTime only.

### 2.3 Host generation thread (T1, entry RVA 0xF2340, complete owner)

Entry: two millisecond ticks are read; the second is kept as "last save"
(LS). Loop while H+0x550 is non-zero:

1. Enter HW's World lock. Walk HW's creature list (HW+0x40); for each
   creature whose byte +0x60 is 0, record its horizontal block position: the
   pair of dwords at +0x1B0/+0x1B4 when its byte +0x68 equals 0x9C, otherwise
   the 64-bit positions +0x10 and +0x18 shifted right by 16 (rounding toward
   zero). Leave the World lock.
2. Candidate zones (no lock held): for each recorded position (x, y), with
   zone coordinates obtained by floor division by 64, every zone (zx, zy)
   from floor((x - 270)/64) to floor((x + 270)/64) and likewise for y is a
   candidate when |zx - cx| <= 5, |zy - cy| <= 5 and
   (zx - cx)^2 + (zy - cy)^2 <= 25 (constant RVA 0x43FD20), where (cx, cy)
   is the creature's own zone. Candidates go into an ordered set.
3. RVA 0x2D2670 is applied to HW with that set (it uses HW's zone lock;
   internals not closed).
4. For each candidate not present (or present with a null zone) in HW's
   zone hash (FNV-1a over the 8-byte zone key; table fields HW+0x470,
   HW+0x480, HW+0x498): compute the smallest squared block distance from any
   recorded position within 384 blocks on both axes to the zone square
   [zx*64, zx*64+64] x [zy*64, zy*64+64] (0 when inside). Candidates with no
   qualifying position are dropped; the others are kept as (zx, zy,
   distance).
5. If no candidate remains: sleep 5 ms. Otherwise: order them by the helper
   RVA 0xF0790, keep at most the first 4, and create each zone in order with
   the zone creation routine RVA 0x2AE0C0 (HW, zx, zy). Zone creation
   generates outside any lock and inserts the zone while holding HW's zone
   lock and then HW's World lock (in that nesting order).
6. Sleep 0 (yield).
7. Periodic state save: let N be the millisecond tick read right after the
   creature snapshot of step 1 (RVA 0xF24E3). When N - LS > 60000 ms and
   H+0x550 is still non-zero: enter HW's World lock,
   copy HW's world-state block (dword HW+0x08, float HW+0x0C and the three
   ordered containers at HW+0x10, HW+0x20, HW+0x30), leave the lock, then
   write the copy through RVA 0x2D38E0 as the blob under the key "state" into
   HW's key/value store (HW+0x3B8), and set LS to N.

On exit: the same copy under HW's World lock and the same "state" write run
once more.

### 2.4 Locks owned by H

| Lock | Initialisation | Protects |
| --- | --- | --- |
| H+0x5F8 "connection lock" | host constructor, spin count 16 | the host tick as a whole; the peer map H+0x568, the pending set H+0x578, the pending containers H+0x588..H+0x5B8, the host reset, peer removal |
| H+0x5D0 "queue lock" | host constructor, spin count 16 | only the hand-over list H+0x620 / H+0x628 |

(Reconstruction lead, unconfirmed naming: "connections lock" and "received
packets lock".)

---------------------------------------------------------------------------

## 3. Creature AI

### 3.1 Thread (T7, entry RVA 0x83F20, complete owner)

State: P = millisecond tick on entry. While G+0x2068 is non-zero:

1. S = millisecond tick. Call the AI routine RVA 0x13830 with the controller
   at G+0x768 (HW's controller) and the elapsed dword S - P. Set P = S.
2. E = millisecond tick - S. If E < 40: sleep (40 - E) ms; otherwise sleep
   1 ms.

There is no pause condition: AI keeps running while the client is attached
to a remote host and while H is idle. The thread does not cap the elapsed
value (an internal cap inside RVA 0x13830 was not closed). Clock source:
timeGetTime only. Nominal period 40 ms start-to-start.

### 3.2 Which World

Only HW. RVA 0x13830 has exactly one caller (RVA 0x83F66). No code ticks the
controller at W+0x78. The controller's World back pointer is read from its
first qword; for HW it is HW.

### 3.3 Synchronisation with HW's step (bounded reading)

The AI routine does not take H+0x5F8. It works in phases:

1. Under HW's World lock (entered RVA 0x138BC, left RVA 0x13AFF): walk HW's
   creature list. When the World's role byte (+0x358) is non-zero only the
   creature at World+0x448 is processed (for HW the byte stays 0 because only
   W's byte is ever set by a host switch, so all creatures are processed).
   For each eligible creature the per-creature AI record (keyed lookup into
   A+0x08) receives a snapshot of position (+0x10..+0x20), velocity-like
   dwords (+0x84..+0x8C), +0x5C, the word +0x128, and +0x174..+0x17C, and the
   per-creature update RVA 0x14E00 runs with the elapsed value.
2. For each AI record (no World lock at the loop head): yield (sleep 0);
   take HW's World lock for a read section (RVA 0x13BD9 .. RVA 0x1400E or
   0x140BC), release it, then take HW's zone lock for the path/decision
   section (RVA 0x140C4 .. RVA 0x148B2; this section yields with sleep 0 at
   RVA 0x1451F and 0x145F7). The two locks are taken one after the other, not
   nested, on these paths.
3. Under HW's World lock again (RVA 0x1492A .. 0x14CE2): a second walk over
   HW's creatures applies results (same role-byte rule as phase 1).

Because the host tick holds HW's World lock for steps 10-14 of section 2.2,
the AI phases 1 and 3 and the read section of phase 2 are mutually exclusive
with HW's step, message application and post-step distribution, but they
interleave freely with the host tick's steps 2-9 (which do not take HW's World
lock). There is no handshake, event or ordering between an AI pass and a host
step; the two run on independent 40 ms and 32 ms cadences.

---------------------------------------------------------------------------

## 4. The live World W while playing through the in-process host

### 4.1 What the Game update advances (main thread)

Frame routine RVA 0x134420, called from the main loop (RVA 0x137B8E):

1. Raise the timer resolution to 1 ms, read the millisecond tick T0, restore
   the timer resolution.
2. Frame time = the high-resolution performance-counter difference since the
   previous frame, converted to milliseconds (double). Call the Game update
   RVA 0xA5A50 with it.
3. After presenting: raise the timer resolution to 1 ms; when
   (millisecond tick - T0) < the dword frame budget at G+0x1BBC, sleep for
   the difference; restore the timer resolution.

The main loop sleeps a further 16 ms before calling the frame routine on some
paths (governed by the global byte at RVA 0x536CE8 and a test at RVA
0x136645; not closed, limit L6).

Game update RVA 0xA5A50 (bounded windows):

1. Cap the frame time at 1000.0 ms (constant RVA 0x437FD8).
2. Enter W's World lock (RVA 0xA6464). It is held until RVA 0xADB23.
3. When the requested and applied world pairs match (companion contract
   section 6): while the remaining time exceeds 16.0 ms, step W by 16.0 ms
   (RVA 0x2DA6C0) and subtract 16.0; then step once with the remainder
   (double arithmetic, converted to float for the call).
4. Later in the frame: RVA 0x2B68E0 on W with the whole capped frame time
   (RVA 0xA7E80), and RVA 0x2E30B0 on W (RVA 0xA8498).
5. Client service RVA 0x3F4F0 on C (RVA 0xAAEBF), section 4.3.
6. Leave W's World lock.

### 4.2 What W's world step does when W mirrors the host

W's world step RVA 0x2DA6C0 (bounded reading by structure):

- Every 16 ms slice, for every creature in W's list (W+0x40), regardless of
  W+0x358: when the creature's float +0x180 exceeds about 1.5e-5 (constant
  RVA 0x42E510) three per-creature routines run (RVA 0x2BB8E0, 0x2C5250,
  0x2C3710), then the per-creature movement routine RVA 0x2DE4C0 always runs.
  So W integrates movement for mirrored creatures between host messages.
- When W+0x358 is 0 only: RVA 0x2E17D0 (with the slice), RVA 0x2A56C0 and
  RVA 0x2A45E0 (with the slice) run near the start of the step, and a block
  that processes the World's record list at W+0x160 runs near the end. With
  W+0x358 = 1 (after any host switch, including to the local host) these are
  skipped.
- Inside the per-creature movement routine and the sub-step RVA 0x2DBA40
  there are further branches on W+0x358 and on "creature is W+0x448"; their
  content is UNRESOLVED (limit L4).
- No AI runs for W (section 3.2).

### 4.3 How H's results reach W

Host side, per message (host send routine RVA 0xF4F50, complete owner):

- When the destination id equals the host's local id (global RVA 0x5516F8):
  if H+0x5C8 is non-null, reset the buffer's read position (dword +0x18) to 0
  and call the first table entry of the object at H+0x5C8. That object is the
  0x18-byte local-client connection (table RVA 0x437E98) created by RVA
  0x7D140 with the client C and the enqueue routine RVA 0x3CB50; its first
  entry (RVA 0x8A360) calls RVA 0x3CB50 (C, sender, buffer). If H+0x5C8 is
  null the message is dropped.
- Otherwise: platform send to the id, send kind 2, channel 0.

Client enqueue RVA 0x3CB50 (complete owner): copy the message into a new
0x20-byte buffer object; enter the client lock C+0xA18; append (sender id,
buffer) to the list headed at C+0xA40 (count C+0xA48); leave C+0xA18. This is
synchronous on the host thread T5, inside the host tick, while H+0x5F8 is
held. The client receive thread T2 appends platform packets to the same list
under the same lock.

Client receive thread T2 (RVA 0x3BAE0, complete owner): while C+0x08 is
non-zero: run the platform callback pump; while a packet is available on
channel 0, read it, append it to C+0xA40 under C+0xA18 as above (only when the
read succeeded), and yield (sleep 0); when no packet remains, sleep 16 ms.

Main thread consumption (client service RVA 0x3F4F0, bounded): under W's
World lock (held by the Game update), enter C+0xA18, move the whole list
C+0xA40 into a local list and empty it, leave C+0xA18; then handle each entry
in arrival order with the client message handler RVA 0x3E450 (which writes
into W), freeing each buffer. Afterwards the service performs the 5000 ms
current-host timeout (companion contract section 5.5), a 6000 ms known-peer
timeout over C+0xA00, and, at most once per more than 32 ms (global tick at
RVA 0x54DE00) and subject to identifier validity tests on C+0x70/C+0x74,
sends a kind 3 message with the local player's identifier and serialised
state to the current host; when the current host is the client's local id
this goes through the hand-over RVA 0xF32C0 (RVA 0x3FAB7).

Summary of the W <- H path: copy (by value) of each message into the client
queue, protected only by C+0xA18; applied to W on the main thread once per
frame, after that frame's W steps, under W's World lock. Latency is therefore
up to one host tick (32 ms) plus up to one frame. There is no shared memory,
no direct World-to-World copy and no lock held across both Worlds.

The direct hand-over RVA 0xF32C0 (complete owner) is the reverse direction
(C -> H): allocate a 0x20-byte buffer copy (dword +0x18 cleared), enter
H+0x5D0, append (sender id, buffer) to H+0x620 (count H+0x628), leave H+0x5D0.
Callers: the host switch (RVA 0x3D8CE, 0x3DAC2), the client send helper RVA
0x3F460 (RVA 0x3F499) and the client service (RVA 0x3FAB7). The client send
helper uses it when the destination equals the client's local id and C+0xA10
is non-null, otherwise a platform send kind 2 on channel 1 (dropped when the
destination is local and C+0xA10 is null).

---------------------------------------------------------------------------

## 5. Locks and protection

### 5.1 Inventory

| Lock | Holder routines (work done under it) |
| --- | --- |
| W+0x3C8 (W World lock) | Game update RVA 0xA5A50 for most of the frame (world steps, client service and thus message handling and any host switch started there); selection routine RVA 0x8A500; world-list persistence RVA 0xA2BE0; map thread T6 for a 24-byte snapshot of G+0x1E20..G+0x1E37 and the dword G+0x1E04; chunk thread T3 and generation thread T4 for snapshots of the local player position and settings; zone creation (inner lock) |
| W+0x3F0 (W zone lock) | zone creation RVA 0x2AE0C0 (outer lock); chunk thread T3 (RVA 0x84604..0x846FC); World helpers RVA 0x2D2410, 0x2D2670, 0x2D2930; other users not enumerated (limit L5) |
| HW+0x3C8 (H+0x3D0) | host tick steps 10-14; host generation thread (creature snapshot, state copy); AI phases 1, 2 (read section) and 3; zone creation on HW (inner) |
| HW+0x3F0 (H+0x3F8) | AI phase 2 path section; RVA 0x2D2670 from the host generation thread; zone creation on HW (outer) |
| H+0x5F8 | host tick (entire body), host reset RVA 0xF4FF0, peer removal RVA 0xF3CF0 (re-entered from the tick) |
| H+0x5D0 | hand-over append RVA 0xF32C0; queue swap in host tick step 4; queue purge in the host reset |
| C+0xA18 | client enqueue RVA 0x3CB50; T2 append; queue swap in client service; deleted by the client destructor |
| G+0x1D38 | music thread T8 and its producers only (unrelated to Worlds) |

All are critical sections (re-enterable by the owner). The only other
synchronisation objects are the runtime mutex/condition pair used once per
thread start (section 1). No event, semaphore or wait-for-multiple-objects is
used on these paths.

### 5.2 Observed nesting (outer -> inner)

- Main thread: W+0x3C8 -> C+0xA18 (queue swap; released before handling);
  W+0x3C8 -> H+0x5D0 (hand-over sends from the client service and from a host
  switch); W+0x3C8 -> H+0x5F8 -> H+0x5D0 (host reset started by a host switch
  inside the client service).
- Host thread T5: H+0x5F8 -> H+0x5D0 (released before handling);
  H+0x5F8 -> C+0xA18 (every message to the local peer); H+0x5F8 -> HW+0x3C8
  (steps 10-14). T5 never takes W+0x3C8 in the routines read.
- Zone creation (any World): World+0x3F0 -> World+0x3C8.
- AI thread T7: HW+0x3C8 and HW+0x3F0 sequentially, not nested, on the paths
  read.

No cycle between W+0x3C8, H+0x5F8, H+0x5D0 and C+0xA18 was observed: C+0xA18
and H+0x5D0 are leaves.

### 5.3 Unprotected accesses (facts, not advice)

- T5 reads C+0x70 without any lock.
- Host tick steps 3-7 read HW's creature list and lookup table while holding
  only H+0x5F8, concurrently with T1 and T7, which use HW's World lock.
- The host reset (section 6.1) destroys every HW creature without HW's World
  lock.
- The client destructor deletes C+0xA18 before stopping T2.

---------------------------------------------------------------------------

## 6. Host reset and host switch

### 6.1 Host reset RVA 0xF4FF0 (complete owner)

Input: H. Order:

1. Enter H+0x5F8.
2. For every peer of H+0x568 whose id differs from the host's local id: send a
   4-byte message holding the dword 2 through the platform provider (send kind
   2, channel 0). For every peer: destroy its 0x150-byte peer object (RVA
   0xF1E10) and free it.
3. Empty the peer map H+0x568 and the pending set H+0x578 (all nodes freed,
   counts 0); empty the lists H+0x588, H+0x598, H+0x5A8, H+0x5B8.
4. Enter H+0x5D0; free every queued buffer of H+0x620 and empty the list;
   leave H+0x5D0.
5. RVA 0x2D22E0 on HW: destroy every creature of HW+0x40 through its first
   table entry with flag 1 (no exception for any creature), empty that list
   (count HW+0x48 = 0), empty the creature lookup table HW+0x450 and the list
   HW+0x50. No lock is taken by this routine.
6. Leave H+0x5F8.

Not touched: H+0x550, H+0x558 (T1), any Game thread, H+0x5C8, HW's zones, HW's
store, HW's AI records at HW+0x78+0x08, HW's role byte.

Callers: the host switch (RVA 0x3DA5D, only when C+0xA10 is non-null) and the
host thread T5 on exit (RVA 0x83F10).

### 6.2 Peer removal RVA 0xF3CF0 (complete owner)

Enter H+0x5F8; remove the id from H+0x578; find the peer in H+0x568; when
found: if the peer's creature exists in HW's lookup table, remove it from HW
(RVA 0x2D2260); destroy and free the peer object; erase the map node. Leave
H+0x5F8. Effective caller: the host message handler (RVA 0xF45A9), i.e. on the
host thread inside the tick. The tick's own call site (RVA 0xF6897) iterates an
always-empty list (section 2.2 step 7).

### 6.3 Host switch RVA 0x3D820 as seen by the host threads

The switch (companion contract section 5.5) runs on the thread of its caller:
the main thread when started by the client service or the client message
handler (W's World lock held), the interface-dispatch thread of the
multiplayer list (not closed, companion limit L2), or the destroying thread.
Effects on host threads:

- Old host was the local id: a 4-byte "dword 2" message is appended to
  H+0x620 through RVA 0xF32C0. Because step 5 of the switch then calls the
  host reset, which purges H+0x620, this message is discarded unless T5
  drained the queue in between (timing dependent).
- The host reset runs (when C+0xA10 is non-null): it waits for any running
  host tick to finish (H+0x5F8) and empties H's peers, queues and HW's
  creatures, as in 6.1.
- New host is the local id: a 4-byte "dword 1" message is appended to H+0x620;
  T5 handles it at step 4 of its next tick.
- No thread is created, paused, signalled, stopped or joined. H+0x550 and
  G+0x2068 are unchanged; T1 and T7 keep running throughout.
- T5's behaviour follows C+0x70 (set at the end of the switch): while it holds
  a valid non-local id, T5 idles (33 ms sleeps, no tick, P frozen); when it is
  the local id (or invalid, e.g. zero), T5 ticks. Switching back to the local
  id therefore resumes ticking with a first elapsed time of at most 1000 ms.
- The switch also purges W's non-local creatures and undoes W's zone changes
  and sets W+0x358 to 1 (companion contract); it never writes HW+0x358.

---------------------------------------------------------------------------

## 7. Explicit limits

- L1. C++ runtime thread start/join, condition variable and Concurrency
  Runtime scheduler internals (how the policy's concurrency limits and
  context priority map onto worker threads) are provider boundaries.
- L2. Platform networking provider: only "packet available / read on a
  channel", "send to id with send kind 2 on channel 0 or 1" and the callback
  pump are asserted; delivery guarantees are not.
- L3. Host message handler RVA 0xF4490, the packet-apply routine RVA
  0x29D980, the post-step distributor RVA 0xF33B0, and the message payload
  formats of kinds 3, 6, 7, 8, 14 were read by structure only. No World lock
  call appears in the handler's own body; its callees were not exhaustively
  checked.
- L4. W's world step RVA 0x2DA6C0, its sub-step RVA 0x2DBA40, the per-creature
  routines RVA 0x2DE4C0, 0x2BB8E0, 0x2C5250, 0x2C3710, and RVA 0x2B68E0 /
  0x2E30B0 were read by structure; only the role-byte gates listed in 4.2 are
  asserted.
- L5. Not every user of the zone lock World+0x3F0 was enumerated.
- L6. The main-loop sleep conditions (global byte RVA 0x536CE8, test at RVA
  0x136645), the global byte at RVA 0x5514C7 that makes T3, T4 and T6 sleep
  100 ms per iteration (no writer found by direct-address search), and the
  frame budget G+0x1BBC origin.
- L7. Timer resolution: the main thread raises the multimedia timer resolution
  to 1 ms only around its own frame-limit sleep; the effective granularity of
  the Sleep calls in T1, T2, T5, T7 is a runtime property.
- L8. The AI routine RVA 0x13830 and its callees (per-creature update RVA
  0x14E00, RVA 0x15640) were read in bounded windows; any internal elapsed
  cap is not closed.

---------------------------------------------------------------------------

## 8. UNRESOLVED

- U1. Whether the dword-2 "leave" message appended by a host switch is ever
  handled by T5 before the host reset purges it (timing).
- U2. Semantics of the host message kinds and of the pending containers
  H+0x578, H+0x588, H+0x598, H+0x5A8, H+0x5B8 (L3).
- U3. The ordering helper RVA 0xF0790 used to choose the four nearest zones
  (assumed ascending distance from usage, not proven).
- U4. The distance metric and units of the helper RVA 0xF3E20 with 270.0.
- U5. The exact effect of the W+0x358 branches inside the per-creature
  movement routine (L4), i.e. how far W extrapolates versus defers to host
  messages for the local player and for mirrored creatures.
- U6. The runtime consequences of the unprotected accesses in 5.3.
- U7. The thread on which the multiplayer-list host switch runs and the W lock
  state at that moment (companion limit L2).
- U8. The meaning of G+0x1BA0 (priority/concurrency toggle) and G+0x1B9C
  (distance setting used by T3 and T4).
- U9. Every limit L1 to L8.

---------------------------------------------------------------------------

## 9. Relation to the companion contracts

- Closes the pending-source contract's limit L5 for the host's threads: the
  host thread is T5 (Game-created) and the host's own thread is T1
  (generation and state save for HW).
- Clarifies the direction of the direct hand-over RVA 0xF32C0 (consistent with
  that contract's section 5.5): it is client-to-host only. It is not the path
  by which H's results reach W; host-to-local-client delivery goes through
  H+0x5C8 into C+0xA40 (section 4.3).
- Confirms its 16 ms slicing of the Game update and the 5000 ms current-host
  timeout.

---------------------------------------------------------------------------

## 10. Reconstruction lead usage

After the original evidence had been read, one reconstruction-owned file of
the release tree (the host translation unit) was searched for thread, sleep,
lock and timing keywords and about 25 matching lines were viewed. They agree
with the original evidence on: a host-owned generation thread with sleeps of
5 ms and 0 ms and the "[HOST] waiting for generation thread..." message; two
host critical sections (a connection lock and a received-packet lock); a
timed "invitation" set; the 1000 ms cap; 16 ms world steps in the host tick.
Mismatches: NONE found. The label "invitations" for H+0x578 is recorded as an
unconfirmed lead. No reconstruction names, expressions or structure are
reproduced here.

---------------------------------------------------------------------------

## 11. Constants at a glance

| Value | Where | Meaning |
| --- | --- | --- |
| 32 ms | RVA 0x83ED5 | host thread target period |
| 33 ms | RVA 0x83EA3 | host thread idle sleep while attached to a remote host |
| 1000 ms | RVA 0xF5567; RVA 0x437FD8 | host tick cap; Game update frame-time cap |
| 16.0 ms | RVA 0x4ACA34; RVA 0x437FC8 / 0x438030 | slice size (host; Game update) |
| 8 ms | RVA 0xF56DE | host receive diagnostic threshold |
| 5000 ms | RVA 0xF5909 | pending-id resend interval |
| 100001 | RVA 0x43F3AC | dword sent to pending ids |
| 40 ms / 1 ms | RVA 0x83F75..0x83F83 | AI thread period / minimum sleep |
| 5 ms / 0 ms | RVA 0xF2C26 / 0xF2CCC | host generation thread sleeps (idle / after work) |
| 60000 ms | RVA 0xF2CDE | HW state save interval |
| 270, 5, 25, 384, 4 | RVA 0xF2533.., 0xF2668, 0x43FD20, 0xF288E, 0xF2C7F | generation reach (blocks), zone box, zone radius squared, distance window (blocks), zones per round |
| 270.0 | RVA 0x42E7DC | host visibility distance constant |
| 0x2800 | RVA 0xF5BF7 | creature flag bits that bypass the one-new-per-tick allowance |
| 16 ms | RVA 0x3BCD0; RVA 0x84020; RVA 0x137B83 | client receive idle sleep; music thread sleep; main-loop pre-frame sleep |
| 10 ms / 100 ms / 5000 ms | RVA 0x83D3B / 0x83CB9 / 0x83D11 | map thread period / paused period / map refresh interval |

---------------------------------------------------------------------------

## 12. Evidence manifest

Supported image:

| Item | Value |
| --- | --- |
| Retail file | cubeworld.exe, Steam release, 64-bit |
| Retail size | 5,939,072 bytes |
| Retail SHA-256 | A1EFBBD41EEB8DF08395C4D6511194D416CB70A14054B2A2000B9F5CDBCFA65B |
| Installed copy | identical SHA-256 and size (checked 2026-11-11, read-only) |
| Analysis image | unpacked derivative, 5,757,952 bytes |
| Analysis SHA-256 | 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244 |
| Preferred base | 0x140000000 |

All RVAs refer to the analysis image. Evidence was static only: the
instruction listing, the call-reference table, the function range table, the
import and symbol tables, the string table, and read-only byte reads of the
analysis image for constants and tables. No executable was run.

Complete owners (every instruction read; counts from the listing):

| RVA range | Role | Instructions |
| --- | --- | --- |
| 0xF17A0..0xF1A40 | host constructor | 160 |
| 0xF0150..0xF02B4 | host thread launch | 92 |
| 0xF3220..0xF32B5 (+ stub 0xF3070) | host thread trampoline | 46 |
| 0xF2340..0xF2F61 | host generation thread entry | 829 |
| 0xF1F20..0xF2338 | host destructor | 258 |
| 0xF32C0..0xF33AE | client-to-host hand-over | 53 |
| 0xF4FF0..0xF53A6 | host reset | 234 |
| 0xF4F50..0xF4FE2 | host send | 38 |
| 0xF3CF0..0xF3E11 | peer removal | 83 |
| 0xF3FD0..0xF4163 | pending-data collection | 94 |
| 0x9AF20..0x9AF92 | thread join helper | 31 |
| 0x83C70..0x83D85 | map thread entry | 65 |
| 0x83E60..0x83F14 | host thread entry | 48 |
| 0x83F20..0x83FAB | ai thread entry | 38 |
| 0x83FB0..0x842C2 | music thread entry | 199 |
| 0x88E40..0x891F5 | six Game thread trampolines | 270 |
| 0x75BA0..0x76434 | six Game thread launch routines | 552 |
| 0x87BA0..0x87BF0 | six table stubs | 6 |
| 0x3BAE0..0x3BD2C | client receive thread entry | 150 |
| 0x3E2D0..0x3E301 | remote-host test | 18 |
| 0x3F460..0x3F4E8 | client send helper | 35 |
| 0x3CB50..0x3CC3E | client enqueue | 53 |
| 0x8A360..0x8A36A | local-connection deliver entry | 3 |
| 0x7D140..0x7D1C5 | local-connection install | 33 |
| 0x7D560..0x7D5B7 | thread argument box | 25 |
| 0x3C7D0..0x3C7E5 | common thread start stub | 7 |
| 0x2D22E0..0x2D2406 | remove all creatures | 78 |
| 0x134420..0x1347C5 | frame routine | 192 |
| 0x911E0..0x913A4 | session teardown | 92 |
| 0x2ABB70, 0x2ABB80, 0x2B9890, 0x2B98A0 | lock helpers | 8 |
| 0x9590..0x95CA | AI controller sub-object constructor | 18 |

Totals for complete owners: 50 routines, 3,808 instructions.

Bounded partial windows (read inside the stated bounds only):

| RVA window | Owner (total instructions) | Instructions read (approx.) |
| --- | --- | --- |
| 0xF5520..0xF6070 and 0xF681C..0xF6A51 in full, remainder by call/constant filter | host tick (1,284) | 810 + filtered |
| 0x13830..0x13BE0 in full, remainder by call/lock/sleep filter | AI routine (1,334) | 260 |
| 0x842D0..0x84660 in full, remainder by filter | chunk thread (856) | 300 |
| 0x84F70..0x851A3 and 0x855E4..0x85740 in full, remainder by filter | Game generation thread (562) | 260 |
| filter on role byte, local-player pointer, creature loops, calls | W world step 0x2DA6C0 (1,216) | 170 |
| filter on role byte and local-player pointer | 0x2DBA40 (about 2,300), 0x2DE4C0 (about 2,000), 0x2BB8E0 head | 90 |
| 0x818D0..0x81C00 and 0x82200..0x82267 | Game destructor (592) | 340 |
| 0x3B100..0x3B240 | client destructor (215) | 95 |
| filter plus 0x3ABC5..0x3AC0F | client constructor (131) | 80 |
| 0x3F4F0..0x3F629 in full, 0x3F63F..0x3FAC0 by filter | client service (882) | 280 |
| 0x7F760..0x7F9C0 plus single stores 0x7EB36, 0x7EB46, 0x7EEE4..0x7EEF9, 0x7EF63 | Game constructor | 145 |
| 0x29A130..0x29A250 | World constructor | 75 |
| 0xA5A50..0xA5C40, 0xA6440..0xA6480, 0xA72C0..0xA7380, 0xA7E50..0xA7E90, 0xA8470..0xA84A0, 0xAAE90..0xAAED0, 0xADB10..0xADB30 | Game update | 180 |
| 0x136440..0x136520, 0x137B60..0x137BB0 | main routine loop | 70 |
| lock section and call filter | zone creation 0x2AE0C0 (192) | 40 |
| call filter | host message handler 0xF4490, packet apply 0x29D980, distributor 0xF33B0 | 160 |
| key and store write | state save 0x2D38E0 | 30 |
| call filter | 0x2B68E0, 0x2E30B0 heads | 30 |
| filter | launch routine 0x38800 (92), trampoline 0x3C920 (45) | 55 |

Totals for partial windows: 20 owners, about 3,470 instructions.

Whole-image checks performed:

- Import table: every thread, priority, scheduler-policy, critical-section,
  mutex, condition, event, wait, sleep and timer import was identified, and
  the call-reference table was searched for every call site of each. Thread
  start sites: exactly eight (RVA 0x388C0, 0x75C60, 0x75DD0, 0x75F40,
  0x760B0, 0x76220, 0x76390, 0xF0210). Priority calls: only RVA 0x844BB,
  0x84FD4, 0x8508B. Event and wait imports are used only inside the runtime
  support at RVA 0x392BF0..0x392EF9.
- Call-reference lookups for every caller of each launch routine, trampoline,
  host tick, host reset, hand-over, AI routine (single caller), world step
  (callers: Game update twice, host tick twice), RVA 0x2B68E0 and 0x2E30B0
  (each: Game update and host tick), zone creation, state save, and the lock
  helpers.
- Listing searches for every access to G+0x1FF8, G+0x2008, G+0x2000,
  G+0x2010 (dormant slots), every byte store to G+0x2068 and H+0x550, every
  store to displacement 0x5C8, every reference to the global at RVA 0x5514C7,
  and every address formation of displacement 0x3F0.
- Byte reads of the constants listed in section 11 and of the six thread-pad
  tables (RVA 0x437EA8..0x437EF8), the host pad table (RVA 0x43FD18) and the
  local-connection table (RVA 0x437E98).

Mismatches between reconstruction leads and original evidence: none for the
stated facts (section 10).

Prohibited-material exposure: none.
