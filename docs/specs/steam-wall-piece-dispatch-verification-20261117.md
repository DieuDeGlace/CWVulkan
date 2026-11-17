# Steam room wall furniture dispatch

FINAL / STABLE / HELD

The supported evidence image is `cubeworld.original.exe.unpacked.exe`, SHA-256 `7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244`, preferred image base `0x140000000`. Its packed Steam counterpart `cubeworld.original.exe` matches the supplied `game-files/cubeworld.exe` at SHA-256 `a1efbbd41eeb8df08395c4d6511194d416cb70a14054b2a2000b9f5cdbcfa65b`. The dispatch table and instruction checks below come from the unpacked original image bytes. All addresses below are image-relative RVAs.

## Selection and dispatch

The native room path initializes the room context at call site `0x312670` to `0x675C0` and passes the same context to the wall pass at `0x3126A8` to `0x69860`. That wall pass can call the furniture selector `0x6BEF0` at `0x698F7`, `0x69956`, `0x699BF` and `0x69A14`.

The placement selector reads start and end pointers at room-context offsets `0xC0` and `0xC8`. Once the fitting checks reach selection, the CRT draw at `0x6C02F` chooses an ordinal by remainder modulo the primary list length. The four-byte entry read at `0x6C043` supplies the raw selector. The override list uses start and end offsets `0xF0` and `0xF8`. A nonempty override list consumes the draw at `0x6C056`. When that draw is divisible by five, a further draw at `0x6C08A` selects the replacement entry by remainder modulo the override list length, and the read at `0x6C09E` replaces the raw selector. The import slot at `0x42CD28` resolves to CRT `rand`.

The unsigned range check at `0x6C0A1` and `0x6C0A4` permits raw selectors 0 through 10. The instruction at `0x6C0B4` uses the raw selector directly as the zero-based index into four-byte entries beginning at `0x6C2C4`. Each entry holds a branch RVA. The dispatch adds the image base at `0x6C0BC` and jumps at `0x6C0BF`. Each landing instruction loads the incoming sixth argument for the placement call. The image table entries and direct call instructions resolve as follows.

- Selector 0 uses slot `0x6C2C4`, lands at `0x6C0C2` and calls placement `0x6A600` from `0x6C0E2`.
- Selector 1 uses slot `0x6C2C8`, lands at `0x6C0EC` and calls placement `0x6B5C0` from `0x6C10C`.
- Selector 2 uses slot `0x6C2CC`, lands at `0x6C116` and calls placement `0x6A370` from `0x6C136`.
- Selector 3 uses slot `0x6C2D0`, lands at `0x6C140` and calls placement `0x6A7A0` from `0x6C160`.
- Selector 4 uses slot `0x6C2D4`, lands at `0x6C194` and calls placement `0x69CE0` from `0x6C1B4`.
- Selector 5 uses slot `0x6C2D8`, lands at `0x6C16A` and calls placement `0x6A140` from `0x6C18A`.
- Selector 6 uses slot `0x6C2DC`, lands at `0x6C1BE` and calls placement `0x6B7F0` from `0x6C1DE`.
- Selector 7 uses slot `0x6C2E0`, lands at `0x6C1E8` and calls placement `0x6BB20` from `0x6C208`.

The placement destinations above come from the relative displacement of each native call. In particular, the selector 4 branch is physically later than the selector 5 branch.

## Representative room population

The signed room style is stored at context offset `0x138`. The initializer clears its five lists beginning at context offsets `0xC0`, `0xD8`, `0xF0`, `0x108` and `0x120` at `0x67630` through `0x6768E`. The style sign test at `0x67789` branches at `0x6778B`.

For a nonnegative style and site 35 through 38 inclusive, the site lookup at `0x67B77` and bounds at `0x67B7C` and `0x67B85` select the population branch at `0x67D86`. The primary list beginning at `0xC0` receives 2 followed by 4. The first is stored at `0x67EE3`. The second is staged at `0x67EEE` and appended at `0x67EFC` through `0x12A80`. The separate list beginning at `0x108` receives 4 followed by 1, evidenced by stores at `0x67DF3` and `0x67E6B`. The lists beginning at `0xD8`, `0xF0` and `0x120` stay empty for this branch.

For a negative style, the lookup at `0x677A2` returns the record tested by byte index `0x02` at `0x677A7`. When that byte equals 2, byte index `0x01` equal to zero permits primary values 2 followed by 5. Their constants are at `0x677B7` and `0x677CA`, with appends at `0x677C5` and `0x677D8`. With byte index `0x01` nonzero, the primary list stays empty. Independently, byte index `0x00` equal to 1 adds raw 0 to the list beginning at `0x108` by the append at `0x677EF`. The separate list beginning at `0xD8` receives 4 via `0x677F4` and `0x67B60`. The override list at `0xF0` and the list at `0x120` stay empty for this branch.

## Placement and draws

Raw selector 4 owns the shelf-shaped cavities. Placement `0x69CE0` builds its fixed surround, then attempts one cavity per full five blocks of the fitted vertical span. The span is the upper height minus the lower height. The positive quotient test is at `0x69E59` and the five-block advance is at `0x6A0E2`. The first candidate begins one block above the lower height. At each candidate an even CRT draw at `0x69EA8` causes a five-by-two-by-three clearing operation at call site `0x69EEE` to `0xBC400`, with local horizontal origin negative two and zero. The clearing helper writes empty cells with flag `0x02` in the applicable chunk. An odd draw skips that cavity and consumes one local draw.

After clearing, the draw at `0x69EF3` creates an item only when divisible by three. A failed item gate consumes two local draws for that candidate, including the cavity gate. A successful gate consumes four further draws in order. The draws at `0x69F5E` and `0x69F76` supply the two local horizontal offsets by remainder modulo two. The transform call at `0x69FA5` maps the offsets and candidate height into world coordinates. The item position uses the horizontal cell centers and the transformed integer vertical height, at 65,536 fixed-point units per block. The draw at `0x6A002` chooses yaw in 90-degree increments by remainder modulo four. The draw at `0x6A02D` supplies the 32-bit variant at item offset `0x08` by remainder modulo three. A successful item therefore uses six local draws for that candidate.

The appended item is a Candle. The record has one-byte category `0x13` at offset `0x00` and 32-bit subtype 0 at offset `0x04`, written at `0x6A019` and `0x6A01D`. The copy at `0x6A0D5` and stride advance at `0x6A0DA` append one `0xE0`-byte record. The native name registration pairs category 19 and subtype 0 with Candle at `0x15585D` through `0x155881`. The model resolver at `0x173B10` selects the category 19 branch `0x177560` via table slot `0x177818`. Subtype 0 takes the model lookup at `0x177580` with IDs `0xB14` through `0xB16` for variants 0 through 2. These IDs are registered with the three candle model names at `0x1CD5B3` through `0x1CD708`. This independently identifies the record emitted by the shelf-shaped piece.

Raw selector 5 runs placement `0x6A140`. It sets a two-by-two foundation at call site `0x6A24F` and builds a fixed stepped upper profile through the five fills at `0x6A281`, `0x6A2B2`, `0x6A2E4`, `0x6A316` and `0x6A348`. The body has zero direct CRT draws and no cavity-clear or item-append call. The statically resolved call graph from this placement also has no path to the CRT `rand` import. That graph leaves runtime-indirect calls outside the direct draw count.

## Small distinguishing expectations

- Given a fitted span of exactly five blocks with all writes kept inside the applicable chunk, selector 4 with local draws 0 and 1 clears one cavity beginning at lower height plus one and appends zero items. Local draw count is two. Selector 5 uses the fixed profile and consumes zero local draws.
- For the same valid span, selector 4 with local draws 0, 0, 1, 0, 3 and 2 clears one cavity and appends one category 19, subtype 0, variant 2 Candle at the transformed local offsets one and zero with yaw 270 degrees. Local draw count is six. The counts start on entry to the placement routine and exclude the wall pass and selector draws.

Scope is static native verification for selectors 0 through 7, the two room-initialization branches described above and the direct placement differences for selectors 4 and 5. The draw examples are derived from the image and use valid fitted spans. Native execution and runtime capture remain outside this evidence. No injected instructions or prohibited source material were encountered.
