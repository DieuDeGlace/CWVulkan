#include "SteamVoxel.h"
#include "worldgen/ZoneLight.h"
#include "CubModel.h"
#include "AlphaLook.h"
#include <cstring>
#include <cstdio>
#include <limits>
#include <vector>

int steamVoxelTests() {
    int failures = 0;
    auto check = [&](bool result, const char *message) {
        if (!result) { std::printf("FAIL Steam voxel: %s\n", message); ++failures; }
    };
    using namespace steamvoxel;
    const Probe open = [](Point) { return Sample{true, 255}; };
    const Probe shut = [](Point) { return Sample{false, 255}; };
    for (int face = 0; face < 6; ++face) {
        check(cubCorner({255, 255, 255}, {}, face, open) == glm::u8vec4(127, 127, 127, 255),
              "open stock CUB halves RGB but keeps V1");
        const auto terrain = terrainCorner(glm::vec4(1), {}, face, open);
        if (terrain != glm::u8vec4(255, 255, 255, 254))
            std::printf("Terrain open face%d packed: %u,%u,%u,%u\n", face, terrain.r, terrain.g, terrain.b, terrain.a);
        check(terrain == glm::u8vec4(255, 255, 255, 254), "open terrain retains nonhalved RGB and native rounded light byte254");
        check(2 * (terrain.r / 255.0f) == 2, "terrain shader palette can exceed1");
        check(cubCorner({255, 255, 255}, {}, face, shut) == glm::u8vec4(55, 55, 55, 255),
              "closed CUB clamps openness to0.5, baking AO in RGB, not V");
        check(terrainCorner(glm::vec4(1), {}, face, shut) == glm::u8vec4(0),
              "closed terrain has no CUB openness floor and no open light samples");
    }
    check(cubCorner({255, 255, 255}, {}, 0, open, {}, CubEncoding::Unhalved) == glm::u8vec4(255, 255, 255, 254),
          "unhalved sprite mode preserves native alpha2 low-byte wrap");
    check(cubCorner({1, 2, 3}, {}, 0, open) == glm::u8vec4(0, 1, 1, 255),
          "half-color quantization truncates rather than rounding or canceling losslessly");
    check(cubCorner({255, 255, 255}, {}, 0, open, {255, 255, 255}) == glm::u8vec4(127, 127, 127, 255),
          "overbright additive shade is luminance-normalized before packing");
    const auto red = cubCorner({255, 0, 0}, {}, 0, shut);
    check(red.r > 55 && red.r < 127 && red.g == 0 && red.b == 0,
          "CPU occlusion changes saturation as well as intensity");

    std::vector<Point> sampled;
    const Probe record = [&](Point p) { sampled.push_back(p); return Sample{true, 255}; };
    cubCorner({255, 255, 255}, {10, 20, 30}, 0, record);
    check(sampled.size() == 36 && sampled[0] == Point(10, 17, 27) && sampled[1] == Point(10, 17, 28) &&
          sampled.back() == Point(10, 22, 32), "positive-X layer is X/Y/Z ordered with half-open tangential offsets");
    const Point first[]{ {9, 17, 27}, {7, 20, 27}, {7, 19, 27}, {7, 17, 30}, {7, 17, 29} };
    for (int face = 1; face < 6; ++face) {
        sampled.clear();
        cubCorner({255, 255, 255}, {10, 20, 30}, face, record);
        check(sampled.size() == 36 && sampled.front() == first[face - 1], "each normal selects the correct front layer");
    }
    auto mixed = [](uint8_t solidLight, uint8_t openLight) -> Probe {
        return [=](Point p) { return Sample{p.y == 0 && p.z == 0, p.y == 0 && p.z == 0 ? openLight : solidLight}; };
    };
    const auto darkSolids = terrainCorner(glm::vec4(1), {}, 0, mixed(0, 128));
    const auto brightSolids = terrainCorner(glm::vec4(1), {}, 0, mixed(255, 128));
    check(darkSolids == brightSolids && darkSolids.a > 126 && darkSolids.a <= 128,
          "only open samples contribute to terrain light mean");
    const auto lowLight = terrainCorner(glm::vec4(1), {}, 0, mixed(255, 64));
    check(brightSolids.a > lowLight.a + 62 && brightSolids.r == lowLight.r,
          "light V changes independently of CPU AO RGB");
    sampled.clear();
    check(terrainCorner({.25f, .5f, .5f, 1}, {}, 4, record, true) == glm::u8vec4(63, 127, 127, 255) && sampled.empty(),
          "special type/water bypass retains base RGBA without sampling");
    check(cubCorner({255, 255, 255}, {}, 0, open, {}, CubEncoding::Stock, 0) == glm::u8vec4(0, 0, 0, 255),
          "zero total sample weight does not invent minimum openness");
    check(byte(2) == 254 && byte(-1) == 1 && byte(std::numeric_limits<float>::quiet_NaN()) == 0,
          "native byte packing wraps rather than saturating");
    const std::array<glm::u8vec3, 4> equal{{{100,100,100}, {100,100,100}, {100,100,100}, {100,100,100}}};
    check(terrainTriangles(equal) == std::array<uint32_t, 6>{0,1,3,2,3,1}, "terrain RGB ties use diagonal1-3");
    auto contrast = equal;
    contrast[0] = {0,0,0}; contrast[2] = {255,255,255};
    check(terrainTriangles(contrast) == std::array<uint32_t, 6>{0,1,3,2,3,1}, "terrain chooses the lower-contrast1-3 diagonal");
    contrast = equal;
    contrast[1] = {0,0,0}; contrast[3] = {255,255,255};
    check(terrainTriangles(contrast) == CUB_TRIANGLES, "terrain chooses the lower-contrast0-2 diagonal");

    // Actual stock CUB meshing must carry the packed encoding and native topology.
    const int32_t size[]{1,1,1};
    std::vector<uint8_t> cubeBytes(15, 255);
    std::memcpy(cubeBytes.data(), size, sizeof size);
    CubModel cube;
    std::string error;
    check(cube.loadMemory(cubeBytes.data(), cubeBytes.size(), &error), "one-voxel CUB fixture loads");
    const auto previousStyle = renderStyle();
    setRenderStyle(RenderStyle::Steam);
    const Mesh nativeMesh = cube.mesh();
    check(nativeMesh.vertices.size() == 24 && nativeMesh.indices.size() == 36, "native CUB emits four vertices per face");
    if (nativeMesh.vertices.size() == 24) {
        bool encoded = true;
        for (const auto &vertex : nativeMesh.vertices)
            encoded &= vertex.r == 127 && vertex.g == 127 && vertex.b == 127 && vertex.ao == 255 && (vertex.flags & STEAM_VERTEX);
        check(encoded, "CUB mesh carries native half-RGB/constantV bytes with explicit encoding flag");
        const auto &firstVertex = nativeMesh.vertices[0];
        check(firstVertex.normal == 1 && firstVertex.x == -.5f && firstVertex.y == -.5f && firstVertex.z == 0,
              "native W face/corner is emitted first");
        bool normals = true;
        for (int face = 0; face < 6; ++face)
            normals &= nativeMesh.vertices[face * 4].normal == FACE_ORDER[face];
        check(normals, "native W/E/S/N/bottom/top face order");
    }
    setRenderStyle(RenderStyle::Alpha);
    const Mesh alphaMesh = cube.mesh();
    check(!alphaMesh.vertices.empty() && alphaMesh.vertices.front().normal == 0 &&
          !(alphaMesh.vertices.front().flags & STEAM_VERTEX), "Alpha keeps its classic layout and own RGB encoding");
    setRenderStyle(previousStyle);

    // Retained light bytes must outlive the terrain used to solve them and match
    // the model sampler, including solid and out-of-column positions.
    std::vector<worldgen::TerrainColumn> columns(4096);
    for (auto &column : columns) {
        column.base = 3;
        worldgen::Block block{};
        block.type = 1;
        column.blocks.push_back(block);
    }
    const worldgen::ZoneColumnsSource source = [&](int32_t, int32_t) -> const auto & { return columns; };
    const auto volume = worldgen::meshZoneLight(-2, -1, source);
    const std::vector<std::array<int64_t, 3>> positions{
        {-128LL * 65536, -64LL * 65536, 2LL * 65536},
        {-128LL * 65536, -64LL * 65536, 3LL * 65536},
        {-128LL * 65536, -64LL * 65536, 4LL * 65536}};
    const auto model = worldgen::sampleZoneLight(-2, -1, source, positions);
    check(model == std::vector<uint8_t>({0, 255, 255}), "mesh-light below-base, retained-solid and open-sky samples");
    columns.clear(); columns.shrink_to_fit();
    check(volume.fixed(positions[0]) == model[0] && volume.fixed(positions[1]) == model[1] &&
          volume.fixed(positions[2]) == model[2], "retained volume owns light bytes independently of source columns");
    check(volume.at(-137, -64, -100) == 255 && volume.at(-136, -64, -100) == 0,
          "native horizontal outside returns sky; below an existing column returns dark");
    std::printf("Steam voxel: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
