# Steam dungeon hanging cone

This contract covers the unpacked original Steam AMD64 image `cubeworld.original.exe.unpacked.exe`, preferred base `0x140000000`, SHA-256 `7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244`. Addresses below are image RVAs. It covers the cone at `0xc0cd0` and its direct detail-12 call at `0x3127f8` in Zone generation `0x2ff6f0`.

## Detail-12 call

The detail dispatch at `0x31270b–0x312735` tests the 32-bit detail value. The fallthrough at `0x31273b` is the value 12. It is a separate test from the palette selector examined earlier at `0x31255b–0x31261f`. The call follows the structure work through `0x69860` at `0x3126a8` and `0x2fc270` at `0x312706`. It uses the same structure write context configured for colors at `0x3127a0`.

The native Windows x64 call passes the context pointer in RCX, the signed 32-bit local center X and Y in EDX and R8D, and the signed 32-bit local center Z in R9D. Nominal radius and height are signed 32-bit stack arguments, fifth and sixth respectively. They are at outgoing stack offsets `+0x20` and `+0x28` before the call. The caller uses the routine for its writes and takes no result.

Both horizontal center arguments are 8. Center Z is the upper structure height read at `0x31270b` minus 2, formed at `0x31270e`. The same upper height was used to form the upper endpoint passed to the preceding structure work and is installed as the high color threshold at `0x3127a0`. It is distinct from the lower structure height read at `0x312685`.

After the color setup, the first draw at `0x3127a5` uses the CRT `rand` import at IAT RVA `0x42cd28`. Its signed remainder modulo 5 plus 8 supplies height, ordinarily 8 through 12. The second draw at `0x3127c5` uses the same import. Its signed remainder modulo 4 plus 3 supplies nominal radius, ordinarily 3 through 6. The draw order is height then radius even though radius is the first stack argument. There is no reseed between these draws and the call. The cone itself makes no direct `rand` call. The caller jumps from `0x3127fd` to the common continuation at `0x312da7`.

## Coordinate spaces and iteration

The write context begins with a pointer to the destination Zone. The placement setup at `0x3063ff–0x306445` builds this context through `0xba8c0` and `0xbfa30` with unrotated axes, zero vertical translation and horizontal translations formed from the structure-grid coordinates in 16-unit steps. The following describes the actual coordinate reads in `0xc0cd0`, including how it uses the context transform.

- The first horizontal world coordinate, X, comes from signed 32-bit local X and Y with their coefficients at context `+0x7c` and `+0x8c` and translation at `+0xac`.
- The second horizontal world coordinate, Y, comes from signed 32-bit local X and Y with coefficients at `+0x80` and `+0x90` and translation at `+0xb0`.
- The projection at `0xc0e70–0xc0ed8` does its products and sums in 32 bits. It omits any local-Z contribution. Each projected coordinate must lie at or above its Zone origin and strictly below that origin plus 64. The origins are the signed 32-bit Zone fields at `+0x10` for X and `+0x14` for Y, each multiplied by 64. The projected coordinates used by the noise are in world space, with no subtraction of the Zone origin.

Local Z increases from center Z minus height through center Z plus 1, inclusive. The signed exclusive end is center Z plus 2. Thus at this call the last layer is one unit below the caller's upper structure height. The first layer has index zero. On each successive layer the index increments by one. The layer radius is formed by converting the index, height and nominal radius to binary32, dividing index by height, then multiplying by nominal radius in a separate binary32 operation. The layer at center Z uses the nominal radius. The additional last layer uses index height plus 1 without a radius cap.

For each layer, the integer lower X and Y bounds come from their binary32 center, subtracting the layer radius and then 1.5, and truncating toward zero to signed 32 bits. The upper X bound adds radius to center X and then adds 1.5. The upper Y bound adds 1.5 to radius before adding center Y. Both upper bounds stay in binary32. Each signed integer counter is converted to binary32 for an inclusive comparison with its upper bound. Native order is ascending local Z, then ascending local X, then ascending local Y as the innermost counter. The bound formation and loop tests are at `0xc0de0–0xc0e67` and `0xc1049–0xc109e`.

## Simplex offsets and inclusion

The two calls to the original two-input simplex sampler `0x2e0c0` cross the horizontal input axis and the local distance axis. Both are after the horizontal Zone test. The scale for the sample inputs is the binary64 value at `0x432d78`.

| Call site | First sampler input | Second sampler input | Local distance component receiving the sample |
| --- | --- | --- | --- |
| `0xc0f16` | Signed **world Y**, converted to binary64 and multiplied by 0.1 | Signed **local layer Z**, converted to binary64 and multiplied by 0.1 | Local X minus center X |
| `0xc0f5e` | Signed **world X**, converted to binary64 and multiplied by 0.1 | The already scaled **local layer Z** used by the first sample, with binary64 **843.0 added afterward** | Local Y minus center Y |

Both horizontal sample inputs preserve the sign of the projected coordinate. The Z input preserves the sign of the local layer coordinate and uses no center subtraction or world-Z translation. Each displacement starts with the sampler's binary32 result, multiplies it by positive binary32 1.5, and adds the signed local difference converted to binary32. The signed difference is formed in integer arithmetic before that conversion. Neither sample is negated. This produces two local distance components even though the horizontal sample coordinates are in world space.

The sampler computes in binary64 and returns binary32 at `0x2e349`. Inside it the inputs are independently scaled again by its binary64 constant at `0x42e8e0`. Its final binary64 scale before conversion is at `0x42e8f8`. The cone separately squares the two binary32 distance components, adds the squares in binary32, and compares the sum with a separately squared binary32 layer radius. There is no square root or fused operation. The branch at `0xc0f88–0xc0f8b` skips a strictly greater sum, so the boundary is included for finite values. The supported detail-12 call uses positive height. Behavior for degenerate or out-of-range arbitrary arguments is outside this contract.

## Color and write order

For an accepted candidate the cone queries `0xbb9e0` at `0xc0fa4` with the same local X, Y and Z. The detail-12 setup at `0x3127a0` installs the already prepared low, middle and high color triplets, sets all three sample modes to zero and clears the relative-height byte at context `+0x50`. The three thresholds are the lower height minus its saved reference height, the lower height itself and the upper height, each converted to binary32. In mode zero, `0xc1140` copies the input triplet. The caller's prepared palette values remain inputs to this contract. In particular the palette override selected by the separate value 12 at `0x3125e3` is the triplet 50, 150, 20 and applies only when that selector has value 12.

The cone multiplies each of the three returned channels by binary32 0.5, clamps finite results to 0 through 255 and truncates each to a byte. Its payload has six bytes in order. The first three are those channels and the last three are `0x00`, `0x06` and `0x22`. At `0xc1044` it immediately passes the six bytes and the candidate's local coordinates to `0xbf600`. Thus each candidate is tested, colored and submitted in the scan order above, before advancing the innermost counter.

`0xbf600` transforms the write coordinates using the full context basis and translation and calls the Zone cell setter `0x31c1c0` at `0xbf767` after any enabled context filters. Context `+0x74`, when positive, admits a terrain-height gate. The original structure setup can set this field to 10 at `0x306471`. Context `+0x70` can enable tests of the existing cell's bytes. The cone has no independent overwrite guard and its inclusion test alone does not prove a committed write under arbitrary context flags. The setter rechecks horizontal ownership and can extend or fill a column while setting a cell. No cone submission occurs for a candidate outside the scan, outside the horizontal Zone test or rejected by the squared-distance test. This does not establish that later structure work leaves those cells alone or that the column setter has no storage effects beyond the submitted coordinates.

## Image constants

| Data RVA | Native value and encoding | Use in this scope |
| --- | --- | --- |
| `0x42e548` | Binary32 1.5, `0x3fc00000` | Bounds padding and positive displacement multiplier. |
| `0x432d78` | Binary64 0.1, `0x3fb999999999999a` | Scale of world X, world Y and local Z before simplex. |
| `0x432d80` | Binary64 843.0, `0x408a580000000000` | Added to the scaled local Z for the second simplex call. |
| `0x42e8e0` | Binary64 0.32, `0x3fd47ae147ae147b` | Additional input scale within `0x2e0c0`. |
| `0x42e8f8` | Binary64 70.0, `0x4051800000000000` | Final simplex scale before its binary32 return. |
| `0x4ac964` | Binary32 0.5, `0x3f000000` | Cone color multiplier. |
| `0x42e7d8` | Binary32 255.0, `0x437f0000` | Cone color upper clamp. |
| `0x42e5b0`, `0x42e938`, `0x42e5a0` | Binary32 50, 150, 20 respectively | Separate palette selector's value-12 override. |

The image call table identifies `0x3127f8` as the direct call to `0xc0cd0`. The audit does not establish an indirect caller census, the full simplex initialization and table lifecycle, a complete upstream palette census, or protection across all Zone generation and write paths.
