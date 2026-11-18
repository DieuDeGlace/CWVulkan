# Steam Bomb terrain persistence

Original evidence is the x64 Steam image `cubeworld.original.exe.unpacked.exe`, SHA-256 `7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244`, and its original instruction and routine exports. All addresses below are relative to image base `0x140000000`.

## Which blocks enter the history

Stock Bomb creation at RVA `0x2C3710` assigns model `0x90` at `0x2C44BE`. During its first detonation update, `0x2A45E0` visits offsets from -8 through +8 in ascending X, then Y, then Z, with Z changing fastest. Only offsets whose squared distance is strictly less than 64 reach the block query. Each offset unit is 65,536 native position units. Each absolute sample position is rounded down to the containing block on all three axes, including negative fractional positions.

The world query at `0xE9A0` supplies six block bytes. The test at `0x2A4A0A` accepts any block with bit `0x01`, bit `0x10`, or both set in byte 5. A block with both bits produces one change. An accepted sample produces debris, calls the live removal at `0x2A59F0`, then appends one change at `0x2A4C14`. The appended old value is the six bytes from the query made before removal. The missing-zone query fallback has neither eligibility bit (`0x1100`, `0xE9A0`).

## Saved change record

Each record is 28 bytes, with no gaps. Integer fields use little-endian byte order. The record's offsets are measured from byte zero of the serialized record.

| Offset | Width | Content |
| --- | --- | --- |
| 0 | 4 | Signed absolute world block X |
| 4 | 4 | Signed absolute world block Y |
| 8 | 4 | Signed absolute world block Z |
| 12 | 6 | Old block bytes, in query order. The first three are red, green and blue. Bytes 3 and 4 are carried verbatim, and byte 5 retains the complete original flags. |
| 18 | 6 | Recorded post-blast block, always `FF FF FF 00 00 00`, at every elevation. |
| 24 | 4 | All four bytes of the current world day at world offset `+0x08`, sampled when the change is appended. |

The day copy is at `0x2A4B85`. The post-blast constant is initialized at `0x5C30`, then copied at `0x2A4BB9` through `0x2A4BF5`. The record copy at `0x38B00` preserves all 28 bytes. The immediate write uses an existing column entry and chooses `FF FF FF 00 00 00` for Z above zero and `FF FF FF 00 02 00` for Z at or below zero (`0x91160`, initializers `0x1CE0` and `0x1D20`). Both live replacements clear both eligibility bits. The saved post-blast bytes come from the constant independently of that live choice.

## From the blast to disk and back

At the end of the world update, `0x2DA6C0` walks the world's change list from first to last. It selects the resident zone using the floor of X divided by 64 and the floor of Y divided by 64. If that zone is present, `0x2DB9D4` appends the entire record to its history at zone offset `+0xD8`. The coordinates inside the record stay absolute. A missing zone receives no append. Interleaved changes for different zones retain their relative order within each zone. The append and storage operations preserve duplicate coordinates. A later blast at a previously cleared cell must pass the same fresh flag test to generate another entry.

`0x2D3E20` copies a selected resident zone's ordered history into the saved state keyed by its signed zone X and Y. This occurs on zone eviction, among other paths (`0x2D2410`). A nonempty history creates the saved zone entry if needed. An empty history replaces a previously saved history when the zone entry exists. In version 44, the zone's two counted collections of 224-byte entries are followed by a four-byte changed-block count and then the 28-byte records in forward order. Writer `0x14AB50` emits every field including the day. `0x2D38E0` writes the serialized world state under the database key `state`. This path writes the zone histories already copied into that state. The blast append itself performs no database write.

`0x2B98B0` loads `state` when present. Reader `0x148E90` reconstructs each zone list in serialized order. When `0x2AE0C0` creates a zone on the local saved-state branch, it generates its terrain, copies the matching saved history to the resident zone, then replays each record in forward order using its absolute XYZ and six post-change bytes. This replay has no equality check against the old block, no day comparison, and no coordinate deduplication. Multiple applicable writes to one coordinate therefore take effect in sequence. A request for an already resident zone returns it directly. The zone setter at `0x31C1C0` applies its ordinary bounds and column-height rules. For Bomb records that it writes at Z at or below zero, it substitutes `FF FF FF 00 02 00` (`0x5F20`). The six saved bytes remain `FF FF FF 00 00 00`.

The separate loaded-zone undo operation at `0x2DA090` traverses the history in reverse, supplies the six old bytes to the zone setter, and clears the resident list. The evidence above establishes the reload behavior and the bytes retained for the day. It does not establish a general expiry interval for Bomb changes.
