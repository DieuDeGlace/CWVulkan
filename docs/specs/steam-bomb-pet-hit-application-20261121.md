# Steam Bomb and Water Bubble area pass against the local player's pet: hit admission, resolution and HP application

**FINAL / STABLE / HELD** · 2026-11-21

Evidence: original Steam x64 image `cubeworld.original.exe.unpacked.exe`, SHA-256 `7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244`, image base 0x140000000 (unpacked from the shipped `cubeworld.original.exe`, SHA-256 `a1efbbd41eeb8df08395c4d6511194d416cb70a14054b2a2000b9f5cdbcfa65b`), via the offline static export of that image (function index, per-function listing, call references, disassembly). No Infinite-Cubes reconstruction was present in the permitted evidence folder, so none was consulted. All facts below are static readings of the Steam image; nothing here is runtime-verified.

## Terms

- "World" is one world-state object. Byte +0x358 of a world is its **client-copy flag**: zero on the authoritative world and nonzero on the local client world. The connect routine at RVA 0x3d820 sets it to 1 on the client's world, and the disconnect routine at RVA 0x911e0 resets it to 0. The world constructor at RVA 0x29a130 clears the world's **local-player pointer** at +0x448. The only write of a live player into +0x448 that was found is in RVA 0x7ea80, which targets the client game's own world. No write of a player into the authoritative world's +0x448 was found. Its value there is therefore presumed null but not proved.
- Actor fields used: +0x08 actor ID (64-bit); +0x10/+0x18/+0x20 position (fixed point, 65536 units per block); +0x5c flags; +0x60 affiliation byte; +0x64 model ID; +0x68 current action byte; +0x6c action/death timer (float); +0x70 level-like integer; +0x80 flag word; +0x84 and +0x8c size floats; +0x128 flag word; +0x12c float; +0x130 stun float; +0x134 float; +0x14c class byte; +0x180 HP; +0x184 MP for creatures, but "strength" for created actors; +0x188 second resource ("remaining" for the bubble); +0x18c float; +0x1a0 owner/creator ID; +0x980 buff list (entry type byte at +0x10, value float at +0x14); +0x9b8 per-actor "already hit" ID set; +0xb88 and +0xb98 threat maps.
- **Hostility test** (RVA 0x50550, two actors A and B). It is false if A and B are the same actor, or if either has bit 22 (0x400000) of +0x80. Otherwise it is true if either affiliation is 7. Failing that, it is false if either class byte +0x14c is 0xA1, either model is 0x12F, or either affiliation is 8. It is then true if exactly one side has affiliation 1, or both have affiliation 1 and bit 15 of +0x80 differs between them, or either has bit 0x20 of +0x128. In every other case it is false.
- **Damageable test** (RVA 0x50520, one actor). It is true for affiliations 0, 1, 3 and 6. For affiliation 7 it is true unless the model is 0x8C–0x8E. It is false for all other affiliations.

## Creation facts relevant to the pass

- **Bomb** (RVA 0x2c3710, item-use event category 1, subtype 9; runs only when the world's +0x358 is zero, so it runs only on the authoritative world). The bomb gets a fresh ID (lowest existing ID minus 1) and the user's position. It gets model 0x90, HP equal to its max-HP value, affiliation 7, and bits 0x4000 and 0x10000 added to +0x80. The creation routine does not set +0x1a0 (owner) or +0x184 (strength). Both keep the actor-record initializer defaults (RVA 0x4d600, via RVA 0x4ce80): owner 0, +0x184 = 1.0, +0x188 = 0.0, +0x68 = 0. This contract does not establish whether any later routine rewrites the bomb's +0x184. The model/appearance setup called at creation (RVA 0x53e80) was checked and does not write it.
- **Bomb fuse** (per-actor update, RVA 0x2de4c0, authoritative world only). While HP > 0, HP falls by max-HP × 0.00025 × elapsed time. When HP goes below 0, it is set to 0 and +0x6c is set to 0.
- **Water Bubble** (RVA 0x2c3710, case 0x0C). It gets model 0x11D, affiliation 7, and the caster's position. Owner +0x1a0 is set to the caster's ID. +0x144 is copied from the caster. +0x184 (strength) is set to a caster stat (RVA 0x65c70 with argument 1) times a random factor. HP is 1.0, +0x188 (remaining) is 1.0, and bit 0x800 is added to +0x80. On the authoritative world only, remaining falls by elapsed time × 0.00005 (RVA 0x2de4c0). When it drops below 0.1, HP is set to 0, +0x6c is set to 0, and a model-0x2D effect is queued.

## The first-death area pass (RVA 0x2a45e0)

This routine is called from the world update at RVA 0x2da6c0. That update has no client-copy gate of its own, so the pass runs on whichever world copy holds the dying actor. For each actor X in the world's actor list, the pass fires when all of these hold: X's HP ≤ 0, X's affiliation is nonzero, X's +0x6c equals exactly 0, and X's model is 0x11D, 0x90 or 0x119. After the pass, +0x6c is advanced by the frame time, so a given actor fires once. The 0x90 terrain crater and its effects come before the candidate loop. They are outside this contract.

### Candidate admission (same on both world copies)

The routine walks every actor C in the world list, in list order, and skips C silently in each of these cases, checked in this order:
1. C is X.
2. The hostility test (X, C) is false.
3. C's HP ≤ 0.
4. C's +0x12c is nonzero.
5. C's ID is already in X's "already hit" set (+0x9b8).
6. C is out of range. Range uses R = C.+0x84 × 0.7 + 8 blocks. The horizontal |dx| or |dy| must not exceed R, and dx²+dy² must not exceed R². The vertical |dz| must be below C.+0x8c × 0.5 + X.+0x8c + 8. All distances are in blocks.

For an admitted C, the routine then:
- Builds a horizontal unit direction from X to C (normalized only if its squared length exceeds 0.01), scaled by 10, with vertical component 2.5.
- Draws one rand(). The hit is critical when the draw modulo 4 is 0.
- Sets the base amount to X.+0x184, doubled on a critical.
- Inserts C's ID into X's "already hit" set. This happens before the next test, so it occurs even when C is then dropped.
- Drops C if C's affiliation is 8.

**Pet as candidate under the Bomb.** The bomb's affiliation 7 makes the hostility test true against an affiliation-6 pet. The only exception is bit 22 of +0x80 on either actor. No stock write of that bit into a pet was located, so this stays unresolved. Ownership is not consulted at any point. The pet is admitted whenever it is alive, has +0x12c = 0, is in range and has not been hit by this bomb before. Its affiliation 6 passes the affiliation-8 drop.

### Bomb hit creation (model 0x90)

Each admitted candidate gets one hit-resolution call (RVA 0x29eb60) with these inputs:
- defender = C
- source = the bomb actor itself, not an owner (the bomb has none)
- amount = X.+0x184 (doubled on a critical)
- critical flag
- presentation byte = 1
- base stun chance = 0
- the bomb's position
- the direction above
- damage category = 1
- special code = 0
- the "may be absorbed" switch = on

This happens on both world copies. What follows depends on the copy.

## Hit resolution (RVA 0x29eb60): conditions that decide whether the pet's HP can change

Entry gate: the call returns with no record, no effect and no HP change if any of these holds:
- the defender's HP ≤ 0
- the defender's affiliation is 8
- the defender's model is 0x114
- on a client copy only (world +0x358 nonzero): the defender is the local player, or the source is not the local player

**Local client world.** The source is the bomb, never the local player. The call therefore returns at the entry gate for every defender, the pet included. On the client copy the Bomb pass creates no hit record and cannot change the pet's HP. The pet's ID is still added to the client copy's "already hit" set. This contract does not establish whether the client copy of the bomb ever reaches the pass at all, since its fuse runs only on the authoritative world and its HP would have to arrive by replication. Either way the result for the pet is the same.

**Authoritative world**, for a pet defender (alive, affiliation 6), in order:
1. Local-player amount bonus. This applies only when the source is the local player and a class query (RVA 0x65750) returns 6. It never applies to the bomb.
2. Source-action gate. If the source's action byte +0x68 is 0x64 or 0x4B and the defender's +0x5c bit 0 is clear, the call returns silently with nothing created. The bomb's action byte is 0, so this gate is passed.
3. Evasion state. The hit becomes an **evaded** outcome with no HP change if both of these hold:
   - the special code is not 0x8C (it is 0 here)
   - and the defender has at least one of these: +0x128 bit 0x80; +0x5c bit 0x20; action byte 0x7D with timer +0x6c below 5000

   The evaded outcome queues effect 0x18 and a record of kind 3 carrying the full amount. On the authoritative world that record is applied, but application skips everything for kind 3 (see below).
4. Guard/absorb state. With the absorb switch on, the hit is **absorbed** with no HP change if either of these holds:
   - the defender is in a guarding state and has +0x184 > 0. The guarding test (RVA 0x5ed30) requires +0x134 ≤ 0 and one of: action 9; action 0x5F inside its timing window; action 0x6E; or action 0x3C/0x40 with class byte 1 and specialization byte +0x14d equal to 1.
   - the defender's action is 0x48, 0x49 or 0x82, and its timer lies inside that action's window (RVA 0x4ead0 and 0x660d0).

   The absorbed outcome charges an MP cost of amount ÷ (RVA 0x50370 value). The cost is zero for actions 0x48, 0x49 and 0x82, and halved for 0x5F. The cost is deducted from +0x184, which is set to −1 if it falls below the threshold at RVA 0x4aca4c. The outcome also queues a record of kind 1 with the full amount and effect 0x18. Kind-1 records are discarded by hit application, so HP does not change. On the authoritative world only, a second resource is recharged first.
5. Shield buffs. The sum S of the defender's buff-list values of type 6 is taken. If S > 0, a record of kind 5 with the full amount is queued and applied, which reduces each type-6 buff by the amount, floored at 0. Effect 0x5E is queued. The amount becomes amount − S. If that is ≤ 0 the call ends and reports "hit". Otherwise it continues at step 6 with the remainder.
6. Defence. Let r = source.+0x70 ÷ (RVA 0x5f9e0 value of the source), capped at 1. The amount becomes max(0, amount − D × (1 − r)). For damage category 1, D is the defender's value from RVA 0x64d90. For category 0 it would come from RVA 0x4f080. For the bomb, +0x70 is the initializer default (0). If RVA 0x5f9e0 then returns a positive value, r = 0 and the full D is subtracted. That routine's return for a bomb is not established here.
7. Side effects that do not change the pet's HP:
   - Category-7 lifesteal: not applicable.
   - Source-buff healing when the source has certain buff kinds: it applies to the source, not the pet.
   - A source item effect via RVA 0x5e3a0 with argument 2: it applies to the source.
   - Presentation effects: category 1 queues effect 0x2A, with intensity from the constant at RVA 0x42e548.
8. Stun roll. This needs a source, the defender's +0x130 above −3000, and a defender affiliation other than 7. The base chance is 0. It becomes 1.0 only for certain source action bytes (0x0D, 0x2F, 0x45, 0x46 or 0x67), or when the defender's action is 0x58. Several multipliers follow, and any type-1 defender buff zeroes it. It is not reached in the stock bomb case unless the pet's action byte is 0x58.
9. Record. The final amount is multiplied by every type-1 buff value on the defender. The record holds:
   - +0x00: source ID = the **bomb's ID**
   - +0x08: target ID = the pet's ID
   - +0x10: amount
   - +0x14: critical flag
   - +0x18: stun duration (0 unless step 8 fired)
   - +0x20: the pet's position
   - +0x38: direction
   - +0x44: presentation byte 1
   - +0x45: **kind 0**
   - +0x46: 0 (it would be 1 only for source action 0x1D)

   The record is appended to the world's hit list at +0x90 and applied immediately by hit application (RVA 0x29dcf0) when world +0x358 is zero. A critical hit on a defender of non-7 affiliation also queues a model-dependent sound (IDs 0x3F–0x53). The call returns "hit".

## Hit application (RVA 0x29dcf0)

This routine is also reached for received hit batches through RVA 0x29d980. That routine is called from RVA 0xf5520, which also drives the world update. It runs in this order:
1. Look up the target by record ID in the world's ID map (+0x450). Stop if the target is absent or its HP ≤ 0.
2. Stop if the world's local-player pointer is null **and** the target's affiliation is 0.
3. Stop if the record kind (+0x45) is 1.
4. Look up the source by record ID. A missing source (already removed) is allowed and simply yields "no source". The source's HP or death state is **never** tested. A bomb in its own first-death pass is still in the ID map, so it is found.
5. If the amount > 0: the target's +0x18c is reduced by 0.25 (floored at 0). A target in action 0x58 has that action cleared and its stun set to 3000.
6. If the amount ≥ 0 and the source exists, aggro bookkeeping runs. Both actors' +0x9e0 are set to each other's IDs. The threat maps (+0xb98 on the source, +0xb88 on the target) are increased, with an attribution factor of 1.0 when the source's affiliation is 0 or 6 and 0.001 otherwise (the bomb gets 0.001). The routine at RVA 0x2c3200 is then called.
7. **Protection branch.** This applies when the target's +0x80 has bit 0x2000. It walks the target's +0xb88 map for an entry with a positive value whose actor exists and has affiliation 0. If none exists, application stops. If one exists, application stops unless the source exists and has affiliation 0 or 6. A bomb source (affiliation 7) therefore never passes. This is the only affiliation-6 special case in application, and it concerns the source, not the defender. No stock write of bit 0x2000 into a pet was located, and taming (RVA 0x2c3710, the conversion that sets affiliation 6 and re-IDs the creature) does not set or clear it. Whether a stock pet carries it is unresolved.
8. Kind 3 stops here, with no HP change.
9. For non-negative amounts, knockback and impulse are applied. Velocity is overwritten only for non-affiliation-7 targets that are on the authoritative world with nonzero affiliation, or that are the local player. Record +0x1c raises +0x134 and +0x18 raises the stun at +0x130. Kind 4 calls RVA 0x4ee50 instead of HP handling.
10. **HP is changed only if the world is authoritative (+0x358 zero) or the target is the world's local player.**
    - Amount < 0 (heal): HP increases by |amount| and is clamped to max HP (RVA 0x5fac0). On the authoritative world, each actor hostile to the source that holds threat on the target gains threat against the source.
    - Amount > 0 with kind 5: only type-6 buffs are reduced (see resolution step 5).
    - Amount > 0 otherwise: HP decreases by the amount if the damageable test passes for the target (affiliation 6: yes) **and** either there is no source or the damageable test passes for the source. The bomb passes as affiliation 7, model 0x90.
    - If HP ≤ 0 after this, it is set to 0, a death effect (0x1B, or 0x2D for model 0x11D) is queued, +0x6c is set to 0 and death processing runs.
    - An amount of exactly 0 changes nothing.

## Answers

**1. Pet admission and the hit record.** The local player's pet is admitted as a Bomb candidate on both world copies, given HP > 0, +0x12c = 0, in range, not yet hit by this bomb, and neither actor carrying +0x80 bit 22. Hit resolution is called on both copies. Only the authoritative world generates a record: source ID = the bomb's own ID, target = the pet, amount = bomb +0x184 (×2 on a 1-in-4 critical) after the defence reduction and buff multipliers, damage category 1, record kind 0, presentation byte 1. The client copy returns at the resolution gate (the source is not the local player) and generates nothing.

**2. Whether the pet's HP decreases.** Under stock conditions, the pet's HP decreases **on the authoritative world** by the reduced amount, unless one of these holds:
- the pet is in an evasion state (kind 3)
- the pet is in a guard or absorb state (kind 1)
- type-6 shields absorb the hit
- defence reduces the amount to 0
- the pet carries +0x80 bit 0x2000 (protection branch, which always rejects a bomb source)
- either actor carries +0x80 bit 22

Neither the pet's owner/master relation nor any pet-specific branch protects the pet. The source is not required to be alive, and a removed source is tolerated. Nothing in this path changes the pet's HP **on the local client world**: no record is made there, and even a received record would not be applied there because the pet is not the local player. How the client copy's pet HP later follows the authoritative value (state replication) is not established by this contract.

**3. Water Bubble healing the caster's damaged pet.** The heal branch runs inside the same pass, with the bubble as X. The pet is admitted through the hostility test (the bubble has affiliation 7). The amount is bubble strength (+0x184), doubled on a critical, times bubble remaining (+0x188). If the result is ≤ 0, nothing happens. Otherwise the owner is looked up by +0x1a0. If the owner is missing, nothing happens. If the hostility test (owner, pet) is true, a category-2 damage hit with the owner as source goes through resolution instead. For a player and their own affiliation-6 pet this test is false unless either has +0x128 bit 0x20. Otherwise, if the pet's affiliation is not 7 and its HP > 0, a heal record is built: source = the owner's ID, target = the pet, amount = −(strength × remaining) with a critical doubling, critical flag, the pet's position, zero direction, presentation 0, kind 0. The heal never goes through hit resolution, so there is no defence, absorb or shield step. The record is appended once to the world's +0x90 list on every copy.
   - On the authoritative world, a second copy is appended, and hit application raises the pet's HP by exactly strength × remaining (×2 on a critical), clamped to max HP. The +0x80 bit-0x2000 protection branch, if present, would demand an affiliation-0 threat entry, and the owner (affiliation 0) would satisfy the source part.
   - On the local client world, no immediate application happens, because that direct application is reserved for the owner being the local player **and** the candidate being the local player himself. The client-copy pet's HP does not rise from this pass. Hit application would not apply a received heal there either, since the pet is not the local player.

   At natural expiry the pass fires just after remaining has dropped below 0.1, so the heal is slightly under 0.1 × strength (×2 on a critical). This contract does not establish whether the client copy's +0x90 entries are transmitted to and re-applied by the authoritative world, which could double the heal.

**4. The local player as defender of the same Bomb hit.** The player (affiliation 0) is admitted the same way. On the client copy, resolution returns at its gate (the defender is the local player and the source is not), so the client makes no record. On the authoritative world, resolution runs fully and builds a kind-0 record from the bomb. If that world's local-player pointer is null (presumed, see Terms), hit application stops at step 2 for an affiliation-0 target, so the authoritative copy of the player loses no HP there. On a client copy, hit application does apply damage to its own local player, so a player's HP would fall when the authoritative record is applied on the client. The delivery path from the authoritative world to the client was not identified, so that half stays unresolved. The pet is the opposite case: it is damaged on the authoritative copy and never by direct application on the client copy.

## Explicitly unresolved

- Whether anything after creation rewrites the bomb's +0x184 (initializer default 1.0), and so the stock bomb damage magnitude.
- The return values of RVA 0x5f9e0 and 0x64d90 for a bomb and pet, which determine the defence reduction.
- Whether stock pets ever carry +0x80 bit 22 or bit 0x2000.
- Whether the authoritative world's +0x448 is null in single-player.
- How server-generated hit records and actor HP reach the client copy, and whether client-generated +0x90 entries are sent to the authoritative world.
- All of the above is static analysis only. No runtime capture was used.
