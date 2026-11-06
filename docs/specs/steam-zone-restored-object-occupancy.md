# Steam Zone restoration and marked block occupancy

This note covers the original unpacked Steam AMD64 image `cubeworld.original.exe.unpacked.exe`, preferred base `0x140000000`, SHA-256 `7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244`. Code addresses are RVAs.

## Creation and saved overlays

`0x2ae0c0` returns a matching non-null Zone already in the World's coordinate map when present. On the observed new-Zone path it constructs the Zone with `0x2f9400` and calls `0x2ff6f0` at `0x2ae179`. The generator's ordinary completion calls `0x31d470` and then `0x31d700` at `0x318f63` and `0x318f6c`. These calls finish before the saved overlay.

The overlay requires a zero byte at `World+0x358` and a saved entry for the exact Zone coordinates in the World tree whose head is at `World+0x10`. If the saved entry's byte at `+0x28` has bit `0x02`, restoration sets `Zone+0xfa` to one and assigns the saved static-object vector at entry `+0x48` to the vector at `Zone+0x60`. The copy is at `0x2ae293`. A separate flag controls assignment of the vector at `Zone+0x30`.

After the vector assignments, the routine copies the saved block edit list to `Zone+0xd8` and replays the saved list in order. Each call at `0x2ae2d8` passes signed 32-bit world block X/Y/Z from list node offsets `+0x10`, `+0x14` and `+0x18` and a pointer to the six block bytes at node `+0x22`. The writer `0x31c1c0` applies its normal Zone bounds and block admission rules. These are explicit block updates. Their block data can affect the occupancy flag independently of the static-object copy.

The restore path has no second call to `0x31d700` or demonstrated enqueue of a footprint rebuild. The static-object assignment and block replay leave the generated marked-coordinate set in place. After the other saved fields are copied, the routine publishes the Zone pointer in the World coordinate map at `0x2ae354`. A saved object state and the footprint generated before overlay can therefore differ at publication. A replayed block can also make the actual occupancy differ from the marked-coordinate set.

## Marked-coordinate set

The tree is embedded at `Zone+0xe8`. `Zone+0xe8` is an eight-byte pointer to its allocated head and `Zone+0xf0` is the eight-byte element count, excluding the head. Construction at `0x2f94e1–0x2f94fc` installs a head from `0x009f20` and leaves the count zero. Both head and ordinary nodes occupy `0x28` bytes.

| Offset from head or node | Width | Ordinary node | Head |
| --- | --- | --- | --- |
| `+0x00` | 8 bytes | Left child | First element |
| `+0x08` | 8 bytes | Parent | Root |
| `+0x10` | 8 bytes | Right child | Last element |
| `+0x18` | 1 byte | Balance color, zero red and one black | One |
| `+0x19` | 1 byte | Nil marker, zero | One |
| `+0x1c` | 4 bytes | Signed world block X | No key |
| `+0x20` | 4 bytes | Signed world block Y | No key |
| `+0x24` | 4 bytes | Signed world block Z | No key |

An empty constructed tree has all three head links pointing to the head itself. Missing children in an ordinary node point to the head. The root's parent is the head. Ordinary nodes are constructed by `0x112f30` with the three coordinate dwords as their sole payload. The key order is lexicographic ascending, with signed X primary, then signed Y, then signed Z. `0x1152c0` compares in this order and suppresses duplicate triples. `0x113a10` links a new node and increases the count. `0x31d700` walks from head's first-element link in successor order and stops on the head. Zone cleanup `0x2f9940` frees the nodes and the head.

## What the set records

`0x31d700` first walks the existing set. It reads the block at each key through `0x00eb90` and, when the sixth block byte has bit `0x40` set, submits the block with that bit cleared to `0x31c1c0`. The pass does not remove the keys during this walk. It then scans the static objects in the current vector at `Zone+0x60/+0x68`. For object types `0x08` and `0x0a`, the byte at object `+0x28` must be nonzero for this footprint pass to visit the object's block range. The state gate is at `0x31d86c–0x31d87a`.

For each visited block whose readback lacks bit `0x40`, the routine submits a block write with the bit set at `0x31dae6` and inserts the world X/Y/Z triple at `0x31db0e`. The two phases retain existing keys and add new ones. The set describes positions tracked for occupancy writes across rebuilds. Its count and membership do not establish current occupancy. The block writer has its own admission rules, and the pass inserts after the write call without testing its result.

## Later rebuilds and lifetime

The runtime gate path in `0x2e3510` compares the old state byte against the state after calling `0x2d3a10`. On a change it inserts the owning Zone pointer into the World set at `+0x1a0`, at `0x2e54e4` or `0x2e5765`. `0x2e30b0` walks that pending set and calls `0x31d700` at `0x2e33e6` before draining it. A separate received-object path in the ordinary frame replaces the vector of a found Zone, inserts that Zone in the pending set at `0x0a83b1`, and calls `0x31d470` and `0x31d700` synchronously at `0x0a83bd` and `0x0a83c9`. These are established rebuild paths separate from the saved overlay.

Within `0x2ae0c0`, the World critical sections at `+0x3f0` and `+0x3c8` are entered at `0x2ae321` and `0x2ae32e`, after generation and restoration. They cover the map insertion and pointer assignment in that routine. `0x31d700` has no direct acquisition of either section. The received-object rebuild is reached from the ordinary frame without a local acquisition around the update. The hosted update at `0x0f6947` reaches the pending-set rebuild with a different critical section held, entered at `0x0f5582` on host object `+0x5f8`, relative to a World at host object `+0x08`. These observations do not establish that holding the World Zone and mesh sections alone excludes all writes to the marked-coordinate tree. A live lease must independently establish mutation exclusion as well as object lifetime before the traversal can be called safe.

For an exclusively owned, live fresh Zone, the tree has its constructed layout before generation and is structurally readable after the synchronous generation returns, provided no writer or cleanup can run during the traversal. It is mutated during generation. The ordinary new-Zone path has not published the pointer in its World map at the generator's return. That map timing does not prove absence of transitive escape from generation or guarantee exclusive ownership of an arbitrary detached call. No general worker, indirect writer, or teardown exclusion is established here.
