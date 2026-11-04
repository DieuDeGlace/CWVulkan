# Steam generated proper names

## 1. Scope, image and native interface

This is a bounded contract for the ordinary generated proper-name routine at RVA **0x611d0** and its directly reached fragment/string operations. It is independent native analysis, not a runtime capture or an account of a replacement implementation. The supported original AMD64 image is `cubeworld.original.exe.unpacked.exe`, preferred image base **0x140000000**, SHA-256 **7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244**. Addresses below are RVAs in that image.

There are two integer inputs after the output storage: the **32-bit seed** in the second argument, and the **signed32 selector** in the third. On the reached Windows x64 path the output address is in RCX, the seed bits in EDX and selector bits in R8D. The caller supplies storage for a native 32-byte wide-string result; normal completion constructs the result there and returns that address. This is not a public exported or validated arbitrary-pointer API.

The selector chooses a pair of fragment arrays and also participates in both hash inputs. It is not a checked enum of eleven values, and not intrinsically a compact creature-race index. The native dispatch has a defined generic-table fallback for every unlisted signed32 value. No float arithmetic, world lookup, dictionary lookup, CRT rand draw or srand call belongs to the reached name-selection computation. Static initialization and ordinary string storage are separate from its deterministic seed arithmetic.

## 2. Exact selector-to-table contract

The two array bases in the following table identify **runtime wide-string records**, not arrays of literal pointers. Count is the separate length of each array. Each selected prefix or suffix advances its base by **0x20 bytes times its zero-based index**. Dispatch is at **0x640af–0x64529**.

|Table label in this note|Admitted selector values, decimal|Count per array|Prefix record-array RVA|Suffix record-array RVA|
|---|---|---:|---|---|
|generic|6; −1; every unlisted signed32 selector|30|0x54e2c0|0x54e690|
|0|0|10|0x54efa0|0x54f0f0|
|1|1|10|0x54f240|0x54f390|
|2|2, 18, 43, 83, 202, 208|10|0x54ea60|0x54ebb0|
|3|3, 45, 84, 203|10|0x54ed00|0x54ee50|
|4|4|10|0x54f4e0|0x54f630|
|5|5|10|0x54f780|0x54f8d0|
|7|7|10|0x54fa20|0x54fb70|
|8|8|10|0x54fcc0|0x54fe10|
|9|9|10|0x54ff60|0x5500b0|
|10|10|10|0x550200|0x550350|
|11|11, 260|10|0x5504a0|0x5505f0|
|12|12, 261|10|0x550740|0x550890|
|13|13|10|0x550f20|0x551070|
|14|14|10|0x5511c0|0x551310|
|15|15, 17, 96, 97, 165, 269|10|0x5509e0|0x550b30|
|16|16|10|0x550c80|0x550dd0|

The numeric table labels above are descriptive local labels. They do not declare a creature-race enum or assign an English species to the fragments.

In particular, among **0 through 10**, only **6** uses the generic 30-entry arrays. **11** is a distinct 10-entry pair; it must not replace 6 or −1. **−1** is admitted through the generic fallback, not through a separate region-only algorithm. Other negative selectors, high positive values absent from the table, and selector bit patterns with bit31 set also take the generic fallback.

Aliases in a row share fragment arrays **only**. Hashing precedes dispatch and uses the original selector, not the row label or a canonicalized alias. Thus 2 and 18 need not choose the same entries; neither do 6 and −1. The signed comparison against 260 and the subsequent unsigned bounded dispatch at **0x640af–0x640e6** preserve this fallback for negative values; no negative value indexes the jump table.

## 3. Arithmetic, indices and composition

The two initial hash-input words are formed independently, using the input seed and selector bit patterns:

- The prefix input is the seed plus **57 times the selector**.
- The suffix input is **57 times the seed**, plus the selector, plus **0x003acaa7**.

Every multiplication, addition, shift, XOR and subtraction in this computation uses a **32-bit word**, discarding overflow. Negative signed inputs contribute their original two's-complement bits. There is no absolute value, signed remainder correction, seed clamp, floating conversion or extra random draw.

Each initial word undergoes the same deterministic transformation, in this order, at **0x64030–0x640ac**:

1. XOR the word with its own left shift by **13**, discarding bits shifted out.
2. Square that XOR result, multiply the resulting word by **0x0000ec4d**, add **0x0131071f**, multiply by the XOR result, then add **0xd208dd0d**. Each stage retains only its low32 bits.
3. Clear bit31, leaving an integer from 0 through 0x7fffffff.
4. Take its nonnegative integer quotient by **21474 (0x53e2)**, then subtract that quotient from **100000**, retaining a 32-bit result.

The native quotient uses the unsigned high-product reciprocal **0x61a8f94d** with a further 13-bit shift; it realizes the stated quotient on the masked input domain. The masked input alone bounds the quotient by **0 through100003**. Consequently a nominal description of the result as always0 through100000 is not a sufficient arithmetic contract: quotients above100000 would produce **0xffffffff, 0xfffffffe or0xfffffffd**, without a clamp. This bound is not a claim that every edge value is attained by a particular seed in the polynomial's image.

The prefix and suffix indices are the **unsigned32 remainders** of their respective complete subtraction-result words by the selected array count, **10 or30**. The consumer does not reinterpret a wrapped hash word as a negative integer, take an absolute value or reduce a signed remainder. Only after reduction is the bounded index widened for the 64-bit record-address calculation and multiplied by0x20. The earlier initial-input additions and both hash subtractions remain32-bit operations; widening the seed/selector arithmetic before its prescribed wraps changes the contract.

For selector −1 specifically, the first initial word is the seed minus57, and the second is 57 times the seed plus0x003acaa6. That specialization does not apply to all generic-table selectors.

The final bit31 mask makes the hash outputs insensitive to input seed bit31. For a fixed selector, seeds differing by 0x80000000 therefore select the same name. This follows from the low-word arithmetic; it is not a special negative-seed branch. The original selector still controls dispatch: changing its bit31 may change the selected table even when the hash outputs coincide.

At **0x6452f**, helper **0x4cc10** constructs the output and appends the selected prefix followed immediately by the selected suffix through **0x3ced0**, after preparing space for their combined code-unit lengths. There is **no separator, added space, case conversion, normalization, gender suffix or dictionary expansion** in this routine. Fragment characters and their case are data. Later grammar substitution or full-item-name composition is outside this string concatenation.

## 4. Fragment storage, initialization and owner-local access

The image contains **380 fragment records** in seventeen pairs of arrays: sixty records in the generic pair and twenty in each of the other sixteen pairs. Entry **0x611d0** checks the guarded static arrays before the selection arithmetic. The initialization region **0x611d0–0x64014** initializes the array groups independently using **0x3d4f0** with explicit lengths or **0x41d80** with a NUL-terminated UTF-16 source. The latter measures the source before reaching the same string-assignment machinery. The ordinary completed first entry checks and initializes every required array group, not just the pair ultimately selected.

These are process-static, lazily populated string records, not World-owned name tables and not a seed-dependent vocabulary. The guarded initialization registers array cleanup for process termination. Their zero-initialized storage in the PE is **not** an on-disk fragment-pointer table. This note does not establish safety of reading partially initialized records concurrently, mutation by injected modules, or recovery from failed initialization.

Each runtime record is a **32-byte UTF-16LE wide string**:

|Record-relative offset|Meaning established by the reached string consumers|
|---|---|
|+0x00|16-byte inline character area, or an eight-byte character-data pointer when not inline|
|+0x10|Unsigned64 length in UTF-16 code units, excluding the terminator|
|+0x18|Unsigned64 capacity in UTF-16 code units|

Capacity at most7 selects the inline area; capacity greater than7 selects the pointer. The verified literal fragments have lengths **1 through 6**, so their normal freshly initialized records fit the inline representation with capacity7. This does not change the general representation of a generated result, whose concatenation may require allocated storage.

The following section supplies owner-local **literal RVA/length metadata only**, with no fragment text. It allows the constant bytes in the supported owner's original image to be identified independently of observing lazy runtime initialization. A literal length is a count of two-byte UTF-16LE code units, excluding the following NUL. For a mapped image an RVA is relative to its actual image base; for a PE file it must be translated through the appropriate section's virtual/raw mapping. Repeated literal RVAs are intentional sharing. This manifest is build-specific, and does not license or bundle the vocabulary.

## 5. Literal-address and length manifest

Each sequence below is in increasing array-index order. A pair is **literal RVA:code-unit length**. The first-index column establishes the index of the first pair; subsequent pairs advance by one. Table labels refer to section2. These addresses and lengths come from the entry's native initialization calls and the directly referenced NUL-terminated literal storage; no fragment characters are reproduced.

|Table|Indices|Prefix literals|Suffix literals|
|---|---|---|---|
|generic|0–9|0x431d60:3, 0x431d68:3, 0x431d70:2, 0x431d78:2, 0x431d80:3, 0x431d88:2, 0x431d90:3, 0x431d98:2, 0x431da0:3, 0x431da8:3|0x431e58:2, 0x431e60:2, 0x431e68:2, 0x431e70:3, 0x431e78:3, 0x431e80:3, 0x431e88:3, 0x431e90:3, 0x431e98:3, 0x431ea0:3|
|generic|10–19|0x431db0:2, 0x431db8:3, 0x431dc0:2, 0x431dc8:3, 0x431dd0:4, 0x431de0:4, 0x431dec:2, 0x431df8:4, 0x431e08:3, 0x431e10:3|0x431ea8:3, 0x431eb0:3, 0x431eb8:3, 0x431ec0:4, 0x431ed0:3, 0x431ed8:2, 0x431eb0:3, 0x431ee0:4, 0x431e70:3, 0x431ef0:3|
|generic|20–29|0x431e18:3, 0x430e28:1, 0x431e20:2, 0x431e28:3, 0x431e30:3, 0x431e38:2, 0x431e40:2, 0x431dc8:3, 0x431e48:3, 0x431e50:2|0x431ef8:3, 0x431f00:3, 0x431e80:3, 0x431f08:3, 0x431f10:3, 0x431f18:3, 0x431f20:3, 0x431f28:4, 0x431f38:3, 0x431f40:3|
|0|0–9|0x432098:2, 0x4320a0:2, 0x4320a8:3, 0x4320b0:4, 0x4320bc:2, 0x4320c4:2, 0x430e64:1, 0x4320d0:3, 0x4320d8:2, 0x4320e0:3|0x4320e8:3, 0x4320f0:3, 0x4320f8:3, 0x431f00:3, 0x432100:3, 0x431fc8:6, 0x432108:4, 0x432118:4, 0x432128:6, 0x432138:5|
|1|0–9|0x431e20:2, 0x430e64:1, 0x432144:2, 0x432150:3, 0x432158:2, 0x431d68:3, 0x432160:2, 0x432168:4, 0x432178:3, 0x4320a0:2|0x432180:4, 0x432190:4, 0x4321a0:4, 0x4321b0:4, 0x432064:2, 0x4321c0:3, 0x4321c8:5, 0x4321d4:2, 0x4321e0:4, 0x4321ec:2|
|2|0–9|0x431f48:2, 0x431d88:2, 0x431d70:2, 0x431d78:2, 0x431db0:2, 0x431f50:3, 0x431f58:4, 0x431f68:3, 0x431f70:2, 0x431f78:3|0x431f80:3, 0x431f88:3, 0x431f90:4, 0x431fa0:4, 0x431fb0:4, 0x431fc0:3, 0x431fc8:6, 0x431fd8:3, 0x431fe0:3, 0x431fe8:2|
|3|0–9|0x431ff0:2, 0x431ff8:3, 0x432000:3, 0x431f70:2, 0x431d98:2, 0x432008:4, 0x432018:4, 0x432028:3, 0x432030:2, 0x432038:3|0x432040:3, 0x431fe8:2, 0x431e70:3, 0x432048:4, 0x432054:2, 0x43205c:2, 0x432064:2, 0x432070:5, 0x432080:4, 0x432090:3|
|4|0–9|0x4321f8:3, 0x432200:2, 0x432208:2, 0x432210:3, 0x432218:3, 0x432220:2, 0x432228:2, 0x432230:3, 0x432238:4, 0x432248:3|0x4320f8:3, 0x432250:3, 0x432258:3, 0x432260:3, 0x432268:3, 0x432270:2, 0x432278:3, 0x431e60:2, 0x432280:3, 0x432288:3|
|5|0–9|0x431d80:3, 0x432200:2, 0x432220:2, 0x432290:2, 0x432298:4, 0x4322a4:2, 0x4322ac:2, 0x4322b8:3, 0x4322c0:2, 0x4322c8:3|0x4322d0:2, 0x4321c0:3, 0x4322d8:2, 0x4322e0:2, 0x4321ec:2, 0x4322e8:2, 0x4322f0:4, 0x431e60:2, 0x4322fc:2, 0x432304:2|
|7|0–9|0x432310:3, 0x432318:3, 0x432320:3, 0x432328:4, 0x432338:3, 0x432340:4, 0x43234c:2, 0x432248:3, 0x432358:3, 0x432360:3|0x432368:3, 0x431f80:3, 0x4320f8:3, 0x432370:3, 0x432378:3, 0x431e88:3, 0x432380:3, 0x432388:3, 0x432390:2, 0x432398:2|
|8|0–9|0x4322c0:2, 0x432318:3, 0x4323a0:2, 0x432220:2, 0x4323a8:3, 0x432340:4, 0x4320a0:2, 0x4323b0:3, 0x432358:3, 0x4323b8:3|0x4323c0:3, 0x4323c8:2, 0x4323d0:3, 0x4322e8:2, 0x4323d8:3, 0x4323e0:3, 0x4323e8:4, 0x4323f8:3, 0x4322d0:2, 0x432400:2|
|9|0–9|0x432408:3, 0x432410:3, 0x432418:3, 0x432420:2, 0x431d70:2, 0x432428:3, 0x432430:4, 0x432208:2, 0x432440:4, 0x432450:3|0x432458:3, 0x432460:4, 0x431e88:3, 0x432470:4, 0x4320f0:3, 0x432480:3, 0x432488:4, 0x432498:3, 0x4324a0:2, 0x4324a8:3|
|10|0–9|0x4324b0:3, 0x4324b8:4, 0x4324c8:4, 0x4324d8:3, 0x4324e0:2, 0x4324e8:3, 0x432208:2, 0x4324f0:3, 0x432440:4, 0x432450:3|0x4324f8:3, 0x432500:4, 0x43250c:2, 0x432518:4, 0x4322e8:2, 0x432528:4, 0x432534:2, 0x432540:3, 0x4323e0:3, 0x432548:3|
|11|0–9|0x432550:2, 0x432208:2, 0x432218:3, 0x432558:2, 0x432560:3, 0x432568:3, 0x432570:2, 0x432208:2, 0x432578:2, 0x432580:3|0x432588:2, 0x432268:3, 0x432590:3, 0x432598:4, 0x4325a8:3, 0x4325b0:3, 0x4325b8:5, 0x4325c8:4, 0x4325d8:3, 0x4325e0:4|
|12|0–9|0x432550:2, 0x432208:2, 0x432218:3, 0x432558:2, 0x432560:3, 0x432568:3, 0x432570:2, 0x432208:2, 0x432578:2, 0x432580:3|0x431ed8:2, 0x4325f0:3, 0x4325f8:3, 0x432600:5, 0x432610:4, 0x432620:3, 0x432628:5, 0x432638:5, 0x432648:4, 0x432658:4|
|13|0–9|0x4327a8:3, 0x4327b0:3, 0x4327b8:4, 0x4327c8:3, 0x4327d0:2, 0x4327d8:3, 0x4327e0:3, 0x4327e8:4, 0x4327f8:3, 0x432800:3|0x432808:3, 0x432810:3, 0x432818:3, 0x432820:3, 0x432828:3, 0x432830:3, 0x432838:3, 0x432840:3, 0x432848:3, 0x432850:3|
|14|0–9|0x4327a8:3, 0x4327b0:3, 0x4327b8:4, 0x4327c8:3, 0x4327d0:2, 0x4327d8:3, 0x4327e0:3, 0x4327e8:4, 0x4327f8:3, 0x432800:3|0x431e70:3, 0x432858:3, 0x432860:3, 0x432858:3, 0x432868:4, 0x432878:4, 0x432888:5, 0x432898:3, 0x4323e8:4, 0x4322e0:2|
|15|0–9|0x432218:3, 0x432668:3, 0x432670:2, 0x432678:3, 0x432560:3, 0x432680:3, 0x432688:3, 0x432690:3, 0x432698:3, 0x431db8:3|0x4326a0:2, 0x431e88:3, 0x4326a8:4, 0x4326b8:6, 0x4326c8:5, 0x4326d8:3, 0x4326e0:5, 0x4326f0:3, 0x4326f8:4, 0x432708:3|
|16|0–9|0x432710:2, 0x432698:3, 0x432670:2, 0x432678:3, 0x432560:3, 0x432718:3, 0x432720:3, 0x432690:3, 0x4320a8:3, 0x432728:3|0x431e68:2, 0x432730:4, 0x432740:4, 0x432750:5, 0x432760:3, 0x432768:4, 0x432778:5, 0x432788:4, 0x432040:3, 0x432798:5|

All380 runtime positions in section2 have exactly one manifest entry. This is the complete bounded fragment data-access manifest for this routine, not a catalog of all game text or all proper-name callers.

## 6. Reached caller contracts and their limits

**Creature-label callers** at **0xb13a6–0xb13dd** and **0xb43d3–0xb43de** forward the low32 bits of the creature ID at creature+0x08 as the seed, and the signed32 race value at creature+0x64 as the selector. Those reached forwarding sites do not compact the race or translate it into an eleven-family range. The alias table in section2 is the generator's own dispatch, not a caller rewrite. This does not establish an exhaustive creature-race catalog or every upstream producer of that field.

**Full-item naming** reaches this routine at **0x162efa**, **0x16406b**, **0x1642c2** and **0x164560**. The selector there is the **unsigned remainder by11 of the low32-bit product of seven and the item's modifier word**. Consequently it admits every value0 through10, including generic-table selector6, and does not request selector11. The ordinary seed is the modifier word itself. The established material-bearing category0x18 branch uses item+0x18 as seed while deriving the selector from the modifier as before. The original item fields and surrounding naming/grammar conditions are specified separately in [steam-item-full-name.md](steam-item-full-name.md).

**Region-label callers**, including **0x108322**, pass selector **−1**. Their coordinate-derived hash is a seed supplied to this routine, not an alternative selector or a reseed of CRT state. The owner-local region-name use is specified in [steam-identification-ui.md](steam-identification-ui.md) and [steam-map-region-history.md](steam-map-region-history.md).

Other direct callers can supply selectors selected upstream, including values beyond0..10. This note establishes the complete selection behavior **inside0x611d0**, not every such upstream rule. It makes no universal claim that every call derives the selector from a creature race, that every race value is naturally produced, or that a shared-table alias can replace an original selector. Dictionary language, chosen-name overrides, malformed output storage and alternate builds remain outside the bounded contract.

## 7. Contract-derived integer examples

These are **independent arithmetic calculations from sections2–3**, not original-game execution, captured names or native runtime oracle records. They require no fragment contents. The two hash-result columns are unsigned32 decimal values after subtracting the quotient from100000. Indices are zero-based and refer to the arrays selected by the original selector.

|Seed bits, shown as signed input|Selector|Prefix hash result|Suffix hash result|Prefix index|Suffix index|
|---|---:|---:|---:|---:|---:|
|0|-1|63999|46952|9|2|
|0|0|35908|4770|8|0|
|0|2|21061|83389|1|9|
|0|11|60280|10960|0|0|
|−1 (0xffffffff)|-1|65383|35150|13|20|
|−1 (0xffffffff)|0|63794|57344|4|4|
|−1 (0xffffffff)|2|42968|3515|8|5|
|−1 (0xffffffff)|11|736|86667|6|7|
|−2147483648 (0x80000000)|-1|63999|46952|9|2|
|−2147483648 (0x80000000)|0|35908|4770|8|0|
|−2147483648 (0x80000000)|2|21061|83389|1|9|
|−2147483648 (0x80000000)|11|60280|10960|0|0|
|0|6|84577|21320|7|20|
|0|18|98499|10896|9|6|

The repeated seed0/0x80000000 rows exercise the hash's seed-bit31 invariance. The selector6/−1 rows exercise distinct hashing within one generic array pair. The selector2/18 rows exercise distinct hashing within an aliased 10-entry pair. These are selection/index fixtures only; no owner vocabulary or complete world/name-generation parity is claimed.
