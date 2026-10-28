# OverworldEvents rules

These files are the gameplay rules of DieuDeGlace's
[OverworldEvents](https://github.com/DieuDeGlace/OverworldEvents) Steam mod (MIT,
`assets/OverworldEvents/LICENSE`), taken unchanged from commit `77ad315`:
encounter planning (`EventRules.h`), instance spacing (`InstanceRules.h`), village
quests (`TownRules.h`), travel locations turned dungeons (`TravelRules.h`), difficulty and guardian health (`DifficultyRules.h`,
`EventHealthRules.h`), event objects (`EventObjectRules.h`), villager dialogue
(`DialogueRules.h`) and the persistent state (`StateStore.*`).

CubeVulkan runs them on its own reimplementation of the game (`src/OverworldEvents.*`).
Creatures are made the way the game makes them, the dialogue goes through the speech
box, the rewards come from the game's item generators, and the markers appear on the
game's map. Changes to these rules belong upstream, as pull requests on the mod's
repository, and are copied back here unchanged.
