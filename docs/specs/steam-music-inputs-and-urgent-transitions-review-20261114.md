# Steam music inputs and urgent transitions

## Scope and original image

The selected viewed Game update at RVA 0xA5A50 chooses the desired music filename, updates transition timing, and publishes the requested path and gain. Its menu override writes “maintheme.ogg” and clears the local ordinary-track flag. The boss flag and positive success-timer snapshot remain available to the later deadline and fade predicates. With positive success time, the inactive-playback branch writes the 10,000 millisecond deadline even when the selected filename is “maintheme.ogg”. The detailed predicates and ordering follow below.

The source authority is binary/cubeworld.original.exe.unpacked.exe under C:/Users/DieuDeGlace/Documents/decompiled cube world steam. It is 5,757,952 bytes with SHA256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. PE machine is AMD64 0x8664, timestamp 0x5D93AB0D, image base 0x140000000, entry RVA 0x3939F0, and image size 0x58A000. All executable addresses in this contract are RVAs. The result is static original evidence. No original execution, reconstruction lead, installation asset, or runtime observation is needed for these claims.

G is the current Game receiver. W is the viewed World read from the eight-byte G+0x8 pointer. E is the current local entity read from W+0x448. Node and entity reads at different points of the block use their respective current owners as specified. The source trace assumes valid ordinary receiver, entity, list and string lifetimes. Arbitrary reentry or concurrent replacement can change an operand between these reads.

## Update boundary and the carried time sample

The resolved direct call export identifies 0x134558 in application frame owner 0x134420 as the selected caller of 0xA5A50. The application loop reaches that frame owner at 0x137B8E. The frame body requires both process pointers at RVAs 0x551A90 and 0x551A80 to be nonnull before this path. It initializes a previous counter time through the native initialization guard when needed, reads the imported performance frequency then counter for the current frame, and forms a signed 64-bit timestamp in nanoseconds using the signed quotient and remainder at divisor frequency, with the scale 1,000,000,000. It subtracts the previous timestamp, stores the current timestamp, converts the signed difference to binary64, divides by binary64 1,000,000,000, and multiplies by binary64 1000. The resulting frame delta is supplied to the Game at 0x551A80. Imports and broken clock/provider domains remain boundaries.

The Game prologue at 0xA5AFC–0xA5B12 uses native binary64 minimum of 1000 and the supplied delta, then saves that result. For ordinary finite input this caps only the high side. The delta used by the music block is the saved value reloaded at 0xAA53B into the carried binary64 register. Later stack reuse for UI data and a timeGetTime sample at 0xAB5BB does not replace that register. The latter sample, the earlier frame timeGetTime calls, and the World subdivision deltas are separate operands. The success and attenuation countdowns, elapsed-time accumulation and fade helper in the music block use the carried delta. The selected clock arithmetic and integer truncations do not guarantee a physical wall-clock duration or a particular number of rendered frames.

On this reached ordinary path the viewed World critical section is entered through 0x2ABB70 at 0xA6464 and released through 0x2B9890 at 0xADB23. These helpers target W+0x3C8. The late Client call at 0xAAEBF and camera call at 0xAAF02 precede the embedded Interface update at 0xAB5CE. The boss scan begins at 0xAB5E4 immediately after that UI-side work and the intervening selected Game field clear. The selected music block finishes before the later draw caller at 0xAD9D4, also within the matching normal World guard. No visible draw, draw success, foreground check, or rendered geometry is tested inside this selected filename/timing branch. The receiver chains and per-call rereads do not prove protection against providers or writers outside the selected locks.

## Per-update boss admission

At 0xAB5E4 the update clears the one-byte local boss flag, carried in the low byte of its saved register. It captures the current W+0x40 live-list head and walks next links. Each live node supplies the eight-byte entity pointer at node+0x10. For that entity the owner selects its E-other+0xB88 tree sentinel and walks tree nodes in forward in-order succession through 0x282660. A candidate tree node contributes only when all of the following conditions hold.

- Its eight-byte key at node+0x20 equals the eight-byte identifier at the current local E+0x8. The update rereads current G+0x8, W+0x448, and E+0x8 at each comparison.
- The candidate entity's byte +0x60 equals 1.
- Original 0x05EEE0 reports bit 18 of that entity's four-byte +0x80 word set, or, only when that result is zero, original 0x05EDA0 reports bit 9 set.

A matching node sets the local byte to one at 0xAB654. The scan continues through both collections and has no subsequent clear within the scan. Empty lists and trees leave zero. This is an identifier membership and bit test. No distance, screen participation, candidate health, or same-type admission is applied in these branches. The selected ordinary live list is valid and has live payloads. There is no general malformed-node recovery claim.

The scan's byte survives the following World and text queries. At 0xAC0A2 a nonzero value causes “boss.ogg” to replace the current desired filename and clears the distinct local ordinary-track byte. This boss byte remains live at 0xAC212 and 0xAC29F even if a later filename override occurs. There is no process-global boss latch in this selected block.

## Stored context and current-position queries

Three position representations are consumed. The stored context pair at E+0x150 and E+0x154 consists of signed 32-bit x and y indices. The current position triple at E+0x10, +0x18, +0x20 consists of signed 64-bit x, y and z, with 65,536 raw units per one integer World coordinate. The music owner uses both truncation toward zero and floor conversion at different call sites. All examples here assume narrowed quotients are representable. The instructions perform low-word narrowing where described, so arbitrary large coordinates do not establish unrestricted mathematical integer behavior.

**Context biome at 0xAB6FC.** The caller reads the two already-stored context words in x then y order and supplies them directly to 0x2B9080 as four-byte arguments. This call uses no direct scaling, additional noise, current camera position, or live XYZ input. The actual original 0x2B9080 body does not consume the W pointer for its decision. Its central case returns 2 when both signed indices are in the inclusive range minus one through one. Outside it, the selected native deterministic coordinate hash uses x plus 57 times y and the offset 0xB3F58. The resulting positive reduction is taken modulo 50. Residues 1–4 return 2, 5–7 return 0, 8–9 return 9, 10–11 return 10, 12–13 return 4, 14 returns 13, 15–16 return 11, 17–18 return 6, 19–22 return 3, 23–25 return 8, 26–27 return 7, 28–29 return 12, and 30–31 return 5. Other residues return 1. The Game retains the full four-byte result for its biome track switch.

The scoped ordinary World-step producer at 0x2DB710–0x2DB827 first admits a nonnull W+0x448, then queries the current full position through 0xEAB0. When returned record byte 5 has bit 5 clear, it passes horizontal coordinates truncated toward zero after division by 65,536 to 0x2ADA20. With that bit set, it first takes the 1024-coordinate groups of those truncated position words, evaluates 0x36160, and obtains the selected integer point from 0x362A0. That point becomes the 0x2ADA20 query. The step reloads the local entity and stores the returned pair to E+0x150 then E+0x154. The music read is subsequent on this normal reached step path. Other writers and runs that skip that step can leave a preexisting pair.

The complete 0x2ADA20 selects a point for the next region query. It groups the signed four-byte input x/y by floor division by 1024, asks 0x36160 and 0x2B99D0 for the group byte/category, and if the category is nonzero computes a 0x362A0 point and the 0x364E0 displacement. It keeps this point only when squared distance is strictly below radius squared and the resulting squared weight is at least binary32 0.25. Otherwise the original integer x/y are passed. The radius is 250 for the zero group byte and 500 otherwise. It forwards to 0x2AD4D0. This latter complete owner takes integer-coordinate inputs, applies two selected noise shifts, and searches the adjacent three by three groups of size 8192 integer coordinates around the warped position. It examines x then y in ascending order within that neighborhood, retaining the candidate with strictly smaller squared distance once a prior candidate is set, and returns the two four-byte group indices of that candidate. This establishes that the stored E pair can be a warped/selected region identity. It does not establish equality with either the current 1024-coordinate group or a current unwarped horizontal position. The transitive noise state, region-generator provider scope, and full alternate context writers remain unclosed.

**Current place coordinates and initial warp-related queries at 0xAB704–0xAB788.** The caller reloads current W and E and takes E+0x10 then +0x18. Each signed 64-bit raw coordinate is divided by 65,536 toward zero, using a sign bias of 65,535 only on negative input and an arithmetic right shift. Its low 32 bits are passed to 0x9BEB0. That complete helper floor-divides the signed word by 1024, including its negative branch. These two 32-bit group indices are retained as the x/y inputs for 0x36160 at 0xAB74A, 0x2B99D0 at 0xAB75F, and 0x362A0 at 0xAB784. The one-byte 0x36160 result goes to the low byte of the fourth argument to 0x2B99D0 and the fifth argument to 0x362A0. The latter receives a refreshed current W plus an output record for two signed four-byte center coordinates in integer World units. The retained category is the four-byte return operand from 0x2B99D0. Its ordinary origin case produces 1. The complete category owner has additional deterministic grouped branches and selected terrain/point queries. The music owner consumes that returned category and does not reclassify it from a name or from the stored context biome.

The 0x36160 group byte is zero or one. It groups x and y by floor division by two and hashes the x half-group with 57 times the y half-group and the offset 0x39A13D. If the selected signed reduction's low bit is clear it returns one. Otherwise it calls 0x36210 on the original group operands. That helper floor-halves both and checks the nonnegative modulo-seven residue of x half-group plus five times y half-group for equality to one. The outer helper then returns the resulting low-byte predicate. These operations use 32-bit native arithmetic. The explicit caller W argument does not supply a provider draw to either predicate.

The original 0x362A0 center helper writes (512, 512) for group (0, 0), independently of the byte. On other groups with byte zero it uses each group coordinate times 1024, adds 282, and adds a deterministic offset with modulus 460. With byte nonzero it rounds the groups down to their even pair anchors via floor-halves, uses the resulting 2048-coordinate group origin, adds 532 and adds an offset with modulus 984. The x and y offset hashes use the selected group x plus 57 times group y with offsets 0x7AB2C and 0x271537 respectively. These are modulo-width integer calculations. The center record captures this first selection before the following cell test. No process random draw is implied by “hash”.

For these center, biome and later parity queries, the common positive hash reduction first XORs the four-byte affine input with its low-width shift left by 13. It squares that mixed word in 32 bits, multiplies by 0xEC4D, adds 0x131071F, then multiplies by the mixed word and adds 0xD208DD0D, retaining the low 32 bits at each step. Clearing the high bit leaves a nonnegative word. The selected unsigned reduction divides it by 0x53E2 and subtracts the quotient from 100,000. The context and center callers use that result with their stated moduli. The one-byte group predicate instead uses its selected signed high-product reduction by multiplier negative 0x61A8F94D, arithmetic shift of the signed high product by 13, and sign correction by one before testing the low bit. These are native hash arithmetic facts. Provider state and provider-derived terrain values remain explicit boundaries.

**Current-position record test at 0xAB789–0xAB7BA.** The caller reloads W+0x448 from the World used for the first center query and passes full signed raw x, y and z to 0xEAB0. That complete helper floor-converts all three coordinates to signed four-byte integer World units, processing z, y, then x internally. Its negative branch decrements the narrowed truncation result when the sign-extended quotient rescaled by 65,536 differs from the original raw input. It forwards to 0xE9A0, which looks up the x/y chunk key at W+0x460 with each component floor-divided by 64. Missing or null chunk uses the selected six-byte process fallback record at RVA 0x54DCB0. A present chunk reaches 0xEB90 with the integer x/y/z. This helper admits x/y within the selected 64 by 64 chunk, indexes the 0x58-byte cell column from chunk+0x518, compares signed z to cell+0x3C and the six-byte vector extent from cell+0x40/+0x48, and returns an admitted stored six-byte value or the selected fallback record. Its local fetch 0xE770 and the process fallback record producers are outside this contract. The music decision consumes byte 5 mask 0x20 through the complete 0x9AE80 leaf. It does not use the record's color, the camera height, or the fractional coordinate bits directly.

If that bit is set, 0xAB7CE calls 0x2AA270 on a fresh current W, the original 32-bit group x/y, current four-byte category, and the retained group byte. Its returned category replaces the initial category. On group byte zero the helper returns zero. With nonzero group byte the direct passthrough categories are 42 and 44 through 47. Input 5 can become 41 when the selected hash's low three bits are zero. Inputs 9 and 15 can become 40 and 43 respectively when the selected low bit is zero. These hash branches floor-halve the original x/y groups and use twice the x half-group, 114 times y half-group and offset 0x62AD0, followed by the same signed reduction as the group byte helper. All other outcomes are zero. This branch bypasses the following warped-distance and weight rejection.

If the bit is clear, 0xAB7D8 first reloads current W and E. It reads the local y coordinate through 0x93870 and then x through 0x93870. This helper floors raw position divided by 65,536 over ordinary representable input. Those resulting 32-bit integer coordinates are the samples for 0x364E0. A negative subunit with raw coordinate minus one therefore supplies sample minus one, even though the earlier grouping and later ground-height input truncate it to zero. The caller obtains a separate center through 0x9BE50 using the retained earlier group x/y. That wrapper recomputes the byte from 0x36160 and forwards it to 0x362A0. It returns the output center record pointer. The earlier center copy remains available for a later category-five query. The 0x364E0 call receives the current W used for this branch, that separately selected center pointer, and x then y as its third and fourth numeric arguments.

The displacement helper calls 0x2E0C0 twice. Both calls convert signed 32-bit sample coordinates to binary64 inputs. The first uses x scaled by 0.005 plus 18843 and y scaled by 0.01 plus 9432. The second uses x scaled by 0.01 plus 299 and y scaled by 0.005 plus 90650. The multiplies precede the additions. These scales and offsets are binary64 constants from the original image. The two noise returns are consumed as binary32. Each return is multiplied by binary32 32, then by binary32 65,536, and truncated to signed 64 bits. Independently for each axis the helper takes the signed integer sample minus center, in 64 bits, multiplies it by 65,536 and adds the corresponding narrow noise displacement. It converts that result to binary32 and multiplies by binary32 one divided by 65,536. It squares each displacement and adds the x square to the y square in the native final lane. The return is a binary32 squared distance, in squared integer-World units. This treats the noise values as provider-return inputs and does not supply the noise implementation's results or state lifetime.

The caller then passes that binary32 squared distance, the retained category and original one-byte group flag to 0x9BEE0. Category zero returns zero. Otherwise radius is 250 for byte zero or 500 for byte nonzero. At finite squared distance below radius squared, the helper divides distance by the squared radius in binary32, subtracts from binary32 one, and squares the remainder. At exact radius or greater it returns zero. At 0xAB836, the caller replaces the category with zero only when binary32 0.5 is ordered greater than the returned weight. Exact 0.5 retains the category. With floating exceptions masked, unordered distance enters the helper arithmetic and unordered weight does not take the caller's clearing move. Unmasked floating exceptions and invalid conversion/domain results are outside the ordinary contract.

The immediate next query at 0xAB86A is separate. It reloads current W/E again and uses 0x137E0 on raw y then x, flooring to integer World units and then flooring the narrowed signed word by 16. The x/y 16-coordinate indices are passed to 0x69C60. That cell-object result feeds the interposed location text, while the place category and stored context biome remain in their separate operands. The fresh position sample at 0xABE66 later uses still another pair of reads.

## Base filename and place overrides

The owner starts with an empty one-byte filename record. Its constructor is 0x7DA80. The selected assign helper 0x82BE0 counts units of the supplied null-terminated literal and replaces the record using the selected owned-string assignment. Names below are exact lower-case one-byte literals, including their internal spaces and hyphen. It stores the base filename before later overrides in this order.

1. If the retained 1024-coordinate group x and y are both zero and the resulting category is nonzero, select “maintheme.ogg” and join at 0xABF84 with ordinary-track byte zero. The no-warp origin category starts at one and can still be cleared by the weight test. If the earlier cell bit diverted to the other category path, its returned category supplies this same nonzero test.
2. Otherwise call 0xEAB0 again at 0xABD91 using fresh current W/E raw x/y/z. If the returned sixth byte has mask 0x20 set, select “shrine.ogg”, write ordinary-track byte zero and join at 0xABF84. The second record result decides this branch even if it differs from the first. It skips the biome, ground and category filename logic.
3. Otherwise select “greenlands.ogg”, then apply the stored-context biome switch, ground-height tests, and place-category switch below. The common join at 0xABF82 sets the ordinary-track byte to one even when no switch case rewrites the filename.

The selected context biome at 0xABDC2 is tested as a four-byte word by a native jump table. Values outside 3–13 and value 8 have no overwrite. The table contains all selected assignments.

| Stored-context biome result | Replacement filename |
| --- | --- |
| 3 | woodlands.ogg |
| 4 | jungle.ogg |
| 5 or 12 | deadlands.ogg |
| 6 or 11 | winter.ogg |
| 7 | mountains.ogg |
| 9 | desert.ogg |
| 10 | savannah.ogg |
| 13 | cult of doom.ogg |

All those outcomes reach the fresh ordinary ground-height call at 0xABE66. It reloads current W and E, fetches raw y then raw x, truncates each signed 64-bit coordinate toward zero after division by 65,536, and passes the low four-byte x then y integer World coordinates to 0x35EA0 with fourth argument zero. No z, 16-coordinate index, 1024-coordinate group, context index, center substitution, camera sample, or earlier distance-return value supplies this call. The full 0x35EA0 owner prepares its own region and place-weight inputs and calls final-height owner 0x2B4450 with mode zero. Its subqueries and final native height provider are outside this music-side closure. The caller takes the binary32 height in original ground-height units and first tests ordered height strictly below binary32 10.0, assigning “beach.ogg” on that branch. It then tests ordered height strictly below binary32 zero, assigning “ocean.ogg” on that branch. Negative finite height runs both overwrites in that order. Exactly zero yields only beach and exactly 10 yields neither. An unordered value with masked exceptions takes neither overwrite.

Finally the retained place category is tested as a four-byte value at 0xABEA7. Only categories 4–37 enter the native table. In-range categories omitted below, and all values outside that range, retain the current filename. This switch occurs after both ground-value overwrites and can replace either one.

| Place category | Replacement filename |
| --- | --- |
| 4 or 9 | unholy pact.ogg |
| 5 | category-five test below |
| 8 | order of the light.ogg |
| 10 | mountains.ogg |
| 13, 20 or 32 | druids of mana.ogg |
| 17 | steel empire.ogg |
| 18 or 37 | cult of doom.ogg |
| 25 | woodlands.ogg |
| 26 | deadlands.ogg |
| 27 | enchanted-forest.ogg |

On category 5, 0xABEE2 calls 0x358E0 with freshly read W and the two integer World coordinates captured in the first output center at 0xAB784. This uses the first center even though the non-bit path previously asked for a separate center to test the current position. The output descriptor has nine entries with a four-byte numeric category and a binary32 squared distance per entry, plus the original input coordinates and its nearest-distance facts. The reached 0x358E0 provider path uses two noise evaluations and the 8192-coordinate neighbor structure, beginning at the previous group on each axis and traversing ascending x then y. Its distance array is real binary32 data despite the integer-pointer presentation in one export. Unproved noise returns and transitive site generation remain dependencies.

At 0xABEF5, 0x2D9F30 reads that descriptor, receives current category 5 and binary32 one. It selects the least available distance among descriptor categories 5 and 12, starting with a negative sentinel when its first entry does not match. If the selected value and descriptor nearest value admit it, the helper subtracts nearest distance from selected distance in binary32, multiplies twice by the binary32 constant at 0x437F68, subtracts from one, and takes the native maximum with zero. The separate category-26 additive arm is not admitted by this call. Ordered returned weight greater than 0.5 assigns “unholy pact.ogg”. Equal, smaller, or unordered return proceeds to 0x2FF5C0 with the retained signed four-byte group x and y. This helper uses the original deterministic positive hash reduction described above with x plus 57 times y and no added seed offset. Low bit set assigns “village.ogg”, low bit clear assigns “village2.ogg”. No rand or srand call is in the selected music decision. This conditional arithmetic does not claim availability or numeric determinism of the whole transitive generator and noise initialization.

The work interposed between the first category calculation and the base filename at 0xABD45 updates location text. It clears the two nodes pointed to by G+0x2D0 and +0x2D8. With local E+0x68 byte 0x9C it reaches the selected “Arrival in ” numeric text and proceeds to the filename stage. The other branch uses the stored context pair/hash and context biome for a “named” term on G+0x2D0. A nonzero 0x69C60 result with nonempty selected term supplies G+0x2D8 before the category fallback. That fallback uses the retained type, group coordinates and group byte when nonzero. The subsequent location notification cache is an independent UTF-16 string at RVA 0x551530, compared with the resulting location term, and conditionally updates names on current G+0x2E0. Its “name”, “detail”, and “location” updates do not write the music filename at RVA 0x551508. The SpiritWorld text test below is also separate. Term dictionaries, font/layout behavior, callbacks, and the location node's provider behavior are not needed to evaluate the native numeric track selection.

## Final override order and flag lifetimes

The automatic path at 0xABF84 performs two separate 0x5E3A0 lookups of numeric argument 0x13 on the current local E. This complete helper searches the E+0x980 linked list and compares the stored one-byte node value at node+0x10, zero-extended, with the four-byte argument. It returns the first matching payload pointer or null. The first lookup at 0xABF94 can write the “SpiritWorld”/“default” term to G+0x2D8. The second at 0xAC05C reloads W/E and tests membership independently. If nonnull it selects “deadlands.ogg”. If that desired name differs from the process-retained name, it writes the process deadline 100. This arm leaves the local ordinary-track byte alone. There is no inference that the two lookups always agree under unproved mutation.

The following steps occur in order.

1. A nonzero boss scan byte selects “boss.ogg” and clears the ordinary-track byte. This branch has no immediate deadline store of its own.
2. The owner updates signed four-byte G+0x2050 first and G+0x204C second. For each, it converts the signed stored value to binary64, subtracts the same carried binary64 delta, truncates toward zero to signed 32 bits, then replaces a negative result with zero and stores that four-byte value. The truncation precedes the sign clamp. With masked invalid conversion the native integer-indefinite word is negative and this path clamps it. Negative elapsed inputs are not ordinary and do not establish countdown monotonicity.
3. It reloads the post-decrement G+0x2050 as a signed four-byte success snapshot. When positive it selects “success.ogg” and writes the process deadline 500 if that name differs from the retained name. This arm does not clear ordinary-track or boss. At zero it makes neither override nor comparison. G+0x204C will be read later under the gain lock as a separate condition.
4. It then tests current node pointers G+0x118, G+0x170, G+0x130 through 0x9AF00 in that short-circuit order. That helper reads node+0x68, its array pointer at +0xE0, and signed four-byte index at +0xAC. It returns true when the selected four-byte integer is nonzero. The three nodes correspond to I+0, I+0x58 and I+0x18 of the embedded Interface beginning at G+0x118. The selected setup creates them under the current parent at the GUI receiver's +0x140. It initializes the first selected integer to one and the two others to zero. This proves state-based admission and the setup defaults, with page/draw behavior outside this block.
5. Any true menu-root test selects “maintheme.ogg”, writes zero to the ordinary-track byte, and writes deadline 500 if the final desired name differs from the retained name. It does not clear the boss scan byte, change either countdown, or replace the post-decrement signed success snapshot. These boss and success operands still govern the later urgent deadline and fade. The compared filename changes, so the menu result can change which subsequent timing branch is reached. Equal filename skips the conditional 500 assignment.

After all these overrides, the owner initializes the one-byte same-track long-hold admission from the ordinary-track byte only if the current four-byte G+0x1BD4 is exactly zero. If that field is nonzero the admission is zero. Only the low byte of the derived register is tested. Its use is on the same-name active branch. The ordinary-track byte is zero for the special origin and shrine branches, one for the full ordinary biome/ground/place branch, unchanged by the type-0x13 and success overrides, and set to zero by boss and by menu. The menu therefore removes that five-minute hold admission while preserving the independent urgency inputs. The retained process same-track byte at RVA 0x533938 is a fourth, distinct byte. It is written by the later timing logic and is not cleared by the menu override itself.

The constructor initializes G+0x2050 to zero. Its contiguous eight-byte store at G+0x2048 writes the binary32 gain 1.0 and zero to the adjacent G+0x204C four-byte countdown. Two earlier selected writers inside the same update place 60,000 at G+0x2050 before the music block. At 0xA7471 a negative result from the earlier local CharacterData membership query and a positive result from its repeat on current E admit the store. That query is 0x5E430, using the selected signed 32-bit group pair, and tests two CharacterData trees in order, at D+0xC8 and +0xD8. An absent D returns false. Before the two membership observations the original group byte can round the pair down to even group anchors. The store is a membership-transition observation, with the source of all membership writers outside this scope.

The other 60,000 store at 0xAAD54 is on the selected source-record/action branch. It is admitted when current E+0x68 is zero or current binary32 E+0x6C is ordered greater than the converted signed result of 0x4F220. The branch inspects the selected record's first byte for 0x18 and its four-byte word at +4 for 1 or 2. Other records set E+0x68 to 0x7C. Literal 1 sets 0x78. Literal 2 with current E+0x5C bit 1 set writes 0x79. The literal-2 case with that bit clear skips these actor record/state copies and still reaches the timer store. The other three paths assign the selected record at E+0x1D8 and clear the four-byte E+0x6C before writing 60,000. These are selected timer producers within this update, without a complete record-generator or actor-action contract.

There is also a selected direct writer of the separate attenuation countdown in original 0xA0420. Its native first argument is G, retained in R9 in the scoped prologue. Its selected admitted block at 0xA05D9–0xA05F5 brings G+0x204C up to at least signed 6000 when the current four-byte selector is at most 10 by unsigned comparison. An already larger signed value is retained. The large effect caller’s earlier route and geometry admissions, and indirect or alias writers of this field, remain unresolved.

## Retained name, active deadline, and gain

The process-retained selected filename begins at RVA 0x551508. It is a narrow owned string with eight-byte length at +0x10 and capacity at +0x18. The selected thread-initialization guard at RVA 0x551528 constructs it empty through 0x7DA80 and registers its exit cleanup before the first selected use. Each track equality decision uses complete narrow-string equality through 0x6DD20/0x6DDF0. It compares byte contents and full lengths and ignores the terminator as a deciding extra unit. The globals at RVAs 0x551504 and 0x533934 are four-byte elapsed and deadline values, compared as signed 32-bit integers. The deadline's initial image value is 300,000 and the byte at RVA 0x533938 initially equals one. The elapsed word lies in the virtual zero-fill area of this image. The normal update writes it as described below. These fields do not establish the exact sound or cursor position of a decoder or provider.

At 0xAC1C6 the Game reads the current audio object from G+0x28. Its ordinary provenance is the startup stack audio owner constructed by 0x328C0 and passed as the sixth argument to 0x7EA80. The constructor stores that pointer in G. The receiver table at RVA 0x42EDA0 maps slot +0x28 to 0x343A0. This complete native leaf is independently bounded by its table entry and two returns, despite lacking the selected decoded C body. It reads the eight-byte receiver+0x48 pointer. Null returns low byte zero. Otherwise it returns the byte at that pointed object's +0x1E0018, zero-extended. Startup 0x34270 can populate that pointer after selected source initialization and imported XAudio calls. A missing channel or cleared status is a concrete low-byte result, with further provider behavior outside the trace. “Active” below denotes nonzero status from this exact leaf. No imported playback status or asset lookup is consulted by the Game at this branch.

### Active status

The 0xAC1E6 block reads the process elapsed word as signed 32 bits, converts it to binary64, adds the carried delta, truncates the sum toward zero to signed 32 bits, and stores it back. Denote that stored elapsed value by T. The caller then compares the final desired filename with the retained filename.

- On equal name, it writes deadline 300,000 when the derived same-track long-hold byte is nonzero. Otherwise it writes T plus 300,000. Both alternatives write the process same-track byte to one. Therefore an ordinary track with G+0x1BD4 zero can cross the same fixed five-minute threshold even on unchanged desired name. Menu clearing this admission instead selects the moving threshold. This branch does not consult boss/success for the deadline.
- On different name and a nonzero current process same-track byte, it writes T plus 500 when the boss byte is nonzero or the signed success snapshot is positive. Otherwise it writes 30,000 for T strictly below 25,000, or T plus 5,000 for T at least 25,000. At exactly 25,000 the latter evaluates to 30,000. It then clears the process same-track byte. These deadline stores replace any earlier 100 or 500 assigned by the filename override stages in this frame.
- On different name and a zero process same-track byte, it skips that recalculation and byte clear. The deadline stays as previously established, which can include an earlier override's conditional 100 or 500 written again in this same frame. In particular, those comparisons occur before the active/different branch, and independent ordered assignments must be retained. The value is not necessarily a fixed T-relative deadline across repeated differing-name updates.

Integer additions use the low 32 bits of native arithmetic and the comparisons use the signed interpretation. Ordinary elapsed values are representable. No saturation, unsigned wrap comparison, or protection of all process globals by the gain guard is inferred. A masked conversion outside range can write the native signed integer-indefinite word. The elapsed accumulator does not apply the sign clamp used by the countdowns.

Next it enters the Game critical section at G+0x1D38. It selects the target and smoothing input in the following order. A fresh signed four-byte G+0x204C value greater than zero forces target binary32 0.015 and rate binary32 0.005. Otherwise it rereads the process elapsed and deadline. Elapsed strictly greater than deadline targets zero. The rate for this fade is binary32 0.005 if the original boss byte survives as nonzero or the saved post-decrement success value is positive, and binary32 0.0004 otherwise. When elapsed is less than or equal to the deadline it targets signed G+0x1BA8 converted to binary32 and multiplied by binary32 0.01, using rate binary32 0.001. Equality with the deadline still uses this nominal-gain arm. This branch calls 0x16EE0 with target address, carried binary64 delta, and the rate, then releases the guard. It writes neither a new requested path nor a new retained filename while status is active.

Complete 0x16EE0 obtains a binary32 factor from 0xA7A0, subtracts the current binary32 gain from its target in binary32, multiplies the difference by that factor, then adds the current binary32 gain and stores the result to G+0x2048. The subtraction, multiplication and addition are separately rounded native scalar operations. The factor uses signed 32-bit truncation of binary64 delta times binary64 100 as its iteration count. At positive count the native double accumulator starts at zero. In ascending order of step indices beginning at one, each step divides that ordinal by the count in binary64, subtracts the current accumulator, multiplies by the supplied binary32 rate promoted to binary64 and scaled by binary64 0.02, then adds the resulting increment back to the binary64 accumulator. The grouped run and tail preserve that ascending index order. The final factor is narrowed to binary32. Nonpositive count produces zero factor. This is the literal ordered gain update, with the current floating-point environment, extreme conversions and provider scheduling unresolved.

### Inactive status

The 0xAC307 block compares the final desired name with the retained name without adding the delta to elapsed. On a difference it first copies the desired name into the process-retained string at RVA 0x551508. Then it enters G+0x1D38, concatenates the six-byte prefix “Music\” and the desired name through 0x6DFA0, assigns the result to G+0x2028, disposes the temporary, and releases the guard. The process string update precedes the guarded path write. Equal filename skips that whole name/path update.

Both cases then assign the process deadline. The value is 10,000 if the saved signed success snapshot is positive, and 300,000 otherwise. This decision follows the menu override and depends solely on the saved success snapshot. The preceding menu/success conditional 500 and type-0x13 conditional 100 do not survive this unconditional inactive deadline assignment. In a separate acquisition of G+0x1D38, the branch writes the nominal converted G+0x1BA8 times binary32 0.01, without using the attenuation countdown or the smoothing path. It releases that guard, then stores process elapsed zero and the process same-track byte one. The published values are the requested path and gain. Actual playback remains a provider boundary.

The complete selected audio consumer at 0x83FB0 is reached by the ordinary constructor's distinct 0x75FF0 thread-start route at 0x7F96A. The one-pointer record supplied through 0x7D560 holds G. The original start wrapper selects native table 0x437EF8 and start boundary 0x3C7D0. That table's forwarder at 0x87BD0 leads to complete entry 0x89020. It detaches the small caller record, signals its selected start condition under the startup mutex, then calls the consumer and ultimately releases the record. This proves the selected handoff, without a complete startup/shutdown/exception guarantee for the imported thread routines.

The consumer checks the current Game+0x2068 active byte for its loop, requests an imported sleep with argument 16 on each admitted iteration, enters G+0x1D38, copies current requested path and reads the binary32 gain, then releases. It obtains its current audio receiver through Game+0x28. Native ordered gain below binary32 0.01 takes slot +0x20. Other comparison outcomes take +0x30 with current gain and a binary32 zero second value. A changed snapshot path relative to that worker’s own retained comparison string reaches slot +0x10 with the current path, an empty secondary path and byte one. It then invokes +0x30 with the sampled gain and binary32 zero before updating its retained copy. Slot +0x18 is invoked each admitted iteration after that comparison. The ordinary original table resolves these slots to 0x34F90, 0x34AA0, 0x34910 and 0x35290 respectively. The corresponding stop/reset, two-channel gain, selected path/load and stream-step boundaries are original native owners. The transitive decoder, file/voice provider, exact status writer census, asynchronous time to silence/start, and provider success/failure remain unresolved. The two Game publications occur in separate critical section acquisitions. The worker’s observation time depends on scheduling.

## Conditional source examples

These conditional examples use hypothetical arguments and state and assume stable ordinary receivers throughout the selected branch.

- If raw horizontal x equals minus one, current-group and later ground-height conversion supply zero for that axis. The no-bit warp-distance sample supplies minus one. The floor-based cell lookup also supplies minus one integer World coordinate and the display query supplies minus one 16-coordinate group index. These callers therefore need not agree on the side of the origin for a negative fraction.
- If a non-special base path gives height minus one and category 8, it selects beach, then ocean, then “order of the light.ogg”. If no place override applies, the negative height leaves “ocean.ogg”. A fresh music-block cell bit on the shrine branch would skip all of those operations.
- Suppose a positive success snapshot and a true menu-root check occur, with “maintheme.ogg” differing from the retained name. If the audio status is inactive, the final requested name is “maintheme.ogg”, the path gets the original prefix, and the final deadline is 10,000. If the status is active, elapsed becomes 40,000 and the process same-track byte was one, the urgent mismatch branch assigns 40,500 and clears that byte. The menu's earlier conditional 500 precedes this later result. Either case retains the success countdown's own post-decrement write.
- Suppose menu is true, the retained name differs, T after accumulation is 12,000, and the prior process same-track byte is one. With boss true and success zero, the active branch writes 12,500. With both urgency inputs clear it writes 30,000. The chosen name in both examples is “maintheme.ogg”. The deadline follows the retained boss byte in both cases.
- If active playback has equal desired/retained name and T equals 300,001, an ordinary-track admission with G+0x1BD4 zero writes deadline 300,000. A menu override selecting the same retained name clears that admission and instead writes 600,001, provided ordinary signed arithmetic remains in range. Positive attenuation still has first priority at the later gain decision. These are different conditions from the active timing latch byte.

## Evidence and limits

The assignment used the standalone docs/CLEANROOM_AUTHOR.md policy and the individually identified original evidence below. Filename discovery was scoped to the original export directories before content reads. The primary 0xA5A50 C export timed out. The successful original retry supplied the selected music and producer contexts, with original image instructions and data resolving widths, aliases, comparison flags and branch order. The complete 0xAB5E4–0xAC3C8 selected instruction interval has 775 instructions and 3,557 matching bytes. The primary assembly analysis omitted 57 reachable jump-table case instructions in the selected 0xABDE2–0xABF81 region. Microsoft's COFF/PE disassembler was applied directly to that original image range, without executing it. Those case instructions and direct targets were reconciled with the image tables and the successful original retry. The joined interval has no unmatched instruction bytes or gaps. No assertion of complete semantic coverage of the rest of the 0xA5A50 owner is made.

Coincident offsets in other owners were not assigned a shared meaning. This is a selected resolved-direct caller and native-table trace. It does not close embedded World generation, noise/global initialization, all context-pair writers, all menu activation, alternate original input/worker routes, malformed ownership, imported clock/audio/thread behavior, asset availability, platform playback timing, or all process-global accesses. In particular the lock ordering here is the reached ordinary path. It gives no universal thread safety or guarantees for an arbitrary callback receiver.

Earlier held artifacts remained unread and unchanged. Exposure was limited to the standalone author policy and the selected original image and exports named here. No active AGENTS.md, WORKSPACE-MEMORY.md, combined policy/history, engine/exporter, tests/results, owner data/captures/configuration/outputs/assets, SDK/provider/dependency source, or reconstruction was read. The original executable was not run. No implementation or verification feedback was used.

### Original authority and selected export identity

| Relative original reference file | Bytes | SHA256 |
| --- | ---: | --- |
| binary/cubeworld.original.exe.unpacked.exe | 5757952 | 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244 |
| decompiled/assembly.txt | 62547291 | FE07326A0F4585432A9D4E42E7CF16C570815A8FCD164DE36616095023A0740C |
| decompiled/function_ranges.csv | 509127 | BC127412C5A8F21D8BA78AA53D9EB35A16AF58C1A85906DA9FF31E968E379601 |
| decompiled/calls.csv | 10840369 | CF410A7DD84BBEF0E9FC4FC6FD1FF3B4903D97E9126C8F1A1D69EF05CD566DCB |
| decompiled/functions.csv | 1551545 | 940E3682B9CCC68458531AC8D948A20F8B9530CDFFF5E568C5517BA91365DCEC |
| decompiled/functions/1400a5a50.c | 146 | 0E3C1FE01A9A2B7CED6455F7813108713A3CCF816AE66C9F976522EEC75C82BA |
| decompiled/retry-180-phase1/functions/1400a5a50.c | 145630 | 7D9C5B6A385DF1F8A1D7FEECA803469FDE939EB457BC3F62D2CF37B6DE32A814 |

Each complete selected owner in the next table was read using its individually verified relative native export decompiled/functions/140 followed by the six hexadecimal digits of its entry RVA and the extension .c. The native manifest uses inclusive RVAs from function_ranges.csv. All listed ranges were compared instruction by instruction with the raw PE-section bytes. Counts describe selected coverage, with repeat ranges across the two tables possible. A complete native selected-owner match does not close the independent callee graph.

### Complete selected native owner ranges

| Entry RVA | Inclusive instruction ranges | Instructions | Matched bytes |
| --- | --- | ---: | ---: |
| 0x2B9080 | 0x2B9080–0x2B918D | 79 | 270 |
| 0x9BEB0 | 0x9BEB0–0x9BED4 | 15 | 37 |
| 0x36160 | 0x36160–0x36206 | 60 | 167 |
| 0x36210 | 0x36210–0x36291 | 50 | 130 |
| 0x2B99D0 | 0x2B99D0–0x2BA145 | 533 | 1910 |
| 0x362A0 | 0x362A0–0x364D9 | 169 | 570 |
| 0xEAB0 | 0xEAB0–0xEB85 | 71 | 214 |
| 0xE9A0 | 0xE9A0–0xEAAE | 77 | 271 |
| 0x9AE80 | 0x9AE80–0x9AE8A | 4 | 11 |
| 0x2AA270 | 0x2AA270–0x2AA3F8 | 121 | 393 |
| 0x93870 | 0x93870–0x938A8 | 21 | 57 |
| 0x9BE50 | 0x9BE50–0x9BEA6 | 25 | 87 |
| 0x364E0 | 0x364E0–0x3661A | 62 | 315 |
| 0x9BEE0 | 0x9BEE0–0x9BF1B | 18 | 60 |
| 0x137E0 | 0x137E0–0x1382E | 32 | 79 |
| 0x69C60 | 0x69C60–0x69CD5 | 34 | 118 |
| 0x35EA0 | 0x35EA0–0x36156 | 146 | 695 |
| 0x5E3A0 | 0x5E3A0–0x5E3C9 | 15 | 42 |
| 0x9AF00 | 0x9AF00–0x9AF19 | 6 | 26 |
| 0x16EE0 | 0x16EE0–0x16F19 | 17 | 58 |
| 0x82BE0 | 0x82BE0–0x82BFF | 9 | 32 |
| 0x6DDF0 | 0x6DDF0–0x6DE02 | 6 | 19 |
| 0x358E0 | 0x358E0–0x35CDB | 267 | 1020 |
| 0x2D9F30 | 0x2D9F30–0x2DA08D | 114 | 350 |
| 0x2FF5C0 | 0x2FF5C0–0x2FF602 | 18 | 67 |
| 0x82BB0 | 0x82BB0–0x82BD2 | 12 | 35 |
| 0x6DD20 | 0x6DD20–0x6DD93 | 39 | 116 |
| 0x6DFA0 | 0x6DFA0–0x6E065 | 55 | 198 |
| 0x7DA80 | 0x7DA80–0x7DA96 | 5 | 23 |
| 0x5EEE0 | 0x5EEE0–0x5EEEC | 4 | 13 |
| 0x5EDA0 | 0x5EDA0–0x5EDAC | 4 | 13 |
| 0x282660 | 0x282660–0x2826BC | 32 | 93 |
| 0xA7A0 | 0xA7A0–0xA945 | 106 | 422 |
| 0x134420 | 0x134420–0x1347C5 | 192 | 934 |
| 0x5E430 | 0x5E430–0x5E64A | 160 | 539 |
| 0x328C0 | 0x328C0–0x3296B | 45 | 172 |
| 0x34270 | 0x34270–0x34397 | 75 | 296 |
| 0x34AA0 | 0x34AA0–0x34B05 | 28 | 102 |
| 0xEB90 | 0xEB90–0xEDA4 | 128 | 533 |
| 0x12700 | 0x12700–0x127EB | 64 | 236 |
| 0x83FB0 | 0x83FB0–0x84208, 0x8420A–0x8420F, 0x84211–0x84216, 0x84218–0x8421D, 0x8421F–0x84224, 0x84226–0x8422B, 0x8422D–0x84232, 0x84234–0x84239, 0x8423B–0x8425E, 0x84260–0x8426E, 0x84270–0x8427E, 0x84280–0x8428B, 0x8428D–0x842C2 | 199 | 775 |
| 0x89C10 | 0x89C10–0x89CAF | 36 | 160 |
| 0x2ABB70 | 0x2ABB70–0x2ABB7D | 2 | 14 |
| 0x2B9890 | 0x2B9890–0x2B989D | 2 | 14 |
| 0x2ADA20 | 0x2ADA20–0x2ADB74 | 103 | 341 |
| 0x2AD4D0 | 0x2AD4D0–0x2AD8EE | 282 | 1055 |
| 0x89020 | 0x89020–0x890B5 | 45 | 150 |
| 0x7D560 | 0x7D560–0x7D5B7 | 25 | 88 |
| 0x75D10 | 0x75D10–0x75E74 | 92 | 357 |
| 0x3C7D0 | 0x3C7D0–0x3C7E5 | 7 | 22 |
| 0x4F220 | 0x4F220–0x4F76C | 311 | 1357 |
| 0x75FF0 | 0x75FF0–0x76154 | 92 | 357 |
| 0x34910 | 0x34910–0x349C0 | 45 | 177 |
| 0x34F90 | 0x34F90–0x35020 | 34 | 145 |
| 0x35290 | 0x35290–0x352BA | 14 | 43 |

### Bounded native windows and independently bounded leaf entries

The application, Game constructor, Interface setup, World step and effect owner claims use these windows. The application and selected other successful C contexts were used where available. The two complete leaf/thunk bounds with no C confirmation come from original table references and native returns or tail transfer.

| Scope | Inclusive range | Instructions | Matched bytes |
| --- | --- | ---: | ---: |
| Main music path and preceding boss producer | 0xAB5E4–0xAC3C8 | 775 | 3557 |
| Game identity and binary64 delta | 0xA5AED–0xA5B12 | 8 | 38 |
| Current viewed World lock entry | 0xA6460–0xA6468 | 2 | 9 |
| Zero and one binary32 producers | 0xA689B–0xA689E | 1 | 4 |
| One binary32 producer | 0xA706D–0xA707E | 2 | 18 |
| Earlier selected success setter | 0xA7167–0xA722C | 74 | 198 |
| Earlier success setter admission and store | 0xA7444–0xA747B | 11 | 56 |
| Held delta reload | 0xAA53B–0xAA542 | 1 | 8 |
| Second success setter admission and store | 0xAACAC–0xAAD5E | 39 | 179 |
| Current threshold 0.5 producer | 0xAB3C4–0xAB3CC | 1 | 9 |
| Nominal gain scale producer | 0xAB598–0xAB5A0 | 1 | 9 |
| Nominal gain scale alternate producer | 0xAB57A–0xAB582 | 1 | 9 |
| First post-music caller window | 0xAC3C9–0xAC3FE | 11 | 54 |
| Render call join | 0xAD9C9–0xAD9D8 | 3 | 16 |
| Current viewed World lock release | 0xADB1F–0xADB27 | 2 | 9 |
| Context selected World-step producer | 0x2DB710–0x2DB827 | 78 | 280 |
| Game audio receiver storage | 0x7EABF–0x7EAFF | 13 | 65 |
| Game constructor zero register | 0x7EB4C–0x7EB4E | 1 | 3 |
| Game path, gain and success initialization | 0x7EF0E–0x7EF3F | 9 | 50 |
| Game audio critical section construction | 0x7F22A–0x7F23B | 3 | 18 |
| Game audio worker start | 0x7F94D–0x7F9A1 | 20 | 85 |
| Application audio construction receiver | 0x135DEA–0x135E05 | 6 | 28 |
| Application Game receiver handoff | 0x1362E9–0x136344 | 22 | 92 |
| Application ordinary frame call | 0x137B8E–0x137B97 | 2 | 10 |
| Complete selected audio status leaf | 0x343A0–0x343B1 | 6 | 18 |
| Other audio effect attenuation writer | 0xA05D9–0xA05F5 | 7 | 29 |
| Audio effect argument and owner binding | 0xA042A–0xA044B | 10 | 34 |
| Late packet call and camera boundary | 0xAAEB8–0xAAF06 | 20 | 79 |
| Interface update call and following local scan entry | 0xAB5BB–0xAB5E3 | 8 | 41 |
| Embedded Interface pointer retained for update | 0xAAF65–0xAAF72 | 2 | 14 |
| Three original Interface roots | 0xF9D25–0xF9F73 | 105 | 591 |
| Audio thread native table forwarder | 0x87BD0–0x87BD4 | 1 | 5 |
| Pair admission before World substeps | 0xA72CE–0xA7376 | 34 | 169 |

The selected raw data correspondence includes the 11 four-byte context dispatch entries at RVA 0xADC54, the 11 four-byte place targets at 0xADC80 and the 34 one-byte place entries at 0xADCAC. The context classifier jump table begins at 0x2B9190 and the two selected group-category dispatch tables begin at 0x2BA148 and 0x2BA194. The original eight-byte audio table slots 0x42EDB0, 0x42EDB8, 0x42EDC0, 0x42EDC8 and 0x42EDD0 resolve the selected audio methods. The eight-byte entry at 0x437EF8 resolves the selected audio-thread thunk. Import names for performance frequency/counter and critical section initialization/enter/leave were read from the original import name records referenced by the IAT. Their implementation remains outside this contract.

### Narrow title and path literals

Lengths exclude the one-byte terminator.

| RVA | Literal | One-byte units |
| --- | --- | ---: |
| 0x436750 | maintheme.ogg | 13 |
| 0x436760 | shrine.ogg | 10 |
| 0x436770 | greenlands.ogg | 14 |
| 0x436780 | deadlands.ogg | 13 |
| 0x436790 | cult of doom.ogg | 16 |
| 0x4367A8 | desert.ogg | 10 |
| 0x4367B8 | winter.ogg | 10 |
| 0x4367C8 | woodlands.ogg | 13 |
| 0x4367D8 | jungle.ogg | 10 |
| 0x4367E8 | savannah.ogg | 12 |
| 0x4367F8 | mountains.ogg | 13 |
| 0x436808 | beach.ogg | 9 |
| 0x436818 | ocean.ogg | 9 |
| 0x436828 | unholy pact.ogg | 15 |
| 0x436838 | village.ogg | 11 |
| 0x436848 | village2.ogg | 12 |
| 0x436858 | order of the light.ogg | 22 |
| 0x436870 | druids of mana.ogg | 18 |
| 0x436888 | enchanted-forest.ogg | 20 |
| 0x4368A0 | steel empire.ogg | 16 |
| 0x4368D0 | boss.ogg | 8 |
| 0x4368E0 | success.ogg | 11 |
| 0x4368EC | Music\ | 6 |

### Selected floating-point constants

Decimal values in this table display the original binary words. The binary word determines the precise value.

| RVA | Format | Original numeric word | Value |
| --- | --- | --- | ---: |
| 0x4AC980 | binary32 | 0x3F800000 | 1 |
| 0x4AC964 | binary32 | 0x3F000000 | 0.5 |
| 0x4AC95C | binary32 | 0x3E800000 | 0.25 |
| 0x4ACA4C | binary32 | 0xBF800000 | -1 |
| 0x42E598 | binary32 | 0x41200000 | 10 |
| 0x42E510 | binary32 | 0x37800000 | 0.0000152587890625 |
| 0x42E5F8 | binary32 | 0x47800000 | 65536 |
| 0x42E7AC | binary32 | 0x42000000 | 32 |
| 0x42E51C | binary32 | 0x3C23D70A | 0.009999999776482582 |
| 0x42E514 | binary32 | 0x3A83126F | 0.0010000000474974513 |
| 0x42E518 | binary32 | 0x3BA3D70A | 0.004999999888241291 |
| 0x437F68 | binary32 | 0x39D1B717 | 0.00039999998989515007 |
| 0x437F78 | binary32 | 0x3C75C28F | 0.014999999664723873 |
| 0x437FD8 | binary64 | 0x408F400000000000 | 1000 |
| 0x4ACA18 | binary64 | 0x4059000000000000 | 100 |
| 0x4AC9B8 | binary64 | 0x3FF0000000000000 | 1 |
| 0x42E540 | binary64 | 0x3F947AE147AE147B | 0.02 |
| 0x4AC978 | binary64 | 0x3F747AE147AE147B | 0.005 |
| 0x42EED0 | binary64 | 0x3F847AE147AE147B | 0.01 |
| 0x42EF08 | binary64 | 0x40D266C000000000 | 18843 |
| 0x42EEF8 | binary64 | 0x40C26C0000000000 | 9432 |
| 0x42EED8 | binary64 | 0x4072B00000000000 | 299 |
| 0x42EF10 | binary64 | 0x40F621A000000000 | 90650 |
| 0x4AC968 | binary64 | 0x3F33A92A30553261 | 0.0003 |
| 0x42EEC8 | binary64 | 0x3F43A92A30553261 | 0.0006 |
| 0x42EEF0 | binary64 | 0x40C0780000000000 | 8432 |
| 0x42EF00 | binary64 | 0x40C26F0000000000 | 9438 |
| 0x42EEE0 | binary64 | 0x4081B80000000000 | 567 |
| 0x42EEE8 | binary64 | 0x40931C0000000000 | 1223 |
