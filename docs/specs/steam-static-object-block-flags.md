# Steam static-object coverage in stored block flag0x40

Stable bounded native-only contract, 2026-11-01. Sources: **0x31d700**,
the shared object construction/placement helpers, **0x2ff6f0 / 0x2ae0c0**,
the reached object-publication callers and existing physical block writers.
No implementation, tests, extractor, SDK, owner data/captures or game
execution were used. This specifies the six-byte block's **flags byte5,
bit0x40**, independently of unrelated object/model record flags.

## 1. Owning records and placement boundary

The marking input is the zone's ordinary static-object vector:
**begin Zone+0x60, end+0x68, stride0xe0**. It is not the ordinary0x40-byte
model vector at Zone+0x18. Relevant object fields are type32 at+0,
signed64 fixed XYZ at+8/+0x10/+0x18, turn byte+0x20, float scale+0x24
and raw state byte+0x28. Positions use65536 fixed units per block.

Constructor **0x2f93a0** establishes scale1 and state byte1 but does not
touch terrain. Shared center-inside placement **0x1194b0** calls append
**0x129b50**; that append copies the object record. Settling **0x31b2d0**
reads terrain and adjusts object Z. These reached shared operations do not
mark block0x40 themselves. The separate marking pass is **0x31d700**.
Their other placement/size rules remain worldgen-structures§4.1/C8.

## 2. Marking decisions and order

### 2.1 Clear tracked coverage first

Zone **+0xe8** owns a unique-coordinate tree, with signed32 XYZ keys.
Constructor0x2f9400 initializes it empty. In0x31d700, first visit its
keys in ascending signed lexicographic X/Y/Z order; the insertion
comparator is0x1152c0. For each key read the **effective** six-byte block
through **0xeb90 at0x31d756**. Only when its effective flags contain0x40,
clear that bit using mask **0xbf** and write the resulting six-byte tuple
through **0x31c1c0 at0x31d77f**. This stage does not clear0x04,0x80 or
other flag bits from the tuple being written.

**0x31d700 does not empty the coordinate tree.** It retains prior keys and
inserts new ones later. Across repeated calls without another tree reset,
it can revisit coordinates no longer covered by a current object. It does
not scan all zone blocks for existing0x40.

### 2.2 Then visit current objects

Visit the current object vector from begin to end, in physical record
order. Always skip types **0x5f,0x60,0x11,0x12,0x14,0x7d**. Also skip an
object whose state byte+0x28 is0 when its type is in
**{3,4,5,6,8,9,10,11,12,13,14,16}**, the bit set0x17f78. For that latter
test only zero versus nonzero matters. It is not a terrain-type test.
The swing clock+0x2c does not participate in this gate or the box; there
is no intermediate animated-door-pose test in0x31d700.

For each admitted object's inclusive box (§3), visit **X ascending outer,
Y ascending middle, Z ascending inner**. Read the effective block through
**0xeb90 at0x31daa2**. If its flags already contain0x40, do nothing for
that cell: no additional bits, no block write and no coordinate insertion.

Otherwise OR **0x40**, or **0xc0 for object type15**, into the effective
flags; additionally OR **0x04 for types3,4,5**. Write the resulting full
tuple through **0x31c1c0 at0x31dae6**, then insert signed32 XYZ into the
tree through **0x1152c0 at0x31db0e**. Insertion follows the write attempt
without testing whether the setter accepted it.

Consequently overlapping-object order can affect the extra0x04/0x80 bits:
on an initially unmarked effective cell, an ordinary object first can
suppress a later type15's0x80 addition, or a later type3–5's0x04 addition.
The clearing stage removes only0x40, so previously written0x04/0x80 can
remain after an object changes or disappears.

An already flagged cell not present in the coordinate tree is not cleared
by stage2.1. Even a current object's stage2.2 visit then skips inserting
it while0x40 remains set. This helper therefore is not a whole-zone
reconstruction from current object geometry alone.

## 3. Exact inclusive object box

At **0x31d887**, size helper **0x13a960** returns the fixed type extents
from0x13a520 multiplied componentwise by the object's float scale. The
existing extents remain in worldgen-structures-tables T8. If turn byte's
**low bit is1**, swap X/Y sizes, at0x31d88c–0x31d8a6. No rejection of
turn values above3, full-angle rotation, hinge transform or CUB-bound
measurement is added by this marking call.

Horizontal fixed half-extents are each size times float **0.49900001287460327**
(word0x3eff7cee at0x47391c), then times float **65536**, with two float
multiplications followed by signed64 truncation toward zero. Subtract/add
that fixed amount from the object's corresponding signed64 X/Y, using
low64 arithmetic, to obtain each lower/upper fixed bound.

Z's lower fixed bound is the object's Z. Its upper bound adds signed64
truncation of sizeZ times float **0.9990000128746033** (word0x3f7fbe77
at0x473350), then times float65536, again two float multiplications.

Each fixed bound uses this native block conversion: divide by65536 toward
zero, narrow the quotient to signed32, and, when the **original fixed
value is negative** and reconstructing that narrowed quotient times65536
does not equal it, decrement in wrapping32-bit arithmetic. Native examples
of the narrowing/reconstruction are **0x31d8cb–0x31d8e8** and
**0x31d906–0x31d921**. This is the already specified0x137e0 block-conversion
rule, including narrowing before the negative reconstruction; there is no
16-block-cell division here. The bounds and coordinate keys are signed32.

All six converted endpoints are inclusive. The ordinary finite contract
assumes representable/allocatable spans and nonwrapping loop increments;
this audit does not add recovery for enormous or invalid object extents.

## 4. Effective records, column growth and retained state

Both passes read through the **zone-local effective accessor0xeb90**, not
a raw byte reference or a World neighbor lookup. For an ordinary stored
cell that is returned unchanged, marking preserves its other five bytes
and other flags. Effective accessor substitutions can instead supply
implicit air, water or buried stone; the full tuple from that accessor
is what gets modified and written.

For example, above a positive-Z stored column top, effective air with
flags0 becomes air with0x40. The full setter admits its nonzero flags and
can extend the column. At/below sea level, an effective canonical-water
read becomes **water with0x40**; it is not a raw type0 request that loses
its flags to the full setter's type0 sea substitution. Below the column
base, effective buried stone can similarly be materialized. Downward
interior gaps retain the full setter's separate generated-fill behavior.
See [full block setter](steam-full-block-setter.md).

The box loops do not first clamp XY to the zone.0x31c1c0 only admits that
zone's own64×64 horizontal bounds. Out-of-zone attempts therefore do not
write a neighboring zone, though their coordinates can still enter this
zone's tracking tree after the rejected attempt. No cross-zone object
coverage is implicitly synthesized by this helper.
Whether a particular builder supplies another object record to an
adjacent zone is outside this bounded ownership trace.

Clearing old coverage does not trim a column or erase previously allocated
air slots. Neither this pass nor the full setter appends a block-change
notice/history entry, creates a new object record or independently requests
a mesh/light rebuild. The caller's object/publication state is separate.

## 5. Proven phase and publication order

The resolved direct callers of0x31d700 are these three sites:

| Phase | Reached order |
| --- | --- |
| Fresh full generation0x2ff6f0 | Final auxiliary lists0x31d470 at0x318f63, then marking at **0x318f6c**. This follows object generation, backup copies and item region re-stamping. Intermediate terrain/structure stages precede this final marking. |
| Received whole-object-list application in Game0xa5a50 | For an existing loaded zone, replace current Zone+0x60 records from the received list, insert that Zone into World+0x1a0, rebuild auxiliary lists0x31d470, then mark at **0xa83c9**. Missing zones are skipped. |
| Object-zone publication0x2e30b0 | First walk pending World+0x1a0 and copy its current object lists into World+0xf0 output records while setting Zone+0xfa. Then walk that pending set again and mark at **0x2e33e6**. Only afterward clear the pending ground/object sets, including World+0x1a0. |

The ordinary Game publication call is **0x2e30b0 at0xa8498**. Therefore a
zone replaced by the received whole-object-list path can be marked there
immediately and again in the later same-frame pending-set publication.
The latter does not mean all loaded zones are marked every frame: its
input is the pending set. Publication's object snapshot precedes its
marking; marking itself does not append corresponding block notices.

Saved-state restoration is a separate boundary. Zone creation calls fresh
generation at **0x2ae179**. On the admitted archived-state branch, it can
subsequently copy ground items at0x2ae279, replace current static objects
at **0x2ae293**, then replay saved six-byte after-values through0x31c1c0
at **0x2ae2d8**. That branch has no additional direct0x31d700 call before
zone publication. The generated tracking tree is not rebuilt by these
object/history copies. A later marking call has the tracked-set/current-
object semantics above.

The host path **0x83e60 → 0xf5520** directly calls the same publication
helper **0x2e30b0 at0xf6947**, with its embedded **World at Host+8**.
It runs once in the reached host-frame common tail, after all World
substeps: the16-ms loop calls0x2da6c0 at **0xf6917**, and the final
residual call is **0xf692d**. Next the single full-capped-host-interval
drawn/linked pass0x2b68e0 runs at **0xf693e**, then publication at0xf6947.
The host subsequently scans queued zero-sleep notices, which can clear
guests' known-creature update caches, before update/event distribution
**0xf33b0 at0xf69b4**. Thus the pending copy→mark→clear work is after
drawn placement and before distribution, not per World substep.

This common-tail publication has no renderer, client-count or positive-
elapsed gate. An admitted0-ms host frame still performs its final World
step0, drawn pass0 and publication. The pending-set membership remains
the marking admission inside0x2e30b0; headless operation does not omit
that consumer. See [host tick](netcode-roles.md#2-the-host-tick-0xf5520).

The remaining limits are the complete producer set for pending World+0x1a0,
whether a particular load/change enqueues a zone before a given publication,
and the adjacent-zone duplicate-object ownership noted in§4. The reached
host common-tail consumer and its ordering are established above.

## 6. Later physical writes and lighting

| Reached writer | Effect on stored0x40 |
| --- | --- |
| Full setter0x31c1c0, including loaded wrapper0xa37d0 and saved replay | Replaces with the supplied six-byte flags, with no merge of old0x40. The documented type0/Z<=0/flag0x02-clear whole-block sea substitution yields canonical flags0. A supplied nonzero type with0x40 retains it. |
| Column writer0xa36e0 / growth0x6d030 | Copies supplied six bytes; retained old cells keep their bytes. New raw gap slots start flags0. Neither derives static-object coverage. |
| In-column removal0x2a59f0, including the reached guarded-removal route | Writes the canonical empty tuple above sea level or water below Z1, whose flags are0; it does not retain old0x40 or extend a column. Its admission remains the existing removal contract. |
| Mesh-light solver0x2a3260 writeback at0x2a3d95 | For raw type0/2 cells, changes red and carries the other five bytes from the raw record read, including flags. It does not calculate/reassert0x40. |

These writes do not themselves call0x31d700. Their history/dirty ownership
remains [block-change lifecycle](steam-block-change-lifecycle.md), and the
solver's publication/locking limits remain
[block-light publication](steam-block-light-publication.md). A carried
flags byte is not an atomic merge with another intervening writer.

This separates fresh final generation, saved restoration, later object
re-marking, physical mutations and meshing. It does not identify the phase
of any particular owner capture or diagnose a replacement's mismatch.
The audit covers these reached shared producers/writers; it is not an
exhaustive census of every game routine capable of supplying a block tuple.
