# Steam World creature visitation and live-list boundaries

Stable bounded native-only supplement to
[creature physics §1](creature-physics.md#1-where-it-runs-and-how-time-is-split) and
[Game frame order §2](steam-gameplay-frame-order.md).
Sources: World step0x2da6c0, time prepass0x2e2e80, pet pass0x2e0db0,
drawn-state pass0x2b68e0, registration0x29d0d0 and reset0x2d2eb0.
No replacement source, tests, owner data or game execution were used.

## 1. Shared container and local-player admission

These traversals use the **existing World+0x40 registration list**,
following next links from sentinel.next until that same sentinel is
reached. They do not walk the ID tree, sort by ID, group the local player
separately or take a creature-count snapshot. List nodes contain next+0,
previous+8 and creature pointer+0x10. The separate World+0x450 map is used
for ID resolution inside individual operations.

The local player is a member of this same list. These passes have no
outer skip-self test: they visit P at its actual list position. Its
initial position relative to every other creature was not established
by the bounded startup audit; preserve that explicit limit in
[player control §4.5.1](player-control.md#451-complete-strong-charge-candidate-selection).
Here a registered entry includes a dead creature still in the list;
the word “live” must not become an HP>0 filter on every pass.

## 2. Per-substep visits inside0x2da6c0

The client/Game frame completes its World batch before once-per-frame
drawn-state processing and local input. Its16-ms/residual scheduling,
including a called zero-ms step, remains as specified in the frame
and camera documents. The following loops have **no positive-dt gate**
of their own.

**Time prepass.** The step calls0x2e2e80 at **0x2da728**. That routine
first walks the registration list in its existing order, including P
and dead entries. Its sleep/healing tests select only their applicable
creatures inside the walk; they do not define the list membership.
It reads the current node's next link at **0x2e2f63**, after the current
entry's work. The World+0x440 clock/time changes and any authoritative
midnight reset follow this completed walk. Their arithmetic/lifecycle
remain [camera §7.2.1](steam-gameplay-camera.md#721-exact-world0x440-clock-producer-and-lifetime)
and remaining-runtime-gaps §6.

**Prelude/pet work.** Authoritative resident/update helpers are gated
by World+0x358 equal0. The common pet pass0x2e0db0 is called at
**0x2da8fb**, before the two main creature traversals below. Its
reconciliation and authority-only linked-creature portions, with their
own nested list searches, remain remaining-runtime-gaps §7. They do
not replace the later full-list physics/upkeep traversal. Common
registration at **0x2e11c8** can append a pet in that pass's permitted
authority branch; it is already in the list before both main loops.
The mirror role does not gain that authority-only creation branch.

**Global bit-clear traversal.** At **0x2db649–0x2db672**, start a fresh
list traversal and clear bit0x10 of Creature+0x128 on **every** current
entry, with no health or local-player test. This loop contains no
creature callback; each next-link read follows that entry's clear.

**Main creature traversal.** At **0x2db674–0x2db6d4**, start again from
the then-current sentinel.next. For each entry, capture its creature
pointer and test its HP+0x180 once. If HP>0, invoke, in this order:

1. Physics0x2bb8e0 at0x2db69c.
2. Ability update0x2c5250 at0x2db6ab.
3. Interaction handling0x2c3710 at0x2db6ba.

Then invoke upkeep0x2de4c0 at0x2db6c9 for that same captured creature,
**whether or not the entry passed the initial HP test**. The caller
does not retest HP between those first three calls. For example, an
HP change during physics does not itself suppress the caller's later
ability/interaction invocation; those helpers retain their own rules.

Only after upkeep returns does the loop read the **current node's
current next link**, at **0x2db6ce**. It does not save next before the
callbacks, iterate an initial-count bound, or build a vector of original
creature pointers for this traversal.

## 3. Synchronous tail births during the main traversal

Common registration0x29d0d0 appends before the sentinel, after the old
tail. Therefore an append during a current creature's callbacks can
extend the very traversal in progress. The new entry is visited in
the same World substep when subsequent next-link reads reach it,
including when the current creature was the old tail.

This is a reached possibility rather than a new proposed spawn family:
interaction handler0x2c3710 has direct0x29d0d0 calls at
**0x2c44a1/0x2c4ad9/0x2c4cdf**. This supplement establishes traversal
consequences, not new trigger rules for those existing interaction paths.
The creator's interaction returns and its upkeep completes before
advancement to any newly appended entry; registration does not recursively
run the newborn's main update at the insertion instruction.

A new entry's HP is sampled when its turn is reached, and its physics,
ability, interaction and upkeep calls then follow §2. It missed the
already completed global bit0x10-clear traversal. A creature appended
in the earlier prelude instead participates in both that clear and
the main traversal. If another substep follows, both are ordinary
members of its newly started traversals.

Derived ordering example, not a capture: start with registration order
A,B. A appends C during its interaction; B appends D. Main visits are
**A,B,C,D**, not A,B followed by a deferred next-substep birth batch.
Each visit still finishes the current entry's callbacks before advancing.

## 4. Once-per-frame0x2b68e0 has two separate full-list passes

The ordinary caller is **0xa7e80**, after the complete World batch,
using the full frame interval narrowed to float32, including when the
batch was skipped. The separate host caller0xf693e has its separately
specified World/clock. These schedules are in frame-order §7; this
supplement does not add another call after local actions.

**Pass A: interpolation/snap and drawn look.** The first traversal
starts from World+0x40 at0x2b68e0. It visits every registered entry,
including P and HP≤0 records. There is no outer health/visibility/distance
gate. The stored interpolation timer+0xb58 chooses the snap/interpolation
branch, not whether the entry is visited. This pass updates drawn fixed
XYZ+0xb10/+0xb18/+0xb20, drawn angles+0xb34/+0xb38/+0xb3c and drawn look
components+0xb4c/+0xb50/+0xb54, among its existing branch-specific state.
Full interpolation formulas and raw-state side effects are now given in
[the drawn-interpolation supplement](steam-creature-drawn-interpolation.md).

Its **next-link load is0x2b6bb8**, after the current entry's helper calls
and after preparing the drawn-look differences, but **before** the final
three drawn-look stores at0x2b6bd1/0x2b6be9/0x2b6bf1. Those stores still
belong to the old current creature. No callback follows that next load
before the loop test at0x2b6bf9. This is the precise frontier for that
pass, not a next pointer saved at the start of the entry.

**Pass B: dependent pose/link work.** Only after all of Pass A has
finished, reload World+0x40 and sentinel.next at
**0x2b6c02–0x2b6c06**. Visit every registered entry again, including P
and dead records. Call0x2a12b0 at0x2b6c5e, then run the existing
ability/link/attachment pose operations for that entry. Their internal
actor/HP/ability conditions are not a new outer filter on this traversal.
The next link is fetched at **0x2b760a**, after all of that entry's work.
The complete finite ordinary relationship/caller placement formulas,
gates and field resets are in
[the linked-placement supplement](steam-creature-linked-placement.md).

Thus these are **all A visits, then all B visits**, not A+B fused per
creature. Pass B's ID resolutions/pose reads observe state left by the
completed Pass A and any earlier Pass B entries; there is no all-poses
snapshot. A list append reached before a pass's next-link frontier
can extend that pass, while an entry added only between the two passes
can participate in B without having participated in A. No normal birth
producer was established in0x2b68e0 or0x2a12b0 by this bounded trace;
the iterator consequence does not invent a smoothing-time spawn event.

## 5. Direct removal boundaries and audit limits

The traced main-loop skeleton does not itself erase, unlink or reorder
the current creature node. Its next link is fetched after callbacks;
this does **not** establish that deleting the current node inside an
arbitrary callback is supported. No saved-next recovery or automatic
iterator repair should be inferred. A synchronous removal of an as-yet
unvisited node changes the later links actually followed; comprehensive
removal policies of all nested callbacks were not audited here.

The explicit common reset **0x2d2eb0** has a different, safe two-phase
policy: first collect registration-node addresses whose creature
hostility byte+0x60 is **nonzero** into a temporary list. Only after
that collection finishes, destroy those creatures and unlink/free their
nodes in collected order, decrementing World+0x48. Relative order of
retained kind0 entries is preserved. It then clears and rebuilds the
separate ID map from the retained registration list. It does not merely
remove the local player's enemies or test pointer equality with P;
other kind0 player entries are retained as well.

The time prepass can call that reset on its authoritative midnight path
**after its own sleep-list walk and before the main traversals**. The
other directly reached reset in0x2da6c0 is **0x2db703**, after the whole
main creature traversal has ended, when authority World+0x358 is0 and
the queued World+0x160 list contains a kind0 notice. Its archive-reset
partner follows at0x2db70b. Neither call invalidates an in-progress
main creature iterator at its call site. An entry can therefore receive
its body update and be removed later in that same substep. The once-per-frame
drawn passes see the survivors of the entire completed World batch.
Exact new-day/loaded-zone
semantics remain remaining-runtime-gaps §6.
The complete archive/local partner and retained-notice/output timing,
including repeated late resets within one host batch, are now in
[the daily-reset supplement](steam-world-daily-reset.md).

No direct World+0x40 unlink/reorder appears in the two0x2b68e0 loop
skeletons. This supplement does not claim that every nested lifecycle
producer, all host-only prelude scans, network receive operations or
concurrent mutation policies have been exhaustively traced. It specifies
the verified ordinary pass ordering and synchronous link semantics above.
