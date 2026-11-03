// Independent formula vectors, plus the explicit surface-stage/data-layout boundary.
// Formula checks establish contract conformance; native runtime oracles remain separate.
#include "alphagen/Terrain.h"
#include "alphagen/Coordinates.h"
#include "alphagen/SurfaceStore.h"
#include "alphagen/SteamBridge.h"
#include "alphagen/ReliefStore.h"
#include "alphagen/Trees.h"
#include "CubModel.h"
#include <stdexcept>
#include <future>
#include <cmath>
#include <cstdio>

int alphaTerrainTests() {
    int failures = 0;
    auto check = [&](bool pass, const char *what) {
        if (!pass) {
            std::printf("FAIL Alpha terrain: %s\n", what);
            ++failures;
        }
    };
    alphacoords::Mapping mapping{{0, 0}, {235, 235}};
    const auto patches = mapping.zone(0, 0);
    check(patches && patches->count == 4, "Steam zone spans four Alpha parents");
    int covered = 0;
    if (patches)
        for (size_t i = 0; i < patches->count; ++i)
            covered += patches->patches[i].width * patches->patches[i].height;
    check(covered == 4096 && !mapping.point(-1000, -1000), "finite nonwrapping Alpha coverage");
    alphagen::Random random(123);
    check(random.draw() == 440 && random.draw() == 19053 && random.draw() == 23075, "native random stream");
    const auto offsets = alphagen::makeSeedOffsets(123);
    check(offsets.A() == 440 && offsets[0x168] == 19053 && offsets[0x178] == 25656 && offsets[0x274] == 26554,
          "seed offsets preserve non-address draw order");
    check(alphagen::noise(0, 0) == -0.28179097175598145f && alphagen::noise(1.25, 2.75) == 0.14833128452301025f &&
              alphagen::noise(-0.3, 0) == -0.3548884093761444f,
          "float noise words including negative fractional truncation");
    alphagen::World world(123);
    const auto *home = world.makeCell(512, 512);
    check(home && home->x == 8397824 && home->y == 8397824 && home->elevation == 74 && home->moisture == 0.4915097653865814f &&
              home->warmth == 0.5601367354393005f,
          "home region cell and climate");
    alphagen::Terrain terrain(world);
    check(!terrain.generateSurface(-1, 32800) && !terrain.generateSurface(65536, 32800), "finite Alpha zone domain");
    auto stage = terrain.generateSurface(32800, 32800);
    check(stage && stage->zone && stage->paddedHeights.size() == 257 * 257, "complete padded surface stage");
    if (stage && stage->zone) {
        const auto &zone = *stage->zone;
        check(zone.zx == 32800 && zone.zy == 32800 && zone.columns.size() == 256 * 256, "Alpha zone is not a Steam 64-column zone");
        check(stage->paddedHeights[256 * 257] == terrain.height(zone.originX() + 256, zone.originY()) &&
                  stage->paddedHeights[256] == terrain.height(zone.originX(), zone.originY() + 256),
              "padded far row and column use absolute coordinates");
        bool valid = true;
        for (const auto &column : zone.columns)
            valid &= !column.blocks.empty() && std::isfinite(column.moisture) && std::isfinite(column.warmth);
        check(valid, "every surface column retains blocks and climate");
        check(zone.shadedLight.empty() && zone.plants.empty() && zone.statics.empty(),
              "surface stage excludes later lighting and placement");
    }
    // Alpha type13 is glowing, while Steam type13 is road. Flags are different too.
    const auto light = alphagen::steamBlock({40, 50, 60, uint8_t(alphagen::GLOWING | alphagen::KEEP)});
    check(light && light->type == worldgen::blocktype::LIGHT && light->glow == 255 && !worldgen::isOpen(*light),
          "glowing Alpha material is not a Steam road");
    const auto dry = alphagen::steamBlock({255, 255, 255, uint8_t(alphagen::AIR | alphagen::DRY_AIR | alphagen::KEEP)});
    const auto plain = alphagen::steamBlock({255, 255, 255, alphagen::AIR});
    worldgen::ZoneTerrain nativeRead;
    nativeRead.columns.resize(worldgen::ZONE_COLUMNS);
    nativeRead.columns[0].base = -1;
    if (dry && plain)
        nativeRead.columns[0].blocks = {*dry, *plain};
    check(dry && plain && nativeRead.read(0, 0, -1).type == worldgen::blocktype::AIR &&
              nativeRead.read(0, 0, 0).type == worldgen::blocktype::WATER && dry->flags == 2,
          "translated dry-air flag preserves native effective underwater reads");
    check(!alphagen::steamBlock({0, 0, 0, 31}), "unknown Alpha materials are rejected rather than reinterpreted");
    const auto wood = alphagen::steamBlock({20, 30, 40, uint8_t(alphagen::WOOD_TYPE | alphagen::SURFACE_COLOURED)});
    const auto leaves = alphagen::steamBlock({50, 60, 70, uint8_t(alphagen::LEAVES_TYPE | alphagen::SURFACE_COLOURED)});
    check(wood && leaves && wood->type == worldgen::blocktype::WOOD && wood->flags == 0 && leaves->type == worldgen::blocktype::LEAVES &&
              leaves->flags == 1 && leaves->g == 60,
          "tree RGB and explicit Steam material metadata survive the Alpha representation bridge");
    {
        alphagen::SurfaceStore source(123, {{0, 0}, {alphacoords::HOME + 107, alphacoords::HOME + 107}});
        std::string why = "old diagnostic";
        const auto tile = alphagen::steamTile(source, 0, 0, why);
        check(tile && why.empty(), "four-parent surface crop converts into one Steam-sized tile");
        if (tile) {
            check(tile->covers(0, 0) && tile->covers(63, 63) && !tile->covers(64, 0), "Steam tile coverage uses source coordinates");
            bool matching = true, axisWitness = false;
            for (int x = 0; x < 64; ++x)
                for (int y = 0; y < 64; ++y) {
                    const auto original = source.column(x, y), transposed = source.column(y, x);
                    if (!original || !transposed) {
                        matching = false;
                        continue;
                    }
                    const auto &a = original->raw();
                    const auto &b = tile->columns()[worldgen::ZoneTerrain::index(x, y)];
                    matching &= a.bottom == b.base && a.blocks.size() == b.blocks.size();
                    for (size_t i = 0; i < a.blocks.size() && i < b.blocks.size(); ++i)
                        matching &= a.blocks[i].r == b.blocks[i].r && a.blocks[i].g == b.blocks[i].g && a.blocks[i].b == b.blocks[i].b;
                    axisWitness |= a.bottom != transposed->raw().bottom || a.blocks.size() != transposed->raw().blocks.size();
                }
            check(matching && axisWitness, "all 4096 converted columns preserve geometry and RGB across an asymmetric four-parent crop");
            for (const auto point : {alphacoords::Point{17, 53}, alphacoords::Point{4, 29}, alphacoords::Point{63, 0}}) {
                const auto original = source.column(point.x, point.y);
                if (!original) {
                    check(false, "mapped source column is available");
                    continue;
                }
                const int z = original->raw().top() - 1;
                const auto a = original->block(z);
                const auto b = tile->read(point.x, point.y, z);
                check(a.r == b.r && a.g == b.g && a.b == b.b && alphagen::clear(a.type()) == worldgen::isOpen(b),
                      "Alpha XY-major and Steam X-major column indexing preserve sampled voxel identity");
            }
        }
        check(!alphagen::steamTile(source, INT32_MAX, 0, why) && !why.empty(), "invalid framing cannot publish a partial converted tile");
        alphagen::SteamTile absent(0, 0);
        check(absent.read(0, 0, 100).type == worldgen::blocktype::SOLID,
              "uncovered Alpha columns are missing-domain filler, not implicit sea or air");
    }
    // Independent caves-contract vectors and behavioral checks of the real voxel writers.
    check(alphagen::formationTop(8397000, 8397100, 180, 25, false) == 211 && alphagen::formationTop(8397000, 8397100, 180, 12, true) == 195,
          "mound and mesa tops use their different noise scales");
    const auto moundUV = alphagen::formationUV(alphagen::FormationTest::Mound, 8397010, 8397100, 190, 8397000, 8397100, 24, 22);
    const auto mesaUV = alphagen::formationUV(alphagen::FormationTest::MesaFill, 8397010, 8397100, 190, 8397000, 8397100, 24, 22);
    check(moundUV.x == 0.39192742109298706f && moundUV.y == 0.073123499751091f && mesaUV.x == 0.8065943717956543f &&
              mesaUV.y == -0.09695065766572952f,
          "formation inside tests preserve specified float intermediates");
    check(alphagen::roadShade(8397000, 8397100) == 144.53866577148438f && alphagen::roadShade(8397002, 8397101) == 160.3988800048828f,
          "road cobbles use integer lattice coordinates");
    const auto pool = alphagen::terraceAt(12.f), raised = alphagen::terraceAt(13.9f);
    check(pool.zb == 10 && pool.zlow == 8 && pool.zw == 10 && pool.a == 0.800000011920929f && raised.zb == 10 && raised.zlow == 13 &&
              raised.zw == 11 && raised.a == -0.22559940814971924f,
          "terrace pools and raised floors retain five-block step rounding");
    const auto flat = [] {
        alphagen::Zone zone;
        zone.zx = 8397000 / 256;
        zone.zy = 8397100 / 256;
        for (auto &column : zone.columns) {
            column.bottom = 180;
            column.blocks = {{30, 80, 40, alphagen::GRASS}};
            column.moisture = column.warmth = 0.5f;
        }
        return zone;
    };
    const auto fingerprint = [](const alphagen::Zone &zone) {
        uint64_t hash = 14695981039346656037ull;
        const auto mix = [&](uint64_t value) { hash = (hash ^ value) * 1099511628211ull; };
        for (const auto &column : zone.columns) {
            mix(uint32_t(column.bottom));
            mix(column.blocks.size());
            for (const auto block : column.blocks)
                mix(uint32_t(block.r) | (uint32_t(block.g) << 8) | (uint32_t(block.b) << 16) | (uint32_t(block.typeByte) << 24));
        }
        return hash;
    };
    {
        auto zone = flat();
        terrain.formation(zone, 8397000, 8397100, 180, 24, 22, 25, false);
        check(zone.read(8397010, 8397100, 190).type() == alphagen::ROCK && zone.column(8396930, 8397100).top() == 181,
              "mounds add rock above the ground and leave distant columns unchanged");
        auto blocked = flat();
        blocked.place(8397010, 8397100, 190, {1, 2, 3, uint8_t(alphagen::SOLID | alphagen::KEEP)});
        const auto before = fingerprint(blocked);
        terrain.formation(blocked, 8397000, 8397100, 180, 24, 22, 25, false);
        check(fingerprint(blocked) == before, "one protected block aborts the whole formation before any writes");
        auto mesa = flat();
        terrain.formation(mesa, 8397000, 8397100, 180, 24, 22, 12, true);
        check(mesa.read(8397010, 8397100, 190).type() == alphagen::FIELD, "layered mesas use field material below their cap");
        auto dry = flat();
        dry.place(8397000, 8397100, 190, {255, 255, 255, alphagen::DRY_AIR});
        terrain.boulder(dry, {8397000, 8397100, 190}, 3, 3, 3, false);
        check(dry.read(8397000, 8397100, 190).typeByte == alphagen::DRY_AIR, "terrace boulders preserve dry-air cells");
        terrain.boulder(dry, {8397000, 8397100, 190}, 3, 3, 3, true);
        check(dry.read(8397000, 8397100, 190).typeByte == (alphagen::ROCK | alphagen::SURFACE_COLOURED),
              "ravine boulders use the specified different dry-air and surface-flag rules");
    }
    {
        std::string why = "previous failure";
        const auto relief = terrain.generateRelief(32800, 32800, why);
        check(relief && why.empty(), "rock-road-terrace boundary publishes a complete immutable stage");
        if (relief && stage) {
            check(relief->random.count() >= stage->random.count(), "relief continues the existing surface random stream");
            check(fingerprint(*relief->zone) != fingerprint(*stage->zone), "post-surface terrain changes actual voxel columns");
        }
        check(!terrain.generateRelief(-1, 0, why) && !why.empty(), "invalid relief zones publish no partial stage");
        bool treeChecked = false;
        for (int rx = 511; rx <= 513 && !treeChecked; ++rx)
            for (int ry = 511; ry <= 513 && !treeChecked; ++ry) {
                const auto *region = world.region(rx, ry);
                if (!region)
                    continue;
                for (const auto &location : region->records)
                    if (location.type == 12) {
                        const int zx = int(location.x >> 16) / 256, zy = int(location.y >> 16) / 256;
                        const auto giant = terrain.generateRelief(zx, zy, why);
                        size_t woodCount = 0, leafCount = 0;
                        if (giant)
                            for (const auto &column : giant->zone->columns)
                                for (const auto block : column.blocks) {
                                    woodCount += block.type() == alphagen::WOOD_TYPE;
                                    leafCount += block.type() == alphagen::LEAVES_TYPE;
                                }
                        check(giant && why.empty() && woodCount > 0 && leafCount > 0,
                              "giant-tree locations produce their trunk roots branches and crown");
                        treeChecked = true;
                        break;
                    }
            }
        check(treeChecked, "giant-tree terrain fixture is exercised");
    }
    {
        alphagen::detail::ParentStages<int> cache(1);
        int attempts = 0;
        bool failed = false;
        try {
            cache.get(0, 0, [&]() -> std::shared_ptr<const int> {
                ++attempts;
                throw std::runtime_error("expected generation failure");
            });
        } catch (const std::runtime_error &) {
            failed = true;
        }
        const auto retried = cache.get(0, 0, [&] {
            ++attempts;
            return std::make_shared<const int>(7);
        });
        const auto reused = cache.get(0, 0, [&] {
            ++attempts;
            return std::make_shared<const int>(8);
        });
        check(failed && attempts == 2 && retried == reused && *reused == 7,
              "failed parent loads can be retried and completed loads are reused");
    }
    {
        std::optional<alphagen::ColumnLease> lease;
        std::weak_ptr<const alphagen::ReliefStage> owner;
        int top = 0;
        {
            alphagen::ReliefStore source(123, {}, 1);
            auto first = std::async(std::launch::async, [&] { return source.parent(32800, 32800); });
            auto second = std::async(std::launch::async, [&] { return source.parent(32800, 32800); });
            const auto a = first.get(), b = second.get();
            check(a && a == b, "concurrent terrain consumers reuse the same completed relief parent");
            owner = a;
            lease = source.column(0, 0);
            std::string error;
            const auto native = alphagen::steamTile(source, 0, 0, error);
            check(native && error.empty() && lease && native->columns()[0].base == lease->raw().bottom &&
                      native->columns()[0].blocks.size() == lease->raw().blocks.size(),
                  "Steam-sized consumers receive the relief stage columns through the shared adapter");
            if (lease)
                top = lease->raw().top();
            check(source.parent(32801, 32800) != a, "relief parents obey the bounded ready-cache capacity");
        }
        check(lease && !owner.expired() && lease->raw().top() == top, "relief column lease survives eviction and store destruction");
        lease.reset();
        check(owner.expired(), "last relief column lease releases the parent stage");
    }
    {
        alphagen::Zone zone;
        for (auto &column : zone.columns) {
            column.moisture = 0.45f;
            column.warmth = 0.5f;
        }
        alphagen::Random stream(123);
        alphagen::TreeBuilder builder(terrain, zone, stream);
        builder.build(100, 200, 50, 8, 10, 0);
        size_t woodCount = 0, leafCount = 0;
        for (const auto &column : zone.columns)
            for (const auto block : column.blocks) {
                woodCount += block.type() == alphagen::WOOD_TYPE;
                leafCount += block.type() == alphagen::LEAVES_TYPE;
            }
        const auto trunk = zone.read(100, 200, 50), leaf = zone.read(104, 204, 62);
        check(stream.count() == 57 && stream.draw() == 27840 && woodCount == 332 && leafCount == 961,
              "independent round-tree example matches draw stream and distinct wood/leaf counts");
        check(trunk.r == 218 && trunk.g == 177 && trunk.b == 98 && trunk.typeByte == 0x27 && leaf.r == 53 && leaf.g == 220 &&
                  leaf.b == 60 && leaf.typeByte == 0x28,
              "independent round-tree example matches mossy bark and leaf colors");
        alphagen::Zone dry;
        for (auto &column : dry.columns) {
            column.moisture = 0.45f;
            column.warmth = 0.5f;
        }
        dry.place(100, 200, 50, {255, 255, 255, alphagen::DRY_AIR});
        alphagen::Random repeat(123);
        alphagen::TreeBuilder(terrain, dry, repeat).build(100, 200, 50, 8, 10, 0);
        check(dry.read(100, 200, 50).typeByte == alphagen::DRY_AIR, "tree writes preserve reserved dry-air cells");
        bool palmRejected = false;
        const auto before = repeat.count();
        try {
            alphagen::TreeBuilder(terrain, dry, repeat).build(100, 200, 50, 8, 10, 3);
        } catch (const std::invalid_argument &) {
            palmRejected = true;
        }
        check(palmRejected && repeat.count() == before, "deferred palm stamps fail before any partial tree or random draws");
    }
    {
        auto model = std::make_shared<CubModel>();
        model->sx = 3;
        model->sy = 5;
        model->sz = 1;
        for (int j = 0; j < 5; ++j)
            for (int i = 0; i < 3; ++i) {
                model->rgb.push_back(uint8_t(10 + i));
                model->rgb.push_back(uint8_t(20 + j));
                model->rgb.push_back(30);
            }
        auto models = std::make_shared<const alphagen::TreeModels>(alphagen::TreeModels{model, {}});
        alphagen::Terrain palms(world, models);
        alphagen::Zone zone;
        for (auto &column : zone.columns) {
            column.moisture = 0.45f;
            column.warmth = 0.5f;
        }
        alphagen::Random stream(123), heights(123);
        for (int i = 0; i < 11; ++i)
            heights.draw();
        int z[4];
        for (auto &value : z)
            value = 46 + heights.draw() % 3;
        alphagen::TreeBuilder(palms, zone, stream).build(100, 200, 50, 8, 1, 3);
        const auto north = zone.read(99, 205, z[0]), east = zone.read(105, 201, z[1]);
        const auto south = zone.read(99, 195, z[2]), west = zone.read(95, 201, z[3]);
        check(stream.count() == 16 && north.r == 10 && north.g == 24 && east.r == 10 && east.g == 24 && south.r == 12 && south.g == 24 &&
                  west.r == 12 && west.g == 24 && north.typeByte == 0x68,
              "owner palm stamps rotate without mirroring and draw heights only for present models");
        auto both = std::make_shared<const alphagen::TreeModels>(alphagen::TreeModels{model, model});
        alphagen::Terrain fullPalms(world, both);
        alphagen::Zone second;
        alphagen::Random full(123);
        alphagen::TreeBuilder(fullPalms, second, full).build(100, 200, 50, 8, 1, 3);
        check(full.count() == 20, "both palm leaf models make eight ordered height draws and no marker draws");
        std::string why;
        check(!alphagen::loadTreeModels("build/alpha-missing-owner-models", why) && !why.empty(),
              "owner palm loader reports missing models without synthesizing replacement assets");
    }
    stage.reset();
    std::optional<alphagen::ColumnLease> held;
    std::weak_ptr<const alphagen::SurfaceStage> pinned;
    alphagen::Block retainedBlock;
    float retainedHeight = 0;
    {
        alphagen::SurfaceStore source(123, {}, 1);
        check(!source.column(INT32_MIN, 0), "unmapped coordinates do not wrap into an Alpha parent");
        auto first = std::async(std::launch::async, [&] { return source.parent(32800, 32800); });
        auto second = std::async(std::launch::async, [&] { return source.parent(32800, 32800); });
        const auto a = first.get(), b = second.get();
        check(a && a == b, "concurrent Steam-sized consumers share one Alpha parent stage");
        pinned = a;
        held = source.column(0, 0);
        check(held && held->position().x == alphacoords::HOME, "column lease preserves absolute Alpha coordinates");
        if (held) {
            retainedBlock = held->block(0);
            retainedHeight = held->surface();
        }
        check(source.parent(32801, 32800) != a, "a different parent can replace the bounded ready-cache entry");
        // The sea cell center has an island; sample its subcell boundary farther east.
        const auto ocean = source.parent(32848, 32796);
        bool waterReadChecked = false;
        if (ocean)
            for (int x = 0; x < 256 && !waterReadChecked; ++x)
                for (int y = 0; y < 256 && !waterReadChecked; ++y) {
                    const int ax = ocean->zone->originX() + x, ay = ocean->zone->originY() + y;
                    if (ocean->zone->read(ax, ay, 0).type() != alphagen::WATER)
                        continue;
                    const auto wet = source.column(ax - alphacoords::HOME, ay - alphacoords::HOME);
                    if (wet && wet->surface(false) < 1) {
                        check(wet->surface() == 1, "water surface is the top of the effective z0 water cell");
                        waterReadChecked = true;
                    }
                }
        if (!waterReadChecked && ocean) {
            float minGrid = ocean->paddedHeights.front(), maxGrid = minGrid;
            for (auto value : ocean->paddedHeights) {
                minGrid = std::min(minGrid, value);
                maxGrid = std::max(maxGrid, value);
            }
            std::printf("Alpha water fixture grid range: %.3f .. %.3f; first bottom/top: %d/%d\n", minGrid, maxGrid,
                        ocean->zone->columns.front().bottom, ocean->zone->columns.front().top());
        }
        check(waterReadChecked, "water and solid-surface reads use different effective column products");
    }
    check(held && !pinned.expired(), "column lease survives ready-cache eviction and store destruction");
    if (held)
        check(held->block(0).typeByte == retainedBlock.typeByte && held->block(0).r == retainedBlock.r && held->surface() == retainedHeight,
              "leased block and surface remain readable without the world store");
    held.reset();
    check(pinned.expired(), "the last lease releases its parent stage");
    std::printf("Alpha surface generation core: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
