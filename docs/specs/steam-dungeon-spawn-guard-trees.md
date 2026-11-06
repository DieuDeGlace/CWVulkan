# Steam dungeon spawn guard and activation trees

This contract covers the unpacked original Steam AMD64 image `cubeworld.original.exe.unpacked.exe`, preferred base `0x140000000`, SHA-256 `7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244`. Addresses are image RVAs. Spawn denotes the heap record constructed by `0x118a60` and retained by `0x119530`. The kind-8 producer at `0x2fa7b0` allocates `0x950` bytes for this record. The guard and activation headers belong to that record.

## Live layout and iteration

Both headers are 16 bytes. `0x118a60` initializes the count and obtains a separate head node from `0x86ea0` for each header. The intervening fields at Spawn `+0x910` through `+0x927` belong to other state.

| Spawn offset | Width | Meaning |
| --- | --- | --- |
| `+0x900` | 8 bytes | Guard tree head pointer. |
| `+0x908` | 8 bytes | Guard tree element count. |
| `+0x928` | 8 bytes | Activation tree head pointer. |
| `+0x930` | 8 bytes | Activation tree element count. |

| Node offset | Width | Meaning |
| --- | --- | --- |
| `+0x00` | 8 bytes | Left child, or minimum node on the head. |
| `+0x08` | 8 bytes | Parent, or root node on the head. The root's parent is the head. |
| `+0x10` | 8 bytes | Right child, or maximum node on the head. |
| `+0x18` | 1 byte | Balancing color. |
| `+0x19` | 1 byte | Head or nil marker. The head carries 1 and an ordinary node carries 0. Missing children refer to the head. |
| `+0x1c` | 4 bytes | Signed X of the coordinate key. Only valid on an ordinary node. |
| `+0x20` | 4 bytes | Signed Y of the coordinate key. Only valid on an ordinary node. |

Both heads are allocated as `0x28` bytes. At successful construction, each has a zero count, its three links point to itself, and bytes `+0x18` and `+0x19` are both 1. Neither head has a constructed coordinate key. A constructed empty tree therefore has a live, nonnull head. The allocator at `0x87980` and ordinary-node initializer at `0x6ed60` establish the same `0x28` allocation and the eight-byte key copy for guard insertion. No additional payload at `+0x24` is established for either tree. The count found at `+0x24` on nodes in a separate Zone aggregation map belongs to that map.

The guard walk at `0x2e44ea–0x2e4571` and the activation walk at `0x2e209c–0x2e22d1` both start at the minimum link on the head. Their successor walk uses the leftmost ordinary node of a right subtree when present. Otherwise it climbs parent links until reaching the first ancestor from a left child, or the head. Reaching the head ends iteration. It does not yield a coordinate. Both native consumers check the corresponding 64-bit count before the walk. A count and a head pointer belong to the same live header and do not independently validate the links.

The guard insertion at `0x745e0` compares signed X first and signed Y second, retaining one member per distinct pair. Its in-order iteration is ascending by that signed lexicographic order. The activation walk uses the same in-order link semantics and reads the same signed X/Y positions in 16-block cells. This audit establishes its head, node and key representation from construction, destruction and consumption. It does not establish a writer to a populated activation tree on the kind-8 paths.

## Kind-8 membership at generation

The kind check at `0x3182f7` leads to `0x2fa7b0` at `0x318326` with the Zone, the current 16-block cell X and Y, an additional generator context pointer, the selected floor code F as the fifth argument, and the address of the selected coordinate pair D as the sixth. All successful spawn construction paths below start with both trees initialized empty by `0x118a60`.

| Producer in `0x2fa7b0` | Guard membership on the fresh spawn | Publication point |
| --- | --- | --- |
| Gate-side branch whose neighbor test matches cell ID `0x2e` at `0x2faa12` | Each emitted spawn gets the one matched neighbor pair in its guard tree at `0x2fab10`. The pair comes from the cardinal neighbor test, with X stored before Y. It is independent of D. | `0x2facd3` |
| Central-cell branch | No guard pair is inserted on the inspected branch. | `0x2fafeb` |
| Floor guard, F equal to 4 | One pair copied by value from D at `0x2fb16d–0x2fb194`. | `0x2fb3f7` |
| Floor guard, F equal to 3 | The branch at `0x2fb19e` bypasses both pair insertions. The spawn retains the empty guard tree. | `0x2fb3f7` |
| Floor guard, default F branch, including F equal to 2 | One pair copied by value from the address supplied for D at `0x2fb121–0x2fb148`. When F is 2, the context scan has initialized D. | `0x2fb3f7` |
| Additional later spawn loop | No guard pair is inserted on the inspected branch. | `0x2fb77e` |

The floor guard reaches allocation only after the cell query returns `0x0d` at `0x2fb041` and the test through `0x1299a0` at `0x2fb058` returns nonzero. The F dispatch at `0x2fb0d7–0x2fb0e9` tests 3 and 4. It supplies no independent nonzero test before the default branch's two loads from D. Whether earlier gates make that load reachable when F is zero remains unproved. The original context scan gives no coordinate value to report for F equal to zero.

None of these producer paths has an established insertion into Spawn `+0x928`. Their construction of a separate activation head and zero count is established even for the branches that populate the guard tree. Guard membership is a pair of coordinates in a distinct tree. It does not imply corresponding activation membership.

## Runtime consumers and lifetime

`0x2e3510` reads the guard tree after finding the spawn's ID in the live World creature tree. In the inspected path it rebuilds a per-Zone count map at `Zone+0x4d8`, copying each guard coordinate into that map's key and adding one to its count at `0x2e450e–0x2e451a`. Later in the same update, the type-8 and type-10 cell-object branch sums matching guard counts from nearby Zone records or the corresponding retained Zone state. The comparison at `0x2e54a3` can reach the state-changing call `0x2d3a10` for the cell object. This is a verified live gate-state use of the guard area. The observed reader does not pass its coordinates to creature steering or pathfinding. Other possible movement paths remain outside the audit.

`0x2e17d0` checks the activation count at `0x2e2089`. For a member it reads signed X and Y from `+0x1c` and `+0x20`, converts each 16-block cell coordinate to its containing 64-block Zone coordinate, and looks up the Zone at `0x2e212e`. The subsequent branch checks the cell-object state at the matching coordinate. A missing Zone can end that path early, so runtime consumption need not visit the full tree. This is a read of a spawn activation condition, separate from guard count aggregation.

`0x119530` takes the pointer into the Zone's spawn tree at `Zone+0x460`, where the outer tree node stores that spawn pointer at `+0x28`. Replacing an occupied entry calls `0x118d30` and frees the old spawn before storing the replacement. `0x118d30` clears both trees through `0x53d90` and frees the separate heads. Clearing can free ordinary nodes and resets all three head links to the head and the count to zero, as shown by `0x50ae0`. The pointer and link layout applies during the live object's ownership interval. The values immediately before the traced `0x119530` calls are on the producing invocation after membership setup, before retention or replacement at that call. The audit does not establish a lock or lifetime guarantee for an asynchronous reader walking either tree during mutation, replacement or Zone teardown.

## Kind-8 floor selection observation

In initial Zone generation `0x2ff6f0`, F is a 32-bit stack value at the native frame's RBP plus `0x200`. It starts at zero at `0x2ff905`. The kind test at `0x2ff90b` gates the selection scan to kind 8. The scan traverses its rectangular range with ascending X outside and ascending Y inside. It tests the result of `0x69c60` through `0x25eb30`, then reads the floor code through `0x2a1d10` with fourth argument zero. Only codes 2, 3 and 4 qualify. Each qualifying cell replaces both F at `0x2ff9f2` and D at `0x2ffa0b`. D contains signed 32-bit X at RBP plus `0x1b40` followed by signed 32-bit Y at RBP plus `0x1b44`. They are world cell indices in 16-block units. The last qualifying cell wins.

RVA `0x2ffa25` is a passive observation point for those values on the producing invocation. It is the common continuation after the scan finishes or its range is empty, before the kind-8 guard call at `0x318326`. The same point is reached when the scan is skipped for other kinds. The 32-bit kind at RBP plus `0x1568` distinguishes the kind-8 path there. At this point F can always be read on the kind-8 path. D is valid for F equal to 2, 3 or 4. If F is zero, the scan performed no qualifying write to D. Bytes in that stack slot have no established coordinate meaning in that case. The stack offsets are specific to this native frame and have no lifetime beyond it.
