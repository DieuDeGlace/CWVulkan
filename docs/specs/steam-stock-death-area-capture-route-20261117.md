# Stock Steam routes to the first death area effect

Two ordinary player actions reach the host area pass: **Use Quick Item** with a **Bomb** creates
model `0x90`, and a Water Mage's **Water Bubbles** ultimate creates model `0x11D`. A Bomb has
the shorter wait once purchased, about four seconds. Water Bubbles needs no consumable and its
first natural pop is about eighteen seconds after the first bubble appears. No stock encounter
was established for model `0x119`. These are static findings and a capture recipe, not a report
of a run.

## Shortest route: Bomb (`0x90`)

1. In stock single player, buy a **Bomb** from an **Item Vendor** at an **Item Shop**, or use one
   already in the bag. The vendor interaction at `0x098180` opens the inventory for an Item Vendor
   (profession `0x80`, labeled at `0x0B0680`). That inventory uses `0x0B75D0` and `0x2ACDD0`,
   which adds the Bomb item at `0x2ACF4C` independently of its random pet food choice. The catalog
   entry is item category 1, subtype 9; `0x157519` through `0x157546` binds that pair to **Bomb**.
   The original English `random:26` guidance also identifies the Item Shop and breakable rocks.
2. Close the shop and select **Bomb** as the displayed quick item. Stand on dry, fairly level
   ground. For the terrain-specific random marker, place it within a few cubes of a breakable
   rock at about the same height. Start observing, then press **Use Quick Item** once (default
   **Q**). The selection and use path is `0x08ECFD` through `0x08EDAF` and `0x0B7740`. It
   consumes one Bomb. This branch requires the item and an unincapacitated character; it has
   no class, equipped weapon or separate Bomb cooldown check.
3. The host handles the item action in `0x2C3710`; the model write at `0x2C44BE` creates a live
   type-7 `0x90` actor at the user's current position. It does not place it at the aim point.
   Keep an eligible living actor nearby on the same level. In a quiet area the player's own
   living avatar can serve; stand several cubes away and avoid being incapacitated. The host
   reduces the Bomb's health at one quarter of its initial maximum per simulation second
   (`0x2DF4E1`, constant `0x472178`). Around four seconds after placement it reaches zero; the
   underflow path caps health at zero and resets the death clock. Include the following host
   update in the capture.

## Portable ability: Water Bubbles (`0x11D`)

1. Use a **Mage** with **Water Magic** specialization (displayed as **Water Mage**). If the Mage
   is Fire, interact with the **Guild Receptionist** (default **E**) and choose the dialogue
   option to change specialization to Water Magic. The label and specialization option are
   evidenced at `0x0B0680` and `0x2D9480`; the chosen dialogue changes specialization at
   `0x089CB0`. `0x26EBA0` identifies Mage class 3, specialization 1 as Water Mage.
2. In open, level ground, start observing and press **Ultimate Skill** once (default **R**)
   while the ability is ready. This is the **Ultimate Skill** binding, distinct from **Class
   Skill** and the weapon's special attack. The stock input reads it at `0x08C4F0`; `0x0656E0`
   selects action `0x90` for Water Mage, and `0x15AE39` ties it to the English **Water Bubbles**
   name. The normal availability check at `0x0507B0` requires the character not be incapacitated
   and the timer be ready. `0x050BD0` gives this action a 40-second cooldown. There is no
   specific weapon, positive mana cost, aimed target, consumable or water requirement on the
   traced path. The stock key names and defaults for both routes come from `0x042670`,
   `0x043760` and `0x049080`; use the configured binding if remapped.
3. The action branch at `0x2CAC73` queues a bubble at `0x2CAD0F` as an update crosses an
   eighth-interval boundary during the cast. The nominal cast window is 1,500 ms divided by
   the caster's attack speed factor (`0x04EAD0`, `0x066460`), so bubbles appear in succession.
   `0x2C3710` creates a type-7 `0x11D` actor at the caster's current position for each such
   event, records the caster as owner, and starts its health and lifetime value at 1.0
   (`0x2C4B4D`, insertion at `0x2C4CDF`). Remain within a few cubes of a bubble on the same
   level until it pops. There is no need for a hostile encounter for the area roll: the live
   local caster is an eligible nearby candidate if the target gates below pass.
4. Each bubble's lifetime value loses about 0.05 per simulation second. When it falls below
   0.10, the host sets health and the death clock to zero and queues the pop sound (`0x2DF51E`,
   `0x2DF558`; rate at `0x472168`). That is about 18 seconds from each bubble's spawn. Capture
   from before **R** through about 20 seconds at ordinary simulation speed and allow for the
   cast's stagger and the following host update.

## Positive capture markers

The host update calls `0x2A45E0` at `0x2DA8F3`, before the actor updates that expire a Bomb or
bubble later in that update. The next area pass sees health at or below zero, a nonzero actor
type, and an exactly zero death clock for the new death. It accepts the three models above and
queues the centered effect at `0x2A4836` (kind `0x14`, variant 1 for `0x90`/`0x119`, variant 2
for `0x11D`). This call proves first-visit entry even on a run with no eligible random work.

- **`0x2A4CCC`** (return `0x2A4CD2`): one terrain-related draw for a `0x90` Bomb if the block
  scan found at least one eligible block. The scan looks inside eight cubes of the center
  and requires a block flag in mask `0x11`. No qualifying block means no draw at this site,
  even if the Bomb entered the effect pass.
- **`0x2A4FDB`** (return `0x2A4FE1`): the draw for each qualifying nearby living actor, shared
  by all three models. A candidate must be distinct from the dying actor, pass the relation
  and protection filter at `0x050550`, have zero in the target timer at `+0x12C`, and pass
  the range checks. A few cubes on level ground is comfortably inside the eight-cube base
  range. For a bubble this draw precedes the owner relationship test that distinguishes
  damage from healing, so it can occur even if the caster is already at full health.

Keep the source actor's model with the event. The draws at `0x2C4B96`, `0x2C4BCE` and `0x2C4C6D`
are Water Bubbles creation work, not the pop's area draw. The pop sound by itself is also not
evidence of a draw at `0x2A4FDB`. The stock inputs above do not require changing game state
outside normal play.

## Static-only Barrel case (`0x119`)

The names table at `0x154708` labels `0x119` **Barrel**, as it does `0x118`. The appearance
selection at `0x053E80` uses art slot `0xE18` for `0x119`, registered as `barrel-tnt.cub` at
`0x1E1A34`. That establishes an appearance and an eligible area branch, but no particular
barrel, location or attack was tied to a stock `0x119` actor that can die. Native immediate
references showed the table, asset setup and area test. Generic world actor registration at
`0x2E17D0` can copy model IDs from world records, so this search does not rule out a
data-driven encounter. A visually similar prop is not a positive test. A forced model,
synthetic death or injected event would be an experimental test, not evidence of a stock
capture route. No debug or injection path was used or investigated.

## Identity and separation

Installed `cubeworld.exe` and archived `cubeworld.original.exe` match: 5,939,072 bytes,
SHA-256 `a1efbbd41eeb8df08395c4d6511194d416cb70a14054b2a2000b9f5cdbcfa65b`.
The referenced PE is x64, timestamp `0x5D93AB0D`, preferred image base `0x140000000`.
The native export identifies `cubeworld.original.exe.unpacked.exe` (5,757,952 bytes; verified
SHA-256 `7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244`). The RVAs are for
that loaded image layout. Code bytes were checked in the unpacked matching image; the
installed image's on-disk code bytes at these points differ. Checked literal names and timer
data matched across the two images. Player-facing localization is the installed `dict_en.xml`
(SHA-256 `2063613470f4fa661b614177fa15d2dda11fd80cf29c7f17a518fea6485381bf`).

Evidence was limited to the author policy, the assigned original Steam exports and original
binary/localization identity checks. No replacement engine, SDK, reconstruction, repository
history, implementation files or tests, owner captures, logs or packages were read. The game
was not launched and no in-game state was changed.
