# Steam zone selected object-index list

Stable bounded native audit, 2026-10-31. This supplements
[steam-resident-effects-prerequisites.md §§1–2](steam-resident-effects-prerequisites.md)
with the producer and lifetime of **Zone+0x4a8**. Sources are the original
native constructor/destructor, zone loader, encounter dispatcher/selector,
**0x123210**, and reached list helpers. No replacement source or owner capture
was consulted.

## 1. Layout and ownership

**Zone+0x4a8 is a doubly linked list header pointer; +0x4b0 is its 64-bit
element count.** Each 0x18-byte node has next/previous pointers at +0/+8 and a
**signed-32 object-vector index at +0x10**. The final four bytes are not part
of the stored index. Forward traversal starts at header.next and ends at the
header, as already established by the resident consumer.

Fresh **Zone constructor 0x2f9400** zeros the pointer/count and creates a
self-linked empty sentinel through **0x3c050 at 0x2f96a8**. The generic node
allocator 0x3c050 allocates 0x18 bytes; value construction **0x112f00** copies
one 32-bit index at node+0x10. The list owns these nodes, not the objects to
which their numeric indices refer.

## 2. Verified producer and ordinary reachability

The identified Zone population writer is **0x123210**, called by encounter
dispatcher **0x1274d0 at 0x127c67** for **encounter number24 (0x18)**. This is
an encounter number, not place-kind24: the ordinary selector maps place-kind24
to encounter27.

The original encounter selector **0x119a00** has no ordinary result24. Its
direct returns and candidate lists, independently specified in
[conversation-script.md §5.5](conversation-script.md#55-encounter-of-a-place-0x119a00),
do not contain24. The supported dispatcher therefore does not reach this
writer through ordinary valid generation inputs. The native helper exists
and its dormant behavior is specified below; no other reachable Zone+0x4a8
population writer was identified by this audit.

In particular, this list must not be reconstructed by scanning all static
objects, all nonzero-state objects, all doors, or even every object of the
type mentioned below. Fresh ordinary zones retain an empty list through the
examined generation path. A nonempty capture would require preservation of
its actual provenance rather than guessing membership from object types.

The old role table's combined “24,26 unreachable” entry was overbroad:
0x119a00 explicitly returns26 for planner role18. Normal generation of that
role is not newly audited here. The absence of result24 is the relevant
reachability fact for **0x123210** and this index list.

## 3. Exact producer selection and append order

The helper uses the place pair P, rounded for a big place, and H(P) as in
[worldgen-creatures-roles.md §1](worldgen-creatures-roles.md#1-entry). It builds
the already specified **Kind cells** candidate sequence: place weight strictly
greater than0.5 and **0x128aa0** fits-the-kind admission, in X-outer/Y-inner
order. An empty candidate sequence falls back to the landmark's 16-block cell.
Select one cell at **unsigned R(H(P)+0x4192) remainder candidateCount**. This
choice is not a search through the zone's existing object vector.

Only the zone containing that selected cell reaches the object/index producer.
The native signed-32 half-open bounds are formed from each wrapped32 zone
block origin (64×zone component), then floor division by16 of that origin and
of its wrapped32 origin+64. In ordinary coordinates this is the zone's usual
4×4 cell rectangle. Test minimum≤selectedCell<maximum on both axes. Other
zones do not append an index for this selected cell.

The produced object is **type0x58**, initially **state byte+0x28=1**. Its
position is obtained through **0x31c0e0(zone, selectedCellX, selectedCellY)**;
the resulting fixed tuple is copied to object+8/+0x10/+0x18. The object takes
the item returned by **0x11a6c0**, copied at object+0x38. Its byte+0xd8 is set
to10. The full dormant encounter's item/appearance/guard generation is outside
this bounded list audit; these fields identify which produced object the
index targets. The index producer does not subsequently filter by that
object's type, item contents, state or byte+0xd8.

The precise operations are:

1. Call **0x31b2d0(zone, object, 0)** at **0x123c21**, before reading the
   current vector length.
2. Calculate the current object count from signed64 **(Zone+0x68 end minus
   +0x60 begin) / 0xe0**, narrowing the quotient to signed32.
3. Append that numeric count to **Zone+0x4a8** through **0x129ae0 at
   0x123c57**. This is tail insertion; the helper increments +0x4b0. It does
   not clear, sort, deduplicate, or validate the index against the vector.
4. Only afterward call **0x1194b0(zone, object) at 0x123c63**. That helper
   appends the object to the +0x60 vector only if its actual fixed XY lies
   within the zone's half-open 64-block rectangle. Each lower block bound is
   wrapped32(64×zone component), each upper bound is wrapped32(lower+64);
   sign-extend each bound to64 and multiply by65536 before the fixed-position
   comparisons. There is no Z test. Its return value does not undo the
   already appended index.

Thus on a successful object append the index is its new zero-based slot.
The index insertion precedes the object's final position admission. If that
admission fails, the list still contains the old end index; it can be invalid
or later address another appended object. Do not silently filter, repair or
retarget it. The resident consumer's own validity/state/cell tests still apply.
Repeated explicit invocations append in invocation order; there is no
membership-set behavior in this helper. The ordinary encounter call produces
at most one such object/index per containing zone.

## 4. Matching spawn+0x928 producer and comparator

The same dormant helper creates **5+(rand remainder4)** associated hostile
guard spawn records, hence5 through8 with the CRT draw range. Each is placed
at the produced object's fixed tuple. For each guard, **0x745e0 at 0x123d75**
inserts **the selected signed-32 cell pair** into **spawn+0x928** before calling
its normal completion/registration helper **0x119530 at 0x123e04**.

This is the previously unidentified filling producer. The insertion helper
is the same independently traced signed-32 two-component tree insertion:
**lexicographic signed X, then signed Y**. Each newly constructed guard tree
starts empty and receives exactly that one pair here. No multiple-cell
filling producer is established by this audit. Consequently this producer's
one-cell case does not exercise the consumer's later-cell retained-byte quirk.

The later separate encounter leader uses prerequisite **IDs at +0x938**;
do not apply the guard's +0x928 cell insertion to every spawn in the zone.
This supplement establishes the cell/index provenance, not the entire
dormant encounter's guard equipment or random-draw sequence.

## 5. Loaded-zone lifetime and absence of automatic rebuilding

**0x2ae0c0** first queries World+0x460. A present nonnull loaded zone is
returned directly. Otherwise it constructs a fresh zone through **0x2f9400
at 0x2ae16a**, calls full generation **0x2ff6f0**, applies supported archived
state, and then publishes the zone in the loaded map. The selected-index list
is generation-owned zone state; normal repeated lookup does not reconstruct
it. Archived object-vector restoration in this loader does not rebuild the
index list or derive it from restored object types.

**Daily refresh 0x31c540** retains this list. Its examined operations remove
old daily spawn points, restore the base object vector through **0x29baf0**,
and generate daily content; they do not clear/populate +0x4a8 or re-run this
encounter writer. The entries remain numeric indices against the **current**
object vector, not retained pointers to pre-refresh objects. A vector restore
does not imply a list refresh.

The ordinary object state transition **0x2d3a10** changes state/swing and queues
its sound as already specified; it does not remove this index or rebuild the
list. The resident consumer reads the object's current state, so a retained
index whose object becomes state0 ceases to block through that state test.
The daily World actor/archive reset preserves loaded zones; it also does not
rebuild this list. See [steam-world-daily-reset.md](steam-world-daily-reset.md).

The Zone destructor **0x2f9940** resets this list's links/count, frees its value
nodes and sentinel, and does not delete objects through their numeric values.
A later fresh construction starts empty again. No per-frame, opening-UI,
object-state-change, or daily-refresh rebuild writer was found in the
examined native paths. This is not a claim about arbitrary injected code or
all possible memory-copy operations outside those paths.

## 6. Oracle requirements and retained limits

An object-vector dump alone cannot establish this selected-list membership.
For a nonempty native case, preserve **+0x4b0 count and signed-32 indices in
native forward-list order**, including duplicates and currently invalid
indices. Preserve the associated spawn's +0x928 pairs in native tree order,
the complete current object vector, and a phase label distinguishing initial
generation, archived-object restoration, and post-daily-refresh state.

The resident consumer then needs loaded-zone presence and each indexed
object's current state/fixed XY, as specified in the prerequisite supplement.
No existing owner-export contents were inspected here, and no claim is made
that a particular existing export already contains these auxiliary fields.
The verified dormant producer and comparator close the earlier source gap;
ordinary reachability beyond the original encounter selector and any
multiple-cell producer remain outside the established contract.
