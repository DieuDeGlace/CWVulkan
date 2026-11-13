# Source review checkpoint — 2026-11-08

## Alpha world-generation shrines - 2026-11-13

The Shrine of Life checkpoint had been checked in game on Steam world generation only: the Alpha
generation test character's home cell holds no shrine. The shrine fixture now falls back to the
nearest Shrine-of-Life site of the land, as revive's second pass finds it. On Alpha generation
it returns to cell (4, -39), finds the active statue two blocks away, plays the flute and
registers the shrine; the old fixture never completed. Fixture only: no engine behaviour changed.
All five targets, full required-owner CPU without skips, every Vulkan group, SDK independence and
the Native/Alpha probes pass.


## Combat text layout - 2026-11-13

Steam's combat texts pass TextShape flag 1 where the progress notices pass 0. The held Plasma
text contract defines that flag as centring each line horizontally on the anchor, with the first
baseline staying on it, so a trailing line feed after BLOCK/MISS/ABSORB moves nothing.
CubeVulkan drew combat texts left-aligned from the anchor; the floating-text records now carry
their flags into the text layout. Two CPU checks and the in-game run failed with flags 0 and pass
now. All five targets, full required-owner CPU without skips, every Vulkan group, SDK
independence and the Native/Alpha probes pass.


## Combat texts - 2026-11-13

An engine survey against the held Game update schedule found that Steam's combat-text step
(phase 68) was missing: CubeVulkan showed only its own older damage numbers and never "K.O.",
"BLOCK", "MISS" or "ABSORB". An independent original-only contract now drives it. Every hit of
the frame shows "K.O." in yellow when it stuns; hits that involve the player show the number
(whole numbers, tenths below one, no sign) or the label, white on others (tinted for spirit
weapons, 60 px and twice the rise when critical), red and sinking on the player, green for
healing, for one second, drawn the same frame. Eighteen CPU checks and the in-game run failed
before and pass now. All five targets, full required-owner CPU without skips, every Vulkan
group, SDK independence and the Native/Alpha probes pass.


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


## Game keyboard, menu controls and map press admission - 2026-11-09

The ordered interface queue now runs the GUI's 16-bit keyboard phase before the
current Game low-byte phase. Raw down/up state, repeated Escape's separate menu
byte, ungated F1/F4 effects and per-message F2/3/5/6 scaling are distinct from
physical action polling. The four scale-bound words and the directional-only
clamps follow the independent Game contract. Chat reads current GUI focus,
Gameplay's own visibility and the raw binding-capture index; character delivery
has its separate focus gate and sign-extends the Game byte. Active Return submits
before clearing current text, and Left/Right edges preserve selection.

Six retained owner menubutton templates use the original resource/index order
under Gameplay. Their dimensions and control classes come from owner data.
Placement inverse-projects the integer screen-centre anchor before adding the
60-unit spacing and -200 local Y offset. Resource fitting aligns current bounds
and scales the two extents independently. A later UI update publishes the menu
byte even while F4 hides the root; frontend page visibility remains independent.
The six callbacks retain their distinct rebuild, refresh, toggle and exclusion
rules. Physical menu bindings share those actions. Right press uses a fresh
post-GUI capture-or-hit result before clearing the byte and three panel nodes.
Callback teardown removes borrowed gameplay owners.

Recent-land arrows now bind retained event2 callbacks. Their refresh publishes
B then A before the caption provider. Game press admission checks the current
map byte once, then queries A and B directly in order after GUI callbacks, using
each call's retained point. Direct queries have their own visibility/shape gates;
ancestor hiding, capture and control-enabled state are separate. A callback can
hide its own arrow and thereby admit the normal chat-clear continuation. Chat
press/key interleaving now stays in one queue; admitted business work is retained
for the remaining frame adapter instead of repeating the admission later.

The original-only author supplied the complete unchanged
[layout/publication contract](specs/steam-interface-six-menu-layout-and-publication-20261109.md)
and [map-press admission contract](specs/steam-game-map-press-early-return-admission-20261109.md),
alongside the earlier Game and menu callback contracts. Each was read in full and
hashed before use. No implementation, tests, owner outputs or feedback reached
the author. Owner data, captures, private evidence and memory remain excluded
from the public source payload.

Source-literal traces and actual owner controls cover repeated keys, GUI-to-Game
rereads, UTF-16/low-byte separation, chat send/selection ordering, six resource
fits, signed/odd-width placement, delayed publication, event2-only actions,
callback teardown, right-press hits/overrides and map-arrow early returns.
All five targets, the full required-owner CPU suite without skips, all Vulkan
groups and SDK independence pass. Fresh Native/Alpha creation, Back actor/camera,
complete save payload/order/profile and reopen checks pass. Eight actual menu
callback state cases and both frontend Escape publication probes also pass.
The Alpha/Omega menu scene passes at the unchanged readiness gate. The Native
scene still exceeds that gate: the current failed capture and preceding controlled
failure/retry are retained. No deadline or readiness condition was relaxed, and no
startup reliability or performance cause is established. Native scene acceptance
remains open alongside the broader gates below.

A longer Windows test directory exposed an owned terrain-profile import issue:
the temporary database filename plus SQLite's journal suffix exhausted the path
budget before character copying. Shorter unpublished names preserve the existing
atomic publication. An actual 230-character destination fails before this fix
and passes afterward; the same-length live Alpha creation also passes, preserving
canonical database bytes and the old compose-v2 profile. This is a local profile
robustness change, separate from native save-format or terrain-generation claims.

This closes the selected dispatch, menu and press-admission paths, with explicit
engine ownership and finite-resource boundaries. The remaining Game mouse/release
business and scalar/node postlude, physical action and binding-capture producers,
menu binding captions, generalized shape/font/provider behavior, arbitrary
reentrancy and native OS/runtime acceptance remain open. This is not complete
Steam input or pixel parity. Alpha terrain/lighting, purple dry Undead Lands,
distinct biome palettes and Omega water retain their separate existing paths.


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
callbacks still need integration. At this earlier checkpoint the Game Escape-key phase was still open; the later Game keyboard checkpoint above covers its selected byte/publication paths.
Native runtime comparison and final provenance/visual/gameplay acceptance remain
release gates. Alpha terrain/lighting, distinct biome palettes and Omega water
retain their separate paths and profile identity.


## Viewed source loading independent of presentation - 2026-11-09

The viewed source worker now completes successive source batches independently
of render updates. It retains the native inclusive footprint, signed priority
selection and four-coordinate prefix. A single request runs directly; parallel
requests finish as a batch before the next demand sample or stop boundary. The
viewed entry priority/concurrency policy is separate from the chunk worker's
unproved initial scheduler policy.

An actual-World before/after probe supplies one application update and then only
observes residency: the prior build loads four of nine requested sources; this
build loads all nine. Controlled callbacks also exercise selected-batch shutdown,
copied destination changes, stale-readiness prevention and deferred failure
delivery. Real source/mesh checks retain all 36 required inputs at the negative
fractional boundary, independently reload the nine fringe inputs, and retain
range, movement and destination invalidation.

Protected value snapshots, a fixed borrowed source owner, duplicate-safe source
publication and failure handoff are engine safety policies. They do not reproduce
native raw-pointer races or establish imported scheduler fairness. Shutdown
drains source work before releasing its inputs or taking the final save snapshot.
Host, chunk and map worker lifecycles remain distinct; the native ordinary-mode
byte's live producer and complete numerical/provider closure remain unproved.
The complete held [original-only worker contract](specs/steam-generation-request-selection-and-worker-drain-20261109.md)
is included unchanged. No implementation or test feedback was sent to its author.

All five builds, full required-owner CPU without skips, every Vulkan group and
SDK independence pass. Fresh Native and Alpha/Omega scenes complete and are
inspected, as is the purple-land mound transition. Native writable/read-only and
Alpha read-only/writable/reopen checks retain complete payloads, creation order
and unchanged canonical/previous-profile bytes. Readiness and deadlines are
unchanged. Prior cold-start variability and broader native acceptance stay open.
This is a bounded cadence correction, with no gameplay-FPS or complete-parity claim.
Alpha terrain/lighting, biome palettes and Omega water remain implemented.


## Steam land biomes and terrain palettes - 2026-11-09

The Alpha hybrid now keeps Steam land floors instead of letting its independent
ocean field flood named land biomes. Higher Alpha relief remains available;
savannahs retain their gentler native columns and map relief. A smooth regional
halo joins the policies to surrounding terrain, while the earlier rounded
construction transition and exact native cores remain intact. Liquid surfaces
stay fixed independently of translated ground.

Ground colors now come from the actual native terrain columns, matched by depth,
and maps retain their native biome palette. This keeps purple Undead Lands and
other distinctive ground colors. Alpha landscape and voxel-tree geometry remain
outside native savannahs and protected cores. Hybrid blocks use Steam color
encoding; raw Alpha generation and its separate full-RGB provider tests remain
unchanged. Alpha lighting and Omega water stay implemented.

A retained owned baseline loses2,063 dry columns in the sampled Undead Lands zone
and4,096 in a sampled savannah zone. The corrected probes lose none. Two savannah
cases, including higher Alpha terrain, retain exact native columns and overview
heights. RGB, map/voxel land coverage, earlier landmark continuity and source
immutability checks pass. Three alternating processes per version measure a
2.899-second baseline median and2.876-second candidate median for the bounded
search/map/zone probe; this establishes no meaningful slowdown or gameplay-FPS
claim. All five builds, full required-owner CPU without skips, every Vulkan group
and SDK independence pass.

The final build also narrows cover-only queries to the same prepared Alpha H
without unrelated climate/color work, and reuses identical XY decisions within
composition. Independent fresh stores match exact height words and finite
admission. Plant-enabled whole-column/model/light/emitter fingerprints remain
identical. Three alternating probe processes measure warmed composition medians
of320.520 and310.140ms; variability limits this small result, with no FPS claim.

Fresh final Native and Alpha/Omega scenes, the corrected purple-land mound view,
and a normal-distance savannah view complete and are inspected. Current writable,
read-only and reopen checks pass, including complete character payloads and
unchanged canonical/previous-compose-v2 database bytes. Earlier readiness failures
remain retained; current scene timings are variable, so reliable cold-start and
broad performance acceptance remain open. No deadline or readiness gate was lowered.

The hybrid selects compose-v3, keeping previous profile folders separate without
geographic migration. This is an owned extension correction, implemented from
project code and permitted data; the separate new GUI source contract is held
and unused. Broad native pixels/world/gameplay, provider/thread, multiplayer and
final provenance acceptance remain open. Only audited source is published;
owner data, captures, private evidence and binaries remain local.

## Exact height rounding and fixed-water plants - 2026-11-09

The Steam final-height path now rounds the peak mask-times-height-gate product
before multiplying by the noise sum. The existing two multiplications remain;
no amplitude, mode switch or random draw is added. A fresh complete original-only
[ordered-arithmetic contract](specs/steam-final-height-ordered-arithmetic-and-conditional-vectors-20261108.md)
qualifies this association with independently selected literal binary32 cases.
It supersedes only the left-associated peak-scalar expression in section8.3 of
the earlier height contract; both original prose artifacts remain unchanged.

Six actual production-function regressions fail before and pass afterward.
Their inputs use the owned fixed Noise provider and synthetic HeightInputs;
these are conditional mathematical references, not new original-runtime
observations. An independent replay of the source author's literal family B
also matches its intermediate/final words. A larger read-only conditional probe
isolates38 changed results in10,240 cases with zero baseline-reference errors.
Private owned baseline/candidate builds both retain all7,646 recorded mode0/4
height outputs at3,823 points. The full current suite retains zero differences
across95,575 recorded pipeline values and60,000 recorded cells. No original
oracle or expected game value was relaxed.

Alpha water lilies now keep the fixed liquid level while solid terrain moves.
Records whose former surface is covered are omitted; grounded plants still
follow the solid shift. Four reproduced placement failures pass. Raw Alpha
generation, vegetation draws and the compose-v2 geometry/profile identity remain
unchanged. Alpha lighting, Omega water and the wider rounded landmark approach
stay implemented.

All five targets, full required-owner CPU without skips, every Vulkan group and
SDK independence pass. The implementer used complete code-free prose and owned
code only; the isolated author received no project results or owner outputs.
Source/runtime payloads are audited separately, with only source public.
Current-binary live Help/creation/control and Alpha landmark/material/water/
new-profile acceptance remain pending because Steam reports offline. Imported
providers, preparation/callback ordering, floating environment, concurrency,
whole-world/native pixels, broader gameplay, multiplayer, hardware/fresh-install
and final provenance acceptance remain open. This is a development checkpoint.

## Help visibility and ordinary control admission - 2026-11-08

Help now keeps its selected value in the retained HUD, under the gameplay branch.
Frontend and F4 ancestor hiding preserve that value. Each dispatched F1 key-down,
including repeats, toggles it through the ordered interface-message route before
polling; raw key edges remain available to binding capture. Help alone no longer
admits the engine's modal-panel gate or clears ordinary gameplay input. Generic
clicks, Escape and the controller panel-dismiss route do not dismiss Help.

Successful creation selects Help before persistence and enters ordinary gameplay
without another dismissal, retaining the live actor and character-owned data.
The frontend and gameplay adapters share the early two-root action-byte and
three-root two-byte flag publication before World work. Immediate entry therefore
clears the stale title action/flag while preserving unrelated record bytes.
Help does not become a World-step or character-name predicate. Other World,
action, focus/chat and control admissions remain distinct.

The complete independently held
[Help contract](specs/steam-help-state-and-control-admission-20261108.md)
is copied unchanged. The author received no implementation, tests or results.
Owned review identified the earlier modal/dismissal behavior; no historical
before-execution native failures are claimed. Actual message routing, retained
owner/menu gates, scene and owner-template ancestry, and all root combinations
with stale and clear flags are checked. All five targets, final full required-owner
CPU without skips, every Vulkan group and SDK independence pass. After only CPU
fixture changes, the full CPU suite was repeated; production sources, library and
engine executable remain byte-identical to the passed GPU run. Existing native
cell comparison still has zero differences across60,000 cells.

Current-binary live creation/Help/control scenes and Alpha compose-v2 landmark,
water/material and first-profile/save acceptance remain pending: Steam still
reports offline. The Alpha continuity source checkpoint is preserved unchanged.
No owner data, private evidence or binary release is public.

The existing frontend pointer adapter still aggregates/defer-routes confirmation;
native same-message-pump Create/F1 interleaving is not established. The actual
help-menu resource callback, native class/action text/layout, cached draw/dirty
state, other panel gates, complete provider/thread behavior and broad gameplay/
visual/multiplayer/hardware/provenance acceptance remain open. This closes the
bounded Help visibility/control integration, not those wider UI behaviors.

## Alpha landmark transition continuity - 2026-11-08

Alpha's hybrid terrain now approaches preserved Steam landmarks through a
256-block rounded halo, including when the surrounding Alpha solid ground is
below sea level. Liquid surfaces remain fixed in world Z. The same field drives
terrain, map heights and nonlocal record reanchoring. Construction footprints,
rooms, layouts and exact native core columns are retained; ordinary Game
generation and the raw Alpha terrain formulas do not change. The composition
profile advances to compose-v2, isolating earlier terrain caches and saves.

A reproduced forest mound/coast boundary changes from a282-block neighboring
column step to1, while the mound and volcano peaks retain281 and269. Read-only
32-block overview samples reduce maximum steps192 to64 and256 to96 respectively;
core masks/heights and sampled raw Alpha height values are unchanged. These are
bounded procedural results. They do not establish universal gradients or live
visual parity. Three alternating CPU probe processes per version measured
1.687 to1.860 seconds by median, about10.3 percent more composition-probe time;
this is not a whole-game FPS or cold-start claim.

All five targets, meaningful before/after continuity cases, fixed-sea/raised-pool/
dry-air/metadata/overflow checks, full required-owner CPU without skips, all
Vulkan groups and SDK independence pass. The existing60,000 native-cell
comparison remains exact. The prior classifier handoffs remain frozen payload
provenance; this user-requested owned Alpha extension imports no new native
behavior and does not use the separate held landform research.

Live native and Alpha/Omega scenes, landmark/material/water visual review and
new-profile first-use/save acceptance are still pending Steam reconnection.
The first native capture timed out without an image while its log reported the
Steam account offline; it is retained and excluded, with no performance cause
asserted. This is an audited development source update, not a final binary
release. Original saves/Mods/display were untouched. Broader native gameplay,
visual, concurrency/provider, original-peer/two-account, hardware/fresh-install
and clean-room provenance gates remain open. See
[Alpha generation](alpha-generation.md) for the composition and evidence limits.

## Cell-object continuation and current World tables - 2026-11-08

An unmatched nearby category-two marker now continues to the shared canonical
lookup instead of returning zero. The existing strict weight-greater-than-0.5
gate is retained; only points outside the nearby box can run the group/road
query. Unsigned remainders now preserve the specified rare hash tail in anchors,
town/marker choices and even-group canonical selection. Town queries use the
current outer-table count, retain row-major entries and still run fallback for
zero slots or out-of-window groups. Empty in-window storage returns zero as an
explicit safe engine policy; this is not native malformed-vector behavior.

The initial full classifier handoff reversed the category-two threshold. A full
recorded-cell check caught 29 mismatches, so that attempt was retained and
excluded. A separately held original-only branch-control audit independently
resolved both the threshold and nearby-miss continuation. Both complete handoffs
remain unchanged for provenance. The
[branch-control contract](specs/steam-category-two-cell-branch-control-flow-20261108.md)
supersedes the category-two paragraphs in the
[earlier classifier handoff](specs/steam-cell-object-classifier-and-world-table-admission-20261108.md);
the latter's bounded table/unsigned facts remain the basis for those paths.
The source author received no implementation, tests, captures or results.

An exact owned published CellObjects translation unit, recompiled privately with
otherwise unchanged providers, reproduces seven supported failures: two anchors,
two table counts, one rare canonical choice and two nearby misses. This is not a
historical whole-executable or original-game run. The corrected build matches all
60,000 recorded cells, and expanded focused checks, all five targets, full
required-owner CPU without skips, every Vulkan group and SDK independence pass.
Actual writable/read-only native and read-only Alpha/Omega creation pass, including
the complete first record and unchanged read-only databases. Matched native and
Alpha/Omega scenes pass in 45.266 and 83.359 seconds; both images are inspected
with the same settings, 150-second deadlines/readiness and Alpha profile identity.

Alpha terrain/lighting, Omega water and existing extensions remain implemented.
The oversized projected minimap is unchanged; native pixel equality, performance,
cold-start reliability and whole-world coverage are not established here.
Original-game reference launches exited before capture with and without mods;
their cause remains unproved. Original saves, Mods, ReShade and root settings/logs
were restored and byte-verified, including after the engine checks. No owner
content or private evidence is published. Broader gameplay, provider/concurrency,
original-peer/two-account, hardware/fresh-install and provenance gates stay open.


## Material-only terrain probe optimization - 2026-11-08

Material probes in the native terrain mesher now skip the solved-red overlay
when they consume only block type. Corner probes still read their required light
sample, and the whole-water shortcut explicitly requests the red byte it tests.
Filler, dry-air, implicit sea, border and material decisions are unchanged, as are
RGB/corner arithmetic and gameplay/model stored-light publication. This is a
refactor of owned code; it introduces no new native behavior.

A private timing run identified solid terrain as its largest mesh stage. Its 766
generation samples contained no repeated generation key; this is one observation,
not a universal cache or missed-deadline diagnosis. Five isolated light-publication
fixture runs per version produced 30 Steam solid-stage samples each. The median
fell from 23.1095 ms to 18.718 ms, about 19 percent. Runs were sequential and not
interleaved. This focused result is not a whole-game FPS, native performance or
cold-start reliability claim; no Alpha performance improvement is asserted.

All five targets, focused light-publication/voxel checks, full required-owner CPU
without skips, every Vulkan group and SDK independence pass. Matched native and
Alpha/Omega scenes complete in 48.360 and 93.750 seconds; both images are inspected
with unchanged settings, readiness/deadlines and Alpha profile identity. Alpha
terrain/lighting and Omega water remain implemented. The earlier native readiness
miss remains recorded; no root cause or reliability fix is claimed here.

The prior complete creation-phase handoff remains frozen and unchanged in the
source payload. It is not a new source contract for this owned-only refactor.
Broader native generation, visuals, gameplay and provenance acceptance stay open.

## Character style draw and account-provider admission - 2026-11-08

Publishing the creation screen's navigation/layout no longer changes the live
character. Palette effects now run through the existing admitted widget-draw
callback, after the ordinary World phase. The working color is saved, restored
from the base triple and optionally replaced by the hovered palette color. Only
a difference from the saved working bytes triggers a synchronous style rebuild.
It no longer compares against a separate staged-character mirror. Hovering does
not commit base color, and an unchanged draw does not regenerate items or HP.
The initial white working triple and synchronous zero-seed rebuild remain
separate from the first later palette draw.

The opaque account provider is now deferred until the full eight-byte entity
operand is zero. A nonzero high half suppresses the provider even when the seed's
low word is zero; returned zero remains eligible for a later update. The return
is stored directly without a second admission test after the provider callback.
Frontend and gameplay reuse their provider binding before World substeps. The
initial reset, later publication, draw-triggered rebuild and Create commit keep
their distinct lifetimes. Unavailable engine actors/providers are handled safely.

Focused checks cover provider call counts, zero/high-word cases, callback stores,
missing owners, literal palette rows/truncation and working/base retention.
Actual owner-template checks distinguish layout publication from draw and verify
principal/creation-ancestor visibility and disabled-owner admission. All five
builds, full required-owner CPU without skips, all Vulkan groups, SDK independence,
disposable writable/read-only and Alpha/Omega creation and four raw-load cases
pass. The initial native scene missed its observation deadline with four source
jobs still active. That failure is retained and excluded from pass evidence. An
unchanged native retry passes in 60.531 seconds and Alpha/Omega in 113.703 seconds;
both images are inspected with unchanged settings/deadlines/readiness and Alpha
profile. No root cause or reliable cold-start/performance result is inferred.

The complete held [original-only update/draw/Help contract](specs/steam-character-creation-update-draw-and-help-handoff-20261108.md)
was read before implementation; its hash remains frozen and the author remains
unexposed. The shared widget callback is an engine binding, without a claim to
exhaust native caching, resource hierarchy or Windows/provider interleavings.
Complete Help/control integration, crafting/appearance closure, imported random
state and broader gameplay, native-pixel and provenance acceptance remain open.
Alpha terrain/lighting and Omega water remain implemented.

## Native home-search progression and admission - 2026-11-08

Home search now takes the two signed provider samples in order, converts the
polar point directly to the coarse grid and increments coarse X before every
candidate. It no longer divides that point by 64 or caches the same candidate
for 64 retries. Candidate hashes use unsigned remainder and defined low-32-bit
coordinate arithmetic, including the rare wrapped hash word. Each attempted
home pair is published before its admission queries.

The lazy query interface preserves the native object/height/regional/descriptor
order, four separate nonpositive weights, three separate inclusive 0.4 thresholds,
opposite eight-block height samples and the final descriptor exclusion. Regional,
block, coarse and home pairs remain distinct. The regional descriptor receives
the original earlier regional pair. Its exclusion compares the correctly floored,
conditionally even block cell; a sentinel does not exclude a candidate.

Native placement preserves the wrapped integer block XY values and separately
queries mode-zero height at the placement point. Z uses the rounded single-precision
height times 65,536 followed by signed truncation, without a voxel ceiling or feet
offset. Existing owned terrain providers are reused; each height call builds its
own query inputs. Captured and Alpha modes retain explicit coverage/surface policies,
Alpha terrain/lighting and Omega water. The native loop is unbounded; the engine's
finite attempt limit, incomplete-binding handling and fallback remain adapters.

Seven original progression/flatness failures and one later independent unsigned-hash
failure were reproduced before their respective fixes. Expanded focused checks pass
for exact call order, early exits, thresholds, negative floors, wrap, output publication,
finite abort policy and large fixed positions. Actual native writable/read-only and
Alpha/Omega read-only creation pass; both read-only database pairs are unchanged.
The first writable blob is fully parsed and retains exact large-world home XY,
starter inventory/templates and the home key. All five targets, full required-owner
CPU without skips, all Vulkan groups and SDK independence pass. Four raw-load cases
also pass. Matched native and Alpha/Omega scenes complete in 51.953 and 86.031 seconds;
both images are inspected and the Alpha profile is unchanged. A read-only copy of
the newly created large-world character also loads, streams and captures a scene
in 122.875 seconds, with its image inspected and databases unchanged. These are
bounded smoke checks, without native-pixel, performance or reliable cold-start claims.

The implementer read the complete held [original-only home contract](specs/steam-new-character-home-search-and-terrain-admission-20261108.md),
which supersedes the older start-spot summary in frontend-characters section 4.2.
The source author remains unexposed to implementation, tests and owner outputs.
The full classifier/registry comparator, numerical terrain/provider/global-random
closure, native scheduling and broad gameplay/native-pixel/performance acceptance
remain open. A successful engine search is not universal world/seed certification.

## Native creation state and commit ownership - 2026-11-08

New selection now resets the retained live Entity without applying the separate
load decoder's default-core assignment. Identity, separate sets/ranges, active
item, surviving bookkeeping and omitted widths remain intact. Reset and preliminary
HP/equipment precede home search; fresh character-owned data follows placement.
The immediate starter rebuild uses the reset zero seed. Later styles use the
current identifier's low word and surviving region operands. Account publication
is a separate ordinary update step.

Style changes replace the starter inventory, cursor, counters, equipment and
freshly computed HP. The exact template list has 19 entries for Warrior and 20
for the other classes; the previous extra material was incorrect. Creation's AI
visits the current owned inventory directly. A shared finite copy/reset/capture
path avoids separate load/creation definitions and per-frame inventory snapshots.
The complete core-copy regression covers 2,302 copied and 114 omitted bytes.

Create now captures the current actor's raw position, HP, experience,
specialization and buffs. It sets the name and level and inserts the live home
key without another heal or placement. Ordinary names narrow each UTF-16 unit to
its low byte, including the first zero-byte terminator; the engine bounds oversized
or malformed input safely. The successful writes remain count, numeric record,
then selected slot. Disk receives full character-owned data; the later list
snapshot has its own Entity identity/defaults and copies only the specified core,
header/templates, inventory and buffs. The live actor survives Help and Main
adopts its current owned data through a separate creation path.

Twelve pre-change starter failures were reproduced. Expanded reset/copy/seed/
canonical-inventory/raw-commit/Unicode/disk/list checks, all five targets, full
required-owner CPU without skips, every Vulkan group and SDK independence pass.
Actual hidden writable and read-only creation pass; SQLite tracing verifies the
first three writes and the complete first blob. Alpha/Omega read-only creation
also passes. Both read-only runs leave their databases unchanged. Four existing
raw-load cases pass. Matched native and Alpha/Omega scenes pass in 42.625 and
82.438 seconds, with both images inspected and unchanged Alpha profile identity.

The implementer read the complete held
[creation contract](specs/steam-new-character-initialization-and-commit-20261108.md)
and [finite core-copy contract](specs/steam-selected-character-default-core-copy-20261108.md);
both hashes remain frozen. Safe backend recovery, typed defaults for unwritten
values, serialization ownership and engine startup boundaries remain adapters.
Full home-query/appearance/crafting closure, imported provider/global random state,
native update/input/draw/Help scheduling and broader gameplay/provenance acceptance
remain open. These checks do not establish native pixels, performance or reliable
cold-start timing. Alpha terrain/lighting and Omega water remain implemented.

## Client timeout and outgoing clock admission - 2026-11-08

The client now keeps connection, matching-sender, invitation and process-output
clocks distinct. Construction leaves the peer baseline at literal zero. Shared
connect samples after its request attempt and before publishing the peer/role;
matching-sender dispatch samples before decoding content, including unknown kinds
and short prefixes. Safe bounded decoding remains an explicit engine policy.
Invitation insertion precedes its own separate fresh timestamp.

After the complete fixed receive batch and core-reset notices, distinct diagnostic,
lazy send-clock initialization and age-comparison readings follow the held native
order. Strict unsigned age above 5000 takes the full local connection transition,
including when the selection is invalid or already local. That branch preserves
earlier batch effects and skips invitation expiry, ordinary output, request work
and the later duration diagnostic. The transition takes its own new clock reading.

On a nonfallback drain, the held comparison expires invitations strictly above
6000. The shared process send clock advances strictly above 32 before testing the
selected peer; a separate process request clock initializes and samples fresh only
inside an admitted send and advances strictly above 1000 before testing the zone
list. Both clocks survive client replacement. Snapshot storage and interaction
reads are deferred until their admitted output stages. The final strict-above-8
duration check is a diagnostic; it does not budget the receive batch or cancel sends.

Eight pre-change admission failures were reproduced. Expanded regressions cover
fresh sample order, insertion/Leave writers, unknown and short content, delayed
batch handlers, unsigned wrap, strict boundaries, empty request rounds, process
lifetime and deferred reads. Protocol fixtures now establish their intended clock
state explicitly; their packet payload/order/count assertions remain unchanged.
All five targets, full required-owner CPU without skips, all Vulkan groups and
extractor SDK independence pass. Four disposable raw-load/read-only cases pass.
Native and Alpha/Omega sessions complete in 54.328 and 97.703 seconds; both images
were inspected with matched settings and unchanged readiness/deadlines. The Alpha
profile is unchanged. These are bounded smoke checks, without native-pixel,
performance or reliable cold-start acceptance claims.

The implementer read the complete held
[original-only clock contract](specs/steam-client-timeout-clock-and-fallback-admission-20261108.md)
and froze its hash. Short synchronized engine clock storage, deterministic test
injection, world-identity negotiation and handling of a missing local snapshot are
explicit engine adapters. Provider resolution, native thread interleavings, socket
lifecycle and actual original-peer/two-account acceptance remain open. Alpha
terrain/lighting and Omega water remain implemented.

## Native Steam startup configuration - 2026-11-08

Normal Game-generated Steam sessions now configure the generator with input 123.
The old startup passed 0 while its game metadata already said 123. A shared native
startup constant now supplies both metadata and the generator's default entry;
explicit inputs remain valid. Existing Alpha composition keeps its established
input 0 and profile identity, with its own landscape seed separate. Captured worlds
continue using their recorded input.

The independent startup audit establishes 123 at both original World configuration
calls. A separate complete configuration audit distinguishes that stored argument
from the shuffled layout rows and 77 derived fields. It proves concrete consumers
and separate fixed-coordinate noise/name paths; it does not establish the imported
random provider's sequence or universal output dependence. The existing owner-data
oracle separately establishes input 123 with zone value 15376 and sand offsets
10211 and 5983.

Five before-fix checks reproduced the wrong startup input, three derived words and
unchanged source identity. The shared default now matches all four independent
values, and explicit 0 still restores its previous identity. The regression uses
the same default entry as Main; existing owner-generation checks remain separate.
Character-associated selection values and retained World data do not silently
reconfigure the generator.

This corrects newly generated native content previously initialized from 0.
Geographic or retained data from that experimental input is not automatically
translated. Alpha input 0 is a versioned extension policy, separate from native
input 123; its existing profile and captured-world inputs remain distinct.

All five targets, focused startup/load/persistence, full required-owner CPU without
skips, all Vulkan groups and extractor SDK independence pass. All four disposable
load/read-only cases also pass with the running engine reporting native input 123.
Native and Alpha/Omega scenes pass in 52.469 and 92.203 seconds; both images were
inspected with matched settings and unchanged deadlines/readiness. The Alpha profile
identity matches the prior build exactly. These remain bounded smoke checks.
The preceding entry-build periodic-write evidence is recorded below.

The implementer read the complete held
[startup selection contract](specs/steam-startup-world-seed-selection-and-overrides-20261108.md)
and [configuration/consumer contract](specs/steam-world-configuration-random-tables-and-derived-fields-20261108.md)
from the unexposed original-only author and froze both hashes privately. No native
world-selection reconfiguration, generic repeated-configuration table semantics,
provider/thread identity or complete world/theme/dungeon parity is claimed.

## Generation cadence entry after initialization - 2026-11-08

Character-save storage construction and source binding no longer sample the
generation cadence clock. An explicit entry point takes the three separate clock
readings described by the original contract; the second supplies the periodic
baseline. Repeated entry and quiescent source rebinding do not reset it. The strict
unsigned interval and the early iteration sample carried across generation remain
unchanged, including timer wrap and menu-gated overdue attempts.

The main session finishes selected-character loading into the actual actor after
UI creation, then enters cadence after the remaining derived setup and before its
first frame. The call is outside the World guard. This is the engine's explicit
startup boundary; native OS-thread handoff/message-loop interleavings, timer-domain
identity and precise scheduler behavior are not claimed. No additional timer
thread, per-frame snapshot or mode control is introduced.

Entry still takes all three readings when stop was already requested; no ordinary
cycle is then admitted and normal exit retains its once-only three-root-gated
writer. Storage that never entered has no worker-exit write. Explicit writes
remain independent of entry. Treating an aborted, unentered engine session this
way is a safe ownership policy, separate from native worker exit.

Two before-fix clock-boundary failures were reproduced. Delayed initialization,
distinct second-sample boundaries, repeated entry/rebinding, pre-entry stop,
never-entered storage and explicit writes now pass with the existing real-worker,
owned-database, unsigned-wrap and selective-load tests. All five targets and the
full required-owner CPU suite without skips, every Vulkan group and extractor
SDK independence pass. Follow-up on this entry checkpoint's tested executable
completed all four zero/negative/signed-zero/read-only runtimes. A disposable
periodic row changed at 62.782 seconds before the normal 89.610-second exit;
two complete blobs were checked. Native and Alpha/Omega scenes passed in 51.062
and 99.734 seconds and both images were inspected, with unchanged settings,
deadlines and readiness. These remain bounded smoke checks, not full native-pixel,
performance or reliable cold-start acceptance.

The isolated original-only author held the complete
[worker-entry contract](specs/steam-generation-worker-entry-and-process-mode-20261108.md).
It was read entirely and its hash frozen privately. The original worker's ordinary
process byte starts at zero; a supported producer of a nonzero value remains
unproved. This checkpoint adds no guessed producer. Other writers/guards, complete
initialization, exact thread/bank interleavings, backend durability and broader
native acceptance remain open. Alpha terrain/lighting and Omega water remain
implemented.

## Selected-character loading and live inventory - 2026-11-08

Native selected characters now load their saved fixed position, health, level,
experience, class, specialization, equipment, appearance, bookkeeping, buffs and
ordered history through one shared state path for writable and read-only sessions.
Zero coordinates are valid. Zero, negative, signed-zero and above-maximum health
remain raw at the load boundary; orientation is consumed and zeroed. Normal later
World/physics updates remain separate. A missing numeric record preserves the
destination rather than applying a reset.

The reset applies the independently specified default-core fields and clears only
the traced transient maps, queued interactions, active/cursor item fields and
other reset operands. Separate identifier sets, skill/summon ranges, group/pose
state and omitted padding survive. Safe bounds for overlong names and malformed
records remain explicit engine policies.

Controlled-player potion use now visits the current owned canonical inventory,
so it consumes the live slot after gameplay changes rather than a load-time copy.
NPC records retain their direct byte-record adapter. Both paths preserve first
match, item-copy/action order, the signed comparison after a wrapping 32-bit
decrement, and the two random draws. Shared ownership avoids a dangling inventory
provider; native sessions avoid a second authoritative inventory or per-frame
full copy. The decoded character is reused instead of reading the database twice.

The regression suite reproduced eighteen raw-load failures, two selective-reset
failures and one canonical-inventory failure before their fixes. Literal count
boundaries already passed on MSVC before the explicit wrap arithmetic; that change
removes C++ signed-overflow ambiguity without claiming a reproduced numeric failure.
All five targets, focused/default required-owner CPU tests without skips, all
Vulkan groups and extractor SDK independence pass. Shutdown test synchronization
now establishes stop-before-join before releasing generation; engine gates and
expected final writes were unchanged.

Four disposable load/save and read-only repetitions now pass on the subsequent
generation-entry checkpoint's executable. Zero/negative/signed-zero health remains
exact, zero coordinates load, writable orientation is zeroed, and both read-only
databases remain unchanged. Its periodic write and inspected native/Alpha/Omega
scene checks also pass. The earlier Steam sign-in failure is retained and excluded;
the user reconnected manually. No ownership check, deadline or scene setting was
weakened. Owner saves, Mods and display were untouched.

The isolated original-only author held the complete
[load contract](specs/steam-selected-character-load-state-and-health-20261108.md),
[reset boundaries](specs/steam-selected-character-reset-boundaries-20261108.md),
[default-core copy](specs/steam-selected-character-default-core-copy-20261108.md)
and [potion count widths](specs/steam-ai-potion-slot-count-widths-20261108.md).
The implementer read all four entirely and froze their hashes privately. Complete
caller/UI initialization, worker-start/process-mode/interleavings, alternate writers,
backend durability and broader native gameplay/multiplayer acceptance remain open.
Alpha terrain/lighting and Omega water remain implemented.

## Character worker saving and native roster - 2026-11-08

Native character saving now completes from the generation worker path. It uses
the strict unsigned interval greater than 60,000 ms and retains the early cycle
sample as the next baseline after a returned writer attempt. Periodic admission
checks the viewed name, the three selection/Start Menu/creation root values in
order, and running state. Rejected name/menu gates leave the interval overdue;
returned backend failure or a negative captured slot still consumes the sample.
Normal worker exit applies only the three root gates. Explicit writes stay separate.

Snapshots own the captured numeric slot, canonical inventory/data, raw current
actor fields, full equipment/buff records, current history and extension notes.
The World guard ends before serialization/compression and guarded database entry.
Database, selection and local-player storage survive their facades. No per-frame
full character/World snapshot or dedicated save timer thread was introduced.
Shutdown drains admitted source/mesh work outside World guards before final capture.

Flight points now read/write as a unique signed lexical pair set. Map-star groups
keep their signed order and each inner list retains order, duplicates and separate
unknown values. Four reproduced regressions cover both flight codec directions,
duplicate-star capture and native roster admission without a class recording.
Native saved characters now remain selectable without recordings; native metadata,
canonical equipment and existing rules supply their class and equipment. Available
recordings retain the compatibility playback route.

All five targets build. Focused/default required-owner CPU tests pass without
skips, as do all Vulkan groups and extractor SDK independence. Real TaskPool and
private-database tests cover carried time, ordered/exit gates, stop completion,
World-lock release, raw field words, owned lifetimes and captured-slot stability.
The disposable native runtime changed its numeric row at 63.031 seconds,
well before its normal exit at 90.797 seconds. Two complete saved payloads
verify canonical inventory, separate duplicate-star values, flight order and raw
bookkeeping words. Its zero-health final actor is saved unchanged. The initial
no-recording run made no numeric writes; that retained failure motivated the
roster regression and is excluded from the successful save evidence.

Fresh native and Alpha/Steam/Omega hidden offline/no-save scenes exit zero in
43.438 and 81.954 seconds. Both images were inspected against the tested
binary, with unchanged baseline settings, readiness rules and deadlines. These
are bounded smoke checks; no native-pixel, performance or reliable cold-start
claim follows. Owner saves, Mods and display were untouched.

The isolated original-only author held the complete
[worker/snapshot contract](specs/steam-character-save-worker-and-snapshots-20261108.md)
and [root/order addendum](specs/steam-character-save-ui-gates-and-flight-point-order-20261108.md).
The implementer read both entirely and froze their original-byte hashes privately.
The generation-owner baseline instant, native global process-mode branch and
attribute-bank/thread interleavings remain unverified. Equal-coordinate star
deletion identity, complete selected-character loading/initialization, other
writers/guards, backend durability and broad gameplay/multiplayer acceptance
remain separate work. Alpha terrain/lighting and Omega water remain implemented.

## Connection role and fresh-source restoration - 2026-11-08

The shared connection path now sets the viewed World's nonzero role after the
Join attempt, timer and selected-host publication. The same final operation runs
for local play, remote connections and current-peer Leave fallback. An ordinary
Steam fallback therefore remains a viewed mirror of the local host; it is not
treated as the separate native socket/thread disconnect routine.

Fresh physical-source restoration now follows that live role. A guarded scalar
update changes future factory admission without replacing retained values, live
payloads, metadata or pending dirty membership. Nonzero role skips all five saved
fields and edit replay after raw generation. Returning to zero permits restoration
on a new source; an already loaded hit keeps its identity and current fields.
Retirement recording remains ungated. Explicit suppression in compatibility
adapters stays separate, and binding an archive respects an already selected role.

Main no longer writes the viewed role before connecting or clears its physical
retained map at that boundary. Captured playback keeps its separate archive
adapter. Connection resets and source-request timing keep their existing phases;
pending packet7 categories survive fallback. The callback borrows its World only
synchronously, and the source worker retains no raw Actor pointer for this gate.

Five pre-change regressions reproduced missing live factory admission and absent
local/remote/fallback role writes. Focused and full required-owner CPU tests pass
without skips. The tests include real physical reloads, COW/loaded-hit identity,
all-five-field retention/restoration, dirty membership, metadata and archive
survival, and reset/own-host-reset/Join/peer/role order. An initial fixture printing
escape and a transposed raw-column index were corrected without changing engine
rules or expected block values.

All five targets, every Vulkan group and SDK independence pass. Fresh hidden,
offline/no-save native and Alpha/Steam/Omega scenes exit zero in 41.250 and 82.062
seconds; both images were fully inspected against the tested binary. No settings,
readiness rule or deadline changed. These are bounded scene checks, not native
pixels, startup reliability, performance or general multiplayer acceptance.
The original game and owner saves/Mods/display were untouched.

The independent [role/name/restore contract](specs/steam-world-role-name-and-restore-gates-20261108.md)
was finalized and held by the isolated original-only author, then entirely read
by the implementer. Its original-byte hash is frozen privately. The earlier
retained-field and connection contracts remain immutable. Complete initial
configuration/database behavior, actual socket/thread teardown, other role
writers, collision/rekey lifetimes, broader guards and character-worker
scheduling remain separate work. Alpha terrain/lighting and Omega water remain.

## Native per-World clock - 2026-11-08

Both gameplay Worlds now advance their own clock at the start of each admitted
World substep. The normal path uses separate binary32 operations for unscaled
elapsed time and the native 10x/100x calendar, the proved actor predicate and local
healing/reference-call order. Packet8 still overwrites the viewed calendar later;
it no longer has to supply every between-packet advance.

Strict day crossings store the incremented day and float remainder before each
role-gated actor/metadata reset and zero notice. Equality remains at one full day;
multiple crossings retain their separate resets. The existing later notice reset
still runs separately. Borrowed actor views rebuild after cleanup, and per-step
upkeep/interaction snapshots receive the new day/time. An empty configured World
name assigns 9am even on a zero-input call. Authority generator-cache updates
remain an engine adapter; an independent viewed crossing does not change them.

`[Time] Mode=Native` is now the default, including when the key is absent. This
replaces the former host-only 60x default. `Mode=Custom` retains the optional
constant double-rate calendar selected by `MinutesPerSecond`; zero freezes that
calendar. Custom mode keeps its separate clock behavior in place of the native
actor-rate/healing path. `StartHour` remains available. Alpha terrain/lighting,
Omega water and all other settings are unchanged.

Three initial actual-World regressions reproduced missing viewed stepping and
incorrect role-nonzero resets/notices. Literal float-word tests now cover separate
rounding and non-reassociated healing, fresh second/third references, rejected
actor predicates, empty-name/zero-input behavior and strict repeated crossings.
Actual Host/viewed tests cover substeps, capped time, clock packets, cleanup/view
lifetimes and the later notice pass. One phase-observation fixture originally
used a role that cannot run its authority-only callback; correcting that fixture
changed no numeric expectation or engine predicate. Explicit float fixture
literals subsequently removed a compiler warning without changing their words.

All five targets build; focused clock and full required-owner CPU tests pass
without skips. All Vulkan groups and extractor SDK independence pass. Fresh hidden
offline/no-save native and Alpha/Steam/Omega captures exit zero in 43.546 and 82.906
seconds, respectively; both images were fully inspected against the tested binary.
These bounded checks do not establish native pixels, fixed scheduling, performance
or general startup reliability. The prior Alpha deadline failure remains recorded.
No original game, owner Save/Mods/display change or binary publication was needed.

The isolated original-only author finalized and held the independent
[clock contract](specs/steam-world-clock-units-and-update-20261108.md).
The implementer read the entire prose and owned source only; its original-byte
hash is frozen privately. Role/name/restore setup, exact worker scheduling,
other clock writers, complete stat-provider re-audit, abnormal/reentrant paths,
connection/rekey lifetimes and broad native runtime acceptance remain open.

## Viewed packet dispatch and frame boundaries - 2026-11-08

Viewed gameplay packets now dispatch after World stepping, drawn-position work,
previously pending event application and local input, before the current camera
and renderer work. The earlier phase advances transport and the independent host
only. Current clock/identity/entity changes therefore reach later presentation
without rerunning the completed World batch. Newly received packet7 category
lists wait for a later frame's earlier consume pass.

The client takes one owned packet batch before applying gameplay callbacks.
Immediate3/4/6/8 operations are forwarded to the World adapter in queue order,
with current-peer admission checked for every entry. A peer transition can reject
later old-peer entries and admit the new peer in that same batch. Source-request
coordinates are sampled before dispatch. Sending occurs in the late phase;
current local events are collected after renderer submission and the World event
group is then cleared. They become eligible for a later send.

Loopback and UDP extract their queued entries under one guard. The existing Steam
backend finishes its available-byte drain before gameplay callbacks; its precise
worker/ingestion timing and cross-producer ordering remain separate work. Owned
batches, synchronous interface borrowing and per-entry payload lifetimes avoid
async callbacks into caller state. The low-level value Inbox remains a protocol
adapter. Broader native connection-role, complete rekey/collision and transport
lifetime behavior are not claimed by this scheduling change.

A real session/World/packet regression reproduced four earlier ordering failures.
It now passes next-frame category application, late day reset after prior death-ID
merge, mixed immediate packet and identity order, peer transitions, callback
packets deferred outside the batch, pre-dispatch requests and later own-event
sending. The existing protocol fixture now asserts no host effect at collection
or the subsequent send, then exactly one hit/sound at host application. A first
fixture compile used an absent convenience constant and was corrected without
changing an engine predicate.

All five targets build; the full required-owner CPU suite passes without skips,
including region-sharing and earlier source/persistence cases. Vulkan checks pass:
92 shader, 2 camera, 102 GUI, 17 map, 14 native-water, shared-water and Alpha-scenery
groups, plus SDK independence. The hidden offline/no-save native process exits
zero in 48.125 seconds. The first Alpha process exits one after 131.297 seconds at
its readiness limit without an image; the unchanged binary/settings/deadline retry
exits zero in 82.907 seconds.
Both successful images were inspected for terrain, vegetation and HUD. The first
failure remains evidence of variable cold-fill readiness; no speedup or original-
pixel/runtime acceptance is inferred. No diagnostic deadline or readiness rule
was relaxed. Alpha/Omega defaults and other enhancements remain supported.

The independent [phase contract](specs/steam-viewed-network-dispatch-phases-20261108-v2.md),
[identity transitions](specs/steam-client-connection-identity-transitions-20261108.md),
[collision limits](specs/steam-local-identity-collision-lifetime-20261108.md) and
[late read-only visit](specs/steam-late-world-event-consumer-20261108.md) were each
finalized and held by an original-only author, then entirely read by the clean
implementer. Four original-byte hashes are frozen privately. The late visitor
has no proved award mutation; no invented operation was added for it. Complete
role-byte/restoration gates, safe rekey lifetime policy, clock arithmetic,
character-worker scheduling, untraced writers/guards and broad native acceptance
remain open.

## Viewed World metadata reset - 2026-11-08

An incoming clock message whose day differs now clears both the viewed World's
death identifiers and retained source map before the new day/time are stored.
Previously the mirror cleared only death IDs, leaving retained zone state behind.
Same-day messages update the time without resetting metadata; a backward day
change reaches the same reset boundary as a forward change.

The reset preserves reward-marker IDs, current day/time until the later stores,
live residents and actual loaded source identity, arrays, flags and summaries.
The broader authority reset reuses this metadata operation while retaining its
separate actor cleanup and optional enemy-respawn policy.
A real loopback client and physical-source fixture reproduced two prior failures.
It now passes those cases and checks reset-before-clock order, same-day admission,
backward days, peer rejection, reward retention and loaded payload/resident
survival. Packet7 checks cover deferred victim-ID application, zero/negative/
minimum signed64 values, duplicate membership and preservation of all event
records independently of the credit identifier. The initial fixture compilation
used a constant private to another test and was corrected without changing an
engine predicate. Main-frame packet-dispatch timing remains separate work.

All five targets build. Focused and full required-owner CPU checks pass without
skips, including the earlier persistence and optional respawn groups. Vulkan
checks pass: 92 shader, 2 camera, 102 GUI, 17 map, 14 native-water, shared-water
and Alpha-scenery groups. SDK independence passes. No new live scene capture was
needed for this metadata change; the earlier startup images retain their own
executable/evidence scope. Native runtime/save-travel acceptance remains open.

The isolated original-only author held the complete
[identifier-writer contract](specs/steam-world-identifier-state-writers-20261108.md).
The implementer read that code-free prose and owned source only; its original-byte
hash is retained privately. Individual-ID erasure, other reward producers,
untraced callers/guards and broader native runtime parity remain unresolved.
Alpha terrain/lighting, Omega water and the other enhancements remain supported.

## Host retained-state persistence - 2026-11-08

Generated host sessions now capture state from the source loader's own wall
clock. The unsigned elapsed threshold is strictly greater than 60,000 ms. Each
cycle samples after its centre snapshot and before source retirement/generation;
a due capture follows the admitted batch while the loader is running. After the
wrapper returns, even when writing is gated or fails, the baseline becomes that
cycle's sample. Large elapsed gaps cause one capture rather than catch-up writes.
The accelerated World calendar does not drive this threshold.

The loader copies the already-retained map, current day, raw float time and both
identifier sets under the owned World guard, then serializes through the existing
state codec outside that acquisition. Shared owned metadata survives actor and
input-save lifetimes without a raw callback into either object. Runtime metadata
is initialized explicitly; binding a retained source archive does not merge old
identifier sets into it. The two World reward sets are restored separately.

Normal source-worker exit drains its admitted work and captures once before its
reader's later teardown records still-loaded sources. Periodic and final captures
do not first record every loaded source. Generated character saves consequently
write character output separately; the captured-playback adapter retains its
explicit archive-save route. Write failures return through an owner-consumed
error queue, and no-save never opens its target or creates a directory.

Controlled real-worker/file tests cover strict boundaries, unsigned wrap, gated
and failed-write baseline reset, large gaps, calendar independence, final capture
before later retention, deep snapshot independence, raw float/identifier/map
round trips, reader reuse and metadata lifetime after actor destruction.

All five targets build. The new focused tests and full required-owner CPU suite
pass without skips. Vulkan checks pass: 92 shader, 2 camera, 102 GUI, 17 map,
14 native-water, shared-water and Alpha-scenery groups; SDK independence passes.
Hidden offline/no-save Steam/native-water and Alpha/Steam/Omega captures exit zero
in 43.438 and 94.984 seconds, respectively. Both images were inspected against
the tested executable. Ground, vegetation and HUD remain present; the existing
oversized map-like triangle remains visible. Native combat, original pixels,
performance and broader save/travel acceptance are not inferred from these runs.
No original game, owner Save/Mods/display change or binary publication was needed.

The separate original-only author finalized and held the complete
[persistence](specs/steam-host-retained-state-persistence-20261107.md),
[save-caller](specs/steam-world-state-save-callers-20261107.md) and
[initialization](specs/steam-world-state-initialization-20261108.md) prose; the
implementer read it without original or reconstruction code exposure. Four held
hashes, including the earlier same-Host teardown audit, are retained privately.
Backend durability, alternative save callers, complete initialization/reload,
character-worker scheduling, untraced identifier writers/guards and broader
native runtime parity remain open. Existing migration, optional respawn and
captured-playback behavior are separate engine policies. Alpha terrain/lighting,
Omega water and other requested enhancements remain supported.

## Physical source payload ownership - 2026-11-08

Generated Worlds now bind ground/static arrays, ordered edit history, coordinate
counters and the tracked scalar to the actual loaded source identity. Raw
production precedes retained restoration and edit replay; final installation
publishes the restored payload. COW terrain replacements share that lifetime.
Existing-source hits and archive rebinding preserve the live payload and sticky
flags. Captured-data playback keeps its separate logical adapter.

Viewed retirement records before unlinking; required-set host retirement unlinks
before recording. Bulk teardown clears loaded membership before per-source
recording. Source allocations are released after the owned guards. Alpha's LRU
adapter also records its actual retiring payloads. Logical demand cleanup no
longer substitutes for these boundaries. Old held leases cannot make a retired
source count as loaded, and fresh reloads get a fresh identity and history.

Source summaries and daily catalog ownership now follow that same physical
instance. Keyed/scalar consumers prefer loaded values, including empty/zero,
then retained values for absent sources. Dirty memberships are separate from
sticky flags: publication snapshots current arrays in source order, including
empty replacements, runs the final object pass and clears memberships. Incoming
ground replacement creates no first-array membership; object replacement does.
The mirror's existing network adapter continues suppressing outbound array echo.

One owned recursive World guard protects mutable views and compound owner work;
immutable terrain reads keep their existing path. Ordinary frame submission and
worker joins run outside the broad owner scope; short scenery/light reads keep
their required guards. Save input is copied into owned storage, and the
owner explicitly copies retained values back for the existing save path. No
background callback captures a caller's save object or actor container. Retired
pending identities are discarded safely; native per-source dirty-set erasure
and arbitrary native race behavior were not established by the separate audit.

Ten focused groups and final all-five-target builds pass. The full required-owner
CPU suite reports no skips; all Vulkan groups and the SDK-independence audit pass.
Controlled cases cover both actual retirement paths, five-field empty/zero
replacement, restore suppression, repeated edits, COW, rebinding, old leases,
reloads, separate Worlds, ordered publication and a blocked concurrent remover.
The first fixture used the API's disabled radius zero and was corrected to one.
Two generated-gate fixtures now mutate the owned World retained map rather than
the external input copy; their state, footprint and sound assertions still pass.

Hidden offline/no-save startup captures complete in 46.500 seconds for Steam look/
native water and 90.235 seconds for Alpha terrain/Steam look/Omega water. Both were
inspected for ground, vegetation and HUD against the tested executable. The native
test player takes damage during the wait, and the preexisting map-like triangle
remains visible. These are bounded startup checks. Variable startup/FPS, broad
original visual, travel/memory and hardware acceptance remain open.

See the independent [physical boundaries](specs/steam-physical-source-retention-boundaries-20261107.md),
[dirty producers](specs/steam-source-array-dirty-producers-20261107.md) and
[pointer retirement](specs/steam-source-dirty-pointer-retirement-20261107.md).
Native persistence cadence/capture ordering, complete connection and prerequisite
behavior, untraced producers/guards and broader Steam parity remain open. Existing
explicit archive-save behavior is separate from the native loader persistence
work. Alpha/Omega defaults and the other requested enhancements are preserved.

## World-step admission and selected movement membership - 2026-11-07

Each ordinary admitted substep now completes its source summaries and object
prelude before resident admission/retention and the existing death, pet and body
phases. Pending births settle before admission; actor views refresh afterwards,
including when new registrations or removal move owned storage. Host and viewed
callers use the same boundary, including a zero remainder. The existing 40-ms AI
policy remains separate; this does not establish native AI-thread scheduling.

Movement now receives the separate prelude-selected resident list. Selection uses
the independently specified flag, positive-health and special-variant predicate
in registration order. Surviving members expose their current fields without an
eligibility recheck; later registrations wait for a subsequent rebuild. Stable
owned registration identities prevent numeric ID reuse from substituting another
actor after storage moves. Missing instances are skipped safely, an engine policy
where the original's deleting-virtual side effects remain unproved. Late reset
clears all selected membership, including surviving records.

Focused checks cover per-substep object clocks/admission, zero elapsed, the viewed
role gate, registration growth/removal, fresh body views, selection boundaries,
order, relocation, ID reuse and reset. An actual movement/contact integration
fixture verifies that admission-time position/eligibility changes remain visible
to earlier selected members while a newly registered ladder enters selection only
on the next prelude. The full required-owner CPU suite without skips, every Vulkan
group and the SDK-independence audit pass. All five targets build.

Native and Alpha/Steam/Omega hidden offline/no-save startup captures complete and
show loaded terrain, vegetation and HUD. Startup performance remains open:
unprofiled matched old/new runs take 56.265/82.344 seconds and, in reverse order,
47.485/65.282 seconds. A profiled pair takes 46.047/44.609 seconds. CPU totals are
similar; these variable observations neither resolve the slower unprofiled startup
nor establish FPS or a speedup. The Alpha capture takes 114.703 seconds. Broad
original-pixel, travel/memory and performance acceptance remain separate gates.

See the independent [phase-order contract](specs/steam-world-step-phase-order-20261107.md)
and [selected-pointer contract](specs/steam-world-selected-pointer-lifetime-20261107.md).
Complete time/day and optional-owner phases, view admission fields, native guards,
physical retained-state binding and persistence remain open. Alpha/Omega defaults
and other enhancements are preserved; this is a bounded source checkpoint.

## Live-resident horizontal retention - 2026-11-07

Resident removal now uses the inclusive horizontal 320-block disk around every
registered byte-zero centre. It ignores Z and health, admits every nonzero
category as a candidate, and removes all such candidates when no centre remains.
All candidates are selected in World registration order before owned storage is
changed. Stable unique engine IDs allow vector erasure to move later records.

The zero-role admission path runs this pass after its complete resident admission
and reconciliation. Source demand no longer bulk-deletes a nonempty resident
bucket; only empty storage releases its catalog bookkeeping at that boundary.
Individual removal releases owned records and AI/activity caches while preserving
recorded world progress. This separates the resident predicate from physical
source demand, mesh readiness and source-array retention.

Asset-free boundary and owned-world tests cover exact axial/diagonal boundaries,
fixed-point oversteps, ignored height/health, multiple centres, empty centre lists,
registration order, fresh centre membership and vector relocation. Two older
respawn fixtures needed explicit NPC positions beside their test player; their
identity-suppression and daily-refresh assertions remain in place. Final all-five-
target builds, both focused modes, the full required-owner CPU suite without
skips, all Vulkan groups and the SDK-independence audit pass.

Hidden offline/no-save Steam-look/native-water scene startup completes at the
negative-half-block position in 41.266 seconds; Alpha terrain/Steam look/Omega
water completes in 79.984 seconds. Both images were inspected for loaded terrain,
vegetation and HUD, and match the tested executable hash. These are bounded smoke
checks, not FPS or original-pixel acceptance. See the
[retention contract](specs/steam-live-resident-retention-20261107.md).

Complete World-step ordering/guards, external reference cleanup, physical source-
state binding and native persistence remain open. The owned catalog leases and
broader travel/memory acceptance retain their separate evidence limits.

## Source summary identity membership - 2026-11-07

The selected-source prelude now rebuilds coordinate counters from catalog-entry
identities present in the World+0x20 lane, represented by `worldDead`, instead of
querying the admitted live-actor map. Its separate tracked-list scalar is reset
and rebuilt independently, counting each matching list entry, including repeats.
The existing projection of each template's coordinate-key tree is preserved.
Source arrays, saved override flags and edit history are untouched by this pass.

A generated gate regression failed when the relevant catalog identifier was
recorded but no matching live actor had been admitted. It now opens and publishes
its state. Value checks cover the outer catalog key versus the template's own ID,
shared coordinate keys, the independent tracked list, repeated list identities,
and authoritative empty/zero replacement. All five targets, the full required-
owner CPU suite without skips, Vulkan image groups and the SDK-independence audit
pass. No extra startup screenshot was required for this producer-only change.

The older guard contract's membership address is World+0x20; its use of “live”
does not establish a lookup in World+0x450's resident map. The independent
[summary contract](specs/steam-source-prerequisite-summaries-20261107.md) explicitly
separates these identity lanes and the two source input collections.

This closes the selected producer correction. Full physical summary ownership and
freshness, loaded-versus-retained consumer routing, broader prerequisite control,
and ordinary update/admission ordering remain open alongside the other retained-
state and persistence work. This is not complete resident or gate parity.

## Physical source request/control lifetime - 2026-11-07

The state-request flag now belongs to each loaded physical source identity. Fresh
sources start flagged; COW terrain replacements share that control, while source
retirement and reload establish a fresh flag. A coordinate reply clears only the
currently loaded source, and a reply received while absent cannot suppress a
later source's requests. Logical ground/object list creation and retirement no
longer own a separate request-key set.

The viewed request scan uses loaded sources in its X-major nine-by-nine square.
Player fixed positions truncate and narrow before the bounds are built; eagle
flight substitutes its destination block point. Selection neither creates sources
nor changes their flags or recency. The existing viewed role predicate skips the
scan when zero. Connection reset flags every loaded source, including those with
no logical lists or edit history, and reverses existing loaded edit journals.

Host replies additionally require an existing physical source, whose lease is
retained while reply values are copied. Old logical lists cannot admit a reply
for a missing physical source. Reply admission does not depend on host F8.

Nine pre-change regressions pass, with fresh/repeated requests, loaded-only
clears, COW identity, factory hits, empty-history reset, retire/reload with old
leases, stale logical host lists, ordered bounds, negative fractions, eagle input
and reverse edit replay. Full required-owner CPU, Vulkan groups and extractor
SDK-independence checks pass. See the independent
[F8 contract](specs/steam-source-f8-control-lifetime-20261107.md) and
[reply mapping](specs/steam-source-state-reply-mapping-20261107.md).

Hidden offline/no-save scene checks also pass: Source=Game with Steam look and
native water at the negative-half-block start in 44.953 seconds; Alpha terrain
with Steam look and Omega water in 80.078 seconds. Both images were inspected for
loaded terrain, vegetation and HUD; their executable hashes match the tested
build. These are bounded startup checks, not FPS or original-pixel acceptance.

This closes the selected physical control and host-admission paths. It does not
close all World+0x358 writers, connection timing/queue phase, full five-field
physical binding, wider synchronization, logical residency or periodic persistence.

## Retained source helper flags and gates - 2026-11-07

Restoring a saved ground-item array now preserves its source override flag, as
already done for static objects. This includes an explicitly empty override.
Later replacement and retirement therefore retain that value, and host source
replies include the saved array instead of omitting it. The five-field recording
routine already matched the ordered contract and was left unchanged: once a
retained entry exists, empty edit/count containers and a zero scalar replace its
previous values, while array flags remain sticky.

The restoration predicate is now named `skipRestore`. It represents the nonzero
World+0x358 gate, independently of dedicated-host status. It suppresses restoration
of generated values but does not suppress recording on later unload. Both actual
standalone and host callers still pass their existing false value; this is not a
claim of a previously broken dedicated-server session.

Seven reproduced helper failures now pass. Focused checks cover ordered edit
replay, both restored flags, explicit empty overrides, all five retained fields,
and recording with restoration suppressed. The local host-reply path additionally
checks nonempty and explicitly empty restored ground arrays. Full required-owner
CPU, Vulkan image groups and SDK-independence checks pass. See the independent
[field-lifetime contract](specs/steam-retained-source-field-lifetimes-20261107.md).

This closes bounded helper semantics. Binding all five fields to physical source
residency, full connection/publication ordering, logical residency and native
periodic persistence remain open.

## Prepared-scene capture readiness - 2026-11-07

The engine's capture/benchmark readiness test now distinguishes required source
work from repeatable background fringe loading. Native source demand truncates
fixed positions to whole blocks, while retirement floors them to zones. At a
negative fractional boundary their circles can differ. All native candidate
selection, ordering, submission and retirement continue unchanged; missing normal
inputs beyond the physical retention circle no longer keep a prepared screenshot
waiting forever. No extra player-provider samples were added.

Cold source work, required mesh jobs and dirty meshes still block readiness.
Changed physical positions, destinations, view radii and source-distance settings
invalidate an earlier background-only decision. Destination demand remains
conservative. This is an engine-owned diagnostic policy, rather than a new claim
about original Steam's worker quiescence or synchronization.

A real small generated-world regression reproduced the failure with 14 prepared
zones, 0 mesh jobs, 40 loaded sources and 4 source jobs: all 36 retained inputs were
present among 45 demanded inputs. The corrected case becomes ready while its
background batch continues. Cold work and changed-input checks also pass. All
five targets, six focused groups, the full required-owner CPU suite without skips,
all Vulkan groups and extractor SDK-independence audit pass.

The default-distance Source=Game/Steam-rendering/Native-water scene now exits
zero in 41.140 seconds with a viewed ground/trees/NPC/HUD image. An explicit
negative-half-block start also exits zero in 40.336 seconds, with a viewed ground/
vegetation/HUD image and final physical position (-0.5,-0.5). Private settings
match the earlier native case byte-for-byte; the second case changes only its
position arguments and output paths. The preceding 126.205-second run reached
its observation deadline without an image. These are bounded scene observations;
changed readiness criteria are not evidence of a generation speedup or FPS gain.
Original pixel, wider cold/travel/memory and other native release gates remain.

The final hidden offline no-save Alpha-terrain/Steam-look/Omega smoke case also
exits zero in 79.760 seconds with a viewed terrain/vegetation/HUD image. All three
measured executables match the compiled hash. Shipped Alpha terrain/lighting and
Omega configuration remain; original game saves, mods and display settings were
unchanged and the original game was not started.

Capture here means a screenshot of the nearby prepared scene. Procedural Game
and Alpha modes generate other regions from the seed as the player travels;
reference captures are comparison samples, not a whole-world seed bake.

## Serial host source-loading cycle - 2026-11-07

Generated host sessions now own a separate source worker. Host steps publish
physical/eagle centres; each worker cycle copies the latest publication, builds
the required union, retires every loaded source outside it, then collects missing
keys. The native signed-priority sorter and minimum admitted rectangle-distance
key select at most four serial factory requests. Hits during collection consume no slot; hits after selection keep their slot.
The admitted prefix finishes before fresh demand or shutdown. Empty demand
retires all sources; an idle cycle waits five milliseconds and yields. Worker
exit joins without performing an extra required-set retirement.

New host sources preserve canonical raw bytes until an actual mesh prepass.
Host/client mutable readers and loaded maps remain distinct. Immutable leases
survive membership removal, derived memos are invalidated, and Alpha's finite
source admission remains. Capture replay and standalone compatibility readers
retain their adapters. Thread ownership, producer publication timing, startup
adapter prehistory and exception delivery are explicit engine policies, with no
new native raw-pointer or frame-wide atomicity claim.

Controlled real-generator tests cover a held first request, changed demand,
joined stop, completion of all four old-prefix requests, existing-hit exclusion,
nearest-key admission, empty retirement, retained leases, memo fallback and
owner-visible worker errors. All 17,000 stored records in the checked host zone
match the canonical raw generator after effective-query normalization. This is
a stage/lifetime regression, not a new original-game runtime oracle.

All five targets, seven focused groups, the full required-oracle CPU/owner-data
suite with no skips, all Vulkan groups and SDK-independence audit pass against
the final finite-domain build. A hidden offline no-save Alpha-terrain/Steam-look/
Omega capture completes in 79.768 seconds with a viewed terrain/vegetation/HUD
image and matching executable hash. The repeated default-distance Steam-terrain/
Native-water case still reaches its internal observation deadline in 126.205
seconds: 665 prepared zones, 0 mesh jobs, 754 loaded sources and 4 source jobs; no
image. Native default-distance capture acceptance stays open. A source-boundary
churn/readiness explanation is under investigation, not established acceptance.

The [host-cycle contract](specs/steam-host-source-cycle-20261107.md) separately
identifies logical admission, 320-block live-resident retention, the World+0x358
instance histories and selected-state persistence. Complete five-field retained
state, connection-dependent restoration, logical admission/retention alignment,
periodic native persistence and broader visual/travel/memory/FPS checks remain
open. The source loader checkpoint does not close those separate gates.

## Resident auxiliary model-light copies - 2026-11-07

Auxiliary light models now belong to the loaded source zone as independent value
copies. Mesh preparation updates every cached scalar and OR-only water flag after
the block-light solve and before terrain faces. Point-light preparation reads the
resident immutable data without parsing files, resampling light or creating source
zones. Changed samples publish a replacement; unchanged samples reuse the vector.
Ordinary-model erasure leaves these independent copies intact. Held old copies
survive retirement safely, while a new residency receives fresh source inputs.

The capture adapter reads ordinary and auxiliary models and static-light records
in one bounded pass per source load. File changes take effect on reload; duplicate
auxiliary tags and wrong strides remain rejected. Generated inputs retain their
existing classifier order and an explicit engine pre-mesh placeholder; no universal
native initialization is claimed. Static-object live lighting remains separate,
and incomplete capture replays retain their existing unbound-light fallback.
The [auxiliary contract](specs/steam-auxiliary-model-lifetime-20261107.md) records
value-copy construction, classification replacement, cached point consumption,
residency teardown and limits. It does not establish every alternate writer,
automatic classification after edits, native races or full visual acceptance.

Seven reproduced lifecycle failures now pass, covering repeated reads, file and
terrain changes, remesh replacement, retirement and fresh residency. Further
regressions check ordinary erasure independence and reuse after an unchanged mesh.

All five targets, six focused groups, the full required-oracle CPU/owner-data
suite with no reported skips, every Vulkan group and SDK-independence audit pass.
A separate owner-capture oracle matches all 2 model lights and 3 static lights
across five complete neighborhoods, with zero mismatches. Two other nonempty
snapshots lack neighboring captures and were excluded; they remain unqualified.
This does not establish broad native pixel or source-lifetime race acceptance.

The final hidden offline no-save Alpha-terrain/Steam-look/Omega smoke run exits
zero in 83.762 seconds, with a viewed terrain/vegetation/HUD image and matching
compiled executable hash. This is a loaded-scene check, not an FPS measurement
or original pixel comparison. No installed-game saves, mods or display settings
were changed, and no original game was started.

## Ordinary model light preparation - 2026-11-07

Ordinary 64-byte model records now receive their float light sample and OR-only
water flag during the actual mesh stage, after stored block-light writeback and
before terrain faces and scenery filtering. Every record participates, including
beam records, missing resources, invalid draw parameters and previously positive
or nonfinite light values. Scene preparation reads the stored scalar divided by
255; terrain setters alone do not reset or resample it. The sampled point is the
record position itself with downward fixed-coordinate conversion, without a
height offset or the separate live-query minimum of five.

The engine publishes changed model bytes and stored block light through one
immutable replacement transaction. Held model snapshots and loaded-zone identity
remain valid; unchanged samples avoid a vector copy. This safe ownership policy
does not claim native atomicity or concurrent raw-pointer behavior. Water tests
read the mesh job's captured neighborhood without loading more source zones.
Static-object live light and auxiliary model copies remain separate paths.
The auxiliary stage/lifetime integration and broader native visual acceptance remain
open. No universal native pre-mesh light default is inferred; generated engine
records keep their explicit safe placeholder until preparation.

The new public-API regression reproduced 34 failures before the correction. It
covers all-record preparation in Steam and Alpha render configurations, negative
fractional positions, outside-cache sky, below-column zero, poisoned old samples,
water normalization and sticky flags, retained snapshots, scene reuse, later
remesh replacement and the distinct gameplay minimum. Alpha scenery still uses
its separate rendering light calculation. See the independent
[model-light contract](specs/steam-source-model-light-stage-20261107.md).

All five targets, the full required-oracle CPU/owner-data suite with no skips,
and all Vulkan groups pass: 92 shader, 2 camera, 102 GUI, 17 map, 14 native-water,
shared water and Alpha scenery. The extractor SDK-independence audit also passes.
Focused checks cover model light, block edits, terrain/light sources, source
streaming, creation bytes and whole-batch preparation. A hidden no-save Alpha-terrain/Steam-look/Omega full-load smoke run completes in
76.895 seconds with a nonblank terrain/vegetation/HUD image. Its executable hash
matches the build. No broad native pixel, travel, memory or frame-performance
acceptance is inferred from these checks.

## Native source bytes before mesh lighting - 2026-11-07

Native client source creation now retains the canonical generator's per-path
bytes instead of unconditionally running a neighborhood light solve. Actual mesh
preparation still solves and publishes stored light. No uniform zero/white byte
rule is introduced; retained edits, effective block queries and compatibility
readers keep their separate behavior. Alpha rendering, capture replay and the
current host adapter retain their eager policy. See the independent
[initial-byte contract](specs/steam-source-initial-block-light-20261107.md).

The isolated public API test reproduced 20,303 premature byte changes among
44,485 stored records in three zones. The corrected path has zero mismatches and
performs three requested raw generations, versus 27 before. The reference is the
independently implemented canonical generator; this is a stage-boundary check,
not a new original-runtime capture or a universal transitive-helper proof.

In matched 30-second Alpha-terrain/Steam-look/Omega runs, generations fall from
685 to 367, excess productions from 370 to 31, and completed mesh jobs rise from
164 to 250. Timing sums are overlapping elapsed job time. With profiling off,
the matching full-load capture completes in 77.511 seconds total versus 130.589
before this change; both exit zero with nonblank images. Arguments/settings match
except private output paths and the measured executable hash matches the build.
A minimum-distance Steam-terrain/Native-water process exits zero in 8.653 seconds,
but its mostly-sky image gives inconclusive ground coverage. Repeating at default
distance reaches the observation deadline in 125.891 seconds with 665 prepared zones,
767 loaded sources and 4 source jobs, without an image. Native default-distance
cold-load acceptance remains open; these cases do not qualify the Alpha result.

All five targets, full required-oracle CPU with no skips, Vulkan groups
(92 shader, 2 camera, 102 GUI, 17 map, 14 native-water, shared water and Alpha
scenery), source-streaming regressions and extractor SDK independence pass.
No original game was started or owner saves/mods/display changed. Evidence is private
under build/source-creation-validation. Broader travel, memory, FPS and native
visual acceptance remain open, as do complete retained-state/remote gates and
native host source-loader work. Alpha terrain, lighting, Omega water and other
enhancements remain configured.


## Native loaded sources and independent preparation - 2026-11-07

Generated Steam-look clients now load source zones separately from mesh work.
Four-coordinate source batches use the native footprint, signed rectangle-distance
priority and shared key-only sorter. Mesh work requires loaded neighbors, captures all nine pointers under one source
cache guard and does not generate padding. At a completed mesh boundary, eight independent physical
center samples each permit one native distance-based retirement. Active geometry
follows loaded-source membership; snapshots and Vulkan retirement keep their
separate safe lifetimes. Alpha terrain remains available and Alpha rendering,
Omega water and enhancements keep their settings.

A clean implementation of the loaded map preserves native hashing, bucket segments,
growth reordering, read stability and tied retirement choices. Conditional serial
examples, eight hash words and growth boundaries exercise those rules. Startup
compatibility prehistory, native runtime priority/interleavings and broad original
acceptance are not claimed. Dedicated-server source ownership is explicit so its
unused renderer cannot suppress host admission or clear host terrain at teardown.

Three streaming failures, one map-order failure and one ready-working-set failure
were reproduced in staged baselines. Two additional dedicated-server ownership
failures were reproduced during final review. Local edit replay/empty replacement,
missing-data fallback, snapshot/memo invalidation, far-distance clamp, retained
boundary, four-source/eight-retirement limits and Alpha adapter checks add coverage.
The existing provider/journal model does not establish complete native retained
arrays, ordered state, scalar or connection-dependent restoration behavior.

Weak recovery no longer promotes an already-owned result into the 96-entry ready
LRU. In one matched 30-second Alpha-terrain/Steam-look/Omega profile, generations
fell from 768 to 685, distinct keys rose from 287 to 315 and completed meshes rose
from 146 to 164. Excess productions fell from 481 to 370. These overlapping elapsed
job measurements establish neither FPS nor a general speedup. Repeated generation
and source-creation work remain measured follow-ups.

Two matching full-load observations produced nonblank captures and exited zero
with profiling off: 129.977 seconds before the final role/snapshot refinements and
130.589 seconds for the final compiled build. The latter's binary hash was checked.
The preceding case hit its observation deadline with 621 prepared zones, 769 loaded
sources and four source jobs. These two observations do not close broader cold,
travel, memory, FPS or native visual acceptance. Evidence is private under
build/source-retention-validation and build/alpha-steam-fill-source-final.
See [terrain loading](terrain-loading.md) and its independently held contracts.


All five targets and the final required-oracle CPU/owner-data suite pass without
skips. Vulkan checks pass: 92 shader, 2 camera, 102 GUI, 17 map, 14 native-water,
shared water and Alpha scenery. The extractor SDK-independence audit passes.
No original game was started or owner saves, mods or display settings changed.

## Live generated-terrain reuse and loading measurements - 2026-11-07

Decoded terrain and resident consumers can outlive the generator's 96 ready
entries. A weak lookup now recovers their immutable results instead of generating
another copy. It retains the coordinate/settings/options/original key, pending
reader coalescing and flush retirement. Expired weak metadata is removed on cache
admission. The strong ready limit stays 96. This is an engine cache policy,
distinct from native source-map retention and scheduling.

A public-API concurrent-reuse regression reproduced one failure before the fix
and passes afterward. Release without a consumer, release after reuse/eviction,
flush, settings/option changes and original-versus-hooked isolation also pass.
The optional private CSV profiler is off by default and preserves math, inputs
and evaluation order. See [terrain loading](terrain-loading.md).

In one matched 30-second hidden/offline/no-save Alpha-terrain/Steam-look/Omega run,
generations fell from 954 for 313 distinct keys to 760 for 329 keys; completed mesh
jobs rose from 168 to 184. There were 391 live-result recoveries. Timing sums are
elapsed wall time in overlapping jobs, not process CPU or FPS. Repeated generation
remains: 431 excess productions versus 641 before. Private evidence is under
build/mesh-cache-validation and build/mesh-live-cache-profile.

With profiling disabled and the exact prior full-capture arguments/settings except
private output paths, the scene still reaches its observation deadline: 440
prepared zones and 8 pending jobs after 133.689 seconds total, versus 384/8 after
128.422 seconds before this fix. Neither run produces a capture. This does not
establish a whole-game speedup or close cold-fill acceptance. Next streaming work
separates native loaded-source retention from generation demand. Alpha terrain,
lighting, Omega water and enhancements retain their settings.

All five targets, the full required-oracle CPU suite with no skips, Vulkan groups
(92 shader, 2 camera, 102 GUI, 17 map, 14 native-water, shared water and Alpha
scenery) and extractor SDK independence pass. No original game was started or
owner saves/mods/display changed. The independent
[colour-evaluation contract](specs/steam-terrain-colour-evaluation-order-20261107.md)
was read in full. It distinguishes base-colour from admitted-corner work and
leaves broad purity assumptions unproved; no speculative colour-skipping change
was needed. Native runtime, travel, memory-pressure, final provenance and wider
parity acceptance remain open.

## Native mesh selection and batch boundary - 2026-11-07

Native preparation now selects at most eight coordinates with the independent
prefix-cost, zero-cost, invalidation and neighborhood gates. Reused bounded worker
lanes complete the whole batch before selected-order publication and subsequent
terrain retirement/selection. Source snapshots and deferred Vulkan releases stay
owned by the engine; model/object updates retain their separate frame route.

Known zero-cost results remain eligible after successful empty meshing. Required
work is tracked separately, so diagnostic capture/benchmark settling can finish
while clean empty refreshes continue. Partial captures with missing native cost
metadata keep their cached fallback behavior. Alpha retains its existing async
path and cap of six successful integrations per frame.

A controlled owned generator hook reproduced two failures before the change:
partial batch publication and premature retirement. Both now pass. Additional
checks exercise the actual eight-coordinate batch, literal prefix boundaries,
neighbor-before-cap order, signed costs and zero-cost readiness; terrain bands,
publication, static objects and fourteen native-water GPU checks also pass.
Evidence: build/native-preparation-validation. The independent
[selection contract](specs/steam-mesh-invalidation-selection-20261107.md) and
[concurrency addendum](specs/steam-mesh-worker-concurrency-20261107.md) are held.

CPU-setting changes cap our hardware-bounded lanes at four for zero or eight for
nonzero, sampled after the current batch. The original default first scheduler
limit is unproved; the existing hardware-based fallback stays an explicit engine
policy. Native scheduler priorities/runtime internals are not reproduced. Incoming
edit requests retain the engine's immutable-snapshot queue behavior; the native
unconditional final dirty-byte clear is documented separately. Exact native race,
publication latency, source-cache/eviction cadence and broad cold-load acceptance
remain open. Alpha terrain/lighting, Omega water and enhancements stay configured.

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

## Native empty-staging publication - 2026-11-07

A successful wholly empty Steam mesh now retains the previous active terrain
and water handles while publishing fresh zone bounds, cost and light inputs.
A nonempty solid-only replacement still removes previous water. Source-mesh
failure now publishes neither geometry nor metadata, and an explicit retry can
replace the retained result after recovery. Alpha keeps its existing successful
empty-replacement behavior. Optional lilies remain attached to retained water.

Public runtime regressions reproduced seven failures before the fix. They now
cover repeated empty results, water-only and solid-only replacements, independent
zero-cost metadata, damaged/repaired disposable source files, and initially empty
zones in both styles. Four additional Vulkan checks exercise actual World
uploads, visible water retained across empty rebuilds/fence reuse, nonempty
replacement and retirement. The fixture first settles then freezes the sky
palette to avoid an inconclusive saturated-blue visibility comparison.
Evidence: build/terrain-publication-validation.

The independent [publication](specs/steam-water-mesh-publication-20261107.md) and
[zone lifetime](specs/steam-zone-retirement-lifetime-20261107.md) contracts delimit
the native behavior. The engine retains immutable source snapshots and deferred
Vulkan resource releases; raw native pointers and unproved shutdown/error paths
are not reproduced. Exact worker-batch cadence, zero-cost reselection and wider
native visual/gameplay/lifetime acceptance remain open. Alpha terrain/lighting,
Omega water, shadows and the other enhancements retain their settings.

The independent [incoming matrix audit](specs/steam-water-incoming-matrix-timing-20261107.md)
confirms the ordinary pre-water segment preserves the frame VIEW/projection for
the depth sweep; color explicitly reinstalls the same pair, even with no groups.
The current ordinary renderer path agrees. No matrix implementation change was
needed; alternative callers/flags remain outside that proved boundary.

All five targets build. Full required-oracle CPU/owner-data validation passes
without skips. Vulkan groups pass: 92 shader, 2 camera, 102 GUI, 17 map, 14 native
water, shared water and Alpha scenery. Extractor SDK independence passes. These
checks remain bounded regressions rather than complete Steam acceptance.

## Local native water groups - 2026-11-07

Steam water now follows the 250-block local terrain bands from CPU meshing to
GPU upload, draw collection and retirement, including groups containing only
water. Integer subtraction before float conversion preserves local unit corners
at distant source-home coordinates. Ordinary water and wet-ground film retain
their lighting, bytes and topology; the whole-sea quad retains its direct V255
and native height1 in the group containing Z0. Zone point-table identity and the
adaptive triangle cost include the new water streams. Optional lily decoration
recovers the former sea-first/cell/layer order and the same zone cap.

Focused tests cover boundary250, omitted intermediate groups, water-only groups,
exact far-coordinate mesh bytes, negative sea anchors, lily callback/output order
and actual asynchronous World upload/draw/retirement. Three regressions fail on
the preceding aggregate-water route and pass with local bands. Evidence:
build/water-band-validation. The independent
[publication contract](specs/steam-water-mesh-publication-20261107.md) is accepted
alongside the shader contract and its mandatory pass-state correction. Complete
empty-staging publication, streaming/lifetime behavior and native visual
acceptance remain open. Alpha terrain/lighting, Omega water, shadows and the
other enhancements retain their settings.


All five targets build. The full CPU/owner-data suite passes with required
procedural oracles and no skips. Vulkan checks pass: 92 shader, 2 camera, 102 GUI,
17 map, 10 native-water, shared water and Alpha scenery. Extractor SDK independence
passes. Logs and private receipts are under build/water-band-validation. These
checks establish the bounded regressions above; native rendering and full
publication/lifetime acceptance remain open.

## Native Steam water shader checkpoint - 2026-11-07

Native packed water now retains clip/view-space inputs through its fragment
stage, correcting two large-coordinate color failures without changing the
shared vertex light solve. Three independent raw pixel references, fixed-origin
geometry/color comparisons and real point-table/slot tests pass in a new ten-case
GPU group. All five targets, full required-oracle CPU checks without skips, the
existing Vulkan groups and extractor SDK independence pass. Alpha terrain/lighting,
Omega water and enhancements remain intact. Evidence: build/native-water-validation.

Use the [shader contract](specs/steam-native-water-shader-20261107.md) together with
the [pass-state addendum](specs/steam-native-water-pass-state-addendum-20261107.md).
The latter corrects the former's White/DepthOffset mapping before any such state
change was implemented: White0/Glow0, retained DepthOffset. Local water mesh-group
submission, empty/incoming pass timing, publication/lifetimes and native visual/
hardware acceptance remain open; this is not complete water or Steam parity.

## Ordered physical wheel delivery - 2026-11-07

Physical wheel messages now share the button/F4 message queue and execute before
new pointer/controller polling, using the retained GUI cursor. Stored panel-tree
queries preserve inventory/shop/crafting priority without capture or ancestor
visibility gates; Controls/Friends remain ahead of camera/map fallback. Reusing
the existing row callback removes its later replay and the separate wheel buffer.
Six initial stateful batching/zero regressions pass after the correction. All five
targets build; full required-oracle CPU validation passes without skips, all Vulkan
groups pass (92 shader, 2 camera, 102 GUI, 17 map, water and Alpha scenery), and
extractor SDK independence passes. Evidence: build/wheel-input-validation. This is
bounded input-route work; physical-device acceptance, complete Game/UI callbacks
and the final provenance/Steam-parity audit remain open. Alpha defaults and the
requested enhancements are unchanged.

## Fresh-PC validation — 2026-11-07

A supported-game session reached the title and a loaded TEST-character world,
confirmed by the owner. A journaled extractor-only run captured seed-123 terrain
and region data; save-file hashes matched the backup after restoration. The
owner's Mods and original display settings were restored and the recovery
journal was cleared. Desktop screenshot capture remained unavailable, so this
is not a new native visual-comparison result.

The CPU suite now passes without skips, including 324 fresh completed native
procedural-item volumes. All existing Vulkan image groups also pass: 92 shader,
two camera, 102 GUI and 17 map cases, plus shared water and Alpha scenery checks.
Fresh comparisons match 95,575 generator-field values, 193,880 block sample values,
60,000 cell objects, 3,000 gear records, 7,704 appearances and two region plans. The bounded zone check also matches 127,085 stored block records
across four zones. The corpus contains 968 zone snapshots; these results do not
mean every captured zone, seed, dungeon transition or gameplay path was checked.
Local logs are under `build/loaded-world-validation` and
`build/loaded-world-oracle-checks`; isolated resource-capture evidence is in
`build/procedural-capture`. All 3,882 prior hashed references stayed unchanged
during that isolated run. Remaining acceptance gates below stay open.

This is a replacement-engine source package for review. It is not a finished-game
release or a claim of pixel/behavioural parity. Older dated development notes may
describe prototypes that have since been replaced; the current README and this
checkpoint describe the intended setup and remaining limits.

## Current handoff items

The newest checkpoints appear first. The latest validation result below qualifies
older successful runs; none of these bounded changes establishes complete parity.

- Right and middle UI events now dispatch targeted and global callbacks using
  the native event keys, capture selection and focus-cleanup order. These paths
  omit the left button's auxiliary and extra release operations. Stateful checks
  reproduced 26 failures in the prior mask-only path and pass after the fix.
  Dynamic widget bodies and live hardware acceptance remain separate gates.

- Mouse motion now runs before controller callbacks and shares the GUI's stored
  button mask. Actual window messages and synthesized controller clicks use that
  mask; left controller clicks also reach the retained capture/tab path. Camera
  dragging uses capture or a fresh root pick, and map/model dragging can continue
  into camera handling. Map pan uses current native-degree yaw. Camera and owner-GUI
  regression cases pass; global cursor preparation and hardware acceptance remain.

- Controller input now distinguishes skipped sampling from a failed XInput poll.
  Foreground loss and binding capture retain sampled controls and both stick
  pairs; a failed admitted poll clears the pairs. Stateful checks reproduced six
  pre-fix failures and pass after the correction. The independent controller
  contract leaves broader active-input and hardware acceptance explicitly open.

- Formula tooltips now borrow a panel-owned item published by resource-admitted
  drawing, including both result and ingredient icons. Hidden/missing-result draws
  retain that copy; missing ingredient icons still permit hover publication.
  Actual owner-GUI tests with synthetic items reproduced nine pre-fix failures
  and pass after the correction. Exact native frame latency remains open.

- A supported Steam session confirmed both named dungeon cases and the
  18 helper rows. Structure 44/detail 12 exposed 145 block differences across
  119 columns near two cones. The independent audit found the missing second
  simplex offset. With the correction, all 12,288 stored columns across the
  three captured zones match exactly. The session restored 60 checked owner
  files byte-for-byte. Capture data remains local. This covers the two cases
  and their captured neighborhoods.

- The headless setup can now prepare the dungeon request by itself with
  `--setup-capture --dungeon-targets <file>`. It uses the existing Save, Mods
  and ReShade recovery journal. Setup stages requests before publishing them
  and refuses to overwrite an existing request. Recovery tests cover a
  competing owner request and an interrupted publication.

- The expanded structure audit can require exact stored spans using
  `WS_COMPLETE=1` and `WS_STORED_EXACT=1` with `EngineTests wgstructures`.
  It uses the bounded CWZ reader and compares nonempty bases, lengths and
  six-byte block records directly. All 3,231,744 columns across 789 overlapping
  seed-123 snapshots passed. This does not supply labels for missing cases.

- The extractor has a separate opt-in dungeon request. It plans bounded 3×3
  cell neighborhoods, saves fresh zone blocks and independently queried dungeon
  decisions, and marks only complete batches. The offline comparator checks
  world context, decision fields and exact stored columns. The two named
  targets still require a supported-game capture with the updated DLL.

- The experimental generation recorder now pauses on a failed terrain, material,
  record, spawn, trace or day-file write. It retains the unfinished coordinate
  and counts it only after both fresh and refreshed outputs close successfully.
  The Windows writer checks cover successful, unavailable and partial writes.
  The changed DLL still needs supported-game live validation.

- `EngineTests castles` now reports which castle cells the engine predicts
  inside the owner's captured zone coordinates. `castles --targets` searches
  nearby big places for the two missing decorative cases. For seed 123 it
  identifies zone (48, 178) for structure 42/detail 11 and zone (-216, 38)
  for structure 44/detail 12. These are engine-side target predictions.
  Neither label has been confirmed by a new original-game capture.

- The strict generation comparison now matches all 2,800 tested object
  placements across 789 overlapping seed-123 snapshots. The only previous
  differences were 11 captured street-lamp turn bytes. The comparison still
  checks type, position and scale for those records and reports their raw
  turns separately. Initialized turns still require exact equality. The
  [independent audit](specs/steam-generated-cell-object-orientation.md)
  distinguishes the unwritten generation input from native copy, equality
  and render behavior. CubeVulkan retains original turns on import and
  uses its existing zero turn for newly generated lamps. Complete structure
  checks matched the sampled blocks and light lists. Wider labeled dungeon,
  region and seed coverage remains open.

- The retained inventory scrollbar now captures ordinary left presses on the
  supported Button template. Movement uses the receiver's local pointer delta
  and the cached row range. It retains a continuous thumb position and leaves
  collection refresh, sound and discrete thumb placement to their existing
  paths. The owner-GUI regression covers capture beyond the control bounds,
  a hidden root, and real versus synthetic release routing. Controller thumb
  input and hardware focus behavior still need acceptance.

- Physical Windows left-button edges and F4 now reach the retained interface
  in message order, before the next admitted cursor sample. GLFW's synthetic
  focus-loss releases still feed gameplay controls, while the retained scene
  receives actual window button-up messages. An owner-GUI test exercises
  visible/hidden/visible presses in one batch. Inventory presses retain their
  identity through overlay filtering, so the later Game path sees the same
  physical point and never delivers a second tab listener. Hardware focus and
  controller comparisons remain open.

- F4 now changes the retained principal scene root before the pointer poll,
  including gameplay with no menu open. Hiding the root blocks fresh tab picks
  without clearing child flags or the last drawn ordinal. The owner-GUI test
  checks independent child changes while hidden and restoration of the old hit.
  Each queued inventory press now shares its classified pointer position across
  tab delivery and the later Game handlers. Bag and shop actions check their
  retained cells at that same point. The native-only visibility contract also
  bounds the observed focus/cancellation messages; full live pointer/capture
  integration and hardware acceptance remain open.

- Published `52a4798` passed a fresh isolated build of all five targets. Its 959
  frozen payloads are byte-identical to the published source archive. The audit
  verified ten cached pinned dependencies, 462 compiler commands, 273 frozen
  project dependency paths, all five x64 images, and the four extractor exports.
  Five focused suites passed, including owner GUI checks without skips. This
  verifies that revision from frozen source; it does not replace live extractor
  qualification in the original game.

- Native-backed inventory tab presses now select one target from the shared
  scene root before delivering the registered listener. No tab is searched
  below a foreground target without that listener. The owner-GUI regression
  proves that an independent visible Options branch blocks the underlying tab
  and that hiding Options restores the hit. All five build targets, the full
  CPU/owner-data suite and 102 Vulkan GUI image checks pass. Physical pointer
  polling and down/up ordering, scrollbar capture and the remaining widget
  routes still need integration and live acceptance.

- Inventory tab listeners now register on one scene-bound input owner. The
  live panel adapter delivers tag two through its registered-event path; it
  still supplies the panel hit separately. Replacing tab widgets removes their
  registrations through scene deletion, and explicit HUD reload detaches input
  before replacing the scene. The owner survives Start Menu with the CPU HUD.
  The owner-GUI regressions cover stale tab ordinals and explicit reload.
  Common root admission, full pointer dispatch and scrollbar capture remain open.

- Bound draws now retain a separate tab ordinal. The native-backed bag, shop
  and crafting left-down adapters require an enabled tab hit but read the last
  ordinal published by drawing. A missed or unbound draw preserves that value;
  invalid or already selected ordinals do not change tabs. Owner-GUI tests
  distinguish the hit tab from the cached ordinal and verify that misses and
  rebinding preserve the cache. The same tests exposed and verified a fix for
  missing event-target initialization on cloned tab descendants. This remains
  within existing panel admission; global callback/capture routing is open.

- Bound inventory-shaped widgets now publish a physical hovered group/row during
  drawing. Hidden or unbound draws preserve the pair. Gameplay readers recheck
  current panel geometry and live source bounds, with separate bag, shop and
  crafting item rules. Crafting left-down can adopt the raw pair separately
  from resolving a recipe; the explicit station-tab setter clears cell hover.
  Owner-HUD tests exercise zero-quantity rows, stale hover between draws, changed
  source bounds and the empty grid tail. All five build targets, the full
  CPU/owner-data suite and 102 Vulkan GUI image checks pass; the hidden inventory
  capture shows the panel and exits normally. Cached tab publication, registered
  input routing and full native event ordering remain open.

- Start Menu now retains the CPU HUD scene and its existing branch/control
  identities across sessions. The outgoing renderer's item handles and queued
  draws are discarded so they cannot be reused by the replacement renderer.
  Owner-HUD checks keep the same roots, reject stale model draws and render again
  after registering resources on a second renderer. All five build targets,
  the full CPU/owner-data suite and 102 Vulkan GUI image checks pass. The window,
  renderer and gameplay session still restart. Live input dispatch and end-to-end Start Menu
  acceptance remain open. The independent inventory-hover audit now distinguishes
  the cell/tab state published by drawing from pointer movement and callback
  target selection. That callback integration is separate from this checkpoint.

- Inventory drawing now respects the controller's gameplay-branch state instead
  of temporarily forcing it active and hiding it after drawing. Two actual
  owner-HUD regressions first reproduced both defects: drawing changed an active
  branch to inactive, and an inactive branch still emitted equipment models.
  Construction and drawing no longer override the shared branch state. This
  closes that visibility edge, not the remaining live input/capture integration.

- Frontend and gameplay controls now occupy separate branches under one scene
  root. Title, character selection and creation disable the gameplay branch;
  Options and Controls remain independent branches. Frontend templates are
  published before pointer decisions, retained between screens and hidden when
  omitted. Detached layout probes cannot enter root picking. Screen publication
  and drawing share placement without copying template callbacks. All five build
  targets, the full CPU/owner-data suite, focused branch/owner-GUI checks and 102
  Vulkan GUI image checks pass. Hidden title and character-selection captures
  exit normally and leave the save fixture unchanged. These are visibility and
  ownership foundations: production callback/capture routing, full stale-template
  cleanup, persistent Return-to-title lifetime and live acceptance remain open.

- Crafting controls now start with four blank roots and are explicitly replaced
  after character loading: recognized classes add their two native icons before
  the four common controls; other class values use the common four. Replacement
  neither accumulates roots nor repairs selection/range state. Class setup and the
  startup crafting rebuild run before the first visible toggle; drawing no longer
  changes class icons. Controller wrapping reads the actual control count. Manual
  tab, region-arrow and recipe-selection paths now require left-down, preserving
  right-click item actions. A new original-only dispatch trace corrects the older
  tab-release label: registered event2 runs synchronously on the concrete down
  path. All five targets, focused lifecycle/owner-GUI tests, full CPU/owner suite
  and102 Vulkan GUI images pass; the owner save fixture is unchanged.
  Complete scene callback/capture/hover routing, persistent frontend lifetime
  and live hardware/visual acceptance remain open.

- Published checkpoint5ae5c91 also passed a fresh isolated five-target build from
  its exact frozen source, using ten verified cached pinned dependencies. All952
  source payloads stayed unchanged;461 compiler commands and272 project dependency
  paths were confined to the frozen source/build or dependency roots, with no
  CWSDK or working-source inputs. All five x64 images and four extractor exports
  passed, as did seven focused suites including owner-GUI checks without skips.
  This checks source/build independence for that revision; dependency sources were
  cached, not freshly downloaded, and it does not replace live-game acceptance.

- Native-backed shop collections now persist between explicit rebuild events.
  Rebuild releases old inner rows and creates exactly two groups; buyback copies
  newest-first without filtering zero/negative/duplicate records, while stock uses
  the shared owned insertion rules. The inactive group is cleared and a missing
  merchant leaves empty stock. Drawing reads retained rows and live presentation
  values without regenerating stock. Opening, tab changes and trade hooks retain
  their separate refresh/thumb stages; tab selection before rebuilding is not
  repaired afterwards. Merchant role comes from the opening event and current
  record/home inputs are resolved at rebuild. Shop range/offset/thumb now use the
  same explicit cache path as bag/crafting. All five targets, focused inventory/
  shop checks, full CPU/owner suite and102 Vulkan GUI images pass; the owner save
  fixture is unchanged. Shared native RNG, complete GUI event/
  geometry lifetime, persistent frontend state and live acceptance remain open.

- Bag/crafting row range and dynamic per-tab offsets now persist between explicit
  refreshes. Unbound/nonpositive-column refreshes preserve prior state; physical
  emptiness writes A0/row0, while a nonempty regional group with no matches uses
  its separate signed division. Only the current row clamps downward. Native-backed
  drawing no longer recomputes the range or places its thumb. Event pairs place
  the thumb from cached A with integer truncation/wrapping, including zero-height
  and negative-track cases; A0 preserves old geometry. Actual player-record region
  supplies native inventory choices/opening and crafting generation; missing actor
  storage defers those region-dependent stages. All five targets, full CPU/owner
  checks and102 Vulkan GUI images pass; the owner save fixture is unchanged.
  Full geometry/callback closure, persistent frontend state,
  shop collection/range lifetime and hardware/native visual acceptance remain open.

- Native-backed crafting now retains physical count/item groups between explicit
  rebuild events, with separately owned details and in-progress recipes. Generation
  and count filling no longer run during native-backed drawing. Rebuilds preserve
  skipped/unmatched state, keep duplicate/zero/negative rows, clear group6 without
  truncating higher groups, and copy a matching full recipe before temporaries
  expire. The later left-down tail publishes from the selected physical row, even
  without a new hover; invalid/failed publication clears details. Tab navigation
  alone does not republish. Toggle, station, equipment, trade, completed-craft and
  shown-crafting pickup paths are connected. Physical collection binding is shared
  by GUI count/navigation work. All five build targets, full CPU/owner-data suite
  and102 Vulkan GUI images pass; the owner save fixture remains unchanged.
  Full cached GUI state, action-pair/tab6 targets,
  missing-resource eligibility, native RNG coupling and live acceptance remain open.

- Native inventory region choices now persist between explicit refresh events,
  rather than rebuilding during drawing. They read the canonical carried tab,
  include every nonzero count and the player region, and preserve the old cache
  on unbound/non-inventory refreshes. Collection binding runs while panels are
  hidden; binding itself does not replay an earlier skipped refresh. Tab changes
  refresh before selection/thumb placement, with the shop's additional rebuild
  afterwards. All five targets, full CPU/owner-data suite and 102 Vulkan GUI image
  checks pass. Persistent frontend binding, complete cached row/range state,
  shop collection lifetime and live callback/hardware acceptance remain open.

- An extractor-only CMake configuration now builds the Windows DLL before
  renderer/dependency setup. A fresh build passed with no FetchContent cache
  entries or dependency directory; compiler/header audit and four required exports
  also passed. The default five-target build remains unchanged. This source/build
  qualification does not replace supported-game live acceptance.

- Crafting counts now retain the native negative-sentinel and signed-division
  results, using wrapped carried totals without an extra final zero clamp. The
  actual inventory total feeds the live formula count query. Native division-fault
  inputs retain an explicit engine fallback; no native successful return is claimed
  for those inputs. Full callback coverage and native shared RNG coupling remain open.

- Pickup notice quantities now reach gameplay as unsigned 16-bit values, preserving
  32,768 and 65,535 through packet decoding and inventory insertion. Regional and
  item-value gold counters use defined low 32-bit addition. The native notice path
  passes coin quantities directly, with item-derived gold added once. Packet-to-
  receipt tests cover zero, signed-word boundary, maximum word, full flags/position
  preservation, counter overflow and quantity-independent gold value. Broader
  receipt presentation and live multiplayer acceptance remain open.

- Experimental generation now publishes an exclusive context-tagged request
  snapshot before consuming input. Observed World/seed/day changes or native
  generation/refresh failures pause the batch and retain pending work. Actual
  Windows file tests check exclusive input ownership, exact-file consumption,
  complete publication and preservation of existing
  final/temporary files. This is tool policy, not native lifetime pinning or
  atomic capture. Live qualification and broader acceptance remain open.

- Inventory row input now preserves separate callback visits and refreshes the
  local canonical inventory before each row decision, including a boundary attempt.
  Refresh clamping is distinct from requested movement, so it does not alone request
  the row sound. Region arrows request sound before changing region; next-region
  adds its explicit canonical sort before refresh, even with no matching choice.
  Asset-free boundary, opposing-input and callback-order checks exercise the shared
  production helpers. Event handlers also place the actual scrollbar with their
  refreshed range. Full cached range/thumb state and live GUI callback binding
  remain unfinished; this does not claim complete controller or UI parity.

- The shipping extractor now builds without CWSDK. Independent owned views cover
  its loader, catalogs, World/Zone data, creatures, appearance, inventory, effects,
  simple event queues, camera and generated text. The actual compiler/header
  dependency audit, DLL exports, five build targets and full CPU/owner-data suite
  pass. Native Zone refresh can retain a raw Zone pointer for a later World
  consumer, and its destructor does not detach that reference. Temporary oracle
  Zones remain retained until process exit while initial-generation escape
  lifetimes remain unproved. Experimental gen.request admission now limits the
  process to 32 allocation attempts, including failures, and preserves rejected
  requests instead of dropping coordinates; this is an attempt limit, not a
  byte-memory bound. Zone seed arithmetic explicitly wraps at 32 bits. These
  changes still need supported-game startup/extraction/recovery qualification;
  they do not establish universal snapshot safety, legal clearance or 1:1 parity.
  The [workboard](extractor-independence.md) tracks remaining audits and acceptance.

- The independent effective-block audit verifies original CRT initialization of
  the shared solid/air/water defaults, their ordinary lookup copies and direct
  writer scope. The World vehicle producer now uses the initialized missing-zone
  flags, superseding the earlier missing-zone deferral. A missing-zone actual-World
  regression reproduces that failure before the binding fix. The same audit
  establishes that an empty column still returns solid below its retained base,
  before sea handling; twelve exact accessor vectors reproduce four failures
  before correction. Two older mutation assertions rested on the disproven
  empty-means-water assumption; their fixture now separately checks below-base
  solid and at-base implicit-water coverage. All five targets, full CPU/owner and
  automated Vulkan image checks pass; the owner save fixture is unchanged. This
  does not certify alternate builds, bypassed startup or arbitrary external writes.

- Climbing upkeep now uses the same wrapped signed32 special-item quantity
  total as the verified availability query, replacing a widened total. A regression
  through actual upkeep reproduces the overflow-admission failure and passes after
  the fix, alongside positive/negative count cases. Ordinary unequip now requests
  native sound0x5b after its inventory refresh. All five targets and full CPU/owner
  checks pass; the save fixture is unchanged. Cached-widget/crafting completion and
  audible original-game acceptance remain separate gaps.

- Vehicle prompts now read a World-step cache generated from availability,
  nearby objects and environment state, replacing the first-carried-item stand-in.
  The implementation follows the independent exact candidate order, physical
  fixed-coordinate probes, signed32 regional quantity query and timing gates.
  Availability cancellations moved out of the early-control shortcut to their
  World cut point, including riding; pet-key ownership uses the same value query.
  The synthetic selected record has zero semantic metadata/region, while direct
  inventory use retains the original region. Shared fieldwise item assignment
  preserves padding, and fresh-character reset initializes both derived caches.
  Regressions reproduce the missing actual-World production and held-padding
  overwrite before their fixes. Focused World/Use/collection checks, all five
  targets and full CPU/owner checks pass; the save fixture is unchanged. Unknown
  missing-zone fallback flags defer new boat/glider choices as replacement policy.
  Wider original-game runtime and alternate-writer acceptance remain open.

- The legacy extractor build command now delegates to the maintained CMake target
  and keeps output under build, using the same configuration as the root build.
  Its functional check passes; obsolete single-file outputs were removed locally.

- The two previously unfinished use-prompt completions, abilities 0x96/0x97,
  now add/update and remove independently owned creature copies. A separate
  application-lifetime owner survives menu/world sessions without inventing a
  persisted format or transition clear. Completion uses the native effect-time
  helper rather than whole duration and preserves reset, sound and selection
  order. Fieldwise assignment retains unassigned padding, inventory and pose
  state, including when updating an existing ID. Tests reproduce the missing
  actual-prompt call and zero-ID sentinel filtering before their fixes; signed-ID
  order/wrapping, missing targets, source immutability, suppression/Climb ordering
  and continuation to Revive also pass. All five targets and the full CPU/owner
  suite pass. This bounded backend does not establish other collection consumers,
  save/network behavior or original-game runtime acceptance.

- Proper-name generation now shares the verified unsigned R hash and explicit
  32-bit seed/selector arithmetic, removing a signed-overflow expression. Shared
  fragment-table aliases retain their original hash inputs. Fourteen independently
  derived index cases, extreme-selector and data-reader cases pass with all five
  targets and the full CPU/owner suite. Fragment capture now reads 380 verified
  literal-data RVA/length entries instead of scanning instruction bytes, retains
  the legacy file order, and publishes through a temporary file. The source
  manifest matches every finalized prose entry; existing owner output agrees on
  all 380 lengths and 89 repeated-address aliases. No vocabulary is bundled. These
  are contract/legacy-output checks, not new native execution or full name-output
  parity. The new extractor still needs supported-game live acceptance.

- Full item naming now starts with the independently specified semantic record
  copy. The four special gear stand-ins reach their actual gear dictionary branch,
  including when nested in a formula. The shared conversion retains the wrapped
  type, material, all 32 attribute records/count and unlisted bytes; naming leaves
  the caller's live item unchanged. A synthetic regression first reproduced four
  wrong names plus one recursive-formula failure, then passed with the fix. Build
  and CPU/owner-data checks pass. The completed native-only material/quality audit
  also corrected unlisted material bytes to use the generic Material key. Three
  additional fallback failures were reproduced before that fix. Unsigned spirit
  material, quality-context and unused attribute-tally regressions pass. The tally
  adds no visible suffix in the traced helper. Proper-name family/fragment data,
  malformed-input and native visual acceptance remain separate boundaries.

- Stored terrain export now uses an independently specified Zone identity/column
  reader and the project's CWZ1 encoder, replacing SDK position/column access.
  Zone enumeration and capture leases are also independent in this export path.
  Guarded ranges preserve all six stored bytes, distinguish capacity from live
  records and reject observed base/header/identity changes. No effective water
  substitution or climate default is manufactured. Biome remains a separate
  World query, in its existing order before column copying. Resource ceilings
  are capture policy. Malformed-memory, exact-byte, long-run and actual engine
  file-consumer tests pass, alongside all five targets and the full CPU/owner
  suite. Its new native integration and the callback replacement below still
  need live acceptance; the attempted session was stopped before launch and all
  owner files restored exactly. Rechecks and mesh admission do not establish an
  atomic whole-Zone snapshot. Other SDK views and final-release gates remain open.

- The extractor factory now returns an original explicit 30-slot callback record,
  replacing SDK GenericMod inheritance and its export macro. Its layout, opaque
  register shapes and neutral results come only from the independent loader ABI
  and lifetime notes. Capture policy enables tick at priority 0 and disables unused
  events; this is not a recovered SDK default. Raw-host fixtures cover every slot,
  priority bytes, initialization/context delivery and preserved mutable state.
  All five targets, DLL export-name checks and the full CPU/owner suite pass.
  This new callback integration needs its own supported-game live check; earlier
  title results qualify the prior buffer/list/lock build, not this change. Other
  SDK game views and release rights/revision/replacement gates remain open.

- Loaded-zone enumeration in terrain-buffer capture now uses an independently
  specified sentinel-linked byte view. It preserves current forward order, counts
  null entries without treating them as Zones, ignores keys/buckets, and rejects
  inconsistent or changing sequences. This removes the SDK container iterator from
  that capture path; other extractor fields still depend on SDK views. The shared
  ordered World-lock lease has real Win32 contention and exception-unwinding tests.
  All five targets, focused capture checks and the full CPU/owner-data suite pass.
  A subsequent extractor-only supported-game title session identified 3,230 sprite
  buffers and 1,172 terrain buffers, decoded six distinct 1920x1080 presented frames,
  and closed on request. Owner Save/Mods/ReShade were restored and hash-checked.
  This covers the integrated terrain/list/lock path; it is not a full SDK
  replacement, ownership extension, modified-session or universal lifecycle proof.

- Terrain buffer identification now uses an independently specified active Zone
  linked-list view and the shared ordinary-backend wrapper reader. The caller
  retains the documented loaded-zone/publication lock pair throughout traversal;
  header-only locking is not substituted. Sentinel/link/count/overflow/changed-
  publication fixtures, all five build targets and the full CPU/owner-data suite
  pass. The later loaded-zone checkpoint above supplies its supported-game title
  smoke and replaces this path's World-container iterator. Other SDK fields remain
  an explicit dependency. No original code, header or complete native class was
  copied.

- An additional no-save Alpha-look flight traveled 4,096 blocks and landed, then
  completed its 145-simulation-second observation and exited normally. Wall time
  was 163.79 seconds; sampled working/private peaks were 1406.7/1698.5 MiB. The final
  scene was inspected and the test save retained its hash. This adds one longer
  default-seed streaming case; it does not close broad biome, forest-edge, native
  parity or sustained-soak acceptance. The original Steam game stayed closed.

- Sprite buffer identification now uses the independently traced ordinary D3D11
  wrapper fields, guarded copies and concrete-type/link rechecks. It distinguishes
  the native wrapper from the COM buffer and invokes no native binding method.
  Null, unreadable, unknown-type, remeshing and overflow fixtures pass. Live
  extractor-only capture identified 3,230 sprite buffers and exited normally.
  The native specification also corrects earlier abstract readiness terminology:
  these concrete methods bind IA state and return success; they are not fence polls.
  Other SDK views, ownership/readback and full replacement remain open.

- Captures now separate modified zone exports and generator diagnostics from
  extractor-only paths, including legacy `.cwmod` inventory and scan failures.
  Session labels qualify new writes, not historical mixed files. Four region-header
  failures after a modded refresh were resolved by independently recapturing those
  same coordinates in an isolated loaded/lit vanilla session. All106,380 compared
  blocks and all four region types match. A terrain-only reference override keeps
  the full CPU/owner suite strict and its other datasets unchanged; that suite
  passes. Generator formulas and expected values were not changed. Raw unlit
  requested zones remain separate from lit samples. Save, 54 original mod files
  and ReShade were restored and independently hash-verified; prior requested-zone
  outputs were restored too. No owner data enters the upload. Modified-path routing
  has build/inspection coverage; a later modified-session smoke remains useful.

- Two additional no-save Alpha-look flights cross positive and negative parent
  boundaries, land and exit normally. The default seed and seed 123 checks take
  about 53 seconds wall each and retain the test save bytes. Inspected captures
  add bounded travel evidence, not full-world, forest-edge or sustained-soak parity.

- Primary sprite-table export now uses a small SDK-independent, guarded byte
  reader based on the independently traced native catalog contract. It preserves
  catalog-ID order, null/unready slots and raw dimensions, rejects corrupt or
  excessive pointer spans, and discards a changed range/catalog before publication.
  The 65,536-entry ceiling is exporter policy, not a native count. All five targets
  build; malformed-memory fixtures and the full CPU/owner-data suite pass. A fresh
  supported-game startup exported all 3,652 rows and sprite groups byte-identically
  to the prior extractor, then closed normally with exit code 0. This replaces one
  metadata read, not the remaining SDK views, a complete adapter or its release gate.

- Loaded-world validation of the request-trigger build also passes on the existing
  modded owner installation: six decoded 1904x1041 character/HUD frames and 1,101
  refreshed zone records with both auxiliary light lists. All 20 model and 7 static
  lights across 18 nonempty zones match the engine reader and neighboring-zone
  light solve, with zero mismatches or incomplete neighborhoods. The game closed
  normally with exit code 0. The existing scene-reference request performed the
  refresh; the short automated F5 pulse produced no snapshot and is not counted.
  Prior generator comparison files were backed up, the modded results kept private,
  and every prior file restored byte-for-byte. This is loaded-world capture/reader
  validation, not a vanilla generation or lighting-parity claim. Character animation
  recording, broader gameplay and clean-machine acceptance remain separate.

- The existing interface-frame capture action now has a fixed-path, one-shot
  `Extract/ui.request` alternative to F10. It polls at most four times per second,
  waits for an active capture batch, and queues six frames without requiring a
  loaded character. Live title-menu validation on the existing modded owner
  installation saved all six 1920x1080 frames; two overlapping requests correctly
  waited and saved twelve more. All eighteen BMPs decoded, had nonuniform content
  and distinct hashes; the first was visually checked as the actual title menu.
  The callback guard and 332 owner-resource comparisons pass, and the game closed
  normally with exit code 0. These images remain local, with no original asset in
  the source package. This verifies capture plumbing, not unmodified visual parity
  or loaded-world/recording/clean-machine acceptance. No SDK layout or new native
  implementation was consulted for this trigger.

- Live validation of the guarded extractor now passes on the existing owner
  installation. The verified rebuilt DLL was installed while Cube World was
  closed, with the prior DLL retained in a checked backup. Startup refreshed the
  sprite exports; a title-menu request captured two existing full-key procedural
  resources with zero lookup/write failures and byte-identical output. The owner
  resource check then compared 332 volumes with zero mismatches. Cube World closed
  through its normal window command and returned exit code 0. No character was
  loaded for this check and no other mod was changed. Direct Windows window
  capture timed out; a fresh Steam screenshot verified the title menu instead.
  The F10 automation attempt produced no capture and is not counted as passing.
  This closes the observed startup/resource/normal-close smoke check, not loaded-
  world recording, every quit branch, clean-machine recovery or SDK release rights.

- The extractor now checks its callback argument against the supported image's
  published native controller and reads the quit byte through a guarded copy
  before any typed controller/World access. The independent native trace proves
  normal context identity but leaves post-PostQuitMessage RAX unproved; those
  calls are rejected even if the register happens to retain the controller.
  Pure SDK-independent guard tests cover poisoned/null/unreadable pointers,
  address overflow, quit handling and title admission without requiring a World.
  All five targets build and the full CPU/owner-data suite passes; final focused
  checks also pass after formatting. The rebuilt DLL was not installed into the
  running original game, so live startup/quit and capture acceptance remain open.
  The DLL still has its existing SDK dependency; no full replacement, permission
  grant or loader/object-lifetime guarantee is inferred from this guard.

- Alpha base-height roughness gating now preserves the independently verified
  float32 multiplication-chain order. Synthetic scalar inputs expose differences
  from the generic easing expression; the shared easing helper is unchanged.
  Two independently derived raw-H words and their explicit partial-world state
  also pass. These are not generated-surface or fully prepared-region oracles.
  Alpha generation identity is now v2: existing v1 profile folders are retained,
  while v2 saves/caches and peer agreement use the separate corrected identity.
  The actual landscape-store one-byte-budget test confirms oversized results keep
  valid caller leases and identical geometry without being retained in ready cache.
  All five targets build, the Alpha suite and full CPU/owner-data suite pass, and
  every image suite passes. A fresh full-distance v2 Alpha-look capture also passes
  in25.836 seconds wall; the corresponding v2 Steam-look capture also exits0 in
  131.453 seconds wall. Both retain the existing internal observation logic and
  external180-second guard, not a promised120-second whole-process runtime. Their
  sampled working/private peaks are1206/1620 and1636/2534 MiB. Broader generation,
  timing variability and runtime acceptance remain open.

- Full-distance Alpha-look captures now also pass on this owner's installation:
  the default-seed forest view completes in26.554 seconds wall, and seed 123 at a
  second location completes in40.674 seconds. Both use fresh processes, no-save
  mode and the existing capture deadline. The corresponding sampled whole-process
  working/private peaks are approximately1067/1617 and986/1425 MiB. Both new images
  were inspected locally and remain excluded from source/runtime packages. These
  are bounded streaming/rendering checks with imported owner settings, not native
  pixel parity, travel testing or a fresh-machine acceptance result.

- Alpha cold streaming now retains up to 64 immutable landscape parents with a
  384 MiB budget for their estimated allocated ready-stage payloads. A private
  count-only65-second run previously generated563 parents for49 coordinates;
  the64-entry diagnostic completed56 generations for56 coordinates with no
  repeats. The production bounded-cache full default Steam-distance capture now completes: exit0,
  113.808 seconds wall, preserving setting50/radius15 and the existing deadline.
  Its sampled whole-process working/private peaks were approximately1612/2542 MiB;
  caller leases, in-flight work, allocator overhead and other engine data are not
  covered by the ready-cache budget. Weighted LRU/exact-budget/oversized/retry
  checks, the full CPU/owner-data suite and all image suites pass. No generation
  math, asset data, view distance or source-role lifetime rules were changed.
  This closes that observed capture failure, not wider cold/travel/low-memory or
  multi-GPU acceptance. Private diagnostic logging was removed before packaging.

- Two additional independent Alpha references now cover a natural positive-fire
  cell and an actually mixed fire/nonfire boundary: eight literal float words,
  with exact cell and absent-location prerequisites. The Alpha suite passes;
  these conditional arithmetic fixtures are separate from runtime/oasis acceptance.

- Independent Alpha climate references now check twelve literal float words across
  land, a mixed land/sea boundary and sea under the native seed 123 nine-cell state.
  Removing any one required cell also checks the specified defaults, including a
  cell whose complete-window weight was zero. The full CPU/owner-data suite passes;
  no generation formula was changed. These are conditional arithmetic references,
  with the separate region/location object absent and the zero-fire branch pinned;
  they do not establish positive-fire, oasis or full native-world runtime parity.

- Host source demand now follows the independently specified per-player union:
  a 270-block box clipped to the radius-5 disk, with phase-dependent 77/78/79
  single-player candidates. Ability 0x9c selects the raw block destination;
  physical player positions remain the resident-admission input. Logical host
  unloads and instance-death release follow this wanted set even when another
  reader retains departure terrain. Mirror-client behavior is unchanged. All five
  targets build and the full CPU/owner-data suite passes, including native phase,
  negative/narrowed coordinates, overlapping-player union and cached-terrain travel
  regressions. Background cache/job timing and broader packet-request producers
  remain qualified; no exact concurrent native cache lifetime is asserted.

- Eagle travel now binds generated-world source/mesh preparation to the existing
  actor's signed block destination only during ability0x9c. Physical retention,
  title flight0x9d and arrival gliding0x78 remain separate; destination fields are
  not implicitly cleared or rejected by a new sentinel/age rule. Current terrain
  remains available while the destination view is prepared, under the replacement
  cache policy. The adaptive camera scan also uses the verified narrowed-block
  producer, including wrapped negative reconstruction. Four independent native
  displacement word pairs are now regression references; the displacement routine
  itself is unchanged. All five targets build. Full CPU/owner-data and all image
  suites pass for the anchor integration; the final camera change additionally
  passes focused range and both camera-style image checks. Live travel/network
  cache-lifetime acceptance and the full-distance performance gate remain open.

- Generated Steam-look worlds now use the independently specified client source
  window, full neighboring-zone admission, signed distance-key ordering and a
  separate mesh-preparation budget. Installed and pending costs contribute, and
  deferred edits keep their dirty state. Partial captured-world replay preserves
  its existing missing-neighbor behavior and passes object/reward regression checks.
  Alpha immutable cell/region reads now use bounded thread-local caches with
  distinct world lifetimes; misses and late publication retain synchronization.
  No generation arithmetic, geometry bytes or owner assets were replaced.
  All five targets build and the full CPU/owner-data suite passes, followed by
  92 Vulkan,2 camera,102 GUI and17 map image checks plus shared water and Alpha
  scenery checks. The full-distance capture still exceeds its120-second observation
  limit: the latest run reached502 prepared zones/15 pending at128.851 seconds wall,
  versus280/15 at140.348 before the immutable-read change. This is diagnostic progress,
  not an FPS result or completed cold-streaming acceptance. Lazy backend cache roles,
  ability-specific source anchors and broader native-runtime comparisons remain open.

- Independently derived Alpha Rk references now check seed 123's original offset
  draw positions, its lattice-noise word and six raw RGB float-word results with
  synthetic cached climate. The existing generation math matches all six exactly;
  no replacement color formula was introduced. The Alpha terrain suite passes.
  These are bounded arithmetic references, not captured world/biome samples.
  The fresh-process full Steam-look radius15 rerun still fails its settlement
  deadline (135.017 seconds wall time); that performance gate remains open.

- Alpha first-run readiness now requires the owner model table and validates the
  two palm and seventeen plant models used by startup. The simulated fresh-install,
  incomplete-table and damaged-model cases pass, as do existing extraction/recovery
  tests. All five targets build; the read-only check of the owner's installation
  passes with Steam ownership verified. Steam mode retains its previous asset
  requirements. This is not fresh-machine or live interrupted-extraction acceptance.
  The independent loader ABI census is documented without SDK implementation;
  no SDK-free adapter has been implemented and the permission gate remains open.

- The published a00d377 source snapshot builds all five targets in an independent
  output directory using only its project files, cached pinned dependency sources
  and the documented separately supplied SDK headers for the extractor. Its full
  CPU/owner-data suite passes. The extractor is correctly optional without that
  SDK path. This establishes source completeness on this development machine,
  not a fresh-machine setup or clearance of the SDK's unresolved permission scope.
  The bounded font registry audit still does not identify a named-byte producer;
  no face bytes, registration behavior or glyph coverage were invented.

- Alpha's light solver stops at a proven unchanged working state, retaining the
  exact synchronous attenuation and maximum16-sweep reach. New golden tunnel and
  isolated-air cases guard propagation limits and the neighbour floor. All five
  targets and the full CPU/owner/Vulkan suite pass. A three-case private benchmark
  retained full-grid hashes in every run; enclosed-case median time improved7.38x,
  with no measured glow-wave slowdown. This is a CPU-kernel result, not whole-game
  FPS or a closure of wider cold-streaming acceptance.

- Alpha clouds now use the independently verified noise hash additive, fixing
  an older spec typo, with an exact native-derived float-word regression vector.
  Their altitude preserves M-before-H query order, separate float stages, two
  scalar fixed64 conversions, post-conversion base clamp and full64 addition.
  Tests catch a one-fixed-unit rounding difference and accidental32-bit narrowing.
  Full-header owner-model centering was already correct. Steam cloud logic and
  both terrain profiles remain separate. Five targets and the full CPU/owner/GPU
  suite pass; these are contract checks, not a new native runtime capture.

- Alpha environment lighting now follows the independently traced mesh-time
  origin sample: exact fixed-coordinate division, retained scalar, dark solid
  origins and a complete OWN_LIGHT material override. It preserves cloud handling,
  underwater RGB and separate distance fading. Steam rendering and the creature
  standing-light policy retain their existing paths. The hybrid's lighting-volume
  and64-block cache policy still differ from a complete original Alpha cache.

  Terrain faces reuse exact corner light through two bounded Z planes. CPU checks
  cover cache hits, eviction, reversed/negative Z and XY fallback; GPU checks cover
  own-light at night/zero cached light and retained far fading. Five targets build
  without warnings; the full CPU/owner/Vulkan suite and final CPU run pass. A live
  no-save foliage capture exits0 and was visually reviewed. The private64-tile CPU
  sampler benchmark was3.66–3.70x faster across three equal-checksum runs; this is
  not a gameplay FPS or full cold-load result. Save/extractor hashes are unchanged.
  Broader native cache lifecycle, runtime comparisons and release gates stay open.

- Alpha live edits now sample source rock for newly stored downward gaps, retaining
  original source Z through translated columns. Missing input fails atomically;
  the target block and Steam setter path retain their existing byte contracts.
  Implicit buried faces now follow the independent first-block/rock blend, with
  separate float contributions preserved through corner shading and final packing.
  Air/water/empty references use pure rock. This does not change collision storage
  or invent implicit block lighting.

  All five targets build without warnings. Focused gap/translation/branch/precision
  checks and the full CPU/owner/Vulkan suite pass (92 shader,2 camera,102 GUI,17 map,
  shared-water and Alpha-scenery checks). A private no-save live cliff capture exits0
  and was visually reviewed; it is not a native pixel-parity oracle. Save/extractor
  hashes remain unchanged. Wider runtime, cold-streaming and release acceptance
  gates remain open.

- Alpha natural ground cover now continues the retained landscape stream through
  an immutable vegetation stage. It replaces tagged Steam plants outside protected
  construction tiles and resolves all17 natural models through the owner's actual
  environment registry. No model or registry output is bundled. Steam town models
  preserve their exact order, position, scale, tint and flags.

  The bridge uses full-header voxel scale/pivot, half-block XY centering, exact
  lily fixed-Z offset and branch-specific flags from the independent plant-consumer
  contract. Its active-list phase follows the reached one-based convention; original
  Alpha cache population and remaining wind-sign limits are still unclosed. Alpha
  ground-item/static decisions retain stream order but their records are not used;
  Steam gameplay items and town lights remain in use. This is cosmetic and leaves
  landscape geometry/profile identity unchanged.

  All five targets, focused integration checks and the full CPU/owner/Vulkan suite
  pass. A private live capture selected from generated plant records shows actual
  owner-model foliage on Alpha terrain; startup/nature captures exit0. The new owned
  GPL import audit confirms unchanged plant arithmetic/draw order with only output
  and storage adaptation. Save/extractor hashes remain unchanged. Broader native
  runtime/biome comparisons, wider live-edit coverage, cold streaming and release
  acceptance remain open.

- Concurrent canonical zone requests now share one generation result or failure.
  Cache flushes retire old generations without changing terrain identity. The
  decoded-world LRU keeps up to96 unpinned zones in addition to explicitly kept
  player neighborhoods; a121-zone keep can no longer evict every farther source
  immediately. In-flight snapshots can retain additional data until their readers
  finish. These are engine cache policies; generation arithmetic is unchanged.

  Concurrent-pointer, geometry/identity,121-zone retention and bounded eviction
  regressions pass. All five targets and the full CPU/owner/Vulkan suite pass
  (92 shader,2 camera,102 GUI,17 map, shared water/scenery). Full Steam-option
  streaming still exceeded the capture deadline; the cache corrections do not
  close the broader performance gate. A separate native-only plant-consumer
  contract now resolves the next ground-cover placement/scale/flag questions;
  its bridge is not implemented in this checkpoint.

- Alpha terrain is now the playable default (`World.Source=Alpha`); Steam
  generation remains separate as `Game`, with captured `Extracted` and the older
  approximate `Generated` modes still explicit choices. The Alpha landscape runs
  relief, ordered boulders and voxel trees while retaining conservative Steam
  town/dungeon tiles. It reanchors residents/scenery/refresh lists and uses the
  canonical collision, water and gameplay-light readers.

  Terrain-profile databases isolate worlds and copy characters once without old
  geographic state. Compatible-engine multiplayer checks generation identity
  before exchanging Alpha world data; local host startup is not gated by a remote
  handshake. Direct Alpha climate drives the palette/HUD, absolute Alpha cloud
  coordinates are kept separate from renderer rebasing, and the Steam-style map
  receives a close-voxel/distant-height overview. Full-RGB metadata prevents Alpha
  terrain from being brightened twice while preserving Steam construction colors.

  All five targets build warning-free. Full CPU/owner checks and92 shader,2 camera,
  102 GUI,17 map, shared-water and Alpha-scenery Vulkan checks pass. No-save startup
  captures pass at128 and400 blocks in Alpha lighting. Separate Steam generation
  starts correctly, and Alpha terrain renders in a bounded Steam-lighting capture.
  Full Steam-option streaming exceeded the capture deadline; cold fill remains an
  open performance issue. Steam's existing save-resume identity is preserved, and
  original-save protection also applies before that directory exists. Exact
  whole-tile and map construction preservation, source/cache separation, spawn and
  collision agreement, profile isolation and handshake ordering checks pass.
  The private save fixture and extractor hashes are unchanged. No game content is
  included. Native full-Alpha random-stream parity, Alpha small ground cover,
  broader transitions/seeds, live-edit material behavior, two-account multiplayer
  and manual release gates remain open; this is not a final release claim.
  See [Alpha generation](alpha-generation.md) for the hybrid policies and limits.

- Ordinary affine scene-graph clipping now renders children into a retained
  fullscreen RGBA8 pool and composes them through the parent's actual shape.
  Ordered vertex/callback cuts preserve empty geometry and own-hook placement;
  a clip node's own callback follows child composition even with DRAW_FIRST.
  Nested targets have null depth for WORLD callbacks and restore parent depth.
  All three mask modes preserve their distinct alpha/RGB rules, including
  conditional layer-RGB unpremultiplication and the separate position/UV bias.
  Ordinary owner button shine is enabled through these masks.

  Pool images are reused on the ordered graphics queue, with explicit barriers,
  guarded allocation, atlas descriptor refresh and safe window-target teardown.
  Invalid command streams leave the resolved scene intact. Existing nearest
  object-atlas sampling and an explicit linear/clamp mask baseline remain engine
  choices; the native inherited sampler state is not an all-frame oracle.
  Widget/node caches, blur, projective/flattened-W GUI and static legacy HUD mask
  approximation remain separate gaps.

  Eleven new Vulkan cases cover mask modes, nonrectangular geometry, unlayered
  control, nested composition, reused full clears, null-depth/restoration, atlas
  replacement and uncaptured frames in flight. Owner-HUD callback/cut checks pass.
  All five targets build warning-free; full CPU/owner and 89 shader,2 camera,
  42 GUI,17 map/shared-water/Alpha-scenery checks pass. Five no-save captures exit0
  and are reviewed, with the save database unchanged. These bounded checks do not
  certify complete native UI or rendering parity.

- Plasma overlay vertices now retain float RGBA through upload, using the native
  32-byte position/UV/color layout while preserving existing API color units.
  Native text/shape emission no longer truncates these colors to bytes. Ordinary
  overlay blending now retains destination alpha with ONE/INV_SRC_ALPHA; debug
  readback can preserve actual framebuffer alpha, while normal captures stay opaque.
  Display fill affects its own shape rather than descendants, and glyphs use their
  text/outline colors independently of their own display fill. The legacy mask
  approximation remains separate. Recording-only panel meshes still explicitly
  convert to their byte-color compatibility format.

  Actual owner-HUD emission at GUI scales1/1.5 catches both old tint errors and
  verifies visible glyphs. Vulkan cases cover sub-byte color, a rounded control
  and source-over alpha. Previous tint/blend behavior fails those regressions;
  the correction passes. All five targets build warning-free; full CPU/owner and
  89 shader,2 camera,31 GUI,17 map/shared-water/Alpha-scenery checks pass. Five
  no-save captures exit0 and are reviewed, with the save database unchanged.
  The independently specified ordinary clip pool is the next implementation;
  real layer composition, cache/blur policy and broader UI parity remain open.

- Shared body/burst particle draws now build Steam WORLD matrices using the
  current installed scene's fixed negative-look-at offset before float conversion.
  Collection follows camera preparation, avoiding a preceding-frame origin.
  A small ParticleScene adapter keeps the source-home transform/Z shift for Alpha
  separate from native unshifted Z and the fixed producer sphere. Alpha avoids
  computing an unused native matrix. Simulation, producer order, material/style
  choices and the actual effect draw-pass order retain their existing contracts.

  CPU cases check fixed fractions, unequal header dimensions, source-home Z and
  the Alpha path. An actual Vulkan coverage case preserves a quarter block at
  block2^24 and matches the near scene; its old home-rounding negative control
  loses that offset. All five targets build warning-free. Full CPU/owner and 89
  shader,2 camera,28 GUI,17 map/shared-water/Alpha-scenery checks pass. Five no-save
  captures exit0 and are reviewed, with the save database unchanged. This closes
  the specified particle-coordinate binding, not all producer/CRT/visual parity.

- Ordinary WORLD now composes its independent float matrices on the GPU before
  evaluating local vertices. Terrain/model, native water depth/color and direct
  GUI paths share that vertex stage, as established by the independent binding
  audit. View position and view normal use the composed WV; point lighting keeps
  its separate WORLD position/normal. DepthOffset changes the composite clip-Z
  coefficient before the local dot. The optional engine leaf-deformation path
  remains separate, and the native-menu early return is preserved.

  Three synthetic actual-Vulkan regressions fail with the previous shader and
  pass after: WORLD translation cancellation, raw GUI translation cancellation
  and GUI depth-coefficient placement. Visible baseline/depth controls prevent
  empty geometry from satisfying the comparisons. All five targets build without
  warnings. Full CPU/owner and88 shader,2 camera,28 GUI,17 map/shared-water/
  Alpha-scenery checks pass; five no-save captures exit0 and are reviewed, with
  the save database unchanged. These large-coordinate sensitivity fixtures are
  not captured native rounding or complete visual-parity oracles. Cross-family
  input/state lifetimes and special shader paths retain their separate scope.

- Map models now retain independent WORLD and VIEW inputs. The frame supplies
  the map view once; the vertex stage composes WV/WVP on the GPU and keeps the
  WORLD lighting normal separate from its view-normal varying. DepthOffset is
  applied to the composite clip coefficient. Exact fixed-axis literals and the
  independently derived default light-vector words replace rounded substitutes.
  Typed Glow, Shininess, material alpha, White and separate Opacity reach their
  stated stages. GPU dot/MAD/reciprocal-square-root rounding remains a bounded
  comparison limit, rather than a CPU or captured-frame numeric oracle.

  Five new Vulkan cases cover view-independent lighting, actual WORLD-basis
  changes, material alpha versus opacity, Glow and view-normal Shininess. All
  five targets build warning-free; full CPU/owner and87 shader,2 camera,24 GUI,
  17 map/shared-water/Alpha-scenery checks pass. Five no-save captures exit0,
  all reviewed, with the save database unchanged. The unused CPU fogAmount and
  pixelColour approximations and their obsolete checks were removed after a
  caller audit; coin-preview color helpers remain in use. Ordinary scene/water/
  direct-GUI WORLD vertex composition is the next bounded shared-shader check.

- Map opaque resources now group by stable engine-owned unsigned identity keys
  and retain descriptor FIFO. Faded vectors keep append order. Opaque buffer
  readiness precedes descriptor submission; faded submissions precede their draw
  readiness check. Unavailable buffers are never bound or drawn. Valid zero-count
  headers are not classified as unready merely from their counts. The map preset
  and fog setup still run for an admitted empty/undrawable context.
  Vulkan cases cover resource-group FIFO and unavailable opaque/faded resources.
  All five targets build warning-free; full CPU/owner and87 shader,2 camera,24 GUI,
  12 map/shared-water/Alpha-scenery checks pass. Five no-save runs exit0, all
  captures reviewed, with the save database unchanged. Native runtime addresses
  and aliases remain separate inputs, without a catalog-to-native-heap claim;
  marker-specific inputs and wider native visual comparisons remain open.

  Local cleanup removed ten obsolete, hash-verified smoke executable copies,
  reclaiming146 MiB. Their screenshots, logs and verified versioned packages
  remain available; the current build and owner files were preserved.

- Steam map region pixels now share the full WORLD pixel implementation with
  scene/GUI rendering. They use the base-two fog exponent and both Fog/SkySun
  blends, retaining the actual raw frame palette, sun position and gradient when
  map matrices change. Frame/color/pixel helpers are factored into shared shader
  includes with explicit build dependencies; the map binds the frame descriptor.
  The Alpha appearance option retains its compatibility coloring, and the CPU
  preview helper is labelled as an approximation rather than a native pixel oracle.
  Marker-specific producers and broader map comparisons remain separate work.

  Three synthetic map cases failed before and pass after: full-distance SkySun,
  a partial-fog numeric reference and retained sun/gradient with a different map
  lens. All five targets build warning-free; full CPU/owner and87 shader,2 camera,
  24 GUI,9 map/shared-water/Alpha-scenery checks pass. Five no-save runs exit0,
  all captures reviewed, with the save database unchanged. No native shader bytes
  or owner assets are included in source/runtime packages. Resource grouping and
  unavailable-buffer handling remain the next bounded map-consumer checks.

- WORLD fog now consumes a retained float reciprocal binding instead of
  recomputing it from every draw's current scene range. Nonpositive/NaN distances
  preserve the prior binding; positive infinity legitimately supplies zero scale.
  Range reset does not reset it. Native water shares that scalar, while immediate
  GUI models use their reached10000 setup without an underwater re-clamp.
  Late overlay, admitted map, item/raw-resource and nonnull card callbacks retain
  their specified ordering. Explicit map admission survives empty geometry, and
  an empty card still submits its setup/palette; null item resources skip setup.
  GPU-readiness failure does not erase earlier setters. Ring composition does not
  incorrectly become another native fog write after the map.

  CPU boundary/cache checks and seven Vulkan cases cover these distinctions;
  the initial world/water cases failed before and pass after. All five targets
  build warning-free; full CPU/owner and87 shader,2 camera,24 GUI,6 map,
  shared-water/Alpha-scenery checks pass. Five no-save captures exit0, all reviewed,
  with the save database unchanged. Initial native GPU contents remain unproved:
  a zero-scale pre-binding fallback is explicit engine policy. These checks do
  not replace native visual comparisons or final release acceptance.

- Ordinary particles and water now retain the preceding WORLD clip depth offset.
  Water depth/color phases have explicit state, its depth phase leaves points zero,
  and only color draws bind zone tables. The water prologue clears points even for
  an empty list; particles retain their material interpretation while submitting
  their own shininess. Two new Vulkan cases failed before the correction and pass
  afterward; a third covers the shared Omega-inspired water path. All five targets
  build warning-free, the full CPU/owner suite and80 shader,2 camera,24 GUI,6 map,
  shared-water/Alpha-scenery checks pass. Five no-save captures exit0 and were
  reviewed, with the save database unchanged. These remain stability/regression
  checks, not original-game visual acceptance.

  The independent point-table lifetime audit now separates CPU collection from
  uploads, identifies late overlay/GUI clears and establishes that construction
  and the early sky path supply no valid numeric initial oracle. The engine's
  empty no-binding fallback is explicit policy, not a native zero claim. Changing
  origin does not imply rebasing a retained GPU table or refreshing a zone key.
  Broader effect callers and native visual comparisons remain open.

- Interaction targets and mission object checks now consume current loaded object
  vectors, including moved/resized footprints, canonical indices, current state,
  full required items and unsigned counts. Generation-time placement recovery and
  native-path optimistic object toggles no longer override received/authority state.
  Chest receipt preserves the full current reward item through inventory and hold-up.
  Rewards remain a separate map, not an inference from the required item.

  The extractor now emits a project-owned reward-map chunk, including explicit
  empty maps, using a bounded guarded reader of the independently specified native
  layout. The engine shares immutable captured maps across source snapshots and
  distinguishes known absence from omitted metadata without hot-path file reads.
  Old captures need refreshing; DataCheck reports missing/invalid maps. Synthetic
  native-memory tests cover signed order, sentinel/black-node distinction, full item
  bytes, invalid/count/cycle cases and visibly changing observations. They establish
  no atomic native snapshot or particular synchronization lock.

  All five targets build warning-free; full CPU/owner and77 shader,2 camera,24 GUI,
  6 map/shared-water/Alpha-scenery checks pass. Five no-save runs exit0, all captures
  viewed, with the save database unchanged. A real owner-data subset passes and
  correctly warns about its older omitted map. The updated extractor is installed
  locally with the previous DLL backed up. Fresh native-map capture acceptance is
  pending: computer control fails during initialization with missing kernel assets,
  including after reset/retry. The source/runtime packages contain no owner data.

- Generated static-object records now preserve the signed64 user/link ID and
  unsigned required-item count rather than losing both during conversion to the
  live format. The default count remains1; explicit0 and 255 remain unchanged.
  Save/restore integration coverage follows these fields from generation through
  recorded zone state back into a live list. All five targets build warning-free;
  full CPU/owner and77 shader,2 camera,24 GUI,6 map/shared-water/Alpha-scenery
  checks pass. A no-save gameplay run exits0 with its capture reviewed and the
  save database unchanged. Current-record interaction adapters and captured chest
  reward-map completeness remain separate work; a required item/count is never
  evidence of a chest's reward contents.

- Queued WORLD consumers now group opaque draws by stable engine-owned resource
  identity, preserve FIFO within each resource and keep winding families/context
  order separate. Faded vectors retain append order. CPU header/identity metadata
  survives unavailable GPU buffers: opaque resources fail before per-instance
  setters, while faded color records still submit controls and selected point
  tables before readiness can suppress their draw. Depth-only iterations leave
  the retained color/point cache unchanged. Explicit controls also respect the
  second-context selector rather than forcing every faded model after water.
  CPU unsigned-key/FIFO and four Vulkan ordering/readiness cases pass; all five
  targets build warning-free, with full CPU/owner and77 shader,2 camera,24 GUI,
  6 map/shared-water/Alpha-scenery checks passing. Five final no-save captures
  exit0, all viewed, and the save database remains unchanged. Temporary profiling
  code was removed. Capture observation failure now returns an error instead of
  success without an image. Native heap-address order cannot be inferred from a
  catalog or equated to the engine's allocation order; exact cross-run/native
  address inputs, the incoming no-point-binding baseline and visual acceptance
  remain open. A separate live-object supplement now specifies required-item
  counts/users and the distinction between object records and chest reward maps.

- Owned ground gear now submits the specified eight owner-extracted `cube.cub`
  draws, using exact-zero delay, adapted category, personal-data and owner gates.
  The per-frame list uses the raw item position, regional RGBA colors, unsigned
  clock/443 stepping, separate pulse/orbit/rise phases, local rotations and full
  allocation-header centering. It has no additional per-cube CPU admission or
  synthesized geometry. The immediate state0 pass follows second-context fades
  and precedes burst particles/water, explicitly setting White0/Glow1 while
  retaining the preceding WORLD opacity, shininess, depth offset and point tables.
  Focused CPU tests and three Vulkan comparisons cover these boundaries. Strict
  floating-point compilation preserves this module's staged arithmetic. All five
  targets build warning-free; full CPU/owner checks and73 shader,2 camera,24 GUI,
  6 map/shared-water/Alpha-scenery image checks pass. Five no-save gameplay/menu
  runs exit0, all frames viewed, with the save database unchanged. A new independent
  queued-state supplement distinguishes opaque grouping/readiness from faded
  setters; full resource grouping, failed-resource inheritance, the no-earlier-point-
  binding baseline, other effect families and native visual acceptance remain open.

- Captured zones now seed their gameplay item/object lists with the owner's exact
  CWR1 records, before the existing saved-state overlay. Missing or empty captured
  lists remain empty rather than being regenerated. Loaded static-object rendering
  follows authoritative replacements, including moved, resized, turned or removed
  objects; an empty received list removes previous geometry. Placement identity
  excludes clocks, interaction state and padding, so live door state can animate
  without rebuilding placement. Synthetic integration coverage checks whole-record
  preservation, source/overlay order, replacement geometry, live clocks and removal.
  All five targets build warning-free; full CPU/owner checks and70 shader,2 camera,
  24 GUI,6 map/shared-water/Alpha-scenery image checks pass. Five no-save captures
  exit0, all viewed, and the save database remains unchanged. This closes the
  captured-list and live-placement gaps for these paths; other content producers
  and native visual acceptance remain open. A separate stable clean-room supplement
  now specifies the owned-ground eight-cube consumer; its implementation is next.

- Steam ground rendering now consumes the live native-format zone lists in physical
  player-centered loaded-zone order. It checks exact timer0, class-adapted resources,
  the raw-position sphere and original-record validity, then uses the specified
  fixed hop, style transforms, two-axis tumble, channel0 material, RGBA pulse and
  selected-tuple addition. Pixel fade retains the un-hopped point and model range;
  normal winding, shininess and separate item opacity reach the descriptor consumer.
  Weapon spirits retain the supplied material and spectral-color policy. Stable
  zone/index tags interleave ground/base/spirits before that zone's static objects
  prior to queue consumption. Native interaction boxes share the placement math,
  and missing native resources no longer become invented bobbing pouch draws.
  Alpha/legacy drawing retains its separate existing path. Synthetic CPU cases and
  two Vulkan cases cover hop/sphere separation, centering, pulse/selection, spectral
  alpha, raw-position admission and faded ground/static ordering. All five targets
  build warning-free; full CPU/owner checks and70 shader,2 camera,24 GUI,6 map/
  shared-water/Alpha-scenery image checks pass. Five no-save gameplay/menu captures
  exit0, all viewed, with the save database unchanged. The owned-ground cube effect's
  downstream appearance, exact resource-pointer ordering, alternate callers, live
  list completeness and native visual acceptance remain open. Current indoor/blue
  gameplay and sky-only menu backgrounds remain stability fixtures, not native
  visual acceptance.

- Ordinary Steam static objects now come from loaded data zones independently of
  terrain GPU publication. Lookup never loads an absent zone; the physical player's
  inclusive seven-zone square supplies absolute XY/record ordering. Cached records
  refresh on zone lifetime changes and explicit invalidation, keep live door clocks,
  and do not duplicate the mesh-published object copies. Only small lifetime tokens
  are retained, so this cache does not pin obsolete terrain snapshots. Valid captured
  objects at fixed position(0,0,0) are no longer discarded. A synthetic headless
  World/Extracted/Renderer integration test covers these boundaries, source reset,
  same-lifetime refresh and missing-zone lookup. All five targets build without
  warnings; full CPU/owner checks and68 shader,2 camera,24 GUI,6 map/shared-water/
  Alpha-scenery image checks pass. Five no-save gameplay/menu runs exit0, all captures
  viewed, with the save database unchanged. This closes terrain-publication gating
  for stable source records; complete live object-list replacement, ground-item
  interleaving, special extra geometry and native visual acceptance remain open.

  Local workspace maintenance also removed122 obsolete generated source-staging
  trees (about1.154GiB) and regenerable Python bytecode. Recent snapshots, the current
  build/dependency setup, owner data, saves and reference captures were preserved.
  No standalone obsolete Makefiles outside dependencies were identified.
  A further62 obsolete smoke-test executable copies (about0.865GiB) were removed,
  retaining their captures/logs and the recent tested binaries. Total local cleanup
  reclaimed about2GiB; current build products and packaged checkpoints remain.

- Ordinary Steam creature poses now receive the prepared scene's frame material,
  effective channel0 physical-position light, ordered buff tint factors and separate
  raw-drawn-position fade/visibility opacity. The descriptor bridge retains RGBA,
  all five scalar controls and the producer's explicit normal/opposite winding.
  The WORLD shader consumes already combined material once, including underwater
  frames, supports negative Glow luminance, and applies clip DepthOffset. Faded
  descriptors use normal/opposite depth then normal/opposite color passes.
  The105 independently verified playable-hair resources retain raw RGB and hide
  only marker-owned faces, preserving raw neighbor and corner-occlusion occupancy.
  Owner models remain required; optional dual-tone hair and Alpha retain their
  separate policies. Full CPU/owner checks and68 shader,2 camera,24 GUI,6 map,
  shared-water/Alpha-scenery image checks pass. Five no-save gameplay/menu runs
  exit0, with all captures viewed and the save database unchanged. The final
  catalog-table readability cleanup was rebuilt and its focused checks rerun.
  These checks cover the specified finite inputs and available resources;
  complete catalog/factory policy, unavailable-resource retained state, exact
  resource-group ordering, alternate callers and native visual acceptance remain
  open. The blue/indoor gameplay fixture and sky-only menu background are not
  accepted as a finished native appearance.

- Ordinary Steam static objects now use their own admission path, independently of
  the environment model's cached terrain-box gate. Available scene records retain
  zone/record order and use the player's physical-coordinate inclusive seven-zone
  square, the independently specified half-height sphere and the model-range
  distance/fade limit without an extra64 blocks. Scenery and animated door/window
  transforms retain exact fixed-position anchors before float conversion; Alpha
  and shadow transforms keep their existing coordinate policy. Synthetic boundary,
  overflow, plane and fade checks pass, including a Vulkan object whose containing
  zone fails the environment-mesh gate. Full CPU/owner checks and57 shader,2 camera,
  24 GUI,6 map/shared-water/Alpha-scenery image checks pass. This closes the renderer
  gate for available scene records; supplying objects from every loaded zone before
  terrain-mesh publication, ground-item ordering and broader native visual
  acceptance remain open. Five hidden no-save Steam/Alpha gameplay, title and
  idle/hover character-selection runs exit0, with all captures viewed and the
  character database unchanged. Their visible blue/indoor scene and sky-only menu
  background are stability coverage, not native visual acceptance.

- Normal world pose admission now uses the current scene's camera, planes and actor
  range, in registration order, before any state/clock/spark/cache updates. The
  independently corrected sphere includes max(body height,0.6*max(width,depth));
  race12e adjustments and low64 center arithmetic remain separate. Actor data and
  sphere math are shared with speech/emotes, and all body parts carry the admitted
  actor sphere instead of generic per-part range gates. Renderer scene preparation
  is reused once by the GPU frame; earlier death/use queries retain the prior CPU
  scene until that boundary. Exact fixed camera coordinates and camera-relative XY
  plane inputs avoid a float-derived distance/origin substitution. Gameplay keeps
  its original descriptor queue position; the title-world caller uses the same
  policy and now performs its previously missing ordinary drawn/link phase.
  Direct widget/character-preview calls retain their separate policy.
  All five targets build without warnings. Focused real headless draw/order/cache,
  shared sphere and particle tests pass, as do full CPU/owner and48 shader,2 camera,
  20 GUI,6 map/shared-water/Alpha-scenery image checks. Current-scene replacement and
  one-time range preparation have Vulkan integration checks. Steam/Alpha no-save
  combat, title and character-selection runs exit0 with all four frames viewed and
  the character database unchanged. These are bounded stability checks; the blue
  gameplay scene, empty-sky frontend underlay and missing owner hair are not accepted
  as native pixel parity. Pose/body matrices still use the engine's source-home
  frame; their native cached-coordinate lifetime, factory/startup ordering, optional
  preview counts and broader shadow/scenery/viewport acceptance remain open.

- World work and local body/burst/scene-pose randomness now use the independently
  specified CRT32-bit transition and15-bit return. Pose sparks no longer use a
  static unrelated Mersenne Twister. Ordinary expansion and its once-frame update
  now precede scene pose, so spark initialization continues earlier World/body/
  burst draws. AI keeps its distinct engine state and unchanged CRT transition.
  The helper chooses no seed or thread; engine role states retain their earlier
  numeric initial choices, explicitly unverified as native entry/first-frame data.
  A known1037-draw initialization subphase and overflow test check the arithmetic;
  actual body, burst and state61 pose consumers check continued draws, zero-draw
  updates and separate execution-role state. First-frame state, all reached
  crafting/stock/daily/pet resets, exact pose admissions/visitation and remaining
  same-thread consumers are not closed by this result. No owner data is packaged.
  All five targets build; focused particles/body and full CPU/owner checks pass,
  with48 shader,2 camera,20 GUI,6 map and shared-water/Alpha-scenery image results.
  Both no-save combat styles exit0 with viewed frames and unchanged character data.
  Short hidden combat FPS samples vary substantially and are not a controlled
  performance comparison; extended profiling and the existing scene/image gates
  remain open.

- Combined sessions now own a separate host terrain reader, following the native
  allocation/local-byte-delivery ownership contract. Source metadata is copied;
  loaded identities, columns, model/light caches, history, queues and worker
  lifetimes are not inherited from the client. Host saved-zone callbacks, travel
  events and item placement use the authority reader; the client retains its own.
  A dedicated host keeps its supplied reader. Synthetic roof/sea tests exercise
  source metadata, fresh base loading, client solve versus host light, independent
  physical/history changes, copied protocol-notice application, client history undo
  and source reload without resetting the host lifetime. Generated rebasing and
  dedicated reuse have coverage. No host raw-light reset or guessed solver schedule
  is introduced; full host scheduling and original-peer/native image acceptance
  remain separate gates. The audited ordinary mesh worker is client-World-bound.
  Owner captures may already contain solved light bytes; forks preserve source data,
  without asserting that those snapshots reproduce every native host initial value.
  All five targets build without warnings. Focused storage/network/cache and full
  CPU/owner suites pass, alongside48 shader,2 camera,20 GUI,6 map and shared-water/
  Alpha-scenery image checks. Both no-save combat runs exit0 with viewed frames and
  unchanged character data. These short runs are stability checks, not performance
  benchmarks or acceptance of the known blue/cavity scene and missing owner hair.

- Ordinary consumers0/3/4/6/7/8 now use the independent style contract alongside
  existing1/2/5 and body records. Their gravity/angular differences, extra style4
  pass types, descending6 full-XYZ contacts and deferred retirement are implemented.
  Style3 stays still while aging; styles7/8 shrink geometry without changing the
  stored culling radius or rejecting lifetime equality. Style7 loads the owner's
  note.cub with its own full header, sets Glow/Shininess1 and resets shininess for
  following draws. Other glowing styles keep shininess0; ordinary lit types retain
  the common stored-block accessor. No new CRT draws or replacement geometry were
  introduced. Synthetic motion/header/retirement and GPU ripple/reset tests cover
  these bounded consumers. The first GPU expectation omitted the camera gradient
  translation; an independent shader-contract calculation corrected that test
  reference without changing rendering. Native duplicate-erasure recovery, unknown
  style numbers, broader producers and native CRT/pixel acceptance remain open.
  All five targets build without warnings; focused particle/body and full CPU/owner
  checks pass, with48 shader,2 camera,20 GUI,6 map and shared-water/Alpha-scenery
  image results. Steam and Alpha no-save combat runs both exit0 with viewed frames
  and the character database unchanged. Those blue/cavity scenes and missing owner
  hair remain unaccepted native visual comparisons.

- Completed mesh-light solves now write target-zone stored air/water red back
  before model samples and terrain/GPU installation, in both rendering styles.
  Other block bytes, halo records, loaded-zone identity, history and dirty queues
  remain intact. Physical writes are immediately readable; dirty requests preserve
  old red until another solve. Upkeep and lit particles use the same corrected
  stored-column accessor, including the native signed fixed-coordinate narrowing.
  Engine commits use serialized immutable snapshots rather than reproducing native
  per-record concurrent writes. Lazy object metadata now follows that same write
  scope, preventing it from being mutated while another writer copies the Zone.
  Actual roofed-room CPU mesh tests verify stored red, the native spread floor21,
  auxiliary/effective sea rules, unchanged halo/identity/queues and physical edits
  before remeshing. The full CPU/owner suite and all Vulkan image suites pass;
  final focused fixture/cleanup checks pass. No-save Steam and Alpha runs both exit0
  with viewed frames and unchanged character data. The first45-second observation
  window was too short; retained-handle runs allowed full startup/cleanup to finish.
  Images remain stability evidence, not native pixel acceptance. Native partial-write
  concurrency and broader visual/peer comparisons remain outside this bounded result.
  The existing combined-session setup still shares one terrain reader between host
  and client; independent World cache/light ownership is the next integration task.
  The independent main-thread CRT contract is available for later stream work; exact
  first-frame seeds and all reached reset/consumer integration remain open.

- Native body fragments now share the ordinary particle list, update, owner cube
  draw and1000-entry tail cap. Prepared sprite IDs and the exact ten cached part
  matrices are retained by value; death visuals no longer recompute a corpse pose
  or fragment an arbitrary draw list. Raw owner CUB data is read without requiring a
  ready source GPU mesh, and policy0 surface/marker checks precede the X/Y/Z sampling.
  Producer draws, quantized homogeneous positions/center velocities, tint, negative
  delay, retained reflected basis and randomized temporary/head insertion follow the
  independent body contract. Body and burst cap/order, the existing relative falling
  sample, cached pose slots and matrix/header behavior have focused regressions.
  Received death notifications now collect frame-local actor-instance candidates;
  strict-negative local-player damage collects one too, while exact-zero subtraction
  does not. Deduplication, prepared/retained-renderer admission and unsigned identity
  order are shared by body production and its following race90 light. Births run
  after received events and before control/revival/current-frame pose preparation.
  Progression/persistent deaths remain separate; later frame notifications may
  collect the same corpse again. Recorded-only DeathEffects remains compatibility code.
  The independent light query correction now reads auxiliary byte3, not flags byte5;
  tests deliberately distinguish those bytes. Stored-red publication/upkeep binding
  is the next integration from its newly written contract.
  All five targets build without warnings. Focused particle/body and full CPU/owner
  suites pass, along with46 shader,2 camera,20 GUI,6 map and shared-water/Alpha-scenery
  image checks. A final main-phase build and no-save combat run exit0 with a viewed
  frame and unchanged character database. This is stability evidence; native body
  pixel/CRT acceptance and the blue scene/missing owner hair remain open. Remaining
  ordinary-style consumer prose is now available; its implementation follows.

- Particle admission now uses native renderer+0x49c, the shorter ordinary-model
  radius, rather than the wider adaptive+0x4a4 scene/fog radius. A Vulkan regression
  reproduced the prior erroneous admission between those radii; both admitted and
  cutoff cases pass after correction, alongside all45 Steam shader checks. All five
  targets build. The preceding particle CPU/other image results remain the baseline;
  this correction changes only the particle gate. The independent
  [body-particle contract](specs/steam-body-voxel-particles.md) now specifies the
  next shared-list integration, including retained prepared parts and head insertion.

- Ordinary particle bursts now enter a local live list before outgoing events
  clear. Every style consumes its shared expansion draws; supported styles1/2/5
  implement their extra color/age draws, fixed-coordinate motion, once-frame
  integer-ms update, collision queries and strict lifetime retirement. The1000-entry
  cap discards the newest tail after expansion. Zero-time style5 queries and the
  collision-off double angle advance have focused regressions.
  Geometry loads the owner's cube.cub with full-header centering; no substitute
  model or owner bytes enter packages. The before-water draw uses an opaque,
  depth-writing LEQUAL/no-cull pipeline, source-size sphere admission and inherited
  WORLD point tables/opacity. A128-byte push layout supplies per-instance material
  RGBA, with identity defaults for existing draws. Style5 uses the documented stored
  block channel0 accessor, including upward type10/12/19 traversal and the light floor.
  All five targets build without warnings; the full CPU/owner-data suite,43 shader,
  2 camera,20 GUI,6 map and shared-water/Alpha-scenery image checks pass. New GPU
  cases check zero material alpha without blending, reversed faces and RGB material.
  A no-save combat run exits0 with a frame and unchanged character database; it is
  a stability check, not native particle-image acceptance. The blue scene and
  missing owner hair remain visually unaccepted. Other particle styles retain only
  shared expansion state; body fragments still have a separate simulation/cap.
  Shared body-list integration, full native CRT streams and completed native
  block-light publication remain open, alongside wider visual/peer acceptance.

- Destructive race0x90 first visits now emit debris, apply guarded terrain removal,
  append day-stamped notices/sound, clean unsupported ordinary models and then visit
  live area targets in the specified order. Zero-time repeats retain their native
  history behavior without breaking blocks twice. Model cleanup selects loaded zones
  from independently clamped bounds, retains water support, erases semantic fields
  backward and preserves destination padding, cached light and sticky water state.
  Auxiliary light/emitter lists remain unchanged. Renderer model refresh is separate
  from terrain notices; immutable snapshots and stale-job checks prevent a completed
  older mesh job from reinstalling deleted models or annotating reindexed entries.
  Pure producer and actual facade/column/model regressions pass. All five targets
  build, the full CPU/owner-data suite and Vulkan shader/camera/GUI/map/water/Alpha
  image checks pass. A no-save combat run exits0 with a captured frame and unchanged
  character database. The blue-lit scene/missing owner hair remain unaccepted;
  native peer, material recapture, publication latency and full CRT parity stay open.
  The independent [particle contract](specs/steam-particle-bursts.md) now specifies
  ordinary styles1/2/5 and binds their geometry to owner cube.cub; its consumer is
  the next implementation task, rather than a completed runtime feature.

- Physical terrain writes, history and mesh requests now have separate runtime
  paths. Abilities and active effects use the guarded in-column removal helper;
  full writes/restoration share the specified setter with generated terrain.
  Batches publish one immutable copy per edited zone while same-thread physics
  sees current edits, including nested resets. Background-reader tests cover
  publication and memo invalidation. Loaded-zone history appends no longer replay
  terrain or create missing-zone archives; every World substep records the entire
  outstanding notice list, retaining native repeated entries.
  Received packets concatenate per list before consumption; zone-change processing
  runs once per admitted client frame even with no received lists. A two-packet
  loopback regression reproduced the prior interleaved ordering before correction.
  Notice-driven mesh requests use deduplicated XY block samples at -13/0/+13,
  loaded zones only. Exact native mesh-publication timing remains an acceptance gap.
  The full setter now preserves requested cells, canonical sea substitution and raw
  upward gaps, and colors only downward interior gaps from cached materials.
  Generated zones retain those inputs while releasing their larger temporary fields.
  The extractor writes optional CWM1 material sidecars; readers validate exact size,
  coordinates and finite values. Legacy block-only exports remain readable but
  cannot supply missing downward-fill materials; DataCheck reports refresh needs.
  All five targets build; focused snapshot/adapter/setter/network tests and the full
  CPU/owner-data suite pass. A no-save loopback combat run exits0, captures a frame
  and leaves the character database hash unchanged; the blue-lit scene/missing hair
  remain visually unaccepted. Fresh native material-capture comparison, model support
  cleanup and the first destructive race0x90 visit remain open.

- Received packet7 world events now remain queued while the client world advances
  and smooths drawn positions, then are consumed before player actions. Creature
  and clock reception still precedes that World pass. This corrects the old early
  application of blocks, buffs, hits and effects against the independently verified
  callsite order; the session/client API comments now state the same cut points.
  All five targets build without warnings and the full CPU/owner-data suite passes.
  A no-save loopback combat run exits0 and captures a frame; the character database
  hash stays unchanged. Its blue-lit scene and missing owner hair remain unaccepted.
  Original-game peer and wider gameplay acceptance remain separate open checks.

- Independent terrain contracts now identify ordinary-model support cleanup,
  in-column physical removal, authority/received history appends and the later
  asynchronous mesh request. The [block lifecycle contract](specs/steam-block-change-lifecycle.md)
  records received empty-to-water handling and retained-notice history duplication;
  these facts are not a claim that all terrain/cache wiring is implemented. Full
  saved-history setter behavior, auxiliary-list membership refresh and exact mesh
  publication latency remain outside that bounded trace.

- Destructive-source deferral now applies only to its first-clock-zero terrain
  branch. A nonzero-clock race0x90 corpse advances and retires normally without
  repeating that branch. New pure and vector-backed facade regressions reproduced
  the previous retained-corpse failure before the correction. All five targets
  build without warnings; focused and full CPU/owner-data suites pass afterward.
  The first explosion's terrain/scenery implementation remains open.

- Non-terrain dead-source areas (races 0x119/0x11d) now execute before their
  death-clock advance. They visit the live registration list with the specified
  double-horizontal/float-vertical bounds, small-distance push, critical draw and
  remembered-target ordering. Blast hits use type1; hostile water hits use the
  resolved owner and type2. Friendly water queues two identical negative-damage
  notices before applying healing once through the shared native hit facade.
  First-burst initialized fields enter the common particle queue before nested
  hit output. Native producer alpha/padding are unspecified; the engine uses
  defined storage without asserting a deterministic native value. The independent
  consumer trace establishes opaque material submission for these styles, rather
  than a native outgoing alpha1. Tests cover wrapping, boundaries, late rejection,
  zero-time repeat, callback births, same-pass deaths and actual facade damage/heal.
  All five targets build without warnings; focused and full CPU/owner-data checks
  pass. A no-save combat run exits0 with the character database unchanged and a
  captured frame. The destructive 0x90 first visit, shared terrain/scenery mutations,
  local burst expansion/rendering and broader visual/CRT parity remain open.

- Ordinary nonplayer corpse lifecycle now runs after the object pass. Authority
  advances the float death clock and retires eligible actors at 1,000 ms in
  registration order, clearing their aggro/damage references before deferred
  vector removal and borrowed-view rebuilding. Owned kind6 actors instead revive
  at 20,000 ms when their owner's HP is nonnegative; physical position and health
  are restored while the specified clock, dead ID and other retained fields stay.
  Missing/negative-health owners do not trigger ordinary corpse removal. Existing
  timed/instanced enemy respawn policy remains in place. Synthetic cases and real
  facade tests cover multiple removals, a subsequent substep and owned revival.
  All five targets build without warnings; focused tests and the full CPU/owner-data
  suite pass. Two no-save combat runs exit0 with the character database unchanged;
  the stationary run produces a frame, while moving capture reaches its settle
  timeout without one. The blue-lit scene and missing owner hair remain unaccepted.
  This lifecycle checkpoint deferred all three area-source races; the newer area
  checkpoint above adds two. Destructive terrain and complete frame ordering stay open.

- The specified static-object trap tail now runs in World substeps, including
  zero-time calls. Collection clears state bits 0x2002 on every registered actor;
  kind0 players select only already-loaded zones, retained and visited in unsigned
  stable-identity order. Each object's clock and personal reset precede its own
  automatic state cycle and hit callbacks. Types 0x11/0x12/0x13 use the verified
  strict ranges, float clock buckets, maxHP fractions, push and sound/RNG order.
  The shared hit facade owns actual damage and notices; direct trap sounds follow
  nested hit outputs and are delivered once locally while retained for distribution.
  Physics also receives the current World clock, restoring timed lava damage.
  Synthetic integration checks cover actual local health/events, a legacy callback
  with player ID 77, rejected hits, callback births, zero-time repeat, float rounding,
  clients and overflow coordinates. All five targets and the full CPU/owner-data
  suite pass; a no-save gameplay run exits0 with the character fixture unchanged.
  The inherited blue movement scene and missing owner hair remain visually unaccepted.

- Personal-state cells now share the verified signed32 block narrowing before
  floor-to-cell conversion across object state, lighting, lore, treasure receipts
  and affected ability lookups. A regression first reproduced six failing object
  cases; boundary checks include negative exact values and int64 minimum. Loaded
  zone identities survive copy-on-write block edits/undo, preserving visit order
  while the block lookup memo still invalidates its replaced storage.
  The independent [trap contract](specs/steam-world-traps.md) is implemented within
  that stated scope. The new [dead-area contract](specs/steam-world-death-area.md)
  documents the separate area pass; only its ordinary lifecycle is wired. Other
  object families, full prelude ordering and the complete native CRT stream remain
  open, as do the broader visual and gameplay release gates.

- A new isolated original-game capture resolves the two region-reference failures.
  Its provenance reports the supported build and zero other mod DLLs. All 3,823
  sampled points across 25 terrain fields, 4,847 block samples and 60,000 cell-object
  samples match without generator or expected-value changes. The refreshed 1,596
  zone records include 17 nonempty lighting zones: 8 model and 11 static light records
  match, with zero incomplete neighborhoods. The full CPU/owner-data suite now
  passes; all five targets build. This sampled coverage does not establish every
  region, dungeon, runtime path or pixel comparison.
  The journaled capture restored Save, Mods and ReShade's dxgi.dll; all 58 original
  file hashes match and the journal is closed. The old EngineTests access violation
  came from the already-corrected native-player test setup; its focused rerun passes.
  Extractor startup no longer scans/exports executable function bytes, and hook
  errors identify their location without printing prologue bytes. The engine
  behavior and asset/data capture paths were not changed by that diagnostic cleanup.

- Loaded-only terrain block lookups now distinguish reader lifetimes and invalidate
  their memo when zones are loaded, evicted or replaced. Weak references avoid
  retaining unloaded terrain. Source resets join the reader's prefetch worker before
  clearing its cache, and concurrent loads preserve one resident identity per key.
  Synthetic checks cover reopening while an old neighbor is pinned, same-storage
  reader reconstruction, LRU eviction and release of the old terrain. The fixture
  deliberately uses a non-home neighbor because opening eagerly loads the home zone.
  All five targets and the focused checks pass. Full CPU/owner-data validation has
  only the two known modded region-reference failures. A no-save movement run exits0
  with the character save unchanged; its blue scene remains unaccepted. Separate
  stationary captures show the house/terrain under Steam Native/Omega water and
  Alpha Native water, without establishing pixel parity or a rendering fix.

- The [host CRT audit](specs/steam-host-crt-stream.md) now separates constructing,
  zone-generation and simulation threads, and records the known refresh/pet seed
  producers and continuation rules. The fresh simulation thread's numeric state
  remains unverified; neither the public world seed nor a per-zone seed is a valid
  substitute. The legacy resident altitude RNG integration remains open.

- Resident zone selection now uses the native fixed/narrowed inclusive bounds,
  snapshots loaded runtime Zone identities, retains their storage through the pass,
  and visits unique nonnull identities in unsigned address order. Each selected zone
  completes archive restoration, daily refresh, resident admission and kind9 request
  production before the next zone refreshes. Metadata is initialized independently
  of whether an actor vector already exists for the zone. Unit checks distinguish
  unsigned pointer order from coordinate/encounter order and deduplicate aliases;
  owner-data checks retain the same loaded identity through daily metadata refresh.
  This matches the ordering rule for the engine's runtime allocations. Exact external
  replay still needs the reference zone sequence and incoming CRT state; the legacy
  altitude RNG remains an explicit gap. All five targets and the new checks pass;
  full CPU/owner-data validation retains the two known modded region-reference failures.
  A no-save gameplay run exits0 with the fixture save unchanged. The inherited blue
  scene and missing owner hair model remain unaccepted; native visual comparison
  and clean generator references are still required.

- The specified resident kind9 producer now tracks existing full-word mode0x8a
  separately from newly copied low-byte modes. Per-zone requests create day-derived
  fixed candidates, apply inclusive200-block XY suppression against current live
  kind9 effects, and append independent live/outgoing records. The ordinary effect
  step advances only the live copy later in that World step. Tests cover native
  numeric references, wrapped centers, request ordering, expired-effect suppression,
  exact fields and copy lifetime. The semantic initializer preserves native-unwritten
  damage/padding in supplied defined storage; engine-owned fresh records use zeroed
  storage without claiming those bytes have a native zero value. No new art or
  replacement kind9 renderer is introduced. Zone processing order, CRT state and
  broader render consumers remain separate limits.
  All five targets build; focused kind9 and full CPU/owner-data checks pass apart
  from the two known modded generator-reference assertions. A no-save gameplay run
  exits0 with the fixture save unchanged. No native visual-acceptance claim is made.

- Resident collection now follows registered kind0 membership, including dead players,
  and snapshots both region-permission sets before zone work. Live positions are read
  through stable actor resolution during admission; streaming stand-ins cannot supply
  a player. Existing nonnull registered IDs suppress duplicates independent of HP or
  generated-ID bookkeeping, and a birth immediately joins that admission set. The
  final petrification pass includes local, network, dead and new registered actors.
  Resident transfer retains native16-block home cells with the specified coordinate
  narrowing, truncates copied position toward zero for region lookup, zero-extends
  style32 into 64 bits, and preserves low64 altitude addition. The shared fixed-block
  conversion also drives terrain rays. Asset-free tests cover collection/permissions,
  moving storage, admission ordering, inclusive range edges, extreme coordinate
  narrowing, field-copy widths and constructor/postprocessing order. Owner-data
  checks retain their respawn assertions with explicit registered player fixtures.
  Zone-pointer processing order, incoming/persistent CRT state and the remaining
  zone-effect/prerequisite producers are still separate integration gaps.
  All five targets build, the full CPU/owner-data suite has only the two known
  modded region-reference assertions, and both camera Vulkan styles pass. A no-save
  movement/combat run exits0 with the fixture save unchanged. The blue scene and
  missing owner hair model remain unaccepted; this is not native visual parity.

- Authoritative resident creation, visited daily refresh and admission now require
  that terrain zone to be loaded. The collector no longer creates creatures for an
  absent terrain world or refreshes retained resident metadata through an unloaded
  zone. This uses the existing independent remaining-runtime-gaps §6.1 contract;
  complete zone visit ordering, RNG producers and prelude integration remain open.
  The unload/reload respawn fixture explicitly loads terrain and retains its single
  comparison zone as a separate view while the host's resident zone unloads. Its
  respawn assertions remain intact. All five targets build; full CPU/owner-data
  checks have only the two known modded generator-reference assertions.
  A no-save movement/combat run exits0 with the fixture save unchanged. The blue
  scene and missing owner hair model remain unaccepted; this is not visual parity.

- Host batches now consume an admitted zero-time residual and run both drawn-state
  passes once with the full capped interval, followed by zone output collection.
  AI collection uses retained World registration order instead of placing the local
  player first. The independent [daily-reset contract](specs/steam-world-daily-reset.md)
  puts the authority notice reset after creature bodies and before active effects.
  Pending births join the reset, and all actor views and ID/pointer caches are rebuilt
  after deletion. Kind0 guests, loaded-zone state, effects and queued events survive.
  A queued zero notice remains eligible in every later substep until host distribution
  clears the outgoing group. Configured respawn deadlines also survive a death caught
  by the reset before the adapter's next death scan. Focused checks cover tied AI
  choices, zero/33-ms host intervals, reset/effect order, repeated notices, compacted
  guest storage, queued kill credits and the mirror's authority boundary. Broader
  time/resident preludes, lifecycle producers and native-game comparisons remain open.
  All five targets build; full CPU/owner-data validation retains only the two known
  modded generator-reference assertions. Both camera Vulkan styles pass. No-save
  movement/combat and dialogue-request runs stop normally with the save unchanged.
  The dialogue request produces no speech bubbles, so it is not native conversation
  acceptance. The inherited blue scene and missing owner hair model remain open.

- World body updates now use retained creature registration order. A separate
  full-list pass clears state0x10 before body callbacks, including dead entries and
  called zero-time steps. Physics/ability/interaction retain one HP admission, then
  upkeep runs without the former outer loaded-terrain skip. Synchronous births
  extend the indexed body traversal after the completed clear pass.
  Once-per-frame drawn state now has two full registered passes before local input:
  native quantized XYZ plus the second Z correction, angle/look smoothing and raw
  timer/step side effects; then ordered carried/seat/spin/look/focus/pet placement.
  Linked poses retain distinct raw/drawn fields, independent fixed conversions,
  live relationship gates and the specified sole smoothing-control reset.
  Mesh drawing no longer recopies raw state after input. Shared native angle math
  corrects the widened-float threshold in control, abilities and physics too.
  Packet receivers preserve omitted fields and accept wire timestamps directly;
  the invented200-ms prediction cap and countdown are removed. Derived math/row/
  gate tests and actual-facade pass-order tests pass. Native packet/interpolation
  regressions replace the former engine-extension prediction expectations. All
  five targets build; the full CPU suite has only the two known modded region
  oracle assertions, and both camera styles plus20 GUI Vulkan cases pass. Read-only
  combat/movement and dialogue runs exit normally with the fixture save unchanged.
  The inherited blue scene and missing owner hair model are still unaccepted.
  Complete
  time/resident preludes, AI collection order, reset/roster-rebuild lifetimes and
  original-game/network comparisons remain open.

- Native gameplay camera positioning now retains fixed eye/look-at state. Its
  candidate uses physical coordinates, separate step/posture truncation, living
  mount offsets and boat height. Native same-cell/corner probes, distinct ordinary
  XY/Z smoothing, ability-specific anchors and the fifty-block safety snap replace
  the fixed head offset. Eight visibility guards and sequential shortening rays
  replace the stepped wall check; shortened current distance persists separately
  from requested zoom. Camera rays use type-only obstruction, including exact
  zero-maximum zone lookup without a block read. Audio and next-frame targeting
  retain the fixed eye/anchor directly. Native inverse rotation drives world,
  clip and speech views even at coincident eye/anchor or a vertical camera pole.
  Posture/probe/rate/rounding/guard regressions pass. Both Vulkan styles pass
  coincident/pole/projection/culling tests, and all40 world/20 GUI/6 map image cases,
  shared water and Alpha scenery pass. All five targets build; the full CPU run
  has only the same two modded generator-reference assertions. Read-only combat
  and dialogue runs exit normally with the fixture save unchanged. The blue
  fixture appearance and missing owner hair model remain unresolved/unaccepted.
  Full double-degree angle storage/producers, native startup/reset transfer,
  FOV/render-origin matrix fidelity and live GUI admission remain open.

- Target-candidate visibility now queries the completed renderer epoch, including
  the outer height-aware sphere and separate step-adjusted inner width/range.
  Candidate float keys use the independently specified insertion/partition/heap
  ordering rather than an always-stable sort. The shared admitted loop retains
  strong-charge selection independently of ordinary selected ID and aim depth.
  Its score truncates each low64 squared axis before summing; the first hostile
  kind seeds it and later kinds replace only on a strictly lower score. The live
  path no longer makes a second line-sphere/nearby charge search. Ability0x23's
  hostile exclusion is strict below duration. Derived regressions cover tall
  candidates, distinct frustum epochs/radii/points, quantization, neutral seeding,
  ties and the duration boundary. Full camera state, GUI focus and native/device
  comparisons remain open. Targeting now visits the engine's retained creature
  registration order, including ordinary outer admission of the local player
  before inner skip-self. Updates/storage moves keep their membership position;
  removal/recreation and a reloaded local player join the tail. Resident, runtime,
  network and newly added creature paths record registration without copying raw
  records. This removes zone-grouped/local-first ordering from candidate input.
  Full native list-mutation producer coverage and other World consumers' visit
  order remain separate validation/integration work. All five targets build;
  control/target/registration regressions pass. Both Vulkan lighting styles pass
  retained culling epoch/range/lifetime checks and camera projection checks.
  The full CPU run retains exactly the same two modded generator-reference
  failures, with no new failures. Read-only combat/movement and conversation runs
  exit normally with the fixture save unchanged. Inherited fixture appearance
  remains unaccepted; these checks do not establish native visual parity.

- Creature targeting now runs once before early control. Selected IDs, the
  projected-box flag and independent minimum aim depth survive into the later
  object/item/fallback pass. Cached aim metadata is evaluated after movement at
  its admitted early stage; assistance follows ultimate processing, before owner
  look. Live speaker override and manual-stick slowdown precede camera smoothing.
  The sixty-block aim ray follows the independently specified fixed traversal,
  loaded-zone lookup order, crossing budget and flag0x40 obstruction rule. All
  four target LOS calls use fixed endpoints and a distinct type-only predicate,
  double range rejection, float normalization and full-length-minus-epsilon
  comparison. Creature step offsets and object half-heights truncate before low64
  coordinate addition; ground LOS retains its raw tuple. Target/control/ray tests
  cover separate phases, depth/rank ties, high-coordinate low bits, strict range,
  flags, zero crossings, missing zones and nearby fallback without extra LOS.
  Legacy radian storage, startup/reset/FOV integration, live GUI focus and
  original-game comparisons remain open. Camera positioning and its eight-corner
  terrain caller now use the independent fixed contract described above. All five targets
  build; focused tests and the full CPU suite add no new failures. The latter
  retains exactly the two modded generator-oracle assertions described below.
  Read-only movement/attack/dodge and conversation runs both exit normally with
  the fixture save unchanged. The inherited blue fixture scene and missing
  owner orc-hair-m08 model remain unresolved; these runs are integration checks,
  not native visual acceptance.

- Local control now has explicit early movement/ultimate, owner-look, pet and late
  attack stages. The canonical live path runs Use before pet, Quick Item before
  late attacks, and samples ordinary UI again at that late gate. Chat skips the
  remaining early actions and edge stores. Movement/lamp/pet/ultimate use retained
  logical inputs rather than a broad panel-visibility mask; Ring and pad/full-map
  still have their separate movement gate. Full focused-Widget plumbing is not
  inferred from visible panels and remains part of shared GUI integration.
  The facade now supplies region-owned special-item checks and direct sound hooks,
  resolves the local player in control lookup, and forwards ultimate buff notices
  to outgoing World events. Device-free staged/actual-facade tests pass, including
  post-Use pet timing, strict mount boundaries, owner look and buff/callback output.
  All five targets build; a read-only movement/attack/dodge run exits normally with
  the fixture unchanged. The full CPU run still has only the two reference failures
  below. Full camera state and live GUI admission
  remain open; these changes do not complete controller/GUI parity.

- The rendered/client World now completes its substeps and one full-frame drawn
  position update before creature targeting and local input. New input abilities
  are not immediately simulated again in the same frame; control precedes camera
  smoothing. World meshes collect once after current object/block updates. All
  five targets build. A hidden read-only run exercised movement/combat and exited
  normally; still captures against the previous build retained the same scene,
  and the fixture save hash stayed unchanged in all three runs. The fixture's
  blue environment is present in the baseline too; this is not a native visual
  parity certificate. Earlier/later input cut points and live GUI admission remain
  open under the expanded independent frame-order specification.

- Movement controls now sample retained displayed angles rather than mouse/pad
  targets. Their unshaken left/up/forward basis and the frontend forward vector
  share native float radians/trig, including pitched/rolled up vectors. Focused
  angle/pole/orthogonality checks, all five builds and both Vulkan camera styles
  pass; the full CPU run still has only the two modded-oracle failures below.
  Gameplay's legacy radian storage, complete frame/target order and full camera
  positioning remain open. The independent ordinary-frame supplement now traces
  World-before-input and separate early/late action gates; it is a specification
  for remaining integration, not a claim that the whole loop is already matched.

- Camera/audio timing now follows the independent ordinary-frame/host distinction.
  Frontend and gameplay orbit distance use rate 0.004 independently of angle
  smoothness; camera easing retains the capped double wall interval. Ordinary
  world simulation splits that interval into float 16-ms steps and a narrowed tail,
  including the zero tail, with no carried whole-ms balance. The live loop no
  longer supplies a separate 100-ms clock. Shake reads the client world's float
  elapsed simulation clock after those substeps. Explosion notices use physical
  player squared distance, strict maximum accumulation and same-frame rate 0.005
  decay, even when playback is muted/inaudible. Native cosine shake translates
  only the render/clip/speech view, leaving stored eye and targeting unshaken.
  A regression reproduced the fractional-time error before its correction.
  Independent float references, actual device-free Audio consumption, fractional/
  long-frame/lifetime tests and two Vulkan projection cases pass. All five targets
  build; 40 world/20 GUI/6 map Vulkan cases, shared water and Alpha scenery pass too.
  The full CPU/owner run completes with exactly two known generator-reference
  failures: an owner modded launch overwrote the region-pair oracle. An isolated
  original-game capture is still required; engine generation and assertions have
  not been changed to match that modded data. Double-degree angle state, full
  startup/reset/render-origin/FOV transfer and remaining GUI integration stay open.

- The shared GUI foundation has independently specified pointer routing, capture/
  focus/auxiliary lifetimes, button state/groups, cached owners and frame placement.
  Numeric, reentrant routing and owner-tree regressions pass. This dispatcher is
  not yet connected to the complete live GUI. Retained cursor images now borrow
  the canonical transient stack at the admitted panel's callback tail; no new
  drag/grab/save producer is inferred. Controls local defaults/edits remain
  separate from active loaded bindings and survive reopening. Full capture and
  Controls thumb-drag integration remain release work.

- Map history, wheel/shoulder controls, hovered/forced land state and border depth
  now have bounded CPU/owner and six Vulkan checks, including inherited world
  occlusion. Procedural artifact resources have a shared full-key cache and atomic
  extractor request/status protocol. 332 owner native-lookup samples, captured
  with the owner's mods active, match generated RGB/semantic metadata; they are
  not an exhaustive or vanilla-world parity claim. Local captures and all
  original assets remain excluded from source/runtime packages.

- Current map matrices now precede GUI drawing, and marker collection/projection
  runs inside the admitted Overlay callback after earlier game-UI widgets.
  F4 suppresses the 3D map too; half radius uses retained Control message state,
  independent of full-map mode. Terrain picks precede later marker cursor snaps;
  pointer text samples retained cursor data earlier using its float32 norm.
  Eased screen centres truncate to pixels before projection. Numeric/owner-tree
  regressions, all five targets and the full CPU/owner suite, 40 world/20 GUI Vulkan,
  water and Alpha checks pass. A fresh hidden owner-data capture completed with
  the test save unchanged. Earlier input/centre/toggle phase fidelity, hovered-land
  highlighting and procedural artifact resources remain open.

- Native map information now uses the independent map-location/lore audit.
  Overlay drawing publishes semantic item records and independent flight/lore
  outputs; hidden draws retain them. Earlier coordinates stay copied while the
  item and target-kind flag remain live. Flight pricing/distance/time and static
  styled lore words use their specified arithmetic, spacing, fixed origins and
  late geometry. Rejected lore suppresses the item branch. Owner-tree and numeric
  regressions pass, along with all five targets and the full CPU/owner suite, 40 world/
  20 GUI Vulkan, water and Alpha checks. The expanded 789-snapshot strict audit
  still reports the existing street-lamp turn differences in three fixture sets;
  this is a development checkpoint, not a final parity release. Complete map
  collection/camera/capture timing and earlier inventory sampling remain open.
  A read-only owner-data capture confirms artifact tooltip text; category23
  procedural voxel resources are not yet supplied to its icon renderer.

- Ground and Ring information are connected. Ring keeps the canonical key sampled
  before later toggle/repeat work; ground fallback resolves existing zone/index
  data fresh at draw without loading zones or applying new validity filters.
  Reserved-origin metadata remains separate when Ring supplies the displayed item.
  Native bottom placement uses the prior admitted widget height. Target production
  now rejects quantity zero while preserving negative nonzero quantities, distinct
  from later record resolution. Packet-backed cleared-slot, target-priority and
  owner-placement regressions pass, as do all five targets and the full CPU/owner,
  40 world/20 GUI Vulkan, water and Alpha checks. Private read-only Ring captures
  exercise both GUI scales; the final executable capture leaves its fixture unchanged.
  Full earlier UI/comparison sampling remains open; the bounded map-output
  implementation is described above.

- Information now reads its borrowed original during the later draw callback,
  after earlier palette updates, rather than freezing its text/image beforehand.
  Hidden GUI ancestors skip widget hooks and late size writes while own panel
  state, placement and bindings survive. Map-marker labels follow the game-UI
  windows and precede information. Owner-tree tests cover live palette reads,
  hidden-state retention and resume; all five targets and the full CPU/owner,
  40 world/20 GUI Vulkan, water and Alpha checks pass. F4 repeats are forwarded and
  visibility changes precede binding-wait gates. A const existing-zone ground
  resolver is ready for source wiring. Earlier sampling, map/ground/Ring bindings,
  full GUI layers and synchronous message/focus timing remain open.

- Native item details now use an independent text plan for the specified category
  rows, stars/region/realm text, artifact/rank/pet effects, healing/demon/consumable
  text, exact float rounding and separately rounded comparison deltas. Equipped
  comparisons preserve original address identity, candidate/Shift order, smaller
  fonts and two late resizes. Inline suffixes follow the real glyph pen; fractional
  fonts share the owner atlas. Tests reproduced missing glyphs and stale standalone
  GUI scale before correcting both. All five targets, the full CPU/owner suite,
  40 world/20 GUI Vulkan cases, water and Alpha checks pass. A private read-only
  gameplay captures at two GUI scales display the selected weapon and its original
  equipped comparison, with unchanged fixture hashes and matching validated binaries.
  Earlier selector/source sampling, live map/ground/Ring bindings and broader
  GUI/input/camera and system checks remain release gates. No owner assets ship.

- Native item-preview selection now positions from stored geometry before its
  later draw. The owner background/children precede icon/text and the text-derived
  resize; a hidden draw retains the size, and growth/shrinking affects the next
  background. Four owner-tree lifecycle regressions fail against the previous
  implementation and pass with this correction. All five targets, the full
  CPU/owner suite, 40 world/20 GUI Vulkan cases, water and Alpha checks pass.
  Ring source selection also retains
  its independent ground-origin flag, rather than forcing it to describe the
  selected canonical item. The later formatter/comparison implementation is described
  above; live source producers and timing still prevent complete PreviewWidget parity.

- Native-save customization now uses the actual owner editor root and shared
  WORLD models: equipped thumbnails, raw base/attributes/candidate and material
  palette, with native picking, cached clicks, rotation and bench/movement gates.
  Ordinary item downs rejoin the editor before shared health/chat/Ring completion.
  Full-coordinate pickup/removal and byte placement retain their distinct rules.
  Palette information follows native source priority and the hidden crafting
  anchor; text and image use a separate class-adapted copy. Gameplay wheel events
  are cleared before each poll, preventing idle-frame replay. Focused regressions,
  all five targets, the full CPU/owner suite, 40 world/20 GUI Vulkan cases, water and
  Alpha checks pass. Private read-only owner-resource captures exercise the pane
  and palette tooltip; they are not original-game pixel comparisons. Broader
  cached inventory/input state, GUI capture/map controls, preview source timing
  and map/ground bindings, controller aim and system checks remain release gates.

- Physical inventory wheel messages now retain their event pointer and sign,
  selecting one owner-tree hit in inventory/shop/crafting priority. Shop and recipe
  grids can receive wheel input while the bag is visible elsewhere. Opposing
  messages remain separate; fractional or large deltas do not accumulate into
  different row counts. Idle input passes leave the stored row alone. Synthetic
  priority/sign cases cover this routing. Cached widget range/thumb state, precise
  refresh callbacks, first binding and broader native input comparisons remain open.
  All five targets build; the full CPU/owner suite,40 world and20 GUI Vulkan cases,
  shared water and Alpha ordering pass.

- Alpha shared water now draws its sorted fading-scenery queue after the sky and
  before the refraction snapshot. Previously, transparent scenery could write
  depth before its sky background and leave dark silhouettes. Native water keeps
  its after-water queue, and CPU fade values and depth writes are unchanged.
  New Vulkan regressions failed before the change and pass afterward: dry partial
  fades match between water modes, and beyond-endpoint fades retain the sky.
  Native admission/cache traversal and broader original-game comparisons remain open.

- Character creation now includes optional dual-tone hair controls: second-color
  preview/selection, enable, transition start and blend. The native palette and
  runtime-loaded hair models are reused without a mod DLL. Per-character engine
  notes retain the style through local selection, play and map rendering, leaving
  the native appearance record unchanged. Synthetic tests cover gradient/skin
  handling, hover/commit, bounds, cache identity and metadata round trips. Hidden
  read-only creation captures exercise Steam and Alpha lighting. Remote-player
  synchronization of this extra appearance remains open; no owner assets ship.
  All five targets build; the full CPU/owner suite,40 world and20 GUI Vulkan cases,
  shared water and Alpha ordering pass. The captures verify the new controls and
  hair; broader background/camera and original-game visual comparisons remain open.

- Customization's native edit projection, object-space pointer ray and voxel/attribute
  traversal now have independent engine implementations and synthetic regressions.
  Checks include full header centering, scale independence, stored lens words,
  exact negative-integer cells, marker exclusion and wrapped attribute coordinates.
  The GUI renderer accepts the raw edit matrices/material with its specified WORLD
  palette and retained state. All five targets, the full CPU/owner suite,40 world
  and20 GUI Vulkan cases, shared water and Alpha ordering pass. The editor root,
  presentation and event continuation are now integrated above; these backend
  checks alone do not establish native pixel parity.

- Native customization palettes now read every canonical tab directly: the selected
  weapon's exact base-material item precedes four spirit-material aggregates.
  Signed counts wrap and may remain negative; aggregation retains its distinct
  full-equality and field-only rules. Clicks consume the preceding draw's palette;
  the later draw rebuilds its entries. Synthetic regressions cover ordering,
  extra tabs, inactive attributes, heterogeneous spirits, cancellation and overflow.
  All five targets, the full CPU/owner suite,40 world/16 GUI Vulkan cases and
  water/Alpha checks pass. The later owner-root integration is described above;
  these palette checks alone do not establish complete customization parity.

- Newly loaded inventory models now share one pending atlas rebuild before drawing,
  instead of rebuilding and uploading the font/icon atlas for every new item.
  Explicit layout updates also clear the pending work. A late-resource owner-tree
  regression, all five targets, the full CPU/owner suite,40 world/16 GUI Vulkan
  cases and water/Alpha checks pass. A hidden read-only recipe capture from the
  optimized binary renders the result/ingredient normally and leaves the save
  unchanged. This is a resource scheduling change, with the same item draw rules.

- Formula detail results and ingredients now use original-record WORLD images;
  hovered item information uses a class-adapted copy. Their separate callback,
  pivot/frame geometry, rotation, dark-pass and depth rules follow the independent
  specification. Missing result resources skip the detail hook without advancing
  its local angle, and hidden draws do not accumulate catch-up. GUI timing now
  narrows each capped elapsed step to float then signed milliseconds and wraps
  its accumulated total across title/game frames. Ingredient hover/navigation
  share the same geometry. The full CPU/owner suite, all five targets,40 world
  and16 GUI Vulkan cases, shared water and Alpha ordering pass. Other icon callers,
  GUI layers and persistent widget refresh state remain open. Hidden, read-only
  native-save captures at GUI scale1 render the recipe result/ingredient and hover
  preview, stop normally and leave the fixture unchanged. Capture executables
  match the validated build; these are not original-game pixel comparisons.

- Inventory, shop and recipe cells now render their original records through the
  shared WORLD item helper during the owner's InventoryWidget callback, after its
  child frames. Physical inventory slots are retained rather than packed; holes,
  per-viewport region cutoffs and full-tab scrollbar counts remain distinct.
  Screen snapping truncates toward zero, hover uses half-open rectangles, and
  icons retain native signed scale, category sizing, GUI spin and spirit data.
  Labels use native class/color and mode-specific quantity admission. Resource
  loads are shared within a session. Formula details, hovered previews and other
  callers still need conversion; full GUI layers and refresh timing remain open.
  All five targets build; the full CPU/owner suite, 40 world and 16 GUI Vulkan
  cases, shared water and Alpha ordering pass. A hidden, read-only native-save
  capture renders the owner grid model and stops normally with the fixture
  unchanged. These checks do not establish complete native visual parity.

- Ring Menu now uses canonical category1 slots, retained signed selection and native
  neutral/repeat/animation state. Selected Use precedes independent Quick Item; the later
  toggle and selection update preserve same-frame ordering and pre-repeat labels.
  Bombs and portal stones use the existing shared item helper, including borrowed shrine
  selection without portal depletion. The owner selector artwork, Display-based arrow
  feedback and selected Quick Item3D SpriteWidget are connected. Ring items use their
  distinct queued50-degree WORLD path, full-header dimensions, world-clock spin and
  retained depth before Plasma, without direct-widget black/spirit passes. Combined UI
  open/close sounds are sampled once after visibility changes. CPU/owner-tree checks,
  all five targets,40 world and16 GUI Vulkan image cases, shared water and Alpha ordering
  pass. Original-game/hardware comparisons and other widget callers remain open.
  Location-banner aging now uses capped UI elapsed time rather than simulation substeps.
  Read-only native-save Ring captures at GUI scales1/0.4096 render three owner items;
  the final capture executable matches the validated build and the fixture save is unchanged.

- Native equipment slots now render original item records through the shared3D WORLD
  helper at each SpriteWidget's owner-tree callback. The copied binding, full-header
  sizing, signed GUI spin, dark/main passes and spirit attributes remain intact;
  child labels/frame art follow the item. Hover does not enlarge this caller.
  Record-less legacy sessions retain compatibility images. Inventory grids,
  crafting and other item-preview callers still need conversion; selected Quick Item
  is covered by the later Ring Menu integration above.
  Headless owner-tree checks cover record preservation, draw order, empty/missing
  resources and visibility. All five targets, the full CPU/owner-data suite,
  40 world and13 GUI Vulkan image checks, shared water and Alpha ordering pass.

- Character-menu controls now initialize and animate the owner's ordinary Button sequences
  instead of showing the saved gray idle pose. Select uses live effective frames and20 physical
  pixels of padding. Card binding is transferred from the copied template; horizontal centering,
  ordered vertical fitting, filtered left-drag and momentum follow the independent specification.
  Pointer polling and frame decay retain separate clocks. CPU state/owner-data checks cover
  these paths. Cached GUI masking/shine and preview creature/world rendering remain open;
  these changes are not a claim of complete menu parity.

- Identification now renders the original selected item through the shared 3D WORLD
  shader path, using owner-extracted CUB geometry and full header dimensions. Native
  projection, rotation, main/dark passes, shininess and weapon spirit cubes are
  integrated with immediate UI ordering and retained depth. CPU checks and 13 actual
  Vulkan image checks pass, including TAA/SMAA, shadows and both lighting presets.
  Read-only native-save captures at GUI scales1/1.5 render the selected owner sprite
  and stop normally with the fixture save unchanged.
  Other widget callers still need conversion from bitmap previews; full inherited
  callback state, offscreen GUI layers and original-game visual comparison remain open.

- Shrine-choice map mode now follows item and creature use, restricts hover to shrine
  targets and accepts the preceding actual draw's target on eligible left release.
  It checks known cells with negative-coordinate floor division, starts the delayed
  native return ability, and preserves item quantity. Deferred mode/source cleanup,
  float world-clock pulses and GUI-scale label sizing are integrated. CPU/owner-data
  checks pass; full original-game/controller comparison remains a release gate.
- Character Select/Create and delete-control placement now uses the owner's effective widget
  frames after resizing, rather than treating the requested sizes as final dimensions. This fixes
  Select extending below its character card. Owner-shape picking replaces the old rectangular
  checks. Owner-data regressions cover UI scales 1 and 1.5; broader menu visual parity remains open.
- Native-save gameplay now uses canonical inventory tabs, equipment and counters.
  Screens derive live slot views and the save bridge writes those actual records.
  Direct equipment/use/drop, pickups, quick items, crafting, cube upgrades, shops,
  identification, pet food and daily reset use their specified distinct operations.
  Shop regressions check transaction data, separate
  sale nodes, whole-node buyback attempts and ordered refresh callbacks. Health
  adjustment stores its rescale before the second maximum-health callback/cap.
  All five build targets, focused inventory/save/loot-window checks and the full
  CPU/owner-data suite pass. A hidden read-only native-character run rendered
  the inventory and stopped normally. Native identification uses the owner's
  blackwidget/itemframe tree, native text/footer positions, live selection and
  release gates. Transformed edge/navigation tests and the CPU/owner suite pass;
  read-only captures at GUI scales1/1.5 exercise the owner artwork and typography.
  The remaining shared3D icon callers still use bitmap compatibility previews; full
  map-panel/capture behavior and extended owner validation remain open. Ring Menu
  and selected Quick Item are covered by the later integration above.
- Native adaptive scene range is integrated: actual unmerged face costs/readiness,
  option-derived window, native equal-key sort, budget threshold, easing and player
  state feed distinct terrain/model/actor/light ranges. Pending cache metadata is
  separate from active GPU geometry and is reused coherently for culling and ground
  lights. Missing legacy cost evidence is not replaced by a guessed triangle count.
  All12 sampled clean native min/max/center/radius and weighted costs match; the full CPU/owner-data suite and
  40 actual Vulkan image checks pass. Native radius15 also completed a hidden no-save
  settled-world capture (sampled private peak1,687MiB). Camera, inherited shader/pass
  inputs, readiness-opacity consumption and wider native visuals remain open.

- NPC speech/emotes now use owner templates and models, with per-speaker playback,
  repeat-Use advancement, exact capped clocks, cached projection and native emote
  pose/depth/culling rules. Lore and receipt scripts append to the general panel;
  final-page lore activation preserves its distinct sound rule. Category24 item
  descriptions receive the item's region context and verified description phrase.
  The full CPU/owner-data suite, 33 actual Vulkan image checks and a hidden no-save
  live speech capture pass. `tools/validate_engine.ps1` runs the full suite followed
  by actual `steamshader` rendering; `steamvoxel` is a CPU data test. These checks
  do not establish complete native visual parity. The other implementation/system
  checks below still need work.
- Saved recipe records now reach the crafting UI through the native progress bridge.
  Missing startup defaults use full item identity; unlisted records remain preserved,
  and crafted results retain formula and all128 upgrade bytes. Full CPU/owner-data
  tests pass, including lifecycle/default-merge and attribute/padding regressions.
  This batch changes no shader path; Vulkan image checks remain the33 passing cases
  from the preceding dialogue checkpoint. No book receipt or decoder-only opcode
  was turned into an invented recipe award.

- Live lighting now reads personal used-cell state, kind3/4 projectiles and
  glowing ground items from gameplay. It preserves native ownership, timer,
  quantity, coordinate conversion and fade rules. Model and terrain light ranges
  are separate. Hit flashes now use consumed local/received hit records, current
  spirit weapons and native fixed positions, first-frame aging and expiration.
  CPU regressions and 28 Vulkan cases cover these sources, including zero-fade
  slots, personal state updates, projectile filtering/removal and owned/delayed/
  picked-up items. The race0x90 death flash is also integrated, using the persistent
  pose-preparation marker, drawn position, cached previous renderer visibility,
  fixed 100-block range and native candidate ordering/800-ms lifetime. Death births
  precede hit-display flashes. Further CPU/GPU checks cover these gates and cached
  visibility across pending origin changes. Live player/NPC records and buffs now
  supply carried/toggled lamps, race/state effects, casting and equipment spirit
  glow, with native anchor/rotation/truncation and source ordering. The casting
  sphere range has its separate native zone-window calculation and 256 cap.
  Native mesh-budget/adaptive range inputs are now connected as described above.
  Camera state, shader/pass inheritance and broader native visual comparisons remain
  open. Source-branch tests do not prove complete frame-input parity.

- Isolated reference capture now runs automatically after game-session
  initialization and refreshes cached light snapshots. The real-install run
  captured both auxiliary lists in 1,590 zones; all 8 model and 9 static lights
  match the engine reader, including mesh-time light samples. All 3,823 terrain
  reference points across 25 fields and 4,847 block samples match. The two region
  failures came from the earlier BigRegions-modified reference. The full suite
  passes, and original Save, Mods and ReShade were restored byte-for-byte.
  These sampled comparisons do not establish full visual parity.

- Pyro regional difficulty now uses the first entrant's level while a region has
  occupants. The host shares it in multiplayer; single player uses the same path.
  The last departure/disconnect clears it, so re-entry chooses a fresh level.
  Permanent discovery difficulty locks are ignored. Existing biome/id variation
  and gear-level records remain. Real two-session UDP tests cover publication,
  retained occupancy, last departure and refreshed re-entry.
- Steam model/beam visibility now uses the mesh job's native column/face prepass,
  published-geometry readiness, five-plane containing-zone test, stored center,
  exact early fixed-distance arithmetic and raw six-plane model-sphere test.
  Native CPU coordinates retain unshifted game Z. Missing neighbor data retains a
  conservative fallback. The camera lens/distance settings remain configurable;
  adaptive inputs are integrated above; native camera-state matching remains open.
  A versioned owner-export record captures coherent bounds/counts/triangle cost
  for future comparisons. Synthetic/file-format regressions and no-save Vulkan
  captures pass. The later12-cache oracle above matches fresh native boxes/costs;
  no complete pixel match is claimed.
- Native Steam front/back/ambient RGBA targets and state smoothing now follow
  the separate author's completed specification. Steam world RGB uses the native
  targets, with optional moon additions kept separate. Tests cover ordered regional/
  structure blends, effect13, low/high day interpolation, constructor values and
  exact native coefficient words. The frame input uses the verified1000-ms wall-time
  cap before float conversion; rendering reads the current client clock/buff state.
  Day/night no-save GPU captures pass. Native ambient alpha is retained in CPU state,
  but its shader saturation/composition stage is still pending. Point registration,
  sky/fog special overrides and remaining shader/color behavior still need work.
- Runtime follow-up: incoming local-player hit rules, native music smoothing,
  gameplay XInput mapping/sticks/camera, page-scoped dialogue marks and daily world/
  character reset behavior now follow the independent runtime-gap specification.
  The complete EngineTests run passed, including the new focused regressions and
  owner-data comparisons. String-script answer activation, inn payments, persisted
  specialization changes and the general feedback panel are now integrated, with
  native word colors, row picking, navigation points and distinct UI action gates.
  Focused tests include the owner's fonts/templates at unit and 1.5 GUI scale.
  Broader GUI controller coverage and the other roadmap
  gaps remain open. The subsequent equipped-pet update pass
  also builds and passes the full suite, with focused creation, naming, health,
  inheritance, authority, travel and detachment regressions.
- Follow-up timing: music receives double-precision elapsed frame time through the
  audio API with the native 1000-ms cap. The title root activates on mouse release,
  correcting the older spec's description of the 0x965e0 input callback.
- Enemy lifecycle: configurable Timed/Instance/Permanent modes. Timed defaults to
  300 seconds; Instance resets on spawn-zone unload for all players. Temporary
  deaths do not enter permanent saves, and mirrors handle repeated deaths.
  Synthetic and owner-data generated-world integration checks cover these paths.

- Water: implemented the written Steam surface curve, colour/depth passes, full-height
  exposed water faces, wet-ground films and underwater tint. Alpha dynamic models
  follow water. Default-on lilies use owner assets as independent render decoration.
  See [water progress](water-progress.md) for checks and remaining lighting/order gaps.
- HP above maximum: fixed native-player initialization and equipment updates at the
  record boundary. The regression preserves wounds between frames. Updated GPU
  captures show 136/136 where the earlier capture showed 169/136.
- Statue slab: compared 81 columns against owner-exported blocks. All 477 stored
  blocks within the slab height range match; the generator was left unchanged.
- Character selection: reuses the owner's rounded `gui.plx` frame, blended behind
  previews so it does not tint them. The owner's alpha is retained without the earlier extra
  0.6 multiplier; a full composition match and the logo overlay's native light are not established.
- Packaging: a separate allowlisted tree and ZIP include required source tables,
  specifications, config templates, project art and dependency notices. Game data,
  screenshots, recordings, saves, SDK checkouts, private handoffs and binaries are
  excluded. Local research is preserved outside the upload tree.
- Source license: the project owner selected GPL version 3 for original engine and
  extractor source (`GPL-3.0-only`). The full text is in the root LICENSE; existing
  third-party licenses and notices remain separate.

## Validation

The development build and full EngineTests suite passed, including synthetic water,
lily and HP checks, owner-data comparisons and the setup wizard's simulated-install
tests. Steam and Alpha Vulkan captures completed on an RX 6600 with saving disabled;
the updated character-select screen was inspected. These cover the tested cases.
The staged source package built without compiler warnings in a separate CMake output
directory using cached dependency sources, and its full EngineTests run passed.
It contains no dependency build products or game exports. This is a source-completeness
check on the development machine, not a fresh-machine installation test.

The latest isolated original-game recapture refreshed generation and lighting
references. Its journaled restoration was verified against the original Save, Mods
and ReShade DLL file hashes. Captures remain private and excluded from the source archive.

## Work still open

- Alpha hybrid: wider seed/biome/domain comparisons, construction-transition and
  forest-edge review, live edits, cold streaming and original cache/input lifetime.
  The implemented default and bounded numeric fixes are documented in
  [Alpha generation](alpha-generation.md); they do not establish full native parity.
- Water: Steam vertex occlusion/point-light parity, Alpha near/far cache/input
  correspondence, and wider waterfall, underwater, wet-ground, dusk/night and
  material comparisons. The independently specified Alpha threshold arithmetic
  and staged draw order have CPU/GPU checks.
- Frontend: wider owner-window visual comparisons and the optional recording-only
  preview compatibility path. Normal saved cards, creation and Options already use
  live owner blackwidget copies, authored fill/Display values and ordinary Plasma
  blending; the independent panel audit confirms that no universal opacity constant
  belongs here. Logo placement, queued projection and fixed WORLD lighting likewise
  have independent specifications and CPU/GPU checks. Full native screenshots and
  unmeasured font/cache/owner hierarchy cases remain separate acceptance work.
- Gameplay edges: sound coverage and feat/chat presentation, ability/AI cases,
  integrated pet combat/appearance comparisons, controller GUI/aim tracking,
  newly observed item-action producers and wider gameplay comparisons,
  and network feat production. See the [audited roadmap](roadmap.md). Recorded
  comparisons do not prove complete gameplay parity.
- Human/system checks: two Steam accounts and an original-game peer, a real first-run
  wizard session, a fresh-machine install and extended save/load gameplay.
- Publication: complete provenance/rights review, including the unresolved original
  CWSDK revision/permission scope for the SDK-dependent extractor. The retained
  Gijs MIT notice matches CubeMegaMod's project notice; it does not establish that
  separate SDK coverage. See [SDK provenance](sdk-license-provenance.md).
  The original-source license choice is complete.
  See [PUBLICATION.md](../PUBLICATION.md). Existing third-party licenses remain separate.

Do not upload the whole development checkout. Review the fresh
`build/CubeVulkan-upload-<timestamp>/` tree produced by `tools/package_source.ps1`,
then upload its contents after the publication decisions above.

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
