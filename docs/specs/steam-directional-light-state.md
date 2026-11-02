# Steam directional-light CPU state

This bounded native-only supplement records the ordinary renderer's front,
back and ambient RGBA lifetime. It complements the provider, easing and upload
contracts in [steam-scene-lighting.md §§2.4–2.8](steam-scene-lighting.md).
RVAs refer to the supported Steam executable. These are native constructor and
reached caller facts, not owner capture measurements.

## 1. Constructor values, including alpha

Each destination is four consecutive float32 components in R,G,B,A order:

| Renderer offset | Meaning | Initial RGBA | Component words |
| --- | --- | --- | --- |
| +0x50c | Front | (0.7,0.7,0.3,1) | 3f333333,3f333333,3e99999a,3f800000 |
| +0x51c | Back | (0.5,0.5,0.5,1) | 3f000000,3f000000,3f000000,3f800000 |
| +0x4fc | Ambient | (0.3,0.3,0.7,1) | 3e99999a,3e99999a,3f333333,3f800000 |

The decimals summarize the listed float32 words; 0.3 and 0.7 are not exact
real-valued storage. All twelve components are explicitly initialized by base
constructor **0xcbd10**: ambient at **0xcbf2b–0xcbf49**, front at
**0xcbf53–0xcbf71**, and back at **0xcbf7b–0xcbf99**. There is no uninitialized
alpha in these records.

The ordinary concrete constructor **0xc3150** calls the base at **0xc31e4**.
Its remaining reached resource construction does not replace these records.
Ordinary startup **0x1355a0** constructs that renderer at **0x136219**, before
constructing the Game at **0x136331**. The table describes successful renderer
construction, not an assurance that later entry into a particular world still
has those colors. Earlier scene calls on that instance may already have
advanced them.

## 2. Requested-world mismatch does not reset the colors

In the ordinary Game frame, **0xad724–0xad74c** compares requested seed
Game+0x206c with current World+0x220, then, if those match, compares requested
name Game+0x2070 with current World+0x200 through **0x6ddf0**. A mismatch reaches
**0xe8a30 at 0xad752**, with renderer Game+0x18. The subsequent ordinary scene
call is **0xe2e00 at 0xad9d4**.

The entire reset helper **0xe8a30–0xe8a3a** only writes zero to float renderer
**+0x4a4**, the adaptive range. It does not write front, back or ambient, their
alphas, or a palette-priming flag. Therefore this reached mismatch/reset path
retains their prior CPU state. It does not substitute constructor colors or
copy the next world's targets. Creating another renderer runs §1 again;
this range reset does not create one.

## 3. First, zero-duration and negative-duration updates

After the reached scene installs current inputs and recomputes its frame
targets, it calls provider **0x2b0400 at 0xe33fa**. Front, back and ambient then
update in that order, before the sky/fog flagged-target decision. The provider
targets are those already specified in scene-lighting; the destination records
are the existing CPU fields above.

The native update at **0xe33ff–0xe35d3** has no first-scene test, initial-color
test, world-mismatch priming condition, or duration-sign branch that assigns
targets directly. It obtains one coefficient through **0xa7a0 at 0xe3446**,
using the scene's float32 milliseconds widened to double and rate float32 word
**0x3a83126f** (0.0010000000474974513). The same returned float32 coefficient
is reused for all twelve components. Front updates at **0xe342c–0xe34be**,
back at **0xe34c3–0xe3550**, and ambient at **0xe355c–0xe35d3**.

Each component's target-minus-previous difference is evaluated in float32,
multiplied by the coefficient in float32, then added to its existing destination
in float32. Alpha takes these same stages. In particular, ambient alpha begins
at 1 and subsequently eases toward its provider alpha; it is not assigned the
current provider alpha on first use.

For ordinary finite durations within signed32 conversion range, **0xa7a0**
truncates double(milliseconds) multiplied by 100 to its signed32 iteration
count. A nonpositive count produces coefficient zero. Thus zero milliseconds,
negative milliseconds, and positive milliseconds below the first iteration
threshold do not trigger a reset, copy, or first-update snap. The caller still
executes the component arithmetic with coefficient zero. This is a statement
of the arithmetic path, not a promise of preserving arbitrary NaN payloads or
signed-zero bit patterns. The positive-count coefficient's ordered double
accumulation is specified in scene-lighting §2.5; no alternative easing law is
introduced here.

## 4. Explicit limits

This establishes CPU field construction and retention through the ordinary
startup, scene entry and reached requested-world range-reset call. The first
scene uses whatever state its renderer currently has, with no additional
priming gate established in those paths. These CPU assignments are not an
oracle for initial GPU constant-buffer contents before the first upload.
Alternate factories, device-loss/recovery paths, external writers and other
unreached reinitialization routes were not exhaustively traced; their initial
or reset behavior remains unproved. Provider target production and later GPU
binding remain separate contracts in the linked lighting specification.
