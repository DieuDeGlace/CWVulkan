# Native initial Zone generator escape audit

This bounded original-evidence contract is FINAL / STABLE / HELD following author readback. The supported image is cubeworld.original.exe.unpacked.exe, AMD64 PE32+, image base 0x140000000, length 5,757,952 bytes, SHA-256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. Evidence consists only of that original binary, original function decompilations, assembly.txt, calls.csv and function_ranges.csv, after the standalone author rules. No other project prose, replacement implementation, SDK, tests, captures or implementation history supplied evidence.

The conclusion remains bounded: this audit does not establish that every normally generated unpublished Zone can be destroyed without outstanding references. It establishes a complete callsite inventory for the normal instruction-control-flow traversal described below, identifies concrete local ownership and synchronous callback paths, and leaves the remaining callee scopes explicitly unproved. Absence of a direct named publication or worker function is not an absence-of-escape proof.

The original generator entry is RVA 0x2ff6f0. Its supplied function records describe twelve discontiguous instruction ranges ending at 0x319040. The supplied call records attribute 2,592 CALL instructions to the generator: 2,317 direct relative calls and 275 calls through identified import slots, across 277 targets. There is also a separate INT3 record at 0x3022c2, which is not a callable edge. All 2,592 attributed CALL sites agree with the attributed original assembly listing. A decompilation timeout is not used to clear any path.

Crucially, those attributed records omit reachable switch cases. The seven computed dispatch instructions at 0x305fc9, 0x30653a, 0x306d7e, 0x314a7a, 0x3173a0, 0x31742c and 0x317fcf use tables in the original image. Their target tables begin at 0x319044, 0x31905c, 0x319084, 0x3190a4, 0x319130, 0x319164 and 0x319188 respectively. The fourth dispatch first uses a 70-byte index table at 0x3190e8, selecting among seventeen four-byte target entries. The other table lengths are six, ten, eight, thirteen, nine and seven four-byte entries. These dispatches include cases omitted from the attributed assembly ranges, especially the substantial interval after 0x314a7a.

Independent decoding of the original image followed ordinary conditional branches and all seven original dispatch tables, without traversing calls into their callees. It recovered 23,775 instruction starts and 2,759 CALL sites through the generator's normal return boundary at 0x319040. It recovered every attributed call and added 167 CALL sites: 148 direct relative calls and nineteen calls through the already identified rand import. There were no unresolved computed jumps, outside-range jump destinations, instruction overlaps or decode failures in this bounded traversal. This is syntactic instruction reachability, not proof that every case occurs for a supported input. Exception handlers, runtime callbacks, externally altered imports and whole-program lifetime are not covered by this normal-flow result.

The combined inventory contains 2,465 direct relative callsites, 294 import-call sites and 294 distinct targets. Seventeen targets occur only in the recovered cases: 0x177c0, 0x2b630, 0xba9a0, 0x145b30, 0x145cf0, 0x145eb0, 0x145fa0, 0x146450, 0x260d50, 0x2626e0, 0x263210, 0x263ea0, 0x264670, 0x2651f0, 0x265840, 0x265e70 and 0x266ad0. The index below includes every recovered CALL site, distinguishes the 167 binary additions from the supplied records, and includes imported calls for completeness. All site numbers in that index are hexadecimal RVAs, even where the repeated 0x prefix is omitted.

The classification applies at the stated level of evidence, not to an unexamined transitive closure. Leaf means an inspected short numeric predicate, conversion or field-read body with no retained input address observed. Destination means an inspected body writes caller-provided storage or returns a borrowed address; the caller's destination ownership and later uses are not thereby cleared. Zone attachment means a concrete child-pointer attachment into a Zone-owned container. Static value means the particular fixed global value initialization described below. Runtime identifies a runtime, allocation, import or diagnostic-stream operation; its callbacks and runtime internals are not universally qualified. Partial means a plausible retention or mutation edge received bounded inspection but its entire descendant scope remains unproved. Unproved means no sufficiently complete ownership qualification was established in this audit. There are 208 unproved targets and 37 partial targets; neither category is cleared for an absence-of-escape conclusion.

The clearest direct pointer attachment is 0x119530. Its first object supplies Zone coordinates at +0x10 and +0x14 and owns a tree at +0x460. It stores the supplied child pointer in the selected node at +0x28, replacing and destroying a previous child in that slot when necessary. This is a Zone-owned child association, not a World+0x460 association merely because the numeric offset happens to match. The generator contains ten direct calls to this routine, all indexed below. At 0x303560, 0x3175f3, 0x317761, 0x317907, 0x318c4c and 0x318de4, nearby assembly explicitly recovers the saved Zone base from the generator's stack before making that attachment. Other direct attachment sites use preserved owner registers. No external refcount or worker registration is performed in the inspected attachment body; its insertion helper and every possible external alias are not given a universal lifetime guarantee.

Child constructor 0x118a60 initializes the supplied child destination and its owned collection headers. It takes no separate Zone input in the observed body, but construction alone is not a full publication audit. Helpers 0x1194b0 and 0x129b50 copy selected 0xe0-byte records into Zone-owned vectors rather than retaining the source-record address. Broader child-population routes remain partial: direct 0x11a820, 0x2fa7b0 and 0x2fb850 reach child allocation, construction and 0x119530 as well as other helpers. Observed child attachment in these routes does not clear every other reached operation. Their full input provenance, nested callback scope and exception cleanup remain unproved.

The late direct calls to 0x31d470 and 0x31d700 rebuild Zone-owned record storage, a list at Zone+0x4b8 and a coordinate-key tree at Zone+0xe8, and write six-byte cell values through the inline column storage beginning at Zone+0x518. The list copies three eight-byte position quantities; the coordinate tree copies three 32-bit coordinate values. The inspected cell write path copies the supplied cell bytes rather than retaining a stack-cell address. These are concrete local retention findings, not a complete qualification of every value helper or outside reader. Normal Zone destructor 0x2f9940 releases its local lists, trees, child associations and column allocations. Destructor existence does not settle outstanding external references or reader races.

A previously opaque callable-object edge can be narrowed. At direct site 0x30299c, 0x146090 constructs a callable object in its own stack storage, borrowing the destination wrapper and scalar argument addresses. It passes that object to iterator 0x146390. The iterator directly invokes the callable during its loop and does not enqueue or retain it. The original image's table at 0x4460d8 has its invocation entry at 0x145980 and cleanup entry at 0x145930. Original assembly at 0x145980 forwards immediately to 0x1448a0. The callback's observed work reads Zone columns and writes copied cell values through 0x31c1c0, while reaching scalar/World-value helpers. The callable wrapper is cleaned before 0x146090 returns. Thus this wrapper and iterator do not themselves introduce a deferred worker lifetime; the callback's complete descendant ownership scope remains partial.

The recovered cases add related callable wrappers 0x145b30, 0x145cf0, 0x145eb0, 0x145fa0 and 0x146450. Their inspected bodies also construct local callable state, call 0x146390 and clean local callable state afterwards. Those variants' invocation/cleanup tables and complete callback descendants were not all independently resolved. They remain partial in the index. Case helpers 0x2626e0, 0x263210, 0x263ea0, 0x264670, 0x2651f0, 0x265840, 0x265e70 and 0x266ad0 also remain partial. Observed portions manipulate local temporary lists, wrapper scalar fields, Zone record values or cell storage; some reach callable processing. This is insufficient to exclude every outward retention edge in their descendants. Their presence would have been missed by using only the generator's attributed call records.

The binary-only helper 0x260d50 is fully visible as a short destination initializer: it stores the supplied pointer in the destination's first eight bytes and initializes adjacent scalar fields. At generator site 0x314a8b the destination is a generator-stack wrapper and the supplied pointer is the current Zone. This proves a local borrowed Zone pointer at that site; it does not mean that the twelve later builder calls using wrappers cannot publish something. Similarly, 0x142ee0 stores an input pointer into caller storage, and 0x2ff610 returns a borrowed inline Zone-column address. These destination-class edges make later-use provenance material and are not blanket no-retention findings.

There is one directly observed lifetime extending beyond the generator return that contains copied scalar data rather than a Zone address. The guarded block around 0x317a6b constructs a global vector at image RVA 0x553540 through 0x2f9180. Its input is a 32-byte range assembled from image constants in generator-stack storage. Helper 0x44850 allocates and copies that range; it does not save the stack-range address. The generator registers the fixed function address 0x42b1e0 with atexit at 0x317ae5. That callback frees and clears the global vector. The registered callback address is fixed, not a closure carrying the Zone. This is persistent global value storage and a process-exit cleanup callback, not evidence that all global writes elsewhere are harmless.

Five direct Sleep calls pass zero. They yield execution; they do not submit a worker or establish an object-lifetime barrier. The direct rand import has 281 static sites in the combined inventory and srand has four; these counts are callsite counts, not a runtime draw count. Diagnostics through 0x7080 reach virtual stream operations. At the two direct 0x7080 sites the string pointer is an image-static literal and the stream comes from an import. No Zone pointer is supplied to that string insertion, but arbitrary stream/runtime behavior is outside a universal purity claim.

A bounded named-call graph search of the supplied call records through depth three found runtime static-initialization wait/signal paths, through 0x392dc8 to 0x392e8c and through 0x392d68 to 0x392e30. It did not identify a named worker-submission API in that search. That search used the supplied metadata, has a depth limit and cannot repair missing descendants or unknown virtual targets. An unresolved-call scan through node depth two identified the constructor callback at 0x9dd8 inside 0x9db0, stream callbacks at 0x70fb and 0x722b inside 0x7080, the iterator callback at 0x14641e inside 0x146390, and a runtime function-pointer call at 0x392ed7 inside 0x392e8c. The constructor callback is supplied explicitly as 0x2f9310 at generator site 0x308ca1 and initializes only each destination element's small scalar fields. The 0x146090 iterator callback was resolved as described above. The runtime and stream function-pointer internals remain runtime boundaries, not inferred game-worker registrations. Recovered wrapper cleanup calls at 0x145cd8, 0x145e98 and 0x14654c are additional partially qualified indirect edges.

No direct call to refresh 0x31c540 or its pointer-tree insertion helper 0x73f40 appears in the combined generator inventory. This excludes those particular direct call edges only. A descendant could call them, an equivalent write could be inlined, or a different registry could be used. Direct stores in the generator and every descendant's shared-memory effects are not exhaustively classified here. Consequently no universal absence of World notifications, borrowed Zone/child pointers, cache entries or shared ownership is established.

Protection remains equally bounded. The entry accepts a borrowed live Zone; its World pointer and local owned allocations must remain valid for the operations that dereference them. The direct inventory does not show the ordinary World enter/leave helpers 0x2abb70 and 0x2b9890, and runtime static-initialization guards protect their own runtime state. Neither fact settles caller-held protection or locks acquired deeper in all callee scopes. No worker drain, universal lock order, disposal policy or general quiescence guarantee follows from this note.

The remaining unknown-edge index is the complete site inventory below: every unproved target remains an open callee scope at every listed site; every partial target remains open beyond the specifically described facts; destination and runtime targets retain their stated provenance/runtime limits. High-priority unresolved scopes include the large owner/population constructor 0x115df0, population route 0x11a820, 0x3191b0, builder/population helpers 0x2fa7b0, 0x2fb850 and 0x2fc270, the recovered callback/builder family, and all remaining unproved geometry/World/helper targets. Complete exception cleanup, direct non-call shared stores, unindexed descendant control flow and external readers are also unresolved. No unexplained target is silently classified as safe.

The following paragraphs are the complete grouped callsite index. Each paragraph names its callee or import slot, classification and static site count. Recorded sites come from the original supplied call records; additional binary sites were recovered from the original executable's switch-case control flow. The INT3 at 0x3022c2 is deliberately excluded from these counts.

Target 0x7080; runtime; 2 callsites. Recorded sites: 30bb1d, 3172d3.

Target 0x9cd0; unproved; 5 callsites. Recorded sites: 31698f, 316afc, 316c84, 316d4e. Additional binary sites: 315f52.

Target 0x9db0; unproved; 1 callsite. Recorded sites: 308ca1.

Target 0xa100; unproved; 1 callsite. Recorded sites: 2ff7be.

Target 0xe770; partial; 2 callsites. Recorded sites: 31636a, 31637f.

Target 0xeb90; partial; 5 callsites. Recorded sites: 30149d, 3018d9, 301c6f, 30231f, 305f85.

Target 0x12a80; unproved; 4 callsites. Recorded sites: 30797b, 3079a1, 3079c8, 3079ec.

Target 0x16fb0; unproved; 20 callsites. Recorded sites: 307c5a, 307cb4, 30a732, 30ab29, 30d2f1, 30d327, 30d4b8, 30ec11, 30ec47, 30edeb, 310553, 310589, 310716, 311e77, 311ead, 312051, 312411, 312556, 312594, 31261f.

Target 0x16fd0; unproved; 50 callsites. Recorded sites: 302e78, 30309e, 3030ca, 303626, 303813, 304010, 3040b2, 304e27, 304e3e, 3052e6, 30555a, 3057c5, 305a38, 30607a, 3061c4, 3062db, 3065aa, 30664c, 306782, 3067a6, 306fde, 30703b, 3070c8, 307d56, 307df8, 309c17, 309c3a, 309c64, 309c90, 30a47d, 30a723, 30a8da, 30ab1a, 30d2e2, 30d318, 30d4a9, 30ec02, 30ec38, 30eddc, 310544, 31057a, 310707, 311e68, 311e9e, 312042, 312402, 312585, 312610, 312818. Additional binary sites: 315555.

Target 0x171c0; unproved; 6 callsites. Recorded sites: 3030b0, 305efd, 3070e5, 312eb3, 313a8c, 313f24.

Target 0x17290; unproved; 61 callsites. Recorded sites: 30c00a, 30c039, 30c04b, 30c172, 30c197, 30c3a0, 30c3b2, 30c4f6, 30c708, 30c73b, 30c74d, 30c874, 30c899, 30cacb, 30cadd, 30cc1e, 30cdea, 30ce19, 30ce2b, 30cf77, 30d284, 30d3da, 30d403, 30d671, 30d8d7, 30da42, 30dfe0, 30e14e, 30eaad, 30ef06, 30f255, 30f267, 30f296, 30f3b5, 30f3e4, 30f5d5, 30f5e7, 30f734, 30f977, 30f989, 30f9b7, 30fad6, 30fafc, 30fcf9, 30fd0b, 30fe50, 310045, 310057, 310082, 31019e, 3104d1, 310614, 310661, 3108bb, 310b77, 310cc4, 311293, 3113dd, 311cfc, 31214d, 316dd7.

Target 0x172b0; unproved; 4 callsites. Recorded sites: 3030dc, 3065c0, 307023, 307078.

Target 0x17300; unproved; 73 callsites. Recorded sites: 30c371, 30c4cd, 30ca9c, 30cbf9, 30cf52, 30d1a3, 30d5ae, 30d909, 30d91b, 30da6a, 30dc44, 30dc76, 30dc88, 30dda3, 30ddcd, 30e015, 30e027, 30e174, 30e377, 30e3a8, 30e3ba, 30e4d4, 30e4fb, 30e6c7, 30e6f9, 30e70b, 30e82e, 30e854, 30eba4, 30ecfc, 30ed36, 30efde, 30f619, 30f75e, 30fd36, 30fe76, 3101c9, 3103ee, 3107f1, 310b36, 310b48, 310c95, 310eb4, 310ec6, 310ef5, 311013, 31103a, 31124f, 311261, 3113b3, 3115df, 3115f1, 31161c, 311737, 31175d, 31192f, 311941, 31196d, 311a8c, 311ab8, 311df5, 311f4b, 311f9c, 31222c, 31236d, 3123a4, 314763, 3147c2, 3161a8, 3161cb, 316e01. Additional binary sites: 314e93, 314eba.

Target 0x17730; unproved; 2 callsites. Recorded sites: 306791, 3067b5.

Target 0x177c0; unproved; 1 callsite. Additional binary sites: 315b7e.

Target 0x177e0; unproved; 4 callsites. Recorded sites: 304a4d, 312e41, 31700e, 317029.

Target 0x2b630; unproved; 1 callsite. Additional binary sites: 315997.

Target 0x2e0c0; unproved; 15 callsites. Recorded sites: 3006f7, 300738, 300867, 3008e7, 3008fd, 300b05, 30157c, 3016e4, 301b3f, 301c1d, 301cd7, 30a04a, 30acd2, 30b0fa. Additional binary sites: 315b79.

Target 0x354a0; unproved; 5 callsites. Recorded sites: 318f86, 318f93, 318fa0, 318fad, 318fba.

Target 0x358e0; unproved; 8 callsites. Recorded sites: 2ffc3b, 313dfa, 314967, 31610f, 3184d7, 318761, 318a87. Additional binary sites: 31561a.

Target 0x35ce0; unproved; 5 callsites. Recorded sites: 313de4, 314984, 3184bc, 318748, 318ab7.

Target 0x35ea0; unproved; 62 callsites. Recorded sites: 300161, 300178, 30018e, 3001a5, 302807, 3031c1, 305226, 305240, 305499, 3054b4, 305706, 30571f, 305978, 305992, 305bcf, 305e77, 30c0a6, 30c1f2, 30c40d, 30c551, 30c7a8, 30c8f4, 30cb38, 30cc79, 30ce86, 30cfd2, 30d976, 30dac5, 30dce3, 30de28, 30e082, 30e1cf, 30e415, 30e556, 30e766, 30e8af, 30f2f1, 30f43f, 30f674, 30f7b9, 30fa12, 30fb57, 30fd91, 30fed1, 3100dd, 310224, 310bd2, 310d1f, 310f50, 311095, 3112ee, 311438, 311677, 3117b8, 3119c8, 311b13, 314937, 316226, 316e6a, 3180da. Additional binary sites: 3158ff, 315a29.

Target 0x364e0; unproved; 9 callsites. Recorded sites: 3051d7, 305448, 3056b4, 305924, 305b7e, 305e24, 316071, 317266, 3181c2.

Target 0x4ce70; destination; 2 callsites. Recorded sites: 30b02f. Additional binary sites: 315ae4.

Target 0x4e070; destination; 5 callsites. Recorded sites: 302db4, 306c58, 306c67, 306c76. Additional binary sites: 315b9d.

Target 0x511e0; unproved; 5 callsites. Recorded sites: 3034d4, 317599, 3176de, 317865, 317bc6.

Target 0x53e80; unproved; 10 callsites. Recorded sites: 30346e, 317536, 317677, 3177ff, 317b60, 317cb8, 317d69, 317e16, 318c3d, 318daf.

Target 0x65750; unproved; 1 callsite. Recorded sites: 318e72.

Target 0x675c0; partial; 1 callsite. Recorded sites: 312670.

Target 0x69860; partial; 1 callsite. Recorded sites: 3126a8.

Target 0x69b30; unproved; 4 callsites. Recorded sites: 3021d8, 30249d, 305d71, 306371.

Target 0x69c60; unproved; 56 callsites. Recorded sites: 2ff9c9, 30269c, 3035e0, 3035f9, 304ee8, 304f52, 304fc0, 30502b, 305183, 3053fe, 305668, 3058d4, 306866, 3068d1, 30693c, 3069a7, 306a8b, 306aa6, 30718e, 3071b1, 3071cd, 3071eb, 30740b, 3074df, 3075b2, 307687, 3077c4, 3077da, 3077f0, 307806, 3078e6, 3078fd, 307914, 30792b, 308cfe, 308d15, 308d2a, 308d42, 308d56, 308d6b, 308d83, 308d97, 308dac, 30ba5a, 312ed4, 312ee6, 312f00, 312f12, 312f25, 312f37, 312f4a, 312f5d, 3160b4, 316b0b, 317232, 318353.

Target 0x6d240; unproved; 7 callsites. Recorded sites: 3052f5, 305569, 3057d4, 305a47, 306089, 3061d3, 3062ea.

Target 0x82450; partial; 1 callsite. Recorded sites: 318f79.

Target 0x83b20; unproved; 4 callsites. Recorded sites: 3050e0, 30b9b8, 30bab5, 30bb07.

Target 0x83c30; unproved; 7 callsites. Recorded sites: 305351, 3055c5, 305830, 305aa3, 306101, 30622f, 306346.

Target 0x93480; unproved; 2 callsites. Recorded sites: 306e86, 306ea1.

Target 0x9afa0; unproved; 1 callsite. Recorded sites: 3180bb.

Target 0x9beb0; unproved; 2 callsites. Recorded sites: 316feb, 316ff5.

Target 0x9bee0; unproved; 9 callsites. Recorded sites: 3051f0, 305461, 3056cd, 30593d, 305b97, 305e3d, 31608a, 317281, 3181dd.

Target 0x9bf20; unproved; 1 callsite. Recorded sites: 317007.

Target 0xba820; unproved; 30 callsites. Recorded sites: 300075, 300121, 3002db, 3003f1, 300517, 300665, 301538, 301784, 30196d, 301fee, 302088, 3023bd, 305f26, 309eec, 309fea, 30a21c, 30a2af, 30a3c6, 30a420, 30a878, 30ac80, 30b14b, 30b1e8, 313bc6, 313c5a, 313da3, 313e8d. Additional binary sites: 315a9a, 315bb7, 315be6.

Target 0xba8c0; unproved; 11 callsites. Recorded sites: 302a05, 302d45, 303287, 303366, 3035b8, 303a4e, 304e08, 306445, 3143ac, 3145c4. Additional binary sites: 315c8b.

Target 0xba9a0; unproved; 1 callsite. Additional binary sites: 315538.

Target 0xbaa80; unproved; 1 callsite. Recorded sites: 30317e.

Target 0xbaca0; unproved; 13 callsites. Recorded sites: 302cc2, 3042a6, 3043f0, 312d24, 312d48, 313280, 31348a, 3136ad, 3138c6. Additional binary sites: 315d5c, 315d81, 315da7, 315dcb.

Target 0xbade0; unproved; 15 callsites. Recorded sites: 302c1f, 304325, 304448, 3132b1, 3134bc, 3136e6, 3138f4, 314443, 31446f, 31449d, 3144ca, 314659, 31467c, 3146a1, 3146c5.

Target 0xbaef0; unproved; 2 callsites. Recorded sites: 307666, 30773e.

Target 0xbb170; unproved; 2 callsites. Recorded sites: 3074bf, 307594.

Target 0xbb400; unproved; 45 callsites. Recorded sites: 3036d2, 30376d, 3038bf, 303958, 304119, 304160, 3041aa, 3045df, 306c23, 307169, 30729e, 30731a, 30739b, 3073d3, 307488, 30755a, 30762f, 307703, 309daf, 309dd9, 30b5d3, 30b75a, 30b7a9, 30bd12, 30bf08, 30d0c2, 30d512, 30d7d5, 30e9b6, 30ee4f, 30f14b, 31031f, 310771, 310a2e, 311c18, 3120b2, 31290f, 312a35, 312d74, 313a57, 313adc, 313f98, 314083, 31418e. Additional binary sites: 315d33.

Target 0xbb9e0; unproved; 8 callsites. Recorded sites: 309ec3, 309fc1, 30a1ee, 30a281, 30b1bf, 313b9d, 313c31, 313d7a.

Target 0xbbcd0; unproved; 1 callsite. Recorded sites: 303d7c.

Target 0xbbfe0; unproved; 1 callsite. Recorded sites: 301f14.

Target 0xbc340; unproved; 3 callsites. Recorded sites: 30b368, 31285d, 312c9d.

Target 0xbc400; unproved; 34 callsites. Recorded sites: 302f43, 302f75, 302fb9, 302fe6, 303013, 303040, 303c49, 303c77, 30464e, 304760, 304794, 3047cc, 3047fa, 30482d, 30485c, 30488f, 3048be, 304a3f, 304a86, 304aaf, 304b8d, 304c56, 304d04, 304ed8, 30b728, 30d0ee, 30d53e, 30e9e2, 30ee81, 31034b, 31079e, 311c44, 3120e5, 313fd1.

Target 0xbc5b0; unproved; 13 callsites. Recorded sites: 302e03, 302e47, 302eb8, 302ee6, 303120, 303ba1, 303cc7, 303e55, 3044e7, 30471e, 304940, 304975. Additional binary sites: 3155e7.

Target 0xbc990; unproved; 5 callsites. Recorded sites: 302f11, 303bcd, 303cf7, 303dd5, 303ef5.

Target 0xbcc90; unproved; 3 callsites. Recorded sites: 302b46, 303b43. Additional binary sites: 3155ab.

Target 0xbd110; unproved; 2 callsites. Recorded sites: 300027, 305eeb.

Target 0xbd590; unproved; 2 callsites. Recorded sites: 3031f9, 30321d.

Target 0xbd7c0; unproved; 1 callsite. Recorded sites: 30b32c.

Target 0xbe4d0; unproved; 1 callsite. Recorded sites: 30b41d.

Target 0xbeac0; unproved; 9 callsites. Recorded sites: 302c72, 3042d2, 30441c, 3049fd, 312d9e, 31324f, 313458, 313674, 313898.

Target 0xbebe0; unproved; 26 callsites. Recorded sites: 302dca, 302e19, 302e8a, 3030ee, 3032c3, 3033a6, 303638, 3037ab, 303825, 303996, 303d59, 3040d1, 30412f, 304176, 306a68, 306beb, 3070f7, 309524, 30a0f6, 30a111, 30b606, 31282a, 312ec5, 3139e0, 313a9e. Additional binary sites: 31556a.

Target 0xbf600; unproved; 14 callsites. Recorded sites: 309f15, 30a013, 30a23d, 30a2d0, 30a3e7, 30a8a2, 30b064, 30b16c, 30b210, 30b390, 313bea, 313c7e, 313dc7, 313eb4.

Target 0xbf840; unproved; 5 callsites. Recorded sites: 309d73, 30b5a2, 3127a0, 312bba, 312c64.

Target 0xbf8c0; unproved; 20 callsites. Recorded sites: 302aaa, 303ade, 303e10, 304209, 304398, 304490, 3046d4, 3048fb, 304e79, 3066d2, 30673f, 3070a5, 30a153, 30bc72, 312e93, 313f5a, 31401f, 314418, 314632. Additional binary sites: 315cfb.

Target 0xc0030; unproved; 1 callsite. Recorded sites: 302ae8.

Target 0xc0430; unproved; 4 callsites. Recorded sites: 304f40, 304fb1, 30d12f, 30ea24.

Target 0xc0880; unproved; 4 callsites. Recorded sites: 305018, 30508c, 31038d, 311c86.

Target 0xc0cd0; unproved; 1 callsite. Recorded sites: 3127f8.

Target 0xc1540; unproved; 1 callsite. Recorded sites: 3031a8.

Target 0xc1610; unproved; 1 callsite. Recorded sites: 3039c8.

Target 0xc19b0; unproved; 1 callsite. Recorded sites: 3037dd.

Target 0xc1d60; unproved; 21 callsites. Recorded sites: 303307, 303401, 3036a6, 30373f, 303893, 30392b, 304eae, 306832, 306a4d, 306aec, 306b2e, 306b70, 306bb9, 30b300, 30b3f3, 3128bd, 312bf4, 31321b, 313423, 313638, 313867.

Target 0xc2180; unproved; 1 callsite. Recorded sites: 30b2b4.

Target 0xc7c40; unproved; 4 callsites. Recorded sites: 306ff8, 307011, 30704d, 307066.

Target 0xcb840; unproved; 109 callsites. Recorded sites: 303545, 304b1f, 304bf8, 304cba, 304d5d, 305360, 3055d4, 30583f, 305ab2, 30623e, 306355, 30bbb3, 30c06f, 30c1bb, 30c3d6, 30c51a, 30c771, 30c8bd, 30cb01, 30cc42, 30ce4f, 30cf9b, 30d1c7, 30d2a8, 30d427, 30d5d2, 30d695, 30d93f, 30da8e, 30dcac, 30ddf1, 30e04b, 30e198, 30e3de, 30e51f, 30e72f, 30e878, 30ead1, 30ebc8, 30ed5a, 30ef2a, 30f002, 30f2ba, 30f408, 30f63d, 30f782, 30f9db, 30fb20, 30fd5a, 30fe9a, 3100a6, 3101ed, 310427, 31050a, 310685, 31082e, 3108f1, 310b9b, 310ce8, 310f19, 31105e, 3112b7, 311401, 311640, 311781, 311991, 311adc, 311d35, 311e2e, 311fc0, 31218a, 312262, 3123c8, 3129ee, 312b14, 31410d, 314273, 314345, 31455d, 3147fe, 3161ef, 316828, 3169de, 316c48, 316c97, 316cdc, 316e25, 3170bb, 3175c1, 317708, 3178d4, 317bf3, 317ceb, 317d98, 317e45, 317ef1, 318176, 318683, 3187d9, 31886e, 318998, 318a57, 318dd5. Additional binary sites: 314ede, 3156b5, 315742, 3157cf, 31585c, 315e39.

Target 0xcb860; unproved; 94 callsites. Recorded sites: 30353a, 304b10, 304be9, 304cab, 304d4e, 30533b, 3055af, 30581a, 305a8d, 3060eb, 306219, 306330, 307a62, 30bba4, 30c060, 30c1ac, 30c3c7, 30c50b, 30c762, 30c8ae, 30caf2, 30cc33, 30ce40, 30cf8c, 30d1b8, 30d299, 30d418, 30d5c3, 30d686, 30d930, 30da7f, 30dc9d, 30dde2, 30e03c, 30e189, 30e3cf, 30e510, 30e720, 30e869, 30eac2, 30ebb9, 30ed4b, 30ef1b, 30eff3, 30f2ab, 30f3f9, 30f62e, 30f773, 30f9cc, 30fb11, 30fd4b, 30fe8b, 310097, 3101de, 310418, 3104fb, 310676, 31081f, 3108e2, 310b8c, 310cd9, 310f0a, 31104f, 3112a8, 3113f2, 311631, 311772, 311982, 311acd, 311d26, 311e1f, 311fb1, 31217b, 312253, 3123b9, 3129df, 312b05, 3140fe, 314264, 314336, 31454e, 3147d7, 3161e0, 316421, 3164e4, 316e16, 3170ac, 3178c9. Additional binary sites: 314ecf, 3156a6, 315733, 3157c0, 31584d, 315e2a.

Target 0xcb990; unproved; 331 callsites. Recorded sites: 3034fb, 303510, 303525, 304acc, 304ae3, 304afb, 304ba8, 304bbb, 304bd4, 304c65, 304c7f, 304c96, 304d13, 304d26, 304d39, 305303, 305315, 305326, 305577, 305588, 30559a, 3057e2, 3057f4, 305805, 305a55, 305a66, 305a78, 305c3e, 305c6e, 305c9d, 305ccc, 305cfc, 305d2b, 3060b3, 3060c4, 3060d6, 3061e1, 3061f2, 306204, 3062f8, 306309, 30631b, 307a23, 307a38, 307a4d, 30bb65, 30bb7a, 30bb8f, 30bc15, 30bfde, 30bff3, 30c026, 30c0c9, 30c141, 30c158, 30c184, 30c215, 30c34a, 30c35f, 30c38d, 30c430, 30c4a8, 30c4bb, 30c4e3, 30c574, 30c6da, 30c6f1, 30c728, 30c7cb, 30c843, 30c85a, 30c886, 30c917, 30ca77, 30ca8a, 30cab8, 30cb5b, 30cbd4, 30cbe7, 30cc0b, 30cc9c, 30cdbe, 30cdd3, 30ce06, 30cea9, 30cf26, 30cf3b, 30cf64, 30cff5, 30d167, 30d17e, 30d190, 30d24c, 30d25f, 30d271, 30d38d, 30d39f, 30d3f0, 30d471, 30d56c, 30d585, 30d59b, 30d63a, 30d64c, 30d65e, 30d8ab, 30d8c0, 30d8f6, 30d999, 30da11, 30da28, 30da57, 30dae8, 30dc1d, 30dc32, 30dc63, 30dd06, 30dd7e, 30dd91, 30ddba, 30de4b, 30dfb2, 30dfc9, 30e002, 30e0a5, 30e11d, 30e134, 30e161, 30e1f2, 30e352, 30e365, 30e395, 30e438, 30e4af, 30e4c2, 30e4e8, 30e579, 30e69b, 30e6b0, 30e6e6, 30e789, 30e802, 30e817, 30e841, 30e8d2, 30ea5d, 30ea74, 30ea86, 30eb57, 30eb6a, 30eb7c, 30ecab, 30ecbd, 30ed0e, 30eda4, 30eeaf, 30eec8, 30eede, 30ef92, 30efa4, 30efb6, 30f222, 30f242, 30f27c, 30f314, 30f38c, 30f3a2, 30f3ca, 30f462, 30f5a6, 30f5c2, 30f5ff, 30f697, 30f70f, 30f721, 30f74c, 30f7dc, 30f944, 30f964, 30f99d, 30fa35, 30faad, 30fac3, 30faea, 30fb7a, 30fcca, 30fce6, 30fd1f, 30fdb4, 30fe2b, 30fe3d, 30fe64, 30fef4, 310016, 310032, 31006b, 310100, 310179, 31018b, 3101b2, 310247, 3103c5, 3103db, 310403, 3104ac, 3104be, 3104e6, 3105ef, 310601, 310626, 3106cf, 3107cc, 3107de, 31080a, 310896, 3108a8, 3108cd, 310b04, 310b23, 310b5d, 310bf5, 310c6d, 310c82, 310caa, 310d42, 310e82, 310ea1, 310edb, 310f73, 310feb, 311000, 311028, 3110b8, 31121b, 31123c, 311279, 311311, 311389, 3113a0, 3113cb, 31145b, 3115af, 3115cc, 311605, 31169a, 311711, 311724, 31174b, 3117db, 3118fd, 31191c, 311956, 3119eb, 311a64, 311a79, 311aa1, 311b36, 311cbf, 311cd5, 311d11, 311dbb, 311dcd, 311e0a, 311f11, 311f23, 311f5d, 31200a, 312113, 312125, 312166, 3121f2, 312204, 31223e, 312330, 312342, 31237f, 312475, 312950, 312962, 3129a3, 312a76, 312aa2, 312adb, 3140bf, 3140d4, 3140e9, 314223, 31423a, 31424f, 3142f9, 31430c, 314321, 314511, 314524, 314539, 3146f6, 31471f, 31478c, 316184, 316196, 3161b9, 3163be, 3163d3, 3163fa, 316481, 316496, 3164bd, 316dae, 316dc0, 316dea, 316eb0, 316f51, 317072, 317084, 317097, 31788c, 31789e, 3178b4. Additional binary sites: 314e79, 314ea8, 31567c, 315691, 315706, 31571e, 315796, 3157ab, 315820, 315838, 315df0, 315e02, 315e15.

Target 0xcc140; unproved; 2 callsites. Recorded sites: 318fc7, 318fd4.

Target 0x10cee0; unproved; 2 callsites. Recorded sites: 2ffb36, 31731b.

Target 0x10d2e0; leaf; 1 callsite. Recorded sites: 31730b.

Target 0x10d310; leaf; 1 callsite. Recorded sites: 3172fb.

Target 0x115db0; unproved; 3 callsites. Recorded sites: 316a16, 316b40. Additional binary sites: 315e76.

Target 0x115df0; partial; 1 callsite. Recorded sites: 2ffba5.

Target 0x118a60; partial; 10 callsites. Recorded sites: 303429, 3174c9, 317633, 317792, 31796a, 317c6d, 317d1e, 317dcb, 318a29, 318ccd.

Target 0x118c20; destination; 8 callsites. Recorded sites: 31419a, 316c71, 31810d, 31836c, 318796, 318845, 318952. Additional binary sites: 314dcb.

Target 0x1194b0; partial; 52 callsites. Recorded sites: 302864, 304b4a, 304c23, 304cd1, 304d74, 30bd34, 30c0f9, 30c460, 30c7fb, 30cb87, 30ced9, 30d208, 30d5ec, 30d6af, 30d9c9, 30dd36, 30e0d5, 30e464, 30e7b5, 30eb14, 30ef44, 30f01c, 30f344, 30f6c7, 30fa65, 30fde0, 31012c, 310468, 310848, 31090b, 310c25, 310fa3, 311341, 3116c6, 311a17, 311d78, 3121a4, 31227c, 31411c, 314354, 31456c, 3148cc, 316958, 316ac5, 316d17, 3170e0, 318040. Additional binary sites: 3156e1, 31576e, 3157fb, 31588c, 315f1b.

Target 0x119530; Zone attachment; 10 callsites. Recorded sites: 303560, 3175f3, 317761, 317907, 317bfe, 317d00, 317dad, 317e5b, 318c4c, 318de4.

Target 0x119800; unproved; 1 callsite. Recorded sites: 306e2e.

Target 0x11a820; partial; 1 callsite. Recorded sites: 317145.

Target 0x1299a0; unproved; 9 callsites. Recorded sites: 316bd8, 3180f9, 31833c, 318557, 3186e1, 318835, 3188fa, 318a03, 318c79.

Target 0x129b50; partial; 8 callsites. Recorded sites: 3053a7, 30561b, 305888, 305afb, 305cad, 30626c, 316260, 316fa3.

Target 0x129c00; unproved; 8 callsites. Recorded sites: 31428a, 316cae, 3181a5, 318693, 3187e9, 3188ae, 3189c7. Additional binary sites: 314ef5.

Target 0x12a600; unproved; 3 callsites. Recorded sites: 306eb8, 306f3e, 30b85c.

Target 0x12a680; unproved; 1 callsite. Recorded sites: 318298.

Target 0x131540; partial; 1 callsite. Recorded sites: 316872.

Target 0x13a960; unproved; 24 callsites. Recorded sites: 30bfcd, 30c339, 30c6c9, 30ca66, 30cdad, 30d1e1, 30d89a, 30dc0c, 30dfa1, 30e341, 30e68a, 30eaeb, 30f211, 30f595, 30f933, 30fcb9, 310005, 310441, 310af3, 310e71, 31120a, 31159e, 3118ec, 311d4f.

Target 0x1420f0; partial; 4 callsites. Recorded sites: 303444, 3174db, 3177a4, 3179a9.

Target 0x142ee0; destination; 11 callsites. Recorded sites: 3024b8, 30297a. Additional binary sites: 314f0e, 314f5d, 314f94, 315193, 3151c4, 3153ca, 315432, 315470, 3158ab.

Target 0x145b30; partial; 1 callsite. Additional binary sites: 3153f0.

Target 0x145cf0; partial; 1 callsite. Additional binary sites: 315458.

Target 0x145eb0; partial; 1 callsite. Additional binary sites: 3151e4.

Target 0x145fa0; partial; 3 callsites. Additional binary sites: 314f80, 3151b3, 3154a2.

Target 0x146090; partial; 1 callsite. Recorded sites: 30299c.

Target 0x146450; partial; 1 callsite. Additional binary sites: 314f3a.

Target 0x146560; partial; 2 callsites. Recorded sites: 302560. Additional binary sites: 314fc9.

Target 0x16e690; destination; 2 callsites. Recorded sites: 30794b, 30bd96.

Target 0x25e940; unproved; 5 callsites. Recorded sites: 303b58, 307436, 307503, 3075dd, 3076ab.

Target 0x25e9a0; unproved; 6 callsites. Recorded sites: 303d12, 3063f2, 306f71, 3094ab, 30b192, 313923.

Target 0x25ea00; unproved; 1 callsite. Recorded sites: 30774e.

Target 0x25ea50; unproved; 2 callsites. Recorded sites: 3064e4, 3123d7.

Target 0x25eb30; unproved; 2 callsites. Recorded sites: 2ff9d0, 30ba63.

Target 0x25eb60; unproved; 4 callsites. Recorded sites: 307415, 3074e9, 3075bc, 307691.

Target 0x25ebc0; unproved; 2 callsites. Recorded sites: 302afb, 30b0aa.

Target 0x25ec30; unproved; 2 callsites. Recorded sites: 302b5b, 30645c.

Target 0x25eca0; unproved; 1 callsite. Recorded sites: 302a5e.

Target 0x25ece0; unproved; 3 callsites. Recorded sites: 302a56, 303aa3, 3064bc.

Target 0x25f540; unproved; 33 callsites. Recorded sites: 3050a4, 307114, 307771, 30778f, 30779b, 307f8e, 307fc2, 307ff8, 30802b, 308062, 30809c, 3080d6, 308111, 3081c4, 3081fa, 30822c, 30825f, 3082a4, 3082bb, 3082d5, 3082ec, 308303, 308317, 30832b, 30833e, 309043, 309052, 30919c, 3091ab, 3092e9, 3092f8, 309433, 30943e.

Target 0x25f5f0; unproved; 1 callsite. Recorded sites: 30ba6e.

Target 0x25f6a0; unproved; 18 callsites. Recorded sites: 3063db, 3077ab, 313072, 3130a8, 3130fa, 31314e, 3132bc, 3132f2, 313344, 313398, 3134ca, 313500, 313555, 3135ad, 3136fd, 313733, 313785, 3137dd.

Target 0x25f790; unproved; 1 callsite. Recorded sites: 302877.

Target 0x25f840; unproved; 2 callsites. Recorded sites: 303d44, 3139ce.

Target 0x25fff0; unproved; 1 callsite. Recorded sites: 3064c4.

Target 0x2600f0; unproved; 19 callsites. Recorded sites: 302a31, 302d70, 3032b1, 303394, 303799, 303984, 303a7c, 306497, 307c4b, 307c9f, 30950a, 309966, 3099c5, 309b61, 309b99, 309cbb, 3143d6, 3145ee. Additional binary sites: 315cb7.

Target 0x260a00; unproved; 2 callsites. Recorded sites: 306fb3, 30a0e3.

Target 0x260d50; destination; 12 callsites. Additional binary sites: 314a8b, 314ac4, 314afd, 314b36, 314b6f, 314ba8, 314c3d, 314cbd, 314d33, 314d8e, 3154d8, 315506.

Target 0x2626e0; partial; 1 callsite. Additional binary sites: 314b1d.

Target 0x263210; partial; 1 callsite. Additional binary sites: 314b8f.

Target 0x263ea0; partial; 1 callsite. Additional binary sites: 314ae4.

Target 0x264670; partial; 2 callsites. Additional binary sites: 314aab, 315526.

Target 0x2651f0; partial; 2 callsites. Additional binary sites: 314c64, 314d5a.

Target 0x265840; partial; 2 callsites. Additional binary sites: 314ce4, 314db5.

Target 0x265e70; partial; 2 callsites. Additional binary sites: 314b56, 3154f8.

Target 0x266ad0; partial; 1 callsite. Additional binary sites: 314bc8.

Target 0x282660; unproved; 1 callsite. Recorded sites: 318ebe.

Target 0x295d00; unproved; 3 callsites. Recorded sites: 309cdd, 309cf0, 309d03.

Target 0x29baf0; unproved; 1 callsite. Recorded sites: 318edb.

Target 0x29bcb0; unproved; 1 callsite. Recorded sites: 318eeb.

Target 0x29be70; unproved; 7 callsites. Recorded sites: 31623a, 317721. Additional binary sites: 314e64, 315664, 3156f1, 31577e, 31580b.

Target 0x29d170; unproved; 1 callsite. Recorded sites: 3139a3.

Target 0x2a0840; unproved; 3 callsites. Recorded sites: 2ffc6b. Additional binary sites: 31511f, 31533a.

Target 0x2a1d10; unproved; 12 callsites. Recorded sites: 2ff9e5, 30a44b, 30a48f, 30a4a4, 30a4ba, 30a4d0, 30a4eb, 30a506, 30a521, 30a53c, 30b7f6, 30ba8d.

Target 0x2a2550; unproved; 1 callsite. Recorded sites: 2ff96a.

Target 0x2a2ab0; unproved; 1 callsite. Recorded sites: 2ff979.

Target 0x2a2b00; unproved; 1 callsite. Recorded sites: 2ff988.

Target 0x2a2c50; unproved; 1 callsite. Recorded sites: 301a46.

Target 0x2a3140; unproved; 3 callsites. Recorded sites: 300656, 301529, 30195e.

Target 0x2a3e30; unproved; 1 callsite. Recorded sites: 301775.

Target 0x2a44a0; unproved; 1 callsite. Recorded sites: 2ffc7b.

Target 0x2a8290; unproved; 12 callsites. Recorded sites: 30264c, 308493, 3084b8, 3084e0, 308505, 30852b, 308555, 308579, 3085a4, 30b986, 3167d9, 3182d8.

Target 0x2a9df0; unproved; 2 callsites. Recorded sites: 2ffae4, 306cef.

Target 0x2aa250; unproved; 9 callsites. Recorded sites: 302685, 3086e4, 308710, 30873f, 30876c, 308796, 3087c0, 3087ea, 30881a.

Target 0x2aa270; unproved; 1 callsite. Recorded sites: 2ffab1.

Target 0x2aadd0; unproved; 11 callsites. Recorded sites: 3088bb, 3088db, 3088fb, 308918, 308938, 308958, 308978, 308998, 3089b8, 30a18e, 30b26d.

Target 0x2ab0a0; unproved; 18 callsites. Recorded sites: 302663, 302678, 3085f9, 308613, 30862d, 308647, 308661, 30867b, 308695, 3086af, 3086d2, 3086fb, 30872a, 308759, 308783, 3087ad, 3087d7, 308801.

Target 0x2abb60; leaf; 1 callsite. Recorded sites: 2ffd30.

Target 0x2ada20; unproved; 9 callsites. Recorded sites: 2ffb57, 2ffb8e, 316938, 316bb7, 3180ab, 318138, 318f43. Additional binary sites: 314e4a, 315f02.

Target 0x2ae6d0; unproved; 1 callsite. Recorded sites: 30057c.

Target 0x2af7b0; unproved; 3 callsites. Recorded sites: 306d22, 3126dd, 317344.

Target 0x2b40b0; unproved; 1 callsite. Recorded sites: 30b121.

Target 0x2b42e0; unproved; 1 callsite. Recorded sites: 316d5d.

Target 0x2b4320; unproved; 2 callsites. Recorded sites: 30037d, 313e46.

Target 0x2b4410; unproved; 1 callsite. Recorded sites: 316b1d.

Target 0x2b4450; unproved; 2 callsites. Recorded sites: 2fff9e, 301afe.

Target 0x2b5820; unproved; 1 callsite. Recorded sites: 3146e5.

Target 0x2b5960; unproved; 3 callsites. Recorded sites: 2ffcae, 318500, 318b70.

Target 0x2b7730; unproved; 1 callsite. Recorded sites: 301ef2.

Target 0x2b8010; unproved; 2 callsites. Recorded sites: 2ffa40, 300428.

Target 0x2b8030; unproved; 1 callsite. Recorded sites: 3173de.

Target 0x2b8b10; unproved; 9 callsites. Recorded sites: 307b9b, 307ecd, 307ee3, 307ef9, 307f0f, 307f26, 307f3d, 307f54, 307f6b.

Target 0x2b9080; unproved; 1 callsite. Recorded sites: 2ffb6d.

Target 0x2b92f0; unproved; 3 callsites. Recorded sites: 2ffe1e. Additional binary sites: 3150b3, 3152ce.

Target 0x2b9750; unproved; 4 callsites. Recorded sites: 2ffce8, 318ad6. Additional binary sites: 315027, 315242.

Target 0x2ba200; unproved; 1 callsite. Recorded sites: 2ffa54.

Target 0x2ba2b0; unproved; 1 callsite. Recorded sites: 300460.

Target 0x2ba5f0; unproved; 1 callsite. Recorded sites: 3004c9.

Target 0x2bab80; unproved; 1 callsite. Recorded sites: 300492.

Target 0x2bb060; unproved; 1 callsite. Recorded sites: 2ffb21.

Target 0x2bb310; unproved; 3 callsites. Recorded sites: 2ffedc. Additional binary sites: 315165, 315380.

Target 0x2bb7e0; unproved; 3 callsites. Recorded sites: 2ffcc4. Additional binary sites: 315009, 315224.

Target 0x2c30b0; leaf; 1 callsite. Recorded sites: 3050f6.

Target 0x2c36f0; leaf; 4 callsites. Recorded sites: 30b91f, 30b92d, 318249, 318258.

Target 0x2d01a0; unproved; 3 callsites. Recorded sites: 2ffe6d. Additional binary sites: 31507b, 315296.

Target 0x2d34b0; unproved; 5 callsites. Recorded sites: 313b2b, 313ca2, 313cd4, 313d10, 313d41.

Target 0x2d36f0; unproved; 1 callsite. Recorded sites: 2fff0b.

Target 0x2d3b60; unproved; 1 callsite. Recorded sites: 31826e.

Target 0x2d3d90; unproved; 3 callsites. Recorded sites: 3003a3, 30203a, 313e64.

Target 0x2d4730; unproved; 11 callsites. Recorded sites: 2ffd5e, 30520a, 30547c, 3056e7, 305958, 305bb1, 305e58, 3160db, 318229. Additional binary sites: 314ff9, 315214.

Target 0x2d82c0; unproved; 22 callsites. Recorded sites: 306849, 306899, 306904, 30696f, 3069dd, 307a12, 307bce, 307f81, 307fb5, 307feb, 308021, 308055, 30808f, 3080c9, 308104, 3081b7, 3081ed, 30821f, 308255, 309e8a, 309f88, 312516.

Target 0x2d8630; unproved; 1 callsite. Recorded sites: 3028dd.

Target 0x2d86e0; unproved; 1 callsite. Recorded sites: 31393e.

Target 0x2d8790; unproved; 5 callsites. Recorded sites: 307bb9, 30b46e, 30b497, 30b4b9, 30b4e3.

Target 0x2d87f0; unproved; 54 callsites. Recorded sites: 302958, 3029c5, 302cfe, 3032d5, 3033d0, 3034ec, 3035ca, 30364b, 3036e5, 303838, 3038d2, 3039f6, 304dc9, 304f02, 304f67, 304fd5, 305040, 3063d0, 306884, 3068ef, 30695a, 3069c5, 307217, 30724f, 3072c4, 30734c, 30781c, 307835, 30784e, 307867, 307881, 30789b, 3078b5, 3078cf, 30815c, 308171, 308186, 30819c, 312f77, 312f90, 312fad, 312fca, 312fe8, 313005, 313022, 313043, 313061, 31404e, 314159, 3142b8, 314374, 31458c, 316d77, 31787e.

Target 0x2d8e00; unproved; 1 callsite. Recorded sites: 3002c8.

Target 0x2d9b60; unproved; 7 callsites. Recorded sites: 2ffc98, 313e1c, 31614a, 3184f1, 31877b, 318b25, 318baa.

Target 0x2d9da0; unproved; 1 callsite. Recorded sites: 303d8d.

Target 0x2d9db0; unproved; 1 callsite. Recorded sites: 303b0b.

Target 0x2d9e10; unproved; 1 callsite. Recorded sites: 303aed.

Target 0x2d9e70; unproved; 1 callsite. Recorded sites: 3149ac.

Target 0x2d9ed0; unproved; 1 callsite. Recorded sites: 314997.

Target 0x2d9f30; unproved; 5 callsites. Recorded sites: 2ffd0c, 316124. Additional binary sites: 315045, 315260, 31562c.

Target 0x2e5f80; unproved; 3 callsites. Recorded sites: 2ffdcf. Additional binary sites: 315102, 31531d.

Target 0x2e6420; unproved; 1 callsite. Recorded sites: 3000b1.

Target 0x2e64e0; leaf; 3 callsites. Recorded sites: 2ffd74. Additional binary sites: 3150c2, 3152dd.

Target 0x2f39e0; unproved; 4 callsites. Recorded sites: 301e24, 301f30. Additional binary sites: 315a8b, 315b54.

Target 0x2f7ed0; unproved; 24 callsites. Recorded sites: 309592, 3095af, 3095c5, 3095db, 30b9f4, 30ba02, 30ba14, 30ba28, 31609e, 3160a7, 316319, 316322, 316748, 316759, 316788, 316796, 3170fa, 317125, 317165, 317170, 3171aa, 3171b5, 318dfd, 318e1a.

Target 0x2f8370; unproved; 11 callsites. Recorded sites: 309854, 3098e6, 30992b, 309975, 3099d4, 309afe, 309b22, 309b70, 309ba8, 309bf1, 309cca.

Target 0x2f8450; unproved; 5 callsites. Recorded sites: 309811, 309866, 3098f8, 309987, 3099e6.

Target 0x2f8580; unproved; 1 callsite. Recorded sites: 301885.

Target 0x2f9160; destination; 7 callsites. Recorded sites: 309541, 309559, 309571, 30976b, 3097a0, 3097cd, 3097ff.

Target 0x2f9180; static value; 1 callsite. Recorded sites: 317ad9.

Target 0x2f9290; destination; 2 callsites. Recorded sites: 30b04a. Additional binary sites: 315afe.

Target 0x2f92c0; destination; 33 callsites. Recorded sites: 305cbe, 306285, 30c10a, 30c471, 30c80c, 30cb9d, 30ceef, 30d231, 30d371, 30d9da, 30dd47, 30e0e6, 30e478, 30e7cb, 30eb3d, 30ec91, 30f355, 30f6d8, 30fa76, 30fdf4, 310142, 310491, 3105d3, 310c36, 310fb4, 311352, 3116da, 311a2d, 311da1, 311ef7, 31230f, 312937, 312a5d.

Target 0x2f92f0; destination; 8 callsites. Recorded sites: 3083cb, 3083e3, 3083fb, 308413, 30842b, 308440, 308458, 30846d.

Target 0x2f9320; destination; 1 callsite. Recorded sites: 308a03.

Target 0x2f9370; destination; 15 callsites. Recorded sites: 305f17, 309edd, 309fdb, 30a20d, 30a2a0, 30a3b7, 30a411, 30a869, 30ac71, 30b13c, 30b1d9, 313bb7, 313c4b, 313d94, 313e7e.

Target 0x2f93a0; destination; 54 callsites. Recorded sites: 304abb, 304b99, 30527e, 3054f2, 30575d, 3059d0, 305c00, 3061a7, 30b816, 30bf68, 30c2d4, 30c664, 30ca01, 30cd48, 30d149, 30d54a, 30d602, 30d835, 30dba7, 30df3c, 30e2dc, 30e625, 30ea3e, 30ee8d, 30ef5a, 30f1ac, 30f530, 30f8ce, 30fc54, 30ffa0, 3103a7, 3107aa, 31085e, 310a8e, 310e0c, 3111a5, 311539, 311887, 311ca0, 3120f1, 3121ba, 31408f, 3142c6, 3144df, 3147e3, 3160fa, 3167f2, 3169a8, 316cbf, 316d8e, 316fd3, 317eba. Additional binary sites: 3155f3, 315dd7.

Target 0x2f9830; unproved; 1 callsite. Recorded sites: 312db2.

Target 0x2f9ef0; unproved; 2 callsites. Recorded sites: 31673b, 317158.

Target 0x2f9f60; unproved; 1 callsite. Recorded sites: 30be05.

Target 0x2fa150; unproved; 6 callsites. Recorded sites: 306752, 306765, 306c41, 306f8c, 30710c, 3139f3.

Target 0x2fa270; unproved; 4 callsites. Recorded sites: 316980, 316aed, 316d3f. Additional binary sites: 315f43.

Target 0x2fa290; unproved; 7 callsites. Recorded sites: 30611b, 307a7c, 31644f, 316513, 316652, 3166a1, 3166dc.

Target 0x2fa2b0; unproved; 1 callsite. Recorded sites: 306662.

Target 0x2fa2f0; unproved; 5 callsites. Recorded sites: 30bbd5, 30d3c3, 31064a, 318396, 3185fa.

Target 0x2fa310; unproved; 31 callsites. Recorded sites: 30d3b1, 30ea9a, 30eb91, 30eccf, 30ece5, 30ed23, 30eef3, 30efcb, 310638, 311ce9, 311de2, 311f38, 311f6f, 311f85, 31213a, 312219, 31235a, 312391, 312435, 312977, 3129b8, 3129ca, 312ab7, 312ac9, 312af0, 314737, 314749, 31479e, 3147b0, 316ed4, 316eed.

Target 0x2fa330; unproved; 8 callsites. Recorded sites: 305c55, 305c85, 305ce3, 305d13, 3163e5, 31640c, 3164a8, 3164cf.

Target 0x2fa350; unproved; 3 callsites. Recorded sites: 316f7c, 317f42, 317f59.

Target 0x2fa360; unproved; 5 callsites. Recorded sites: 309820, 309875, 309907, 309996, 3099f5.

Target 0x2fa390; unproved; 3 callsites. Recorded sites: 309a98, 309aa8, 309bbf.

Target 0x2fa570; unproved; 1 callsite. Recorded sites: 2ff7e4.

Target 0x2fa7b0; partial; 1 callsite. Recorded sites: 318326.

Target 0x2fb7d0; unproved; 30 callsites. Recorded sites: 30c267, 30c5c6, 30c977, 30ccdf, 30d03b, 30d345, 30d4c7, 30db3a, 30de9d, 30e252, 30e5bc, 30e918, 30ec65, 30edfa, 30f4ba, 30f827, 30fbda, 30ff37, 310291, 3105a7, 310725, 310d9a, 311103, 3114bb, 31181e, 311b80, 311ecb, 312060, 3124b6, 312b46.

Target 0x2fb850; partial; 1 callsite. Recorded sites: 3182f2.

Target 0x2fc270; partial; 1 callsite. Recorded sites: 312706.

Target 0x2fdf90; unproved; 2 callsites. Recorded sites: 3182ae, 318caa.

Target 0x2fea10; unproved; 13 callsites. Recorded sites: 305390, 305604, 305872, 305ae5, 30bbfa, 30d454, 30ed87, 3106b2, 311fed, 31245a, 316f0f, 3183c2, 31861b.

Target 0x2feaf0; unproved; 9 callsites. Recorded sites: 307c01, 30882c, 30883e, 308850, 308862, 308874, 308883, 308895, 3088a4.

Target 0x2feb30; unproved; 4 callsites. Recorded sites: 307c76, 3098d7, 309b13, 30b545.

Target 0x2fec70; unproved; 6 callsites. Recorded sites: 307c1c, 309845, 30991c, 309aef, 309be2, 30b561.

Target 0x2ff5c0; leaf; 5 callsites. Recorded sites: 31420e, 314a2d, 31616a, 3167b6, 3171f6.

Target 0x2ff610; destination; 2 callsites. Recorded sites: 316336. Additional binary sites: 3159bd.

Target 0x2ff670; unproved; 2 callsites. Recorded sites: 30b8e2, 316aac.

Target 0x2ff690; leaf; 43 callsites. Recorded sites: 30c0ba, 30c206, 30c421, 30c565, 30c7bc, 30c908, 30cb4c, 30cc8d, 30ce9a, 30cfe6, 30d98a, 30dad9, 30dcf7, 30de3c, 30e096, 30e1e3, 30e429, 30e56a, 30e77a, 30e8c3, 30f305, 30f453, 30f688, 30f7cd, 30fa26, 30fb6b, 30fda5, 30fee5, 3100f1, 310238, 310be6, 310d33, 310f64, 3110a9, 311302, 31144c, 31168b, 3117cc, 3119dc, 311b27, 316e7e. Additional binary sites: 315a60, 315a6e.

Target 0x2ff6c0; leaf; 1 callsite. Recorded sites: 30affe.

Target 0x3191b0; partial; 1 callsite. Recorded sites: 317136.

Target 0x31a5d0; unproved; 1 callsite. Recorded sites: 2ff8b4.

Target 0x31a690; unproved; 1 callsite. Recorded sites: 2ffc51.

Target 0x31a720; leaf; 1 callsite. Recorded sites: 306e1b.

Target 0x31a770; leaf; 1 callsite. Recorded sites: 306f13.

Target 0x31a7c0; unproved; 4 callsites. Recorded sites: 30518d, 30540a, 305671, 3058e2.

Target 0x31a7f0; leaf; 2 callsites. Recorded sites: 306a92, 306aad.

Target 0x31ab90; unproved; 25 callsites. Recorded sites: 30686d, 3068d8, 306943, 3069ae, 307761, 30795f, 307986, 3079ac, 3079d0, 30be10, 30d6d7, 30f053, 310930, 31307f, 3130d2, 313125, 3132c9, 31331c, 31336f, 3134d7, 31352d, 313584, 31370a, 31375d, 3137b5.

Target 0x31abb0; leaf; 2 callsites. Recorded sites: 307939, 3173ee.

Target 0x31abe0; leaf; 1 callsite. Recorded sites: 305115.

Target 0x31abf0; unproved; 3 callsites. Recorded sites: 312870, 312cb0, 316f20.

Target 0x31ac20; unproved; 10 callsites. Recorded sites: 307195, 3071b8, 3071d4, 3071f2, 30a0a1, 30d21d, 30eb29, 31047d, 311d8d, 3122b5.

Target 0x31ac70; leaf; 1 callsite. Recorded sites: 318f03.

Target 0x31ac90; unproved; 1 callsite. Recorded sites: 314a1a.

Target 0x31acd0; leaf; 1 callsite. Recorded sites: 3172aa.

Target 0x31ad40; unproved; 8 callsites. Recorded sites: 30bc02, 30d45c, 30ed8f, 3106ba, 311ff5, 312462, 31639a, 31646a.

Target 0x31ad50; unproved; 1 callsite. Recorded sites: 31655a.

Target 0x31aec0; unproved; 1 callsite. Recorded sites: 316531.

Target 0x31aef0; unproved; 4 callsites. Recorded sites: 300c7a, 300e6a, 30105a, 30124a.

Target 0x31afa0; unproved; 4 callsites. Recorded sites: 300c54, 300e44, 301034, 301224.

Target 0x31b100; leaf; 6 callsites. Recorded sites: 30a0c4, 30b895, 30b8aa, 3171df, 317944, 31807d.

Target 0x31b1f0; unproved; 8 callsites. Recorded sites: 306126, 307a87, 31645e, 316522, 316661, 3166b0, 3166eb, 317379.

Target 0x31b2d0; partial; 32 callsites. Recorded sites: 302855, 30c0e2, 30c449, 30c7e4, 30cb74, 30cec2, 30d9b2, 30dd1f, 30e0be, 30e451, 30e7a2, 30f32d, 30f6b0, 30fa4e, 30fdcd, 310119, 310c0e, 310f8c, 31132a, 3116b3, 311a04, 3148bd, 31624c, 316854, 316a0a, 316d08, 3170d1. Additional binary sites: 3156ce, 31575b, 3157e8, 315875, 315e6a.

Target 0x31bd90; partial; 20 callsites. Recorded sites: 316819, 3169cf, 316bfa, 316c0e, 316c2b, 31736a, 3175b6, 3176fd, 317be8, 317ce0, 317d8d, 317e3a, 317ee2, 318167, 318674, 318706, 31885f, 318989, 318a4c, 318dca.

Target 0x31c0e0; partial; 1 callsite. Recorded sites: 316c39.

Target 0x31c1c0; partial; 22 callsites. Recorded sites: 300092, 30013e, 300316, 30040f, 300534, 300682, 301557, 3015f8, 3016a8, 3017a3, 30198c, 301df7, 30200c, 3020a5, 302224, 302377, 3023df, 305f43. Additional binary sites: 315ab7, 315b13, 315bd3, 315c03.

Target 0x31c470; unproved; 10 callsites. Recorded sites: 3073e6, 30bd67, 30bed6, 30d7a2, 30f119, 3109fb, 3124cf, 313a19, 313b55, 313c09.

Target 0x31c4e0; unproved; 4 callsites. Recorded sites: 316965, 316ad2, 316d24. Additional binary sites: 315f28.

Target 0x31c510; unproved; 2 callsites. Recorded sites: 316345, 3166f7.

Target 0x31d470; partial; 1 callsite. Recorded sites: 318f63.

Target 0x31d700; partial; 1 callsite. Recorded sites: 318f6c.

Target 0x34ee70; unproved; 2 callsites. Recorded sites: 3124d9, 313908.

Target 0x392bac; runtime; 10 callsites. Recorded sites: 303415, 3174b5, 31761f, 31777e, 317956, 317c59, 317d0a, 317db7, 318a15, 318cb9.

Target 0x392d68; runtime; 1 callsite. Recorded sites: 317af2.

Target 0x392dc8; runtime; 1 callsite. Recorded sites: 317a6b.

Target 0x393030; runtime; 1 callsite. Recorded sites: 318fe3.

Target 0x393070; runtime; 1 callsite. Recorded sites: 2ff707.

Target 0x393460; runtime; 1 callsite. Recorded sites: 317ae5.

Target 0x395408; runtime; 1 callsite. Recorded sites: 302720.

Target 0x395450; runtime; 5 callsites. Recorded sites: 302beb, 302c3e, 302c8e, 30457c, 3049cc.

Target 0x395456; runtime; 5 callsites. Recorded sites: 302bd6, 302c29, 302c79, 304571, 3049b7.

Import Sleep, IAT slot RVA 0x42c358; runtime; 5 callsites. Recorded sites: 2ffbe2, 301a66, 305dda, 308ee6, 3162c6.

Import length-error import, IAT slot RVA 0x42c488; runtime; 1 callsite. Recorded sites: 3022bc.

Import ostream insertion import, IAT slot RVA 0x42c4b8; runtime; 2 callsites. Recorded sites: 30bb37, 3172e2.

Import ostream insertion import, IAT slot RVA 0x42c4c0; runtime; 1 callsite. Recorded sites: 30bb27.

Import rand, IAT slot RVA 0x42cd28; runtime; 281 callsites. Recorded sites: 3020f5, 3024d3, 3024f7, 302517, 30274a, 302785, 30282e, 303e90, 303ea8, 303f01, 303f25, 303f4c, 303f73, 303fa3, 303fd3, 304015, 304045, 304075, 30420e, 30439d, 305154, 3051a2, 305283, 3053d6, 30541c, 3054f7, 305638, 305685, 305762, 3058ab, 3058f2, 3059d5, 305b34, 305bdd, 305c05, 305d3a, 30604d, 306130, 306159, 306243, 3062a8, 306bc5, 306e5d, 307ab2, 307ade, 307cb9, 307ce9, 307d19, 307d5b, 307d8b, 307dbb, 307e22, 307e48, 307e73, 30be49, 30bf16, 30bf32, 30bf79, 30bf98, 30c10f, 30c224, 30c282, 30c29e, 30c2e5, 30c304, 30c476, 30c583, 30c612, 30c62e, 30c675, 30c694, 30c811, 30c926, 30c9af, 30c9cb, 30ca12, 30ca31, 30cba2, 30ccab, 30ccf6, 30cd12, 30cd59, 30cd78, 30cef4, 30d004, 30d607, 30d715, 30d7e3, 30d7ff, 30d846, 30d865, 30d9df, 30daf7, 30db55, 30db71, 30dbb8, 30dbd7, 30dd4c, 30de5a, 30deea, 30df06, 30df4d, 30df6c, 30e0eb, 30e201, 30e28a, 30e2a6, 30e2ed, 30e30c, 30e47d, 30e588, 30e5d3, 30e5ef, 30e636, 30e655, 30e7d0, 30e8e1, 30ef5f, 30f08c, 30f15a, 30f176, 30f1bd, 30f1dc, 30f35a, 30f471, 30f4de, 30f4fa, 30f541, 30f560, 30f6dd, 30f7eb, 30f87c, 30f898, 30f8df, 30f8fe, 30fa7b, 30fb89, 30fc02, 30fc1e, 30fc65, 30fc84, 30fdf9, 30ff03, 30ff4e, 30ff6a, 30ffb1, 30ffd0, 310147, 310256, 310863, 31096e, 310a3c, 310a58, 310a9f, 310abe, 310c3b, 310d51, 310dba, 310dd6, 310e1d, 310e3c, 310fb9, 3110c7, 311153, 31116f, 3111b6, 3111d5, 311357, 31146a, 3114e7, 311503, 31154a, 311569, 3116df, 3117ea, 311835, 311851, 311898, 3118b7, 311a32, 311b45, 3121bf, 3122c8, 3127a5, 3127c5, 3128c2, 312914, 31297f, 312a3a, 312a7e, 31409b, 31419f, 3141cb, 3141e5, 3142d6, 3144ee, 3146fe, 31476b, 314803, 3162cc, 3162f2, 31682d, 31688b, 316897, 3168c3, 3169e3, 316a22, 316a47, 316a5f, 316b4c, 316b71, 316b7d, 316c4d, 316ce1, 31704f, 317487, 3174f8, 3175d1, 31764a, 31773c, 3177bf, 3178e4, 317911, 317972, 3179cf, 3179ee, 317a0d, 317b09, 317b31, 317e88, 317ef6, 317f1e, 317f62, 317f91, 31817b, 3181f2, 3183da, 318411, 318457, 318473, 318641, 3186a9, 3187ac, 3187f9, 318873, 318905, 31899d, 3189d6, 318ae0, 318b30, 318b82, 318bbe, 318c03, 318c51, 318c86, 318ce3, 318cff, 318d25, 318d44, 318d63, 318d82, 318e89. Additional binary sites: 305ff0, 314bd2, 314bee, 314c10, 314c6e, 314c90, 314cee, 314d06, 314d61, 314dd7, 314dfc, 314e08, 315396, 3153fe, 315e3e, 315e82, 315ea7, 315eb3, 318019.

Import srand, IAT slot RVA 0x42cd38; runtime; 4 callsites. Recorded sites: 30293f, 314a46, 3167bd, 317222.
