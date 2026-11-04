// Independent formula vectors, plus the explicit surface-stage/data-layout boundary.
// Formula checks establish contract conformance; native runtime oracles remain separate.
#include "alphagen/Terrain.h"
#include "AlphaLight.h"
#include "alphagen/Coordinates.h"
#include "alphagen/SurfaceStore.h"
#include "alphagen/SteamBridge.h"
#include "alphagen/ReliefStore.h"
#include "alphagen/LandscapeStore.h"
#include "alphagen/Trees.h"
#include "CubModel.h"
#include "worldgen/AlphaTerrain.h"
#include "worldgen/MapModel.h"
#include "Extracted.h"
#include <bit>
#include <optional>
#include <stdexcept>
#include <future>
#include <barrier>
#include <fstream>
#include <chrono>
#include <tuple>
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
    {
        alphagen::World published(321);
        check(!published.cell(100, 100), "a missing immutable cell is not cached as a lasting miss");
        const auto *cell = published.makeCell(100, 100);
        check(cell && published.cell(100, 100) == cell, "cell publication becomes visible after a previous miss");
        std::vector<std::future<bool>> readers;
        for (int thread = 0; thread < 4; ++thread)
            readers.push_back(std::async(std::launch::async, [&] {
                for (int read = 0; read < 1000; ++read)
                    if (published.cell(100, 100) != cell)
                        return false;
                return true;
            }));
        for (auto &reader : readers)
            check(reader.get(), "concurrent immutable readers retain the published cell");
        check(!published.region(512, 512), "a region awaiting publication remains absent");
        alphagen::Terrain prepared(published);
        prepared.prepareRegion(512, 512);
        const auto *region = published.region(512, 512);
        check(region && published.region(512, 512) == region, "late region publication is visible and stable");

        std::optional<alphagen::World> reused;
        auto *first = &reused.emplace(123);
        check(first->makeCell(512, 512) && first->cell(512, 512), "first world lifetime warms immutable reads");
        reused.reset();
        auto *second = &reused.emplace(456);
        check(first == second && !second->cell(512, 512), "reusing a World address cannot retain a previous lifetime's cell");
        const auto *replacement = second->makeCell(512, 512);
        check(replacement && second->cell(512, 512) == replacement, "replacement world publishes its own cell");
    }
    alphagen::World world(123);
    const auto *home = world.makeCell(512, 512);
    check(home && home->x == 8397824 && home->y == 8397824 && home->elevation == 74 && home->moisture == 0.4915097653865814f &&
              home->warmth == 0.5601367354393005f,
          "home region cell and climate");
    alphagen::Terrain terrain(world);
    {
        // Original-instruction references: alpha-rock-colour-precision.md §5.1.
        // Seed offsets are real; cached climate is synthetic, not a native world capture.
        check(world.offsets()[0x22c] == 28488 && world.offsets()[0x230] == 12097,
              "native seed123 detail offsets retain their draw positions");
        check(std::bit_cast<uint32_t>(alphagen::noise(28488, 12097)) == 0xbf2d52f4u,
              "native lattice noise word at the seed123 detail offsets");
        struct RockFixture {
            float warmth, fire;
            std::array<uint32_t, 3> rgb;
        };
        constexpr RockFixture fixtures[] = {
            {0, 0, {0x4280c69cu, 0x4294c69cu, 0x42d73c21u}},     {.25f, 0, {0x430b634eu, 0x4310634eu, 0x43279e10u}},
            {.5f, 0, {0x432e634eu, 0x432e634eu, 0x433b9e10u}},   {.75f, 0, {0x432e634eu, 0x432e634eu, 0x433b9e10u}},
            {.5f, .5f, {0x42d7ace2u, 0x42be0171u, 0x42c59e10u}}, {.5f, 1, {0x4225264fu, 0x4179e234u, 0x41200000u}},
        };
        alphagen::Zone cached;
        auto &column = cached.column(0, 0);
        for (const auto &fixture : fixtures) {
            column.warmth = fixture.warmth;
            column.fire = fixture.fire;
            const auto actual = terrain.rock(0, 0, 0, &cached);
            check(std::bit_cast<uint32_t>(actual.r) == fixture.rgb[0] && std::bit_cast<uint32_t>(actual.g) == fixture.rgb[1] &&
                      std::bit_cast<uint32_t>(actual.b) == fixture.rgb[2],
                  "raw Alpha rock RGB float words match the independent cached-climate references");
        }
    }
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
    {
        alphagen::PlantModels models;
        for (int i = 0; i < int(models.environment.size()); ++i)
            models.environment[size_t(i)] = 1000 + i;
        alphagen::Plant lily{3, -4, 10, .09f, 90, 31, 0};
        const auto record = alphagen::steamPlant(lily, models);
        check(record && record->type == 1031 && record->x == 229376 && record->y == -229376 && record->z == 648807 &&
                  record->size == .09f && record->yaw == 90 && record->flags == 2 && record->groundCover,
              "Alpha lily uses half-block XY, exact minus6553 fixed Z and one uniform voxel scale");
        lily.kind = 22;
        lily.flags = 4;
        const auto reed = alphagen::steamPlant(lily, models, 5);
        check(reed && reed->flags == 4 && reed->z == 15 * 65536, "reeds overwrite constructor flags and follow the column translation");
        lily.kind = 4;
        const auto temperate = alphagen::steamPlant(lily, models);
        lily.flags = 0;
        const auto hot = alphagen::steamPlant(lily, models);
        check(temperate && hot && temperate->flags == 6 && hot->flags == 2,
              "the same kind keeps its branch-dependent sway request rather than guessing from the model name");
        lily.kind = 18;
        models.environment[18] = -1;
        check(!alphagen::steamPlant(lily, models), "missing owner plant kinds cannot produce a replacement model");

        const auto directory = std::filesystem::temp_directory_path() /
                               ("CubeVulkan-plant-registry-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directory(directory);
        std::string table = "# environment\n";
        for (int kind = 0; kind < 33; ++kind) {
            const auto name = alphagen::plantSprite(kind);
            if (name.empty())
                continue;
            table += std::to_string(1000 + kind) + " sprites/" + std::string(name) + ".cub\n";
            // The resolver checks names and file presence; CubModel owns byte validation.
            std::ofstream(directory / (std::string(name) + ".cub"), std::ios::binary).put('x');
        }
        table += "# building-part\n777 sprites/flowers2.cub\n";
        {
            std::ofstream output(directory / "sprite-groups.txt");
            output << table;
        }
        std::string error;
        const auto owner = alphagen::loadPlantModels(directory, directory, error);
        check(owner && error.empty() && owner->type(0) == 1000 && owner->type(32) == 1032,
              "owner registry maps Alpha kinds by name while ignoring other model tables");
        std::filesystem::remove(directory / "flowers2.cub");
        check(!alphagen::loadPlantModels(directory, directory, error) && !error.empty(),
              "missing owner artwork is reported without synthesis or substitution");
        for (int kind = 0; kind < 33; ++kind)
            if (!alphagen::plantSprite(kind).empty())
                std::filesystem::remove(directory / (std::string(alphagen::plantSprite(kind)) + ".cub"));
        std::filesystem::remove(directory / "sprite-groups.txt");
        std::filesystem::remove(directory);
    }
    {
        worldgen::TerrainColumn column;
        column.base = 10;
        column.blocks = {{30, 40, 50, 0, worldgen::blocktype::SOLID, 0}};
        const auto before = column;
        worldgen::ColumnFiller missing = [](int32_t) -> std::optional<worldgen::Block> { return std::nullopt; };
        check(!worldgen::storeColumn(column, 0, 0, 5, {1, 2, 3, 0, 6, 0}, {}, {}, &missing) && column.base == before.base &&
                  column.blocks == before.blocks,
              "missing source gap colors reject the entire downward extension without partial writes");
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
    {
        std::string why;
        const auto landscape = terrain.generateLandscape(32800, 32800, why);
        check(landscape && why.empty() && !landscape->zone->trees.empty(), "hybrid landscape places a voxel forest after terrain boulders");
        if (landscape) {
            const auto vegetation = terrain.vegetation(*landscape->zone, landscape->random);
            check(!vegetation.plants.empty() && vegetation.random.count() > landscape->random.count() && landscape->zone->plants.empty(),
                  "vegetation continues the retained stream without mutating published landscape storage");
            alphagen::LandscapeStore nature(123);
            const auto plants = nature.plantsForTile(0, 0);
            check(!plants.empty() && std::all_of(plants.begin(), plants.end(),
                                                 [](const auto &p) { return p.x >= 0 && p.x < 64 && p.y >= 0 && p.y < 64 && p.scale > 0; }),
                  "Alpha parent plants clip and rebase into exactly one Steam tile");
            check(nature.vegetationParent(32800, 32800) == nature.vegetationParent(32800, 32800),
                  "plant consumers share an immutable cached vegetation continuation");
            size_t wood = 0, leaves = 0;
            for (const auto &column : landscape->zone->columns)
                for (const auto block : column.blocks) {
                    wood += block.type() == alphagen::WOOD_TYPE;
                    leaves += block.type() == alphagen::LEAVES_TYPE;
                }
            check(wood > 0 && leaves > 0 && landscape->zone->statics.empty(),
                  "forest geometry exists without silently generating deferred Alpha structures");
            check(stage && landscape->random.count() > stage->random.count(),
                  "landscape terrain and trees continue a distinct completed stream");
            bool positions = true;
            for (const auto &tree : landscape->zone->trees)
                positions &= landscape->zone->contains(tree.x, tree.y) && tree.size > 0 && tree.height > 0;
            check(positions, "forest anchors stay inside their owning Alpha parent");
        }
        alphagen::LandscapeStore source(123, {}, 1);
        const auto parent = source.parent(32800, 32800);
        check(parent && landscape && fingerprint(*parent->zone) == fingerprint(*landscape->zone),
              "cached landscape generation reproduces uncached terrain and forests");
        const auto native = alphagen::steamTile(source, 0, 0, why);
        const auto home = source.column(0, 0);
        check(native && home && why.empty() && native->columns()[0].base == home->raw().bottom &&
                  native->columns()[0].blocks.size() == home->raw().blocks.size(),
              "completed landscape columns reach Steam-sized terrain consumers");
        check(!terrain.generateLandscape(65536, 0, why) && !why.empty(), "landscape finite-world rejection is explicit");
    }
    {
        const auto nativeWorld = worldgen::worldValues(0);
        auto zone = worldgen::generateZone(nativeWorld, 0, 0);
        const auto original = zone;
        worldgen::SpawnPoint point;
        point.x = point.y = 0;
        point.z = int64_t(100) * worldgen::DOTS;
        zone.spawns.points[123] = point;
        auto plantModels = std::make_shared<alphagen::PlantModels>();
        for (int i = 0; i < 33; ++i)
            plantModels->environment[size_t(i)] = 1000 + i;
        worldgen::AlphaTerrain hybrid(123, {}, 0, plantModels);
        std::string why;
        check(hybrid.compose(zone, nativeWorld, why) && why.empty(), "Alpha landscape composes with a complete Steam-generated zone");
        check(std::any_of(zone.models.begin(), zone.models.end(), [](const auto &m) { return m.groundCover && m.type >= 1000; }) &&
                  std::none_of(zone.models.begin(), zone.models.end(), [](const auto &m) { return m.groundCover && m.type < 1000; }),
              "Alpha cover replaces tagged Steam plants outside construction while preserving other model roles");
        const auto ground = [](const worldgen::TerrainColumn &column) {
            for (int i = int(column.blocks.size()) - 1; i >= 0; --i) {
                const auto &b = column.blocks[size_t(i)];
                if (!worldgen::isOpen(b) && b.type != 7 && b.type != 8 && b.type != 11)
                    return column.base + i + 1;
            }
            return column.base;
        };
        const int delta = ground(zone.terrain.columns[0]) - ground(original.terrain.columns[0]);
        check(zone.spawns.points[123].z == point.z + int64_t(delta) * worldgen::DOTS,
              "Steam spawn height follows composed terrain while XY and fractional position remain unchanged");
        const auto steamAgain = worldgen::generateZone(nativeWorld, 0, 0);
        bool unchanged = true;
        for (size_t i = 0; i < original.terrain.columns.size(); ++i)
            unchanged &= original.terrain.columns[i].base == steamAgain.terrain.columns[i].base &&
                         original.terrain.columns[i].blocks == steamAgain.terrain.columns[i].blocks;
        check(unchanged, "hybrid composition does not mutate the separate Steam generator");
        auto broken = original;
        broken.terrain.columns.resize(1);
        check(!hybrid.compose(broken, nativeWorld, why) && broken.terrain.columns.size() == 1 && !why.empty(),
              "incomplete native base rejection cannot publish partial Alpha terrain");
        check(hybrid.identity() != hybrid.identity(2) && hybrid.identity() != worldgen::AlphaTerrain(124).identity(),
              "generation identity separates seed and world-scale inputs");
        worldgen::CellObjects cells(nativeWorld);
        worldgen::BuildingRules rules(cells);
        bool constructionChecked = false;
        for (int px = -16; px <= 16 && !constructionChecked; ++px)
            for (int py = -16; py <= 16 && !constructionChecked; ++py) {
                const bool big = worldgen::bigCell(px, py);
                if (worldgen::placeKind(px, py, big) != 5)
                    continue;
                const auto town = worldgen::cellPoint(px, py, big);
                const int centerX = worldgen::floorDiv(town.x, 4), centerY = worldgen::floorDiv(town.y, 4);
                for (int dx = -12; dx <= 12 && !constructionChecked; ++dx)
                    for (int dy = -12; dy <= 12 && !constructionChecked; ++dy) {
                        const int cx = centerX + dx, cy = centerY + dy;
                        if (!worldgen::isBuilding(rules.type(cx, cy)) && rules.castleCell(cx, cy).kind == 0)
                            continue;
                        const int zx = worldgen::floorDiv(cx, 2), zy = worldgen::floorDiv(cy, 2);
                        auto built = worldgen::generateZone(nativeWorld, zx, zy);
                        const auto before = built.terrain.columns;
                        const auto originalModels = built.models;
                        const bool composed = hybrid.compose(built, nativeWorld, why);
                        bool kept = composed;
                        for (size_t i = 0; i < before.size(); ++i)
                            kept &= built.terrain.columns[i].base == before[i].base && built.terrain.columns[i].blocks == before[i].blocks;
                        check(kept, "whole Steam construction-admitted tile retains all original surface and underground columns");
                        check(!built.terrain.columns[0].fullColour, "retained Steam construction keeps its original color encoding");
                        bool keptModels = built.models.size() == originalModels.size();
                        for (size_t i = 0; i < std::min(built.models.size(), originalModels.size()); ++i) {
                            const auto &a = built.models[i], &b = originalModels[i];
                            keptModels &= std::tie(a.type, a.x, a.y, a.z, a.size, a.yaw, a.r, a.g, a.b, a.flags) ==
                                          std::tie(b.type, b.x, b.y, b.z, b.size, b.yaw, b.r, b.g, b.b, b.flags);
                        }
                        check(keptModels, "Steam construction keeps exact model order, positions, scales, tint and flags");
                        const auto originalMap = worldgen::mapRegion(zx, zy, 0);
                        auto composedMap = originalMap;
                        hybrid.composeMap(composedMap);
                        bool mapKept = true;
                        for (int i = 3; i < 19; ++i)
                            for (int j = 3; j < 19; ++j)
                                for (int k = 0; k < originalMap.sizeZ; ++k) {
                                    const int mappedK = originalMap.base() + k - composedMap.base();
                                    if (mappedK < 0 || mappedK >= composedMap.sizeZ) {
                                        mapKept = false;
                                        continue;
                                    }
                                    const auto *before = originalMap.voxel(i, j, k);
                                    const auto *after = composedMap.voxel(i, j, mappedK);
                                    mapKept &= std::equal(before, before + 3, after);
                                }
                        check(mapKept, "Steam construction map columns keep their full RGB geometry and absolute height");
                        constructionChecked = true;
                    }
            }
        check(constructionChecked, "construction preservation fixture is exercised");
    }
    {
        const auto seed = worldgen::currentWorld().seed;
        auto profile = std::make_shared<worldgen::AlphaTerrain>(123, nullptr, seed);
        const auto steamIdentity = worldgen::terrainIdentity();
        worldgen::setAlphaTerrain(profile);
        const auto climate = worldgen::alphaTerrainSample(0, 0);
        check(climate && climate->warmth == world.warmth(alphacoords::HOME, alphacoords::HOME) &&
                  climate->moisture == world.moisture(alphacoords::HOME, alphacoords::HOME) &&
                  climate->sea == world.seaShare(alphacoords::HOME, alphacoords::HOME) &&
                  climate->fire == world.fireShare(alphacoords::HOME, alphacoords::HOME),
              "sky factors use the mapped Alpha World's continuous blends");
        check(!worldgen::alphaTerrainSample(INT32_MAX, 0), "sky/map overview does not wrap outside Alpha");
        worldgen::forgetZones();
        std::barrier begin(4);
        std::array<std::future<std::shared_ptr<const worldgen::ZoneContents>>, 4> readers;
        for (auto &reader : readers)
            reader = std::async(std::launch::async, [&] {
                begin.arrive_and_wait();
                return worldgen::zone(0, 0);
            });
        const auto shared = readers[0].get();
        bool coalesced = true;
        for (size_t i = 1; i < readers.size(); ++i)
            coalesced &= readers[i].get() == shared;
        check(coalesced, "concurrent canonical readers share one immutable Alpha zone generation");
        const auto profileIdentity = worldgen::terrainIdentity();
        worldgen::forgetZones();
        const auto reloaded = worldgen::zone(0, 0);
        check(reloaded != shared && reloaded->terrain.columns[0].blocks == shared->terrain.columns[0].blocks &&
                  worldgen::terrainIdentity() == profileIdentity,
              "cache flush retires ready generations without changing terrain identity or geometry");
        const auto alphaMap = worldgen::mapRegion(0, 0, 0);
        int32_t mapTop = INT32_MIN;
        for (int k = 0; k < alphaMap.sizeZ; ++k) {
            const auto rgb = alphaMap.voxel(3, 3, k);
            if (rgb[0] || rgb[1] || rgb[2])
                mapTop = (alphaMap.base() + k) * alphaMap.unit;
        }
        const auto &mapColumn = worldgen::zone(0, 0)->terrain.columns[0];
        int32_t columnTop = mapColumn.base;
        for (int64_t k = int64_t(mapColumn.blocks.size()) - 1; k >= 0; --k) {
            if (!worldgen::isOpen(mapColumn.blocks[size_t(k)])) {
                columnTop = mapColumn.base + int32_t(k) + 1;
                break;
            }
        }
        check(mapColumn.fullColour && mapTop == (std::max(1, columnTop) / alphaMap.unit) * alphaMap.unit,
              "close Alpha map top agrees with the playable landscape at the same coordinate");
        check(worldgen::usingAlphaTerrain() && worldgen::terrainIdentity() != steamIdentity &&
                  worldgen::terrainIdentity() == profile->identity(worldgen::worldScale()),
              "canonical generator mode changes cache/profile identity without replacing Steam generation");
        check(worldgen::terrainZoneAvailable(0, 0) && !worldgen::terrainZoneAvailable(INT32_MAX, 0),
              "generated-world admission respects the Alpha finite coordinate frame");
        {
            ExtractedWorld reader;
            reader.openGenerated(0, 0);
            float surface = 0;
            check(reader.generated() && reader.sourceIdentity() == worldgen::terrainIdentity() && reader.surface(0, 0, surface),
                  "playable world reader loads Alpha geometry through the canonical generation path");
            check(reader.solidAt(0, 0, surface - 0.25) && !reader.solidAt(0, 0, surface + 0.25) && reader.home().z == surface,
                  "Alpha spawn height and collision agree on the generated surface");
            const auto &sourceColumn = worldgen::zone(0, 0)->terrain.columns[0];
            const int base = sourceColumn.base;
            const ExtractedWorld::GameBlock target{12, 34, 56, 0, 6, 0};
            check(reader.setGameBlock(0, 0, base - 4, target), "Alpha reader admits a full setter below its stored run");
            const auto gap = reader.gameBlock(0, 0, base - 2);
            const auto expected = terrain.rock(alphacoords::HOME, alphacoords::HOME, base - 2 - sourceColumn.sourceShift);
            auto nativeColumn = sourceColumn;
            nativeColumn.fullColour = false;
            check(worldgen::alphaImplicitColour(sourceColumn, 0, 0, base - 2) &&
                      !worldgen::alphaImplicitColour(nativeColumn, 0, 0, base - 2),
                  "implicit Alpha colour selection leaves native Steam columns unchanged");
            auto renderColumn = sourceColumn;
            renderColumn.base = 10;
            renderColumn.sourceShift = 7;
            renderColumn.blocks = {{10, 20, 30, 0, worldgen::blocktype::ROCK, 0}};
            const auto rock = terrain.rock(alphacoords::HOME, alphacoords::HOME, 1);
            const auto near = worldgen::alphaImplicitColour(renderColumn, 0, 0, 8);
            const glm::vec3 blended((10.f * .8f) / 255.f + (rock.r * .2f) / 255.f, (20.f * .8f) / 255.f + (rock.g * .2f) / 255.f,
                                    (30.f * .8f) / 255.f + (rock.b * .2f) / 255.f);
            check(near && *near == blended,
                  "buried solid faces blend the first stored block with unquantized source rock at translated height");
            bool pureRock = true;
            const glm::vec3 pure(rock.r / 255.f, rock.g / 255.f, rock.b / 255.f);
            for (const uint8_t type : {uint8_t(0), uint8_t(2)}) {
                renderColumn.blocks.front().type = type;
                const auto sampled = worldgen::alphaImplicitColour(renderColumn, 0, 0, 8);
                pureRock &= sampled && *sampled == pure;
            }
            renderColumn.blocks.clear();
            const auto empty = worldgen::alphaImplicitColour(renderColumn, 0, 0, 8);
            check(pureRock && empty && *empty == pure && !worldgen::alphaImplicitColour(renderColumn, 0, 0, 10),
                  "air, water and empty references select pure rock only strictly below the stored range");
            renderColumn.blocks = {{10, 20, 30, 0, worldgen::blocktype::ROCK, 0}};
            const auto deepRock = terrain.rock(alphacoords::HOME, alphacoords::HOME, -8);
            const auto deep = worldgen::alphaImplicitColour(renderColumn, 0, 0, -1);
            check(deep && *deep == glm::vec3(deepRock.r / 255.f, deepRock.g / 255.f, deepRock.b / 255.f),
                  "the reference contribution reaches zero after ten blocks of depth");
            alphalight::LightVolume closed({0, 0, 0}, {8, 8, 8});
            bool retainedPrecision = false;
            for (int i = 0; i < 512 && !retainedPrecision; i++) {
                const glm::vec3 rgb((float(i) + .5f) / 512.f, .37f, .23f);
                const auto precise = alphalight::shadeCorner(closed, {4, 4, 4}, 0, rgb);
                const VoxelColor packed{uint8_t(rgb.r * 255.f), uint8_t(rgb.g * 255.f), uint8_t(rgb.b * 255.f), 0};
                const auto rounded = alphalight::shadeCorner(closed, {4, 4, 4}, 0, packed);
                retainedPrecision = precise.r != rounded.r || precise.g != rounded.g || precise.b != rounded.b;
            }
            check(retainedPrecision, "corner shading retains sub-byte implicit colour until final vertex packing");
            check(gap[0] == uint8_t(int32_t(expected.r)) && gap[1] == uint8_t(int32_t(expected.g)) &&
                      gap[2] == uint8_t(int32_t(expected.b)) && gap[4] == worldgen::blocktype::ROCK &&
                      reader.gameBlock(0, 0, base - 4) == target,
                  "Alpha downward gaps use full RGB from the source rock palette and retain all target bytes");
            auto fork = reader.forkSource();
            float forkHeight = 0;
            check(fork->sourceIdentity() == reader.sourceIdentity() && fork->surface(0, 0, forkHeight) && forkHeight == surface,
                  "independent host/mirror readers reproduce the same Alpha source geometry");
        }
        const auto retainedFiller = worldgen::alphaColumnFiller(0, 0, 7);
        const auto shiftedColour = retainedFiller(20);
        const auto rawColour = terrain.rock(alphacoords::HOME, alphacoords::HOME, 13);
        check(shiftedColour && shiftedColour->r == uint8_t(int32_t(rawColour.r)) && shiftedColour->g == uint8_t(int32_t(rawColour.g)) &&
                  shiftedColour->b == uint8_t(int32_t(rawColour.b)),
              "translated Alpha columns sample material noise at the original source height");
        worldgen::setAlphaTerrain({});
        check(!worldgen::alphaColumnFiller(0, 0, 0) && retainedFiller(20) == shiftedColour,
              "gap sampler retains its source context while Steam mode keeps its native material path");
        check(!worldgen::alphaTerrainSample(0, 0), "Steam mode retains its separate climate provider");
        check(!worldgen::usingAlphaTerrain() && worldgen::terrainIdentity() == steamIdentity,
              "returning to Steam mode restores its distinct source identity");
    }
    {
        // Exercise the normal121-zone keep radius through the real generated reader.
        // Far-zone identity must remain stable while the protected neighborhood is full.
        ExtractedWorld reader;
        reader.openGenerated(0, 0);
        reader.keepAround(0, 0, 5);
        for (int x = -5; x <= 5; ++x)
            for (int y = -5; y <= 5; ++y)
                reader.propsAbsolute(x, y);
        check(reader.gameZoneLoaded(-5, -5) && reader.gameZoneLoaded(5, 5), "native player keep radius retains its full neighborhood");
        reader.propsAbsolute(6, 0);
        const auto first = reader.gameZoneToken(6, 0);
        reader.propsAbsolute(7, 0);
        const auto again = reader.gameZoneToken(6, 0);
        check(first && again && first->order == again->order, "pinned player zones do not evict every unpinned mesh source immediately");
        // Unpinned entries still have a bound; loaded-zone lookup does not refresh LRU.
        for (int x = 8; x < 105; ++x)
            reader.propsAbsolute(x, 0);
        check(!reader.gameZoneLoaded(6, 0) && reader.gameZoneLoaded(5, 5),
              "unpinned streaming LRU evicts old data while explicit player keeps survive");
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
