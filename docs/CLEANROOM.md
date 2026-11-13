# Clean-room process

## Shrines of Life - 2026-11-12

An independent original-only contract, prompted by a player's recollection, describes Steam's
Shrines of Life. Near a shrine statue the use key plays the flute every character owns; after
five seconds the shrine is registered for that character, which makes it glow, puts its icon
on the map for a free teleport and makes it a respawn point. CubeVulkan had the glow, the map
icon and the teleport, but the use key never offered the flute in the Native world (it read
the world's folder name, which a generated world lacks, instead of the World's own name), and
the flute's registration was never connected. Both now work. Revive now picks the nearest
registered shrine in the player's land, else the land's nearest shrine within about 8,192
blocks, else the place of death, standing at the shrine's level. Three CPU checks and the
in-game flute run failed before; all pass now. All five targets, full required-owner CPU
without skips, every Vulkan group, SDK independence and the Native/Alpha probes pass.


## Title-screen clock and goddess statue - 2026-11-12

Two independent original-only contracts are held. On the title, character selection and
character creation, Steam steps its named World at the ordinary 10x rate in 16 ms steps,
starting from the saved world time (10:00 on day 1 when there is none); there is no fixed
backdrop hour. CubeVulkan already used Steam's clock in play but ran these pages at 60x; they
now use the same native clock. [Time] Mode=Custom keeps the optional constant speed. The
goddess statue's flute gift is unreachable in Steam, and CubeVulkan's statue is already inert.
Three CPU checks failed against a stub and pass now. All five targets, full required-owner
CPU without skips, every Vulkan group, SDK independence and the Native/Alpha probes pass.


## Pickup requests and success music - 2026-11-12

An independent original-only contract describes Steam's success-music triggers and how
ground coins are requested. Each world step, Steam counts down a dropped item's pickup delay
in the zones near players, on the host as well; CubeVulkan's host never did, so every resend
restarted the delay. A walked-over coin is requested every frame unless the same request is
still waiting; CubeVulkan asked only once a second. Both now match. The success track now
starts as in Steam: when a treasure, artifact, special item or glyph is received while the
player is free to hold it up, when a reward chest's hold pose is taken, and when the place the
player stood in becomes completed; shop purchases, crafting, identifying and taming no longer
start it. Seven CPU checks failed against stubs and pass now. All five targets, full
required-owner CPU without skips, every Vulkan group, SDK independence and the Native/Alpha
probes pass.


## Music order and reward popup - 2026-11-12

An independent original-only contract answers the reward popup and music questions.
Steam's reward popup never appears: nothing in the game ever queues one, so CubeVulkan's
lack of it already matches. Steam updates the music inside the game update, before the
frame-limit sleep and present; CubeVulkan did it after present, in the world and on the
title pages. It now runs before present in both, and the title pages count the success and
instrument timers down as the world does. Those timers and the track's play counter are
whole milliseconds in Steam, truncated every frame; CubeVulkan kept fractions. Three CPU
checks failed against a stub and pass now. All five targets, full required-owner CPU without
skips, every Vulkan group, SDK independence and the Native/Alpha probes pass.


## Night lamp - 2026-11-12

Steam lights the player's lamp by night (before 05:00 or from 19:00, by the world's time
of day) while the title, character selection or character creation is shown, and during
an eagle flight, and puts it out by day; otherwise the lamp is the player's own toggle.
CubeVulkan also drove it on the world pages and never during a flight. Both now follow
Game update phase 10 through one rule read before the world step. Eight of ten CPU checks
failed against a stub, and an in-game flight with the lamp set the wrong way stayed wrong
at 23:00 and at noon; Native and Alpha flights now set and clear it. All five targets,
full required-owner CPU without skips, every Vulkan group, SDK independence and the
Native/Alpha probes pass.


## Alpha coin fixture - 2026-11-12

The notice fixture's coins were never collected in the Alpha world. The cause was the
fixture, not the game: it laid the coins at 4 s, before the host world had loaded the
player's zone, and Steam's drop rule discards an item laid in a zone that is not loaded.
The host loads that zone about 7 s into a Native session and 12 to 16 s into an Alpha
one. The coin fixtures now wait for the host's zone, and the notice check waits for all
five coins (up to 30 s). The published build failed in Alpha (nothing collected); Alpha
and Native now both collect three gold and two platinum with every notice. All five
targets, full required-owner CPU without skips, every Vulkan group, SDK independence and
the Native/Alpha probes pass.


## Gold and platinum notices - 2026-11-12

An independent original-only contract now describes Steam's progress notices. Gold
gained during the frame shows a pale gold "+N GOLD", and platinum coins collected in
the current zone show "+N PLATINUM", each floating up from a random point near the
player for three seconds. The XP and level notices ("+N XP", "LEVEL UP!", their chat
lines and the level sound) are implemented as specified, but Steam never shows them in
ordinary play because the level is recomputed before its snapshot; CubeVulkan behaves
the same. Gold from selling does not show a notice, as in Steam. The FEAT! text now
shares the same floating-text records.

Twelve of sixteen CPU checks failed against stubs, and an in-game fixture that lays
three gold and two platinum coins showed no notice before the change. Every gain now
arrives as its notice in Native, and a capture shows the text. In Alpha the fixture's
coins are never collected, before and after the change, so it proves nothing there. All five
targets, full required-owner CPU without skips, every Vulkan group, SDK independence
and the earlier probes pass. Pyro mode keeps its own XP texts.


## Frame budget sleeps before present - 2026-11-12

Following the held Game update schedule (section 2.3), the frame limit is now
measured from a millisecond tick taken when the frame starts and its sleep is taken
just before the frame is presented, with the timer resolution raised to 1 ms. Before,
CubeVulkan slept after presenting. The title, character and creation pages share
the same frame driver in Steam, so they now take the same sleep; before they had no
limit. Map presentation smoothing now steps by the 1000 ms-capped interval (section
2.4, phase 81) instead of a 100 ms cap.

Three of six CPU checks of the budget rule failed against a stub. An in-game fixture
with a 1500 ms budget, above the start-up frame time, shows every frame sleeping
just before present and the mean interval reaching the budget in Native and Alpha.
All five targets, full required-owner CPU without skips, every Vulkan group, SDK
independence and the earlier probes pass. The Ring closing while the full map is
open was checked and already matches Steam, where the Interface update hides it.


## Maximum health follows the world step - 2026-11-12

An independent original-only contract now lists every phase of Steam's Game update
in order, with its gates. Following it, the player's maximum health is taken before
the world step and, if the step changes it, the player's health is rescaled by the
new maximum over the old and capped at the new maximum, before creatures' drawn
places are updated. Before, health stayed where it was: a buff that raised the
maximum could run out and leave health above the maximum.

Six of seven CPU checks failed against a stub, and an in-game fixture whose doubling
buff runs out inside the world step left health at twice the maximum before the
change. Both now pass in Native and Alpha. All five targets, full required-owner CPU
without skips, every Vulkan group, SDK independence and twenty-five Native/Alpha
probes pass. The other compared phases that differ (notices, the frontend lamp, the
Ring close, short-frame caps, frame-limit placement, the reward popup and some
ordering) are recorded as open and unchanged.


## First-frame HUD parent - 2026-11-12

Following the held first-frame contract, the HUD parent that gates Chat's keydown now
keeps its value until the interface's late publication, as in Steam. It starts at one,
so an Enter in the very first message drain opens Chat on the title page; the first
late publication then writes zero. Page changes never publish it: after a return to
the title a key still sees the gameplay value until that frame's publication. The
publication also runs on the frame whose callback ends a menu visit, except on quit.
Before, the engine published at the start of every visit and on in-game page changes.

Five of seven first-frame checks failed before the change and all seven pass in both
profiles now. All five targets, full required-owner CPU without skips, every Vulkan
group, SDK independence and twenty-five Native/Alpha probes pass. The fixture queues
messages directly; OS message timing during loading and the contract's unproved items
remain open.


## Doors and seats through the host thread - 2026-11-11

A new `--door-seat-test` fixture checks that, with the host thread running, the
player's door and seat uses still reach the host and come back. It hands the player
two door uses and one seat use as the use key hands them. Each reaches the host as a
packet-9 interaction; the host applies it and sends the changed zone's objects back.
The viewed World shows the door opened, shut again and the seat taken, each after the
minimum two frames, and the host World holds the same state. Over a 70 s Native
session, host-side creatures swung doors on their own (counted in the host World,
which the creature log line now reports). The Alpha start has no town nearby, so no
Alpha door scene was run; aiming and OS input are not exercised, and remote guests
are still unrun.


## Host and AI threads - 2026-11-11

Following the held host-threading contract, the in-process host now runs as Steam
runs it. A host thread ticks it every 32 ms measured start to start, without catching
up, and idles in 33 ms sleeps while this player is a guest elsewhere. The host
World's creature AI runs on its own 40 ms thread with the real elapsed time, so the
host's 16 ms substeps no longer think. A viewed mirror World runs no AI at all, as
Steam never ticks AI for the live World; its player's doors and seats come from the
host's AI. The main thread keeps the frame, the mirror World and the incoming
messages.

Host results for game-thread state (deaths, the daily reset, region levels and
notices) are posted and run at the start of the next frame. Every other game-thread
use of the host World or host link takes the host lock; the AI thread takes it too,
so AI and the host tick never overlap. Steam and UDP transports are serialized for
the two threads.

Nine thread checks and one mirror-AI check failed before the change and now pass.
All five targets, focused groups, full required-owner CPU without skips, every Vulkan
group, SDK independence and twenty-three Native/Alpha probes pass. Two Native
full-distance Help captures settle in 45 s and 43 s (63-73 s before) and the Alpha
capture in 89 s. Remote multiplayer was not run, and no FPS or hardware claim is made.


## Host threading contract recorded - 2026-11-11

An independent original-only contract now describes how the Steam release runs its
in-process host. A dedicated thread calls the host tick every 32 ms, capped at
1000 ms and stepping the host World in 16 ms slices. Creature AI runs on its own
40 ms thread, for the host World only; the live World never runs AI and only
mirrors host messages, applied once per frame on the main thread. The contract
lists every thread, lock and lock order.

The engine already matches the host tick's cadence, cap and slicing, and gives the
host World its own terrain source and streaming worker. It differs in three ways:
the host tick and its AI run on the main thread inside the frame instead of on
their own threads, AI is stepped inside the host's substeps instead of on a
separate 40 ms clock, and a client-mirror live World still runs AI for the local
player. No scheduling change is made in this checkpoint; the binaries are those of
the last full validation.


## World pages built as an optional extension - 2026-11-11

At the user's request the Steam world selection and world creation pages are now
built, following the held world-list contract: cards for the entries of the current
mode and a trailing New world card, the sticky current card with its Select or
Create, small and Delete World buttons, drag scrolling shared with the character
list, the multiplayer toggle and its captions, the Name and Seed fields with the
digits-only Create World rule, the select, create and delete handlers, and Back.

Steam never shows these pages, so the default flow is unchanged. CubeVulkan's own
[Menus] WorldPages=1 makes a valid character's Select open world selection;
selecting or creating a world then enters gameplay with that character and records
the world after the character entry, as Steam orders it. The list lives for one
session because Steam never opens its world store. No files are deleted, the card
preview model is not drawn, and the recorded world does not yet change the
generated world: the applied seed and name stay at their start-up values.

Twelve world-list assertions failed against empty rule bodies and now pass. A nine-
case page fixture passes in Native and Alpha, and both pages were captured and
inspected. All five targets, six focused groups, full required-owner CPU without
skips, every Vulkan group, SDK independence and twenty-three Native/Alpha probes
pass. Private save bytes are unchanged.


## World list pages - 2026-11-11

An independent original-only contract describes the Steam world selection and
world creation pages, their cards, edit fields and handlers, and the world-list
store. Its headline: in the supported image both pages are built but cannot be
reached from the title or character flow. The selection page is shown only by
Back from the creation page, and the creation page only from the selection page.
A successful character selection goes straight to gameplay. The world-list store
is never opened or read, so the list is empty at every start.

The engine already matches everything reachable. Back moves world creation to
world selection, world selection and character creation to character selection,
and character selection to the title, as the existing literal Back tests show.
No engine path makes either world page visible. The unreachable widgets, cards,
handlers and store writes are deliberately not implemented, so no dead interface
is added. The contract leaves open whether a path outside the decoded code can
show a world page; that needs an owner-side runtime check. The contract was
whole-read and hash-verified, and its author received no engine, test or result
feedback. No binary changed; the last full validation still applies.


## Native full-distance readiness: host AI and physics cost - 2026-11-11

The Native full-distance Help capture that failed at its unchanged 120 s observation
deadline now settles. A sampling profile of the main thread showed the cause: the
in-process host World runs inside the session frame, and its AI ticks and physics
took most of every frame, so only a few frames ran each second. Terrain batches are
integrated once per frame, so preparation outlasted the deadline. Deadlines,
streaming order and demand are unchanged.

Three measured, behavior-identical changes remove most of that cost. The AI search
pick walks its ordered open and step containers together instead of looking up each
open block; it keeps the first lowest cost in block order and still skips open
blocks without a step. The search containers compare block keys directly in exactly
the standard x, y, z order. The physics easing factor, a loop of one iteration per
hundredth of a millisecond, is cached per thread by its complete inputs.

A new test checks the pick against the previous loop and the container order against
the standard order on 4000 random searches, with ties and stepless open blocks. All
five targets, full required-owner CPU without skips, every Vulkan group, SDK
independence and twenty-three actual Native/Alpha probes pass. Two Native and one
Alpha full-distance Help captures settle. The recorded native AI tick files are not
present on this machine, so their replay was not repeated. The host World still
shares the main thread. No FPS, hardware or reliability claim is made.


## World selection record and non-local creature removal - 2026-11-11

A world selection is recorded, never applied. The retained Game state now owns
the requested seed/name pair, starting at seed 123 and the name "world". The
selection recorder stores the raw signed seed and the name into the bound
character's last-world record, then into the requested pair, and then removes
every creature except the local player, all inside one World guard. The applied
pair, loaded zones, the world-state block and the local entity are not touched.
The comparison is exact: seeds first, then case-sensitive name bytes.

The whole independent [selection contract](specs/steam-pending-source-selection-and-world-reconfiguration-20261110.md)
was read and hash-verified before implementation. It was written by a fresh
original-only author that received no engine, tests, runtime outputs or
verification feedback. Seven new CPU assertions failed against empty bodies and
now pass. Twenty-three actual Native/Alpha probes, all five targets, six focused
test groups, full required-owner CPU without skips, every Vulkan group and SDK
independence pass. Private save bytes are unchanged. Broader regression checks
retain the Alpha terrain/lighting, dry purple Undead Lands, distinct palettes,
gentler Savannah and Omega water paths.

No frontend page calls the recorder yet. The contract leaves the visible result
of differing requested and applied pairs unresolved, so the update's comparison
gate is deliberately not wired. The create/select world pages, host-switch zone
undo, the unidentified second World list and the contract's provider limits
remain open. Unbound character data skips only the last-world record, as an
owned engine policy. No new settled-scene, original-pixel, hardware, FPS or
cold-start reliability claim is made. Owner content, captures, saves, private
memory and binaries are excluded from public source.


## Frontend Chat ingress and retained pending order - 2026-11-10

Frontend messages now reach the retained Game chat through the same admission
helpers as gameplay, after the GUI receiver returns. Keydown tests the HUD parent's
own selected value, raw GUI focus and signed Controls capture index. Characters
retain their separate focus/typing admission and signed low-byte conversion.
Hidden manager ancestors and frontend pages do not add character gates. Shift is
queried only at the reached caret-edit phase. Escape retains the editor draft.

Frontend page callbacks publish their page roots while retaining the previous
HUD-parent value until the selected later update. Chat callbacks are scoped to
each message; completed Session objects leave no stored callback behind. Shared
command dispatch binds the current actor, feedback and pending-message owner
before title menus run. Ordinary transitions retain the editor, history and queue.

One pending-message traversal chooses the newest tail record. It repeats the
reached name/tail lookups, logs from the current tail and removes the then-current
tail after logging. New-token metrics run before space/line-break decisions;
unordered carried widths append, and finite negative row capacities preserve
the selected signed-to-unsigned comparison. Generic providers and unsafe native
interleavings are not inferred from these normally returning selected paths.

The whole independent [Chat ingress/ownership contract](specs/steam-frontend-chat-ingress-and-owner-lifetime-20261110.md)
was read and hash-verified before implementation. Earlier lifetime/entry contracts
and their receipts remain unchanged. The original-only author received no engine,
tests, runtime outputs or verification feedback. Five actual frontend assertions
and seven CPU queue/layout assertions failed before the correction and now pass.
Eight frontend Chat cases pass in each Native/Alpha profile, along with twenty-one
affected actual probes, all five targets, six focused test groups, full required-
owner CPU without skips, every Vulkan group and SDK independence. An earlier
build failed four Native Help assertions because that fixture assumed immediate
HUD-parent publication; the fixture now requests the late publication explicitly
and the failed run is preserved. Private save
bytes are unchanged. Broader regression checks retain the Alpha terrain/lighting,
dry purple Undead Lands, distinct palettes, gentler Savannah and Omega water paths.

Native first-frame HUD-parent initialization, the complete Game update schedule,
transport delivery/producers, full GUI geometry/fonts, reentry/concurrency and
broader Game/World/backend behavior remain open. The owned first-frame publication,
legacy direct-state API, safe missing-provider/invalid-height handling and existing
gameplay transport/local-echo adapter remain explicitly bounded engine policies.
The pending-tail rule does not establish whole native network-chat delivery.
No new settled-scene, original-pixel, hardware, FPS or cold-start reliability claim
is made. The earlier Native full-distance readiness failure and source deadline
miss retain their exact evidence and unchanged deadlines. Owner content, captures,
saves, private memory and binaries are excluded from public source.


## Retained character entry and single startup handoff - 2026-11-10

Continue and character Select previously entered through a cached preview and a
replacement-style player reset. The selected frontend entry now reads the current
backend row into the existing live actor. Saved position, zero or negative health
and level keep their raw values; loaded buffs, separate retained collections and
omitted core/appearance/outer bytes follow the selected decoder rules.

Continue retains its six camera angles, publishes both distances as eight and
leaves the backend slot key alone. Stored Select publishes its current choice
after loading, updates the selected class tabs and applies its separate camera
reset. A normally returning lookup miss retains the actor and CharacterData while
still publishing the frontend exit and Continue distances. A separate owned full
CharacterData source supplies that miss path's inventory/data snapshot. Routine
persistence keeps its existing core-only callback and single progress capture.

Gameplay receives the already-live entry data and avoids a second decoder or
startup equipment reset. An actual frontend-to-gameplay fixture stages a mutation
after the frontend decoder; its reset-owned map survives the startup bridge. The
shared data callbacks own their inputs without borrowing a completed gameplay
stack, renderer or World. Recorded-model placement retains its compatibility path.

The complete [Continue/entry contract](specs/steam-continue-and-saved-world-entry-publication-20261110.md)
was whole-read and hash-verified before use, alongside unchanged entity/lifetime
handoffs. It distinguishes title Continue from system Start Menu and stored-world
Select. No implementation, tests, owner output or results reached the isolated
original-only author, and all prior held prose and receipts remain unchanged.

Eight actual entry assertions failed before the change. Ten selected-entry checks
and four actual frontend-to-gameplay checks now pass in each Native/Alpha profile,
with seventeen affected runtime regressions. All five targets, focused ownership/
Continue/frontend checks, full required-owner CPU without skips, every Vulkan
group and SDK independence pass on this checkpoint. A new ownership check initially
used the core-only observer as a full-data snapshot and indexed an empty inventory;
that failed run is preserved. The separate full-data API and bounds-checked test
close the corresponding entry-miss gap. An Alpha handoff setup initially used a
Native-profile save with no selected Alpha character; that failed setup is retained,
and the corrected Alpha-profile fixture passes. Private save bytes are unchanged.

This closes selected entry decoding and adapter publication, not the complete
native frontend release prefix/GUI admission, derived D+0x128 refresh, candidate
rebuild, Start Menu, stored-world publication or full Game/World/backend graph.
Generic providers, reentry/concurrency, frontend Chat/mouse, other input consumers
and final native/human/provenance acceptance remain open. No new settled gameplay
capture or original pixel/hardware/FPS claim is made. The earlier Native full-distance
readiness failure and source deadline miss remain unexplained; no repeated case,
observation deadline or production terrain/shader/streaming change masks them.
Alpha relief/lighting, dry purple Undead Lands, biome palettes, gentler Savannah and
Omega water retain their existing paths. Owner files, captures and binaries stay private.


## Help rendering and retained player state - 2026-11-10

Help now draws action-specific rows through its retained HUD node, with descriptions
from the owner's dictionary. Numeric action IDs retain all 32 bits; the modifier is
read once, while bindings, characters and selected buff/specialization queries are
read at their reached phases. The optional class action controls the pet row.
Terms, variants and macros compile once, retaining the selected UTF-16 keys,
conditional/seed decisions, reference contexts, word joins and first-block output.
Missing descriptions remain empty. Existing NPC/Speech paths stay separate.

The Help node is placed relative to window height through its actual parent
hierarchy. Text keeps the separate raster-space row point and fractional-scale
rounding order. The whole Help body runs inside its admitted draw; the second
annotation for action 50 uses the first annotation's returned flow, truncated to
a signed DWORD and advanced by twenty. It needs no duplicate measurement. Hidden
ancestors suppress even a previously prepared draw body without clearing Help.

The CPU player record and its separately owned collections survive render/World
adapter teardown. A fresh adapter registers the retained player once before later
residents; the selected decoder still replaces saved position/health and clears
its established maps. Native startup gear copies defined appearance fields,
preserving omitted padding. Complete native Game/World/backend identity and
ordinary frontend Continue/saved-world entry remain separate unfinished work.

The complete independent [dictionary](specs/steam-help-action-description-dictionaries-and-term-resolution-20261110.md),
[entity](specs/steam-live-entity-reset-default-and-character-load-fields-20261110.md),
[placement](specs/steam-help-root-placement-and-text-space-20261110.md) and
[annotation](specs/steam-help-annotation-flow-arguments-20261110.md) handoffs were
whole-read and hash-verified before use, alongside the previously held row/lifetime
contracts. The annotation audit independently corrects earlier fixed double-cost
placement and selected binding/query-order clauses. Earlier prose and receipts
remain unchanged; the newer contract governs those corrected clauses. The
original-only author received no implementation, tests, owner outputs or results.

Source-derived before-failure checks cover row semantics, retained allocation and
fields, buff padding, height-relative placement, fractional text space and flow/
query ordering. All five targets and focused checks pass. Seventeen actual Native/
Alpha probes pass, including two-session entity/input lifetimes, seventeen Help
assertions per profile, frontend publication, mouse/Chat/menu admissions and
Controls at ordinary and 1.5 scale. Full required-owner CPU has no skips; every
Vulkan group and SDK independence pass. Alpha's render-distance-50 scene and a
separately declared Native render-distance-10 UI scene were visually inspected for
Help layout. The Native render-distance-50 scene exceeded its unchanged world-settle
deadline with 599 prepared zones, one mesh job, 723 loaded source zones and four
source jobs. That failed run remains private and unchanged; the smaller UI fixture
does not resolve full-distance readiness. No production streaming or observation
deadline changed. Private save bytes are unchanged. These are owned fixtures and
scene checks, without original pixel, hardware, FPS or complete gameplay acceptance.

UTF-8 XML/locale handling, safe ownership/cycle/length guards, explicit transactional
reload, fresh World registration and invalid floating conversion behavior are
engine adapter policies. Native XML/font/matrix/cache/provider/reentry/thread and
first-frame behavior, broader localization, full Game/World persistence, frontend
Chat/mouse, other input consumers and final native/human/provenance acceptance
remain open. Native full-distance readiness and the earlier source deadline miss
remain unexplained, without a production streaming change. Alpha relief/lighting,
dry purple Undead Lands,
distinct palettes, gentler Savannah and Omega water retain their existing paths.
Owner files, captures, saves and binaries stay outside public source.


## Frontend sampled input publication - 2026-11-10

Title and character pages now publish into the same retained control owner used
by gameplay. Admitted physical bytes become visible before synchronous pointer
work. The later controller merge keeps compatibility edges relative to the previous
completed sample, so a held controller button does not become a new press after
an intermediate physical zero. Capture and inactive visits retain current levels.

The frontend runner completes its controller merge even when a callback has just
cleared every frontend page. Gameplay menu runners begin the physical phase and
leave completion to their existing gameplay caller, once. Caller mode supplies
this distinction; current page visibility cannot replace it. Detached callers
retain the existing combined GameInput update API.

This consumes the already whole-read, unchanged [lifetime contract](specs/steam-game-interface-and-input-lifetime-through-session-transitions-20261110.md)
and the existing independent physical/sampled contracts through their owned input
paths. A new private receipt retains their provenance; no source artifact was
rewritten and no implementation, tests, owner outputs or verification feedback
reached the original-only author.

Three baseline failures showed a real physical level of one while the borrowed
Game consumer remained zero. Five actual frontend cases now pass in Native and
Alpha: ordinary publication, capture, inactive polling, release and a final visit
with all frontend pages cleared. They also check the value at the pointer stage.
The held-controller/intermediate-zero regression, actual two-session lifetimes,
mouse boundaries, both Chat orders, sampled menu admissions and all twenty Controls
cases pass on the current executable, including GUI scale 1.5. Private save bytes
are unchanged. All five targets, full required-owner CPU without skips, all Vulkan
groups and SDK independence pass. No new settled-scene or original-runtime claim
is made by these explicit device-buffer and queue fixtures.

Complete native Game/World/entity identity, frontend Chat and full mouse business,
early zoom application placement/native resource metadata, the full main-loop
scheduler and imported providers remain unfinished. Other state owners, Controls
parent/registration/general geometry, full Help and final native/human/provenance
acceptance remain open. The earlier source deadline miss remains unexplained with
no production streaming change. Alpha relief/lighting, purple dry Undead Lands,
distinct biome palettes and Omega water retain their existing paths. Owner data,
captures, saves and binaries stay outside public source.


## Retained Game input, Chat and Interface cache - 2026-11-10

Sampled control state, Chat editor/history/pending messages and the Interface's
cached mouse Y now have one coordinator owner across render/world session teardown.
The session borrows these values. Returning through the engine's Start Menu adapter
therefore does not recreate them as empty values. Existing raw keys, physical
buffers, staged Controls and independent process consumers retain their own paths.

The complete [Game/Interface lifetime contract](specs/steam-game-interface-and-input-lifetime-through-session-transitions-20261110.md)
was whole-read and hash-verified before use, then copied unchanged. It establishes
retained native owners across the selected ordinary transitions, with subordinate
character-data and UI-row replacements and separate release/draw/input writers.
The original-only author received no implementation, tests, owner outputs or feedback.
Native construction does not prove first values for all 23 sampled bytes; defined
first zeros remain an explicit engine policy.

Three baseline failures reproduced lost sampled bytes, Chat state and exact cached-Y
bits across two real runSession startups/shutdowns. Both Native and Alpha now pass
the retained-state checks with actual World/renderer/device teardown and unchanged
private saves. This explicit CLI fixture bypasses frontend selection on both visits;
its Chat messages remain local. Mouse boundaries, both queued Chat orders, sampled
menu admissions and all twenty Controls cases also pass on the current executable,
including GUI scale 1.5. All five targets, full required-owner CPU without skips,
all Vulkan groups and SDK independence pass. Previous Controls/title images retain
their earlier exact-binary scope; no new settled-scene claim is made here.

This change retains the selected values. The engine still reconstructs its World,
character and network backend objects; complete native G/W/E identity, subordinate
reset mapping, frontend physical/Chat routing, other widget state and independent
Use/quick-item observers remain unfinished. Native startup/reentrant callback,
provider/thread, visual/hardware/gameplay/multiplayer and final human/provenance
acceptance remain separate gates. The earlier asynchronous source deadline miss
is still unexplained, with no production streaming change or reliability claim.
Alpha relief/lighting, purple dry Undead Lands, distinct biome palettes and Omega
water retain their existing paths. Owner data, captures, saves and binaries remain
excluded from public source.


## Controls thumb and layout - 2026-11-10

The retained Controls thumb now receives GUI event 12 after the target's virtual
motion work. The ordinary Button press/capture path also runs for a motion-only
binding, so held dragging continues through a hidden ancestor. The callback tests
the manager's held level before geometry, retains candidate/capture rows and keeps
continuous thumb position separate from the truncated selected row. Callback
teardown clears the binding before its Session ends.

Up/down callbacks now publish the actual retained thumb geometry synchronously,
position before size, using the current frame and thumb width. Arrow insets and
button positions use the independently specified formulas and queried dimensions;
Cancel uses OK's height. Existing pending edits, Defaults/OK/Cancel behavior and
draw-produced binding capture retain their separate paths.

The complete [Controls and Help contract](specs/steam-controls-scroll-thumb-and-help-action-rows-20261110.md)
was whole-read and hash-verified before use, then copied unchanged. The original-only
author received no implementation, tests, owner outputs or verification feedback.
This checkpoint applies selected Controls behavior only. Full Help rows and their
dynamic description dictionaries/resources remain unfinished.

Five actual baseline layout/motion failures become passing checks. Twenty actual
Controls cases pass in Native and Alpha at GUI scale one and Native at scale 1.5,
with unchanged private saves. The larger-scale fixture now queries its actual
owning scene rather than a detached scene that omitted the live root scale;
earlier failed runs remain recorded. All five targets, full required-owner CPU
without skips, all Vulkan groups and SDK independence pass. Current Native and
Alpha Controls images were captured at the unchanged readiness limits and both
inspected. Their fixed layout shows the retained title beneath the panel.

The current frontend still uses sibling templates and projects their geometry
into the Controls frame. This is an owned adapter, not proof of the native Q parent
graph, exact name/propagation registrations, arbitrary matrices/properties/providers,
reentrant layout query order, draw point timing or exception/lifetime/thread closure.
These checks do not certify full Controls, original pixels, OS hardware, gameplay,
multiplayer or final human/provenance acceptance. The earlier asynchronous source
deadline miss remains unexplained; no production streaming code changed. Alpha
relief/lighting, purple dry Undead Lands, distinct biome palettes and Omega water
retain their existing paths. Owner files, screenshots, saves and binaries remain
excluded from public source.


## Sampled control admission and process history - 2026-11-10

Native directional input now tests Chat after the actor's earlier preparation,
alongside the retained keyboard target and selector/map admissions. Ordinary
unfocused UI or Controls capture alone does not add a movement return. Main
supplies the current GUI keyboard target and current interface composite instead
of a stale panel snapshot. The two eight-byte water/jump rise operands now retain
their exact widened binary32 values. Four focused regressions failed before these
changes and pass afterward, including actor preparation that still precedes Chat.

The application retains controller history across Game/world and actor changes.
The six late menu consumers have separate process bytes, one outer Chat/three-page
admission, the verified 15/17/18/16/20/19 order and fresh level stores after each
synchronous callback. Their two selected mirror joins retain their order. Skipped
blocks retain their old bytes. The menu pass now follows packet/chat/feat work;
movement clears and selector hiding follow it, before Escape-menu publication.

Early zoom has its own two retained bytes and native-width arithmetic before the
ordered lower/upper corrections, including the zero-count -1 result. The current
engine supplies its existing seven-layer collection; complete native resource
metadata remains a separate boundary. Sampled levels preserve their byte values,
and producer edges remain available to compatibility callers. Native consumers
use their own reached history. Ring repeat arm/counters likewise survive selector
reconstruction independently of that selector's index and animation.

The complete [sampled consumer contract](specs/steam-sampled-control-consumers-and-gameplay-admission-20261110.md)
was whole-read and hash-verified before use. The original-only author received no
implementation, tests, owner outputs or feedback. Actual Native and Alpha probes
verify a Chat-skipped menu latch, retained capture levels under an ancestor hidden
by F4, and a skipped release that must not become a new consumer rise. Existing
mouse/scalar/two-drop and both chat-order probes also pass, with private saves
unchanged. All five targets, required-owner CPU without skips, all Vulkan groups
and SDK independence pass on the final binaries.

One first full-suite run missed the existing 20-second asynchronous source test
deadline. No production streaming code was changed. Added observation records
loaded masks and pending counts without driving another World update or widening
the deadline. The standalone and whole-suite diagnostic runs reached all nine
sources in 128.316 ms and 115.547 ms respectively. The original miss is retained;
its cause and general loading reliability are not established by those passes.

Other independent action/Use/rising/falling observers, current-actor and general
numeric/provider closure, actual Game/Interface construction/reset lifetime,
Controls thumb and complete Help, frontend release/motion/marker/flight geometry,
and final native/human/clean-room acceptance remain open. Detached simulation
state is an owned API policy. Prior scene captures retain their earlier binary
scope; no new native runtime, pixel or hardware claim is made. Alpha relief and
lighting, purple dry Undead Lands, distinct palettes and Omega water retain their
existing paths.


## Synchronous gameplay mouse transactions - 2026-11-10

Each queued Native mouse message now finishes its selected gameplay work before
the next message. The dispatcher retains the entry Game across GUI callbacks,
copies cached GUI Y as raw binary32 bits, then snapshots the current identification
root before the two map-arrow returns. The wrapping process clock is published
before its strict 200 ms test. These two process values survive Session changes;
the Interface Y cache has its separate Game lifetime.

Coordinate actions precede the clock, the existing movement-dirty flag resets
after it, and the initial maximum-health query precedes item changes. Canonical
inventory, shop, equipment, crafting and customization actions run before the
existing current-actor health adjustment. Chat stops editing and the direct
retained selector hides afterward. The movement byte and Chat's editing byte are
distinct. Unknown button values skip GUI bits and item actions while retaining
the admitted common tail. Valid-slot middle-button drops have no extra map gate.

Native tab, row and region GUI callbacks execute during GUI dispatch. The old
Native item mutations have been removed from the later presentation adapter.
Release orders GUI, selected-answer work, current identification/snapshot work,
shrine work and the final Controls reset/rearm. Identification retains its own
root when the bag hides. The direct selector is instantiated once under Gameplay,
keeps its identity when hidden/reopened and is distinct from crafting's selection
frame. Large Session callables are borrowed rather than copied per mouse event.

The new [clock, snapshot and widget contract](specs/steam-mouse-clock-snapshot-and-retained-widget-state-20261110.md)
was hash-verified and whole-read before use. Its original-only author received no
implementation, tests, owner outputs or verification feedback. Earlier complete
mouse-owner prose remains unchanged. The owner GUI receiver-replacement test,
ordered core checks, actual Native/Alpha scalar and release tests, two immediate
canonical inventory drops with a shown map, and both full-frame chat orders pass.
Private save hashes are unchanged. All five targets, full required-owner CPU with
no skips, all Vulkan groups and SDK independence pass.

This closes selected queue/transaction work, with explicit remaining boundaries:
the frontend menu-selection prefix, the separate G+370 price-widget provider,
motion-cache writer/timer, complete marker-tree/toggle and flight-provider closure,
general direct-query geometry, callback/object lifetime and threading, sampled
gameplay admission, full Help and Controls thumb behavior. No purchase/debit is
invented for the discarded price return. The queue fixtures do not establish OS
delivery, native runtime/hardware, broad gameplay/multiplayer or final human and
clean-room provenance acceptance. Alpha relief/lighting, purple dry Undead Lands,
distinct biome palettes and Omega water retain their existing paths.



Both current widget scenes were captured and inspected at the existing readiness
limits with identical private saves. The fixed layout extends beyond the 640x480
viewport. The Alpha actor reached Revive during ordinary loading. These single
captures do not establish native UI pixels, gameplay acceptance, FPS or reliability.

## Physical input, retained Controls and detailed binding text - 2026-11-10

Device bytes now produce the 23 current binding levels before motion and the
controller pass. Ordinary interface visibility does not suppress those physical
levels. Capture and inactive passes retain existing levels. Controls capture uses
device-buffer rising edges, including the original repeated mouse scans inside
the keyboard scan; window key messages remain a separate channel.

The Controls editor retains pending defaults/edits independently of active
bindings across screen sessions. Its admitted draw publishes the candidate row;
release resets capture and can arm that cached row without a new pick or matching
press. Ancestor hiding and its own visibility remain separate. Defaults, OK and
Cancel use retained GUI release callbacks; OK copies active bindings before the
persistence attempt and hide, while Cancel retains pending edits. The config
writer supplies tokens in the independently specified order.

Detailed names use the complete first dictionary and its exact 144 literals,
separate from compact menu labels and OS naming. Empty pairs, unsupported codes,
all category words and signed wrapped one-based mouse labels follow the held
formatter contract. UTF-16 output reaches the existing font path. Full native
Help action-specific rows/descriptions and general font/geometry providers remain
open.

The independent [physical producer contract](specs/steam-physical-binding-and-controls-capture-producers-20261109.md),
[detailed text contract](specs/steam-detailed-binding-name-dictionary-and-controls-text-20261110.md)
and [descriptor inventory](specs/steam-physical-device-format-tables-20261110.md)
were whole-read and hash-verified before use, then retained unchanged. No
implementation, tests, owner outputs or feedback reached the author. A separate public Windows ABI/data check matches both format
headers, all263 records and seven referenced identifiers. It caught and corrected
the initial ANSI/Unicode interface mismatch before publication. Native uses the
Unicode interface. Provider implementations and current device availability are
separate evidence boundaries.

A missing DirectInput mouse has an explicitly logged GLFW compatibility backend;
this is owned host policy outside the native valid ordinary graph. Failed reads
from a supplied device still consume actual retained/partial output without fresh
clears or retries. Defined initial storage, null-provider diagnostics and neutral
device input in explicit hidden-pointer fixtures are also engine policies.
Ordinary foreground device sampling retains its distinct admission. The owned
menu-query API also keeps an isolated editor when no input service is supplied;
live menu sessions share only the application's retained editor.

One current Native scene and one Alpha/Omega scene pass the unchanged readiness
deadlines with identical private saves; both images were inspected. The Alpha actor
reached death/Revive during loading, so this is not full gameplay acceptance.
These single runs do not establish loading reliability, FPS or a performance cause.

Five actual Controls baseline cases failed before. Twelve current retained-scene
cases and both actual Main chat orders pass in Native and Alpha/Omega with
unchanged private saves. All five targets, full required-owner CPU without skips,
all Vulkan groups and SDK independence pass. These queue/scene adapters are not
OS injection or original-runtime/hardware parity proof. Full synchronous Game
mouse business, title/release prefix, other widget/global lifetime, Controls thumb
drag, complete controller/provider behavior and final native/human/provenance
acceptance remain open. Owner files, captures, private evidence and binaries stay
local; purple dry Undead Lands, distinct biome palettes, Alpha relief/lighting and
Omega water retain their existing paths.

## Mouse chat order and health-tail continuation, 2026-11-10

The original-only author finalized the complete mouse-owner contract without
implementation, SDK, tests, owner outputs or feedback. The implementer read all
493 lines, verified its hash and copied unchanged code-free prose before using
it. This checkpoint applies only the admitted-press chat order and selected common
health tail; it does not claim the full mouse owner or its transitive effects.

The actual Main frame had a second delayed chat-typing clear after the ordered
message clear. Removing it preserves MouseDown then Enter/character and the
opposite order. Health rescaling now reloads the current actor after each maximum
provider, keeps binary32 division before multiplication, and selects the sampled
cap on unordered comparison while preserving current equal-value bits. The
previous zero-maximum test was corrected: invalid division is still stored before
the second sample, whose zero cap then wins. Defined engine handling of an absent
actor is outside the native ordinary valid graph and is not a recovery claim.

Two scalar regressions and the actual full-frame chat probe failed before; the
current-actor, signed-zero/NaN and both message-order cases pass afterward in
Native and Alpha/Omega private runs with unchanged save bytes. All five targets,
full required-owner CPU without skips, all Vulkan groups and SDK independence
pass. These deterministic queue adapters are not OS injection or original-runtime
proof. Prior settled-scene captures retain their own earlier exact-binary scope.

Inventory business and other mouse work still use the remaining frame adapter.
Complete synchronous item/secondary work, process-global snapshot/clock lifetime,
the other chat-widget byte, retained selector, release-before-GUI prefix, final
widget resets, generic providers/lifetimes and broad native acceptance remain
open. No implementation or verification feedback reached the source author;
owner data, captures and private evidence stay local.

## Owned derived-light publication concurrency, 2026-11-09

This change uses only the engine's own immutable terrain/model storage, lock
lifetimes, diagnostic measurements and actual concurrency regressions. It consumes
no new original/reconstruction/SDK facts. Source research continues independently
without implementation, verification or owner-output feedback. Earlier complete
held caption contracts are retained unchanged provenance payloads, not authority
for this engine-specific synchronization policy.

Only private mesh-light publication uses a terrain-only transaction. It keeps
the terrain writer mutex, cache-guarded COW commit and current loaded-identity
check. Public world-state edits retain source-then-terrain protection; nesting a
world-state edit into a terrain-only root is rejected before an inverted lock.
Stored-light arithmetic, channel/record bytes, notification rules, owner data,
terrain palettes, Alpha composition/lighting, Omega water and readiness gates
are unchanged. Optional timing separates source/writer acquisition from remaining
publication work, without clock sampling when disabled.

The actual mesh regression failed on retained frame-source protection before
the change and passes afterward while still enforcing terrain-writer exclusion,
physical edits/dirty requests, held snapshots, stored light and loaded identity.
All five targets, full required-owner CPU without skips, all Vulkan groups and
SDK independence pass. One current Native scene and one Alpha/Omega scene pass
the unchanged readiness deadlines with byte-identical private saves; images were
inspected. The Alpha actor reached death/Revive during loading, so this is not
full gameplay acceptance. The interrupted Steam-offline probe and earlier Native
readiness failures remain retained evidence. Concurrent timings and these single
runs do not establish FPS, loading reliability or complete original native
concurrency/provider/OS/visual/gameplay parity. Owner assets and evidence stay local.

## Six menu caption continuation, 2026-11-09

The original-only author independently finalized the complete six-caption and
compact key-name contracts without implementation, tests, SDK, owner data or
feedback. The implementer read both entire held artifacts, verified their hashes
and copied unchanged prose to docs/specs before implementing. The early caption
phase, per-binding formatting/publication, counted subtree text comparison and
selected constructor controls persistence use only those contracts and owned code.
The detailed controls-screen formatter is a separate bounded path.

The compact dictionary is the complete constructor population; later independent
writers, imported numeric/font/cache providers, shared native component aliases,
arbitrary scheduling and broad native runtime/parity acceptance remain open.
Actual-owner caption/provider-order, compact key/pair and counted-subtree
regressions, all five builds, full required-owner CPU without skips, all Vulkan
groups and SDK independence pass. Short private Native/Alpha frontend and actual
menu callback checks pass with unchanged private saves. These are owned-engine
checks, not original OS-input or visual-parity evidence. Full startup ordering
relative to resource/provider failures also remains open. The separate Native
settled-scene readiness failure is retained. Owner assets and evidence stay local.

## Owned biome land and palette correction, 2026-11-09

The implementer used project-owned composition, region, map and stored-color
paths plus permitted runtime data to correct the user-reported hybrid flooding.
No original/reconstruction/SDK source or new source-side findings were used.
The independent GUI keyboard/editor handoff is held and remains unused here.
Steam biome land floors, native savannah relief and palette selection are explicit
hybrid policies, separate from unchanged native Steam and raw Alpha algorithms.

Actual owned baseline/candidate probes, material/land/map/continuity regressions,
all targets, full required-owner CPU without skips, Vulkan and SDK independence
pass. Previous profiles are separated through compose-v3; no geographic migration
is implied. Native pixels, broader biome/world/gameplay and final provenance
acceptance remain open. Owner files, captures and private evidence are excluded
from public source and runtime payloads.

## Scalar-order and floating-plant follow-up, 2026-11-09

An isolated original-only author independently reread the complete preparation
and final-height owners and their normal-return guard, byte-verified the selected
ranges and produced a new complete held ordered-arithmetic contract. It includes
conditional literal-word calculations, not native runtime observations. The
implementer read the entire artifact, checked its hash and copied it unchanged;
no implementation, tests, prior handoff, owner outputs or feedback reached the
author. Only the qualified small-peak multiplication association changes in the
native height path. The earlier full landform review remains private and unmodified.

The Alpha lily follow-up is owned hybrid policy: floating plants retain the fixed
water surface while grounded plants follow translated solids. It introduces no
new native source interpretation. Actual before/after conditional-height and
plant regressions, unchanged recorded oracles, all targets, required-owner CPU
without skips, all Vulkan groups and SDK independence pass. Provider/order/cache/
thread and broad runtime/provenance limits remain explicit. Steam-disconnected
live checks are pending; owner data and private evidence stay out of publication.

## Help visibility/control continuation, 2026-11-08

The independent native-only author finalized the complete Help state/control
contract without implementation, SDK, tests, owner outputs or prior handoff
access. The implementer read the entire held prose, verified its hash and copied
it unchanged, then used only that contract and owned code. Retained Help
visibility, dispatched F1 repeats, conditional input admission and early
frontend record publication follow that bounded prose. No original or
reconstruction code was inspected or copied by the implementer.

Synthetic message/record/scene and owner-template checks cover the change.
All targets, full required-owner CPU without skips, all Vulkan groups and SDK
independence pass. Only CPU fixtures changed after the GPU run; its production
source/library/executable hashes remain exact and the final full CPU rerun passes.
Live current-binary Help and Alpha transition/profile acceptance await Steam
reconnection and are not claimed. The existing aggregate frontend callback
adapter, native help-menu/text/layout, cached rendering and universal input,
provider/thread/lifecycle parity remain explicit limits. No owner content,
private evidence or binaries enter the public source payload.

## Owned Alpha transition extension, 2026-11-08

The wider rounded landmark transition and fixed-liquid column translation are
user-requested hybrid policies, implemented from CubeVulkan's owned composition
and bridge code. The implementer did not inspect original, decompiled, SDK or
Infinite-Cubes reconstruction source, and did not use the separate new held
landform contract. Prior published classifier handoffs are unchanged provenance
payloads only; they are not new native authority for this extension.

Private before/after procedural cross-sections, actual composed columns and
asset-free water/storage cases validate the bounded change. Raw Alpha formulas,
Steam construction cores and normal Game generation remain intact. The
composition identity advances independently to v2. All build, required-owner CPU,
Vulkan and SDK-independence checks pass; Steam-disconnected live visual and
new-profile acceptance remain explicitly open. No owner data, imagery, binaries
or private memory enter the public source payload.

**Role boundary:** this document includes implementation history below the process
rules. Source-side specification authors must read the standalone
[author policy](CLEANROOM_AUTHOR.md) instead and must not read this combined file.

CubeVulkan is to Cube World what OpenMW is to Morrowind: its own engine, written from a
description of how the game behaves, that reads the content of the owner's own copy. To keep
source provenance separate it follows a clean-room process: the people (or agents) who read the game's code don't write
CubeVulkan's code, and those who write the code don't read the game's code.

## How a feature is ported

1. **Specification.** One agent reads the decompiled game (`decompiled cube world steam`, by
   RVA) and the binary for its constants. It writes a plain-language behavioural spec in
   `docs/specs/`: inputs, outputs, the order of decisions and random draws, and constants, ids
   and record offsets (facts). It contains no code, no transcribed pseudocode, no decompiler
   variable names or copied expression/structure. Alpha specs come from the Alpha decompile
   (`disassembled alpha cube world`). Original routines are cited by RVA.

   The user's 2026-11-07 clarification permits the source-side agent to research
   Infinite-Cubes in isolation. Reconstruction observations are reference leads;
   Steam behavior must be confirmed against original Steam evidence and unsupported
   findings remain unresolved. The source-side agent does not see our engine/exporter,
   SDK, tests, owner outputs, probes or implementation history. Use a fresh context
   without inheriting the implementation chat. Follow the separate
   [author policy](CLEANROOM_AUTHOR.md), including its exposure-stop rule.
2. **Implementation.** A second agent reads only the spec and CubeVulkan's own sources. It
   never opens the decompile, the binary's code or Infinite-Cubes. It writes the code with
   CubeVulkan's structure and names, readable on its own, commented in plain words.
3. **Verification.** The result is checked against the game's own behaviour: its exported
   data (`Extract/`), recordings and captures. It is never checked by comparing against code.

Similarities with the game are expected where behaviour must match (constants, the order of
random draws, data layouts). What must not happen is lifting code or its expression.

## Final provenance and parity audit

The user requested a final review after the remaining implementation work. Keep
source-side and implementation-side role provenance, specification references,
evidence limits and test results traceable during development. Before a final
release, audit those handoffs, identify uncertain or inherited contributions,
verify owner-content/package boundaries and repeat broader Steam gameplay,
visual, save and multiplayer comparisons. Unsubstantiated areas remain open for
independent specification or reimplementation. A later audit does not erase an
actual role exposure; the author policy's exposure-stop rule still applies.

## Shared interface branches, 2026-11-04

The independent native-only author finalized the interface-input-root lifetime
and inventory-thumb capture contracts without implementation, tests, SDK, asset
or capture access. The implementer used those prose contracts and engine sources
only. InterfaceRoot and frontend publication implement shared branch ownership,
visibility and retained template placement. Synthetic root/input-model tests and
owner-data GUI/image checks exercise this bounded change; production callbacks,
capture integration and full native application lifetime remain separate work.
The subsequent HUD-owner change retains the CPU scene across Start Menu and
separates the outgoing renderer's handles from retained interface data. The same
native-only author separately finalized the hover-publication contract without
implementation or owner-data access. A private data-only check of the installed
`gui.plx` found an ordinary Button on the scrollbutton template root. This does
not change the native audit's resource-evidence boundary or establish the class
for modified resources. No original asset or game-derived capture is packaged.
The same native-only author independently traced the root visibility writer,
ordinary pointer admission and named Windows focus/capture message paths.
The implementer used only that prose and engine sources to retain the F4 root
state. Unproved cancellation and indirect writer behavior was not inferred.

## Street-lamp generation records, 2026-11-05

A separate native-only author traced the street-lamp emitter, record copies and
native consumers without implementation or fixture access. The implementer
used the finalized orientation contracts and engine sources. The strict
comparison distinguishes initialized turns from the traced producer's unwritten
input and reports raw owner values locally. Captured game data remains outside
the source and runtime packages.

## Alpha terrain integration

The generator reuses the sister AlphaVulkan project's original GPL clean-room
implementation under its independent prose contracts. The native-only author
separately supplied alpha-terrain-generation-boundaries.md and
alpha-sky-terrain-inputs.md without inspecting engine, tests, exporter or captures.
The same native-only author subsequently supplied alpha-plant-record-consumption.md
with the same separation, resolving ordinary scale/pivot, centered XY, lily fixed Z,
raw flags and reached sway-phase facts. The implementer used that prose for the
record bridge and reused only the sister project's original GPL plant arithmetic.
The vegetation continuation reads immutable columns and keeps its stream separate
from gameplay. Owner registry lookup and construction-aware replacement are project
policies; they do not establish the original Alpha cache population or full-world
parity. No art or game-derived registry output is bundled.
The implementer read only those contracts and owned code. Steam construction
preservation, transition bands, map overview, source-color metadata, isolated
profiles and compatible-engine network agreement are explicit project policies.
They do not establish native full-Alpha random-stream or runtime parity. Owner
models and all game-derived data remain local and excluded from packages.

## Engine-specific appearance extension

The optional dual-tone hair controls are a user-requested engine feature, not a
port of native behavior. Their implementation uses CubeVulkan's existing tinting,
frontend, configuration and profile-note APIs. Initial values may be read from the
owner's INI data; no mod implementation or native source is consulted. Hair models
and palette artwork are loaded from the owner's installation at runtime and stay
out of source and runtime packages. The native character appearance format is
unchanged, and the extra style currently applies only to the local player.

## World object prelude, 2026-11-01

An independent native-only author wrote steam-world-traps.md and
steam-world-death-area.md without inspecting the engine, tests or extractor.
The implementer read only those contracts and existing independent prose plus
CubeVulkan's own code. NativeWorldObjects implements the stated trap scope;
NativeDeadCreatures implements ordinary lifecycle and the non-terrain 0x119/0x11d
areas from the dead-area contract. The native-only author separately traced the
first-burst consumer in section 2.1, without engine implementation access.
Native outgoing alpha remains unspecified; defined engine storage is not treated
as native oracle data. The traced draw's style-specific opacity does not establish
a producer alpha1 or a generic particle rule. Only destructive race0x90's first
visit and shared terrain/scenery mutations remain deferred; its nonzero-clock
lifecycle follows the same independently specified retirement rules.
Coordinate, state/hit/event, float-order and lifecycle checks use synthetic records and
asset-free generated terrain fixtures. The runtime still loads the owner's
content; no captured models, terrain, sounds or game exports enter either package.

The same native-only author separately clarified effect-objects.md sections 2.7.1–2.7.2
and wrote steam-block-change-lifecycle.md, resolving model-list identity and the
physical-write/history/received-queue/mesh-request cut points. The implementer used
only that prose and engine code to move received event consumption after the World
and drawn-position passes. Broader terrain/restoration behavior was not inferred
from the limited helper trace.

## Stored light publication, 2026-11-01

The implementer used steam-block-light-publication.md, corrected creature-upkeep4.7,
worldgen-terrain-zone6 and engine sources only. The independent native-only author
traced storage, phase boundaries and byte3 without implementation access. Target
stored-red writeback, effective queries and upkeep binding follow that prose. The
engine deliberately publishes a safe immutable zone snapshot and serializes writers;
it does not claim native per-record race/timing identity. Synthetic roof/sea inputs
exercise the actual CPU mesh path in both styles. No owner block/model bytes enter
source or runtime packages. The separate main-thread CRT prose remains a later
implementation contract, with unknown initial numeric state explicitly preserved.

## Shared body particles and death candidates, 2026-11-01

The implementer read steam-body-voxel-particles.md, the linked death-candidate and
local-hit contracts, creature-pose.md's prepared bindings/math and engine code only.
The independent native-only author wrote the new body contract without implementation
or capture access. Cached bindings, frame-local candidate delivery and the shared
body/burst producer/update/draw now use that bounded prose. No native code, original
instructions or third-party reconstruction was read or copied. Raw source CUBs and
the drawn cube remain owner data; asset-free RGB arrays/triangles and existing relative
numeric samples provide tests. The same author corrected channel0's auxiliary source
to byte3 and wrote steam-block-light-publication.md, plus the remaining style-consumer
supplement. Publication and other-style integration follow separately; no native
concurrency or CRT seed equality is inferred from these behavioral contracts.

## Ordinary burst particle consumer, 2026-11-01

NativeParticles, the main-loop adapter and the dedicated WORLD particle pass were
written from steam-particle-bursts.md, creature-upkeep.md section4.7 and engine code
only. The independent native-only author separately resolved shared expansion for
all style tags and cube.cub's owner-file identity/catalog/data binding. No native
implementation, original instructions, reconstructed code or model bytes were read
by the implementer. Synthetic CPU records and asset-free GPU triangles check the
bounded1/2/5 rules. The World random provider remains the engine's legacy provider;
its draw ordering is not asserted to close native CRT state/seed parity. Body
particles and later-style consumers were implemented in subsequent checkpoints below;
unknown style numbers and native CRT-state equality remain outside that result.

## Destructive terrain and model cleanup, 2026-11-01

The same independent native-only author supplied the complete bounded destructive
race0x90 producer and ordinary-model cleanup contracts in steam-world-death-area.md
and effect-objects.md. The implementer read those prose contracts and engine/tests
only. NativeDeadCreatures now emits the ordered terrain work; ExtractedWorld owns
immutable model vectors and semantic deletion, with an engine renderer refresh that
is deliberately separate from native block notices and auxiliary lists. Synthetic
producer, actual facade, material and loaded-model tests exercise this scope.
The author also wrote steam-particle-bursts.md, including file-backed cube.cub
identity and resource data facts, without accessing engine/tests/extractor sources.
Its bounded particle implementation follows separately. Owner content stays local.

## Terrain mutation and cached inputs, 2026-11-01

An independent native-only author supplied steam-full-block-setter.md and clarified
netcode-roles.md's empty-frame/list-major consumer behavior without inspecting the
engine, its tests or extractor. The implementer read only those contracts and the
existing terrain/lifecycle prose plus engine sources. Shared setter logic, physical
edit scopes, history ownership and mesh-request selection use that written scope.
CWM1 is an engine-defined owner-data format: the extractor copies the six documented
column material fields and world sand offsets; no artwork or captures are packaged.
Synthetic files and loopback packets test the codec and behavior. No fresh original-
game CWM1 capture or full model/particle/rendering parity is claimed by those checks.

## Capture-log cleanup, 2026-10-31

Routine capture inspection exposed raw hexadecimal function bytes from an old
extractor diagnostic. They were not decoded, analyzed or used to implement engine
behavior. The extractor's startup function scan/export and prologue-byte error
printing have been removed; asset and behavioral-data capture are unchanged.
Historical raw diagnostic files remain local and outside source/runtime packages.
Subsequent routine log inspection filters to data and provenance messages.

## Audit, 2026-10-27

- Comments and docs no longer use names that a third-party reconstruction invented for the
  game's methods, members and constants (about 300 lines). They now describe the behaviour in
  plain words, citing RVAs where known.
- Class names that the game's own binary carries (its RTTI: `Game`, `World`, `Creature`,
  `Zone`, `MapOverlayWidget`, `InventoryWidget`, `ScalableFont`, ...) and outside APIs remain.
- Code identifiers borrowed from the reconstruction were renamed (`mapLabelScale`,
  `mapViewCentre`, `boughtFlights`, `boughtFromVendor`, `nameHash`, `plx::keyOf`,
  `setTextColour`). The game's own item keys (`WaterFlask`, ...) are its data and stay.

### To redo clean-room

These were written while reading the Infinite-Cubes reconstruction. They are to be specified
from the decompile and reimplemented from the spec, then their notes here removed:

(None left: the last, `src/RaceKeys.inc`, is under "Done so far" below.)

Everything ported from here on follows the process above.

### Also to redo: modules transcribed straight from the decompile

These were written by one hand while reading the decompile, often close to its shape (raw
float bit patterns, offset-named helpers). They don't use the third-party reconstruction, but
they aren't clean-room either, and many aren't readable the way the project wants. Each gets
a spec from the decompile and a fresh implementation from that spec, verified against the
same oracles that check them today:
- the recorded creature updates, AI ticks, hits, ability calls and effect steps;
- the oracle zones extracted from the game.

(None left: the last two, `NativePlayer`'s `transformed()` and `NativeMenu`, are below.)

Done so far with the full process:
- Resident zones follow the independent unsigned pointer-order contract through
  retained engine Zone tokens. Per-zone restoration/refresh/admission/effect phases
  now remain together. Tests use synthetic unsigned identities and owner-data token
  lifetime checks; this is not a claim to reproduce a reference allocator's addresses
  or an unrecorded incoming CRT state.

- The resident kind9 request/producer follows the separate resident-effects contract,
  including distinct full-word/low-byte gates, day-derived fixed math and independent
  live/output lifetimes. Tests preserve unspecified bytes instead of assuming a
  native damage value. Existing no-contact effect updating is reused, with no new
  artwork or guessed rendering consumer.

- Resident collection/admission and final flag reconciliation use the independent
  authoritative-residents contract. Typed collections preserve membership and region
  snapshots while resolving current positions across engine storage moves. Resident
  field widths and coordinate conversions follow the same contract and shared numeric
  rules. Asset-free checks plus owner-data duplicate/respawn fixtures exercise these
  bounded consumers; RNG lifetime and complete zone/prelude behavior remain open.

- Host/reset scheduling follows the independent daily-reset supplement and the AI
  contract's World-order scans. Actor views are rebuilt after deletion before effects;
  retained players, live zones and outgoing events keep their specified lifetimes.
  Synthetic actual-facade checks cover zero/full-interval host updates, persistent
  notices, compacted actor storage and reset-before-effects. Configured respawning
  remains an explicit engine policy; these changes do not establish full native parity.

- Ordered World interpolation and linked placement follow the separate native-only
  pass contracts. The implementation preserves fixed arithmetic, raw/drawn field
  differences, row-specific side effects and live relationship reads. Shared
  angle math uses the independently derived threshold regression. Packet timing
  follows the specified receiver instead of the former engine prediction clock.
  Synthetic references, actual-facade ordering and CPU/network tests cover these
  bounded changes; full producer/lifecycle and native-peer validation remain.

- Fixed gameplay camera positioning follows the independent camera §§5–6 and
  terrain-ray contract: typed physical/posture/mount snapshots, low64 arithmetic,
  ordered probes, distinct smoothing, eight guards and sequential shortening.
  The renderer consumes an independently constructed inverse rotation for
  coincident/pole views. Derived CPU references and actual Vulkan images verify
  these bounded consumers; full angle/reset/render-origin integration remains.

- Shared GUI routing follows `steam-gui-pointer-capture-routing.md`: cached and
  previous pointers, held bits, capture-or-root selection, independent virtual/
  registered gates, focus/auxiliary bookkeeping, nested release moves and ordered
  global visitors. Synthetic traces cover owner changes and reentrant movement.
  Widget initialization/copy, dirty-byte identity, Button groups and owned sequence
  starts follow the independently traced scene lifecycle. Layout preserves Widget
  identity and resize-flag propagation, with separate own hooks and disabled-subtree
  admission. The dispatcher is a tested core; full live binding, other dynamic-class
  hooks and native frame/capture comparisons remain integration work.
- Map history, fixed map controls and region borders follow
  `steam-map-region-history.md` and `steam-map-border-and-information-hud.md`.
  Personal ordered history and borrowed-entry lifetime are separate from discovery
  sets; native resources provide arrow and border geometry. Fixed-coordinate pan,
  wheel recenter, quantized easing and the distinct zoom limits use the independent
  arithmetic contract. Border descriptors retain generated points, native float
  clock/opacity and separate depth/color passes. The independent retained-item/
  pass-family supplement confirms normal-only map queues and inherited world
  depth. Actual Vulkan checks distinguish both winding directions and reproduce
  the former private-clear occlusion bug before verifying its correction.
  Hidden-root dispatch and standalone
  hit queries remain distinct. Physical-wheel Controls/Multiplayer fallbacks use
  `steam-physical-wheel-panel-fallbacks.md` for priority, signed/unsigned row
  arithmetic and separate thumb refreshes; they add no controller-wheel eligibility.
  The movement-close batch follows region-history §7.4.1, preserving Options,
  Controls, panel data and unrelated answer fields. Wider input/GUI phase gaps are
  recorded in the roadmap.
- The retained item image follows `steam-retained-item-and-map-pass-family.md`:
  each admitted inventory/shop/crafting callback samples the canonical transient
  player stack after its children and text. Pointer coordinates, literal size,
  signed-clock rotation and shared helper state follow the independent contract.
  No positive cursor-origin action or new save field is manufactured. Controls'
  local-edit retention and separate startup defaults follow the wheel supplement's
  independently traced lifecycle. Its finite drag arithmetic is implemented from
  the supplied query outputs; actual owner geometry/capture routing remains a
  separate integration boundary, rather than a guessed rectangle.
- Procedural artifact resources follow `steam-procedural-item-resources.md`:
  independent generators implement the seven named families and default sphere,
  preserving full ID/modifier keys, allocation dimensions and native color mode.
  The extractor copies completed native resources on the game thread to a separate
  owner-data format; captured RGB and game data stay outside publication. GUI and
  dropped-item consumers prefer those owner records. Synthetic codec, cache,
  geometry and rendering checks pass. A separate 332-key owner capture, taken with
  the owner's mods active, matches independent generation across all RGB bytes and
  semantic metadata. The private
  captures are excluded from publication; synthetic fixtures do not supply that
  native reference evidence.
- Map draw phases follow `steam-map-rendering-phases.md`: current 3D map work
  precedes GUI, and flat markers collect at the admitted Overlay callback rather
  than using the preceding camera's screen positions. F4 suppresses 3D and GUI
  map draws; retained Control message state selects half radius independently
  of full-map mode. Pointer text samples retained cursor data earlier, with its
  distinct float32 distance norm. Pixel-centre, owner callback-order and hidden
  collection regressions cover these changes. Broader input/camera phase fidelity
  and hovered-land highlighting remain separate work.
- Map information follows `steam-preview-map-location-lore.md`: complete semantic
  item payloads stay live in the overlay member, while flight/lore coordinates
  are copied before the later draw. Reached minimap/drag draws reset outputs;
  hidden ancestors retain them. Flight quotes use current physical coordinates,
  known points and the live target flag. Static lore keeps fixed headers, styled
  words, native punctuation gaps and page spacing without activating page effects.
  Owner-tree tests cover draw order, previous backgrounds and late 115/145/124/103
  heights. Full GUI collection/capture and earlier inventory sampling remain open.
- Ground/Ring information bindings follow `steam-preview-source-lifetimes.md`,
  `steam-ring-selector-rendering.md` and preview sections4/5/7. Ring sampling keeps
  the earlier canonical key; draw resolves current ground storage without loading
  zones or rechecking source validity. Ground metadata remains independent when
  Ring supplies the item; only the adapter's temporary item value is copied, never
  retained as an original-source snapshot. Native bottom placement uses the earlier
  stored size. Packet-backed post-pickup and owner-geometry cases cover these paths.
- Live information reads, hidden-ancestor draw admission and the bounded
  game-UI/MapOverlay/Preview order follow `steam-preview-hidden-interface-order.md`.
  Information formatting occurs after earlier widget callbacks instead of freezing
  their mutable member beforehand. Owner-tree tests exercise palette mutation,
  hidden-state retention and resumed reads. A const ground-key resolver reads only
  existing zone state and index bounds; it does not invoke lazy generation or add
  source-validity filters. Complete source sampling and key/focus timing remain open.
- Item-detail text and equipped comparisons follow
  `steam-voxel-widget-information-preview.md` sections4/7/8. A separate engine text
  plan preserves row admission, owner dictionary callbacks, original equipped
  records, artifact/rank/effect literals, exact float rounding and separately
  rounded deltas. Glyph output reports its actual pen for inline suffix placement;
  comparison fonts retain unscaled outline widths in the shared atlas. Synthetic
  record tests and owner glyph/lifecycle tests exercise these bounded consumers.
  Native selector timing and live map/ground/Ring bindings remain separate work.
- Native information-panel size lifetime follows
  `steam-voxel-widget-information-preview.md` section7: earlier selection uses
  stored geometry, root/children precede the own callback, and later text-derived
  resizes do not redraw that background. Owner-tree regressions cover initial,
  hidden, growth and shrink transitions. `steam-preview-source-lifetimes.md`
  independently clarifies that Ring can retain a distinct ground-origin record;
  source selection preserves that flag without inventing a copied stack or a
  quantity formatter. Full formatter rows/deltas and live source wiring remain open.
- Native customization root, draw plan and event continuation follow
  `steam-voxel-widget-presentation-input.md`,
  `steam-weapon-customization-edit-view.md` and
  `steam-voxel-widget-down-event-routing.md`. The implementation reads only
  those independent descriptions and owner runtime resources. Full-coordinate
  pickup/removal and byte placement are tested separately; no native code or
  geometry is included in either package. Palette information-source precedence
  follows `steam-voxel-widget-information-preview.md`, preserving the original
  record and using a separate class-adapted copy for text/image. Full cached GUI
  lifecycle, comparison/map/ground bindings and controller aim remain separate.
- Inventory wheel target priority and per-message sign normalization follow
  `steam-inventory-refresh-events.md` section4.2. Input retains the event pointer
  and reuses the engine's existing owner-tree hit test; no game input code is read.
  Synthetic cases cover overlap priority, no-hit and signed fractional/large input.
  Cached range/thumb state and the remaining refresh lifecycle stay separate.
- Alpha static-scenery pass ordering remains based on
  `alpha-far-static-ordering.md`. Its new normalized-target/fade clarification
  confirms that CPU opacity must not gain a new endpoint clamp or discard.
  The shared-water extension now blends its deferred scenery over the sky before
  its refraction snapshot; the native-water path and retained depth are preserved.
  Synthetic Vulkan controls reproduce the prior shared-water error and its fix.
- Native edit projection/ray/voxel traversal and raw WORLD draw support from
  `steam-weapon-customization-edit-view.md`. Full allocation, negative aspect,
  stored lens constants, signed cell/byte semantics and raw shader state are
  independently implemented. Synthetic CPU and Vulkan cases verify these bounded
  paths; the root/event integration is described above. No owner geometry is shipped.
- Canonical customization palette production from
  `steam-weapon-customization-palette.md`: selected-material semantic equality,
  four field-only spirit totals, signed32 wrapping and retained click input.
  One engine-owned pass accumulates the five totals without copying game code.
  Editor presentation is described above; no artwork or owner asset is added.
- Recipe result/ingredient and information-preview WORLD calls from
  `steam-gui-item-icons.md` §§11/12/16. Class adaptation affects the information
  icon's copied record, preserving the source. Resource-gated recipe rotation,
  callback order, strict result hover, ingredient rounding/navigation and GUI
  step/total precision follow the independent facts. Original records also reach
  the existing naming helper without discarding wrapped item or spirit data.
  Synthetic math/state and local owner-tree checks cover these bounded paths.
- Inventory, shop and recipe grid WORLD item calls from `steam-gui-item-icons.md`
  §9 and `steam-inventory-grid-layout.md`. Native inventory views retain physical
  slots, including holes, and original records; region drawing stops independently
  of whole-tab scrollbar counting. Cell rounding, hover scaling, names and colors
  follow the independent facts. Only locally loaded owner geometry reaches the
  renderer; no original asset is added to source or runtime packages. Persistent
  widget refresh/callback timing and broader original-game comparisons remain open.
- Ring Menu input, canonical candidate identity, selected Use/Quick Item, combined UI
  sound edges and queued screen-overlay rendering, from `steam-inventory-slot-lifecycle.md`
  §9 and `steam-ring-selector-rendering.md`. Original item records, owner selector/fonts
  and runtime CUB resources remain external. Synthetic state/math, local owner-tree checks
  and actual Vulkan image cases cover the bounded implementation; original-game/hardware
  comparisons remain open. Earlier use-prompt inventory terminology was corrected by the
  separate spec author before the implementation changed that fallback.
- equipment SpriteWidget item calls from `steam-gui-item-icons.md` §10. Original
  equipment records and owner CUB geometry reach the shared WORLD helper inside
  each copied widget's draw callback; legacy record-less previews remain separate.
  Synthetic placement and local owner-tree checks cover sizing, callback order,
  attributes, visibility and missing resources. No owner assets are distributed.
- character-menu Button initialization/hover animation, raw custom-widget binding transfer,
  physical-pixel control placement and filtered drag/inertia, from `frontend-characters.md`,
  `plasma-format-and-scene.md` and `steam-character-list-input-and-frame-init.md`.
  Instance animation does not modify owner documents. Synthetic state/clock/geometry checks
  and local owner-data comparisons cover these bounded paths; GUI layers and broader visual
  comparisons remain open. No original game asset is part of the publication package.
- shrine-choice map opening, borrowed source lifetime, render-produced target and left-release
  acceptance, from `steam-item-map-selection.md` and `map-markers.md`. The existing native
  return ability keeps its effect boundary and signed32 cell arithmetic. World elapsed time,
  GUI bob time and calendar time remain distinct; no source-item depletion was invented.
- character-selection control placement using effective owner widget frames, preserving
  template transforms and picking through the copied shapes, from `frontend-characters.md`
  §2.3.1. Size requests are not substituted for measured geometry. Owner-data checks cover
  GUI scales 1 and 1.5; remaining frontend animation/rendering gaps stay explicit.
- identification owner-tree layout, independent footer mouse/navigation adapters,
  caption/details/cost typography and release/Goodbye behavior, from
  `steam-identification-ui.md` and `steam-controller-handoff.md`. The implementation
  clones owner data at runtime; cached bitmap icon previews remain explicitly
  separate from the unfinished shared3D GUI item renderer.
- canonical native-save gameplay inventory/equipment/counters, direct item actions,
  live identification indices and shop transactions, from
  `steam-inventory-slot-lifecycle.md`, `steam-identification-ui.md`, `loot-crafting.md`
  and `loot-shops.md`. Independent tests cover operation order, semantic assignment,
  signed counts, distinct removal callers, refresh callbacks and raw save retention.
  No native code or owner asset is included; auxiliary UI presentation remains open.
- received-feat projection, lifetime and physical-pixel owner-font display, from
  `steam-chat-feat-presentation.md` sections4.1-4.4. Synthetic geometry/timing and
  live owner-font captures check this display without inferring awards. Scene/sky
  clock narrowing and stock water-opacity lifecycle follow their independent specs.
- the stackable-type mask correction, from `steam-inventory-slot-lifecycle.md`:
  type22 merges, type18 stays separate, and an equal stack wins over a prior free
  slot. These helpers do not replace the remaining canonical player-slot work.
- chat history/editor, entered commands and friend-game UI eligibility, from
  `steam-chat-feat-presentation.md`. Owner fonts/templates stay external; synthetic
  editor/wrap/queue/command records and a live owner-font frame check the bounded
  implementation. No new feat producer/award or ownership bypass was added.
- adaptive mesh-readiness ranges, native face-cost metadata and key-only tie ordering,
  from `steam-scene-lighting.md` §§2.6/6 and `steam-controller-handoff.md` §4. Pending
  metadata is independently published from worker jobs without asserting new geometry
  is active. Owner cache costs and CPU/Vulkan regressions verify the supported paths.
- saved/default recipe lifecycle and full crafting-result item retention, from
  `steam-dialogue-item-actions.md` §4 and `zone-object-interaction.md` §0.7.
  Synthetic bridge/default/equality cases and the full owner-data suite verify
  preserved records; no new recipe grant or numeric award producer is inferred.
- per-speaker NPC speech/emote lifecycle, literal emote-word pages, cached projection,
  owner-template placement, world emote transforms/depth/culling and general lore/
  receipt routing, from `steam-npc-speech-rendering.md` and
  `steam-dialogue-item-actions.md`. Tests use synthetic state, owner templates and
  Vulkan captures; no native code or replacement assets are included.
- live static-object clocks/pose synchronization, generated light shafts, native
  mesh-time model sunlight/glow/fade queues and castle unsigned-choice arithmetic,
  from `worldgen-parity-gaps.md` §§2.1,4.4,9–10. Verification separates fresh
  generation, partial refresh traces and mesh-time light inputs. No original
  source code or replacement game assets are included.
- static-object visual sizing, captured door/window transforms and crop/furnishing
  model metadata, from `worldgen-parity-gaps.md`. Owner voxel dimensions are read
  from local model headers; no replacement assets or reference source code are included.
- controller GUI events, ranking and repeat timing, inventory region/tab state,
  navigation registration, shape picking and transformed bounds, from the independent
  `remaining-runtime-gaps.md` sections 4.1–4.17. Implementations and synthetic checks
  use only that specification and the engine's sources; owner assets remain external.
- the special skill and interaction kinds 1/11/12/13;
- the conversation builder (`src/TalkGen.*`);
- map markers, lore and artifacts (`src/MapMarks.*`, `src/Lore.*`);
- the .plx reader and the Plasma scene, shapes and text (`src/Plx.*`, `src/Plasma*`), with
  FreeType for glyphs.
- the use key: targets, prompts and actions (`src/UsePrompt.*`).
- the creature upkeep (`src/NativeUpkeep.cpp`), all 10,800 recorded calls matching.
- the hit routine (`src/NativeHit.cpp`), all 600 recorded hits matching.
- drops, rewards and item generation (`src/NativeDrop.*`), the drop, item and gear oracles matching.
- the effect objects (`src/NativeEffects.cpp`), 697/700 steps as before; the 3 failures are a recorder gap.
- the stat rules (`src/NativeRules.cpp`), playerstats and rulesoracle matching.
- the creature physics (`src/NativeUpdate.cpp`), all 8822 recorded updates matching.
- the creature AI (`src/NativeAI.cpp`), 1776/1800 recorded ticks; the rest are recording artefacts.
- the creature abilities (`src/NativeAbility.cpp`), the ability oracle as before. (Slip on record: the implementer saw the old file's first ~30 lines, includes and a byte accessor, plus one asinf line, before deleting it; the new code was written from the specs.)
- the creature pose (`src/NativePose.cpp`, `NativePoseTables.inc`), every recorded pose bit for bit.
- the player control (`src/NativeControl.cpp`), the control and attack oracles matching.
- the zone-object interactions, kinds 2, 3, 5, 7, 8 and 10, and the open/shut routine (`src/NativeInteract.cpp`), from unit tests (no recording exists).
- the item models (`src/NativeItems.cpp`), all 117,000 recorded model slots matching.
- the world generator (`src/worldgen/`, replacing `GameGen.cpp`, `GameGenMap.inc` and
  `GameCreatures.cpp`, deleted unread): terrain, structures, creatures, region roles, looks and
  gear, every caller moved to it. The oracle zones match draw for draw (gen_seed_123, gen2, genb:
  every .rtrace whole), with every spawn point but those whose race comes from the three picker
  lists not yet extracted, and genr's shopper that reads past the game's list.
- shops, identifying and crafting (`src/GameShop.*`, `src/GameCraft.*`, deleted unread) and ground items and
  pick-up (`src/GroundItems.*`, replacing CubeVulkan's invented coin flight), from `loot-shops.md`,
  `loot-crafting.md` and `loot-pickup.md`; checked by unit tests of the seeds, prices, the craft timing,
  the hop and pick-up timing (no shop, crafting or pick-up recording exists yet).
- the world map's region model and its markers (`src/worldgen/MapModel.*`, 0x2eb7b0), written
  fresh from `worldmap-model.md` in place of CubeVulkan's own stand-in; checked by unit tests of
  what the spec states (sizes, units, marker lists and their order around a town), since no
  recording of the game's map exists yet (`EngineTests mapdump` prints what one would be
  compared with).
- the creature looks (`NativeLooks.cpp`, now `worldgen/Looks.*`) and the creature record, residents
  and pets (`NativeCreature.cpp`, now `worldgen/Residents.*`), deleted unread; looks.bin 7704/7704.
- the class-adapted treasure item, `transformed()` in `src/NativePlayer.cpp` (0x10a720), its old body deleted
  unread and written from `player-item-adaptation.md`; checked by unit tests of every rule over 3000 modifiers
  and each class (no recording of the routine exists yet).
- the race-number-to-dictionary-key table (`src/RaceKeys.inc`), deleted and written again from
  `race-keys.md`, whose facts come from the game's own table (0x1401510f0); checked by unit tests of the
  table and that every key names an entry of the install's `dict_en.xml` (all but Bullterrier and Vase,
  which the game's dictionary lacks). (Slip on record: the implementer saw the old file's first five
  lines, its two-line header and three lines of entries, before deleting it.)
- the game's save files (`src/NativeSaves.*`, from `native-saves.md`) and the front screens (`src/Frontend*.cpp`,
  `src/HudFrontend.cpp`, from `frontend-title.md`, `frontend-characters.md`, `frontend-options.md`).
  `src/NativeMenu.*` (the CWM1 replay and its third-party `shade()`) is retired with `tools/MenuProbe.cpp`,
  `tools/MenuCapture.cpp` and `tools/ConvertMenuCapture.py`: the title is now the ordinary world. Nothing of
  `shade()` was kept. The former logo lighting stand-in is replaced by the independently specified
  queued WORLD path described below and in `docs/menu.md`.


## Remaining ordinary particle consumers, 2026-11-01

The implementer read only steam-particle-style-consumers.md, its linked existing
contracts and engine sources. The independent native-only author established the
finite style0/3/4/6/7/8 consumers and note.cub catalog binding without implementation
access. Shared motion/age/cap, descending contact retirement, owner note geometry,
full-header lifetime scaling and explicit per-draw shininess follow that prose.
The engine removes a retiring record once after all visits; undefined native
duplicate-erasure recovery is not reproduced or claimed. No new random draws,
model bytes, native instructions or reconstruction sources enter this change.
Other producers, unknown style numbers, malformed input and full native CRT/pixel
acceptance remain open. Synthetic motion and GPU material/state checks cover the
bounded paths; owner geometry stays local at runtime.


## Local World terrain ownership, 2026-11-01

The implementer read the independent steam-local-world-zone-ownership.md allocation,
column lifetime, local byte-delivery and received-record contracts, plus engine
sources only. The authorized native-only author inspected native ownership and the
actual local connection without engine/test/capture access. ExtractedWorld forks
source settings while loaded zones, edit history, lighting and prefetch lifetimes
remain per reader. Combined sessions route host gameplay to their owned reader;
client consumers retain their reader and apply copied notice records. Dedicated
hosts reuse their one supplied reader. Shared source configuration does not assert
that every catalog cache or generator helper is immutable. Full native host light
scheduling and original-peer/image acceptance remain separate audits; no raw-light
reset or guessed native solve is introduced by this ownership change. Synthetic
roof/sea records cover both styles without adding owner data to either package.


## Shared CRT transition and scene sparks, 2026-11-01

The implementer read steam-main-crt-stream.md, steam-host-crt-stream.md and the
existing body/burst/pose-item contracts, then engine sources only. NativeCrt adds
the established unsigned32 transition and15-bit return; it selects no seed or
process/thread state. The engine's logical main and host execution roles retain
separate states, with body/burst/scene-pose draws sharing their relevant World
provider. AI retains its separate state and prior transition. The legacy numeric
initial choices remain explicitly engine choices; native entry/first-frame values
are not inferred from seed123 or the arithmetic initialization reference. No system
CRT implementation was inspected or copied. Known subphase/wrap values and actual
body/burst/pose-spark consumer continuity are tested with synthetic data. Complete
reseed, thread/task assignment, pose admission/visitation and whole-frame consumer
closure remain separate work; no startup or rendered-particle parity is asserted.


## Current-scene actor pose admission, 2026-11-01

The independent native-only author wrote steam-world-pose-invocation.md and corrected
creature-pose12.4 from the upstream height-minimum instructions. The implementer read
that prose, the linked existing frame/camera/NPC-speech contracts and engine sources
only. CreatureScene factors the engine's existing speech actor parser/sphere math
into shared data; normal flags/health/visibility now precede pose/cache/random work.
Registration ordinals preserve the already implemented World membership ordering.
A shared prepared CPU scene supplies exact fixed-camera distance/origin math and
current planes/ranges, then is reused by the renderer rather than advanced twice.
Earlier Game queries retain the prior scene until the new preparation boundary.
Gameplay and title-underlay callers use it; widget/direct preview calls retain their
separate policy. The title underlay also gains the ordinary drawn/link phase, whose
front-panel admission is established by gameplay-frame-order7. Headless real-draw,
synthetic sphere/order and Vulkan scene-reuse cases introduce no owner data.
Native startup/factory ordering, all preview consumers, full shader/scenery origin
precision and cached pose/body coordinate-lifetime comparisons remain separate;
the current engine pose matrices still use its source-home coordinate frame.


## Separate eye, render origin and cached-body lifetime, 2026-11-01

The independent native-only author corrected world-pose-invocation2 and wrote
steam-body-particle-coordinate-lifetime.md. The implementer read that prose, the
existing camera, particle and light-center contracts, and engine/tests only. Final
fixed eye remains the distance anchor; exact negative look-at XY supplies plane and
normal-pose origins. Native cached part matrices are retained in that frame, with
engine mesh conversion performed only on submitted draws. Part light-zone selection
uses the native matrix/header center before that conversion. Body births consume a
separate zero-initialized Game origin, updated at camera completion after the body
phase; they neither restore cached actor origins nor repair stale matrices. The
previous checkpoint's source-home pose origin is replaced by this implementation.
Synthetic large-coordinate, real headless pose-cache and fragment-birth cases use
no owner assets. Full shader/scenery-origin precision, alternate preview/reset
paths and native visual acceptance remain separate work. No native code or system
CRT implementation was read by the implementer; no owner content enters packages.


## Exact CPU scene consumers and runtime-state audit, 2026-11-01

The implementer used the existing scene-lighting1/1.2/1.3, worldgen-parity-gaps9.6/9.7,
gameplay-camera7.2, NPC speech cache and independent coordinate-lifetime prose plus
engine/tests only. A shared View now admits fixed spheres and cached boxes; the
prepared native projection also supplies retained speech at its existing publication
boundary. Integer relative negative-eye differences precede float conversion.
Source-home XY metadata converts to absolute once, without a second Z shift;
light binding, fade and fixed beam-distance adapters retain their documented roles.
Synthetic large-coordinate/bounds/beam cases and asset-free Vulkan triangles cover
these paths. Full GPU-origin precision and original-game visual parity remain open.

The independent native-only author additionally inspected a version/hash-qualified
runtime binary for steam-crt-runtime-initialization.md; the implementer read only
that prose and its main/host cross-links. Its fresh-context default1 and bounded
startup/reset facts do not imply a universal first-frame or worker-entry state.
No runtime code was read or copied by the implementer, and this checkpoint does not
change the engine's random initialization. Earlier notes about not inspecting a CRT
implementation describe their earlier checkpoints; this later author-only audit
preserves the same prose/implementation separation. No game assets enter packages.


## Saved-character card preview, 2026-11-01

The independent native-only author supplied steam-character-card-preview.md, including
its scalar-opacity classification and five descriptor inputs, without inspecting engine,
tests, extractor, owner assets or captures. The implementer read that contract, linked
pose/GUI/clock/world-control prose and engine sources only. CharacterCard owns pure
record updates, temporary drawn-origin restoration and the documented consumed projection.
The frontend retains one runtime pose record per cached saved character, reuses native
model headers and owner-loaded voxel meshes, and supplies the callback at the live
widget's own draw position. Resource grouping uses the engine's unsigned mesh identities;
portable native allocation-address identity is outside the tested parity claim.

The existing ordered GUI renderer supplies fixed card lighting/white sky, material,
separate opacity and effect inputs, opposite winding and inherited depth. Shared pose
output fields now name their independently established shader meanings. Recordings remain
a compatibility path when the native appearance backend is unavailable. The optional
user-requested dual-tone extension reuses its existing runtime owner-hair tinting.
No model art, recordings, screenshots or native records enter either package.

Synthetic record and geometry tests cover dual clocks, cached-state mutation, zero-health
admission, temporary XYZ restore, signed M00 and the effective50-degree screen projection.
Four asset-free Vulkan cases cover material alpha, opposite winding, text ordering and
retained depth. Broader owner-cache composition, full frontend pixel acceptance and the
other roadmap gates remain open. The same independent author additionally resolved
saved playable appearance CRT consumption; the implementer read the full new supplement
and its corrected frontend/initialization links. No startup stream state is inferred.

## Static-object block coverage, 2026-11-01

The implementer read steam-static-object-block-flags.md in full, its independently
verified host-publication addendum, the two corrected contents/setter links and engine
sources only. The existing native-only author traced marking, effective reads, fixed
coordinate narrowing, persistent tracking, overlap suppression and reached generation,
restoration, received-list and host/Game publication boundaries without implementation,
test, extractor, reconstruction, owner-data or game access.

Generator and loaded-zone paths share an engine-owned marking pass. Immutable terrain
snapshots retain their own tracking keys; generated keys survive subsequent stored-state
replay. Received lists mark immediately and pending publication marks after copying the
current object records. The host keeps its established post-drawn publication boundary.
Neither marking path invents a history notice or mesh request. Empty stored columns now
expose effective sea water rather than treating their unused base as buried terrain.

Synthetic tests exercise retained keys, overlap flags, narrowing, effective water,
column extension and pending-set clearing. A new owner-only comparison includes full
creature/object generation, applying daily refresh only to its corresponding fixture
set. All26,418,885 block comparisons and3,231,744 column comparisons match across789
fresh/refreshed zone fixtures. An initial expanded comparison incorrectly refreshed
the fresh fixture set too; that test phase error was corrected rather than changing
the generator to fit it. The older structure-only comparison remains labelled as such.
This does not close global region/theme coverage,
all pending-set producers, adjacent-zone object ownership or pixel acceptance. No owner
blocks, records, captures or assets are included in source or runtime packages.

## Separate WORLD matrix uploads, 2026-11-01

The implementer read steam-world-gpu-coordinates.md in full, its world-pose link,
the relevant existing shader/camera contracts and engine sources only. The existing
native-only author established the fixed-to-float boundaries, local vertex spaces
and independent float WORLD/view/projection uploads without implementation, tests,
extractor, SDK, reconstruction, owner-data or game access.

The engine's column-vector convention retains separate float projection and view
uniforms. Ordinary WORLD vertices now transform their model-space point through
view and then projection on the GPU; immediate GUI draws supply their own lens.
Other rendering paths retain their existing combined matrices. Full CPU/owner-data,
52 shader, 2 camera, 24 GUI, 6 map, shared-water and Alpha-scenery checks pass.
This closes the separate-lens upload step, not the complete coordinate contract:
native fixed-origin translations and terrain band-local construction remain work
in progress. No owner assets or captured data enter either distribution package.

## Posed WORLD matrices at the fixed origin, 2026-11-01

The implementer used the independent ordinary WORLD GPU contract, camera/pose
contracts, scene-lighting§3.2's per-zone GPU light positions and engine sources only.
Spawned posed actors retain the already prepared origin-relative float matrix before
source-home conversion. The ordinary Steam draw submits that matrix with the retained
native view/lens; the source-home matrix remains available to other rendering passes.
Point tables now add the matching fixed XY origin before float conversion, and legacy
geometry supplies lighting positions in that same frame. Shader shadow coordinates
retain the source-home convention. No extra matrix exceeds the guaranteed128-byte
Vulkan push-constant limit; the additional camera data uses the existing frame buffer.

Asset-free Vulkan regressions verify that a quarter-block offset at world block2^24
remains distinct from an integral offset and matches the near-zero coverage reference.
A saturated nearby point light reproduces its independent RGB reference at that offset.
The full CPU/owner suite and55 shader,2 camera,24 GUI,6 map,shared-water and Alpha-scenery
checks pass. Terrain bands and static/environment placement still require integration;
the independent band's full supplement has been read without importing native code.
No owner model, terrain, record or capture bytes enter source or runtime packages.

## Local ordinary terrain bands, 2026-11-01

The implementer read steam-terrain-mesh-bands.md in full and its GPU-coordinate link,
the existing bounds/vertex contracts and engine sources only. The native-only author
established the relative anchor, signed division, source-block ownership, corner order,
empty-band omission and reached category order without implementation or capture access.
Terrain now selects a band from the retained lowest exposed block and subtracts its
integer origin before converting corners to floats. Steam terrain visits source XYZ
in the specified order; other mesh families retain their own traversal. The renderer
submits fixed zone/band anchors and uses the established native zone gate. Backend
opaque/lava resources remain engine-owned; their local geometry and category order
follow the independent contract. Zone remesh, unload and destruction retire those resources.

Synthetic CWZ fixtures verify negative/boundary arithmetic, local top250 ownership,
sparse band bases37/287/787, physical colored-corner and face-count agreement with the
aggregate path, and unchanged adaptive triangle cost. A beyond2^24 source-corner case
verifies integer-before-float packing, and a Vulkan fixed-anchor case preserves the
quarter-block reference. Full CPU/owner checks and56 shader,2 camera,24 GUI,6 map,
shared-water and Alpha-scenery image checks pass. Water and other geometry families,
concurrent inconsistent inputs and complete original visual acceptance retain their
explicit scope limits. No owner assets or records are packaged.

The same independent author also clarified WORLD material-buffer delivery/readiness
and historical pose-control names. Those stable addenda have been read; they are a
separate implementation contract and do not establish that all posed material/effect
bindings are already integrated.

## Static-object scene admission and fixed scenery anchors, 2026-11-02

The implementer read steam-static-object-scene-admission.md, its coordinate link,
the existing independent scene/fade contracts and engine sources only. The separate
native-only author established the physical player-zone conversion, inclusive zone
square, center/radius arithmetic, model-range binding and independent mesh admission
without reading the engine or tests. StaticObjectScene expresses those facts using
CubeVulkan's fixed arithmetic and shared scene view. World keeps static objects
after environment records in available zone/record order; the renderer uses a
separate static-object sphere rather than the environment mesh gate. Scenery keeps
its raw fixed position while local wind and object transforms are refreshed.

Asset-free CPU cases cover narrowing, negative boundaries, low64 addition, sphere
extent, range equality, all-plane admission and fade. A synthetic GPU object renders
despite its containing zone's failed environment-mesh bounds. Full engine and image
checks pass. The renderer test does not establish complete loaded-but-unmeshed zone
production, ground-item ordering or native pixel acceptance; those remain separate
integration work. No owner assets or captured records enter either package.

## Ordinary pose materials and playable-hair geometry, 2026-11-02

The implementer used the independent WORLD-controls, pose-items, pose-material
preparation and pose-sprite-marker contracts, their cross-links and engine code
only. The separate native-only author verified caller light/material/buff/fade
bindings and the105 playable-hair catalog entries without engine, tests, extractor
or owner-data access. The implementer read both complete stable supplements and
their links before implementing the corresponding behavior.

WorldModelControls separates physical light coordinates, base material, buff
factors, pixel opacity and descriptor controls. The renderer shares the prepared
frame material with gameplay/title poses; the queued shader does not apply it
twice. Pose resources retain source RGB instead of baking native tint into voxels.
CubModel's marker policy suppresses owned geometry while preserving raw occupancy
for adjacent faces and corner shading. Alpha's existing path and the deliberate
dual-tone hair extension remain separate from this native contract.

Synthetic records, tiny asset-free RGB volumes and GPU triangles verify light
offset/narrowing, ordered factors, fade/opacity, material/control delivery, negative
Glow, explicit winding, clip-depth changes and once-only underwater material.
Marker tests distinguish exact colors, neighboring-face blockage and unchanged
packed colors of surviving vertices. Full engine/image checks and five no-save
owner-model smoke captures pass; these do not establish complete native pixel
acceptance, all resource factories or unavailable-resource retained-state timing.
No native implementation or owner asset/capture bytes are published.

## Loaded static-object production, 2026-11-02

The implementer used the existing independently authored static-object scene
admission contract and engine sources only. The loaded-zone lookup, physical-player
square and independent terrain-mesh admission were read before implementation.
ExtractedWorld now offers a lookup-only object snapshot, sharing its existing record
decoder without collecting environment models. World owns an absolute-zone object
cache that retains small identity tokens and updates source lifetimes/notices
independently of terrain GPU jobs. A shared collection helper preserves live door
state while avoiding duplicate callbacks and draws. Alpha retains its prior path.

The integration fixture creates tiny synthetic CWZ/CWR/CUB inputs and an inert asset
resolver sentinel; it never executes a game or substitutes runtime owner assets.
It checks absent-zone no-load, raw origin records, ordered unmeshed objects, live
hinges, physical-square retirement, source reset, terrain publication and explicit
same-lifetime invalidation. Full engine/image suites and five no-save owner-data
stability runs pass. Stable record coverage does not establish concurrent native
publication timing, every live list mutation, ground-item order or full visual
acceptance. No owner content is included in source or runtime packages.

## Ordinary ground-item scene, 2026-11-02

The separate native-only author verified steam-ground-item-scene-rendering.md and
corrected loot-pickup.md without engine, tests, extractor, reconstruction or owner
data access. The implementer read the complete stable supplement and changed links
before implementing its bounded geometry/material/queue contract. GroundItemScene
uses engine-owned matrix and fixed arithmetic, native-format records and owner
resource dimensions. Gameplay consumes the live ordered ground lists, keeps raw
admission/fade positions distinct from hopped GPU anchors and shares geometry with
native interaction boxes. Existing personal-validity and shader-control helpers
remain separate. Ground/static tags preserve the specified collection order.

Tiny synthetic records and matrices test the documented numeric boundaries, and
asset-free GPU triangles test raw-sphere rejection and faded collection ordering.
Full engine/image checks and five no-save owner-data stability runs pass. The
supplement's separate kind8 producer does not establish its downstream appearance;
that effect consumer is not claimed implemented. Exact native resource-pointer
sorting, alternate callers and complete native visual acceptance remain separate.
No owner art, exports or native implementation is added to either package.

## Live object records and reward-map capture, 2026-11-02

The independent native-only author specified current static-object fields, required
counts/users and the separate Zone reward-map ABI. That author read no engine,
tests or exporter code. The implementer used those prose contracts, project-owned
sources (including the existing exporter bridge) and synthetic data, without reading
the original game's code, SDK implementation or reconstructions.

Interaction and mission adapters now read current physical records. Required items
remain separate from complete reward-map item values. The owner-data capture adds
a versioned project chunk, including known-empty maps; omitted data remains unknown.
The capture reader uses guarded memory observations and no native tree methods.
Synthetic nodes test signed order, sentinel behavior, full item bytes and visibly
changing/unavailable data. These checks do not prove a native lock or atomic snapshot;
the bounded ABI assumes a valid quiescent map. Real-install capture acceptance is
separate. Owner items, models and exports stay out of source/runtime packages.

## Retained water/particle state and point lifetime, 2026-11-02

The implementer read steam-world-pass-controls section3, steam-particle-bursts
section 6, the independent point-table-state-lifetime supplement and engine sources
only. The native-only author traced construction, early sky, late overlays and
shared GUI point setters without implementation access. Uninitialized native
sky-table inputs do not define a numeric oracle: the engine deliberately uses
an empty fallback until a scene consumer binds a table. It does not simulate
uninitialized memory, retain a selected-zone key or infer an origin rebase.

Particle and water clip offsets retain preceding WORLD state; water's depth and
color phases remain distinct. Asset-free Vulkan triangles exercise a failed
faded-resource setter followed by each consumer, without native model bytes.

## Shared fog binding, 2026-11-02

The native-only author separately wrote steam-fog-scale-state-lifetime sections1–5
and corrected the earlier conflicting nonpositive-distance description. The
implementer read that prose, character-card-preview section 6, ring-selector
section5 and engine code only. No original code, instructions or shader binary
was read by the implementer. FogScale, the explicit map admission and nonnull
empty-card request preserve reached setup independently of visible pixels.
Unproved first GPU contents have a defined engine fallback and are not a native
numeric reference. CPU scalar inputs and asset-free Vulkan triangles test the
world/water, empty callback, null resource and unavailable GPU-buffer boundaries.

## Shared map pixel stages, 2026-11-02

The native-only author independently wrote steam-map-world-pixel-contract sections1–4
and corrected the older map paraphrase, without implementation or capture access.
The implementer read that prose, the corrected worldmap sections3.1/3.3 and engine
sources only. The existing engine WORLD pixel function and verified color helpers
are shared through project shader includes; no native shader, original instructions,
SDK implementation or reconstructed code was opened or copied. Asset-free map
planes check full and partial fog plus retained raw sun/gradient inputs. The Alpha
appearance compatibility path and legacy CPU previews are kept distinct from the
bounded Steam map-region claim. Marker-specific inputs and native visual acceptance
remain separate work; source/runtime archives contain no owner assets.

## Mode0 map resource consumers, 2026-11-02

The native-only author extended the independent map WORLD contract with section5
and a queued-state cross-link. The implementer read that prose and engine code
only. Native address ordering is represented by engine-owned stable identities;
no catalog/native-heap equivalence or native resource-address capture is claimed.
The renderer separates opaque readiness from faded descriptor submission and
never submits unavailable Vulkan buffers. Synthetic resource/FIFO cases use no
owner assets, and unavailable-buffer cases were run only after the guards were
implemented, avoiding a deliberately invalid Vulkan baseline. Zero geometry
counts remain distinct from resource readiness under valid header lifetimes.

## Map WORLD/view normal and composition precision, 2026-11-02

The implementer read independently authored map-contract section 6, the corrected
worldmap precision links and engine sources only. The native-only author inspected
no implementation or captures. A project-owned shared frame matrix and typed map
controls preserve independent WORLD/VIEW inputs; GPU composition is expressed
with engine shader helpers. Normalization constants are instruction-derived
references under the stated float environment, not runtime captures or a GPU
rounding guarantee. Synthetic planes compare view versus WORLD changes, material
alpha, Glow and view-normal Shininess. No native instructions/shader source, SDK
implementation, reconstructed code or owner asset bytes were read or copied.
An audit found no production users of the old CPU fog/pixel approximations; they
were removed while the CPU color helpers actually used by coin previews remain.

## Shared WORLD vertex composition, 2026-11-02

The implementer read steam-world-vertex-composition-applicability.md sections1–5,
its WORLD-output/pass-control links, the existing map section 6 and engine sources
only. The independent native-only author inspected no engine, tests, exporter,
owner data or captures. The shared vertex stage reuses the project-owned matrix
helper to compose GPU coefficients before local dots, with separate WORLD lighting
and view-normal inputs. Engine leaf deformation retains its own extension path.
Synthetic finite matrix inputs test actual Vulkan coverage and retained occlusion;
the corrected winding/visible controls demonstrate three old-shader failures and
new-shader passes, without asserting a native numeric or cross-GPU rounding oracle.
No native code, shader instructions, reconstruction or owner assets were copied.

## Particle scene-coordinate binding, 2026-11-02

The implementer read existing independent particle-bursts section7.2, the shared
style-consumer transform statement and WORLD-coordinate prose plus engine sources.
No new native-code investigation or original implementation access was used.
ParticleScene adapts the existing strict matrix builder to the installed scene
origin while retaining the source-home Alpha path. Owner cube/note geometry stays
in the local asset loader. Synthetic CPU inputs check header/Z/fraction behavior;
a Vulkan triangle fixture compares near/distant coverage and a deliberately
pre-rounded home-space control. It is not an original-game capture or a general
last-bit GPU oracle. No models, captures, recordings or owner bytes are packaged.

## Plasma color precision and tint isolation, 2026-11-02

The implementer read the independent ordinary clip contract sections1–8 and its
updated links, Plasma scene2.3.2, shape vertex/pixel operands and text6.8 plus
engine code. The native-only author inspected no engine/tests/exporter/captures.
The engine's float overlay representation and independent tint paths follow those
facts; no native shader instructions/source or reconstruction were read/copied.
Owner-HUD emission compares a visible text/shape subtree under changed parent
and glyph display fills at two scales. Actual Vulkan cases exercise fractional
color and preserved destination alpha; deliberate previous-behavior runs fail.
Source-over alpha uses optional diagnostic readback, without changing ordinary
screenshot opacity. This is color/tint groundwork, not a claim to implement the
newly documented clip pool, cache, blur or all inherited sampler state. Owner
art/fonts remain local and are excluded from source and runtime archives.

## Ordinary affine Plasma clip layers, 2026-11-02

The implementer read the independent clip contract sections1–8, its bounded
sampler-provenance appendix6.1 and updated prose links plus engine code only.
The native-only author inspected no engine/tests/exporter/owner captures. Public
Microsoft PSSetSamplers documentation supplies a separately cited conditional
NULL-sampler rule; it does not establish actual native bindings at arbitrary draws.
Project-owned command cuts, retained Vulkan targets, descriptors, barriers and
shader formulas implement ordinary affine clipping. Existing object-atlas
sampling and an explicit single-mip mask baseline retain their stated engine
scope; no native sampler-runtime oracle is asserted. Safe resize/allocation
handling is engine policy where native external/reset/failure paths are unproved.
Synthetic GPU geometry checks the three modes, nesting, empty clears, item depth,
atlas refresh and queued reuse. Owner data is used only for local callback/cut
validation and startup captures. No native shader instructions, original code,
reconstruction or owner assets were read/copied into implementation/packages.
Caches, blur, flattened projective coordinates and static legacy clipping remain
separate work; this is not a full GUI or pixel-parity certificate.

## Legacy HUD mask composition, 2026-11-02

The implementer used the already independently authored clip-layer, shape/text
and sequence contracts plus the engine's existing evaluator and renderer. The
document-placement adapter now emits real child-layer/mask cuts and evaluates
animated shape attributes instead of substituting an averaged mask color.
The owner location template exercises balanced composition and changing shape
output at two GUI scales. The existing 42 Vulkan GUI cases and full regression
suite pass; hidden no-save captures are stability checks, not native visual
acceptance. Transform/Display animation in this compatibility adapter, surface
caches, blur and projective GUI remain separate. The new cache specification
was written by the independent native-only author and read as prose only;
it is not yet implemented. No native implementation or owner assets enter the
source or runtime packages.

## Legacy HUD animation channels, 2026-11-02

The implementer used Plasma scene sections2.2/2.6/2.7.9 and the existing project
animation evaluator. Document-placement rendering now initializes and samples
shape, Transformation and Display channels, propagates the absolute-seek force
gate, and applies layout overrides to their own channels. Disabled nodes are
excluded from emission. Synthetic movement, discrete clipping and visibility
tracks use the actual owner HUD glyph binding at two scales; deliberately
restoring shape-only sampling fails all six integration assertions. The corrected
source is restored and rebuilt before validation. This does not establish
persistent shared-keyable clock identity, cache/blur support or whole-GUI pixel
parity. No original implementation, reconstruction or owner assets are packaged.

## Homogeneous Plasma drawing, 2026-11-02

The implementer read the independently authored projective-vertex contract
sections1–5, the GUI-option/standard-shape supplement sections1–3, the corrected
scene2.7.4 and all updated blur/cache/clip/shape links. The native-only author
inspected no engine, tests, exporter, owner data or captures. The engine retains
undivided XY, geometric Z and W in its own overlay format, applies its existing
pixel bias before projection, and uses the specified default/layer Z coefficients.
Drawing's grouped descendant-to-owner mappings and four-component deformation
are separate from the CPU picking queries. Existing ordinary GUI tree matrices
supply drawing WORLD here; temporary cache/blur projection and native depth-stack
state remain separate work. Synthetic scene geometry checks ordinary, Widget and
descendant results; actual Vulkan cases check color interpolation, raw mask UVs
and near/far admission. A deliberately flattened shader fails four assertions;
the correct source is restored with a fresh timestamp and rebuilt before the
complete checks. These are finite-input functional regressions, not original-game
captures or a cross-GPU last-bit oracle. No native instructions, implementation,
reconstruction or owner assets enter source or runtime packages.

## Bounded blur clearing and callback traversal, 2026-11-02

The implementer read the independent blur contract sections1–6, options/shape
supplement1–3, depth-state contract1–5 and every updated link. The native-only
author inspected no engine, tests, exporter, owner data or captures. The final
source-A clear, distinct child/own-hook placement, null-depth layer behavior and
absence of shape-counter consumption by rectangle copies come from that prose.
CubeVulkan keeps child work in its offscreen command stream, reopens/clears the
retained source, then reaches the own hook after restoring the parent. Under its
explicit clamp sampling and draw-request API, copying the transparent source has
no pixel effect; it omits that copy and the discarded intermediate images rather
than introducing a Gaussian. Intermediate reduced projection, inherited native
binding side effects, changed samplers and untraced callback state are not claimed
as API/state parity. Widget/node caching and externally enabled depth-stack behavior
remain separate. The default native depth-stack bit is off; default Plasma depth
testing is disabled independently of the bound DSV.

Local GPU integration uses synthetic red/green geometry through the actual HUD
with only its existing white atlas cell. Small/large radii and both clip admissions
check discarded child color, visible own hooks and the empty retained mask. The
test loads the owner's atlas locally when available; it saves none of that data.
Omitting the rebind clear fails exactly the two plain retained-mask cases; the
correct source is restored and rebuilt before full checks. CPU callback/cut tests
cover two GUI scales. Point queries avoid building the drawing-only inverse matrix.
No native code, instructions, reconstruction or owner assets are packaged.

## Queued title logo and native GUI voxel admission, 2026-11-02

The implementer used the independent frontend font/logo contract sections1–5,
the Ring queued transform/consumer and WORLD controls section4. The native-only
author read no engine, tests, exporter, owner data/captures or game execution.
The replacement removes the guessed60-degree constant-magnification logo camera
and face-baked lighting. It shares the ordinary finite queued lens, appends the
native model transforms around the full allocation and renders before Plasma,
retaining frame sky/fog/sun inputs and scene depth. Layer item cuts advance past
the inserted logo; no private depth clear or missing-resource substitute is added.

The engine's own GPU checks exposed an untagged-CUB path into its generic shader.
The shared GUI CUB producer now retains packed RGB/V admission for logo and item
resources. The synthetic real-producer cases cover both world styles, queue order
and inherited depth; an untagged-face control fails the two WORLD color assertions.
CPU checks cover the finite perspective, header center and fixed effect/palette
reset. Local no-save captures are stability checks, not original-game pixel oracles.

The immediate font request remains resource1.dat as established by the native
constructors. Named-memory and basename cache precedence do not prove the effective
font bytes in every running installation. No resource2 switch, glyph-case claim or
font artwork is inferred from old modded screenshots. Source/runtime packages
contain no owner models/fonts, native instructions, native binaries or captures.

## File-backed font catalog and inline document requests, 2026-11-02

The implementer read frontend font/logo sections1–5 and the independent font
registration note sections1–4. The separate author used no engine/tests/exporter,
owner assets/captures or game execution. The new catalog follows the described
TTF/DAT file and basename-cache route with the requested path preceding registered
directories. A failed request does not acquire a cached face; only TextShapes
retry Arial. The fixed font1 catch-all is removed. Backend glyph IDs and per-atlas
bindings are engine structure, not native field/layout copies.

The PLX parser's optional complete-shape callback runs inline before its next
shape, keeping shared shape-index encounter order. The three specified startup
documents use it before immediate menu fonts are resolved. This does not claim a
complete global request schedule: named-byte insertion/effective byte provenance,
native vector-font nested documents, arbitrary clone callbacks and concurrent
resource changes remain outside the implementation. No absent registry producer
is inferred from a bounded unsuccessful trace.

Local owner-face tests use generated temporary directories to distinguish first
basename retention, requested-file priority, ordered directory retry, template
fallback isolation and a file becoming available after failure. Glyph comparisons
use locally loaded references, not copied reference pixels in source. The parsed
owner document checks complete properties and callback timing; temporary font
copies are removed only after checking their generated direct-child path. Full
engine/image checks and private no-save captures validate integration. Original
game code, owner/system fonts and captures are excluded from distribution.

The independent cache-startup supplement sections1–3 was also read. It establishes
the ordinary engine byte's initial0 and reached teardown1, distinct Widget cache
scalar defaults/copy/attachment and explicit destruction bounds. It supplies a
future cache consumer contract; it is not a completed persistent-cache feature.

## Retained ChatWidget target and explicit invalidation, 2026-11-02

The implementer read cache invalidation sections1–6, cache callers sections1–3,
ChatWidget lifecycle sections1–5 and the existing surface/clip rectangle contracts.
The independent native-only author inspected no engine/tests/exporter, owner data
or captures and executed no game. ChatWidget is a proved ordinary game requester;
base constructors/options alone do not enable caching on every object. No broad
absence of other callers is inferred from a bounded search.

CubeVulkan's renderer keeps Widget targets separate from its fullscreen clip pool.
The admitted root ChatWidget stream records redraw/restore/copy cuts, uses rounded
physical dimensions and a local GUI projection, preserves main depth, and copies
the retained RGBA8 image through the ordinary surface blend. Explicit clamp/linear
surface sampling is the engine baseline, not an unmodified native binding oracle.
Weak owner keys release expired resources safely after in-flight work; unchanged
targets/descriptors are reused without a per-frame device wait. Resize and disposal
wait before replacing resources. Source image dimensions remain separate from a
fresh final destination query. Rectangle copies do not consume a shape-depth entry.

Chat notifications come from append, short-key and Enter-close paths; character
insertion and direct editor-byte clearing do not acquire invented dirty writes.
The interface adapter marks editing before drawing; the own hook has no generic
time rule. A point-placement equality guard preserves its pending indication.
Cache creation/resize dirties after clearing, and a cache hit suppresses manual
drawing/pruning/caret work. This is the concrete leaf ChatWidget adapter, not a
complete replacement for shared keyable49/4a traversal on arbitrary scene graphs.
General node caches, nested cache/clip projections, nondefault cache scalar
setters, arbitrary custom callbacks and full GPU binding-state replay remain open.

CPU tests exercise signed rounding, stream admission, equal placement, no-op keys,
empty append/close and no extra row-prune notification. Synthetic Vulkan tests
distinguish retained RGBA8 composition from direct alpha, cross-frame reuse, moved
destinations and restored main-depth attachment. Actual HUD emission tests initial
and post-allocation repaint, skipped callbacks and repeated editing invalidation.
Only local owner faces/atlas cells are loaded; no artwork or reference pixels enter
test source. No owner assets, native instructions or original-game binaries ship.

## Queued HUD coin, 2026-11-02

The implementer used independent hud-money.md section2, WORLD pass controls
section4 and the shared vertex/output contracts plus engine sources only. No
additional native-code investigation or access to original implementations was
used. The native-only author remains separate. A common queued-model request
now serves the logo and coin; the coin loads the owner's CUB once and supplies
its own material, Glow and shader DepthOffset. Full header allocation and the
configured screen camera replace the baked-color approximation. The earlier
CPU map-color helpers are no longer used by this runtime coin path; existing
independent CPU test references still use them. Synthetic solid CUBs exercise
actual renderer uploads and pixels in both world appearance styles. No coin
artwork, original binaries, extracted files or captures enter either package.
These checks do not establish complete original-game visual parity or general
resource grouping across all queued overlay producers.

## Compass and HUD descriptor typing, 2026-11-02

The independent native-only author supplied steam-hud-model-overlays.md1–6,
including a separately traced normal-caller flag0x20 addendum. The implementer
fully read both the stable contract and that addendum, plus the WORLD ripple
prose, before using them with engine sources only. The author inspected no
engine, tests, exporter, owner data/captures, SDK/reconstruction implementation
or running game; it is held again. Its descriptor-to-setter trace corrects the
older ambiguous money labels: coin Glow0/Shininess1, compass Glow1/Shininess0.
The shared WORLD path therefore retains the positive-Shininess frame-gradient
input under Alpha appearance too. Before that binding correction, the focused
coin test produced173 under Alpha versus the independent90.3 reference; the
expected pixel was not retuned to the incorrect binding.

Compass geometry comes only from the owner's file at runtime. Synthetic solid
CUBs, negative/signed-zero headings, physical placement and existing shader/depth
checks provide bounded tests without distributing artwork. Context/special
producer prose is available but its remaining implementation/upstream selection
is not claimed complete. No original code, instructions, binaries or owner assets
were copied into source or runtime packages.

## Context-item lifecycle and special HUD row, 2026-11-02

The implementer fully read the independent steam-hud-context-item-lifecycle.md
sections1–6 and previously read HUD-model-overlays1–6, including its scene-mask
addendum. The native-only author read no engine, tests, extractor, owner data,
captures, SDK/reconstruction implementation or running game; all author work is
held. Only those prose contracts and existing engine sources informed this change.

NativeHudContext stores an embedded semantic item/count, queries loaded zone lists
without allocating zones, and counts the category-selected canonical tab. Its World
adapter runs between the prelude/pet work and ordinary creature traversal; admission
does not acquire a positive-dt, authority, health or UI gate. Main binds the active
engine world identifier and borrowed session inventory. No context cache is added
to saves or packets. Renderer row selection uses current player region and physical
tab1 slots. The legacy HUD emits the context numeral through its existing outlined
font adapter; a complete persistent named-TextShape/cache binding is not claimed.

Synthetic records test padding, inactive/active attributes, formula words, signed
counts, last-candidate rejection and coordinates. Actual World-facade tests verify
the zero-time snapshot and later inventory change. Generated solid CUBs test WORLD
lighting, geometric shadows, Plasma order and depth in both appearance modes;
an optional local HUD check tests resource requests and count threshold/font emission.
The lit row pixel reference comes from the independent fixed-palette equations,
not an owner screenshot or retuning the shader to a sampled color. Invalid resource
skip is an engine admission policy, not a newly asserted native completed branch.
No original code, native instructions, owner artwork or extracted data is packaged.

## Root HUD queue identity and signed money, 2026-11-02

The implementer used the already independently authored/read HUD-model-overlays
section 6, Ring-rendering resource order and HUD-money section4, plus engine sources
only. No new native tracing or access to original implementations was used. The
renderer orders its admitted root opaque screen prefix by its own resource handles
and keeps FIFO within one handle. It leaves explicit interior layer/cache cuts,
later callbacks and faded/opposite streams intact. This is an engine identity
mapping, not a claim that handles numerically reproduce another process's resource
pointers or establish every owner-resource alias. Existing native numeric/visual
comparison gates therefore remain.

Synthetic overlapping CUBs verify resource order and same-resource FIFO in both
appearance modes. Actual local-font HUD emission verifies signed gold separately
from missing display data. Original native-save gold decoding remains unchanged.
Only engine-owned shader/geometry and local owner font inputs are used; no owner
artwork, native instructions or original-game files enter the source/runtime packages.

## Steam sky RGBA and retained transitions, 2026-11-02

The implementer used only the independent sky-and-clouds B.3/B.4,
scene-lighting 2.6/2.8 and new sky-state-initialization sections1–5, plus
engine-owned code/tests. The existing isolated native-only author supplied the
new constructor/reset/transition supplement without engine or owner-data access,
then held all tracing/writes. Its correction supersedes earlier sky-copy and
flagged-rate prose; the older sections now point to that correction.

The engine retains four-channel sky/fog/sun state, initializes it from the
documented constructor and applies the corrected ordinary/interior rates without
first-update snapping. Actual physical-player block flags feed the target;
unavailable legacy mesh costs still leave that separately known colour usable.
Flight uses canonical player fixed coordinates/anchor/start and strict horizontal
thresholds. Provider tests cover partial place alpha, selected flight keys,
water-before-effect13 ordering, zero/negative/rounded/capped frame intervals and
both transition branches. The optional shared Omega water deliberately keeps its
immediate underwater presentation; Alpha appearance remains a separate extension.
These tests do not establish an original-game pixel oracle, exhaustive recovery
lifetime, or native GPU initialization. No original implementation, owner artwork
or extracted content is included in either package.

## Directional-light state retention, 2026-11-02

The existing isolated native-only author supplied directional-light-state sections
1–4 without engine/tests/exporter or owner-data access, then held all tracing and
writes. The implementer read only that prose, scene-lighting 2.5/6.2 and engine-owned
sources. The renderer no longer substitutes constructor front/back/ambient colours
when its application clock moves backward. Constructor initialization stays intact.

A fresh Vulkan renderer draws a synthetic triangle before and after a positive
interval, then verifies the changed result survives rollback and range reset.
Two older structural admission fixtures now compare the same geometry against
an immediate ungated reference under the current retained lights. Original fixed
shader-equation checks remain separate; production colours were not retuned.
This does not establish alternate recovery/reset paths or initial GPU contents.
No original code, owner artwork or extracted data is distributed.

## Gameplay camera double-degree storage, 2026-11-02

The implementer read the existing independent gameplay-camera sections1–3.4,
the already established camera/control contracts and engine-owned sources only.
No new original-game tracing or owner capture was required. Gameplay now reuses
the frontend's existing independently implemented OrbitCamera state and smoothing,
retaining target/displayed pitch, roll and yaw as double degrees. Native control
bases, assistance targets, camera positioning, view rotation and compass inputs
no longer round-trip through float radians.

The mouse helper applies float speed/displacement staging before widening the
delta into double targets and clamps only the mouse pitch path. Pad and assistance
update degree targets directly. The existing polling adapter dispatches movement
when its pointer changes so idle polling cannot impose the mouse-only clamp on
pad targets. This is not proof of exact original callback/event coalescing or
complete GUI capture/device admission. Those adapter gaps remain explicit.

Legacy startup defaults, saved-angle/CLI compatibility and legacy presentation
consumers retain named radian adapters. The broader frontend fixed-camera handoff
and original event-specific initialization/reset assignments are still separate
work; this change does not claim all camera parity. Tests use known native mouse
increments, full pitch bounds, float-before-double staging, sub-float degree
displacement/smoothing and non-wrapped yaw. No original source, native instruction
bytes, owner artwork or extracted data enters the publication packages.

## Frontend camera angle/distance handoff, 2026-11-02

Using only the already independent gameplay-camera3.4, frontend-character8 and
engine-owned sources, the implementer connected the frontend Play result's value
snapshot to gameplay. Continue keeps all displayed/target angles and sets both
distances to8; selecting an existing character assigns90/0/180 and8. Selection of
New Character retains its existing90/0/0 assignment. The separate never-shown
Create World handler in the specification is not treated as Create Character.

Gameplay installs the snapshot after compatibility save restoration and preserves
different displayed and target values through startup. It does not claim the full
fixed camera eye/look-at state has been transferred; that remains separate work.
Direct diagnostic launches keep their explicit compatibility initialization.
The Continue regression seeds unequal, nontrivial displayed/target values and
checks only distances change. No original code or owner assets are distributed.

## Canonical fixed frontend camera and retained position, 2026-11-02

The existing isolated native-only author supplied camera-position-lifetime1–4,
then held all tracing/writes. The implementer fully read that independent prose,
gameplay-camera5–6.2 and the existing engine-owned position/terrain components.
Native construction explicitly zeros all eight fixed position/origin components;
ordinary frontend transitions retain anchors, and the normal update has no special
first-call or zero-time priming gate. Alternate callbacks/recovery and initial GPU
matrix contents remain outside the established result.

A shared creature-facade reader supplies physical position, posture, velocity,
ability, creation visibility and resolved mount metadata to both cameras. The
canonical frontend now runs the existing fixed-point follow/orbit/eight-corner
terrain checks rather than its compatibility stand-in sweep. Its retained zero-
initialized position state advances ordinarily and transfers on Play without
reconstruction from float local coordinates. Transition distance resets update
the snapshot's distance without rebuilding its retained eye/look-at. Gameplay
restores the completed negative look-at XY origin separately; eye is not the origin.
Compatibility launches and stand-in views remain explicitly bounded alternatives.

Actual-facade checks distinguish physical position from a deliberately different
drawn record and verify creation-layer input. Existing independent posture,
fixed-weight follow, strict safety-distance and terrain tests cover the reused
algorithm. Entry capture runs verify both angle and fixed-state handoff markers.
This does not close first/pre-camera matrix lifetime or full live pixel parity.
No original implementation, instruction bytes or owner artwork is distributed.


## CPU camera publication and retained early consumers, 2026-11-02

The existing isolated native-only author supplied camera-matrix-lifetime1–5,
read back the complete independent prose and held all tracing/writes. The engine
implementer fully read that note without access to original implementation.
The ordinary constructor supplies no canonical numeric values for its four CPU
matrices; known anchor/angle defaults do not initialize them. Direct frontend
handlers retain completed matrices. Every reached late camera phase rebuilds
them in full, including zero elapsed time, before downstream scene submission.

The engine now publishes its computed projection at prepareScene rather than
waiting for swapchain acquisition/presentation. Earlier speech and feat consumers
retain one value snapshot of the preceding camera and matching fixed origin,
including through the later UI callback. This preserves ordinary scheduling
while keeping CPU camera updates independent of skipped GPU submission. The
absent projection before the engine's first CPU build is an explicit safe engine
policy, not a claimed native constructor default. Destruction/recovery parity
and untraced callbacks remain outside this bounded result.

Actual renderer checks query a changed fixed origin before any presentation,
then rebuild shaken/restored matrices at an unchanged clock. Existing native
projection, large-origin, lifetime and Vulkan image checks remain in validation.
No owner artwork, original implementation or instruction bytes enter the source.


## Stationary mouse polls and relative device units, 2026-11-02

The existing isolated native-only author supplied mouse-delivery1–6 and held all
tracing/writes. The implementer fully read that independent note and the existing
character-list/input-frame7 and gameplay-camera3.1 prose. The reached native loop
calls its movement handler once before XInput even with zero relative displacement.
UI-false calls bypass the UI-only last-device return and still clamp pitch.

The engine's earlier coordinate-change guard was incorrect for that admitted path;
it is removed. Focus, binding capture, existing UI admission and explicit camera
compatibility options are otherwise bounded by the existing engine adapters.
CPU boundary cases cover stationary upper/lower poles, unchanged yaw and zero
sensitivity/inverted Y. This does not establish the full shared GUI capture bridge.

Native relative arguments are device counts, whereas absolute UI arguments come
from cursor APIs. The engine requests supported GLFW raw motion for captured look;
GLFW supplies it only with disabled cursor mode, leaving ordinary GUI coordinates
absolute. The API contract is the public
[GLFW3.4 input guide](https://www.glfw.org/docs/3.4/input_guide.html#raw_mouse_motion),
not game implementation. Mode readback is logged; unavailable raw input retains
an explicit cursor-motion compatibility path. This bridge does not certify device
failure/reacquisition, exact polling/coalescing, untraced raw-input paths or hardware
motion parity. No original implementation, instruction bytes or owner assets ship.


## Shared native lens and projection consumers, 2026-11-02

The implementer fully read the existing independent gameplay-camera7.3,8 and8.1
without access to original implementation. The documented global float starts at
the exact stored50-degree radians, easing toward30 when imported u16 toggle0x400
is set and toward50 otherwise, at distinct0.01/0.002 rates. The completed float
radians feed projection; requested orbit distance is independent. A local set
producer remains unproved. Existing record import and acceleration-based clear
are separate contracts; no scope key, gesture or save field is invented here.

A typed CPU lens owns that retained value, explicitly shared by the process's
gameplay/frontend services. Standalone frontend callers have their own fallback.
Canonical camera phases update it from the existing creature facade and pass raw
radians to CPU visibility, both world render styles, targeting rays and the legacy
plate projector. There is no float degree conversion roundtrip or reset on normal
character/frontend transitions. Already-at-target updates omit only the pure gain
calculation; ordinary matrix construction remains independent and continues.
The generic Graphics.FieldOfView setting remains the stand-in/compatibility lens;
it does not override a canonical native camera's supplied radians. Finite GUI/map
projections keep their separate contracts. Alternate recovery remains unproved.

Derived float references cover constructor, stationary/narrow/release/fractional
and capped intervals. Actual Vulkan checks deliberately set compatibility FOV110
while supplying raw50/30 radians, checking pinhole displacement and a side-frustum
sphere in both appearance styles. These are bounded consumer checks, not a claim
of every native visual/device/recovery path. No owner assets or native code ship.


## Retained mouse-phase device selection, 2026-11-02

The implementer used the already independently authored mouse-delivery1,3,5 and
character-list/input-frame7 contracts. The reached physical-control pass may clear
last-device before the mouse handler; subsequent XInput work may set it again.
The mouse handler's UI-only return therefore observes the earlier selection,
not the result of the later controller work. No original implementation was read.

MenuInput now retains that selection immediately after its existing mapped physical
check and before its once-per-frame controller sample. The map movement/drag,
customization rotation and answer/customization look handlers consume this earlier
snapshot. Current controller callbacks retain their live selection and cannot
rewrite an already completed mouse snapshot. Buffer clearing preserves both
retained values until the next sample. The snapshot does not itself admit a poll.

CPU cases exercise default selection, mapped physical takeover, actual later
finishPointerCallbacks, clear-buffer lifetime and the following iteration's
controller selection. This bounded change assumes the existing engine adapter's
UI admission/scheduling; it does not implement the entire native normal-input gate,
shared capture/point-hit root or all UI changes during callbacks. Physical-device,
coalescing/failure and broader live GUI comparisons remain acceptance work.
No owner assets or original code are included.


## Retained frontend/gameplay interface resources, 2026-11-02

The existing isolated native-only author supplied gui-root-construction-lifetime1–7,
read back the independent note and held all tracing/writes. The implementer fully
read it without original implementation access. A detached document source is not
the live E+140 draw/pick root. Direct frontend-to-play handlers retain their existing
Interface/root; the separately proved card-child rebuild is not a whole-root reset.
Hide/detach and actual node/Widget deletion have distinct cleanup contracts.

Main now supplies one Hud to the frontend and subsequent gameplay instead of
discarding a session-owned frontend interface and loading a second one on Play.
Completed same-install loading is reused explicitly through ensureLoaded; explicit
load remains a reload, menu-only loading invalidates full readiness, and a different
installation is not accepted from the cache. Preview draw submissions are cleared
at entry because temporary frontend meshes have been released; persistent resources
and UI templates remain. The separate frontend/panel scene graphs and full runtime
capture/global-pick/deletion bridge are still open and are not claimed implemented.

An owner-backed local temporary fixture loads its copied font/UI document, removes
those private copies, and checks retained geometry and same-install/path-alias reuse
with cleared diagnostics.
It then checks rejection of a different missing install. Original resources remain
untouched; fixture cleanup uses only its exact files and empty directories without
recursive deletion. No owner font/art bytes are stored in source or release assets.
Actual frontend-entry captures cover the shared Hud through the Play transition;
broader live UI and alternate reconstruction/recovery paths remain acceptance work.


## Owned GUI node mutation and deletion, 2026-11-02

Using only gui-root-construction-lifetime §§1–6, Scene now retains detached trees
outside its live root, appends reparented nodes without replacing their identities,
and rejects cross-scene/cyclic changes. A scene-bound Input observes explicit node
and own-Widget deletion. Capture release runs the unchanged-pointer pass before
node detachment; a retained hit on the dying node is then cleared. Own-Widget
replacement/deletion clears matching focus, auxiliary and down-owner references
without an ordinary focus event or a held-mask reset. Registered callbacks belong
to that Widget lifetime. Hide/detach do not invoke deletion cleanup.

Template sizing probes and inventory-cell prototypes are now detached from the live
root. Probes, speech retirement, icon replacement, inventory-cell clearing and panel
Widget replacement use the scene operations. Bulk clearing without a bound Input
keeps the existing linear vector clear. Ordered asset-free tests cover the retained
identities, callback-before-detach order, a newly picked surviving hover, a repick of
the dying node, Widget replacement, child clearing and a detached source whose live
copy survives its destruction. Structural callback reentry
is explicitly rejected as an engine guard, not claimed as native recovery behavior.
The scene must outlive its bound Input; whole-scene teardown, external raw tree
mutation and arbitrary callback destruction remain outside this contract. Runtime
frontend/panel scene unification and the full live input bridge are still open.


## Explicit interface reload boundary, 2026-11-02

This is an engine resource-lifetime correction using its own code and the existing
root-lifetime distinction, without further native investigation. Explicit load/loadMenu
starts a fresh Hud value before reading owner resources. Cached templates, document
shape geometry, cursor/panel nodes, fonts, glyphs and transient submissions therefore
cannot carry bindings from the previous document into the new load. A failed explicit
load leaves no interface reported as ready. Ordinary same-install ensureLoaded calls
continue retaining the completed value and do not reread removed private resources.

The owner-backed private fixture now compares a reused template key after explicit
reload against a freshly loaded interface, using different real button templates to
make stale node reuse observable. It separately verifies same-install retention and
failure clearing. Owner data remains temporary/local. No game content is packaged;
full live GPU hot-reload and the wider shared input bridge are not certified here.


## Shared frontend/gameplay GUI scene, 2026-11-02

Following the already isolated root/lifetime contract, frontend templates and
speech panels now use the same gui.plx Scene as gameplay panels. The redundant
frontend Scene owner and lazy allocation are removed; successful full loading
constructs the common scene before its clients use it. Menu-only or failed loads
have no usable full scene. Shape geometry can share the existing document/index
cache; live node, Widget and animation values remain per instance. Constructor
sizing still uses scale1 and restores the interface scale, and source probes and
inventory prototypes remain detached. Drawing continues through explicit subtrees.

This closes the duplicate gui.plx scene ownership boundary, not the complete native
UI tree. Cursor units remain in their separate scene. Semantic root insertion,
active visibility, cached hit-only templates, global picking and runtime input
binding still require the remaining integration. In particular, the inventory
composite's temporary visibility is not yet the native global gameUI visibility.
No original source or asset bytes are added.


## Alpha surface generation core, 2026-11-02

The user prioritized real Alpha terrain/biomes as the eventual default while
retaining Steam towns/dungeons and a separate Steam generation option, then asked
for verified Alpha-only locations and compatible TerrainVariance changes. The
isolated native-only author supplied alpha-terrain-generation-boundaries §§1–6,
read it back and held all tracing/writes. The implementer fully read that note;
no original native implementation, SDK or reconstruction was opened.

AlphaVulkan's README records the same clean-room process and GPL-3.0-only license.
Its original random/region/location/column code was mechanically namespaced as
alphagen, with source hashes recorded privately. The unchanged height/color/surface
arithmetic was checked against that own-code source; an explicit surface-stage API
retains the padded grid, random state and later boulder candidates without including
later placement/lighting. No original assets or captured voxel arrays are copied.
The six independent entry/completion contracts accompany the implementation.

Dedicated formula/region/padded-surface checks and the full existing CPU/owner suite
pass, with precision-sensitive sources built without contraction. A subsequent
CMake source-list cleanup produced no work, preserving those binaries. These are
contract/engine regressions, not original-game oracle evidence. Later terrain,
hybrid Steam structure placement, runtime world consumers and mode/default changes
remain open. No current default was renamed or switched to claim false parity.

The TerrainVariance GitHub checkout is isolated under build: the user's separate
local mod tree has unrelated uncommitted BigRegions work and was not modified.
Only the mod's own MIT code/metadata was inspected, not its native SDK. Its existing
column-offset hooks cannot alone establish Alpha column or biome parity. GPL Alpha
source must not silently replace MIT mod sources without resolving the license
boundary. The observed Steam Sphinx model is owner data and stays out of packages.


## Shared Alpha coordinate mapping, 2026-11-02

A new original MIT-licensed helper was authored in the existing MIT TerrainVariance
project from the independent coordinate-domain facts, then copied byte-for-byte
into CubeVulkan with that project's existing MIT notice. No GPL Alpha implementation
was copied into the mod. Explicit integer anchors map signed Steam block positions
to absolute finite Alpha positions; 64-column coverage splits across 256-column
parents and clips world edges without wrapping. No native SDK was opened.

Mod-side coverage checks exercise four-parent seams, every covered column, signed
coordinates, invalid/extreme inputs and both finite edges. Portable coordinate and
existing terrain-rule tests pass with the available MSVC toolchain; the separate
clang-cl compiler needed by native mod/placement builds was unavailable, so no
native DLL or placement-suite build is claimed. Engine Alpha checks pass against
the identical header. No runtime profile or default changes in this step.


## Cached Alpha surface consumers, 2026-11-02

SurfaceStore uses the existing independent surface/coordinate contracts and the
engine's own data types. It retains a seed/name/frame context, shares in-flight
parent requests, caches ready parents and provides owning column leases. Stage
publication now transfers a completed local Zone into const-owned storage, so the
shared const stage cannot mutate voxel data through a mutable unique_ptr pointee.
No original native investigation, data capture, asset or mod-code change was used.

All five targets build without warnings. The full CPU/owner suite passes after
const publication/cache integration; the subsequently added focused water fixture
also passes. Concurrent parent identity, lease survival/last-owner release and
water/solid surface distinction are exercised. The initial water fixture selected
an island (grid heights8.214..25.472), so it was moved to the adjacent exposed-sea
boundary without production formula changes. GPU groups were not repeated for
these not-yet-wired math/data consumers; the preceding full render checks remain
recorded separately. Runtime conversion/placement/default integration is still open.


## Alpha surface representation bridge, 2026-11-02

The implementer used the independent Alpha contracts, the bounded generation
assessment and engine sources to translate packed Alpha material/flag records
into Steam's explicit block representation. No native implementation or original
instructions were read. The new adapter preserves column geometry/RGB, translates
dry-air flags and rejects unspecified materials. Its glowing-to-light emission is
an explicit hybrid policy, not a claim that the native lighting pipelines match.
Synthetic material records and asset-free generated columns check all 4096 columns
in a four-parent crop, effective underwater reads and missing-domain admission.
There is no new playable default, structure generation or native-oracle claim.
No original model, terrain or capture bytes are added to packages.


## Alpha rock, road and terrace boundary, 2026-11-03

The implementer read the independent Alpha caves contract, its correction that
formations add rock, the bounded parity assessment and the sister AlphaVulkan
project's original GPL code. Owned PostPasses arithmetic is reused under the same
license with namespace and stage-boundary adaptations. The native-only author
remained held; no native implementation, original instructions, SDK code or
third-party reconstruction was opened. A private import audit checks the donor
hash, declared adaptations and unchanged initial-surface arithmetic/order.

The new immutable relief boundary stops before Alpha dungeon calls and refuses
unimplemented giant-tree centers. No native full-generation or running-game
oracle equivalence is claimed. Formula vectors and synthetic voxels check rock
addition, whole-formation protection, layered mesa material, terrace pool/floor
rounding and the distinct boulder flag rules. The shared cache and typed relief
consumer check concurrent reuse, failed-load retry and retained column lifetime.
Native models, columns, captures and other original assets remain outside packages.
Steam town/dungeon layout integration and runtime default selection remain open;
extra ruin structures were deferred by the user.


## Alpha voxel trees and owner palm leaves, 2026-11-03

The implementer read the independent tree contract and the required vegetation
placement and prefab-mode0 sections. Original GPL tree code from AlphaVulkan was
reused with namespace changes. The incomplete palm branch was replaced with an
implementation of the independently described owner-model stamping facts. No native
implementation, original instructions or third-party reconstruction was opened.
Only the needed placement sections of the full independent structure document were
read for this extension; unrelated structure builders remain deferred.

A private source audit checks unchanged non-palm arithmetic and stable donor/prose
hashes. Independent vectors verify the tree stream, cell counts and RGB; synthetic
models verify palm rotation, model-presence-dependent draws and type/flag handling.
Both the focused Alpha checks and full CPU/owner-data suite pass. These formula and
synthetic checks are not native runtime-oracle evidence. Native model data is read
from the owner's install and is never copied into either package.


## Alpha materials, lighting, clouds and frontend provenance, 2026-11-03

The existing separate native-only author supplied bounded prose for implicit
terrain color, environment model light, fixed cloud placement/noise constants,
normal frontend panel backgrounds and font-registry production. The implementer
read those notes, the engine's own code and the sister project's original GPL
code. No original game implementation, SDK implementation or reconstructed game
code was inspected or copied by the implementer. Owner resources were used locally
for integration checks and captures and remain excluded from publication.

The material path retains source Z and float precision; environment light keeps
its mesh-time sample and distinct own-light rule. Cloud tests include a verified
native-derived float word and fixed64 staging. Numeric checks remain distinct
from new native runtime oracles. Corner reuse and converged synchronous floods
change work, not their checked values or propagation cap. CPU/GPU checks and
private benchmarks qualify those bounded changes; full parity is not asserted.

The panel audit established owner-template provenance rather than a common RGBA
constant. Existing normal rendering already consumes that data. The font audit
left byte registration unresolved, so no new producer or source identity was
inferred. A separate metadata-only SDK license audit found the copied MIT notice
but did not establish original SDK permission/revision scope. No further SDK
implementation reuse is authorized by that observation. The extractor's automated
build uses its separate header dependency; this does not constitute a license
grant or implementation-source review. Final redistribution remains gated by the
rights/provenance checks described in sdk-license-provenance.md.


## Generation-aware setup and loader ABI note, 2026-11-03

The setup change uses the engine's existing model loader, plant-name mapping and
configuration selector. It introduces no original-game implementation or assets.
Synthetic models exist only in temporary test folders. The separate native-only
author inspected the installed original loader binary and supplied only the
bounded prose in specs/steam-mod-loader-capture-abi.md. The implementer read that
note, not the binary implementation or SDK code. No adapter was implemented from
it; interface observations do not establish SDK redistribution permission.


## Alpha raw rock precision references, 2026-11-03

The separate native-only author supplied alpha-rock-colour-precision.md from
original native instructions/constants and independent prose. The implementer
read that note and the engine's own code, without inspecting the original game
implementation, SDK, reconstructions or owner captures. Six conditional RGB-word
checks use independently derived seed123 offsets and synthetic cached climate.
The existing math passes unchanged. No original world sample or additional
runtime parity claim follows from these arithmetic fixtures.


## Native source windows and mesh preparation, 2026-11-03

The separate native-only author supplied steam-world-zone-window-demand.md,
including its sorter addendum, from original native evidence and independent
prose. The implementer read that complete note and the engine's own code only.
The original game implementation, SDK and reconstructed game code were not
inspected or copied by the implementer. Existing independent key-only ordering
is reused with signed preparation keys. New source-window, neighbor, retention
and budget helpers implement the bounded facts; backend timing/cache-role
limits remain explicit. Temporary local counters were removed before the normal
rebuild. No owner assets, exports, captures or original executable bytes are added.


## Immutable Alpha reads and local profiling, 2026-11-03

CPU sampling targeted only the engine's own built executable, with an exact path
check. No original game or SDK implementation was read by the implementer. The
read optimization uses the existing owned code's immutable cell/region lifetime
and synchronized publication contract. Successful thread-local reads are bounded
and keyed by distinct world lifetimes; misses still use the original synchronization.
No native numeric formula, asset or generation stream was replaced. Local sampler
sources and outputs remain private under build and outside publication.


## Eagle source anchors and displacement vectors, 2026-11-03

The separate native-only author supplied steam-source-window-ability-anchor.md
and alpha-region-displacement-vectors.md. The implementer read both complete
notes and the engine's own code, not original game/SDK implementation or captures.
Generated-world preparation consumes the existing owned actor's raw destination
only for ability0x9c; physical retention, title0x9d and arrival0x78 stay distinct.
Four independent arithmetic float-word fixtures guard displacement axes/staging.
The established displacement formula remains unchanged.
The geometry-cache lifetime policy is explicitly the replacement backend's policy.
No original assets, executable code or recorded owner data were added.

## Direct Alpha climate arithmetic references, 2026-11-03

The existing native-only author independently wrote alpha-climate-blend-vectors.md
from original Alpha code/constants and independent prose. The implementer used
only that note, its prerequisite contracts and owned code to check literal
four-field outputs and missing-cell defaults. The shortened-pi constant's rounding
bounds were separately reviewed without sending engine results to the author.
No generation implementation was replaced. These conditional arithmetic fixtures
are not runtime captures, prepared-world or positive-fire/oasis acceptance.
No owner assets, native implementation or SDK implementation enter the source.

## Host source-demand union, 2026-11-03

The implementer used steam-world-zone-window-demand.md section 7, the separate
steam-source-window-ability-anchor.md contract and owned engine code only. Host
requested-zone geometry, player-union membership and logical resident unloads now
follow those facts. Physical resident admission remains distinct from the travel
source anchor. Synthetic geometry references and the existing owner-data lifecycle
fixture check the change, including instance-death release while a separate reader
pins the old terrain. The replacement retains its own background loader and cache;
this does not establish native concurrent scheduling or every network demand path.
No native or SDK implementation was read, and no owner data is added to packages.

## Alpha parent retention and positive-fire references, 2026-11-03

The parent-cache continuation is an engine policy derived from count-only probes
of owned code, not original implementation or native cache reconstruction. Private
logging was removed; ready data has entry and estimated-payload budgets, while
leases/in-flight state retain the existing lifetime semantics. No generation math
or content was replaced. The separate native-only author wrote the positive-fire
supplement without engine/output access. The implementer used only that stable
prose and owned sources for literal returned-word checks. Full CPU/owner-data and
image suites pass. Neither conditional arithmetic fixtures nor one successful
full-distance capture certify whole-world/native-runtime parity. Owner imagery,
probes, exports, SDK implementation and game content remain out of packages.

## Alpha raw height staging, 2026-11-03

The independent native-only author wrote alpha-base-height-vectors.md sections 1–7
from original Alpha evidence and independent prose, without engine or output
access. The implementer read that stable note and owned sources only. A dedicated
H gate now follows the specified float32 operation chains, with literal synthetic
scalar-stage and conditional raw-H references; the generic easing helper is not
changed. No new runtime capture or final-surface oracle is inferred. Landscape
version2 keeps potentially changed terrain/profile/peer identities separate.
The actual ready-budget lease test uses wholly engine-generated deterministic
terrain; runtime screenshots and private probes remain local and unbundled.

## Extractor tick-context guard, 2026-11-03

The native-only author independently traced the installed loader and original
Steam image to write steam-mod-loader-tick-context.md, without SDK, engine,
exporter or capture access. The implementer used only that prose and owned code
for a small SDK-independent admission helper. Synthetic guarded-memory fixtures
verify read ordering and rejection without executing the game. The existing
owned safe-copy helper was moved to shared extractor scope; no SDK implementation
or full SDK class layout was introduced. The actual DLL still depends on the
previous SDK build and has not been live-validated after this change. No new
redistribution right or unproved teardown/cadence/readiness policy is asserted.
Owner data and private artifacts stay outside source and runtime packages.

## Live extractor validation and loader lifetime facts, 2026-11-03

The implementer validated the owned guarded DLL through Cube World's title menu,
existing request-file protocol and normal close. Two refreshed resources retained
their previous bytes, the owner volume comparator passed, and the process exited0.
Only that game's window was controlled; screenshots, owner resource copies and
logs remain private and unbundled. This was the existing modified installation,
not a fresh setup or proof of every native quit branch.

The separate native-only author wrote steam-mod-loader-object-lifetime.md from
the scoped original loader and independent prose only. The implementer read its
stable sections1–5 without implementation access. Array/record cleanup and retained
factory pointers are distinct facts; no loader-owned release, full SDK class,
eternal lifetime or safe external unload policy is inferred. No independent adapter
was implemented by this checkpoint, and the SDK permission gate remains open.

### Extractor frame-request trigger, 2026-11-03

The implementer reused the project's existing F10 frame-capture action behind a
fixed `Extract/ui.request` flag. This change consulted only owned extractor code
and Windows/D3D interfaces already used by that code; it added no native layout
or SDK implementation. Existing-owner live validation produced eighteen decoded
title-menu frames, including correctly queued batches, and a normal game exit.
The reference images and capture logs remain owner-local. This is capture-plumbing
validation with existing mods retained, not a vanilla pixel-parity claim or a
resolution of the existing SDK redistribution/revision gate.

### Independent primary sprite-catalog view, 2026-11-03

A native-only spec author traced the original catalog ownership, ordered pointer
range and raw dimension fields, then finalized
[the bounded registry note](specs/steam-native-sprite-catalog.md). The implementer
read that prose and owned extractor code, authored a small guarded byte reader and
malformed-memory fixtures, and compared fresh owner exports with the prior output.
All 3,652 primary rows and sprite groups were byte-identical; no original or SDK
implementation/header was read by the implementer. This is one independent data
view, not a complete class, exhaustive live registry or SDK replacement. Raw owner
images, records and the loaded-world modded generator results remain private.
Prior generator baseline files were restored exactly after the separate loaded-
world capture/light-reader check; that run is not a vanilla parity oracle.

### Sprite buffer view and capture provenance, 2026-11-03

The native-only author supplied steam-sprite-buffer-resources.md and qualified the
primary catalog's readiness wording without reading SDK, engine, tests, exporter
or owner data. The implementer read those completed prose contracts and owned
code only, then authored a guarded ordinary-backend field view and failure-path
fixtures. No original or SDK implementation/header was read or copied. Live native
buffer identification and exact loaded terrain samples use owner-local evidence.
Output routing is project policy: modified captures stay separate from vanilla
candidates, and a session label does not certify historical files. No generator
formula or expected value was changed to accommodate modified references. All
original images, records, traces, DLL backups and save data remain private.

### Independent active terrain-buffer view, 2026-11-04

The native-only author finalized steam-terrain-buffer-resources.md without reading
engine, SDK, exporter, tests or owner data. The implementer read the full prose and
owned code, authored a linked-list byte view and explicit ordered capture guards,
and extended the shared ordinary-wrapper reader. Allocation ceilings and malformed-
data rejection are capture policies. Tests use synthetic, noncontiguous pointer
fixtures; they are not a native runtime oracle. No original/SDK implementation or
class declaration was copied. The later loaded-zone phase supplies a bounded
supported-game integration smoke; other SDK-dependent views remain open. No owner
capture or original asset enters either package.

### Independent loaded-zone sequence and capture lease, 2026-11-04

The same native-only author supplied steam-loaded-zone-container.md without
implementation, SDK or owner-data access. The implementer read all five finalized
sections and owned code, then authored a bounded linked sequence reader and a
shared Win32 lease. Keys, bucket algorithms and complete classes were not copied
or reconstructed. Synthetic fixtures verify the +0x18 Zone payload distinction
and null-entry counts; actual Win32 sections verify contention and unwinding.
The shared byte-read arithmetic is original project code. After implementation
and CPU checks, the implementer ran an isolated, extractor-only native title smoke:
3,230 sprite buffers, 1,172 terrain buffers and six distinct presented frames.
The native-only author did not read these outputs. Owner Save/Mods/ReShade were
restored and hash-checked; only bounded integration is established. No owner data
or original asset is added to source/runtime packages.

### Independent loader callback record, 2026-11-04

The implementer used the full native-only loader ABI and factory-lifetime prose,
plus owned source, to author an explicit callback record and its neutral handlers.
It does not inherit an SDK class. Its complete numeric table and byte priorities
represent independently traced machine facts, not a reconstructed SDK declaration.
Priority0 for tick and 255 for unused events are original capture policy. Synthetic
host tests invoke every reached signature and check initialization, routing,
neutral results and unchanged caller state; own DLL exports are also inspected.
No original or SDK implementation/header was read or copied. The new integration
still needs native execution validation, and other SDK dependencies remain open.
No owner captures or original assets enter either package.

### Independent primitive Zone storage, 2026-11-04

The same native-only author finalized steam-zone-terrain-storage.md sections 1–7
using only native evidence and independent prose, then held. The implementer read
all sections and owned code before authoring a primitive identity/column byte view
and original CWZ1 encoder. Complete classes, caches, effective fallbacks and an
unproved inline theme header were not reconstructed. Resource budgets, malformed-
data rejection and metadata rechecks are project capture policy, not native limits
or a universal concurrent snapshot protocol. Opaque SDK path/blob metadata used
for provenance supplied no behavioral implementation information.

Synthetic fixtures leave unlisted caches nonzero, exercise changing/invalid spans,
and feed the original project's existing file consumer. No original/SDK header or
implementation was read or copied by the implementer. CPU/build checks pass;
new native integration remains unvalidated after the pre-launch control stop.
Owner Save/Mods/ReShade restoration was hash-verified. No original assets or
owner test data enter either package; other SDK dependencies remain open.

### Full item-copy naming boundary, 2026-11-04

The implementer used steam-inventory-refresh-events section 6 and owned code to
reuse the existing independent stand-in normalization in a complete 160-byte item
copy. Only category/subtype change; wrapped type, material, 32 attributes/count and
unlisted bytes remain. Naming performs that copy before choosing a branch and
retains the existing recursive formula rule. Tests use an invented dictionary,
compare full copied records and verify caller immutability. Five naming failures
were reproduced before the fix, then passed. No original assets, dictionary text,
SDK/header or original implementation was read or copied. Broader naming rules
remain subject to the separate native-only specification process.

### Full-name material and attribute audit, 2026-11-04

The native-only author finalized steam-item-full-name.md sections 1–7 without
engine, SDK, exporter, tests, owner data or execution access, then held. The
implementer read all 229 lines before changing the generic material fallback.
The existing own-material table and quality selector already matched the traced
ordinary boundary. The unused attribute tally supplies no dominant-spirit rule;
no visible suffix or override was invented. Original fragment/dictionary contents
remain owner data and were not included. Tests use synthetic definitions to check
unsigned 128..131 material bytes, generic keys, quality macro context and unchanged
output under different ordinary tallies. Three fallback regressions fail before
the correction and pass afterward. Full CPU/owner checks pass. Proper-name
family-specific data and native-output/visual acceptance remain unqualified.

### Generated proper names and fragment-data access, 2026-11-04

The native-only author finalized steam-generated-proper-names.md sections 1–7 and
held. The implementer read all 145 lines, including every 380-entry RVA/length fact,
before changing code. The owned shared R function supplies the exact unsigned
word hash; selector dispatch preserves original bits instead of collapsing table
aliases. Source includes numeric address/length metadata only. Literal characters
remain owner data. The new guarded reader replaces instruction scanning and keeps
legacy file order; a source/prose audit verifies all 380 tuples. The legacy owner's
output matches lengths and repeated-literal identity, with no text placed in the
report or either package. This uses output data, not original code inspection.

The 14 arithmetic examples are explicitly independent contract-derived fixtures,
not native runtime captures. New data-reader failure/format cases and full CPU/
owner checks pass. No original/SDK implementation or header was read by the
implementer. Native live/output, alternate builds, malformed/concurrent records
and broader upstream caller/grammar coverage remain separate boundaries.

## Controller creature-copy completions, 2026-11-04

The independent native-only author supplied steam-use-creature-collection.md,
including exact assigned fields and unassigned gaps, without reading the engine,
tests, exporter, SDK or owner outputs. The implementer read the complete finalized
prose, its existing constructor-default contract and engine sources only.
UseCreatureCollection and the prompt/runtime binding implement that bounded
contract with project-owned records and containers. Synthetic records check the
assignment complement, signed ordering, selection and actual prompt cut point;
no native implementation or game content is included. Defined project defaults
are not asserted to prove native padding initialization. Unknown transition,
persistence, other consumer, concurrency and large-count behavior remains open.

## Derived vehicle selection, 2026-11-04

The independent native-only author finalized steam-selected-vehicle.md without
reading the engine, tests, exporter, SDK or owner outputs. The implementer read
all 122 lines of that prose, existing item/coordinate/frame contracts and owned
sources only. NativeVehicle, the World/control/prompt bindings and shared ItemValue
assignment use that bounded contract. Synthetic inventory/object/block records
and an asset-free zero-time World facade check the actual producer and consumer.
No original game code, vocabulary or assets enter either package. The unresolved
native missing-zone fallback initialization is not assigned guessed flags: the
engine initially deferred new boat/glider candidates for an unavailable block,
retaining any active provisional selection. That checkpoint's replacement policy
is superseded for ordinary missing zones by the later effective-fallback audit
below; genuinely unavailable external providers remain distinct.

## Special-item query and unequip completion follow-up, 2026-11-04

The implementer used the finalized vehicle contract section4, creature-upkeep4.6
and inventory-refresh-events5 with owned code only. The existing raw-inventory
climbing consumer now keeps signed32 word accumulation; the actual upkeep test
compares no ownership, positive totals and wrapped overflow. The ordinary unequip
sound follows the existing independent completion order. No new native tracing,
SDK access, assets or inferred universal widget refresh was introduced.

## Effective fallback startup and empty columns, 2026-11-04

The native-only author finalized steam-effective-block-fallback.md and its empty
column supplement without engine, test, exporter, SDK or owner-output access.
The implementer read the complete 100-line prose and owned sources only. The
startup/data/lookup facts qualify the earlier missing-zone uncertainty; production
vehicle selection now uses the effective accessor's initialized returned record.
A real World-facade regression and twelve synthetic empty-column boundary cases
exercise that binding and the base-before-sea correction. Two former test
expectations were explicitly contradicted by the newly traced branch and were
corrected without dropping the independent implicit-water marking checks. The
six-byte defaults are compatibility facts, not bundled terrain/art assets. No
native instructions or implementation expression entered the engine. Native
writer/startup scope remains bounded as the specification states.

## Extractor item and creature views, 2026-11-04

The native-only author finalized steam-world-creature-views.md without reading
engine/exporter/tests/SDK, owner outputs or reconstructed implementations. The
implementer read its complete 121-line prose, the independent host ownership
identity and existing item/equipment contracts, then authored the views from
those facts and owned code. The item record is initialized project storage;
byte-preserving capture and native semantic assignment remain distinct.
Client/local links and signed ID resolution no longer obtain their layouts from
SDK classes. The observed native frame guard is held for the new consumers' use
scope, with no invented lifetime pin or universal snapshot claim. Registration
list/header/tree fixtures and actual Win32 guard tests are synthetic and asset-free.
No SDK implementation, headers or compiler layout was inspected. Remaining
SDK fields/containers, complete independence and new native acceptance stay open.

## Temporary Zone refresh lifetime, 2026-11-04

The independent original-only author supplied
steam-zone-refresh-reference-lifetimes.md. The implementer read the finalized
prose and owned exporter sources only. Refresh can register the Zone's address
in a World-owned notification tree; later consumption dereferences the Zone and
clears tree storage without destroying the referenced object. The Zone destructor
does not detach pending notification references. Initial-generation full escape
lifetimes remain unproved, so no temporary native Zone destructor is introduced.
The 32-attempt process budget and bounded, all-or-nothing request admission are
explicit replacement-tool policies, not native allocation or quiescence claims.
The wrapped unsigned 32 generation seed follows worldgen-terrain-zone.md section 2.
Asset-free coordinate, malformed-input, admission and boundary fixtures qualify
these policies; original-game live execution remains a separate acceptance gate.

## Inventory row and region request ordering, 2026-11-04

The implementer read the independent steam-inventory-refresh-events section 4.2,
steam-inventory-grid-layout section 4 and owned engine code only. Inventory,
crafting and shop row requests now refresh/sort the local canonical inventory
before calculating the available row count and applying one directional request.
Separate input visits are not summed into a cancelled or multirow request.
A downward clamp performed by refresh is not itself a requested row movement.
Region arrows request sound first, change the selected region when found, add
next-region's explicit canonical sort, then run refresh and the thumb stage even
when no choice matches. The extra sort avoids redundant presentation rebuilding.
Synthetic post-refresh, boundary, opposing-input and ordered-callback fixtures
exercise the shared production helpers. Event handlers now place the actual
scrollbar using their refreshed range; the shared helper retains its local width
before moving/resizing it. Full cached-range/thumb lifetimes and live callback/
hardware acceptance remain separate unfinished work. No native source or game assets were
read or added to the implementation or packages.

## Generation request context and preservation, 2026-11-04

The context gate, sticky pause and immutable request snapshot are original tool
policies. The implementer used the already independently specified World metadata
fields and owned extractor sources only. No native generation algorithm, source,
SDK or additional native code was read. The copied World address is a comparison
key, not ownership or a lifetime pin. Checks occur around the native operations;
no atomic native snapshot, rollback, full escape proof or exception cleanup is
inferred. Pending requests/coordinates remain available after admission, context
or native-operation failures. Input stays exclusively held until exact-file
consumption; a producer cannot replace it after reading and lose a newer request.
Asset-free state-machine checks and actual isolated
Windows publication/no-overwrite fixtures exercise this policy. No game assets
or captured output enter either package; live supported-game acceptance is separate.

## Pickup notice quantities and counters, 2026-11-04

The independent original-only author supplied
steam-pickup-quantity-consumption.md. The implementer read that final prose and
owned code only. A shared owned decoder now retains the notice's unsigned16
quantity, complete flags32 and signed positions; the gameplay bridge zero-extends
that word instead of passing negative signed16 values. The ordinary producer's
word copy remains byte-equivalent. Regional and direct item-value gold additions
use defined low32-bit sums; the native receipt path passes notice quantity without
an extra unit-value multiplier. Item-derived gold still applies once independent
of quantity. Packet-to-receipt/inventory tests cover high words, poisoned gaps,
flag/position retention, zero quantity, wrapped counters and distinct gold paths.
No native instructions, SDK or reconstruction code were read or copied by the
implementer, and no original assets enter either package. Live multiplayer and
broader presentation/RNG acceptance remain separate.

## Crafting count arithmetic, 2026-11-04

The independent original-only author supplied steam-crafting-collection-rebuild.md.
The implementer read that final prose and owned engine code only. Formula counts
retain signed division and the actual negative-candidate-as-unset test, with empty
ingredients zero and no general final negative clamp. The live count query uses
the actual canonical inventory's wrapped full-identity total; its legacy sum also
uses defined low32-bit addition. Zero-divisor and signed-division-overflow inputs
have a defined engine fallback, explicitly outside a qualified native successful
return. Inventory-backed negative/overflow/order tests exercise the count path.
This arithmetic change does not claim persistent derived-collection, selected-
button lifetime, native RNG-role or full GUI callback completion. No native code,
SDK or reconstruction was read by the implementer; no original assets are bundled.

## Inventory region-choice cache, 2026-11-04

The independent original-only author supplied steam-inventory-region-choice-cache.md.
The implementer read the final prose, previously finalized event/layout contracts
and owned engine sources only. The owned value cache rebuilds from the canonical
carried tab on bound inventory refreshes, preserves skipped/non-inventory updates,
includes nonzero negative counts and the player region, and does not repair the
separate selected region. Invalid engine tab headers defer safely; no native
successful invalid access is asserted. Native-backed drawing consumes this cache;
recording-only compatibility remains separate. Ordinary collection assignment now
runs independently of panel visibility and does not replay skipped refreshes.
Tab changes refresh before selection/thumb work, with the shop's second rebuild
stage afterwards. Synthetic lifecycle/quantity/order fixtures and the full CPU/
owner suite plus102 Vulkan GUI images pass. Complete frontend/persistent binding,
derived collections, cached row/range state and live callback/hardware acceptance
remain open. No native source, SDK, reconstruction or original assets were added.

## Retained crafting collection and recipe publication, 2026-11-04

The same independent original-only author supplied the final
steam-crafting-selected-recipe.md after inspecting only original native evidence.
The implementer read that prose, steam-crafting-collection-rebuild, the earlier
refresh/down-tail/station contracts and owned engine sources only. Collection
storage owns count/item groups and a separately copied full selected recipe.
Generation inputs are temporary; matching publication retains owned ingredients,
unmatched/skipped rebuilds preserve state, and the specified left-down tail clears
invalid/failed publication without repairing the independent list selection.
Native-backed drawing reads retained rows/details, with live ingredient queries;
tab changes alone do not republish. Connected rebuild events replace per-draw
recipe generation/counts. The active craft copies details at its start and hiding
clears that action value without destroying the details recipe. Legacy recordings
retain their separate compatibility path. Synthetic canonical inventory/count,
copy/reset and actual owner-GUI physical binding checks exercise the implementation.
Full native shared RNG state, callback closure, cached range/thumb/eligibility,
tab6 target actions and persistent frontend lifetime remain open. No native code,
SDK/reconstruction or original assets were read or added by the implementer.

## Cached inventory range and thumb state, 2026-11-04

The implementer used the already finalized steam-inventory-grid-layout,
steam-inventory-refresh-events, region-choice and crafting contracts plus owned
sources only. A separate range stage owns A and dynamic tab offsets, with source/
column skip preservation, physical-empty and regional-zero distinctions, current
row-only downward clamping and defined low32 scalar arithmetic. Bound bag/crafting
draws read state without refreshing or placing the thumb. Connected event pairs
place it from retained A; null/missing headers and A0 preserve old geometry.
Invalid engine indices and native division-fault inputs use explicit safe engine
deferral rather than asserting successful native invalid access. The actor's
actual stored region feeds choice refresh, bag opening and crafting generation;
missing actor storage defers, and drawing does not invent an initial selection.
Synthetic state/arithmetic fixtures and actual owner-GUI stale-range checks
exercise the production helpers. Complete frontend/geometry callback closure,
shop state and hardware/live visual acceptance remain outside this result. No
original source, SDK, reconstruction or owner assets were added to either package.

## Retained shop collection and shared insertion, 2026-11-04

The independent original-only author supplied steam-shop-collection-rebuild.md.
The implementer read the finalized note, prior inventory refresh/slot contracts
and owned code only. Stock and history rebuilds replace two owned row groups;
reverse history copies preserve order, duplicates and quantities, while generated
stock shares the existing independently implemented row insertion. Canonical
counter interception and tab selection remain outside that pure insertion helper.
Native-backed drawing regenerates presentation from retained rows, not stock.
Connected opening/tab/transaction rebuilds preserve refresh/selection/thumb order,
clear inactive/missing-merchant stock and retain the selected tab/pair. Bound shop
range/offset/thumb uses the existing event cache. Source buffers are released on
rebuild; value copies keep engine readers safe without claiming native borrowed-
pointer/race equivalence. Missing actor data has a defined engine-safe empty stock
policy rather than a native successful-null-source claim. Shared RNG and complete
callback/geometry/frontend lifetime remain open. Synthetic insertion/history,
canonical inventory and actual owner-GUI binding checks exercise the implementation.
No native instructions, SDK, reconstruction or original assets were added.

## Crafting control lifecycle and tab dispatch, 2026-11-04

The independent original-only author supplied steam-inventory-tab-callback-dispatch
and steam-crafting-tab-control-lifetime after original-evidence readback. The
implementer read only those finalized notes and owned sources. Initial four blank
roots and explicit class-dependent replacement now have separate stages, with
owner resource identifiers and unchanged selected/range state. The completed
character setup also requests the retained crafting rebuild. The draw-time icon
observer and its unused class fields were removed; controller wrap uses actual
logical controls. Manual tab/region/recipe selection is restricted to left-down.
The older refresh-events prose's release label is corrected from the final direct
key2/Windows0x201 proof, without reading native instructions. Asset-free class/icon
contracts and actual owner-scene repeated replacement checks qualify these changes;
full callback/capture/hover/frontend and hardware acceptance remain separate. No
original code, SDK/reconstruction or game assets were added.

## Retained formula tooltip publication, 2026-11-07

The isolated source-only author finalized steam-preview-selection-20261107.md and
its ingredient-admission addendum, then read them back and held further writes.
The author used Infinite-Cubes only as a research lead for the main contract and
confirmed stated Steam behavior against the original supported image; the
addendum used original evidence only. It reports no prohibited implementation
exposure. The clean implementer read both entire held prose contracts and owned
engine/test sources only. Their accepted prose is retained under docs/specs.

The formula panel now owns a stable semantic item copy published by its real draw
callback after result-resource admission. Result and ordered ingredient hits share
that copy; ingredient-resource absence does not suppress its publication. Hidden,
invalid-presentation and missing-result draws preserve it, while an admitted miss
clears category only. The ordinary selector borrows it while the formula panel is
locally shown, independently of formula presentation validity, and the information
callback reads the still-live item after earlier draw callbacks. Removed the old
per-frame ingredient-only selector scan. The separate legacy bitmap presentation
remains supported. Synthetic items in the actual owner GUI scene exercise this
bounded behavior; the focused checks reproduced nine failures before the fix and
pass afterward. Exact native update/draw latency and unproved skill/specialization
publishers remain unresolved. Alpha defaults and extensions are unchanged. Final
provenance and wider live Steam parity acceptance remain release gates.

All five targets build. Full CPU/owner-data validation with required procedural
oracles passes without skips. All Vulkan groups pass (92 shader, 2 camera, 102 GUI,
17 map, shared water and Alpha scenery), and the extractor SDK-independence audit
passes. Local evidence: build/formula-tooltip-validation. These regression results
do not establish complete live native tooltip or visual parity.

## Controller sampling bypass and retained axes, 2026-11-07

The same isolated source-only author independently finalized and held
steam-controller-aim-20261107.md after complete readback. It uses Infinite-Cubes
only as a discovery aid and grounds its stated behavior in original Steam
executable evidence. No prohibited implementation exposure was reported. The
clean implementer read the entire held contract and owned engine/test sources
only; accepted prose under docs/specs preserves the held text.

GameInput now bypasses its normal binding/pad refresh on lost foreground or
binding capture, retaining combined sampled held state. The per-frame press-edge
adapter discards its delivery flags rather than replaying an earlier edge.
GamepadFrame's explicit skip path retains held contributions, movement/turn axes
and the B latch while clearing its transient dismissal request. Successful polls
rebuild controller contributions; failure neutralizes both stick pairs. Ordinary
panel diversion zeroes movement but preserves turn without a temporary copy.
These paths implement the independently described state distinctions rather
than inferring axis ownership from the separate last-device indication.

Stateful production-facade tests exercise cold and repeated bypasses, retained
camera input, panel diversion, failure/reconnect, map publication and B-latch
survival. Six checks reproduced the former reset error and pass after the fix.
Full original global-active-input producers, consumer/callback scheduling and
hardware/foreground/capture comparisons remain open. No native or reconstruction
source was read by the implementer. Alpha terrain, lighting, Omega water and the
other requested extensions are unchanged; final provenance and wider live Steam
parity acceptance remain release gates.

All five targets build. The full CPU/owner-data suite with required procedural
oracles passes without skips, as do all Vulkan groups (92 shader, 2 camera,
102 GUI, 17 map, shared water and Alpha scenery). The extractor SDK-independence
audit also passes. Evidence: build/controller-input-validation. These checks do
not substitute for the remaining native hardware and caller-schedule acceptance.

## Mouse-mode handoff and source-context replacement, 2026-11-07

The isolated source_feat_contract author finalized/read back/held
steam-mouse-mode-admission-20261107.md. The clean implementer read the complete
held prose. During a subsequent held-mask task, that author reported that a broad
reference search returned excluded third-party SDK/header matches. It stopped
immediately, relayed no matched contents, and produced no held-mask addendum.
That context is excluded from further source work. Its earlier held contracts
were unchanged; this records a later source-side exposure, not an assertion that
ignoring exposed material restores separation.

A fresh source_mouse_mask context, without the implementation chat or the stopped
author's context, read the standalone role policy and original Steam evidence
only. It finalized/read back/held steam-mouse-held-mask-20261107.md and the separate
steam-map-pan-yaw-20261107.md. It reports no prohibited exposure. The implementer
read both entire held contracts before using their facts. Accepted prose under
docs/specs matches the held text; local provenance hashes are recorded under
build/cleanroom-research/mouse-handoff-provenance.json. The author policy now
requires filename discovery and explicitly scoped content searches, avoiding
mixed reconstruction/dependency roots. The final provenance audit retains this
incident and the replacement boundary.

The implementation uses owned code only. The input owner stores one held mask;
actual window edges and admitted controller callbacks modify its individual
bits. Native-backed UI consumes window edge snapshots, while configured action
polling retains its existing GLFW adapter. Controller left presses enter shared
capture/listener routing and keep their recorded identity through inventory
continuation. Ordinary mouse publication and motion now precede XInput work;
focus/binding-capture skips do not supply a substitute motion call. Pointer
publication remains admitted while the root is hidden, with capture or a fresh
root pick deciding camera admission. Shared routing separates relative arguments
from cached GUI displacement, retains entry-versus-later ownership, and permits
successive map/model/camera branches. Map panning now receives current yaw narrowed
to float degrees directly, replacing a legacy 90-degree adapter at both callers.
The existing diagnostic pointer also feeds the retained in-game input owner.

Focused production-math and actual owner-GUI fixtures exercise simultaneous
routes, stationary versus skipped calls, shared per-bit mask updates across
window/controller sources, focus-loss release exclusion, cached displacement,
hidden-root capture, and controller scrollbar capture. Broader active-input and
mask writers, original cursor preparation/warp and coordinate-option behavior,
right/middle widget callbacks, complete Game/UI callback scheduling and real
physical-device acceptance remain open. Alpha terrain, lighting, Omega water and
other enhancements remain configured. No excluded contents or owner game files
are included in the implementation or source publication.

All five targets build. The full CPU/owner-data suite with required procedural
oracles passes without skips. Vulkan checks pass for 92 shader, 2 camera, 102 GUI,
17 map, shared water and Alpha scenery cases; extractor SDK independence passes.
An isolated hidden/offline/no-save Alpha-world system-menu run with a synthetic
pointer exits zero and renders the menu (build/mouse-input-smoke). This exercises
the in-game service binding and retained pointer stage, not native hardware or
pixel acceptance. Full regression logs are in build/mouse-input-validation.

## Secondary-button dispatch, 2026-11-07

The clean source_mouse_mask author read the updated standalone author policy and
individually selected original Steam evidence only. It finalized/read back/held
steam-secondary-button-dispatch-20261107.md, reported no prohibited exposure and
left earlier held files unchanged. The implementer read the entire final prose
before implementation; the accepted docs/specs text matches it. Original event
keys, virtual identities, field distinctions and direct operation order are facts;
no native source, reconstruction, SDK or callback body was read by the implementer.
The earlier source_feat_contract context remains quarantined.

The original mask-only adapter was first extracted into named secondary entry
points without adding dispatch. Native-key trace/state tests then reproduced 26
failures. The completed owned input path updates the held bit, selects capture or
a fresh pick, performs down-only focused-object dirty/key19/null cleanup, invokes
the associated owner's virtual hook, rereads registered-callback admission and
attempts its button-specific global traversal. Focus cleanup is shared with the
existing left-down path without adding the left-only auxiliary/qualifying-owner
steps to secondary buttons. Secondary releases neither force capture clearing
nor synthesize the left release's outside-owner leave. Window and controller
adapters call the same right/middle dispatch methods.

Tests cover native numeric registration keys, ordering, live owner/enabled/style
changes after callbacks, own versus associated widgets, passive/disabled capture,
ownerless capture, callback-induced nested movement, focus replacement before the
outer null store, preserved auxiliary state and release without matching down.
Those focused checks pass after the correction. The new global hook identities
are kept separate from registered keys; unproved global registration numbers or
callback body effects were not invented. Dynamic widget behavior, arbitrary tree
mutation/lifetime guarantees, complete Game/UI scheduling and physical-device
acceptance remain open. Alpha defaults and enhancements are unchanged.

All five targets build. Full CPU/owner-data validation with required procedural
oracles passes without skips. All Vulkan groups pass: 92 shader, 2 camera,
102 GUI, 17 map, shared water and Alpha scenery. The extractor dependency audit
passes without CWSDK. Evidence: build/secondary-button-validation. These regression
results do not establish native physical-device or complete widget-body acceptance.


## Ordered physical wheel delivery, 2026-11-07

The existing clean source_mouse_mask author reread the standalone role policy and
selected original Steam files individually. It finalized, fully read back and
held steam-gui-wheel-dispatch-20261107.md, reporting no prohibited exposure. The
implementer read the complete finalized contract before implementation. Accepted
text under docs/specs matches the held text; the private hash receipt is under
build/wheel-input-validation/handoff.json. Earlier held files remain unchanged,
and the quarantined source_feat_contract context remains excluded.

Original evidence proves direct WM_MOUSEWHEEL-to-Game routing, signed-high-word
normalization, retained-pointer subtree selection, five-panel priority and the
message-before-pointer/device/XInput order. No generic GUI wheel virtual slot,
registered key, global notification or callback packet was inferred. The broader
Widget/Button wheel bodies and arbitrary reentrant/coalescing/lifetime behavior
remain unproved; this continuation does not close those boundaries.

The owned adapter now puts normalized wheel values in the same ordered queue as
actual button and F4 messages. Each reached wheel dispatch rereads the retained
GUI pointer before the next ordinary pointer/device stage, without using message
coordinates or a newly sampled/window-scaled cursor. Zero values remain deliveries.
Inventory/shop/crafting use the stored node's own visibility and a fresh subtree
pick, without capture override, ancestor admission or layout/visibility repair.
An eligible panel consumes even zero or a row-boundary attempt. Existing Controls
and Friends checks follow those panels, before the existing mapped-Walk camera
or map-zoom arithmetic. The same row refresh/sound/thumb callback is reused at
physical delivery and by the existing later controls; the wheel-specific replay,
duplicate vector and unused accumulator are removed.

Stateful owner-GUI fixtures reproduce six ordering/zero failures at the extracted
former batching boundary and pass after correction. They check interleaved held
bits and visibility, live retained-pointer changes, signed16 extremes/magnitudes,
zero/repeated deliveries, clearing, hidden ancestors versus own-hidden panels,
and scrollbar capture versus independent fresh wheel picks. The general legacy
panel-eligibility helper remains separate; no broader controller or layout rewrite
is inferred from this bounded wheel contract. Full Game/UI business-callback
scheduling, actual hardware/DPI/focus acceptance, unproved buffering/reentrancy
and final provenance/wider Steam parity remain release gates. No original-game
run, owner capture, save/mod/display mutation or content publication was needed.
Shipped Alpha terrain/lighting, Omega water and enhancements are unchanged.

All five targets build. Full CPU/owner-data validation with required procedural
oracles passes without skips. Vulkan groups pass: 92 shader, 2 camera, 102 GUI,
17 map, shared water and Alpha scenery. Extractor SDK independence also passes.
Focused guiitems, guiinput and padui checks pass. Evidence is under
build/wheel-input-validation; these are bounded regression checks, not native
physical-device or complete Game/UI callback acceptance.


## Native water view-space shader inputs, 2026-11-07

The clean source_mouse_mask author independently resolved the original water
vertex/pixel blobs, their bindings and arithmetic using original Steam evidence.
It finalized/read back/held steam-native-water-shader-20261107.md. The implementer
read the entire final prose. A conflict with earlier independent pass/control
contracts was identified before any pass-state change. The same isolated author
rechecked original assembly, concrete vtable cells, wrappers, buffer destinations
and embedded resource records, then finalized/read back/held the separate
steam-native-water-pass-state-addendum-20261107.md. The implementer read it fully.
The addendum supersedes the main contract's mistaken White/DepthOffset mapping:
water requests White0/Glow0 and retains DepthOffset. Existing controls agree with
that correction; they were not changed on the mistaken claim. Both held files
remain immutable and accepted text matches them. The private receipt/hashes are
in build/native-water-validation/handoff.json. The author reports no prohibited
exposure; Microsoft tool-usage documentation was used only to operate the original
bytecode disassembler under the assignment's explicit permission. The earlier
source-context quarantine remains in effect.

Synthetic anchored geometry reproduces two water-only color failures at positive
and negative 16,777,216-block origins, while matching ordinary-WORLD geometry
controls pass. The owned shared vertex stage now retains a full native-basis view
normal, transformed directional vector and view position, plus the existing
homogeneous clip varying. Native water consumes those values for screen gradients,
reflection, distance, fog and opacity, retaining the shared vertex-lit blue and
optional engine shadows. It no longer reconstructs native reflection/fog distance
from rounded source-home world floats. The legacy vertex fallback and Alpha/Omega
paths remain distinct. The water pixel arithmetic is factored into one owned GLSL
helper used by the production fragment shader and an asset-free compute probe.
The native branch also avoids the unused legacy fragment-light calculation.

The isolated source-side arithmetic references are copied as explicit inputs and
expected raw RGBA values. The actual shared GPU helper passes all three within
0.00001 per component, before blend/UNORM conversion; these values were not fitted
to rendering output. Positive/negative fixed-origin images now match their near
reference exactly. Real zone-table tests verify first-ten point-slot selection and
registered vertex point lights. CPU checks distinguish sampled ordinary water
V254 from whole-sea bypass V255 and retain fully occluded water. The compute probe
uses coherent host memory, a compute-to-host dependency and a fence following
[Khronos readback guidance](https://docs.vulkan.org/guide/latest/synchronization_examples.html).

All five targets build. Full CPU/owner-data validation with required procedural
oracles passes without skips. All Vulkan groups pass: 92 shader, 2 camera, 102 GUI,
17 map, 10 native-water checks, shared water and Alpha scenery. Extractor SDK
independence passes. Evidence: build/native-water-validation. These checks do not
certify native GPU captures, nonfinite inputs or cross-GPU last-bit behavior.
Remaining water work includes actual runtime local mesh-group submission, empty/
incoming pass-state timing and publication/lifetime acceptance. Broader rendering,
hardware and final provenance/Steam parity remain release gates. No original game,
owner capture or save/mod/display mutation was needed. Alpha terrain/lighting,
Omega water, shadows and the other enhancements remain configured.


## Native water local groups and publication evidence, 2026-11-07

The clean source_mouse_mask context independently inspected scoped original Steam
mesh construction, active/staging lists, concrete buffer wrappers, worker and
ordinary draw guards. It finalized, fully read back and held
steam-water-mesh-publication-20261107.md with no prohibited exposure. The clean
implementer read the entire held prose before use and accepted matching text
under docs/specs. Hashes of this contract and the earlier shader/state-correction
contracts are recorded in build/water-band-validation/handoff.json. Earlier held
files remain immutable and the quarantined source_feat_contract is not reused.

Owned meshing now routes ordinary water and wet-ground films into the same
lowest-relative 250-block bands as solid geometry. Whole-sea water uses the band
containing Z0, native height1 and direct V255. Water-only bands survive upload,
carry fixed native anchors and their owning zone light key, contribute to cost,
and release through the existing fence-aware renderer path. Optional lilies
retain sea-first/cell/layer visitation, duplicate admission and the zone cap;
Alpha aggregate decoration stays on its existing route.

Synthetic regressions reproduced three failures on the prior aggregate route.
Focused checks now pass for unit corners/top250, packed colors/topology, a
water-only band, omitted middle bands, source-home displacement16777216, a sea
quad above a negative anchor, and actual asynchronous World publication, draw
collection and retirement. Lily callback/output equivalence is checked across
caps/densities and repeated cells. These are owned-fixture regressions, not
native image or timing acceptance. Remaining work includes wholly empty staging
preservation, full native task/retirement lifetimes and incoming matrix timing.
The source contract explicitly leaves device-error/cancellation/GPU completion
unproved; the engine retains its safer Vulkan resource ownership. No original
game run, owner capture or save/mod/display mutation was needed. Alpha terrain,
lighting, Omega water, shadows and other enhancements remain configured.

All five targets build. The full CPU/owner-data suite passes with required
procedural oracles and no skips. Vulkan checks pass: 92 shader, 2 camera, 102 GUI,
17 map, 10 native-water, shared water and Alpha scenery. Extractor SDK independence
passes. Logs and private receipts are under build/water-band-validation. These
checks establish the bounded regressions above; native rendering and full
publication/lifetime acceptance remain open.


## Native empty-staging publication and zone lifetime, 2026-11-07

The same isolated original-only source_mouse_mask author traced the ordinary
World instance, mesh/eviction worker phases, concrete thread launch and joins,
guard intervals, raw snapshots, erasure/destruction and the distinct linked
alternate World. It finalized/read back/held
steam-zone-retirement-lifetime-20261107.md with no exposure. The implementer read
the ENTIRE held prose before accepting matching text under docs/specs. Earlier
held files remain immutable. The new and prior publication/shader/state contracts
are hashed in build/terrain-publication-validation/handoff.json; the stopped
source_feat_contract context remains quarantined.

The held publication contract's nonempty-whole-staging admission now preserves
old active GPU groups on a successful empty Steam result. Fresh bounds/cost,
light inputs and ordinary scenery remain independent; optional decorations keep
their active water. Nonempty groups lacking water replace old water normally.
Mesh-source failure is a separate explicit result and publishes neither geometry
nor cache metadata. Alpha's successful-empty replacement behavior stays separate.
The engine uses immutable shared source snapshots and its own fence-aware GPU
retirement. Native raw pointer copies, possible alternate erasers and unproved
concurrent insertion/shutdown/error behavior are not adopted as safe guarantees.

Seven public-runtime regressions fail before this correction and pass afterward.
The owned fixture covers repeated wholly empty results, retained solid/water
handles with fresh zero-cost metadata, nonempty replacement, initial emptiness,
failed source read and explicit repaired-source retry in both styles. Four GPU
checks use actual World jobs/uploads and fixed camera/palette: visible water,
repeated empty rebuilds spanning fence reuse, solid/water replacement and retired
buffers. The initial GPU visibility control was inconclusive under the frozen
constructor-blue sky; settling the palette before freezing makes it informative.
This was a fixture correction, not a shader/engine fix or native pixel match.
No original game run, owner capture or save/mod/display change was required.
Exact worker batch cadence, zero-cost reselection, alternate lifecycle paths and
broader native visual/gameplay/final provenance acceptance remain release gates.

The original-only author also independently finalized/read back/held
steam-water-incoming-matrix-timing-20261107.md. The implementer read it ENTIRELY
before accepting matching text. Concrete setter/binding histories prove the
ordinary pre-water mode2 consumers preserve the frame VIEW/projection, which
therefore reaches depth; water color explicitly reinstalls that same pair, even
for empty lists. The current ordinary path agrees and no matrix code change was
needed. Alternative callers/flags remain unresolved. The private receipt now
hashes five held contracts. No prohibited exposure was reported.

All five targets, full required-oracle CPU/owner-data suite without skips and
all Vulkan groups (92 shader, 2 camera, 102 GUI, 17 map, 14 native-water plus
shared-water/Alpha) pass. Extractor SDK independence passes. Evidence:
build/terrain-publication-validation. No original-game or owner-state mutation.


## Native mesh selection and bounded batch execution, 2026-11-07

The isolated original-only source_mouse_mask context finalized/read back/held
steam-mesh-invalidation-selection-20261107.md and the separate
steam-mesh-worker-concurrency-20261107.md. The implementer read each ENTIRE held
file before accepting matching text. They identify raw candidate order, signed
prefix budget, zero-cost and invalidation gates, complete-neighborhood-before-cap
order, the eight-item batch, normal completion and publication phases, and the
CPU-setting change branch. Both report no prohibited exposure; all earlier held
files remain immutable and the quarantined source_feat_contract remains excluded.
The private hash receipt is build/native-preparation-validation/handoff.json.

Owned PreparationBatch and World code now use those bounded admission and
completion facts. A small owned TaskPool reuses bounded worker lanes, drains
queued work and joins before World inputs are released. Native results publish
in selected order after every task returns, before another terrain retirement/
selection phase. Independent object/model frame updates remain separate. Vulkan
uploads and deferred fence-aware destruction stay on the engine's existing path.

Known zero-cost results remain eligible without an invented completed-empty latch.
Required work/dirty requests are distinguished from clean refreshes for capture
and benchmark readiness. Incomplete owner captures with unknown native cost stay
on their cached fallback policy. Alpha retains its own async jobs and six-success
integration cap; discarded/failed results do not consume that cap.

The concurrency addendum specifically does NOT establish an initial native max8:
the default raw preference1 matches the worker's remembered1 and skips explicit
scheduler creation. On later changes, zero/nonzero configure max4/max8, min1.
Our hardware-based initial lane count is explicit engine policy, bounded by those
later caps; native runtime priorities/attachment-stack behavior are not copied.
The original final invalidation clear can overwrite a mark made earlier during
meshing. Our existing queue retains edits arriving while immutable snapshots run.
That safety adaptation is not a claim of identical native dirty-byte races.
Per-zone render interleavings, exact batch/cold-streaming and source-eviction
cadence, alternate writers/cancellation/shutdown and final parity remain open.

A public-API owned-hook fixture holds one source reader while siblings finish,
reproducing two prior failures: partial publication and early retirement. Both
pass after the change, including the actual8-coordinate batch and preference
changes during it. Independent literal cases cover prefix-boundary accounting,
nonzero dirty work over budget, unselected signed costs, incomplete neighbors,
and all neighbor calls beyond the cap. Existing CPU terrain/publication/object
fixtures and fourteen native-water GPU checks pass in focused runs. No original
game was started or owner Save/Mods/display modified.

Baseline private own-engine profiles are bounded and do not certify native FPS:
minimum-distance Steam terrain/Native water exits0 in14.531seconds including10s
play; default-distance Alpha terrain/Steam rendering/Omega exits0 in17.234seconds
including10s play. The latter's separate full-fill capture reaches its observation
deadline and exits1 at128.234seconds with433 prepared zones and3 pending jobs,
without a capture. This distinguishes bounded startup from settled cold-fill
acceptance. Logs are under build/native-cold-profile, build/alpha-steam-cold-profile
and build/alpha-steam-fill-profile. Alpha/Omega shipped settings remain unchanged.

All five targets and the full required-oracle CPU/owner-data suite pass without
skips. Vulkan groups pass: 92 shader, 2 camera, 102 GUI, 17 map, 14 native-water,
shared water and Alpha scenery. Extractor SDK independence passes. Final logs
include the restored Alpha successful-integration cap.

A matching hidden/offline/no-save Alpha/Steam/Omega full-fill observation still
reaches its deadline: 128.422seconds, 384 prepared zones and8 queued/running
coordinates, no capture. The baseline was128.234seconds, 433 prepared and3 running
coordinates. The pending counts describe different batch representations. This
single comparison establishes no speedup and leaves cold-fill performance open.
Evidence: build/native-preparation-validation/cold-profile-comparison.json and
build/alpha-steam-fill-after-preparation. No original game was run; owner saves,
mods and display settings were not changed. Shipped Alpha/Omega defaults remain.

## Ordinary mesh-time model samples, 2026-11-07

The unexposed original-only author source_mouse_mask finalized and held the
source-model light-stage contract. The implementer read the complete prose,
retained its original-byte hash privately, and changed owned engine code only.
All ordinary records receive their cached scalar and captured-neighborhood water
flag in mesh preparation; draw filtering and static-object live queries remain
separate. Immutable replacement is an explicit engine ownership policy, without
a native atomicity claim. The bounded regression reproduced 34 initial failures;
focused and full required-owner CPU, Vulkan and SDK-independence checks pass.
Auxiliary lifecycle integration and broader original-runtime acceptance remain
separate work. No original code or owner data is included in source packages.

## Auxiliary source-model lifetime, 2026-11-07

The same unexposed original-only author finalized a separate auxiliary-model
lifetime contract. The implementer read it entirely, held its original hash and
used code-free prose with owned implementation only. Auxiliary models retain
independent value-copy storage; meshing prepares their cached light and water
flags, and point registration reads them until a later mesh or new residency.
The archive reader shares one bounded load pass. Immutable leases and incomplete
replay handling remain explicit engine policies. Seven prior lifecycle failures
and additional independence/reuse checks exercise this change; broader native
visual, alternate-writer and concurrent-lifetime acceptance remains separate.

## Host source-loader cycle, 2026-11-07

The unexposed original-only author held a separate host cycle contract. The
implementer read it entirely with the earlier held priority/initial-byte contracts,
recorded their original-byte hashes privately and used owned code only. A dedicated
host source worker now performs required-set retirement and serial cap-four
requests with independent lifetime and raw generation. Controlled source-worker
and raw-byte checks, full required-owner CPU/Vulkan validation and SDK audit pass.
The Alpha/Omega scene capture passes; the native default-distance observation
still times out and remains open. Immutable leases, latest published centres and
exception handling are engine policies. Full retained fields, logical residency,
periodic persistence and broader original-runtime acceptance remain separate.

## Native background fringe and capture diagnostics, 2026-11-07

The implementer used the entire held source-retention and generation-order prose
with owned code to separate required source work from background capture waits.
The diagnostic classification is an engine policy; native candidate sets, sorter,
batch submission, retirement and player-provider call counts remain unchanged.
A controlled real-world failure now passes, together with cold/changed-context
checks and full required-owner CPU/Vulkan/SDK validation. Default-distance origin
and negative-half-block captures produce viewed ground images; original pixels,
generation speed and broader runtime parity are not inferred from those images.

## Retained source helper flags and gates, 2026-11-07

The isolated original-only author supplied the complete held five-field lifetime
contract without replacement-code, SDK or fixture exposure. The implementer read
that prose and owned engine sources only. Saved ground overrides now restore
their sticky flag; the restoration gate no longer suppresses later unload
recording. Existing five-field recording order was retained. Seven reproduced
helper failures and nonempty/empty local host replies exercise this bounded fix.
Physical source binding, wider synchronization and native persistence remain open;
these checks do not establish full source-lifecycle or network parity. The private
receipt freezes the original held bytes; owner content is not published.

## Physical source request control, 2026-11-07

The isolated original-only author supplied the held F8 control and state-reply
mapping contracts without replacement, SDK, fixture or owner-output exposure.
The implementer read both complete prose files and owned engine sources only.
Physical source identities now own request control across COW, clears, reset and
reload; logical lists no longer own a duplicate request-key set. Host replies
require a physical source lease. Nine reproduced failures and selected integration
checks exercise these paths. Broader connection timing/writers, full retained
payload binding and synchronization remain open. The private receipt freezes both
held hashes; no owner data is published.

## Source prerequisite summary producers, 2026-11-07

The isolated original-only author independently supplied the held summary
producer/consumer contract. The implementer read the complete prose and owned
sources only. Catalog membership now uses the World+0x20 lane instead of admitted
live actors; the separate tracked-list scalar is rebuilt independently. One
reproduced generated-gate failure and bounded value checks pass with full required-
owner CPU, Vulkan and SDK checks. The older guard prose's “live” wording is not
used as evidence of resident-map membership. Physical ownership/freshness,
consumer routing and ordinary update/admission order remain open. The private
receipt freezes the held bytes; no owner data is published.

## Live-resident distance retention, 2026-11-07

The isolated original-only author supplied the held resident-retention contract;
the implementer read its complete prose and owned sources only. The engine uses
the inclusive horizontal disk and World-order candidate selection, with unique
owned IDs preserving selection across vector movement. Source demand no longer
erases nonempty resident buckets. Boundary/owned-world checks and final required-
owner CPU, Vulkan, SDK and two scene smoke checks pass. Two unpositioned respawn
fixtures now explicitly place their actors near their test player; the engine
rule was not relaxed. Complete phase ordering, source-state binding, external
aliases and broader travel/memory acceptance remain open. The private receipt
freezes the held hash and no owner data is published.

## World-step admission and movement selection, 2026-11-07

The isolated original-only author held the phase-order and selected-pointer
contracts without implementation, SDK, fixture or owner-output exposure. The
implementer read both complete prose files and owned sources only. Each admitted
substep now places its source prelude before resident admission and refreshes
body views afterwards. Separate selected movement membership retains live owned
fields without admitting new registrations early. Stable registration identities
and skipping removed instances are explicit engine lifetime policies, not a claim
of original destructor behavior. Bounded unit and actual movement/contact checks,
full required-owner CPU/Vulkan and SDK checks pass. Startup captures complete,
but repeated unprofiled native startup is slower; performance and broad native
acceptance remain open. The private receipt freezes both held hashes; no owner
content is published.

## Physical source gameplay payloads, 2026-11-08

The unexposed original-only author held separate physical-boundary, dirty-producer
and pointer-retirement contracts. The implementer read the complete prose with
the earlier field-lifetime, summary and phase-order specifications, froze six
original-byte hashes privately, and changed owned source only. Five-field state
now belongs to physical source lifetimes; retained restoration precedes publication
and real retirement records before destruction. Loaded/retained summary routing
and deferred dirty flags use those same instances.

Shared owned state, consistent World/cache locking, immutable COW leases, safe
pending-identity disposal and the mirror's existing no-echo network adapter are
explicit engine policies. The source audit did not prove native per-source dirty
pointer cleanup or universal race safety. Controlled lifetime/ordering checks,
ten focused groups, final required-owner CPU without skips, every Vulkan group,
SDK independence and two viewed startup smoke images pass. The fixture correction
used a valid positive radius; gate fixtures now update the owned World map. No
engine predicate was relaxed. Native persistence/save-callers work and broader
acceptance remain open. No owner content or private instrumentation is published.

## Host retained-state persistence, 2026-11-08

The unexposed original-only author held independent host-persistence, save-caller
and initialization prose. The implementer read every complete contract with the
held same-Host teardown evidence, froze four original-byte hashes privately and
used owned engine code only. Loader cadence and retained-only captures now have
controlled real-worker/file tests, with shared metadata ownership and separate
character output. Native final-before-World-teardown ordering is preserved without
an invented all-loaded-source flush. Explicit runtime initialization does not
merge source-binding archives into the identifier sets. Backend durability,
alternate callers/writers, character-worker scheduling, complete initialization
and native guard coverage remain open; captured and optional respawn policies
retain their separate scope. No original code or owner output is published.

## Viewed metadata reset, 2026-11-08

The original-only author finalized and held the identifier runtime-writer prose
before its research context was lost during workspace reload. The implementer
read the entire held file, froze its original-byte hash and used owned source
only. A real client/source fixture reproduced two missing retained-state reset
failures; metadata reset now has a distinct shared operation, preserving reward
IDs and loaded lifetimes. Signed victim-ID and packet peer/clock checks retain
their bounded scope. A fresh isolated author is separately tracing complete
viewed dispatch phases; no unfinished phase output is accepted. Other writers,
individual erasure, universal guards and broad native runtime parity remain open.

## Viewed packet dispatch, 2026-11-08

A fresh isolated original-only author held four separate phase, connection,
collision-lifetime and late-record contracts. The implementer read each complete
artifact, froze its original-byte hash and used owned code only. No unfinished
pre-reload phase handoff was accepted. Owned client dispatch now follows the
proved frame boundary and immediate queue order, with pre-dispatch requests and
post-renderer event collection. Four reproduced failures and stronger real-session
cases pass, with full required-owner CPU/Vulkan/SDK checks. The protocol fixture
now explicitly checks the three outgoing stages rather than relaxing counts.
Native and Alpha scene images pass; the first Alpha deadline failure is retained
before an unchanged retry and cold-fill reliability stays open. Precise Steam
transport ingestion, connection-role/rekey policy, native clocks and broader
acceptance remain separate. No original code, owner data or private probe is
published, and no unsupported effect was assigned to the read-only late visitor.

## Native per-World clock, 2026-11-08

The isolated original-only author held the independent World-clock contract;
the implementer read the entire prose, froze its original-byte hash and used
owned engine source only. A separate small kernel preserves the proved binary32
operation order, actor predicate, fresh local reference calls and strict crossing
sequence. Existing actor views are borrowed synchronously and rebuilt after
cleanup. The ordinary Host/viewed substeps now own their calendar advancement;
packet assignments remain later explicit writers. Native mode is the default;
the constant custom clock and authority generator cache are explicit engine
adapters. No native code, instructions or reconstruction source was inspected.

Three pre-change failures and literal/actual-World checks pass, together with
all-five-target, full required-owner CPU without skips, Vulkan, SDK and native/
Alpha scene checks. One authority-only observation fixture was corrected from
the wrong role, without relaxing numeric or engine predicates. Compiler-warning
cleanup preserves literal float words. Full scheduling, role/name/restoration
setup, other writers, complete reference-provider provenance and native runtime
acceptance remain separate. No owner content or private evidence is published.

## Connection role and source restoration, 2026-11-08

The isolated original-only author finalized/read back/held the separate role,
name and configuration-admission contract. The implementer read it entirely,
froze its original-byte hash and used owned source with the earlier independent
field-lifetime/connection prose only. A final synchronous connection callback
now selects the viewed role after peer publication, including local fallback.
A guarded source gate tracks that role without rebinding retained/live data;
retirement remains ungated. Compatibility suppression, serialized engine guards
and captured playback are explicit adapter policies.

Five pre-change failures and actual reload/COW/retention/order checks pass with
all-five-target, full required-owner CPU without skips, all Vulkan groups, SDK
independence and inspected native/Alpha scenes. Fixture output/index corrections
changed no engine predicate or expected block value. Complete initial database
configuration, socket lifecycle, alternate writers/guards, rekey collision policy
and broad runtime acceptance remain separate. No original code, owner content,
private evidence or memory is published.

## Character generation-worker saves, 2026-11-08

The unexposed original-only author finalized/read back/held separate character
writer/worker and root/collection-order prose. The implementer read both complete
artifacts, froze their original-byte hashes privately, and used owned engine code
only. Captured slot/data and player/database ownership are explicit safe engine
policies; no unused World-map copies or native temporary allocation shapes were
reproduced. The three root booleans/atomic publication bridge and generation-owner
baseline are adapters, not claims of native attribute-bank or worker timing identity.

Four reproduced behavior/admission failures, real TaskPool and temporary database
tests, the full required-owner CPU suite without skips, all Vulkan groups and SDK
independence pass. A disposable writable native run proves a numeric row change
well before exit; complete blob checks retain native collections and raw values.
The first no-recording attempt is retained and excluded from that proof. Native
and Alpha/Omega smoke images pass with unchanged settings/deadlines. Optional
recordings no longer gate native roster admission or canonical equipment.

No native code, reconstruction source, owner content, private evidence or memory
is published. Complete selected-character loading, process-mode/worker-start and
thread/bank interleavings, equal-coordinate star edit identity, alternate writers,
durability and final native gameplay/provenance acceptance remain open.

## Selected-character reset and canonical inventory, 2026-11-08

The isolated original-only author finalized/read back/held four independent
load, selective-reset, default-core-copy and potion-count-width contracts without
implementation, SDK, tests or owner-output access. The implementer read every
complete artifact, froze its original-byte hash, and used owned engine source only.
A finite field mask reuses owned default values without reconstructing the whole
actor or its surviving containers. Raw saved fields stay separate from legacy
presentation and later World updates. Bounded malformed/name handling and drawn
position initialization are explicit safe engine adapters.

Eighteen raw-load failures, two reset failures and one live-inventory ownership
failure were reproduced before fixes. Count-width literals already passed on MSVC;
defined unsigned wrap arithmetic removes language-level overflow ambiguity rather
than claiming a new numeric mismatch. Shared canonical inventory ownership avoids
per-frame snapshots and stale aliases; NPC byte records retain their separate path.
All-five-target, required-owner CPU without skips, Vulkan and SDK checks pass.
Follow-up on the entry build passes all four disposable load/read-only runtimes,
a periodic write before exit, and both inspected native/Alpha/Omega scenes.
The earlier sign-in-blocked attempt is retained and excluded. Complete caller/UI initialization, worker baseline/global
mode/interleavings, alternate writers, durability and broad native acceptance
remain open. No owner data, private evidence, memory or original code is published.

## Explicit generation-cadence entry, 2026-11-08

The original-only author independently held the complete thread-entry, three-clock,
process-byte and lifetime contract without implementation/results access. The
implementer read the whole artifact and froze its hash. Storage construction,
source binding and explicit cadence entry now have separate ownership boundaries.
The selected actor is loaded before the main session's post-initialization entry.
This boundary and treatment of aborted/unentered storage are explicit engine
adapters; native scheduler, thread-handoff/message-loop interleavings and precise
timer-domain identity remain outside the claim.

Two before-fix clock-read failures and delayed/second-sample/rebinding/stop/explicit
write checks pass with all5, full required-owner CPU without skips, all Vulkan
checks and SDK independence. Subsequent four-case/periodic native runtime and
inspected native/Alpha/Omega scene checks pass on the entry executable after manual
Steam reconnection. The earlier offline failure is retained. No nonzero process-byte
producer or user-facing toggle was invented. No original code, owner data, private
evidence or memory is published; full initialization and broader provenance/parity
acceptance remain open.

## Native startup input and derived configuration, 2026-11-08

The unexposed original-only author separately held complete startup-selection and
configuration/random/table/consumer prose. The implementer read both entire files
and froze their original hashes. Source-established ordinary caller input 123 is
separate from the owner-oracle values 15376/10211/5983; the source audit does not
calculate an imported runtime provider sequence. Existing owned generator code
supplies the derivation. No original or reconstruction implementation was read.

Five pre-fix default/derived-word/identity mismatches were reproduced. A shared
native default now connects normal Main generation and game metadata; explicit 0
and the existing Alpha/capture profiles retain their separate semantics. This is
a startup correction, separate from generic native reconfiguration or whole-seed
coverage. Older experimental 0 geography is not silently converted. All five
targets, full required-owner CPU without skips, Vulkan, SDK and four live cases
confirming the native input pass. Both scene images pass with matched settings and
unchanged Alpha profile identity. No owner content, memory or private
probe is published. Provider concurrency, UI reconfiguration, full generation and
final provenance/parity acceptance remain open.

## Client timeout and process-output clocks, 2026-11-08

The unexposed original-only author finalized/read back/held the complete timeout,
dispatch/connect writers and outgoing-clock contract independently. The implementer
read the entire artifact, froze its hash privately and used owned engine code only.
Fresh clock reads retain their separate store/admission positions. Safe malformed
content handling, missing-entity handling, synchronized shared clock storage and
world-identity negotiation remain explicit engine policies; native stack remnants
and unsafe thread races are not reproduced.

Eight pre-change admission failures were reproduced. Expanded clock/order/wrap/
lifetime/deferred-source checks pass. Old protocol fixtures now explicitly establish
the clock state that their packet assertions require, and synthetic UDP frames bind
an owned logical clock; production binds fresh process readings. All five targets,
full required-owner CPU without skips, all Vulkan groups, SDK independence, four
disposable load cases and both inspected native/Alpha/Omega scenes pass with
unchanged settings, deadlines and Alpha profile. Provider resolution, original thread/socket lifecycle,
other writers and broad original-peer/provenance acceptance remain open. No owner
content, original code, private evidence or memory is published.

## Retained creation state and commit, 2026-11-08

The unexposed original-only author independently finalized/read back/held the
complete creation/style/first-save/list-handoff prose. The implementer read the
whole artifact and the earlier complete finite-core-copy contract, froze both
hashes, and used owned code only. Shared finite reset/copy/capture operations keep
reset-alone distinct from the decoder's default assignment. Creation owns its
canonical inventory, current actor snapshot and separately constructed list data.
No original or reconstruction implementation was inspected.

Twelve pre-change starter failures were reproduced. Exact copied/omitted-byte,
retained-container/identity, search-boundary, zero/current seed, template/counter,
raw/Unicode commit and disk/list tests pass. All5/full required-owner CPU without
skips/Vulkan/SDK, actual writable/read-only native and read-only Alpha/Omega
creation, four raw-load cases and both inspected scenes pass. Private SQLite
triggers verify count/numeric/slot order and complete first-record parsing; no
owner files or private evidence are published. Alpha's profile remains unchanged.

Safe name bounds, typed unwritten defaults, backend failure recovery, guarded
snapshot/serialization ownership and startup adapters are explicit engine policies.
Native uninitialized values and unsafe race behavior are not reproduced. Complete
home/appearance/crafting routes, provider/shared random state, input/update/draw/
Help interleavings and broad gameplay/native-pixel/provenance acceptance remain
open. The separate home-search contract and scheduling audit are future work.

## Native home-search admission, 2026-11-08

The independent original-only author held the complete home-search/terrain-admission
prose without implementation, tests, SDK or owner-output access. The implementer
read that prose and owned engine code only. A shared lazy driver now preserves
coarse progression, signed draws, ordered admission, separate coordinate units,
unsigned candidate hashing and native fixed placement. Existing owned terrain
providers remain reusable boundaries, without a claim that this handoff exhausts
their classifier, registry, noise or random-state closure. Finite aborts, missing
bindings and captured/Alpha coverage/surface behavior are explicit engine policies.
Focused before/after checks, all five builds, full required-owner CPU without skips,
all Vulkan groups, SDK independence, actual disposable writable/read-only creation
and four raw-load cases pass. Native, Alpha/Omega and large-home streaming scenes
are inspected with unchanged readiness/deadlines; Alpha identity is unchanged.
These bounded checks do not establish native pixels, performance or cold-start
reliability. Owner assets, saves, Mods and display settings are untouched and
excluded from source/runtime packages.

## Character style draw and provider admission, 2026-11-08

The independent original-only author held the complete application/update/draw/
creation/Help scheduling prose without implementation, tests, SDK or owner-output
access. The implementer read that full prose and owned code only. Palette side
effects now use the existing admitted owner-draw hook instead of layout/navigation
publication, retaining separate base/working bytes and changed-byte rebuilds.
The opaque provider is queried only under the full eight-byte zero gate, before
World substeps, without inferring imported provider internals. Focused actual-owner
and synthetic state checks, full required-owner CPU without skips, all Vulkan
checks, SDK independence and disposable creation/raw-load runs pass. The initial
native readiness miss is retained/excluded; an unchanged native retry and the
Alpha/Omega scene pass, with both images inspected and unchanged Alpha identity.
This does not establish a root cause or reliable cold-start/performance behavior. Full native caching, resource/Help/control integration,
provider/thread interleavings and broad parity/provenance acceptance remain open.
No original source, SDK or reconstruction implementation was read by the implementer.
Owner assets, saves, Mods and display settings remain untouched and excluded from
source/runtime packages. Alpha terrain/lighting and Omega water remain implemented.

## Owned material-probe refactor, 2026-11-08

The implementer reviewed owned meshing code and measured its existing profiler.
No new native behavior or original-source interpretation is introduced. Material-
only queries skip a pure solved-light read whose red byte was discarded; the
actual water red-byte consumer and required corner samples retain their reads.
Existing light-publication, voxel, full CPU/owner and Vulkan checks pass, as do
SDK independence and both inspected native/Alpha scenes. The prior creation-phase
held artifact is verified unchanged as part of the source payload, rather than
claimed as a new contract for this refactor. Fixture timing results are bounded;
the earlier native readiness miss and broader provenance/parity gates stay open.
Source researchers have not seen implementation, tests or timing results. Owner
content and private evidence remain outside both packages and GitHub.


## Cell-object branch correction and World table queries, 2026-11-08

The unexposed source-side author held the complete cell-classifier/table prose,
then independently re-derived category-two control flow from complete original
instruction bodies without reading the earlier handoff or receiving implementation,
test or owner-output feedback. The implementer read both entire held artifacts,
froze their hashes and used owned code only. The initial threshold interpretation
failed the unchanged 60,000-cell comparison in 29 places and was retained/excluded.
The independent
[branch-control contract](specs/steam-category-two-cell-branch-control-flow-20261108.md)
is authoritative for category two and supersedes that part of the
[earlier full handoff](specs/steam-cell-object-classifier-and-world-table-admission-20261108.md).
Neither held artifact was silently edited.

The final change preserves the existing weight gate, continues an unmatched nearby
point through canonical selection, retains outside-box-only query admission, uses
the specified unsigned reductions and indexes the current table count. Typed
empty-vector safety and existing immutable/provider bindings remain engine policies.
Seven supported failures in a privately recompiled owned baseline unit are distinct
from the eight rejected initial specification expectations. All five targets,
focused checks, full required-owner CPU without skips, Vulkan, SDK independence,
three actual creation cases and both inspected native/Alpha/Omega scenes pass.
No whole native/provider/thread or visual parity follows. Owner state was restored
and byte-verified; code, content, captures, saves and private evidence remain
separated. Only the audited source payload is intended for GitHub.

## Viewed source-worker cadence, 2026-11-09

The original-only author held the complete generation selection, worker and
drain prose before implementation, without receiving engine, tests, owner outputs
or combined history. The implementer read the complete unchanged handoff and
owned code only. Its hash is retained in the private checkpoint receipt; the
whole [contract](specs/steam-generation-request-selection-and-worker-drain-20261109.md)
is published without source excerpts.

The bounded integration decouples viewed absent-source batches from presentation
while retaining native selection and normal batch-drain boundaries. Protected
demand copies, a fixed source-owner lifetime, deferred errors, finite admission
and duplicate-safe publication are explicit safe engine policies. Native raw
aliases, unsafe duplicate races, imported PPL fairness, arbitrary global-mode
producers and universal numerical/random/provider closure are not claimed.
Chunk, host and map worker lifecycles are separate future integration work.
Alpha terrain/lighting, biome palettes and Omega water stay distinct extensions.
Owner assets, saves, Mods and private evidence remain excluded from publication.

Actual-World before/after and gated callback/readiness checks pass, with all five
builds, full required-owner CPU without skips, every Vulkan group, SDK independence,
inspected Native/Alpha/purple-land scenes and disposable save/profile checks. No
readiness gate or deadline was lowered. Complete native worker/provider, broader
parity, performance and final provenance acceptance remain open.

## Retained UTF-16 creation editor - 2026-11-09

The creation name now belongs to its persistent owner-GUI Edit caption. Raw
Windows keydown, keyup and character units enter the shared ordered GUI message
queue with pointer edges; repeats and individual UTF-16 units survive. The
frame-based ASCII name accumulator and its length cap are removed. The later
creation visibility poll admits exactly 2..15 units without limiting editing.
Create is a separate captured event-3 connection: it hides the actual page, reads
the current caption and uses the retained live actor's established low-byte save
bridge. Enter clears focus. Teardown removes the session's retained callback.

Independent complete held contracts cover keyboard/focus, Edit pointer/layout,
indexed metrics/font configuration and ordinary Game/window input. Source author
and implementer remained isolated, with no implementation/test feedback. The
indexed measurement path shares the real loaded font but preserves its distinct
source traversal, spacing, wrap and configuration arithmetic. Scene attachment,
clone reset, native select-all/Shift/drag operations and caption events are bound.
The ordinary manager's selection rectangle virtual is return-only; it still
receives two measurements. Its selected caret uses red, width one, the current
draw matrix and the signed shared-clock quotient phase. Font/provider, borrowed
caption replacement, generalized geometry and imported/backend limits remain
explicit. Engine ownership and finite-domain guards do not reproduce unsafe
native aliases or overflow.

Meaningful owner-scene checks cover 17-unit editing, literal surrogate and
low-byte-zero units, hidden-ancestor focus, selection replacement, Enter, retained
text, captured outside release and callback teardown. Four added Vulkan checks
exercise opaque red and blink phases at GUI scales 1 and 1.5, with the test clone's
clip disabled to isolate material RGB. Normal clipped images are retained and
visually inspected; layer interpolation can filter a one-pixel caret. These are
local regressions, not an original-runtime pixel-parity certificate.

This checkpoint does not close the newly audited full Game input phase. The GUI
queue drains before physical pointer polling, but broader synchronous WinProc
GUI-to-Game rereads, Game low-byte/sign-extended chat delivery, raw Game keys,
repeated Escape's six-button publication, binding capture and remaining business
callbacks still need integration. The Game Escape-key phase remains open; Back's page transition is closed below.
Native runtime comparison and final provenance/visual/gameplay acceptance remain
release gates. Alpha terrain/lighting, distinct biome palettes and Omega water
retain their separate paths and profile identity.

The independent [attachment display-field audit](specs/steam-edit-attachment-display-field-20261109.md)
additionally traces the concrete registered clipping property separately from
visibility. Attachment enables the shaped parent's clipping, including when its
own caption is absent; it preserves hidden-parent visibility. Both independent
conditional examples fail before that store correction and pass afterward. The
failed run is retained. This closes the selected concrete field identity rather
than inferring meaning from an adjacent member or UI convention.

## Native Back page transition and press binding - 2026-11-09

The independent [menu and Back callback contract](specs/steam-interface-menu-button-and-back-callbacks-20261109.md)
closes Back's complete original native leaf despite the missing decoded owner
attribution. Back tests new-world, worlds, creation and characters in that order,
clearing only the first selected page and selecting its defined destination.
Other simultaneous page flags and independent options, controls and Help remain.
The callback performs no actor rebuild, camera reset or menu-byte clear.

The persistent Back button now binds event2 under the shared root. Its press
immediately publishes the page change; release does not repeat Back. Session
teardown clears both retained press and release callbacks. The old manual Back
rectangle action and its actor/camera resets are removed. Existing application
overlay dismissal remains a separate compatibility adapter while the full Game
keydown/Escape phase is implemented. The frame adapter suppresses duplicate
legacy frontend handling after a queued Back press; whole-burst Game semantics
remain an explicit broader input task.

Nine source-literal priority/multiple-page examples and actual owner-template
press/release/teardown checks pass. An explicit bounded back-test CLI adapter
compares page state, actor identity and all displayed/target orbit fields around
the live callback. Such replacement-engine test adapters do not establish actual
OS input delivery or original-runtime pixel/behavior parity. The other six menu
callbacks' reached rebuild/refresh/RNG/provider effects remain in the complete
contract for subsequent implementation; their outer toggles are not substituted
with a generic exclusive-menu policy.

All five targets, required-owner CPU without skips, 92 shader/2 camera/106 GUI/
17 map/14 native-water plus shared-water/Alpha-scenery checks and SDK independence
pass. Native and Alpha Back probes retain actor/camera and private save bytes;
creation, complete payload/order, old-profile isolation and reopen checks pass.

## Game keyboard/menu and map-press integration - 2026-11-09

The independent original-only author held the complete six-menu layout and
map-press admission prose without implementation, SDK, tests, owner files or
verification feedback. The implementer read each whole saved artifact, checked
its hash and copied it unchanged. Earlier held Game/window and menu callback
contracts retain their exact bytes. Only those prose contracts, owned code and
permitted owner data were used for this integration.

The selected keyboard, six controls, distinct callback effects, right-press
null-target branch and post-GUI arrow admission follow the stated scope.
Template dimensions, flags and dynamic control classes are retained from actual
owner data; a resource label does not establish that its control is a Button.
Normal and captured hit tests are separate from direct A/B subtree queries.
Owned callback lifetime and the retained queue are safe engine policies, without
claiming native raw-pointer or arbitrary message-pump reentrancy guarantees.

Focused source-conditional and actual-owner checks cover the selected paths.
Whole Game mouse/release business, physical/controller admission, binding
capture, menu binding captions and generic geometry/font/providers remain
bounded; final native runtime, provenance and broader parity acceptance remain
release gates. Owner assets, saves, captures, packages and private evidence stay
out of the source payload. Existing Alpha and Omega extensions remain distinct.

The Windows import follow-up uses only the owned ProgressFile and database paths.
It shortens an unpublished temporary filename to leave space for SQLite's journal;
it adds no native source interpretation. A genuine 230-character-path failure and
the unchanged succeeding regression are retained, with the same-length live Alpha
creation/canonical/old-profile checks. Generic long-path and broader native/runtime
acceptance are separate from this bounded fix.
