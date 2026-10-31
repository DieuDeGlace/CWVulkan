# Steam weapon-customization palette: stable source/consumption checkpoint

Independent native-only facts from draw **0x292990**, click consumer
**0x292650**, item equality **0x4df60**, and item copy/placement helpers.
No engine, tests, exporter/build, SDK, owner content or capture was read;
no game was executed. This checkpoint closes palette production and
its consumption mismatch. The broader main edit view, controls and
root geometry remain a separate incomplete audit, not claimed here.

## 1. Rebuild, inputs and ordered output

Each reached0x292990 draw resets its temporary stack-vector live end
**Widget+0x200 to begin+0x1f8**, retaining allocation capacity+0x208.
This is a display palette, not canonical inventory insertion. Resolve
the selected nonempty equipped weapon through the draw's existing
selection/fallback path. Without one, the material build is skipped
and the vector remains empty. Weapon selection0 means creature+0x6d8,
selection1 means+0x638; the weapon's material is its original byte+0x1c.

Palette records have signed32 quantity first, then the original160-byte
item at+4, stride **0xa4**. In this exact order:

1. Make a **semantic-default item**, then set category11, subtype10
   and material to the selected weapon's material byte. Across every
   canonical tab in order and every physical slot in order, compare
   that candidate with the actual item using **0x4df60**. Sum quantities
   of equal slots. If the final sum is nonzero, append this one default-
   based candidate with that sum.
2. For material bytes **128,129,130,131 only**, in ascending order,
   scan every canonical tab/physical slot again. Match **only category11,
   subtype14 and that material byte**. Sum the matching quantities;
   append when the final sum is nonzero, with a new semantic-default
   item whose only nonzero fields are category11, subtype14 and material.

The native subtype14 loop starts with **integer128 at0x293432**, advances
at0x29361b and continues only while **less than132** at0x29361d–0x293623.
It is **not** material0 through131. A normal complete palette therefore
contains at most five records: one optional subtype10 plus at most four
subtype14 entries. No deduplication between those different subtypes is
performed, even if their material bytes happen to coincide.

Semantic-default means category/subtype/modifier/region/rarity/wrapped-
type/material, all32 attribute entries and attribute count initialized0,
before the listed assignments. Padding is not part of this contract.
No stored record donates its modifier, region, rarity, formula field,
attributes or attribute count to an aggregate output. These initializations
precede0x293425's semantic copy and the subtype14 append path's
0x29360e semantic copy.

Both sums are **signed32 accumulation with32-bit wrap**, adding each
matching stored quantity as-is. There is no positive-quantity filter,
no quantity!=0 prefilter and no treatment of−1 as infinite. Zero slots
add0; negative slots reduce or cancel positive totals. Only the final
**sum!=0** test decides append. A negative aggregate is retained rather
than clamped or rejected. These scans include saved extra tabs and
physical holes; they do not compact/reorder canonical slots or call
the inventory refresh/sort merely to produce the palette.

## 2. Exact equality versus field-only aggregation

The subtype10 candidate uses the full semantic equality already
specified in **steam-dialogue-item-actions4.1.1**.0x4df60 compares
category, all32 subtype bits, modifier, both region words, all32
wrapped-type bits, material, attribute count and rarity. With a count
of0, no attribute payload bytes participate; unused entries and item
padding do not make an otherwise equal record different. No160-byte
memcmp is involved.

Subtype14 production does **not** use that helper. For example,
category11/subtype14/material128 records in different regions, with
different modifier/rarity/formula/count/attributes, all contribute to
the same material128 total. Their aggregate output still has zero
region/modifier/rarity/formula/count and zero attributes. This field-
only sum does not prove those heterogeneous records are interchangeable
for every later native item operation.

## 3. Placement consumes through full equality, not the aggregation key

The later **0x292650** click consumer uses the selected palette record
from the most recent draw. After its valid palette-index/weapon/count
admission, it first attempts attribute placement at **0x292743**, using
that candidate's material. Then it scans canonical tabs/physical slots
in order. Only a slot with **quantity!=0 and full0x4df60 equality to
the selected palette payload** can supply the one-unit decrement.
The first qualifying slot loses1; if below1 it clears quantity,
category and subtype only, and returns. This helper has no inventory
refresh. Detailed cap/replacement ordering is slot-lifecycle3.4 and
loot-crafting4.

Consequently the subtype14 displayed sum can include records that
the consumption search will never accept. If all contributing records
have a nonzero region, modifier, rarity, wrapped-type or active
attribute count, none equals the zero-based candidate. Placement was
already attempted; no match produces **no source decrement and no
rollback**. If a matching default-based stack also exists, it supplies
the decrement in canonical order even though nonmatching stacks
contributed to the displayed total. A matching negative quantity also
qualifies and is decremented/cleared; no infinite-stack exception is
introduced by the aggregate's signed total.

Inactive attribute bytes with count0, and padding, are the equality
exceptions in section2: they do not by themselves prevent consumption.
Do not broaden consumption to category/subtype/material merely to
make it agree with the display aggregation. Do not silently replace
the displayed sum with the stricter consumable subset either.

This click uses the retained palette state;0x292650 does not rebuild
the sum or retest the selected candidate's quantity before placement.
A collection change between draw and click is therefore not repaired
by this helper. No asset, speculative grant, refund or new cursor
interaction is implied by these native operations.
