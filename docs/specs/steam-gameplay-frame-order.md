# Steam ordinary Game-frame input, targeting and camera order

Independent native-only supplement. Addresses are Steam executable RVAs.
Sources are0xa5a50's longer-budget decompile and native instructions,
0x8ae50,0x9c050,0x342090,0xadcd0, and the existing independent specs linked
below. No implementation, tests, exporter, owner data or game execution
was used. G means Game; W is its rendered/client World pointer+0x08;
P is that World's local creature+0x448. The host has a separate World.

## 1. Invocation and elapsed-time scope

One ordinary frame here means one admitted **0x134420→0xa5a50** call,
not each simulation substep, successful XInput poll or GPU completion.
The retained elapsed input is double milliseconds capped at1000. World
substeps, local control and camera smoothing do not share a substituted
integer/remainder interval. The exact client/host distinction, fractional
steps and zero-frame behavior are **gameplay camera §§7.2.1/7.2.3**.

Queued window messages, ordinary mouse/keyboard polling and successful
XInput routing have their own earlier main-loop phases, before0x134420.
They can change bindings, GUI capture/focus and event-driven state before
this frame consumer runs. They are not moved after physics by the order
below. See **GUI pointer/capture routing §8**, **dialogue-panel routing
§12.1** and **remaining-runtime-gaps §4** for those input boundaries.

## 2. Ordered stages in one ordinary frame

|Order|Native stage and scope|
|---|---|
|1|Clear target creature **G+0x1e58=0 at0xa6769**, plus the related use-prompt target fields/sentinels. Clear projected-creature-box byte **+0x1e88=0 at0xa67da**. This is once per Game frame, before its world-step batch.|
|2|Test wanted/loaded world seed and name, then run the complete client World batch through **0x2da6c0 at0xa7328/0xa734c**: successive16-ms steps and a final float residual. Each step runs its own clock/physics/ability/upkeep work. The batch finishes before the local player-control call below.|
|3|After world updates and intervening frame consumers, including drawn-position pass **0x2b68e0 at0xa7e80** (§7), construct/sort the creature candidate vector, then test the separate local-control admission **0xa9ac4–0xa9b1c** and, if admitted, call **0x8ae50 at0xa9b2c**, with the retained capped double interval and candidate vector. This is at most once in this frame, not once per world substep.|
|4|Within0x8ae50: creature-box/LOS selection; movement and early ability/ultimate work; movement yaw assistance; ability0x9a owner-look; static-object and ground-item passes/fallbacks; prompt/Use/Ring/fallback; pet/call/mount key; Quick Item; finally the UI-gated late normal/strong/class/standing-dodge region. Exact internal gates are§§3–4 and6 below.|
|5|After0x8ae50 returns or is skipped, later frame UI work reaches the visible **general DialogWidget G+0x418** speaker override. Its target-ID store is **0xa9df6**. This can replace the result of the ordinary selection, without re-running that selection or aim assistance.|
|6|The right-stick block beginning **0xa9e81** computes optional slowdown from the then-current target/box byte, then changes camera targets or customization angles. Its customization visibility check is **0xa9f26**; cubic right-X/right-Y calls for camera mode are **0xa9f43/0xa9f86**. It runs in the outer frame, separately from0x8ae50's admission/early returns.|
|7|The shared speaker/general dialogue consumer runs at **0xaa1bc→0xb40d0**. Queued world sound processing later supplies explosion shake before camera update. These do not re-run the earlier target/manual-stick decisions.|
|8|After separate integer-argument helper calls, **0xaaf02→0xadcd0** receives the retained capped double interval and performs displayed-angle/distance/shake smoothing, eye/origin/view/projection work once. Scene submission **0xad9d4→0xe2e00** subsequently copies these new matrix inputs.|

This ordering remains meaningful for a zero-ms Game frame: a matching
world reaches its final0-ms world step, and the admitted frame control,
edge tests, target rebuild, dialogue and camera routes still run. There
is no global elapsed>0 admission added to these stages. Their own math
and gates determine effects. Skipping the world batch because wanted and
loaded seed/name differ does **not** automatically skip the later player
pass: that pass has its own independent tests below.

No additional ordinary0x2da6c0 call follows the local-control selection
in that same frame. Do not insert one so a newly selected ability or
intent is simulated immediately. Conversely, the control handler's own
direct writes, helper calls and queued interactions happen when reached;
they are not all deferred merely because the earlier world batch is over.
Per-creature physics/ability/upkeep iteration remains **creature-physics
§1**, rather than an excuse to repeat local input/aim logic per creature
or substep.

## 3. Local-control caller gate and target lifetime

The call at0xa9b2c requires all of:

* **W+0x210 nonzero**: this is the64-bit length of loaded World name
  string+0x200, not a database/object pointer. World constructor0x29a130
  initializes that string's size/capacity at+0x210/+0x218; ordinary save
  paths likewise use this nonempty-name test.
* P's then-current ability is neither **0x9c nor0x9d**. This is tested
  after the world batch, not latched from frame entry.
* The three live node visibility tests0x9af00 are false for **title
  G+0x118, character selection+0x130, character creation+0x170**.

Those are live **own-node** visibility queries, not a replacement
ancestor-visibility or last-draw-admission test.

There is no extra Controls binding-wait, general ordinary-UI, target
existence, player-HP, System/Options-menu or elapsed-time test in this
caller gate. Individual movement/actions retain their own conditions.
The early target clears occurred even if this call is skipped; the later
general-widget override can still write a target.

Target creation within0x8ae50 uses the caller's candidate vector. Its
projected-box flag is set **at0x8ba32 before line-of-sight acceptance**;
the selected creature ID is stored **at0x8bcc4** under its separate
LOS/ranking rules. Exact candidate admission, corners, ray and ties are
**use-prompt §§2.1–2.2**. A box hit need not yield a selected creature.
The later object/item passes may clear/replace the creature ID at
**0x8d334/0x8d408/0x8dc79**; fallback creature selection can store an ID
at **0x8dd88**. Those branches do not clear+0x1e88. Their full priority
and dead-player rules remain **use-prompt §§2.3–2.6**.

Candidate preparation and this targeting precede the current frame's
camera update. They use the camera/renderer state retained at their
actual phase, alongside the creatures' post-world-update data. The ray's
stored look-at origin **G+0x1dd8..+0x1de8** and unshaken forward state
come from the preceding camera update in the ordinary sequence. The
caller's0xe08c0 candidate tests similarly use the renderer's then-retained
clip planes, before this frame's later scene entry rebuilds them. Do not
precompute a new camera/frustum first or re-cast the ray after aim assist
to substitute a different target pass.

The distinct aim-override ID **G+0x1e60** is not one of those early
frame-entry target clears.0x8ae50 clears it later at **0x8c610**, after
its earlier aim/ultimate work and before movement assistance. Preserve
its separate lifecycle from+0x1e58.

## 4. Internal movement/action/UI gates and order

The opening cursor-item drop, climbing maintenance and creature targeting
precede the movement call. Unless its sailing/gliding exception applies,
the routine clears ordinary player intent. It calls **0x9c050 at0x8bdbb**
only when both float stun timer **P+0x130** and lock timer **+0x134** are
≤0; unordered timers skip the call. Dodge timer+0x12c is not this gate.

After that movement opportunity, **0x8bdc4–0x8be37** returns from the
rest of0x8ae50 if any of these holds:

* **GUI engine+0x170 is nonnull**, queried by0x342090. This is the
  retained **focused Widget pointer**. Older “modal window” shorthand
  must not be expanded to mean every shown menu/panel.
* Actual chat editor active byte **(G+0x398)+0x1d8** is nonzero.
* Gliding predicate0x5ee70 is true and stamina **P+0x9a0 is strictly
  less than float0.001**. Equality does not take this early return.
* Current ability is **0x7d** and timer+0x6c is **strictly below2000**.

These exits happen **after creature selection**, so they do not undo
that frame's selected target/box flag. They precede lamp/ultimate edge
stores, movement aim assistance, object/item replacement, Use/Quick and
late attack/class handling. Do not globally advance edge-history bytes
for handlers skipped by those returns.

Movement0x9c050 has its own staging: sailing/gliding prework precedes
the ordinary-input returns. That ordinary part returns for retained GUI
focus, active chat input, shown **Ring selector G+0x310**, or full map
**G+0x668 with last-device pad G+0x1958 nonzero**. It does not make the
whole0x8ae50 target pass disappear. Exact movement/jump/dodge tests remain
**player-control §3**, with the corrected ordinary-UI semantics in
**dialogue-panel routing §12**.

When the early0x8ae50 exits are not taken, it updates run/walk, lamp and
long-finished ability maintenance, prepares weapon/aim state and handles
the **ultimate** rising edge around **0x8c4fe–0x8c61f**. It then performs
movement yaw assistance, calling0x17840 at **0x8cafd** and storing the
float result widened to yaw-target double at **0x8cb09**. Its filters,
lead and blend are **controller handoff §3.1**. It is before object/item
replacement and before the general-widget override, with no second assist
pass afterward. Nor is movement0x9c050 repeated after assistance/manual
target changes to rebuild the intent it already wrote.

Static-object/ground-item/fallback selection then precedes prompt
**0x98180 at0x8df69**, Use/Ring dispatch, and independent Quick Item.
Those direct actions precede the late ordinary-UI gate; the exact
selector/fallback priority is **Ring rendering §7.2** and **use-prompt §4**.

Only afterward, **0x1014f0 at0x8ee23** can jump to cleanup and suppress
late normal/strong attack, class-skill and standing-dodge/release logic.
This is the **complete ordinary-UI predicate**, not chat typing alone or
a pointer-hit/GUI-consumed result. Earlier movement/jump, ultimate,
run/walk, aim assistance and direct-use stages have already had their
own opportunity. The selected abilities, release rules and branch-local
previous-bit stores remain **player-control §§4.9–4.12** together with
**dialogue-panel routing §12**. Do not turn that late gate into a blanket
pre-target or pre-world pause.

Controls' **waiting index+0x2e0** is tested by0x9ae50 in the upstream
poll/capture routing; it suppresses the normal pointer/XInput branch
under the previously specified main-loop conditions. It is not directly
tested in0x8ae50's caller gate, by these four early exits, by the outer
manual-stick block or by camera smoothing. Those frame consumers use
whatever stored logical/analog states the input path left. They do not
perform a fresh poll per simulation step or invent a wait-driven reset.

## 5. General speaker override, stick and camera phase

The outer-frame general-widget test uses current live visibility of
**G+0x418's root**, not the NPC bubble vector or answer panel. If shown,
store its speaker ID+0x210 at **G+0x1e58**, including0 or a missing ID.
If that ID resolves, it also replaces the player's look vector+0x174
with speaker **physical position minus player physical position** in
float blocks and sets the existing main-record mask4. The override does
not require speakerHP>0, replace+0x1e88, or repeat earlier aim assistance.

The later manual-stick slowdown separately requires+0x1e88 nonzero and
the **then-current ID** to resolve to a creature withHP>0. Its distance
uses the stored eye; the box byte may have come from another creature.
The ordinary cubic right-stick rate, minimum0.3 slowdown factor and
customization-visible angle destination remain **gameplay camera §3.2**.
There is no fresh focus/chat/wait/ordinary-UI gate in this outer block;
upstream analog routing still determines its retained right-X/right-Y.

After these choices,0xb40d0 at0xaa1bc can change speech/general-widget
visibility/pages. Queued sounds then raise shake before0xadcd0's
independent same-frame decay. Camera update uses the resulting separate
angle targets, with no target rebuild after displayed-angle smoothing.
The later movement-close block **0xab283** can change panel visibility
after this camera call and before scene submission. Neither that later
visibility nor dialogue-update visibility is retroactively applied to
the earlier action gate, speaker-ID override or stick decision.
Projection/render ordering is **gameplay camera §§7.2.2–7.3**.

This supplement establishes the ordinary pipeline and the verified
gates above. It does not move message-driven item/GUI callbacks into
0x8ae50, replace detailed ability/usability rules, or claim that arbitrary
other native callers/modifications share the same admission.

## 6. Exact owner-look, Use, pet and late-action cut points

These are successive regions of the **same reached0x8ae50 call**, not
independent per-substep callbacks:

|Order|Verified boundary|
|---|---|
|1|Movement assistance finishes its yaw-target store at **0x8cb09**.|
|2|The inline **ability0x9a owner-look block0x8cb1b–0x8cc26** runs next. This is an inline branch, not a separate owner-look helper call.|
|3|Static-object scanning begins at **0x8cc26**, followed by ground-item scans and target fallbacks.|
|4|Prompt test **0x8df69**, then its existing Use/Ring/no-prompt fallback branches. The Use previous byte **0x5515a3** is stored at the common continuation **0x8eb40**.|
|5|Pet/call/mount input **G+0x1a7e, logical action14**, beginning **0x8eb46**; previous byte **0x5515a4** stored at **0x8ecf7**.|
|6|Independent Quick Item **G+0x1a74, action4**, with candidate-list builder **0x93e60 at0x8ed1f**, selected canonical-slot use **0xb7740 at0x8edaf**, then previous byte **0x5515a5 at0x8ee05**.|
|7|Only then test ordinary UI through **0x1014f0 at0x8ee23**. Its true branch skips the late attack/class/standing-dodge region.|

The outer general-dialog speaker override is **after the entire0x8ae50
call**, including its late action region. Do not combine it with the
earlier ability0x9a owner-look block or move it ahead of late attacks.

### 6.1 Ability0x9a owner-look

Test P's **then-current ability**, after ultimate/assistance. When it
is0x9a, first clear **mask4 of u16 P+0x128**. Compare float timer+0x6c
against integer duration0x4f220(P) converted to float; only **timer
strictly below duration** continues. Resolve ID **P+0x1a0** through0xe720
at **0x8cb73**. There is no separate resolved-creature HP test here.

When resolved, subtract P's **physical signed64 XYZ+0x10/+0x18/+0x20**
from that creature's **drawn XYZ+0xb10/+0xb18/+0xb20**, before converting
each difference to float and multiplying exact1/65536. Store the vector
at **P+0x174/+0x178/+0x17c**, then set mask4 at **0x8cc1e**. It is not
normalized and receives no added head/step-height offset in this block.

If duration has been reached or the ID is missing, the mask remains
cleared; this block does not additionally zero the existing look vector.
For any other ability it performs none of these writes. It changes
neither target ID G+0x1e58 nor box byte+0x1e88, and does not re-run the
already completed movement-yaw assistance. The later general-dialog
override can independently replace this look vector and mask (§5).

### 6.2 Pet/call/mount key and its precise gate

After the complete Use/Ring/fallback continuation, require the logical
pet held byte nonzero and its **previous0x5515a4 zero**. Then read the
player's current ability/time **at this stage**, and require **timer
strictly greater than duration0x4f220(P)**. Equality does not qualify.
An earlier Use action may have changed these fields; they are not a
snapshot from before Use or from the beginning of the frame.

On that admitted edge, choose in order:

1. If current ability is **0x76**, write ability0 (dismount branch).
2. Otherwise resolve **P.petID+0x9d8**, at0x8eba7. If it exists, its
   physical-position float squared separation from P is **strictly
   below36 blocks²**, and special-item predicate **0x5e890(P,3)** succeeds,
   write ability0x76. The ownership predicate call is **0x8ec4f**.
   This branch adds no pet-HP filter.
3. Otherwise take the call branch: write held-up item **type byte P+0x1d8
   to0x21**, set ability0x9b and timer+0x6c=0, play sound0x7c at the
   **stored camera eye G+0x1dc0** through0xa0420 at **0x8ecb9**, then
   append a freshly initialized **kind10 interaction** at **0x8ecea**.
   Payload/host consumption remain zone-object-interaction §8. This
   branch does not rebuild all other bytes of the held-up item record.

The pet separation test's native **0x8ebd6–0x8ec2e** subtracts signed64
pet-minus-player fixed coordinates first, converts each difference to
float, multiplies exact1/65536, squares those normalized components, then
adds X²+Y² followed by Z², with separate float rounding. It compares that
sum strictly against float36; it does not compute a square root.

The dismount and nearby-pet mount branches do **not** reset the ability
timer; the explicit zero belongs to the call branch. Later camera mount
validation has its separate living/missing-mount behavior (camera §5.1)
and must not be moved into this key handler as an invented extra gate.

Store the current pet held byte to0x5515a4 whenever this stage is reached,
including an unqualified edge/duration test. Holding the key does not
retry when the ability later becomes finished. The earlier focus/chat/
ability returns in§4 skip this store; the **later** ordinary-UI predicate
does not. Quick Item then uses its own independent edge and current
candidate list, with the exact wrap/use/hide rules in slot-lifecycle §9.5
and Ring rendering §7.2. Its previous byte likewise updates on the reached
stage even with an empty candidate list. A shown Ring's bypass of ordinary
no-prompt ground/vehicle fallback is not a return skipping pet/Quick Item.

Finite gate references, evaluated at the named stage rather than as
claims about all subsequent frame effects:

* Ability0x9a, timer equal to duration, resolved owner: clear mask4;
  do not perform the owner-look vector write.
* A pet rising edge with timer equal to duration: no mount/dismount/call,
  but store previous pet=1. Continuing to hold through a later finished
  frame does not become a fresh edge.
* Finished non-riding ability, resolved pet physical fixed delta
  **(393216,0,0)**, i.e.(6,0,0) blocks, special-item predicate true:
  squared distance36 fails the strict test, so take the call branch.
  Delta(327680,0,0), i.e.(5,0,0) blocks, passes the distance test instead.
* Finished ability0x76 with a pet edge: ability0, retaining its timer.
* Ordinary UI true alone, without the earlier exit conditions: pet and
  Quick Item still have their opportunity before late attacks are skipped.

## 7. Drawn-position pass is after World stepping and before local input

The verified ordinary direct caller is **0xa5a50 at0xa7e80→0x2b68e0**.
It occurs **after** both world-step sites0xa7328/0xa734c have completed,
and **before** candidate construction/sorting and0x8ae50 at0xa9b2c.
It is outside the wanted/loaded-world match branch and the local-control
caller gate. It remains reached when that world-step batch is skipped,
when front panels prevent0x8ae50, and for a zero-ms Game frame.

At **0xa7e6f**, convert the retained capped **double frame milliseconds
to float32**; **0xa7e79** places that float in the argument register for
0xa7e80. The interval is the **whole frame**, not the final simulation
residual or a16-ms step. There is **one call**, not interpolation after
each world substep. In a33-ms frame, world intervals are16,16,1, followed
by one drawn-position pass with float33. In16.25 then16.75-ms frames the
two passes receive float16.25 and float16.75 respectively.

This pass walks the World creature list and updates drawn position
**+0xb10/+0xb18/+0xb20** and orientation **+0xb34/+0xb38/+0xb3c**, among
its other pose/state operations. The later candidate/owner-look/assist
stages therefore see this frame's already updated drawn state. They
still see the earlier stored camera state until0xadcd0 (§3). There is
no second0x2b68e0 after pet/Use/late action selection in this ordinary
frame; do not re-run it to adjust a newly selected mount/ability.

The other verified direct caller is the separate **host tick0xf5520 at
0xf693e**, also after its complete World batch. Native0xf6932–0xf693e
converts the saved **full capped integer host interval** to float for
that call, rather than passing its final step remainder. The host does
not run0x8ae50. These are two different World objects/call schedules,
not two smoothing calls to the rendered World in one Game frame.

This section establishes call placement/admission and argument precision.
It does not specify a replacement uniform interpolation formula for the
complete0x2b68e0 body: the network blend-state description is netcode-roles
§5, and further ability-specific drawn-position/pose overrides inside
that native body are not newly resolved by this bounded ordering trace.
