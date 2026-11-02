# Steam Zone reward-map capture ABI

Independent native-only layout supplement for **Zone+0x450**. Sources:
Zone constructor **0x2f9400**, head allocation **0x2fa3f0**, real-node allocation
**0x119100**, key/value construction **0x112cf0**, insertion/balancing
**0x115750/0x113f40/0x1134b0**, lookup **0x936e0**, successor logic
**0x2fedd0**, full clear **0x2fe910**, and Zone destruction **0x2f9940**.
No engine, tests, exporter, SDK, reconstruction, owner data/captures or game
execution. These are the original Steam x64 native-memory facts, not a generic
portable C++ container ABI or an export-file format.

## 1. Wrapper and node layout

All offsets below are bytes; pointers are **64-bit native process addresses**,
and the native scalar representation is little-endian.

| Relative to Zone | Width | Meaning |
| --- | --- | --- |
| +0x450 |8 bytes | Pointer to the allocated head/sentinel node H |
| +0x458 |8 bytes, unsigned64 | Number of real entries, excluding H |

The wrapper occupies16 bytes here. It is not a pointer-vector begin/end pair,
and the count is not an object-vector index or a reward quantity. This is
**Zone+0x450**, not the unrelated World+0x450 creature-ID map.

Both the sentinel and each real node allocate **0xc0 bytes**. A real node is:

| Node offset | Width | Meaning |
| --- | --- | --- |
| +0 |8 | Left child pointer |
| +8 |8 | Parent pointer |
| +0x10 |8 | Right child pointer |
| +0x18 |1 | Red/black color:0 red,1 black |
| +0x19 |1 | Nil/sentinel flag: real node0, head1 |
| +0x1a..+0x1b |2 | No key/value meaning established here |
| +0x1c |4, signed32 | Canonical static-object index key |
| +0x20..+0xbf |0xa0 | **Inline full item value**, not a pointer to an item |

Allocation size is directly visible in0x119100 and0x2fa3f0.0x112cf0 writes
the real node's key at+0x1c and constructs its semantic empty item at+0x20.
It initially writes color0/nil0; balancing0x1134b0 can change color and makes
the root black. **A black node is not necessarily a sentinel**: the distinct
nil byte at+0x19 controls that test. The item span's existence is not a claim
that every item padding byte is initialized or meaningful; item semantics
remain the existing item-record contract.

## 2. Header meanings and known-empty representation

For head H, the same three pointer positions have special meanings:

| Head offset | Meaning in a stable nonempty tree |
| --- | --- |
| +0 | Minimum/leftmost real node |
| +8 | Root real node |
| +0x10 | Maximum/rightmost real node |

The root's parent points to H. Missing child links of real nodes point to H;
there is no separately allocated nil node for each leaf. Signed-key insertion
and the maintained head links are explicit in0x115750/0x1134b0. Lookup0x936e0
starts from **H+8**, not the leftmost pointer, and returns H on a miss without
inserting a value.

The completed constructor's **known-empty map** has a nonnull allocated H,
count0, **H.left=H.parent=H.right=H**, color1 and nil1.0x2fa3f0 initializes
the two flag bytes together to word0x0101.0x2fe910's full clear restores the
three self-links and count0 while retaining the head allocation. No head
key or item payload is initialized by the head allocator: **H is never a
reward entry**, including in an empty map.

A null head is not this completed native empty representation. The Zone
constructor briefly zeroes the wrapper before allocating/assigning H;
layout alone does not establish whether an observed null is partial
construction, stale memory or another invalid/unavailable state. A nonempty
map whose stored items have category0 is also not an empty map: its real
nodes still contribute to+0x458, although the interaction reader treats a
matching category0 value as no reward.

## 3. In-order traversal and end recognition

For a valid stable tree, the first entry is **H.left** and iterator end is
the **address H itself**. Traversing real left subtrees, then each node, then
real right subtrees produces unique keys in **ascending signed32 order**.
Node-address order and catalog order do not determine this sequence.

The native successor walk is present in the map's0x2fedd0 specialization.
With a real right child, successor is the leftmost real node in that right
subtree. With a nil right child, ascend parent links past ancestors reached
from their right side; the first ancestor reached from its left side is
successor. Passing the maximum reaches H and ends iteration. Native branch
tests use the nil byte at+0x19, and range-end comparisons use pointer identity
against the end node. No sentinel key/item is emitted. This describes the
successor facts, **not a read-only call to0x2fedd0**: that routine erases ranges.
Likewise0x2fe910 clears the map, and0x115750 can insert a missing key.

In a consistent quiescent map, the number of real entries is the wrapper's
unsigned64 count. That count does not replace sentinel/end recognition.
The comparator/layout establish a signed32 key, not a proof that every stored
key is currently in range for Zone+0x60. Object-index validity is separately
checked by interaction consumers; this ABI does not reinterpret the key as
an unsigned count or an item ordinal.

## 4. Lifetime, synchronization and interpretation limits

Real nodes and H are owned allocations. Full clear frees real nodes;
0x2f9940 also frees the head with its0xc0-byte size. Pointer validity therefore
depends on the selected Zone/map lifetime and absence of conflicting mutation
during observation. Lookup and successor code contain no capture snapshot
protocol, and this audit establishes **no particular World/Zone lock as
sufficient to freeze this map**. Count, color and nil flags are ordinary data,
not memory-validity certificates or an atomic snapshot/version marker.
Concurrent mutation, invalid pointers/cycles, partial construction and failure
recovery are not specified by the valid-layout facts above.

Known-empty/known-absent data is distinct from a source that supplies no map
data. Reward values are not encoded in the0xe0 object record; their current
reader behavior and the absence of a generation fallback remain
[live object metadata §3–4](steam-live-static-object-metadata.md#3-chest-rewards-are-a-separate-current-map).
This supplement defines no export-format policy or synthetic reward contents.
