# Original native stat-query call boundaries

This bounded contract supports cubeworld.original.exe.unpacked.exe, AMD64 PE32+, 5,757,952 bytes, preferred image base 0x140000000, SHA-256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. Addresses are RVAs. Evidence is restricted to the original binary and original native function, assembly and call records. No game, probe or build was run. Native UI labels below establish observed meanings; they do not establish an imported SDK declaration, complete stat taxonomy or reproduced formula.

## Common ABI and return qualification

All nine audited entries return a single binary32 floating-point value in the low 32 bits of XMM0 under the Windows x64 convention. They do not return the stat as an integer or pointer in RAX, do not require a hidden result destination, and do not return a double merely because a decompiler displays an eight-byte temporary. Upper XMM0 bits and incidental RAX contents are not part of the established scalar-result contract.

The original decompilations of 0x50d30 and 0x66460 misleadingly describe void returns, while some other entries show an eight-byte concatenation. Assembly resolves those ambiguities. Routine 0x50d30 finishes by moving its floating accumulator to XMM0 at 0x50e7f; 0x66460 does so at 0x66685. Native callers immediately consume their low XMM0 value with scalar floating operations. Similarly, 0x4f080 returns through XMM0 at 0x4f204, 0x64d90 at 0x64f14, 0x647b0 at 0x648a1, 0x4fa70 at 0x4fd18 and 0x65c70 at 0x65e78. These are scalar floating results, not the decompiler's apparent composite integer values.

Every entry requires a live, valid primary Creature pointer in RCX. None of the examined entries establishes null primary-Creature support. Their reads can include inline equipment records, scalar state, flags and the separately allocated effect list rooted at Creature+0x980. The supplied pointer must therefore identify usable constructed state and usable referenced collections. The entries do not validate arbitrary memory or acquire a general lifetime reference to the Creature.

## Single-Creature entries

Entry 0x5fac0 requires only the Creature in RCX and returns the float in XMM0. Original UI routine 0x273380 displays its result under HIT POINTS. Native upkeep 0x2de4c0 also compares the Creature's health at +0x180 with this result and caps health against it. Thus a hit-point capacity/limit role is independently supported; this is not a getter for the current +0x180 value. The body reads equipment, flags and effects. No second Creature, byte mode, output destination or additional logical argument is established.

Entry 0x4f080 likewise requires only RCX and returns a float in XMM0. UI 0x273380 displays it under ARMOR. It reads selected equipment, equipment eligibility, Creature flags and effect records. Entry 0x64d90 has the same one-Creature call boundary and is displayed under RESISTANCE. It uses a different equipment contribution helper and some different effect selection. These meanings come from native labels and consumers, not from treating the two similarly structured routines as interchangeable. Their complete combat interpretation is outside this audit.

Entry 0x66460 requires only RCX and returns a float in XMM0. UI 0x273380 labels it HASTE and formats the result after a baseline adjustment. Timing consumers 0x4ead0 and 0x4f220 use this result when determining action thresholds; hit processing 0x2dba40 also uses it while adjusting action timing. This supports a timing multiplier role rather than an already formatted display percentage. The body consults Creature selectors, the counter at +0x70, equipment, flags and an effect-list condition. No boolean second argument is established despite its several internal branches.

Entry 0x647b0 requires only RCX and returns a float in XMM0. The native UI label is REGENERATION. Upkeep 0x2de4c0 uses this result in an elapsed-time-dependent change to Creature+0x9a0. This establishes the particular resource/accumulator consumer; the UI name alone does not make it a universal health-regeneration rate. The examined body obtains contributions from equipment records. No second Creature or incoming mode is established.

Entry 0x5f8d0 requires only RCX and returns a float in XMM0. The UI label is MANA GENERATION. Its body searches the effect list for kind byte 0x22 and returns a value based on that record's magnitude, with a native fallback when no matching record is found. The examined body contains no calls. Native hit and upkeep routines consume its floating result. It is not established as the current mana amount or as a second-actor query. The complete resource-update interpretation and numeric domain are not specified here.

## Equipment-selection byte arguments

Entries 0x4fa70 and 0x65c70 require the Creature pointer in RCX and a byte selection mode in DL. Their entry assembly explicitly tests DL, not an eight-byte pointer or an integer stat identifier. Zero and nonzero choose the two native branches; ordinary callers supply zero or one. Values outside those ordinary byte modes have no additional enumeration established here. Incoming R8, R9, floating argument registers and a hidden output buffer are not required logical inputs for these entries. Apparent extra decompiler parameters are forwarded to allocation machinery which does not consume them as stat inputs.

For both entries, zero mode invokes equipment-selection helper 0x4fd40; nonzero mode invokes 0x52ce0. The latter builds a temporary list from the type-3 equipment records at Creature+0x6d8 and +0x638 when present. The zero branch uses different subtype/eligibility selection. The mode therefore changes which equipment contributes; it must not be omitted or guessed from a neighboring boolean declaration. A complete equipment-selection algorithm is outside scope.

UI 0x273380 calls 0x65c70 with DL equal to one and displays the result under SPELL POWER. It calls 0x4fa70 with the same byte mode and displays the result under ATTACK POWER. Assembly call sites 0x274672 and 0x2747d3 independently establish both arguments and immediate XMM0 consumption. Zero-mode native examples include 0x29d501 for 0x65c70 and 0x2bc197 for 0x4fa70. UI labels with mode one do not establish that all modes are numerically identical.

Both routines create, transfer and destroy temporary lists of pointers to equipment records, evaluate equipment contributions, and read Creature flags/effects. Their temporary nodes and sentinels are allocated and freed during the query. No such temporary ownership is returned to the caller; the only qualified normal result is the float in XMM0. Entry 0x4fa70 additionally consults action/resource state, so sharing a contribution helper with 0x65c70 does not make their outputs equivalent.

## Optional second Creature at 0x50d30

Entry 0x50d30 takes the primary Creature in RCX, an optional second Creature pointer in RDX and a byte mode in R8B, returning a float in XMM0. It captures R8B as a byte and retains the full RDX pointer. No fourth logical argument or result destination is established.

Null RDX is directly supported. The body explicitly tests the saved second pointer and skips its secondary-Creature adjustment when null. Native UI call site 0x274d45 supplies RDX equal to zero, primary Creature in RCX and a computed byte mode in R8B, then consumes XMM0. The null behavior is therefore both a callee branch and an ordinary native use, not an inference from the word optional. With null RDX the byte mode does not affect that skipped adjustment; ordinary callers still initialize it.

With nonnull RDX, zero mode selects the second Creature's helper 0x50ea0, which derives a value through the ARMOR query 0x4f080. Nonzero mode selects helper 0x5f740, which derives a value through the RESISTANCE query 0x64d90. The primary result is adjusted using that secondary value. A nonnull second argument must identify usable Creature/equipment/effect state; no null-tolerant behavior is established for the secondary helpers themselves.

The native UI displays the null-secondary result under CRITICAL. Hit processing 0x2dba40 passes a nonnull second Creature and compares the returned float against a random-derived value, supporting a critical-chance/threshold role in that consumer. The query itself does not draw that random value. The mode chooses the secondary armor/resistance-related branch; it is not a guarantee that the result is one fixed unadjusted display chance. No universal clamp, probability range or complete critical-hit policy is qualified here.

## Mutation, runtime effects and protection

The examined query bodies do not directly write Creature scalar, equipment or effect-list fields. They read those values and, in 0x4fa70/0x65c70, mutate and free their own temporary collection storage. This distinction matters: those two queries are not allocation-free accessors. Native allocation and length-error paths are reachable through their selection helpers, so a successful float return is not an error-status protocol and no nonthrowing guarantee is established.

No direct rand or srand call was identified in the nine query entries. The random comparison following 0x50d30 belongs to its hit-processing caller. Item contribution helpers and the optional-second-Creature helpers include mathematical runtime calls such as powf. Their complete transitive runtime behavior, floating environment effects and failure behavior have not been exhaustively qualified. The direct-read findings do not establish a universal pure or side-effect-free API.

None of these entries acquires the World+0x3c8 section in its examined body. An established enclosing route is hosted update 0xf5520: it holds hosted-object+0x5f8, enters the embedded World's +0x3c8 section, and reaches World simulation 0x2da6c0, physics 0x2bb8e0, hit processing 0x2dba40 and upkeep 0x2de4c0. Stat calls reached on that route inherit the caller's World section. Helpers 0x2abb70 and 0x2b9890 enter and leave it. This is a particular native caller scope, not a lock acquired by each query.

UI routine 0x273380 provides particularly useful ABI and label evidence, but no local World-section acquisition was established in that UI routine. Its own callers' complete protection is outside this audit. This note does not prove that one World lock excludes every equipment/effect writer, protects all Creature lifetimes, or makes multiple queries observe one coherent state. No implementation lock, snapshot policy or universal writer census follows from these call-boundary facts.

Status: FINAL / STABLE / HELD after readback.
