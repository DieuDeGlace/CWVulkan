# Steam burst particle consumers: styles0/3/4/6/7/8

Independent clean-room native facts from updater0xb37f0, renderer0xe6840,
catalog initializer0x177f50 and executable constants. No implementation,
owner assets/captures or game execution were used. This supplements
[ordinary particle bursts](steam-particle-bursts.md); its record layout,
expansion, frame placement, shared cap and ordinary transforms remain binding.
Addresses are RVAs and particle offsets are payload-relative, excluding the
linked-list node's0x10-byte header.

## 1. Shared admission, age and ordinary burst state

Expansion does not reject these six style numbers. Each receives the already
specified12 base CRT draws, original RGBA/size, identity extra matrix, age0,
collision byte+0xb0=0, freeze byte+0xb1=0 and lifetime+0xb4=4000ms. The
style1/5 extra draw does not apply to them. This establishes their consumer
behavior for such input records; it is not an audit of their independent
native award/effect/burst producers.

All share the live list and its1000-entry cap, applied before the visit.
Age receives the signed32 frame-ms addition even for style3 or a frozen
record. Motion requires resulting age>=0, freeze byte0 and style!=3.
Ordinary expiry remains strictly age>lifetime, with deferred unlink after
the visit, not an age>=4000 test. The updater does not rewrite these styles'
lifetime, RGBA, size, collision flag, freeze flag or extra matrix.

Let seconds be the separately rounded float32 product of float32(frame-ms)
and0.001f, word0x3a83126f. The first angular advance means each stored angle
receives its angular velocity×seconds, with separate float32 multiply/add
and no angle wrapping, at0xb3948–0xb3988. The ordinary collision-off route
advances angles again at0xb3d3e onward. Two advances are separate rounded
operations, not one doubled increment.

| Style | Gravity before movement | First angular advance | Unmodified burst's motion and total angular advances |
| --- | --- | --- | --- |
| 0 | Subtract seconds×30.0f from velocityZ | Yes | Free integration; two advances |
| 3 | None | No | No position/velocity/angle update; zero advances |
| 4 | None | Yes | Free integration; two advances |
| 6 | Subtract seconds×30.0f from velocityZ | Yes | Updated velocityZ>=0: free integration and two advances. Updated velocityZ<0: §3 and one advance |
| 7 | None | No | Free integration; one advance in the collision-off route |
| 8 | None | Yes | Free integration; two advances |

The gravity constant is30.0f, word0x41f00000. Free integration is the shared
route in burst §5.1: component velocity×seconds, then×65536, each multiply
rounded to float32, truncation toward zero to signed64 and low64 addition
to fixed XYZ. It performs no terrain query. Style3's motion exclusion
applies regardless of its collision byte; its age and final expiry still run.
No CRT draws are introduced by these update or draw paths.

## 2. Conditional collision-on route

An unmodified burst from the six styles above has collision0 and does not
enter this generic route. It is nevertheless a reached consumer route for
a supplied record with collision nonzero; the known body-fragment producer
supplies such style0 records, as specified in
[body particles](steam-body-voxel-particles.md).

For an admitted motion visit other than descending style6, collision nonzero
selects the shared axis-at-a-time point stepping from burst §5.2. Its cached
step vector, maximum10 steps, X/Y/Z order, queries even for zero components,
axis rollback and velocity clearing are unchanged. There is no second
angular advance on this route. Consequently style7 spins zero times here;
styles0/4/8 and ascending/nonnegative style6 spin once before the steps.

The effective block query is0xe9a0 at0xb3c77. Styles0/6/7/8 pass only
types0/2. Style4 additionally passes types7,8 and11, so its complete pass
set is0,2,7,8,11. Type12 is explicitly excluded from its extra admission.
This test does not add a flags0x40 rule. Style3 cannot reach this route
because its motion gate has already failed.

## 3. Descending style6

After gravity and the first angular advance, finite updated velocityZ<0
selects a distinct route, irrespective of the collision byte. This is
therefore reachable by an ordinary collision0 style6 burst. VelocityZ==0,
including negative zero, selects the ordinary collision/free route instead.

Compute float32 displacement XYZ as updated velocity×seconds. The native
length sums squared Y and squared X, then squared Z, before sqrtf. The
step count is trunc32(length)+1, capped above at10. For ordinary finite
representable input it is at least1. Compute float32 reciprocal1/count.
For each step, advance all three fixed coordinates by the separately
rounded displacement-component×reciprocal×65536, truncating toward zero
to signed64 and adding in low64 arithmetic. Only after all three
coordinates advance does the route query its current full fixed XYZ.

The query is0xeab0 at0xb3a7b. Any effective type other than0 queues this
particle for removal at0xb3aab and ends its step loop. Water2 therefore
retires it, as do foliage/other nonair types; flags0x40 do not supply an
exception. The missing-zone default solid also retires it. Effective-column
air/water substitution is still the ordinary getter's behavior. The route
does not restore the last position, zero velocity/spin, freeze the record,
change age/lifetime or create another particle. It is not the generic
axis-by-axis collision route.

A zero-ms/zero-displacement admitted descending visit still has one step
and queries its current location. If all tested points are air, every step
completes. There is no additional second angular advance in this descending
route, including when no hit occurs.

The final strict age-expiry test remains independent of the hit test and
queues through0x6eb60 at0xb3db0. Both enqueue sites target the same deferred
list. No deduplication is present in this path; stable behavior after a
duplicate erasure of the same entry has not been established here. This
supplement does not assign recovery semantics to a record that hits and
also exceeds its lifetime in that visit.

## 4. Render controls, lifetime scaling and visibility

All use the same forward live-list walk, retained0xe08c0 sphere admission,
WORLD shader state2 and pre-water placement from burst §6. The tested
sphere radius is the unmodified stored size+0xac, even for styles7/8's
shrinking draw geometry. No age test or additional zone-mesh readiness
test is introduced by these styles.

Material RGB is stored RGB times the frame RGB+0x530/+0x534/+0x538, with
separate float32 products as in burst §6. Define the lit fourth component
as stored alpha×frame alpha+0x53c, then×max(0.2f, float32(channel0-light)/255.0f).
The light query is0xdab20 at0xe6999; see
[block-light publication §3](steam-block-light-publication.md#3-exact-channel0-query),
including the byte3 auxiliary source rather than flags byte5.

| Style | Fourth material component | Glow | Shininess | Catalog slot | Age scale before stored size |
| --- | --- | --- | --- | --- | --- |
| 0 | Lit component | 0 | 0 | 0xa46 | 1 |
| 3 | Explicit1 | 1 | 0 | 0xa46 | 1 |
| 4 | Lit component | 0 | 0 | 0xa46 | 1 |
| 6 | Lit component | 0 | 0 | 0xa46 | 1 |
| 7 | Explicit1 | 1 | 1 | 0xe2d | Float32(1−float32(age)/float32(lifetime)), then×0.03f |
| 8 | Explicit1 | 1 | 0 | 0xa46 | Float32(1−float32(age)/float32(lifetime)) |

The ratio uses separately converted signed32 age and lifetime, float32
division and subtraction from1.0f. Style7 additionally multiplies by
0.03f, word0x3cf5c28f at0x42e6ec. Finally multiply this factor by stored
size, separately rounded to float32, to obtain the model's scalar scale.
The native8 branch is0xe6a83–0xe6ac0; the native7 branch is
0xe6ac2–0xe6b0b. **Shininess1 is set for7 at0xe6af9, not for8.**
Shader submissions remain material0xe6b1b, Glow0xe6b2b and
Shininess0xe6b38. These controls are assigned for each admitted record,
so7 does not leave Shininess1 for the following particle.

For ordinary lifetime4000,7 starts with factor0.03f and8 with factor1;
both factors reach0 at age4000. They shrink geometry rather than fading
stored alpha or separate pixel opacity. Age4000 still survives the updater;
there is no extra CPU rejection at zero scale. Styles0/3/4/6 have no
age-dependent geometry scale or alpha fade in this renderer.

White is0 at entry. All these records retain the ordinary disabled-alpha-
blend, depth-test/write state2 pipeline. There is no style-specific blended
pass, depth clear, private viewport, billboard or separate shader. Point-light
tables, scene view/projection, sky/palette, separate pixel Opacity and
DepthOffset remain inherited exactly as in the established shared draw.

## 5. Geometry/resource provenance and model transform

Catalog0xa46 is the file-backed cube.cub already traced in burst §7.1.
Style7's different source is **catalog0xe2d, file-backed note.cub**:

| Provenance | Native evidence |
| --- | --- |
| Filename | Embedded ASCII filename note.cub at0x461b40, referenced0x1d8e63 |
| Path construction |0x6dec0 at0x1d8e7a joins the common catalog asset prefix and this filename |
| Slot |0x1734c0 at0x1d8eab selects slot0xe2d |
| Load |0x16f320 at0x1d8ec8, with the same1/0 load flags as cube.cub |
| Render lookup |World+0x70 catalog; vector begin/end+0x10/+0x18; slot pointer at begin+0x7168,0xe6ade–0xe6ae2 |

This is a resource loaded by initializer0x177f50, not runtime replacement
geometry inferred from a particle style. Its actual CUB dimensions/RGB and
resource metadata remain owner data. The same raw allocation/header and
prepared GPU-resource contract from burst §7.1 applies. The renderer requires
catalog count>0xe2d to obtain its pointer; no alternate model is selected
when the slot is unavailable. The later dimension access is not a supported
missing-resource fallback.

All six styles use the shared model construction in burst §7.2: rebased
fixed-position translation, the same ordered three Euler rotations, the
scalar scale above, then the retained extra matrix and negative half of the
chosen resource's **full header dimensions**. Ordinary bursts retain identity
extra matrix. Style7 therefore centers note.cub using its own header, not
cube.cub's dimensions or an occupied-voxel bound. Shininess1 does not change
this matrix construction or turn the model toward the camera.

## 6. Bounded completeness

This closes the finite ordinary consumers for the six requested style IDs
with valid loaded resources, positive ordinary lifetime and representable
motion/step counts. It does not establish that every one has an independently
reachable ordinary native burst producer, nor specify style numbers outside
0..8, malformed/nonfinite input, duplicate-erasure recovery or outer asset-root
configuration. Other producers can initialize different age/collision/matrix/
lifetime fields; the verified body style0 differences remain in the body spec.
