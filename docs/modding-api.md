# CubeVulkan modding API, version 2 (design)

Status: approved by the owner on 2026-11-22. Implemented so far: the world step and its services
(`players`, `zone_loaded`, `block_type`, `ground`, `biome`, `spawn_creature`, `add_flags`) and the
`changes_world`/`version` declaration, proven by `examples/AquaticMod`. The remaining services
(creature reads and steering, deaths, talk pages, rewards, HUD lines, map pins, mod save data) and
the multiplayer mod-list check are added as each add-on that needs them moves to a mod.

## Where it starts

CubeVulkan already loads code mods: every DLL in `mods/` beside the executable that exports
`cwv_mod_load` receives the `CwvApi` table from `include/cubevulkan/ModApi.h` (version 1).
Version 1 covers asset overrides (`mods/<Name>/sprites|music|sounds`), decorative models
(`load_model`, `spawn`, `move`, `remove`), terrain height and water depth, the clock, the camera
focus, per-mod configuration, logging, and per-frame and key callbacks. `examples/HelloMod` shows it.

What version 1 cannot do is gameplay. The owner's add-ons (travel parties, aquatic encounters,
villager requests, overworld events) are therefore compiled into the engine and called from
`src/main.cpp`. Version 2 adds the hooks those add-ons use, so they, and new mods, can live in
DLLs instead.

## Rules that do not change

- **C ABI, append-only.** New fields go at the end of `CwvApi` and `CwvMod`; `CWV_API_VERSION`
  becomes 2. A version 1 mod keeps working unchanged. A mod checks `api->version` before using a
  version 2 field; the engine checks the size the mod reports before calling a version 2 callback.
- **Off unless installed.** No DLLs ship in `mods/`. With no mods, the game is exactly the verified
  Steam behaviour. A mod that fails to load, or returns 0, is logged and skipped.
- **CubeVulkan's own concepts only.** The API exposes the engine's creatures, blocks, items and
  clock as plain typed values. It does not expose raw record offsets, original-game internals or
  anything from the old Cube World SDK or CubeModLoader, which it is unrelated to. This keeps the
  clean-room boundary: mods see the replacement engine, not the original game.
- **Main thread, no exceptions across the boundary.** Callbacks run on the engine's thread; a
  mod must not throw out of a callback.

## Additions in version 2

### World logic on the host (`on_world_step`)

`CwvMod.on_world_step(const CwvWorld *world, double milliseconds)` is called once per host world
step, inside the host lock, on the world that decides (the host's world in multiplayer, the local
host in single player). The creature and block services below are valid only inside this
callback, because that is when the engine guarantees exclusive access.

Services on `CwvWorld`:

| Service | Purpose |
|---|---|
| `players(out, max)` | positions of the players in the world |
| `spawn_creature(race, hostility, level, pos)` | make a creature; returns its id, 0 if its zone is not loaded |
| `creature(id, out)` | read a creature as `CwvCreature` (id, race, hostility, level, health, maximum health, position, home, flags) |
| `for_each_creature(fn, user)` | visit loaded creatures |
| `set_home(id, pos)`, `set_flags(id, flags)`, `remove_creature(id)` | steer, mark (boss, fish ...) or remove a creature the mod made |
| `group(ids, count)` | keep creatures together as a party |
| `block(x, y, z, out)` | read a block's type and flags; reports whether its zone is loaded |
| `ground(x, y)` | ground height |
| `places(x, y, radius, fn, user)` | villages and other places near a point (kind, centre, radius) |

### Events

| Callback | When |
|---|---|
| `on_creature_death(id, killer, race, flags)` | a creature dies; `killer` is a player id or 0 |
| `on_talk(npc, CwvTalk *talk)` | the player talks to a creature; the mod may add pages with `talk->page(text, r, g, b)` before the game's own script |
| `on_save(CwvSave *)`, `on_load(const CwvSave *)` | the character's world is saved or loaded; key/value text the mod owns, stored in `Saves/mods/<Name>.state`, never in the game's own save |

### Player and interface

| Service | Purpose |
|---|---|
| `give_gold(amount)`, `give_xp(amount)`, `give_item(kind, rarity, level)` | rewards, through the engine's normal reward paths |
| `hud_line(text)` | a line in the quest tracker for this frame |
| `map_pin(x, y, icon, label)` | a marker on the map and minimap for this frame; `icon` names a sprite in the mod's folder |
| `chat(text)` | a chat line for the local player |

## Lua scripts

A folder `mods/<Name>/` holding `main.lua` is a script mod, loaded after the DLLs in alphabetical
order. It sees the same services as a code mod:

- `cwv.log(...)` (also `print`), `cwv.config(section, key, fallback)` (reads `mods/<Name>.ini`
  first, then `CubeVulkan.ini`), `cwv.get_hour()`, `cwv.set_hour(h)`, `cwv.focus()` (x, y, z),
  `cwv.version` (2).
- Optional callbacks: `on_update(seconds)`, `on_key(key)` (return true to consume it),
  `on_world_step(world, milliseconds)`, `on_shutdown()`.
- During `on_world_step`, `world.seed` and `world.players()` (a list of `{x, y, z}`),
  `world.zone_loaded(zx, zy)`, `world.block_type(x, y, z)` (type, loaded), `world.ground(x, y)`,
  `world.biome(x, y)`, `world.spawn_creature(race, hostility, level, x, y, z)` (id, 0 on failure)
  and `world.add_flags(id, flags)`.
- An optional global `mod = { name = ..., version = ..., changes_world = true }`.

Scripts are sandboxed. Only the base, table, string, math, utf8 and coroutine libraries are
present; `io`, `os`, `package`, `debug`, `require`, `load`, `loadfile` and `dofile` are not, and
precompiled bytecode is refused. Each call has an instruction budget (200 million). A script that
raises an error or exceeds its budget is disabled and the reason is logged; the game carries on.
A world-changing script's multiplayer identity is its name, version and a hash of its text.
Lua 5.4.6 is built from the official source tarball, pinned by SHA-256 (MIT licence).

`examples/AquaticLua/main.lua` is the aquatic add-on as a script. EngineTests' `aquatic` group runs
it through the host in the same world as the built-in rules and the `AquaticMod` DLL and requires
the same creatures; it also checks the sandbox and the instruction budget.

## Multiplayer

World logic runs only on the host (`on_world_step` is never called on a guest's mirror world).
Guests see the results through normal replication: creatures, hits, items and chat. A mod that
changes the world sets `CwvMod.changes_world = 1` and a version string. The host sends the list
of such mods when a guest joins, and the guest refuses a host whose list differs, the same way
mismatched terrain settings are refused today. Purely visual mods (version 1 style) are not
compared.

## Proving it: the aquatic add-on as a mod

The aquatic encounter rules live in `src/AquaticPlan.cpp`, free of engine types. The built-in
add-on (`src/AquaticEncounters.cpp`) and the example mod (`examples/AquaticMod`, built into
`build/examples/AquaticMod.dll`) both run them. EngineTests' `aquatic` group loads the real DLL
through `ModHost`, drives it with the version 2 world services in a synthetic sea-and-pond world,
and requires the same creatures (race, hostility, level, place, flags) as the built-in rules.

At the owner's request the built-in aquatic life stays and is **on by default**
(`[Creatures] AquaticEnemies=1`): the only add-on that is. `AquaticMod` remains an example of the
API; installing it while the built-in add-on is on would make schools twice. Travel parties,
villager requests and overworld events stay built in and off by default; any of them can move to a
mod the same way, one checkpoint each, with the owner's approval.

## Out of scope for version 2

Rendering hooks (shaders, post effects), new item types, new races or UI panels, other scripting
languages, and changing the original game's behaviour when no mod is installed.
