# Steam live static-object interaction and reward metadata

Independent native-only bounded audit: object constructor **0x2f93a0**, semantic
copy **0x9860**, live resolver **0x2c3150**, prompt **0x98180**, interaction
consumer **0x2c3710**, map lookup **0x936e0**, and directly relevant object/world
readers. Native sources and existing independent prose only; no engine, tests,
extractor, owner assets/captures, SDK, reconstruction or game execution.

## 1. Live object fields and construction defaults

The vector **Zone+0x60 begin/+0x68 end** contains0xe0-byte static-object
records in physical index order. This is not the same-layout ground-item
vector+0x30. The following are the verified semantic fields:

| Offset | Type | Meaning/default from0x2f93a0 |
| --- | --- | --- |
| +0 | signed32 | Object type; constructor does not initialize it |
| +8/+0x10/+0x18 | signed64×3 | Fixed XYZ,65536 units/block; constructor does not initialize them |
| +0x20 | **unsigned8** | Turn; constructor does not initialize it |
| +0x24 | float32 | Dimensionless scale; default1 |
| +0x28 | unsigned8 | Type-specific state; default1 |
| +0x2c | float32 | Swing/animation milliseconds; default0 |
| +0x30 | signed64 | User/link creature ID; default0 |
| +0x38..+0xd7 |0xa0-byte item | Required/accepted item, semantic empty-item default |
| +0xd8 | **unsigned8** | Required item count; default1 |

The embedded item's initialized semantic default has category/subtype/modifier,
region, rarity, formula, material, attribute payload and attribute count zero.
This is not a guarantee that every padding byte of the0xe0 record is zero.
**0x9860** copies type, position, turn byte, scale, state byte, clock, user ID,
the item through0x35400 and the **one count byte**. It is not a raw0xe0-byte
memcmp/copy contract. The constructor's defaults apply to construction, not
an instruction to overwrite supplied live values such as count0 or user IDs.

Turn is not merely a Boolean flag or a guaranteed0..3 value. Footprint readers
use its low bit to swap X/Y, while drawing consumes the **full unsigned byte
times90 degrees**. Uninitialized-but-consumed lamp turns remain the separate
bounded finding in [world-generation parity §1](worldgen-parity-gaps.md#1-priority-finding-street-lamp-turns-are-undefined-but-consumed).
State is likewise not a universal alive/dead field: its door, requirement,
personal-chest and occupant meanings are type-specific.

## 2. Current-record resolution and item hand-in

**0x2c3150** resolves the requested zone key through the World's loaded-zone
map+0x460. Missing entry/null zone returns null. It checks the signed32 object
index is nonnegative and below **(end−begin)/0xe0**, then returns the pointer
into the **current vector**. It does not generate a zone, recover an object
by position or restore an original-generation record. The index includes all
physical entries; neither type nor state is a compaction criterion here.

The ordinary prompt0x98180 uses that current record. Its requirement branch
comes before the type-specific branch and applies when **item category+0x38
is nonzero and state+0x28 is nonzero**. It uses that actual full item for the
name/icon, inventory equality/sum and interaction payload; count is **u8+0xd8**.
The zero-extension is verified at **0x99a56/0x99ba2**. Values128..255 are
positive counts, not signed negatives or a signed32 field. Count0 is not
replaced by1; its established sum/removal edge behavior remains
[use prompt §3.7.1](use-prompt.md#371-objects-that-take-an-item-checked-first).

With enough items, the client removes this count unless object type is9,
plays its existing sound and queues kind5 with the canonical zone/index and
a copy of the required item. That does not copy a reward or encode a reward
count. The removal semantics and lack of an extra sort/refresh remain the
inventory-slot contract linked there.

The reached authority kind5 handler **0x2c3710** resolves the zone and index
again against its **current** object vector. Missing zone/out-of-range index
does nothing to an object. For the hand-in route it checks current required
item nonempty, current state nonzero, and full-item equality against the queued
item through0x4df60. It does not repeat the client inventory/count removal or
replace the current object metadata with the queued item's original source.
Opening through0x2d3a10 changes state/clock; it does not clear the required item
or reset its count. Exact seat/door/hand-in precedence remains
[zone interactions §2](zone-object-interaction.md#2-kind-5-using-a-zone-object).

User+0x30 is not a ground-item reservation ID. The seat/bed kind5 route reads
the current ID, requires0 before taking the seat and writes the acting creature's
ID. The ordinary object prelude0x2e3510 also resolves nonzero user IDs through
the live creature lookup and uses current object type/state for placement or
release, including the quest-object types beginning0x54. This supplement does
not expand that entire linked-placement lifecycle or declare all users to be
seated players.

## 3. Chest rewards are a separate current map

**Zone+0x450** is a separate ordered map: **signed32 object index → one full
0xa0-byte reward item**. Node key is+0x1c and item value+0x20. It is neither
the required item at object+0x38 nor the required count at+0xd8, and has no
reward-quantity field in that item value. Generator0x31c4e0 counts the physical
object vector; the described chest producers associate the reward with the
appended chest's **count−1 index**. See
[structure records §4.1](worldgen-structures.md#41-static-objects-zone-0x60-0xe0-bytes).

Client **0x936e0** is a signed-key **find**, returning the map sentinel when
absent. It does not insert or default-construct a missing entry. In the
type0x16 chest branch of0x98180, after current state and personal-used-cell
checks, the prompt finds the current loaded zone and current index's map
entry. A **known absent entry or reward category0 produces no Open prompt
and no grant**. That path does not generate a substitute item or use the
embedded requirement as the reward. It does not insert a used-cell mark or
repair the reward map on that failure.
This is the later type-specific branch; an active nonempty requirement can
instead take §2 before reaching it.

On a valid local chest press, it marks the personal16-block cell, copies the
map item's current full value and adapts category0x1e for the current player.
It grants **one** resulting item, or its existing money conversion for the
applicable money-item category. It does not use object+0xd8 as reward quantity.
These reads do not erase the map entry or overwrite it with the adapted item.
The later object prelude can derive type0/0x16 state from the personal used-cell
set, rather than the prompt directly replacing the object with a fresh record.
Full receipt behavior remains [use prompt §3.7.3](use-prompt.md#373-chests-type-0x16).

The authority hand-in route has a different result for map absence. After
matching the current required item and opening the current object, it looks
up that same current index in the current reward map. Present nonempty value:
drop **one** of that map item with the already specified random placement.
Absent/category0: **no mapped-content drop**, no insertion and no generation
repair. Separately, for object types **0x15/0x16**, this successful hand-in
calls0x2a7ca0 with the required item's rarity at object+0x4c. That independent,
explicitly gated tier reward still runs when the map entry is absent. It is
not a fallback used by the local no-requirement chest path. The reached
contents reader does not erase the reward-map entry; see
[zone interactions §2.6](zone-object-interaction.md#26-objects-that-take-an-item-locked-doors-chests-braziers).

## 4. Mission dependencies and raw-record-only boundary

A required item can be mission metadata rather than a reward. For example,
quest-camp0x11f280 constructs its category0x16 key requirement in the object's
embedded item for the applicable layouts; the guard carries the corresponding
key. That item is consumed/checked as the current requirement, not decoded as
the chest-content map. Its generation is
[creature roles §3.1](worldgen-creatures-roles.md#31-quest-camp-0x11f280).

Other mission state is outside the0xe0 record: the documented zone-cleared
reader0x2de4c0 uses linked zones, tracked IDs and World dead IDs. In the
World+0x358==0 branch,0x2e3510 visits zone+0x498 mission IDs, resolves each
against the World's creature lookup and sets **Creature+0x128 bit0x0100**
for resolved entries whose kind is not8. This check adds no HP>0 test.
This does not derive the list from the object's required item or reward map.
The resident prerequisite consumer uses the
separate selected-object-index collection+0x4a8 to resolve current vector
records and inspect state+0x28. See creature-upkeep§1d and
[resident prerequisites](steam-resident-effects-prerequisites.md). Raw object
records do not establish the contents of those other containers or the
player's personal used/finished sets.

In particular, **no supplied reward-map data is not proof that the native map
entry was absent**. A capture/format containing only the object vector can
establish its current fields, but cannot recover the reward item's category,
attributes, region, rarity, modifier or presence from those fields alone.
The known-absence outcomes in§3 apply when the map's actual state is known.
This bounded trace establishes no generation fallback for missing metadata,
no map/vector replacement synchronization policy, and no exhaustive audit of
all object or mission writers. It must not be read as permission to reset live
fields to constructor defaults or to synthesize rewards from object type.
