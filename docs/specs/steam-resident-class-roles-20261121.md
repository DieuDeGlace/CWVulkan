# Steam Cube World: classes of friendly residents and service NPCs (source-side contract)

Author role: isolated source-side specification author (docs/CLEANROOM_AUTHOR.md).
Date: 2026-11-21.
Image: cubeworld.original.exe.unpacked.exe, SHA-256
7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244, image base
0x140000000. All RVAs below are relative to that base.
Evidence used: the unpacked binary's bytes (jump tables and constants read
directly), the static disassembly listing and the decompiler export of that same
image. The Infinite-Cubes reconstruction (release tree) was used for leads only.
Every statement marked "confirmed" was checked against the original evidence.
Statements marked "lead only" were not.

Format: plain behavioural prose. It contains no source transcription, no
pseudocode and no decompiler symbol names.

## 1. Fields involved

- Creature class: one byte at creature +0x14C (the question's premise; confirmed by
  every routine cited here). Specialization is the next byte, +0x14D.
- Creature hostility: byte at creature +0x60. Friendly NPC = 3. Pets and the flight
  master's eagle use 4 (neutral).
- Creature appearance flags: 32-bit value at creature +0x80.
- Generated resident record (the record a zone builds before it creates a creature;
  heap size 0x950): hostility dword at +0x1C, race dword at +0x20, level dword at
  +0x24, faction/form dword at +0x28, realm dword at +0x30, class byte at +0x34,
  facing angle float at +0x18, appearance block from +0x50 with the flags dword at
  +0x58, behaviour bit-set dword at +0x8B8.
- Resident to creature copy (confirmed, routine RVA 0x2E17D0): the record's
  hostility byte (+0x1C) goes to creature +0x60 and its class byte (+0x34) goes to
  creature +0x14C. The appearance block from +0x50 is copied into the creature's
  appearance, so the record's +0x58 flags become creature +0x80. This last mapping
  is inferred from the block copy and the matching layout; the individual dword move
  was not traced.

## 2. Class table (Steam values)

The "label" column gives the bracketed title that the creature-info / name display
routine appends after the name (RVA 0xB0680, confirmed; a switch on +0x14C). The
"Talk" column is described in section 4.

| Class (dec / hex) | Label shown | Who gets it during generation (confirmed unless stated) | What Talk does |
|---|---|---|---|
| 1 to 4 (warrior, ranger, mage, rogue) | None normally. With appearance bit 0x40 set: "[Warrior Master]", "[Ranger Master]", "[Mage Master]" or "[Rogue Master]" | Daily inn guests from the zone daily update (RVA 0x31C540): hostility 3, class drawn uniformly 1 to 4, level drawn 0 to 4. No routine was found that sets the 0x40 "master" bit on a generated resident (see limits) | Plain conversation. The mission/region talk script has class-specific lines for 1 to 4 ("I want to be the best ...", "... are the strongest", a reaction to the player's class) |
| 128 / 0x80 | [Item Vendor] | House plot 3 in a village | Opens the vendor (shop) panel |
| 129 / 0x81 | [Weapon Vendor] | House plot 5 | Opens the vendor panel |
| 130 / 0x82 | " [Armor Vendor]" (the original string starts with a space) | House plot 4 | Opens the vendor panel |
| 131 / 0x83 | [Analyzer] | House plot 6 | Opens the enchant/analyse panel |
| 132 / 0x84 | [Innkeeper] | House plot 2 | Innkeeper dialogue (rest offer, gold and time checks) |
| 133 / 0x85 | [Smith] | House plot 7 | Plain conversation (no panel is opened) |
| 134 / 0x86 | [Carpenter] | House plot 8 | Plain conversation |
| 135 / 0x87 | [Tailor] | House plot 9 | Plain conversation |
| 136 / 0x88 | [Archaeologist] | No generating site found | Plain conversation (falls to the default path) |
| 137 / 0x89 | [Alchemist] | No generating site found | Plain conversation (default path) |
| 138 / 0x8A | None | Ordinary villager: default for an occupied house plot that has no service (1 in 10); village wanderers; the namesake of named special places (section 3.5) | Plain conversation. The mission talk has a villager-specific rumour branch |
| 139 / 0x8B | None | No generating site found | Default path |
| 154 / 0x9A | [Flight Master] | House plot 10, together with a neutral eagle | Opens the world map in flight-master mode |
| 155 / 0x9B | [Guild Receptionist] | The cell whose building type is 91 (guild hall centre) | Receptionist dialogue: offers a change of specialization |
| 156 / 0x9C | [Gem Trader] | Daily visitor: a single gnome (race 304) from the zone daily update | Opens the vendor panel |
| 157 / 0x9D | [Shop Supplier] | No generating site found | Default path. It also has special mission-reveal handling (section 4.4) |
| 158 / 0x9E | None | Not observed | Not observed |
| 159 / 0x9F | [Master Sailor] | No generating site found | Default path. The AI routine (RVA 0x10B90) branches on it |
| 160 / 0xA0 | [Master Glider] | No generating site found | Default path. The AI routine (RVA 0x10B90) branches on it |
| 161 / 0xA1 | None | Arena champion: the cell whose building type is 97, hostility 3, facing angle 270 | Mission talk opens with an arena branch ("congratulations", "good work", "a new challenger entered the arena") |

The question also lists other roles:
- Climbing teacher and reins teacher: no class value and no label for either exists
  in this image. The only teacher-like labels are Master Sailor (159) and Master
  Glider (160).
- A dedicated "food" or "potion" shopkeeper class: none exists. The only shop
  classes that open the vendor panel are 128, 129, 130 and 156.
- Faction residents with class 141 or 143: a mission-side routine (RVA 0x126160)
  writes these values into resident records. They are faction members, not service
  roles. That routine was not examined further.

## 3. Generation: which routine assigns each class

### 3.1 Village residents: zone generation, RVA 0x2FF6F0

The decompiler timed out on this routine, so the facts below come from the
disassembly and the image bytes.

Which place the zone belongs to (confirmed):
- Before its cell loop, the routine takes the place grid cell that contains the
  zone's origin. That is the zone coordinate times 64 blocks, divided by 1024
  blocks and floored.
- It then fetches that place's description through RVA 0x31A5D0: the place kind
  (dword), the "large" flag (byte) and the place centre in blocks.
- Every cell of the zone is tested against this one place description.

Cell loop (confirmed):
- The loop visits every 16-block cell of the zone.
- For each cell, the C runtime random generator is seeded with the cell's position
  hash modulo 32767. The hash comes from RVA 0x2FF5C0.
- The building type of the cell comes from RVA 0x69C60.
- The cell centre is the cell position times 16 plus 8.

Influence and radius (confirmed, RVA 0x9BEE0 and RVA 0x364E0):
- Squared distance (RVA 0x364E0): the squared distance in blocks from the place
  centre to the cell centre. The cell centre is first displaced by two noise terms.
  Each term is a noise sample times 32 blocks. The x term samples at (x*0.005+18843,
  y*0.01+9432) and the y term samples at (x*0.01+299, y*0.005+90650).
- Influence (RVA 0x9BEE0): zero when the place kind is 0. Otherwise the radius R is
  500 blocks for a large place and 250 blocks otherwise. When the squared distance
  d² is below R², the influence is (1 - d²/R²) squared. Otherwise it is 0.

Village membership (confirmed):
- A cell gets village residents only when its influence is greater than 0.4 (the
  constant at RVA 0x42E52C is 0.4) and the place kind equals 5 (village).
- Influence above 0.4 is the same as d²/R² below about 0.3675. In other words, the
  cell's noise-displaced centre lies within about 0.606·R of the place centre: about
  152 blocks for a small village and about 303 blocks for a large one.
- Other place kinds go elsewhere in this routine (not covered here).

Petrification (confirmed):
- For a village cell, RVA 0x31ACD0 compares the zone's place indices with one pair
  of place coordinates that the mission state stores.
- If they are equal, the routine ORs bit 0x400000 into the appearance flags of every
  person resident it builds for that village: house residents, the flight master's
  eagle, the guild receptionist and wanderers.
- This is the witch-curse village. A matching talk line exists in the mission talk:
  "she has petrified the villagers of the location".
- The pet branch of the wanderer code was not seen applying the flag.

Faction and realm (confirmed in outline):
- The routine reads the village's realm id.
- The form/faction value is filled only when two realm predicates hold:
  RVA 0x10D310 must be false and RVA 0x10D2E0 must be true. The form value then
  comes from RVA 0x10CEE0.
- Lead only: those predicates are "no tyrant" and "has people". The reconstruction
  names them that way; the names were not checked against original evidence.

Sheltered positions (confirmed):
- The plot of the cell comes from RVA 0x2AF7B0.
- For plot 2 (inn), the routine draws one sheltered position (kind 2) and appends
  it to the zone's resident-position list (zone +0x500).
- That list later provides seats for the daily inn guests (section 3.3).

Race table by land biome (confirmed values; the meaning of each biome number was
not verified). The jump table at RVA 0x319130 is indexed by biome minus 1. The
default table is 2. Biome 1 gives 202. Biomes 3 and 6 give 0. Biomes 4 and 8 give
13. Biomes 5 and 12 give 15. Biome 7 gives 9. Biome 9 gives 7. Biome 10 gives 11.
Biome 13 gives 4. Biomes 2 and 11 keep the default 2. The race of each person
resident is drawn from that table through RVA 0x1420F0.

House residents (confirmed). Both conditions must hold for the cell's building type:
"covered" (RVA 0x2B8030) and "has a street door" (RVA 0x31ABB0).
- Plot 0: nobody.
- Otherwise the class starts as 138 and the position kind starts as 4.
- The plot jump table at RVA 0x319164 covers plots 2 to 10 and gives:
  - plot 2: 132 (innkeeper)
  - plot 3: 128 (item vendor)
  - plot 4: 130 (armor vendor)
  - plot 5: 129 (weapon vendor)
  - plot 6: 131 (analyzer)
  - plot 7: 133 (smith)
  - plot 8: 134 (carpenter)
  - plot 9: 135 (tailor)
  - plot 10: 154 (flight master), with position kind 1
- Any other non-zero plot keeps class 138. For those, one random draw decides: the
  resident is created only when the draw modulo 10 is 0.
- A created resident has hostility 3, a race from the biome table, the village form
  and realm, and the petrify flag when it applies. Its appearance is filled
  (RVA 0x53E80) and it is equipped at level 0 (RVA 0x511E0). It gets a position of
  the chosen kind in the cell (RVA 0x31BD90) and behaviour value 4. The facing angle
  is drawn twice as rand/32767·360, and the second draw is the one kept. The
  resident is then adopted into the zone (RVA 0x119530).
- Plot 10 also creates a second resident: hostility 4, race 303 (the eagle), the
  petrify flag when it applies, equipped at level 0, placed at a position of kind 3.
  The eagle is raised by an amount that depends on its height (the multiplier
  register was not resolved). It has behaviour 4 and a drawn angle.

Guild hall (confirmed). The cell's building type is 91 (0x5B). This is also where
the debug text "GUILD HALL CENTER" is printed.
- One resident is created with hostility 3, class 155, a race from the biome table,
  the village form and realm, and the petrify flag when it applies.
- It is equipped at level 3.
- It stands at the cell centre (cell·16+8) at the height of the guild-hall structure
  (RVA 0x2D87F0).

Wanderers (confirmed). This applies to any other village cell.
- One random draw must be 0 modulo 20, and (cellX+cellY) modulo 2 must be 0
  (RVA 0x31B100).
- A second draw then decides what is created:
  - When the draw modulo 10 is not 0: a villager with hostility 3, class 138, a race
    from the biome table, the village form and realm, and the petrify flag when it
    applies. Its behaviour is 0x10, plus 0x20, 0x08 and 0x04, each added on its own
    random parity draw.
  - Otherwise: a pet with hostility 4. Its race comes from a static 8-entry list
    (the race ids were not resolved here) and its behaviour is 0x10.
- Both are placed at a kind 1 position.

### 3.2 Arena champion: RVA 0x2FF6F0, structure section (confirmed)

- In the building-structure part of the same zone routine, a cell whose building
  type is 97 (0x61) first gets its structure built and stacked.
- Then a resident is created at RVA 0x303436 onward: hostility 3, class 161, facing
  angle 270.0.
- This happens outside the village-influence loop described in 3.1. Whether the
  place kind also gates it was not resolved.

### 3.3 Daily residents: zone daily update, RVA 0x31C540 (confirmed in part)

- The routine reseeds the random generator from the world day plus a hash of the
  zone position. The hash is salted with 4420577 (0x4373E1).
- Inn guests (confirmed): this runs when the zone's resident-position list
  (zone +0x500, filled from inn plots) is not empty.
  - Count: a draw modulo (list size + 1), raised to at least 2.
  - Each guest has hostility 3, level drawn modulo 5 and a race from the general
    folk race table (RVA 0x1408E0 with argument -1, then RVA 0x1420F0).
  - Its behaviour is 0x20 and its class is drawn 1 to 4.
  - Guests are seated on the listed positions in turn.
- Gem trader (confirmed): in the visitors part, one draw selects the gem trader.
  The visitor group then becomes a single resident of race 304 (gnome) and class
  156.
- Lead only (not checked): the gating before that draw. The reconstruction has
  visitors on every fifth day, staggered by zone x + 3y, in one zone of five, with
  a 1-in-4 draw for the gem trader. The other visitor groups (faction-dependent,
  possibly hostile) were not covered.

### 3.4 Other assigners seen

- RVA 0x12A420 (confirmed): a resident namesake setter. When a named variant exists
  for a special place and the place type is 1, 2 or 3, it sets the resident's class
  to 138. It also sets the resident's race and name.
- RVA 0x126160 (confirmed write of 141 and 143 only): mission/faction residents.

## 4. Talk/use behaviour (RVA 0x98180, interaction prompt; confirmed)

### 4.1 When the prompt appears

The prompt appears for a creature with hostility 3 whose squared distance from the
local player is less than the square of the target's interaction range
(RVA 0x5EC30).

### 4.2 Petrified target

If bit 0x400000 of creature +0x80 is set, the prompt reads "Examine" instead of
"Talk". On use, the routine builds a speech script with these parts:
1. "define_model" with the target's race;
2. "he_is_petrified".

The script is shown with a seed of the creature id modulo 100000. Nothing else
happens: no shop opens and no interaction is sent.

### 4.3 Any other friendly target

The prompt reads "Talk". On use, the routine branches on class as follows.

- Classes 128, 129, 130 or 156: opens the vendor panel and switches the interface
  to the inventory/equipment tab. It records the target's id and its class (as the
  vendor kind) in the game state and rebuilds the vendor stock (RVA 0xB75D0).
- Class 131: opens the enchant/analyse panel and the same tab.
- Class 154: sets the game's map-open flag and the world map's flight-master flag.
- Any other class:
  - If a speech bubble from this creature is already shown, the routine advances
    or extends that bubble.
  - Otherwise it builds the talk script (RVA 0x2D9480) and says it to the player
    (RVA 0xA3170). It then queues an interaction record of kind 3 for the target on
    the local player (RVA 0x98040).

The talk script routine (RVA 0x2D9480) works as follows:

- Class 155 (guild receptionist): "hello", "how_can_i_help_you", then
  "define_specialization" with the player's class and the other specialization
  (1 minus the current one), then "change_specialization?" and "goodbye?".
- Class 132 (innkeeper): first "define_number" 10, "hello" and "new_page". After
  that:
  - Outside the night window: "you_can_rest_here_between_six_and_six". The window
    runs from game time 64,800,000 ms (18:00) and through 21,600,000 ms (06:00),
    inclusive at both ends.
  - At night, if the player's gold dword (player +0xAAC) is below 10:
    "you_dont_have_enough_gold_to_rest_here".
  - Otherwise: "how_can_i_help_you", "rest_inn?" and "goodbye?".
- Every other class: the script comes from the mission/region talk generator
  (RVA 0x12A6F0) for the region of the creature's origin. That generator branches
  on class itself:
  - 161: the arena opening;
  - 138: the villager rumour branch (monster, item, gnome and witch rumours);
  - 1 to 4: class banter;
  - not 138: a hint-topic line in one sub-case. The topics are #glassflasks,
    #sugarcubes, #bombs, #petfood, #flight, #buyweapon, #craftweapon, #buyarmor,
    #craftarmor, #identify, #specialization, #villagers, #friends and #sell, and the
    line includes "im_in_this_zone_to_do_tasks".
  - The full sub-case conditions were not documented here.

Lead only, not verified here: what the answers to "rest_inn?" and
"change_specialization?" do, and what the world does with the kind 3 interaction
record. The reconstruction has kind 3 start the arena champion's fight and the
innkeeper's rest.

### 4.4 Shop supplier (class 157), confirmed in outline at RVA 0x8AE50

In the per-frame game update, when the player looks at a creature, the routine
checks whether the creature reveals a mission destination. A hostility 3 creature
qualifies only if its class is 157. A captive appearance flag is also tested.
Lead: the reconstruction says such a creature reveals its destination, and the
captive flag is 0x1000.

### 4.5 Display label

The display label comes from RVA 0xB0680, as listed in the table in section 2.

## 5. Petrified/statue flag

- The flag is bit 0x400000 of creature +0x80 (confirmed).
- Effect on interaction: the interaction prompt (RVA 0x98180) shows "Examine" with
  the "he_is_petrified" text and gives no Talk or shop access.
- Village generation (RVA 0x2FF6F0) sets it as described in 3.1.
- Other routines also test the bit: RVA 0x17AB0 and RVA 0x2BB8E0. Their roles were
  not examined.
- Lead only (not confirmed): the reconstruction says the petrified creature is drawn
  grey, cannot be damaged, and is un-petrified when the witch is defeated.

## 6. Explicit limits

- RVA 0x2FF6F0 has no decompiler output, so its facts come from disassembly. The
  helpers whose meaning was taken from their call shape are "covered", "street
  door", "plot", "structure level", "race from table", "position of kind" and
  "adopt". Their internals were not opened.
- The meanings of the biome numbers and race ids are not verified: the values in
  the race table, 202, 303 and 304. "Eagle" and "gnome" are inferred from the role
  and the reconstruction. The ids of the 8 pets were not resolved.
- No generating site was found for classes 136, 137, 139, 157, 159 and 160, or for
  the class-master bit 0x40. The search covered resident class byte immediates, the
  zone generation and daily update routines, and the 0x40 OR forms. A byte stored
  through a register elsewhere (for example in mission or structure generators not
  examined) could still assign them. Treat them as "labels exist; spawn source
  unresolved", not as "never spawned".
- The castle/structure residents and the mission-spawned residents were not
  surveyed, apart from the sites cited.
- Daily-visitor gating constants are lead only.
- The two random facing-angle draws and the draw order are stated as observed. The
  random draws consumed inside the race, appearance and equip helpers were not
  enumerated.
- The noise function used by the squared distance (RVA 0x2E0C0) was not opened. Its
  output range, and therefore the exact maximum displacement, is not stated.
- No runtime or owner-capture verification was done in this role.

## Exposure

- The harness put the repository's AGENTS.md project-instruction text and a
  git-status snapshot into this author context at session start. They contain only
  project policy, not implementation material. They were not acted on beyond the
  assigned role and are not repeated here.
- No other repository file was read apart from docs/CLEANROOM_AUTHOR.md and this
  output file. No SDK headers or mod source were read.
- Reconstruction files read as leads (Infinite-Cubes release tree):
  include/cube/CreatureIds.h (class and appearance-flag catalogue), excerpts of
  src/cube/Zone.cpp, src/cube/Game.cpp, src/cube/World.cpp and src/cube/Missions.cpp,
  and grep hit lines from src/gfx/Renderer.cpp, src/cube/Creature.cpp,
  src/cube/Creature/Animation.cpp and src/cube/World/AI.cpp.
