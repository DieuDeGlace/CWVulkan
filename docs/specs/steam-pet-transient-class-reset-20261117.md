# Steam equipped-pet transient class reset

This is a static contract for the existing equipped-pet route in original Steam image `cubeworld.original.exe.unpacked.exe`, image base `0x140000000`, SHA-256 `7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244`. The addresses below are RVAs. Actor offsets use the full pet pointer passed by the caller.

On the existing-pet update branch of `0x2E0DB0`, the reset at callsite `0x2E0FAB` receives the full pet pointer. It runs when the owner's 32-bit equipped value at `+0x8BC` differs from its 32-bit cached value at `+0x154C` (comparison at `0x2E0FA0`). On the reached reset path, `0x064A90` calls `0x064C00` at `0x064AA6` with the pet pointer advanced by `0x10`.

**Both full pet bytes `+0x14C` and `+0x14D` are zeroed by a single 16-bit store at `0x064D25`.** Its effective address is the nested base plus `0x13C`, which is the full pet plus `0x14C`. The stored word is `0x0000`. This store runs on every normal execution of that reset. Its value does not depend on the incoming class or specialization bytes.

The preceding 64-bit store of value 1 at `0x064D1A` starts at nested `+0x134` and spans full pet `+0x144` through `+0x14B` inclusive. The 64-bit zero store at `0x064CD5` uses nested `+0x14C` and spans full pet `+0x15C` through `+0x163`. The zero-fill call at `0x064C65` covers full pet `+0x278` through `+0x957` inclusive. The appearance copy at `0x064D38` writes within full pet `+0x78` through `+0x127` inclusive. These ranges exclude the two class bytes. The rest of `0x064A90` resets fields and containers at higher actor offsets.

On the same changed-equipment route, the caller writes the pet model at `0x2E0FB6`, conditionally updates name bytes starting at full pet `+0x958`, writes the owner ID at `0x2E1032`, then passes the pet's appearance area at `+0x78` to `0x07E780` at `0x2E1058`. That copy has a highest touched full-pet offset of `+0x127`. The two class bytes therefore both enter setup `0x2B67B0` at callsite `0x2E1067` as zero on the scoped reset path. This contract ends at that call's entry.
