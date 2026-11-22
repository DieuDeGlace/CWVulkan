# Steam Frogman and Frogwoman primary hair colour

FINAL / STABLE / HELD — 2026-11-21

## Character creator result

Changing only Hair color leaves the normal playable Frogman or Frogwoman model colours the same. The creator retains the selected RGB in the character's customization record. The appearance builder explicitly sets the three haircut draw-colour bytes to 255, 255, 255 for both sexes. White preserves the haircut's authored voxel colours through the hair-colour multiplication. Face and Haircut remain independent choices, and either can change the appearance.

The creator labels race 7 Frogman and numbers Face and Haircut from one. Its working selection uses zero-based indices. Sex 0 selects native appearance type 13 and sex 1 selects type 14. The refresh routine copies the face and haircut indices into separate 32-bit fields at customization offsets 8 and 12. It also copies the three working picker bytes at creator offsets 0x257 through 0x259 to customization offsets 0x10 through 0x12. The appearance builder then writes white at appearance offsets 2 through 4 regardless of those bytes. This distinction matters when changing race or recording a colour selection. Native evidence comes from RVAs 0x271030, 0x272D90 and 0x53E80, including the two white writes around 0x54216 and 0x54285.

| Playable selection | Face choices and native model IDs | Haircut choices and native model IDs | Valid pairs |
| --- | --- | --- | --- |
| Male | 1–5, 0x601–0x605 | 1–5, 0x606–0x60A | 25 |
| Female | 1–4, 0x60B–0x60E | 1–4, 0x60F–0x612 | 16 |

The IDs ascend with the two-digit suffix of the corresponding frogman-head-m, frogman-hair-m, frogman-head-f or frogman-hair-f CUB filename. Each choice wraps independently in the creator. At appearance construction, nonnegative indices repeat every five for male and every four for female. The separate 16-bit face and haircut fields are at appearance offsets 0x18 and 0x1A. The same cases select frogman-hand.cub (0x613), foot.cub (0x267) and body4.cub (0x001). Neither the hair-base filename nor the eye-patch filename is a numbered haircut selection on this path. Native evidence comes from RVAs 0x272D90, 0x53E80 and the asset registrations in 0x177F50.

## Drawing the two head parts

The native character draw resolves the face and haircut independently at RVAs 0x18135–0x18191. The normal face submission at 0x27CDE receives the ordinary draw colour. For the haircut submission at 0x2813A, the renderer reads the appearance RGB from creature offsets 0x7A–0x7C, normalizes each byte by 255 and multiplies the incoming colour channel by channel. The colour multiplication is at RVA 0x171F0. Thus the haircut has its own drawable and its own multiplier. Both Frogman types give it neutral white. Head markings, hands, feet and body have no primary hair-colour operation in this normal draw path. Both drawables can still receive ordinary scene or character colour effects.

## Model markers and paint

The native CUB reader at RVA 0x16F320 reads three 32-bit dimensions and three colour bytes per cell. Registration at RVA 0x177F50 places all nine numbered haircut models in marker-aware mode. All nine numbered faces and the hand use ordinary colour mode. In marker-aware mode, RVA 0x170030 records the positions of exactly (255, 0, 0), (0, 255, 0) and (0, 0, 255) in three separate groups. The mesh classification at RVA 0x16F230 suppresses those exact colours and the empty (0, 0, 0) cell. Ordinary colour mode suppresses only the empty colour. The mesh builder at RVA 0x170700 uses the other stored RGB values as the base vertex colours. A marker's coordinate can be retained while its own coloured geometry is omitted. The mesh builder also uses an exact empty-cell comparison in its neighbour checks, so marker cells need not behave as empty space in every geometric test.

The numbered CUBs distinguish markings from controls without colour-name guesses. All nine haircut files have 70 exact-blue markers and zero exact-green markers. Each has 188 exact-red markers except male haircut 5, which has 190. All nine numbered faces and the hand have zero of the three exact marker triplets. The common bright green (55, 251, 0) is authored paint. Female haircut 1 also contains the near-blue (0, 17, 255), which qualifies as ordinary paint alongside its exact-blue markers. Neither those markers nor a visually green or blue marking instructs the renderer to sample the creator's RGB.

Changing Face can change markings and palette within the same sex. Male face 3 includes (171, 255, 147) and (59, 167, 3), and female face 3 includes (144, 246, 168). Changing Haircut also selects different authored paint. Male haircut 2 contains (255, 137, 0) and female haircut 4 contains (119, 0, 255). The hand contains (55, 251, 0), (255, 211, 182), (132, 255, 51) and (255, 182, 182). These examples are values in the original source assets before lighting. For a concrete creator check, hold male Face 3 and Haircut 2 fixed and compare two very different swatches. The selected record bytes may differ while the two model IDs, authored colours and white haircut multiplier agree. The same comparison holds for female Face 3 and Haircut 4.

## Evidence boundary

The supported owned Steam executable has SHA-256 A1EFBBD41EEB8DF08395C4D6511194D416CB70A14054B2A2000B9F5CDBCFA65B. RVAs refer to its unpacked x64 image with SHA-256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244 and preferred base 0x140000000. The 18 numbered head and haircut assets, hand, body and foot were byte-compared against the installed Steam files. Evidence is static. Exact final screen pixels, visibility of a particular painted cell under overlap or clothing, special-effect colour overrides, and the meaning of each marker group to other gameplay consumers remain outside the finding. No game was launched.
