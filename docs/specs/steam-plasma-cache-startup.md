# Steam ordinary GUI cache startup and engine-byte destruction gate

Independent native-only bounded supplement to
[surface caches §§1/6](steam-plasma-surface-caches.md). Sources are ordinary
graphics startup0x134cc0, constructors0x38e100/0x339b70, node destructor
0x32f0c0, common destructor0x366a10, engine teardown0x38e700/0x38e420/
0x33a440/0x33a860 and Widget construction/copy/attachment.
No engine, tests, exporter, owner data/captures or game execution was used.
The byte below is identified by address and behavior, without assigning
an unproved ownership name.

## 1. The game GUI engine initializes byte+0x08 to0

Ordinary graphics startup calls concrete GUI constructor **0x38e100 at
0x135121**. Its base call **0x38e12a →0x339b70** explicitly writes
**byte(engine+0x08)=0 at0x339bae**. The concrete constructor and reached
ordinary initialization do not replace that value in this bounded trace.

This is the engine byte referenced through a node's engine pointer
**node+0x08**. It is not a byte on the node's cached surface, and not the
engine option word+0x288. Widget-cache preparation's separate write
**byte(surface+0x08)=0** at0x32d8d0 does not change this engine byte.

## 2. Reached later write and exact node-destructor consequence

The concrete deleting-destructor route **0x38e700 →0x38e420** continues
to base teardown **0x33a440 at0x38e6ef**, then to **0x33a860 at0x33a7b4**.
At **0x33a871**,0x33a860 writes **byte(engine+0x08)=1** before visiting
the registered-object list at engine+0x10. It visits that list forward,
invokes each nonnull object's deleting destructor, then releases list
entries/head. This is a reached teardown write, not a startup cache setting.

Node destructor **0x32f0c0** reads its nonnull retained surface **+0x88**,
then tests **byte(node.engine+0x08)==0 at0x32f0f6**. Only that zero case
calls **0x33dfa0 at0x32f0fc**, which invokes the surface's deleting
destructor. A nonzero byte skips this node-surface destruction call.
The ordinary initialized engine therefore admits the zero branch when
such a node is independently destroyed before another writer changes
the byte. Nodes reached after the verified teardown write see1 instead.

Common registered-object destructor **0x366a10** tests the same engine
byte: with0 it unlinks/frees that object's registration entry and decrements
the engine's list count; with nonzero it skips that unlink. This is a
second concrete consumer of the byte, not proof of a broader semantic name.

These facts do not establish another path that frees a skipped node+0x88
surface during whole-engine teardown. Do not replace the nonzero branch
with an invented per-node destruction or assume another cleanup merely
from the list traversal. Widget-surface destruction remains its separate
0x329500 route in the existing cache contract.

The directly identified engine-byte writers in this bounded ordinary
constructor/initialization/teardown trace are the0 and1 writes above.
Other numeric+0x08 writes on surfaces or library result/collection records
are different receivers. Untraced aliases, bulk/external writes, alternate
engine construction/reset paths and failure/concurrent entry remain limits;
this is not a proof that every runtime writer has been excluded.

## 3. Directly reached cache scalar defaults and attachment distinction

Fresh base Widget construction **0x328ff0** supplies:

| Widget field | Constructor value |
|---|---|
|+0x40 float cache scale|1|
|+0x44/+0x48 float cache offsets|0,0|
|+0x4c signed32 divisor|1|
|+0x58 retained surface|null|
|+0x150 flags|0|
|+0x15c dirty byte|0|

Actual Widget copy **0x329210** copies the source's four scale/offset/
divisor scalar fields while starting with a null retained surface and
dirty0, as already specified. This is distinct from attaching a newly
constructed derived Widget to an already bound node:
**0x334410 →0x329c70** transfers the old frame/margin/bind quantities,
but0x329c70 does not write the four cache scalar fields at+0x40..+0x4c.
That geometric transfer alone does not inherit an old Widget's cache scale,
offsets or divisor. Later actual setters/copies/derived constructors are
not erased by this constructor-default statement.

Fresh node construction **0x32ee60** starts node flags+0x80 at0,
changed+0x84 at0 and retained surface+0x88 null. Node-cache geometry and
its separate negative-scale final copy remain the existing cache contract;
the Widget scalar layout must not be applied to node fields with the same
numeric offsets. GUI option+0x288 initially7 permits Widget caching but
does not itself set Widget flag0x40 or node flag0x400.

This supplement establishes these direct ordinary defaults and the engine
byte's reached lifetime boundary. It does not add a census of all later
cache scalar setters, owner template values, alternate surface cleanup or
automatic DPI/GUI-scale rewriting of those scalar fields.
