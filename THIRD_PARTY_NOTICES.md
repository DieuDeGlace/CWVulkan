# Third-party notices

CubeVulkan's original engine and extractor source is GPL-3.0-only (root `LICENSE`).
The components and art below retain their separate upstream licenses and notices.

Cube World and Steam are properties of their respective owners. This is an unofficial project. No game content or Steam runtime is licensed by this document or included in the source-review archive.

`src/MoonRules.h` is copied from DieuDeGlace's MIT-licensed MoonandShadow project. Related day/night, shadow and terrain-mod rules cite that project's source in their comments. The retained MIT notice is in `licenses/MoonandShadow-MIT.txt`.

`src/TerrainRules.h` is copied from DieuDeGlace's MIT-licensed SteamCubeWorldMods
TerrainVariance module; its notice is `licenses/SteamCubeWorldMods-MIT.txt`.

The build fetches these pinned open-source dependencies. License copies below are retained from the fetched versions; consult upstream per-file notices when distributing a compiled build or vendored source.

| Dependency | Pinned revision | License notice |
| --- | --- | --- |
| [Vulkan-Headers](https://github.com/KhronosGroup/Vulkan-Headers) | v1.4.304 | `licenses/Vulkan-Headers-LICENSE.md`, `Vulkan-Headers-Apache-2.0.txt`, `Vulkan-Headers-MIT.txt` |
| [volk](https://github.com/zeux/volk) | 1.4.304 | `licenses/volk-MIT.txt` |
| [GLM](https://github.com/g-truc/glm) | 1.0.1 | `licenses/glm-copying.txt` (MIT or Happy Bunny) |
| [miniaudio](https://github.com/mackron/miniaudio) | 0.11.21 | `licenses/miniaudio-LICENSE.txt` (public domain or MIT) |
| [stb](https://github.com/nothings/stb) | a7d4c2175630e1ea5b6f8dd00ab96fed0b97b96d | `licenses/stb-LICENSE.txt` (MIT or public domain) |
| [GLFW](https://github.com/glfw/glfw) | 3.4 | `licenses/glfw-LICENSE.txt` (zlib/libpng) |
| [SQLite](https://www.sqlite.org) | 3.46.1 (amalgamation) | public domain, no license file needed (https://www.sqlite.org/copyright.html) |
| [zlib](https://zlib.net) | 1.3.1 | `licenses/zlib-LICENSE.txt` (zlib License) |
| [glslang](https://github.com/KhronosGroup/glslang) | 15.0.0 | `licenses/glslang-LICENSE.txt` (multiple notices) |
| [FreeType](https://freetype.org) | 2.13.3 | `licenses/FreeType-LICENSE.txt`, `FreeType-FTL.txt`, `FreeType-GPLv2.txt`; this project uses the FreeType License option |
| [SMAA](https://github.com/iryoku/smaa) | vendored files in `third_party/smaa` | `third_party/smaa/LICENSE.txt` (MIT); shader, area and search tables retain their notices |

This software uses the FreeType Project (https://freetype.org). FreeType is copyright
David Turner, Robert Wilhelm, Werner Lemberg and its contributors.

Extractor tools include CWSDK headers from the separate sibling `SteamCubeWorldMods/vendor/cwmods` checkout. That checkout attributes the MIT-licensed SDK to Gijs/gijsgroenewegen's [CubeMegaMod](https://github.com/gijsgroenewegen/CubeMegaMod) and [CWSDK](https://github.com/gijsgroenewegen/CWSDK), with local loader compatibility edits. Its MIT notice is retained in `licenses/CWSDK-MIT.txt`. The SDK headers and external CubeModLoader binary are not included in this archive.

The quest sprites in `assets/OverworldEvents/quest-icons.png` come from DieuDeGlace's [OverworldEvents](https://github.com/DieuDeGlace/OverworldEvents) (MIT); its license is `assets/OverworldEvents/LICENSE`.
The same project supplies the rules in `src/events/`, as described in that directory's README.
