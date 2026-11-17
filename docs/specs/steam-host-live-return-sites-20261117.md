# Steam authority simulation CRT return sites

Status **FINAL / STABLE / HELD**. Original-only static handoff dated 2026-11-17.

## Image and event format

The matching x64 export uses image base 0x140000000 and the unpacked Steam image SHA-256 7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244. The supplied packed original SHA-256 is a1efbbd41eeb8df08395c4d6511194d416cb70a14054b2a2000b9f5cdbcfa65b. Both PE import directories identify rand at IAT RVA 0x42CD28 and srand at 0x42CD38 in api-ms-win-crt-utility-l1-1-0.dll. The instruction checks use the unpacked image.

An event identifies its thread, the import, the raw rand result or unsigned 32-bit srand input, and the RVA where execution resumes in the image after the call. All CRT calls listed here use the six-byte indirect call through the named import slot. The direct internal calls in the stack table are five bytes. The stack table gives return RVAs from progressively older frames.

## Distinguishing the callers

The ordinary authority worker enters 0x083E60 through 0x088EE0. The thread launch at 0x075DD0 dispatches to that wrapper through the entry at 0x087BB0. The worker updates the host world through 0x0F5520 and then the shared world step at 0x2DA6C0. The two calls into the step cover 16 ms subdivisions and the remaining interval. The ordinary frame thread enters the same step through 0x0A5A50 using its own world object. The host world is embedded in the host object at offset 0x08, while the frame path loads its world pointer from offset 0x08 of the game object.

| Context | Return RVAs moving outward |
| --- | --- |
| Authority world step | 0x0F691C or 0x0F6932, then 0x083ECB, then 0x088F57 |
| Frame world step | 0x0A732D or 0x0A7351, then 0x13455D |
| Resident pass and daily refresh | At a daily event, 0x2E1D81, then 0x2DA8E4, then one of the world-step chains above. A direct altitude event starts with 0x2DA8E4. |
| Area death | 0x2DA8F8, then a world-step chain |
| Pet setup from the player-pet pass | 0x2E106C for an existing pet rebuild or 0x2E12C2 for a new pet, then 0x2DA900, then a world-step chain |
| Pet setup from taming | 0x2CEA57, then 0x2DB6B0, then a world-step chain. The binary verifies this call inside the state dispatch at 0x2C5250, whose static export omits that branch. |
| Creature physics | 0x2DB6CE, then a world-step chain |
| Host area generation | Shared generator work can show 0x2AE17E, then 0x0F2CC3, then 0x0F3297. At the generator's own seed, the chain starts with 0x0F2CC3. |
| Frame-side area generation | The generation call into 0x2AE0C0 returns to 0x07D61D, 0x085999, or 0x085C2C. The single-job route from the frame-side generation loop also shows 0x0855DC and 0x0891D7 farther out. The parallel jobs may have a scheduler stack. |

The only direct incoming call found for 0x31C540 belongs to the resident pass at 0x2E17D0. The world step gates this pass on the world byte at offset 0x358 being zero. **The frame thread can execute daily refresh during ordinary local play.** Its world starts with that byte clear at 0x29A276. Startup loads the frame's selected world name and seed through 0x07F279, and the frame advances the loaded world on a name-and-seed match at 0x0A72CE. The frame-side generation loop supplies loaded areas around its player. The connection path writes one to the frame world's byte at 0x03DB2D, which suppresses refresh on that world while the byte remains set.

## CRT events in the live step

| Import | Call RVA | Return RVA | Event |
| --- | --- | --- | --- |
| rand | 0x2E49DE | 0x2E49E4 | Timed world-object effect in 0x2E3510, before the resident pass |
| rand | 0x2E5D32 | 0x2E5D38 | World-object interaction effect in 0x2E3510 |
| srand | 0x31C72F | 0x31C735 | Reseed for an area whose remembered day differs from the world day |
| rand | 0x31C77C | 0x31C782 | First direct draw if the daily refresh has a nonempty placement list |
| rand | 0x2E2686 | 0x2E268C | New resident with initial state byte 0x8A, adding 80 through 129 units to its initial altitude |
| rand | 0x2A4CCC | 0x2A4CD2 | Effect after an eligible type 0x90 death changes at least one terrain block |
| rand | 0x2A4FDB | 0x2A4FE1 | Per eligible nearby target of a death-area effect. A result divisible by four selects doubled effect strength. |
| rand | 0x2CE8D9 | 0x2CE8DF | Taming effect before possible conversion to a player pet |
| srand | 0x2B67D1 | 0x2B67D7 | Pet setup using the creature's 32-bit type at offset 0x64 |
| rand | 0x2DEBA7 | 0x2DEBAD | Physics effect for type 0x12E on the eligible terrain and accumulator branch |
| rand | 0x2E0455 | 0x2E045B | Local actor's drowning damage effect on the eligible 500 ms boundary |
| rand | 0x2E05F7 | 0x2E05FD | Physics effect when the eligible actor's timer at offset 0x12C passes below 590 from at least 590 |
| srand | 0x2AE13D | 0x2AE143 | Area generation cache miss at 0x2AE0C0, before the generated area is published |
| srand | 0x2B5F9A | 0x2B5FA0 | Whole-world initialization at 0x2B5F20 |

A refreshed area takes its seed from the world day plus 100,000 minus a deterministic area-coordinate term. The refresh can reseed and return with zero draws of its own. Its other direct rand return sites, in address order, are 0x31C7D5, 0x31C811, 0x31C97F, 0x31C9A2, 0x31C9C1, 0x31C9E6, 0x31CA09, 0x31CA49, 0x31CA8A, 0x31CB21, 0x31CC53, 0x31CD24, 0x31D086, 0x31D19B, 0x31D204, 0x31D26E, and 0x31D2E9. It can also draw through shared appearance, equipment, and selection routines.

The resident pass collects loaded area records near eligible players. For each selected area it refreshes the day before processing that area's resident entries. The direct death draws occur in the following death pass. Player-pet handling follows the death pass. Later the step iterates creatures, invoking living-creature updates for living entries and physics for each entry. Taming can also enter pet setup during that later living-creature update. Subsequent physics draws depend on their own conditions. Different areas, creatures, and substeps can contribute intervening events.

### Pet appearance and equipment

The pet seed is followed by appearance setup at 0x053E80 with five consecutive initial rand returns at 0x053F27, 0x053F41, 0x053F60, 0x053F8E, and 0x053FB4. The appearance type can add draws. The appearance caller return from pet setup is 0x2B67EA.

The pet equipment call enters 0x0511E0 through 0x052C40. For an equipment draw on this path the next returns are 0x052CCB and 0x2B67F2 before the pet setup caller in the stack table. Two direct equipment callers belonging to daily refresh instead return to 0x31C89F and 0x31CE95.

The common final nine equipment rand returns occur consecutively in this order: 0x05297B, 0x052987, 0x052993, 0x05299F, 0x0529AB, 0x0529B7, 0x0529C3, 0x0529CF, and 0x0529DB. An earlier nine-draw block for creature kind other than 6 returns in order to 0x0528D5, 0x0528E1, 0x0528ED, 0x0528F9, 0x052905, 0x052911, 0x05291D, 0x052929, and 0x052935. Kind 6, which includes a newly created or successfully tamed player pet on these routes, uses deterministic type-derived values for the earlier block and still takes the final nine. Earlier conditional equipment draws can precede these blocks. The final block by itself identifies equipment assembly and is shared by multiple callers.

The physics return at 0x2E05FD is a conditional later marker. The eligible actor is the world's local actor or a non-player creature on a world with mode byte zero. This effect can occur when an impulse sets the timer to 600 and a subsequent physics interval carries it below 590. The source shows such timer writes at 0x09D1A3 for the frame's actor and 0x00DD2C in creature control. An equipment return at 0x0529DB gives no guarantee about which entity or import event follows.

## Using records with only the immediate return

Read the events in recorded order for each thread ID within a single process run. A daily seed at 0x31C735 marks live resident processing, with the frame and authority possibilities above. Direct frame rand returns such as 0x0A70DF and 0x0A70FA identify the frame thread if present. Given the two established ordinary incoming paths, a daily seed on a different thread from the positively identified frame thread identifies the simulation worker in that run. A stack containing the authority chain provides direct caller evidence even if the frame marker is absent. Area-generation seeds can identify generation activity in the event-only records. The older return sites distinguish host service generation from the frame-side generation jobs. Seed values and raw draw results can match across caller paths.

In a newly populated solo session, the first reliable opportunity for a daily seed is the first live resident sweep that actually sees a published area near the player with a different remembered day. New areas start with a remembered day of minus one at 0x2F9732. The seed can appear without combat, a pet, an in-game day rollover, or a subsequent daily draw. For an already current area, a new eligible area or a world-day change gives another opportunity. An idle window over current areas may contain no daily seed. For a separate pet marker, the source shows new pet creation from an eligible equipped pet item, a type-changing pet rebuild, and successful taming as conditional triggers.

After identifying a worker thread, retain the order of all its observed CRT events, including same-thread seeds, intervening draws, and unfamiliar sites. These sites label events and phases rather than tick boundaries. A partial trace with neither a frame anchor nor older return sites can leave the caller unresolved. The import events carry no area, entity, tick, or world identity. This is a static contract for the ordinary source paths. It establishes no process-specific thread number, first event for an arbitrary session, or exhaustive inventory of all CRT callers.
