# Potion slot count widths and native consumption order

This complete, code-free original-side contract audits the potion-consumption branch of fight routine RVA 0xBAC0, with emphasis on the counted-slot admission, 32-bit decrement, subsequent signed comparison and complete negative-count cases. E denotes the entity passed as the routine's second argument. B denotes the selected counted inventory slot, whose item begins at B+4. All routine and call-site addresses below are RVAs in the supported original Steam x64 image.

The original file is binary/cubeworld.original.exe.unpacked.exe beneath C:/Users/DieuDeGlace/Documents/decompiled cube world steam. Its SHA256 is 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. PE Machine is 0x8664, image base is 0x140000000, and timestamp is 0x5D93AB0D. The original export metadata agrees with that identity. The 167 selected instruction records covering entry/branch sites, the reached item copier and selected epilogue sites were compared with the file-backed image bytes; all matched. The float constants below were also read from that image.

Only the standalone author policy and filename-selected original executable, original routine exports, native instructions and original metadata were inspected. No replacement implementation, SDK/dependency source, tests/results, project startup/memory/history, owner outputs or earlier held artifacts were read. This is factual source-side prose, with no implementation guidance.

## Admission and traversal

This branch follows the routine's earlier flag/action admission work. Its immediate numerical conditions, at 0xBBFD through 0xBC35, are ordered, strict single-precision comparisons. The routine first samples E+0x180, calls 0x5FAC0 on E, multiplies that result by float 0.25 from RVA 0x4AC95C, and requires the sampled E+0x180 to be strictly below the product. Only if that succeeds does it sample float E+0x6C, call 0x4F220 on E, convert that helper's signed 32-bit return to single precision, and require the sampled E+0x6C to be strictly greater. Equality and unordered comparisons do not enter the inventory scan. No fixed value for the entity-dependent helper result is assumed.

The scan begins with the eight-byte begin/end pointers at E+0x9F0 and E+0x9F8. It visits each 24-byte outer record in pointer order, using that record's first two eight-byte fields as the inner begin/end. Inner counted slots advance by 0xA4 bytes. Thus B has a four-byte count followed by a 160-byte item. There is no random selection among these slots: the first candidate satisfying the checks below is consumed, and this route returns before considering later candidates.

The checks, in order, are:

1. At 0xBC60, the four-byte value at B is compared with zero. Equality skips this candidate.
2. At 0xBC65, the one-byte value at B+4 must equal 1.
3. At 0xBC6B, the four-byte value at B+8 must equal 1.

The count admission is a 32-bit nonzero test. Its equality comparison establishes no positive signed bound and no upper bound. Every nonzero 32-bit pattern, including the patterns for -1 and INT32_MIN, passes this first test. The item fields must still satisfy the remaining two tests, and the entity/scan preconditions must be met. A zero count rejects this slot; a later slot may still match.

There is no eight-bit, sixteen-bit or sixty-four-bit count load in these checks, and no sign extension is used for admission. The later clearing decision does interpret the resulting 32-bit value as signed.

## Exact order once a candidate matches

The selected candidate enters at 0xBC83. The reached order is:

1. Store one byte 0x54 to E+0x68.
2. Establish a zero register value and store four zero bytes to E+0x6C at 0xBC8A.
3. At 0xBC99, call item copier 0x9CD0 with destination E+0x1D8 and source B+4.
4. At 0xBC9E, decrement the four-byte memory operand at B by one.
5. At 0xBCA0, compare the now-stored four-byte value at B against zero. The signed greater-than branch at 0xBCA3 skips the clearing stores only when this new value is strictly positive.
6. Otherwise store four zero bytes to B at 0xBCA5, then one zero byte to B+4 at 0xBCA8, then four zero bytes to B+8 at 0xBCAC.
7. Both clearing and non-clearing routes converge at the first random call at 0xBCB0. The subsequent random work and entity stores are described below, followed by a return value of 1.

The decrement is a native 32-bit memory operation, so it writes the low 32-bit subtraction result before the clearing comparison. The comparison after it supplies the flags for the signed branch. The branch is not controlled by an original-count comparison, and it does not use the decrement's overflow flag directly. Consequently the INT32_MIN case is resolved from its wrapped, already-stored result.

These are actual machine operations and their order. No conclusion is drawn from a source-language signed-overflow rule.

## Complete count cases

For these cases, assume a complete readable counted slot, ordinary non-aliasing storage, the item predicates equal to 1, and an entity that has reached the scan. The bit patterns and resulting slot effects are:

| Original count at B | Value written by the 32-bit decrement | Subsequent signed decision | Final count and item fields in this slot |
| --- | --- | --- | --- |
| 0, pattern 0x00000000 | No decrement for this slot | Admission rejects it | This branch makes no change to this slot |
| 1, pattern 0x00000001 | 0, pattern 0x00000000 | Not greater than zero | Count zero; byte B+4 zero; four-byte B+8 zero |
| 2, pattern 0x00000002 | 1, pattern 0x00000001 | Greater than zero | Count 1; selected item fields retained |
| -1, pattern 0xFFFFFFFF | -2, pattern 0xFFFFFFFE | Not greater than zero | Count zero; byte B+4 zero; four-byte B+8 zero |
| INT32_MIN, pattern 0x80000000 | INT32_MAX, pattern 0x7FFFFFFF | Greater than zero | Count 2147483647; selected item fields retained |
| INT32_MAX, pattern 0x7FFFFFFF | 2147483646, pattern 0x7FFFFFFE | Greater than zero | Count 2147483646; selected item fields retained |

For -1, the intermediate -2 is stored, then the count is overwritten with zero before the two identifying item fields are cleared. The already-copied active item at E+0x1D8 still contains the pre-clear item fields. The action byte 0x54 and zero timer have already been stored. The two random calls still occur, their entity results are stored, and the branch returns 1.

For INT32_MIN, the decrement wraps to 0x7FFFFFFF. The fresh signed comparison treats that stored result as positive, so all three clearing stores are skipped. The source item remains selected with its original byte B+4 and four-byte B+8 equal to 1, and the count remains 2147483647. The active item was copied before this decrement, the same action/timer stores have occurred, both random calls still occur, and the branch returns 1.

Every other negative signed count decrements to a negative value and takes the clearing route. Every positive original count greater than 1 leaves a positive decremented count and retains the selected item fields. None of these count cases adds an extra random call or prevents the two calls once this candidate has matched.

## Item-copy boundary and limited clearing

The call at 0xBC99 copies the item from B+4 before modifying B. The count itself is outside this item-copy source. Copier 0x9CD0 has no nested call, random draw or allocation.

Relative to its 160-byte item base, it copies one byte at +0; four bytes at +4, +8, +0xC and +0x10; one byte at +0x14; four bytes at +0x18; one byte at +0x1C; all 128 bytes +0x1D through +0x9C in its fixed byte loop; and one byte at +0x9D. The loop is not shortened by item type or a component count. This covers 152 defined bytes. The eight bytes in relative ranges +1 through +3, +0x15 through +0x17 and +0x9E through +0x9F are not copied.

Thus the selected source item is copied to the entity even when the count is -1 or INT32_MIN. The active-item destination padding is not supplied by this copier. When the clearing route follows, it clears only the count at B, the one-byte field B+4 and four-byte field B+8. It does not erase the rest of the source item's fields or its padding, and it does not undo the entity's earlier item assignment.

## Random-call and final-store order

Original metadata identifies the indirect target at RVA 0x42CD28 as the imported rand entry. There are exactly two such calls in the selected consumption block after its candidate match:

- The first call is at 0xBCB0. Its signed 32-bit result is converted to single precision, multiplied by float 4, divided by float 32767, then reduced by float 2. The result is kept for E+0x44.
- The second call is at 0xBCDA. It receives the same ordered conversion, multiplication, division and subtraction. Its result is stored first, as four bytes to E+0x40 at 0xBCFF.
- The first call's saved result is then stored as four bytes to E+0x44 at 0xBD04. Four zero bytes are stored to E+0x48 at 0xBD0A.

The constants are float bit patterns 0x40800000 at RVA 0x4ACA20, 0x46FFFE00 at 0x42E5F0, and 0x40000000 at 0x4AC9D0. The arithmetic uses separate single-precision operations in the stated order. No assumed random range, seed, distribution or external library implementation is needed to establish this call-to-destination mapping.

Only after those stores does the route set the return byte to 1 at 0xBD0E and enter the ordinary epilogue. No later target-selection or attack branch of this invocation is reached along this successful consumption route.

## Evidence limits

The contract describes the reached branch of the supported image, including complete count inputs of -1 and INT32_MIN. It does not claim that ordinary inventory producers generate negative counts, that every actor reaches this branch, or that all potion-related routes use this count rule. The earlier fight admission gates and entity-dependent threshold helpers are boundaries, not a universal caller or eligibility census.

The selected branch contains no lock-prefixed count operation or synchronization call. Its instruction order does not establish atomic publication, concurrent-observer behavior or wall-clock guarantees. Malformed begin/end pointers, invalid storage, aliasing between an inventory item and the entity's active-item destination, asynchronous modification and failed native calls were not established as supported inputs. Random calls made by other callers or earlier helper internals, backend behavior, process-wide seeding and interleavings remain unproved. No unmentioned entity or item byte is inferred cleared.
