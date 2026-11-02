#include "Extracted.h"
#include "AlphaLook.h"
#include <array>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <set>

namespace {
template <class T> void field(std::ostream &out, const T &value) {
    out.write(reinterpret_cast<const char *>(&value), sizeof value);
}
void fixture(const std::filesystem::path &directory, int32_t x, int32_t y) {
    std::ofstream out(directory / ("z_" + std::to_string(x) + "_" + std::to_string(y) + ".cwz"), std::ios::binary);
    out.write("CWZ1", 4);
    field(out, x);
    field(out, y);
    field(out, int32_t(0));
    field(out, int32_t(0));
    const auto run = [&](uint16_t count, worldgen::Block block) {
        field(out, count);
        field(out, block);
    };
    for (int lx = 0; lx < 64; ++lx)
        for (int ly = 0; ly < 64; ++ly) {
            int top = 37;
            if (x == 0 && y == 0 && ly == 3)
                top = lx == 3 ? 286 : lx == 4 ? 287 : lx == 5 ? 787 : 37;
            field(out, int32_t(37));
            field(out, uint32_t(top == 37 ? 1 : 3));
            run(1, {80, 100, 120, 0, 6, 0});
            if (top != 37) {
                run(uint16_t(top - 38), {255, 255, 255, 0, 0, 0});
                run(1, {80, 100, 120, 0, 6, 0});
            }
        }
}
using Corner = std::array<int, 9>;
Corner corner(const Vertex &v, int base = -ExtractedWorld::Z_OFFSET) {
    return {int(v.x), int(v.y), int(v.z) + base + ExtractedWorld::Z_OFFSET, v.r, v.g, v.b, v.ao, v.normal, v.flags};
}
} // namespace

int terrainBandTests() {
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        if (!ok) {
            ++failures;
            std::printf("FAIL terrain bands: %s\n", message);
        }
    };
    check(terrainBandBase(-2, -251) == -251 && terrainBandBase(-1, -251) == -1 && terrainBandBase(0, -251) == -1,
          "negative heights use the lowest-exposed relative grid");
    check(terrainBandBase(-252, -251) == -251, "below-anchor arithmetic truncates toward zero");
    Mesh precise;
    precise.localOrigin = glm::ivec3(0, 0, 16777216);
    meshFaces(
        {0, 0, 16777465}, {1, 1, 16777466}, [](int x, int y, int z) { return x == 0 && y == 0 && z == 16777465; },
        [](int, int, int) { return VoxelColor{1, 2, 3, STEAM_VERTEX}; },
        [](int, int, int, int, int, VoxelColor) { return CornerShade{1, 2, 3, 255}; }, precise, 1, {}, VoxelLayout::SteamTerrain);
    check(precise.min.z == 249 && precise.max.z == 250,
          "local integer subtraction preserves the top250 corner beyond float's integer precision");

    const auto previousStyle = renderStyle();
    setRenderStyle(RenderStyle::Steam);
    const auto directory = std::filesystem::temp_directory_path() /
                           ("CubeVulkan-terrain-bands-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    {
        std::ofstream manifest(directory / "manifest.txt");
        manifest << "seed=0\nhomeX=0\nhomeY=0\nhomeZ=37\n";
    }
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
            fixture(directory, x, y);
    {
        ExtractedWorld world;
        std::string error;
        check(world.open(directory, error), "synthetic band world opens");
        Mesh opaque, water, lava;
        std::optional<scenevisibility::Bounds> bounds;
        worldgen::MeshLightVolume light;
        std::vector<TerrainMeshBand> bands;
        check(world.mesh(0, 0, opaque, water, &bounds, &light, &lava, &bands), "actual terrain mesher produces local bands");
        std::vector<int32_t> bases;
        std::multiset<Corner> local;
        size_t indices = 0;
        bool bounded = true, boundary = false;
        for (const auto &band : bands) {
            bases.push_back(band.base);
            for (const auto *mesh : {&band.opaque, &band.lava}) {
                indices += mesh->indices.size();
                for (const auto &v : mesh->vertices) {
                    bounded &= v.x >= 0 && v.x <= 64 && v.y >= 0 && v.y <= 64 && v.z >= 0 && v.z <= 250;
                    boundary |= band.base == 37 && v.z == 250;
                    local.insert(corner(v, band.base));
                }
            }
        }
        check(bounds && bounds->lowest == 37 && bases == std::vector<int32_t>{37, 287, 787},
              "empty intermediate band is omitted while retained bases and order survive");
        check(bounded && boundary && opaque.empty() && lava.empty(), "source-block ownership keeps top250 in its original local band");
        Mesh legacy, legacyWater, legacyLava;
        std::optional<scenevisibility::Bounds> oldBounds;
        check(world.mesh(0, 0, legacy, legacyWater, &oldBounds, nullptr, &legacyLava), "aggregate compatibility mesher remains available");
        std::multiset<Corner> aggregate;
        for (const auto *mesh : {&legacy, &legacyLava})
            for (const auto &v : mesh->vertices)
                aggregate.insert(corner(v));
        check(local == aggregate && indices == legacy.indices.size() + legacyLava.indices.size(),
              "local packing retains every physical colored corner and face count");
        check(bounds && oldBounds && bounds->triangleCost == oldBounds->triangleCost,
              "native adaptive triangle cost includes the band index arrays");
    }
    setRenderStyle(previousStyle);
    if (std::filesystem::equivalent(directory.parent_path(), std::filesystem::temp_directory_path()) &&
        directory.filename().string().rfind("CubeVulkan-terrain-bands-", 0) == 0)
        std::filesystem::remove_all(directory);
    std::printf("Native terrain mesh bands: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
