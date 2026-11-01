# Steam block-change notice, history and mesh lifecycle

Independent native-only bounded supplement for the ordinary0x2a45e0/
0x2dba40 destruction outputs and their downstream consumers. RVAs refer
to the original Steam executable. No engine/test/extractor, SDK,
reconstruction or owner-capture data was inspected; no game was executed.

## 1. Producer write versus notice payload

The shared removal path **0x2a59f0→0x91160→0xa36e0** physically overwrites
an existing stored block before the producer appends its notice. It writes
water for integer block Z<1, otherwise empty. Its in-column/no-extension
and no-notification/no-dirty-side-effect contract is in
[effect-objects.md§2.7.2](effect-objects.md#272-in-column-block-removal-and-notification-ownership).
The race0x90 area calls it at0x2a4afe, then appends the notice at0x2a4c14;
the active-effect path0x2dba40 has its separately specified block-breaking
branch in effect-objects§1.

The notice's semantic28 bytes are signed32 block XYZ at+0/+4/+8,
six before-bytes at+0x0c, six advertised after-bytes at+0x12 and signed32
day at+0x18. Its native stride is0x20; padding+0x1c..+0x1f is not assigned.
These producers advertise the global empty block even where actual
physical removal wrote water below Z1. An advertised empty block is not
proof that the source physically forced type0 there.

## 2. World-step history append does not repeat mutation

Late in every admitted **0x2da6c0 World substep**, including a zero interval,
call **0x2dba40 at0x2db933**, then walk the current **World+0xc0** notice
list forward. For each record, floor-divide its signed32 block X/Y by64,
with no negative-to-zero clamp, and look up that zone in **World+0x460**
through **0x12700 at0x2db9ad**. For an existing nonnull zone, append the
record to **Zone+0xd8**, incrementing its64-bit count at+0xe0, through
**0xa1680 at0x2db9d4**. Missing/null zones receive no history append.

This tail has no separate authority-byte gate: it is reached after the
earlier role-specific work. There is **no physical block write, before-byte
comparison, day check, coordinate deduplication, cache invalidation or
notice removal** in this history loop. **0xa1680** is a plain append;
its copy chain0x36db0→0x38b00 copies the28 semantic bytes and does not
assign record padding. Subsequent0x2e1750 is the separate local planner
work, not a hidden block-notice reapplication.

The outgoing notice remains in World+0xc0 until the surrounding outgoing
group is cleared. Consequently, retained notices are appended to history
again in another admitted substep. For example, one notice produced in
the first of two substeps, with no new notice in the second, contributes
**two history entries**, while the still-outstanding outgoing list contains
one record. There is no consumed-prefix cursor here. Host batch/output
lifetime is specified in [steam-world-daily-reset.md§6](steam-world-daily-reset.md).

Thus the ordinary authority's below-Z1 physical water is **not replaced
with its advertised empty bytes** by this tail. The history stores the
advertised bytes as written; it does not take a new physical-block snapshot.
On zone unload/save, the history is copied into stored zone state, and
its28-byte entries are serialized in list order; see
[native-saves.md§5](native-saves.md).

## 3. Received notices have a separate physical-application path

Host update distribution includes block-change notices for guests without
the particle-distance filter; zone-state answers can also send the saved
history. Packet7 appends the received records to **Game+0x19a0/+0x19a8**.
See netcode-roles§4/§8 and netcode-packets§2.7 for those transport contracts.
This queue is distinct from the authority's already-produced World+0xc0.

The Game frame **0xa5a50** processes a nonempty received block queue in
list order. When both advertised **after type byte+0x16 and flags byte+0x17
are0**, call **0x91160 at0xa80db** with the record's block XYZ. Otherwise
call **0xa37d0 at0xa80fa**, passing the six after-bytes; that wrapper resolves
the loaded zone and calls its full setter0x31c1c0. The removal branch is
the same guarded, in-column-only operation from§1. It does not compare
the record's before-bytes with the current block or require matching day.

After the physical-application attempt, independently resolve the loaded
owning zone using floor(block X/Y÷64), and append the received record to
its history through **0xa1680 at0xa817f** when that zone exists. A failed
in-column physical removal does not prevent this history append. Missing
zones are not generated to satisfy either operation.

In particular, the received empty after-value from§1 selects0x91160:
**below Z1 it writes water, not literal type0**, if the stored cell exists.
The authority and receiving mirror therefore both use the water-preserving
removal rule despite retaining an empty after-value in their histories.
Saved-history replay uses the full setter0x31c1c0. Its broader extension,
color and global-substitution behavior is not redefined by this supplement;
this is not a completed saved-history restoration audit.

The received-block path is **after** this frame's admitted World batch
(0x2da6c0 calls at0xa7328/0xa734c) and drawn pass0x2b68e0 at0xa7e80.
It is not a pre-World-step physical application. Later, at **0xa8775**,
the received records are appended to the World+0xc0 list through0x38e00 for
the frame's later ordinary consumers. This does not route the authority's
own outstanding notices back through the received-queue mutation branch.

The received-block loop tracks extrema but contains no direct call to
the support-model cleanup0x2e0710. That cleanup remains a separate producer
operation; model removal must not be inferred merely from the history append.

## 4. Later Game-frame mesh invalidation

The same Game frame calls **0xaf110 at0xaaf0c**, after the received-list
append and before scene draw0xe2e00 at0xad9d4. This helper reads **all current
World+0xc0 records**, independently of whether they came from local effects
or the received queue. It captures **timeGetTime once** on entry.

For each notice, take the nine block-space samples formed by X offsets
**−13,0,+13** and Y offsets **−13,0,+13**. X is the outer loop; Y the inner.
Add each offset in signed32 low-bit arithmetic, then floor-divide the
result by64. Deduplicate the resulting zone-coordinate keys in a temporary
set. These are **nine block samples, not a3×3 zone neighborhood**. No Z,
before/after type, day, mutation-success or actual-coordinate-change test
filters this invalidation collection, and negative sample zones are retained.

For each collected key, look up the existing loaded zone. Only if its
**mesh dirty byte+0x23c is currently0**, write1 at **0xaf38c**, and write
the captured **unsigned32 timeGetTime value to+0x238 at0xaf393**. An already
dirty zone retains its previous timestamp. Missing/null zones are skipped.
This phase neither repeats the physical block change nor appends history.
It does not consume/remove World+0xc0. Later outgoing-group clearing is
separate; the Game frame's final clear is at0xadb0e.

This is a mesh request, not immediate GPU replacement. The mesh worker
**0x842d0** admits work for dirty zones (also allowing its separate zero-cost
condition), subject to its range/task constraints. **0xe9530** rebuilds pending
mesh groups and clears+0x23c under the mesh lock; publication installs them
after preparation. Existing active groups can remain in use meanwhile.
See [steam-scene-lighting.md§1](steam-scene-lighting.md) for that established
pending/active/bounds lifecycle.

The mesh job also reruns the independently specified lighting solve and
updates cached model-light samples+0x28, for both ordinary+0x18 models and
the separate+0x90 auxiliary list, as specified in worldgen-parity-gaps§9.
This does not mean that notice consumption reconstructs model resources
or automatically removes ordinary models. The support cleanup's direct
ordinary-vector erasure is specified in effect-objects§2.7.1.

## 5. Explicit limits and corrected earlier interpretations

There is no synchronous dirty write hidden in0x2a59f0/0x91160, the history
append helper, or the World-step history loop. The proved downstream dirty
producer is the later Game helper above. The headless host batch's history/
distribution sequence is not itself a rendered Game-frame cache pass.

This trace does not establish when auxiliary-list membership is rebuilt
after a support-model removal, promise a next-frame mesh-publication deadline,
or audit every non-destruction/full-setter caller. It establishes the
actual notice→history, received-application and dirty-request cut points;
those must not be collapsed into a generic always-reapply notice consumer.

The older native-saves open question about who fills Zone+0xd8 is resolved
by§2/§3. The older netcode-roles statement placing block-queue consumption
before the client World step is superseded by the callsite order in§3.
