# Steam Water Bubbles creation and caster power

FINAL / STABLE / HELD

## Supported image and scope

This contract covers the creation of a type 0x11D bubble by action 12 at RVA 0x2C3710 in the Windows x64 Steam image. The original unpacked executable is 5,757,952 bytes, SHA-256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244, preferred image base 0x140000000, image size 0x58A000, PE timestamp 0x5D93AB0D. The companion packed original is 5,939,072 bytes, SHA-256 A1EFBBD41EEB8DF08395C4D6511194D416CB70A14054B2A2000B9F5CDBCFA65B. All RVAs below refer to the unpacked image. Field offsets are relative to the named record. Subtype and action lists below use decimal numbers unless a number has a 0x prefix.

## The three creation draws

After the type 0x11D appearance initializer returns, the creation branch makes three direct calls to the imported C runtime rand. The unpacked image's import descriptor resolves RVA 0x42CD28 to api-ms-win-crt-utility-l1-1-0.dll!rand. Each call uses this same slot. The return RVAs are the first instructions in 0x2C3710 after the corresponding calls.

| Purpose | Call RVA | Return RVA | Effect of that draw |
| --- | --- | --- | --- |
| Bubble power | 0x2C4B96 | 0x2C4B9C | Convert the draw to float, multiply by 0.5, divide by 32767 and add 0.75. Multiply the caster power specified below by that factor and store the result as a float at bubble +0x184. The caster-power call itself is at 0x2C4BBD and returns to 0x2C4BC2. |
| Bubble size | 0x2C4BCE | 0x2C4BD4 | Convert the new draw to float, multiply by 0.75, divide by 32767 and add 0.75. Multiply each initialized size component at bubble +0x84, +0x88 and +0x8C by this one shared factor. |
| Spawn sound | 0x2C4C6D | 0x2C4C73 | Convert the new draw to float, multiply by 0.1, divide by 32767 and add 0.8. Store this float immediately after the 32-bit sound identifier 0x30 in the queued event. The following float is 1.0. The sound uses the bubble's position and is queued before registration of the creature. |

Arithmetic for the three factors and their application is binary32, with multiplication before division. For draws of 0 and 32767 the power factors are 0.75 and 1.25, and the size factors are 0.75 and 1.5. With bare, unmodified caster power of 5, the two bubble power endpoints are 3.75 and 6.25. These three draws are the direct draws for the specified fields. The contract does not count random activity inside the appearance initializer or other subsystems.

## Selector 1 and caster modifiers

Creation passes the caster and selector byte 1 to RVA 0x065C70. For any nonzero selector, RVA 0x052CE0 includes an equipped item at caster +0x6D8 first and the one at +0x638 second. Each qualifies independently when its category byte at item +0x00 equals 3. Both can contribute, including items that would fail the selector-zero class or action checks. No other equipment slots enter this sum.

Begin with binary32 value 5. Add each selected item's contribution in the given order. Then apply the following multipliers in order, rounding each operation to binary32. Flags are bits of the 32-bit word at caster +0x80.

| Condition | Multiplier |
| --- | --- |
| Bit 9 set, mask 0x00000200 | 1.5 |
| Bit 17 set, mask 0x00020000 | 1.2 |
| Bit 18 set, mask 0x00040000 | 4 |
| First effect of type 0x17 in forward order in the list at caster +0x980 | The effect's signed float at payload +0x04. The payload begins at list node +0x10 and its type is the first byte. Only the first match is used. |
| Bit 26 set, mask 0x04000000 | 1.1 |

An absent condition leaves the current value unchanged. The first matching effect contributes its stored magnitude directly, without an age, owner, or positive-value test in this routine. The caster-level field copied during bubble creation does not enter this computation.

## Contribution from one equipped item

RVA 0x10AD90 uses the 32-bit subtype at item +0x04. RVA 0x109C50 computes a base value from item and caster fields, and 0x109720 supplies the effective regional rarity.

1. Read the item's two signed 32-bit region coordinates at +0x0C and +0x10, its unsigned 32-bit seed at +0x08 and unsigned rarity byte at +0x14. The comparison region is the caster's pair at +0x150 and +0x154 when the caster byte at +0x60 is 0, 4 or 6. Every other value uses the item's own region.
2. Equal regions give the item's rarity. A category-3 item also retains its rarity in neighboring regions, including diagonals, when the original seed is evenly divisible by ten times one plus rarity (the divisor is 10 at rarity 0 and 20 at rarity 1). In every other region use effective rarity -1. For the neighboring-region check the original uses 32-bit wrapping coordinate differences and their 32-bit absolute values, then compares the larger as signed with 1.
3. Use a copy of the seed for a single unsigned 32-bit xorshift32 transformation, with shift distances 13 left, 17 right and 5 left, in that order. At each step XOR the current word with its shifted value and retain 32 bits. Take the unsigned result modulo 1000 and divide by 2000 as a float. Add this fraction to the effective rarity as a float. This deterministic item variation consumes no rand draw.
4. Read the unsigned byte at item +0x9D. Multiply this value by 0.5 and divide by 32 for subtypes 5, 6, 7, 8, 10, 11, 15, 16, 17, 18 and 23. For every other subtype divide by 16. Add this term to the preceding sum. The binary uses the C runtime single-precision powf with base 2 and this exponent. The term arithmetic and sums are binary32.
5. For subtypes 3 and 4, contribute the base value added to itself. For subtypes 5 and 13, contribute four times the base. For subtype 11 with a nonzero caster byte at +0x60, contribute six times the base. For subtypes 6, 7, 8, 10, 11, 15, 16, 17, 18 and 23 otherwise, contribute eight times the base. For every remaining subtype contribute four times the base. Category other than 3 contributes zero.

For example, a category-3 subtype-5 item with seed, rarity and +0x9D all zero in its own region has a base of 1 and contributes 4. With just that item and no modifiers, caster power is 9. Subtype 11 with the same item inputs contributes 8 for caster byte 0 or 6 for a nonzero caster byte, provided the region condition is still satisfied.

## Selector 0

Zero selects slots by RVA 0x04FD40. Once it has selected the items, the same initial 5, item calculation, ordered sum and caster modifiers apply. This selector is not used for the bubble power draw.

A slot is eligible if its category is 3. A caster byte at +0x60 of zero additionally requires class eligibility using the caster byte at +0x14C (RVAs 0x1094D0 and 0x050640). For category 3, subtypes 0, 1, 2, 13, 15, 16 and 17 require class 1. Subtypes 6, 7, 8 and 14 require class 2. Subtypes 10, 11 and 12 require class 3. Subtypes 3, 4 and 5 require class 4. Subtype 9 and unsigned subtypes above 17 pass for any class.

First look for an eligible subtype 6 or 7 at +0x6D8, then at +0x638. If found, select just that item and finish slot selection. Otherwise read the caster's action byte at +0x68 and use the following order, skipping ineligible items.

| Caster action, decimal | Eligible slots in order |
| --- | --- |
| 3, 5, 8, 9, 11, 19, 40, 42 | +0x638 only |
| 41, 43 | +0x6D8 only |
| 12, 13, 17, 18, 21, 55, 95, 100, 108, 140, 148, 158 | +0x638 then +0x6D8 |
| All others | +0x6D8, then +0x638 only when that second slot has subtype 12 |

## Evidence boundary

Source exposure was limited to the isolated author policy, filenames, the original Steam executable pair, original Steam disassembly and import exports, and the individual original decompiler exports for the RVAs cited above. The import slot, random call lengths, return RVAs, constants and selector switch data were checked against the unpacked binary. Identifiers for equipment, flags and buffs are given as observed numeric fields. The evidence is static and specific to the stated image. Sound mixing, per-thread runtime seeding, the behavior of other skills, and runtime test outcomes are outside the contract.
