# Continue and saved-world-entry publication in the supported Steam image

This independent original-source contract describes the selected frontend release, stored-character selection, Start Menu callback and saved-world selection routes. Their concrete admissions and publication orders differ. The complete owner at 0x9FDF0 is reached by the system menu's Start Menu selection; the title menu's Continue selection is handled in the frontend prefix of 0x965E0. The separate stored-character Select callback is 0x9F7C0. The stored-world Select binding resolves to the native leaf at 0x9FD10, whose admitted existing-record route transfers to 0x8A500. Those identities follow original literals, native setup operands, callback tables and caller bodies, rather than a name guessed for an address.

The scope is normal returning execution with live receivers and valid complete records, with partial-read, failure, alternate-caller and concurrency limits stated separately. The entire selected 0x965E0 owner was read and byte-matched, but its behavioral scope here ends after the frontend/page prefix and the immediately following GUI release dispatch. Its later commerce, map and Controls postlude is outside this contract. Large Interface setup and ordinary Game update bodies are used only through the exact bounded native windows identified in the manifest. Their other behavior is not implied to be closed.

The author read the standalone CLEANROOM_AUTHOR.md policy and selected supported-original executable/export evidence only. No prior held artifact was read or changed. No active AGENTS, project memory or combined history, replacement implementation, exporter, tests, results, owner output or capture, installation configuration, assets, SDK, provider/dependency implementation or reconstruction source was inspected. No executable was run, and no implementation or runtime feedback was requested or received. This handoff is code-free behavioral prose and factual tables; it contains no original code, pseudocode, disassembly, instruction byte sequences or replacement guidance.

## Supported original and evidence method

The supported file is C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe: 5,757,952 bytes, SHA256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. It is AMD64, machine 0x8664, preferred base 0x140000000, PE timestamp 0x5D93AB0D, entry RVA 0x3939F0, SizeOfImage 0x58A000. All addresses below are RVAs, not raw-file offsets.

Reference filenames were discovered before scoped original reads. Original C exports under decompiled/functions were reconciled with original assembly.txt, function_ranges.csv, calls.csv and functions.csv. Each selected native instruction row was compared with the bytes mapped from the supported PE image; all reported owned ranges are completely covered, excluding identified padding between disjoint owned ranges. Direct relative call targets were decoded from their matched native operands. Original literals and selected virtual/callback table entries were read directly from the image. Raw leaves lacking a decoded C owner are identified separately. The one callback invoker absent from the assembly rows was independently decoded over its complete 15-byte original-image leaf from the table-resolved entry through its indirect tail transfer; the following byte is padding. No neighboring routine was substituted for that leaf.

## Concrete receivers and selected GUI integer

G is the Game receiver retained by each selected Game method. I is the embedded Interface object at address G+0x118. The eight-byte field at G+0x118 is I's first node pointer, not a separately allocated Interface object. The complete Game constructor 0x7EA80 calls 0xF8690 with receiver I and owner argument G. The complete 0xF8690 stores that owner at I+0x5C0. Interface setup 0xF9AC0 receives I; its selected registration windows reload this same owner field for their callback context.

W denotes the eight-byte pointer currently read from G+8. E denotes the eight-byte local-entity pointer currently read from W+0x448. D denotes the eight-byte subordinate pointer currently read from E+0x1528. These aliases are current operands, not a frozen tuple across calls. The explicit loader 0x9BB20 retains its entity argument independently; that target remains its target even when its lock/unlock code later reloads G+8. The decoder has that retained entity receiver and does not obtain a replacement target from a process Game global partway through reading.

For a node, the selected GUI integer used here is the four-byte element in the array whose eight-byte address is at node-property receiver+0xE0, selected by its signed 32-bit index at +0xAC; the property receiver is the node's eight-byte +0x68 field. The index is sign-extended before four-byte indexing. The complete getter 0x9AF00 returns whether that selected integer is nonzero. The complete setter 0xA3A20 stores four-byte 0 or 1 after comparing its supplied byte to zero. The release and traversal bodies directly use the same selected integer as an admission. This establishes the selected native gating behavior without assigning other indexed properties an inferred visibility meaning.

| Game field | Interface-relative field | Selected concrete object |
| --- | --- | --- |
| G+0x118 | I+0 | Frontend root node; setup initially stores selected integer 1 |
| G+0x120 | I+8 | Title child node created under the frontend root |
| G+0x128 | I+0x10 | Title widget constructed by 0x2910C0 with the title child node |
| G+0x130 | I+0x18 | Character-selection page node; setup stores selected integer 0 |
| G+0x170 | I+0x58 | New-character page node; setup stores selected integer 0 |
| G+0x198 | I+0x80 | World-selection page node; setup stores selected integer 0 |
| G+0x200 | I+0xE8 | Additional frontend page node selected by the world-selection leaf's nonexisting-record branch; setup stores selected integer 0 |
| G+0x1E0 | I+0xC8 | Select control created under the world-selection page |
| G+0x358 | I+0x240 | System-menu widget constructed by 0x291C70 under a node produced with parent I+0x108; initially gated off through its node |
| G+0x650 | I+0x538 | Selected widget whose node is enabled by title selection 1 |
| G+0x6C0 | I+0x5A8 | Signed 32-bit character-choice value consumed by 0x9F7C0 |

The selected setup windows prove the page publication and the two Select bindings; they do not close the generic node factories, imported allocation behavior, asset geometry, every page property, or the full layout/teardown lifecycle. In particular, setup's title-child creation does not by itself establish every provider default. Later admissions test the currently stored selected integers.

## Ordinary window and GUI caller joins

The complete window owner at 0x133C70 handles message numbers 0x202, 0x205 and 0x208 by calling the imported ReleaseCapture boundary first, then loading the current eight-byte Game pointer from process RVA 0x551A80 and calling 0x965E0 with 32-bit button values 0, 1 and 2 respectively. This is an order of native calls; no OS capture, message-pump, reentrancy or lifetime guarantee is inferred from an import's name. The Game pointer is loaded after the import returns. The release method receives no new coordinate argument on these joins.

The complete Interface adapter 0x101120 receives I and the 32-bit button value. It reloads the current Game owner at I+0x5C0 and then its manager at G+0x20. Button 0 calls manager release owner 0x3410E0; button 1 calls 0x341780; button 2 calls 0x3414A0; other values make no selected manager release call. The frontend Game prefix described next is executed before this adapter on every normal 0x965E0 route, including routes whose frontend admission fails.

In the complete button-0 manager owner, the manager's 32-bit state word at +0x1A8 first has bit 0 cleared. A current capture node at +0x168 is preferred; absent capture, 0x333F60 is called with current root +0x140, the address of cached two-component coordinates at +0x180, and a zero flag. This query's geometry and recursive provider internals are outside this audit. If both manager +0x178 and +0x1B0 are nonnull, 0x342270 is called with two zero arguments before selected-node dispatch.

For a selected node, its current widget at node+0x78 receives virtual slot +0x60 when nonnull and its one-byte +0x1A0 gate is nonzero. After that callback returns, the owner rereads the node's 32-bit flags and current widget. With flags bit 1 clear, a nonnull widget and a nonnull event-3 record returned by 0x32A210, that record is invoked if the widget's current +0x1A0 byte is nonzero or the callback record's byte +8 is zero. The setup registrations selected here store byte +8 equal to 1, so their event-3 invocation requires the current widget gate. The callback lookup itself can insert an absent integer-keyed map entry; a missing entry is not assumed to be a read-only lookup.

After the event-3 stage, the manager rereads capture or performs a new hit query, performs another hit query for the current pointer target, and compares the current widgets. If they differ, the former widget can receive virtual +0xB8 and event 14 under the same flag/record gate pattern. It then traverses the current root and children: the selected integer must be nonzero and 32-bit flags bits 2 and 1 must both be clear. A node's widget at +0x70 is passed through the selected traversal callback before children in current list order. The complete 0x33C360 preserves those admissions recursively. The selected button-0 traversal adapter resolves to virtual slot +0xF0. The corresponding button-1 and button-2 owners clear state bits 1 and 2, invoke virtual +0x78/event 6 or virtual +0x90/event 9, and traverse through virtual +0x108 or +0x120 respectively. Concrete targets of arbitrary widgets at those slots remain outside this bounded closure.

The ordinary Game callback object created by complete helpers 0xF8410 and 0xF8380 is a 0x20-byte allocation. It stores the one-byte gate at +8, captured context at +0x10, target pointer at +0x18, and table 0x441BA8 at its beginning. A previous record is freed before replacement; failed allocation stores null. The table resolves to the complete raw invoker at 0xF99A0, which passes the captured context as receiver and transfers to the stored target. Interface setup binds event integer 3 to 0x9F7C0 for the character Select control and to 0x9FD10 for the world Select control, using G from I+0x5C0 as context. The two supplied registration bytes are both 1. The registration helper also applies the selected binding recursively when its supplied propagation byte is nonzero; descendant collection and allocation helpers remain boundaries.

The GUI phase therefore uses current node/widget/capture/record loads after earlier callbacks. A frontend publication can change the subsequent GUI admissions within the same release. There is no proof that one release invokes exactly one application callback, that capture and current hit are identical, or that the Game postlude sees the pre-callback state.

## Title Continue and the frontend prefix of 0x965E0

The complete title constructor 0x2910C0 stores G at widget+0x1E8, initializes the signed 32-bit choice at +0x1F0 to -1, and installs primary table 0x470630, whose +8 entry is 0x291210. The complete draw owner 0x291210 resets that choice to -1 every time it runs. It writes choice 0 for the selected Start Game region, 1 for Options, 2 for Exit, and 3 for Continue. The Continue row is considered only when current signed G+0x2264 is nonnegative and below the low signed 32-bit slot-vector count derived from G+0x1778 and +0x1780. The label Continue is the image-backed eight-code-unit UTF-16 literal at 0x4707B8. The pointer/size queries determining a selected row are the selected 0x32B180/0x32A1D0 provider boundaries; this contract does not invent their general geometry.

The release prefix tests, in order, the current frontend-root selected integer, the current title-child selected integer, and button value 0. If either integer is zero or the button differs from zero, it skips all title-choice actions and continues to GUI release. It then reads the title widget's signed 32-bit +0x1F0 once. Negative choices skip the actions. Values other than 0, 1, 2 and 3 also skip them. The selected action order is:

| Captured title choice | Direct native prefix work before GUI release |
| ---: | --- |
| 0 | Store selected integer 0 on the frontend root, then selected integer 1 on the character-selection page |
| 1 | Store selected integer 1 on the current node of widget G+0x650, through widget+0x178 |
| 2 | Store byte 1 at G+0x1D68 |
| 3 | Call 0x9BB20 with G, current four-byte G+0x2264 and current W+0x448; reload and disable the frontend root; store binary32 8 at G+0x1DB8 and then G+0x1DB4; reload W and call 0x2E0B50 |

The Continue load call is at 0x966B4 and the following 0x2E0B50 call is at 0x966EE. Continue does not perform the title draw's slot-range admission again. A stale or externally changed choice 3 reaches the load with the current slot value. It does not call 0x9F7C0, persist the slot key, call the class-dependent Interface rebuild 0x107690 or Game rebuild 0xB2180, set the six selected double camera fields, invoke the saved-world selection leaf, or directly call shared peer connect. These are bounded absences of calls/stores in this prefix, not promises about all transitive providers or later frames.

The loader returns no success value to this prefix. After a normally returning backend miss, the prefix still disables the frontend root, resets the two binary32 camera-distance operands and calls 0x2E0B50 using the then-current W. It does not roll the GUI publication back based on a successful decode. After every normal title path, admitted or skipped, call 0x96767 dispatches 0x101120 on I with the retained button. No early return separates Continue publication from that GUI phase. Subsequent 0x965E0 postlude behavior is outside the interpreted scope here, despite complete-byte verification of its owner.

## Explicit character load 0x9BB20 and its failure boundary

The complete 0x9BB20 receives G, a signed 32-bit slot input, and an explicit eight-byte entity pointer T. It first calls 0x2ABB70 on current G+8. The complete guard wrapper adds 0x3C8 to that World receiver and transfers to the imported EnterCriticalSection boundary. The slot input is passed to an imported integer-stream conversion and its resulting byte-string key is obtained through 0xA5870. The exact locale/runtime implementation of that conversion is not inspected here.

The loader then enters the critical section at G+0x1C98 and calls 0x670F0 with backend receiver address G+0x2268, the generated key and a local byte-buffer output. The buffer begins with null pointers and zero cursor/scalar fields. The backend's one-byte result is retained across normal key cleanup. A zero result skips buffer preparation, decoding and the post-read record insertion; there is no explicit entity reset on that skipped branch. The method still leaves G+0x1C98, reloads current G+8 for 0x2B9890, and cleans up local buffer/stream state before returning. No success result is returned to Continue, stored-character Select, or the preview reload loop.

The complete backend wrapper 0x670F0 uses the original narrow query SELECT value FROM blobs WHERE key = ?. It prepares a statement through an unclosed backend provider, returning zero if that status is nonzero. It binds the current string key and requests a step. Numeric status 100 admits the returned blob; another status invokes the selected cleanup/status boundary, and only result 17 repeats the preparation loop. Other results return zero. On the admitted branch it obtains a signed 32-bit blob length, allocates/copies through provider-returned storage, sizes and fills the caller's buffer, then returns low-byte 1. These native statuses, calls and copies do not certify a well-formed character record or backend durability. The linked backend/provider internals, resource ownership on all failing provider paths, invalid lengths and allocation failure remain unclosed.

On a nonzero backend result, 0x9BB20 calls selected buffer-preparation owner 0x66D10, constructs a reader context through 0x147750 and calls 0x147800 at 0x9BCDA with that context and retained entity T. The native second argument at this call is the retained entity pointer; its omission from one C call presentation is not interpreted as a different decoder target. The buffer-preparation owner writes its four-byte cursor at +0x18 to zero and uses 0x66DC0/0x66A30 with temporary storage before cleanup. Its transitive format transformation is not assigned an invented compression or validation contract.

After the decoder normally returns, the loader prepares a candidate 0xA0-byte item-like record by its selected defined-field stores: leading byte 1, four-byte field +4 equal to 7, and zero values for the other compared defined fields, including its byte +0x9D. It does not prove that all padding in that temporary was cleared. Complete helper 0x5F440 returns zero for null T+0x1528, otherwise scans that D's +0x18 linked collection and tests each record with complete comparator 0x4DF60. That comparator checks selected byte and four-byte fields and the additional fixed-size entries admitted by byte +0x9D, rather than comparing every padding byte. When no equal defined-field record is found, the loader constructs a node through 0x6E600 and appends it at the current retained D+0x18 list's tail, after the native 64-bit count limit check, then increments D+0x20. The list-construction allocator is a boundary; null D does not make the subsequent insertion branch safe by implication.

This post-read insertion occurs under both selected caller protection scopes, after decoding and before the loader unlocks. It is not a second entity reset, a health normalization or an inventory-slot replacement. Backend lookup failure skips it. Successful blob retrieval admits it even when the decoder's internal field reads were incomplete, because the caller has no whole-record success test.

## Ordered decoder and reset/default boundaries

The complete reader-context initializer 0x147750 stores the supplied buffer pointer, eight-byte constants 11 and 44 at context+8 and +0x10, and four-byte zero at +0x18. The decoder then reads its four-byte record-version value into context+0xC. The individual native reads use the buffer's four-byte cursor and signed 32-bit size/address-difference operands. When an individual extent test fails, it places the cursor at the available extent and continues through subsequent code. It does not return a whole-record success flag or restore an earlier entity snapshot.

The following chronology describes the normal complete-record path of the entire 0x147800 owner. It is deliberately scoped to its actual stores and selected call boundaries. A helper's passed addresses are not evidence that its entire transitive implementation has been closed.

| Order | Reached entity/subordinate work |
| ---: | --- |
| 1 | Call complete entity reset 0x64A90 on retained T |
| 2 | If current T+0x1528 is nonnull, call complete CharacterData destruction owner 0x4DB70 and free that allocation; allocate 0x300 bytes, call complete 0x7FF20 when nonnull, and publish its result at T+0x1528 |
| 3 | Prepare an equipment temporary, call 0x4D300 and 0x3B6B0 on T+0x278; exchange the T+0x9F0 nested-container aggregate through 0x83540 with an initially empty temporary and perform the returned-storage cleanup; size that aggregate to six through 0x64F30 |
| 4 | Construct a temporary default core through complete 0x4D600, assign its defined destinations to T+0x10 through complete 0x3B7A0 at 0x147984, then store byte zero at T+0x60 |
| 5 | Read record version, 24 position bytes into T+0x10, 12 orientation bytes into T+0x28, then unconditionally overwrite those 12 orientation bytes with zero |
| 6 | Read four-byte fields in order at T+0x180, +0x144, +0x148, then one-byte fields +0x14C and +0x14D, then four-byte +0x9A4 and +0x9A8 |
| 7 | Call complete 0x1496D0 on eleven consecutive 0xA0-byte equipment records beginning at T+0x278 and ending at T+0x8B8 |
| 8 | Read a signed four-byte name length, copy that many bytes to T+0x958 when admitted by the individual extent test, and write a trailing zero byte at that name-relative length |
| 9 | Destroy/free the first newly published CharacterData allocation, allocate and construct another 0x300-byte CharacterData, publish it at T+0x1528, and read its initial 20 bytes as five four-byte fields |
| 10 | Call the unclosed appearance/rebuild boundary 0x53E80 with addresses T+0x64, T+0x78, the current D and a zero byte argument |
| 11 | Read the nested aggregate count and per-group record counts, size T+0x9F0 through the selected vector helpers, and read each admitted group's count multiplied by stride 0xA4 into its current storage |
| 12 | Read four-byte T+0xAAC and +0xAB0, then apply the selected +0xAAC comparison/store described below; populate the D+0x18 record list from admitted 0xA0-byte records in input order |
| 13 | For a signed record version greater than 9, read count-prefixed records into D+0x38, +0x48, +0x58, +0x88, +0x98, +0xA8, +0xB8, +0xC8, +0xD8 and +0xE8 through their selected insertion helpers, in that order |
| 14 | For a signed version below 4, consume the reached obsolete count/string/24-byte records and discard those temporaries; no new gameplay meaning is assigned to them |
| 15 | Read four-byte D+0x2C8, then a count-prefixed byte string into D+0x2D0; clear/populate the D+0x28 collection from eight-byte input records |
| 16 | Read eight bytes into T+0x1D0; clear T+0x980 and populate its linked records from 24-byte input records in input order |
| 17 | Read the remaining D+0xF8 keyed records, the flattened 16-byte records grouped through D+0x108, the eight-byte records appended through D+0x118, and the nested records reached through D+0x138, then clean up the final temporary string and return |

Complete 0x1496D0 copies all 160 bytes of each admitted equipment input record in ten ordered sixteen-byte chunks and advances the cursor; on an insufficient extent it clips the cursor and returns without making that record copy. Its caller ignores a separate whole-record result. The version-gated D records include eight-byte paired records and twelve-byte triples; their original insertion helpers, comparator details and malformed uniqueness behavior are outside this contract. The D+0x108 grouping arithmetic uses the reached signed 32-bit coordinate components with truncation toward zero by 4096 before its selected group insertion. No gameplay labels or coordinate units are inferred for these records here.

On complete valid input, the position just read remains the decoder's stored position, and the orientation is explicitly zeroed after the input orientation read. The health word at T+0x180 is a direct four-byte input store, following default-core assignment. No direct health clamp or rescale follows it in the remainder of 0x147800, 0x9BB20, the Continue prefix, or the stored-character branch's own direct stores. Finite zero or negative health is therefore not replaced merely because decoding completed. The class/description/query/provider closures and later ordinary Game updates are not a global no-write guarantee; their unclosed effects are not erased by this bounded observation.

The +0xAAC adjustment sign-extends its loaded 32-bit value and compares its unsigned 64-bit interpretation against decimal 10,000,000,000. When greater, it stores four-byte numeric 0x540BE400. Thus every negative signed 32-bit input meets this selected unsigned comparison, while nonnegative signed 32-bit values cannot exceed that threshold. This is the reached operand-width behavior, not an inferred ten-billion-valued 64-bit field or a general serialization rule for malformed data.

The complete 0x64A90 reset first calls 0x64C00 on T+0x10. It clears selected transient eight-byte, four-byte and byte fields, frees/reinitializes the linked T+0xAB8 collection and its count +0xAC0, invokes selected cleanup helpers on +0xB88, +0xB98 and +0xB78, resets the reached T+0x9F0 nested contents, sets its end equal to its current begin, clears +0xC18 and sets +0xC28 to the pointer at +0xC20, then clears/frees the nodes of T+0x980 while retaining its list head. It is not an allocation-wide clear. The complete core reset zeroes its equipment span and selected action/motion fields, supplies level 1/experience 0 and the two adjacent zero bytes through its defined stores, clears the selected name/core fields, sets its two selected coordinate pairs to -1 values, and assigns a default appearance through 0x4D1F0/0x7E780. Those two appearance helpers are boundaries in this contract.

Reset alone does not make the later default-core assignment redundant. The complete 0x4D600 constructor defines a health value 500 in its temporary core, while 0x64C00 has no direct store to the live health offset T+0x180. The later 0x3B7A0 assignment supplies that default before the input health overwrites it. The assignment copies the defined scalar/byte/record destinations, appearance through its selected helper, active item through its defined-field copier, the eleven equipment entries, sixteen name bytes and selected trailing core fields. Its preserved gaps and item padding are not assumed to be zero. The temporary's earlier limited initialization is not a whole-entity or whole-core zeroing claim.

The eight-byte entity identifier at T+8 lies outside the core-assignment receiver T+0x10 and is not a decoder replacement store. The separate eight-byte field T+0x968 is within the selected reset/default-core destinations and is zeroed before normal complete decoding; it is not an alias for T+8. Later producers of that separate field are outside this selected load contract. The decoder destroys/replaces D twice but does not replace the containing entity object or directly publish a new W+0x448 pointer.

Loaded T+0x980 records are populated after the initial reset. The post-read candidate insertion does not clear them. The selected callers' direct native stores and the 0x2E0B50 refresh do not perform another T+0x980 clear after that decode. In contrast, invoking 0x64A90 independently performs its initial list clear. The selected reset/decoder bodies do not directly empty T+0x9B8 or +0x9C8, and no whole-zero claim is made for those containers. The cursor record at T+0xA0C and all otherwise unmentioned bytes are not assigned invented reset values. Transitive helper effects, malformed name lengths, invalid counts, allocation failure, incomplete-field temporaries and asynchronous mutation remain explicit boundaries.

## Stored-character Select 0x9F7C0

The concrete Select caption is the six-code-unit UTF-16 literal at 0x4410F0. The bounded Interface setup stores the character Select node at I+0x38 under the character page I+0x18 and binds its widget to event 3, target 0x9F7C0 and captured context G. The complete callback itself has no additional root, hit, focus or button test. Direct alternate calls would enter its first store without the ordinary GUI admissions described earlier.

The callback first reloads the character page through G+0x130 and stores its selected integer to 0. It then locks current W through 0x2ABB70, reads signed 32-bit G+0x6C0, and immediately copies that value to four-byte G+0x2264. A negative value or a value at least the current signed low-32 slot count takes the new-character branch. A valid index takes the stored-character branch; it does not copy the preview entity at that vector position into the live entity.

On the stored branch, call 0x9F856 passes G, the captured index and a newly read current W+0x448 to 0x9BB20. After it returns, the callback builds a four-byte buffer containing a newly read current G+0x6C0 and publishes it through 0x672C0 on backend G+0x2268 under the four-byte byte-string key slot at 0x4360D0. This reread can differ from the captured decode index if an intervening unclosed provider/reentry changes the field. The callback does not repeat its original admission after that reread and does not condition this slot publication on a successful character decode. There is no additional G+0x1C98 guard directly around this slot-key publication; the outer current-World lock remains held on the normal route. Complete 0x672C0 forwards the current byte buffer, its pointer or null for an empty buffer, and its 64-bit length to backend boundary 0x672E0; backend return/durability behavior is not closed.

Next it calls 0x107690 on I, calls 0xE8A20 on I, then calls 0xB2180 on G. The supported 0xE8A20 body returns without effective object work; its compiler/runtime label is not evidence of an extra reset. It then stores binary32 8 at G+0x1DB8 and +0x1DB4, eight-byte binary64 90 at +0x1D98, zero at +0x1DA0 and 180 at +0x1DA8, and copies those current double values into +0x1D80, +0x1D88 and +0x1D90 in order. It cleans the slot buffer, rejoins, reloads current W for 0x2E0B50 and then reloads current W for 0x2B9890. The loader's own World guard is nested within this selected outer guard on an unchanged-owner ordinary path; imported critical-section/reentry behavior and changed-owner paths are not manufactured.

The invalid-index branch instead publishes the two float camera distances and double camera values 90, 0, 0, enables the new-character page, calls 0x64A90 on current local E, clears the first name byte and byte +0x60, zeroes the selected orientation/motion components and +0x5C, writes level 1 and assigns a health-provider result before its later class byte 1 store. It performs its explicit starter-equipment stores, calls 0x2D1100 for the two current home components and 0x35EA0 for the later height-dependent position store, replaces current D with 0x7FF20 construction, clears the creation editor through 0x334C50 and invokes synchronous 0x272950 and 0x272D90 on the selected style widget. Those home, height, appearance/style and allocation closures are not numerically specified here. It does not call the selected-character decoder or publish a slot-key buffer on that branch. Both branches call the final 0x2E0B50 and unlock current W. Invalid-index behavior is a separate new-character route; the stored branch is not assigned its reset or position-placement effects.

## Rebuild/publication callees on the stored-character branch

Complete 0x107690 uses I+0x5C0 to find the current Game/World/local entity and samples the current byte E+0x14C for its branch. It creates a temporary node list from the selected template at I+0x320. Byte 1 chooses the image-backed background resource names craft-warrior-weapons and craft-warrior-armor; byte 2 chooses craft-ranger-weapons and craft-ranger-armor; byte 3 chooses craft-mage-weapons and craft-mage-armor; byte 4 chooses craft-rogue-weapons and craft-rogue-armor. Other byte values add none of those class-dependent pair nodes. In every case it then appends nodes with backgrounds craft-jewels, craft-cooking and craft-ingredients, publishes the list through 0x280440 on I+0x228, and destroys the temporary list. These exact original resource names identify the selected publication inputs; no asset contents or class mechanics are inferred from them. The node/background factories and 0x280440 internals remain provider boundaries.

Complete 0xB2180 first reads current E through current W. Equal eight-byte begin/end pointers at E+0x9F0/+0x9F8 cause it to return without the selected candidate/UI rebuild. Otherwise it walks current D+0x18 records, queries 0x2AEC30 with W and each record, and accumulates admitted returned candidates into six temporary groups. It also uses native seed/hash arithmetic based on current E+0x150/+0x154, imported srand/rand boundaries, current E+0x14C branches, and five successive integer query inputs 0 through 4 to add selected generated candidates. The exact candidate-query and imported random numerical closures are not closed by this contract. Their call and input-read order is not a process-wide random-stream guarantee.

After candidate construction, 0xB2180 calls complete 0xB2EE0 six times with G and category integers 0, 1, 2, 3, 4 and 5, in that order. It ensures at least seven outer records at G+0x17A8, makes the seventh selected record's end equal its begin, clears the two selected maps at G+0x1938/+0x1948 and their 64-bit counts +0x1940/+0x1950, then calls 0x280A40 followed by 0x280C80 on current widget G+0x340. Temporary candidate storage is cleaned up afterward. When the initial inventory aggregate is empty, these selected publication/clear calls are not performed.

For each category, complete 0xB2EE0 makes that G+0x17A8 row empty by setting its end to its current begin, then publishes one 0xA4-byte count-and-item record per admitted 0xC0-byte candidate. For each candidate with component requirements, it reads current live inventory groups, sums matching signed count fields through the selected 0x4DF60 item comparisons, performs the reached signed division by the candidate requirement's four-byte count, and retains the limiting result across requirements. An empty requirement list supplies zero. No universal zero-divisor or malformed-count protection is inferred. It then publishes the selected count and defined item fields. If the current category and candidate index match current widget G+0x340 fields +0x278/+0x27C, it also updates the selected record and dependency-list fields of current widget G+0x348 and copies the selected pair to its +0x2B8 area. Actual live inventory is read for this publication; no direct live-health store is part of this helper. Its item/list copy and allocation helpers remain bounded dependencies.

Complete 0x2E0B50 is a further distinct derived-state pass. It clears the current local entity's D+0x128 tree and count +0x130, then traverses current D+0xA8 paired-coordinate records. It queries 0x69C60 on W with each pair, admits the selected result group through the original unsigned range test and bit mask, converts the selected coordinates through signed 32-bit multiplication by 16 and the native negative-coordinate division adjustment, then calls 0x36160 and 0x2BB060. The resulting four-byte key selects D+0x128 entries to which the selected coordinate pair is inserted. World/local-D reads are repeated at the native points; the query/generation/helper numerical closures are not expanded here. This complete owner makes no direct E+0x10 position store, E+0x180 health store, E+0x980 clear, or W+0x448 replacement. Its invocation is not an entity decoder or a source-selection publication.

The current-record helpers above and their providers can have separate lifetimes and protection requirements. The contract states their selected direct outputs and current receiver joins, without claiming that every transitive callback has no other side effect. The outer 0x9F7C0 normal World guard surrounds these reached publication calls; the Continue prefix calls only the final 0x2E0B50 after its explicit load has already released its own guards.

## Start Menu callback 0x9FDF0

The selected system widget is built by 0x291C70, retaining G at +0x1E8 and initializing four-byte choice +0x1F0 to -1. Its installed primary table is 0x4708B8, with draw target 0x291E00 at +8. The complete draw resets choice -1, evaluates its current provider-returned point against the selected row bounds, and writes choices 0, 1, 2 and 3 for the exact original labels Options, Controls, Start Menu and Exit Game respectively. Start Menu is the ten-code-unit UTF-16 literal at 0x470A40. Thus its choice 2 is not the title widget's Continue choice 3.

At the end of every normally completed system draw, complete registration owner 0x291AB0 installs event integer 3 with captured context equal to that same widget and the selected member target 0x292300. Its two registration bytes are 1. The callback record is a 0x28-byte allocation with table 0x470A78, one-byte gate +8, context +0x10, target +0x18 and a signed four-byte receiver adjustment at +0x20. This selected adjustment is zero. It replaces/frees the previous record and can propagate registrations to descendants. The table-resolved original-image invoker at 0x288C40 adds the sign-extended adjustment to the captured context and transfers to the target. Allocation failure can leave a null record; registering a target is not a promise that an invocation will occur.

The complete native leaf at 0x292300 first disables the current widget node, using widget+0x178 and the selected GUI integer. It then reads current signed 32-bit widget+0x1F0. Choice 0 reloads widget+0x1E8, adds 0x118 and transfers to 0x103110; choice 1 similarly transfers to 0x102ED0; choice 2 reloads that G and transfers to 0x9FDF0 at 0x29234C; choice 3 transfers to 0x9F660. Other values return. The alternative targets are not interpreted here. The widget hide therefore precedes 0x9FDF0 on this ordinary event route, whereas an independently invoked 0x9FDF0 has no such leading store of its own.

The complete 0x9FDF0 has no selected root/button/slot admission before its first call. On retained G it calls 0xA27B0 at 0x9FE02, then 0x911E0 at 0x9FE0A. It then starts signed 32-bit index zero and, when the signed low-32 vector count is positive, calls 0x9BB20 once per reached index, with the pointer currently read from G+0x1778 at that eight-byte element offset. The selected call is at 0x9FE40. It increments the element offset by eight and the index by one; each loop test rereads current G+0x1780 and +0x1778 to derive the count. This is not a copied vector snapshot or a whole-loop World/backend lock. Each loader independently performs its own selected guard and failure handling.

Only after those calls normally complete does 0x9FDF0 enable the current frontend root with selected integer 1, store byte zero at G+0x420, then byte zero at G+0x668. It then stores selected integer zero on the nodes currently reached through these widget pointers, in exact order: G+0x330, +0x340, +0x350, +0x360, +0x388, +0x418 and +0x378. Each widget node is reloaded through +0x178 and the current selected property/index/array chain. The callback does not directly set the title child G+0x120, reset all GUI state, reset the two camera floats, set the six camera doubles, or reload current live E through the Continue call. Its direct selected stores do not clear the raw key array or the 23 sampled action-state bytes.

The ordinary constructor establishes the receiver distinction for the loop. Its num-record path allocates and constructs slot entities in separate 0x15F8-byte construction calls, loads each through 0x9BB20 and appends its pointer to G+0x1778 through complete 0xA1A70. Later it performs a separate 0x15F8-byte construction, inserts that entity into the current W through 0x29D0D0, and publishes it to W+0x448. It subsequently loads the slot key and can load that live entity too. No selected constructor store aliases a preview-vector element into W+0x448. This proves separate ordinary publication routes, while arbitrary later vector mutation, allocation failure and aliases introduced by unclosed callers remain outside the census. The Start Menu loop's explicit target is each current vector element, not an implicit reread of W+0x448.

## Save-before-return and the selected socket stop helper

Complete 0xA27B0 first locks current W and captures signed 32-bit G+0x2264. If negative, it unlocks current W and returns without the selected save construction/backend sequence. On a nonnegative value it allocates a temporary 0x15F8-byte entity through the selected constructor boundary, assigns current live E's core through 0x3B7A0, copies four-byte +0x9A4/+0x9A8, constructs a new 0x300-byte subordinate through 0x7FBB0 with current live D as its source argument, copies the selected inventory aggregate through 0x83290, the four-byte field +0xA08, the cursor record through 0x9CD0, four-byte +0xAAC/+0xAB0, and the current +0x980 linked records through 0x7CD70. These selected copy callees and the temporary entity constructor are not promoted to complete transitive clone/destructor contracts here.

It also captures current W's four-byte day/time operands at +8/+0xC and makes temporary copies from W+0x10/+0x20/+0x30 through 0x6D400/0x6D5E0. It then unlocks current W before constructing the byte-buffer reader/writer context and calling 0x149760 with the temporary entity. The temporary entity is deleted through its current virtual deleting slot with argument 1 after that call. The three World-state temporary copies are later destroyed; no selected call passes them to 0x149760, whose receiver argument here is the temporary entity. This is a character-save route, not proof of a retained World-save transaction.

The captured slot integer is converted to a key through the stream/string boundary. After selected buffer preparation 0x66C10, the saver enters G+0x1C98 and publishes through 0x672C0 on G+0x2268, then leaves that guard and cleans temporary stream/buffer/World copies. The backend result is not tested to admit the following 0x911E0 or preview reloads in 0x9FDF0. The sequence is not an atomic save/disconnect/reload transaction, and no backend durability or error-recovery promise is made.

Complete 0x911E0 first stores four-byte zero at G+0x2260. If current byte G+0x21E8 is zero, it performs no selected socket/thread/World-role work after that store. With that byte nonzero, it clears the byte, tests the eight-byte socket operand G+0x21F8 and, when nonzero, calls the selected close import, stores eight-byte zero there, and stores byte zero at current W+0x358.

It then handles two thread records in order: eight-byte handle storage G+0x1FF8 with four-byte identifier G+0x2000, followed by G+0x2008 with identifier G+0x2010. A zero identifier skips that join. A nonzero identifier is compared with the current-thread provider result; equality calls the selected C++ error boundary with integer 5. Otherwise the native 16-byte thread representation is copied for the join boundary; a nonzero join result calls the error boundary with integer 2. On the normally returning success path it clears the stored handle and four-byte identifier before continuing. After both selected joins it locks current W, stores byte zero at W+0x358, publishes the fourteen-code-unit UTF-16 text Disconnected. followed by a line-feed code unit through the selected text boundary, and unlocks current W.

These are socket/active/thread fields on G. The complete helper does not call 0x3D820, does not directly zero the separate Client's selected-peer field +0x70, and does not destroy G, I or the pointer stored at G+8. The close/join/runtime/provider, exceptional unwinding and actual thread-exit behavior remain external boundaries. The two role-byte stores are reached at their stated current-World reload points, not under a proved immutable W snapshot. No universal cleanup, Steam-peer disconnect or safe concurrent teardown is inferred from the helper's name.

## Saved-world Select and direct World publication

The second selected Select control is created under I+0x80, published at I+0xC8, and bound to event 3, native target 0x9FD10 and captured context G. There is no decoded C export for this target in the selected original function collection. Its complete original native leaf was nevertheless established independently from that registration entry through every branch, the final return at 0x9FDEB and the tail transfer at 0x9FDC6; its 220 instruction bytes match the image continuously. The absence of an export is not filled with a neighboring function's body.

The leaf first disables the current world-selection page at G+0x198. It stores binary32 8 at G+0x1DB8 and then +0x1DB4. Its native double-store order is +0x1DA8 equal to 180, +0x1D98 equal to 90, +0x1DA0 equal to zero, +0x1D80 equal to 90, then copies current +0x1DA0 into +0x1D88 and current +0x1DA8 into +0x1D90. Only after those GUI/camera publications does it derive the low signed 32-bit eight-byte-element count from G+0x1790/+0x1798 and sign-extend current signed 32-bit G+0x1770 for selection.

The existing-record branch is selected when the signed 32-bit selected index is below that count. There is no separate nonnegative test in this leaf. Valid ordinary existing-record behavior here therefore presupposes a nonnegative index within the current vector; negative or otherwise malformed values are not assigned invented safe behavior. The branch reads that current vector element pointer, reads its four-byte +0x30 value, takes its byte-string object address +0x10, and transfers to complete 0x8A500 with G and those two source arguments. Otherwise it sets selected integer 1 on the current additional frontend page G+0x200 and returns. It does not directly invoke a character loader or constructor on either branch.

Complete 0x8A500 locks current W first. It reloads current W+0x448 and its D, stores the supplied four-byte value at D+0x2C8, then assigns the supplied byte-string object to D+0x2D0 unless those addresses are identical. It stores the same four-byte value at G+0x206C and assigns the same source byte string to G+0x2070 unless self-assignment by address. It then reloads current W for 0x2D2C00 and reloads current W again for unlock. No return status is checked for the string helper, and source-buffer/provider lifetime across these calls is not supplied by this contract.

This direct route publishes the stored source/configuration operands. It does not call 0x9BB20 or 0x147800, does not call either entity reset, does not replace D or W+0x448, and makes no direct local-entity position, orientation, home, health, inventory or buff store. The page/camera work occurs outside its selected World guard; the D/Game source fields and following World cleanup occur after that guard call. Imported/reentrant mutation could change the current receivers at explicit reloads; no immutable owner snapshot is assumed.

Complete 0x2D2C00 builds a temporary list of World+0x40 list nodes whose contained pointer differs from the current W+0x448. It finishes that traversal before its deletion pass. In temporary-list order, it calls a nonnull contained object's current virtual deleting slot with argument 1, unlinks that original World list node, decrements W+0x48 and frees the list node. A null contained pointer skips the virtual deletion but still reaches the selected node-unlink/count work. Concrete deleting target identities are not universal across this list and their object internals are not closed here.

The helper next clears the W+0x450 lookup tree and its 64-bit count +0x458, then rebuilds it by traversing the remaining World+0x40 list and publishing each current object's eight-byte identifier from object+8 with its pointer. Existing lookup keys receive the current pointer; no local identifier reassignment is performed by this helper. It clears/frees the linked W+0x50 records and zeroes count W+0x58, then destroys the temporary list. It does not directly store W+0x448. Thus the selected ordinary local pointer is retained by its pointer-equality admission; null, malformed, duplicate or reentrant list states are not turned into a guaranteed safe lifetime policy.

## Source/configuration and connection boundaries after publication

The decoder's D+0x2C8 and +0x2D0 input fields are distinct from the Game's four-byte +0x206C and byte-string +0x2070 fields and from current World's four-byte +0x220 and byte-string +0x200 fields. A complete normal decode can restore the D fields without a direct corresponding G-field store in 0x9BB20, the Continue prefix or the stored-character branch of 0x9F7C0. The saved-world callback's 0x8A500 route explicitly writes both D and G fields. It does not call World configuration 0x2B5F20 or directly write the World +0x220/+0x200 pair. Equality of these separate owners must not be inferred merely from a successful character load.

The complete Game constructor's selected direct stores initialize G+0x206C to 123 and G+0x2070 to the five-byte string world before its current-World configuration call. It opens the character backend at address G+0x2268 using the eighteen-byte literal Save/characters.db at 0x4360A0. Its num and slot keys are the three-byte literal at 0x436070 and four-byte literal at 0x4360D0. Conditional slot-record reads can publish G+0x2264 and load the separate live entity after the preview construction. This is concrete startup provenance, not a claim that every input allocation byte, missing-key value or later configured World field is initialized by these statements. The World configuration provider's numerical work and later writers are outside this contract.

A bounded native window of ordinary Game update 0xA5A50 compares current G+0x206C with current W+0x220, then calls complete byte-string equality helper 0x6DD20 on G+0x2070 and W+0x200. It enters the selected World substep calls at 0xA7328 and 0xA734C only on equality of both operands; mismatch branches past them. The equality helper compares byte contents through the selected memcmp provider and requires equal eight-byte lengths, using the inline/external-string selection at capacity 15. A later bounded window repeats the current numeric/string comparison through 0x6DDF0 and, on mismatch, calls complete 0xE8A30 on current renderer G+0x18. That small owner stores four-byte zero at renderer+0x4A4. No renderer source rebuild or field meaning is inferred from this one store.

These selected update windows show a consumer of pending Game publication, not the complete asynchronous configuration route or a completion acknowledgment. The original primary C export for 0xA5A50 is a timeout record; no behavior was inferred from it. The reported facts are from the selected image-matched native windows. The full 29,860-byte update owner, its elapsed arithmetic, all branch predecessors and all provider/reentrant effects are not claimed as closed here. The frame's later mutations, packet effects and eventual World configuration can change the entity state beyond the direct transition stores recorded above.

The complete Game constructor builds a Client at G+0xD18 with owner G passed to 0x3AA10. That constructor stores G at Client+0xA50, sets the packed selected peer at +0x70 to zero through the reached width-specific stores, writes four-byte timestamp +0x9EC zero, initializes its selected collections and starts its selected transport-worker boundary. The separate Host object is at G+0x6E8; complete 0xF17A0 constructs its World subobject at Host+8 and its selected thread/collection/provider state. Later the Game constructor stores that Host address at G+0x1728, which is Client+0xA10, and binds a Host callback through 0x7D140 with context equal to the Client and target 0x3CB50. This establishes the ordinary object joins; transport/Host callback internals are not imported from provider names.

The bounded native Game call at 0xAAEBF invokes complete Client owner 0x3F4F0 with receiver G+0xD18. No selected Continue, stored-character Select, saved-world Select or 0x9FDF0 body directly invokes this drain or 0x3D820. They therefore have no synchronous shared-connect completion promise. The separate ordinary update call can run later according to its own native caller/loop admissions, which are not a wall-clock schedule guarantee.

For the selected later fallback boundary, complete 0x3F4F0 first obtains a local work collection through 0x3A850 while holding Client+0xA18, clears the original Client+0xA40 queue nodes/count and leaves that guard. It dispatches the local records through 0x3E450 before reaching its subsequent World-record scan and clock checks. Dispatcher internals, accepted/rejected sender effects and every timestamp writer are outside this contract. Consequently +0x9EC is called a timestamp operand here, not an assumed universal last-seen timestamp.

After that local batch, the drain samples the selected 32-bit time provider. Its unsigned 32-bit difference from current Client+0x9EC, including modulo wrap at that width, admits the nonfallback work for differences through 5000 inclusive. A difference of at least 5001 selects 0x3D820 with the current eight-byte process value at RVA 0x54DDC8 as destination. Conditional invitation, elapsed-send and other send work in the complete owner remains a separate route; this fallback is not part of the frontend loader's return value. No clock availability, real elapsed duration, interthread timing or provider delivery guarantee is inferred.

Complete 0x3D820 first tests the old selected peer through complete packed predicate 0x3BE00. When admitted it sends the selected message-2 buffer to that old peer through the original local-Host or imported transport boundary and removes the selected old-peer map entry. It then assigns a default core to Client+0x78, sets byte Client+0x9E8 to 1, clears the packed peer, clears the selected Client+0x9F0 map through 0x3D6C0, and empties the selected linked collections at +0x20, +0x30, +0x50 and +0x40, in that order. The core assignment target is Client state, not current live E+0x10.

Next it invokes 0x2D2C00 and the unclosed 0x2DA090 on the current viewed World reached through Client+0xA50 and G+8. If Client+0xA10 is nonnull, it calls Host boundary 0xF4FF0 with that Host receiver. It creates the selected message-1 buffer and sends it to the supplied destination: equality with current process value 0x54DDC8 uses 0xF32C0 on the local Host when available; other destinations use the selected imported provider. On the normally returning path it samples the 32-bit time provider, stores that word at Client+0x9EC, publishes the destination as the eight-byte selected peer +0x70, and stores byte 1 at the then-current viewed W+0x358. Transport return values are not a success admission around those last stores. All unclosed Host, World-reset and transport-provider internals remain limits.

This shared-peer route is separate from the selected G+0x21E8/socket/thread route in 0x911E0. Reaching the latter does not establish that the former selected peer was cleared. Conversely a later local fallback can publish a role byte under its own route; this contract invents no synchronous or interthread order between those independently reached operations. The selected frontend routes retain G and embedded I and use the current W/local pointer; their own direct code does not allocate a new Game for a session change. Subordinate D replacement, World-list retirement and callback replacement have the narrower lifetimes explicitly described above.

## Independently chosen conditional examples

The examples fix source-visible current fields or provider return conditions to make the selected order reviewable. They are not original runtime observations, imported-provider sequences or a guarantee that the UI reaches those conditions.

1. Suppose both title-prefix selected integers are nonzero, the button is 0, title choice is 3 and the current slot input is 7. If 0x670F0 returns zero normally, 0x9BB20 performs no selected reset/decode/insertion. Continue still disables the frontend root, writes the two float camera distances to 8 and calls the derived D+0x128 refresh before GUI release. It does not substitute the stored-character callback's GUI/camera sequence.

2. Suppose the character Select callback captures a valid choice 2 and backend retrieval produces a valid complete record. Its retained live target is reset/defaulted and decoded; the later input position and health stores follow that defaulting, the orientation is zeroed after its input read, and the buff list is read after the earlier reset clear. If an unclosed provider/reentry changes G+0x6C0 to 4 before the callback's slot-buffer reread, that buffer contains 4 while the load input was 2. This is a conditional reread distinction, not evidence that such reentry ordinarily occurs.

3. Suppose the system widget event-3 callback is admitted with current choice 2. Its raw member callback disables the system node first, transfers to 0x9FDF0, calls the save helper and selected socket-stop helper, then reloads the current preview-vector entries through separate loader invocations. Only after those normally finish does it enable the frontend root, clear the two Game bytes and disable its seven listed widget nodes. Save miss or absent socket activity is not a separate rollback admission in that outer callback.

4. Suppose the world Select leaf sees a valid nonnegative in-range index and the current stored record supplies four-byte value 42 and a live byte-string object whose text is region-a. It disables its page and writes the camera operands before 0x8A500 locks current W. That helper publishes 42/string to current D and G, then retires the selected nonlocal World-list entries. It does not decode a character, place the retained local entity or prove that the current World +0x220/+0x200 pair has already become 42/string when it returns.

5. Suppose a later Client drain's current unsigned timestamp difference is exactly 5000. It does not select the timeout fallback on that comparison. Difference 5001 does. If the fallback's providers return normally, its final selected-peer/time/role stores follow its World cleanup and outgoing message-1 handoff; that still proves no provider delivery or wall-clock completion. These clock conditions are independent of whether a preceding Continue load succeeded.

## Explicit limits

Complete-byte coverage certifies correspondence of selected original instruction operands and extents, not universal runtime completeness. The call census includes statically resolved direct operands and the selected concrete table joins. It does not enumerate every computed caller, injected callback, alternate object subtype, global writer, user configuration or process teardown path.

Valid complete-record conclusions do not define arbitrary negative lengths/counts, integer-overflowed size products, name lengths exceeding the native storage, partial-read temporaries, malformed trees/lists, invalid virtual receivers, allocation failure or nonreturning error providers. No atomic-load, transaction, safe-null, whole-allocation zeroing or rollback behavior is inferred. No English gameplay meaning is assigned to unresolved identifiers, category bytes, source scalars or item/table results.

The full internals of generic string/allocation/copy helpers, 0x4CE80 entity construction, temporary clone/serialization 0x7FBB0/0x149760, the large 0x53E80 appearance boundary, item/terrain/world-generation queries, World reset 0x2DA090, Host callbacks, packet dispatcher 0x3E450, linked backend provider, imported clock/thread/socket/runtime/OS operations and actual font/GUI geometry remain unclosed except at the stated call/argument boundaries. Original 0x84F70 was read as an ancillary worker lead; no unrequested generation-request or scheduler closure is claimed from it. No earlier held prose supplied any behavior to this contract.

The entire 0x965E0 body has correspondence coverage, while only its frontend prefix and immediately reached GUI adapters are interpreted here. Only the explicitly listed Interface setup and Game update windows are interpreted from those large owners. The failed primary C export for 0xA5A50 supplies no behavioral evidence. Provider reentry can change later current loads where permitted by unclosed providers; source order does not assert the occurrence or absence of that reentry. Locks are named and ordered at their original import boundaries without importing a complete synchronization or scheduler model. No normal frame duration, packet arrival schedule or eventual World reconfiguration is promised.

## Complete selected-owner and correspondence manifest

The complete exported-owner coverage below totals 64 owners, 120 owned ranges, 11,263 original instruction rows and 48,210 code bytes. Every range and row matches the supported image, with no unexplained gap within a reported owned range. Disjoint intervals are listed explicitly; intervening padding is not counted as code. Eight hundred ninety-three direct relative calls across these owners were independently target-decoded and matched. Some complete small/ancillary bodies are included to bound construction, deletion or alternate-route claims; their presence does not enlarge the interpreted frontend scope of 0x965E0 or close their unlisted transitive callees.

| Owner RVA | Complete inclusive owned ranges | Instructions | Code bytes |
| --- | --- | ---: | ---: |
| 0x3AA10 | 0x3AA10–0x3AC0F | 131 | 512 |
| 0x3B100 | 0x3B100–0x3B1CC; 0x3B1CE–0x3B1D3; 0x3B1D5–0x3B1DA; 0x3B1DC–0x3B1E1; 0x3B1E3–0x3B41D | 215 | 794 |
| 0x3B6B0 | 0x3B6B0–0x3B790 | 41 | 225 |
| 0x3B7A0 | 0x3B7A0–0x3BAD4 | 164 | 821 |
| 0x3BE00 | 0x3BE00–0x3BE66 | 33 | 103 |
| 0x3D6C0 | 0x3D6C0–0x3D73D | 35 | 126 |
| 0x3D820 | 0x3D820–0x3DB64 | 191 | 837 |
| 0x3F4F0 | 0x3F4F0–0x3F628; 0x3F62A–0x3F62F; 0x3F631–0x3F636; 0x3F638–0x3F63D; 0x3F63F–0x4042D | 882 | 3,898 |
| 0x4D300 | 0x4D300–0x4D5F2 | 131 | 755 |
| 0x4D600 | 0x4D600–0x4D79E | 71 | 415 |
| 0x4DB70 | 0x4DB70–0x4DF34 | 225 | 965 |
| 0x4DF60 | 0x4DF60–0x4E062 | 73 | 259 |
| 0x5F440 | 0x5F440–0x5F4BB | 38 | 124 |
| 0x64A90 | 0x64A90–0x64BFA | 69 | 363 |
| 0x64C00 | 0x64C00–0x64D87 | 72 | 392 |
| 0x66D10 | 0x66D10–0x66D71; 0x66D73–0x66D81; 0x66D83–0x66D91; 0x66D93–0x66D9E; 0x66DA0–0x66DB2 | 43 | 159 |
| 0x670F0 | 0x670F0–0x671E9 | 68 | 250 |
| 0x672C0 | 0x672C0–0x672DB | 8 | 28 |
| 0x6DD20 | 0x6DD20–0x6DD93 | 39 | 116 |
| 0x6DDF0 | 0x6DDF0–0x6DE02 | 6 | 19 |
| 0x7D140 | 0x7D140–0x7D1C5 | 33 | 134 |
| 0x7EA80 | 0x7EA80–0x7F35D; 0x7F35F–0x7F36D; 0x7F36F–0x7F37D; 0x7F37F–0x7F38A; 0x7F38C–0x7F9DF | 806 | 3,932 |
| 0x7FF20 | 0x7FF20–0x80266 | 193 | 839 |
| 0x8A500 | 0x8A500–0x8A5A6 | 39 | 167 |
| 0x911E0 | 0x911E0–0x913A4 | 92 | 453 |
| 0x913B0 | 0x913B0–0x91C35 | 428 | 2,182 |
| 0x965E0 | 0x965E0–0x96832; 0x96834–0x96842; 0x96844–0x96852; 0x96854–0x9685F; 0x96861–0x96E6F | 508 | 2,188 |
| 0x9AF00 | 0x9AF00–0x9AF19 | 6 | 26 |
| 0x9BB20 | 0x9BB20–0x9BC7A; 0x9BC7C–0x9BC8A; 0x9BC8C–0x9BC9A; 0x9BC9C–0x9BCA7; 0x9BCA9–0x9BDF1 | 160 | 718 |
| 0x9F7C0 | 0x9F7C0–0x9F8E8; 0x9F8EA–0x9F8F8; 0x9F8FA–0x9F908; 0x9F90A–0x9F915; 0x9F917–0x9FD02 | 276 | 1,343 |
| 0x9FDF0 | 0x9FDF0–0x9FF98 | 84 | 425 |
| 0xA1A70 | 0xA1A70–0xA1AFF | 44 | 144 |
| 0xA1BD0 | 0xA1BD0–0xA1CEB | 69 | 284 |
| 0xA27B0 | 0xA27B0–0xA2ADC; 0xA2ADE–0xA2AEC; 0xA2AEE–0xA2AFC; 0xA2AFE–0xA2B09; 0xA2B0B–0xA2BD6 | 240 | 1,059 |
| 0xA3A20 | 0xA3A20–0xA3A3F | 8 | 32 |
| 0xB2180 | 0xB2180–0xB29B8; 0xB29BA–0xB29BF; 0xB29C1–0xB29C6; 0xB29C8–0xB29CD; 0xB29CF–0xB29D4; 0xB29D6–0xB2C34; 0xB2C36–0xB2C50; 0xB2C52–0xB2C60; 0xB2C62–0xB2C70; 0xB2C72–0xB2C7D; 0xB2C7F–0xB2CD6; 0xB2CD8–0xB2CF2; 0xB2CF4–0xB2D02; 0xB2D04–0xB2D12; 0xB2D14–0xB2D1F; 0xB2D21–0xB2D75; 0xB2D77–0xB2D91; 0xB2D93–0xB2DA1; 0xB2DA3–0xB2DB1; 0xB2DB3–0xB2DBE; 0xB2DC0–0xB2EDE | 852 | 3,403 |
| 0xB2EE0 | 0xB2EE0–0xB33E6 | 281 | 1,287 |
| 0xE8A20 | 0xE8A20–0xE8A22 | 1 | 3 |
| 0xE8A30 | 0xE8A30–0xE8A3A | 2 | 11 |
| 0xF17A0 | 0xF17A0–0xF1994; 0xF1996–0xF19A4; 0xF19A6–0xF19B4; 0xF19B6–0xF19C1; 0xF19C3–0xF1A40 | 160 | 669 |
| 0xF8380 | 0xF8380–0xF8401 | 35 | 130 |
| 0xF8410 | 0xF8410–0xF858D | 99 | 382 |
| 0xF8690 | 0xF8690–0xF87C9 | 67 | 314 |
| 0x101120 | 0x101120–0x10115E | 16 | 63 |
| 0x107690 | 0x107690–0x1081A9 | 681 | 2,842 |
| 0x133C70 | 0x133C70–0x133EEB | 174 | 636 |
| 0x147750 | 0x147750–0x14776B | 7 | 28 |
| 0x147800 | 0x147800–0x148E8F | 1,531 | 5,776 |
| 0x1496D0 | 0x1496D0–0x149757 | 36 | 136 |
| 0x2910C0 | 0x2910C0–0x291200 | 75 | 321 |
| 0x291210 | 0x291210–0x291AAC | 487 | 2,205 |
| 0x291AB0 | 0x291AB0–0x291C6D | 111 | 446 |
| 0x291C70 | 0x291C70–0x291DB0 | 75 | 321 |
| 0x291DC0 | 0x291DC0–0x291DF3 | 16 | 52 |
| 0x291E00 | 0x291E00–0x2922F3 | 284 | 1,268 |
| 0x2ABB70 | 0x2ABB70–0x2ABB7D | 2 | 14 |
| 0x2B9890 | 0x2B9890–0x2B989D | 2 | 14 |
| 0x2D2C00 | 0x2D2C00–0x2D2EAE | 180 | 687 |
| 0x2E0B50 | 0x2E0B50–0x2E0DA7 | 158 | 600 |
| 0x32A210 | 0x32A210–0x32A2B4 | 46 | 165 |
| 0x33C360 | 0x33C360–0x33C406 | 49 | 167 |
| 0x3410E0 | 0x3410E0–0x3412F4 | 141 | 533 |
| 0x3414A0 | 0x3414A0–0x3415E4 | 87 | 325 |
| 0x341780 | 0x341780–0x3418C4 | 87 | 325 |

The selected raw leaves add 75 instructions and 393 code bytes. Six have complete matched original assembly rows; the member-callback invoker is the separately identified original-image decode. The complete leaf boundaries follow their registered/table-resolved entries, reached branches, returns or tail transfers, and following original padding. No decoded export was fabricated for an absent owner.

| Native leaf | Inclusive extent | Instructions | Code bytes | Correspondence method |
| --- | --- | ---: | ---: | --- |
| saved-world selection leaf | 0x9FD10–0x9FDEB | 37 | 220 | Original instruction rows and continuous image match |
| system-menu event leaf | 0x292300–0x292377 | 25 | 120 | Original instruction rows and continuous image match |
| ordinary Game callback invoker | 0xF99A0–0xF99AA | 3 | 11 | Original instruction rows and continuous image match |
| member callback invocation leaf | 0x288C40–0x288C4E | 4 | 15 | Complete direct-image leaf decode; no assembly rows for this leaf |
| left-release traversal virtual adapter | 0x33AADC–0x33AAE4 | 2 | 9 | Original instruction rows and continuous image match |
| right-release traversal virtual adapter | 0x33AA94–0x33AA9C | 2 | 9 | Original instruction rows and continuous image match |
| middle-release traversal virtual adapter | 0x33AAAC–0x33AAB4 | 2 | 9 | Original instruction rows and continuous image match |

The ten bounded native windows add 387 instructions and 2,050 code bytes. All are continuously matched to the image and contain 45 further independently decoded direct relative call operands. Together with the complete owners and raw leaves, selected correspondence covers 11,725 instructions and 50,653 code bytes. Those totals do not claim complete coverage of either large owner below.

| Selected bounded window | Owning RVA | Inclusive matched extent | Instructions | Code bytes |
| --- | --- | --- | ---: | ---: |
| Interface setup receiver | 0xF9AC0 | 0xF9AC0–0xF9B65 | 30 | 166 |
| frontend page construction and selected values | 0xF9AC0 | 0xF9D3F–0xFA0F7 | 170 | 953 |
| title child and widget publication | 0xF9AC0 | 0xFB644–0xFB6E0 | 34 | 157 |
| stored-character Select binding | 0xF9AC0 | 0xFCB3E–0xFCBBF | 27 | 130 |
| stored-world Select binding | 0xF9AC0 | 0xFCF42–0xFCFCB | 27 | 138 |
| system-menu construction and initial admission | 0xF9AC0 | 0x1006A2–0x100710 | 24 | 111 |
| ordinary World-step current-selection gate | 0xA5A50 | 0xA72CE–0xA7376 | 34 | 169 |
| ordinary Client drain call | 0xA5A50 | 0xAAEB8–0xAAEC3 | 2 | 12 |
| late current-selection renderer reset | 0xA5A50 | 0xAD724–0xAD756 | 11 | 51 |
| ordinary Game update receiver | 0xA5A50 | 0xA5A50–0xA5AF2 | 28 | 163 |

The concrete data joins include primary table 0x470630 offset +8 to title draw 0x291210, primary table 0x4708B8 offset +8 to system draw 0x291E00, callback table 0x441BA8 to raw invoker 0xF99A0, and member-callback table 0x470A78 to raw invoker 0x288C40. The selected Game callback target operands are 0xFCBA4 for 0x9F7C0 and 0xFCFB0 for 0x9FD10. Registration calls are 0xFCBBB and 0xFCFC7 respectively. The ordinary system callback's transfer to 0x9FDF0 is 0x29234C. These are concrete pointer/caller joins, not an unrestricted virtual-target census.

The selected original literal-data facts are:

| Literal RVA | Encoding and length excluding terminator | Text |
| --- | --- | --- |
| 0x4707A0 | UTF-16, 10 code units | Start Game |
| 0x4707B8 | UTF-16, 8 code units | Continue |
| 0x4707D0 | UTF-16, 7 code units | Options |
| 0x4707E0 | UTF-16, 4 code units | Exit |
| 0x470A28 | UTF-16, 8 code units | Controls |
| 0x470A40 | UTF-16, 10 code units | Start Menu |
| 0x470A58 | UTF-16, 9 code units | Exit Game |
| 0x4410F0 | UTF-16, 6 code units | Select |
| 0x4360A0 | Byte string, 18 bytes | Save/characters.db |
| 0x436070 | Byte string, 3 bytes | num |
| 0x4360D0 | Byte string, 4 bytes | slot |

All corresponding selected literal-load operands, terminators, construction lengths, branch constants, allocation dimensions and camera words were reconciled with the supported image. The camera values used here are exact binary32 8 and exact binary64 0, 90 and 180. No provider-returned screen coordinates, font extents, backend blobs, current World data or imported random values were invented as original observations.

The original resolved-call metadata lists five direct character-loader sites: 0x7F488 and 0x7F71A in construction, 0x966B4 in release, 0x9F856 in stored-character Select, and 0x9FE40 in Start Menu. The selected callback data establishes the two Select callers even though callback target loads are not ordinary direct-call records. The resolved direct/tail census for 0x8A500 includes 0x9FDC6 and an additional site in 0x9F060; the latter owner's behavior is outside this contract. The release method also has additional resolved callers in large startup owner 0x1355A0, outside the ordinary window joins closed here. These are bounded discovery statements from original metadata, not a negative proof that no computed or alternate caller exists.

All complete C exports listed below are under C:/Users/DieuDeGlace/Documents/decompiled cube world steam/decompiled/functions. Fresh source-byte hashes were checked against the complete bodies used in this audit.

| Original export filename | File bytes | SHA256 |
| --- | ---: | --- |
| 14003aa10.c | 2,823 | 4B70F2DEA5EB7E2AD5DA2037D11D1CDEB1816C1A7647855CDF5E97D857573A2D |
| 14003b100.c | 5,522 | 803E663150A71B47F79AEC15F0C305C175338E3A32C83263455F7C77B261AFBA |
| 14003b6b0.c | 702 | 45AD41F05B935F6126816D23F4A2198922485A4F768BD15852E19D3A73C1ADD2 |
| 14003b7a0.c | 5,611 | 7664DC840972F975BE81BE9E38F0DB1D44E934B9BD01F2EBDB559D98758A1E86 |
| 14003be00.c | 764 | 52908379DE962EE745F98A835315AF260F6C7278BB646F9A4ED99870A6DB2878 |
| 14003d6c0.c | 800 | 1833169B6D3F3A95B80A99E04EE13FB6A27551879F87D1D5E99A808FBE17F4DB |
| 14003d820.c | 5,064 | 574703A50518E3C2AB706DCAA08E92DA7BE365E22C5C6D224C3A2B050FA4DB9D |
| 14003f4f0.c | 26,137 | 2E9CBA16423837752E162A73A998E4D5B59F103D28CD393D25BF01BCA9742041 |
| 14004d300.c | 2,890 | 442739DBC573FA2EDE3930A0BD7420C5D2AA6901B1CEC62D3B7FA5B3901279C1 |
| 14004d600.c | 2,205 | ACF06DD7CEF8BB2C489002A496581A248295CADDF3F662AF080C1094D024C17D |
| 14004db70.c | 4,073 | 4EB134E4B9C0D8EE8967BCB3BACF58A901C6D7CF7CF9A9225F4C6EE978F25241 |
| 14004df60.c | 2,043 | 9F0D752F6CFED8BC0C74F1D311444A41255B39DB91B4EF8B3A65C10018FBC12E |
| 14005f440.c | 606 | CB03823A0FBDC57A44FDCCA83C32B5B3D1BB2B19A33863E5BD68AD0D8D532247 |
| 140064a90.c | 2,168 | 3616169AF7A65974A860A077F40052CEFB4FF028B9560B1336179C3C62A842A9 |
| 140064c00.c | 2,265 | 42E43FA98DA263A40381A93EA223241735E8C8D9F1E55CEBEF0A23599E7FB438 |
| 140066d10.c | 1,387 | 0E510DFB7DD2773E4B0B11CE182D3B12727712423D164A6081C868AA79D6595B |
| 1400670f0.c | 1,101 | 93991F3FA017B46310098D5C5ADA91E8DCA7614248321D9F5D1DA5485919B4F7 |
| 1400672c0.c | 379 | 6F405D4946986D086EE6EDCF33BAEF7E780C938471616B65B19EDCE2F1AEFA9B |
| 14006dd20.c | 1,016 | C083DFB883A158EA06610E0747D7F5032A76F28C78DA9179258D36A72ED5FC0C |
| 14006ddf0.c | 197 | 8EBE307AAC5BB5F5A2CEE6AC6482264C4EA1FC115E5B345562FD08E45A9AEA94 |
| 14007d140.c | 571 | 1FB230E2EE095A4C3798EE48E7F14A6903ECDFCD0E9BE406B5626DE386FA5E3A |
| 14007ea80.c | 21,373 | C278E06AE3FA997392319ABA657A62FD6C97CEEBD6EA0202A31A75544CABB9B4 |
| 14007ff20.c | 4,597 | B0CE04D639502F365FE71A945B0AC979F350A53D637904373692F2686BDD1084 |
| 14008a500.c | 813 | 719BFE6DE2B680796FB2274C6B7A94EA354CB4959278B20CB986A5D6C9DD2FB0 |
| 1400911e0.c | 3,023 | B264491FD6868079362FBF6FE6BD15E82C4030CC4B73A91AAD50A0B9F2C90E13 |
| 1400913b0.c | 10,164 | 5C79512CBC6E3EC758516216F8CFB07E11D3E1035A8D56D633D6BE4D70F3FF9B |
| 1400965e0.c | 13,188 | 6C18DA23F8B2F14C7316808F887FF80637062784069105FDA8DAD3E633C11AFE |
| 14009af00.c | 404 | D3BE3BE847C61374EBFCFD454B816651701BDEB4D2FBB81673131F62CE33861C |
| 14009bb20.c | 4,696 | CC5FCD0759D88192477CF7C929E880E7486F83D3225400080C32DA026E9D6350 |
| 14009f7c0.c | 7,617 | 1DA9C16370AA04FD1DD44E578070D39AF025862E2531F11E74A3ED635FD5DAE7 |
| 14009fdf0.c | 2,117 | 6126E8DB59AE055FC48E5BBF65732B36AC549EAEFA899F7E09AA41050B276761 |
| 1400a1a70.c | 920 | 90864EDC655217D6967BF12627986D0010063825DDA07422CDCE367F5E2EBFE2 |
| 1400a1bd0.c | 1,159 | B3BF756B309164A1B92C143649632819404BE1C1C52C976E7D900118D3B3EAA7 |
| 1400a27b0.c | 6,223 | DF08B1F7CB7A89DBFD16F12B1B3A0596B02574065429B68D196B8A1C9C06FCFA |
| 1400a3a20.c | 329 | 275410621ABA6219E34F98C9AB4A6A25FB7836BBD7FEAB40C5C6220CA0321BEB |
| 1400b2180.c | 19,361 | 7465AA0276F5CA23A05380BC2889BA43A6641B36B558186EC59BC3FFADB2420E |
| 1400b2ee0.c | 7,279 | 0BD34CBF0A7347F3AA2618BFF8E27700781FC5D450B825CAE3469F1E5E708AA4 |
| 1400e8a20.c | 145 | 1AC167C2723F370234EDA3B684BAD16EB04AB78251E39999B97E53DCB7AEB168 |
| 1400e8a30.c | 189 | 58A08E5A04F447CD21430BA27BA488DC48C7B02A1A3551837060B9DF677664B7 |
| 1400f17a0.c | 3,737 | 3B9D56A104B40DD50B0EA0567730E229E1CD17A673FC0FC700F129B4377D0212 |
| 1400f8380.c | 603 | F63C3753215B6B5E37B9CF21BCE0DF2D28B65BF85A0CB716E167A1E69F0BBF01 |
| 1400f8410.c | 2,056 | 4F7FF8251719FA7E0BB332373ACFD8558CFFFA612C564DACCE01698A5188CDC4 |
| 1400f8690.c | 1,842 | 12235D34B2002ED1F32FDF1706748CD75D6B5A83982F3E1FB16B709DD6EFCBD5 |
| 140101120.c | 512 | 180377213ED05A238C37895193ACB540ECA8DC3AB50BC27CD37D8D67DE672413 |
| 140107690.c | 10,673 | EE213FE866C13D03004346D6BD682E3B0DAAF4A6E561EC1C4B796AA7B39DC567 |
| 140133c70.c | 2,615 | A86B078CCBD0047E55CF4A34D983ABE58B774D1B1E128003A67158B59268E002 |
| 140147750.c | 290 | 5C941040B75C34C92CD4AD8B60CECB4ACBD6AF685D474C2F03982AFA46290346 |
| 140147800.c | 37,659 | 0E3B16F0925B533ACBD3EC3D62DC861213528C7C2F1C9B327C10E287F83D398C |
| 1401496d0.c | 1,435 | DBDFBC340EF2639B4F1B9280E34F270EFFC8D49A27C25BF92BE03BB9E48785A4 |
| 1402910c0.c | 1,425 | 651B00D9AB49AB5DC4514BB88E6F911FF15F2E1DFC0497A770B47B976A5DEA0C |
| 140291210.c | 9,429 | 7A051504C4ACB39F6A707379DABDB7840A431FED70B26B5B38AD1E4634C08C3D |
| 140291ab0.c | 2,808 | A7AB2C9B4A13B2FE6350B5F3451E31DD24A1286576D39FA2F54EF207A655A0FC |
| 140291c70.c | 1,425 | 8090D8B4F434AEF203685B3A0903B7393B6501D17F53053674B549FB7F603C76 |
| 140291dc0.c | 248 | 8F8955517571D42D9B01DAD4C98C5385A73DC18ABD23F18505D1B29D9A384203 |
| 140291e00.c | 5,057 | 5FBA4EC703B52E2FFFB62BEF514811FD41F32B8A9B5CCBBBD4F74D624EA18732 |
| 1402abb70.c | 381 | 2C6B20698456B5DF225CBCE26D9E1438DAAEB4FAB54092EC52BDA8FF0F2D9FDD |
| 1402b9890.c | 381 | D116110AEC3414C9B247867733D7B8B3A1AE8E099D4D40DC08D959469B8B4E13 |
| 1402d2c00.c | 4,153 | 748982AC6A30959792C0BBD707652E6E7C8C386E1D007FA89ADBD985B32F562D |
| 1402e0b50.c | 3,776 | 38767ABA125F0DD3B54387A7F1C3E25E7E80CE75386AC2B4FF52DBFACEB250AE |
| 14032a210.c | 1,204 | DCBFC095F367EA56FDC11209FD64E7E470504DA51E9DAFF47CB3ED172C1369BD |
| 14033c360.c | 1,261 | DF98B7630E4E96721CCAB5D15E11A5C2A2904172CCE16A9FCEEF8B22B1CFCFE2 |
| 1403410e0.c | 2,995 | 221FFBEC6AE61D940FF353D337328D70C23B9A2EFD8BD07118895BB830DDF530 |
| 1403414a0.c | 1,920 | B9A0CEEA632C20C493573C09C29AB4E1F558128EA0D0E311288C44B4D7A58B93 |
| 140341780.c | 1,920 | CFE76A6EEF4DEC3DFFE2B3268FADBC37C09E3B202E3E9CA02D5ED782F2633A28 |

Additional original export files were individually selected for the bounded windows or explicit unclosed leads:

| Original export filename | File bytes | SHA256 | Evidence scope |
| --- | ---: | --- | --- |
| 1400f9ac0.c | 100,185 | 20002CE3930C1734EE610C6F2D19F4E519EE2A01F3CF32A486508066679E81DA | Only the listed setup/registration windows are behaviorally closed |
| 1400a5a50.c | 146 | 0E3C1FE01A9A2B7CED6455F7813108713A3CCF816AE66C9F976522EEC75C82BA | Timeout record; behavioral evidence comes only from the listed original native windows |
| 140053e80.c | 187,887 | B7907695B53976167D878B90CB7015BF036C031BC2566F5FC426A230F91E9F76 | Large reached call boundary; no complete-owner or numerical closure claimed |
| 140084f70.c | 12,109 | E849566A0F93739977B36A3FEB5FE3EA9E6A7EC78C5AB011A110F16485D3299E | Ancillary original worker lead; no generation/scheduler closure claimed |

The original global export/metadata files used for selected lookups, row correspondence and bounded caller discovery are all under the original decompiled directory:

| Original evidence file | File bytes | SHA256 |
| --- | ---: | --- |
| assembly.txt | 62,547,291 | FE07326A0F4585432A9D4E42E7CF16C570815A8FCD164DE36616095023A0740C |
| function_ranges.csv | 509,127 | BC127412C5A8F21D8BA78AA53D9EB35A16AF58C1A85906DA9FF31E968E379601 |
| calls.csv | 10,840,369 | CF410A7DD84BBEF0E9FC4FC6FD1FF3B4903D97E9126C8F1A1D69EF05CD566DCB |
| functions.csv | 1,551,545 | 940E3682B9CCC68458531AC8D948A20F8B9530CDFFF5E568C5517BA91365DCEC |

The saved artifact is a new independent code-free handoff. Before reporting FINAL / STABLE / HELD, the author reads its entire saved contents back, verifies strict UTF-8 without BOM and exact equality with the prepared bytes, checks the final byte count and SHA256, and freshly verifies the supported original image identity. The reported hash identifies the complete file, including this evidence and limits manifest. All writes stop after that hold report. Earlier held artifacts remain unread and immutable.
