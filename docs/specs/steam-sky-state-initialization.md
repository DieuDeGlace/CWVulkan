# Steam renderer sky-state initialization and lifetime

This is an independent native-only behavioral supplement to
[steam-scene-lighting.md §§2.4–2.8](steam-scene-lighting.md). Its scope is the
ordinary renderer's five CPU RGBA records, their reached construction/reset
paths, and their first and subsequent scene updates. RVAs refer to the supported
Steam executable. No owner asset or runtime capture supplies these values.

## 1. Fully initialized CPU records

Each record consists of four consecutive float32 values in R,G,B,A order. The
base renderer constructor **0xcbd10** initializes every component, including
alpha:

| Renderer offset | Record | Initial RGBA |
| --- | --- | --- |
| +0x4ac | sky1 | (0,0,1,1) |
| +0x4bc | sky2 | (0,0,1,1) |
| +0x4cc | fog color | (0,0,1,1) |
| +0x4dc | sun color | (0,0,0,0) |
| +0x4ec | flagged-block target | (0,0,0,0) |

The zero components have word **0x00000000** and the one components have word
**0x3f800000**. These are constructor assignments, not values inferred from
neighboring fields or an assumed allocation clear. The stores at
**0xcbeb4–0xcbf25** cross record boundaries: in particular, the write beginning
at +0x4a8 also initializes sky1 red, and the writes beginning at +0x4b8,
+0x4c8 and +0x4d8 initialize the preceding alpha and following red components.
Readiness +0x4a8 is a separate float initialized to 1.

The ordinary concrete constructor **0xc3150** calls this base constructor at
**0xc31e4**. Its remaining reached construction initializes renderer resources
without replacing these five palette records. Startup **0x1355a0** constructs
this renderer at **0x136219**, before constructing the Game at **0x136331**.
Thus these are the CPU values at completion of successful ordinary renderer
construction. They are not necessarily the values at the first later gameplay
scene: any earlier scene updates of the same instance have already advanced
them.

## 2. Reached scene reset retains palette state

The reset helper **0xe8a30**, reached from the ordinary Game frame at
**0xad752**, writes zero only to float renderer **+0x4a4**, the adaptive range.
It does not write +0x4a8 or any component in +0x4ac through +0x4f8. The reached
caller checks requested versus current world seed/name before the ensuing scene
call; see [steam-scene-lighting.md §6.2](steam-scene-lighting.md).

Consequently this reset does not restore blue skies, clear the sun, initialize
alphas again, or prime a next-update copy. The existing palette state survives
this reset. Actual construction of a new renderer gives §1's initial values;
the reached range-reset operation is not such construction.

## 3. Every reached scene update recomputes the target

The ordinary scene entry **0xe2e00** installs its current inputs and calls
**0xecda0 at 0xe3096**, before the provider call and palette transitions. The
target's complete RGBA is cleared to zero at **0xece02/0xece09** on every such
call. The admitted flagged-block branch later writes only its RGB, at
**0xed536/0xed548/0xed55a**. Alpha remains zero. Its physical-block admission,
noise and RGB production remain as specified in
[steam-scene-lighting.md §2.6](steam-scene-lighting.md).

The target therefore is recomputed temporary frame state, not an accumulated
color and not the previous frame's target. If no branch writes nonzero RGB,
it stays zero RGBA. The four-component zero comparison at
**0xe35e0–0xe3609** chooses the ordinary versus flagged transition; an unordered
comparison also chooses the flagged branch. This comparison is unrelated to
whether the current palette already equals its new target.

## 4. No first-update snapping; precise transition correction

The provider is called at **0xe33fa**. The subsequent reached palette path has
no first-scene flag, constructor-value test, or special copy on world change.
It uses the stored components as the previous state on the first update just
as on later updates. Every RGBA component, including alpha, participates.

**This table supersedes the direct-copy wording and flagged-branch rates in
steam-scene-lighting.md §2.8.** That earlier section correctly describes the
provider and branch roles, but sky1/sky2 are eased in both branches:

| Destination | Zero flagged target | Nonzero/unordered flagged target |
| --- | --- | --- |
| sky1 +0x4ac | provider sky1, rate 0.001 | flagged target, rate 0.001 |
| sky2 +0x4bc | provider sky2, rate 0.001 | flagged target, rate 0.001 |
| fog +0x4cc | provider fog, rate 0.0005 | flagged target, rate 0.0005 |
| sun +0x4dc | provider sun, rate 0.002 | zero RGBA, rate 0.0005 |

The exact float32 rate words are **0x3a83126f**, **0x3a03126f** and
**0x3b03126f**, respectively. The ordinary fog coefficient is selected at
**0xe362b/0xe3653**, and ordinary sun at **0xe36d8/0xe3707**. Ordinary sky1/2
invoke **0xcb5c0 at 0xe3781/0xe379c**, with rate 0.001 loaded at **0xe33ff**.
In the flagged branch, fog invokes that helper at **0xe37c7** with rate 0.0005;
the same rate is passed to the sun's coefficient call at **0xe380d**. Flagged
sky1/2 use rate 0.001 at **0xe3884/0xe389f**.

The helper **0xcb5c0** computes the difference from the saved target and previous
components, obtains the coefficient from **0xa7a0**, multiplies each difference
by that coefficient in float32, and adds the existing destination component
in float32. It is not a four-component assignment. The directly expanded fog
and sun updates follow the same difference/multiply/add stages. The coefficient
and scene-duration precision are already specified in
[steam-scene-lighting.md §§2.5 and 5.1](steam-scene-lighting.md): the scene's
float32 milliseconds are widened to double for the helper. For finite input
whose duration multiplied by 100 truncates to a nonpositive integer, its
coefficient is zero, including on the first scene. There is no independent
positive-duration admission test or first-update snap in this reached path.

Thus construction starts sky/fog alpha at 1 and sun alpha at 0, but their later
alphas are stateful results of these updates. They must not be replaced by
constant alpha 1 or a new provider alpha merely because it is the first scene.

## 5. Bounds of this result

The initialization values above are CPU renderer fields, not a first-upload
oracle for GPU constant buffers. Provider target tables/environment modifiers
are outside this supplement and remain in the linked palette specifications.
Only the ordinary startup constructor, reached scene-range reset and ordinary
scene entry are covered. This is not an exhaustive proof about external writes,
alternate factories, device-loss recovery, or arbitrary memory corruption.
No additional reset/first-update priming condition is established for those
untraced paths. Nothing here implies that a newly selected world gets a newly
constructed renderer.
