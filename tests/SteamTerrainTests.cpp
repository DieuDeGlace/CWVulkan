#include "Extracted.h"
#include "DataCheck.h"
#include "Spawned.h"
#include "AlphaLook.h"
#include <array>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <cstring>
#include <limits>

int steamTerrainTests() {
    int failures = 0;
    auto check = [&](bool result, const char *message) {
        if (!result) { std::printf("FAIL Steam terrain: %s\n", message); ++failures; }
    };
    namespace fs = std::filesystem;
    using Block = std::array<uint8_t, 6>;
    struct Column { int32_t base; std::vector<Block> blocks; };
    struct Meshes { Mesh opaque, water, lava; };
    const auto root = fs::temp_directory_path() / ("CubeVulkan-native-terrain-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directory(root);
    { std::ofstream(root / "manifest.txt") << "seed=123\nhomeX=0\nhomeY=0\nhomeZ=5\n"; }
    const auto previousStyle = renderStyle();
    setRenderStyle(RenderStyle::Steam);
    auto build = [&](const std::function<Column(int, int)> &columnAt) {
        for (int zx = -1; zx <= 1; ++zx)
            for (int zy = -1; zy <= 1; ++zy) {
                std::ofstream out(root / ("z_" + std::to_string(zx) + "_" + std::to_string(zy) + ".cwz"), std::ios::binary);
                out.write("CWZ1", 4);
                const int32_t header[]{zx, zy, 0, 0};
                out.write((const char *)header, sizeof header);
                for (int x = 0; x < 64; ++x)
                    for (int y = 0; y < 64; ++y) {
                        const auto column = columnAt(zx * 64 + x, zy * 64 + y);
                        const uint32_t runs = (uint32_t)column.blocks.size();
                        out.write((const char *)&column.base, 4); out.write((const char *)&runs, 4);
                        for (const auto &block : column.blocks) {
                            const uint16_t count = 1;
                            out.write((const char *)&count, 2); out.write((const char *)block.data(), 6);
                        }
                    }
                check(bool(out), "native terrain fixture written");
            }
        ExtractedWorld world;
        std::string error;
        Meshes meshes;
        std::optional<scenevisibility::Bounds> bounds;
        worldgen::MeshLightVolume light;
        check(world.open(root, error) && world.mesh(0, 0, meshes.opaque, meshes.water, &bounds, &light, &meshes.lava),
              "complete terrain neighborhood meshes");
        check(bounds.has_value() && !light.columns.empty(), "mesh metadata and retained light share the input snapshot");
        return meshes;
    };
    const Block white{255,255,255,0,1,0};
    const auto flat = build([&](int, int) { return Column{3, {white}}; });
    check(flat.opaque.vertices.size() == 4096 * 4 && flat.water.empty() && flat.lava.empty(),
          "flat opaque zone emits only its top faces");
    bool whiteEncoded = !flat.opaque.empty();
    for (const auto &v : flat.opaque.vertices)
        whiteEncoded &= v.r == 255 && v.g == 255 && v.b == 255 && v.ao == 254 && v.normal == 4 && (v.flags & STEAM_VERTEX);
    check(whiteEncoded, "native terrain preserves P2 and exact averaged V254 independently");
    if (flat.opaque.indices.size() >= 6)
        check(std::vector<uint32_t>(flat.opaque.indices.begin(), flat.opaque.indices.begin() + 6) ==
              std::vector<uint32_t>({0,1,3,2,3,1}), "equal-color terrain top takes native1-3 split");

    const auto special = build([&](int x, int y) {
        auto block = white;
        if (y == 30 && x >= 30 && x <= 33)
            block[4] = x == 30 ? 17 : x == 31 ? 19 : x == 32 ? 3 : 20;
        return Column{3, {block}};
    });
    check(special.lava.vertices.size() == 4 && special.opaque.vertices.size() == 4095 * 4,
          "lava separates from poison/wet/light ordinary group with shared interfaces hidden");
    bool noGlow = true, onlyTops = true, lightBypass = false;
    for (const Mesh *mesh : {&special.opaque, &special.lava})
        for (const auto &v : mesh->vertices) {
            noGlow &= !(v.flags & GLOW);
            onlyTops &= v.normal == 4;
            if (v.x == 34 && v.y == 31)
                lightBypass |= v.ao == 255;
        }
    check(noGlow && onlyTops && lightBypass, "special types retain cube normals; type20 V1 bypass does not imply Glow");
    check(special.water.vertices.size() == 4, "wet ground adds only its coplanar water top film");
    for (const auto &v : special.water.vertices)
        check(v.z == ExtractedWorld::Z_OFFSET + 4 && v.normal == 4 && v.r == 63 && v.g == 127 && v.b == 127 && v.ao == 254,
              "wet film uses native sampled water RGB/V");

    auto seaColumn = [&](int, int) { return Column{-1, {white}}; };
    const auto sea = build([&](int x, int y) {
        if (x == 64 && y == 20)
            return Column{-1, {white, Block{0,0,0,0,0,2}}}; // air beside targetZ0 water
        return seaColumn(x, y);
    });
    check(sea.water.vertices.size() == 4 && sea.water.indices == std::vector<uint32_t>({0,1,2,0,2,3}),
          "whole-sea quad uses fixed split and suppresses all ordinaryZ0 water faces, including side faces");
    const glm::vec2 seaCorners[]{{0,0}, {64,0}, {64,64}, {0,64}};
    for (size_t i = 0; i < std::min(size_t(4), sea.water.vertices.size()); ++i) {
        const auto &v = sea.water.vertices[i];
        check(v.x == seaCorners[i].x && v.y == seaCorners[i].y && v.z == ExtractedWorld::Z_OFFSET + 1 &&
              v.r == 63 && v.g == 127 && v.b == 127 && v.ao == 255 && v.normal == 4,
              "optimized sea corners use direct base water RGBA, not averaged V");
    }
    const auto storedSea = build([&](int, int) {
        return Column{-1, {white, Block{0,0,0,0,2,0}, Block{0,0,0,0,0,0}}};
    });
    check(storedSea.water.vertices.size() == 4, "post-solver red overlay admits stored air that was initially unlit");
    auto roofSea = [&](uint8_t roofRed) {
        return build([&](int x, int y) {
            auto air = Block{0,0,0,0,0,0};
            if (x == 5 && y == 5)
                air = {roofRed,255,255,0,1,0};
            return Column{-1, {white, Block{0,0,0,0,2,0}, air}};
        });
    };
    check(roofSea(255).water.vertices.size() == 4, "sea eligibility checksZ1 red byte, not air type");
    check(roofSea(254).water.vertices.size() == 4095 * 4, "a non255 solid red byte disables the whole-zone optimization");
    // Auxiliary light records are independent copies, not the live draw lists.
    // A poisoned captured +0x28 must be replaced by this mesh's light solve.
    auto writeLights = [&](bool moved, bool duplicate, bool wrongStride) {
        std::ofstream out(root / "z_0_0.rec", std::ios::binary);
        out.write("CWR1", 4);
        auto block = [&](uint32_t kind, uint32_t stride, uint32_t count, const std::vector<uint8_t> &bytes) {
            const uint32_t header[]{kind, stride, count};
            out.write((const char *)header, sizeof header);
            out.write((const char *)bytes.data(), bytes.size());
        };
        auto store = [](auto &bytes, size_t offset, auto value) {
            std::memcpy(bytes.data() + offset, &value, sizeof value);
        };
        std::vector<uint8_t> model(2 * 0x40);
        store(model, 0, int32_t(0x4c));
        store(model, 24, int64_t(moved ? 5 * 65536 : 0));
        store(model, 40, std::numeric_limits<float>::quiet_NaN());
        store(model, 44, .2f); store(model, 48, .4f); store(model, 52, .6f);
        store(model, 0x40, int32_t(20));
        store(model, 0x40 + 24, int64_t(5 * 65536));
        store(model, 0x40 + 56, int32_t(1));
        block(0x18, 0x40, 1, std::vector<uint8_t>(model.begin(), model.begin() + 0x40));
        block(0x90, wrongStride ? 0x41 : 0x40, 2, model);
        if (duplicate)
            block(0x90, 0x40, 0, {});
        std::vector<uint8_t> object(0xe0);
        store(object, 0, int32_t(0x3f)); store(object, 36, 1.5f);
        block(0xc0, 0xe0, 1, object);
        block(0x60, 0xe0, 1, object); // duplicate live object does not add another source
    };
    writeLights(false, false, false);
    ExtractedWorld lightWorld;
    std::string lightError;
    Mesh lightOpaque, lightWater;
    worldgen::MeshLightVolume solved;
    check(lightWorld.open(root, lightError) && lightWorld.mesh(0, 0, lightOpaque, lightWater, nullptr, &solved),
          "source-record fixture has a complete retained light solve");
    const auto sources = lightWorld.lightSources(0, 0, solved);
    check(sources && sources->key == steampoints::ZoneKey(0,0) && sources->models.size() == 2 && sources->objects.size() == 1,
          "only auxiliary lists provide light sources; origin-position/null-marker records survive");
    if (sources && sources->models.size() == 2 && sources->objects.size() == 1) {
        check(sources->models[0].record.type == 0x4c && sources->models[0].bakedLight == 255 &&
              (sources->models[0].record.flags & 8), "auxiliary light/water writes replace poisoned capture bytes");
        check(!(sources->models[1].record.flags & 8) && sources->objects[0].scale == 1.5f,
              "dry auxiliary models and native object scales retain distinct inputs");
    }
    writeLights(true, false, false);
    const auto moved = lightWorld.lightSources(0, 0, solved);
    check(moved && !moved->models.empty() && (moved->models[0].record.flags & 8),
          "auxiliary water flag is sticky independently of the live model list");
    const auto loaded = lightWorld.loadedLightZones();
    check(loaded.size() >= 9, "light binding includes loaded neighboring zones before they have their own meshes");
    const auto lightReport = dataCheck::inspect(root, root, true);
    check(lightReport.text().find("1/9 zones have auxiliary light snapshots") != std::string::npos,
          "data check distinguishes refreshed light snapshots from older exports");
    // A live packet replacement feeds the light adapter; pickup empties that
    // same slot immediately, without relying on the captured placement list.
    SpawnedCreatures liveItems;
    cwnet::ZoneObjects itemPacket;
    itemPacket.key = {0,0};
    itemPacket.records.resize(1);
    auto &rawItem = itemPacket.records[0];
    rawItem[0] = 0x0b;
    const int32_t itemId = 0x13, timer = -7, quantity = 3;
    const int64_t owner = 99;
    const steampoints::Point itemPosition{123,456,789};
    std::memcpy(rawItem.data() + 4, &itemId, 4);
    std::memcpy(rawItem.data() + 0xa0, itemPosition.data(), sizeof itemPosition);
    std::memcpy(rawItem.data() + 0xcc, &timer, 4);
    std::memcpy(rawItem.data() + 0xd4, &quantity, 4);
    std::memcpy(rawItem.data() + 0xd8, &owner, 8);
    liveItems.replaceGroundItems(lightWorld, itemPacket);
    const auto *borrowedGround = liveItems.findGroundItem({0, 0, 0});
    check(borrowedGround && (*borrowedGround)[0] == 0x0b && !liveItems.findGroundItem({0, 0, -1}) && !liveItems.findGroundItem({0, 0, 1}) &&
              !liveItems.findGroundItem({10000, 10000, 0}),
          "preview resolves only an existing zone and in-range ground slot");
    const auto ground = liveItems.groundLights(lightWorld, 0, 0);
    check(ground.items.size() == 1, "live item packet supplies ground-light inputs");
    if (ground.items.size() == 1) {
        const auto &item = ground.items[0];
        check(item.category == 0x0b && item.id == 0x13 && item.position == itemPosition &&
              item.timerMs == -7 && item.quantity == 3 && item.owner == 99,
              "ground-light adapter reads live item identity, position, timer, quantity and owner");
    }
    liveItems.takeGroundItem(0, 0, 0);
    const auto *clearedGround = liveItems.findGroundItem({0, 0, 0});
    check(clearedGround && clearedGround == borrowedGround && (*clearedGround)[0] == 0,
          "fresh preview resolution retains the live slot after pickup without a type/quantity admission filter");
    const auto taken = liveItems.groundLights(lightWorld, 0, 0);
    check(taken.items.size() == 1 && taken.items[0].category == 0, "pickup clears the item used by light registration");
    writeLights(false, true, false);
    check(!lightWorld.lightSources(0, 0, solved), "duplicate auxiliary list is rejected");
    writeLights(false, false, true);
    check(!lightWorld.lightSources(0, 0, solved), "wrong auxiliary stride is rejected");
    setRenderStyle(previousStyle);
    fs::remove_all(root);
    std::printf("Steam terrain: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
