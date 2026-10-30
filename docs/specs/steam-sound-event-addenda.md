# Steam sound-event clarifications

Independent native-only prose supplement to sound.md. No engine, tests,
exporter, generated outputs or owner captures inspected; no game execution.
This closes specifically traced old uncertainties rather than assigning
sounds to actions for which no producer has been found.

## 1. Two formerly computed IDs are constants

The shop/purchase call in **0x94900 at0x9617d** uses **sound0x3c**, not
a variable item-category sound. Its apparent 0x3b-plus-register expression
adds the value **1** initialized at0x960f4 on the successful purchase path.
The existing owner item price/inventory/purchase bookkeeping precedes it;
it plays at the local creature's physical position, volume1, pitch1,
ordinary final playback flag1. Do not substitute item-drop, coin or rarity
IDs based on that decompiler expression. Other independent coin/drop
calls elsewhere in the same handler retain their own explicit IDs.

The use-prompt call in **0x98180 at0x98561** uses **sound0x30**, not a
variable pickup/salvage result. All writes to the contributing saved
register in this routine set it to0 (0x98253,0x983de,0x984c9);0x9855a
adds0x30. The call follows successful resolution of the selected creature
and **WorldMap0x2f4290** update, before assigning WorldMap+0x148 to that
creature's ID and rebuilding the personal-list selection. It is a personal
map-list action, **not** proof of an additional physical ground-item
pickup or salvage producer. It uses **G+0x1dc0 camera/listener fixed
position**, volume1, pitch1, final flag1. Physical item pickup/salvage
retain the separate native paths documented in their interaction specs.

Thus sound.md's open question5 needs no run-time oracle for these two
exact call sites. They do not establish additional branches for0x31/0x3b.

## 2. Trap-impact pitch

The relevant object/trap traversal in **world update0x2e3510** uses
float **0.3** from **0x42e704**, binary word **0x3e99999a**. It is loaded
before the per-zone traversal at **0x2e46fc** and restored at **0x2e4ad1**
after the ground-item loop. There are no further changes to that saved
float before the trap impact calculation at **0x2e5d32–0x2e5d59**. The
intervening ground-item-path reload of frame elapsed at0x2e477f therefore
does **not** supply the pitch coefficient to the trap path.

For the existing non-type0x11 trap hit that produces **sound1**, take one
nonnegative CRT rand, convert signed32 to float32, **multiply by float0.3,
divide by float32767, then add float1**, rounding after each scalar SSE
operation. The resulting pitch ranges1..approximately1.3. The sound's
volume remains1, and the position remains the hit creature's physical
fixed XYZ. The predicate, damage and object update order remain those
already specified for0x2e3510; no extra random chance is introduced.

## 3. Area block-breaking sound pitch

In **0x2a45e0**, saved float loaded at **0x2a46b1** comes from
**0x4ac964**, binary word **0x3f000000 =0.5**. It remains unchanged
through the block-breaking sound construction at **0x2a4ccc–0x2a4cf2**.
For the existing once-per-area event that actually broke blocks, the
sound is **1**, at that area's center, volume1. Take one CRT rand,
convert signed32 to float32, **multiply by float0.4** (0x42e52c, word
0x3ecccccd), **divide by float32767** (0x42e5f0), then **add float0.5**.
Round after each scalar float operation. This is approximately0.5..0.9,
not1..1.4 and not an inherited frame elapsed/volume coefficient.

## 4. Queue and received-event wiring already established

remaining-runtime-gaps **§8.2** supersedes sound.md's old queue-lifetime
question: the frame plays the entire current World+0xa0 list once in
list order, then the common event-block reset removes it after distribution/
network sends. A local module that produces a sound request must enter
that common list with its complete48-byte record, preserving position,
pitch and volume; retaining it separately for another frame would replay it.
Direct0xa03f0/0xa0420 interface calls play immediately and do not become
second copies in that list.

The received-feat UI sound **0x1f** is established in
steam-chat-feat-presentation §4. It accompanies literal FEAT! from that
consumer; the asset filename does not prove a quest-award producer.
The client hit/death path and its authority/HP arithmetic remain
remaining-runtime-gaps §1, including its existing death sound, rather
than an unbounded second creature-death hook.

### 4.1 Open/close sound is a sampled UI-state edge

At **0xad7e6**, frame0xa5a50 plays **0x56** on a false→true transition
of **ordinary-UI predicate0x1014f0 OR visibility of G+0x310**; on the
reverse transition it plays **0x57**.0x9ae70 is only the Game wrapper
around0x1014f0 with UI-array base G+0x118. The previous boolean is global
byte **0x551560**, updated to the sampled current state later in this
same frame at0xad7eb's following block. No transition means no sound.
Both calls use camera/listener position G+0x1dc0, volume1 and pitch1.

Thus this edge is broader than explicitly opening an inventory window:
chat's editing byte and the answer panel enter the ordinary-UI predicate,
so their first entry/last exit can produce these sounds. Switching from
one qualifying UI state directly to another does not produce a close/open
pair. This is separate from explicit button58/tab59 feedback. An ordinary
NPC bubble and general DialogWidget G+0x418 do not by themselves make
0x1014f0 true; their appearance alone is not a new menu-open producer.
Use the complete UI predicate already specified in the controller/dialogue
specs, rather than adding each widget's own unconditional open sound.

## 5. Explicitly unresolved source families

These clarifications do not establish a reachable producer for sound IDs
that sound.md lists as unlocated (0x1e,0x33,0x5a,0x6c,0x7a or untraced
instrument branches). Loaded owner sound names alone are insufficient.
Existing ability, projectile, physics, upkeep, object-interaction, ground
item and ambient specs remain the authority for the events they actually
trace. No generic speech-end/item-grab/skill-up event is inferred here.
