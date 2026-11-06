# Steam dungeon dome observation sites

This note covers the unpacked original Steam AMD64 image `cubeworld.original.exe.unpacked.exe`, preferred base `0x140000000`, SHA-256 `7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244`. Addresses are image RVAs.

The sites `0x30aca8`, `0x30afa0` and `0x30b009` are all inside the same native `0x2ff6f0` generator frame. RBP is the frame base established at `0x2ff6fa`. The following slots are readable and have the same meanings at all three sites when reached in the original generator.

| Native frame slot | Width | Value |
| --- | --- | --- |
| RBP minus `0x80` | 8 bytes | Current generating Zone pointer, saved from entry RCX at `0x2ff79e`. |
| RBP minus `0x78` | 4 bytes | Signed world cell X, in 16-block units, stored at `0x3025e7`. |
| RBP minus `0x68` | 4 bytes | Signed world cell Y, in 16-block units, stored at `0x302612`. |
| RBP minus `0x5c` | 4 bytes | Signed current structure layer index. The layer loop stores its initial index at `0x307b50` and advances it at `0x312dc3–0x312dc6`. |

The X/Y slots describe the outer structure cell and can refer to the generator's halo. The finer sample coordinates used within that cell are separate. The signed layer index is loop state, separate from the sampled or selected Z height. The generator retains these slots across the three sites. It has not reached the next layer or cell update at those points.

The Zone pointer leads to signed 32-bit X at `Zone+0x10` and Y at `Zone+0x14`. The constructor stores the incoming pair at `0x2f942e–0x2f9438`. These indices use 64-block Zone units, as shown by the shifts at `0x3025c4` and `0x3025ef` before the conversion to the outer cell coordinates. They identify the generating Zone even when its current cell lies in the halo.

`0x30aca8` tests the frame byte at RBP plus `0x28` before the dome calculation. When that byte is zero, the branch at `0x30acac` goes directly to `0x30b009` and does not pass `0x30afa0`. On the calculation path, `0x30afa0` begins the final weighting arithmetic after selection of the input in XMM2. The calculation stores its resulting dword at frame RBP plus `0xd4` at `0x30b003`. `0x30b009` is the common continuation, and `0x30b00c` reads that height slot. Reaching the common continuation alone does not show that the dome calculation ran for that sample.

These observations apply at the named instruction boundaries during the live native frame. They do not establish the contents of unrelated scratch slots on bypass paths, lifetime of a saved pointer after generation, or an ABI for entry into the middle of the generator.

## Continuation fingerprint

At `0x30b009`, the first whole instruction is three bytes, `41 8B DC`. The next instruction starts at `0x30b00c` and is six bytes, `8B 85 D4 00 00 00`. The combined nine-byte fingerprint was read directly from the original image. The following instruction starts at `0x30b012`.
