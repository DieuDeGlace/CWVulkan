# Steam creature appearance-flag boss bits (0x200, 0x40000, 0x20000) - behavioural contract

Author role: isolated source-side specification author (docs/CLEANROOM_AUTHOR.md).
Date: 2026-11-21. Status: FINAL / STABLE / HELD.

## Evidence base

- Image: Steam `cubeworld.exe`, unpacked image `cubeworld.original.exe.unpacked.exe`,
  SHA-256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244,
  preferred base 0x140000000. All RVAs below are relative to that base.
- Original evidence used: the instruction listing, the recovered per-function
  decompilation, the call list, the string table and string-reference table from the
  original Steam evidence folder, plus float constants read directly from the
  unpacked image's data sections.
- Infinite-Cubes (alpha reconstruction) was consulted as a lead only. Its alpha-era
  "is boss" test is race based and it calls bit 0x200 a "tinted" flag. Neither label
  is a Steam fact. The Steam findings below come from original evidence; where they
  agree with the lead this is stated.

## Field

- The flags word is the 32-bit value at creature +0x80, which is appearance-block
  offset +0x08 (the appearance block starts at creature +0x78).
- In the 0x950-byte spawn records built by the world/encounter generators, the same
  appearance block starts at record +0x50, so the same flags word is at record +0x58.
  The record's creature-type word is at record +0x20 and its hostility word at
  record +0x1C. Spawners write the flags before they call the appearance initialiser,
  so the flag-dependent size rules below apply to generated creatures.
- Single-bit testers: RVA 0x5EDA0 returns bit 9 (0x200). RVA 0x5EEE0 returns bit 18
  (0x40000). Many routines also test these bits inline, so the tester call graph does
  not list every consumer.

## Naming verdict

- **0x40000 = boss.** It is set on the big "boss races" by type in the appearance
  initialiser, and it is added by the biome-boss and lair-boss encounter generators.
  It gives the largest stat multipliers and size growth, and it triggers boss music and
  guaranteed equipment drops. The lead's race list (Troll, DarkTroll, Yeti, Mammoth,
  Cyclops, RuneGiant, SeaCrab, Lich, Dragon, Saurian) overlaps Steam's intrinsic set,
  but Steam moved the test from race to this flag. Confidence: high.
- **0x200 = named boss (a "rare-name" unique leader).** No creature type has it by
  default. Encounter generators set it only on one leader per group or site. A
  creature with this bit and no stored name has a name generated from its entity id
  and type. That name goes into the target frame's "rarename" text element and into
  the world label. The bit also gives about 10x health, always drops one worn item,
  10x coins, guaranteed equipment, boss music and a glow. Confidence: high for the
  "named/unique leader" meaning. "Boss" fits because it shares every boss-only
  behaviour with 0x40000 (music, guaranteed equipment, slow immunity, no taming).
- **0x20000 = elite / heavy tier, the only "mini-boss" candidate.** It is intrinsic
  to some large non-boss types. Generators add it to a site leader when that leader
  carries a special item. It gives only moderate stat and size multipliers, and none of
  the boss-only behaviours (music, drops, AI, taming, movement) test it. No string or
  UI element names it. "Mini-boss/elite" is an inference. Confidence: medium-low.
- The evidence shows no separate "mini-boss" bit beyond 0x20000. No bit in this word
  selects a distinct boss health-bar style (see Limits).

## Consumers by routine

Multipliers are float constants read from the image. "Either bit" means 0x200 or
0x40000 (mask 0x40200).

### Appearance initialiser, RVA 0x53E80 (size and intrinsic bits)
- The input is a pointer to the creature type and a pointer to the appearance block.
  A per-type switch sets defaults and ORs per-type bits into appearance +0x08.
- After the switch the size vector at appearance +0x0C..+0x14 (three floats,
  creature +0x84..+0x8C) is adjusted in this fixed order:
  1. If 0x20000: multiply the vector by 1.5. Then, if its first component is above
     8.0, rescale the whole vector so that the first component equals 8.0.
  2. If 0x200: the same rule (x1.5, then cap the first component at 8.0).
  3. If 0x40000: multiply by 7.0, then apply the same 8.0 cap.
  The effects stack in that order. For example, 0x200 and 0x20000 together give
  2.25x before the cap.
- Types that get 0x40000 by default (type id, name from the type-name table built at
  RVA 0x1510F0): 0x6C Troll, 0x6D DarkTroll, 0x6E HellDemon, 0x6F Golem,
  0x70 EmberGolem, 0x71 SnowGolem, 0x72 Yeti, 0x73 Cyclops, 0x74 Mammoth,
  0x76 RuneGiant, 0x77 Saurian, 0x106 ManaMech.
  0x75 Lich and 0x6B SeaCrab do not get it by default. Generators add it.
- Types that get 0x20000 by default (at least these case labels): 0x2E Ogre,
  0x52 Minotaur, 0xD0 PirateMale, 0x104 OrcMale, 0x105 OrcFemale, 0x10D Monstrosity.
- No type gets 0x200 by default. A register is loaded with 0x200 for the tail test
  only and is never ORed in.

### Maximum health, RVA 0x5FAC0
- After the base value and the equipment sum: 0x200 gives x10. Then 0x20000 gives
  x1.5. Then 0x40000 gives x15. The multipliers stack.

### Character-stat getters (identified by the character-sheet labels at RVA 0x273380)
- Attack power, RVA 0x4FA70: 0x200 gives x1.5, 0x20000 gives x1.2, 0x40000 gives x2.
- Spell power, RVA 0x65C70: 0x200 gives x1.5, 0x20000 gives x1.2, 0x40000 gives x4.
- Armor, RVA 0x4F080, and resistance, RVA 0x64D90: x1.2 if 0x200 or 0x20000 is set
  (applied once), then x2 if 0x40000.
- Haste, RVA 0x66460: 0x200 gives x0.75, 0x40000 gives x0.6. 0x20000 is not tested.
  Bit 0x4000000 gives x1.1. Whether a lower value means faster is not established.
- Shield/guard value, RVA 0x50370 (used as the divisor for mana cost while blocking at
  RVA 0x29EB60): 0x200 gives x2, 0x20000 gives x1.2, 0x40000 gives x4. Bit 0x4000000
  gives x1.1.
- Climbing speed, RVA 0x50B60, and swimming speed, RVA 0x663F0: if either boss bit
  is set, return the constant 1.0. Otherwise use the skill-derived formula.

### Boss music, inside the client frame routine RVA 0xA5A50 (test sites at RVA 0xAB63F and 0xAB64B)
- The routine scans all creatures. The boss state is on if any creature meets all of
  these: its hostility byte (+0x60) is 1, its threat/aggro list (+0xB88) contains the
  local player's entity id, and it has 0x40000 or 0x200.
- When the boss state is on, the chosen track becomes "boss.ogg". This replaces the
  biome, zone or faction track and the "deadlands.ogg" curse override. Later checks in
  the same routine can still override it: "success.ogg" while the success timer is
  running, and "maintheme.ogg" while certain menus are open. The boss state (or a
  running success timer) also shortens the track-change deadline (current time
  +500).

### Drops, RVA 0x2A6860 (death/loot routine)
- Worn-item drop: the candidate list is the creature's non-empty equipment slots. If
  the list is not empty, one random entry drops with chance 1/16. With 0x200 it
  always drops.
- Generated equipment drop, hostile creatures (+0x60 = 1) only: chance 1/20. With
  either boss bit it always happens. With either bit, a byte field of the generated
  item that the routine caps at 4 (likely rarity) is raised by 1, up to 4.
- Coin drop, hostile only: the base amount, times a random factor, is multiplied by
  10 for 0x200 and by 20 for 0x40000. Both multipliers stack.
- The tester and inline sites are at RVA 0x2A6E4B and 0x2A7508 to 0x2A751D, plus the
  0x40200 masks in the same routine.

### Movement, RVA 0x2BB8E0 (creature physics/update)
- Base horizontal acceleration/speed scalar: 6.0, or 8.0 when either bit is set
  (consistent with the lead's "6, 8 for bosses and tinted").
- For the local player and for non-player creatures on the host, the speed factor at
  +0x194 normally applies state reductions. These come from field +0x5C: bit 0x2 uses
  the swimming-speed getter, 0x80 gives 0.35, 0x200 gives 0.25 and 0x400 gives 0.98.
  Creatures with either boss bit skip all four reductions. The 0x800 reduction (0.85)
  is not gated by boss bits.

### AI, RVA 0xBAC0 (creature AI update)
- Taming/feeding capture (test at RVA 0xCAFF): a creature with either boss bit
  cannot be captured through the path in which a player in action state 0x56, holding
  a matching item kind 0x15 for the creature's type, becomes its master.
- Random evasive impulse (tests at RVA 0xD81A and 0xDB2A): a 1-in-10 random sidestep
  or hop velocity impulse is never used by creatures with 0x40000. It is also limited
  to creatures whose size first component is below 2.0. 0x200 alone does not block
  it.

### Hit processing, RVA 0x29EB60
- On a hit, the chance of the secondary hit event (event kind 0x17 queued at the
  struck creature) is multiplied by 0.15 if the struck creature has 0x40000. It is
  multiplied by a further factor (most likely 0.1; register path not fully resolved)
  if the attacker has no 0x40000 and its hostility is nonzero.

### Name display
- Target frame, RVA 0xB0680 (it uses UI elements "bar2", "rarename" and "faction").
  If the creature's stored name (+0x958) is empty, the "rarename" text is a name
  generated by RVA 0x611D0 (a syllable generator seeded by the entity id low word and
  the creature type). This happens when 0x200 is set, or when hostility is 3.
  Otherwise "rarename" is left empty. A stored name is shown as-is.
- World label/speech overlay, RVA 0xB40D0 (test at RVA 0xB42D7): when the stored name
  is at most one character, a generated name with a fixed suffix is drawn. This
  happens for hostility 3, or for hostility 1 with 0x200.
- Neither path tests 0x40000. Boss-race creatures without 0x200 get no generated name.

### Glow/light effect, RVA 0xCE5F0 (render; test at RVA 0xD172A)
- 0x200 creatures get an attached light/particle record at their position, raised by
  a per-entity varying offset. Its parameters are 16.0 followed by 0.25, 0.1 and 0.5
  (likely radius and an RGB purple tint). Bit 0x4000000 gets the same record with
  0.25, 0.1 and 0.1. This is consistent with the lead's "tinted" label for 0x200.

### Pet inheritance, RVA 0x2E0DB0
- When the host spawns a pet for an owner, the pet gets 0x200 if the owner has 0x200.
  Bit 0x8000 is copied separately for hostility-1 owners.

## Where generated creatures get the bits

Encounter-kind dispatcher, RVA 0x1274D0. It seeds the random generator from the zone
coordinates, gets the zone's site kind (RVA 0x119A00) and calls the spawner for that
kind. The kinds that matter here:
- Kind 0 -> RVA 0x11D970: a group of 5 to 8 hostile creatures (count from a position
  hash). Every member gets 0x2000. The first member also gets 0x200 and carries the
  site's special item. This is the named group leader.
- Kind 1 -> RVA 0x125720: a single biome boss. Its type comes from the zone-dependent
  picker RVA 0x1292F0, whose pool is SeaCrab, Troll, DarkTroll, HellDemon, Golem,
  EmberGolem, SnowGolem, Yeti, Cyclops, Mammoth, Lich, RuneGiant and Saurian. It gets
  0x2000 and 0x40000 and carries the site item.
- Kinds 0x11, 0x12, 0x16, 0x17 -> lair spawner RVA 0x1241B0, with the leader flag
  argument 0x40000. The leader types are:
  - 0x11: SeaCrab (0x6B).
  - 0x12: type 0x25 plus a zone-derived offset clamped at 0 (0x25 to 0x28 are the
    slimes).
  - 0x16: one of BarkBeetle, SnoutBeetle and LemonBeetle (picker RVA 0x141390).
  - 0x17: Habanero or Bloomling (picker RVA 0x142000).
- Kinds 0x13, 0x14, 0x15 -> the same lair spawner with the leader flag argument 0x200,
  a per-kind word 5, 4 or 6 written at record +0x28, and leader types from picker
  RVA 0x142760, RVA 0x13FEB0 and EmpireSoldier (0xEC) respectively.
- In the lair spawner, RVA 0x1241B0, the leader gets the flag argument with 0x3000,
  hostility 1 and the kind's type. If a special item exists for the site, the leader
  carries it and also gets 0x20000. Followers get only 0x2000 from the dispatcher's
  calls; the follower-flag argument is 0. Followers' types are drawn from the kind's
  list.
- Kind 0x18 -> RVA 0x123210: one leader is a type-0x28 slime with 0x2200. The leader
  that carries the site item gets 0x20200 added.
- Kind 0x20 (and 0x1E) -> RVA 0x120BC0: the item-carrying member gets 0x20200 added to
  0x2000.
- Kinds 9 to 0x10 -> RVA 0x11F280: one member gets 0x2200. Its type is the
  caller-supplied leader type, for example Ogre, Witch, Skeleton, Monstrosity,
  Necromancer, DarkCultist or DemonMarauder.
- Kind 0x1B (RVA 0x11CD80) and the generic group spawner RVA 0x126160 only OR 0x2000
  with a caller value, and the dispatcher passes no boss bits.

The structure/site populator, RVA 0x2FF6F0 (reached from RVA 0x2AE0C0), calls:
- RVA 0x11A820, for sites on tiles of kind 0x34, with these placements:
  - Site leaders with 0x2200 (one with 0x102200 that carries an item).
  - EmpireSoldier (0xEC-based) guards with 0x2200.
  - Hostility-8 ManaInductor (0xF0) or ManaPump (0xEF) objects with 0x42000.
  - A hostile ManaMech (0x106) with 0x42000.
  - A Witch-led group via RVA 0x11E7F0, whose leader gets 0x2000, the caller's extra
    flags and 0x200, and carries the item.
- RVA 0x2FB850, which prints "END BOSS?" on structure kind 4: for structure kind 2 it
  places a DemonMarauder (0xE2) with 0x2200, and for kind 4 a DarkCultist (0xE3)
  with 0x22200.
- RVA 0x2FA7B0: one member with 0x2200 when a site counter is 1. A site-centre member
  gets 0x22200. When the mode argument is 3, a member gets 0x2200 and carries an
  item. When the mode argument is 4, a member gets 0x20000.
- RVA 0x2FDF90: some members get 0x2200, with random selection by count.

RVA 0x31C540 ORs only 0 or 0x100000. The flag value built in the UI routine
RVA 0xF9AC0 is a local render word, not creature +0x80.

## Limits

- I found no boss-specific health bar, nameplate frame or title string keyed to any of
  these bits. The target frame's "bar2" path does not test them. A frame or bar
  difference driven by other fields (for example level or hostility) is outside this
  contract and not ruled out.
- The meaning of several referenced fields is inferred: the +0x5C state bits, the
  item byte capped at 4 ("rarity"), event kind 0x17, the RGB reading of the glow
  parameters, and the haste direction. The values and orders above are exact.
- Encounter kinds are described by spawner behaviour. The world-design names of kinds
  0..0x20, of structure kinds 2/4/6 and of tile kind 0x34 are not established. The
  zone-offset range for kind 0x12 is not proven. The type lists of pickers
  RVA 0x142760 and 0x13FEB0 were seen only partially.
- Intrinsic-bit case lists come from the recovered switch. Further case labels that
  share the same body may exist. The 0x20000 entry for 0xD0 PirateMale comes from a
  case label directly above the write and should be confirmed in a runtime check.
- The inline-test scan covered the whole instruction listing for dword loads at +0x80
  to +0x83 followed within seven instructions by a test of bits 9, 17 or 18 or a mask
  containing them, plus decompiled-text searches. Tests through other base registers,
  copied flag words or longer gaps could be missed. Bit 0x1000 (always set on lair
  leaders) and bit 0x2000 (set on almost every generated creature) were not analysed.
- Type names come from the type-name table at RVA 0x1510F0, assuming it shares the id
  space of the creature type at +0x64. All checked ids are consistent with that.
- 0x20000 as "mini-boss/elite" is a functional inference, not a named fact.
- No runtime capture was used. All parity claims need verification against the Steam
  game by the verification role.
