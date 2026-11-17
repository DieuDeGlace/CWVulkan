# Steam authority pet setup and CRT random stream

FINAL / STABLE / HELD  
2026-11-17

## Evidence and scope

Supported image is the x64 Steam cubeworld.original.exe.unpacked.exe, preferred base 0x140000000, SHA-256 7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244. All instruction addresses below are RVAs in that image. This contract covers 0x2E0DB0, its two calls to 0x2B67B0, and the random stream through the reached setup routines. It uses original binary bytes and matching original static exports. Numeric examples are static derivations. Live call occurrences, installed CRT identity, and unrelated work on either thread were outside the observation.

## Callers and thread roles

- The resolved entry into 0x2E0DB0 is the ordinary world tick 0x2DA6C0 at 0x2DA8FB. With world byte +0x358 clear, its preceding calls include 0x2E17D0, 0x2A56C0 and 0x2A45E0. It reaches the pet pass with either value of +0x358. The two statically identified setup calls are the existing actor call at 0x2E1067 and the fresh actor call at 0x2E12BD.
- The timed worker comes from app initialization 0x07EA80 at 0x07F879. The launcher 0x075D10 calls the C++ runtime thread-start import at 0x075DD0 with entry 0x03C7D0. For this launch, the table at 0x437EC8 resolves the indirect dispatch at 0x03C7D7 to the thunk 0x087BB0, then to 0x088EE0. The latter performs the launch handshake and calls the timed loop 0x083E60 at 0x088F52. The app stores its returned thread record at +0x2018.
- With app byte +0x2068 set and the loop's pause check clear, 0x083E60 calls 0x0F5520 at 0x083EC6 with the app's embedded object at +0x6E8 and elapsed milliseconds. That caller enters the critical section at its +0x5F8 at 0x0F5582. It passes its embedded world at +8 to 0x2DA6C0, using 16 ms steps at 0x0F6917 for time in excess of 16 ms and a final remainder at 0x0F692D. The elapsed input is capped at 1000 ms. Both calls run inside that critical section.
- The window message-pump thread has a separate route through 0x1355A0, 0x134420 at 0x137B8E, and 0x0A5A50 at 0x134558. That routine obtains its world via the app pointer at +8. Its native gates compare the app dword +0x206C to world dword +0x220 and the app string at +0x2070 with the world string at +0x200, using 0x06DD20 for equal byte strings. If those gates pass, the calls into 0x2DA6C0 are 0x0A7328 for 16 ms steps and 0x0A734C for the remainder. Both routes invoke the pet pass synchronously on their respective calling threads. Setup on this route still requires the pet pass's authority gates.
- The application passes a world pointer into its initializer for the frame route and separately initializes the worker's embedded object. The connection path writes 1 to the frame world's +0x358, and disconnect routine 0x0911E0 clears it. The zero test at the pet pass is the authoritative evidence for where setup can execute. A run supplies the actual world instance, mode, thread ID and branch taken.

## Native layout used by the pass

An actor address here is the full object base. The fresh allocation at 0x2E119E requests 0x15F8 bytes. The constructor 0x04CE80 installs a virtual-table pointer at the object base and a 64-bit actor ID at +0x08. The embedded actor data begins at +0x10. The same field is therefore 0x10 lower relative to the embedded data base. For example, race is actor +0x64 and embedded data +0x54. The pet helper receives the full actor.

| Base and offset | Width and role in this contract |
| --- | --- |
| World +0x40, +0x450 | List sentinel and actor-ID tree sentinel. The list node stores the actor pointer at +0x10. The tree uses signed 64-bit keys at node +0x20 and actor pointers at +0x28. |
| World +0x210, +0x358, +0x448 | Nonzero eight-byte gate for the first scan, one-byte authority gate with zero enabling setup, and the local actor pointer. The wider identity of the +0x210 member remains unresolved. |
| Actor +0x08, +0x10/+0x18/+0x20 | Eight-byte ID followed by three eight-byte position components. |
| Actor +0x60, +0x64, +0x68 | One-byte actor kind, 32-bit race, and one-byte action state. Kind 0 selects a player owner. Stable fresh creation from that owner writes kind 6. |
| Actor +0x78, +0x80 | Appearance base and its 32-bit flags at appearance +0x08. |
| Actor +0x144, +0x14C/+0x14D, +0x180, +0x190 | A 32-bit equipment level input, the one-byte class and specialization, current HP as a float, and a float checked for pet changes. |
| Actor +0x1A0, +0x9D8 | Eight-byte owner ID on the linked actor and eight-byte current pet ID on the owner. |
| Actor +0x278, +0x8B8, +0x970 | Equipment block, current pet item (kind byte at +0x8B8, 32-bit subtype at +0x8BC), and a byte used during equipment material selection. The pet item is also assigned after the original TAMED event at 0x0A0750. |
| Actor +0x1544, +0x1548, +0x154C | Cached owner float, cached pet item, and its cached 32-bit subtype. |

## Authority decisions and timing

The first scan of 0x2E0DB0 requires nonzero world +0x210. It processes actors in list order. A candidate owner must be kind 0 with item kind 0x14.

1. A nonzero pet ID with a nonnull exact tree hit is accepted for the first scan. For zero, a missing hit, or a hit with a null actor pointer, the pass clears the ID. It walks the same list and takes the first actor whose +0x1A0 equals the owner's ID, then writes that actor's ID back to the owner. This repair uses only the ownership match. With world +0x358 nonzero the eligible-owner branch ends after this link handling.
2. With +0x358 zero, the existing-actor branch requires a nonzero pet ID and a nonnull exact tree hit. A pet with HP at most zero clears the owner's list at +0xB98 through 0x00B810 and still proceeds through the existing-actor checks. Its HP by itself is insufficient to select a fresh actor.
3. The change gate at 0x2E0F69 compares owner floats +0x1544 and +0x190. Unequal or unordered floats select the change. Equal, ordered floats defer to item equality at 0x2E0F8A. The comparison at 0x04DF60 checks item-relative byte +0, dwords +4, +8, +0xC, +0x10 and +0x18, bytes +0x1C, +0x9D and +0x14, and as many four-byte groups starting at +0x1D as given by byte +0x9D. Zero groups skip the group comparison.
4. A change is recorded before the race comparison. The current item subtype at owner +0x8BC is compared with the cached subtype at +0x154C. A difference invokes the reset 0x064A90 at 0x2E0FAB. The change path writes the current subtype into pet +0x64, updates its name when eligible, emits the change message and writes the owner ID into pet +0x1A0. A second comparison at this point gates the default appearance initialization and copy (0x04D1F0 and 0x07E780), then the setup call at 0x2E1067. This is a comparison with the owner's cache. A race already present on the live actor can still be reseeded if the cache differs. A difference confined to other compared item fields or the float performs the change work without a setup call.
5. For every resolved existing pet, including the unchanged case, the owner effect 0x13 is copied if present on the owner and absent on the pet. The pass evaluates the pet's maximum-health routine 0x05FAC0, writes owner +0x144 to pet +0x144, evaluates the maximum again, and multiplies the current pet HP by the ratio of the later maximum to the earlier one. This level copy occurs after an existing pet's setup. Then the owner float and item cache are refreshed. The cache is also refreshed on the authority path when there is no resolved pet.
6. A fresh actor requires no change recorded for an existing pet, a zero owner pet ID after cache refresh, and owner action outside 0x78, 0x79, 0x98, 0x9C and 0x9D. The action gate is at 0x2E1134. A nonzero ID recovered from the list can therefore block fresh creation even if the subsequent exact tree lookup failed. The pass finds the signed minimum of the actor IDs and 0x80000000000003E8, decrements it by one, and stores it as the pet ID. It allocates and constructs the actor, appends it to the list and inserts it in the ID tree at 0x2E11C8, then applies reset 0x064A90. Normal successful creation continues with the owner ID, kind 6 for a current kind-0 owner (alternate native store is kind 1), copy of owner dword +0x1530, current item subtype as race, copied position, full HP from 0x05FAC0, and copied owner byte +0x970. It applies the name and inherits owner flag 0x200 if set, before setup at 0x2E12BD. The spawn message follows setup. A trailing, separate kind-1 owner check can mirror owner flag 0x8000 to the pet after setup.
7. Both name updates require an unsigned length at owner +0x955 below 16. The pass copies that many low bytes of successive four-byte entries at owner +0x8D8 to pet +0x958 and then writes a zero terminator. Length zero writes the terminator alone. These writes consume zero CRT draws.

For a first-scan actor outside the eligible owner/item pair, the pass looks up its stored pet ID. A found actor with effect kind 7 on its +0x980 list preserves the link during this scan. Otherwise the pass clears the ID and zeros the found actor's HP. This branch is outside the +0x358 authority zero test.

After the first scan, the pet pass raises local actor dword +0x8D0 to 1 if below 1. Independently of +0x210, a second scan runs with +0x358 zero. It considers owned actors with nonzero owner ID and flag 0x800 clear. A resolved kind-0 owner supplies both dwords +0x150/+0x154. Under a stable kind-0 owner value, retention then requires the owner's pet ID to match this actor, owner action different from 0x79, and either a false 0x05EE70 check or owner float +0xBB4 at most 3000.0. The 0x05EE70 check tests action 0x78, bit 0 clear at +0x5C, and both floats +0x130 and +0x12C at most zero. A second native load of owner kind can also enter retention if the value became nonzero. Retention propagates item metadata for the three equipment records at actor +0x638, +0x6D8 and +0x3B8 and rescales positive-max HP. The item checks use 0x109720 and include the kind, item seed divisibility and position conditions. Failure of retention resets the owned actor, sets HP zero, kind 3, float +0x6C to 5000.0, and clears the owner link. The second scan makes zero setup calls.

## The reseed and appearance work

At 0x2B67D1 the game seeds the imported CRT utility stream with the raw unsigned 32-bit value already installed at pet +0x64. This is the owner item subtype copied to the pet's race field. The helper's other inputs are the current world and the 64-bit owner ID. The seed is loaded from the pet, independently of those arguments.

The call at 0x2B67E5 passes the pet race address and appearance base to 0x053E80, with a null preset and a zero fourth argument. The call at 0x2B67ED then runs equipment wrapper 0x052C40. At 0x2B67F2 the helper resumes, writes the owner ID to pet +0x1A0 and dword 0x80 to pet +0xBCC, then writes the pet ID to the found owner's +0x9D8 using world +0x450. This completion work has zero CRT random calls.

The null preset consumes five draws at 0x053F21, 0x053F3B, 0x053F5A, 0x053F88 and 0x053FAE, in that order. It prepares three temporary color bytes from the low bytes of draws 3, 2 and 1 in ascending field order, followed by two temporary 32-bit variant indices from draws 4 and 5 reduced modulo 100. All five are consumed even if the race uses none of those values. This path keeps the installed race and selects the case through the image-relative table at 0x05DECC. Appearance alone supplies some races with additional draws. The following table is exhaustive for extra calls in this invocation. Destinations are relative to actor +0x78. A reduction is the remainder of the returned nonnegative CRT value.

| Race | Extra draw call sites in order | Draw use |
| --- | --- | --- |
| 0x06 | 0x0544D8, 0x0544F7, 0x054516, 0x054542 | Three new color bytes, stored in reverse draw order at +2 through +4, then +0x18 from 0xCAD plus remainder 3. |
| 0x2E | 0x0568E6, 0x056905, 0x056924 | Three new color bytes in reverse draw order at +2 through +4. |
| 0x3A | 0x05A9CB | Even selects the 0x183 to 0x186 variant group, odd the 0x17F to 0x182 group. |
| 0x53, 0x54 | 0x055AE5 or 0x055B0A | +0x18 from 0x118 or 0x11B, respectively, plus remainder 3. |
| 0x76 | 0x05563E | +0x20 from 0xB3A plus remainder 2. |
| 0x77 | 0x05C850, 0x05C87E, 0x05C8AC, 0x05C8DD, 0x05C90B, 0x05C93B, 0x05C9DA, 0x05CA21 | In order, +0x18 from 0xB30 plus remainder 5, +0x20 from 0xB24 plus remainder 9, both +0x1C and +0x1E from 0xB2D plus remainder 3, +0x22 from 0xB35 plus remainder 2, +0x28 from the draw times 0.1 divided by 32767 then increased by 0.4, one discarded draw, then two independently drawn floats at +0x80 and +0x8C each formed by dividing by 32767 and adding 3.0. |
| 0x7C, 0x8F | 0x05B3C8 or 0x05B940 | +0x20 from 0xAE9 less remainder 2, or 0xB10 plus remainder 4, respectively. |
| 0x91 or 0x92 | 0x056CB7 | A uniform component scale from the draw times 0.5 divided by 32767, then increased by 1.2 and multiplied by 0.8. |
| 0x9F, 0xA1 | 0x05C2A7 or 0x05C691 | The respective size-vector multiplier is the draw divided by 32767 plus 6.0, or the draw times 0.5 divided by 32767 plus 4.0. |
| 0xCA, 0xCB | 0x055BC3 or 0x055BEA, then 0x055C14 | +0x18 from 0x69 or 0x72 plus remainder 9, then +0x20 from 0x7B plus remainder 9. |
| 0xCC, 0xCD | 0x05D691 or 0x05D6F0 | +0x20 from 0xA4F or 0xA54 plus remainder 5. |
| 0xCE, 0xCF | 0x05D74F or 0x05D78F | 0xCE discards its draw and writes 0xA59 at +0x20. 0xCF writes 0xA5A plus remainder 2 there. |
| 0xD0, 0xD4 | 0x0551FA or 0x05D2AA | 0xD0 sets +0x18 to 0x27 plus remainder 2. 0xD4 sets +0x20 to 0xDD7 plus remainder 2. |
| 0xE3, 0xE4 | 0x055B4C or 0x055B64 | +0x18 from 0x122 or 0x127 plus remainder 5. |
| 0xE7, 0xE9, 0x121 | 0x05B49C, 0x05C022 or 0x05B500, respectively | +0x20 from 0xAEF, 0xAF8 or 0xAF4, plus remainder 5, 6 or 4, respectively. |

The equipment gates read post-appearance flags. In this invocation the appearance adds the following bits among 0x08, 0x10, 0x20 and 0x200. Unlisted races, including the default case, add zero bits in this mask. Values below are hexadecimal and ranges are inclusive. With ordinary fresh setup the starting bits in this mask are zero or inherited 0x200. With race-changing reconfiguration the default appearance copy resets them to zero. Appearance preserves the inherited 0x200. Its flag-controlled scale adjustments have zero draws.

| Bits added | Race IDs |
| --- | --- |
| 0x10 | 56 |
| 0x20 | 18, 2B, 2D–2E, 30–31, 33–34, 44, 4C–54, 5F, 6C–6E, 72–73, 8C–8E, 94–95, A5, A7, AF–B4, BF, C7, CA–CB, D0, D8, E3–E4, E6, E8–E9, EE–F1, FC–FD, FF–100, 113, 124, 128, 12A–12B, 130–131 |
| 0x28 | 11, 25–2A, 2F, 32, 3F–43, 45–46, 55, 57–5E, 60, 68–6B, 6F–71, 75–76, 9C, A0, A2, AC, C0–C1, D2, E2, 106–107, 10D, 114 |
| 0x30 | 13–17, 19–24, 35–3E, 47–4B, 62–67, 74, 77, 91–93, 96–9B, 9D–9F, A1, A3–A4, A6, A8–AB, AD–AE, C8–C9, D1, D3, D5, D7, D9–DC, E0–E1, E5, 10E, 115, 11F–120, 122–123, 125–127, 12F |

## Equipment draws

The wrapper at 0x052C40 calls 0x0511E0 at 0x052CC6. It supplies the pet's kind, race, current level, flags after appearance, byte +0x970 and addresses of its class, specialization, equipment and auxiliary containers. A fresh actor's class and specialization start at zero. The existing actor reset and default appearance replacement preserve its class and specialization, so a race-changing existing actor can use different equipment draw gates from a fresh actor of the same race. The equipment generator starts by resetting its equipment records using 0x04D300 and 0x03B6B0. Equipment offsets in the next two paragraphs refer to the equipment block at actor +0x278.

**Before the main selection**, race class 3 from 0x065750 takes one draw at 0x0512A8. If the remainder modulo 4 is 2 or 3, the draw adds quantity 1 or 2 of item kind 1, subtype 1 to inventory at actor +0x9F0. The inventory insertion adds zero draws. The class-3 races in hexadecimal are 0–10, 12, 2B–2E, 30–31, 33–34, 36, 4C–51, 53–54, 5F, 94–95, A7, BF, C7, CA–CB, D0, D8, E3–E4, E6, EC–ED, FC–FE, 100, 103–105, 128 and 130–131. This race classification is independent of the actor's own class byte.

**Main selection** follows this priority. Each bypass goes to the shared finalization.

1. Actor class 0x8A takes one draw at 0x052244 for chest subtype 2 plus remainder 5. Classes 0x80–0x87, 0x8B and 0x9B bypass with zero draws. Race 0x12 with class zero and races 0xFC, 0xFD, 0xFE, 0xFF, 0x100, 0x103 and 0x106 also bypass with zero draws.
2. Actor class 0x90 bypasses with zero draws. Class 0x8F bypasses and takes one draw at 0x0514FE only when bit 0x20 is clear, for the provisional seed at equipment +0x468. Class 0x88 bypasses with zero draws.
3. After those class and race checks, kind 7 or 8, either bit in 0x18 set, or race 0x76 bypasses with zero draws. Races 0x75, 0x6C, 0x72, 0x6E, 0x2E, 0x73, 0x6D, 0x51, 0x52, 0x2F and 0x6F–0x71 also bypass with zero draws at this stage. Races 0x2B, 0x2D, 0xA7, 0xC7 and 0xE6 instead use one draw at 0x0521D5 for the weapon subtype at +0x464, giving 10 plus remainder 2, then bypass the generic selection.
4. In the remaining cases a zero actor class takes the 0x051885 draw to produce class 1 plus remainder 4. Race 0x61 then forces class 4 and race 0xA5 forces class 1, so the earlier draw still occurs if the initial class was zero. The resulting class governs weapon draws.

| Resulting class | Weapon draw order and results |
| --- | --- |
| 1 or 0x8D | Class 1 first draws specialization modulo 2. Both draw a selector modulo 3. Selector 0 takes one draw modulo 3 for +0x464 with a base of 15 and leaves the +0x3C0 weapon record empty. Selector 1 takes two draws modulo 3 for +0x464 then +0x3C4. Selector 2 takes one draw modulo 3 for +0x464 and sets +0x3C4 to 13. |
| 2 | Draw specialization modulo 2, then +0x464 as 6 plus remainder 3. |
| 3 | Draw specialization modulo 2, then one selector modulo 3. Results 0 and 1 give +0x464 values 10 and 11 and clear the second weapon kind/subtype. Result 2 gives both subtypes 12. |
| 4 | Draw specialization modulo 2, then +0x464 as 3 plus remainder 3. When the resulting subtype is 5 the +0x3C0 weapon is cleared. Otherwise both weapons use the result. |
| 0x8E | Draw a selector modulo 4. Result 0 takes one draw for +0x464 as 15 plus remainder 3. Result 1 takes two successive parity draws for +0x464 and +0x3C4, with even producing 1 and odd 0. Results 2 and 3 give +0x464 values 5 and 10 with zero extra draws. |
| Other | Zero weapon draws. |

If the generic selection reaches the armor stage with bit 0x20 clear, four draws choose materials at relative offsets +0x15C, +0x33C, +0x29C, +0x1FC in that order, with each draw reduced modulo pool length. For the weapon subtype at +0x464, pools are listed in selection order below. Class 0x8D or 0x8E replaces the selected pool with the singleton 11 or 5, respectively. A singleton still consumes all four draws. Three subsequent draws choose materials for +0xBC, +0x5BC and +0x51C, in that order, assigning 12 for even or 11 for odd. The seven draw sites are 0x05201F, 0x05203E, 0x05205D, 0x05207C, 0x05209B, 0x0520C3 and 0x0520EB. With bit 0x20 set the armor stage is skipped.

| Weapon subtype range | Actor +0x970 equals 6 | Other +0x970 |
| --- | --- | --- |
| 10–12 | 35 | 26, 32, 35 |
| 6–8 | 33 | 27, 31, 33 |
| 3–5 | 34 | 28, 30, 34 |
| Other | 4 | 1, 29, 4 |

**Shared finalization** applies flag-derived equipment changes and the race-specific writes before its optional draws. Bit 0x08 writes kind 3 and subtype 0x16 for both weapon entries. Bit 0x10 writes kind 3 and subtype 0x17 for the +0x460 weapon and clears the +0x3C0 entry. Bit 0x20 writes chest kind 4 and subtype 1. The race-specific writes in this stage take zero draws. For bit 0x200, 0x052709 draws an index modulo 9 and appends the selected ID to the actor +0xBD0 vector. Its ordered pool is 0x65, 0x67, 0x5F, 0x62, 0x60, 0x37, 0x73, 0x71, 0x2F. If 0x65 was appended, 0x05272C draws once more modulo 3 and stores the residue at actor +0xBE8, then appends 0x11 and 0x60 to the actor +0xBF0 vector. Both optional draws precede the final item seed passes.

1. For kind 6, the first seed pass uses no CRT draws. In order, the inputs are race plus 100 times an index from 0 to 8, with 32-bit wrap. Each input undergoes successive exclusive-ORs with itself shifted left 13, logically right 17, and left 5, keeping 32 bits at each step. The resulting dwords go to actor +0x640, +0x6E0, +0x5A0, +0x3C0, +0x500, +0x460, +0x320, +0x780 and +0x820. An existing actor of another kind uses nine CRT draws at 0x0528CF through 0x05292F for this first pass in the same field order.
2. The low byte of the level captured by the wrapper is written to each of those item records at 0xC beyond the seed dword. The generator then takes nine unconditional CRT draws, including for empty item entries. This last pass overwrites all nine seed dwords. In order, the draw sites are 0x052975, 0x052981, 0x05298D, 0x052999, 0x0529A5, 0x0529B1, 0x0529BD, 0x0529C9 and 0x0529D5. The corresponding absolute actor offsets are +0x6E0, +0x640, +0x5A0, +0x3C0, +0x500, +0x460, +0x320, +0x780 and +0x820. The two weapon offsets are reversed compared with the first pass.

## Full sequences for two stock races

The original type-0x15 food producer 0x2ACDD0 includes both 0x13 and 0x1A. The original name table instantiated at 0x1510F0 associates them with Collie and Terrier at the entries around 0x153263 and 0x153464. The original taming branch requires the food subtype to match the target's race. The following are successful kind-6 fresh setups from a kind-0 owner with eligible action, a valid pet item, no current pet and the declared flag state. CRT draw values are decimal. Use the normal Microsoft CRT provider convention for these vectors. Start with the unsigned seed, advance by multiplying by 214013 and adding 2531011 modulo 2 to the power 32, and report bits 16 through 30 of each advanced state. This recurrence describes the expected CRT provider. The game image establishes the import calls.

| Case | Appearance draws | Optional equipment draws | Final nine equipment draws | State on helper return and next draw |
| --- | --- | --- | --- | --- |
| Collie, seed 0x13, owner bit 0x200 clear | 100, 15331, 209, 888, 8224 | None | 20869, 1966, 9435, 29258, 12103, 3043, 28937, 28565, 19749 | After draw 14, state 0x4D258EE1. Next return 16204. |
| Terrier, seed 0x1A, owner bit 0x200 set | 123, 25034, 26954, 5491, 14890 | 15588, 23753 | 21712, 13767, 1834, 2008, 29246, 19697, 4501, 7709, 7959 | After draw 16, state 0x1F179F2A. Next return 18084. |

- For Collie the prepared color bytes are 209, 227, 100 and the prepared variant indices are 88, 24. The appearance retains the default color bytes of 255 and has flags 0x31. Its consecutive model words at appearance +0x18 through +0x22 are 0x1F7, 0xFFFF, 0x1F8, 0x1F9, 0x1F6, 0x1FA. Bits 0x10 and 0x20 cause the class/weapon and armor selection draws to be bypassed. Finalization gives the +0x460 weapon subtype 0x17 and chest subtype 1.
- For Terrier the prepared color bytes are 74, 202, 123 and the prepared variant indices are 91, 90. The appearance retains the default color bytes of 255 and has flags 0x231. Its model words in the same order are 0x21C, 0xFFFF, 0x21D, 0x21E, 0x21B, 0x21F. The same weapon and chest subtype writes apply. Draw 6 selects pool index zero and appends 0x65. Draw 7 writes 2 to actor +0xBE8 and appends 0x11 and 0x60 to the second vector. If this race is reached through the race-changing reset, the default appearance copy gives initial flag 0x200 clear and those two draws are skipped.

## Stream continuation and observation points

- The seed import is at image slot 0x42CD38 and the draw import at 0x42CD28, both under API-MS-WIN-CRT-UTILITY-L1-1-0.DLL. The calls use the current thread's CRT state. The helper has no saved state or restoration step. At 0x2B67F2 the stream is already at the state after that invocation's final item seed draw. A subsequent setup on the same thread replaces it with its own seed. The worker and window thread's states are distinct even when both enter an equivalent world tick.
- The normal native list handling, resetting, registration, name copy, HP synchronization, effect propagation and second scan in this scope have zero resolved CRT random calls. The print path at 0x007080 has stream-buffer calls at 0x0070FB and 0x00722B whose ultimate virtual targets remain unresolved. The imported stream operators also run outside the image's resolved game call graph. The exact state at helper return precedes these later spawn-message calls. Existing-actor change logging precedes reseeding. With no setup taken, the pass itself has no known seed or draw side effect.
- The same tick continues on the same caller, including actor work at 0x2DB69C, 0x2DB6BA and 0x2DB6C9. The respective targets 0x2BB8E0, 0x2C3710 and 0x2DE4C0 contain conditional draws, with example sites 0x2BDF19, 0x2C39AE and 0x2DEBA7. Their reachability and any intervening consumption depend on the tick's other work. If any later operation reaches the same draw import on this thread before another stream change, it starts from the state left by the pet work.
- In a real run, identify the thread at 0x083EC6 or 0x134558 and the world at 0x2DA8FB. For the existing case pair 0x2E0F8A and 0x2E1067 with current/cache subtype, pet ID, class, kind and flags. For the fresh case pair 0x2E1134 and 0x2E12BD with the owner action, both actor IDs and the inherited flag. At 0x2B67D1 record the unsigned seed argument and thread ID, then use the appearance sites, 0x052709 and 0x05272C if reached, and the nine final seed sites to account for draw returns. The first post-equipment site 0x2B67F2 isolates helper completion before either caller's later work. A proof of the next consumer can be obtained by observing the next reached existing import call on the recorded thread.

Source exposure was limited to the source-author policy, the isolated assignment, the supported original Steam image, and the matching original static exports for these traces. The game was never launched. No instrumentation was written.
