# Steam WORLD point-table state lifetime

Independent native-only audit of the ordinary renderer and its directly reached
sky, overlay, map and GUI paths. Sources are the original executable and native
decompilation; no engine, tests, exporter, owner data or game execution was used.
This closes the incoming-state boundary left open in
[queued WORLD state §3](steam-queued-world-state-order.md#3-state-immediately-before-and-after0xe6270-entry).
Collection/source arithmetic remains [scene lighting §3](steam-scene-lighting.md#3-point-light-collector-common-records-and-selection).

## 1. One shader cache and one GPU buffer

The concrete renderer owns the WORLD shader object at **renderer+0x558**:
constructor **0xc3150**, shader construction **0xc5ac0/0xc60d0**. Renderer
virtual+0xd0 reaches **0xc5a40**, which forwards to **0xc7010** with that
same shader object. Ordinary world draws, the late screen-overlay context,
the reached 3D map and the reached direct GUI model paths use this state;
they do not obtain independent point buffers merely by changing projection,
pipeline state or descriptor context.

Relative to the shader object, the CPU comparison cache is **16 float4
position/radius entries at+0x1c8**, followed by **16 float4 color entries
at+0x2c8**. The GPU buffer pointer is **+0x410**. Creation at
**0xc63b8–0xc63d2** requests **320 bytes**, with **no initial-data pointer**.
The GPU payload consists of the first10 position/radius float4s followed by
the first10 color float4s. Binding **0xc68c0** installs that same buffer at
**WORLD vertex-shader constant-buffer slot4**; binding is not an upload or
table reset.

**0xc7010** compares all16 entries of both submitted tables against the
CPU cache, component by component as floats, not by zone identity, pointer
identity or raw-byte equality. If all comparisons match, it returns without
uploading. On a difference, it copies both complete16-entry tables into the
CPU cache, constructs the320-byte10-plus10 payload, and updates the GPU
buffer at **0xc7210**. Changing only a cached entry10–15 can consequently
cause another upload of the same first10 entries. Equal positive/negative
zero components do not themselves force a byte replacement.

## 2. Construction does not establish a zero GPU default

After buffer creation, **0xc6514** calls **0xc6550**. Its two16-entry loops
at **0xc6750–0xc6786** initialize **every CPU cached float4 to(-1,0,0,0)**,
for positions and colors alike. Those are comparison-cache initial values,
not a default light table submitted to the GPU. This initializer contains
no point-buffer upload. The reached constructor does not supply initial
point-buffer contents or call0xc7010 afterward.

Thus no numeric first GPU point-table default is established by this
construction trace. In particular, neither all-zero tables nor the cache's
(-1,0,0,0) entries are proved initial GPU contents. This statement does not
assign a portable value to storage for which native creation supplies none.

The explicit clear **0xce580** constructs one256-byte all-zero table and
passes it as both position and color inputs through renderer virtual+0xd0.
Its first call after the verified cache initialization necessarily differs
from that cache and uploads zeros. Later calls may elide the upload when
the comparison cache already matches zeros; their established result is
still a zero table, under the ordinary valid-buffer lifetime.

## 3. Early sky submission versus current-scene collection

Ordinary scene **0xe2e00** does not begin by clearing point tables. Its
camera/view/projection, directional palette, scalar and material setters
do not themselves upload points. There is, however, an important earlier
point setter before **0xd3170** and **0xce5f0**:

* When the scene's **drawFlags bit0x80 is clear**, the call at **0xe3ad4**
  invokes the sky-quad path **0xc5120**. After selecting state8, this path
  passes two256-byte stack regions to0xc7010 at **0xc516b**. The reached
  native instructions do **not initialize their table components** before
  that call. This is not0xce580 and not an all-zero initializer. The
  ordinary comparison/upload behavior of§1 applies to these indeterminate
  inputs. They have no established numeric/default light meaning.
* When **bit0x80 is set**, that sky call is skipped. The existing shader
  point contents survive this part of scene setup until another setter
  actually changes them.

This distinguishes the sky quad's table side effect from whether its
particular shader needs those values to produce the sky image. A known
zero at the end of the preceding frame does **not** prove zero at the
current terrain/effect entry when the early sky-table call runs.

**0xd3170** clears current considered meshed zones' CPU point tables/counts;
**0xce5f0** collects/ranks lights and populates per-zone tables. Neither
operation alone binds or uploads those tables to the shader. Terrain
**0xe5070**, at **0xe5166**, binds a considered zone's tables only when its
mesh-group count+0x1d0 is nonzero. Ordinary opaque/faded world consumers
bind per-descriptor tables as already specified, including their different
readiness ordering. If none of those consumers binds a table, their incoming
shader state remains; collection of a new light or recalculation of a zone
table is insufficient to change it.

## 4. Late overlays and GUI share the state, and clear it

Within every completed ordinary **0xe2e00** call, after the world/water and
late world work, **0xe4e8f** unconditionally invokes0xce580. It then consumes
the screen-overlay context through **0xe6e60 at0xe4f10** and **0xe7980
at0xe4f22**, both with mode1, before GUI traversal through **0xc4af0 →
0x33e170**.

The opaque consumer itself tests its mode bit2 and calls0xce580 at
**0xe6ecf** when that bit is clear, **before inspecting whether any opaque
group exists or is ready**. Mode1 therefore clears even an empty overlay
context. Its mode1 descriptors do not select world-zone point tables.
The following mode1 faded consumer likewise does not select world-zone
tables. Consequently the late ordinary overlay portion establishes zero
points even when it draws no models.

The directly reached later map/card/item paths confirm shared clearing,
rather than restoration of the preceding scene zone:

| Path | Verified explicit clear | Following queue mode / scope |
|---|---|---|
| 3D map0xe64b0 |0xe6699 | Map-context opaque/faded consumers at0xe67df/0xe67f1 use mode0; opaque entry also clears; neither mode0 consumer binds world-zone tables |
| Character card0xdb630 |0xdb856 | Local preview setup clears before its local resource-tree draws |
| Shared GUI item0xddbc0 |0xde276 | Main colored image setup clears; the earlier optional dark pass inherits the incoming state until that setup is reached |
| Raw GUI sprite0xdf230 |0xdf672 | Its reached main model setup explicitly clears |

These are setters of the same renderer shader state in§1, not private
map/card/icon point tables. A skipped callback or an earlier resource
return need not run its own clear; that does not undo the already reached
late ordinary scene clear. Ordinary pipeline/material/view changes do not
implicitly restore the earlier nonzero world table.

The bounded ordinary overlay and listed GUI paths therefore do not carry
the last scene zone's nonzero point table into the next scene. This does
not eliminate§3's next-frame sky overwrite, nor prove behavior for an
untraced external draw, another renderer instance, device recreation,
failed GPU update, or an interrupted/incomplete scene call.

## 5. Coordinate meaning when no setter runs

Per-zone positions already contain the renderer's then-current fixed
X/Y origin translation **+0x428/+0x430**, followed by fixed-to-float block
conversion; their radius is position.w and color.w is zero. See scene
lighting§3.2. The uploaded point coordinates are ordinary float values in
that rendered coordinate space, not fixed world positions accompanied by
an origin/zone identifier in the GPU point buffer.

Installing another view, projection, renderer origin or model WORLD matrix
does **not** automatically rebase the cached/uploaded point coordinates.
If no point setter runs, the old numeric buffer values remain while the
new model/view inputs change. Rebuilding a CPU zone table at a new origin
affects the shader only when a consumer actually submits it through0xc7010;
there is no persistent selected-zone-key refresh in this shader cache.

This is a conditional state-lifetime fact, not a claim that every frame
retains its preceding world table:§§3–4 give the reached early overwrite
and late zeroing stages. Uninitialized sky-table components, initial GPU
storage, synchronization, invalid resources and untraced alternate callers
remain unconstrained rather than assigned replacement numeric values.
