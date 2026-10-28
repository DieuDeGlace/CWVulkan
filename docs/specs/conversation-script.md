# Conversation script of a townsperson

Behavioural specification of how Steam Cube World builds what a creature says when the player
talks to it. Sources: the decompiled executable and the binary's constants. The results were
checked against the game's own exported scripts (`Extract/talk-scripts.txt`).

Routines are cited by RVA (image base 0x140000000). Offsets are byte offsets into the record
named. World positions are signed 64-bit integers in 1/65536 of a block.

The builder is 0x2d9480. It handles two special speakers itself and passes everyone else to
the townsperson part at 0x12a6f0. The result is a list of text tokens: the **script**. The
speech system later plays it line by line. That player is not part of this spec, except for
what each token means (§7).

---

## 0. Conventions

* **Unsigned 32-bit.** Every draw, hash and counter is an unsigned 32-bit value. Addition,
  multiplication and shifts wrap modulo 2^32. "a mod m" of such a value is the ordinary
  non-negative remainder. The game computes it on the 64-bit zero-extended value, which gives
  the same result.
* **Signed decimal output.** A number becomes a token as **signed 32-bit decimal** (§7.2). So a
  draw of 0x80000000 or more prints as a negative number.
* **Blocks from a position.** "trunc(p)" of a position p means p / 65536 rounded **toward
  zero**. The game adds 65535 to a negative value before shifting right by 16.
* **Place cell of a block coordinate.** "cell(b)" means floor(b / 1024), rounded toward minus
  infinity: −1 → −1, −1024 → −1, −1025 → −2 (routine 0x9beb0).
* **Rounding a place cell to even.** "even(x)" means floor(x / 2) × 2, with floor division
  again: −3 → −4, −1 → −2, 3 → 2.

---

## 1. Inputs

### 1.1 The speaking creature (its record, the "speaker")

| Offset | Type | Use |
|---|---|---|
| +0x08 | u64 | Creature id. Feeds the seed (§2.2). Its low 32 bits are printed by the greeting's `define_mob` and feed the choice of help request (§4.6). |
| +0x10, +0x18 | i64 | Current position x, y. Used for `define_direction` (§4.6) and the weather remark (§4.10, case 3). |
| +0x60 | byte | Hostility kind. 3 = friendly townsperson (the exporter only asks about these). |
| +0x64 | u32 | Race. |
| +0x80 | u32 | Appearance flags ("flags2" in the export). Bit 0x2000 marks a **mission resident**. |
| +0x128 | u16 | State toggles. Bit 0x0002 gives the **alarm call** (§4.1). Bit 0x0100 means the resident's **task is done** (§4.4). |
| +0x14c | byte | Class. 1 warrior, 2 ranger, 3 mage, 4 rogue, 132 innkeeper, 138 (a villager kind, see §4.5 and §4.9), 155 specialisation trainer, 161 arena master. Others (0, 128-131, 133, 135, 141, 143, 154, 156, ...) are ordinary townspeople here. |
| +0x19c | byte | Interaction stage, read only for the arena master (0xff = the fight is finished). |
| +0x1b8, +0x1c0 | i64 | **Origin** (home) x, y: where the creature was spawned. |

### 1.2 The player (the local player's record, world+0x448)

| Offset | Type | Use |
|---|---|---|
| +0x14c | byte | Class (1..4). |
| +0x14d | byte | Specialisation (0/1). |
| +0x150, +0x154 | i32 | The player's current region. Used by the special-item test (§1.4, 0x5e890). |
| +0x278 + k×0xa0, k = 0..9 | item | The ten equipment slots, in this order: an unnamed first slot, neck, chest, feet, hands, shoulder, left weapon, right weapon, left ring, right ring. A slot is filled when its first byte (the item category) is not 0. |
| +0x8b8 | item | The pet slot. The category byte at +0x8b8 is not 0 when the player has a pet. The i32 at +0x8bc is the pet's id (the creature kind). |
| +0x9f0 .. +0x9f8 | list | Inventory. A list of tabs, each a list of 0xa4-byte stacks: an i32 count, then a 0xa0-byte item. |
| +0xaac | i32 | Gold. |
| +0x1528 | pointer | The player's mission data, or none. It holds three sets of place cells (x, y): **known** (+0xb8), **solved** (+0xc8) and **finished** (+0xd8). |

Item record (0xa0 bytes), the fields used here:

| Offset | Field |
|---|---|
| +0x00 | category (byte) |
| +0x04 | id (i32) |
| +0x08 | modifier (u32) |
| +0x0c, +0x10 | region x, y (i32) |
| +0x14 | rarity (byte) |
| +0x18 | formula / realm (i32) |
| +0x1c | material (byte) |
| +0x1d .. | spirits (32 × 4 bytes) |
| +0x9d | spirit count (byte) |

### 1.3 The world

| Offset | Type | Use |
|---|---|---|
| +0x08 | i32 | The **day**. |
| +0x0c | f32 | Time of day, in milliseconds since midnight. Read only for the innkeeper. |
| +0x360 .. +0x368 | list of i32 | The **pet list**: the creature kinds that can be pets. In the exported data these are also the ids of their pet-food items (category 21). CubeVulkan already loads this list as `specials` (src/Spawned.h, src/NativeDrop.h, from Extract/oracle/items.bin). |
| +0x448 | pointer | The local player. |

### 1.4 World queries

The table names each routine the script needs, what it gives, and what CubeVulkan already
has. "gamegen::" names are in src/GameGen.h. "GameGen.cpp internal" means the function
exists in src/GameGen.cpp but is not in the header. Routines that CubeVulkan lacks, or has
only in a different form, are described in §5.

| RVA | Gives | In CubeVulkan |
|---|---|---|
| 0x2ada20 | The region (8192-block cell) a block point belongs to. | `gamegen::regionAt(x, y)` |
| 0x115df0 | The region's **planner**: roles, levels, artifacts and the special town of its places (§1.5). | `RegionRoles` / `makeRegionRoles` (GameGen.cpp internal), partly exposed as `gamegen::regionRolesOf`. The two lists in §1.5 must be derived from the role map. |
| 0x093480 | The role of a place cell in the planner, or 0 if it has none. | `RegionRoles::roleAt` |
| 0x12a200 | The level of a place cell in the planner, or 0. | `RegionRoles::levelAt` |
| 0x036160 | Whether a place cell is "big" (it then spans 2×2 cells anchored at the even corner). | `gamegen::bigCell(cx, cy)` |
| 0x2b99d0 | The kind of landmark (1..47, 0 = none) in a place cell, given its big flag. | `gamegen::placeKind(cx, cy, big)` |
| 0x09be00 | placeKind(cx, cy, bigCell(cx, cy)). | as above |
| 0x129150 | The same as 0x9be00. Used for the home cell after rounding. | as above |
| 0x0362a0 / 0x09be50 | The landmark's point (in blocks) in a place cell. 0x9be50 computes the big flag itself. | `gamegen::cellPoint(cx, cy, big)` |
| 0x0364e0 | Squared, noise-warped distance from a block point to a landmark point. | `gamegen::warpedDist2(centre, x, y)` |
| 0x09bee0 | The **place weight** (§5.8). | not exposed, see §5.8 |
| 0x2bb060 / 0x09bf20 | The **realm** of a place cell. 0x9bf20 computes the big flag itself; for a big cell the realm is that of its even corner. | `gamegen::placeRealm(x, y)` (computes the big flag itself) |
| 0x119660 | The artifact the planner gives a role-14 place. | `gamegen::placeArtifact(x, y)` |
| 0x2aa270 | A big place's "big structure" kind (0 = none). | GameGen.cpp internal `bigStructure` (§5.8) |
| 0x119a00 | A place's **encounter** number. | GameGen.cpp internal `encounterKind`. The public `placeEncounter` remaps the value, so use the description in §5.5. |
| 0x131540 | A place's role treasure: whether it has one. | GameGen.cpp internal `roleTreasureAt` |
| 0x2b9080 | A region's biome (the "land"). | `gamegen::biome(rx, ry)` |
| 0x2ff5c0 | R(x + 57·y) (§2.4). Used as the "land seed" of a region. | `gamegen::rnd` |
| 0x035ce0 | The place around a block point: kind, weight, ... | `gamegen::placeAt(x, y)` (fields `kind`, `weight`) |
| 0x0358e0 | The 3×3 regions around a point. | `gamegen::neighbourhood(x, y)` |
| 0x2d9b60 | Warmth of a point, 0..1. | `gamegen::warmth(neighbourhood, kind, weight)` |
| 0x05e430 | Mission **completed**: the cell is in the player's solved set or finished set. False if the player has no mission data. | `missions::completed` |
| 0x05e650 | Mission **finished**: the cell is in the finished set (+0xd8) only. | the `finished` set |
| 0x05f4c0 | Mission **known**: the cell is in the known set (+0xb8) or the solved set (+0xc8). | needs the known set (+0xb8). CubeVulkan saves marked places with the character. |
| 0x05e890 | The player has special item id n: n = 4 always counts. Otherwise the player has at least one item of category 24, id n, whose region equals the player's current region (+0x150, +0x154), with every other field 0. Items are compared as in 0x4df60: all fields, spirits included. | `missions::World::hasSpecial` |
| 0x05e780 | The player carries at least one of a given item, compared as in 0x4df60 and counted over all tabs. | `missions::World::carries` |

### 1.5 The planner (result of 0x115df0)

The script uses these fields of the planner built for the speaker's region:

| Offset | Content |
|---|---|
| +0x00 | the world |
| +0x08, +0x0c | the region x, y |
| +0x10 | the **role map**: place cell (x, y) → role. Sorted ascending by x, then by y (signed). A big place is keyed by its even corner. |
| +0x20 | the artifact map: place cell → item (category 23) |
| +0x30 | the **help-target list** (below) |
| +0x48 | the **artifact-place list** (below) |
| +0x60 | the level map |
| +0x80 | the **special town** cell (x, y), or (0x7fffffff, 0x7fffffff) if none |

The two lists are built at the end of 0x115df0 by walking the role map in its sorted order:

* **Help-target list (+0x30).** Every cell whose role is not 0 and not one of 13, 14, 15, 17,
  21, 22, 23. In practice that is roles 1-12, 16, 18, 19 and 20.
* **Artifact-place list (+0x48).** Every cell whose role is 14.

Both keep the map order. CubeVulkan's `RegionRoles::role` is a `std::map` keyed the same
way, so walking it in order gives the same lists.

---

## 2. Random numbers

### 2.1 The xorshift step X

X(n), for a 32-bit n:
1. t = n XOR (n shifted left by 13)
2. t = t XOR (t shifted right by 17)
3. X(n) = t XOR (t shifted left by 5)

The game sometimes skips step 3 when it only tests the lowest 1, 2 or 3 bits. Step 3 does not
change those bits, so X can always be used. Test vectors: X(0) = 0, X(1) = 270369,
X(12345) = 3336926330.

Notation used below:
* "**odd(n)**": X(n) is odd.
* "**X(n) & m**": the low bits (m = 3 or 7).
* "**X(n) mod m**": the remainder.

### 2.2 The script seed

1. id = the speaker's id (+0x08) as **unsigned** 64-bit. The export prints it signed, so
   negative printed ids must be read back as 2^64 + id.
2. s = (id mod 333333) + 13 × day, in 32-bit wrapping arithmetic. The day is world+8, signed.
3. c = X(s).

c is the base of every draw in §4. Example: id 9223364892176678848, day 135 gives
id mod 333333 = 127732, s = 129487 and c = 3627600947.

The first draws are:
* X(c) = 4133966188: & 7 = 4, so there is no artifact rumour;
* X(c+1) = 4135301515: & 3 = 3, so there is no help request;
* X(c+2) = 4135555498: mod 3 = 1, so there is a greeting.

The game's script for that day begins `hello | new_page | nice_to_meet_you | new_page`.

Inside the routine, the draws are X(c), X(c+1), X(c+2), ...: consecutive inputs, each passed
through X once. Each section below says which offsets it uses. Where a sub-step "takes the
next values", the offsets continue from where the previous step stopped.

### 2.3 The greeting counter

Two sub-routines (§5.1 and §5.2) take a counter g by reference, read X(g), X(g+1), ... and
leave g advanced. §4 says which value g has at each call.

### 2.4 Integer noise N and R (used by world functions)

* N(n) = ((m × (m × m × 60493 + 19990303) + 1376312589) AND 0x7fffffff), where m = n XOR (n
  shifted left by 13), all in 32-bit wrapping arithmetic. The binary adds 0xd208dd0d, which
  is the same after the mask.
* R(n) = 100000 − floor(N(n) / 21474). The binary divides with a multiply and shift,
  N × 0x61A8F94D shifted right by 45, which equals floor(N / 21474) for every 31-bit N.

This is `gamegen::hash` and `gamegen::rnd`. Test vectors: N(0) = 1376312589, R(0) = 35908,
R(1234567) = 79496.

### 2.5 The C runtime's rand()

Two places use the C library's global generator:
* §4.10 case 18 seeds it with srand.
* §5.6 draws from it **without** seeding.

It is the Microsoft one:
* state = state × 214013 + 2531011 (mod 2^32)
* result = (state shifted right by 16) AND 0x7fff

srand(v) sets state = v. Test vector: after srand(1), the draws are 41, 18467, 6334.

---

## 3. The builder (0x2d9480)

Inputs: the world, the speaker. Output: the script. Evaluate in this order:

1. **Specialisation trainer (class 155).** The script is: `hello`, `how_can_i_help_you`,
   `define_specialization`, *player class*, *1 − player specialisation*,
   `change_specialization?`, `goodbye?`.
   The two numbers come from the player's +0x14c and +0x14d bytes (unsigned), printed as
   signed decimals: spec 0 gives 1, spec 1 gives 0.
2. **Innkeeper (class 132).** The script is `define_number`, `10`, `hello`, `new_page`,
   followed by:
   * If time of day (world+0xc) ≥ 64,800,000 or ≤ 21,600,000 (18:00 to 06:00, float
     comparison):
     * if player gold (+0xaac) < 10: `you_dont_have_enough_gold_to_rest_here`;
     * otherwise: `how_can_i_help_you`, `rest_inn?`, `goodbye?`.
   * Otherwise: `you_can_rest_here_between_six_and_six`.
3. **Everyone else.**
   1. Find the region: regionAt(trunc(origin x), trunc(origin y)), from the speaker's
      +0x1b8 and +0x1c0, in blocks.
   2. Build the planner for that region (0x115df0).
   3. Run the townsperson part (§4) with the planner, the speaker and the world. Its result is
      the script.

---

## 4. The townsperson part (0x12a6f0)

Compute c (§2.2) first. Then take the steps below **in order**. The first one that produces a
script ends the routine.

Terms used throughout:

* **H** = the speaker's home cell: hx = cell(trunc(origin x)), hy = cell(trunc(origin y)).
* **big(H)** = bigCell(hx, hy).
* **H′** = H rounded to even when big(H), otherwise H.

### 4.1 Alarm call (toggles bit 0x0002)

If bit 0x0002 of the speaker's toggles (+0x128) is set, the script is the alarm call (§5.2)
with counter g = c. Stop.

### 4.2 Races 257 and 258

If the race (+0x64) is 257 or 258, the script is `hello`. Stop. (The game tests
race − 257 < 2, unsigned.)

### 4.3 Arena master (class 161)

* If the interaction stage (+0x19c) is 0xff: `congratulations`. Then, if odd(c):
  `new_page`, `good_work`.
* Otherwise: `a_new_challenger_entered_the_arena`.

Stop.

### 4.4 Mission resident

This applies when hostility (+0x60) is 3 **and** flags2 (+0x80) has bit 0x2000.

1. Compute H, big(H) and H′ = (x, y). kind = placeKind(x, y, big(H)).
2. **Active task.** This applies when toggles bit 0x0100 is clear **and** the cell is not in
   the player's finished set (0x5e650, applied to H′).
   1. Emit `define_location`, x, y, kind.
   2. e = the encounter of H′ (§5.5, with the speaker's planner).
   3. If e is in the table below:
      1. Run the alarm call (§5.2) with g = c.
      2. Emit `new_page`.
      3. If the row has a first model M1: `define_model`, M1.
      4. Emit the row's "full of" line, then `new_page`.
      5. If the row has a second model M2: `define_model`, M2, `you_must_destroy_the_objects`.
         Otherwise: `you_must_defeat_them`.

      For any other e, nothing more is emitted. Stop.
3. **Task done** (otherwise):
   1. Emit `thank_you`.
   2. If odd(c): `new_page`, `good_work`.
   3. If odd(c+1): `!love`.
   4. Stop.

| e | M1 | "full of" line | M2 |
|---|---|---|---|
| 2 | −99996 | `the_location_is_full_of_creatures` | 239 + (R(x + 57·y + 0x1c52c9) mod 3) (routine 0x12a690) |
| 3 | −99997 | `the_location_is_full_of_creatures` | 175 |
| 4 | 96 | `the_location_is_full_of_creatures` | 178 |
| 5 | 17 | `the_location_is_full_of_creatures` | 179 |
| 6 | 37 | `the_location_is_full_of_creatures` | 176 |
| 7 | 87 | `the_location_is_full_of_creatures` | 180 |
| 8 | 53 | `the_location_is_full_of_creatures` | 177 |
| 18 | 37 | `the_location_is_full_of_creatures` | — |
| 19 | −99994 | `the_location_is_full_of_creatures` | — |
| 20 | −99995 | `the_location_is_full_of_creatures` | — |
| 21 | −99996 | `the_location_is_full_of_creatures` | — |
| 22 | — | `the_location_is_full_of_insects` | — |
| 23 | — | `the_location_is_full_of_plant_creatures` | — |

The negative models are the binary's constants 0xfffe7963..0xfffe7967, printed signed.

### 4.5 Artifact rumour (first draw)

If X(c) & 7 = 0:
1. **realm** = placeRealm(hx, hy), the realm of the home cell. Unrounded H is passed; the
   routine rounds it itself.
2. **Candidates**: the artifact-place list (§1.5, +0x48) in order, keeping each cell whose
   placeRealm equals **realm**.
3. If there is at least one candidate:
   1. Pick T = candidates[X(c+1) mod count].
   2. item = placeArtifact(T).
   3. carried = the player carries item (0x5e780).
   4. The game also computes T's kind and big-structure kind here (0x9be00, 0x2aa270). Both
      are passed on but have **no effect** on the output.
   5. The script is the artifact rumour (§5.3) with: realm, item, seed s = c+1, carried. Use
      the class-138 variant if the speaker's class is 138. Stop.
4. With no candidates, go on to §4.6. (This happened 217 times in the export.)

### 4.6 Help request (second draw)

If X(c+1) & 3 = 0 (this runs whether or not the artifact draw passed):

1. **Pick the target T** from the help-target list (§1.5, +0x30). The index is
   X(13 × day + low32(id)) mod count, in 32-bit wrapping arithmetic.
   * This uses neither c nor the counter. A person points at the same place all day.
   * If the list is empty the game divides by zero. It is never empty in practice.
2. Compute:
   * kind = placeKind(T, bigCell(T));
   * P = cellPoint(T, bigCell(T)), in blocks;
   * dx = trunc(P.x × 65536 − speaker +0x10);
   * dy = trunc(P.y × 65536 − speaker +0x18).

   The products and differences are 64-bit; truncation is toward zero.
3. Emit `new_page`, `define_location`, T.x, T.y, kind, `define_direction`, dx, dy.
4. role = roleAt(T).
5. A **greeting** below means §5.1 with a counter g that starts at **c+2**. It always leaves
   g = c+4.
6. "Completed", "known" and "has" are the player tests of §1.4 at T.
7. "**mark**" means emit `mark_location`, T.x, T.y.

The remaining tokens depend on the role.

#### Roles 2-13: a special item

The item has category 24 and the id below. Its other fields are 0.

| role | item id | item |
|---|---|---|
| 2 | 14 | Magic Book of Crafting |
| 3 | 15 | Rare Book of Crafting |
| 4 | 16 | Epic Book of Crafting |
| 5 | 17 | Legendary Book of Crafting |
| 6 | 1 | Hang Glider |
| 7 | 2 | Boat |
| 8 | 18 | Climbing Spikes |
| 9 | 3 | Reins |
| 10 | 5 | Harp |
| 11 | 7 | Sky Whistle |
| 12 | 8 | Spirit Bell |
| 13 | 9 | (role 13 is never in the help-target list, so this is unreachable) |

Steps:

1. Emit `define_item`, 24, id, 0, 0, 0.
2. **If the player has the special item** (0x5e890 with this id):
   1. If odd(c+2): `!alert`.
   2. Emit `oh_you_already_have_the_item`, `new_page`.
   3. **If id = 8:** emit `new_page`, `spooky`, `!sweat`. Stop.
   4. Otherwise emit `nice`. If odd(c+3): `!love`.
   5. Let k = c+4. If id is 14..17: emit `new_page`, `im_curious_what_you_will_craft_with_it`
      when odd(c+4), and in either case set k = c+5.
   6. If id is 1, 2, 18, 7, 5 or 3, and odd(k): emit `new_page`, then the remark below with
      r = X(k+1). Otherwise nothing more. Stop. No mark.

      | id | remark |
      |---|---|
      | 1 | r mod 5: 1 `i_wish_i_could_fly`; 2 `arent_you_afraid_of_heights`, `!sweat`; 3 `how_far_can_you_fly`; 4 `how_high_can_you_fly`; 0 `how_does_it_feel_to_fly` |
      | 2 | r & 3: 1 `i_wish_you_a_safe_journey`; 2 `id_love_to_sail_from_island_to_island`; 3 `my_relative_was_a_sailor`; 0 `i_wonder_whats_on_the_other_side_of_the_ocean` |
      | 18 | r mod 3: 1 `be_careful_during_climbing`; 2 `arent_you_afraid_of_heights`, `!sweat`; 0 `whats_the_tallest_mountain_that_you_climbed` |
      | 7 | r odd: `it_has_a_nice_sound`, else `i_wonder_if_the_bird_statues_have_to_do_with_it` |
      | 5 | r odd: `it_has_a_nice_sound`, else `it_looks_so_nice` |
      | 3 | r mod 5: 1 `i_wish_i_could_ride`; 2 `i_would_ride_through_the_land`; 3 `how_fast_can_you_ride`; 4 `where_will_you_ride_next`; 0 `whats_your_favorite_pet_to_ride` |
3. **If the player does not have it, and the mission is not known:**
   1. Run a greeting, then emit `new_page`. Let u = c+4.
   2. Emit `define_mob`, X(u), friend race. X(u) is printed signed. The friend race is the
      sibling race of the speaker's race (§5.6, which uses the unseeded C rand).
   3. Branch on q = X(u+1) mod 5:
      * **q = 1:** if odd(u+2) `!alert`; emit `i_saw_an_item_in_the_zone`; if odd(u+3)
        `new_page`, `i_dont_need_it_and_left_it_there`.
      * **q = 2:** let k = u+3. If odd(u+2): emit `!question` when odd(u+3), else `!alert`,
        and set k = u+4. Emit `do_you_need_an_item`, `new_page`. Then by X(k) mod 3:
        1 `my_friend_found_one_in_the_location`;
        2 `there_are_rumors_about_an_item_in_the_location`;
        0 `i_found_one_in_the_location`.
      * **q = 3:** let k = u+3. If odd(u+2): if odd(u+3) emit `!question`; emit
        `i_dont_know_if_its_true_but`, `new_page`; set k = u+4. Then if odd(k):
        `my_friend_found_an_item_in_the_location`, else
        `there_are_rumors_about_an_item_in_the_location`.
      * **q = 4:** `my_friend_is_an_adventurer`, `new_page`,
        `when_he_was_exploring_the_location_he_found_an_item`.
      * **q = 0:** if odd(u+2) `i_was_exploring_the_location`, else
        `i_was_gathering_ingredients_in_the_location`; then `new_page`; if odd(u+3) `!alert`;
        emit `i_saw_an_item`; if odd(u+4) `new_page`, `i_dont_need_it_and_left_it_there`.
   4. **mark.** Stop.
4. **If the player does not have it, but the mission is known:**
   * If odd(c+2): if odd(c+3) `!question`; emit `havent_you_found_the_item_yet`, `new_page`.
     Then by X(c+4) mod 3:
     * 1: if odd(c+5) `!angry`; emit `what_a_shame`.
     * 2: `i_wish_you_good_luck_finding_it`; if odd(c+5) `!love`.
     * 0: `it_should_be_in_the_location`.
   * Otherwise: `i_wish_you_good_luck_finding_the_item`; if odd(c+3) `!love`.

   No mark. Stop.

#### Role 1: a missing supplier gnome

* **If completed:** emit `so_you_already_helped_the_gnome_in_location`, `new_page`, `nice`.
  If odd(c+2) emit `!love`. **mark.**
* **If not completed and not known:**
  1. Run a greeting, then emit `new_page`. Let u = c+4.
  2. If odd(u): emit `a_supplier_gnome_went_missing_in_the_location`; set k = u+1.
     Otherwise:
     1. Emit `i_was_exploring_the_location` if odd(u+1), else
        `i_was_gathering_ingredients_in_the_location`.
     2. Emit `new_page`; if odd(u+2) emit `!alert`; emit `i_heard_a_gnome_screaming`.
     3. Set k = u+3.
  3. Emit `new_page`, `define_model`, captor, `he_was_captured_by_creatures`. The captor
     comes from e = the encounter of T (§5.5):

     | e | captor model |
     |---|---|
     | 10, 14 | 46 |
     | 11 | 45 |
     | 12 | 17 |
     | 13 | 269 |
     | 15 | 227 |
     | 16 | 226 |
     | any other | −99993 |

  4. By X(k) & 3: 0 emit `!angry`; 1 emit `!sweat`; otherwise nothing.
  5. If odd(k+1): `new_page`, `i_hope_someone_is_going_to_help_him`.
  6. **mark.**
* **If not completed but known:**
  * If odd(c+2): if odd(c+3) `!question`; emit `havent_you_found_the_gnome_yet`, `new_page`,
    `he_should_be_in_the_location`.
  * Otherwise: `i_wish_you_good_luck_rescuing_the_gnome_in_the_location`; if odd(c+3)
    `!love`.

  No mark.

#### Role 16: a monster

1. Emit `define_model`, monster. The monster comes from routine 0x1292f0 with T and its
   kind:
   * if kind is 23 or 36: 107;
   * otherwise the list (108, 109, 110, 111, 112, 113, 115, 115, 116, 117, 118, 119), indexed
     by R(T.x + 57·T.y + 0x2b3a706) mod 13.

   The list's 13 entries are 108-113, 115 twice, then 116-119. 114 is absent.
2. **If completed:**
   1. By X(c+2) & 3: 0 `!alert`; 1 `!question`; otherwise nothing.
   2. Emit `so_you_defeated_that_monster_in_the_location`, `new_page`, `nice`.
   3. If odd(c+3): `!love`.
   4. **mark.**
3. **If not completed but known:** emit
   `i_wish_you_good_luck_finding_that_monster_in_the_location`. By X(c+2) & 3: 0 `!love`;
   1 `!sweat`; otherwise nothing. **mark.**
4. **If neither:**
   1. Run a greeting. Do **not** emit `new_page`.
   2. By X(c+4) & 3: 0 `!alert`; 1 `!sweat`; otherwise nothing.
   3. Emit `a_monster_was_sighted_in_the_location`. **mark.**

#### Role 18: items lying about (never occurs in practice)

1. Emit `define_item`, 22, id, modifier, 0, level. Each argument is built from T:
   * id = 48 + (T.x AND 1) if kind = 20, otherwise 27 + (T.x AND 1);
   * modifier = R(T.x + 57·T.y + 0xb9ad0e);
   * level = levelAt(T).

   The item's region is the planner's region and its +0x18 field is
   R(T.x + 57·T.y + 0x38fba). Neither is printed. The game computes the parity as
   ((v mod 2) + 2) mod 2 of v = T.y·64 + T.x, which equals T.x AND 1.
2. If not known: run a greeting and set u = c+4. Otherwise u = c+2.
3. If X(u) is even: emit `you_can_find_items_in_the_location`. Otherwise:
   1. Emit `are_you_looking_for_items`.
   2. If odd(u+1): `!alert`.
   3. Emit `new_page`, `you_can_find_some_in_the_location`.
4. **mark.**

#### Role 19: a witch

* **If not completed:**
  1. Emit `a_witch_lives_in_location_direction_of_here`, then **mark**.
  2. Emit `define_location`, S.x, S.y, placeKind(S, bigCell(S)), where S is the planner's
     special town (+0x80).
  3. Emit `she_has_petrified_the_villagers_of_the_location`, `!sweat`,
     `you_must_defeat_her_to_break_the_curse`.
* **If completed:** emit `!alert`, `so_you_defeated_the_witch`, `new_page`, `nice`. If
  odd(c+2): `!love`.

#### Role 20: a demon portal

* **If not completed:** emit:
  1. `a_demon_portal_was_seen_in_location_direction_of_here`;
  2. `define_land`, biome(region), R(region.x + 57·region.y) — the planner's region (§5.4);
  3. **mark**;
  4. `the_demons_are_possessing_the_residents_of_the_land`, `!sweat`.
* **If completed:** emit `!alert`, `so_you_destroyed_the_demon_portal`, `new_page`, `nice`.
  If odd(c+2): `!love`.

#### Any other role (0, 14, 15, 17, 21-23; unreachable from the list)

Only **mark**.

After any role, stop. Roles 1, 16, 18 and 2-13 got their item or model first; tokens are
emitted in the order written above.

### 4.7 Greeting

The counter g starts at c+3. If X(c+2) mod 3 ≠ 0:
1. Run the greeting (§5.1). It leaves g = c+5.
2. Emit `new_page`.

Let **u** = g, which is c+3 or c+5.

### 4.8 Adventurer remarks (speaker class 1-4 only)

If the speaker's class is 1..4:

1. **Equipment.** If X(u) & 3 = 0, list the player's filled equipment slots in slot order
   (§1.2). If the list is not empty:
   1. If odd(u+1): `!alert`.
   2. Pick item = list[X(u+2) mod count].
   3. Emit `define_item`, then the item's five arguments (§7.1), then `nice_equipment_item`.
   4. If odd(u+3), branch on X(u+4) mod 3:
      * 1: `new_page`, `im_jealous`; then if odd(u+5) `!angry`;
      * 2: `new_page`, `nice`; then if odd(u+5) `!love`;
      * 0: `!love`.
   5. Stop.

   With no filled slot, nothing is emitted and the routine goes on.
2. **Class.** If X(u+1) & 3 = 0:
   * **If odd(u+2):** emit `i_want_to_be_the_best_warrior` / `_ranger` / `_mage` / `_rogue`,
     by the **speaker's** class. Stop.
   * **Otherwise:**
     1. If odd(u+3): `!alert`.
     2. If the **player's** class is 1..4: `so_you_are_a_warrior` / `_ranger` / `_mage` /
        `_rogue`. Otherwise nothing.
     3. Emit `new_page`.
     4. If the player's class equals the speaker's class: emit `!love`. Then, if odd(u+4),
        emit `good_choice` and stop.
     5. If the classes differ: emit `!angry`.
     6. Emit `warriors_are_the_strongest` / `rangers_…` / `mages_…` / `rogues_are_the_strongest`
        by the **speaker's** class. Stop.
3. Otherwise go on with **v** = u+2.

For speakers of any other class, **v** = u.

### 4.9 Village activity

1. Let C = H′ (the home cell, rounded to even if big). Let big = big(H).
2. kind = placeKind(C, bigCell(C)) (routine 0x129150).
3. P = cellPoint(C, bigCell(C)).
4. d² = warpedDist2(P, trunc(origin x), trunc(origin y)).
5. weight = the place weight of (kind, big, d²) (§5.8).

The tests run in this order and stop at the first failure:
1. weight > 0.5 (strict, float);
2. kind = 5;
3. X(v) & 3 = 0.

If tests 1 and 2 pass, the counter moves on by one even if test 3 fails. So:
* **w** = v+1 when tests 1 and 2 passed;
* **w** = v otherwise.

When all three pass:
1. Emit `define_location`, C.x, C.y, 5.
2. If the speaker's class is not 138: emit one activity tag, from the table in §6.1 indexed by
   X(v+1) mod 15, then `im_in_this_zone_to_do_tasks`.
3. If the class is 138: `welcome_to_location` if odd(v+1), else `how_do_you_like_our_zone`.
4. Stop.

Otherwise go on to §4.10 with w.

### 4.10 Small talk

The topic is X(w) mod 19. In the table, "pet(n)" = the world's pet list
[X(n) mod length] (routine 0x12a160).

| Topic | Tokens |
|---|---|
| 1 | `define_model`, pet(w+1), `my_favorite_pet_is`; if odd(w+2) `!love`. |
| 2 | **Land history** (§5.4). This **replaces** the whole script: the greeting emitted in §4.7 is discarded. Arguments: realm = placeRealm(hx, hy) of the home cell; biome = biome(planner region); land seed = R(region.x + 57·region.y); counter s = w+1. |
| 3 | **Weather.** p = placeAt(trunc(position x), trunc(position y)), the current position (+0x10, +0x18). n = neighbourhood at the same point. t = warmth(n, p.kind, p.weight). If t > 0.8: `its_hot`, `!sweat`. Otherwise, if t > 0.6: `its_warm`, then `!love` if odd(w+1). Otherwise, if t ≥ 0.3: `its_pleasant`, then `!love` if odd(w+1). Otherwise: `its_cold`, then `!angry` if odd(w+1). The constants are the floats 0.8, 0.6, 0.3. |
| 4 | If odd(w+1) `!alert`; `i_like_your_style`; if odd(w+2) `!love`. |
| 5 | If odd(w+1) `!question`; `you_look_strange`; if odd(w+2) `!sweat`. |
| 6 | If odd(w+1) `!question`; `do_we_know_each_other`. |
| 7 | **The player's pet.** If the player has no pet (+0x8b8 = 0): if odd(w+1) `!question`; `dont_you_have_a_pet`; then, if odd(w+2), `!angry` if odd(w+3), followed by `what_a_shame`. If the player has a pet: `define_model`, the pet id (+0x8bc), `your_pet_is_adorable`; if odd(w+1) `!love`; if odd(w+2) `may_i_pet_it`. |
| 8 | `define_model`, pet(w+1), `i_have_a_new_pet`. If odd(w+2): `new_page`, then: if odd(w+3), `he_is_cheeky` and then `!sweat` if odd(w+4), else `!angry`; otherwise `i_love_him`, `!love`. |
| 9 | `i_like_your_name` |
| 10 | `are_you_an_adventurer` |
| 11 | `i_love_hang_gliding` |
| 12 | `do_you_like_hang_gliding`. If odd(w+1): `new_page` and the hang-glider remark of §4.6 (id 1) with r = X(w+2). |
| 13 | `are_you_a_good_climber`. If odd(w+1): `new_page` and the climbing remark (id 18) with r = X(w+2). |
| 14 | `do_you_like_sailing`. If odd(w+1): `new_page`, then by X(w+2) mod 5: 1 `i_wish_you_a_safe_journey`; 2 `id_love_to_sail_from_island_to_island`; 3 `my_relative_was_a_sailor`; 4 `i_dont_have_a_boat` followed by `!sweat` if odd(w+3), else `!angry`; 0 `i_wonder_whats_on_the_other_side_of_the_ocean`. Note this is mod 5, unlike the boat remark of §4.6. |
| 15 | By X(w+1) mod 6: 1 `i_love_the_beach`; 2 `i_love_the_river`; 3 `i_love_the_lake`; 4 `i_love_the_forest`; 5 `i_love_the_mountains`; 0 `i_love_nature`. Then `!love` if odd(w+2). |
| 16 | `define_model`, pet(w+1), `i_want_to_have_a_pet` |
| 17 | `!sweat` if odd(w+1), else `!angry`; then `i_dont_like_walking`; if odd(w+2): `new_page`, `i_prefer_riding`, `!love`. |
| 18 | **Strong creatures.** `define_model`, m, `creatures_are_really_strong`; if odd(w+2) `!sweat`. m comes from three steps: (1) srand(w+1); (2) choose a list by X(w+1) mod 3: 1 the beast list, 2 the golem list, 0 the undead list (§6.3); (3) entry = list[rand() mod length], then m = the sibling race of the entry (§5.6), which draws rand() again. |
| 0 | **Pet food.** f = pet(w+1). Emit `define_model`, f, `do_you_know_what_creatures_eat`, `new_page`. Then, if odd(w+2), `!question` if odd(w+3), else `!love`. Then `define_item`, 21, f, 0, 0, 0, `items_of_course`. |

Then stop.

---

## 5. Sub-routines

### 5.1 Greeting (0x128090)

Inputs: the speaker, a counter g.

1. Emit `hello`.
2. If X(g) & 3 = 0: emit `define_mob`, low32(id), race, `my_name_is`.
   * low32(id) is the speaker's id at +0x08, as a signed 32-bit number.
   * race is +0x64.
3. If X(g+1) & 3 = 0: emit `new_page`, `nice_to_meet_you`.
4. g becomes g+2, whatever was emitted.

### 5.2 Alarm call (0x128570)

Input: a counter g.

1. If odd(g): `!alert`.
2. If odd(g+1): `hey_you`, `new_page`.
3. Emit `help`.
4. If X(g+2) is even: `!angry`. Otherwise: `!sweat`.
5. g becomes g+3.

### 5.3 Artifact rumour (0x10b080; class 138: 0x112100)

Inputs: realm, item, seed s, carried.

The script **replaces** anything accumulated before (nothing has been at this point). Both
variants first emit:
1. `define_realm`, realmKind(realm), realm (§5.4.1);
2. `define_item`, then the item's five arguments (§7.1).

**Normal variant (0x10b080):**
1. By X(s) mod 3: 1 `im_looking_for_an_artifact`; 2 `there_is_a_mighty_artifact`;
   0 `there_is_a_legend_about_an_artifact`.
2. If not carried: `!question`, `i_wonder_where_it_could_be`.
3. If carried:
   1. `!alert` if odd(s+1), else `!question`.
   2. `you_found_the_artifact`, `new_page`.
   3. If odd(s+2): `nice`, then `!love` if odd(s+3).
   4. Otherwise: `im_jealous`, then `!angry` if odd(s+3).

**Class-138 variant (0x112100):**
1. `there_is_a_mighty_artifact` if odd(s), else `there_is_a_legend_about_an_artifact`.
2. Let b = odd(s+1).
3. If not carried: if b, `!question`, `i_wonder_if_it_really_exists`; otherwise
   `are_you_looking_for_it`.
4. If carried:
   1. `!alert` if b, else `!question`.
   2. `you_found_the_artifact`, `new_page`, `nice`.
   3. `!love` if odd(s+2).

### 5.4 Land history (0x10d4f0)

Inputs: realm R, biome B, land seed L, counter s.

The script **replaces** anything accumulated before, including the greeting of §4.7.

Header (always, in this order):
1. `define_realm`, realmKind(R), R;
2. `define_land`, B, L;
3. `define_number`, (X(R + 0x13309) mod 9000) + 100;
4. `define_ruler`, rulerTitle(R);
5. `define_mob`, X(R + 0x8144) (printed signed), rulerRace(R).

Then:
1. If odd(s): `history_time`, `new_page`.
2. **A perished civilisation** (when odd(R + 0xbd8b9)):
   * If odd(s+1):
     * If odd(s+2): `have_you_heard_about_the_realm`, `new_page`. Then
       `there_are_legends_about_realm_artifacts` if odd(s+3), else
       `you_can_still_find_realm_ruins_everywhere`.
     * Otherwise: `there_is_a_perished_civilization`, `new_page`,
       `you_can_still_find_realm_ruins_everywhere`.
   * Otherwise:
     1. Emit `many_years_ago_the_land_was_full_of_residents`, `new_page`,
        `their_ruler_was_famous`.
     2. If X(s+2) is even: stop.
     3. Emit `new_page`.
     4. If odd(s+3): magic(R), `he_was_a_master_of_realm_magic`.
     5. Otherwise: trait(R), `he_was_known_for_his_trait`. Then, if odd(s+4), the trait's
        emote (`!love` for a good trait, `!sweat` for a bad one).

   Stop.
3. **A living realm** (otherwise):
   * If odd(s+1):
     1. Emit `have_you_heard_about_the_realm`, `new_page`.
     2. By X(s+2) & 3:
        * 1: `there_are_legends_about_realm_artifacts`;
        * 2: `you_can_find_realm_ruins_everywhere`;
        * 3: `define_model`, nationModel(R, s+3), then
          `the_residents_are_a_realm_of_creatures` if odd(s+3), else
          `its_a_nation_of_creatures`;
        * 0: `there_are_still_some_residents_in_the_land`; then, if odd(s+3), `new_page`,
          `did_you_meet_any`.

     Stop.
   * Otherwise:
     1. Let t = s+3.
     2. If odd(s+2): emit `there_are_still_some_residents_in_the_land`; set t = s+4; if
        odd(s+3), emit `new_page`, `did_you_meet_any`.
     3. If X(s+2) is even: emit `did_you_meet_any_residents`.
     4. Emit `new_page`.
     5. By X(t) mod 3:
        * 1: `there_are_legends_about_realm_artifacts`;
        * 2: `you_can_find_realm_ruins_everywhere`;
        * 0: `their_ruler_is_famous`. If X(t+1) is even, stop. Otherwise emit `new_page`.
          Then, if odd(t+2): magic(R), `he_is_a_master_of_realm_magic`. Otherwise: trait(R),
          `he_is_known_for_his_trait`, and the trait's emote if odd(t+3).

     Stop.

Helper functions:

* **5.4.1 realmKind(R)** (0x10cee0): [12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22]
  [X(R + 0x7b2650) mod 11].
* **rulerRace(R)** (0x10f320): base[X(R + 0x80ea) & 7] + (X(R + 0x4bc) & 1), where
  base = (0, 2, 4, 7, 9, 11, 13, 15) for indices 0..7.
* **female(race)** (0x5ee20): race < 17 with bit `race` set in 0x1552a (races 1, 3, 5, 8, 10,
  12, 14, 16), or race 84, or race 228.
* **rulerTitle(R)** (0x10ec60): 2 × (X(R + 0xa306b) mod 13), plus 1 if
  female(rulerRace(R)). The value is 0..25.
* **nationModel(R, n)** (0x10eb10): by X(R + 0x1337c) mod 11:
  * 1..7: base[k] + (X(n) & 1), with the base table above;
  * 8: 48;
  * 9: 51;
  * 10: 76;
  * 0: X(n) & 1.
* **magic(R)** (0x10e6f0): the tag table §6.4 [X(R + 0x392fa) mod 15].
* **trait(R):**
  * If X(R + 0xe6cb) is even, the trait is **good**: §6.5 [X(R + 0x1e2e + 0x20b3) mod 10]
    (0x10d000 is called with R + 0x1e2e).
  * Otherwise it is **bad**: §6.6 [X(R + 0x1e2e + 0x140e) mod 9] (0x10bbd0).

### 5.5 Encounter of a place (0x119a00)

Inputs: the planner, a place cell (x, y).

1. If bigCell(x, y), round x and y to even.
2. kind = placeKind(x, y, big).
3. role = roleAt(x, y) from **this** planner.
4. Decide, in this order:
   1. role 18: the encounter is 26.
   2. role 16: 1.
   3. role 17, or kind 24: 27.
   4. **role 1.** Take a list by kind:
      * 4 or 9: (12, 13, 14);
      * 22: (11);
      * 37: (15, 16);
      * otherwise: (9, 10, 11, 12, 13, 14, 15, 16).

      The encounter is list[R((x + 0x3d7) + 57·(y + 0x15948)) mod length]. No day, no X.
   5. **Otherwise:**
      1. treasure = whether the place has a role treasure (0x131540): roles 2..5, or role 14
         with an artifact.
      2. If bigStructure(x, y, kind, big) ≠ 0 (§5.8): the encounter is 28.
      3. Otherwise, take a list by kind:

         | kind | list |
         |---|---|
         | 2 | (8, 7) |
         | 4, 9 | (4, 5, 19, 12, 13, 14, 30, 31) |
         | 8 | the encounter is 29; no list |
         | 13, 17, 20, 25, 28, 29, 31, 32, 33 | (2, if the place is **not** big), then 18, 19, 20, 21, 22, 23 |
         | 22 | (11) |
         | 24 | (27) (unreachable: caught above) |
         | 36 | (17) |
         | 37 | (15, 16, 20, 3, 32) |
         | any other | (3, 6, if **not** big), then 18, 19, 20, 21, 22, 23, 0, 9, 10, 11, 12, 13, 14, 15, 16, 30, 31, 32 |

      4. h = R((x + 0x3d7) + 57·(y + 0x15948)).
      5. If there is **no** treasure, h = h + day (world+8, wrapping).
      6. The encounter is list[X(h) mod length].

The mission resident of §4.4 therefore gets a different encounter each day unless its place
holds a treasure.

### 5.6 Sibling race (0x1420f0 with 0x13f0d0)

Inputs: a race r.

1. Build the group of r from the table below. Any race not listed forms a group of one: r
   itself.
2. Pick group[rand() mod size] with the C runtime's **global** rand() (§2.5).
3. The draw happens even for a group of one.

| r | group (in the game's order) |
|---|---|
| 0, 1 | 0, 1 |
| 2, 3 | 2, 3 |
| 4, 5 | 4, 5 |
| 7, 8 | 7, 8 |
| 9, 10 | 9, 10 |
| 11, 12 | 11, 12 |
| 13, 14 | 13, 14 |
| 15, 16 | 15, 16 |
| 22, 23 | 22, 23 |
| 37, 38, 39, 40 | 40, 38, 39, 37 |
| 77, 78 | 77, 78 |
| 83, 84 | 83, 84 |
| 163, 164 | 163, 164 |
| 202, 203, 208 | 202, 203, 208 |
| 227, 228 | 227, 228 |
| 236, 237 | 236, 237 |
| 260, 261 | 260, 261 |
| −99991 | 243, 143, 204, 280, 242 |

In §4.10 case 18 the generator was just seeded with srand(w+1), so the result is
reproducible. In §4.6 (the item rumour's friend) it is **not seeded**: the result depends on
every earlier rand() call on the game's thread. The export shows both members of each pair
for the same speaker race. An implementation cannot reproduce the game's choice there; see §9.

### 5.7 The three creature lists of §4.10 case 18

| Routine | Name here | List (order matters) |
|---|---|---|
| 0x13ffc0 | beast list | 226, 85, 73, 229, 225, 227 |
| 0x142000 | golem list | 70, 69, 287, 71, 291, 290 |
| 0x142190 | undead list | 96, 17, 165, 41, 269, 97, 230, 173 |

Each picks list[rand() mod length], then applies §5.6 to the entry. That is two rand() calls in
total: one for the list, one for the sibling group.

### 5.8 Small helpers

* **Place weight (0x9bee0)**, given kind, big and d²:
  * 0 if kind = 0;
  * otherwise the radius is 500 for a big place and 250 otherwise;
  * if d² < radius², the weight is (1 − d² / radius²)² in float arithmetic (the constant 1.0
    is at 0x4ac980);
  * otherwise 0.
* **Big structure (0x2aa270)**, given x, y, kind, big. It is 0 unless big. For a big place,
  with x, y rounded to even:
  1. q = N(x + 57·y + 0x62ad0) divided by 21474 (integer). Only q's parity or low three bits
     are tested, so the sign convention the game uses does not matter.
  2. By kind:
     * 5: 41 if q mod 8 = 0, else 0;
     * 9: 40 if q is even, else 0;
     * 15: 43 if q is even, else 0;
     * 42, 44, 45, 46, 47: the kind itself;
     * any other: 0.
* **Item push (0x839c0)**: see §7.1.

---

## 6. Tables

### 6.1 Activity tags (§4.9), in index order

0 `#glassflasks`, 1 `#sugarcubes`, 2 `#bombs`, 3 `#petfood`, 4 `#flight`, 5 `#inn`,
6 `#buyweapon`, 7 `#craftweapon`, 8 `#buyarmor`, 9 `#craftarmor`, 10 `#identify`,
11 `#specialization`, 12 `#villagers`, 13 `#friends`, 14 `#sell`.

### 6.2 Help-request roles

See the role tables in §4.6.

### 6.3 Strong-creature lists

See §5.7.

### 6.4 Magic tags (§5.4), in index order

0 `#rune_magic`, 1 `#blood_magic`, 2 `#gem_magic`, 3 `#dark_magic`, 4 `#black_magic`,
5 `#unholy_magic`, 6 `#holy_magic`, 7 `#divine_magic`, 8 `#life_magic`, 9 `#death_magic`,
10 `#mana_transformation`, 11 `#matter_transformation`, 12 `#mana_infusion`,
13 `#particle_fusion`, 14 `#advanced_alchemy`.

### 6.5 Good traits

0 `#kindness`, 1 `#friendliness`, 2 `#goodness`, 3 `#gentleness`, 4 `#honesty`,
5 `#pureness`, 6 `#loyalty`, 7 `#fairness`, 8 `#generosity`, 9 `#happyness`.

### 6.6 Bad traits

0 `#brutality`, 1 `#cruelty`, 2 `#harshness`, 3 `#relentlessness`, 4 `#unmercifulness`,
5 `#ruthlessness`, 6 `#badmood`, 7 `#badness`, 8 `#viciousness`.

### 6.7 Realm kinds (§5.4.1)

12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22.

---

## 7. Tokens

### 7.1 Token kinds and argument order

| Token | Arguments (each is its own token) | Notes |
|---|---|---|
| a dictionary id | — | A `<speech id="...">` of dict_en.xml. Ids ending in `?` (`rest_inn?`, `goodbye?`, `change_specialization?`) are answer choices offered to the player. |
| `#tag` | — | A grammar tag. The dictionary's next line uses it to choose its wording: `im_in_this_zone_to_do_tasks` with `#inn` says "rest in an inn"; `he_was_known_for_his_trait` with `#kindness` names the trait. The tag is not a line itself. |
| `new_page` | — | A pause: the next line follows on the next press. |
| `!alert`, `!love`, `!sweat`, `!angry`, `!question` | — | An emote over the speaker. |
| `define_location` | x, y, kind | A 1024-block place cell and its landmark kind. |
| `define_direction` | dx, dy | Blocks from the speaker to the place's landmark point. |
| `define_item` | category, id, modifier, **material**, **rarity** | Pushed by 0x839c0 as bytes +0x00, i32 +0x04, i32 +0x08, byte **+0x1c**, byte **+0x14**. The bytes are zero-extended. The order is material **then** rarity. src/TalkScripts.h and the exporter label them "rarity material", and the exporter builds its `item=` name with them swapped. Evidence: the player's iron chest armour exports as `4 0 1100 1 0`. |
| `define_mob` | id, race | A person. Their name is the name generator's for (id, race). |
| `define_model` | creature kind | A creature to show or name; negative values are the game's special models. |
| `define_realm` | realm kind, realm | — |
| `define_land` | biome, land seed | — |
| `define_ruler` | title index 0..25 | — |
| `define_number` | n | — |
| `define_specialization` | class, specialisation | — |
| `mark_location` | x, y | Marks the place cell on the player's map. |

### 7.2 Number format

Every number is a token of its own: the value converted to a 32-bit signed integer and
written in decimal, with a leading `-` for negatives (0x759f0). There are no leading zeros and
no `+`.

### 7.3 The export file (Extract/talk-scripts.txt)

The exporter is tools/ZoneExtractor, key K or the creature tour. It calls 0x2d9480 for every
loaded creature with hostility 3, other than the player. It does so for the world's day and
the next 63 days, setting world+8 for each call. Every other input is the live game state at
export time: the player's class, gear, pet, gold, missions and the time of day.

* At the start of each batch: a line `# t=<milliseconds since boot> day=<today>`.
* Then one line per (person, day), with single spaces:
  `id=<id as signed 64-bit> day=<day> race=<race> class=<class> spec=<specialisation> flags=<+0x5c as 8 lowercase hex digits> flags2=<+0x80 as 8 hex digits> speaker=<the person's generated name> pos=<x>,<y>,<z> :`
  * The position is in blocks, with one decimal.
  * Every token then follows as a space, the token and ` |`.
* Extras written after a command's arguments, in the same ` <text> |` form:
  * after `define_location`'s three numbers: `name=<the place's proper name>`;
  * after `define_item`'s five numbers: `item=<item name>`, then `key=<dictionary key>`;
  * after `define_mob`'s two numbers: `name=<the person's name>`.

  These are not part of the game's script. Drop every token that starts with `name=`,
  `item=` or `key=` before comparing.

Example (Gedan, day 139):
`… : new_page | define_location | 1 | -5 | 15 | name=Jaron | define_direction | 1493 | -3025 | hello | new_page | a_supplier_gnome_went_missing_in_the_location | new_page | define_model | 17 | he_was_captured_by_creatures | !angry | new_page | i_hope_someone_is_going_to_help_him | mark_location | 1 | -5 |`

---

## 8. Verification

A Python model of §2 and §4-§5 was run on all 98,688 records of the export. It covered every
token whose value does not need world generation. For world-derived numbers (cells, kinds,
realms, pet ids, captor models), only their position in the sequence was checked; their
values were taken from the record. Results:

| Branch | Records | Result |
|---|---|---|
| Seed (§2.2) and mission resident "task done" (§4.4) | 64 | all exact |
| Specialisation trainer (§3.1): player warrior, spec 0 → `1 1` | 128 | all exact |
| Innkeeper (§3.2): rest window, gold < 10 | 128 | all exact |
| Artifact draw gate X(c) & 7 = 0 (§4.5) | 859 artifact scripts | all had it. 217 help scripts also passed it but had no candidate. |
| Help draw gate X(c+1) & 3 = 0 (§4.6) | 24,932 | all |
| Greeting (§4.7, §5.1), including define_mob id and race | 70,232 | all exact |
| Equipment remark (§4.8) | 10,899 | all exact. Within one export batch the item chosen is consistent with X(u+2) mod (number of filled slots). |
| Class remark (§4.8) | 16,086 | all exact |
| Village activity and class-138 welcome (§4.9) | 629 | all exact; all 15 tags occur |
| Small talk, the 18 topics other than land history (§4.10), with w = v (~40,600) and w = v+1 (~2,000) | ~42,600 | all exact |
| Small talk topic 18: rand lists, srand and sibling pick | 2,118 | all exact |
| Weather emotes (topic 3) | 2,773 | all exact |
| Pet topic, player without a pet (topic 7) | 2,801 | all exact |
| Land history (§5.4), realm kind, ruler, ruler race, number, magic, traits | 2,345 | all exact |
| Artifact rumour, normal variant, not carried (§5.3) | 644 | all exact |
| Artifact rumour, class 138, not carried | 215 | all exact |
| Help role 2-13, item rumour, not owned, not known (§4.6) | 15,097 | all exact (friend race wildcarded) |
| Help role 1, gnome, not completed, not known | 5,286 | all exact. Captor models seen: 17, 226, 46, 227, 45, 269, −99993. |
| Help role 16, monster, not completed, not known | 1,235 | all exact |
| Help role 19, witch, not completed | 1,103 | all exact |
| Help role 20, demon portal, not completed | 2,211 | all exact |

In total, every generic townsperson record in the file falls into one of these categories,
and none failed.

**Not confirmed by the export.** These were read from the decompile only:
* the alarm call (§4.1, §5.2) and the meaning of toggle bits 0x0002 and 0x0100;
* races 257 and 258;
* the arena master;
* a mission resident's active-task branch, with the §4.4 encounter table;
* role 18;
* "player already has the special item";
* every "known" or "completed" variant of a help request;
* a carried artifact;
* a player with a pet;
* the innkeeper's other two outcomes.

The equipment slot **order** was not checked, only that the index rule fits.

---

## 9. Open points and cautions

1. **Unseeded rand in §4.6.** The friend's race in an item rumour comes from the game's global
   C rand without a fresh seed. It is the speaker's own race whenever the race has no sibling,
   which is most races. For paired races (2/3, 4/5, ...) the game's choice cannot be
   reproduced. Recommendation: keep the rand state that §4.10 case 18 leaves behind as a global
   generator, as the game does, and accept a mismatch with the export for paired races.
2. **Empty help-target list.** The game would divide by zero. Never seen. A safe choice is to
   skip §4.6 then.
3. **Planner region.** The planner used everywhere (roles, encounters, special town, biome,
   land seed) is that of the speaker's **origin** (§3.3). It is not the region of the place
   being talked about, nor the speaker's current position.
4. **Encounters depend on the day** (§5.5), and so do the scripts. That is deliberate.
5. **define_item order.** Material comes before rarity (§7.1). CubeVulkan's TalkScripts.h
   comment and the exporter's `item=` names assume the reverse.
6. **0x5f4c0 "known".** It also counts solved places. The known set (+0xb8) is presumably the
   set of places marked by `mark_location`, but that was not traced.
