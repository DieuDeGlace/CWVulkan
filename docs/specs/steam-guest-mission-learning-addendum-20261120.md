# Addendum: speech consumers, listener and speaker gates (RVA 0xB40D0)

Author role: isolated source-side specification author (docs/CLEANROOM_AUTHOR.md).
Date: 2026-11-20. Status: FINAL / STABLE / HELD.
Companion to steam-guest-mission-learning-20261120.md, which is held and not
edited. Same image: cubeworld.original.exe.unpacked.exe, SHA-256
7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244, base
0x140000000. Static evidence only. Infinite-Cubes was used as a lead only.

## How speech is stored (speech routine RVA 0xA3170)

The speech routine takes a speaker creature, an optional listener creature and
a list of dialog pages (0x1B0 bytes each). Confirmed behaviour:

- If the listener is the local player (world +0x448), the page list of a
  separate widget (game +0x378, +0x1E8) is cleared first.
- It finds the speech bubble whose speaker id (bubble +0x210) equals the
  speaker's id. If none exists, it creates a new bubble and appends it to the
  bubble list (game +0x400 .. +0x408).
- Pages are routed one at a time:
  - If page +0x20 is zero, the page goes to the bubble's page list
    (bubble +0x1E8).
  - Otherwise it goes to the game +0x378 widget's page list, and that widget's
    +0x200 is set to the speaker id.
- The bubble stores the speaker id at +0x210. It stores the listener id at
  +0x218, or 0 when there is no listener.
- The main speech panel (game +0x418) is not filled by this routine. Ordinary
  Talk pages therefore go to a bubble (or, when page +0x20 is non-zero, to the
  +0x378 widget). They do not go to the +0x418 panel.

## Bubble consumer (first half of RVA 0xB40D0)

Per bubble in the bubble list (game +0x400), in order:

1. The bubble's speaker id (+0x210) is looked up in the world's creature-by-id
   map (world +0x450). If the id is missing, or maps to null, the whole
   current-page block is skipped for this frame and nothing is written.
2. The speaker's displayed position is projected to the screen. This only sets
   the bubble's visibility (shown when in front of the camera, hidden
   otherwise). It is not a gate: the block below runs either way.
3. If the bubble's read time (+0x200, a float) equals 0.0, its current page
   index (+0x204) is in range, and the page exists, then for the current page:
   - The item in the page's taken-item field (page +0x30 count, item at +0x34)
     is removed from the local player's inventory.
   - The given item (count at page +0xD4, item at +0xD8) is given to the local
     player if they can take it.
   - Lore cells (page +0x190): see section (d).
   - Marked locations (page +0x1A0): see section (a)-(c).

Bubble fields that this routine never reads:

- The listener id (+0x218) appears nowhere in RVA 0xB40D0.
- The speaker is not compared with the local player in the current-page block.

The only comparison of a bubble's speaker with the local player is elsewhere in
the routine. It runs when a bubble's last page has expired: the bubble is
removed, and an interaction is queued on the local player.

- If the speaker is the local player and bubble +0x220 is non-empty, the
  interaction has kind 2 and carries that item.
- Otherwise it has kind 4, with the speaker's id as target.

That path does not write any mission or lore set.

## Panel consumer (second half of RVA 0xB40D0)

This consumer handles the main panel (game +0x418). For its current page it
requires:

- the page list is non-empty;
- the read time (panel +0x200) is 0.0;
- the index (panel +0x204) is in range.

When these hold, it applies the item take/give, then lore cells, then marked
locations. No speaker, listener, distance or visibility check exists on this
path.

## Answers

(a) A bubble with no listener (the spontaneous talking-creature path, which
calls the speech routine with listener 0): yes. Every marked location on its
current page that the local player does not already know is written. That means
an insert into +0xB8, an insert into +0x38, the timer +0xE8 set to 180000.0, and
sound 0x20. The write is gated only by "knows mission" (RVA 0x5F4C0) returning
false for that pair, and by the speaker id resolving in world +0x450.

(b) A bubble addressed to another creature (another player or an NPC): yes,
written exactly as in (a). The bubble consumer does not read the listener id
(+0x218), so the addressee makes no difference. The speaker identity matters
only in that the speaker's id must resolve to an existing creature in the local
world. The speaker is never compared with the local player before the write.
Note that the speech routine merges speech per speaker, not per listener: pages
for one speaker are appended to that speaker's single bubble whatever the
listener.

(c) Other gates before the write:

- No distance check.
- No visibility check (the screen projection only toggles display).
- No host/guest check.
- No hostility or class check.

The gates that do exist are these.

- Bubble consumer:
  1. the speaker id resolves in world +0x450 to a non-null creature;
  2. the read time is exactly 0.0, which makes the write happen once per page;
  3. the page index is in range;
  4. per pair, "knows mission" is false.
- Panel consumer: items 2-4 above, with no speaker check.

(d) Lore cells (page +0x190), each cell in page order, before the marked
locations:

- The cell is always inserted into discovered markers (+0xA8). No check of any
  kind comes before this, beyond the per-page gates in (c).
- It then calls the world's has-lore test (RVA 0x2B42E0) with the cell. Only if
  that returns true does it convert the cell to place coordinates (cell x 16,
  divided by 1024, rounded toward negative infinity) and find the realm:
  - bubble: realm lookup RVA 0x9BF20;
  - panel: RVA 0x36160 followed by RVA 0x2BB060.
- If the cell is not yet in lore-by-realm (+0x128) for that realm, it inserts
  the cell and prints two chat lines: the realm name, then " lore increased to N%"
  (N = 10 x the set size).
  - The bubble consumer then also plays sound 0x20.
  - The panel consumer plays no sound on this lore path.
- Listener and speaker play the same role as in (a)/(b): the listener is never
  read, and the speaker only has to resolve (bubble consumer only). There is no
  distance, visibility, role, hostility or class check.

## Limits

- The meaning of page +0x20 (the field that sends a page to the +0x378 widget)
  was not established.
- Whether pages routed to the +0x378 widget ever have their marked locations or
  lore applied was not traced. That widget's handler (RVA 0x965E0) reads the
  discovered-markers set but was not audited for +0xB8 writes.
- Which callers fill the +0x418 panel was not enumerated.
- Static evidence only; nothing was exercised at run time.
