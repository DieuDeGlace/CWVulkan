#include "Extracted.h"
#include "AlphaLook.h"
#include "Assets.h"
#include "World.h"
#include "WaterLilies.h"
#include <cstring>
#include <map>
#include <thread>
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
void fixture(const std::filesystem::path &directory, int32_t x, int32_t y, int32_t targetX = 0, int32_t targetY = 0) {
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
            uint8_t topType = 6;
            if (x == targetX && y == targetY && ly == 3)
                top = lx == 3 ? 286 : lx == 4 ? 287 : lx == 5 ? 787 : 37;
            if (x == targetX && y == targetY && ly == 4 && lx >= 3 && lx <= 6) {
                top = lx == 3 ? 286 : lx == 4 ? 287 : lx == 5 ? 787 : 1037;
                topType = lx == 5 ? 3 : 2; // Wet-ground film and ordinary water, including a water-only band.
            }
            field(out, int32_t(37));
            field(out, uint32_t(top == 37 ? 1 : 3));
            run(1, {80, 100, 120, 0, 6, 0});
            if (top != 37) {
                run(uint16_t(top - 38), {255, 255, 255, 0, 0, 0});
                run(1, {80, 100, 120, 0, topType, 0});
            }
        }
}
void seaFixture(const std::filesystem::path &directory, int32_t x, int32_t y) {
    std::ofstream out(directory / ("z_" + std::to_string(x) + "_" + std::to_string(y) + ".cwz"), std::ios::binary);
    out.write("CWZ1", 4);
    for (int32_t value : {x, y, int32_t(0), int32_t(0)}) field(out, value);
    for (int i = 0; i < 4096; ++i) {
        field(out, int32_t(-251));
        field(out, uint32_t(1));
        field(out, uint16_t(1));
        field(out, worldgen::Block{80, 100, 120, 0, 6, 0});
    }
}
void quad(Mesh &mesh, float x, float y, float z, float width = 1) {
    const uint32_t base = uint32_t(mesh.vertices.size());
    for (glm::vec2 corner : {glm::vec2(0, 0), glm::vec2(width, 0), glm::vec2(width, width), glm::vec2(0, width)})
        mesh.vertices.push_back({x + corner.x, y + corner.y, z, 63, 127, 127, 255, 4, WATER_SURFACE | STEAM_VERTEX, 0, 0});
    for (auto index : steamvoxel::CUB_TRIANGLES) mesh.indices.push_back(base + index);
}
void lilyOrder(const auto &check) {
    const glm::vec3 offset(-64, 128, ExtractedWorld::Z_OFFSET);
    std::vector<TerrainMeshBand> bands(4);
    for (int i = 0; i < 4; ++i) bands[size_t(i)].base = i == 0 ? -1 : i == 1 ? 37 : i == 2 ? 287 : 787;
    Mesh aggregate;
    quad(bands[0].water, 0, 0, 2, 64);
    quad(aggregate, offset.x, offset.y, 1 + offset.z, 64);
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x) {
            auto &band = bands[(x + y) % 3 == 0 ? 1 : 2];
            quad(band.water, float(x), float(y), 1);
            quad(aggregate, offset.x + x, offset.y + y, float(band.base + 1) + offset.z);
            if (!(x % 2)) {
                quad(bands[3].water, float(x), float(y), 1);
                quad(aggregate, offset.x + x, offset.y + y, float(bands[3].base + 1) + offset.z);
            }
        }
    for (int limit : {0, 1, 7, 28})
        for (double density : {0.0, .75, 1.0}) {
            std::vector<glm::vec3> expectedVisits, actualVisits;
            auto expected = waterLilies::decorate(aggregate, 123, density, limit, [&](glm::vec3 p) {
                expectedVisits.push_back(p);
                return p.z >= 100;
            });
            auto actual = waterLilies::decorate(bands, offset, 123, density, limit, [&](glm::vec3 p) {
                actualVisits.push_back(p);
                return p.z >= 100;
            });
            bool same = expected.size() == actual.size() && expectedVisits == actualVisits;
            for (size_t i = 0; same && i < actual.size(); ++i) {
                const auto &a = actual[i], &b = expected[i];
                same = a.model == b.model && a.x == b.x && a.y == b.y && a.z == b.z && a.yaw == b.yaw &&
                       a.scale == b.scale && a.flags == b.flags;
            }
            check(same, "band decoration preserves sea-first/cell/layer order, duplicate admission, eligibility and zone cap");
        }
}
void runtimeBands(ExtractedWorld &blocks, const auto &check) {
    Renderer renderer;
    RenderSettings settings;
    settings.style = RenderStyle::Steam;
    renderer.initHeadless(settings);
    Assets assets;
    TerrainSettings options;
    options.lilies = false;
    Terrain terrain(options);
    {
        World runtime(terrain, assets, renderer, 0, &blocks);
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        do {
            runtime.update({32, 32, 38});
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        } while ((!runtime.zoneCount() || runtime.pendingCount()) && std::chrono::steady_clock::now() < deadline);
        check(runtime.zoneCount() == 1 && !runtime.pendingCount(), "actual water mesh job installs into the headless runtime");
        std::vector<DrawItem> opaque, water;
        runtime.collect(opaque, water);
        std::set<int32_t> bases;
        bool anchors = water.size() == 4;
        for (const auto &draw : water) {
            anchors &= draw.mesh && draw.kind == DrawKind::Terrain && !draw.castShadow && draw.nativeModel &&
                       draw.nativeZone && draw.nativeZone->meshed && draw.nativePosition && draw.nativeLightZone &&
                       *draw.nativeLightZone == steampoints::ZoneKey{0, 0};
            if (draw.nativePosition) {
                anchors &= (*draw.nativePosition)[0] == 0 && (*draw.nativePosition)[1] == 0;
                bases.insert(int32_t((*draw.nativePosition)[2] / 65536));
            }
        }
        check(anchors && bases == std::set<int32_t>{37, 287, 787, 1037},
              "runtime water-only and mixed bands retain fixed anchors, zone lights and no-shadow policy");
        runtime.update({2048, 2048, 38});
        opaque.clear(); water.clear();
        runtime.collect(opaque, water);
        check(water.empty(), "retiring the source zone removes every installed water-band draw");
    }
    renderer.shutdown();
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

    lilyOrder(check);
    std::map<int32_t, Mesh> referenceWater;
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
        size_t indices = 0, waterIndices = 0;
        std::multiset<Corner> localWater;
        bool bounded = true, boundary = false, waterBoundary = false, waterOnly = false;
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
            waterOnly |= band.base == 1037 && !band.water.empty() && band.opaque.empty() && band.lava.empty();
            waterIndices += band.water.indices.size();
            if (!band.water.empty()) referenceWater.emplace(band.base, band.water);
            for (const auto &v : band.water.vertices) {
                bounded &= v.x >= 0 && v.x <= 64 && v.y >= 0 && v.y <= 64 && v.z >= 0 && v.z <= 250;
                waterBoundary |= band.base == 37 && v.z == 250;
                check(v.normal == 4 && (v.flags & (WATER_SURFACE | STEAM_VERTEX)) == (WATER_SURFACE | STEAM_VERTEX),
                      "all water face directions retain positive-Z lighting and native packed flags");
                localWater.insert(corner(v, band.base));
            }
        }
        check(bounds && bounds->lowest == 37 && bases == std::vector<int32_t>{37, 287, 787, 1037},
              "empty intermediate band is omitted while retained bases and order survive");
        check(bounded && boundary && opaque.empty() && lava.empty(), "source-block ownership keeps top250 in its original local band");
        check(water.empty() && waterBoundary && waterOnly,
              "water top250 stays in its source band and water-only groups survive publication");
        Mesh legacy, legacyWater, legacyLava;
        std::optional<scenevisibility::Bounds> oldBounds;
        check(world.mesh(0, 0, legacy, legacyWater, &oldBounds, nullptr, &legacyLava), "aggregate compatibility mesher remains available");
        std::multiset<Corner> aggregate;
        for (const auto *mesh : {&legacy, &legacyLava})
            for (const auto &v : mesh->vertices)
                aggregate.insert(corner(v));
        check(local == aggregate && indices == legacy.indices.size() + legacyLava.indices.size(),
              "local packing retains every physical colored corner and face count");
        std::multiset<Corner> aggregateWater;
        for (const auto &v : legacyWater.vertices)
            aggregateWater.insert(corner(v));
        check(!legacyWater.empty() && localWater == aggregateWater && waterIndices == legacyWater.indices.size(),
              "local water groups retain ordinary faces, wet film, bytes and triangle counts");
        check(bounds && oldBounds && bounds->triangleCost == oldBounds->triangleCost,
              "native adaptive triangle cost includes the band index arrays");
        runtimeBands(world, check);
    }
    const auto farDirectory = directory / "far";
    std::filesystem::create_directory(farDirectory);
    std::ofstream(farDirectory / "manifest.txt") << "seed=0\nhomeX=0\nhomeY=0\nhomeZ=37\n";
    constexpr int32_t farZone = 262144; // 16,777,216 blocks from the source-home frame.
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
            fixture(farDirectory, farZone + x, y, farZone);
    fixture(farDirectory, 0, 0); // The read-only capture format requires its source-home zone too.
    {
        ExtractedWorld far;
        std::string error;
        Mesh opaque, water, lava;
        std::optional<scenevisibility::Bounds> bounds;
        std::vector<TerrainMeshBand> bands;
        check(far.open(farDirectory, error) && far.mesh(farZone, 0, opaque, water, &bounds, nullptr, &lava, &bands),
              "large-coordinate native water fixture meshes");
        size_t compared = 0;
        bool exact = water.empty();
        for (const auto &band : bands) {
            if (band.water.empty()) continue;
            const auto found = referenceWater.find(band.base);
            exact &= found != referenceWater.end();
            if (found != referenceWater.end()) {
                const auto &reference = found->second;
                exact &= reference.indices == band.water.indices && reference.vertices.size() == band.water.vertices.size();
                if (reference.vertices.size() == band.water.vertices.size())
                    exact &= std::memcmp(reference.vertices.data(), band.water.vertices.data(), reference.vertices.size() * sizeof(Vertex)) == 0;
            }
            ++compared;
        }
        check(exact && compared == referenceWater.size(),
              "far water keeps exact local unit corners, packed bytes and topology instead of rounded source-home positions");
    }
    const auto seaDirectory = directory / "sea";
    std::filesystem::create_directory(seaDirectory);
    std::ofstream(seaDirectory / "manifest.txt") << "seed=0\nhomeX=0\nhomeY=0\nhomeZ=-250\n";
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
            seaFixture(seaDirectory, x, y);
    {
        ExtractedWorld sea;
        std::string error;
        Mesh opaque, water, lava;
        std::optional<scenevisibility::Bounds> bounds;
        std::vector<TerrainMeshBand> bands;
        check(sea.open(seaDirectory, error) && sea.mesh(0, 0, opaque, water, &bounds, nullptr, &lava, &bands),
              "whole-sea fixture meshes against a negative band anchor");
        size_t vertices = 0, indices = 0;
        bool exact = water.empty() && bounds && bounds->lowest == -251;
        for (const auto &band : bands) {
            vertices += band.water.vertices.size();
            indices += band.water.indices.size();
            for (const auto &v : band.water.vertices)
                exact &= band.base == -1 && v.z == 2 && v.r == 63 && v.g == 127 && v.b == 127 && v.ao == 255 && v.normal == 4;
        }
        check(exact && vertices == 4 && indices == 6,
              "whole-sea quad stays in the zero-cell band at native height1 with direct V255");
    }
    setRenderStyle(previousStyle);
    if (std::filesystem::equivalent(directory.parent_path(), std::filesystem::temp_directory_path()) &&
        directory.filename().string().rfind("CubeVulkan-terrain-bands-", 0) == 0)
        std::filesystem::remove_all(directory);
    std::printf("Native terrain mesh bands: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
