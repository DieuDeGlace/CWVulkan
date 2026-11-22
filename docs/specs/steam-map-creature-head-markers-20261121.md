# Steam world-map creature portraits for Frogman and Frogwoman

FINAL / STABLE / HELD — 2026-11-21

## Map result

For a playable Frogman or Frogwoman shown on the world map, Face supplies an ordinary-colour model and Haircut supplies a separate marker-aware model. The portrait uses the same two registered sprite objects as the normal creature. The map's main face material is white. Its main haircut material uses the creature's resolved appearance RGB. Both playable Frogman types resolve that RGB to white for every numbered face and haircut. Changing only the character creator's hair-colour swatch therefore preserves the portrait's authored colours. Changing Face or Haircut can change them.

| Portrait part | Geometry source | Main material before map lighting |
| --- | --- | --- |
| Face | Male IDs 0x601–0x605 or female IDs 0x60B–0x60E, loaded in ordinary-colour mode | RGBA (1, 1, 1, 1) |
| Haircut | Male IDs 0x606–0x60A or female IDs 0x60F–0x612, loaded in marker-aware mode | Resolved RGB bytes divided by 255, with alpha 1. For both playable types this is RGBA (1, 1, 1, 1). |

## Map evidence

The startup routine at RVA 0x1355A0 constructs the world-map object at RVA 0x2EAA00. The map rendering pass at RVA 0xE64B0 calls its 3D draw at RVA 0x2F44A0. Within that draw, RVA 0x2F5DEB copies the creature's resolved appearance, including the head and haircut model IDs and the three colour bytes, into a temporary map record. The copy routine is RVA 0x7E780. The original creature appearance starts at offset 0x78. Within it, face and haircut are separate signed 16-bit model IDs at 0x18 and 0x1A, and the resolved RGB bytes are at offsets 2–4.

The map looks up the face ID in the world's sprite registry. A valid head uses its own object and the unit RGBA constant at RVA 0x42E8C0. If the head is unavailable, this branch tries the appearance's body ID at offset 0x20 as the first portrait part. It then looks up the haircut ID independently and attaches the three normalized colour bytes to that part. The two first-part alternatives enter the map list at RVAs 0x2F65F7 and 0x2F666D. The haircut enters at 0x2F6764. A null model produces no submission for that entry. An ordinary numbered Frogman selection with the installed assets resolves both objects.

For each non-null portrait part, the map submits 26 offset copies with a zero four-component material followed by one centred copy with the part's own material. Those calls are at RVAs 0x2F6DB6 and 0x2F6E27. The offset copies use the same model pointer as the central copy and provide the surrounding outline treatment. This describes submission inputs. A zero outline material does not establish the final pixel alpha or the colour at every screen position.

The ordinary creature draw independently resolves the two objects at RVAs 0x18135–0x18191. Its normal face draw receives the incoming draw colour at 0x27CDE. Its haircut draw multiplies that incoming colour by the appearance RGB at 0x28083 and submits at 0x2813A. The map has its own part list, white face material and outline submissions. It does not need the creature's ordinary draw routine to choose the geometry mode. Both paths reach the same registered models. For both sexes the appearance builder at RVA 0x53E80 sets the effective haircut colour to (255, 255, 255), including the branches around 0x54216 and 0x54285.

## Markers and neighbouring faces

At registration RVA 0x177F50, each numbered frogman-head model is passed to the CUB reader in ordinary-colour mode and each numbered frogman-hair model in marker-aware mode. The reader at RVA 0x16F320 records this mode on the sprite. RVA 0x170030 retains the raw three-byte grid and indexes exact red (255, 0, 0), green (0, 255, 0) and blue (0, 0, 255) coordinates when marker handling is enabled. RVA 0x16F230 classifies the current cell. Empty black (0, 0, 0) never generates painted geometry. In marker-aware mode none of the three marker colours generates its own geometry either. In ordinary-colour mode a nonblack pure primary can be paint. The nine installed selectable faces contain none of the exact marker triplets.

For an ordinary painted cell, the mesh builder at RVA 0x170700 tests each in-bounds axis neighbour's raw triplet against empty black. A nonblack neighbour blocks the painted cell's face on that side. This includes every exact marker colour in a marker-aware haircut. The marker itself remains invisible, and it still blocks the face of adjoining paint. Treating the marker as cleared space would add faces the original mesh omits. Changing a haircut's material has no effect on those decisions. The map's central and outline copies use the resulting mesh alike.

One shared example occurs in the installed male and female Haircut 1 CUBs. In each 16 × 17 × 13 grid, cell (3, 7, 2) holds authored green (55, 251, 0). Its negative-X neighbour (2, 7, 2) is exact red. The green cell's negative-X quad is suppressed and the red cell has no quad of its own. Coordinates are zero-based in the CUB's stored axis order. All nine numbered haircut models contain exact red and exact blue, and none contains exact green. Greenish and bluish paint survives the marker classifier when its bytes differ from the reserved triplets.

## Evidence limits

This report covers the numbered playable models once a live creature is admitted to the world-map portrait list. It does not determine the visibility of individual pixels after overlap, map lighting, outline compositing, clipping or zoom, and does not extend the result to every other map icon or gameplay use of marker coordinates. The evidence is static. No game was run.

The supported owned Steam executable has SHA-256 A1EFBBD41EEB8DF08395C4D6511194D416CB70A14054B2A2000B9F5CDBCFA65B. The cited RVAs refer to its unpacked x64 image, preferred base 0x140000000, SHA-256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244.
