# Race numbers and their dictionary keys

How Steam Cube World turns a creature's race number (creature +0x64, see
`creature-physics.md`) into the key of its `<name id="...">` entry in `dict_en.xml` (for example
race 0 → `ElfMale`). Written from the Steam decompile and binary; replaces the provenance of
`src/RaceKeys.inc` (CLEANROOM.md, "To redo clean-room"), which was not consulted.

## Where the game keeps it

* **The table is built in code**, by the constructor 0x1401510f0, called from 0x14007ea80 for
  the object at +0x50 of that larger object (the frame at 0x14007eb0e); the constructed
  object's address is also stored in the global at 0x140551e80.
* The race table is the ordered map (an MSVC `std::map<int, std::wstring>`, red–black tree;
  key at node +0x20, value at node +0x28) at **+0x8 of that object**. The constructor fills it
  with 195 insertions (0x1401512e4..0x140153c8d): for each, the race number is stored to a
  stack slot, the map's find-or-insert routine 0x140048fb0 is called with it, and the node's
  wide string is assigned (0x14003d4f0) from a UTF-16 literal with an explicit length.
* The literals are UTF-16 strings in `.rdata`, nearly all in one pool from 0x140448d98
  (`ElfMale`) to 0x140449d88 (`NomadMale`); `Wolf` reuses an earlier literal at 0x140431f58.
  The pool's order is the insertion order, **not** the race order, so the strings alone do not
  give the numbers.
* Three races are inserted twice with the same value (36 `Porcupine`, 62 `Mosquito`), which
  changes nothing; 304 and 305 both map to `Gnome`. 193 distinct race numbers are present.
* The code that reads the table (finding a race in the map and using the string as a
  dictionary key, e.g. for creature and pet names) inlines the tree search; it was not
  traced to a single routine here.

## Can it be read from the owner's install at run time?

No, not in a form worth using; **put the mapping in the code as facts.**

* `dict_en.xml` has the keys (`<name id="ElfMale" tags="m an">`) but no race numbers, and
  some keys in the table (e.g. `Bullterrier`) have no `<name>` entry at all.
* The shipped `cubeworld.exe` (SHA-256 begins a1efbbd41eeb8df0, identical to
  `binary/cubeworld.original.exe`) is wrapped by the Steam DRM stub (`.bind` section): its
  `.rdata` strings are readable at the same file offsets as in the unpacked image, but its code
  section is encrypted on disk (the bytes at 0x1401512f3 differ from the unpacked image's). The
  race numbers are immediates inside that encrypted code, so reading them would mean undoing
  the DRM, which CubeVulkan must not do.
* What could be read at run time is only the strings by address, which still needs the numbers
  as facts. That adds nothing.

So CubeVulkan should carry the table below as data (race number → key). The game's own
dictionary is then looked up by that key at run time from the owner's `dict_en.xml`, as now.

## The facts

Race number → dictionary key, as inserted by 0x1401510f0, in race order. Numbers not listed
(84, 89, 95, 129, 151, 156–164, 181–190, 194–199, 202–208, 212–214, 218–219, 221–223,
227–228, 231–259, 262–291, 297–303, and above 305) have no entry in the table (what the
untraced reader does for them is an open question).

0 ElfMale, 1 ElfFemale, 2 HumanMale, 3 HumanFemale, 4 GoblinMale, 5 GoblinFemale,
6 Bullterrier, 7 LizardmanMale, 8 LizardmanFemale, 9 DwarfMale, 10 DwarfFemale, 11 OrcMale,
12 OrcFemale, 13 FrogmanMale, 14 FrogmanFemale, 15 UndeadMale, 16 UndeadFemale, 17 Skeleton,
18 OldMan, 19 Collie, 20 ShepherdDog, 21 SkullBull, 22 Alpaca, 23 BrownAlpaca, 24 Egg,
25 Turtle, 26 Terrier, 27 ScottishTerrier, 28 Wolf, 29 Panther, 30 Cat, 31 BrownCat,
32 WhiteCat, 33 Pig, 34 Sheep, 35 Bunny, 36 Porcupine, 37 GreenSlime, 38 PinkSlime,
39 YellowSlime, 40 BlueSlime, 41 Frightener, 42 SandHorror, 43 Wizard, 44 Bandit, 45 Witch,
46 Ogre, 47 Rockling, 48 Gnoll, 49 PolarGnoll, 50 Monkey, 51 Gnobold, 52 Insectoid, 53 Hornet,
54 InsectGuard, 55 Crow, 56 Chicken, 57 Seagull, 58 Parrot, 59 Bat, 60 Fly, 61 Midge,
62 Mosquito, 63 PlainRunner, 64 LeafRunner, 65 SnowRunner, 66 DesertRunner, 67 Peacock,
68 Frog, 69 PlantCreature, 70 RadishCreature, 71 Onionling, 72 DesertOnionling, 73 Devourer,
74 Duckbill, 75 Crocodile, 76 SpikeCreature, 77 Anubis, 78 Horus, 79 Jester, 80 Spectrino,
81 Djinn, 82 Minotaur, 83 NomadMale, 85 Imp, 86 Spitter, 87 Mole, 88 Biter, 90 Squirrel,
91 Raccoon, 92 Owl, 93 Penguin, 94 Werewolf, 96 Zombie, 97 Vampire, 98 Horse, 99 Camel,
100 Cow, 101 Dragon, 102 BarkBeetle, 103 FireBeetle, 104 SnoutBeetle, 105 LemonBeetle,
106 Crab, 107 SeaCrab, 108 Troll, 109 DarkTroll, 110 HellDemon, 111 Golem, 112 EmberGolem,
113 SnowGolem, 114 Yeti, 115 Cyclops, 116 Mammoth, 117 Lich, 118 RuneGiant, 119 Saurian,
120 Bush, 121 SnowBush, 122 SnowBerryBush, 123 CottonPlant, 124 Scrub, 125 CobwebScrub,
126 FireScrub, 127 Ginseng, 128 Cactus, 130 ThornTree, 131 GoldDeposit, 132 IronDeposit,
133 SilverDeposit, 134 SandstoneDeposit, 135 EmeraldDeposit, 136 SapphireDeposit,
137 RubyDeposit, 138 DiamondDeposit, 139 IceCrystalDeposit, 140 Scarecrow, 141 Aim,
142 Dummy, 143 Vase, 144 Bomb, 145 SapphireFish, 146 LemonFish, 147 Seahorse, 148 Mermaid,
149 Merman, 150 Shark, 152 LanternFish, 153 MawFish, 154 Piranha, 155 Blowfish,
165 SkeletonKnight, 166 SkeletonHorse, 167 Sorcerer, 168 Hyena, 169 Giraffe, 170 Warthog,
171 Antelope, 172 Makake, 173 SkeletonDog, 174 LavaTurtle, 175 DemonPortal, 176 SlimeWell,
177 HornetsNest, 178 UnholyTomb, 179 BonePile, 180 Molehill, 191 Mantis,
192 RadishCreatureSprout, 193 PlantCreatureSprout, 200 BabyElephant, 201 BabyMammoth,
209 GoldKnightHorse, 210 Flamingo, 211 Toucan, 215 BlueTurtle, 216 SnowGnobold,
217 SavageDog, 220 SnowLeopard, 224 AlphaDog, 225 FlyingEye, 226 DemonMarauder, 229 Crawler,
230 Necromancer, 260 OrcMale, 261 OrcFemale, 292 EarthPile, 293 Caterpillar,
294 EarthCaterpillar, 295 Snail, 296 DarkJester, 304 Gnome, 305 Gnome.

(Spelling as in the game, including `Makake` and `Spectrino`. 260/261 repeat the Orc keys of
11/12.)

## Checking

* Static: the list above was produced by walking the 195 insert/assign pairs of 0x1401510f0 in
  the unpacked image and reading each literal with its length; an implementer can regenerate
  it the same way to audit the data file, without reading any other code.
* Oracle (for the user): the in-game `tools/ZoneExtractor` can dump the live map — the object
  pointer at 0x140551e80, map at +0x8, walk the tree — to a text file, and compare it with
  CubeVulkan's table. Every race in the world-generator oracles (`.rtrace`, looks.bin) should
  then name correctly through `dict_en.xml`.

## Errata

- Race 143's key (Vase) is also missing from the install's dict_en.xml, alongside Bullterrier.
