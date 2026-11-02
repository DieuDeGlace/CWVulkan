# Steam camera-position construction and lifetime

Independent native-only supplement to
[steam-gameplay-camera.md §§1,3.4,5–7](steam-gameplay-camera.md). RVAs identify
the supported Steam executable. This note establishes the CPU position state
and the reached frontend transitions; the linked camera contract supplies the
already specified position arithmetic and obstruction tests.

## 1. Exact initial state

All fields below are signed64 fixed coordinates, **65536 units per block**:

| Game offsets | Meaning | Constructor value |
| --- | --- | --- |
| +0x1dc0,+0x1dc8,+0x1dd0 | Final eye XYZ | (0,0,0) |
| +0x1dd8,+0x1de0,+0x1de8 | Retained look-at XYZ | (0,0,0) |
| +0x1df0,+0x1df8 | Negative look-at X/Y, render origin | (0,0) |

Game constructor **0x7ea80** explicitly writes all eight zero qwords:
eye at **0x7ed80/0x7ed87/0x7ed8e**, look-at at
**0x7eda1/0x7eda8/0x7edaf**, and negative XY at **0x7edb6/0x7edbd**.
These fields are initialized, not allocation padding or uninitialized state.

The constructor initially zeros the displayed/target angles and sets both
orbit distances to 8. Its later completed-startup camera assignments at
**0x7f767–0x7f7a2** set displayed and target pitch/roll/yaw to **90/0/180**.
They do not write any of the eight position fields. The reached construction
does not invoke **0xadcd0** to prime those fields. In particular, loading a
saved local creature during construction does not assign its saved physical
position to the Game camera anchors. At successful completion of this Game
constructor the table's zero position state remains.

This describes completion of construction, not the first displayed title or
gameplay image. A subsequent ordinary camera call can already have replaced
these values before an image is observed.

## 2. Reached frontend events retain the anchors

The following ordinary handlers do not directly assign eye, look-at or negative
look-at XY and do not call the camera updater to prime them:

| Event and native handler | Relevant change | Position-field consequence |
| --- | --- | --- |
| Title Start Game, entry 0 of **0x965e0** | Hides title and shows character selection | Existing anchors retained |
| Title Continue, entry 3 of **0x965e0** | Loads selected save into local creature; sets current/requested orbit distance to 8 at **0x966d6/0x966e0** | Existing anchors retained; displayed/target angles also retained |
| Select existing character, **0x9f7c0** | Loads selected save; distance 8 and displayed/target angles 90/0/180 at **0x9f93f–0x9f998** | Existing anchors retained |
| Select New Character entry, **0x9f7c0** | Distance 8 and displayed/target angles 90/0/0 at **0x9f9ad–0x9f9fc**; shows creation, resets/repositions local creature | Existing anchors retained |
| Create Character, **0x9ea10** | Hides character creation, saves/appends the character and refreshes its UI | No direct reset of anchors, angles or orbit distances |

The existing/New Character branch is selected using the list cursor against
the saved-character list bounds; an out-of-range cursor takes the new-character
branch. The local-creature physical position changes in the load/start-position
operations are distinct from Game camera positions. Their established save and
creation contracts are [frontend-title.md §6.1](frontend-title.md) and
[frontend-characters.md §§3–5](frontend-characters.md). A click's angle/distance
reset must not be read as an accompanying copy into the position fields.

**World creation is different:** **0x9f060** is the separate world-creation
handler, not character creation **0x9ea10**. Its angle/distance assignments at
**0x9f0c4–0x9f11d** likewise do not directly assign these anchors. The ordinary
frontend does not show that world-creation route; its isolated/unshown status
is covered in frontend-characters §8. Its reset must not be attributed to the
ordinary Create Character button.

## 3. Ordinary camera completion owns subsequent values

The ordinary admitted Game frame calls **0xadcd0 at 0xaaf02**, with the retained
capped double frame milliseconds. This is once per admitted client frame,
separate from World substeps; see
[steam-gameplay-frame-order.md §§1–3](steam-gameplay-frame-order.md).

Within the reached camera routine there is no first-call flag, zero-anchor
test, freshly loaded-character test, or separate initial-position assignment.
It constructs the posture/step-adjusted physical candidate and applies the
ability-specific retained-look-at update in gameplay-camera §5. In particular,
ability **0x9c** copies raw physical XYZ, and **0x9d** copies physical X/Y and
physical Z plus one fixed block. These are ordinary ability routes on every
reached call, not first-update priming.

After the ability route, the **strict squared-distance >2500** safety test
between retained look-at and raw physical position can replace all three
look-at components with raw physical XYZ, at **0xae582–0xae5a5**. This can
therefore snap an initially distant zero anchor, but uses the same condition
on later calls and on transitions. There is no unconditional load-event snap
or special first-frame exception to the 50-block threshold.

The eye is then initialized from that retained look-at at
**0xae5ac–0xae5d8**, offset by the ordinary orbit calculation, and potentially
rebuilt by terrain shortening. Finally, the solid-final-eye test can copy raw
physical XYZ into **both** eye and look-at and set current distance to zero,
at **0xaebda–0xaec27**. These are the existing gameplay-camera §6 operations,
not extra startup behavior.

Near camera completion, **0xaee21–0xaee43** derives negative look-at X/Y from
the then-current look-at and stores them at +0x1df0/+0x1df8. The negations have
the native low64 result. They are not negative eye coordinates, there is no
third Z-origin field, and no frontend event in §2 independently refreshes this
pair. Its consumers/phase lifetime remain specified in
[steam-body-particle-coordinate-lifetime.md §3](steam-body-particle-coordinate-lifetime.md).

Zero elapsed time is not a first-update suppression condition inside this
routine: smoothing coefficients can be zero while the ordinary ability-copy,
safety, eye construction and final-origin stages still run. Consequently
retaining the anchors through a frontend event does not mean retaining them
after the following camera update.

## 4. Boundaries

This note covers explicit ordinary construction, the identified frontend
handler routes and reached camera update. It does not claim an exhaustive
writer census for external callbacks, alternate Game factories, arbitrary
state cloning, or recovery paths. Such paths' anchor behavior remains unproved.
The constructor values are CPU data, not evidence of initial GPU matrix-buffer
contents or of a rendered first image. Scene submission and renderer origins
remain distinct downstream state; no GPU initialization or per-actor cache
correction is inferred here.
