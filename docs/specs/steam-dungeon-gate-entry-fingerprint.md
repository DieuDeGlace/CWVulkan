# Steam dungeon gate state entry fingerprint

This addendum covers the unpacked original Steam AMD64 image `cubeworld.original.exe.unpacked.exe`, preferred base `0x140000000`, SHA-256 `7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244`. Addresses are image RVAs.

At entry `0x2d3a10`, the shortest span of at least twelve bytes ending at an instruction boundary is thirteen bytes. Its fingerprint, read directly from the image, is `40 53 48 83 EC 60 48 8B 05 73 52 27 00`. The starts of the three instructions are `0x2d3a10`, `0x2d3a12` and `0x2d3a16`. The next instruction starts at `0x2d3a1d`.

The first two instructions occupy six bytes and have no relative operands. The seven-byte instruction at `0x2d3a16` reads a qword through RIP-relative addressing. Its original target is RVA `0x548c90`. Every whole-instruction span of at least twelve bytes starting at the specified entry therefore contains a RIP-relative instruction. Exactly twelve bytes would end inside that instruction.

This result is limited to the entry and span criterion above. It does not establish a relocation or overwrite contract elsewhere in the image.

## Five-byte branch footprint

The shortest relocation-free whole-instruction span at `0x2d3a10` that can contain a five-byte relative jump is six bytes. Its image fingerprint is `40 53 48 83 EC 60`, and the first instruction beyond the span is at `0x2d3a16`. The first five bytes alone would end inside the second instruction. The scoped native assembly lists no direct branch to the interior of the six-byte span.

The jump destination must be within signed 32-bit displacement range of the address following the five-byte branch. The span establishes instruction boundaries and relative-addressing properties only. Runtime patch safety, unwind behavior and reach to any particular destination remain outside this audit.
