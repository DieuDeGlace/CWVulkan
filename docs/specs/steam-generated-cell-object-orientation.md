# Generated cell-object records 0x3f and 0x41

This note covers the unpacked original Steam AMD64 image `cubeworld.original.exe.unpacked.exe`, preferred base `0x140000000`, SHA-256 `7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244`. Code addresses are image RVAs. Field offsets are relative to the beginning of the named record or owner. The findings come from the original image and its native assembly, function and call records.

## Established emission

Initial Zone generation `0x2ff6f0` has a shared emission branch at `0x316020–0x316260`. It tests a subset of a Zone-local 16-unit candidate grid. Candidate world coordinates at `0x31603d–0x316056` use the Zone's two coordinates at `+0x10` and `+0x14`, with a 64-unit Zone span and an 8-unit offset within the grid step. The branch tests a scalar sample, requires the cell selector `0x69c60` to return zero, and tests a second scalar against lower and upper bounds. These are admission tests before the object record is prepared.

The first type sample, at `0x316124` through `0x2d9f30`, can select `0x42`. The second, at `0x31614a` through `0x2d9b60`, selects `0x41` when its result exceeds the threshold at `0x42e714` on the remaining path. The original image value there is the single-precision encoding of 0.85. The final selection at `0x316161–0x316175` uses the low bit of the result from `0x2ff5c0` on a coordinate pair. A clear bit yields `0x3f`, and a set bit yields `0x40`. The bit controls the ID only. The type writes for `0x41` and `0x3f` are at `0x316155` and `0x316175` respectively.

The four branches converge on the same position and placement path. The horizontal coordinates use the candidate X and Y with the shared fractional offset loaded at `0x315f5f–0x315f79`, whose original image value is 0.5. The three position components are signed 64-bit fixed-point values with 16 fractional bits. The height sample at `0x316226` plus the height offset supplies the initial Z, converted by `0x29be70`. The placement call to `0x31b2d0` at `0x31624c` checks the Zone and the ground, can adjust Z and snaps accepted Z to a whole world unit. It leaves X and Y as supplied. The successful path appends the record to the live vector at `Zone+0x60` through `0x129b50` at `0x316260`. That vector uses a stride of `0xe0`.

There is also a coordinate-dependent ID selector at `0x2ba990` that can return `0x41` for input class 10 and `0x3f` for input class 13. The call from `0x2d53d0` at `0x2d5791` reaches it. This establishes another source of the numeric IDs. It does not by itself establish another emission into the live vector or a source of its turn byte. The identified emission branch is the one traced through construction and storage here.

## Writes and retained turn

The emission uses a stack temporary and calls the partial constructor `0x2f93a0` at `0x3160fa`. These are the writes established for both IDs on that branch.

| Record offset | Established write |
| --- | --- |
| `+0x00` | 32-bit selected ID. |
| `+0x08`, `+0x10`, `+0x18` | Three 64-bit position components. The placement helper can replace the third component before append. |
| `+0x20` | No initialization by the constructor or the traced emitter. Placement reads its low bit and append copies the existing byte. |
| `+0x24` | 32-bit float 1.0. `0x13a960` uses it to scale the dimensions obtained from the ID. |
| `+0x28` | Byte 1. |
| `+0x2c` | 32-bit float positive zero. |
| `+0x30` | 64-bit zero. |
| `+0x38` | Embedded item value with zero in its defined fields. The initialized spans are `+0x38`, `+0x3c–0x4b`, `+0x4c`, `+0x50–0x53`, `+0x54`, `+0x55–0xd4` and `+0xd5`. |
| `+0xd8` | Byte 1. |

The stack location of the turn byte is 32 bytes into the same temporary passed to the constructor, placement and append. The position copies at `0x3161ef` and `0x31623a` end before it. Placement reads it at `0x31b37d` and writes the Z field only. The append copy at `0x9860` preserves the full byte. Thus its immediate source is the storage already present at that point in the generator's frame. The branch does not supply a zero, a coordinate-derived turn or a random turn draw. The preceding writer of that storage and any repeatability of its value across invocations remain unproved. The byte is a defined field of the stored record even though this branch leaves its input value unwritten.

## Later copies and consumers

At `0x318edb` initial generation copies the live vector into the separate baseline vector at `Zone+0x78`. Refresh `0x31c540` uses the opposite direction at `0x31c629`. The vector assignment helper `0x29baf0` and its range-copy paths `0x296190` and `0x299750` copy `+0x20` with the other fields. This restores the turn from the retained baseline. `0x31d470` also includes both IDs in the selection copied from the live vector into the Zone's separate vector at `+0xc0`. It copies their bytes through the ordinary appender. These copies can write a destination turn without computing a new one.

There is another replacement path through the World record keyed by Zone coordinates. `0x2d3e20` saves the live vector when the Zone's byte at `+0xfa` is set. On the reached restore branch, `0x2ae0c0` applies that stored vector after initial generation and before publishing the Zone, when the record has its vector-present bit set. Both use `0x29baf0`. A restored live record can therefore have the turn of the retained World value. The inspected refresh and copy paths establish no standalone turn recomputation for live `0x3f` or `0x41` records. Other sources of saved values and other possible direct writers are outside the established scope.

Placement `0x31b2d0` and the spatial rebuild `0x31d700` both consume bit zero of the turn to exchange the horizontal dimensions. Both IDs enter the relevant spatial rebuild branch. Their unscaled dimensions from `0x13a520` are 2 by 2 by 8 for `0x3f` and 1 by 1 by 4 for `0x41`. The two horizontal dimensions are equal for each ID, so the exchange alone does not change the footprint or the placement height for the same candidate and terrain. In the Game object targeting walk at `0x08ae50`, the turn is also read for the horizontal extent exchange on admitted records. Its type masks exclude both IDs, and the alternative admission through a nonzero byte at record `+0x38` is false for the freshly emitted values above. The inspected targeting branch therefore does not use their turn on those fresh values. This is a branch-specific observation about targeting.

The renderer's live-vector walk `0xd3170` calls `0xe0ac0` at `0xd9244` after its visibility tests. On the common model path, when the lookup returns a model, `0xe0ac0` takes the turn as an unsigned byte and uses it in 90-degree steps for the horizontal rotation. The rotation uses sine and cosine with the original radians-per-degree constant at `0x42e520`. That calculation uses the full byte with no preceding parity or four-value mask. The evidence establishes a transform input and does not settle the visual symmetry of either model.

## Equality scope

The original field comparator `0x2fa1c0` compares the 32-bit ID, all three 64-bit coordinates, the full turn byte, the floats at `+0x24` and `+0x2c` by floating-point equality, the byte at `+0x28`, the 64-bit field at `+0x30`, the embedded item through `0x4df60`, and the byte at `+0xd8`. The item comparison tests its scalar fields and uses the count at object `+0xd5` to bound comparison of the four-byte entries beginning at `+0x55`. For these fresh records the count is zero, so no entries are compared even though the constructor zeros their storage. The whole-vector check `0x2f83a0` applies record equality in order after checking length. Refresh uses the record comparator and the whole-vector comparison, so a difference in the full turn byte can affect native change detection.

For corresponding records emitted by the traced branch, the ID and position writes are comparable given the same selector, candidate and placement inputs. The initialized scalar and item fields have the defaults above before later replacements. The turn byte has exact value-copy semantics when comparing the same retained record across copies. The generation path establishes no expected turn value for a separately generated record. The horizontal extent calculation's use of only parity does not reduce the byte comparison made by the native equality routine.

The defined-field copy and equality paths omit the intervening bytes at `+0x04–0x07`, `+0x21–0x23`, `+0x29–0x2b`, `+0x39–0x3b`, `+0x4d–0x4f`, `+0xd6–0xd7` and `+0xd9–0xdf`. Equality of all `0xe0` storage bytes does not follow from native record equality. These conclusions are bounded to the traced generation, copy and consumer paths. A complete inventory of other emitters, direct turn writers and interaction paths remains open.
