# Steam world-generation parity audit

Independent clean-room specification audit, 2026-10-29. Native sources and existing
specifications were inspected; engine sources/tests and third-party reconstructions
were not. No original game execution. The reported fixture results are evidence
from the primary implementer, not new tests run by this author.

## 1. Priority finding: street-lamp turns are undefined but consumed

Sources: **0x2ff6f0, 0x315fd8–0x316270** (street lamps), **0x2f93a0**
(object defaults), **0x13a520 / 0x13a960** (size), **0x31b2d0 / 0x31d700**
(settling/marking), and **0xe0ac0** (object drawing).

Decimal object types **63 = 0x3f** and **65 = 0x41** occur in the street-lamp
placement pass described in `worldgen-structures-buildings.md` §13. That placement
constructs a temporary object, selects its type, writes its position, settles it
and appends it when settling succeeds. It **does not write turn byte +0x20**.
The constructor also does not initialize that byte. It initializes scale **+0x24
to 1**, active **+0x28 to 1**, animation **+0x2c to 0**, owner **+0x30 to 0**, and
an empty item. The lamp turn is therefore prior stack data, not a specified zero,
random quarter turn, or another deterministic output of generation.

This explains why the reported same-position type-65 lamp has turn **48** in one
capture and **191** in later captures, and why four type-63 lamps carry 191.
Those observations corroborate the initialization trace; they are not by
themselves proof that the byte is unused.

**Consumers do read it:**

* Settling and occupancy marking test **turn bit 0** to swap footprint x/y.
  Scale-one sizes are **(2,2,8)** for type 63 and **(1,1,4)** for type 65.
  Swapping their square footprints changes neither dimension.
* The generic object renderer **0xe0ac0** includes both types in its normal
  model path and uses the **entire unsigned turn byte multiplied by 90 degrees**
  in its sine/cosine rotation. There is **no modulo-four mask before that
  calculation**. The byte is consequently an input to rendering. This audit
  has not established visual rotational symmetry of the actual lamp assets,
  and does not claim that their rendered orientation is unaffected.

Classify the **street-lamp producer's +0x20 only** as **uninitialized and consumed**.
It cannot have a deterministic byte-equality generation oracle across independent
native runs. Preserve/report the raw captured value separately; do not describe
it as padding, mask all object turns, or call it proven behaviorally irrelevant.
All initialized placement fields and all other producers' turn bytes remain
eligible for strict comparison. Reproducing one recorded lamp's rendering would
require retaining that captured byte; generating zero is a deterministic engine
policy, not a native guaranteed value.

## 2. Model size/yaw audit: checkpoint, consumer trace unfinished

Sources inspected so far: **0x264670 / 0x263210 / 0x2626e0 / 0x263ea0** (tree
markers), **0x69b30** (model append), and **0xd3170** (drawing).

For the ordinary tree producers, leaf marker **0x4d** and fruit markers
**0x4e–0x50** do not initialize model size **+0x20**. They explicitly write yaw
**+0x24 = 0**, position, tint and flags. Thus the known unwritten-size exception
must **not** be broadened to yaw or other model types. Model append copies both
fields; copying an unwritten field does not make its value initialized.

The renderer normally consumes a drawable model's size and yaw. Its lookup can
also return no model. `vegetation-wind.md` §3.2 identifies tree marker types
0x4c–0x50 as null model-table entries; the exact loader/table and downstream
consumer trace is **not finished at this checkpoint**. No new claim that fruit
size is universally unused, or that all model yaw/size fields may be ignored,
is established here. Complete that trace before defining further fixture masks.

## 3. Scope of currently reported evidence

The primary reports 789 snapshots across gen_seed_123 (125), gen2 (122), genb
(271) and genr (271), with the specified lamp-turn differences. These are
**snapshots**, not necessarily 789 distinct coordinates or generation contexts.
Earlier six-zone final-stage counts were objects **3/3**, models **442/442**,
items **13/13**, spawn records **25/25**. Matching those totals is not a substitute
for initialized-field comparisons or dungeon/theme coverage. The primary also
reported that the full-stage command lacked hard object/model-equality failure
checks. The subsequent implementation checkpoint added opt-in
`WC_STRICT_RECORDS=1` checks for object keys, model counts/positions and item
keys/angles. The 789-snapshot run now reports three failing dataset checks for
the lamp-turn record differences. This does not complete model-transform coverage.

The broader dungeon/building, theme-coverage and generation-stage audit is
**in progress**. Intermediate terrain/structure stages must not be compared as
though they were the completed native generation stage.
