# Steam sprite buffer resources: ordinary D3D11 backend

Independent native-only supplement to
[the primary sprite catalog contract](steam-native-sprite-catalog.md).
Only the original executable/native evidence and independent prose were read.
No SDK headers or implementation, engine, exporter, test, reconstruction, owner
data/capture or game execution was used. The scope is the reached ordinary
backend, not every possible resource implementation or a complete renderer.

## 1. Image and device/context provenance

Addresses are RVAs in the original analyzed unpacked AMD64 image
`cubeworld.original.exe.unpacked.exe`, preferred base **0x140000000**, SHA-256
`7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244`.
This identifies one analyzed image, not all distributions or modified builds.

Device setup **0x134cc0** invokes imported **D3D11CreateDevice at 0x134d91**.
The native argument preparation at **0x134d56–0x134d8f** supplies device output
storage at main-module **0x551a90** and immediate-context output storage at
**0x551a98**. Backend constructor **0xc28a0** retains the supplied device and
context pointers at **backend+0x08/+0x10**, respectively. Its table is at
**0x4383d0**.

The ordinary catalog retains this backend input at catalog+0x08 and passes it to
sprite constructor **0x16e6b0**, which retains it at **sprite+0x60**. Mesher
**0x170700** reaches the buffer factories through that backend. These native
creation and binding paths establish the D3D11 meanings below; they are not
names inferred from a game SDK class.

## 2. Minimal observed resource view

| Location | Established meaning on this backend |
| --- | --- |
| Sprite+0x70 | Eight-byte pointer to the native vertex-buffer wrapper |
| Sprite+0x78 | Eight-byte pointer to the native index-buffer wrapper |
| Either wrapper+0x00 | Native wrapper's table pointer |
| Either wrapper+0x08 | Eight-byte immediate-context pointer |
| Either wrapper+0x10 | Eight-byte **ID3D11Buffer interface pointer** |
| Index wrapper+0x18 | DWORD index-format value: **0x39/R16_UINT** or **0x2a/R32_UINT** |
| Sprite+0x80 | 32-bit vertex-count field written by meshing |
| Sprite+0x84 | 32-bit triangle-count field written by meshing |

The concrete wrapper tables are **0x438390** for vertices and **0x4383b0** for
indices. Observed allocation requests are **0x18 bytes** and **0x20 bytes**,
respectively. These facts supply neither complete class declarations nor an
allocation/lifetime guarantee for arbitrary objects that happen to have similar
fields.

The **data field wrapper+0x10** must be distinguished from **virtual slot+0x10**.
The field contains the buffer interface. The slot in both concrete tables points
to no-op **0xe8a20**; it is not a buffer getter. Likewise, the sprite's +0x70 and
+0x78 values are native wrapper pointers, not direct ID3D11Buffer pointers and
not CPU vertex/index byte addresses.

## 3. Factory and mesh-production facts

| Backend virtual offset | Reached factory | Result |
| --- | --- | --- |
| +0x08 | **0xc2c90** | Vertex-buffer wrapper |
| +0x10 | **0xc2b90** | 32-bit-index wrapper, format 0x2a |
| +0x18 | **0xc2ac0** | 16-bit-index wrapper, format 0x39 |

Each factory supplies the temporary source array to the device's **CreateBuffer**
entry at device virtual+0x18, then stores the resulting buffer interface in its
new wrapper's +0x10 field. The vertex factory's call is at **0xc2cf6** and its
buffer-field store at **0xc2d24**. The wrapper's context field is copied from the
backend's +0x10 value. No interface getter or extra pointer layer is involved in
this store.

The observed buffer descriptors use **Usage 0**, **BindFlags 1 for vertices or 2
for indices**, **CPUAccessFlags 0**, **MiscFlags 0** and **StructureByteStride 0**.
For ordinary nonoverflowing counts, vertex byte width is eight times the vertex
count; index byte width is the index-element count times two or four, according
to the selected format. The descriptor byte width is a DWORD; this is not an
unlimited-size buffer contract. The descriptor's
zero StructureByteStride is distinct from the **eight-byte IA vertex stride**
used when drawing.

Mesher **0x170700** selects 16-bit indices for **fewer than 65536 vertices**, and
32-bit indices otherwise. It writes sprite+0x80 from the count of eight-byte
vertex elements, and sprite+0x84 from the index-element count divided by three.
These count fields are established by reached meshing, not merely by sprite
construction or successful raw CUB allocation. The complete packed vertex format
and color conventions remain separate from this buffer-pointer contract.

## 4. Binding calls and the readiness qualification

The two concrete virtual+0x08 methods have observable binding side effects:

* Vertex method **0xc2a70**, with IA call at **0xc2aac**, supplies the address of
  wrapper+0x10 to **IASetVertexBuffers** at context virtual+0x90. It selects
  **slot 0, one buffer, stride 8 bytes, offset 0**, then returns byte 1.
* Index method **0xc2a40**, with IA call at **0xc2a59**, loads the interface from
  wrapper+0x10 and supplies it to **IASetIndexBuffer** at context virtual+0x98.
  It uses the format DWORD from wrapper+0x18 and **offset 0**, then returns byte 1.

Neither concrete method tests buffer creation status, a GPU fence or completion
state before returning the low-byte result 1. Thus the abstract resource-admission helper
**0x16eba0** is **not a pure readiness query on this backend**. It first requires
nonnull wrapper pointers, calls the vertex wrapper's virtual+0x08, and if that
returns nonzero calls the index wrapper's virtual+0x08. With these concrete
wrappers, both calls bind IA state and return byte 1 unconditionally. A nonzero helper
result consequently establishes that this call path ran, not independently
validated GPU readiness.

Ordinary opaque consumer **0xe6e60** invokes 0x16eba0 once per admitted resource
group before its descriptor loop. Its later **0x16ecc0** calls check the two
wrapper pointers but do not rebind them; they reach backend draw+0x20. Consumer
**0x16ec60** instead repeats the two virtual+0x08 calls before its draw. These
distinctions complement the retained-state/resource-order contract in
[queued WORLD state](steam-queued-world-state-order.md).

Backend draw **0xc2d80** reaches **DrawIndexed at context virtual+0x60**, using
three indices per submitted triangle, start index 0 and base vertex 0. The supplied
vertex-count argument is not used by this concrete draw routine. No separate
buffer getter or CPU-data accessor is reached here.

## 5. Replacement, empty output and lifetime

Sprite constructor 0x16e6b0 initializes both wrapper pointers to null. On a reached
nonempty mesh-output branch, 0x170700 creates the new vertex and index wrappers,
retains the previous pointers, publishes the new index/vertex pointers at
+0x78/+0x70, and invokes cleanup on the previous nonnull wrappers. The temporary
CPU vertex/index arrays are subsequently released. They are not persistent
payload fields in the wrappers.

When generated geometry is empty, meshing can write zero counts while skipping
buffer creation/replacement. **Previous wrapper pointers can remain present.**
Thus nonnull wrappers do not guarantee nonzero current geometry, and their buffer
contents need not describe the current raw RGB allocation during intermediate
reload/remesh phases. Zero-count draws and lack of visible pixels likewise do not
undo earlier IA-binding calls.

Vertex cleanup **0xc2950** and index cleanup **0xc28f0** invoke the stored buffer
interface's **Release entry at virtual+0x10** and conditionally release the native
wrapper allocation. That COM Release operation is different from the wrapper's
own no-op virtual+0x10. It does not prove that the buffer's final external
reference disappeared. Sprite cleanup **0x16e950** reaches wrapper cleanup;
catalog ownership and raw RGB replacement remain bounded separately in
[the catalog lifetime contract §5](steam-native-sprite-catalog.md#5-lifetime-readiness-and-separate-resource-families).

The context field is copied into the wrapper on these factory paths. No context
AddRef or ownership transfer is established by those assignments. Neither a raw
wrapper pointer nor its stored buffer pointer gives a caller a lifetime extension.

## 6. Failure, concurrency and readback boundary

The factories do **not test CreateBuffer's HRESULT** before constructing and
publishing their wrappers. This note supplies no safe creation-failure, null-buffer
or device-recovery protocol. The concrete bind methods' unconditional byte result 1
must not be substituted for such validation.

No coherent concurrent snapshot of counts, both wrapper fields, COM interfaces
and raw RGB is established by these simple access paths. The replacement writes
and the earlier count updates are not specified here as one atomic publication.
Broader meshing locks, resource mutation, alternate backends and external lifetime
management remain outside the audited scope.

The reached consumers establish where the **ID3D11Buffer interfaces** reside and
how native drawing uses them. They do **not** establish a CPU readback, Map,
staging-copy, GetDesc-call ABI, complete COM table or captured mesh-byte schema.
These are bounded native data-flow facts, not a runtime oracle, SDK replacement
or claim that an independent capture adapter is complete.
