#include "WaterLilies.h"
#include "AlphaLook.h"
#include <array>
#include <deque>
#include <set>
#include <tuple>
// Engine checks that need no GPU: .cub parsing, meshing, terrain rules.
#include "Body.h"
#include "DeathEffects.h"
#include "Assets.h"
#include "DataCheck.h"
#include "CubModel.h"
#include "Extracted.h"
#include "Character.h"
#include "Combat.h"
#include "Gear.h"
#include "Creatures.h"
#include "Actors.h"
#include "Recording.h"
#include "SteamSession.h"
#include "Renderer.h"
#include "NativeAttackTiming.h"
#include "NativeLoot.h"
#include "ClassMoves.h"
#include "Progression.h"
#include "Speech.h"
#include "Saves.h"
#include "NativeProgress.h"
#include "GameWorld.h"
#include "TalkScripts.h"
#include "TalkGen.h"
#include "ItemNames.h"
#include "GameNames.h"
#include "worldgen/Generator.h"
#include "worldgen/Gear.h"
#include "worldgen/Looks.h"
#include "worldgen/MapModel.h"
#include "OverworldEvents.h"
#include "PyroCreatures.h"
#include "Missions.h"
#include "Lore.h"
#include "MapMarks.h"
#include "UsePrompt.h"
#include "GameCraft.h"
#include "GameShop.h"
#include "GroundItems.h"
#include "NativePose.h"
#include "NativeRules.h"
#include "NativeItems.h"
#include "NativeUpdate.h"
#include "NativeAI.h"
#include "NativeUpkeep.h"
#include "NativeAbility.h"
#include "NativeHit.h"
#include "NativeEffects.h"
#include "NativeDrop.h"
#include "NativeInteract.h"
#include "NativeControl.h"
#include "GameInput.h"
#include "Plx.h"
#include "PlasmaShape.h"
#include "PlasmaText.h"
#include "PlasmaScene.h"
#include "SoundRules.h"
#include "stb_image_write.h"
#include <functional>
#include "worldgen/Residents.h"
#include "worldgen/Noise.h"
#include "worldgen/Regions.h"
#include "Hud.h"
#include <windows.h>
#include <limits>
#include <string>
#include "Terrain.h"
#include "TerrainRules.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <fstream>
#include <sstream>
#include <limits>

// The game's zones to compare with: the newest Extract\seed_<n>, or CW_ZONES_DIR (such as
// Extract\gen_seed_123: zones the game generated afresh for the oracle's gen.request, with
// their .spawns beside them).
// A generated zone's spawn points in id order, its static objects, and a spawn point made into a
// creature by the resident pass (worldgen/Residents.h), with full health by the game's rules.
static std::vector<worldgen::SpawnPoint> zoneSpawnList(int32_t zx, int32_t zy) {
    std::vector<worldgen::SpawnPoint> out;
    for (const auto &[id, sp] : worldgen::zone(zx, zy)->spawns.points)
        out.push_back(sp);
    return out;
}
static std::vector<worldgen::StaticObject> generatedObjects(int32_t zx, int32_t zy) { return worldgen::zone(zx, zy)->objects; }
static worldgen::CreatureRecord residentOf(const worldgen::SpawnPoint &sp, std::function<int()> rand) {
    return worldgen::makeResident(sp, worldgen::Draw(rand), false, [](const uint8_t *r) { return nativerules::maxHealth(r); }).record;
}

static std::filesystem::path zonesDir(const std::filesystem::path &gameDir) {
    if (const char *d = std::getenv("CW_ZONES_DIR"); d && *d)
        return std::filesystem::path(d);
    return ExtractedWorld::find(gameDir);
}
// The world seed of a zones folder: the number after its last '_' (seed_123, gen_seed_123).
static int seedOf(const std::filesystem::path &dir) {
    const std::string name = dir.filename().string();
    return std::atoi(name.substr(name.find_last_of('_') + 1).c_str());
}
static std::filesystem::path spawnsDir(const std::filesystem::path &gameDir) {
    if (const char *d = std::getenv("CW_ZONES_DIR"); d && *d)
        return std::filesystem::path(d);
    return gameDir / "Extract" / "oracle";
}

static int failures = 0;
int alphaRenderingTests(); // tests/AlphaRenderTests.cpp
int skyTests();           // tests/SkyTests.cpp
int raceKeyTests();       // tests/RaceKeyTests.cpp
int worldMapLookTests();   // tests/WorldMapTests.cpp
int saveFormatTests();     // tests/SaveTests.cpp
int frontendTests();       // tests/FrontendTests.cpp
int windContinueTests();   // tests/WindContinueTests.cpp
int enemyRespawnTests();   // tests/EnemyRespawnTests.cpp
int playerHitTests();      // tests/PlayerHitTests.cpp
int dialoguePageTests();   // tests/DialoguePageTests.cpp
int petTests();            // tests/PetTests.cpp
int steamSceneRangeTests();
int steamMeshCostTests();
int gamepadUiTests();      // tests/GamepadUiTests.cpp
int plasmaHitTests();      // tests/PlasmaHitTests.cpp
int objectVisualTests();
int sceneCaptureTests();
int regionLevelsTests();
int sceneVisibilityTests();
int steamLightingTests();
int steamVoxelTests();
int steamTerrainTests();
int steamShaderTests();
int steamPointLightsTests();
int steamTransientLightsTests();
int steamCreatureLightsTests();
int dialogueChoicesTests();
int npcSpeechTests();
int steamLightOracleTests(const fs::path &, const fs::path &);
int objectClockTests();   // tests/ObjectVisualTests.cpp
int netTests();            // tests/NetTests.cpp: the game-compatible network layer (src/net/)
int ownerSetupTests();     // tests/OwnerSetupTests.cpp: the first-run setup (src/OwnerSetup.*)
int poseHudTests();        // tests/PoseHudTests.cpp: the pose state, weapon spirits, the HUD coin
int waterLookTests();
int alphaSceneryTests();
int alphaSceneryShaderTests();
int waterShaderTests();
int featTextTests();
int lootWindowTests();     // tests/LootWindowTests.cpp: the identify window, upgrade widget, chat line
int addTestLoot(const char *folder);
int poseStateOracle(const std::vector<std::string> &files);
int netDecodeCommand(int argc, char **argv);
int makeTestSave(const char *folder, const char *recording, const glm::vec2 *nearHome = nullptr);
int worldgenTerrainTests(); // tests/WorldgenTerrainTests.cpp: the new generator's terrain, fast subset
int worldgenTerrainCommand(int maxZones);
int worldgenStructureTests(); // tests/WorldgenStructureTests.cpp: the new generator's structures, fast subset
int worldgenStructureCommand(int maxZones);
int worldgenCreatureTests(); // tests/WorldgenCreatureTests.cpp: the new generator's creatures, fast subset
int worldgenCreatureCommand(int maxZones);
int worldgenLightCommand(int maxZones);
int worldgenLightProbe(int32_t x, int32_t y, int32_t z);
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);                              \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

static void cubFormat() {
    // 2x1x1: one red voxel at x=0, empty at x=1.
    uint8_t data[12 + 6] = {};
    int32_t dims[3] = {2, 1, 1};
    std::memcpy(data, dims, 12);
    data[12] = 255;
    CubModel m;
    CHECK(m.loadMemory(data, sizeof data));
    CHECK(m.solid(0, 0, 0) && !m.solid(1, 0, 0));
    Mesh mesh = m.mesh();
    CHECK(mesh.vertices.size() == 24 && mesh.indices.size() == 36); // One cube, six faces.
    CHECK(!m.loadMemory(data, sizeof data - 1));                    // Size mismatch.
}

static void facesPointOutward() {
    // Each face's triangle winding must agree with its normal (CCW outside).
    for (int f = 0; f < 6; f++) {
        glm::vec3 a(FACE_CORNERS[f][0][0], FACE_CORNERS[f][0][1], FACE_CORNERS[f][0][2]);
        glm::vec3 b(FACE_CORNERS[f][1][0], FACE_CORNERS[f][1][1], FACE_CORNERS[f][1][2]);
        glm::vec3 c(FACE_CORNERS[f][2][0], FACE_CORNERS[f][2][1], FACE_CORNERS[f][2][2]);
        glm::vec3 n = glm::cross(b - a, c - a);
        glm::vec3 expected(FACE_NORMALS[f][0], FACE_NORMALS[f][1], FACE_NORMALS[f][2]);
        CHECK(glm::dot(n, expected) > .5f);
    }
}

static void zoneBordersAgree() {
    // Neighbouring zones generate the same blocks in their shared border.
    Terrain t;
    ZoneData a = t.generate(3, 5), b = t.generate(4, 5);
    int mismatches = 0;
    for (int y = 0; y < ZONE; y++)
        for (int z = 0; z < WORLD_HEIGHT; z++)
            mismatches += a.at(ZONE, y, z) != b.at(0, y, z) || a.at(ZONE - 1, y, z) != b.at(-1, y, z);
    CHECK(mismatches == 0);
}

static void varianceMatchesMod() {
    TerrainSettings s;
    s.alphaStyle = false;
    Terrain t(s);
    double v = terrain::variation(1000, 2000, s.seed, 68, 768);
    CHECK(std::abs(v) <= 68 * 1.06 + 1e-9);
    // Heights stay inside the world.
    for (int i = 0; i < 2000; i++) {
        auto c = t.column(i * 37.0 - 30000, i * 53.0 + 9000);
        CHECK(c.ground >= 3 && c.ground < WORLD_HEIGHT - 15);
    }
}

static void alphaHasTallMountains() {
    // Alpha style reaches far higher than Steam's gentle ground somewhere.
    Terrain t;
    int highest = 0;
    for (int y = -8000; y < 8000; y += 64)
        for (int x = -8000; x < 8000; x += 64)
            highest = std::max(highest, t.column(x, y).ground - t.config().seaLevel);
    std::printf("highest Alpha-style ground: %d blocks above the sea\n", highest);
    CHECK(highest > 120);
}

static void riversAndLilies() {
    // Rivers exist, and lilies sit only on shallow fresh water.
    Terrain t;
    int river = 0;
    for (int y = -4000; y < 4000; y += 16)
        for (int x = -4000; x < 4000; x += 16)
            river += t.column(x, y).river > .5f;
    CHECK(river > 50);
    int pads = 0, bad = 0;
    for (int zy = -16; zy < 16 && pads < 40; zy++)
        for (int zx = -16; zx < 16 && pads < 40; zx++) {
            bool wet = false;
            for (int k = 0; k < 16 && !wet; k++) {
                auto c = t.column(zx * 64 + (k % 4) * 16 + 8, zy * 64 + (k / 4) * 16 + 8);
                wet = c.water && c.freshwater;
            }
            if (!wet)
                continue;
            ZoneData z = t.generate(zx, zy);
            for (auto &s : z.scenery)
                if (s.model.rfind("water-lily", 0) == 0) {
                    if (!pads)
                        std::printf("first lily pad at %.0f %.0f\n", s.x, s.y);
                    pads++;
                    auto col = t.column(std::floor(s.x), std::floor(s.y));
                    bad += !col.water || !col.freshwater;
                }
        }
    std::printf("lily pads found: %d\n", pads);
    CHECK(pads > 0);
    CHECK(bad == 0);
}

static void regionsAreLarge() {
    // With 2048-block regions the biome rarely changes over 256 blocks.
    Terrain t;
    int changes = 0, samples = 0;
    for (int i = 0; i < 400; i++) {
        double x = i * 997.0, y = i * 613.0;
        changes += t.column(x, y).biome != t.column(x + 256, y).biome;
        samples++;
    }
    CHECK(changes < samples / 3);
}

static void allRegionsOccur() {
    // Each of the game's ten region types appears, with a dry interior spot.
    Terrain t;
    bool found[BIOMES] = {};
    for (int y = -24000; y < 24000; y += 512)
        for (int x = -24000; x < 24000; x += 512) {
            auto c = t.column(x, y);
            if (found[c.biome] || c.weights[c.biome] < .99f || (c.biome != OCEAN && c.water))
                continue;
            found[c.biome] = true;
            std::printf("  %-10s at %d %d\n", biomeName(c.biome), x, y);
        }
    for (int b = 0; b < BIOMES; b++)
        CHECK(found[b]);
}

static void steamAssets() {
    // Optional: the real Steam sprites parse (skipped without an install).
    Assets a;
    if (!a.locate("")) {
        std::printf("skip: Steam Cube World not found\n");
        return;
    }
    int ok = 0, total = 0;
    for (auto &name : a.list("sprites", ".cub")) {
        CubModel m;
        total++;
        ok += m.load(*a.resolve(fs::path("sprites") / name));
    }
    std::printf("parsed %d / %d Steam sprites\n", ok, total);
    CHECK(total > 0 && ok == total);
    for (auto n : {"water-lily01", "water-lily02", "lily", "grass", "reed", "bush"})
        CHECK(a.resolve(fs::path("sprites") / (std::string(n) + ".cub")).has_value());
}

static void writeZoneFixture(const fs::path &file, int x, int y, int baseZ = 0) {
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    out.write("CWZ1", 4);
    int32_t header[] = {x, y, 0, 0};
    out.write((const char *)header, sizeof header);
    for (int i = 0; i < 4096; i++) {
        int32_t base = baseZ;
        uint32_t runs = 1;
        uint16_t count = 1;
        uint8_t block[] = {50, 60, 70, 0, 1, 0};
        out.write((const char *)&base, 4); out.write((const char *)&runs, 4);
        out.write((const char *)&count, 2); out.write((const char *)block, 6);
    }
    CHECK(bool(out));
}

static void waterSurfaceGeometry() {
    const auto root = fs::temp_directory_path() / ("CubeVulkan-water-" + std::to_string(GetCurrentProcessId()));
    fs::create_directory(root);
    const auto zone = root / "z_0_0.cwz";
    std::ofstream(root / "manifest.txt") << "seed=1\nhomeX=0\nhomeY=0\nhomeZ=0\n";
    writeZoneFixture(zone, 0, 0, -2);
    ExtractedWorld world;
    std::string error;
    Mesh opaque, water;
    auto oldStyle = renderStyle();
    setRenderStyle(RenderStyle::Steam);
    CHECK(world.open(root, error) && world.mesh(0, 0, opaque, water));
    CHECK(water.indices.size() == 4096 * 6);
    auto allEligible = [](glm::vec3) { return true; };
    auto pads = waterLilies::decorate(water, 123, 1, 28, allEligible);
    CHECK(pads.size() == 28);
    CHECK(waterLilies::decorate(water, 123, 0, 28, allEligible).empty());
    CHECK(waterLilies::decorate(water, 123, 1, 0, allEligible).empty());
    CHECK(waterLilies::decorate(water, 123, 1, 28, [](glm::vec3) { return false; }).empty());
    auto repeated = waterLilies::decorate(water, 123, 1, 28, allEligible);
    for (size_t i = 0; i < pads.size(); i++) {
        CHECK(pads[i].model == "water-lily01" || pads[i].model == "water-lily02");
        CHECK(pads[i].z == ExtractedWorld::SEA + .1f && pads[i].scale == .09f && pads[i].flags == 0);
        CHECK(pads[i].x == repeated[i].x && pads[i].y == repeated[i].y && pads[i].yaw == repeated[i].yaw);
    }
    for (auto &v : water.vertices) CHECK(v.z == ExtractedWorld::SEA); // no invented 0.1 lowering
    {
        std::ofstream out(zone, std::ios::binary | std::ios::trunc);
        out.write("CWZ1", 4);
        int32_t header[] = {0, 0, 0, 0}; out.write((char *)header, sizeof header);
        for (int i = 0; i < 4096; i++) {
            bool special = i == 2 * 64 + 2 || i == 4 * 64 + 4;
            int32_t base = 0; uint32_t runs = special ? 3 : 1;
            out.write((char *)&base, 4); out.write((char *)&runs, 4);
            for (uint32_t run = 0; run < runs; run++) {
                uint16_t count = 1;
                uint8_t type = run == 0 ? 1 : run == 1 ? 0 : i == 2 * 64 + 2 ? 2 : 3;
                uint8_t block[] = {50, 60, 70, 0, type, 0};
                out.write((char *)&count, 2); out.write((char *)block, 6);
            }
        }
    }
    for (auto style : {RenderStyle::Steam, RenderStyle::Alpha}) {
        setRenderStyle(style); opaque = {}; water = {};
        CHECK(world.open(root, error) && world.mesh(0, 0, opaque, water));
        CHECK(water.indices.size() == 7 * 6); // six hanging-water faces and one wet-ground film
        CHECK(world.solidAt(4.5, 4.5, ExtractedWorld::Z_OFFSET + 2.5));
        bool bottom = false;
        for (auto &v : water.vertices) {
            bottom |= v.z == ExtractedWorld::Z_OFFSET + 2;
            if (style == RenderStyle::Steam) CHECK(v.normal == 4 && v.b == 255 && v.ao == 255);
        }
        CHECK(bottom);
    }
    setRenderStyle(oldStyle);
    fs::remove(zone); fs::remove(root / "manifest.txt"); fs::remove(root);
}

static void extractedPlacements() {
    // Raw game records contain nonzero padding after the one-byte rotation.
    // Exercise the real file reader, including negative world coordinates.
    auto root = fs::temp_directory_path() /
                ("CubeVulkan-placement-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto extract = root / "Extract";
    auto seed = extract / "seed_123";
    CHECK(fs::create_directory(root));
    CHECK(fs::create_directory(extract));
    CHECK(fs::create_directory(seed));
    { std::ofstream(seed / "manifest.txt") << "seed=123\nhomeX=-65\nhomeY=-1\nhomeZ=7\n"; }
    writeZoneFixture(seed / "z_-2_-1.cwz", -2, -1);
    { std::ofstream(extract / "sprite-groups.txt") << "# environment sprites\n0 sprites/flower.cub\n"
                                                   "# building-part sprites\n3 sprites/other.cub\n0 sprites/window.cub\n"; }
    auto records = seed / "z_-2_-1.rec";
    auto writeRecords = [&](uint32_t stride, uint32_t count, const std::vector<uint8_t> &payload) {
        std::ofstream out(records, std::ios::binary | std::ios::trunc);
        out.write("CWR1", 4);
        uint32_t header[] = {0x60, stride, count};
        out.write(reinterpret_cast<const char *>(header), sizeof header);
        out.write(reinterpret_cast<const char *>(payload.data()), payload.size());
    };
    std::vector<uint8_t> payload(4 * 0xe0);
    for (int turn = 0; turn < 4; turn++) {
        auto r = payload.data() + turn * 0xe0;
        int64_t pos[] = {-65LL * 65536 + 32768, -65536, 8LL * 65536};
        memcpy(r + 8, pos, sizeof pos);
        r[0x20] = (uint8_t)turn;
        r[0x21] = 0xa5;
        r[0x22] = 0xff;
        r[0x23] = 0x74;
        float scale = 1;
        memcpy(r + 0x24, &scale, sizeof scale);
    }
    writeRecords(0xe0, 4, payload);
    ExtractedWorld ex;
    std::string error;
    CHECK(ex.open(seed, error));
    auto props = ex.props(0, 0);
    CHECK(props.size() == 4);
    CHECK(!ex.meshCapture(0, 0)); // legacy files remain valid without an oracle
    scenecapture::MeshCache captured;
    captured.minimum = {-128LL * 65536, -64LL * 65536, -10LL * 65536};
    captured.maximum = {-63LL * 65536, 65536, 90LL * 65536};
    captured.center = {-95LL * 65536 - 32768, -31LL * 65536 - 32768, 40LL * 65536};
    captured.activeGroups = 0x100000001ull;
    captured.radius = 112.5f;
    captured.triangleCost = 12345;
    auto appendCapture = [&] {
        std::ofstream out(records, std::ios::binary | std::ios::app);
        const uint32_t header[] = {scenecapture::MESH_CACHE_RECORD, scenecapture::MESH_CACHE_BYTES, 1};
        const auto bytes = scenecapture::encode(captured);
        out.write((const char *)header, sizeof header);
        out.write((const char *)bytes.data(), bytes.size());
    };
    appendCapture();
    auto metadata = ex.meshCapture(0, 0);
    CHECK(metadata && metadata->activeGroups == captured.activeGroups && metadata->minimum == captured.minimum &&
          metadata->center == captured.center && metadata->triangleCost == 12345);
    CHECK(ex.props(0, 0).size() == 4); // metadata never becomes scenery
    appendCapture();
    CHECK(!ex.meshCapture(0, 0)); // ambiguous duplicate metadata is not evidence
    writeRecords(0xe0, 4, payload);
    captured.minimum[0] += 64LL * 65536;
    captured.maximum[0] += 64LL * 65536;
    appendCapture();
    CHECK(!ex.meshCapture(0, 0)); // a coherent record from the wrong zone is rejected
    writeRecords(0xe0, 4, payload);
    for (size_t i = 0; i < props.size(); i++) {
        CHECK(props[i].model == "window");
        CHECK(std::abs(props[i].x - 63.5f) < 1e-5f);
        CHECK(std::abs(props[i].y - 63.f) < 1e-5f);
        CHECK(std::abs(props[i].z - 136.f) < 1e-5f);
        CHECK(std::abs(props[i].yaw - i * 1.5707963f) < 1e-5f);
        CHECK(props[i].estimatedScale);
    }
    { std::ofstream(extract / "sprite-scales.txt") << "11\tsprites/window.cub\t0.125\t12\n"; }
    CHECK(ex.open(seed, error));
    props = ex.props(0, 0);
    CHECK(props.size() == 4 && !props[0].estimatedScale && props[0].scale == .125f);
    CHECK(fs::remove(extract / "sprite-scales.txt"));
    CHECK(ex.open(seed, error));
    CHECK(ex.props(0, 0)[0].estimatedScale); // no stale scale after re-opening
    for (uint8_t turn : {4, 48, 191, 255}) {
        payload[0x20] = turn; // Steam's renderer accepts every unsigned rotation byte.
        writeRecords(0xe0, 4, payload);
        const auto rotated = ex.props(0, 0);
        CHECK(rotated.size() == 4);
        CHECK(std::abs(rotated[0].yaw - turn * 1.5707963f) < 1e-5f);
        CHECK(rotated[0].x == props[0].x && rotated[0].y == props[0].y && rotated[0].z == props[0].z);
    }
    CHECK(fs::create_directory(root / "sprites"));
    {
        std::ofstream model(root / "sprites" / "window.cub", std::ios::binary);
        const int32_t dimensions[] = {7, 14, 7}; // X is the normalization denominator, even when Y is wider.
        model.write((const char *)dimensions, sizeof dimensions);
        std::ofstream(extract / "sprite-scales.txt") << "11\tsprites/window.cub\t0.125\t12\n";
    }
    const float objectScale = 1.5f;
    memcpy(payload.data() + 0x24, &objectScale, sizeof objectScale);
    writeRecords(0xe0, 4, payload);
    CHECK(ex.open(seed, error));
    const auto normalized = ex.props(0, 0);
    CHECK(normalized.size() == 4 && !normalized[0].estimatedScale);
    CHECK(std::abs(normalized[0].scale - 3.0f / 7) < 1e-6f);
    CHECK(normalized[0].object.scale == objectScale && normalized[0].object.turn == 255);
    const float largeScale = 12;
    memcpy(payload.data() + 0x24, &largeScale, sizeof largeScale);
    writeRecords(0xe0, 4, payload);
    const auto largeObject = ex.props(0, 0);
    CHECK(largeObject.size() == 4 && std::abs(largeObject[0].scale - 24.0f / 7) < 1e-6f);
    {
        std::ofstream out(records, std::ios::binary | std::ios::app);
        auto append = [&](uint32_t offset, uint32_t stride, const std::vector<uint8_t> &record) {
            const uint32_t header[] = {offset, stride, 1};
            out.write((const char *)header, sizeof header);
            out.write((const char *)record.data(), record.size());
        };
        std::vector<uint8_t> model(0x40), item(0xe0);
        const int64_t position[] = {-65LL * 65536, -65536, 8LL * 65536};
        memcpy(model.data() + 8, position, sizeof position);
        const float size = 2.5f, yaw = 45;
        const int32_t flags = 4;
        memcpy(model.data() + 0x20, &size, sizeof size);
        memcpy(model.data() + 0x24, &yaw, sizeof yaw);
        memcpy(model.data() + 0x38, &flags, sizeof flags);
        const float light = 21;
        memcpy(model.data() + 0x28, &light, sizeof light);
        append(0x18, 0x40, model);
        append(0x90, 0x40, model); // Derived light list must not duplicate the model as a building part.
        append(0xa8, 0x40, model); // Nor should ambient emitter records draw a voxel model.
        item[0] = 3; // A real item category, distinct from building-part type3.
        memcpy(item.data() + 0xa0, position, sizeof position);
        append(0x30, 0xe0, item);
        append(0x48, 0xe0, item);
    }
    const auto sceneProps = ex.props(0, 0);
    CHECK(sceneProps.size() == 5);
    CHECK(sceneProps.back().model == "flower" && sceneProps.back().scale == 2.5f);
    CHECK(sceneProps.back().modelFlags == 4 && sceneProps.back().listPosition == 1);
    CHECK(sceneProps.back().modelType == 0 && std::abs(sceneProps.back().light - 21.0f / 255) < 1e-6f);
    CHECK(sceneProps[0].objectKey.index == 0 && sceneProps[3].objectKey.index == 3);
    {
        std::ofstream out(records, std::ios::binary | std::ios::app);
        std::array<uint8_t, 0x40> beam{};
        const uint32_t header[] = {0x18, 0x40, 1};
        const int32_t type = 0x59;
        const int64_t position[] = {-65LL * 65536, -65536, 8LL * 65536};
        const float height = 20, width = 0.7f;
        memcpy(beam.data(), &type, sizeof type);
        memcpy(beam.data() + 8, position, sizeof position);
        memcpy(beam.data() + 0x20, &height, sizeof height);
        memcpy(beam.data() + 0x24, &width, sizeof width);
        out.write((const char *)header, sizeof header);
        out.write((const char *)beam.data(), beam.size());
    }
    const auto shafts = ex.props(0, 0);
    CHECK(shafts.size() == 6 && shafts.back().beam.has_value());
    CHECK(shafts.back().beam->height == 20 && shafts.back().beam->width == 0.7f);
    CHECK(shafts.back().beam->position[0] == 63LL * 65536);
    CHECK(shafts.back().beam->position[2] == (8LL + ExtractedWorld::Z_OFFSET) * 65536);
    // Complete mesh-neighbor input, then a water edit and its removal: baked light
    // uses current terrain, while the native runtime water flag remains ORed.
    for (int zx = -3; zx <= -1; ++zx)
        for (int zy = -2; zy <= 0; ++zy) {
            if (zx == -2 && zy == -1) continue;
            std::ofstream out(seed / ("z_" + std::to_string(zx) + "_" + std::to_string(zy) + ".cwz"), std::ios::binary);
            out.write("CWZ1", 4);
            const int32_t header[] = {zx, zy, 0, 0};
            out.write((const char *)header, sizeof header);
            const uint64_t emptyColumn = 0;
            for (int i = 0; i < 4096; ++i) out.write((const char *)&emptyColumn, sizeof emptyColumn);
        }
    CHECK(ex.open(seed, error));
    nativesaves::BlockChange wet;
    const int32_t editedBlock[] = {-65, -1, 8};
    memcpy(wet.bytes.data(), editedBlock, sizeof editedBlock);
    wet.bytes[0x12 + 4] = 2;
    ex.setBlockChanges(-2, -1, {wet});
    auto wetModels = ex.props(0, 0);
    ex.lightModels(0, 0, wetModels);
    CHECK(wetModels[4].light == 1 && (wetModels[4].modelFlags & 8));
    wet.bytes[0x12 + 4] = 0;
    ex.setBlockChanges(-2, -1, {wet});
    auto dryModels = ex.props(0, 0);
    ex.lightModels(0, 0, dryModels);
    CHECK(dryModels[4].light == 1 && (dryModels[4].modelFlags & 8));
    for (int zx = -3; zx <= -1; ++zx)
        for (int zy = -2; zy <= 0; ++zy)
            if (zx != -2 || zy != -1)
                fs::remove(seed / ("z_" + std::to_string(zx) + "_" + std::to_string(zy) + ".cwz"));
    writeRecords(0xe0, 5, payload); // Truncated payload.
    CHECK(ex.props(0, 0).empty());
    writeRecords(0, UINT32_MAX, {});
    CHECK(ex.props(0, 0).empty());
    writeRecords(4, 1, std::vector<uint8_t>(4)); // Unknown layout: no position field.
    CHECK(ex.props(0, 0).empty());
    fs::remove(records);
    fs::remove(seed / "manifest.txt");
    fs::remove(seed / "z_-2_-1.cwz");
    fs::remove(extract / "sprite-groups.txt");
    fs::remove(extract / "sprite-scales.txt");
    fs::remove(root / "sprites" / "window.cub");
    fs::remove(root / "sprites");
    fs::remove(seed);
    fs::remove(extract);
    fs::remove(root);
}

static void extractedWorldReloadAndValidation() {
    auto root = fs::temp_directory_path() /
                ("CubeVulkan-world-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(root / "seed_1"); fs::create_directories(root / "seed_2");
    for (int seed : {1, 2})
        std::ofstream(root / ("seed_" + std::to_string(seed)) / "manifest.txt") <<
            "seed=" << seed << "\nhomeX=0\nhomeY=0\nhomeZ=0\n";
    auto a = root / "seed_1" / "z_0_0.cwz", b = root / "seed_2" / "z_0_0.cwz";
    writeZoneFixture(a, 0, 0, -2); writeZoneFixture(b, 0, 0, 8);
    { std::ofstream(root / "seed_1" / "z_5_5.rec") << "CWR1"; }
    { std::ofstream(root / "seed_1" / "z_7_7.cwz.tmp") << "pending"; }
    ExtractedWorld world; std::string error; float height = 0;
    CHECK(world.open(root / "seed_1", error) && world.zoneCount() == 1);
    CHECK(world.surface(0, 0, height) && height == 129); // water over a negative base
    CHECK(world.open(root / "seed_2", error) && world.seed() == 2 && world.zoneCount() == 1);
    CHECK(world.surface(0, 0, height) && height == 137); // no cached terrain from seed_1
    CHECK(ExtractedWorld::validateZoneFile(a, error));
    int x, y;
    CHECK(!ExtractedWorld::zoneCoordinates("z_1_2.rec", x, y));
    CHECK(!ExtractedWorld::zoneCoordinates("z_1_2.cwz.tmp", x, y));
    CHECK(!ExtractedWorld::zoneCoordinates("z_01_2.cwz", x, y));
    CHECK(!ExtractedWorld::zoneCoordinates("z_2147483648_0.cwz", x, y));
    CHECK(ExtractedWorld::zoneCoordinates("z_-1_2.cwz", x, y) && x == -1 && y == 2);
    writeZoneFixture(b, 1, 0); // misplaced zone must not be used at another coordinate
    CHECK(!ExtractedWorld::validateZoneFile(b, error));
    CHECK(!world.open(root / "seed_2", error) && !world.surface(0, 0, height));
    writeZoneFixture(b, 0, 0);
    { std::fstream out(b, std::ios::binary | std::ios::in | std::ios::out); out.seekp(28); uint16_t zero = 0; out.write((char *)&zero, 2); }
    CHECK(!ExtractedWorld::validateZoneFile(b, error));
    writeZoneFixture(b, 0, 0, INT32_MIN);
    CHECK(!ExtractedWorld::validateZoneFile(b, error));
    writeZoneFixture(b, 0, 0, INT32_MAX - ExtractedWorld::Z_OFFSET);
    CHECK(!ExtractedWorld::validateZoneFile(b, error));
    writeZoneFixture(b, 0, 0);
    fs::resize_file(b, fs::file_size(b) - 1);
    CHECK(!ExtractedWorld::validateZoneFile(b, error));
    { std::ofstream out(b, std::ios::binary | std::ios::trunc); out << "CWZ1"; }
    CHECK(!ExtractedWorld::validateZoneFile(b, error));
    writeZoneFixture(b, 0, 0);
    { std::ofstream out(b, std::ios::binary | std::ios::app); out << "trailing"; }
    CHECK(!ExtractedWorld::validateZoneFile(b, error));
    std::ofstream(root / "seed_2" / "manifest.txt") << "seed=2\nhomeX=invalid\nhomeY=0\nhomeZ=0\n";
    CHECK(!world.open(root / "seed_2", error));

    // Header checks must not accept a timestamp from a truncated or non-PE file.
    std::vector<uint8_t> pe(512);
    pe[0] = 'M'; pe[1] = 'Z'; uint32_t nt = 128;
    memcpy(pe.data() + 0x3c, &nt, 4); memcpy(pe.data() + nt, "PE\0\0", 4);
    uint16_t machine = 0x8664, optional = 240, magic = 0x20b;
    uint32_t stamp = dataCheck::SUPPORTED_BUILD;
    memcpy(pe.data() + nt + 4, &machine, 2); memcpy(pe.data() + nt + 8, &stamp, 4);
    memcpy(pe.data() + nt + 20, &optional, 2); memcpy(pe.data() + nt + 24, &magic, 2);
    auto exe = root / "cubeworld.exe";
    auto writePe = [&] { std::ofstream out(exe, std::ios::binary | std::ios::trunc); out.write((char *)pe.data(), pe.size()); };
    writePe(); uint32_t found = 0;
    CHECK(dataCheck::gameBuild(exe, found, error) && found == stamp);
    pe[nt] = 0; writePe(); CHECK(!dataCheck::gameBuild(exe, found, error) && found == 0);
    pe[nt] = 'P'; pe[nt + 24] = 0x0b; pe[nt + 25] = 1; writePe();
    CHECK(!dataCheck::gameBuild(exe, found, error)); // PE32 is not supported by x64 offsets
    pe.resize(140); writePe(); CHECK(!dataCheck::gameBuild(exe, found, error));
    auto report = dataCheck::inspect(root, root / "seed_2");
    CHECK(report.errors() && !report.gameplayData && !report.frontendData);
    CHECK(report.text().find("invalid extraction manifest") != std::string::npos);
    CHECK(report.text().find("no usable recorded character") != std::string::npos);
    // Delete only the unique temporary fixture directory created above.
    bool safeFixture = fs::equivalent(root.parent_path(), fs::temp_directory_path()) &&
                       root.filename().string().rfind("CubeVulkan-world-test-", 0) == 0;
    CHECK(safeFixture);
    if (safeFixture) fs::remove_all(root);
}

struct RecordingFixture {
    fs::path root = fs::temp_directory_path() /
                    ("CubeVulkan-recording-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::vector<fs::path> files;
    RecordingFixture() {
        CHECK(fs::create_directory(root));
        fs::create_directories(root / "Extract" / "character");
        fs::create_directories(creatures());
    }
    fs::path creatures() const { return root / "Extract" / "seed_123" / "creatures"; }
    void text(const fs::path &file, const std::string &value) {
        files.push_back(file);
        std::ofstream out(file);
        out << value;
        CHECK(bool(out));
    }
    void clip(const fs::path &file, bool positioned, const std::vector<recording::Frame> &frames) {
        files.push_back(file);
        std::ofstream out(file, std::ios::binary);
        out.write(positioned ? "CWA2" : "CWA1", 4);
        auto put = [&](const auto &value) { out.write(reinterpret_cast<const char *>(&value), sizeof value); };
        for (auto &f : frames) {
            put(f.ms);
            if (positioned)
                for (auto p : f.position) put(p);
            put(f.state); put(f.timer); put(f.flags); put(f.ability);
            put(f.sinceAbility); put(f.yaw);
            for (auto v : f.velocity) put(v);
            for (auto s : f.slots) put(s);
            for (auto &m : f.matrices)
                for (auto v : m) put(v);
        }
        CHECK(bool(out));
    }
    ~RecordingFixture() {
        for (auto &file : files) fs::remove(file);
        fs::remove(creatures());
        fs::remove(creatures().parent_path());
        fs::remove(root / "Extract" / "character");
        fs::remove(root / "Extract");
        fs::remove(root);
    }
};

static recording::Frame recordedFrame(uint64_t ms) {
    recording::Frame f;
    f.ms = ms;
    f.position[0] = -65LL * 65536;
    f.position[1] = -65536;
    f.position[2] = 10LL * 65536;
    std::fill(std::begin(f.slots), std::end(f.slots), -1);
    glm::mat4 root(.1f);
    root[3] = glm::vec4(0, 0, 10, 1);
    memcpy(f.matrices[0], glm::value_ptr(root), 64);
    root[3].z = 9;
    memcpy(f.matrices[2], glm::value_ptr(root), 64);
    memcpy(f.matrices[15], glm::value_ptr(root), 64);
    return f;
}

static void recordingFormats() {
    RecordingFixture fixture;
    auto file = fixture.root / "Extract" / "character" / "format.cwa";
    auto a = recordedFrame(1000), b = recordedFrame(1033);
    a.state = 17; a.timer = 19; a.flags = 0x1234; a.ability = 24;
    a.sinceAbility = .25f; a.yaw = 1.5f;
    a.velocity[0] = 1; a.velocity[1] = 2; a.velocity[2] = 3;
    a.slots[0] = 9;
    fixture.clip(file, true, {a, b});
    std::vector<recording::Frame> frames;
    bool positioned = false;
    CHECK(recording::read(file, frames, &positioned) && positioned && frames.size() == 2);
    if (!frames.empty()) {
        const auto &f = frames[0];
        CHECK(f.position[0] == -65LL * 65536 && f.position[2] == 10LL * 65536);
        CHECK(f.state == 17 && f.timer == 19 && f.flags == 0x1234 && f.ability == 24);
        CHECK(f.sinceAbility == .25f && f.yaw == 1.5f && f.velocity[2] == 3 && f.slots[0] == 9);
        CHECK(recording::matrix(f, 0)[3].z == 10);
    }
    fixture.clip(file, false, {a, b});
    CHECK(recording::read(file, frames, &positioned) && !positioned && frames.size() == 2);
    CHECK(frames[0].position[0] == 0 && frames[0].ability == 24);
    fixture.clip(file, true, {a});
    { std::ofstream tail(file, std::ios::binary | std::ios::app); tail.write("abc", 3); }
    CHECK(recording::read(file, frames) && frames.size() == 1); // A live writer's partial next frame.
    fixture.clip(file, true, {b, a});
    CHECK(!recording::read(file, frames) && frames.empty()); // Time must not run backwards.
    fixture.text(file, "BAD!");
    CHECK(!recording::read(file, frames) && frames.empty());

    glm::mat4 m(.1f);
    m[3] = glm::vec4(0, 0, 10, 1);
    m[0].x = -.1f; // Native right feet can be mirrored.
    CHECK(recording::isPart(m));
    m[3][3] = std::numeric_limits<float>::quiet_NaN();
    CHECK(!recording::isPart(m));
    m[3][3] = 1;
    m[1] = glm::vec4(0);
    CHECK(!recording::isPart(m));
}

static void characterClipBoundaries() {
    RecordingFixture fixture;
    auto dir = fixture.root / "Extract" / "character";
    fixture.text(dir / "snapshot.txt", "sprite feet slot=7 sprites/foot.cub\n");
    fixture.clip(dir / "anim-1.cwa", false, {recordedFrame(1000), recordedFrame(1033)});
    fixture.clip(dir / "anim-2.cwa", true,
                 {recordedFrame(2000), recordedFrame(2033), recordedFrame(2066)});
    CharacterRecording character;
    std::string error;
    CHECK(character.load(fixture.root / "Extract" / "character", error));
    CHECK(character.frameCount() == 5 && character.parts().size() == 2);
    CHECK(character.summary().find("idle 3 frames") != std::string::npos);
    std::vector<glm::mat4> pose;
    character.pose(CharacterRecording::Motion::Idle, 0, 0, pose);
    CHECK(pose.size() == 2 && std::abs(pose[0][3].z) < 1e-5f && pose[1] == glm::mat4(0));
    CHECK(character.load(fixture.root / "Extract" / "character", error));
    CHECK(character.frameCount() == 5 && character.parts().size() == 2);
    CHECK(!character.load(fixture.root / "missing", error));
    character.pose(CharacterRecording::Motion::Idle, 0, 0, pose);
    CHECK(pose.empty());
}

static void nativeAttackPlayback() {
    CHECK(nativeAttackDuration(20, 1) == .8);
    CHECK(nativeAttackDuration(19, 1) == .5);
    CHECK(nativeAttackDuration(23, 1) == .55);
    CHECK(nativeAttackDuration(31, 1) == 1.5);
    CHECK(nativeAttackDuration(33, 1) == 1.5);
    CHECK(nativeAttackDuration(34, 1) == 2.9);
    CHECK(nativeAttackDuration(58, 1) == 1.4);
    CHECK(nativeAttackDuration(31, 2) == 1.05); // Fixed middle phase survives haste.
    CHECK(nativeAttackDuration(34, 2) == 2.05);
    CHECK(nativeAttackDuration(20, 0) == 0);
    CHECK(classMoves(3, 1).attack[0] == 33 && classMoves(3, 1).special == 34);
    RecordingFixture fixture;
    auto dir = fixture.root / "Extract" / "character";
    fixture.text(dir / "snapshot.txt", "name=Native pose fixture\nlevel=3\nsprite chest slot=7 sprites/body.cub\nsprite feet slot=8 sprites/foot.cub\n");
    std::vector<recording::Frame> frames;
    for (int i = 0; i < 6; i++) {
        auto f = recordedFrame(1000 + i * 100);
        if (i > 0 && i < 5) {
            f.state = 2; f.ability = 58; f.sinceAbility = (i % 2 == 0) ? 100.f : 0.f;
            f.yaw = i % 2 == 0 ? 90.f : 0.f;
            float angle = i % 2 == 0 ? 120.f : 90.f;
            auto matrix = glm::rotate(glm::translate(glm::mat4(1), glm::vec3(0, 0, 10.5f)),
                                      glm::radians(angle), glm::vec3(0, 0, 1)) * glm::scale(glm::mat4(1), glm::vec3(.1f));
            memcpy(f.matrices[0], glm::value_ptr(matrix), 64);
            memcpy(f.matrices[2], glm::value_ptr(matrix), 64);
        }
        frames.push_back(f);
    }
    fixture.clip(dir / "anim-1.cwa", true, frames);
    CharacterRecording character;
    std::string error;
    CHECK(character.load(dir, error));
    CHECK(character.name == "Native pose fixture" && character.level == 3);
    // Two consecutive uses of the same id are individual swings, not one 400ms clip.
    CHECK(std::abs(character.abilityDuration(58) - .2) < 1e-6);
    std::vector<glm::mat4> pose;
    CHECK(character.abilityPose(58, 0, pose));
    if (!pose.empty()) {
        CHECK(std::abs(pose[0][0].x) < 1e-5f && std::abs(pose[0][0].y - .1f) < 1e-5f);
        CHECK(std::abs(pose[0][3].z - 1.5f) < 1e-5f); // Native root bob survives.
    }
    CHECK(character.abilityPose(58, .05, pose));
    if (!pose.empty()) {
        float angle = std::atan2(pose[0][0].y, pose[0][0].x);
        CHECK(std::abs(angle - glm::radians(60.f)) < 1e-4f); // Smooth rotation, full scale.
        CHECK(std::abs(glm::length(glm::vec3(pose[0][0])) - .1f) < 1e-5f);
    }
    CHECK(!character.abilityPose(58, .2, pose) && pose.empty());
    CHECK(!character.abilityPose(58, -1, pose) && pose.empty());
    CHECK(!character.abilityPose(58, std::numeric_limits<double>::quiet_NaN(), pose));
    // Reloading another character must not leave the old ability ranges behind.
    frames = {recordedFrame(1000), recordedFrame(1100)};
    fixture.clip(dir / "anim-1.cwa", true, frames);
    CHECK(character.load(dir, error) && character.abilities().empty());
    CHECK(!character.load(fixture.root / "missing", error) && character.abilities().empty());
    CHECK(character.name.empty() && character.level == 0 && character.maxHp == 0);

    Assets assets;
    if (assets.locate("")) {
        for (auto &entry : listCharacters(assets.gameDir())) {
            std::error_code scanError;
            bool hasClips = false;
            for (const auto &file : fs::directory_iterator(entry.dir, scanError))
                hasClips |= file.is_regular_file(scanError) && file.path().extension() == ".cwa";
            CHECK(!scanError);
            if (scanError)
                continue;
            if (!hasClips) {
                std::printf("SKIP recorded attacks for %s: appearance snapshot only, no animation clips\n", entry.id.c_str());
                continue;
            }
            CharacterRecording actual;
            const bool loaded = actual.load(entry.dir, error);
            CHECK(loaded);
            if (!loaded) {
                std::printf("Recorded character %s: %s\n", entry.id.c_str(), error.c_str());
                continue; // Report the bad provided clip once, without cascading pose failures.
            }
            auto moves = classMoves(entry.classType, entry.specialization);
            for (int id : moves.attack) {
                CHECK(actual.abilityDuration(id) > 0);
                CHECK(std::abs(actual.abilityDuration(id) - nativeAttackDuration(id, actual.haste)) < 1e-6);
                for (double phase : {0., .25, .5, .75, .999}) {
                    CHECK(actual.abilityPose(id, phase * actual.abilityDuration(id), pose));
                    for (const auto &part : pose)
                        CHECK(part[3][3] == 0 || recording::isPart(part));
                }
            }
            if (entry.classType == 3 && entry.specialization == 1)
                CHECK(actual.abilityDuration(33) > 0 && actual.abilityDuration(34) > 0);
        }
    }
}

static void creatureRecordings() {
    RecordingFixture fixture;
    auto dir = fixture.creatures();
    // Latest manifest wins; an early empty appearance can be recovered from a
    // later frame's known sprite slot. Other unusable captures are reported.
    fixture.text(dir / "creatures.txt",
                 "id=1 race=2 sprites=-,-,old-body,-,-,-,-,-,-,-,-,-,-,-\n"
                 "id=1 race=3 flags2=00040200 equip=0:0:0:0:0,8:0:11:2:4294967295,4:0:28:1,5:0:1:0:-1 sprites=-,-,-,-,-,-,-,-,-,-,-,-,-,-\n"
                 "id=2 race=3 sprites=-,-,body,-,-,-,-,-,-,-,-,-,-,-\n"
                 "id=3 race=3 sprites=-,-,-,-,-,-,-,-,-,-,-,-,-,-\n"
                 "id=4 race=3 sprites=-,-,body,-,-,-,-,-,-,-,-,-,-,-\n");
    fixture.text(fixture.root / "Extract" / "sprites.txt", "7\tsprites/recorded-body.cub\t10\t8\t10\n");
    auto invalid = recordedFrame(1000), a = recordedFrame(1100), b = recordedFrame(1200);
    memset(invalid.matrices, 0, sizeof invalid.matrices);
    a.slots[2] = b.slots[2] = 7;
    b.position[0] += 65536;
    fixture.clip(dir / "c_1.cwa", true, {invalid, a, b});
    fixture.clip(dir / "c_2.cwa", false, {a}); // NPC playback needs world positions.
    fixture.clip(dir / "c_3.cwa", true, {invalid});
    fixture.clip(dir / "c_4.cwa", true, {invalid});
    auto local = [](const int64_t *p) { return glm::vec3(p[0] / 65536., p[1] / 65536., p[2] / 65536. + 128); };
    CreatureRecordings recordings;
    CHECK(recordings.load(dir, local) == 1);
    CHECK(recordings.stats().listed == 4 && recordings.stats().missingFrames == 1 &&
          recordings.stats().missingAppearance == 1 && recordings.stats().missingPose == 1);
    if (!recordings.all().empty()) {
        auto &c = recordings.all()[0];
        CHECK(c.race == 3 && c.parts.size() == 1 && c.parts[0].sprite == "recorded-body");
        CHECK(c.flags2.has_value() && *c.flags2 == 0x40200);
        CHECK(c.worn.size() == 4 && c.worn[1].modifier == UINT32_MAX && c.worn[2].category == 4 &&
              !c.worn[2].modifier.has_value() && c.worn[3].category == 0);
        CHECK(c.times.size() == 2 && c.times[0] == 0 && c.positions[0] == glm::vec3(-65, -1, 138));
        CHECK(std::abs(c.poses[0][0][3].z + 1) < 1e-5f);
        CHECK(recordings.frameAt(c, .05) == 0 && recordings.frameAt(c, .1) == 1 && recordings.frameAt(c, .15) == 0);
    }
    CHECK(recordings.load(dir, local) == 1); // Reload must not duplicate creatures.
    fixture.text(dir / "creatures.txt", "id=1 race=3 flags2=invalid equip=3:3:1:0 sprites=-,-,body,-,-,-,-,-,-,-,-,-,-,-\n");
    CHECK(recordings.load(dir, local) == 1);
    CHECK(!recordings.all()[0].flags2.has_value() && recordings.all()[0].worn.size() == 1 &&
          !recordings.all()[0].worn[0].modifier.has_value()); // no stale metadata, legacy stays readable
}

static void nativeLootSelection() {
    CreatureRecordings::Creature c;
    c.hostility = 1; c.flags2 = 0; c.worn.resize(11);
    for (size_t i = 1; i <= 7; i++) c.worn[i] = {3, 0, 1, 0};
    c.worn[10] = {10, 0, 0, 0};
    auto scripted = [&](std::initializer_list<unsigned> values) {
        auto at = values.begin();
        auto selection = nativeLoot::select(c, [&] {
            CHECK(at != values.end());
            return at == values.end() ? 32767u : *at++;
        });
        CHECK(at == values.end());
        return selection;
    };
    // Native candidate insertion order, distinct from numerical equipment order.
    const size_t nativeOrder[] = {6, 7, 2, 5, 4, 3, 1};
    for (unsigned choice = 0; choice < 7; choice++) {
        auto s = scripted({0, choice, 0, 0});
        CHECK(s.wornSlot == nativeOrder[choice] && s.pet && s.generated && !s.legacyFlags);
    }
    c.worn[6].id = 22; c.worn[7].id = 23; c.worn[2].id = 1;
    CHECK(scripted({0, 0, 1, 1}).wornSlot == 5); // native excludes these three items
    c.flags2 = 0x800;
    auto suppressed = scripted({});
    CHECK(suppressed.wornSlot == SIZE_MAX && !suppressed.pet && !suppressed.generated);
    c.flags2 = 0x200;
    auto guaranteed = scripted({15, 0, 1, 19});
    CHECK(guaranteed.wornSlot == 5 && guaranteed.generated && !guaranteed.pet);
    c.flags2 = 0x40000;
    auto boss = scripted({15, 1, 19});
    CHECK(boss.wornSlot == SIZE_MAX && boss.generated && !boss.pet);
    c.hostility = 7;
    CHECK(!scripted({15, 1, 19}).generated); // the override is for hostility1 only
    c.flags2.reset();
    CHECK(scripted({15, 1, 19}).legacyFlags);
    c.flags2 = 0;
    for (int hostility : {0, 3}) {
        c.hostility = hostility;
        auto s = scripted({});
        CHECK(s.wornSlot == SIZE_MAX && !s.pet && !s.generated);
    }
    // Exhaust the native 15-bit random domain: non-power-of-two chances retain
    // the measured modulo arithmetic rather than an idealized uniform coin.
    size_t worn = 0, generated = 0, pets = 0;
    c.worn.assign(11, {}); c.worn[1] = {8, 0, 11, 0}; c.hostility = 2;
    for (unsigned draw = 0; draw < 32768; draw++) {
        unsigned calls = 0;
        worn += nativeLoot::select(c, [&] { return calls++ ? 0u : draw; }).wornSlot != SIZE_MAX;
    }
    c.worn.clear(); c.hostility = 1;
    for (unsigned draw = 0; draw < 32768; draw++) generated += nativeLoot::select(c, [&] { return draw; }).generated;
    c.worn.resize(11); c.worn[10] = {10, 0, 0, 0}; c.hostility = 2;
    for (unsigned draw = 0; draw < 32768; draw++) pets += nativeLoot::select(c, [&] { return draw; }).pet;
    CHECK(worn == 2048 && generated == 1639 && pets == 656);
}

static void reflectedFaces() {
    // A native mirrored foot reverses the outward face's triangle winding.
    glm::vec4 corners[3];
    for (int i = 0; i < 3; i++)
        corners[i] = glm::vec4(FACE_CORNERS[0][i][0], FACE_CORNERS[0][i][1], FACE_CORNERS[0][i][2], 1);
    for (int flips = 0; flips <= 2; flips++) {
        DrawItem item;
        item.model[0].x = flips ? -.1f : .1f;
        item.model[1].y = flips == 2 ? -.1f : .1f;
        item.model[2].z = .1f;
        glm::vec3 a(item.model * corners[0]), b(item.model * corners[1]), c(item.model * corners[2]);
        glm::vec3 outward(item.model * glm::vec4(1, 0, 0, 0));
        float orientation = glm::dot(glm::cross(b - a, c - a), outward);
        CHECK((orientation < 0) == item.mirrored());
        CHECK(item.mirrored() == (flips == 1));
    }
}

static void steamFailsClosed() {
    RecordingFixture fixture;
    SteamSession steam;
    std::string error;
    CHECK(!steam.start(fixture.root, error) && !error.empty());
    CHECK(!steam.check(error) && !error.empty());
    std::string stale = "previous data";
    CHECK(steam.cloudReadState("cubevulkan.sav", stale) == SteamSession::CloudRead::Unavailable && stale.empty());
    steam.pump(); // Missing runtime does not create a usable session.
}

// Damage follows the game's own rules (Combat.h, docs/combat.md), checked against
// hits recorded in the vanilla game on 2026-10-22 with ZoneExtractor's hit probe:
// the damage each hit carried, the defender's armor, the attacker's combo, and the
// health the defender actually lost.
static void damageMatchesNativeHits() {
    struct Sample { float damage, armor; int combo, maxCombo; float lost; };
    const Sample samples[] = {
        {54.0190f, 1.5417f, 0, 30, 52.480f},  // a level 2 monster (race 294) hitting the test warrior
        {40.5848f, 1.5417f, 1, 30, 39.090f},  // ...its combo now ignores 1/30 of the armor
        {57.6673f, 1.5417f, 2, 30, 56.230f},
        {16.3224f, 1.5417f, 0, 30, 14.780f},  // a level 1 monster (race 71)
        {7.3701f, 4.8388f, 0, 30, 2.5313f},   // the warrior hitting a level 0 monster (race 27)
        {5.8248f, 4.8388f, 1, 30, 1.1473f},
        {5.4709f, 4.8388f, 2, 30, .9547f},
        {8.8706f, 4.8388f, 3, 30, 4.5157f},
        {7.1957f, 24.1303f, 0, 30, 0.f},      // too weak for a level 2 monster's armor: nothing
    };
    for (auto &s : samples)
        CHECK(std::abs(combat::taken(s.damage, s.armor, s.combo, s.maxCombo) - s.lost) < .006f);
    CHECK(combat::taken(10, 5, 30, 30) == 10); // a full combo ignores armor

    // Outgoing damage: the ability's multiplier times power, times 0.75-1.25, doubled when critical.
    std::mt19937 dice(7);
    float lowest = 1e9f, highest = 0;
    double sum = 0;
    const int N = 20000;
    for (int i = 0; i < N; i++) {
        float d = combat::outgoing(46.6141f, 76, 0, false, dice);
        lowest = std::min(lowest, d), highest = std::max(highest, d), sum += d;
    }
    CHECK(lowest >= .75f * 46.6141f - .01f && highest <= 1.25f * 46.6141f + .01f);
    CHECK(std::abs(sum / N - 46.6141) < .5);
    CHECK(combat::outgoing(10, 31, 0, false, dice) <= 10 * .4f * 1.25f + 1e-4f); // mage cast: x0.4
    float critical = combat::outgoing(10, 76, 0, true, dice);
    CHECK(critical >= 15 - 1e-4f && critical <= 25 + 1e-4f);
    CHECK(combat::abilityMultiplier(53) == 3 && combat::abilityMultiplier(76) == 1);
    // Armor counts against critical chance: 0.05 per 16 armor.
    int criticals = 0;
    for (int i = 0; i < N; i++)
        criticals += combat::critical(.1f, 16, dice);
    CHECK(std::abs(criticals / double(N) - .05) < .01);
    CHECK(combat::maxCombo(-1) == 30 && combat::maxCombo(-1, 0x10) == 15 && combat::maxCombo(17) == 15 &&
          combat::maxCombo(3) == 50 && combat::maxCombo(6) == 60 && combat::maxCombo(8) == 80 && combat::maxCombo(11) == 20);
}

// Effects, charge and stun follow the decoded native rules (docs/combat.md).
// NativeHit's paths the recorded hits don't reach (creature-hit.md §2.7, §2.8, §2.11):
// a mana shield soaking a hit, poison, and a guard blocking.
static void nativeHitPaths() {
    std::vector<uint8_t> targetRecord(nativepose::CREATURE_SIZE), attackerRecord(nativepose::CREATURE_SIZE);
    nativeai::Creature target, attacker;
    target.record = targetRecord.data(), attacker.record = attackerRecord.data();
    auto put = [](uint8_t *r, size_t o, auto v) { std::memcpy(r + o, &v, sizeof v); };
    auto getf = [](const uint8_t *r, size_t o) {
        float v;
        std::memcpy(&v, r + o, 4);
        return v;
    };
    put(target.record, 0x08, int64_t(1)), put(attacker.record, 0x08, int64_t(2));
    target.record[0x60] = attacker.record[0x60] = 1;
    put(target.record, 0x180, 100.0f), put(attacker.record, 0x180, 100.0f);
    size_t draws = 0;
    nativehit::World w;
    w.local = attacker.record; // a server with a player somewhere
    w.creatures = {&target, &attacker};
    w.byId = [&](int64_t id) { return id == 1 ? &target : id == 2 ? &attacker : nullptr; };
    w.rand = [&] { return draws++, 16384; };
    nativeupkeep::Hit h;
    h.target = 1, h.attacker = 2, h.damage = 20.0f;

    // A shield of 50 soaks all 20: it lands, the shield is worn to 30, health untouched.
    target.buffs.push_back(nativebuffs::make(6, 50.0f, 1000.0f, 0));
    CHECK(nativehit::hit(w, &target, &attacker, h));
    CHECK(w.numbers.size() == 1 && w.numbers[0][0x45] == 5 && draws == 1);
    CHECK(nativebuffs::strength(target.buffs[0]) == 30.0f && getf(target.record, 0x180) == 100.0f);
    target.buffs.clear(), target.aggro.clear(), draws = 0;
    w.numbers.clear(), w.buffNotes.clear(), w.aggroNotes.clear(), w.sounds.clear();

    // Poison: a buff of a fifth of the damage for 3 s, and the hit lands in full (no armour).
    h.ability = 0x12, h.damage = 10.0f;
    CHECK(nativehit::hit(w, &target, &attacker, h));
    CHECK(target.buffs.size() == 1 && target.buffs[0][0] == 4 && nativebuffs::strength(target.buffs[0]) == 2.0f);
    CHECK(getf(target.record, 0x180) == 90.0f && w.buffNotes.size() == 1 && draws == 1);
    CHECK(target.aggro[2] > 0.0f && w.aggroNotes.size() == 1);
    target.buffs.clear(), target.aggro.clear(), draws = 0;
    w.numbers.clear(), w.buffNotes.clear(), w.aggroNotes.clear(), w.sounds.clear();

    // A guard (ability 9) with block power blocks: no damage, power spent down to -1.
    h.ability = 0, h.last = 1;
    target.record[0x68] = 9;
    put(target.record, 0x184, 1.0f);
    CHECK(!nativehit::hit(w, &target, &attacker, h));
    CHECK(w.numbers.size() == 1 && w.numbers[0][0x45] == 1 && getf(target.record, 0x180) == 90.0f);
    CHECK(getf(target.record, 0x184) == -1.0f && draws == 1);
}

// Blocking, as the game's hit routine and block factor (0x50370) work it out.
static void blocking() {
    // Modifier 0 rolls nothing, so an item's power is 2^rarity times its kind's factor.
    CHECK(combat::itemPower(3, 13, 2, 0) == 16);           // shield: x4
    CHECK(combat::itemPower(3, 3, 1, 0) == 4);             // dagger: x2
    CHECK(combat::itemPower(3, 15, 0, 0) == 8);            // greatsword: x8
    CHECK(combat::itemPower(3, 11, 0, 0, 0, false) == 6);  // a monster's wand: x6
    CHECK(combat::itemPower(4, 13, 2, 0) == 0);            // not a weapon
    uint32_t x = 12345; // the modifier's xorshift picks up to +0.5 rarity
    x ^= x << 13, x ^= x >> 17, x ^= x << 5;
    CHECK(std::abs(combat::itemPower(3, 0, 1, 12345) - 4 * std::pow(2.f, 1 + (x % 1000) / 2000.f)) < 1e-4f);
    combat::HeldItem shield{3, 13, 2, 0}, sword{3, 0, 0, 0};
    CHECK(combat::blockFactor(shield, sword, 1, 1, 0) == 5 + 16 * 6); // guardian
    CHECK(combat::blockFactor(shield, sword, 1, 0, 0) == 5 + 16 * 4);
    CHECK(combat::blockFactor(sword, sword, 2, 0, 0x40000) == (5 + 4 + 4) * 4); // elite x4
    CHECK(combat::guarding(95, 1, 0, false) && combat::guarding(60, 1, 1, false));
    CHECK(!combat::guarding(60, 1, 0, false) && !combat::guarding(95, 1, 0, true) && !combat::guarding(58, 1, 0, false));

    std::mt19937 dice(4);
    float block = 1, mana = 0;
    combat::Defender d{0, 0, nullptr, &block, true, 60, 101, &mana, 1};
    combat::Attacker a{50, 0, 0, 58};
    combat::Hit h = combat::hit(a, d, dice);
    CHECK(h.blocked && h.damage == 0 && block < 1 && block > 0 && mana > 0 && mana <= .5f);
    float drained = 1 - block;
    CHECK(std::abs(mana - (1 - drained) * .5f) < 1e-5f);
    while (block > 0)
        combat::hit(a, d, dice);
    CHECK(block >= -1 && !combat::hit(a, d, dice).blocked); // spent: hits land again
    combat::refillBlock(block, 1, 60, true); // no refill while guarding...
    float spent = block;
    combat::refillBlock(block, 1, 0, false); // ...0.5 a second otherwise
    CHECK(std::abs(block - std::min(1.f, spent + .5f)) < 1e-5f);
    d.guarding = false, block = 1;
    CHECK(!combat::hit(a, d, dice).blocked);
}

// Poison hits again each time its time left crosses 400 ms: 7 times over 3 s,
// whatever the frame rate.
static void poisonTicks() {
    for (double dt : {1 / 30.0, 1 / 60.0, 1 / 144.0}) {
        int ticks = 0;
        for (double left = 3; left > 0; left -= dt)
            ticks += combat::poisonTick(left, left - dt);
        CHECK(ticks == 7);
    }
}

static void combatEffects() {
    std::mt19937 dice(3);
    combat::Active e;
    CHECK(combat::skillEffect(115, 10, e) && e.type == combat::MANA_SHIELD && e.value == 40 && e.left == 30);
    CHECK(combat::skillEffect(113, 0, e) && e.type == combat::BULWARK && e.value == .5f && e.left == 10);
    CHECK(!combat::skillEffect(58, 0, e));
    combat::Effects shield, bulwark, frenzy;
    shield.add(combat::MANA_SHIELD, 1000, 30);
    bulwark.add(combat::BULWARK, .5f, 10);
    frenzy.add(combat::WAR_FRENZY, 0, 8);
    combat::Attacker a{20, 20, 0, 76, 0, 30};
    CHECK(combat::hit(a, {0, 0, &shield}, dice).absorbed); // a shield stronger than the hit takes it all
    a.effects = &frenzy;
    combat::Hit plain = combat::hit(a, {2, 2}, dice);
    CHECK(plain.damage >= 13 && plain.damage <= 23 && std::abs(plain.healed - plain.damage * .5f) < 1e-4f);
    combat::Hit halved = combat::hit(a, {2, 2, &bulwark}, dice);
    CHECK(halved.damage >= 6.5f && halved.damage <= 11.5f && std::abs(halved.healed - halved.damage) < 1e-3f);
    shield.update(31);
    CHECK(shield.list.empty()); // effects end
    // Charge: x(1 + 5 charge^2), halved for abilities 6 and 12, and it is the stun chance.
    CHECK(combat::releasedCharge(6, .8f) == .4f && combat::releasedCharge(56, .8f) == .8f);
    float charged = combat::outgoing(10, 76, 1, false, dice);
    CHECK(charged >= 45 - 1e-3f && charged <= 75 + 1e-3f);
    CHECK(std::abs(combat::stunChance(76, .5f, true, 0, 0) - .45f) < 1e-5f);
    CHECK(std::abs(combat::stunChance(59, 0, false, 0, 0) - .09f) < 1e-5f);              // a monster's stunning swing
    CHECK(std::abs(combat::stunChance(59, 0, true, 0, combat::ELITE) - .135f) < 1e-5f);  // against an elite
    CHECK(combat::stunSeconds(true) == 1.5 && combat::stunSeconds(false) == 3);
    // Mana per landed hit: 0.1 +- 0.05, x length/500 ms, x1.5 one time in eight.
    double sum = 0;
    for (int i = 0; i < 8000; i++) {
        float m = combat::manaFromHit(23, .55, 1, dice);
        CHECK(m >= .05f * 1.1f - 1e-4f && m <= .15f * 1.1f * 1.5f + 1e-4f);
        sum += m;
    }
    CHECK(std::abs(sum / 8000 - .1 * 1.1 * 1.0625) < .004);
}

// Equipping and taking off gear follows what the game measured (when the
// user's gear survey is present): the cotton chest added 15.7 HP.
static void gearFollowsSurvey() {
    Assets a;
    Gear gear;
    std::string why;
    if (!a.locate("") || !gear.load(a.gameDir() / "Extract" / "characters" / "dieudeglace-assassin", why))
        return; // No survey on this machine.
    float before = gear.stats().maxHp;
    if (!gear.wornMeasured(Gear::Chest))
        return;
    float chest = gear.equipped(Gear::Chest).adds.maxHp;
    gear.unequip(Gear::Chest);
    CHECK(std::abs(gear.stats().maxHp - (before - chest)) < .01f);
    for (size_t i = 0; i < gear.itemCount(); i++)
        if (gear.slotOf(i) == Gear::Chest && !gear.isWorn(i))
            gear.equip(i);
    CHECK(std::abs(gear.stats().maxHp - before) < .01f);
}

// Monsters react at the distances measured in the game (docs/creatures.md): one
// that can walk notices a player 26 blocks away, runs at 11.5 blocks/s, swings when
// within 3.1 blocks, and gives the chase up beyond 42. Uses the owner's recordings.
static void monsterBehaviour() {
    Assets a;
    ExtractedWorld ex;
    std::string why;
    if (!a.locate("") || !ex.open(zonesDir(a.gameDir()), why))
        return; // No extracted world on this machine.
    CreatureRecordings recordings;
    if (!recordings.load(ex.directory() / "creatures", [&](const int64_t *dots) { return ex.toLocal(dots); }))
        return;
    // Level ground wherever a walker looks (Actors asks from 1.2 blocks above its feet):
    // grounded and flying recordings alike can walk.
    auto noFloor = [](float, float, float from, float &h) { return h = from - 1.2f, true; };
    int checked = 0;
    for (size_t k = 0; checked < 3; k++) {
        Actors actors;
        actors.build(recordings);
        if (k >= actors.count())
            break;
        if (!actors.hittable(k) || glm::length(glm::vec2(actors.position(k))) > 1e5f)
            continue; // (recorded far from the extraction's home, where floats lose the steps)
        bool alone = true; // pack-mates push each other; measure a lone monster
        for (size_t j = 0; j < actors.count(); j++)
            alone = alone && (j == k || glm::length(glm::vec2(actors.position(j) - actors.position(k))) > 10);
        if (!alone)
            continue;
        glm::vec3 home = actors.position(k), player = home + glm::vec3(26, 0, 0);
        float damage = 0;
        for (int i = 0; i < 10; i++) // Others may be closer to this spot; only k matters.
            actors.update(.05, player, noFloor, combat::Defender{}, damage);
        if (!actors.engaged(k))
            continue; // cannot walk (a fixed or swimming creature)
        glm::vec3 before = actors.position(k);
        for (int i = 0; i < 10; i++)
            actors.update(.05, player, noFloor, combat::Defender{}, damage);
        float ran = glm::length(glm::vec2(actors.position(k) - before));
        CHECK(ran > 4.5f && ran < 5.8f); // 11.5 blocks/s for 0.5 s, less its turn
        auto gapTo = [&](glm::vec3 p) { return glm::length(glm::vec2(actors.position(k) - p)); };
        for (int i = 0; i < 80 && gapTo(player) > 3.1f; i++)
            actors.update(.05, player, noFloor, combat::Defender{}, damage);
        CHECK(gapTo(player) <= 3.1f); // within reach (it swings there)
        for (int i = 0; i < 20; i++)
            actors.update(.05, player, noFloor, combat::Defender{}, damage);
        CHECK(gapTo(player) > 1.5f && gapTo(player) < 2.6f); // and stands about 2 blocks off
        glm::vec3 away = actors.position(k) + glm::vec3(60, 0, 0);
        actors.update(.05, away, noFloor, combat::Defender{}, damage);
        actors.update(.05, away, noFloor, combat::Defender{}, damage);
        glm::vec3 at = actors.position(k);
        for (int i = 0; i < 10; i++)
            actors.update(.05, away, noFloor, combat::Defender{}, damage);
        float toward = glm::length(glm::vec2(at - away)) - glm::length(glm::vec2(actors.position(k) - away));
        CHECK(toward < .5f); // no longer coming
        checked++;
    }
    CHECK(checked > 0);
}

// Death is permanent: the game's world state lists the dead (GameWorld.h), and
// CubeVulkan's kills are saved per world and joined across characters.
static void deadCreatures() {
    auto state = [](int version, std::vector<int64_t> ids) {
        std::string s(16 + 8 * ids.size() + 8, '\0');
        int32_t head[4] = {version, 135, 0x4c8314bf, (int32_t)ids.size()};
        std::memcpy(s.data(), head, sizeof head);
        std::memcpy(s.data() + 16, ids.data(), 8 * ids.size());
        return s;
    };
    // The order and form the game wrote on 2026-10-22: negative ids first.
    std::vector<int64_t> ids = {(int64_t)0x80000000000003e0ull, (int64_t)0x7ffffcc07fffffc2ull,
                                (int64_t)0x7ffffd407fffffc1ull, (int64_t)0x7ffffd407fffffc2ull};
    std::set<int64_t> dead;
    CHECK(gameworld::parseDead(state(44, ids), dead) && dead.size() == 4 && dead.count((int64_t)0x7ffffd407fffffc2ull));
    CHECK(!gameworld::parseDead(state(43, ids), dead));                     // another version
    CHECK(!gameworld::parseDead(state(44, {ids[1], ids[0]}), dead));         // not sorted
    CHECK(!gameworld::parseDead(std::string(10, '\0'), dead));

    SaveGame a;
    a.world = "seed_123";
    a.killed = {ids[0], ids[3]};
    SaveGame back;
    CHECK(back.parse(a.text()) && back.killed == a.killed);
    SaveGame b;
    b.world = "seed_123";
    b.killed = {ids[1]};
    SaveGame elsewhere;
    elsewhere.world = "seed_7";
    elsewhere.killed = {ids[2]};
    std::string bundle = SaveGame::intoBundle("", "warrior-warrior", a.text());
    bundle = SaveGame::intoBundle(bundle, "mage-mage", b.text());
    bundle = SaveGame::intoBundle(bundle, "rogue-rogue", elsewhere.text());
    auto kills = SaveGame::worldKills(bundle, "seed_123");
    CHECK(kills.size() == 3 && !kills.count(ids[2]));
}

// Debug aid: `EngineTests props X Y` lists extracted props within 4 blocks.
static int listProps(float x, float y) {
    Assets a;
    ExtractedWorld ex;
    std::string why;
    if (!a.locate("") || !ex.open(zonesDir(a.gameDir()), why))
        return 1;
    int zx = (int)std::floor(x / 64), zy = (int)std::floor(y / 64);
    for (auto &s : ex.props(zx, zy))
        if (std::abs(s.x - x) < 4 && std::abs(s.y - y) < 4)
            std::printf("%-20s %.2f %.2f %.2f yaw %.2f scale %.3f\n", s.model.c_str(), s.x, s.y, s.z, s.yaw, s.scale);
    return 0;
}

// Crafting (docs/specs/loot-crafting.md): the recipe table, the default recipes, the crafting
// lists and their rank gear, the craft button's timing, the extra yield and the upgrade cubes.
static void craftingRules() {
    using namespace gamecraft;
    // A saved recipe result retains its full formula and every attribute byte.
    auto savedRecipe = nativesaves::makeItem(3, 0, 1);
    const int32_t formulaWord = 0x12345678;
    std::memcpy(savedRecipe.data() + 0x18, &formulaWord, 4);
    savedRecipe[0x9d] = 2;
    for (size_t i = 0; i < 128; ++i) savedRecipe[0x1d + i] = (uint8_t)(i * 7 + 3);
    const auto complete = fromRecord(savedRecipe);
    const auto completeResult = recipeOf(complete).result;
    CHECK(toRecord(completeResult) == savedRecipe && completeResult.formula == formulaWord);
    auto inactive = complete;
    inactive.attributes[127] ^= 0xff;
    CHECK(inactive == complete); // inactive data survives the result but is not an identity key
    inactive.attributes[0] ^= 1;
    CHECK(!(inactive == complete));
    auto differentFormula = complete;
    differentFormula.formula ^= 0x100;
    CHECK(!(differentFormula == complete));
    CHECK(toRecord(recipeOf(fromRecord(nativesaves::makeItem(29, 4))).result) == ItemRecord{});
    Thing sword;
    sword.category = 3, sword.id = 0, sword.material = 1, sword.rarity = 2; // a rarity-2 iron sword
    Formula f = recipeOf(sword);
    CHECK(f.workstation == Anvil && f.ingredients.size() == 2);
    CHECK(f.ingredients[0].item.id == 10 && f.ingredients[0].item.material == 1 && f.ingredients[0].quantity == 15);
    CHECK(f.ingredients[1].item.id == 0x21 && f.ingredients[1].item.rarity == 2 && f.ingredients[1].quantity == 2); // the tier stone
    Thing greatsword = sword;
    greatsword.id = 15; // two-handed: twice the cubes, four stones
    f = recipeOf(greatsword);
    CHECK(f.ingredients[0].quantity == 30 && f.ingredients[1].quantity == 4);
    Thing bow;
    bow.category = 3, bow.id = 7, bow.material = 2, bow.rarity = 0; // iron, wood and yarn at the workbench
    f = recipeOf(bow);
    CHECK(f.workstation == Workbench && f.ingredients.size() == 3 && f.ingredients[1].quantity == 8 && f.ingredients[2].item.material == 0x1b);
    Thing chest;
    chest.category = 4, chest.material = 0x1a, chest.rarity = 1; // cloth: yarn at the loom, and four stones
    f = recipeOf(chest);
    CHECK(f.workstation == Loom && f.ingredients[0].item.id == 9 && f.ingredients[0].quantity == 20 && f.ingredients[1].quantity == 4);
    Thing potion;
    potion.category = 1, potion.id = 1, potion.rarity = 3; // consumables take no tier stone
    f = recipeOf(potion);
    CHECK(f.ingredients.size() == 3 && f.ingredients[1].item.id == 0x1e && f.ingredients[1].item.rarity == 3);
    CHECK(f.ingredients[2].item.id == 0x1a && f.workstation == None);
    Thing planks;
    planks.category = 11, planks.id = 10, planks.material = 2; // sawn from a log
    f = recipeOf(planks);
    CHECK(f.workstation == Saw && f.ingredients.size() == 1 && f.ingredients[0].item.id == 1 && f.ingredients[0].item.material == 2);
    Thing dish;
    dish.category = 0x1c, dish.id = 5;
    f = recipeOf(dish);
    CHECK(f.workstation == Campfire && f.ingredients.size() == 3 && f.ingredients[2].item.id == 0x1a);
    Thing none;
    none.category = 0x1d, none.rarity = 2;
    f = recipeOf(none);
    CHECK(f.result.category == 0 && f.ingredients.empty());
    // The default recipes: 7 consumables, 7 dishes, 2 resources, 4 cubes; the yarn of all but warriors.
    CHECK(startingFormulas(1).size() == 20 && startingFormulas(3).size() == 21);
    // The lists with rank 0 only.
    auto rankZero = [](int rank) { return rank == 0; };
    const auto tabs = formulas(startingFormulas(1), 1, 3, -2, rankZero);
    CHECK(tabs[Weapons].size() == 3 && tabs[Armor].size() == 4 && tabs[Amulets].size() == 2);
    CHECK(tabs[Cooking].size() == 8 && tabs[Alchemy].size() == 6 && tabs[Ingredients].size() == 6);
    // The rank gear's draws, by hand: srand(R(3 + 57 * -2 + 0xf5f9) + rank), a warrior's order.
    {
        worldgen::CrtRandom r;
        r.seed(worldgen::random(worldgen::cellKey(3, -2) + 0xf5f9u));
        const int subtype = r.next() % 3;
        const uint32_t m0 = (uint32_t)r.next(), m1 = (uint32_t)r.next();
        const int subtype2 = r.next() % 3 + 15;
        const uint32_t m2 = (uint32_t)r.next();
        uint32_t armour[4];
        for (uint32_t &m : armour)
            m = (uint32_t)r.next();
        const int neck = r.next() % 2 == 0 ? 11 : 12;
        const uint32_t m7 = (uint32_t)r.next();
        const std::vector<Formula> &w = tabs[Weapons];
        CHECK(w[0].result.id == subtype && w[0].result.modifier == m0 && w[0].result.region[0] == 3 && w[0].result.region[1] == -2);
        CHECK(w[1].result.id == 13 && w[1].result.modifier == m1);
        CHECK(w[2].result.id == subtype2 && w[2].result.modifier == m2);
        CHECK(tabs[Armor][0].result.category == 4 && tabs[Armor][1].result.category == 7 && tabs[Armor][3].result.category == 6);
        CHECK(tabs[Armor][2].result.modifier == armour[2] && tabs[Armor][0].result.material == 1);
        CHECK(tabs[Amulets][0].result.material == neck && tabs[Amulets][0].result.modifier == m7);
    }
    // The same region gives the same gear; two unlocked ranks give twice as much.
    const auto again = formulas({}, 3, 3, -2, rankZero);
    CHECK(again[Weapons].size() == 3 && again[Weapons][0].result.id == 11 && again[Weapons][2].result.id == 12);
    CHECK(formulas({}, 3, 3, -2, rankZero)[Weapons][2].result.material == again[Weapons][2].result.material);
    const auto twoRanks = formulas({}, 2, 3, -2, [](int rank) { return rank <= 1; });
    CHECK(twoRanks[Weapons].size() == 6 && twoRanks[Weapons][3].result.rarity == 1 && twoRanks[Armor][4].result.material == 0x1b);
    // Times: the fewest an ingredient allows.
    auto have = [](const Thing &t) { return t.id == 10 ? 31 : t.id == 0x21 ? 5 : 0; };
    CHECK(times(recipeOf(sword), have) == 2);
    CHECK(times(recipeOf(none), have) == 0);
    // The extra yield: one in five, up to three times, drawing only while it succeeds.
    {
        std::vector<int> draws = {0, 5, 3, 0};
        size_t used = 0;
        auto scripted = [&] { return draws[used++]; };
        CHECK(extraPieces(planks, scripted) == 2 && used == 3);
        used = 0;
        Thing flask;
        flask.category = 11, flask.id = 0x1a;
        CHECK(extraPieces(flask, scripted) == 0 && used == 0);
        CHECK(extraPieces(sword, scripted) == 0 && used == 0);
        draws = {10, 15, 20, 25};
        CHECK(extraPieces(planks, scripted) == 3 && used == 3);
    }
    // The craft button: 1000 ms for the first item, then 1.2 times faster each, down to 200 ms.
    {
        CraftButton b;
        auto holdUntilMade = [&](bool &first) {
            int ms = 0;
            bool started = false;
            while (!b.update(1.0f, true, true, &started))
                ms++, first |= started;
            return ms + 1;
        };
        bool started = false;
        const int first = holdUntilMade(started);
        CHECK(started && first >= 999 && first <= 1001);
        const int second = holdUntilMade(started);
        CHECK(second >= 832 && second <= 835); // 1000 / 1.2
        for (int k = 0; k < 10; k++)
            holdUntilMade(started);
        CHECK(b.speed == CraftButton::TOP_SPEED);
        const int capped = holdUntilMade(started);
        CHECK(capped >= 199 && capped <= 201);
        // Releasing slows it again; a started item still completes.
        b.update(100.0f, true, true);
        CHECK(b.progress > 0.0f);
        b.update(1.0f, true, false);
        CHECK(b.speed == CraftButton::START_SPEED && b.progress > 0.0f);
        b.update(1.0f, false, false); // closing the window drops it
        CHECK(b.progress == -1.0f);
    }
    CHECK(requirement(Workbench) == "Requires Workbench" && stationEntity(Anvil) == 0x63 && stationEntity(Water) == -1);
    // Upgrade cubes: 16 on a one-handed weapon, 32 on a two-handed one; the weapon damage reads them.
    {
        std::array<uint8_t, 0xa0> w{};
        w[0] = 3, w[0x14] = 2; // a sword of rarity 2
        CHECK(cubeCap(w.data()) == 16);
        const std::vector<uint8_t> holder(worldgen::CREATURE_SIZE);
        const float plain = nativerules::weaponDamage(w.data(), holder.data());
        for (int k = 0; k < 16; k++)
            CHECK(placeCube(w.data(), (int8_t)k, 0, 1, 11));
        CHECK(!placeCube(w.data(), 20, 0, 1, 11) && w[0x9d] == 16);
        CHECK(placeCube(w.data(), 3, 0, 1, 12) && w[0x9d] == 16 && w[0x1d + 3 * 4 + 3] == 12); // only recoloured
        CHECK(nativerules::weaponDamage(w.data(), holder.data()) > plain);
        CHECK(removeCube(w.data(), 0, 0, 1) && w[0x9d] == 15 && w[0x1d] == 1 && !removeCube(w.data(), 0, 0, 1));
        std::array<uint8_t, 0xa0> big{};
        big[0] = 3, big[4] = 15;
        CHECK(cubeCap(big.data()) == 32);
    }
}

// Shops (docs/specs/loot-shops.md): the stock and its seeds, the bought list, the tier, prices,
// the buy-back list and identifying.
static void shopRules() {
    using namespace gameshop;
    auto none = [](const Item &) { return 0; };
    Shopkeeper general;
    general.cls = GENERAL_SHOP, general.id = 0x1234567800000042LL;
    // The general shop: flasks, a resource and type 1:9 without limit, one pet food of the day.
    {
        const std::vector<Offer> a = stock(general, 7, 1, 0, none);
        worldgen::CrtRandom r;
        r.seed(7u * 13u + 0x42u);
        const int32_t foods[] = {0x22, 0x23, 0x13, 0x1a, 0x1e, 0x57};
        const int32_t food = foods[r.next() % 6];
        CHECK(a.size() == 4 && a[0].count == -1 && a[0].item[0] == 11 && a[0].item[0x1c] == 0x19);
        int32_t subtype;
        std::memcpy(&subtype, a[2].item.data() + 4, 4);
        CHECK(a[2].item[0] == 0x15 && subtype == food && a[2].count == 1 && a[3].item[0] == 1);
        // Bought today: it is gone until another day brings another (or the same) food.
        const std::vector<Offer> b = stock(general, 7, 1, 0, [&](const Item &it) { return boughtCount({a[2].item}, it); });
        CHECK(b.size() == 3 && b[2].item[0] == 1);
    }
    // The tier stones: rand mod 5 + 3, + 2, + 1, + 0 less what was bought.
    {
        Shopkeeper gems = general;
        gems.cls = TIER_SHOP;
        worldgen::CrtRandom r;
        r.seed(12u * 13u + 0x42u);
        std::vector<int> expected;
        for (int k = 0; k < 4; k++)
            expected.push_back(r.next() % 5 + 3 - k);
        const std::vector<Offer> a = stock(gems, 12, 1, 0, none);
        size_t at = 0;
        for (int k = 0; k < 4; k++)
            if (expected[(size_t)k] > 0) {
                CHECK(at < a.size() && a[at].count == expected[(size_t)k] && a[at].item[0x14] == k + 1);
                at++;
            }
        CHECK(at == a.size());
    }
    // The weapon shop: deterministic for a shopkeeper and a day; tier 0 gives 1 or 2 pieces of
    // rarity 0; the region is where the shopkeeper lives; a higher tier lists in rarity order.
    {
        Shopkeeper smith;
        smith.cls = WEAPON_SHOP, smith.id = 99;
        smith.home[0] = (int64_t)5000 * 65536, smith.home[1] = (int64_t)-300 * 65536 - 7;
        const std::vector<Offer> a = stock(smith, 3, 1, 0, none), b = stock(smith, 3, 1, 0, none);
        CHECK(a.size() == b.size());
        for (size_t k = 0; k < a.size() && k < b.size(); k++)
            CHECK(a[k].item == b[k].item && a[k].count == 1);
        worldgen::CrtRandom r;
        r.seed(99u + 3u * 13u + 0x156b6u);
        CHECK((int)a.size() == r.next() % 2 + 1);
        const worldgen::Int2 home = worldgen::regionAt(5000, -300); // (truncated toward zero)
        int32_t region[2];
        std::memcpy(region, a[0].item.data() + 0xc, 8);
        CHECK(a[0].item[0] == 3 && a[0].item[0x14] == 0 && region[0] == home.x && region[1] == home.y);
        const std::vector<Offer> high = stock(smith, 3, 1, 4, none);
        for (size_t k = 1; k < high.size(); k++)
            CHECK(high[k - 1].item[0x14] <= high[k].item[0x14]);
        CHECK(high.size() >= 5 && high.back().item[0x14] == 4);
        CHECK(stock(smith, 4, 1, 0, none)[0].item != a[0].item); // another day, another stock
        Shopkeeper armourer = smith;
        armourer.cls = ARMOUR_SHOP;
        const std::vector<Offer> c = stock(armourer, 3, 2, 0, none);
        CHECK(!c.empty() && c.size() <= 4 && c[0].item[0] >= 4 && c[0].item[0] <= 9);
        // Bought: that piece is gone, the rest stay.
        const std::vector<Offer> d = stock(smith, 3, 1, 0, [&](const Item &it) { return boughtCount({a[0].item}, it); });
        CHECK(d.size() + 1 == a.size());
    }
    CHECK(shopTier(0, 0, {}) == 0);
    // Prices (stat-rules.md §2.7) and the identify price.
    {
        Item sword{};
        sword[0] = 3, sword[0x14] = 2, sword[8] = 1;
        CHECK(price(sword) == 100 && sellPrice(sword) == 5 && !renowned(sword));
        Item unknown{};
        unknown[0] = UNIDENTIFIED, unknown[0x14] = 3;
        CHECK(price(unknown) == 8 && identifyPrice(unknown) == 4);
        unknown[0x14] = 0;
        CHECK(identifyPrice(unknown) == 1);
        // Identifying: an even draw makes a weapon, an odd one armour, bound to the item's region.
        const int32_t where[2] = {7, -9};
        std::memcpy(unknown.data() + 0xc, where, 8);
        unknown[0x14] = 2;
        int n = 0;
        const Item weapon = identify(unknown, [&] { return (n++) * 2; });
        int32_t region[2];
        std::memcpy(region, weapon.data() + 0xc, 8);
        CHECK(weapon[0] == 3 && weapon[0x14] == 2 && region[0] == 7 && region[1] == -9);
        n = 0;
        const Item armour = identify(unknown, [&] { return (n++) * 2 + 1; });
        CHECK(armour[0] >= 4 && armour[0] <= 9 && armour[0x14] == 2);
    }
    // What was bought, the per-player items taken and the region counters go into the game's
    // character (player data +0x138, +0x88, +0xf8) and come back.
    {
        SaveGame s;
        Item bought{};
        bought[0] = 3, bought[8] = 77;
        s.purchases[42] = {bought};
        s.perPlayerTaken = {{3, -2, 7}};
        s.regionCounters[{4, 5}] = 6;
        nativesaves::Character c;
        std::string notes;
        nativeprogress::fromProgress(s, nullptr, 100.0f, {}, c, notes);
        CHECK(c.purchases[42].size() == 1 && c.purchases[42][0] == bought && (c.map88[{3, -2}] == 7) && (c.regionValues[{4, 5}] == 6));
        const SaveGame back = nativeprogress::toProgress(c, notes, {}, {});
        CHECK(back.purchases == s.purchases && back.perPlayerTaken == s.perPlayerTaken && back.regionCounters == s.regionCounters);
    }
    // The buy-back list keeps the last ten sold.
    {
        BuyBack list;
        for (int k = 0; k < 12; k++) {
            Item it{};
            it[0] = 11, it[4] = (uint8_t)k;
            rememberSold(list, it);
        }
        CHECK(list.size() == BUY_BACK_LENGTH && list.front().first[4] == 2 && list.back().first[4] == 11 && list.back().second == 1);
    }
}

// Ground items and receiving them (docs/specs/loot-pickup.md).
static void groundItemRules() {
    using namespace grounditems;
    // The hop: in place, 0 at 500 ms left, five blocks at 250, 0 at the end; below the ground
    // point before 500 (open question 1). It tumbles 0.7 degrees per ms left.
    CHECK(hopHeight(500.0f) == 0.0f && hopHeight(250.0f) == 5.0f && hopHeight(0.0f) == 0.0f);
    CHECK(hopHeight(750.0f) == -15.0f && hopHeight(125.0f) > 3.7f && hopHeight(125.0f) < 3.8f);
    CHECK(tumble(100.0f) == 70.0f && tumble(0.0f) == 0.0f);
    // The delay runs out once, to exactly 0; the timer counts down too.
    {
        float delay = 650.0f;
        int32_t timer = 50;
        int frames = 0;
        while (!countDown(delay, timer, 16.0f))
            frames++;
        CHECK(frames == 40 && delay == 0.0f && timer == 0); // 41 frames of 16 ms: 656 ms
        CHECK(!countDown(delay, timer, 16.0f));
    }
    // Coins are taken by themselves within 4 blocks once their delay has ended, and never
    // otherwise: a coin farther away stays on the ground.
    CHECK(picksUpItself(type::COIN, 0, 0.0f, 15.9f) && picksUpItself(type::TYPE_0E, 0, 0.0f, 16.0f));
    CHECK(!picksUpItself(type::COIN, 0, 1.0f, 1.0f) && !picksUpItself(type::COIN, 0, 0.0f, 16.1f));
    CHECK(!picksUpItself(type::COIN, ground::STAYS_FOR_OTHERS, 0.0f, 1.0f) && !picksUpItself(3, 0, 0.0f, 1.0f));
    {
        // A coin laid beside the player hops 500..799 ms and is taken the frame it lands.
        float delay = 799.0f;
        int32_t timer = 0;
        int ms = 0;
        bool taken = false;
        while (!taken && ms < 2000) {
            countDown(delay, timer, 1.0f);
            ms++;
            taken = picksUpItself(type::COIN, 0, delay, 4.0f);
        }
        CHECK(taken && ms == 799);
    }
    // Wall items move 0.2 block from a solid neighbour and face away; later tests win.
    {
        int64_t x = 10 * BLOCK + 30000, y = 20 * BLOCK + 40000;
        float angle = 45.0f;
        snapToWall(x, y, angle, 5 * BLOCK, [](int32_t bx, int32_t by, int32_t) { return bx == 10 && by == 19; });
        CHECK(y == 20 * BLOCK + 13107 && x == 10 * BLOCK + 30000 && angle == 180.0f);
        snapToWall(x, y, angle, 5 * BLOCK, [](int32_t bx, int32_t by, int32_t) { return (bx == 10 && by == 19) || bx == 11; });
        CHECK(x == 11 * BLOCK - 13107 && angle == 270.0f);
    }
    // Receiving a pick-up.
    {
        Keeper keeper;
        const int64_t at[3] = {3 * BLOCK + 5, -2 * BLOCK, 7 * BLOCK};
        const int32_t region[2] = {4, 5};
        Item coin{};
        coin[0] = type::COIN, coin[0x1c] = COIN_MATERIAL;
        Receipt r = receive(keeper, coin, coin, 1, 0, at, region, 0, true);
        CHECK(r.into == Into::Inventory && r.amount == 1 && r.dialog.empty() && r.announce);
        Item regional = coin;
        regional[0x1c] = REGION_COIN_MATERIAL;
        r = receive(keeper, regional, regional, 3, 0, at, region, 0, true);
        CHECK(r.into == Into::RegionCounter && (keeper.regionCounters[{4, 5}] == 3));
        Item bag{};
        bag[0] = type::GOLD_BAG, bag[0x14] = 1;
        r = receive(keeper, bag, bag, 1, 0, at, region, 0, true);
        CHECK(r.into == Into::Gold && r.amount == moneyValue(bag) && r.amount >= 20 && r.amount < 30);
        // An item that stays for others is taken once per character, by its block.
        Item herb{};
        herb[0] = 11, herb[4] = 0x16;
        r = receive(keeper, herb, herb, 1, ground::STAYS_FOR_OTHERS, at, region, 0, true);
        CHECK(r.announce && r.into == Into::Inventory && keeper.perPlayerTaken.count({3, -2, 7}));
        r = receive(keeper, herb, herb, 1, ground::STAYS_FOR_OTHERS, at, region, 0, true);
        CHECK(!r.announce && r.into == Into::Nothing);
        // Treasure: its cell becomes used, and a fine piece is "the item", held up.
        Item treasure{};
        treasure[0] = type::TREASURE;
        Item sword{};
        sword[0] = 3, sword[0x14] = 3;
        r = receive(keeper, treasure, sword, 1, 0, at, region, 0, true);
        CHECK(r.markCell && r.cell[0] == 0 && r.cell[1] == -1 && r.dialog.size() == 7 && r.dialog.back() == "you_received_the_item");
        CHECK(r.holdUpAbility == 0x7c);
        // Vehicles have their own hold-up.
        Item boat{};
        boat[0] = type::VEHICLE, boat[4] = 1;
        r = receive(keeper, boat, boat, 1, 0, at, region, 0, true);
        CHECK(r.holdUpAbility == 0x78 && r.dialog.back() == "new_page");
        auto described = receive(keeper, boat, boat, 1, 0, at, region, 0, true, ReceiptRegion{13,-47});
        CHECK(described.dialog.size() == 12 && described.dialog[8] == "define_land" && described.dialog[9] == "13" &&
              described.dialog[10] == "-47" && described.dialog.back() == "description_hang_glider");
        boat[4] = 17;
        described = receive(keeper, boat, boat, 1, 0, at, region, 0, true, ReceiptRegion{2,91});
        CHECK(described.dialog.back() == "description_crafting_book4");
        boat[4] = 4;
        described = receive(keeper, boat, boat, 1, 0, at, region, 0, true, ReceiptRegion{2,91});
        CHECK(described.dialog.size() == 11 && described.dialog.back() == "91");
        boat[4] = 2;
        CHECK(receive(keeper, boat, boat, 1, 0, at, region, 0, true).holdUpAbility == 0);
        CHECK(receive(keeper, boat, boat, 1, 0, at, region, 2, true).holdUpAbility == 0x79);
        CHECK(receive(keeper, boat, boat, 1, 0, at, region, 2, false).holdUpAbility == 0);
    }
    // Dialog pages: give and take.
    {
        Item coins{};
        PageItems page;
        scriptOperation(page, 9, coins, 25);
        CHECK(page.giveCount == 25 && page.give[0] == type::COIN && page.give[0x1c] == COIN_MATERIAL);
        Item boat{};
        boat[0] = type::VEHICLE;
        CHECK(pageGives(1, boat, true).receiptDialog && !pageGives(1, boat, false).gives && !pageGives(0, boat, true).gives);
        int32_t gold = 10;
        std::vector<Stack *> none;
        pageTakes(none, gold, page.give, 25);
        CHECK(gold == 0);
        Item herb{};
        herb[0] = 11, herb[4] = 0x16;
        Stack a{2, herb}, b{5, herb};
        std::vector<Stack *> stacks = {&a, &b};
        pageTakes(stacks, gold, herb, 4);
        CHECK(a.count == 0 && a.item[0] == 0 && b.count == 3);
    }
    // Taming's client side: the pet slot, the pet id and the pet food held.
    {
        std::vector<uint8_t> me(worldgen::CREATURE_SIZE), pet(worldgen::CREATURE_SIZE);
        const int64_t myId = 1, petId = 77;
        const int32_t race = 0x22;
        std::memcpy(me.data() + 0x08, &myId, 8);
        std::memcpy(pet.data() + 0x08, &petId, 8);
        std::memcpy(pet.data() + 0x64, &race, 4);
        me[0x1d8] = type::PET_FOOD;
        std::memcpy(me.data() + 0x1d8 + 4, &race, 4);
        me[0x8b8] = type::PET, me[0x8b8 + 4] = 9; // an old pet
        std::array<uint8_t, 0x48> number{};
        std::memcpy(number.data() + 0x00, &petId, 8); // from the tamed creature to the player (creature-hit.md §0.3)
        std::memcpy(number.data() + 0x08, &myId, 8);
        float value = 0.5f;
        std::memcpy(number.data() + 0x10, &value, 4);
        number[0x45] = 6;
        auto byId = [&](int64_t id) -> const uint8_t * { return id == petId ? pet.data() : nullptr; };
        CHECK(!tame(number.data(), me.data(), byId).happened);
        value = 1.0f;
        std::memcpy(number.data() + 0x10, &value, 4);
        const Tamed t = tame(number.data(), me.data(), byId);
        int32_t subtype;
        int64_t newPet;
        std::memcpy(&subtype, me.data() + 0x8b8 + 4, 4);
        std::memcpy(&newPet, me.data() + 0x9d8, 8);
        CHECK(t.happened && t.hadPet && t.oldPet[4] == 9 && t.food[0] == type::PET_FOOD);
        CHECK(me[0x8b8] == type::PET && subtype == race && newPet == petId && me[0x1d8] == 0);
        CHECK(!tame(number.data(), me.data(), byId).happened); // no pet food held any more
    }
}

// The stat rules no recording covers (docs/specs/stat-rules.md "Not covered"), with values
// worked out by hand from the spec.
static void statRules() {
    using namespace nativerules;
    auto item = [](uint8_t type, int32_t subtype, uint32_t modifier, uint8_t rarity) {
        std::array<uint8_t, 0xa0> it{};
        it[0] = type;
        std::memcpy(&it[4], &subtype, 4);
        std::memcpy(&it[8], &modifier, 4);
        it[0x14] = rarity;
        return it;
    };
    // Prices (§2.7): a one-handed weapon of rarity 2; renowned (its modifier a multiple of
    // 30) twice that; a two-handed one of rarity 0; anything unlisted at the floor of 1.
    CHECK(itemPrice(item(3, 0, 1, 2).data()) == 100);
    CHECK(itemPrice(item(3, 0, 30, 2).data()) == 200);
    CHECK(itemPrice(item(3, 5, 1, 0).data()) == 50);
    CHECK(itemPrice(item(0x16, 0, 1, 0).data()) == 250);
    CHECK(itemPrice(item(1, 0, 1, 0).data()) == 1);
    CHECK(sellPrice(100) == 5 && sellPrice(1) == 1);
    // Consumables (§2.6).
    CHECK(healAmount(item(1, 1, 0, 0).data()) == 200.0f && healAmount(item(1, 10, 0, 0).data()) == 100.0f);
    CHECK(healAmount(item(1, 3, 0, 0).data()) == 0.0f && healAmount(item(3, 1, 0, 0).data()) == 0.0f);
    CHECK(useTime(item(1, 1, 0, 0).data()) == 7500 && useTime(item(1, 1, 0, 4).data()) == 5000);
    CHECK(useTime(item(1, 2, 0, 0).data()) == 9000 && useTime(item(1, 3, 0, 0).data()) == 500);
    CHECK(useTime(item(0x1c, 0, 0, 0).data()) == 500 && useTime(item(3, 0, 0, 0).data()) == 0);
    CHECK(demonDamage(item(0x18, 12, 0, 0).data()) == 1.0f && demonDamage(item(0x18, 11, 0, 0).data()) == 0.0f);
    // Item level (§1.2): renowned items count in the eight regions around home.
    {
        auto it = item(3, 0, 30, 2);
        const int32_t home[2] = {5, 5};
        std::memcpy(&it[0xc], home, 8);
        const int32_t nextDoor[2] = {6, 4}, away[2] = {7, 5};
        CHECK(itemLevel(it.data(), home) == 2 && itemLevel(it.data(), nextDoor) == 2 && itemLevel(it.data(), away) == -1);
        const uint32_t plain = 31;
        std::memcpy(&it[8], &plain, 4);
        CHECK(itemLevel(it.data(), home) == 2 && itemLevel(it.data(), nextDoor) == -1);
    }
    // Class use (§1.3), through a formula too.
    CHECK(classMayUse(item(3, 3, 0, 0).data(), 4) && !classMayUse(item(3, 3, 0, 0).data(), 1));
    {
        auto formula = item(2, 10, 0, 0);
        formula[0x18] = 3; // a staff's formula
        CHECK(classMayUse(formula.data(), 3) && !classMayUse(formula.data(), 2));
    }
    // Creatures.
    std::vector<uint8_t> a(nativepose::CREATURE_SIZE), b(nativepose::CREATURE_SIZE);
    auto setInt = [](std::vector<uint8_t> &r, size_t at, int32_t v) { std::memcpy(&r[at], &v, 4); };
    auto setFloat = [](std::vector<uint8_t> &r, size_t at, float v) { std::memcpy(&r[at], &v, 4); };
    CHECK(stunDuration(a.data()) == 1500 && skillLevel() == 1000);
    a[0x14c] = 3;
    CHECK(classSkill(a.data()) == 0x2f && usesMana(a.data()) && isRanged(a.data()));
    // Skills (§9): a player's own points; anyone else's level x 10.
    setInt(a, 0xb5c, 20);
    CHECK(skillValue(a.data(), Skill::Climbing) == 0.75f && skillValue(a.data(), Skill::LightRadius) == 8.0f);
    b[0x60] = 2;
    setInt(b, 0x144, 2);
    CHECK(std::fabs(skillValue(b.data(), Skill::Riding) - 1.8f) < 1e-6f && skillValue(b.data(), Skill::Diving) == 0.0f);
    // Level curves (§10).
    CHECK(experienceForLevel(1) == 10 && experienceForLevel(2) == 14 && experienceForLevel(3) == 20);
    CHECK(stealthGain(1) == 0.8f && cloneAuraStrength(1) == 0.5f && cloneAuraDuration(51) == 22500);
    CHECK(glideFactor(1) == 1.2f && std::fabs(levelFactor(3.0f) - 1.2f) < 1e-6f);
    // Race groups (§8.4).
    CHECK(raceGroup(0x78) == 1 && raceGroup(0x49) == 6 && raceGroup(0x12f) == 4 && raceGroup(0) == 3 && raceGroup(0x200) == 0);
    // Hostility (§8.2): kind 1 fights a player, players don't fight each other, kind 8 never.
    b[0x60] = 1;
    CHECK(hostile(a.data(), b.data()) && !hostile(a.data(), a.data()));
    b[0x60] = 0;
    CHECK(!hostile(a.data(), b.data()));
    b[0x128] = 0x20;
    CHECK(hostile(a.data(), b.data()));
    b[0x60] = 8;
    CHECK(!hostile(a.data(), b.data()));
    // Starting an ability (§8.1): costs, cooldowns, stun.
    setFloat(a, 0x6c, 100000.0f); // long past the current ability
    setFloat(a, 0x9a0, 1.0f);
    std::map<int32_t, float> cooldowns{{5, 100.0f}, {6, 0.0f}};
    CHECK(!canStartAbility(a.data(), 0x33, nullptr, nullptr) && !canStartAbility(a.data(), 0x1d, nullptr, nullptr));
    CHECK(!canStartAbility(a.data(), 5, nullptr, &cooldowns) && canStartAbility(a.data(), 6, nullptr, &cooldowns));
    setFloat(a, 0x188, 1.0f);
    CHECK(canStartAbility(a.data(), 0x33, nullptr, nullptr) && canStartAbility(a.data(), 0x1d, nullptr, nullptr));
    setFloat(a, 0x130, 500.0f);
    CHECK(!canStartAbility(a.data(), 6, nullptr, nullptr) && canStartAbility(a.data(), 0x71, nullptr, nullptr));
    CHECK(manaCost(0x20, [](int t) { return t == 9; }) == 0.0f && manaCost(0x20, nullptr) == 0.3f);
    CHECK(staminaCost(0x34, nullptr) == 1.0f && staminaCost(0x34, [](int t) { return t == 13; }) == 0.0f);
    CHECK(abilityDamageFactor(0x35) == 3.0f && abilityDamageFactor(0) == 1.0f);
}

static void progressionRules() {
    using namespace progression;
    CHECK(xpToNext(1) == 54); // the game's XP bar read 0/54 at level 1
    CHECK(xpToNext(10) == 90);
    // Equal levels: stars + 1. A weaker enemy gives less, a stronger one more.
    CHECK(killXp(2, 5, 5) == 3);
    CHECK(killXp(2, 1, 10) < 3);
    CHECK(killXp(2, 15, 10) > 3);
    CHECK(killXp(0, 5, 5, true) == 10);
    // Enemy levels: easy biomes stay at or below the player, hard ones above.
    for (uint64_t e = 0; e < 50; e++) {
        int easy = enemyLevel(20, 0, regionRoll(123, 3, 4), e), hard = enemyLevel(20, 9, regionRoll(123, 3, 4), e);
        CHECK(easy >= 17 && easy <= 21);
        CHECK(hard >= 22 && hard <= 28);
    }
    CHECK(enemyLevel(1, 0, 0, 0) >= 1);
    CHECK(std::abs(creatureScale(1) - (1 + 1000 * std::log2(1002.f / 1000))) < 1e-3f);
    CHECK(creatureScale(10) > creatureScale(1));
    // Loot levels: player minus 0-2, never below 1.
    for (uint64_t r = 0; r < 50; r++) {
        int l = lootLevel(10, 10, r);
        CHECK(l >= 8 && l <= 10);
        CHECK(lootLevel(1, 1, r) == 1);
    }
    CHECK(gearStrength(Mode::Pyro, 20, 2, 0) > gearStrength(Mode::Pyro, 5, 2, 0));
    CHECK(gearStrength(Mode::Vanilla, 20, 2, 0) == gearStrength(Mode::Vanilla, 5, 2, 0));
    CHECK(gearStrength(Mode::Vanilla, 1, 3, 0) > gearStrength(Mode::Vanilla, 1, 1, 0));
    CHECK(pyroRand(0) == 0 && pyroRand(1) == 16838); // the C library's classic LCG
    // On the game's creatures (PyroCreatures): the mod's level text, levels by region, and
    // the stat hooks, which scale a monster's health by 0.05 * 3 * its scale and leave a
    // player alone, and go with their owner.
    CHECK(pyrocreatures::levelText(12) == "LV 12 " && pyrocreatures::levelText(1500) == "LV 1.50K " &&
          pyrocreatures::levelText(2000000) == "LV 2.00M ");
    {
        int64_t at[3] = {(int64_t)300 * 65536, (int64_t)400 * 65536, 0};
        worldgen::CreatureRecord monster =
            worldgen::makeInPlay(77, 2, 1, 1, at, {0, 0}, 0, worldgen::creatureTables(), [](const uint8_t *r) { return nativerules::maxHealth(r); }).record;
        worldgen::CreatureRecord you = monster;
        you[worldgen::creature::HOSTILITY] = 0;
        const worldgen::Int2 r = worldgen::regionAt(300, 400);
        const int level = pyrocreatures::level(monster.data(), 123, 20);
        CHECK(level == enemyLevel(20, worldgen::regionBiome(r.x, r.y), regionRoll(123, r.x, r.y), 77));
        CHECK(pyrocreatures::level(you.data(), 123, 20) == 1); // its own (+0x144)
        const float plain = nativerules::maxHealth(monster.data()), yours = nativerules::maxHealth(you.data());
        {
            pyrocreatures::StatHooks hooks(123, [](worldgen::Int2) { return 20; });
            CHECK(std::abs(nativerules::maxHealth(monster.data()) - plain * 0.05f * 3 * creatureScale(level)) < plain * 1e-4f);
            CHECK(nativerules::maxHealth(you.data()) == yours);
        }
        CHECK(nativerules::maxHealth(monster.data()) == plain);
    }
    { // The discovery lock: a region keeps the level the player first entered it at.
        pyrocreatures::RelativeLevels locked, relative;
        locked.open(123, "world", "tester", true, {}, {}, false);
        relative.open(123, "world", "tester", false, {}, {}, false);
        locked.visit({5, 6}, 3);
        locked.visit({5, 6}, 9); // (entered again later: kept)
        relative.visit({5, 6}, 3);
        CHECK(locked.basis({5, 6}, 10) == 3 && locked.basis({5, 7}, 10) == 10);
        CHECK(relative.basis({5, 6}, 10) == 10 && relative.size() == 0);
        // Gear keeps the level it is first given: a drop's, or an owned piece's old one.
        const int dropped = locked.dropped(3, 1, 777, 1, 10, 10);
        CHECK(dropped >= 8 && dropped <= 10);
        CHECK(locked.dropped(3, 1, 777, 1, 40, 60) == dropped && locked.itemLevel(3, 1, 777, 1, 40) == dropped);
        locked.own(4, 0, 555, 28, 6);
        locked.own(4, 0, 555, 28, 2);
        CHECK(locked.keptLevel(4, 0, 555, 28) == 6 && locked.keptLevel(4, 0, 556, 28) == 0);
        CHECK(pyrocreatures::RelativeLevels::legacyLevel({3, 4}, {1, 4}, 0) == progression::ownedItemLevel(2, 0));
    }
    { // Mission progress (the mission update, 0x131620) in a made-up world: a gnome's
      // resident whose task is done solves its place, a carried glider its placement, and the
      // flames of power count into their buff.
        worldgen::setWorldSeed(0x7b);
        int tried = 0, gnomes = 0, gliders = 0;
        for (int rx = 120; rx < 136 && (gnomes == 0 || gliders == 0); rx++)
            for (int ry = 120; ry < 136 && (gnomes == 0 || gliders == 0); ry++, tried++) {
                std::array<int32_t, 2> gnome{INT32_MIN, 0}, glider{INT32_MIN, 0};
                for (const auto &r : worldgen::regionRoles({rx, ry}).roles) {
                    if (r[2] == 1)
                        gnome = {r[0], r[1]};
                    if (r[2] == 6)
                        glider = {r[0], r[1]};
                }
                if (gnome[0] == INT32_MIN && glider[0] == INT32_MIN)
                    continue;
                const std::array<int32_t, 2> at = gnome[0] != INT32_MIN ? gnome : glider;
                const int64_t px = ((int64_t)at[0] * 1024 + 512) << 16, py = ((int64_t)at[1] * 1024 + 512) << 16;
                worldgen::CreatureRecord me(0x1600), resident(0x1600);
                auto put = [](worldgen::CreatureRecord &r, size_t o, auto v) { std::memcpy(r.data() + o, &v, sizeof v); };
                put(me, worldgen::creature::REGION, rx), put(me, worldgen::creature::REGION + 4, ry);
                put(me, worldgen::creature::POSITION, px), put(me, worldgen::creature::POSITION + 8, py);
                put(resident, worldgen::creature::FLAGS, (uint32_t)0x2000), put(resident, 0x128, (uint16_t)0x100), resident[0x19c] = 1;
                put(resident, worldgen::creature::POSITION, px + (5 << 16)), put(resident, worldgen::creature::POSITION + 8, py);
                put(resident, worldgen::creature::HOME, px), put(resident, worldgen::creature::HOME + 8, py);
                std::set<missions::Cell> finished, solved;
                missions::World w;
                w.player = me.data(), w.finished = &finished, w.solved = &solved;
                w.creatures = [&](const std::function<void(const uint8_t *)> &f) { f(resident.data()); };
                w.objects = [](int, int) { return std::vector<worldgen::ZoneObject>(); };
                w.active = [](const worldgen::ZoneObject &) { return true; };
                w.missionCreatures = [](int, int) { return std::vector<int64_t>(); };
                w.dead = [](int64_t) { return false; };
                w.hasSpecial = [](int id) { return id == 1 || id == 4; };
                missions::Effects fx;
                CHECK(missions::update(w, fx));
                if (gnome[0] != INT32_MIN) {
                    CHECK(solved.count(gnome) && finished.count(gnome));
                    gnomes++;
                }
                if (glider[0] != INT32_MIN) {
                    CHECK(solved.count(glider));
                    gliders++;
                }
                CHECK(!fx.witch && !fx.demon && fx.flames == 0);
                me[worldgen::creature::ABILITY] = 0x9c; // (nothing while flying on the eagle)
                CHECK(!missions::update(w, fx));
            }
        CHECK(gnomes > 0 && gliders > 0);
        // Every artifact reward place (role 14) has its artifact (0x11d7b0): category 23, one of
        // seven kinds, its place cell as its region and its realm (0x2bb060) at +0x18.
        int artifacts = 0;
        for (int rx = 120; rx < 124; rx++)
            for (const auto &r : worldgen::regionRoles({rx, 128}).roles)
                if (r[2] == 14) {
                    const worldgen::Item a = worldgen::placeArtifact(r[0], r[1]);
                    CHECK(a.category == 23 && a.id >= 0 && a.id < 7 && a.region.x == r[0] && a.region.y == r[1] &&
                          a.realm == worldgen::placeStyle(r[0], r[1]) && a.realm >= 1 && a.realm <= 100000);
                    artifacts++;
                }
        CHECK(artifacts >= 4); // (at least one a region)
        std::printf("missions: %d gnomes and %d glider placements solved in %d regions\n", gnomes, gliders, tried);
    }
    // Progress survives a save round trip.
    SaveGame save;
    save.level = 7, save.xp = 31, save.gold = 412;
    save.treasures.push_back({-128445745, -15873101, 208}); // a star marked on the map
    save.found.push_back({4, 0, 28, 3, 6, 2, 123456u, "cotton-chest1", 20, 0, 1.5f, 0});
    save.finishedMissions = {{125, -15}, {-3, 8}};
    save.solvedMissions = {{-3, 8}};
    save.missionTimers[{4, 5}] = 1234.5f;
    save.deathPosition = {-128445750, -15874800, 30}, save.tombstoneMs = 250000.0f; // a tombstone on the map
    save.usedCells = {{-8020000, -990000}, {12, 7}}; // a lore cell read, a chest opened
    SaveGame back;
    CHECK(back.parse(save.text()));
    CHECK(back.finishedMissions == save.finishedMissions && back.solvedMissions == save.solvedMissions);
    CHECK(back.deathPosition == save.deathPosition && back.tombstoneMs == 250000.0f);
    CHECK(back.usedCells == save.usedCells);
    CHECK(save.text().find("\nlore ") == std::string::npos); // (the lore by realm is rebuilt, not saved)
    { // An older save's lore lines: their cells become used cells, once each.
        SaveGame old;
        CHECK(old.parse("cubevulkan-save 1\nopened 12 7\nlore 61234 -8020000 -990000\nlore 61234 12 7\n"));
        CHECK(old.usedCells == (std::vector<std::array<int32_t, 2>>{{12, 7}, {-8020000, -990000}}));
    }
    CHECK(back.missionTimers.size() == 1 && back.missionTimers.begin()->first == (std::array<int32_t, 2>{{4, 5}}) && back.missionTimers.begin()->second == 1234.5f);
    CHECK(back.level == 7 && back.xp == 31 && back.gold == 412);
    CHECK(back.treasures.size() == 1 && back.treasures[0][0] == -128445745 && back.treasures[0][2] == 208);
    CHECK(back.found.size() == 1 && back.found[0].modifier == 123456u && back.found[0].sprite == "cotton-chest1" &&
          back.found[0].level == 6 && back.found[0].slot == 2);
    // Several characters share one cloud file; each keeps its own section.
    std::string bundle = SaveGame::intoBundle("", "warrior-warrior", save.text());
    bundle = SaveGame::intoBundle(bundle, "mage-mage", "cubevulkan-save 1\nlevel 3\n");
    bundle = SaveGame::intoBundle(bundle, "warrior-warrior", "cubevulkan-save 1\nlevel 9\n");
    SaveGame w, m;
    CHECK(w.parse(SaveGame::fromBundle(bundle, "warrior-warrior")) && w.level == 9);
    CHECK(m.parse(SaveGame::fromBundle(bundle, "mage-mage")) && m.level == 3);
    CHECK(SaveGame::fromBundle(bundle, "nobody").empty());
}

static void saveIntegrity() {
    SaveGame original;
    original.world = "seed_123"; original.hasPosition = true;
    original.position = {12345.6787109375f, -9876.5439453125f, 140.1234588623f};
    original.yaw = .812345624f; original.heading = 3.94159269f; original.hpFraction = .678912342f;
    original.savedAt = 123456; original.level = 7; original.xp = 31; original.gold = 412;
    original.hasGear = true; original.worn.resize(Gear::SLOTS);
    original.found.push_back({4, 0, 28, 3, 6, 2, 123456u, "", 20.123456f, 0, 1.5123456f, .1234567f});
    original.quests.push_back({42, 1, 3, 3, "crawler", "Arvinran Hills"});
    original.created.classType = 4; original.created.specialization = 1;
    original.created.race = "elf"; original.created.gender = "f";
    original.created.name = "Test Character"; original.created.head = "elf-head-m04";
    original.created.hair = ""; original.created.hand = "hand2"; original.created.foot = "foot";
    SaveGame parsed;
    CHECK(parsed.parse(original.text()));
    CHECK(parsed.position == original.position && parsed.yaw == original.yaw && parsed.heading == original.heading);
    CHECK(parsed.hpFraction == original.hpFraction && parsed.found[0].hp == original.found[0].hp);
    CHECK(parsed.found[0].sprite.empty() && parsed.created.hair.empty() && parsed.created.name == "Test Character");
    for (const auto &bad : {"", "gold 12\n", "cubevulkan-save 2\n", "cubevulkan-save 1\nposition 1 2\n",
                            "cubevulkan-save 1\nposition 1 nan 3\n", "cubevulkan-save 1\ngold -2\n",
                            "cubevulkan-save 1\nworn 0 0 0 0 0\n", "cubevulkan-save 1\ntime 1 99\n",
                            "cubevulkan-save 1\ncubevulkan-save 1\n"}) {
        std::string prior = parsed.text();
        CHECK(!parsed.parse(bad) && parsed.text() == prior); // failure must not destroy a valid loaded state
    }
    CHECK(parsed.parse("cubevulkan-save 1\nlevel 3\n"));
    CHECK(parsed.world.empty() && parsed.savedAt == 0 && parsed.yaw == 0 && parsed.heading == 0 && !parsed.hasPosition);
    std::string bundle = SaveGame::intoBundle("", "one", original.text());
    CHECK(!bundle.empty());
    auto untouched = SaveGame::fromBundle(bundle, "one");
    auto added = SaveGame::intoBundle(bundle, "two", "cubevulkan-save 1\ngold 9\n");
    CHECK(!added.empty() && SaveGame::fromBundle(added, "one") == untouched);
    CHECK(SaveGame::intoBundle("cubevulkan-cloud 2\n", "two", original.text()).empty());
    CHECK(SaveGame::intoBundle(bundle + "character broken\ncubevulkan-save 1\nposition 1\n", "two", original.text()).empty());
    CHECK(SaveGame::intoBundle(bundle, "two", "truncated").empty());

    auto root = fs::temp_directory_path() / ("CubeVulkan-save-test-" + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(GetTickCount64()));
    CHECK(fs::create_directory(root));
    auto file = root / "progress.sav";
    CHECK(original.save(file));
    SaveGame loaded; CHECK(loaded.load(file) && loaded.gold == 412 && loaded.position == original.position);
    HANDLE held = CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    CHECK(held != INVALID_HANDLE_VALUE);
    original.gold = 999;
    CHECK(!original.save(file)); // locked destination: retain complete previous save
    if (held != INVALID_HANDLE_VALUE) CloseHandle(held);
    CHECK(loaded.load(file) && loaded.gold == 412);
    CHECK(original.save(file) && loaded.load(file) && loaded.gold == 999);
    original.position.x = std::numeric_limits<float>::infinity();
    CHECK(!original.save(file) && loaded.load(file) && loaded.gold == 999);
    size_t files = 0; for (const auto &e : fs::directory_iterator(root)) files++;
    CHECK(files == 1); // no temporary files left after success or failure
    fs::remove(file); fs::remove(root);
}

// The game's dialogue grammar (dict_en.xml) expands into plain sentences.
// The game's own conversation scripts (TalkScripts.h), as ZoneExtractor exported them
// on 2026-10-22: Asmore's tip, which the game said as "Oh, hello!", "You won't believe
// what I saw at the Jakor Rocks!", "a Hang Glider!", and Thalkor's introduction.
static void nativeConversations() {
    const char *exported =
        "# t=83842812 day=135\n"
        "id=9223369290223189761 race=48 class=2 spec=0 flags=00000000 flags2=00000000 speaker=Asmore pos=-161.3,-531.0,106.9 : "
        "new_page | define_location | -6 | -2 | 29 | name=Jakor | define_direction | -4839 | -800 | define_item | 24 | 1 | 0 | 0 | 0 | "
        "item=Hang Glider | key=HangGlider | hello | new_page | define_mob | 2127003821 | 48 | name=Aridan | !alert | "
        "i_saw_an_item_in_the_zone | mark_location | -6 | -2 |\n"
        "id=9223368465589469504 race=304 class=156 spec=0 flags=00000000 flags2=00000000 speaker=Thalkor pos=432.1,-723.1,118.9 : "
        "hello | define_mob | -2147483328 | 304 | name=Thalkor | my_name_is | new_page | !sweat | i_dont_like_walking |\n"
        // Newer exports write each person for a run of days, with day= on the line.
        "id=9223368465589469504 day=137 race=304 speaker=Thalkor : hello | i_dont_like_walking |\n";
    TalkScripts scripts;
    CHECK(scripts.parse(exported) && scripts.count() == 2);
    const TalkScript *asmore = scripts.find(9223369290223189761LL, 135);
    CHECK(asmore && asmore->speaker == "Asmore" && asmore->day == 135 && asmore->tokens.size() == 28);
    bool exact = true;
    const TalkScript *later = scripts.find(9223368465589469504LL, 137, &exact);
    CHECK(later && later->day == 137 && later->tokens.size() == 2 && exact);
    later = scripts.find(9223368465589469504LL, 135, &exact);
    CHECK(later && later->day == 135 && exact);
    later = scripts.find(9223368465589469504LL, 140, &exact); // not exported: the nearest day
    CHECK(later && later->day == 137 && !exact && scripts.days() == std::make_pair(135, 137));
    CHECK(std::string(locationKind(29)) == "Rocks" && std::string(locationKind(21)) == "Rock" && std::string(locationKind(6)).empty());
    Assets assets;
    Speech speech;
    std::string why;
    if (!asmore || !assets.locate("") || !speech.load(assets.gameDir() / "dict_en.xml", why))
        return;
    std::mt19937 random(5);
    bool sawPlace = false, sawItem = false;
    for (int run = 0; run < 20; run++) { // the dictionary's alternatives are random
        TalkPlay play = playScript(*asmore, speech, random);
        CHECK(play.unknown.empty());
        CHECK(play.marked.size() == 1 && play.marked[0].x == -6 && play.marked[0].y == -2 &&
              play.marked[0].place == "Jakor Rocks" && play.marked[0].item == "Hang Glider");
        for (auto &line : play.lines) {
            CHECK(line.find('@') == std::string::npos && line.find('*') == std::string::npos);
            sawPlace = sawPlace || line.find("at the Jakor Rocks") != std::string::npos;
            sawItem = sawItem || line.find("a Hang Glider") != std::string::npos;
        }
    }
    CHECK(sawPlace && sawItem);
    TalkPlay thalkor = playScript(*scripts.find(9223368465589469504LL, 135), speech, random);
    bool named = false;
    for (auto &line : thalkor.lines)
        named = named || line.find("Thalkor") != std::string::npos;
    CHECK(named && thalkor.unknown.empty());
}

// The conversation builder (TalkGen.h) on the numbers docs/specs/conversation-script.md gives:
// the xorshift step, the script seed and its first draws, the noise, the C runtime's rand(), and
// the two special speakers.
static void talkGeneration() {
    using talkgen::xorshift;
    CHECK(xorshift(0) == 0 && xorshift(1) == 270369u && xorshift(12345) == 3336926330u);
    // Gedan (id 9223364892176678848) on day 135: id mod 333333 = 127732, s = 129487.
    const uint64_t gedan = 9223364892176678848ull;
    const uint32_t c = talkgen::scriptSeed(gedan, 135);
    CHECK(gedan % 333333 == 127732 && c == xorshift(129487) && c == 3627600947u);
    CHECK(xorshift(c) == 4133966188u && (xorshift(c) & 7) == 4);         // no artifact rumour
    CHECK(xorshift(c + 1) == 4135301515u && (xorshift(c + 1) & 3) == 3); // no help request
    CHECK(xorshift(c + 2) == 4135555498u && xorshift(c + 2) % 3 == 1);   // a greeting
    CHECK(worldgen::hash(0) == 1376312589u && worldgen::random(0) == 35908u && worldgen::random(1234567) == 79496u);
    talkgen::CRand runtime;
    runtime.seed(1);
    const int first = runtime.next(), second = runtime.next(), third = runtime.next();
    CHECK(first == 41 && second == 18467 && third == 6334);
    CHECK(talkgen::siblingGroup(38) == std::vector<int>({40, 38, 39, 37}) && talkgen::siblingGroup(304) == std::vector<int>({304}));
    CHECK(talkgen::femaleRace(3) && !talkgen::femaleRace(2) && talkgen::femaleRace(84) && !talkgen::femaleRace(17));

    using Tokens = std::vector<std::string>;
    talkgen::Speaker trainer;
    trainer.cls = 155;
    talkgen::Player warrior;
    warrior.cls = 1, warrior.spec = 0;
    talkgen::World world;
    world.day = 135;
    CHECK(talkgen::script(trainer, warrior, world, runtime) ==
          Tokens({"hello", "how_can_i_help_you", "define_specialization", "1", "1", "change_specialization?", "goodbye?"}));
    warrior.spec = 1;
    CHECK(talkgen::script(trainer, warrior, world, runtime)[4] == "0");

    talkgen::Speaker innkeeper;
    innkeeper.cls = 132;
    const Tokens opening = {"define_number", "10", "hello", "new_page"};
    auto innkeeperSays = [&](float hour, int gold) {
        world.timeOfDayMs = hour * 3600000.0f;
        warrior.gold = gold;
        Tokens said = talkgen::script(innkeeper, warrior, world, runtime);
        CHECK(Tokens(said.begin(), said.begin() + 4) == opening);
        return Tokens(said.begin() + 4, said.end());
    };
    CHECK(innkeeperSays(3, 5) == Tokens({"you_dont_have_enough_gold_to_rest_here"}));
    CHECK(innkeeperSays(21, 10) == Tokens({"how_can_i_help_you", "rest_inn?", "goodbye?"}));
    CHECK(innkeeperSays(6, 50) == Tokens({"how_can_i_help_you", "rest_inn?", "goodbye?"})); // 6:00 still counts
    CHECK(innkeeperSays(12, 50) == Tokens({"you_can_rest_here_between_six_and_six"}));

    // Gedan's day 135 begins with the greeting the export shows.
    talkgen::Speaker person;
    person.id = gedan, person.race = 304, person.cls = 156, person.hostility = 3;
    person.x = person.homeX = (int64_t)(53.7 * 65536), person.y = person.homeY = (int64_t)(-1563.1 * 65536);
    std::string branch;
    const Tokens said = talkgen::script(person, talkgen::Player(), world, runtime, &branch);
    CHECK(said.size() >= 4 && Tokens(said.begin(), said.begin() + 4) == Tokens({"hello", "new_page", "nice_to_meet_you", "new_page"}));

    // Playing the commands a generated script uses, with CubeVulkan's own names.
    Assets assets;
    Speech speech;
    std::string why;
    if (!assets.locate("") || !speech.load(assets.gameDir() / "dict_en.xml", why))
        return;
    std::mt19937 random(7);
    TalkNames names;
    int itemSeen[5] = {};
    names.item = [&](const int(&it)[5], std::string &name, std::string &key) {
        std::copy(std::begin(it), std::end(it), itemSeen);
        name = "Chest Armor", key = "ChestArmor";
    };
    names.land = [](int, int) { return std::string("Ikotar"); };
    auto play = [&](const Tokens &tokens) {
        TalkScript s;
        s.tokens = tokens;
        return playScript(s, speech, random, &names);
    };
    warrior.spec = 0;
    const TalkPlay trainerPlay = play(talkgen::script(trainer, warrior, world, runtime)); // offers the other one
    CHECK(trainerPlay.choices.size() == 2 && trainerPlay.choices[0].find("Guardian") != std::string::npos &&
          trainerPlay.choiceIds[0] == "change_specialization?");
    const TalkPlay inn = play(Tokens({"define_number", "10", "hello", "new_page", "you_dont_have_enough_gold_to_rest_here"}));
    CHECK(!inn.lines.empty() && inn.lines.back().find("10") != std::string::npos && inn.unrendered.empty());
    const TalkPlay pet = play(Tokens({"define_model", "28", "my_favorite_pet_is"}));
    CHECK(pet.lines.size() == 1 && pet.lines[0].find("Wol") != std::string::npos && pet.unrendered.empty());
    const TalkPlay portal = play(Tokens({"define_land", "2", "65383", "the_demons_are_possessing_the_residents_of_the_land"}));
    CHECK(portal.lines.size() == 1 && portal.lines[0].find("Ikotar") != std::string::npos);
    // define_item: category, id, modifier, then material before rarity (iron chest armour).
    const TalkPlay gear = play(Tokens({"define_item", "4", "0", "1100", "1", "0", "nice_equipment_item"}));
    CHECK(itemSeen[3] == 1 && itemSeen[4] == 0 && !gear.lines.empty());
    // Realms and rulers have no names in CubeVulkan yet: the line is said, and the gap reported.
    const TalkPlay history = play(Tokens({"define_realm", "13", "86105", "have_you_heard_about_the_realm"}));
    CHECK(!history.unrendered.empty() && history.unrendered[0].find("@realm") != std::string::npos);
}

// talkverify: every script of Extract/talk-scripts.txt made again by TalkGen, for the person's
// id, race and day. The person's record comes from the generated spawn points around where the
// export saw them; the exporting player's state is not in the file, so what the export shows of
// it is read back per export batch (class, specialisation, worn gear), and the rest is taken as
// the export suggests: no gold, night, no pet, no special items, no missions.
namespace talkverify {

struct Record {
    int batch = 0;
    int64_t id = 0;
    int day = 0, race = 0, cls = 0;
    uint32_t flags2 = 0;
    double x = 0, y = 0; // blocks
    std::vector<std::string> tokens;
};

std::vector<Record> read(const std::filesystem::path &file) {
    std::vector<Record> out;
    std::ifstream in(file, std::ios::binary);
    std::string line;
    int batch = -1;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.rfind("# ", 0) == 0) {
            batch++;
            continue;
        }
        const size_t colon = line.find(" :");
        if (line.rfind("id=", 0) != 0 || colon == std::string::npos)
            continue;
        Record r;
        r.batch = batch;
        std::istringstream head(line.substr(0, colon));
        std::string field;
        while (head >> field) {
            const size_t eq = field.find('=');
            const std::string key = field.substr(0, eq), value = eq == std::string::npos ? "" : field.substr(eq + 1);
            if (key == "id") r.id = std::strtoll(value.c_str(), nullptr, 10);
            else if (key == "day") r.day = std::atoi(value.c_str());
            else if (key == "race") r.race = std::atoi(value.c_str());
            else if (key == "class") r.cls = std::atoi(value.c_str());
            else if (key == "flags2") r.flags2 = (uint32_t)std::strtoul(value.c_str(), nullptr, 16);
            else if (key == "pos") std::sscanf(value.c_str(), "%lf,%lf", &r.x, &r.y);
        }
        // " tok | tok | ... |": the exporter's names (name=, item=, key=) are not the game's script
        std::istringstream rest(line.substr(colon + 2));
        std::string token;
        while (std::getline(rest, token, '|')) {
            const size_t a = token.find_first_not_of(' '), b = token.find_last_not_of(' ');
            if (a == std::string::npos)
                continue;
            token = token.substr(a, b - a + 1);
            if (token.rfind("name=", 0) != 0 && token.rfind("item=", 0) != 0 && token.rfind("key=", 0) != 0)
                r.tokens.push_back(token);
        }
        out.push_back(std::move(r));
    }
    return out;
}

// The world's pet list (world+0x360): the first list of Extract/oracle/items.bin.
std::vector<int32_t> petList(const std::filesystem::path &file) {
    std::ifstream in(file, std::ios::binary);
    char magic[4] = {};
    uint32_t count = 0;
    in.read(magic, 4).read(reinterpret_cast<char *>(&count), 4);
    std::vector<int32_t> pets;
    if (!in || std::memcmp(magic, "CIG1", 4) != 0)
        return pets;
    pets.resize(count);
    in.read(reinterpret_cast<char *>(pets.data()), (std::streamsize)count * 4);
    return in ? pets : std::vector<int32_t>();
}

// Branches whose words depend on the player's state.
bool playerDependent(const std::string &branch) {
    return branch == "trainer" || branch == "innkeeper" || branch == "equipment remark" || branch == "class remark" ||
           branch == "small talk 7" || branch.rfind("artifact rumour", 0) == 0 || branch.rfind("help:", 0) == 0;
}

// Equal but for what the export cannot pin down: a direction off by a block (the export rounds
// the speaker's place to a tenth of a block) and a friend's race within its family (the game's
// unseeded rand()).
bool closeEnough(const std::vector<std::string> &a, const std::vector<std::string> &b) {
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); i++) {
        if (a[i] == b[i])
            continue;
        const int va = std::atoi(a[i].c_str()), vb = std::atoi(b[i].c_str());
        const bool direction = (i >= 1 && a[i - 1] == "define_direction") || (i >= 2 && a[i - 2] == "define_direction");
        if (direction && std::abs(va - vb) <= 1)
            continue;
        if (i >= 2 && a[i - 2] == "define_mob") { // its race
            const std::vector<int> family = talkgen::siblingGroup(va);
            if (std::find(family.begin(), family.end(), vb) != family.end())
                continue;
        }
        return false;
    }
    return true;
}

std::string joined(const std::vector<std::string> &tokens) {
    std::string s;
    for (const std::string &t : tokens)
        s += (s.empty() ? "" : " ") + t;
    return s;
}

} // namespace talkverify

static int talkVerify(int show) {
    using namespace talkverify;
    Assets a;
    if (!a.locate(""))
        return 1;
    const std::filesystem::path extract = a.gameDir() / "Extract";
    worldgen::setWorldSeed(seedOf(zonesDir(a.gameDir())));
    worldgen::loadCreatureTables(extract / "oracle");
    const std::vector<Record> records = read(extract / "talk-scripts.txt");
    talkgen::World world;
    world.pets = petList(extract / "oracle" / "items.bin");
    world.timeOfDayMs = 0; // the innkeepers all spoke of the night's rest
    std::printf("%zu records in %d batches; %zu pets\n", records.size(), records.empty() ? 0 : records.back().batch + 1,
                world.pets.size());

    // Each person's record, from the spawn points of the zones around where the export saw them.
    std::map<int64_t, worldgen::CreatureRecord> people;
    std::set<std::pair<int32_t, int32_t>> zonesMade;
    std::set<int64_t> missing;
    for (const Record &r : records) {
        if (people.count(r.id) || missing.count(r.id))
            continue;
        const int32_t zx = (int32_t)std::floor(r.x / 64), zy = (int32_t)std::floor(r.y / 64);
        for (int ring = 0; ring <= 4 && !people.count(r.id); ring++)
            for (int32_t x = zx - ring; x <= zx + ring; x++)
                for (int32_t y = zy - ring; y <= zy + ring; y++) {
                    if (!zonesMade.insert({x, y}).second)
                        continue;
                    for (const worldgen::SpawnPoint &sp : zoneSpawnList(x, y)) {
                        if (sp.hostility != 3)
                            continue;
                        int seed = 1;
                        people[sp.id] = residentOf(sp, [&] { return (seed = seed * 1103515245 + 12345) >> 16 & 0x7fff; });
                    }
                }
        if (!people.count(r.id))
            missing.insert(r.id);
    }
    std::set<int64_t> exported;
    for (const Record &r : records)
        exported.insert(r.id);
    std::printf("%zu of the %zu people exported are among the spawn points of %zu zones; the others are made from the export alone\n",
                exported.size() - missing.size(), exported.size(), zonesMade.size());

    // The exporting player per batch: class and specialisation from what trainers and class
    // remarks said.
    const int batches = records.empty() ? 0 : records.back().batch + 1;
    std::vector<talkgen::Player> players(batches);
    {
        int cls = 1, spec = 0;
        size_t at = 0;
        for (int b = 0; b < batches; b++) {
            for (; at < records.size() && records[at].batch == b; at++) {
                const std::vector<std::string> &t = records[at].tokens;
                for (size_t i = 0; i < t.size(); i++) {
                    if (t[i] == "define_specialization" && i + 2 < t.size())
                        cls = std::atoi(t[i + 1].c_str()), spec = 1 - std::atoi(t[i + 2].c_str());
                    static const char *const classes[4] = {"so_you_are_a_warrior", "so_you_are_a_ranger", "so_you_are_a_mage", "so_you_are_a_rogue"};
                    for (int k = 0; k < 4; k++)
                        if (t[i] == classes[k])
                            cls = k + 1;
                }
            }
            players[b].cls = cls, players[b].spec = spec;
        }
    }
    // The speaker: its record when the generated world has it, else what the export tells
    // (id, race, class, flags2), at home where the export saw it. A mission resident that
    // thanked the player had its task done (+0x128 bit 0x100), which the export cannot show.
    std::set<int64_t> tasksDone;
    for (const Record &r : records)
        if ((r.flags2 & 0x2000) && !r.tokens.empty() && r.tokens[0] == "thank_you")
            tasksDone.insert(r.id);
    auto speakerFor = [&](const Record &r) {
        talkgen::Speaker s;
        const int64_t x = (int64_t)std::llround(r.x * 65536), y = (int64_t)std::llround(r.y * 65536);
        if (auto it = people.find(r.id); it != people.end())
            s = talkgen::speakerOf(it->second.data());
        else {
            s.id = (uint64_t)r.id, s.race = r.race, s.cls = r.cls, s.appearance = r.flags2, s.hostility = 3;
            s.homeX = x, s.homeY = y;
        }
        s.x = x, s.y = y; // where the export saw them
        if (tasksDone.count(r.id))
            s.toggles |= 0x100;
        return s;
    };
    talkgen::CRand runtime;

    // Worn gear per batch: which of n placeholder slots each equipment remark picks, against the
    // item the export named; the fewest slots that agree with every remark of the batch.
    {
        struct Remark {
            std::array<int, 11> pick{}; // the slot picked, with 1..10 slots filled
            std::string item;           // the export's five numbers; empty if it made no such remark
        };
        std::vector<std::vector<Remark>> byBatch(batches);
        for (const Record &r : records) {
            world.day = r.day;
            talkgen::Player probe = players[r.batch];
            Remark remark;
            bool remarks = false;
            for (int n = 1; n <= 10; n++) {
                probe.equipment = {};
                for (int k = 0; k < n; k++)
                    probe.equipment[k].category = 255, probe.equipment[k].id = k;
                std::string branch;
                const std::vector<std::string> t = talkgen::script(speakerFor(r), probe, world, runtime, &branch);
                if (branch != "equipment remark")
                    break; // (the draw for a remark does not depend on the gear)
                remarks = true;
                for (size_t i = 0; i + 2 < t.size(); i++)
                    if (t[i] == "define_item")
                        remark.pick[n] = std::atoi(t[i + 2].c_str());
            }
            if (!remarks)
                continue;
            const std::vector<std::string> &e = r.tokens;
            if (std::find(e.begin(), e.end(), "nice_equipment_item") != e.end())
                for (size_t i = 0; i + 5 < e.size(); i++)
                    if (e[i] == "define_item")
                        remark.item = e[i + 1] + " " + e[i + 2] + " " + e[i + 3] + " " + e[i + 4] + " " + e[i + 5];
            byBatch[r.batch].push_back(remark);
        }
        std::map<int, int> slotCounts;
        for (int b = 0; b < batches; b++) {
            int chosen = -1;
            std::map<int, std::string> items;
            for (int n = 0; n <= 10 && chosen < 0; n++) {
                items.clear();
                bool fits = true;
                for (const Remark &s : byBatch[b]) {
                    if (n == 0) { // nothing worn: no remark at all
                        fits = fits && s.item.empty();
                        continue;
                    }
                    if (s.item.empty()) {
                        fits = false;
                        break;
                    }
                    auto [it, added] = items.emplace(s.pick[n], s.item);
                    fits = fits && (added || it->second == s.item);
                }
                if (fits)
                    chosen = n;
            }
            slotCounts[chosen]++;
            if (chosen <= 0)
                continue;
            for (int k = 0; k < chosen; k++) {
                worldgen::Item &slot = players[b].equipment[k];
                slot.category = 254, slot.id = k; // a slot no remark of the batch picked
                if (auto it = items.find(k); it != items.end()) {
                    int v[5] = {};
                    std::sscanf(it->second.c_str(), "%d %d %d %d %d", &v[0], &v[1], &v[2], &v[3], &v[4]);
                    slot.category = (uint8_t)v[0], slot.id = v[1], slot.modifier = (uint32_t)v[2];
                    slot.material = (uint8_t)v[3], slot.rarity = (uint8_t)v[4];
                }
            }
        }
        std::string counts;
        for (auto &[n, count] : slotCounts)
            counts += " " + (n < 0 ? std::string("none fit") : std::to_string(n) + " slots") + ": " + std::to_string(count);
        std::printf("worn gear read back per batch:%s\n", counts.c_str());
    }

    struct Tally {
        int records = 0, exact = 0, close = 0;
        std::vector<std::string> examples;
    };
    // By where the speaker came from, then by branch.
    std::map<std::string, std::map<std::string, Tally>> bySource;
    int classDiffers = 0, flagsDiffer = 0;
    std::map<int, std::map<std::string, int>> monsterByIndex;
    // Playing the generated scripts (playScript, with CubeVulkan's names): what cannot be said yet.
    Speech speech;
    GameNames generatedNames;
    std::string why;
    const bool canPlay = speech.load(a.gameDir() / "dict_en.xml", why) && generatedNames.load(extract / "names.txt");
    TalkNames names;
    names.place = [&](int, int, int kind) { return speech.place(locationKind(kind), "Name"); };
    names.person = [&](int id, int race) { return generatedNames.name(id, race); };
    names.land = [&](int, int seed) { return generatedNames.name(seed, -1); };
    names.item = [&](const int(&it)[5], std::string &name, std::string &key) { key = itemnames::itemKey(it[0], it[1]), name = key; };
    std::mt19937 random(1);
    std::map<std::string, int> unsaid;
    runtime = talkgen::CRand();
    for (const Record &r : records) {
        const bool spawnedRecord = people.count(r.id) != 0;
        const talkgen::Speaker s = speakerFor(r);
        if (spawnedRecord) {
            classDiffers += s.cls != r.cls;
            flagsDiffer += s.appearance != r.flags2;
        }
        world.day = r.day;
        std::string branch;
        const std::vector<std::string> made = talkgen::script(s, players[r.batch], world, runtime, &branch);
        if (canPlay)
            for (const std::string &what : playScript(TalkScript{r.id, r.race, r.day, "", made}, speech, random, &names).unrendered)
                unsaid[what.substr(0, what.find(" ("))]++;
        Tally &t = bySource[spawnedRecord ? "speaker from its generated spawn point" : "speaker from the export alone"][branch];
        t.records++;
        const bool exact = made == r.tokens, close = exact || closeEnough(made, r.tokens);
        t.exact += exact;
        t.close += close;
        if (!close && (int)t.examples.size() < show)
            t.examples.push_back("    id " + std::to_string(r.id) + " day " + std::to_string(r.day) + "\n      game: " +
                                 joined(r.tokens) + "\n      ours: " + joined(made));
        if (branch.rfind("help: monster", 0) == 0) // which model each of the 13 draws gives
            for (size_t i = 0; i + 3 < r.tokens.size(); i++)
                if (r.tokens[i] == "define_location") {
                    const int32_t x = std::atoi(r.tokens[i + 1].c_str()), y = std::atoi(r.tokens[i + 2].c_str());
                    const int kind = std::atoi(r.tokens[i + 3].c_str());
                    for (size_t j = i; j + 1 < r.tokens.size(); j++)
                        if (r.tokens[j] == "define_model" && kind != 23 && kind != 36) {
                            monsterByIndex[(int)(worldgen::random((uint32_t)x + 57u * (uint32_t)y + 0x2b3a706u) % 13)][r.tokens[j + 1]]++;
                            break;
                        }
                    break;
                }
    }
    std::printf("spawned records whose class differs from the export: %d; whose flags2 differ: %d\n", classDiffers, flagsDiffer);
    if (!unsaid.empty()) {
        std::printf("\nnot said in full when played (scripts):\n");
        for (auto &[what, n] : unsaid)
            std::printf("  %6d  %s\n", n, what.c_str());
    }
    for (auto &[source, byBranch] : bySource)
        for (const bool dependent : {false, true}) {
            std::printf("\n%s; branches that %s on the player's state:\n", source.c_str(), dependent ? "depend" : "do not depend");
            int records = 0, exact = 0, close = 0;
            for (auto &[branch, t] : byBranch) {
                if (playerDependent(branch) != dependent)
                    continue;
                records += t.records, exact += t.exact, close += t.close;
                std::printf("  %-34s %6d records, %6d exact (%5.1f%%), %6d within tolerance\n", branch.c_str(), t.records,
                            t.exact, t.records ? 100.0 * t.exact / t.records : 0.0, t.close);
                for (const std::string &e : t.examples)
                    std::printf("%s\n", e.c_str());
            }
            std::printf("  %-34s %6d records, %6d exact (%5.1f%%), %6d within tolerance\n", "total", records, exact,
                        records ? 100.0 * exact / records : 0.0, close);
        }
    if (!monsterByIndex.empty()) {
        std::printf("\nmonster models by draw (R mod 13):\n");
        for (auto &[index, models] : monsterByIndex) {
            std::string seen;
            for (auto &[model, n] : models)
                seen += " " + model + "x" + std::to_string(n);
            std::printf("  %2d:%s\n", index, seen.c_str());
        }
    }
    return 0;
}

static void speechGrammar() {
    Assets assets;
    if (!assets.locate(""))
        return;
    Speech speech;
    std::string why;
    CHECK(speech.load(assets.gameDir() / "dict_en.xml", why));
    CHECK(speech.count() > 900);
    CHECK(speech.name("Wizard") == "Wizard" && speech.name("Wizard", true) == "Wizards");
    Speech::Context context;
    context.tags = {"mob_m", "east"};
    context.value = [](const std::string &fn, const std::string &args) -> std::string {
        if (fn == "mob_name") return "Gotik";
        if (fn == "mob") return args == "plural" ? "Radishlings" : "Radishling";
        if (fn == "zone") return "Lalan Village";
        return "";
    };
    std::mt19937 random(7);
    for (const char *id : {"hello", "my_name_is", "objective_defeat_enemies_in_zone", "the_enemies_are_in_zone_direction_of_here",
                           "solved_defeat_enemies_in_zone", "good_work", "npc_needs_your_help", "goodbye"})
        for (int i = 0; i < 20; i++) {
            std::string line = speech.say(id, context, random);
            CHECK(!line.empty());
            CHECK(line.find_first_of("{}[]*@$#|") == std::string::npos);
        }
    CHECK(speech.say("objective_defeat_enemies_in_zone", context, random) == "Defeat the Radishlings in Lalan Village.");
    CHECK(speech.say("my_name_is", context, random).find("Gotik") != std::string::npos);
    std::string where = speech.say("the_enemies_are_in_zone_direction_of_here", context, random);
    CHECK(where.find("east of here") != std::string::npos); // #east picks its alternative
}

// Deaths as recorded in the game: coins pay one gold each 0.59-0.89 s after the
// kill, and the body's voxels fly out, fall and lie on the ground.

// The body's footprint against blocks, as measured in the game: a one-block step
// at x = 37 is climbed between these centre positions (three logged step-ups).
static void bodyFootprint() {
    Body body(.96f);
    // A floor at z 112 everywhere, with one more block from x = 37 on.
    auto floor = [](float x, float, float, float &h) { h = x >= 37 ? 113.f : 112.f; return true; };
    const float before[3] = {37 - .5155f, 37 - .5424f, 37 - .4834f}, after[3] = {37 - .4489f, 37 - .4751f, 37 - .4392f};
    for (int i = 0; i < 3; i++) {
        CHECK(body.floorUnder(floor, before[i], 0, 114.1f, 0) == 112.f);
        CHECK(body.floorUnder(floor, after[i], 0, 114.1f, 0) == 113.f);
    }
    // A wall two blocks tall from x = 10: the edge stops at it.
    auto wall = [](float x, float, float z) { return x >= 10 && z < 3; };
    CHECK(!body.blocked(wall, 10 - .49f, 0, 0));
    CHECK(body.blocked(wall, 10 - .47f, 0, 0));
    CHECK(body.blocked(wall, 10 - .2f, 5 + .3f, 0)); // a corner reaches it before the centre does
    CHECK(!Body(.96f).blocked(wall, 9.4f, 0, 5)); // above it
}

// Walking speed tick by tick against the game's log (start from rest, then release).
static void walkingSpeed() {
    Walk walk;
    const float logged[11] = {1.29f, 2.516f, 3.694f, 4.825f, 5.912f, 6.951f, 7.948f, 8.912f, 9.828f, 10.706f, 11.561f};
    for (float v : logged) {
        walk.update({0, 1}, Walk::TICK);
        CHECK(std::abs(walk.velocity.y - v) < .05f);
    }
    walk.update({0, 1}, 1);
    CHECK(std::abs(walk.velocity.y - 11.57f) < 1e-3f); // capped
    walk.velocity = {0, 8.327f};
    walk.update({0, 0}, 4 * Walk::TICK); // logged: 8.327 -> 5.977 over four ticks
    CHECK(std::abs(walk.velocity.y - 5.977f) < .1f);
}

static void deathEffects() {
    DeathEffects fx;
    auto floor = [](float, float, float, float &h) { h = 0; return true; };
    std::vector<DeathEffects::Piece> body;
    for (int z = 0; z < 10; z++)
        for (int x = 0; x < 4; x++)
            body.push_back({{x * .1f, 0, .5f + z * .1f}, {200, 50, 50}, .1f});
    fx.burst(body, {0, 0, 1});
    CHECK(fx.fragmentCount() > 0 && fx.fragmentCount() < body.size());
    size_t emitted = fx.fragmentCount();
    for (int i = 0; i < 120; i++) // two seconds: all down on the floor
        fx.update(1 / 60.0, floor);
    Mesh mesh;
    fx.fragmentMesh(mesh);
    CHECK(mesh.vertices.size() == emitted * 24);
    for (const auto &f : fx.fragmentStates()) {
        CHECK(f.resting && f.at.z >= 0 && f.at.z < .5);
        CHECK(glm::length(f.velocity) == 0 && glm::length(f.spin) == 0);
    }

    // A pair of native samples, 32 ms apart (two 16 ms native updates).
    // Positions are relative, with no native sprite/colour/recording bundled.
    DeathEffects::Fragment observed{};
    observed.at = {0, 0, 10};
    observed.ageMS = 11;
    observed.velocity = {.3179931640625f, -.03424072265625f, -.7522473335266113f};
    observed.angles = {-.643208146f, .681344092f, .460243523f};
    observed.spin = {-40.2005081f, 42.5840034f, 28.7652187f};
    DeathEffects::stepFragment(observed, 16, {});
    DeathEffects::stepFragment(observed, 16, {});
    CHECK(observed.ageMS == 43);
    CHECK(std::abs(observed.velocity.z - -1.7122473717f) < 1e-6f);
    CHECK(glm::length(observed.at - glm::dvec3(.010162353515625, -.001068115234375, 9.9528961181640625)) < 1e-9);
    CHECK(glm::length(observed.angles - glm::vec3(-1.929624438f, 2.044032335f, 1.380730629f)) < 1e-6f);

    // Native draw's matrix order, evaluated independently from one measured
    // part basis and spin. This relative corner catches local-axis rotation.
    DeathEffects::Fragment rotated{};
    rotated.angles = {-5.788872719f, 6.132096767f, 4.142191410f};
    rotated.basis = glm::mat3(
        glm::vec3(.060267963f, -.058513016f, -.000000765f),
        glm::vec3(-.034292720f, -.035322130f, .068061419f),
        glm::vec3(.047410782f, .048832104f, .049230516f));
    CHECK(glm::length(DeathEffects::fragmentBasis(rotated) * glm::vec3(-.5f) -
                      glm::vec3(-.044035702f, .013325621f, -.056349667f)) < 1e-7f);
    rotated.angles = glm::vec3(0);
    CHECK(DeathEffects::fragmentBasis(rotated) == rotated.basis);

    DeathEffects::Fragment delayed{};
    delayed.at = {0, 0, 2}; delayed.ageMS = -100; delayed.velocity = {0, 0, 0}; delayed.spin = {10, 20, 30};
    DeathEffects::stepFragment(delayed, 80, {});
    CHECK(delayed.at.z == 2 && delayed.angles == glm::vec3(0));
    DeathEffects::stepFragment(delayed, 20, {}); // starts on crossing zero; full native tick
    CHECK(std::abs(delayed.velocity.z + .6f) < 1e-6f && delayed.at.z < 2);

    DeathEffects::Fragment wall{};
    wall.at = {.99, 0, 1}; wall.velocity = {2, 1, 0}; wall.spin = {0, 0, 0};
    DeathEffects::stepFragment(wall, 16, [](glm::dvec3 p) { return p.x >= 1; });
    CHECK(wall.at.x == .99 && wall.velocity.x == 0 && wall.at.y > 0);
    DeathEffects::Fragment landing{};
    landing.at = {0, 0, .001}; landing.velocity = {2, 1, -1}; landing.spin = {10, 20, 30};
    auto solidFloor = [](glm::dvec3 p) { return p.z < 0; };
    DeathEffects::stepFragment(landing, 16, solidFloor);
    auto restingAt = landing.at;
    CHECK(landing.resting && landing.velocity == glm::vec3(0) && landing.spin == glm::vec3(0));
    DeathEffects::stepFragment(landing, 100, solidFloor);
    CHECK(landing.at == restingAt);

    // All exposed voxels are eligible; internal voxels never become particles.
    CubModel cube;
    cube.sx = cube.sy = cube.sz = 3; cube.rgb.assign(3 * 3 * 3 * 3, 255);
    int surface = 0;
    for (int x = 0; x < 3; x++) for (int y = 0; y < 3; y++) for (int z = 0; z < 3; z++)
        surface += cube.exposed(x, y, z);
    CHECK(surface == 26 && !cube.exposed(1, 1, 1) && !cube.exposed(-1, 0, 0));

    // Match the native global cap without enlarging the selected fragments.
    DeathEffects capped;
    body.assign(10000, {{1, 2, 3}, {120, 100, 80}, .1f});
    body[0].basis[0][0] = -1; // reflected body part retains outward winding
    for (auto &p : body) p.basis[0][0] = -1;
    capped.burst(body, {1, 2, 3});
    CHECK(capped.fragmentCount() == DeathEffects::MAX_PIECES);
    for (const auto &f : capped.fragmentStates()) {
        CHECK(f.ageMS > -1000 && f.ageMS <= 0);
        CHECK(std::abs(glm::length(f.basis[0]) - .1f) < 1e-6f);
        CHECK(f.velocity == glm::vec3(0)); // no added upward kick
    }
    capped.fragmentMesh(mesh);
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        const Vertex &a = mesh.vertices[mesh.indices[i]], &b = mesh.vertices[mesh.indices[i + 1]], &c = mesh.vertices[mesh.indices[i + 2]];
        glm::vec3 ab(b.x-a.x,b.y-a.y,b.z-a.z), ac(c.x-a.x,c.y-a.y,c.z-a.z);
        glm::vec3 n = glm::vec3(a.normal, a.pad0, a.pad1) / 127.5f - 1.f;
        CHECK((a.flags & PACKED_NORMAL) && glm::dot(glm::cross(ab, ac), n) > 0);
    }
    capped.update(4.9, {}, {});
    CHECK(capped.fragmentCount() == DeathEffects::MAX_PIECES); // no early shrink/fade
    capped.update(1.2, {}, {});
    CHECK(capped.fragmentCount() == 0);

    // A later burst replaces old debris when the shared particle budget fills.
    DeathEffects newest;
    body.assign(10000, {{1, 0, 0}, {120, 100, 80}, .1f});
    newest.burst(body, {});
    for (size_t i = 0; i < body.size(); i++) body[i].at.x = 100 + (float)i;
    newest.burst(body, {});
    CHECK(newest.fragmentCount() == DeathEffects::MAX_PIECES);
    CHECK(newest.fragmentStates().back().at.x > 100); // old burst discarded
    CHECK(newest.fragmentStates().front().at.x > newest.fragmentStates().back().at.x);
}

// NativeEffects on hand-made worlds, for what the recorded steps (arrows and healing areas)
// don't reach: a healing spell bursting on a wall it breaks and staying as an area, and a
// drifting effect running out (docs/specs/effect-objects.md §1.5, §1.8, §1.9).
static void effectObjects() {
    using nativeeffects::Effect;
    auto i32 = [](const Effect &e, size_t o) { int32_t v; std::memcpy(&v, e.data() + o, 4); return v; };
    auto f32 = [](const Effect &e, size_t o) { float v; std::memcpy(&v, e.data() + o, 4); return v; };
    auto i64 = [](const Effect &e, size_t o) { int64_t v; std::memcpy(&v, e.data() + o, 8); return v; };
    auto setF = [](Effect &e, size_t o, float v) { std::memcpy(e.data() + o, &v, 4); };
    auto setI = [](Effect &e, size_t o, int32_t v) { std::memcpy(e.data() + o, &v, 4); };
    auto setL = [](Effect &e, size_t o, int64_t v) { std::memcpy(e.data() + o, &v, 8); };

    const Effect fresh = nativeeffects::made();
    CHECK(i64(fresh, 0) == -1 && f32(fresh, 0x54) == 0.5f && f32(fresh, 0x58) == 1.0f && f32(fresh, 0x5c) == 1.0f &&
          i32(fresh, 0x74) == 5000 && i32(fresh, 0x68) == 0 && fresh[0x64] == 0);

    // Open air, but for one block at (2, 0, 0) that effects break (flag byte bit 0).
    const nativeai::Block breakable{10, 20, 30, 0, 1, 1};
    std::set<std::array<int, 3>> broken;
    auto blockOf = [&](int x, int y, int z) {
        return x == 2 && y == 0 && z == 0 && !broken.count({x, y, z}) ? breakable : nativeai::Block{};
    };
    nativeeffects::World w;
    w.blockAt = [&](int64_t x, int64_t y, int64_t z) { return blockOf((int)(x >> 16), (int)(y >> 16), (int)(z >> 16)); };
    w.block = blockOf;
    w.zone = [](int, int) { return true; };
    w.rand = [] { return 0; };
    w.removeBlock = [&](int x, int y, int z) { broken.insert({x, y, z}); };

    // An ownerless healing spell (kind 3, hit type 2) flying +x at 50 blocks a second.
    Effect spell = nativeeffects::made();
    setL(spell, 0x08, 0x18000), setL(spell, 0x10, 0x8000), setL(spell, 0x18, 0x8000);
    setF(spell, 0x44, 50.0f);
    setF(spell, 0x50, 10.0f), setF(spell, 0x5c, 0.4f);
    setI(spell, 0x68, 3);
    spell[0x6c] = 2;
    std::list<Effect> effects{spell};
    nativeeffects::step(w, effects, 16.0f);
    CHECK(effects.size() == 1);
    const Effect &area = effects.front();
    CHECK(i32(area, 0x68) == 6 && f32(area, 0x54) == 0.4f * 5.0f + 5.0f && f32(area, 0x70) == 3000.0f);
    CHECK(f32(area, 0x50) == 10.0f * 0.05f && f32(area, 0x5c) == 0.0f && area[0x64] == 0);
    CHECK(i64(area, 0x08) < 0x20000); // put back out of the wall
    CHECK(broken.count({2, 0, 0}) && w.blockChanges.size() == 1);
    int32_t cell[3];
    std::memcpy(cell, w.blockChanges[0].data(), 12);
    CHECK(cell[0] == 2 && cell[1] == 0 && cell[2] == 0 && std::memcmp(w.blockChanges[0].data() + 0xc, breakable.data(), 6) == 0);
    int32_t ids[2];
    CHECK(w.sounds.size() == 2);
    std::memcpy(&ids[0], w.sounds[0].data() + 0x18, 4), std::memcpy(&ids[1], w.sounds[1].data() + 0x18, 4);
    CHECK(ids[0] == 0x1a && ids[1] == 0x2d); // the debris, then the burst
    CHECK(w.particles.size() == 2);

    // A drifting effect (kind 8) moves without touching anything and runs out.
    Effect drift = nativeeffects::made();
    setI(drift, 0x68, 8), setI(drift, 0x74, 20);
    setF(drift, 0x44, 10.0f);
    effects = {drift};
    nativeeffects::step(w, effects, 16.0f);
    CHECK(effects.size() == 1 && i64(effects.front(), 0x08) == (int64_t)(0.16f * 65536.0f));
    nativeeffects::step(w, effects, 16.0f);
    CHECK(effects.empty());
}


// CubeVulkan's world generator against the game's own (worldgen/Height.h): ZoneExtractor's O key
// writes every intermediate value of the game's ground height at ~2900 points
// (Extract/gen-oracle.txt). Each must match exactly. `EngineTests gen` prints details.
static int genMatchesGame(bool verbose, const char *only = nullptr) {
    Assets a;
    if (!a.locate(""))
        return 0;
    std::ifstream in(a.gameDir() / "Extract" / "gen-oracle.txt");
    if (!in)
        return 0; // no oracle on this machine
    const char *names[] = {"regionX", "regionY", "kind", "big", "variant", "radius", "cx", "cy", "dist2", "weight",
                           "lake", "base", "roads", "w13", "w125", "w8", "river", "lava", "swamp", "high", "a44",
                           "b59", "veg", "height0", "height4"};
    const int N = 25;
    int wrong[N] = {}, points = 0, shown = 0;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        for (auto &ch : line)
            if (ch == '|')
                ch = ' ';
        std::istringstream s(line);
        int x, y;
        double game[N];
        s >> x >> y;
        for (double &v : game)
            s >> v;
        if (!s)
            continue;
        points++;
        const worldgen::ColumnValues c = worldgen::columnValues(x, y, 0);
        worldgen::Int2 r = worldgen::regionAt(x, y);
        double mine[N] = {(double)r.x, (double)r.y, (double)c.place.kind, (double)c.place.big, (double)c.place.variant,
                          (double)c.place.radius, (double)c.place.point.x, (double)c.place.point.y, c.place.dist2,
                          c.place.weight, c.lake, c.base, c.roads, c.lavaRegion, c.forestRegion, c.wet, c.river,
                          c.lava, c.swamp, c.highlands, c.dryRegion, c.dryness, c.vegetation, c.height,
                          worldgen::groundHeight(x, y, 4)};
        for (int i = 0; i < N; i++)
            if ((float)game[i] != (float)mine[i]) {
                wrong[i]++;
                if (verbose && shown < 40 && (!only || !std::strcmp(only, names[i])))
                    std::printf("  %d %d %s: game %.9g, CubeVulkan %.9g\n", x, y, names[i], game[i], mine[i]), shown++;
            }
    }
    int total = 0;
    for (int i = 0; i < N; i++) {
        total += wrong[i];
        if (verbose || wrong[i])
            std::printf("gen %-8s %d of %d differ\n", names[i], wrong[i], points);
    }
    CHECK(points > 0 && total == 0);
    return total;
}

// The generator's blocks against the game's (ZoneExtractor key O also writes
// Extract/gen-blocks.txt: the zone generator's column inputs and 0x2d8e00's output at the
// surface and 1, 3, 10 and 40 blocks down).
static int genBlocksMatchGame(bool verbose, const char *only = nullptr) {
    Assets a;
    if (!a.locate(""))
        return 0;
    std::ifstream in(a.gameDir() / "Extract" / "gen-blocks.txt");
    if (!in)
        return 0;
    // The oracle's world: its seed is in the header ("seed 123"), 123 before it was logged.
    int worldSeed = 123;
    std::string header;
    if (std::getline(in, header) && header.find(" seed ") != std::string::npos)
        worldSeed = std::atoi(header.c_str() + header.find(" seed ") + 6);
    worldgen::setWorldSeed(worldSeed);
    CHECK(header.find("world+0x330 " + std::to_string(worldgen::currentWorld().sandX()) + " world+0x334 " +
                      std::to_string(worldgen::currentWorld().sandY())) != std::string::npos);
    const worldgen::SandOffsets sand = worldgen::sandOffsets(worldgen::currentWorld());
    const int D = 5, N = 10 + D * 6;
    std::vector<std::string> names = {"h", "west", "east", "south", "north", "warmth", "kind27", "snow",
                                      "snowR", "snowG", "snowB", "bedR", "bedG", "bedB", "top"};
    names.erase(names.begin(), names.begin() + 5); // heights are checked by genMatchesGame
    for (int d : {0, 1, 3, 10, 40})
        for (const char *p : {"r", "g", "b", "glow", "type", "flags"})
            names.push_back(std::string(p) + std::to_string(d));
    std::vector<int> wrong(N);
    int points = 0, shown = 0;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        for (auto &ch : line)
            if (ch == '|')
                ch = ' ';
        std::istringstream s(line);
        int x, y;
        double h[5], game[N];
        s >> x >> y;
        for (double &v : h)
            s >> v;
        for (double &v : game)
            s >> v;
        if (!s)
            continue;
        points++;
        const worldgen::ColumnValues c = worldgen::columnValues(x, y, 0);
        const worldgen::ColourInputs colours = worldgen::colourInputs(c);
        const worldgen::NeighbourHeights around{(float)h[1], (float)h[2], (float)h[3], (float)h[4]};
        int fz = (int)std::floor(c.height);
        worldgen::BlockProps bed = worldgen::riverBedColour(x, y, fz - 1, sand);
        std::vector<double> mine = {colours.warmth, colours.kind27, worldgen::snowAt(x, y, colours.warmth, around) ? 1.0 : 0.0,
                                    200, 280, 400, bed.r, bed.g, bed.b, worldgen::waterLevel(c.base, c.place)};
        for (int d : {0, 1, 3, 10, 40}) {
            worldgen::BlockProps p = worldgen::blockAt(x, y, fz - d, c.height, around, colours, sand);
            for (double v : {(double)p.r, (double)p.g, (double)p.b, (double)p.glow, (double)p.type, (double)p.flags})
                mine.push_back(v);
        }
        for (int i = 0; i < N; i++)
            if ((float)game[i] != (float)mine[i]) {
                wrong[i]++;
                if (verbose && shown < 40 && (!only || names[i] == only))
                    std::printf("  %d %d %s: game %.9g, CubeVulkan %.9g\n", x, y, names[i].c_str(), game[i], mine[i]), shown++;
            }
    }
    int total = 0;
    for (int i = 0; i < N; i++) {
        total += wrong[i];
        if (verbose || wrong[i])
            std::printf("blocks %-7s %d of %d differ\n", names[i].c_str(), wrong[i], points);
    }
    CHECK(points > 0 && total == 0);
    return total;
}

// Generated terrain columns against zones extracted from the game (Extract/seed_*): every
// block the generator lays down is compared with the game's block at that height.
// Structures (trees, buildings, objects) are not generated yet, so columns the game built
// on are counted apart.
// Block types the terrain itself makes (air, water, wet, grass, ground, rock, snow, road,
// lava ground, lava, cliff face, poison).
static bool terrainType(int t) {
    return t == 0 || t == 2 || t == 3 || t == 4 || t == 5 || t == 6 || t == 10 || t == 13 || t == 14 || t == 17 ||
           t == 18 || t == 19;
}
// With a kind, only columns inside places of that kind are compared (to study them).
static int zonesPlaceKind = 0;
static std::map<int, int> placeKinds; // columns per place kind, counted apart
static int genZonesMatchExtracted(bool verbose, int maxZones) {
    Assets a;
    if (!a.locate(""))
        return 0;
    std::filesystem::path dir = zonesDir(a.gameDir());
    if (dir.empty())
        return 0;
    struct B { uint8_t r, g, b, glow, type, flags; };
    // The extraction's world seed (folder seed_<n>) seeds each zone's rand().
    worldgen::setWorldSeed(seedOf(dir));
    worldgen::loadCreatureTables(spawnsDir(a.gameDir())); // (the creature passes share the zones' rand())
    int zones = 0, columns = 0, exact = 0, built = 0, inPlaces = 0, regionsWrong = 0, topStats[4] = {}, lightDiffers = 0, blocks = 0, blocksSame = 0, shown = 0;
    int structGame = 0, structOurs = 0, structSame = 0;
    std::map<int, int> differingBy; // differing blocks by the object of their column's cell
    std::map<std::pair<int, int>, int> objectlessByZone; // differing blocks in cells without an object, by zone
    std::map<int, std::array<int, 5>> differingHow; // ... only flag 0x40, missing, extra, air (both empty), other
    for (auto &e : std::filesystem::directory_iterator(dir)) {
        int ax, ay;
        if (!ExtractedWorld::zoneCoordinates(e.path(), ax, ay))
            continue;
        // Zones saved during the owner's normal play (around -94 million blocks, 2026-10-21)
        // were made with TerrainVariance and BigRegions running, so they are not vanilla;
        // the vanilla tours are within a few million blocks of the origin.
        if (std::abs((long long)ax) > 100000 || std::abs((long long)ay) > 100000)
            continue;
        if (zones++ >= maxZones)
            break;
        std::ifstream in(e.path(), std::ios::binary);
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), {});
        size_t at = 20;
        int32_t region;
        std::memcpy(&region, &data[12], 4);
        worldgen::Int2 r = worldgen::regionAt(ax * 64 + 32, ay * 64 + 32);
        regionsWrong += worldgen::regionBiome(r.x, r.y) != region;
        if (verbose && worldgen::regionBiome(r.x, r.y) != region)
            std::printf("  zone %d %d: game region type %d, CubeVulkan %d\n", ax, ay, region, worldgen::regionBiome(r.x, r.y));
        std::vector<worldgen::TerrainColumn> generated = worldgen::litZone(ax, ay);
        int zoneDiffers = 0;
        for (int col = 0; col < 4096; col++) {
            int32_t baseZ;
            uint32_t runs;
            std::memcpy(&baseZ, &data[at], 4), std::memcpy(&runs, &data[at + 4], 4);
            at += 8;
            std::vector<B> game;
            for (uint32_t r = 0; r < runs; r++) {
                uint16_t n;
                B b;
                std::memcpy(&n, &data[at], 2), std::memcpy(&b, &data[at + 2], 6);
                at += 8;
                game.insert(game.end(), n, b);
            }
            int x = ax * 64 + col / 64, y = ay * 64 + col % 64;
            const worldgen::TerrainColumn &t = generated[col];
            columns++;
            int top = t.base + (int)t.blocks.size();
            // The game built something here if it has blocks above the terrain, or blocks the
            // terrain never makes (tree trunks and leaves, buildings...).
            bool extra = false;
            for (size_t i = 0; i < game.size(); i++)
                if ((baseZ + (int)i >= top && game[i].type != 0) || !terrainType(game[i].type))
                    extra = true;
            // Places (towns, castles...) reshape their ground with their layout.
            worldgen::Place pl =
                worldgen::placeForColumn(worldgen::cellPlace(worldgen::floorDiv(x, 10), worldgen::floorDiv(y, 10)), x, y);
            bool place = pl.weight > 0.0f;
            // Columns of places the game left plain still count in the block tallies by object
            // below, but not in the plain-column statistics.
            const bool skipped = zonesPlaceKind ? !(place && pl.kind == zonesPlaceKind) : place && !extra;
            if (skipped) {
                inPlaces++;
                placeKinds[pl.kind]++;
            }
            bool same = true, onlyLight = true;
            for (int z = t.base; z < top && !skipped; z++) {
                const worldgen::Block &m = t.blocks[z - t.base];
                if (z < baseZ)
                    continue; // the game keeps no block there
                size_t i = (size_t)(z - baseZ);
                B g = i < game.size() ? game[i] : B{};
                blocks++;
                bool eq = g.r == m.r && g.g == m.g && g.b == m.b && g.glow == m.glow && g.type == m.type && g.flags == m.flags;
                blocksSame += eq;
                if (!eq) {
                    same = false;
                    bool lightOnly = g.g == m.g && g.b == m.b && g.glow == m.glow && g.type == m.type && g.flags == m.flags &&
                                     (g.type == 0 || g.type == 2);
                    onlyLight = onlyLight && lightOnly;
                    if (verbose && shown < 40 && !extra && !lightOnly)
                        std::printf("  %d %d z %d: game %d %d %d %d %d %d, CubeVulkan %d %d %d %d %d %d\n", x, y, z, g.r, g.g,
                                    g.b, g.glow, g.type, g.flags, m.r, m.g, m.b, m.glow, m.type, m.flags),
                            shown++;
                }
            }
            // Structure blocks (any type the terrain never makes), both ways, and every
            // differing block (but the light of air and water) by the cell's object.
            int lo = baseZ, hi = std::max(baseZ + (int)game.size(), top); // the game keeps nothing below baseZ
            for (int z = lo; z < hi; z++) {
                B g = z >= baseZ && z - baseZ < (int)game.size() ? game[(size_t)(z - baseZ)] : B{};
                worldgen::Block m = z >= t.base && z < top ? t.blocks[(size_t)(z - t.base)] : worldgen::Block{};
                bool eq = g.r == m.r && g.g == m.g && g.b == m.b && g.type == m.type && g.flags == m.flags;
                bool gs = !terrainType(g.type), ms = !terrainType(m.type);
                structGame += gs, structOurs += ms, structSame += gs && eq;
                bool lightOnly = g.g == m.g && g.b == m.b && g.type == m.type && g.flags == m.flags && (g.type == 0 || g.type == 2);
                if (eq || lightOnly)
                    continue;
                int object = worldgen::cellObject(worldgen::floorDiv(x, 4), worldgen::floorDiv(y, 4));
                differingBy[object]++;
                if (!object)
                    objectlessByZone[{ax, ay}]++;
                {
                    const bool gAir = !(g.type & 0xfd), mAir = !(m.type & 0xfd);
                    int how = (g.flags ^ m.flags) == 0x40 && g.type == m.type && g.r == m.r && g.g == m.g && g.b == m.b ? 0
                              : gAir && mAir                                                                   ? 3
                              : mAir                                                                           ? 1
                              : gAir                                                                           ? 2
                                                                                                               : 4;
                    static const char *hows[5] = {"flag 0x40", "missing", "extra", "air", "other"};
                    // A few of each, spread over the run (the 1st, 1000th, 100000th... of a kind).
                    int n = differingHow[object][how]++;
                    if (verbose && (n < 2 || n == 1000 || n == 30000 || n == 300000))
                        std::printf("  object %d %s, %s %d %d z %d: game %d %d %d %d %d, CubeVulkan %d %d %d %d %d\n", object, hows[how],
                                    place ? "place" : "plain", x, y, z, g.r, g.g, g.b, g.type, g.flags, m.r, m.g, m.b, m.type, m.flags);
                }
            }
            if (skipped)
                continue;
            built += extra;
            exact += same && !extra;
            if (verbose && !same && !extra) { // what the game's top is, against the heights
                int gameTop = INT32_MIN;
                for (size_t i = 0; i < game.size(); i++)
                    if (game[i].type != 0)
                        gameTop = baseZ + (int)i;
                int h0 = (int)std::floor(worldgen::groundHeight(x, y, 0)), h4 = (int)std::floor(worldgen::groundHeight(x, y, 4));
                topStats[gameTop == h0 ? 0 : gameTop == h4 ? 1 : gameTop < h0 ? 2 : 3]++;
                zoneDiffers++;
                lightDiffers += onlyLight;
            }
        }
        if (verbose && zoneDiffers > 200)
            std::printf("  zone %d %d: %d plain columns differ\n", ax, ay, zoneDiffers);
    }
    std::printf("zones %d, columns %d: %d exact, %d with structures, %d in places; blocks %d of %d the same\n",
                zones - (zones > maxZones), columns, exact, built, inPlaces, blocksSame, blocks);
    int plain = columns - built - inPlaces;
    if (verbose)
        std::printf("differing plain columns, game top at: own height %d, cliff-step height %d, lower %d, higher %d\n",
                    topStats[0], topStats[1], topStats[2], topStats[3]);
    if (verbose)
        std::printf("differing plain columns that differ only in the light of air and water: %d\n", lightDiffers);
    std::printf("structure blocks: game %d, CubeVulkan %d, the same %d\n", structGame, structOurs, structSame);
    if (verbose)
        for (auto [k, n] : differingBy)
            std::printf("  differing blocks in cells of object %d: %d (flag 0x40 %d, missing %d, extra %d, air %d, other %d)\n", k, n,
                        differingHow[k][0], differingHow[k][1], differingHow[k][2], differingHow[k][3], differingHow[k][4]);
    if (verbose) {
        std::vector<std::pair<int, std::pair<int, int>>> worst;
        for (auto [z, n] : objectlessByZone)
            worst.push_back({n, z});
        std::sort(worst.rbegin(), worst.rend());
        for (size_t k = 0; k < worst.size() && k < 25; k++) {
            int32_t c1x = worldgen::floorDiv(worst[k].second.first * 64, 10), c1y = worldgen::floorDiv(worst[k].second.second * 64, 10);
            bool big = worldgen::bigCell(c1x, c1y);
            std::printf("  zone %d %d: %d differing blocks in cells without an object (place kind %d%s)\n", worst[k].second.first,
                        worst[k].second.second, worst[k].first, worldgen::placeKind(c1x, c1y, big), big ? ", big" : "");
        }
    }
    std::printf("zone region types: %d wrong\n", regionsWrong);
    if (verbose)
        for (auto [k, n] : placeKinds)
            std::printf("place kind %d: %d columns\n", k, n);
    CHECK(regionsWrong == 0);
    // Over all 7615 vanilla zones (2026-10-23), 99.95% of plain columns are exact. The rest
    // are the light near the game's rand()-placed overhang lights, and the layouts of
    // places reaching past their radius (structures, not generated yet).
    CHECK(plain > 0 && exact >= plain * 0.999);
    return plain - exact;
}

// Cell objects (trees, rocks, houses...) against the game's 0x69c60 (ZoneExtractor key O
// writes Extract/gen-cells.txt for the TEST world, seed 123).
static int genCellsMatchGame(bool verbose) {
    Assets a;
    if (!a.locate(""))
        return 0;
    std::ifstream in(a.gameDir() / "Extract" / "gen-cells.txt");
    if (!in)
        return 0;
    worldgen::setWorldSeed(123);
    std::map<std::pair<int, int>, int> wrongByType;
    int cells = 0, wrong = 0, shown = 0;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        std::istringstream s(line);
        int cx, cy, game;
        if (!(s >> cx >> cy >> game))
            continue;
        cells++;
        int mine = worldgen::cellObject(cx, cy);
        if (mine != game) {
            wrong++;
            wrongByType[{game, mine}]++;
            if (verbose && shown < 30)
                std::printf("  cell %d %d: game %d, CubeVulkan %d\n", cx, cy, game, mine), shown++;
        }
    }
    if (verbose)
        for (auto &[k, n] : wrongByType)
            std::printf("  game %d / CubeVulkan %d: %d cells\n", k.first, k.second, n);
    std::printf("cell objects: %d of %d differ\n", wrong, cells);
    CHECK(cells > 0 && wrong == 0);
    return wrong;
}

// [World] Scale: at 1.5 a square of land holds about 1.5^2 = 2.25x fewer regions (regions,
// landmasses and oceans are wider); at 1 the game's world is unchanged.
static void worldScaleWidensRegions() {
    auto regions = [] {
        std::set<std::pair<int, int>> seen;
        for (int y = -150000; y <= 150000; y += 1024)
            for (int x = -150000; x <= 150000; x += 1024) {
                worldgen::Int2 r = worldgen::regionOf(x, y);
                seen.insert({r.x, r.y});
            }
        return (int)seen.size();
    };
    // The zone seed the game derives from world seed 123 (read from world+0x244 in game).
    worldgen::setWorldSeed(123);
    CHECK(worldgen::currentWorld().zoneSeed() == 15376);
    worldgen::setWorldSeed(0);
    float before = worldgen::groundHeight(5000, -7000);
    int normal = regions();
    worldgen::setWorldScale(1.5);
    int wide = regions();
    worldgen::setWorldScale(1.0);
    float ratio = (float)normal / (float)std::max(wide, 1);
    std::printf("world scale 1.5: %d regions in the square against %d (%.2fx fewer)\n", wide, normal, ratio);
    CHECK(ratio > 2.0f && ratio < 2.5f);
    CHECK(worldgen::groundHeight(5000, -7000) == before);
    CHECK(worldgen::worldScale() == 1.0); // the default is the game's

    // Mods' hooks (worldgen/Options.h): a place kind replaced, the game's own still asked for
    // under OriginalGeneration and inside the hook, and the zones it changes told apart.
    int32_t px = 3, py = 7;
    while (worldgen::placeKind(px, py, worldgen::bigCell(px, py)) == 0)
        px++;
    const bool big = worldgen::bigCell(px, py);
    const int game = worldgen::placeKind(px, py, big);
    int seenInHook = -1;
    worldgen::ModHooks hooks;
    hooks.placeKind = [&](int32_t cx, int32_t cy, bool b, int kind) {
        if (cx != px || cy != py || b != big)
            return kind;
        seenInHook = worldgen::placeKind(cx, cy, b); // the game's own while the hook runs
        return 43;
    };
    worldgen::setModHooks(hooks);
    CHECK(worldgen::placeKind(px, py, big) == 43 && seenInHook == game);
    {
        worldgen::OriginalGeneration original;
        CHECK(worldgen::placeKind(px, py, big) == game);
    }
    const worldgen::Int2 point = worldgen::cellPoint(px, py, big);
    CHECK(worldgen::hooksChange(worldgen::floorDiv(point.x, 6), worldgen::floorDiv(point.y, 6)));
    CHECK(!worldgen::hooksChange(worldgen::floorDiv(point.x, 6) + 2048, worldgen::floorDiv(point.y, 6)));
    worldgen::setModHooks({});
    CHECK(worldgen::placeKind(px, py, big) == game && !worldgen::hooksChange(worldgen::floorDiv(point.x, 6), worldgen::floorDiv(point.y, 6)));
}

// The world map's region model (docs/specs/worldmap-model.md): its sizes, and its markers in the
// order and with the kinds the spec gives, around a town. (No recording of the game's map exists
// yet; `EngineTests mapdump` prints what a recording would be compared with.)
static void worldMapModel() {
    namespace wg = worldgen;
    wg::setWorldSeed(0);
    // §1: units and reach by level.
    const int units[6] = {4, 8, 16, 32, 64, 128}, reaches[6] = {2, 2, 2, 1, 1, 1};
    for (int level = 0; level < 6; level++)
        CHECK(wg::mapUnit(level) == units[level] && wg::mapReach(units[level]) == reaches[level]);
    auto regionOf = [](int32_t x, int level) { return wg::floorDivBy(x, 16 * wg::mapUnit(level)); };
    // The place list's groups in order (§6): shrines, then lore buildings and treasure cells,
    // then a town's buildings; then the missions, lands and flight points.
    auto group = [](const wg::MapRegion::Marker &k) {
        if (k.list != 0)
            return 2 + k.list;
        return k.type == 12 ? 0 : (k.type == 14 || k.type == 15) ? 1 : 2;
    };
    auto wellFormed = [&](const wg::MapRegion &m) {
        const int32_t B = 16 * m.unit;
        bool ok = m.side == 22 && m.sizeZ >= 1 && m.sizeZ <= 255 && m.rgb.size() == (size_t)22 * 22 * m.sizeZ * 3 &&
                  m.base() == m.lowest / m.unit;
        for (size_t i = 0; i < m.markers.size(); i++) {
            const wg::MapRegion::Marker &k = m.markers[i];
            ok = ok && k.x >= B * m.rx && k.x < B * (m.rx + 1) && k.y >= B * m.ry && k.y < B * (m.ry + 1);
            ok = ok && (i == 0 || group(m.markers[i - 1]) <= group(k));
        }
        return ok;
    };
    // A town near the origin, and the region (level 2) around its landmark point.
    wg::Int2 town{}, townCell{};
    bool found = false;
    for (int32_t px = -40; px <= 40 && !found; px++)
        for (int32_t py = -40; py <= 40 && !found; py++) {
            const bool big = wg::bigCell(px, py);
            const int32_t qx = big ? 2 * wg::floorDiv(px, 1) : px, qy = big ? 2 * wg::floorDiv(py, 1) : py;
            if (wg::placeKind(qx, qy, big) == 5)
                town = wg::cellPoint(qx, qy, big), townCell = {qx, qy}, found = true;
        }
    CHECK(found);
    const int32_t rx = regionOf(town.x, 2), ry = regionOf(town.y, 2);
    const wg::MapRegion m = wg::mapRegion(rx, ry, 2);
    CHECK(wellFormed(m) && m.unit == 16 && m.reach == 2);
    // §6.1: the town's own mission marker at its landmark, named by its landmark kind.
    int missions = 0;
    for (const wg::MapRegion::Marker &k : m.markers)
        if (k.list == 1 && k.x == town.x && k.y == town.y) {
            missions++;
            CHECK(k.type == 5 && k.px == townCell.x && k.py == townCell.y && k.z == (int32_t)wg::groundHeight(town.x, town.y, 0));
        }
    CHECK(missions == 1);
    // §6.6: the town's buildings, one per block corner with an inn or a shop, in cell order, at
    // the cell's centre 20 blocks over the ground.
    std::vector<std::array<int32_t, 4>> expected;
    for (int32_t cx = 16 * rx; cx < 16 * (rx + 1); cx++)
        for (int32_t cy = 16 * ry; cy < 16 * (ry + 1); cy++) {
            const int t = wg::cellObject(cx, cy);
            if (!(t >= 22 && t <= 26) && t != 91 && t != 92)
                continue;
            if (wg::blockCorner(wg::floorDivBy(cx, 3), wg::floorDivBy(cy, 3)) != wg::Int2{cx, cy})
                continue;
            const int32_t zx = wg::floorDivBy(16 * cx, 1024), zy = wg::floorDivBy(16 * cy, 1024);
            const int plot = wg::townBlock(wg::currentWorld(), cx, cy, wg::placeKind(zx, zy, wg::bigCell(zx, zy)));
            if (plot > 1 && plot != 13)
                expected.push_back({plot, 16 * cx + 8, 16 * cy + 8, (int32_t)(wg::groundHeight(16 * cx + 8, 16 * cy + 8, 0) + 20.0f)});
        }
    std::vector<std::array<int32_t, 4>> got;
    for (const wg::MapRegion::Marker &k : m.markers)
        if (k.list == 0 && group(k) == 2)
            got.push_back({k.type, k.x, k.y, k.z});
    CHECK(!expected.empty() && got == expected);
    for (const auto &e : got)
        CHECK(e[0] >= 2 && e[0] <= 11);
    // §6.3: the flight point of the 2048-block square at the origin; none once a unit exceeds 256.
    const uint32_t key = wg::cellKey(0, 0);
    const int32_t fx = (int32_t)(wg::random(key + 0x474d6) & 2047u), fy = (int32_t)(wg::random(key + 0x181f0) & 2047u);
    for (int level : {2, 7}) {
        const wg::MapRegion f = wg::mapRegion(regionOf(fx, level), regionOf(fy, level), level);
        int points = 0;
        for (const wg::MapRegion::Marker &k : f.markers)
            points += k.list == 3 && k.x == fx && k.y == fy;
        CHECK(wellFormed(f) && points == (level == 2 ? 1 : 0));
    }
    // §6.2: a land's name at its region's centre, from unit 128 (level 5).
    const wg::Int2 centre = wg::regionCentre(0, 0);
    for (int level : {4, 5}) {
        const wg::MapRegion l = wg::mapRegion(regionOf(centre.x, level), regionOf(centre.y, level), level);
        int lands = 0;
        for (const wg::MapRegion::Marker &k : l.markers)
            if (k.list == 2) {
                lands++;
                CHECK(k.x == centre.x && k.y == centre.y && k.type == wg::regionBiome(0, 0));
            }
        CHECK(wellFormed(l) && lands == (level == 5 ? 1 : 0));
        // Places at unit 128: shrines only (no lore, treasure or town buildings past unit 64).
        if (level == 5)
            for (const wg::MapRegion::Marker &k : l.markers)
                CHECK(k.list != 0 || k.type == 12);
    }
}

// An extracted zone's columns, game blocks from baseZ up (empty if not extracted).
struct GameColumn {
    int32_t baseZ = 0;
    std::vector<worldgen::Block> blocks;
    worldgen::Block at(int32_t z) const {
        return z >= baseZ && z - baseZ < (int)blocks.size() ? blocks[(size_t)(z - baseZ)] : worldgen::Block{};
    }
};
static std::vector<GameColumn> readGameZone(int32_t zx, int32_t zy) {
    Assets a;
    std::vector<GameColumn> out;
    if (!a.locate(""))
        return out;
    std::filesystem::path dir = zonesDir(a.gameDir());
    std::ifstream in(dir / ("z_" + std::to_string(zx) + "_" + std::to_string(zy) + ".cwz"), std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), {});
    if (data.size() < 20)
        return out;
    out.resize(4096);
    size_t at = 20;
    for (int col = 0; col < 4096; col++) {
        uint32_t runs;
        std::memcpy(&out[col].baseZ, &data[at], 4), std::memcpy(&runs, &data[at + 4], 4);
        at += 8;
        for (uint32_t r = 0; r < runs; r++) {
            uint16_t n;
            worldgen::Block b;
            std::memcpy(&n, &data[at], 2), std::memcpy(&b, &data[at + 2], 6);
            at += 8;
            out[col].blocks.insert(out[col].blocks.end(), n, b);
        }
    }
    return out;
}

// The game's static objects of an extracted zone (the .rec file's zone+0x60 records).
static std::vector<worldgen::StaticObject> readGameObjects(int32_t zx, int32_t zy, bool &found) {
    Assets a;
    std::vector<worldgen::StaticObject> out;
    found = false;
    if (!a.locate(""))
        return out;
    std::filesystem::path dir = zonesDir(a.gameDir());
    std::ifstream in(dir / ("z_" + std::to_string(zx) + "_" + std::to_string(zy) + ".rec"), std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), {});
    if (data.size() < 4 || std::memcmp(data.data(), "CWR1", 4) != 0)
        return out;
    found = true;
    size_t at = 4;
    while (at + 12 <= data.size()) {
        uint32_t offset, stride, count;
        std::memcpy(&offset, &data[at], 4), std::memcpy(&stride, &data[at + 4], 4), std::memcpy(&count, &data[at + 8], 4);
        at += 12;
        if (!stride || count > (data.size() - at) / stride)
            break;
        if (offset == 0x60 && stride == 0xe0)
            for (uint32_t i = 0; i < count; i++) {
                const uint8_t *r = &data[at + (size_t)i * stride];
                worldgen::StaticObject o;
                std::memcpy(&o.type, r, 4), std::memcpy(&o.x, r + 8, 8), std::memcpy(&o.y, r + 0x10, 8), std::memcpy(&o.z, r + 0x18, 8);
                o.turn = r[0x20], std::memcpy(&o.scale, r + 0x24, 4);
                out.push_back(o);
            }
        at += (size_t)stride * count;
    }
    return out;
}
// The objects of one list that the other lacks (as a multiset).
static std::vector<worldgen::StaticObject> objectsMissing(const std::vector<worldgen::StaticObject> &from,
                                                         const std::vector<worldgen::StaticObject> &in) {
    auto key = [](const worldgen::StaticObject &o) {
        uint32_t s;
        std::memcpy(&s, &o.scale, 4);
        return std::make_tuple(o.type, o.x, o.y, o.z, (int)o.turn, s);
    };
    std::multiset<decltype(key(from[0]))> have;
    for (const auto &o : in)
        have.insert(key(o));
    std::vector<worldgen::StaticObject> out;
    for (const auto &o : from) {
        auto it = have.find(key(o));
        if (it == have.end())
            out.push_back(o);
        else
            have.erase(it);
    }
    return out;
}
static void printObject(const char *who, const worldgen::StaticObject &o) {
    std::printf("  %s: type %d at %.3f %.3f %.3f turn %d scale %g\n", who, o.type, o.x / 65536.0, o.y / 65536.0, o.z / 65536.0, o.turn, o.scale);
}

// A zone's models (zone+0x18) against the extracted ones: per type, the game's count, ours,
// how many stand at the same place and how many of those have the same two floats too.
static bool compareModels(int32_t zx, int32_t zy, std::map<int, std::array<int, 4>> &byType, int show) {
    Assets a;
    if (!a.locate(""))
        return false;
    std::filesystem::path dir = zonesDir(a.gameDir());
    std::ifstream in(dir / ("z_" + std::to_string(zx) + "_" + std::to_string(zy) + ".rec"), std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), {});
    if (data.size() < 4)
        return false;
    using Key = std::tuple<int, int64_t, int64_t, int64_t>;
    std::multiset<Key> game, ours;
    std::map<Key, std::pair<float, float>> gameFloats, ourFloats;
    for (size_t at = 4; at + 12 <= data.size();) {
        uint32_t offset, stride, count;
        std::memcpy(&offset, &data[at], 4), std::memcpy(&stride, &data[at + 4], 4), std::memcpy(&count, &data[at + 8], 4);
        at += 12;
        if (offset == 0x18 && stride == 0x40)
            for (uint32_t i = 0; i < count; i++) {
                const uint8_t *r = &data[at + (size_t)i * stride];
                int t;
                int64_t p[3];
                float f[2];
                std::memcpy(&t, r, 4), std::memcpy(p, r + 8, 24), std::memcpy(f, r + 0x20, 8);
                game.insert({t, p[0], p[1], p[2]});
                gameFloats[{t, p[0], p[1], p[2]}] = {f[0], f[1]};
            }
        at += (size_t)stride * count;
    }
    for (const worldgen::Model &m : worldgen::zone(zx, zy)->models)
        ours.insert({m.type, m.x, m.y, m.z}), ourFloats[{m.type, m.x, m.y, m.z}] = {m.size, m.yaw};
    for (auto &k : game)
        byType[std::get<0>(k)][0]++;
    for (auto &k : ours)
        byType[std::get<0>(k)][1]++;
    int shown = 0;
    for (auto &k : game)
        if (ours.count(k)) {
            byType[std::get<0>(k)][2]++;
            if (gameFloats[k] == ourFloats[k])
                byType[std::get<0>(k)][3]++;
            else if (shown++ < show)
                std::printf("  type %d at %.3f %.3f %.3f: game %g %g, ours %g %g\n", std::get<0>(k), std::get<1>(k) / 65536.0,
                            std::get<2>(k) / 65536.0, std::get<3>(k) / 65536.0, gameFloats[k].first, gameFloats[k].second,
                            ourFloats[k].first, ourFloats[k].second);
        }
    if (show)
        for (int side = 0; side < 2; side++)
            for (auto &k : side ? ours : game)
                if (!(side ? game : ours).count(k) && shown++ < show * 4)
                    std::printf("  %s only: type %d at %.3f %.3f %.3f\n", side ? "ours" : "game", std::get<0>(k), std::get<1>(k) / 65536.0,
                                std::get<2>(k) / 65536.0, std::get<3>(k) / 65536.0);
    return true;
}

// The oracle's scripted rand(): xorshift32 of the record's seed, 15 bits.
struct ScriptedRand {
    uint32_t state;
    int calls = 0;
    explicit ScriptedRand(uint32_t seed) : state(seed ? seed : 0x9e3779b9u) {}
    int operator()() {
        state ^= state << 13, state ^= state >> 17, state ^= state << 5;
        calls++;
        return (int)(state & 0x7fff);
    }
};

// Extract/oracle/gear.bin (tools/ZoneExtractor): the game's 0x511e0 on scripted rand()
// against worldgen::makeGear.
static bool gearOracle(const fs::path &file, int show) {
    std::ifstream in(file, std::ios::binary);
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
    if (b.size() < 4 || memcmp(b.data(), "CWG2", 4)) {
        std::printf("no gear oracle at %s\n", file.string().c_str());
        return false;
    }
    size_t at = 4;
    auto i32 = [&](size_t o) { int32_t v; memcpy(&v, &b[o], 4); return v; };
    int records = 0, wrong = 0;
    std::map<std::string, int> fields;
    while (at < b.size()) {
        const int host = i32(at), race = i32(at + 4), level = i32(at + 8), cult = i32(at + 12);
        const uint32_t flags = (uint32_t)i32(at + 16);
        const uint8_t clsIn = b[at + 20];
        const uint32_t seed = (uint32_t)i32(at + 21), calls = (uint32_t)i32(at + 25);
        const uint8_t cls = b[at + 29], spec = b[at + 30];
        const uint8_t *eq = &b[at + 31];
        at += 31 + 0x6e0;
        auto ints = [&] {
            std::vector<int> v((size_t)i32(at));
            for (size_t k = 0; k < v.size(); k++)
                v[k] = i32(at + 4 + 4 * k);
            at += 4 + 4 * v.size();
            return v;
        };
        const std::vector<int> v1 = ints();
        const int pick = i32(at);
        at += 4;
        const std::vector<int> v2 = ints();
        std::vector<std::array<int, 4>> inv; // tab, quantity, category, id
        const int tabs = i32(at);
        at += 4;
        for (int t = 0; t < tabs; t++) {
            const int bytes = i32(at);
            at += 4;
            for (int s = 0; s + 0xa4 <= bytes; s += 0xa4)
                inv.push_back({t, i32(at + s), b[at + s + 4], i32(at + s + 8)});
            at += bytes;
        }
        records++;
        ScriptedRand r(seed);
        worldgen::Gear g;
        g.cls = clsIn;
        worldgen::makeGear({host, race, level, flags, cult}, worldgen::Draw(r), g);
        std::vector<std::string> bad;
        if ((uint32_t)r.calls != calls)
            bad.push_back("calls " + std::to_string(r.calls) + "/" + std::to_string(calls));
        if (g.cls != cls || g.spec != spec)
            bad.push_back("class " + std::to_string(g.cls) + "." + std::to_string(g.spec) + "/" + std::to_string(cls) + "." +
                          std::to_string(spec));
        for (int s = 0; s < 11; s++) {
            const uint8_t *it = eq + s * 0xa0;
            const auto &m = g.equipment[s];
            int32_t id, mod;
            memcpy(&id, it + 4, 4), memcpy(&mod, it + 8, 4);
            if (m.category != it[0] || m.id != id || m.modifier != (uint32_t)mod || m.rarity != it[0x14] || m.material != it[0x1c])
                bad.push_back("slot " + std::to_string(s) + " " + std::to_string(m.category) + "/" + std::to_string(m.id) + "/" +
                              std::to_string(m.material) + "/" + std::to_string(m.rarity) + (m.modifier != (uint32_t)mod ? "/mod" : "") +
                              " vs " + std::to_string(it[0]) + "/" + std::to_string(id) + "/" + std::to_string(it[0x1c]) + "/" +
                              std::to_string(it[0x14]));
        }
        if (g.abilities != v1 || g.abilities2 != v2 || g.abilityPick != pick)
            bad.push_back("abilities");
        std::vector<std::array<int, 4>> ours;
        for (size_t t = 0; t < g.inventory.tabs.size(); t++)
            for (const worldgen::Stack &s : g.inventory.tabs[t])
                ours.push_back({(int)t, s.quantity, s.item.category, s.item.id});
        if (ours != inv)
            bad.push_back("inventory");
        if (!bad.empty()) {
            for (auto &f : bad)
                fields[f.substr(0, f.find(' '))]++;
            if (wrong++ < show) {
                std::printf("host %d race %d (0x%x) level %d people %d flags %x class %d:", host, race, race, level, cult, flags, clsIn);
                for (auto &f : bad)
                    std::printf(" [%s]", f.c_str());
                std::printf("\n");
            }
        }
    }
    std::printf("%d of %d gear records match\n", records - wrong, records);
    for (auto &[f, n] : fields)
        std::printf("  %s: %d\n", f.c_str(), n);
    return wrong == 0;
}

// The oracle's spawn points (Extract/oracle/z_<x>_<y>.spawns, tools/ZoneExtractor) against
// the generator's (worldgen::zone), zone by zone.
static void allSpawns(const fs::path &oracle, int show) {
    std::string why;
    if (!worldgen::loadCreatureTables(oracle, &why)) {
        std::printf("%s\n", why.c_str());
        return;
    }
    struct Game {
        int64_t x, y, z;
        float yaw;
        int host, race, level, people, style, roam, mode;
        std::array<int32_t, 2> home;
        uint32_t flags;
        uint8_t cls, spec;
        std::vector<uint8_t> equip;
        std::vector<uint8_t> appearance; // +0x50 (0xb0 bytes; +8 is the flags)
    };
    std::map<int, std::array<int, 4>> byHost; // game, ours, same place, all the same
    std::map<std::string, int> wrongFields;
    int zones = 0, shown = 0;
    for (auto &e : fs::directory_iterator(oracle)) {
        int zx, zy;
        if (e.path().extension() != ".spawns" || std::sscanf(e.path().filename().string().c_str(), "z_%d_%d", &zx, &zy) != 2)
            continue;
        std::ifstream in(e.path(), std::ios::binary);
        std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
        uint32_t n = 0;
        if (b.size() >= 8)
            memcpy(&n, &b[4], 4);
        std::vector<Game> game;
        size_t at = 8;
        for (uint32_t i = 0; i < n && at + 0x950 <= b.size(); i++) {
            const uint8_t *r = &b[at];
            Game g;
            memcpy(&g.x, r, 8), memcpy(&g.y, r + 8, 8), memcpy(&g.z, r + 16, 8), memcpy(&g.yaw, r + 0x18, 4);
            memcpy(&g.host, r + 0x1c, 4), memcpy(&g.race, r + 0x20, 4), memcpy(&g.level, r + 0x24, 4);
            memcpy(&g.people, r + 0x28, 4), memcpy(&g.style, r + 0x30, 4), memcpy(&g.flags, r + 0x58, 4);
            memcpy(&g.roam, r + 0x8b8, 4), memcpy(&g.mode, r + 0x8bc, 4), memcpy(g.home.data(), r + 0x8f8, 8);
            g.cls = r[0x34], g.spec = r[0x35];
            g.equip.assign(r + 0x110, r + 0x110 + 0x6e0);
            g.appearance.assign(r + 0x50, r + 0x100);
            at += 0x950;
            for (int v = 0; v < 2; v++) {
                uint32_t k;
                memcpy(&k, &b[at], 4);
                at += 4 + 4 * (size_t)k;
            }
            game.push_back(std::move(g));
        }
        if (game.empty())
            continue;
        zones++;
        std::vector<worldgen::SpawnPoint> ours = zoneSpawnList(zx, zy);
        std::vector<bool> used(ours.size());
        for (auto &g : game)
            byHost[g.host][0]++;
        for (auto &o : ours)
            byHost[o.hostility][1]++;
        for (auto &g : game) {
            size_t best = ours.size();
            for (size_t i = 0; i < ours.size(); i++)
                if (!used[i] && ours[i].x == g.x && ours[i].y == g.y && ours[i].z == g.z && ours[i].race == g.race) {
                    best = i;
                    break;
                }
            if (best == ours.size()) {
                if (shown++ < show)
                    std::printf("zone %d %d: game only host %d race %d at %.2f %.2f %.2f level %d\n", zx, zy, g.host, g.race, g.x / 65536.0,
                                g.y / 65536.0, g.z / 65536.0, g.level);
                continue;
            }
            used[best] = true;
            const worldgen::SpawnPoint &o = ours[best];
            byHost[g.host][2]++;
            std::vector<std::string> bad;
            if (o.hostility != g.host)
                bad.push_back("hostility");
            if (o.level != g.level)
                bad.push_back("level");
            if (o.flags() != g.flags)
                bad.push_back("flags");
            if (o.people != g.people || (int)o.style != g.style)
                bad.push_back("people");
            if ((int)o.roam != g.roam)
                bad.push_back("roam");
            if (o.mode != g.mode)
                bad.push_back("mode");
            if (o.home.x != g.home[0] || o.home.y != g.home[1])
                bad.push_back("home");
            if (o.yaw != g.yaw)
                bad.push_back("yaw");
            if (o.gear.cls != g.cls || o.gear.spec != g.spec)
                bad.push_back("class");
            {
                std::array<uint8_t, 0xb0> mine = o.appearance.bytes;
                std::memcpy(&mine[5], &g.appearance[5], 3); // padding: whatever the game's memory held
                if (memcmp(mine.data(), g.appearance.data(), mine.size()))
                    bad.push_back("appearance");
            }
            for (int s = 0; s < 11; s++) {
                const uint8_t *it = &g.equip[s * 0xa0];
                int32_t id;
                uint32_t mod;
                memcpy(&id, it + 4, 4), memcpy(&mod, it + 8, 4);
                const auto &m = o.gear.equipment[s];
                if (m.category != it[0] || m.id != id || m.modifier != mod || m.material != it[0x1c] || m.rarity != it[0x14]) {
                    bad.push_back("equipment");
                    break;
                }
            }
            if (bad.empty())
                byHost[g.host][3]++;
            else {
                for (auto &f : bad)
                    wrongFields[f]++;
                if (shown++ < show) {
                    std::printf("zone %d %d: host %d race %d level %d/%d flags %x/%x roam %d/%d yaw %.3f/%.3f class %d.%d/%d.%d:", zx, zy, g.host,
                                g.race, o.level, g.level, o.flags(), g.flags, (int)o.roam, g.roam, o.yaw, g.yaw, o.gear.cls, o.gear.spec, g.cls, g.spec);
                    for (auto &f : bad)
                        std::printf(" %s", f.c_str());
                    std::printf("\n");
                }
            }
        }
        for (size_t i = 0; i < ours.size(); i++)
            if (!used[i] && shown++ < show)
                std::printf("zone %d %d: ours only host %d race %d at %.2f %.2f %.2f\n", zx, zy, ours[i].hostility, ours[i].race,
                            ours[i].x / 65536.0, ours[i].y / 65536.0, ours[i].z / 65536.0);
    }
    std::printf("%d zones with spawn points\nhostility: game, ours, same place, all the same\n", zones);
    for (auto &[h, c] : byHost)
        std::printf("  %d: %d %d %d %d\n", h, c[0], c[1], c[2], c[3]);
    for (auto &[f, c] : wrongFields)
        std::printf("  wrong %s: %d\n", f.c_str(), c);
}

// Extract/oracle/regions.txt (the game's 0x115df0) against worldgen::regionRoles.
static void rolesOracle(const fs::path &file, int show) {
    std::ifstream in(file);
    struct Game {
        std::map<std::pair<int, int>, int> role, level;
        worldgen::Int2 special;
        int marked = 0;
        std::vector<int> items;
    };
    std::map<std::pair<int, int>, Game> regions;
    Game *g = nullptr;
    for (std::string line; std::getline(in, line);) {
        std::istringstream l(line);
        std::string what;
        l >> what;
        if (what == "region") {
            int x, y;
            l >> x >> y;
            g = std::abs(x) < 1000 && std::abs(y) < 1000 ? &regions[{x, y}] : nullptr;
        } else if (!g)
            continue;
        else if (what == "map") {
            std::string off;
            int x, y, v;
            l >> off >> x >> y >> v;
            if (off == "10")
                g->role[{x, y}] = v;
            else if (off == "60")
                g->level[{x, y}] = v;
        } else if (what == "special")
            l >> g->special.x >> g->special.y;
        else if (what == "marked")
            l >> g->marked;
        else if (what == "items")
            for (int v; l >> v;)
                g->items.push_back(v);
    }
    int shown = 0;
    for (auto &[r, game] : regions) {
        const worldgen::RegionRoles ours = worldgen::regionRoles({r.first, r.second});
        std::map<std::pair<int, int>, int> role, level;
        for (auto &e : ours.roles)
            role[{e[0], e[1]}] = e[2];
        for (auto &e : ours.levels)
            level[{e[0], e[1]}] = e[2];
        int sameRole = 0, sameLevel = 0;
        for (auto &[k, v] : game.role) {
            auto it = role.find(k);
            if (it != role.end() && it->second == v)
                sameRole++;
            else if (shown++ < show)
                std::printf("region %d %d place %d %d: role game %d ours %d\n", r.first, r.second, k.first, k.second, v,
                            it == role.end() ? -1 : it->second);
        }
        for (auto &[k, v] : game.level) {
            auto it = level.find(k);
            if (it != level.end() && it->second == v)
                sameLevel++;
            else if (shown++ < show)
                std::printf("region %d %d place %d %d: level game %d ours %d\n", r.first, r.second, k.first, k.second, v,
                            it == level.end() ? -1 : it->second);
        }
        std::printf("region %d %d: roles %d/%d (ours %zu), levels %d/%d (ours %zu), special %s, marked %s, items %s\n", r.first,
                    r.second, sameRole, (int)game.role.size(), role.size(), sameLevel, (int)game.level.size(), level.size(),
                    game.special == ours.special ? "same" : "differ", (game.marked != 0) == ours.marked ? "same" : "differ",
                    game.items == ours.items ? "same" : "differ");
    }
}


// Extract/oracle/anim-*.bin (ZoneExtractor's animation oracle, CWA2 or CWA3): every call of the game's
// creature pose 0x17ab0 with the creature as the call found it. Our pose is run on each
// record and its result compared with the same creature's next record, which holds what the
// game's call made: the eased pose fields (a+0x20..0xd0) and the 33 part matrices.
struct AnimRecord {
    uint64_t address = 0;
    std::vector<uint8_t> creature;
    nativepose::Call call;
    // The rule values the game computed for the call (stat-rules.md), checked by rulesOracle.
    int abilityTime = 0, abilityLength = 0, abilityLength2 = 0, abilityLength3 = 0, useTime = 0;
    bool buffE = false;
    std::array<bool, 9> usable{};
    std::array<uint64_t, 9> items{}; // the item slots' models (game addresses)
    std::map<int, uint64_t> ids;     // the model ids' models
    std::map<uint64_t, nativepose::Model> models; // by game address, so equal models compare equal
    std::vector<uint8_t> mount;                   // the creature ridden (its first 0x200 bytes), if any
};
static const int ANIM_ITEM_SLOTS[9] = {0x1d8, 0x278, 0x318, 0x3b8, 0x458, 0x4f8, 0x598, 0x638, 0x6d8};
static std::vector<AnimRecord> readAnimRecords(const std::filesystem::path &path) {
    std::vector<AnimRecord> out;
    std::ifstream in(path, std::ios::binary);
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
    if (b.size() < 4 || (std::memcmp(b.data(), "CWA2", 4) && std::memcmp(b.data(), "CWA3", 4)))
        return out;
    const bool v3 = std::memcmp(b.data(), "CWA3", 4) == 0;
    size_t at = 4;
    auto get = [&](size_t &p, void *to, size_t n) { std::memcpy(to, &b[p], n), p += n; };
    while (at + 4 <= b.size()) {
        uint32_t n;
        std::memcpy(&n, &b[at], 4);
        at += 4;
        if (at + n > b.size())
            break;
        size_t p = at;
        at += n;
        AnimRecord r;
        get(p, &r.address, 8);
        r.creature.assign(&b[p], &b[p] + nativepose::CREATURE_SIZE);
        p += nativepose::CREATURE_SIZE;
        get(p, &r.call.dt, 4);
        uint64_t stack[8];
        get(p, stack, 64);
        std::memcpy(&r.call.alpha, &stack[1], 4);
        r.call.hide = (uint32_t)stack[4];
        std::memcpy(&r.call.depth, &stack[6], 4);
        std::memcpy(&r.call.scale, &stack[7], 4);
        get(p, r.call.tint, 16);
        get(p, &r.call.cameraX, 8);
        get(p, &r.call.cameraY, 8);
        if (v3) {
            uint8_t hasMount;
            get(p, &hasMount, 1);
            r.mount.assign(&b[p], &b[p] + 0x200);
            p += 0x200;
            if (!hasMount)
                r.mount.clear();
        } else
            p += 8;
        get(p, &r.call.nowMs, 4);
        get(p, &r.abilityTime, 4);
        get(p, &r.abilityLength, 4);
        get(p, &r.abilityLength2, 4);
        if (v3)
            get(p, &r.abilityLength3, 4);
        get(p, &r.useTime, 4);
        uint8_t buff;
        get(p, &buff, 1);
        r.buffE = buff != 0;
        p += 3;
        for (int k = 0; k < 9; k++) {
            uint8_t u;
            get(p, &u, 1);
            p += 7;
            r.usable[k] = u != 0;
            uint64_t addr;
            get(p, &addr, 8);
            r.items[k] = addr;
            if (addr)
                std::memcpy(r.models[addr].size, &b[p + 0x88], 12);
            p += 0xa0;
        }
        uint32_t count;
        get(p, &count, 4);
        for (uint32_t k = 0; k < count; k++) {
            int32_t id, pad;
            get(p, &id, 4), get(p, &pad, 4);
            uint64_t addr;
            get(p, &addr, 8);
            if (addr) {
                std::memcpy(r.models[addr].size, &b[p + 0x88], 12);
                r.ids[id] = addr;
            }
            p += 0xa0;
        }
        out.push_back(std::move(r));
    }
    return out;
}
// The rules the pose takes as inputs (NativeRules) against the values the game gave for the
// same records: the current ability's times, the held item's use time and which worn items
// are usable. Only buff 0xe is recorded; the record says nothing of buffs 0xc and 0xd.
static int rulesOracle(const std::vector<std::string> &files) {
    size_t records = 0;
    // The game's model list (id -> size) and item model table, next to the recordings.
    std::vector<std::array<int, 3>> spriteSizes;
    if (!files.empty()) {
        const std::filesystem::path oracle = std::filesystem::path(files[0]).parent_path();
        nativeitems::load(oracle);
        std::ifstream in(oracle.parent_path() / "sprites.txt");
        std::string line;
        while (std::getline(in, line)) {
            std::istringstream l(line);
            int id;
            std::string path;
            std::array<int, 3> s{};
            if (l >> id >> path >> s[0] >> s[1] >> s[2] && id >= 0) {
                if ((size_t)id >= spriteSizes.size())
                    spriteSizes.resize(id + 1, {-1, -1, -1});
                spriteSizes[id] = s;
            }
        }
    }
    std::map<std::string, std::array<size_t, 2>> byRule; // differ count, compared
    int shown = 0;
    std::map<int, std::array<size_t, 2>> byAbility;
    std::map<int, std::array<size_t, 2>> healthByRace; // records at exactly our maximum, records
    int shownHp = 0;
    for (const std::string &file : files)
        for (const AnimRecord &r : readAnimRecords(file)) {
            records++;
            const uint8_t *c = r.creature.data();
            const bool buffE = r.buffE;
            nativerules::HasBuff hasBuff = [&](int type) { return type == 0xe && buffE; };
            bool all = true;
            auto check = [&](const char *rule, long long ours, long long game) {
                auto &n = byRule[rule];
                n[1]++;
                if (ours != game) {
                    n[0]++, all = false;
                    if (shown++ < 16)
                        std::printf("%s ability %d class %d.%d: ours %lld game %lld\n", rule, c[0x68], c[0x14c], c[0x14d], ours, game);
                }
            };
            check("abilityTime 0x4f220", nativerules::abilityDuration(c, hasBuff), r.abilityTime);
            check("abilityLength 0x4ead0", nativerules::abilityEffectTime(c, -1, hasBuff), r.abilityLength);
            check("abilityLength2 0x660d0", nativerules::abilityMiddleTime(c, -1, hasBuff), r.abilityLength2);
            check("abilityLength3 0x5e9f0", nativerules::abilityRecoveryTime(c, -1, hasBuff), r.abilityLength3);
            check("useTime 0x108fa0", nativerules::useTime(c + 0x1d8), r.useTime);
            for (int k = 0; k < 9; k++)
                check("usable 0x50640", nativerules::mayUse(c, c + ANIM_ITEM_SLOTS[k]), r.usable[k]);
            if (nativeitems::loaded() && !spriteSizes.empty())
                for (int k = 0; k < 9; k++) { // the item's model (0x173b10): none, or one of the game's size
                    const int id = nativeitems::modelId(c + ANIM_ITEM_SLOTS[k]);
                    const nativepose::Model *game = r.items[k] ? &r.models.at(r.items[k]) : nullptr;
                    long long ours = id < 0 || (size_t)id >= spriteSizes.size() ? -1
                                                                                  : spriteSizes[id][0] * 1000000LL + spriteSizes[id][1] * 1000 + spriteSizes[id][2];
                    long long theirs = game ? game->size[0] * 1000000LL + game->size[1] * 1000 + game->size[2] : -1;
                    check("item model 0x173b10 (size)", ours, theirs);
                }
            auto &a = byAbility[c[0x68]];
            a[1]++;
            if (!all)
                a[0]++;
            // Health (c+0x180) equals 0x5fac0's maximum whenever the creature is unhurt.
            float hp;
            std::memcpy(&hp, c + 0x180, 4);
            const float maxHp = nativerules::maxHealth(c);
            auto &h = healthByRace[*(int32_t *)(c + 0x64)];
            h[1]++;
            if (hp == maxHp)
                h[0]++;
            else if (hp > maxHp && shownHp++ < 6)
                std::printf("health above our maximum: race %d hp %.9g ours %.9g\n", *(int32_t *)(c + 0x64), hp, maxHp);
        }
    std::printf("%zu records\n", records);
    size_t wrong = 0;
    for (auto &[rule, n] : byRule) {
        std::printf("  %s: %zu of %zu differ\n", rule.c_str(), n[0], n[1]);
        wrong += n[0];
    }
    for (auto &[ability, n] : byAbility)
        std::printf("  ability %d: %zu of %zu records differ\n", ability, n[0], n[1]);
    for (auto &[race, n] : healthByRace)
        std::printf("  race %d: health at our maximum (0x5fac0) in %zu of %zu records\n", race, n[0], n[1]);
    return wrong ? 1 : 0;
}

// worldgen::makeLooks against every appearance in looks.bin: the game made each with rand() scripted
// by the extractor (xorshift32 from the record's seed, 15 bits), so the same draws must
// give the same 0xb0 bytes and the same number of draws.
static int looksOracle(const std::filesystem::path &dir) {
    std::string why;
    worldgen::CreatureTables tables;
    if (!tables.load(dir, &why)) {
        std::printf("looksoracle: %s\n", why.c_str());
        return 1;
    }
    constexpr size_t SIZE = worldgen::Appearance::SIZE;
    std::ifstream in(dir / "looks.bin", std::ios::binary);
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
    size_t records = 0, same = 0, sameDraws = 0;
    std::map<int, int> wrongRaces;
    std::map<int, int> wrongBytes;
    for (size_t at = 4; at + 16 + SIZE <= b.size(); at += 16 + SIZE) {
        int32_t race;
        uint32_t seed, calls;
        std::memcpy(&race, &b[at], 4), std::memcpy(&seed, &b[at + 4], 4), std::memcpy(&calls, &b[at + 8], 4);
        uint32_t state = seed ? seed : 0x9e3779b9u;
        uint32_t drawn = 0;
        auto rand = [&]() {
            state ^= state << 13, state ^= state >> 17, state ^= state << 5;
            drawn++;
            return (int)(state & 0x7fff);
        };
        worldgen::Appearance looks;
        worldgen::makeLooks(tables, race, false, worldgen::Draw(rand), looks);
        const std::array<uint8_t, SIZE> &a = looks.bytes;
        records++;
        const bool bytes = std::memcmp(a.data(), &b[at + 16], SIZE) == 0;
        if (bytes)
            same++;
        if (drawn == calls)
            sameDraws++;
        if (!bytes || drawn != calls) {
            if (wrongRaces[race]++ == 0 && wrongRaces.size() <= 25) {
                std::printf("race %d (0x%x): draws ours %u game %u;", race, race, drawn, calls);
                for (size_t k = 0; k < SIZE; k++)
                    if (a[k] != b[at + 16 + k])
                        std::printf(" +%zx %02x/%02x", k, a[k], b[at + 16 + k]);
                std::printf("\n");
            }
            for (size_t k = 0; k < SIZE; k++)
                if (a[k] != b[at + 16 + k])
                    wrongBytes[(int)k]++;
        }
    }
    std::printf("%zu appearances: %zu with every byte the same, %zu with the same draws; %zu races differ\n", records, same,
                sameDraws, wrongRaces.size());
    return same == records && sameDraws == records ? 0 : 1;
}

// NativeUpdate against the game's own creature updates (0x2bb8e0), recorded by ZoneExtractor's
// update.request into update.bin (CWU2): each with the creature before and after the call,
// the dt, the world's time, the creature's buffs, the creatures and objects near it, the
// blocks around it and every rand() the call drew. Every byte of the record must match.
// NativeAI against the game's own AI ticks (0x13830), recorded by ZoneExtractor's
// ai.request into ai.bin (CAI2): the loaded zones and their objects, every creature and
// AI record before and after the tick, the blocks it looked up and every rand() and
// timeGetTime() it took. Each tick is replayed from its "before" and must give its
// "after": every creature's record and containers, every AI record and what the tick
// told the world.
namespace aioracle {
constexpr size_t RECORD = 0xc10;
struct Reader {
    const uint8_t *p;
    size_t k = 0;
    template <class T> T get() {
        T v;
        std::memcpy(&v, p + k, sizeof v);
        k += sizeof v;
        return v;
    }
    void take(void *to, size_t n) { std::memcpy(to, p + k, n), k += n; }
};
struct CreatureState {
    uint64_t address = 0;
    std::vector<uint8_t> record;
    nativeai::Creature c;
};
struct State {
    std::vector<CreatureState> creatures;
    std::vector<std::pair<int64_t, uint64_t>> ids;
    std::map<int64_t, nativeai::Entry> ai;
    std::vector<std::array<uint8_t, 0x18>> aggro;
    std::vector<std::array<uint8_t, 0x30>> buffs;
    std::vector<int64_t> calls;
};
static int version = 2;
static void readCreature(Reader &r, CreatureState &s) {
    s.address = r.get<uint64_t>();
    s.record.assign(nativepose::CREATURE_SIZE, 0);
    r.take(s.record.data(), RECORD);
    s.c.buffs.resize(r.get<uint32_t>());
    for (auto &b : s.c.buffs)
        r.take(b.data(), b.size());
    for (uint32_t n = r.get<uint32_t>(); n; n--) {
        nativeai::Interaction i;
        r.take(i.data(), i.size());
        s.c.interactions.push_back(i);
    }
    for (auto *m : {&s.c.aggro, &s.c.damage})
        for (uint32_t n = r.get<uint32_t>(); n; n--) {
            const int64_t id = r.get<int64_t>();
            (*m)[id] = r.get<float>();
        }
    if (version >= 2)
        for (uint32_t n = r.get<uint32_t>(); n; n--)
            s.c.group.insert(r.get<int64_t>());
    s.c.skills.resize(r.get<uint32_t>());
    for (auto &v : s.c.skills)
        v = r.get<int32_t>();
    s.c.inventory.resize(r.get<uint32_t>());
    for (auto &tab : s.c.inventory) {
        tab.resize(r.get<uint32_t>());
        for (auto &item : tab)
            r.take(item.data(), item.size());
    }
}
static nativeai::IntVec readVec(Reader &r) {
    nativeai::IntVec v;
    r.take(v.data(), 12);
    return v;
}
static void readEntry(Reader &r, nativeai::Entry &e) {
    r.take(e.raw.data(), e.raw.size());
    for (uint32_t n = r.get<uint32_t>(); n; n--) {
        nativeai::Interaction i;
        r.take(i.data(), i.size());
        e.waiting.push_back(i);
    }
    for (uint32_t n = r.get<uint32_t>(); n; n--) {
        const nativeai::IntVec key = readVec(r);
        nativeai::Step s;
        s.g = r.get<int32_t>(), s.h = r.get<int32_t>(), s.f = r.get<int32_t>();
        s.from = readVec(r);
        e.steps[key] = s;
    }
    for (uint32_t n = r.get<uint32_t>(); n; n--)
        e.open.insert(readVec(r));
    e.best = e.steps.end();
    if (!r.get<uint8_t>())
        e.best = e.steps.find(readVec(r));
    for (uint32_t n = r.get<uint32_t>(); n; n--)
        e.path.push_back(readVec(r));
    for (uint32_t n = r.get<uint32_t>(); n; n--)
        e.taken.insert(r.get<uint64_t>());
    for (uint32_t n = r.get<uint32_t>(); n; n--) {
        std::array<uint8_t, 0x10> o;
        r.take(o.data(), o.size());
        e.other.push_back(o);
    }
}
static void readState(Reader &r, State &s) {
    s.creatures.resize(r.get<uint32_t>());
    for (auto &c : s.creatures)
        readCreature(r, c);
    for (uint32_t n = r.get<uint32_t>(); n; n--) {
        const int64_t id = r.get<int64_t>();
        s.ids.push_back({id, r.get<uint64_t>()});
    }
    for (uint32_t n = r.get<uint32_t>(); n; n--) {
        nativeai::Entry e;
        readEntry(r, e);
        int64_t id;
        std::memcpy(&id, e.raw.data() + 0x20, 8);
        s.ai[id] = e;
    }
    s.aggro.resize(r.get<uint32_t>());
    for (auto &a : s.aggro)
        r.take(a.data(), a.size());
    s.buffs.resize(r.get<uint32_t>());
    for (auto &a : s.buffs)
        r.take(a.data(), a.size());
    s.calls.resize(r.get<uint32_t>());
    for (auto &a : s.calls)
        a = r.get<int64_t>();
}
// The bytes of a creature record that are the game's container pointers and sizes.
static bool creatureSkip(size_t o) {
    static const std::pair<size_t, size_t> skip[] = {{0x0, 8},     {0x980, 0x10}, {0x9f0, 0x18},
                                                     {0xab8, 0x10}, {0xb88, 0x10}, {0xb98, 0x10},
                                                     {0xbd0, 0x18}};
    for (auto [at, n] : skip)
        if (o >= at && o < at + n)
            return true;
    return false;
}
// ... and of an AI record (the tree node's links, the containers).
static bool entrySkip(size_t o) {
    static const std::pair<size_t, size_t> skip[] = {{0x0, 0x20},   {0x78, 0x10},  {0xb8, 0x28},
                                                     {0x138, 0x10}, {0x148, 0x10}, {0x158, 0x10}};
    for (auto [at, n] : skip)
        if (o >= at && o < at + n)
            return true;
    return false;
}

static int run(const std::vector<std::string> &files, int only) {
    size_t ticks = 0, sameTicks = 0, creatures = 0, sameCreatures = 0, entries = 0, sameEntries = 0;
    size_t moved = 0, movedSame = 0; // ticks during which a player's record moved (the network writes it)
    size_t outside = 0, randShort = 0;
    std::map<std::string, size_t> where;
    int shown = 0, index = 0;
    for (const std::string &file : files) {
        std::ifstream in(file, std::ios::binary);
        std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
        // CWA1 (the first recordings) or CAI2 (with each creature's group).
        if (b.size() < 8 || (std::memcmp(b.data(), "CWA1", 4) != 0 && std::memcmp(b.data(), "CAI2", 4) != 0)) {
            std::printf("%s: not an AI recording\n", file.c_str());
            return 1;
        }
        version = b[3] - '0';
        size_t at = 8;
        while (at + 4 <= b.size()) {
            uint32_t n;
            std::memcpy(&n, &b[at], 4);
            at += 4;
            if (at + n > b.size())
                break;
            Reader r{&b[at]};
            at += n;
            const int tickIndex = index++;
            if (only >= 0 && tickIndex != only)
                continue;
            const uint32_t dt = r.get<uint32_t>();
            nativeai::World w;
            w.time = r.get<float>();
            if (version >= 2)
                w.day = r.get<float>();
            else // v1 has no time of day; its creatures' night flags (+0x128 bit 0) show daytime
                w.day = 12 * 3600000.0f;
            const uint64_t localAddress = r.get<uint64_t>();
            // Zones and their objects.
            std::map<std::pair<int, int>, std::vector<nativeai::Object>> zones;
            std::map<std::pair<int, int>, uint64_t> objectBase;
            for (uint32_t z = r.get<uint32_t>(); z; z--) {
                const int32_t zx = r.get<int32_t>(), zy = r.get<int32_t>();
                const uint64_t zone = r.get<uint64_t>();
                if (!zone)
                    continue;
                objectBase[{zx, zy}] = r.get<uint64_t>();
                auto &list = zones[{zx, zy}];
                list.resize(r.get<uint32_t>());
                for (auto &o : list)
                    r.take(o.data(), o.size());
            }
            State before, after;
            readState(r, before);
            readState(r, after);
            std::map<std::array<int32_t, 3>, nativeai::Block> blocks;
            for (uint32_t k = r.get<uint32_t>(); k; k--) {
                r.get<int32_t>(), r.get<int32_t>();
                std::array<int32_t, 3> p{r.get<int32_t>(), r.get<int32_t>(), r.get<int32_t>()};
                nativeai::Block v;
                r.take(v.data(), 6);
                blocks[p] = v;
            }
            std::vector<int32_t> draws(r.get<uint32_t>());
            for (auto &d : draws)
                d = r.get<int32_t>();
            std::vector<uint32_t> clock(r.get<uint32_t>());
            for (auto &c : clock)
                c = r.get<uint32_t>();
            std::map<std::pair<int, int>, std::deque<std::array<int64_t, 3>>> spots; // 0x31c0e0's answers, in order
            if (version >= 2)
                for (uint32_t k = r.get<uint32_t>(); k; k--) {
                    const int cx = r.get<int32_t>(), cy = r.get<int32_t>();
                    std::array<int64_t, 3> p;
                    p[0] = r.get<int64_t>(), p[1] = r.get<int64_t>(), p[2] = r.get<int64_t>();
                    spots[{cx, cy}].push_back(p);
                }

            // v1 recordings don't carry groups (+0xc08). The townsfolk in them were one group
            // (one spawn): in the game the later ones follow the first (§4.6).
            if (version < 2) {
                std::set<int64_t> townsfolk;
                for (auto &c : before.creatures)
                    if (c.record[0x60] == 3) {
                        int64_t id;
                        std::memcpy(&id, &c.record[8], 8);
                        townsfolk.insert(id);
                    }
                for (auto &c : before.creatures)
                    if (c.record[0x60] == 3)
                        c.c.group = townsfolk;
            }
            // The world as the tick saw it.
            std::map<uint64_t, nativeai::Creature *> byAddress;
            for (auto &c : before.creatures) {
                c.c.record = c.record.data();
                byAddress[c.address] = &c.c;
                w.creatures.push_back(&c.c);
                if (c.address == localAddress)
                    w.local = &c.c;
            }
            for (auto [id, address] : before.ids)
                if (auto f = byAddress.find(address); f != byAddress.end())
                    w.byId[id] = f->second;
            bool out = false;
            auto floorDiv = [](int v) { return v >= 0 ? v / 64 : -((-v + 63) / 64); };
            w.zone = [&](int zx, int zy) { return zones.count({zx, zy}) != 0; };
            w.objects = [&](int zx, int zy) -> const std::vector<nativeai::Object> * {
                auto f = zones.find({zx, zy});
                return f == zones.end() ? nullptr : &f->second;
            };
            w.objectKey = [&](int zx, int zy, int i) { return objectBase[{zx, zy}] + (uint64_t)i * 0xe0; };
            w.block = [&](int x, int y, int z) -> nativeai::Block {
                if (!zones.count({floorDiv(x), floorDiv(y)}))
                    return {0xc8, 0xc8, 0xc8, 0, 1, 0};
                auto f = blocks.find({x, y, z});
                if (f == blocks.end()) {
                    out = true;
                    return nativeai::Block{0xc8, 0xc8, 0xc8, 0, 1, 0}; // solid, so a lost search still ends
                }
                return f->second;
            };
            size_t drawn = 0, timed = 0;
            w.rand = [&] {
                if (only >= 0)
                    std::printf("    rand %zu: %d\n", drawn, drawn < draws.size() ? draws[drawn] : -1);
                return drawn < draws.size() ? draws[drawn++] : (drawn++, 0);
            };
            if (only >= 0)
                w.turn = [](int64_t id) { std::printf("  turn %lld\n", (long long)id); };
            w.clock = [&] { return timed < clock.size() ? clock[timed++] : (timed++, 0u); };
            w.spot = [&](int cx, int cy, int64_t to[3]) {
                auto f = spots.find({cx, cy});
                if (f == spots.end() || f->second.empty()) {
                    out = true;
                    return false;
                }
                const auto at = f->second.front();
                f->second.pop_front();
                to[0] = at[0], to[1] = at[1], to[2] = at[2];
                // The game took the spot from the cell's list with a rand(); only a cell without
                // spots gets its zone's middle without one.
                const int zx = cx * 16 >= 0 ? cx * 16 / 64 : -((-cx * 16 + 63) / 64), zy = cy * 16 >= 0 ? cy * 16 / 64 : -((-cy * 16 + 63) / 64);
                if (to[0] != ((int64_t)(zx * 64 + 32) << 16) + 32768 || to[1] != ((int64_t)(zy * 64 + 32) << 16) + 32768)
                    w.rand();
                return true;
            };
            w.aggroNotes = before.aggro, w.buffNotes = before.buffs, w.calls = before.calls;

            if (only >= 0)
                for (auto &c : before.creatures) {
                    int64_t id;
                    uint32_t flags;
                    int32_t timer = 0;
                    std::memcpy(&id, &c.record[8], 8);
                    std::memcpy(&flags, &c.record[0xbcc], 4);
                    if (auto f = before.ai.find(id); f != before.ai.end())
                        std::memcpy(&timer, f->second.raw.data() + 0x28, 4);
                    std::printf("  creature %lld: kind %d race %d ability %d flags %x group %zu skills %zu timer %d\n",
                                (long long)id, c.record[0x60], c.record[0x64], c.record[0x68], flags, c.c.group.size(),
                                c.c.skills.size(), timer);
                    for (int64_t g : c.c.group)
                        std::printf("    group %llx\n", (unsigned long long)g);
                }
            std::map<int64_t, nativeai::Entry> initialAi;
            if (only >= 0)
                initialAi = before.ai;
            nativeai::tick(w, before.ai, dt);

            ticks++;
            if (out)
                outside++;
            if (drawn != draws.size())
                randShort++;
            bool tickSame = true;
            auto report = [&](const std::string &what, const std::string &detail) {
                tickSame = false;
                where[what]++;
                static const int showLimit = [] {
                    char v[16] = {};
                    return GetEnvironmentVariableA("AI_SHOW", v, sizeof v) ? std::atoi(v) : 30;
                }();
                if (shown++ < showLimit)
                    std::printf("tick %d: %s %s\n", tickIndex, what.c_str(), detail.c_str());
            };
            // Creatures.
            std::map<uint64_t, CreatureState *> afterBy;
            for (auto &c : after.creatures)
                afterBy[c.address] = &c;
            for (auto &c : before.creatures) {
                auto f = afterBy.find(c.address);
                if (f == afterBy.end())
                    continue;
                creatures++;
                const auto &a = f->second->c;
                bool same = true;
                std::string detail;
                // A player's record also changes from the network while the tick runs; the AI
                // only writes into its containers.
                for (size_t o = 0; o < RECORD && c.record[0x60] != 0; o += 4) {
                    if (creatureSkip(o) || std::memcmp(&c.record[o], &f->second->record[o], 4) == 0)
                        continue;
                    same = false;
                    float x, y;
                    std::memcpy(&x, &c.record[o], 4), std::memcpy(&y, &f->second->record[o], 4);
                    char s[64];
                    std::snprintf(s, sizeof s, " +%zx %.7g/%.7g", o, x, y);
                    if (detail.size() < 200)
                        detail += s;
                    where["creature +" + std::to_string(o)]++;
                }
                if (c.c.buffs != a.buffs)
                    same = false, detail += " buffs";
                if (c.c.interactions != a.interactions)
                    same = false, detail += " interactions";
                if (c.c.aggro != a.aggro)
                    same = false, detail += " aggro";
                if (c.c.damage != a.damage)
                    same = false, detail += " damage";
                if (same)
                    sameCreatures++;
                else {
                    int64_t id;
                    std::memcpy(&id, &c.record[8], 8);
                    report("creature", std::to_string(id) + " race " + std::to_string(c.record[0x64]) + detail);
                }
            }
            // AI records.
            for (auto &[id, e] : after.ai) {
                entries++;
                auto f = before.ai.find(id);
                if (f == before.ai.end()) {
                    report("entry", std::to_string(id) + " missing");
                    continue;
                }
                const auto &m = f->second;
                bool same = true;
                std::string detail;
                for (size_t o = 0; o < 0x178; o += 4) {
                    if (entrySkip(o) || std::memcmp(&m.raw[o], &e.raw[o], 4) == 0)
                        continue;
                    same = false;
                    float x, y;
                    std::memcpy(&x, &m.raw[o], 4), std::memcpy(&y, &e.raw[o], 4);
                    char s[64];
                    std::snprintf(s, sizeof s, " +%zx %.7g/%.7g", o, x, y);
                    if (detail.size() < 200)
                        detail += s;
                    where["entry +" + std::to_string(o)]++;
                }
                if (m.waiting != e.waiting)
                    same = false, detail += " waiting";
                if (m.open != e.open)
                    same = false, detail += " open";
                if (m.path != e.path)
                    same = false, detail += " path";
                if (m.taken != e.taken)
                    same = false, detail += " taken";
                bool stepsSame = m.steps.size() == e.steps.size();
                for (auto i = m.steps.cbegin(), j = e.steps.cbegin(); stepsSame && i != m.steps.cend(); ++i, ++j)
                    stepsSame = i->first == j->first && i->second.g == j->second.g && i->second.h == j->second.h &&
                                i->second.f == j->second.f && i->second.from == j->second.from;
                if (!stepsSame)
                    same = false, detail += " steps(" + std::to_string(m.steps.size()) + "/" +
                                            std::to_string(e.steps.size()) + ")";
                const bool bestEnd = m.best == m.steps.end(), eEnd = e.best == e.steps.end();
                if (bestEnd != eEnd || (!bestEnd && m.best->first != e.best->first))
                    same = false, detail += " best";
                if (same)
                    sameEntries++;
                else {
                    report("entry", std::to_string(id) + detail);
                    if (only >= 0) { // one tick: the searches side by side
                        auto dump = [](const char *who, const nativeai::Entry &x) {
                            std::printf("  %s: %zu steps, %zu open, best %s, path %zu\n", who, x.steps.size(), x.open.size(),
                                        x.best == x.steps.end() ? "-" : "set", x.path.size());
                            std::printf("    path:");
                            for (auto &p : x.path)
                                std::printf(" %d,%d,%d", p[0], p[1], p[2]);
                            std::printf("\n");
                            for (auto &[k, s] : x.steps)
                                std::printf("    %d,%d,%d g%d h%d f%d from %d,%d,%d%s\n", k[0], k[1], k[2], s.g, s.h, s.f,
                                            s.from[0], s.from[1], s.from[2], x.open.count(k) ? " open" : "");
                        };
                        if (auto b = initialAi.find(id); b != initialAi.end())
                            dump("before", b->second);
                        dump("ours", m);
                        dump("game", e);
                    }
                }
            }
            if (before.ai.size() != after.ai.size())
                report("entries", std::to_string(before.ai.size()) + "/" + std::to_string(after.ai.size()));
            // Notes compare without the padding the game leaves unset.
            for (auto *notes : {&w.aggroNotes, &after.aggro})
                for (auto &n : *notes)
                    std::memset(n.data() + 0x14, 0, 4);
            if (w.aggroNotes != after.aggro)
                report("aggro notes", std::to_string(w.aggroNotes.size()) + "/" + std::to_string(after.aggro.size()));
            if (w.buffNotes != after.buffs)
                report("buff notes", std::to_string(w.buffNotes.size()) + "/" + std::to_string(after.buffs.size()));
            if (w.calls != after.calls)
                report("calls", std::to_string(w.calls.size()) + "/" + std::to_string(after.calls.size()));
            if (drawn != draws.size())
                report("rand", std::to_string(drawn) + "/" + std::to_string(draws.size()));
            if (timed != clock.size())
                report("clock", std::to_string(timed) + "/" + std::to_string(clock.size()));
            // A player whose place changed while the tick ran: the AI read it somewhere between.
            bool playerMoved = false;
            for (auto &c : before.creatures)
                if (c.record[0x60] == 0)
                    if (auto f = afterBy.find(c.address); f != afterBy.end())
                        playerMoved |= std::memcmp(&c.record[0x10], &f->second->record[0x10], 24) != 0;
            if (playerMoved)
                moved++, movedSame += tickSame;
            if (tickSame)
                sameTicks++;
        }
    }
    std::printf("%zu ticks: %zu exactly the same; creatures %zu/%zu, AI records %zu/%zu (%zu read outside the "
                "recorded blocks, %zu with another number of draws)\n",
                ticks, sameTicks, sameCreatures, creatures, sameEntries, entries, outside, randShort);
    std::printf("  in %zu ticks a player moved while the tick ran (%zu of them came out the same); %zu other ticks differ\n",
                moved, movedSame, (ticks - sameTicks) - (moved - movedSame));
    std::vector<std::pair<size_t, std::string>> worst;
    for (auto &[k, v] : where)
        worst.push_back({v, k});
    std::sort(worst.rbegin(), worst.rend());
    for (size_t i = 0; i < worst.size() && i < 25; i++)
        std::printf("  %s: %zu\n", worst[i].second.c_str(), worst[i].first);
    return ticks && sameTicks == ticks ? 0 : 1;
}
} // namespace aioracle

static int updateOracle(const std::vector<std::string> &files) {
    constexpr size_t SIZE = nativepose::CREATURE_SIZE;
    size_t compared = 0, same = 0, skippedLocal = 0, outside = 0, randShort = 0;
    std::map<int, size_t> byOffset;
    std::map<int, std::array<size_t, 2>> byAbility;
    int shown = 0;
    for (const std::string &file : files) {
        std::ifstream in(file, std::ios::binary);
        std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
        if (b.size() < 8 || std::memcmp(b.data(), "CWU2", 4) != 0) {
            std::printf("%s: not a CWU2 recording\n", file.c_str());
            return 1;
        }
        size_t at = 8;
        while (at + 4 <= b.size()) {
            uint32_t n;
            std::memcpy(&n, &b[at], 4);
            at += 4;
            if (at + n > b.size())
                break;
            const uint8_t *r = &b[at];
            size_t k = 0;
            auto take = [&](void *to, size_t len) { std::memcpy(to, r + k, len), k += len; };
            auto get = [&](auto &v) { take(&v, sizeof v); };
            uint64_t address;
            get(address);
            const bool isLocal = r[8] != 0, client = r[9] != 0;
            float dt;
            std::memcpy(&dt, r + 12, 4);
            k = 16;
            std::vector<uint8_t> creature(r + k, r + k + SIZE), after(r + k + SIZE, r + k + 2 * SIZE);
            k += 2 * SIZE;
            float time;
            get(time);
            uint32_t count;
            get(count);
            std::vector<nativebuffs::Buff> buffs(count);
            for (auto &e : buffs)
                take(e.data(), 0x18);
            // Every record the update might read, as full-size buffers.
            std::deque<std::vector<uint8_t>> keep;
            std::map<int64_t, uint8_t *> ids;
            nativeupdate::World w;
            w.client = client;
            static uint8_t elsewhere[SIZE];
            w.local = isLocal ? creature.data() : elsewhere;
            w.time = time;
            get(count);
            for (uint32_t i = 0; i < count; i++) {
                uint64_t o;
                uint8_t isNear;
                get(o), get(isNear);
                if (o == address) {
                    w.creatures.push_back(creature.data());
                    continue;
                }
                keep.emplace_back(SIZE, 0);
                uint8_t *p = keep.back().data();
                if (isNear) {
                    take(p, 0x200);
                } else {
                    int64_t away;
                    std::memcpy(&away, &creature[0x10], 8);
                    away += (int64_t)100000 << 16;
                    std::memcpy(p + 0x10, &away, 8);
                }
                w.creatures.push_back(p);
            }
            get(count);
            for (uint32_t i = 0; i < count; i++) {
                uint64_t o;
                get(o);
                keep.emplace_back(SIZE, 0);
                take(keep.back().data(), 0x200);
                w.objects.push_back(keep.back().data());
            }
            // The fire trails (effects of hit type 5) the creature owns: owner and position.
            std::list<nativeeffects::Effect> effects;
            w.effects = &effects;
            get(count);
            for (uint32_t i = 0; i < count; i++) {
                nativeeffects::Effect e{};
                e[0x6c] = 5;
                take(&e[0], 8);
                take(&e[8], 24);
                effects.push_back(e);
            }
            for (int i = 0; i < 3; i++) {
                uint64_t o;
                get(o);
                if (!o)
                    continue;
                int64_t id;
                if (o == address) {
                    std::memcpy(&id, &creature[8], 8);
                    k += 0x9b0;
                    ids[id] = creature.data();
                    continue;
                }
                keep.emplace_back(SIZE, 0);
                take(keep.back().data(), 0x9b0);
                std::memcpy(&id, keep.back().data() + 8, 8);
                ids[id] = keep.back().data();
            }
            int32_t x0, y0, z0, nx, ny, nz;
            get(x0), get(y0), get(z0), get(nx), get(ny), get(nz);
            std::vector<uint8_t> loaded((size_t)nx * ny);
            std::vector<nativeupdate::Block> blocks((size_t)nx * ny * nz);
            for (int x = 0; x < nx; x++)
                for (int y = 0; y < ny; y++) {
                    get(loaded[(size_t)x * ny + y]);
                    for (int z = 0; z < nz; z++)
                        take(blocks[((size_t)x * ny + y) * nz + z].data(), 6);
                }
            std::vector<int32_t> draws;
            get(count);
            draws.resize(count);
            for (auto &d : draws)
                get(d);
            at += n;
            if (isLocal) {
                skippedLocal++;
                continue;
            }
            bool out = false;
            w.block = [&](int x, int y, int z) {
                x -= x0, y -= y0, z -= z0;
                if (x < 0 || y < 0 || z < 0 || x >= nx || y >= ny || z >= nz) {
                    out = true;
                    return nativeupdate::Block{};
                }
                return blocks[((size_t)x * ny + y) * nz + z];
            };
            w.loaded = [&](int x, int y) {
                x -= x0, y -= y0;
                if (x < 0 || y < 0 || x >= nx || y >= ny) {
                    out = true;
                    return false;
                }
                return loaded[(size_t)x * ny + y] != 0;
            };
            size_t drawn = 0;
            w.rand = [&] { return drawn < draws.size() ? draws[drawn++] : (drawn++, 0); };
            w.byId = [&](int64_t id) -> uint8_t * {
                auto f = ids.find(id);
                return f == ids.end() ? nullptr : f->second;
            };
            w.buffs = [&](const uint8_t *c) -> const std::vector<nativebuffs::Buff> * {
                return c == creature.data() ? &buffs : nullptr;
            };
            nativeupdate::update(w, creature.data(), dt);
            compared++;
            if (out)
                outside++;
            if (drawn != draws.size())
                randShort++;
            const int ability = after[0x68];
            byAbility[ability][1]++;
            bool equal = true;
            for (size_t o = 0; o < SIZE; o += 4)
                if (std::memcmp(&creature[o], &after[o], 4) != 0) {
                    equal = false;
                    byOffset[(int)o]++;
                }
            if (equal) {
                same++;
                continue;
            }
            byAbility[ability][0]++;
            if (shown++ < 12) {
                std::printf("record %zu (ability 0x%x, dt %.3f, draws %zu/%zu):", compared, ability, dt, drawn, draws.size());
                int listed = 0;
                for (size_t o = 0; o < SIZE && listed < 12; o += 4)
                    if (std::memcmp(&creature[o], &after[o], 4) != 0) {
                        float fa, fb;
                        std::memcpy(&fa, &creature[o], 4), std::memcpy(&fb, &after[o], 4);
                        std::printf(" +%zx %.9g/%.9g", o, fa, fb);
                        listed++;
                    }
                std::printf("\n");
            }
        }
    }
    std::printf("%zu updates: %zu with every byte the same (%zu of the local player skipped, %zu read outside the "
                "recorded blocks, %zu with another number of draws)\n",
                compared, same, skippedLocal, outside, randShort);
    for (auto &[o, cnt] : byOffset)
        std::printf("  +%x differs in %zu\n", o, cnt);
    for (auto &[a, cnt] : byAbility)
        if (cnt[0])
            std::printf("  ability 0x%x: %zu of %zu differ\n", a, cnt[0], cnt[1]);
    return compared && same == compared ? 0 : 1;
}

// NativeUpkeep against the game's own creature upkeep (0x2de4c0), recorded by ZoneExtractor's
// update.request ("<count> 2de4c0") into update.bin (CWU3): the CWU2 record, then the
// creature's buffs and sides and the world's lists before the call, and after it its buffs,
// sides, what the lists gained and the hits it applied. Each call is replayed from its
// "before" and must give its "after".
static int stepOracle(const std::vector<std::string> &files, uint32_t which) {
    constexpr size_t SIZE = nativepose::CREATURE_SIZE;
    size_t compared = 0, same = 0, skippedLocal = 0, unknown = 0, hitting = 0, otherStep = 0, changed = 0, sameHits = 0;
    int shownHits = 0;
    std::map<std::string, size_t> wrong;
    std::map<int, size_t> byOffset;
    int shown = 0;
    const int show = [] {
        char v[16] = "";
        return GetEnvironmentVariableA("AI_SHOW", v, sizeof v) ? std::atoi(v) : 12;
    }();
    using Buff = std::array<uint8_t, 0x18>;
    struct Sides {
        std::map<int64_t, float> aggro, damage;
        std::set<int64_t> counted, struck;
    };
    for (const std::string &file : files) {
        std::ifstream in(file, std::ios::binary);
        std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
        // CWU4 adds the effect objects (world+0x60) to the lists.
        const bool v4 = b.size() >= 8 && std::memcmp(b.data(), "CWU4", 4) == 0;
        if (b.size() < 8 || (!v4 && std::memcmp(b.data(), "CWU3", 4) != 0)) {
            std::printf("%s: not a CWU3 or CWU4 recording\n", file.c_str());
            return 1;
        }
        const int lists = v4 ? 7 : 6;
        size_t at = 8;
        while (at + 4 <= b.size()) {
            uint32_t n;
            std::memcpy(&n, &b[at], 4);
            at += 4;
            if (at + n > b.size())
                break;
            const uint8_t *r = &b[at];
            at += n;
            size_t k = 0;
            auto take = [&](void *to, size_t len) { std::memcpy(to, r + k, len), k += len; };
            auto get = [&](auto &v) { take(&v, sizeof v); };
            uint64_t address;
            get(address);
            const bool isLocal = r[8] != 0, client = r[9] != 0;
            float dt;
            std::memcpy(&dt, r + 12, 4);
            k = 16;
            std::vector<uint8_t> creature(r + k, r + k + SIZE), after(r + k + SIZE, r + k + 2 * SIZE);
            k += 2 * SIZE;
            float time;
            get(time);
            uint32_t count;
            get(count);
            k += (size_t)count * 0x18; // its buffs (again below)
            // The creatures near it (their first 0x200 bytes) and those its record names (0x9b0).
            std::deque<std::vector<uint8_t>> keep;
            std::map<int64_t, uint8_t *> records;
            std::vector<uint8_t *> order; // the world's list; those not near, far away
            get(count);
            for (uint32_t i = 0; i < count; i++) {
                uint64_t o;
                uint8_t isNear;
                get(o), get(isNear);
                if (o == address) {
                    order.push_back(creature.data());
                    continue;
                }
                keep.emplace_back(SIZE, 0);
                uint8_t *q = keep.back().data();
                order.push_back(q);
                if (!isNear) {
                    int64_t away;
                    std::memcpy(&away, &creature[0x10], 8);
                    away += (int64_t)100000 << 16;
                    std::memcpy(q + 0x10, &away, 8);
                    continue;
                }
                take(q, 0x200);
                int64_t id;
                std::memcpy(&id, q + 8, 8);
                records[id] = q;
            }
            get(count);
            k += (size_t)count * (8 + 0x200); // objects
            get(count);
            k += (size_t)count * 0x20; // effect objects
            for (int i = 0; i < 3; i++) {
                uint64_t o;
                get(o);
                if (!o)
                    continue;
                if (o == address) {
                    k += 0x9b0;
                    continue;
                }
                keep.emplace_back(SIZE, 0);
                take(keep.back().data(), 0x9b0);
                int64_t id;
                std::memcpy(&id, keep.back().data() + 8, 8);
                records[id] = keep.back().data();
            }
            int32_t x0, y0, z0, nx, ny, nz;
            get(x0), get(y0), get(z0), get(nx), get(ny), get(nz);
            std::vector<nativeai::Block> blocks((size_t)nx * ny * nz);
            std::vector<uint8_t> loaded((size_t)nx * ny);
            for (int x = 0; x < nx; x++)
                for (int y = 0; y < ny; y++) {
                    get(loaded[(size_t)x * ny + y]); // the zone is loaded
                    for (int z = 0; z < nz; z++)
                        take(blocks[((size_t)x * ny + y) * nz + z].data(), 6);
                }
            std::vector<int32_t> draws;
            get(count);
            draws.resize(count);
            for (auto &d : draws)
                get(d);
            uint32_t rva, extraSize;
            get(rva), get(extraSize);
            const size_t extraEnd = k + extraSize;
            if (rva != which) {
                if (otherStep++ == 0)
                    std::printf("(a record of step 0x%x)\n", rva);
                continue;
            }
            auto buffsOf = [&] {
                uint32_t c;
                get(c);
                std::vector<Buff> v(c);
                for (auto &e : v)
                    take(e.data(), 0x18);
                return v;
            };
            auto sidesOf = [&] {
                Sides s;
                for (auto *m : {&s.aggro, &s.damage}) {
                    uint32_t c;
                    get(c);
                    for (uint32_t i = 0; i < c; i++) {
                        int64_t id;
                        float v;
                        get(id), get(v);
                        (*m)[id] = v;
                    }
                }
                for (auto *m : {&s.counted, &s.struck}) {
                    uint32_t c;
                    get(c);
                    for (uint32_t i = 0; i < c; i++) {
                        int64_t id;
                        get(id);
                        m->insert(id);
                    }
                }
                return s;
            };
            // Aggro notes, buff notes, calls, damage numbers, sounds, particles.
            static const size_t listBytes[7] = {0x18, 0x30, 8, 0x48, 0x30, 0x48, 0x80};
            std::array<std::vector<std::vector<uint8_t>>, 7> before, gained;
            const std::vector<Buff> buffs0 = buffsOf();
            const Sides sides0 = sidesOf();
            for (int l = 0; l < lists; l++) {
                get(count);
                for (uint32_t i = 0; i < count; i++)
                    before[l].emplace_back(r + k, r + k + listBytes[l]), k += listBytes[l];
            }
            const std::vector<Buff> buffs1 = buffsOf();
            const Sides sides1 = sidesOf();
            for (int l = 0; l < lists; l++) {
                get(count);
                for (uint32_t i = (uint32_t)before[l].size(); i < count; i++)
                    gained[l].emplace_back(r + k, r + k + listBytes[l]), k += listBytes[l];
            }
            get(count);
            std::vector<std::vector<uint8_t>> hits;
            for (uint32_t i = 0; i < count; i++)
                hits.emplace_back(r + k, r + k + 79), k += 79;
            // Newer recordings end with each hit's rand() draws (before, after) and result:
            // after the world's dead and the zone's links, which this step does not read.
            std::vector<std::array<uint32_t, 2>> hitDraws;
            std::vector<uint8_t> hitResults;
            {
                size_t q = k;
                auto u32 = [&] {
                    uint32_t v = 0;
                    if (q + 4 <= extraEnd)
                        std::memcpy(&v, r + q, 4);
                    q += 4;
                    return v;
                };
                q += 8 * (size_t)u32(); // the dead
                q += 1;                 // the zone is loaded
                const uint32_t links = u32();
                for (uint32_t i = 0; i < links && q < extraEnd; i++) {
                    q += 8 + 1;
                    q += 8 * (size_t)u32();
                }
                if (q + 4 <= extraEnd) {
                    const uint32_t n = u32();
                    for (uint32_t i = 0; i < n && q + 9 <= extraEnd; i++) {
                        std::array<uint32_t, 2> d;
                        d[0] = u32(), d[1] = u32();
                        hitDraws.push_back(d);
                        hitResults.push_back(r[q++]);
                    }
                }
            }
            if (isLocal) {
                skippedLocal++;
                continue;
            }
            nativeai::Creature me;
            me.record = creature.data();
            me.buffs = buffs0;
            me.aggro = sides0.aggro, me.damage = sides0.damage;
            me.counted = sides0.counted, me.struck = sides0.struck;
            std::deque<nativeai::Creature> others;
            std::map<int64_t, nativeai::Creature *> byId;
            std::map<uint8_t *, nativeai::Creature *> byRecord;
            for (auto &[id, p] : records) {
                others.emplace_back();
                others.back().record = p;
                byId[id] = &others.back();
                byRecord[p] = &others.back();
            }
            int64_t myId;
            std::memcpy(&myId, &creature[8], 8);
            byId[myId] = &me;
            byRecord[creature.data()] = &me;
            nativeupkeep::World w;
            w.client = client;
            w.time = time;
            bool out = false;
            w.byId = [&](int64_t id) -> nativeai::Creature * {
                auto f = byId.find(id);
                return f == byId.end() ? nullptr : f->second;
            };
            w.blockAt = [&](int64_t x, int64_t y, int64_t z) {
                const int bx = (int)(x >> 16) - x0, by = (int)(y >> 16) - y0, bz = (int)(z >> 16) - z0;
                if (bx < 0 || by < 0 || bz < 0 || bx >= nx || by >= ny || bz >= nz) {
                    out = true;
                    return nativeai::Block{};
                }
                return blocks[((size_t)bx * ny + by) * nz + bz];
            };
            w.light = [&](int, int, int) { // the time of day and the light are not recorded
                out = true;
                return 0;
            };
            size_t drawn = 0;
            w.rand = [&] { return drawn < draws.size() ? draws[drawn++] : (drawn++, 0); };
            if (std::memcmp(creature.data(), after.data(), SIZE) != 0 || buffs0 != buffs1 || !gained[1].empty() ||
                !gained[3].empty() || !gained[4].empty())
                changed++;
            // The ability step's world: the same, with the world's list and the hits it applies.
            nativeability::World aw;
            aw.client = client;
            aw.byId = w.byId;
            aw.blockAt = w.blockAt;
            aw.block = [&](int x, int y, int z) {
                x -= x0, y -= y0, z -= z0;
                if (x < 0 || y < 0 || z < 0 || x >= nx || y >= ny || z >= nz) {
                    out = true;
                    return nativeai::Block{};
                }
                return blocks[((size_t)x * ny + y) * nz + z];
            };
            aw.zone = [&](int zx, int zy) { // a recorded column of the zone says
                for (int x = std::max(zx * 64, x0); x < std::min(zx * 64 + 64, x0 + nx); x++)
                    for (int y = std::max(zy * 64, y0); y < std::min(zy * 64 + 64, y0 + ny); y++)
                        return loaded[(size_t)(x - x0) * ny + (y - y0)] != 0;
                out = true;
                return true;
            };
            aw.rand = w.rand;
            for (uint8_t *q : order) {
                auto f = byRecord.find(q);
                if (f == byRecord.end()) {
                    others.emplace_back();
                    others.back().record = q;
                    f = byRecord.emplace(q, &others.back()).first;
                }
                aw.creatures.push_back(f->second);
            }
            // The game's hit runs inside the call: where the recording says, its rand() draws are
            // passed over and its result given (otherwise it lands and draws nothing).
            size_t hitsAsked = 0;
            aw.hit = [&](const nativeupkeep::Hit &) {
                if (hitsAsked >= hitDraws.size())
                    return true;
                const auto d = hitDraws[hitsAsked];
                const bool landed = hitResults[hitsAsked++] != 0;
                drawn += d[1] - d[0];
                return landed;
            };
            std::list<nativeeffects::Effect> fired;
            aw.effects = &fired;
            if (which == 0x2de4c0)
                nativeupkeep::upkeep(w, &me, dt);
            else
                nativeability::ability(aw, &me, dt);
            if (which != 0x2de4c0) {
                w.sounds = aw.sounds, w.numbers = aw.numbers, w.buffNotes = aw.buffNotes, w.hits = aw.hits;
            }
            compared++;
            { // the containers' sizes as the record keeps them
                const uint64_t sizes[3] = {me.buffs.size(), me.counted.size(), me.struck.size()};
                std::memcpy(&creature[0x988], &sizes[0], 8);
                std::memcpy(&creature[0x9c0], &sizes[1], 8);
                std::memcpy(&creature[0x9d0], &sizes[2], 8);
            }
            if (out) {
                unknown++;
                continue;
            }
            if (!hits.empty()) { // the game applies them inside the call (0x29eb60, not ported yet)
                hitting++;
                // The hits asked for, as the probe logs them (79 bytes each).
                std::vector<std::vector<uint8_t>> ours;
                for (const nativeupkeep::Hit &h : w.hits) {
                    std::vector<uint8_t> v;
                    auto put = [&](const void *p, size_t len) { v.insert(v.end(), (const uint8_t *)p, (const uint8_t *)p + len); };
                    put(&h.target, 8), put(&h.attacker, 8), put(&h.damage, 4), put(&h.critical, 1), put(&h.flag, 1);
                    put(&h.charge, 4), put(h.where, 24), put(h.push, 12), put(&h.type, 4), put(&h.ability, 4);
                    put(&h.extra, 8), put(&h.last, 1);
                    ours.push_back(v);
                }
                if (ours == hits)
                    sameHits++;
                else if (shownHits++ < show) {
                    std::printf("call %zu: hits %zu/%zu", compared, ours.size(), hits.size());
                    auto dump = [](const std::vector<uint8_t> &v) {
                        float d, ch;
                        std::memcpy(&d, &v[16], 4), std::memcpy(&ch, &v[22], 4);
                        int32_t type, ab;
                        std::memcpy(&type, &v[62], 4), std::memcpy(&ab, &v[66], 4);
                        std::printf(" [damage %.9g crit %d flag %d charge %.9g type %d ability 0x%x last %d]", d, v[20], v[21],
                                    ch, type, ab, v[78]);
                    };
                    for (auto &v : ours)
                        dump(v);
                    std::printf(" game:");
                    for (auto &v : hits)
                        dump(v);
                    std::printf("\n");
                }
                continue;
            }
            std::vector<std::string> bad;
            bool equal = true;
            for (size_t o = 0; o < SIZE; o += 4)
                if (std::memcmp(&creature[o], &after[o], 4) != 0) {
                    equal = false;
                    byOffset[(int)o]++;
                }
            if (!equal)
                bad.push_back("record");
            if (me.buffs != buffs1)
                bad.push_back("buffs");
            if (me.aggro != sides1.aggro || me.damage != sides1.damage)
                bad.push_back("maps");
            if (me.counted != sides1.counted || me.struck != sides1.struck)
                bad.push_back("sets");
            auto asLists = [](const auto &v) {
                std::vector<std::vector<uint8_t>> o;
                for (auto &e : v)
                    o.emplace_back(e.begin(), e.end());
                return o;
            };
            if (asLists(w.buffNotes) != gained[1])
                bad.push_back("buff notes");
            // Where two lists of the same length first differ (for the log).
            auto firstDiff = [&](const char *what, const std::vector<std::vector<uint8_t>> &ours,
                                 const std::vector<std::vector<uint8_t>> &game) {
                if (ours.size() != game.size() || shown >= show)
                    return;
                for (size_t i = 0; i < ours.size(); i++)
                    for (size_t o = 0; o < ours[i].size(); o++)
                        if (ours[i][o] != game[i][o]) {
                            std::printf("  %s %zu +0x%zx: %02x vs %02x\n", what, i, o, ours[i][o], game[i][o]);
                            return;
                        }
            };
            // A damage number's padding (+0x15..+0x17, +0x47) is whatever the game's stack held.
            // What a step reads of others beyond their heads, and stack slots the game never
            // sets, are not compared: an ally's health in 0x1d's heal (+0x10), and the alpha
            // (+0x30) of 0x1d's and the ray spells' (0x68-0x6b) bursts.
            const uint8_t stepAbility = after[0x68];
            const bool allyHeal = stepAbility == 0x1d, unsetAlpha = allyHeal || (stepAbility >= 0x68 && stepAbility <= 0x6b);
            auto unpaddedNumbers = [&](std::vector<std::vector<uint8_t>> v) {
                for (auto &e : v) {
                    e[0x15] = e[0x16] = e[0x17] = e[0x47] = 0;
                    if (allyHeal)
                        std::memset(e.data() + 0x10, 0, 4);
                }
                return v;
            };
            if (unpaddedNumbers(asLists(w.numbers)) != unpaddedNumbers(gained[3])) {
                bad.push_back("numbers");
                firstDiff("number", unpaddedNumbers(asLists(w.numbers)), unpaddedNumbers(gained[3]));
            }
            // A particle burst ends at +0x44; its padding (+0x44..+0x47) is the stack's too.
            auto unpaddedBursts = [&](std::vector<std::vector<uint8_t>> v) {
                for (auto &e : v) {
                    e[0x44] = e[0x45] = e[0x46] = e[0x47] = 0;
                    if (unsetAlpha)
                        std::memset(e.data() + 0x30, 0, 4);
                }
                return v;
            };
            const bool burstsDiffer = which != 0x2de4c0 && unpaddedBursts(asLists(aw.particles)) != unpaddedBursts(gained[5]);
            if (burstsDiffer)
                firstDiff("particle", unpaddedBursts(asLists(aw.particles)), unpaddedBursts(gained[5]));
            // A sound's padding between its volume and owner (+0x24) is whatever the game's
            // stack held.
            auto unpadded = [](std::vector<std::vector<uint8_t>> v) {
                for (auto &e : v)
                    std::memset(e.data() + 0x24, 0, 4);
                return v;
            };
            if (unpadded(asLists(w.sounds)) != unpadded(gained[4])) {
                bad.push_back("sounds");
                const auto ours = asLists(w.sounds);
                if (ours.size() == gained[4].size() && shown < show)
                    for (size_t i = 0; i < ours.size(); i++)
                        for (size_t j = 0; j < ours[i].size(); j++)
                            if (ours[i][j] != gained[4][i][j])
                                std::printf("  sound %zu byte +%zx: %02x/%02x\n", i, j, ours[i][j], gained[4][i][j]);
            }
            if (burstsDiffer)
                bad.push_back("particles");
            // An effect object's padding (after its flag +0x64, spec +0x6c, and at +0x7c) is
            // whatever the game's stack held.
            auto unpaddedEffects = [](std::vector<std::vector<uint8_t>> v) {
                for (auto &e : v) {
                    std::memset(e.data() + 0x65, 0, 3);
                    std::memset(e.data() + 0x6d, 0, 3);
                    std::memset(e.data() + 0x7c, 0, 4);
                }
                return v;
            };
            if (v4 && unpaddedEffects(asLists(fired)) != unpaddedEffects(gained[6])) {
                bad.push_back("effect objects");
                const auto ours = unpaddedEffects(asLists(fired)), game = unpaddedEffects(gained[6]);
                if (ours.size() != game.size())
                    std::printf("  effects %zu/%zu\n", ours.size(), game.size());
                else {
                    for (size_t i = 0; i < ours.size(); i++)
                        for (size_t j = 0; j < 0x80; j += 4)
                            if (std::memcmp(&ours[i][j], &game[i][j], 4) != 0) {
                                float fa, fb;
                                std::memcpy(&fa, &ours[i][j], 4), std::memcpy(&fb, &game[i][j], 4);
                                std::printf("  effect %zu +%zx: %.9g/%.9g\n", i, j, fa, fb);
                            }
                }
            }
            if (!gained[0].empty() || !gained[2].empty() || (which == 0x2de4c0 && !gained[5].empty()))
                bad.push_back("other lists");
            if (drawn != draws.size())
                bad.push_back("draws");
            if (bad.empty()) {
                same++;
                continue;
            }
            for (auto &s : bad)
                wrong[s]++;
            if (shown++ < show) {
                std::printf("call %zu (ability 0x%x, dt %.3f, client %d):", compared, after[0x68], dt, (int)client);
                for (auto &s : bad)
                    std::printf(" [%s]", s.c_str());
                int listed = 0;
                for (size_t o = 0; o < SIZE && listed < 12; o += 4)
                    if (std::memcmp(&creature[o], &after[o], 4) != 0) {
                        float fa, fb;
                        std::memcpy(&fa, &creature[o], 4), std::memcpy(&fb, &after[o], 4);
                        std::printf(" +%zx %.9g/%.9g", o, fa, fb);
                        listed++;
                    }
                std::printf("  sounds %zu/%zu numbers %zu/%zu notes %zu/%zu buffs %zu/%zu\n", w.sounds.size(),
                            gained[4].size(), w.numbers.size(), gained[3].size(), w.buffNotes.size(),
                            gained[1].size(), me.buffs.size(), buffs1.size());
            }
        }
    }
    std::printf("%zu %s calls (%zu changing something): %zu the same (%zu of the local player skipped, %zu needing "
                "the light or blocks not recorded, %zu applying hits, %zu of them asking for the same hits, %zu records "
                "of another step)\n",
                compared, which == 0x2de4c0 ? "upkeep" : "ability", changed, same, skippedLocal, unknown, hitting,
                sameHits, otherStep);
    for (auto &[s, cnt] : wrong)
        std::printf("  %s differ in %zu\n", s.c_str(), cnt);
    for (auto &[o, cnt] : byOffset)
        std::printf("  +%x differs in %zu\n", o, cnt);
    return compared && same + unknown + hitting == compared ? 0 : 1;
}

// NativeHit against the game's own hits (0x29eb60 with 0x29dcf0), recorded by
// ZoneExtractor's hit.request into hit.bin (CHT1): the world's flags, the call's
// arguments and result, the target's and attacker's records, buffs and sides, every
// creature's first 0x200 bytes, +0x9e0 and aggro, before and after the call, what the
// world's lists gained and every rand() drawn. Each call is replayed from its "before".
// Extract/oracle/drops.bin (CDR1, tools/ZoneExtractor's drops.request): each death the
// game dropped things for (0x2a6860), with every call to 0x2a5b70 and the drops it laid.
// The port (src/NativeDrop.cpp) runs on the recorded creature and world with the
// recorded rand() values; its calls and drops must match byte for byte (the game's
// uncopied padding masked).
static int dropOracle(const std::vector<std::string> &files) {
    constexpr size_t SIZE = nativepose::CREATURE_SIZE;
    size_t deaths = 0, same = 0, callsSeen = 0, dropsSeen = 0;
    int shown = 0;
    auto itemDiff = [](const uint8_t *a, const uint8_t *b, size_t base) {
        for (size_t i = 0; i < 0xa0; i++) {
            if ((i >= 1 && i <= 3) || (i >= 0x15 && i <= 0x17) || i >= 0x9e)
                continue;
            if (a[i] != b[i])
                return (int)(base + i);
        }
        return -1;
    };
    for (const std::string &file : files) {
        std::ifstream in(file, std::ios::binary);
        std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
        if (b.size() < 4 || std::memcmp(b.data(), "CDR1", 4) != 0) {
            std::printf("%s: not a CDR1 recording\n", file.c_str());
            return 1;
        }
        size_t at = 4;
        while (at + 4 <= b.size()) {
            uint32_t n;
            std::memcpy(&n, &b[at], 4);
            at += 4;
            if (at + n > b.size())
                break;
            const uint8_t *r = &b[at];
            at += n;
            size_t k = 0;
            auto take = [&](void *to, size_t len) { std::memcpy(to, r + k, len), k += len; };
            auto get = [&](auto &v) { take(&v, sizeof v); };
            nativedrop::World w;
            get(w.day);
            uint64_t deadAddress;
            get(deadAddress);
            std::vector<std::vector<uint8_t>> records;
            records.emplace_back(r + k, r + k + SIZE);
            k += SIZE;
            std::list<nativeai::Creature> creatures(1);
            nativeai::Creature &dead = creatures.front();
            dead.record = records.back().data();
            uint32_t tabs;
            get(tabs);
            dead.inventory.resize(tabs);
            for (auto &t : dead.inventory) {
                uint32_t m;
                get(m);
                t.resize(m);
                for (auto &s : t)
                    take(s.data(), 0xa4);
            }
            uint32_t angry;
            get(angry);
            for (uint32_t i = 0; i < angry; i++) {
                int64_t id;
                float v;
                get(id), get(v);
                dead.aggro[id] = v;
            }
            std::map<int64_t, nativeai::Creature *> byId;
            uint32_t count;
            get(count);
            for (uint32_t i = 0; i < count; i++) {
                uint64_t address;
                int64_t id;
                uint8_t hostility;
                get(address), get(id), get(hostility);
                nativeai::Creature *c = nullptr;
                if (address == deadAddress) {
                    c = &dead;
                    if (hostility == 0)
                        k += SIZE;
                } else {
                    records.emplace_back(SIZE, 0);
                    if (hostility == 0)
                        take(records.back().data(), SIZE);
                    else {
                        std::memcpy(records.back().data() + 8, &id, 8);
                        records.back()[0x60] = hostility;
                    }
                    creatures.emplace_back();
                    c = &creatures.back();
                    c->record = records.back().data();
                }
                w.creatures.push_back(c);
                byId[id] = c;
            }
            w.byId = [&](int64_t id) -> nativeai::Creature * {
                auto f = byId.find(id);
                return f == byId.end() ? nullptr : f->second;
            };
            get(count);
            w.petList.resize(count);
            for (auto &v : w.petList)
                get(v);
            get(count);
            for (uint32_t i = 0; i < count; i++) {
                int32_t key;
                uint32_t m;
                get(key), get(m);
                auto &v = w.petListsByKey[key];
                v.resize(m);
                for (auto &e : v)
                    get(e);
            }
            std::map<std::array<int, 3>, nativeai::Block> blocks;
            std::map<std::pair<int, int>, bool> zones;
            {
                nativeai::Block bl;
                take(bl.data(), 6);
                auto pos = [&](size_t o) {
                    int64_t v;
                    std::memcpy(&v, dead.record + 0x10 + o, 8);
                    return (int)(v >> 16);
                };
                blocks[{pos(0), pos(8), pos(16)}] = bl;
            }
            struct Call {
                nativedrop::Place p;
                uint64_t from = 0;
                std::vector<nativedrop::Drop> drops;
            };
            uint32_t callCount;
            get(callCount);
            std::vector<Call> calls(callCount);
            for (auto &c : calls) {
                take(c.p.item.data(), 0xa0);
                take(c.p.pos, 24);
                get(c.p.angle), get(c.p.scale), get(c.p.count), get(c.from), get(c.p.owner);
                uint8_t loaded;
                get(loaded);
                const int bx = (int)(c.p.pos[0] >> 16), by = (int)(c.p.pos[1] >> 16);
                zones[{bx >> 6, by >> 6}] = loaded != 0;
                int32_t z0;
                uint32_t m;
                get(z0), get(m);
                for (uint32_t i = 0; i < m; i++) {
                    nativeai::Block bl;
                    take(bl.data(), 6);
                    blocks[{bx, by, z0 - (int)i}] = bl;
                }
                uint32_t d;
                get(d);
                c.drops.resize(d);
                for (auto &e : c.drops)
                    take(e.data(), 0xe0);
            }
            uint32_t drawn;
            get(drawn);
            std::vector<int32_t> draws(drawn);
            for (auto &v : draws)
                get(v);
            size_t next = 0;
            bool ranOut = false;
            w.rand = [&] {
                if (next < draws.size())
                    return (int)draws[next++];
                ranOut = true;
                return 0;
            };
            w.block = [&](int x, int y, int z) {
                auto f = blocks.find({x, y, z});
                return f == blocks.end() ? nativeai::Block{0x80, 0x80, 0x80, 0, 1, 0} : f->second;
            };
            w.zone = [&](int zx, int zy) {
                auto f = zones.find({zx, zy});
                return f != zones.end() && f->second;
            };
            std::vector<nativedrop::Place> placed;
            w.placing = [&](const nativedrop::Place &p) { placed.push_back(p); };
            nativedrop::drop(w, &dead);
            deaths++;
            callsSeen += calls.size();
            std::string why;
            std::vector<nativedrop::Drop> expected;
            for (auto &c : calls)
                expected.insert(expected.end(), c.drops.begin(), c.drops.end());
            dropsSeen += expected.size();
            if (placed.size() != calls.size())
                why = "calls " + std::to_string(placed.size()) + " vs " + std::to_string(calls.size());
            for (size_t i = 0; why.empty() && i < calls.size(); i++) {
                const auto &a = placed[i], &e = calls[i].p;
                char s[160] = "";
                if (int d = itemDiff(a.item.data(), e.item.data(), 0); d >= 0)
                    std::snprintf(s, sizeof s, "call %zu item +0x%x: %02x vs %02x (category %d)", i, d, a.item[d], e.item[d], e.item[0]);
                else if (std::memcmp(a.pos, e.pos, 24))
                    std::snprintf(s, sizeof s, "call %zu pos %lld,%lld,%lld vs %lld,%lld,%lld", i, (long long)a.pos[0],
                                  (long long)a.pos[1], (long long)a.pos[2], (long long)e.pos[0], (long long)e.pos[1], (long long)e.pos[2]);
                else if (a.angle != e.angle || a.scale != e.scale || a.count != e.count || a.owner != e.owner)
                    std::snprintf(s, sizeof s, "call %zu angle %g/%g scale %g/%g count %d/%d owner %lld/%lld", i, a.angle, e.angle,
                                  a.scale, e.scale, a.count, e.count, (long long)a.owner, (long long)e.owner);
                else if (calls[i].from != deadAddress)
                    std::snprintf(s, sizeof s, "call %zu from another creature", i);
                why = s;
            }
            if (why.empty() && w.drops.size() != expected.size())
                why = "drops " + std::to_string(w.drops.size()) + " vs " + std::to_string(expected.size());
            for (size_t i = 0; why.empty() && i < expected.size(); i++) {
                const auto &a = w.drops[i], &e = expected[i];
                int d = itemDiff(a.data(), e.data(), 0);
                if (d < 0)
                    for (size_t o = 0xa0; o < 0xe0; o++)
                        if (!(o >= 0xc5 && o <= 0xc7) && a[o] != e[o]) {
                            d = (int)o;
                            break;
                        }
                if (d >= 0) {
                    char s[96];
                    std::snprintf(s, sizeof s, "drop %zu +0x%x: %02x vs %02x (category %d)", i, d, a[d], e[d], e[0]);
                    why = s;
                }
            }
            if (why.empty() && (ranOut || next != draws.size()))
                why = "rand " + std::to_string(next) + (ranOut ? "+" : "") + " vs " + std::to_string(draws.size());
            if (why.empty())
                same++;
            else if (shown++ < 12)
                std::printf("death %zu (race %d, hostility %d, level %d): %s\n", deaths - 1, *(int32_t *)(dead.record + 0x64),
                            dead.record[0x60], *(int32_t *)(dead.record + 0x144), why.c_str());
        }
    }
    std::printf("drop oracle: %zu of %zu deaths match (%zu calls to 0x2a5b70, %zu drops laid)\n", same, deaths, callsSeen,
                dropsSeen);
    return same == deaths ? 0 : 1;
}

// Extract/oracle/stats.bin (CST1, tools/ZoneExtractor's stats.request): the local player's
// record and buffs with the game's own numbers for them; NativeRules on the same record.
// The player's controller (src/NativeControl.cpp) against the game's 0x9c050 calls
// (Extract/oracle/control.bin, CCT1): each call's keys, camera and gates on the "before"
// record and statics, checked against the "after".
static int controlOracle(const std::string &file) {
    constexpr size_t SIZE = nativepose::CREATURE_SIZE;
    std::ifstream in(file, std::ios::binary);
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
    if (b.size() < 4 || std::memcmp(b.data(), "CCT1", 4) != 0) {
        std::printf("%s: not a CCT1 recording\n", file.c_str());
        return 1;
    }
    size_t k = 4, calls = 0, same = 0, moving = 0;
    while (k + 4 <= b.size()) {
        uint32_t size;
        std::memcpy(&size, &b[k], 4);
        k += 4;
        if (k + size > b.size())
            break;
        const uint8_t *r = &b[k];
        k += size;
        size_t o = 0;
        auto take = [&](void *to, size_t n) { std::memcpy(to, r + o, n), o += n; };
        double dt;
        uint8_t keys[0x10], cam[0x30], gates[4];
        float stick[2];
        double yaw;
        take(&dt, 8), take(keys, 0x10), take(stick, 8), take(&yaw, 8), take(cam, 0x30), take(gates, 4);
        nativecontrol::State st, want;
        uint8_t statics[12], after[12];
        take(statics, 12), take(after, 12);
        std::memcpy(&st.lastJump, statics, 1), std::memcpy(&st.jumpHeld, statics + 4, 4), std::memcpy(&st.lift, statics + 8, 4);
        std::memcpy(&want.lastJump, after, 1), std::memcpy(&want.jumpHeld, after + 4, 4), std::memcpy(&want.lift, after + 8, 4);
        std::vector<uint8_t> rec(r + o, r + o + SIZE);
        o += SIZE;
        const uint8_t *theirs = r + o;
        o += SIZE;
        uint32_t n;
        take(&n, 4);
        std::vector<uint8_t> buffTypes;
        for (uint32_t i = 0; i < n; i++)
            buffTypes.push_back(r[o]), o += 0x18;
        nativecontrol::Input input;
        input.classSkill = keys[2], input.forward = keys[5], input.back = keys[6], input.left = keys[7], input.right = keys[8];
        input.jump = keys[9], input.dodge = keys[10];
        input.stickX = stick[0], input.stickY = stick[1], input.cameraYaw = yaw;
        for (int row = 0; row < 3; row++)
            std::memcpy(input.camera[row], cam + 4 + row * 16, 12);
        input.blocked = gates[0], input.typing = gates[1];
        std::vector<uint8_t> mount(0x200, 0);
        mount[0x80] = gates[2];
        nativecontrol::World world;
        world.byId = [&](int64_t) -> const uint8_t * { return gates[2] == 0xff ? nullptr : mount.data(); };
        world.hasBuff = [&](int t) {
            for (uint8_t e : buffTypes)
                if (e == (uint8_t)t)
                    return true;
            return false;
        };
        nativecontrol::control(input, st, rec.data(), dt, world);
        int first = -1, diffs = 0;
        for (size_t i = 0; i < SIZE; i++)
            if (rec[i] != theirs[i]) {
                if (first < 0)
                    first = (int)i;
                diffs++;
            }
        const bool statOk = st.lastJump == want.lastJump && st.jumpHeld == want.jumpHeld && st.lift == want.lift;
        const bool ok = diffs == 0 && statOk;
        moving += keys[5] || keys[6] || keys[7] || keys[8] || keys[9] || keys[10];
        if (!ok && calls - same < 12) {
            std::printf("call %zu (ability 0x%x, flags 0x%x, keys %d%d%d%d j%d d%d s%d, gates %d%d): ", calls, rec[0x68],
                        rec[0x5c], keys[5], keys[6], keys[7], keys[8], keys[9], keys[10], keys[2], gates[0], gates[1]);
            if (diffs) {
                float a, t;
                std::memcpy(&a, &rec[first & ~3], 4), std::memcpy(&t, theirs + (first & ~3), 4);
                std::printf("%d bytes differ, first +0x%x (%.9g/%.9g)", diffs, first, a, t);
            }
            if (!statOk)
                std::printf(" statics %d %d %d / %d %d %d", st.lastJump, st.jumpHeld, st.lift, want.lastJump, want.jumpHeld,
                            want.lift);
            std::printf("\n");
        }
        calls++;
        same += ok;
    }
    std::printf("controller oracle: %zu of %zu calls the same (%zu with keys held)\n", same, calls, moving);
    return same == calls ? 0 : 1;
}

// The local player's attack starts (nativeai::startHit / startCharge with the class key
// flag) against the game's 0x60460 and 0x60720 calls (Extract/oracle/attack.bin, CAT1).
static int attackOracle(const std::string &file) {
    constexpr size_t SIZE = nativepose::CREATURE_SIZE;
    std::ifstream in(file, std::ios::binary);
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
    if (b.size() < 4 || std::memcmp(b.data(), "CAT1", 4) != 0) {
        std::printf("%s: not a CAT1 recording\n", file.c_str());
        return 1;
    }
    size_t k = 4, calls = 0, same = 0, started = 0;
    while (k + 4 <= b.size()) {
        uint32_t size;
        std::memcpy(&size, &b[k], 4);
        k += 4;
        if (k + size > b.size())
            break;
        const uint8_t *r = &b[k];
        k += size;
        const int which = r[0];
        const bool force = r[1], result = r[2];
        uint32_t n;
        std::memcpy(&n, r + 4, 4);
        size_t o = 8;
        nativeai::Creature c;
        for (uint32_t i = 0; i < n; i++) {
            std::array<uint8_t, 0x18> e;
            std::memcpy(e.data(), r + o, 0x18);
            c.buffs.push_back(e);
            o += 0x18;
        }
        std::vector<uint8_t> rec(r + o, r + o + SIZE);
        const uint8_t *theirs = r + o + SIZE;
        c.record = rec.data();
        const bool ours = which == 0 ? nativeai::startHit(&c, force) : nativeai::startCharge(&c, force);
        int first = -1, diffs = 0;
        for (size_t i = 0; i < SIZE; i++)
            if (rec[i] != theirs[i]) {
                if (first < 0)
                    first = (int)i;
                diffs++;
            }
        const bool ok = diffs == 0 && ours == result;
        started += result;
        if (!ok && calls - same < 12)
            std::printf("call %zu (%s, flag %d, ability 0x%x -> 0x%x): returned %d/%d, %d bytes differ, first +0x%x\n", calls,
                        which ? "0x60720" : "0x60460", force, r[o + 0x68], theirs[0x68], ours, result, diffs, first);
        calls++;
        same += ok;
    }
    std::printf("attack oracle: %zu of %zu calls the same (%zu started a move)\n", same, calls, started);
    return same == calls ? 0 : 1;
}

static int playerStatsOracle(const std::string &file) {
    constexpr size_t SIZE = nativepose::CREATURE_SIZE;
    std::ifstream in(file, std::ios::binary);
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
    if (b.size() < 4 || std::memcmp(b.data(), "CST1", 4) != 0) {
        std::printf("%s: not a CST1 recording\n", file.c_str());
        return 1;
    }
    size_t k = 4, samples = 0, same = 0;
    static const char *names[] = {"max health", "attack (0)", "spell (0)", "armor",      "resistance",
                                  "critical",   "haste",      "attack (1)", "spell (1)", "stamina factor"};
    while (k + SIZE + 4 <= b.size()) {
        std::vector<uint8_t> r(b.begin() + (long long)k, b.begin() + (long long)(k + SIZE));
        k += SIZE;
        uint32_t n;
        std::memcpy(&n, &b[k], 4);
        k += 4;
        std::vector<std::array<uint8_t, 0x18>> buffs(n);
        for (auto &e : buffs)
            std::memcpy(e.data(), &b[k], 0x18), k += 0x18;
        float game[10];
        std::memcpy(game, &b[k], sizeof game);
        k += sizeof game;
        std::vector<nativerules::Buff> values;
        for (auto &e : buffs) {
            float v;
            std::memcpy(&v, &e[4], 4);
            values.push_back({e[0], v});
        }
        auto has = [&](int t) {
            for (auto &e : buffs)
                if (e[0] == (uint8_t)t)
                    return true;
            return false;
        };
        using nativerules::Weapons;
        const float ours[10] = {nativerules::maxHealth(r.data(), values.data(), values.size()),
                                nativerules::attackPower(r.data(), values.data(), values.size()),
                                nativerules::spellPower(r.data(), values.data(), values.size()),
                                nativerules::armor(r.data(), values.data(), values.size()),
                                nativerules::resistance(r.data(), values.data(), values.size()),
                                nativerules::critChance(r.data(), has, nullptr, nullptr, 0, true),
                                nativerules::attackSpeed(r.data(), has),
                                nativerules::attackPower(r.data(), values.data(), values.size(), Weapons::Both),
                                nativerules::spellPower(r.data(), values.data(), values.size(), Weapons::Both),
                                nativerules::staminaFactor(r.data())};
        const float theirs[10] = {game[0], game[6], game[7], game[3], game[4], game[5], game[8], game[1], game[2], game[9]};
        bool ok = true;
        std::printf("sample %zu (class %d, spec %d, level %d, %u buffs):", samples, r[0x14c], r[0x14d],
                    *reinterpret_cast<int32_t *>(&r[0x144]), n);
        for (int i = 0; i < 10; i++) {
            const bool eq = ours[i] == theirs[i];
            ok = ok && eq;
            if (!eq)
                std::printf(" %s %.6g/%.6g", names[i], ours[i], theirs[i]);
        }
        std::printf("%s\n", ok ? " all the same" : "");
        samples++;
        same += ok;
    }
    std::printf("player stats oracle: %zu of %zu samples the same\n", same, samples);
    return same == samples ? 0 : 1;
}

// The player's stats from the game's own rules (NativeRules on a player record) against
// what the gear surveys measured (Extract/characters/*/gear.txt: each line the player's
// totals with one slot emptied, restored or holding another item, and the item's own
// health). The player stands in the region most of its items come from. The sheet adds up
// both weapons (selector 1). The surveys of 2026-10-20 were taken with the PyroProgression
// mod installed: its levelled gear and per-level bonuses (+0.2 armour and resistance at
// level 1 with nothing worn) are not the game's own rules, so they cannot match these.
static int statsOracle() {
    Assets a;
    if (!a.locate(""))
        return 1;
    namespace fs = std::filesystem;
    size_t lines = 0, same = 0, itemsSame = 0, items = 0;
    std::map<std::string, size_t> wrong;
    int shown = 0;
    static const char *slotNames[] = {"unk", "neck", "chest", "feet", "hands", "shoulder", "weapon_left", "weapon_right",
                                      "ring_left", "ring_right", "pet"};
    auto slotOf = [](const std::string &s) {
        for (int i = 0; i < 11; i++)
            if (s == slotNames[i])
                return i;
        return -1;
    };
    auto fields = [](const std::string &text) { // key=value words
        std::map<std::string, std::string> f;
        std::istringstream in(text);
        std::string w;
        while (in >> w)
            if (auto eq = w.find('='); eq != std::string::npos)
                f[w.substr(0, eq)] = w.substr(eq + 1);
        return f;
    };
    for (auto &dir : fs::directory_iterator(a.gameDir() / "Extract" / "characters")) {
        std::ifstream snap(dir.path() / "snapshot.txt"), gear(dir.path() / "gear.txt");
        if (!snap || !gear)
            continue;
        std::map<std::string, std::string> who;
        for (std::string t; std::getline(snap, t);)
            if (t.rfind("race=", 0) == 0 || t.rfind("class=", 0) == 0 || t.rfind("specialization=", 0) == 0 ||
                t.rfind("level=", 0) == 0 || t.rfind("flags2=", 0) == 0)
                for (auto &[k, v] : fields(t))
                    who[k] = v;
        std::vector<std::string> text;
        for (std::string t; std::getline(gear, t);)
            text.push_back(t);
        using Item = std::array<uint8_t, 0xa0>;
        auto itemOf = [&](std::map<std::string, std::string> &f) {
            Item it{};
            it[0] = (uint8_t)std::atoi(f["category"].c_str());
            const int32_t id = std::atoi(f["id"].c_str());
            const uint32_t mod = (uint32_t)std::strtoul(f["modifier"].c_str(), nullptr, 10);
            int32_t region[2] = {0, 0};
            std::sscanf(f["region"].c_str(), "%d,%d", &region[0], &region[1]);
            const int32_t formula = std::atoi(f["formula"].c_str());
            std::memcpy(&it[4], &id, 4), std::memcpy(&it[8], &mod, 4), std::memcpy(&it[0xc], region, 8);
            it[0x14] = (uint8_t)std::atoi(f["rarity"].c_str());
            std::memcpy(&it[0x18], &formula, 4);
            it[0x1c] = (uint8_t)std::atoi(f["material"].c_str());
            return it;
        };
        std::array<Item, 11> worn{};
        std::map<std::pair<int32_t, int32_t>, int> regions;
        for (auto &t : text)
            if (t.rfind("equipped ", 0) == 0 || t.rfind("inventory ", 0) == 0) {
                auto f = fields(t);
                const Item it = itemOf(f);
                int32_t r[2];
                std::memcpy(r, &it[0xc], 8);
                if (r[0] || r[1])
                    regions[{r[0], r[1]}]++;
                if (t.rfind("equipped ", 0) == 0 && slotOf(f["slot"]) >= 0)
                    worn[(size_t)slotOf(f["slot"])] = it;
            }
        std::pair<int32_t, int32_t> home{0, 0};
        int most = 0;
        for (auto &[r, n] : regions)
            if (n > most)
                most = n, home = r;
        for (auto &t : text) {
            const bool base = t.rfind("baseline ", 0) == 0, restored = t.rfind("restored ", 0) == 0, tried = t.rfind("item ", 0) == 0;
            if (!base && !restored && !tried)
                continue;
            const size_t bar = t.find(" | player ");
            if (bar == std::string::npos)
                continue;
            auto f = fields(t.substr(0, bar));
            auto p = fields(t.substr(bar + 10, t.find(" | ", bar + 10) - (bar + 10)));
            const int slot = slotOf(f["slot"]);
            if (slot < 0)
                continue;
            worldgen::CreatureRecord r = worldgen::newCreature(1);
            r[0x60] = 0;
            const int32_t race = std::atoi(who["race"].c_str()), level = std::atoi(who["level"].c_str());
            std::memcpy(&r[0x64], &race, 4), std::memcpy(&r[0x144], &level, 4);
            const uint32_t flags2 = (uint32_t)std::strtoul(who["flags2"].c_str(), nullptr, 16);
            std::memcpy(&r[0x80], &flags2, 4);
            r[0x14c] = (uint8_t)std::atoi(who["class"].c_str());
            r[0x14d] = (uint8_t)std::atoi(who["specialization"].c_str());
            const int32_t region[2] = {home.first, home.second};
            std::memcpy(&r[0x150], region, 8);
            std::array<Item, 11> now = worn;
            const Item it = itemOf(f);
            now[(size_t)slot] = base ? Item{} : it;
            for (int s = 0; s < 11; s++)
                std::memcpy(&r[0x278 + (size_t)s * 0xa0], now[(size_t)s].data(), 0xa0);
            const float ours[6] = {nativerules::maxHealth(r.data()),
                                   nativerules::attackPower(r.data(), nullptr, 0, nativerules::Weapons::Both),
                                   nativerules::spellPower(r.data(), nullptr, 0, nativerules::Weapons::Both),
                                   nativerules::armor(r.data(), nullptr, 0),
                                   nativerules::resistance(r.data(), nullptr, 0),
                                   nativerules::critChance(r.data(), [](int) { return false; }, nullptr, nullptr, 0, false)};
            static const char *names[6] = {"maxhp", "attack", "spell", "armor", "resistance", "critical"};
            static const float tolerance[6] = {0.006f, 0.0006f, 0.0006f, 0.0006f, 0.0006f, 0.00006f};
            std::string bad;
            for (int k = 0; k < 6; k++) {
                const float game = std::strtof(p[names[k]].c_str(), nullptr);
                if (std::fabs(ours[k] - game) > tolerance[k] * std::max(1.0f, std::fabs(game) / 100.0f)) {
                    char s[96];
                    std::snprintf(s, sizeof s, " %s %.4f/%.4f", names[k], ours[k], game);
                    bad += s;
                    wrong[names[k]]++;
                }
            }
            lines++;
            if (bad.empty())
                same++;
            else if (shown++ < 15)
                std::printf("%s %s %s:%s\n", dir.path().filename().string().c_str(), base ? "empty" : "with", f["slot"].c_str(),
                            bad.c_str());
            if (!base && it[0]) {
                items++;
                const float hp = nativerules::itemHealth(it.data(), r.data()), game = std::strtof(f["item_hp"].c_str(), nullptr);
                if (std::fabs(hp - game) <= 0.0006f * std::max(1.0f, game / 100.0f))
                    itemsSame++;
            }
        }
    }
    std::printf("stats oracle: %zu of %zu survey lines match the game's totals; item health %zu of %zu\n", same, lines,
                itemsSame, items);
    for (auto &[k, n] : wrong)
        std::printf("  %s differs in %zu\n", k.c_str(), n);
    return same == lines ? 0 : 1;
}

// Extract/oracle/interact.bin (CIN1, tools/ZoneExtractor's interact.request): calls of
// 0x2c3710 with interactions to take up. The port (src/NativeInteract.cpp) runs on the
// recorded creature and objects; its record, objects and sounds must match.
static int interactOracle(const std::string &file) {
    constexpr size_t SIZE = nativepose::CREATURE_SIZE;
    std::ifstream in(file, std::ios::binary);
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
    if (b.size() < 4 || std::memcmp(b.data(), "CIN1", 4) != 0) {
        std::printf("%s: not a CIN1 recording\n", file.c_str());
        return 1;
    }
    size_t at = 4, calls = 0, same = 0, seats = 0, doors = 0;
    int shown = 0;
    while (at + 4 <= b.size()) {
        uint32_t n;
        std::memcpy(&n, &b[at], 4);
        at += 4;
        if (at + n > b.size())
            break;
        const uint8_t *r = &b[at];
        at += n;
        size_t k = 0;
        auto take = [&](void *to, size_t len) { std::memcpy(to, r + k, len), k += len; };
        auto get = [&](auto &v) { take(&v, sizeof v); };
        nativeinteract::World w;
        uint8_t client;
        get(client), get(w.timeOfDay);
        w.client = client != 0;
        std::vector<uint8_t> record(SIZE), after(SIZE);
        take(record.data(), SIZE);
        nativeai::Creature me;
        me.record = record.data();
        uint32_t count;
        get(count);
        me.buffs.resize(count);
        for (auto &e : me.buffs)
            take(e.data(), 0x18);
        get(count);
        struct Named {
            int zx, zy, index;
            bool found;
            nativeai::Object before{}, after{};
        };
        std::vector<Named> named;
        std::map<int64_t, std::vector<uint8_t>> others; // (their class is all the step reads)
        std::map<std::pair<int, int>, std::vector<nativeai::Object>> zones;
        for (uint32_t i = 0; i < count; i++) {
            nativeai::Interaction it;
            take(it.data(), 0xc8);
            me.interactions.push_back(it);
            if (it[0xc4] != 5)
                continue;
            Named m;
            std::memcpy(&m.zx, &it[0xa0], 4), std::memcpy(&m.zy, &it[0xa4], 4), std::memcpy(&m.index, &it[0xa8], 4);
            uint8_t found;
            get(found);
            m.found = found != 0;
            if (m.found)
                take(m.before.data(), 0xe0);
            uint8_t hasOther, cls;
            get(hasOther), get(cls);
            if (hasOther) {
                int64_t id;
                std::memcpy(&id, &it[0xb0], 8);
                others[id].assign(SIZE, 0);
                others[id][0x14c] = cls;
            }
            named.push_back(m);
        }
        take(after.data(), SIZE);
        for (auto &m : named) {
            uint8_t found;
            get(found);
            if (found)
                take(m.after.data(), 0xe0);
            if (m.found) { // the zone's list, as long as the index needs, with this object in place
                auto &list = zones[{m.zx, m.zy}];
                if ((int)list.size() <= m.index)
                    list.resize((size_t)m.index + 1);
                list[(size_t)m.index] = m.before;
                const uint32_t type = *reinterpret_cast<const uint32_t *>(m.before.data());
                (type == 0x1c || type == 0x73 || type - 0x1e < 3 || type - 0x5f < 2 ? seats : doors)++;
            }
        }
        get(count);
        std::vector<std::array<uint8_t, 0x30>> sounds(count);
        for (auto &s : sounds)
            take(s.data(), 0x30);
        std::vector<nativeai::Creature> otherSides;
        otherSides.reserve(others.size());
        std::map<int64_t, nativeai::Creature *> byId;
        for (auto &[id, rec] : others) {
            otherSides.emplace_back();
            otherSides.back().record = rec.data();
            byId[id] = &otherSides.back();
        }
        w.byId = [&](int64_t id) -> nativeai::Creature * {
            auto f = byId.find(id);
            return f == byId.end() ? nullptr : f->second;
        };
        w.objects = [&](int zx, int zy) -> std::vector<nativeai::Object> * {
            auto f = zones.find({zx, zy});
            return f == zones.end() ? nullptr : &f->second;
        };
        nativeinteract::step(w, &me);
        calls++;
        std::string why;
        if (std::memcmp(record.data(), after.data(), SIZE) != 0)
            for (size_t o = 0; o < SIZE; o++)
                if (record[o] != after[o]) {
                    why = "record +0x" + std::to_string(o);
                    break;
                }
        for (auto &m : named)
            if (why.empty() && m.found && zones[{m.zx, m.zy}][(size_t)m.index] != m.after)
                why = "object";
        if (why.empty()) {
            auto unpadded = [](std::vector<std::array<uint8_t, 0x30>> v) {
                for (auto &e : v)
                    std::memset(e.data() + 0x24, 0, 4);
                return v;
            };
            if (unpadded(w.sounds) != unpadded(sounds))
                why = "sounds " + std::to_string(w.sounds.size()) + "/" + std::to_string(sounds.size());
        }
        if (!me.interactions.empty() && why.empty())
            why = "list not emptied";
        if (why.empty())
            same++;
        else if (shown++ < 12)
            std::printf("call %zu (ability 0x%x, %u interactions): %s\n", calls - 1, after[0x68], (unsigned)count, why.c_str());
    }
    std::printf("interaction oracle: %zu of %zu calls match (%zu seats, %zu doors and others named)\n", same, calls, seats,
                doors);
    return same == calls ? 0 : 1;
}

// Extract/oracle/items.bin (CIG1, tools/ZoneExtractor's items.request): the game's item
// generators (0x2d15d0, 0x2d0680, 0x2d0fc0) on scripted rand(); the port must give the
// same item and draw as many values.
static int itemOracle(const std::string &file) {
    std::ifstream in(file, std::ios::binary);
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
    if (b.size() < 4 || std::memcmp(b.data(), "CIG1", 4) != 0) {
        std::printf("%s: not a CIG1 recording\n", file.c_str());
        return 1;
    }
    size_t k = 4;
    auto take = [&](void *to, size_t len) { std::memcpy(to, &b[k], len), k += len; };
    auto get = [&](auto &v) { take(&v, sizeof v); };
    nativedrop::World w;
    uint32_t n;
    get(n);
    w.petList.resize(n);
    for (auto &v : w.petList)
        get(v);
    get(n);
    for (uint32_t i = 0; i < n; i++) {
        int32_t key;
        uint32_t m;
        get(key), get(m);
        auto &v = w.petListsByKey[key];
        v.resize(m);
        for (auto &e : v)
            get(e);
    }
    size_t total[3] = {}, same[3] = {};
    int shown = 0;
    while (k + 15 + 0xa0 <= b.size()) {
        uint8_t which, rarity, plain;
        int32_t cls;
        uint32_t seed, calls;
        get(which), get(cls), get(rarity), get(plain), get(seed), get(calls);
        uint8_t expected[0xa0];
        take(expected, sizeof expected);
        uint32_t state = seed ? seed : 0x9e3779b9u, drawn = 0;
        w.rand = [&] { // ZoneExtractor's scripted rand()
            state ^= state << 13, state ^= state >> 17, state ^= state << 5;
            drawn++;
            return (int)(state & 0x7fff);
        };
        uint8_t item[0xa0];
        std::memset(item, 0xcc, sizeof item);
        if (which == 0)
            nativedrop::weapon(w, item, rarity, cls);
        else if (which == 1)
            nativedrop::armour(w, item, rarity, cls, plain != 0);
        else
            nativedrop::petFood(w, item, cls);
        int diff = -1;
        for (int i = 0; i < 0xa0 && diff < 0; i++)
            if (!((i >= 1 && i <= 3) || (i >= 0x15 && i <= 0x17) || i >= 0x9e) && item[i] != expected[i])
                diff = i;
        total[which]++;
        if (diff < 0 && drawn == calls)
            same[which]++;
        else if (shown++ < 12)
            std::printf("generator %d class %d rarity %d plain %d: +0x%x %02x vs %02x, rand %u vs %u\n", which, cls,
                        rarity, plain, diff < 0 ? 0 : diff, diff < 0 ? 0 : item[diff], diff < 0 ? 0 : expected[diff],
                        drawn, calls);
    }
    std::printf("item oracle: weapons %zu/%zu, armour %zu/%zu, specials %zu/%zu\n", same[0], total[0], same[1], total[1],
                same[2], total[2]);
    return same[0] + same[1] + same[2] == total[0] + total[1] + total[2] ? 0 : 1;
}

static int hitOracle(const std::vector<std::string> &files) {
    constexpr size_t SIZE = nativepose::CREATURE_SIZE;
    using Buff = std::array<uint8_t, 0x18>;
    struct Sides {
        std::map<int64_t, float> aggro, damage;
        std::set<int64_t> counted, struck;
    };
    struct Side {
        std::vector<uint8_t> record;
        std::vector<Buff> buffs;
        Sides sides;
    };
    struct Other {
        uint64_t address = 0;
        std::vector<uint8_t> head;
        int64_t lastHit = 0;
        std::map<int64_t, float> aggro;
    };
    size_t compared = 0, same = 0, kills = 0;
    std::map<std::string, size_t> wrong;
    std::map<int, size_t> byOffset;
    std::map<std::string, std::array<size_t, 2>> byCase;
    int shown = 0;
    const int show = [] {
        char v[16] = "";
        return GetEnvironmentVariableA("AI_SHOW", v, sizeof v) ? std::atoi(v) : 12;
    }();
    for (const std::string &file : files) {
        std::ifstream in(file, std::ios::binary);
        std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
        if (b.size() < 4 || std::memcmp(b.data(), "CHT1", 4) != 0) {
            std::printf("%s: not a CHT1 recording\n", file.c_str());
            return 1;
        }
        size_t at = 4;
        while (at + 4 <= b.size()) {
            uint32_t n;
            std::memcpy(&n, &b[at], 4);
            at += 4;
            if (at + n > b.size())
                break;
            const uint8_t *r = &b[at];
            at += n;
            size_t k = 8;
            auto take = [&](void *to, size_t len) { std::memcpy(to, r + k, len), k += len; };
            auto get = [&](auto &v) { take(&v, sizeof v); };
            const bool client = r[0], targetLocal = r[1], attackerLocal = r[2], hasLocal = r[3], result = r[4];
            nativeupkeep::Hit h;
            get(h.target), get(h.attacker), get(h.damage), get(h.critical), get(h.flag), get(h.charge);
            take(h.where, 24), take(h.push, 12);
            get(h.type), get(h.ability), get(h.extra), get(h.last);
            uint8_t hasAttacker;
            get(hasAttacker);
            auto buffsOf = [&] {
                uint32_t c;
                get(c);
                std::vector<Buff> v(c);
                for (auto &e : v)
                    take(e.data(), 0x18);
                return v;
            };
            auto sidesOf = [&] {
                Sides s;
                for (auto *m : {&s.aggro, &s.damage}) {
                    uint32_t c;
                    get(c);
                    for (uint32_t i = 0; i < c; i++) {
                        int64_t id;
                        float v;
                        get(id), get(v);
                        (*m)[id] = v;
                    }
                }
                for (auto *m : {&s.counted, &s.struck}) {
                    uint32_t c;
                    get(c);
                    for (uint32_t i = 0; i < c; i++) {
                        int64_t id;
                        get(id);
                        m->insert(id);
                    }
                }
                return s;
            };
            auto sideOf = [&] {
                Side s;
                s.record.assign(r + k, r + k + SIZE);
                k += SIZE;
                s.buffs = buffsOf();
                s.sides = sidesOf();
                return s;
            };
            auto othersOf = [&] {
                uint32_t c;
                get(c);
                std::vector<Other> v(c);
                for (auto &o : v) {
                    get(o.address);
                    o.head.assign(r + k, r + k + 0x200);
                    k += 0x200;
                    get(o.lastHit);
                    uint32_t m;
                    get(m);
                    for (uint32_t i = 0; i < m; i++) {
                        int64_t id;
                        float f;
                        get(id), get(f);
                        o.aggro[id] = f;
                    }
                }
                return v;
            };
            Side target = sideOf(), attacker;
            if (hasAttacker)
                attacker = sideOf();
            const std::vector<Other> others0 = othersOf();
            uint32_t before[7];
            for (auto &c : before)
                get(c);
            const Side target1 = sideOf();
            Side attacker1;
            if (hasAttacker)
                attacker1 = sideOf();
            const std::vector<Other> others1 = othersOf();
            static const size_t listBytes[7] = {0x18, 0x30, 8, 0x48, 0x30, 0x48, 0x10};
            std::array<std::vector<std::vector<uint8_t>>, 7> gained;
            for (int l = 0; l < 7; l++) {
                uint32_t c;
                get(c);
                for (uint32_t i = before[l]; i < c; i++)
                    gained[l].emplace_back(r + k, r + k + listBytes[l]), k += listBytes[l];
            }
            std::vector<int32_t> draws;
            {
                uint32_t c;
                get(c);
                draws.resize(c);
                for (auto &d : draws)
                    get(d);
            }
            // The world: every creature as recorded (the target and attacker whole), the
            // local player as the flags say.
            std::deque<std::vector<uint8_t>> keep;
            std::deque<nativeai::Creature> creatures;
            std::map<int64_t, nativeai::Creature *> byId;
            nativehit::World w;
            w.client = client;
            static uint8_t elsewhere[16];
            auto make = [&](Side &s) {
                creatures.emplace_back();
                nativeai::Creature &c = creatures.back();
                c.record = s.record.data();
                c.buffs = s.buffs;
                c.aggro = s.sides.aggro, c.damage = s.sides.damage;
                c.counted = s.sides.counted, c.struck = s.sides.struck;
                return &c;
            };
            nativeai::Creature *tc = make(target);
            nativeai::Creature *ac = hasAttacker ? make(attacker) : nullptr;
            const int64_t tid = h.target, aid = h.attacker;
            for (const Other &o : others0) {
                int64_t id;
                std::memcpy(&id, o.head.data() + 8, 8);
                if (id == tid) {
                    w.creatures.push_back(tc);
                    continue;
                }
                if (ac && id == aid) {
                    w.creatures.push_back(ac);
                    continue;
                }
                keep.emplace_back(SIZE, 0);
                std::memcpy(keep.back().data(), o.head.data(), 0x200);
                std::memcpy(keep.back().data() + 0x9e0, &o.lastHit, 8);
                creatures.emplace_back();
                creatures.back().record = keep.back().data();
                creatures.back().aggro = o.aggro;
                w.creatures.push_back(&creatures.back());
            }
            for (nativeai::Creature *c : w.creatures) {
                int64_t id;
                std::memcpy(&id, c->record + 8, 8);
                byId[id] = c;
            }
            byId[tid] = tc;
            if (ac)
                byId[aid] = ac;
            w.local = targetLocal ? tc->record : attackerLocal && ac ? ac->record : hasLocal ? elsewhere : nullptr;
            w.byId = [&](int64_t id) -> nativeai::Creature * {
                auto f = byId.find(id);
                return f == byId.end() ? nullptr : f->second;
            };
            size_t drawn = 0;
            w.rand = [&] { return drawn < draws.size() ? draws[drawn++] : (drawn++, 0); };
            const bool got = nativehit::hit(w, tc, ac, h);
            compared++;
            // The containers' sizes as the records keep them.
            for (nativeai::Creature *c : {tc, ac}) {
                if (!c)
                    continue;
                const uint64_t sizes[5] = {c->buffs.size(), c->counted.size(), c->struck.size(), c->aggro.size(),
                                           c->damage.size()};
                std::memcpy(c->record + 0x988, &sizes[0], 8);
                std::memcpy(c->record + 0x9c0, &sizes[1], 8);
                std::memcpy(c->record + 0x9d0, &sizes[2], 8);
                std::memcpy(c->record + 0xb90, &sizes[3], 8);
                std::memcpy(c->record + 0xba0, &sizes[4], 8);
            }
            std::vector<std::string> bad;
            if (got != result)
                bad.push_back("result");
            auto compareSide = [&](nativeai::Creature *c, const Side &want, const char *who) {
                bool equal = true;
                for (size_t o = 0; o < SIZE; o += 4)
                    if (std::memcmp(c->record + o, &want.record[o], 4) != 0) {
                        equal = false;
                        byOffset[(int)o]++;
                    }
                if (!equal)
                    bad.push_back(std::string(who) + " record");
                if (c->buffs != want.buffs)
                    bad.push_back(std::string(who) + " buffs");
                if (c->aggro != want.sides.aggro || c->damage != want.sides.damage)
                    bad.push_back(std::string(who) + " maps");
                if (c->counted != want.sides.counted || c->struck != want.sides.struck)
                    bad.push_back(std::string(who) + " sets");
            };
            compareSide(tc, target1, "target");
            if (ac)
                compareSide(ac, attacker1, "attacker");
            for (const Other &o : others1) {
                int64_t id;
                std::memcpy(&id, o.head.data() + 8, 8);
                if (id == tid || (ac && id == aid))
                    continue;
                nativeai::Creature *c = byId[id];
                int64_t last;
                std::memcpy(&last, c->record + 0x9e0, 8);
                if (std::memcmp(c->record, o.head.data(), 0x200) != 0 || last != o.lastHit || c->aggro != o.aggro) {
                    bad.push_back("others");
                    break;
                }
            }
            auto asLists = [](const auto &v) {
                std::vector<std::vector<uint8_t>> out;
                for (auto &e : v)
                    out.emplace_back(e.begin(), e.end());
                return out;
            };
            auto unpadded = [](std::vector<std::vector<uint8_t>> v) {
                for (auto &e : v)
                    std::memset(e.data() + 0x24, 0, 4);
                return v;
            };
            auto unpaddedNotes = [](std::vector<std::vector<uint8_t>> v) { // +0x14: the stack's
                for (auto &e : v)
                    std::memset(e.data() + 0x14, 0, 4);
                return v;
            };
            if (unpaddedNotes(asLists(w.aggroNotes)) != unpaddedNotes(gained[0]))
                bad.push_back("aggro notes");
            if (asLists(w.buffNotes) != gained[1])
                bad.push_back("buff notes");
            if (!gained[2].empty())
                bad.push_back("calls");
            // A damage number's padding after its critical byte (+0x15) and its last (+0x47)
            // is whatever the game's stack held.
            auto unpaddedNumbers = [](std::vector<std::vector<uint8_t>> v) {
                for (auto &e : v)
                    e[0x15] = e[0x16] = e[0x17] = e[0x47] = 0;
                return v;
            };
            if (unpaddedNumbers(asLists(w.numbers)) != unpaddedNumbers(gained[3]))
                bad.push_back("numbers");
            if (unpadded(asLists(w.sounds)) != unpadded(gained[4]))
                bad.push_back("sounds");
            if (asLists(w.particles) != gained[5])
                bad.push_back("particles");
            {
                std::vector<std::vector<uint8_t>> kills;
                for (auto &[a, d] : w.kills) {
                    std::vector<uint8_t> e(16);
                    std::memcpy(e.data(), &a, 8), std::memcpy(e.data() + 8, &d, 8);
                    kills.push_back(e);
                }
                if (kills != gained[6])
                    bad.push_back("kills");
            }
            // A death's drop (0x2a6860, the world's `died`) rolls its own rand()s after ours.
            float hpAfter;
            std::memcpy(&hpAfter, &target1.record[0x180], 4);
            const bool killed = !(hpAfter > 0.0f);
            if (killed)
                kills++;
            if (killed ? drawn > draws.size() || !std::equal(draws.begin(), draws.begin() + (long long)std::min(drawn, draws.size()), draws.begin())
                       : drawn != draws.size())
                bad.push_back("draws");
            char kind[96];
            std::snprintf(kind, sizeof kind, "client %d, %s, ability 0x%x, result %d", (int)client,
                          targetLocal ? "on the local player" : attackerLocal ? "by the local player" : "between others",
                          h.ability, (int)result);
            byCase[kind][1]++;
            if (bad.empty()) {
                same++;
                continue;
            }
            byCase[kind][0]++;
            for (auto &s : bad)
                wrong[s]++;
            if (shown++ < show) {
                std::printf("hit %zu (%s, damage %.6g crit %d flag %d type %d):", compared, kind, h.damage, h.critical,
                            h.flag, h.type);
                for (auto &s : bad)
                    std::printf(" [%s]", s.c_str());
                int listed = 0;
                for (size_t o = 0; o < SIZE && listed < 10; o += 4)
                    if (std::memcmp(tc->record + o, &target1.record[o], 4) != 0) {
                        float fa, fb;
                        std::memcpy(&fa, tc->record + o, 4), std::memcpy(&fb, &target1.record[o], 4);
                        std::printf(" t+%zx %.9g/%.9g", o, fa, fb);
                        listed++;
                    }
                std::printf("  numbers %zu/%zu sounds %zu/%zu draws %zu/%zu\n", w.numbers.size(), gained[3].size(),
                            w.sounds.size(), gained[4].size(), drawn, draws.size());
                auto note = [](const std::vector<uint8_t> &e) {
                    int64_t a, b2;
                    float v;
                    std::memcpy(&a, e.data(), 8), std::memcpy(&b2, e.data() + 8, 8), std::memcpy(&v, e.data() + 0x10, 4);
                    std::printf(" (%llx %llx %.6g)", (unsigned long long)a, (unsigned long long)b2, v);
                };
                std::printf("  target hostility %d; aggro notes ours:", target1.record[0x60]);
                for (auto &e : asLists(w.aggroNotes))
                    note(e);
                std::printf(" game:");
                for (auto &e : gained[0])
                    note(e);
                std::printf("\n");
                if (w.numbers.size() == gained[3].size())
                    for (size_t i = 0; i < w.numbers.size(); i++)
                        for (size_t j = 0; j < 0x48; j++)
                            if (w.numbers[i][j] != gained[3][i][j] && j != 0x15 && j != 0x16 && j != 0x17 && j != 0x47)
                                std::printf("  number %zu byte +%zx: %02x/%02x\n", i, j, w.numbers[i][j], gained[3][i][j]);
            }
        }
    }
    std::printf("%zu hits: %zu the same (%zu killed: the drop's own draws are the world's)\n", compared, same, kills);
    for (auto &[c, v] : byCase)
        std::printf("  %s: %zu of %zu differ\n", c.c_str(), v[0], v[1]);
    for (auto &[s, cnt] : wrong)
        std::printf("  %s differ in %zu\n", s.c_str(), cnt);
    for (auto &[o, cnt] : byOffset)
        std::printf("  +%x differs in %zu\n", o, cnt);
    return compared && same == compared ? 0 : 1;
}

// NativeEffects against the game's own effect-object steps (0x2dba40), recorded by
// ZoneExtractor's effects.request into effects.bin (CFX1): the effects, every creature (the
// owners whole), the blocks around each effect, before and after the step, what the
// world's lists gained, the hits it asked for with their results, and every rand() drawn.
static int effectOracle(const std::vector<std::string> &files) {
    constexpr size_t SIZE = nativepose::CREATURE_SIZE;
    using Buff = std::array<uint8_t, 0x18>;
    struct Seen {
        uint64_t address = 0;
        bool owner = false;
        std::vector<uint8_t> record;
        std::vector<Buff> buffs;
        std::map<int64_t, float> aggro, damage;
        std::set<int64_t> counted, struck;
    };
    struct Box {
        int x0 = 0, y0 = 0, z0 = 0, n = 0;
        std::vector<uint8_t> loaded;
        std::vector<nativeai::Block> blocks;
    };
    size_t compared = 0, same = 0, outside = 0;
    std::map<std::string, size_t> wrong;
    std::map<std::string, std::array<size_t, 2>> byKind;
    int shown = 0;
    const int show = [] {
        char v[16] = "";
        return GetEnvironmentVariableA("AI_SHOW", v, sizeof v) ? std::atoi(v) : 12;
    }();
    for (const std::string &file : files) {
        std::ifstream in(file, std::ios::binary);
        std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
        if (b.size() < 4 || std::memcmp(b.data(), "CFX1", 4) != 0) {
            std::printf("%s: not a CFX1 recording\n", file.c_str());
            return 1;
        }
        size_t at = 4;
        while (at + 4 <= b.size()) {
            uint32_t n;
            std::memcpy(&n, &b[at], 4);
            at += 4;
            if (at + n > b.size())
                break;
            const uint8_t *r = &b[at];
            at += n;
            size_t k = 8;
            auto take = [&](void *to, size_t len) { std::memcpy(to, r + k, len), k += len; };
            auto get = [&](auto &v) { take(&v, sizeof v); };
            const bool client = r[0], hasLocal = r[1];
            float dt;
            std::memcpy(&dt, r + 4, 4);
            auto effectsOf = [&] {
                uint32_t c;
                get(c);
                std::list<nativeeffects::Effect> v;
                for (uint32_t i = 0; i < c; i++) {
                    v.emplace_back();
                    take(v.back().data(), 0x80);
                }
                return v;
            };
            auto creaturesOf = [&] {
                uint32_t c;
                get(c);
                std::vector<Seen> v(c);
                for (auto &s : v) {
                    get(s.address);
                    uint8_t owner;
                    get(owner);
                    s.owner = owner;
                    s.record.assign(SIZE, 0);
                    take(s.record.data(), owner ? SIZE : 0x200);
                    uint32_t m;
                    get(m);
                    s.buffs.resize(m);
                    for (auto &e : s.buffs)
                        take(e.data(), 0x18);
                    if (owner) {
                        for (auto *map : {&s.aggro, &s.damage}) {
                            get(m);
                            for (uint32_t i = 0; i < m; i++) {
                                int64_t id;
                                float f;
                                get(id), get(f);
                                (*map)[id] = f;
                            }
                        }
                        for (auto *set : {&s.counted, &s.struck}) {
                            get(m);
                            for (uint32_t i = 0; i < m; i++) {
                                int64_t id;
                                get(id);
                                set->insert(id);
                            }
                        }
                    }
                }
                return v;
            };
            std::list<nativeeffects::Effect> effects = effectsOf();
            std::vector<Seen> before = creaturesOf();
            std::vector<Box> boxes;
            {
                uint32_t c;
                get(c);
                boxes.resize(c);
                for (Box &bx : boxes) {
                    get(bx.x0), get(bx.y0), get(bx.z0), get(bx.n);
                    bx.loaded.resize((size_t)bx.n * bx.n);
                    bx.blocks.resize((size_t)bx.n * bx.n * bx.n);
                    for (int x = 0; x < bx.n; x++)
                        for (int y = 0; y < bx.n; y++) {
                            get(bx.loaded[(size_t)x * bx.n + y]);
                            for (int z = 0; z < bx.n; z++)
                                take(bx.blocks[((size_t)x * bx.n + y) * bx.n + z].data(), 6);
                        }
                }
            }
            static const size_t listBytes[6] = {0x30, 0x48, 0x48, 0x30, 0x18, 0x20};
            uint32_t counts[6];
            for (auto &c : counts)
                get(c);
            const std::list<nativeeffects::Effect> effectsAfter = effectsOf();
            const std::vector<Seen> after = creaturesOf();
            std::array<std::vector<std::vector<uint8_t>>, 6> gained;
            for (int l = 0; l < 6; l++) {
                uint32_t c;
                get(c);
                for (uint32_t i = counts[l]; i < c; i++)
                    gained[l].emplace_back(r + k, r + k + listBytes[l]), k += listBytes[l];
            }
            std::vector<std::vector<uint8_t>> hits;
            {
                uint32_t c;
                get(c);
                for (uint32_t i = 0; i < c; i++)
                    hits.emplace_back(r + k, r + k + 79), k += 79;
            }
            std::vector<uint8_t> results(r + k, r + k + hits.size());
            k += hits.size();
            std::vector<int32_t> draws;
            {
                uint32_t c;
                get(c);
                draws.resize(c);
                for (auto &d : draws)
                    get(d);
            }
            // The world as it was.
            std::deque<nativeai::Creature> creatures;
            std::map<int64_t, nativeai::Creature *> byId;
            nativeeffects::World w;
            w.client = client;
            static uint8_t elsewhere[16];
            w.local = hasLocal ? elsewhere : nullptr;
            for (Seen &s : before) {
                creatures.emplace_back();
                nativeai::Creature &c = creatures.back();
                c.record = s.record.data();
                c.buffs = s.buffs;
                c.aggro = s.aggro, c.damage = s.damage, c.counted = s.counted, c.struck = s.struck;
                w.creatures.push_back(&c);
                int64_t id;
                std::memcpy(&id, s.record.data() + 8, 8);
                byId[id] = &c;
            }
            w.byId = [&](int64_t id) -> nativeai::Creature * {
                auto f = byId.find(id);
                return f == byId.end() ? nullptr : f->second;
            };
            bool out = false;
            auto lookup = [&](int x, int y, int z, bool *loaded) -> nativeai::Block {
                for (const Box &bx : boxes) {
                    const int lx = x - bx.x0, ly = y - bx.y0, lz = z - bx.z0;
                    if (lx < 0 || ly < 0 || lz < 0 || lx >= bx.n || ly >= bx.n || lz >= bx.n)
                        continue;
                    if (loaded)
                        *loaded = bx.loaded[(size_t)lx * bx.n + ly] != 0;
                    return bx.blocks[((size_t)lx * bx.n + ly) * bx.n + lz];
                }
                out = true;
                if (loaded)
                    *loaded = true;
                return nativeai::Block{};
            };
            auto floorOf = [](int64_t v) { return (int)(v >> 16); };
            w.blockAt = [&](int64_t x, int64_t y, int64_t z) { return lookup(floorOf(x), floorOf(y), floorOf(z), nullptr); };
            w.block = [&](int x, int y, int z) { return lookup(x, y, z, nullptr); };
            w.zone = [&](int zx, int zy) {
                for (const Box &bx : boxes)
                    for (int x = std::max(zx * 64, bx.x0); x < std::min(zx * 64 + 64, bx.x0 + bx.n); x++)
                        for (int y = std::max(zy * 64, bx.y0); y < std::min(zy * 64 + 64, bx.y0 + bx.n); y++)
                            return bx.loaded[(size_t)(x - bx.x0) * bx.n + (y - bx.y0)] != 0;
                out = true;
                return true;
            };
            size_t drawn = 0;
            w.rand = [&] { return drawn < draws.size() ? draws[drawn++] : (drawn++, 0); };
            std::vector<std::vector<uint8_t>> asked;
            std::vector<std::array<uint8_t, 0x18>> hitAggro;
            size_t hitResultsDiffer = 0;
            w.hit = [&](const nativeupkeep::Hit &h) {
                std::vector<uint8_t> v;
                auto put = [&](const void *p, size_t len) { v.insert(v.end(), (const uint8_t *)p, (const uint8_t *)p + len); };
                put(&h.target, 8), put(&h.attacker, 8), put(&h.damage, 4), put(&h.critical, 1), put(&h.flag, 1);
                put(&h.charge, 4), put(h.where, 24), put(h.push, 12), put(&h.type, 4), put(&h.ability, 4);
                put(&h.extra, 8), put(&h.last, 1);
                asked.push_back(v);
                // The hit itself is the game's 0x29eb60 (NativeHit), its output in order.
                nativehit::World hw;
                hw.client = client;
                hw.local = w.local;
                hw.creatures = w.creatures;
                hw.byId = w.byId;
                hw.rand = w.rand;
                nativeai::Creature *target = w.byId(h.target), *attacker = h.attacker ? w.byId(h.attacker) : nullptr;
                const bool landed = target && nativehit::hit(hw, target, attacker, h);
                w.sounds.insert(w.sounds.end(), hw.sounds.begin(), hw.sounds.end());
                w.particles.insert(w.particles.end(), hw.particles.begin(), hw.particles.end());
                w.numbers.insert(w.numbers.end(), hw.numbers.begin(), hw.numbers.end());
                w.buffNotes.insert(w.buffNotes.end(), hw.buffNotes.begin(), hw.buffNotes.end());
                hitAggro.insert(hitAggro.end(), hw.aggroNotes.begin(), hw.aggroNotes.end());
                if (asked.size() <= results.size() && landed != (results[asked.size() - 1] != 0))
                    hitResultsDiffer++;
                return landed;
            };
            // A heal's number is applied by the game's 0x29dcf0 (NativeHit).
            std::vector<std::array<uint8_t, 0x18>> aggroNotes;
            std::vector<std::array<uint8_t, 0x30>> applyNotes, applySounds;
            w.apply = [&](const std::array<uint8_t, 0x48> &number) {
                nativehit::World hw;
                hw.client = client;
                hw.local = w.local;
                hw.creatures = w.creatures;
                hw.byId = w.byId;
                hw.rand = w.rand;
                nativehit::apply(hw, number);
                if (GetEnvironmentVariableA("FX_DEBUG", nullptr, 0)) {
                    int64_t from, on;
                    float amount;
                    std::memcpy(&from, number.data(), 8), std::memcpy(&on, number.data() + 8, 8);
                    std::memcpy(&amount, number.data() + 0x10, 4);
                    nativeai::Creature *t = hw.byId(on), *a2 = hw.byId(from);
                    std::printf("  apply %llx -> %llx amount %g: target %s", (unsigned long long)from, (unsigned long long)on,
                                amount, t ? "found" : "missing");
                    if (t) {
                        float hp;
                        uint32_t flags;
                        std::memcpy(&hp, t->record + 0x180, 4), std::memcpy(&flags, t->record + 0x80, 4);
                        std::printf(" hp %g host %d flags %08x aggro %zu", hp, t->record[0x60], flags, t->aggro.size());
                    }
                    std::printf(" attacker %s\n", a2 ? "found" : "missing");
                    for (nativeai::Creature *o : hw.creatures) {
                        int64_t oid;
                        std::memcpy(&oid, o->record + 8, 8);
                        if (o->aggro.empty())
                            continue;
                        std::printf("    creature %llx host %d aggro:", (unsigned long long)oid, o->record[0x60]);
                        for (auto &[k2, v2] : o->aggro)
                            std::printf(" %llx=%g", (unsigned long long)k2, v2);
                        std::printf("\n");
                    }
                }
                aggroNotes.insert(aggroNotes.end(), hw.aggroNotes.begin(), hw.aggroNotes.end());
                applyNotes.insert(applyNotes.end(), hw.buffNotes.begin(), hw.buffNotes.end());
                applySounds.insert(applySounds.end(), hw.sounds.begin(), hw.sounds.end());
            };
            std::set<int> kinds;
            for (auto &e : effects) {
                int32_t kind;
                std::memcpy(&kind, e.data() + 0x68, 4);
                kinds.insert(kind);
            }
            nativeeffects::step(w, effects, dt);
            compared++;
            if (out) // (compared all the same: a ray past the recorded blocks found them empty)
                outside++;
            std::vector<std::string> bad;
            { // (an effect's padding after +0x64, +0x6c and at +0x7c is the game's stack)
                auto unpadded = [](const std::list<nativeeffects::Effect> &l) {
                    std::vector<nativeeffects::Effect> v(l.begin(), l.end());
                    for (auto &e : v) {
                        std::memset(e.data() + 0x65, 0, 3);
                        std::memset(e.data() + 0x6d, 0, 3);
                        std::memset(e.data() + 0x7c, 0, 4);
                    }
                    return v;
                };
                if (unpadded(effects) != unpadded(effectsAfter))
                    bad.push_back("effects");
            }
            for (size_t i = 0; i < after.size() && i < creatures.size(); i++) {
                const size_t len = after[i].owner ? SIZE : 0x200;
                if (std::memcmp(creatures[i].record, after[i].record.data(), len) != 0 || creatures[i].buffs != after[i].buffs) {
                    bad.push_back("creatures");
                    break;
                }
            }
            auto asLists = [](const auto &v) {
                std::vector<std::vector<uint8_t>> o;
                for (auto &e : v)
                    o.emplace_back(e.begin(), e.end());
                return o;
            };
            auto masked = [](std::vector<std::vector<uint8_t>> v, size_t from, size_t len) {
                for (auto &e : v)
                    std::memset(e.data() + from, 0, len);
                return v;
            };
            w.sounds.insert(w.sounds.end(), applySounds.begin(), applySounds.end());
            w.buffNotes.insert(w.buffNotes.end(), applyNotes.begin(), applyNotes.end());
            if (masked(asLists(w.sounds), 0x24, 4) != masked(gained[0], 0x24, 4))
                bad.push_back("sounds");
            if (asLists(w.particles) != gained[1])
                bad.push_back("particles");
            // (padding after a number's critical byte and its last byte: the stack's)
            if (masked(masked(asLists(w.numbers), 0x15, 3), 0x47, 1) != masked(masked(gained[2], 0x15, 3), 0x47, 1))
                bad.push_back("numbers");
            if (asLists(w.buffNotes) != gained[3])
                bad.push_back("buff notes");
            aggroNotes.insert(aggroNotes.begin(), hitAggro.begin(), hitAggro.end());
            if (hitResultsDiffer)
                bad.push_back("hit results");
            if (masked(asLists(aggroNotes), 0x14, 4) != masked(gained[4], 0x14, 4))
                bad.push_back("aggro notes");
            if (asLists(w.blockChanges) != gained[5])
                bad.push_back("block changes");
            // (A hit's `extra` is the effect's address in the game; it isn't compared.)
            auto unextra = [](std::vector<std::vector<uint8_t>> v) {
                for (auto &e : v)
                    std::memset(e.data() + 70, 0, 8);
                return v;
            };
            if (unextra(asked) != unextra(hits))
                bad.push_back("hits");
            if (drawn != draws.size())
                bad.push_back("draws");
            std::string kind = std::string("client ") + (client ? "1" : "0") + ", kinds";
            for (int kk : kinds)
                kind += " " + std::to_string(kk);
            byKind[kind][1]++;
            if (bad.empty()) {
                same++;
                continue;
            }
            byKind[kind][0]++;
            for (auto &s : bad)
                wrong[s]++;
            if (shown++ < show) {
                std::printf("step %zu (%s, dt %.3f, effects %zu->%zu/%zu):", compared, kind.c_str(), dt, effectsAfter.size(),
                            effects.size(), effectsAfter.size());
                for (auto &s : bad)
                    std::printf(" [%s]", s.c_str());
                std::printf(" hits %zu/%zu draws %zu/%zu sounds %zu/%zu particles %zu/%zu\n", asked.size(), hits.size(), drawn,
                            draws.size(), w.sounds.size(), gained[0].size(), w.particles.size(), gained[1].size());
                auto note = [](const auto &e) {
                    int64_t a, b2;
                    float v;
                    std::memcpy(&a, e.data(), 8), std::memcpy(&b2, e.data() + 8, 8), std::memcpy(&v, e.data() + 0x10, 4);
                    std::printf(" (%llx %llx %.6g)", (unsigned long long)a, (unsigned long long)b2, v);
                };
                std::printf("  aggro notes ours:");
                for (auto &e : aggroNotes)
                    note(e);
                std::printf(" game:");
                for (auto &e : gained[4])
                    note(e);
                std::printf("\n");
                for (size_t i = 0; i < after.size() && i < creatures.size(); i++) {
                    const size_t len = after[i].owner ? SIZE : 0x200;
                    for (size_t o = 0; o < len; o += 4)
                        if (std::memcmp(creatures[i].record + o, after[i].record.data() + o, 4) != 0) {
                            float fa, fb;
                            std::memcpy(&fa, creatures[i].record + o, 4), std::memcpy(&fb, after[i].record.data() + o, 4);
                            std::printf("  creature %zu +%zx: %.9g/%.9g\n", i, o, fa, fb);
                        }
                }
                if (effects.size() == effectsAfter.size()) {
                    auto x = effects.begin();
                    auto y = effectsAfter.begin();
                    for (; x != effects.end(); ++x, ++y)
                        for (size_t o = 0; o < 0x80; o += 4)
                            if (std::memcmp(x->data() + o, y->data() + o, 4) != 0) {
                                float fa, fb;
                                std::memcpy(&fa, x->data() + o, 4), std::memcpy(&fb, y->data() + o, 4);
                                std::printf("  effect +%zx: %.9g/%.9g\n", o, fa, fb);
                            }
                }
            }
        }
    }
    std::printf("%zu effect steps: %zu the same (%zu read blocks past those recorded)\n", compared, same, outside);
    for (auto &[k, v] : byKind)
        std::printf("  %s: %zu of %zu differ\n", k.c_str(), v[0], v[1]);
    for (auto &[s, cnt] : wrong)
        std::printf("  %s differ in %zu\n", s.c_str(), cnt);
    return compared && same == compared ? 0 : 1;
}

static int animOracle(const std::vector<std::string> &files, int onlyState) {
    size_t compared = 0, same = 0;
    std::map<int, std::array<size_t, 2>> byField; // field offset: differ count, compared
    std::map<int, std::array<size_t, 2>> byState, byMatrix;
    int shown = 0, shownM = 0;
    size_t sameMatrices = 0, clockTicked = 0;
    for (const std::string &file : files) {
        std::vector<AnimRecord> recs = readAnimRecords(file);
        std::map<uint64_t, size_t> last;
        for (size_t i = 0; i < recs.size(); i++) {
            AnimRecord &next = recs[i];
            auto found = last.find(next.address);
            const size_t prev = found == last.end() ? SIZE_MAX : found->second;
            last[next.address] = i;
            if (prev == SIZE_MAX)
                continue;
            AnimRecord &r = recs[prev];
            int32_t state;
            std::memcpy(&state, &r.creature[nativepose::ANIM + 0xc], 4);
            if (onlyState >= 0 && state != onlyState)
                continue;
            std::vector<uint8_t> work = r.creature;
            nativepose::Models models;
            models.byId = [&](int id) -> const nativepose::Model * {
                auto m = r.ids.find(id);
                return m == r.ids.end() ? nullptr : &r.models[m->second];
            };
            models.forItem = [&](const uint8_t *item) -> const nativepose::Model * {
                for (int k = 0; k < 9; k++)
                    if (item == work.data() + ANIM_ITEM_SLOTS[k])
                        return r.items[k] ? &r.models[r.items[k]] : nullptr;
                return nullptr;
            };
            nativepose::Call call = r.call;
            const bool buffE = r.buffE; // the only buff the recording holds
            call.hasBuff = [buffE](int type) { return type == 0xe && buffE; };
            call.mount = r.mount.empty() ? nullptr : r.mount.data();
            nativepose::pose(work.data(), call, models, nullptr);
            // The probe reads timeGetTime() just before the game's call does, so the game may
            // have seen the next millisecond (the races whose size beats with the clock).
            auto matches = [&]() { // the pose fields, then the matrices and trails
                const uint8_t *ours = &work[nativepose::ANIM], *game = &next.creature[nativepose::ANIM];
                return !std::memcmp(ours + 0x20, game + 0x20, 0xd4 - 0x20) && !std::memcmp(ours + 0x138, game + 0x138, 0x8c8 - 0x138);
            };
            if (!matches()) {
                std::vector<uint8_t> later = r.creature;
                nativepose::Call call2 = call;
                call2.nowMs++;
                std::swap(work, later); // the models' lambdas look for item slots in work
                nativepose::pose(work.data(), call2, models, nullptr);
                if (!matches())
                    std::swap(work, later);
                else
                    clockTicked++;
            }
            compared++;
            bool all = true;
            for (int off = 0x20; off < 0xd4; off += 4) {
                float ours, game;
                std::memcpy(&ours, &work[nativepose::ANIM + off], 4);
                std::memcpy(&game, &next.creature[nativepose::ANIM + off], 4);
                const bool ok = std::memcmp(&ours, &game, 4) == 0;
                byField[off][1]++;
                if (!ok) {
                    byField[off][0]++, all = false;
                    if (shown++ < 12)
                        std::printf("record %zu->%zu state %d ability %d dt %.3f field a+0x%x: ours %.9g game %.9g (was %.9g)\n", prev,
                                    i, state, r.creature[0x68], r.call.dt, off, ours, game,
                                    *(float *)&r.creature[nativepose::ANIM + off]);
                }
            }
            // The matrices (a+0x138..0x5b8, 16 floats each) and the trails (to a+0x8c8).
            bool matricesSame = true;
            for (int off = 0x138; off < 0x8c8; off += 4) {
                const bool ok = std::memcmp(&work[nativepose::ANIM + off], &next.creature[nativepose::ANIM + off], 4) == 0;
                const int key = off < 0x5b8 ? 0x138 + (off - 0x138) / 0x40 * 0x40 : off < 0x5bc ? 0x5b8 : 0x5bc + (off - 0x5bc) / 0xc0 * 0xc0;
                byMatrix[key][1]++;
                if (!ok) {
                    byMatrix[key][0]++, matricesSame = false;
                    if (shownM++ < 8) {
                        float ours, game;
                        std::memcpy(&ours, &work[nativepose::ANIM + off], 4);
                        std::memcpy(&game, &next.creature[nativepose::ANIM + off], 4);
                        std::printf("record %zu->%zu state %d matrix a+0x%x (+%d): ours %.9g game %.9g\n", prev, i, state, key,
                                    (off - key) / 4, ours, game);
                    }
                }
            }
            if (matricesSame)
                sameMatrices++;
            byState[state][1]++;
            if (all)
                same++;
            else
                byState[state][0]++;
        }
    }
    std::printf("%zu with every matrix and trail the same\n", sameMatrices);
    if (clockTicked)
        std::printf("  (%zu of them with the clock a millisecond after the probe read it)\n", clockTicked);
    for (auto &[key, n] : byMatrix)
        if (n[0])
            std::printf("  matrix/trail a+0x%x: %zu of %zu floats differ\n", key, n[0], n[1]);
    std::printf("%zu pose calls compared, %zu with every pose field the same\n", compared, same);
    for (auto &[st, n] : byState)
        std::printf("  state %d: %zu of %zu differ\n", st, n[0], n[1]);
    for (auto &[off, n] : byField)
        if (n[0])
            std::printf("  field a+0x%x: %zu of %zu differ\n", off, n[0], n[1]);
    return 0;
}

// --- The special skill on R and interactions 1, 11, 12, 13 (docs/specs/skills-and-interactions.md) ---

namespace skilltest {

template <class T> void put(nativeai::Creature *c, size_t at, T v) { std::memcpy(c->record + at, &v, sizeof v); }
template <class T> T get(const nativeai::Creature *c, size_t at) {
    T v;
    std::memcpy(&v, c->record + at, sizeof v);
    return v;
}
int64_t idOf(const nativeai::Creature *c) { return get<int64_t>(c, 0x8); }

constexpr int64_t FIRST_MADE = std::numeric_limits<int64_t>::min() + 999; // the first id the handler gives

// A small world of game-layout creatures, with the hooks the interaction handler needs.
struct World {
    std::list<std::pair<worldgen::CreatureRecord, nativeai::Creature>> kept;
    std::map<int64_t, nativeai::Creature *> ids;
    nativeinteract::World w;
    std::vector<int> draws; // what rand() gives, in order
    size_t drawn = 0;
    World() {
        w.construct = [this](int64_t id) { return keep(id); };
        w.added = [this](nativeai::Creature *c) { ids[idOf(c)] = c; };
        w.byId = [this](int64_t id) -> nativeai::Creature * {
            auto f = ids.find(id);
            return f == ids.end() ? nullptr : f->second;
        };
        w.rand = [this] { return drawn < draws.size() ? draws[drawn++] : 0; };
    }
    nativeai::Creature *keep(int64_t id) {
        kept.emplace_back(worldgen::newCreature(id), nativeai::Creature{});
        kept.back().second.record = kept.back().first.data();
        return &kept.back().second;
    }
    // A creature already in the world: its id, hostility kind (+0x60) and place (x, in blocks).
    nativeai::Creature *add(int64_t id, uint8_t kind, double x = 0.0) {
        nativeai::Creature *c = keep(id);
        c->record[0x60] = kind;
        put<int64_t>(c, 0x10, (int64_t)(x * 65536.0));
        put<float>(c, 0x180, 100.0f);
        w.creatures.push_back(c);
        ids[id] = c;
        return c;
    }
    void handle(nativeai::Creature *c, const nativeai::Interaction &in) {
        c->interactions.push_back(in);
        nativeinteract::step(w, c);
    }
};

void specialSkillTable() {
    struct Row {
        uint8_t cls, spec;
        int skill;
    } rows[] = {
        {1, 0, 0x94}, {1, 1, 0x95}, {2, 0, 0x8f}, {2, 1, 0x8e}, {3, 0, 0x2f}, {3, 1, 0x90},
        {4, 0, 0x6d}, {4, 1, 0x8d}, {4, 7, 0x8d}, // any non-zero specialisation is the second
        {0, 0, 0},    {5, 0, 0},    {0xa1, 1, 0},
    };
    for (const Row &r : rows) {
        worldgen::CreatureRecord rec = worldgen::newCreature(1);
        rec[0x14c] = r.cls, rec[0x14d] = r.spec;
        CHECK(nativecontrol::specialSkill(rec.data()) == r.skill);
    }
}

void cooldownTable() {
    World s;
    nativeai::Creature *c = s.add(1, 0);
    const std::pair<int, int> table[] = {
        {0x94, 20000}, {0x95, 30000}, {0x8f, 60000}, {0x8e, 40000}, {0x2f, 40000}, {0x90, 40000}, {0x6d, 40000},
        {0x8d, 60000}, {0x16, 12000}, {0x34, 12000}, {0x61, 12000}, {0x70, 40000}, {0x71, 40000}, {0x72, 40000},
        {0x92, 40000}, {0x32, 16000}, {0x6c, 16000}, {0x33, 16000}, {0x37, 16000}, {0x73, 16000}, {0x49, 15000},
        {0x5f, 30000}, {0x62, 30000}, {0x88, 30000}, {0x6f, 4000},  {0x8c, 60000}, {0x15, 0},     {0x96, 0},
        {0x00, 0},     {0x50, 0},     {0x93, 0},
    };
    for (const auto &[ability, ms] : table)
        CHECK(nativecontrol::cooldown(*c, ability) == ms);
    // Ninjutsu's buff (13) shortens intercept and the shuriken only.
    c->buffs.push_back(nativebuffs::make(13, 0.0f, 20000.0f, 1));
    CHECK(nativecontrol::cooldown(*c, 0x32) == 1800);
    CHECK(nativecontrol::cooldown(*c, 0x6c) == 1800);
    CHECK(nativecontrol::cooldown(*c, 0x94) == 20000);
}

void usableCheck() {
    World s;
    nativeai::Creature *c = s.add(1, 0);
    put<float>(c, 0x188, 1.0f); // mana
    put<float>(c, 0x9a0, 1.0f); // stamina
    CHECK(nativeai::canUse(c, 0x94));
    // 1. Not while the same ability is still running.
    c->record[0x68] = 0x94;
    put<float>(c, 0x6c, 0.0f);
    CHECK(!nativeai::canUse(c, 0x94));
    put<float>(c, 0x6c, 1e6f);
    CHECK(nativeai::canUse(c, 0x94));
    c->record[0x68] = 0;
    // 2. Not stunned (except 0x71), not during intercept (0x32).
    put<float>(c, 0x130, 500.0f);
    CHECK(!nativeai::canUse(c, 0x94));
    CHECK(nativeai::canUse(c, 0x71));
    put<float>(c, 0x130, 0.0f);
    c->record[0x68] = 0x32;
    put<float>(c, 0x6c, 1e6f);
    CHECK(!nativeai::canUse(c, 0x94));
    c->record[0x68] = 0;
    // 4. A cooldown entry: usable only at exactly 0; 0x1d also needs some mana.
    c->cooldowns[0x94] = 0.5f;
    CHECK(!nativeai::canUse(c, 0x94));
    c->cooldowns[0x94] = 0.0f;
    CHECK(nativeai::canUse(c, 0x94));
    put<float>(c, 0x188, 0.0f);
    CHECK(!nativeai::canUse(c, 0x1d));
    CHECK(nativeai::canUse(c, 0x94)); // (every special skill costs no mana)
}

void specialSkillKey() {
    nativecontrol::Input press, release;
    press.ultimate = true;
    { // A warrior's rock fist: an edge on the key, then its cooldown.
        World s;
        nativeai::Creature *c = s.add(1, 0);
        c->record[0x14c] = 1;
        put<float>(c, 0x12c, 300.0f);
        nativecontrol::State st;
        nativecontrol::World w;
        nativecontrol::specialSkillKey(press, st, *c, w);
        CHECK(c->record[0x68] == 0x94 && get<float>(c, 0x6c) == 0.0f && get<float>(c, 0x12c) == 0.0f);
        CHECK(c->cooldowns[0x94] == 20000.0f);
        c->record[0x68] = 0;
        c->cooldowns.clear();
        nativecontrol::specialSkillKey(press, st, *c, w); // still held: nothing
        CHECK(c->record[0x68] == 0);
        nativecontrol::specialSkillKey(release, st, *c, w);
        c->cooldowns[0x94] = 100.0f;
        nativecontrol::specialSkillKey(press, st, *c, w); // pressed again, but cooling down
        CHECK(c->record[0x68] == 0);
        // A window open: the key is not read, and its last state stays.
        nativecontrol::Input blocked = release;
        blocked.blocked = true;
        nativecontrol::specialSkillKey(blocked, st, *c, w);
        CHECK(st.lastUltimate == 1);
    }
    { // A rogue's ninjutsu: a buff on itself; the ability running goes on and its cooldown restarts.
        World s;
        nativeai::Creature *c = s.add(7, 0);
        c->record[0x14c] = 4, c->record[0x14d] = 1;
        c->record[0x68] = 0x16;
        put<float>(c, 0x6c, 1e6f);
        nativecontrol::State st;
        nativecontrol::World w;
        nativecontrol::specialSkillKey(press, st, *c, w);
        CHECK(c->record[0x68] == 0x16);
        CHECK(c->buffs.size() == 1 && c->buffs[0][0] == 13 && nativebuffs::duration(c->buffs[0]) == 20000.0f &&
              nativebuffs::source(c->buffs[0]) == 7);
        CHECK(c->cooldowns[0x8d] == 60000.0f && c->cooldowns[0x16] == 12000.0f);
        CHECK(w.buffNotes.size() == 1 && w.buffNotes[0][0x28] == 1);
        // Camouflage refreshes its own buff rather than adding another.
        c->record[0x14d] = 0;
        nativecontrol::selfBuff(*c, 0x6d, w.buffNotes);
        nativecontrol::selfBuff(*c, 0x6d, w.buffNotes);
        CHECK(c->buffs.size() == 2 && c->buffs[1][0] == 3 && w.buffNotes.back()[0x28] == 0);
    }
}

// The frame around the movement routine (player-control.md 2 and 4).
void controlFrame() {
    nativecontrol::Input forward;
    forward.forward = true;
    { // The lock timer (+0x134) holds the movement routine back; the dodge timer (+0x12c) does not.
        World s;
        nativeai::Creature *c = s.add(1, 0);
        put<uint32_t>(c, 0x5c, 1); // on the ground
        put<float>(c, 0x9a0, 1.0f);
        nativecontrol::State st;
        nativecontrol::World w;
        put<float>(c, 0x134, 50.0f);
        nativecontrol::frame(forward, st, *c, 16.0, w);
        CHECK(get<float>(c, 0x40) == 0.0f && get<float>(c, 0x44) == 0.0f);
        put<float>(c, 0x134, 0.0f);
        put<float>(c, 0x12c, 50.0f);
        nativecontrol::frame(forward, st, *c, 16.0, w);
        CHECK(get<float>(c, 0x44) == -80.0f); // camera yaw 0: forward is -y
        CHECK(get<uint16_t>(c, 0x128) & 0x40); // running unless walking
        nativecontrol::Input walk = forward;
        walk.walk = true, walk.lamp = true;
        nativecontrol::frame(walk, st, *c, 16.0, w);
        CHECK((get<uint16_t>(c, 0x128) & 0x41) == 1); // walking, and the lamp went on
        nativecontrol::frame(walk, st, *c, 16.0, w);
        CHECK(get<uint16_t>(c, 0x128) & 1); // still held: no second toggle
    }
    { // The attack binding (controls.cfg's default: the left mouse button) starts a blow that
      // the ability clock then runs through.
        World s;
        nativeai::Creature *c = s.add(1, 0);
        put<uint32_t>(c, 0x5c, 1); // on the ground
        put<float>(c, 0x6c, 1e6f); // the last ability long over
        nativecontrol::State st;
        nativecontrol::World w;
        nativecontrol::Input press;
        press.attack = GameInput::buttonMatches(gameoptions::defaultControls()[gameoptions::ATTACK], 0);
        CHECK(press.attack);
        nativecontrol::frame(press, st, *c, 16.0, w);
        CHECK(c->record[0x68] != 0 && get<float>(c, 0x6c) == 0.0f);
    }
    { // A charged shot goes off as the strong-attack key is let go.
        World s;
        nativeai::Creature *c = s.add(1, 0);
        c->record[0x68] = 0x19;
        put<float>(c, 0x6c, 300.0f);
        nativecontrol::State st;
        nativecontrol::attack(nativecontrol::Input{}, st, *c);
        CHECK(c->record[0x68] == 0x18 && get<float>(c, 0x6c) == 0.0f);
        // A guard with nothing charged is dropped.
        c->record[0x68] = 0x09;
        nativecontrol::attack(nativecontrol::Input{}, st, *c);
        CHECK(c->record[0x68] == 0 && get<float>(c, 0x158) == 0.0f);
    }
    { // The pet key with no pet near: the call item held up, the call queued and heard.
        World s;
        nativeai::Creature *c = s.add(1, 0);
        put<float>(c, 0x6c, 1e6f); // the ability over
        nativecontrol::State st;
        nativecontrol::World w;
        int heard = -1;
        w.playSound = [&](int sound) { heard = sound; };
        nativecontrol::Input pet;
        pet.pet = true;
        nativecontrol::frame(pet, st, *c, 16.0, w);
        CHECK(c->record[0x68] == 0x9b && c->record[0x1d8] == 0x21 && heard == 0x7c);
        CHECK(c->interactions.size() == 1 && c->interactions.back()[nativeinteract::KIND] == 10);
    }
    { // The aim point: where the ray meets the blocks, pulled onto a creature ahead.
        World s;
        nativeai::Creature *c = s.add(1, 0);
        nativecontrol::Aim aim;
        aim.forward[0] = 1, aim.forward[1] = 0, aim.forward[2] = 0;
        aim.wallDistance = 10;
        int64_t at[3];
        nativecontrol::aimPoint(aim, c->record, at);
        CHECK(at[0] == 10 * 65536 && at[1] == 0 && at[2] == 0);
        aim.aimDepth = 5;
        nativecontrol::aimPoint(aim, c->record, at);
        CHECK(at[0] == 5 * 65536);
        c->record[0x68] = 0x33; // not while teleporting
        nativecontrol::aimPoint(aim, c->record, at);
        CHECK(at[0] == 10 * 65536);
    }
    { // The aim assist turns the camera a little toward a creature to the left.
        World s;
        nativeai::Creature *c = s.add(1, 0), *t = s.add(2, 1);
        put<int64_t>(t, 0xb18, 10 * 65536);
        nativecontrol::AimAssist a;
        a.player = c->record, a.target = t->record;
        a.forward[0] = 1, a.forward[1] = 0, a.forward[2] = 0;
        a.dt = 16.0;
        CHECK(nativecontrol::aimAssist(a) == 0.0); // the stick at rest
        a.stickX = 1;
        const double yaw = nativecontrol::aimAssist(a);
        CHECK(yaw > 0.0 && yaw < 1.0);
    }
}

// The three skills that queue an interaction, on the caster's own list.
void skillInteractions() {
    World s;
    nativeai::Creature *c = s.add(1, 0);
    nativeability::World a;
    a.local = c->record; // the local player's casts count; another player's would not
    auto queued = [&](uint8_t ability, float from, float dt) {
        c->record[0x68] = ability;
        put<float>(c, 0x6c, from);
        c->interactions.clear();
        nativeability::ability(a, c, dt);
        return c->interactions.empty() ? 0 : (int)c->interactions.back()[nativeinteract::KIND];
    };
    CHECK(queued(0x8f, 0.0f, 16.0f) == nativeinteract::SHADOW_CLONE);
    CHECK(queued(0x8f, 16.0f, 16.0f) == 0);
    CHECK(queued(0x95, 0.0f, 16.0f) == nativeinteract::HEROIC_SHOUT);
    CHECK(queued(0x95, 16.0f, 16.0f) == 0);
    // Water bubbles: eight over the effect window.
    int bubbles = 0;
    for (float t = 0.0f; t < 3000.0f; t += 16.0f)
        bubbles += queued(0x90, t, 16.0f) == nativeinteract::WATER_BUBBLE;
    CHECK(bubbles == 8);
    put<uint32_t>(c, 0x80, 0x800); // a clone can't cast another
    CHECK(queued(0x8f, 0.0f, 16.0f) == 0);
    a.local = nullptr; // a player that isn't this world's local one
    CHECK(queued(0x95, 0.0f, 16.0f) == 0);
}

// The ability step's hums come every 200 ms; its tail runs the cooldowns down to 0 and drops
// a combo after 4 s without a landed hit.
void abilityHumsAndTail() {
    World s;
    nativeai::Creature *c = s.add(1, 1); // a creature the server decides for
    nativeability::World a;
    a.rand = [] { return 0; };
    c->record[0x68] = 0x19; // a charging move: a hum on each 200-ms boundary
    int hums = 0;
    for (float t = 0.0f; t < 1000.0f; t += 16.0f) {
        put<float>(c, 0x6c, t);
        a.sounds.clear();
        nativeability::ability(a, c, 16.0f);
        hums += (int)a.sounds.size();
    }
    CHECK(hums == 5);
    c->record[0x68] = 0;
    c->cooldowns[0x33] = 10.0f;
    put<int32_t>(c, 0x70, 3);
    put<float>(c, 0x74, 3990.0f);
    nativeability::ability(a, c, 16.0f);
    float left, since;
    int32_t combo;
    std::memcpy(&left, &c->cooldowns[0x33], 4);
    std::memcpy(&since, c->record + 0x74, 4), std::memcpy(&combo, c->record + 0x70, 4);
    CHECK(left == 0.0f && since == 4006.0f && combo == 0);
}

void interactionRecord() {
    static_assert(sizeof(nativeai::Interaction) == 0xc8);
    static_assert(nativeinteract::KIND == 0xc4 && nativeinteract::TARGET == 0xb0);
    const nativeai::Interaction in = nativeinteract::fresh(nativeinteract::HEROIC_SHOUT);
    CHECK(in[0xc4] == 13);
    for (size_t k = 0xa0; k < 0xa8; k++)
        CHECK(in[k] == 0xff);
    int32_t object;
    int64_t target;
    int16_t one;
    std::memcpy(&object, &in[0xa8], 4), std::memcpy(&target, &in[0xb0], 8), std::memcpy(&one, &in[0xc6], 2);
    CHECK(object == -1 && target == 0 && one == 1);
    // The handler reads the kind at +0xc4 and empties the list.
    World s;
    nativeai::Creature *c = s.add(1, 0);
    s.handle(c, in);
    CHECK(c->interactions.empty() && s.w.sounds.size() == 1);
    // Kind 3 names its creature at +0xb0.
    nativeai::Creature *lift = s.add(55, 1, 3.0);
    put<int32_t>(lift, 0x64, 0xf5);
    nativeai::Interaction activate = nativeinteract::fresh(nativeinteract::ACTIVATE);
    const int64_t id = 55;
    std::memcpy(&activate[0xb0], &id, 8);
    s.handle(c, activate);
    CHECK(lift->record[0x19c] == 1);
    // On a client nothing is handled and the list stays.
    s.w.client = true;
    c->interactions.push_back(in);
    nativeinteract::step(s.w, c);
    CHECK(c->interactions.size() == 1 && s.w.sounds.size() == 1);
}

void bombs() {
    World s;
    nativeai::Creature *user = s.add(1, 0, 10.0);
    s.add(42, 1);
    nativeai::Interaction in = nativeinteract::fresh(nativeinteract::USE_ITEM);
    in[0] = 1; // a bomb: item type 1, subtype 9
    const int32_t bomb = 9, portalStone = 13;
    std::memcpy(&in[4], &bomb, 4);
    s.handle(user, in);
    CHECK(s.w.creatures.size() == 3);
    const nativeai::Creature *b = s.w.creatures.back();
    CHECK(idOf(b) == FIRST_MADE);
    CHECK(get<int32_t>(b, 0x64) == 0x90 && b->record[0x60] == 7);
    CHECK((get<uint32_t>(b, 0x80) & 0x14000) == 0x14000);
    CHECK(get<int64_t>(b, 0x10) == get<int64_t>(user, 0x10));
    CHECK(get<float>(b, 0x180) > 0.0f && get<int64_t>(b, 0x1a0) == 0);
    CHECK(s.drawn == 0);
    s.handle(user, in); // each next one a step lower
    CHECK(idOf(s.w.creatures.back()) == FIRST_MADE - 1);
    s.add(std::numeric_limits<int64_t>::min() + 10, 1); // one already lower still
    s.handle(user, in);
    CHECK(idOf(s.w.creatures.back()) == std::numeric_limits<int64_t>::min() + 9);
    // Other used items do nothing here.
    std::memcpy(&in[4], &portalStone, 4);
    const size_t before = s.w.creatures.size();
    s.handle(user, in);
    CHECK(s.w.creatures.size() == before && user->interactions.empty());
}

void shadowClones() {
    World s;
    nativeai::Creature *caster = s.add(1, 0, 4.0);
    caster->record[0x14c] = 2;
    put<int32_t>(caster, 0x144, 12);
    put<float>(caster, 0x6c, 250.0f);
    caster->record[0x68] = 0x8f;
    nativeai::Creature *pet = s.add(20, 4); // a pet it owns, and a dead bubble: both count
    put<int64_t>(pet, 0x1a0, 1);
    nativeai::Creature *deadBubble = s.add(21, 7);
    put<int64_t>(deadBubble, 0x1a0, 1);
    put<float>(deadBubble, 0x180, 0.0f);
    const nativeai::Interaction in = nativeinteract::fresh(nativeinteract::SHADOW_CLONE);
    s.handle(caster, in);
    CHECK(s.w.creatures.size() == 4);
    const nativeai::Creature *clone = s.w.creatures.back();
    CHECK(idOf(clone) == FIRST_MADE);
    CHECK(clone->record[0x60] == 4 && clone->record[0x14c] == 2 && clone->record[0x68] == 0);
    CHECK(get<float>(clone, 0x6c) == 250.0f && get<int64_t>(clone, 0x10) == get<int64_t>(caster, 0x10));
    CHECK(get<int64_t>(clone, 0x1a0) == 1 && get<int32_t>(clone, 0x144) == 12);
    CHECK(get<uint32_t>(clone, 0xbcc) == 0x480 && (get<uint32_t>(clone, 0x80) & 0x800));
    CHECK(clone->buffs.size() == 1 && clone->buffs[0][0] == 0x0f && nativebuffs::source(clone->buffs[0]) == 0);
    CHECK(std::abs(nativebuffs::strength(clone->buffs[0]) - 0.9761678f) < 1e-6f);
    CHECK(nativebuffs::duration(clone->buffs[0]) == 29285.0f);
    CHECK(s.w.buffNotes.size() == 1 && s.w.buffNotes[0][0x28] == 1);
    // Now it owns three: no more.
    s.handle(caster, in);
    CHECK(s.w.creatures.size() == 4 && caster->interactions.empty());
    // A monster's clone is a monster.
    World m;
    nativeai::Creature *monster = m.add(5, 2);
    m.handle(monster, in);
    CHECK(m.w.creatures.back()->record[0x60] == 1);
}

void waterBubbles() {
    World s;
    nativeai::Creature *caster = s.add(1, 0, -2.0);
    put<int32_t>(caster, 0x144, 9);
    s.draws = {0, 0, 0, 32767, 32767, 32767};
    const nativeai::Interaction in = nativeinteract::fresh(nativeinteract::WATER_BUBBLE);
    s.handle(caster, in);
    CHECK(s.drawn == 3);
    s.handle(caster, in);
    CHECK(s.drawn == 6);
    const nativeai::Creature *small = s.w.creatures[1], *big = s.w.creatures[2];
    CHECK(idOf(small) == FIRST_MADE && idOf(big) == FIRST_MADE - 1);
    CHECK(get<int32_t>(small, 0x64) == 0x11d && small->record[0x60] == 7 && get<int64_t>(small, 0x1a0) == 1);
    CHECK(get<int32_t>(small, 0x144) == 9 && get<int64_t>(small, 0x10) == get<int64_t>(caster, 0x10));
    CHECK(get<float>(small, 0x180) == 1.0f && get<float>(small, 0x188) == 1.0f && get<uint32_t>(small, 0xbcc) == 0x200);
    CHECK(get<uint32_t>(small, 0x80) & 0x800);
    // Power 0.75 to 1.25 of the caster's, size 0.75 to 1.5 of the race's.
    CHECK(std::abs(get<float>(big, 0x184) / get<float>(small, 0x184) - 1.25f / 0.75f) < 1e-5f);
    for (size_t axis = 0; axis < 3; axis++)
        CHECK(std::abs(get<float>(big, 0x84 + 4 * axis) - 2.0f * get<float>(small, 0x84 + 4 * axis)) < 1e-5f);
    CHECK(s.w.sounds.size() == 2);
    int32_t sound;
    float pitch;
    std::memcpy(&sound, &s.w.sounds[0][0x18], 4), std::memcpy(&pitch, &s.w.sounds[0][0x1c], 4);
    CHECK(sound == 0x30 && pitch == 0.8f);
}

void heroicShout() {
    World s;
    nativeai::Creature *shouter = s.add(1, 0);
    nativeai::Creature *ally = s.add(2, 0, 49.9);   // an ally inside 50 blocks
    nativeai::Creature *edge = s.add(3, 0, 50.0);   // exactly 50: out
    nativeai::Creature *angry = s.add(4, 1, 300.0); // far, but angry at that ally
    nativeai::Creature *calm = s.add(5, 1, 10.0);   // near, angry at the ally out of range
    nativeai::Creature *object = s.add(6, 7, 1.0);  // bombs and bubbles are skipped
    angry->aggro[2] = 1.0f;
    calm->aggro[3] = 1.0f;
    calm->aggro[999] = 1.0f; // (one not in the world)
    const nativeai::Interaction in = nativeinteract::fresh(nativeinteract::HEROIC_SHOUT);
    s.handle(shouter, in);
    auto only = [](const nativeai::Creature *c, uint8_t type, float strength, float duration) {
        return c->buffs.size() == 1 && c->buffs[0][0] == type && nativebuffs::strength(c->buffs[0]) == strength &&
               nativebuffs::duration(c->buffs[0]) == duration && nativebuffs::source(c->buffs[0]) == 1;
    };
    CHECK(only(shouter, 0x11, 0.05f, 10000.0f));
    CHECK(only(ally, 0x11, 0.05f, 10000.0f));
    CHECK(edge->buffs.empty());
    CHECK(only(angry, 0x12, 0.0f, 5000.0f));
    CHECK(calm->buffs.empty() && object->buffs.empty());
    CHECK(s.w.buffNotes.size() == 3 && s.w.buffNotes[0][0x28] == 0);
    int32_t sound;
    std::memcpy(&sound, &s.w.sounds.at(0)[0x18], 4);
    CHECK(sound == 0x6b);
    s.handle(shouter, in); // a second shout refreshes the same buffs
    CHECK(ally->buffs.size() == 1 && angry->buffs.size() == 1);
    CHECK(s.drawn == 0);
}

// --- Zone objects and kinds 2, 3, 5, 7, 8, 10 (docs/specs/zone-object-interaction.md) ---

using ItemBytes = std::array<uint8_t, 0xa0>;

template <class T> T field(const uint8_t *p, size_t at) {
    T v;
    std::memcpy(&v, p + at, sizeof v);
    return v;
}
template <class T> void setField(uint8_t *p, size_t at, T v) { std::memcpy(p + at, &v, sizeof v); }

// A zone object of `type` at (x, y, z) blocks.
nativeai::Object zoneObject(int32_t type, double x, double y = 0.0, double z = 0.0) {
    nativeai::Object o{};
    setField<int32_t>(o.data(), 0x00, type);
    setField<int64_t>(o.data(), 0x08, (int64_t)(x * 65536.0));
    setField<int64_t>(o.data(), 0x10, (int64_t)(y * 65536.0));
    setField<int64_t>(o.data(), 0x18, (int64_t)(z * 65536.0));
    return o;
}

// A sound request's id, pitch, volume and x.
struct Heard {
    int32_t id;
    float pitch, volume;
    int64_t x;
};
Heard heard(const nativeinteract::Sound &s) {
    return {field<int32_t>(s.data(), 0x18), field<float>(s.data(), 0x1c), field<float>(s.data(), 0x20),
            field<int64_t>(s.data(), 0x00)};
}

// Kind 5 on a world with one zone (3, 4) of objects.
struct ObjectWorld : World {
    std::vector<nativeai::Object> objects;
    struct Laid {
        ItemBytes item;
        int64_t pos[3];
        float angle;
        int count;
        nativeai::Creature *by;
        const uint8_t *from;
    };
    std::vector<Laid> laid;
    std::vector<std::pair<std::array<int64_t, 3>, int>> rewards;
    std::map<int, ItemBytes> chests; // what object i holds
    ObjectWorld() {
        w.objects = [this](int zx, int zy) { return zx == 3 && zy == 4 ? &objects : nullptr; };
        w.contents = [this](int zx, int zy, int i) -> const uint8_t * {
            auto f = chests.find(i);
            return zx == 3 && zy == 4 && f != chests.end() ? f->second.data() : nullptr;
        };
        w.lay = [this](const uint8_t *item, const int64_t pos[3], float angle, int count, nativeai::Creature *by,
                       const uint8_t *from) {
            Laid l{};
            std::memcpy(l.item.data(), item, l.item.size());
            std::copy(pos, pos + 3, l.pos);
            l.angle = angle, l.count = count, l.by = by, l.from = from;
            laid.push_back(l);
        };
        w.reward = [this](const int64_t pos[3], int tier, const uint8_t *) {
            rewards.push_back({{pos[0], pos[1], pos[2]}, tier});
        };
    }
    nativeai::Interaction use(int index) const {
        nativeai::Interaction in = nativeinteract::fresh(nativeinteract::USE_OBJECT);
        const int32_t where[3] = {3, 4, index};
        std::memcpy(&in[nativeinteract::ZONE_X], where, sizeof where);
        return in;
    }
};

void seatTaken() {
    ObjectWorld s;
    s.objects = {zoneObject(3, 1.0), zoneObject(0x1c, 2.0)};
    nativeai::Creature *sitter = s.add(7, 0, 2.0);
    put<float>(sitter, 0x40, 0.5f), put<float>(sitter, 0x44, -1.0f), put<float>(sitter, 0x48, 0.25f);
    sitter->record[0x68] = 0x50; // sneaking: it ends as the object is used
    put<float>(sitter, 0x18c, 1.0f);
    s.handle(sitter, s.use(1));
    const nativeai::Object &seat = s.objects[1];
    CHECK(field<int64_t>(seat.data(), nativeinteract::OBJECT_USER) == 7);
    CHECK(s.w.changedZones.count({3, 4}) == 1 && s.w.sounds.empty());
    CHECK(sitter->record[0x68] == 0 && get<float>(sitter, 0x18c) == 0.0f);
    CHECK(get<float>(sitter, 0x40) == 0.0f && get<float>(sitter, 0x44) == 0.0f && get<float>(sitter, 0x48) == 0.0f);
    // Someone else finds it taken; sitting again changes nothing.
    nativeai::Creature *other = s.add(8, 0, 2.0);
    s.w.changedZones.clear();
    s.handle(other, s.use(1));
    s.handle(sitter, s.use(1));
    CHECK(field<int64_t>(seat.data(), nativeinteract::OBJECT_USER) == 7 && s.w.changedZones.empty());
    // A creature still in a move cannot sit; one resting can.
    s.objects[1] = zoneObject(0x73, 2.0);
    other->record[0x68] = 0x15;
    put<float>(other, 0x6c, -1.0f);
    s.handle(other, s.use(1));
    CHECK(field<int64_t>(s.objects[1].data(), nativeinteract::OBJECT_USER) == 0);
    other->record[0x68] = 0x57;
    s.handle(other, s.use(1));
    CHECK(field<int64_t>(s.objects[1].data(), nativeinteract::OBJECT_USER) == 8 && other->record[0x68] == 0);
    // A seat out of the list, or in a zone not loaded, does nothing.
    nativeai::Interaction missing = s.use(5);
    s.handle(sitter, missing);
    missing[nativeinteract::ZONE_X] = 9;
    s.handle(sitter, missing);
    CHECK(sitter->interactions.empty() && s.drawn == 0);
}

void doorSwings() {
    ObjectWorld s;
    s.objects = {zoneObject(3, 6.0)};
    nativeai::Object &door = s.objects[0];
    door[nativeinteract::OBJECT_STATE] = 1; // shut
    setField<float>(door.data(), nativeinteract::OBJECT_SWING, 300.0f);
    nativeai::Creature *user = s.add(1, 0, 5.0);
    s.handle(user, s.use(0)); // opens: the swing counts back from where it stood
    CHECK(door[nativeinteract::OBJECT_STATE] == 0);
    CHECK(field<float>(door.data(), nativeinteract::OBJECT_SWING) == 700.0f);
    CHECK(s.w.changedZones.count({3, 4}) == 1);
    CHECK(s.w.sounds.size() == 1);
    const Heard h = heard(s.w.sounds[0]);
    CHECK(h.id == 0x39 && h.pitch == 1.0f && h.volume == 1.0f && h.x == 6 * 65536);
    s.handle(user, s.use(0)); // and shuts again
    CHECK(door[nativeinteract::OBJECT_STATE] == 1 && field<float>(door.data(), nativeinteract::OBJECT_SWING) == 300.0f);
    CHECK(s.w.sounds.size() == 2);
    // A swing past its full length turns back from 0; a nearly finished one from what is left.
    setField<float>(door.data(), nativeinteract::OBJECT_SWING, 1250.5f);
    s.handle(user, s.use(0));
    CHECK(field<float>(door.data(), nativeinteract::OBJECT_SWING) == 0.0f);
    setField<float>(door.data(), nativeinteract::OBJECT_SWING, 999.75f);
    s.handle(user, s.use(0));
    CHECK(field<float>(door.data(), nativeinteract::OBJECT_SWING) == 0.25f);
    // 0x2d3a10's sounds by type; asking for the state it is in does nothing.
    std::vector<nativeinteract::Sound> sounds;
    auto soundOf = [&](int32_t type, uint8_t from, uint8_t to) {
        nativeai::Object o = zoneObject(type, 0.0);
        o[nativeinteract::OBJECT_STATE] = from;
        sounds.clear();
        const bool changed = nativeinteract::openOrShut(o, to, sounds);
        CHECK(changed == (from != to) && o[nativeinteract::OBJECT_STATE] == to);
        return sounds.empty() ? -1 : heard(sounds[0]).id;
    };
    CHECK(soundOf(5, 1, 0) == 0x39 && soundOf(8, 1, 0) == 0x36 && soundOf(0x10, 0, 1) == 0x36);
    CHECK(soundOf(0x11, 1, 0) == -1 && soundOf(0x11, 0, 1) == 0x38);
    CHECK(soundOf(0x12, 1, 0) == -1 && soundOf(0x12, 0, 1) == 0x37);
    CHECK(soundOf(0x0c, 1, 0) == 0x39 && soundOf(0x16, 1, 0) == 0x39 && soundOf(3, 1, 1) == -1);
    // Objects that are neither seats nor doors and take nothing: nothing.
    s.objects[0] = zoneObject(0x40, 6.0);
    s.objects[0][nativeinteract::OBJECT_STATE] = 1;
    s.w.sounds.clear();
    s.handle(user, s.use(0));
    CHECK(s.objects[0][nativeinteract::OBJECT_STATE] == 1 && s.w.sounds.empty());
}

void itemTaken() {
    ObjectWorld s;
    ItemBytes key{};
    key[0] = 0x0b, key[0x14] = 3; // its rarity: the reward's tier
    const int32_t subtype = 0x11;
    std::memcpy(&key[4], &subtype, 4);
    s.objects = {zoneObject(0x16, 10.0, 20.0, 1.0)};
    nativeai::Object &chest = s.objects[0];
    chest[nativeinteract::OBJECT_STATE] = 1; // still waiting for its item
    std::memcpy(chest.data() + nativeinteract::OBJECT_ITEM, key.data(), key.size());
    ItemBytes held{};
    held[0] = 5;
    s.chests[0] = held;
    nativeai::Creature *user = s.add(1, 0, 9.0);
    nativeai::Interaction in = s.use(0);
    ItemBytes wrong = key;
    wrong[0x1c] = 1; // another material
    std::memcpy(&in[0], wrong.data(), wrong.size());
    s.handle(user, in);
    CHECK(chest[nativeinteract::OBJECT_STATE] == 1 && s.laid.empty() && s.rewards.empty() && s.w.changedZones.empty());
    std::memcpy(&in[0], key.data(), key.size());
    s.draws = {100, 200, 300};
    s.handle(user, in);
    CHECK(chest[nativeinteract::OBJECT_STATE] == 0 && s.w.changedZones.count({3, 4}) == 1 && s.drawn == 3);
    CHECK(s.w.sounds.size() == 1 && heard(s.w.sounds[0]).id == 0x39);
    // What it held, at a random spot around it: the first draw to y, the second to x.
    CHECK(s.laid.size() == 1 && s.laid[0].item[0] == 5 && s.laid[0].count == 1 && !s.laid[0].by);
    auto away = [](int r) { return (int64_t)((1.0f - ((float)r + (float)r) / 32767.0f) * 65536.0f); };
    CHECK(s.laid[0].pos[0] == 10 * 65536 + away(200) && s.laid[0].pos[1] == 20 * 65536 + away(100));
    CHECK(s.laid[0].pos[2] == 65536 && s.laid[0].angle == 300.0f * 360.0f / 32767.0f);
    // Then the reward, as rich as the item handed in, where the chest stands.
    CHECK(s.rewards.size() == 1 && s.rewards[0].second == 3 && s.rewards[0].first[0] == 10 * 65536);
    // Given once: a second time nothing happens.
    s.handle(user, in);
    CHECK(s.laid.size() == 1 && s.rewards.size() == 1 && s.drawn == 3);
    // An empty chest only opens; a locked door (type 3) gives no reward.
    ObjectWorld d;
    d.objects = {zoneObject(3, 1.0)};
    d.objects[0][nativeinteract::OBJECT_STATE] = 1;
    std::memcpy(d.objects[0].data() + nativeinteract::OBJECT_ITEM, key.data(), key.size());
    nativeai::Creature *opener = d.add(1, 0);
    nativeai::Interaction open = d.use(0);
    std::memcpy(&open[0], key.data(), key.size());
    d.handle(opener, open);
    CHECK(d.objects[0][nativeinteract::OBJECT_STATE] == 0 && d.laid.empty() && d.rewards.empty() && d.drawn == 0);
    // Item equality: the attributes in use count, the padding and the unused ones don't.
    ItemBytes a = key, b = key;
    a[0x9d] = b[0x9d] = 1;
    a[0x1d] = 4, b[0x1d] = 4;
    a[0x21] = 9; // past the one attribute in use
    a[0x15] = 7; // padding after the rarity
    CHECK(nativeinteract::sameItem(a.data(), b.data()));
    b[0x1e] = 1;
    CHECK(!nativeinteract::sameItem(a.data(), b.data()));
}

void innSleep() {
    ObjectWorld s;
    nativeai::Creature *guest = s.add(1, 0, 3.0);
    nativeai::Creature *keeper = s.add(30, 4, 4.0);
    keeper->record[0x14c] = 0x84;
    nativeai::Interaction in = s.use(0);
    const int64_t id = 30;
    std::memcpy(&in[nativeinteract::TARGET], &id, 8);
    s.w.day = 12;
    s.w.timeOfDay = 12.0f * 3600000.0f; // noon: no sleep
    s.handle(guest, in);
    CHECK(s.w.day == 12 && s.w.sleepNotices.empty());
    s.w.timeOfDay = 23.0f * 3600000.0f;
    s.handle(guest, in);
    CHECK(s.w.day == 13 && s.w.timeOfDay == 25200000.0f && s.w.sleepNotices.size() == 1);
    CHECK(s.w.sounds.size() == 1);
    const Heard h = heard(s.w.sounds[0]);
    CHECK(h.id == 0x1f && h.pitch == 1.0f && h.volume == 0.75f && h.x == 3 * 65536);
    s.w.timeOfDay = 6.0f * 3600000.0f; // 6:00 sharp is still night
    s.handle(guest, in);
    CHECK(s.w.day == 14);
}

void groundItems() {
    World s;
    std::vector<nativeinteract::GroundItem> lying(2);
    s.w.groundItems = [&](int zx, int zy) { return zx == 0 && zy == 0 ? &lying : nullptr; };
    nativeai::Creature *picker = s.add(5, 0, 1.0);
    auto lay = [](nativeinteract::GroundItem &g, uint8_t type, int32_t subtype, double x, uint8_t flags) {
        g.fill(0);
        g[0] = type;
        setField<int32_t>(g.data(), 4, subtype);
        setField<int64_t>(g.data(), 0xa0, (int64_t)(x * 65536.0));
        g[0xc4] = flags;
        setField<int16_t>(g.data(), 0xd4, 3);
    };
    auto pick = [&](int index, const nativeinteract::GroundItem &g) {
        nativeai::Interaction in = nativeinteract::fresh(nativeinteract::PICK_UP);
        std::memcpy(&in[0], g.data(), 0xa0);
        const int32_t where[3] = {0, 0, index};
        std::memcpy(&in[nativeinteract::ZONE_X], where, sizeof where);
        s.handle(picker, in);
    };
    lay(lying[0], 2, 1, 4.0, 0);
    const nativeinteract::GroundItem before = lying[0];
    s.draws = {32767};
    pick(0, before);
    CHECK(s.drawn == 1 && s.w.sounds.size() == 1);
    Heard h = heard(s.w.sounds[0]);
    CHECK(h.id == 0x30 && h.pitch == 32767.0f * 0.1f / 32767.0f + 1.0f && h.volume == 1.0f && h.x == 65536);
    CHECK(s.w.pickUps.size() == 1);
    const nativeinteract::PickUpNotice &n = s.w.pickUps[0];
    CHECK(field<int64_t>(n.data(), 0) == 5 && n[8] == 2 && field<int32_t>(n.data(), 0xc) == 1);
    CHECK(field<int16_t>(n.data(), 0xa8) == 3 && field<int64_t>(n.data(), 0xb0) == 4 * 65536 && field<int32_t>(n.data(), 0xc8) == 0);
    // The slot it leaves is empty.
    CHECK(s.w.changedGroundZones.count({0, 0}) == 1 && lying[0][0] == 0);
    CHECK(field<int32_t>(lying[0].data(), 0xd0) == -1 && field<int16_t>(lying[0].data(), 0xd4) == 1);
    CHECK(field<uint64_t>(lying[0].data(), 0xb8) == ~0ull && field<int64_t>(lying[0].data(), 0xd8) == 0);
    // One that stays where it lies (flag 2) is still given; the shiny 0x0b:0x25 sounds its own.
    lay(lying[1], 0x0b, 0x25, -2.0, 2 | 0x80);
    s.w.changedGroundZones.clear();
    s.draws = {0};
    s.drawn = 0;
    pick(1, lying[1]);
    h = heard(s.w.sounds.back());
    CHECK(h.id == 0x2d && h.pitch == 2.0f && lying[1][0] == 0x0b && s.w.changedGroundZones.empty());
    CHECK(s.w.pickUps.size() == 2 && field<int32_t>(s.w.pickUps[1].data(), 0xc8) == (int32_t)(int8_t)0x82);
    // Out of reach (more than 4 blocks), or not the item named: nothing, and no draw.
    lay(lying[1], 2, 1, 5.5, 0);
    s.drawn = 0;
    pick(1, lying[1]);
    nativeinteract::GroundItem other = before;
    other[0] = 3;
    lay(lying[0], 2, 1, 1.0, 0);
    pick(0, other);
    CHECK(s.drawn == 0 && s.w.pickUps.size() == 2 && lying[0][0] == 2);
}

void dropAndPet() {
    // Kind 8: the item laid up to a block away, by the dropper, with the drop sound.
    ObjectWorld s;
    nativeai::Creature *u = s.add(1, 0, 8.0);
    nativeai::Interaction in = nativeinteract::fresh(nativeinteract::DROP);
    in[0] = 4;
    s.draws = {0, 32767};
    s.handle(u, in);
    CHECK(s.drawn == 2 && s.laid.size() == 1 && s.laid[0].item[0] == 4);
    CHECK(s.laid[0].pos[0] == 8 * 65536 + 65536 && s.laid[0].pos[1] == -65536 && s.laid[0].pos[2] == 0);
    CHECK(s.laid[0].angle == 0.0f && s.laid[0].count == 1 && s.laid[0].by == u && s.laid[0].from == u->record);
    CHECK(s.w.sounds.size() == 1 && heard(s.w.sounds[0]).id == 0x3c);
    // Kind 10: a pet beyond 200 blocks comes to its owner; it forgets its fights.
    nativeai::Creature *pet = s.add(40, 4, 300.0);
    put<int64_t>(u, 0x9d8, 40);
    put<int64_t>(u, 0x9e0, 77);
    pet->aggro[9] = 1.0f;
    u->damage[9] = 5.0f;
    s.handle(u, nativeinteract::fresh(nativeinteract::CALL_PET));
    CHECK(pet->aggro.empty() && get<int64_t>(pet, 0x10) == 8 * 65536);
    CHECK(u->damage.empty() && get<int64_t>(u, 0x9e0) == 0);
    put<int64_t>(pet, 0x10, (int64_t)(150.0 * 65536.0)); // near enough: it stays
    s.handle(u, nativeinteract::fresh(nativeinteract::CALL_PET));
    CHECK(get<int64_t>(pet, 0x10) == (int64_t)(150.0 * 65536.0));
    // No pet: the owner's side is cleared all the same.
    put<int64_t>(u, 0x9d8, 999);
    put<int64_t>(u, 0x9e0, 3);
    s.handle(u, nativeinteract::fresh(nativeinteract::CALL_PET));
    CHECK(get<int64_t>(u, 0x9e0) == 0 && s.drawn == 2);
}

void gateOpener() {
    World s;
    std::vector<nativeai::Object> zone00 = {zoneObject(0x10, 5.0), zoneObject(0x10, 9.0), zoneObject(3, 1.0)};
    zone00[0][nativeinteract::OBJECT_STATE] = zone00[1][nativeinteract::OBJECT_STATE] = 1;
    zone00[2][nativeinteract::OBJECT_STATE] = 1;
    std::set<std::pair<int, int>> asked;
    s.w.objects = [&](int zx, int zy) -> std::vector<nativeai::Object> * {
        asked.insert({zx, zy});
        return zx == 0 && zy == 0 ? &zone00 : nullptr;
    };
    nativeai::Creature *u = s.add(1, 0, 0.5);
    nativeai::Creature *keeper = s.add(2, 1, 10.0), *farKeeper = s.add(3, 1, 25.0);
    put<int32_t>(keeper, 0x64, 0xf6), put<int32_t>(farKeeper, 0x64, 0xf6);
    nativeai::Interaction in = nativeinteract::fresh(nativeinteract::GATE_OPENER);
    in[0] = 0x18;
    const int32_t other = 4, opener = 5;
    std::memcpy(&in[4], &other, 4);
    s.handle(u, in);
    CHECK(keeper->record[0x19c] == 0 && zone00[0][nativeinteract::OBJECT_STATE] == 1 && asked.empty());
    std::memcpy(&in[4], &opener, 4);
    s.handle(u, in);
    CHECK(keeper->record[0x19c] == 1 && farKeeper->record[0x19c] == 0);
    CHECK(zone00[0][nativeinteract::OBJECT_STATE] == 0 && zone00[1][nativeinteract::OBJECT_STATE] == 1);
    CHECK(zone00[2][nativeinteract::OBJECT_STATE] == 1 && s.w.changedZones.count({0, 0}) == 1);
    // The zones within 20 blocks of x = 0.5 and y = 0: -1 and 0 each way.
    CHECK(asked == (std::set<std::pair<int, int>>{{-1, -1}, {-1, 0}, {0, -1}, {0, 0}}));
    // The keeper's call, then the gate's own sound and the opener's.
    CHECK(s.w.sounds.size() == 3 && heard(s.w.sounds[0]).id == 0x70 && heard(s.w.sounds[1]).id == 0x36);
    CHECK(heard(s.w.sounds[1]).x == 5 * 65536 && heard(s.w.sounds[2]).id == 0x70 && heard(s.w.sounds[2]).x == 32768);
    s.handle(u, in); // all done: the keeper is awake and the gate open
    CHECK(s.w.sounds.size() == 3 && s.drawn == 0);
}

void residentsAndArena() {
    ObjectWorld s;
    nativeai::Creature *u = s.add(1, 0);
    nativeai::Creature *master = s.add(50, 4, 2.0);
    master->record[0x14c] = 0xa1;
    nativeai::Interaction in = nativeinteract::fresh(nativeinteract::ACTIVATE);
    int64_t id = 50;
    std::memcpy(&in[nativeinteract::TARGET], &id, 8);
    s.handle(u, in);
    CHECK(master->record[0x19c] == 1);
    s.handle(u, in); // a round under way: left alone
    CHECK(master->record[0x19c] == 1);
    // A resident whose task is done: thanked once, with a reward and what it carries.
    nativeai::Creature *resident = s.add(60, 4, 3.0);
    put<uint16_t>(resident, 0x128, 0x101);
    put<int32_t>(resident, 0x144, 7);
    resident->inventory.resize(2);
    resident->inventory[0].resize(2);
    resident->inventory[1].resize(1);
    for (auto &tab : resident->inventory)
        for (auto &entry : tab)
            entry[4] = 2, setField<int32_t>(entry.data(), 0, 6);
    std::set<int64_t> rewarded;
    s.draws.assign(11, 0);
    s.w.rewarded = &rewarded;
    id = 60;
    std::memcpy(&in[nativeinteract::TARGET], &id, 8);
    s.handle(u, in);
    CHECK(resident->record[0x19c] == 0xff && get<uint16_t>(resident, 0x128) == 1);
    CHECK(s.rewards.size() == 1 && s.rewards[0].second == 4 && rewarded.count(60) == 1);
    CHECK(s.laid.size() == 3 && s.laid[2].count == 6 && s.laid[2].item[0] == 2 && s.drawn == 2 + 3 * 3);
}

} // namespace skilltest

static void specialSkillsAndInteractions() {
    skilltest::specialSkillTable();
    skilltest::cooldownTable();
    skilltest::usableCheck();
    skilltest::specialSkillKey();
    skilltest::controlFrame();
    skilltest::skillInteractions();
    skilltest::abilityHumsAndTail();
    skilltest::interactionRecord();
    skilltest::bombs();
    skilltest::shadowClones();
    skilltest::waterBubbles();
    skilltest::heroicShout();
    skilltest::seatTaken();
    skilltest::doorSwings();
    skilltest::itemTaken();
    skilltest::innSleep();
    skilltest::groundItems();
    skilltest::dropAndPet();
    skilltest::gateOpener();
    skilltest::residentsAndArena();
}

// Lore, tales and artifacts (Lore.h, worldgen/Planner.h; docs/specs/lore-and-artifacts.md).
static void loreAndArtifacts() {
    worldgen::setWorldSeed(123);
    using talkgen::xorshift;
    // The planner's artifacts (spec 5.1): region (0, 0)'s counter starts at R(0x46d7c2) = 5986;
    // an artifact made there is a Stone (id 3) with modifier X(5987).
    CHECK(worldgen::random(0x46d7c2) == 5986u);
    const worldgen::Item made = worldgen::makeArtifact(-3, 5, 5986);
    CHECK(made.category == 23 && made.id == 3 && made.modifier == 1600304309u && made.region.x == -3 && made.region.y == 5 &&
          made.rarity == 0 && made.material == 0 && made.realm == worldgen::placeStyle(-3, 5));
    // Every role-14 place of a region hides its artifact; every region hides at least one;
    // other places hide none.
    for (int rx = -2; rx <= 2; rx++)
        for (int ry = -2; ry <= 2; ry++) {
            int hidden = 0, places = 0;
            for (const auto &[x, y, role] : worldgen::regionRoles({rx, ry}).roles) {
                places++;
                const worldgen::Item a = worldgen::placeArtifact(x, y);
                if (role != 14) {
                    CHECK(a.category == 0);
                    continue;
                }
                hidden++;
                CHECK(a.category == lore::ARTIFACT_CATEGORY && a.id >= 0 && a.id < 7 && a.region.x == x && a.region.y == y &&
                      a.rarity == 0 && a.realm == worldgen::placeStyle(x, y));
            }
            CHECK(places == 0 || hidden >= 1);
        }

    // The realm of a place (spec 4.3): one of nine draws of its 16 x 16 block of places.
    {
        static const int32_t SALTS[9] = {0x1e44622, 0x1e44621, 0x1e44623, 0x1e445e9, 0x1e4465b,
                                         0x1e445e8, 0x1e445ea, 0x1e4465a, 0x1e4465c};
        std::map<uint32_t, int> realms;
        int checked = 0;
        for (int32_t x = 32; x < 48; x++)
            for (int32_t y = -16; y < 0; y++) {
                if (worldgen::bigCell(x, y))
                    continue;
                const uint32_t draw = worldgen::random((uint32_t)x + 57u * (uint32_t)y + 0x4f7cffu) % 30;
                const uint32_t expected = worldgen::random(2u + 57u * (uint32_t)-1 + (uint32_t)SALTS[draw <= 8 ? draw : 0]);
                CHECK(worldgen::placeStyle(x, y) == expected);
                realms[worldgen::placeStyle(x, y)]++;
                checked++;
            }
        CHECK(checked > 100 && realms.size() <= 9);
        // A big place answers for its even corner.
        for (int32_t x = -40; x < 40; x++)
            for (int32_t y = -40; y < 40; y++)
                if (worldgen::bigCell(x, y))
                    CHECK(worldgen::placeStyle(x, y) == worldgen::placeStyle(worldgen::floorDiv(x, 1) * 2, worldgen::floorDiv(y, 1) * 2));
    }

    // Carried artifacts (spec 5.2): one level and one skill point a stack, by modifier mod 7.
    {
        auto artifact = [](uint32_t modifier) {
            worldgen::Item a;
            a.category = 23, a.modifier = modifier;
            return a;
        };
        worldgen::Item glider;
        glider.category = 24, glider.id = 1;
        const lore::ArtifactProgress p = lore::artifactProgress({artifact(7), artifact(15), artifact(14), glider, artifact(13)});
        CHECK(p.level == 5);
        CHECK(p.skills[lore::CLIMBING] == 2 && p.skills[lore::SWIMMING] == 1 && p.skills[lore::LIGHT_RADIUS] == 1 && p.skills[lore::DIVING] == 0);
        CHECK(lore::artifactProgress({}).level == 1);
    }

    // Lore buildings and the order Examine looks in (spec 2.3).
    std::vector<lore::Cell> loreCells;
    lore::Cell tombstone{INT32_MIN, 0};
    for (int32_t cx = 0; cx < 400 && loreCells.size() < 40; cx++)
        for (int32_t cy = 0; cy < 400; cy++) {
            const int object = worldgen::cellObject(cx, cy);
            const bool lore = object == 13 || object == 37 || object == 55 || object == 69 || object == 70;
            CHECK(lore::isLoreBuilding({cx, cy}) == lore);
            if (lore)
                loreCells.push_back({cx, cy});
            if (object == 37 && tombstone[0] == INT32_MIN)
                tombstone = {cx, cy};
        }
    CHECK(!loreCells.empty());
    CHECK(lore::examinable(118) && lore::examinable(124) && lore::examinable(126) && lore::examinable(130) && !lore::examinable(127) &&
          !lore::examinable(22));
    CHECK(lore::cellOf((int64_t)-1, (int64_t)16 * 65536 * 3 + 5) == (lore::Cell{-1, 3}));
    CHECK(lore::placeOf({-1, 64}) == (lore::Cell{-1, 1}));
    int examined = 0;
    for (const lore::Cell &building : loreCells) {
        CHECK(lore::searchOrder(building) == std::vector<lore::Cell>{building});
        const lore::Cell beside{building[0] + 1, building[1] + 1};
        if (lore::isLoreBuilding(beside))
            continue;
        const std::vector<lore::Cell> order = lore::searchOrder(beside);
        CHECK(order.size() == 9 && order[0] == (lore::Cell{building[0], building[1]}) && order[1] == (lore::Cell{building[0], building[1] + 1}) &&
              order[3] == (lore::Cell{building[0] + 1, building[1]}) && order[8] == (lore::Cell{building[0] + 2, building[1] + 2}));
        const lore::Cell first = *std::find_if(order.begin(), order.end(), [](const lore::Cell &c) { return lore::isLoreBuilding(c); });
        const std::optional<lore::Reading> reading = lore::examine(beside);
        CHECK(reading && reading->cell == first && reading->tale == lore::tale(first) && reading->tale.size() > 4 &&
              reading->tale[0] == "define_realm" && reading->tale[2] == std::to_string((int32_t)lore::realmOf(first)));
        examined++;
    }
    CHECK(examined > 0);
    std::printf("lore: %zu lore buildings, %d examined from beside them, a tombstone %s\n", loreCells.size(), examined,
                tombstone[0] != INT32_MIN ? "found" : "not found");
    if (tombstone[0] != INT32_MIN) { // a crypt's tombstone tells who rests there
        const std::vector<std::string> t = lore::tale(tombstone);
        CHECK(t.size() > 7 && t[3] == "define_mob" && std::find(t.begin(), t.end(), "new_page") != t.end());
    }

    // The tales' shapes (spec 3.3-3.5).
    {
        const uint32_t realm = 61234;
        worldgen::Item item = made;
        const std::vector<std::string> dungeon = lore::artifactTale(realm, item, {5, -3}, 41, 7);
        const std::vector<std::string> expected = {"define_realm", std::to_string(talkgen::realmKind(realm)), "61234", "define_item", "23", "3",
                                                   "1600304309", "0", "0",
                                                   std::vector<std::string>{"there_is_a_legend_about_an_artifact", "the_item_was_a_mighty_artifact",
                                                                            "there_is_a_mighty_artifact"}[xorshift(7) % 3],
                                                   "define_location", "5", "-3", "41", "it_is_said_to_be_located_in_the_dungeon",
                                                   "mark_location", "5", "-3"};
        CHECK(dungeon == expected);
        CHECK(lore::artifactTale(realm, item, {5, -3}, 8, 7)[14] == "it_is_said_to_be_located_in_the_location");
        for (uint32_t seed = 0; seed < 200; seed++) {
            const std::vector<std::string> story = lore::realmStory(realm, seed, 3, 777);
            CHECK(story.size() >= 7 && story[0] == "define_realm" && story[3] == "define_ruler" &&
                  story[4] == std::to_string(talkgen::rulerTitle(realm)));
            if (xorshift(seed) % 7 == 6)
                CHECK(story.back() == (talkgen::perishedRealm(realm) ? "the_ruler_was_buried_in" : "the_ruler_lives_in"));
            const std::vector<std::string> grave = lore::tombstoneTale(realm, seed);
            CHECK(grave[3] == "define_mob" && grave[4] == std::to_string((int32_t)xorshift(seed)) &&
                  grave[5] == std::to_string(talkgen::nationModel(realm, seed)));
        }
    }

    // The lore book (spec 4.1, 4.4): read cells join the used cells and their realm's lore,
    // once each; it is rebuilt from the used cells alone.
    {
        std::set<lore::Cell> used;
        lore::Book book;
        const lore::Cell chest{loreCells[0][0], loreCells[0][1] + 1000};
        lore::Credit c = lore::credit(loreCells[0], used, book);
        CHECK(c.newlyUsed && c.newPiece && c.pieces == 1 && c.realm == lore::realmOf(loreCells[0]));
        c = lore::credit(loreCells[0], used, book);
        CHECK(!c.newlyUsed && !c.newPiece && c.pieces == 1);
        for (size_t i = 1; i < loreCells.size(); i++)
            lore::credit(loreCells[i], used, book);
        if (!lore::isLoreBuilding(chest)) {
            c = lore::credit(chest, used, book);
            CHECK(c.newlyUsed && !c.newPiece && used.count(chest));
        }
        CHECK(lore::rebuild(used) == book);
        size_t pieces = 0;
        for (const auto &[realm, cells] : book)
            pieces += cells.size();
        CHECK(pieces == loreCells.size());
        CHECK(lore::percent(3) == 30 && lore::percent(11) == 110);
    }

    // Every tale plays through the conversations' playback: each of its words is a line of the
    // game's dictionary or a command the playback knows.
    Assets assets;
    Speech speech;
    std::string why;
    if (!assets.locate("") || !speech.load(assets.gameDir() / "dict_en.xml", why))
        return;
    std::mt19937 random(7);
    auto playable = [&](const std::vector<std::string> &tokens) {
        TalkScript script;
        script.tokens = tokens;
        const TalkPlay play = playScript(script, speech, random);
        if (!play.unknown.empty())
            std::printf("lore tale: unknown token %s\n", play.unknown.c_str());
        return play.unknown.empty() && !play.lines.empty();
    };
    for (const lore::Cell &cell : loreCells)
        CHECK(playable(lore::tale(cell)));
    for (uint32_t seed = 0; seed < 300; seed++)
        for (uint32_t realm : {61234u, 7u, 99999u, 4242u})
            CHECK(playable(lore::realmStory(realm, seed, 3, 777)) && playable(lore::tombstoneTale(realm, seed)) &&
                  playable(lore::artifactTale(realm, made, {5, -3}, seed % 2 ? 41 : 8, seed)));
}

// The map's markers (MapMarks.h; docs/specs/map-markers.md).
static void mapMarkers() {
    using namespace mapmarks;
    // Sizes by zoom (unit: blocks a map unit) and the map's scale.
    CHECK(placeScale(12, 4, 1.0f) == 1.0f && placeScale(2, 64, 1.0f) == SMALLEST_PLACE && placeScale(14, 16, 1.0f) == 1.0f &&
          placeScale(3, 8, 1.0f) == 0.5f);
    CHECK(missionSize(128, 1.0f).shown && missionSize(128, 1.0f).scale == SMALLEST && !missionSize(256, 1.0f).shown &&
          missionSize(32, 1.0f).scale == 1.0f);
    CHECK(landSize(4096, 1.0f).shown && landSize(4096, 1.0f).scale == SMALLEST);
    CHECK(pointSize(64, 1.0f).shown && !pointSize(128, 1.0f).shown && playerSize(1024, 1.0f).shown);
    CHECK(namesShown(64) && !namesShown(128));
    // Icons and colours.
    CHECK(std::string(placeIcon(14)) == "npc" && std::string(placeIcon(13)) == "home" && std::string(placeIcon(12)) == "shrine");
    CHECK(missionIcon(0) == nullptr && std::string(missionIcon(1)) == "gnome" && std::string(missionIcon(15)) == "enemy" &&
          std::string(missionIcon(14)) == "destination" && std::string(missionIcon(25)) == "enemy");
    CHECK(levelColour(-5) == glm::vec4(0.7f, 0.7f, 0.7f, 1) && levelColour(2) == glm::vec4(0.3f, 0.45f, 1, 1) && levelColour(9) == glm::vec4(1, 1, 0, 1));
    CHECK(landColour(13) == glm::vec4(0.7f, 0.2f, 0.2f, 1) && landColour(20) == glm::vec4(1));
    // A mission marker: shown when named (discovered, near) or known.
    MissionFacts f;
    f.kind = 1, f.level = 2, f.unit = 64;
    CHECK(!missionLook(f).shown);
    f.discovered = true;
    MissionLook look = missionLook(f);
    CHECK(look.shown && look.named && look.icon == nullptr && look.colour == glm::vec4(1) && look.stars == 0);
    f.unit = 128;
    CHECK(!missionLook(f).shown); // too far to name it, and not known
    f.known = true;
    look = missionLook(f);
    CHECK(look.shown && !look.named && std::string(look.icon) == "gnome" && look.colour == levelColour(2) && look.scale == SMALLEST);
    f.unit = 256;
    CHECK(!missionLook(f).shown); // smaller than 0.3
    f.unit = 16;
    look = missionLook(f);
    CHECK(look.named && look.stars == 3 && !look.solved && !look.note);
    f.showRarity = false;
    CHECK(missionLook(f).stars == 0);
    f.completed = true;
    CHECK(missionLook(f).solved);
    f.known = false;
    CHECK(missionLook(f).solved && missionLook(f).icon == nullptr); // solved does not need known
    f.kind = 14, f.known = true;
    CHECK(missionLook(f).reportsHover);
    CHECK(revealedByLore(14, false, 10) && !revealedByLore(14, false, 9) && !revealedByLore(14, true, 12) && !revealedByLore(13, false, 12));
    // The "NEW" note blinks 600 ms on, 600 off, while its timer lives and the mission is not completed.
    CHECK(!noteShown(0) && !noteShown(599) && noteShown(600) && noteShown(1199) && !noteShown(1200) && noteShown(1800));
    f.completed = false, f.newNote = true, f.clockMs = 700;
    CHECK(missionLook(f).note);
    f.clockMs = 1300;
    CHECK(!missionLook(f).note);
    f.clockMs = 700, f.completed = true;
    CHECK(!missionLook(f).note);
    NoteTimers timers{{{1, 2}, 100.0f}, {{3, 4}, 20.0f}};
    tickNotes(timers, 20.0f);
    CHECK(timers.size() == 1 && timers.begin()->second == 80.0f);
    CHECK(TALK_NOTE_MS == 180000.0f && LORE_NOTE_MS == 18000.0f);
    CHECK(bob(0) == 5.0f);

    // The tombstone: five minutes, counted down, gone within 20 blocks, held while flying.
    CHECK(TOMBSTONE_MS == 300000.0f);
    CHECK(tombstoneLeft(300000.0f, 16.0f, 400.0) == 299984.0f && tombstoneLeft(300000.0f, 16.0f, 399.0) == 0.0f &&
          tombstoneLeft(10.0f, 16.0f, 1e6) == 0.0f);
    CHECK(tombstoneCounts(0) && !tombstoneCounts(0x98) && !tombstoneCounts(0x9c) && !tombstoneCounts(0x9d));

    // The world map's creature markers.
    CHECK(showsCreature(0, 0, 0, true) && !showsCreature(0, 0x9d, 0, false) && showsCreature(3, 0, 0x100, false) &&
          !showsCreature(3, 0, 0x100, true) && !showsCreature(3, 0, 0, false));
    CHECK(std::abs(creatureMarkerScale(1.0f) - 0.5f) < 1e-6f && std::abs(creatureMarkerScale(0.5f) - 1.0f) < 1e-6f);
    CHECK(markerPulse(0.0, true) == 1.0f && markerPulse(1000.0, false) == 0.0f && markerPulse(3.14159265 / 0.006, true) < 1e-6f);
    const glm::dvec3 pulled = pullToSquare({300.0, 0.0, 60.0}, 100.0);
    CHECK(std::abs(pulled.x - 100.0) < 1e-9 && std::abs(pulled.z - 20.0) < 1e-9);
    CHECK(pullToSquare({50.0, -400.0, 0.0}, 100.0) == glm::dvec3(12.5, -100.0, 0.0));
    CHECK(pullToSquare({99.9, 0.0, 1.0}, 100.0) == glm::dvec3(99.9, 0.0, 1.0));

    // Hover: the nearest hit wins, and a later, nearer hit leaves the fields it does not write.
    Marker flight, building, distant;
    flight.at = {100, 100}, flight.block = {4096, 2048, 30}, flight.flightTarget = flight.reportsHover = flight.flightPoint = true;
    building.at = {105, 100}, building.block = {-24, 40, 12}, building.reportsHover = building.reportsCell = true;
    distant.at = {300, 300}, distant.block = {1, 1, 1}, distant.reportsHover = true;
    const Hover h = hover({flight, building, distant}, {104, 100}, 1.0f, nullptr);
    CHECK(h.best == 1 && h.flight == glm::ivec2(4096, 2048) && !h.flightToPlayer && h.chosen == glm::ivec2(-24, 40) &&
          h.cell == (Cell{-2, 2}) && h.cursor == glm::ivec3(-24, 40, 12) && !h.shrine);
    CHECK(hover({distant}, {104, 100}, 1.0f, nullptr).best == -1);
    Marker labelled = distant;
    labelled.label = "Jakor Rocks";
    CHECK(hover({labelled}, {360, 300}, 1.0f, [](const Marker &) { return 140.0f; }).best == 0); // within half the label's width
    std::vector<Marker> all{flight, building, distant};
    enlargeTargets(all, h);
    CHECK(all[0].enlarged && all[1].enlarged && !all[2].enlarged);
}

// Writes a .plx stream with plain (unsealed) tag names, as a test document.
struct PlxWriter {
    std::vector<uint8_t> bytes;
    std::map<std::string, int32_t> ids;
    void put(int32_t v) { bytes.insert(bytes.end(), (uint8_t *)&v, (uint8_t *)&v + 4); }
    void tag(const std::string &name) { // (a definition the first time a name is used)
        if (!ids.count(name)) {
            const int32_t id = (int32_t)ids.size() + 1;
            ids[name] = id;
            put(0), put(8 + (int32_t)name.size()), put(id), put((int32_t)name.size());
            bytes.insert(bytes.end(), name.begin(), name.end());
        }
        put(ids[name]);
    }
    size_t open(const std::string &name) {
        tag(name);
        put(0);
        return bytes.size();
    }
    void close(size_t start) {
        const int32_t size = (int32_t)(bytes.size() - start);
        std::memcpy(&bytes[start - 4], &size, 4);
    }
    template <class T> void leaf(const std::string &name, const T &value) {
        tag(name);
        put((int32_t)sizeof(T));
        bytes.insert(bytes.end(), (const uint8_t *)&value, (const uint8_t *)&value + sizeof(T));
    }
    void text(const std::string &name, const std::string &s) {
        tag(name);
        put(4 + (int32_t)s.size()), put((int32_t)s.size());
        bytes.insert(bytes.end(), s.begin(), s.end());
    }
};

// The interface library (docs/specs/plasma-format-and-scene.md, plasma-shapes-and-text.md).
static void plasmaLibrary() {
    // The seal: key derivation and the spec's worked decryption (gui.plx at 0x58).
    CHECK(plx::keyOf("PlasmaXGraphics") == 0xFB3A260B5EC7CD5Cull);
    CHECK(plx::keyOf("PlasmaGraphics") == 0x62F15BACC28BC932ull);
    {
        const uint8_t sealed[] = {0x80, 0x98, 0x9F, 0xB0, 0x32, 0x3F, 0xD2};
        const std::vector<uint8_t> name = plx::unseal(sealed, sizeof sealed, plx::keyOf("PlasmaXGraphics"));
        CHECK(std::string(name.begin(), name.end()) == "Texture");
        CHECK(plx::unseal(sealed, sizeof sealed, 0) == std::vector<uint8_t>(sealed, sealed + sizeof sealed));
    }
    { // A small document: definitions met at every depth, the root standing for the target node.
        PlxWriter w;
        w.leaf("PlasmaGraphics", (int32_t)1);
        w.text("Seal", "PlasmaGraphics");
        size_t t = w.open("Transformation");
        size_t a = w.open("Transformation.translation");
        w.leaf("Attribute.frame", glm::vec2(5, 6));
        w.leaf("Attribute.frame", glm::vec2(0, 0));
        w.close(a);
        w.close(t);
        size_t n = w.open("Node");
        w.leaf("Node.transformation", (int32_t)0);
        w.leaf("Node.child", (int32_t)1);
        w.close(n);
        n = w.open("Node");
        w.text("Node.name", "a");
        w.leaf("Node.transformation", (int32_t)0);
        w.close(n);
        plx::Document d;
        std::string why;
        CHECK(plx::read(w.bytes, d, {}, &why));
        CHECK(d.version == 1 && d.nodes.size() == 2 && d.nodes[0].children == std::vector<int>{1});
        CHECK(d.nodes[0].transformation == -1 && d.find("a") == 1 && d.nodes[1].transformation == 0);
        CHECK(d.transformations.size() == 1 && plasma::translation(d.transformations[0]) == glm::vec2(5, 6));
        w.bytes[0x26] = 2; // (a newer version: the PlasmaGraphics chunk's value, spec 1.3)
        CHECK(!plx::read(w.bytes, d, {}, &why));
    }

    // The local matrix: Translate(t) . Translate(p) . Rx . Ry . Rz . D . Translate(-p), z flattened.
    {
        plasma::Transformation t;
        t.translation = {10, 20}, t.pivot = {3, 4}, t.rotation = {0, 0, 90};
        t.deformation = glm::scale(glm::mat4(1), glm::vec3(2, 1, 1));
        const glm::vec4 p = t.local() * glm::vec4(5, 4, 0, 1); // (2, 0) from the pivot, deformed to (4, 0), turned to (0, 4)
        CHECK(std::abs(p.x - 13) < 1e-4f && std::abs(p.y - 28) < 1e-4f);
        t.rotation = {30, 0, 0};
        const glm::mat4 m = t.local();
        CHECK(m[0][2] == 0 && m[1][2] == 0 && m[3][2] == 0 && m[2][2] == 1); // (the z row flattened)
    }

    // The curve between keys: a Bezier whose handles sit a quarter along the chord (smoothness
    // 0, a mild ease, not straight) or along the neighbour chords (smoothness 1).
    CHECK(plasma::bezier(0, 0, 1, 1, 0, 0, 0) == 0 && plasma::bezier(0, 0, 1, 1, 0, 0, 1) == 1);
    CHECK(std::abs(plasma::bezier(0, 0, 1, 1, 0, 0, 0.5f) - 0.5f) < 1e-6f);
    CHECK(std::abs(plasma::bezier(0, 0, 1, 1, 0, 0, 0.25f) - 0.2265625f) < 1e-6f);
    // (smoothness 1 between evenly spaced keys: the handles a quarter of the neighbour chord of 2
    // along, half the segment, so the curve is (0, 0.5, 0.5, 1))
    CHECK(std::abs(plasma::bezier(-1, 0, 1, 2, 1, 1, 0.25f) - 0.296875f) < 1e-6f);
    { // Playing a sequence whose first key is not at 0: it starts from the live value.
        auto frame = [](float v) {
            plx::Chunk c{"Attribute.frame", std::vector<uint8_t>(4), {}};
            std::memcpy(c.bytes.data(), &v, 4);
            return c;
        };
        auto field = [](const char *name, int32_t v) {
            plx::Chunk c{name, std::vector<uint8_t>(4), {}};
            std::memcpy(c.bytes.data(), &v, 4);
            return c;
        };
        plx::Chunk name{"Attribute.sequence.name", {1, 0, 0, 0, 's'}, {}};
        plx::Chunk late{"Attribute.sequence.key", {}, {field("Attribute.sequence.key.frame", 3), field("Attribute.sequence.key.time", 500)}};
        plx::Chunk early{"Attribute.sequence.key", {}, {field("Attribute.sequence.key.frame", 2), field("Attribute.sequence.key.time", 100)}};
        plx::Chunk sequence{"Attribute.sequence", {}, {name, late, early}};
        const plx::Chunk attribute{"Display.blurRadius", {}, {frame(0), frame(-99), frame(10), frame(20), sequence}};
        plasma::AttributeAnimation anim(attribute);
        CHECK(anim.play("s") && anim.active.size() == 3 && anim.active[0].slot == 1 && anim.active[1].time == 100);
        std::vector<float> v;
        CHECK(plasma::sampleSequence(attribute, "s", 50, v) && v.size() == 1 && std::abs(v[0] - 5) < 1e-5f);
        CHECK(plasma::sampleSequence(attribute, "s", 900, v) && v[0] == 20);
        CHECK(!plasma::sampleSequence(attribute, "other", 50, v));
        CHECK(anim.evaluate(600) && !anim.evaluate(700)); // (the end pose once, then no change)
    }

    // Shapes: a quad without subdivision is a fan; in outline mode the tessellator fills it.
    {
        plasma::SmoothMesh m;
        m.positions = {{0, 0}, {2, 0}, {2, 1}, {0, 1}};
        m.texCoords.resize(4), m.colors.assign(4, glm::vec4(1)), m.strokeColors.assign(4, glm::vec4(1));
        m.strokeWidths.assign(4, 1.0f), m.vertexFlags.assign(4, 0), m.vertexParameters.assign(4, 0.0f);
        m.faces = {{0, 1, 2, 3}};
        m.subdivisions = 0;
        plasma::ShapeDrawing d = plasma::build(m);
        CHECK(d.filled && !d.stroked && d.fill.indices == std::vector<uint32_t>({0, 1, 2, 0, 2, 3}));
        CHECK(d.minimum == glm::vec2(0, 0) && d.maximum == glm::vec2(2, 1));
        auto area = [](const plasma::Triangles &t) {
            float sum = 0;
            for (size_t i = 0; i + 2 < t.indices.size(); i += 3) {
                const glm::vec2 a = t.vertices[t.indices[i]].position, b = t.vertices[t.indices[i + 1]].position,
                                c = t.vertices[t.indices[i + 2]].position;
                sum += std::abs((b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y)) * 0.5f;
            }
            return sum;
        };
        m.flags = plasma::MESH_OUTLINE;
        d = plasma::build(m);
        CHECK(d.fill.indices.size() == 6 && std::abs(area(d.fill) - 2) < 1e-4f);
        m.positions = {{0, 0}, {2, 0}, {2, 1}, {1, 1}, {1, 2}, {0, 2}}; // an L: concave
        m.faces = {{0, 1, 2, 3, 4, 5}};
        m.texCoords.resize(6);
        m.colors.assign(6, glm::vec4(1)), m.strokeColors.assign(6, glm::vec4(1)), m.strokeWidths.assign(6, 1.0f);
        m.vertexFlags.assign(6, plasma::POINT_SHARP), m.vertexParameters.assign(6, 0.0f);
        CHECK(std::abs(area(plasma::build(m).fill) - 3) < 1e-4f);
        // Subdivided once, each polygon corner becomes a quad: 6 quads, 2 triangles each.
        m.flags = 0, m.subdivisions = 1;
        d = plasma::build(m);
        CHECK(d.fill.indices.size() == 6 * 6 && d.fill.vertices.size() == 6 + 6 + 1);
        CHECK(std::abs(area(d.fill) - 3) < 1e-3f); // (sharp corners: the outline stays)
        // A stroke along the closed outline: a strip of 2 pairs per visit with no joins needed.
        m.flags = plasma::MESH_STROKE | plasma::MESH_NO_FILL, m.subdivisions = 0;
        m.vertexFlags.assign(6, plasma::POINT_SMOOTH);
        d = plasma::build(m);
        CHECK(!d.filled && d.stroked && d.stroke.vertices.size() == 7 * 2 && d.stroke.indices.size() == 14 * 3);
        // The loops of a face list: a square's outline and a segment's.
        CHECK(plasma::outlineLoops({{0, 1, 2}, {0, 2, 3}}) == std::vector<std::vector<int>>({{0, 1, 2, 3}}));
        CHECK(plasma::outlineLoops({{4, 7}}) == std::vector<std::vector<int>>({{4, 7}}));
    }

    // Text pen arithmetic: kerning and spacing except after the last character; whole raster
    // pixels in the scalable mode.
    CHECK(plasma::nextPen(3, 7.6f, 0, 1, false, true) == 11.0f);
    CHECK(plasma::nextPen(3, 7.6f, 0, 1, true, true) == 10.0f);
    CHECK(std::abs(plasma::nextPen(3, 7.6f, 0.5f, 1, false, false) - 12.1f) < 1e-5f);
    CHECK(std::abs(plasma::nextPen(3, 7.6f, 0.5f, 1, true, false) - 10.6f) < 1e-5f);

    Assets assets;
    if (!assets.locate("")) {
        std::printf("skip: Steam Cube World not found (Plasma documents)\n");
        return;
    }
    const fs::path dir = assets.gameDir();
    const std::vector<uint64_t> keys = {plx::keyOf("PlasmaXGraphics")};
    plx::Document gui;
    std::string why;
    CHECK(plx::read(dir / "gui.plx", gui, keys, &why, plx::LOAD_SCALABLE_TEXT));
    int texts = 0;
    for (const plx::Object &o : gui.shapes)
        texts += o.kind == "TextShape";
    CHECK(gui.version == 1 && gui.nodes.size() == 685 && gui.transformations.size() == 677 && gui.displays.size() == 677);
    CHECK(gui.shapes.size() == 659 && texts == 160 && gui.widgets.size() == 144 && gui.textures.size() == 4);
    CHECK(gui.pageWidth == 760 && gui.pageHeight == 530 && gui.dpi == 72);
    CHECK(gui.nodes[0].children.size() == 141);
    for (auto [file, nodes] : {std::pair{"icons.plx", 1119}, std::pair{"cursor.plx", 4}, std::pair{"quest-tag.plx", 93}}) {
        plx::Document d;
        CHECK(plx::read(dir / file, d, keys, &why) && (int)d.nodes.size() == nodes);
    }
    { // (the reader's own keys do not open the game's seals)
        plx::Document d;
        CHECK(!plx::read(dir / "cursor.plx", d, {}, &why));
    }

    // The bars' trees (spec Verification): names, order and placement.
    auto names = [&](int node) {
        std::vector<std::string> out;
        for (int c : gui.nodes[(size_t)node].children)
            out.push_back(gui.nodes[(size_t)c].name);
        return out;
    };
    auto placed = [&](int node, glm::vec2 translation, glm::vec2 pivot) {
        const plx::Node &n = gui.nodes[(size_t)node];
        if (n.transformation < 0)
            return false;
        const plasma::Transformation t = plasma::transformationOf(gui.transformations[(size_t)n.transformation]);
        return glm::all(glm::lessThan(glm::abs(t.translation - translation), glm::vec2(0.01f))) &&
               glm::all(glm::lessThan(glm::abs(t.pivot - pivot), glm::vec2(0.01f)));
    };
    auto textOf = [&](int node) {
        plasma::TextStyle s;
        const int shape = gui.nodes[(size_t)node].shape;
        return shape >= 0 && plasma::textStyle(gui.shapes[(size_t)shape], gui.loaderFlags, s) ? s.string : std::wstring(L"-");
    };
    const int hp = gui.find("hpbar"), mp = gui.find("mpbar"), stamina = gui.find("staminabar");
    CHECK(hp == 27 && mp == 16 && stamina == 328);
    CHECK(names(hp) == std::vector<std::string>({"background", "bar", "Shape", "frame", "info", "text", "text"}));
    CHECK(placed(hp, {566.25f, 882.5f}, {-56.25f, -37.5f}));
    const std::vector<int> &hpKids = gui.nodes[(size_t)hp].children;
    CHECK(placed(hpKids[0], {-7.25f, -25.0f}, {-50, -12.5f}) && placed(hpKids[1], {-7.25f, -25.0f}, {-50, -12.5f}));
    CHECK(names(hpKids[3]) == std::vector<std::string>({"Shape"}) && names(hpKids[4]) == std::vector<std::string>({"info"}));
    CHECK(textOf(hpKids[4]) == L"HP" && textOf(hpKids[5]) == L"20000/2000o");
    CHECK(names(mp) == std::vector<std::string>({"background", "bar", "chargebar", "Shape", "frame", "info", "text", "text"}));
    CHECK(textOf(gui.nodes[(size_t)mp].children[5]) == L"MP");
    CHECK(names(stamina) == std::vector<std::string>({"background", "bar", "frame"}));
    CHECK(placed(stamina, {176.25f, 1012.5f}, {3.75f, -2.5f}));
    const plx::Node &staminaBar = gui.nodes[(size_t)gui.nodes[(size_t)stamina].children[1]];
    CHECK(glm::length(plasma::pivot(gui.transformations[(size_t)staminaBar.transformation]) - glm::vec2(-45.0f, -13.5f)) < 0.01f);
    CHECK(names(gui.nodes[(size_t)stamina].children[2]) == std::vector<std::string>({"Shape", "Mesh", "Mesh", "Mesh"}));
    // Every gui.plx mesh builds.
    int built = 0;
    for (const plx::Object &o : gui.shapes) {
        plasma::ShapeDrawing d;
        built += plasma::buildSmoothMesh(o, d);
    }
    CHECK(built == 659 - 160);

    // Text from the game's own font: pens on whole raster pixels, lines centred on the origin.
    std::ifstream in(dir / "resource1.dat", std::ios::binary);
    plasma::Font font;
    CHECK(font.load(std::vector<unsigned char>((std::istreambuf_iterator<char>(in)), {})));
    plasma::TextStyle style;
    style.pixelSize = 12, style.scalable = true;
    const plasma::Raster r = plasma::rasterFor(glm::mat4(1), style);
    CHECK(r.size == glm::vec2(12) && r.stroke == 0);
    const float a = font.glyph(L'A', r).advance, b = font.glyph(L'B', r).advance;
    CHECK(a > 0 && a == std::floor(a) && font.kerning(L'A', L'B', r) == 0);
    const plasma::Layout plain = plasma::layout(font, L"AB", r, 0);
    CHECK(plain.glyphs.size() == 2 && plain.glyphs[1].pen.x == a && plain.pen.x == a + b);
    const plasma::Layout centred = plasma::layout(font, L"AB\nA", r, plasma::TEXT_CENTER);
    CHECK(centred.glyphs.size() == 3 && centred.glyphs[0].pen.x == -(a + b) * 0.5f && centred.glyphs[2].pen.x == -a * 0.5f);
    CHECK(centred.glyphs[2].pen.y == 12);
    style.wrapWidth = a * 2;
    plasma::Raster wrapping = plasma::rasterFor(glm::mat4(1), style);
    CHECK(plasma::wrapText(font, L"AA AA AA", wrapping) == L"AA\nAA\nAA");
}

// The use key (UsePrompt.h, docs/specs/use-prompt.md), on synthetic worlds: the targets it
// chooses and the order of its prompts.
namespace usekey {
template <class T> void set(worldgen::CreatureRecord &r, size_t at, T v) { std::memcpy(&r[at], &v, sizeof v); }

// A living creature of size 1 x 1 x 2.
worldgen::CreatureRecord creature(int64_t id, int hostility, int race = 0, int cls = 0) {
    worldgen::CreatureRecord r = worldgen::newCreature(id);
    set<float>(r, worldgen::creature::HEALTH, 100.0f);
    r[worldgen::creature::HOSTILITY] = (uint8_t)hostility;
    set<int32_t>(r, worldgen::creature::RACE, race);
    r[worldgen::creature::CLASS] = (uint8_t)cls;
    set<float>(r, 0x84, 1.0f), set<float>(r, 0x88, 1.0f), set<float>(r, 0x8c, 2.0f);
    set<float>(r, 0x990, 0.0f);
    set<int32_t>(r, 0x1b0, 0x7fffffff);
    set<uint32_t>(r, worldgen::creature::FLAGS, 0);
    r[worldgen::creature::ABILITY] = 0;
    return r;
}

// The player (id 1) at the origin, idle and over its first frame, with the camera 6 blocks
// behind it looking along +x.
struct UseWorld {
    worldgen::CreatureRecord player = creature(1, 0);
    std::deque<worldgen::CreatureRecord> others; // (a deque: records keep their place)
    useprompt::Scene scene;
    UseWorld() {
        set<float>(player, 0x6c, 20.0f);
        set<float>(player, 0x9a0, 1.0f);
        scene.camera.eye = glm::vec3(-6, 0, 1);
        scene.camera.forward = glm::vec3(1, 0, 0), scene.camera.right = glm::vec3(0, -1, 0), scene.camera.up = glm::vec3(0, 0, 1);
        scene.camera.tanHalfWidth = scene.camera.tanHalfHeight = 0.7f;
    }
    worldgen::CreatureRecord &add(worldgen::CreatureRecord r, glm::vec3 at, bool hostile = false) {
        others.push_back(std::move(r));
        scene.creatures.push_back({others.back().data(), at, hostile});
        return others.back();
    }
    useprompt::Scene &ready() {
        scene.player = player.data();
        scene.creatureById = [this](int64_t id) -> const uint8_t * {
            for (const auto &r : others)
                if (std::memcmp(&r[worldgen::creature::ID], &id, 8) == 0)
                    return r.data();
            return nullptr;
        };
        return scene;
    }
    useprompt::Prompt prompt(bool press = false) {
        useprompt::KeyState key;
        ready().useDown = press;
        return useprompt::decide(scene, useprompt::chooseTargets(scene), key);
    }
};

useprompt::Object object(int type, glm::vec3 at, int index = 0) {
    useprompt::Object o;
    o.type = type, o.at = at, o.index = index, o.zoneX = 3, o.zoneY = 4;
    o.size = glm::vec3(1, 1, 2);
    return o;
}

nativeai::Creature side(worldgen::CreatureRecord &record) {
    nativeai::Creature me;
    me.record = record.data();
    return me;
}
} // namespace usekey

static void useKeyTargets() {
    using namespace usekey;
    using useprompt::Camera;
    // The crosshair's test: a box around the view's middle line, not one beside or behind it.
    Camera camera;
    camera.eye = glm::vec3(0), camera.forward = glm::vec3(1, 0, 0), camera.right = glm::vec3(0, -1, 0), camera.up = glm::vec3(0, 0, 1);
    CHECK(camera.crosshairIn(useprompt::boxCorners(glm::vec3(4, -0.5f, -0.5f), glm::vec3(1))));
    CHECK(!camera.crosshairIn(useprompt::boxCorners(glm::vec3(4, 0.5f, -0.5f), glm::vec3(1))));
    CHECK(!camera.crosshairIn(useprompt::boxCorners(glm::vec3(-5, -0.5f, -0.5f), glm::vec3(1))));
    CHECK(camera.sees(glm::vec3(10, 3, 0), 0) && !camera.sees(glm::vec3(-10, 0, 0), 1) && !camera.sees(glm::vec3(1, 5, 0), 0.5f));

    // Line of sight: blocked by a solid block on the way, not by the one it starts in; too far
    // is blocked, too near is clear.
    auto wallAt5 = [](int x, int, int) { return x == 5; };
    auto startInside = [](int x, int, int) { return x == 0; };
    CHECK(!useprompt::clearLine(glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(9.5f, 0.5f, 0.5f), 256, wallAt5));
    CHECK(useprompt::clearLine(glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(4.5f, 0.5f, 0.5f), 256, wallAt5));
    CHECK(useprompt::clearLine(glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(4.5f, 0.5f, 0.5f), 256, startInside));
    CHECK(!useprompt::clearLine(glm::vec3(0), glm::vec3(9, 0, 0), 8, [](int, int, int) { return false; }));
    CHECK(useprompt::clearLine(glm::vec3(0), glm::vec3(0.001f, 0, 0), 8, [](int, int, int) { return true; }));
    CHECK(!useprompt::clearLine(glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(5.5f, 5.5f, 0.5f), 256, [](int x, int y, int) { return x == 3 && y == 3; }));

    { // Of the creatures under the crosshair, the one nearest the view's middle line wins, however far.
        UseWorld w;
        w.add(creature(10, 3), glm::vec3(30, 0, 1));
        w.add(creature(11, 3), glm::vec3(8, 0.4f, 1));
        const useprompt::Targets t = useprompt::chooseTargets(w.ready());
        CHECK(t.creature == 10 && t.overCreature);
        // Out of sight from the camera, it is not taken.
        w.scene.sight = [](glm::vec3, glm::vec3 to, float) { return to.x < 20; };
        CHECK(useprompt::chooseTargets(w.scene).creature == 11);
    }
    { // While an attack runs, friendly creatures are left out of the crosshair; during 0x86 the hostile ones.
        UseWorld w;
        w.player[0x68] = 0x05;
        w.scene.abilityDuration = 500;
        w.add(creature(10, 3), glm::vec3(8, 0, 1));
        w.add(creature(11, 1), glm::vec3(12, 0.3f, 1), true);
        CHECK(useprompt::chooseTargets(w.ready()).creature == 11);
        w.player[0x68] = 0x86;
        CHECK(useprompt::chooseTargets(w.ready()).creature == 10);
        // Over, the ability leaves nobody out.
        w.player[0x68] = 0x05;
        set<float>(w.player, 0x6c, 600.0f);
        CHECK(useprompt::chooseTargets(w.ready()).creature == 10);
    }
    { // A static object within 4 blocks under the crosshair beats a creature under it.
        UseWorld w;
        w.add(creature(10, 3), glm::vec3(8, 0, 1));
        w.scene.objects.push_back(object(0x1c, glm::vec3(3, 0, 0)));
        useprompt::Targets t = useprompt::chooseTargets(w.ready());
        CHECK(t.creature == 0 && t.object == 0);
        // Past 4 blocks it is not looked at.
        w.scene.objects[0].at = glm::vec3(4.5f, 0, 0);
        t = useprompt::chooseTargets(w.ready());
        CHECK(t.creature == 10 && t.object == -1);
        // Nor is a type the use key has nothing for, unless it takes an item.
        w.scene.objects[0] = object(0x30, glm::vec3(3, 0, 0));
        CHECK(useprompt::chooseTargets(w.ready()).object == -1);
        w.scene.objects[0].item[0] = 0x0b;
        CHECK(useprompt::chooseTargets(w.ready()).object == 0);
    }
    { // Nothing under the crosshair: the nearest friendly creature within 4 blocks stands in ...
        UseWorld w;
        // (Looking up from far below: everything in view, nothing under the crosshair.)
        w.scene.camera.eye = glm::vec3(0, 0, -50);
        w.scene.camera.forward = glm::vec3(0, 0, 1), w.scene.camera.right = glm::vec3(1, 0, 0), w.scene.camera.up = glm::vec3(0, 1, 0);
        w.scene.camera.tanHalfWidth = w.scene.camera.tanHalfHeight = 100.0f;
        w.add(creature(10, 3), glm::vec3(3, 0, 0));
        w.add(creature(11, 1), glm::vec3(1, 0, 0), true);
        useprompt::Targets t = useprompt::chooseTargets(w.ready());
        CHECK(t.creature == 0); // (the nearest is a monster, which never stands in)
        w.scene.creatures.pop_back();
        t = useprompt::chooseTargets(w.ready());
        CHECK(t.creature == 10 && t.object == -1 && t.groundItem == -1);
        // ... unless an object within 4 blocks is seen from the player ...
        w.scene.objects.push_back(object(0x20, glm::vec3(-2, 0, 0)));
        t = useprompt::chooseTargets(w.ready());
        CHECK(t.creature == 0 && t.object == 0);
        // ... and a valid ground item within 4 blocks cancels both.
        useprompt::GroundItem g;
        g.type = 0x0b, g.subtype = 1, g.at = glm::vec3(0, 3, 0);
        w.scene.groundItems.push_back(g);
        t = useprompt::chooseTargets(w.ready());
        CHECK(t.creature == 0 && t.object == -1 && t.groundItem == 0);
        // One reserved for someone else is not looked at.
        w.scene.groundItems[0].reservedFor = 77;
        t = useprompt::chooseTargets(w.ready());
        CHECK(t.groundItem == -1 && t.object == 0);
        // Coins are picked up by walking over them, once settled, and are never a target.
        w.scene.groundItems[0] = g;
        w.scene.groundItems[0].type = 0x0c;
        t = useprompt::chooseTargets(w.ready());
        CHECK(t.groundItem == -1 && t.walkedOver == std::vector<int>{0});
        w.scene.groundItems[0].settleMs = 30;
        CHECK(useprompt::chooseTargets(w.ready()).walkedOver.empty());
        // A dead player aims at nothing near.
        set<float>(w.player, worldgen::creature::HEALTH, 0.0f);
        w.scene.groundItems.clear();
        t = useprompt::chooseTargets(w.ready());
        CHECK(t.creature == 0 && t.object == -1);
    }
    // What a ground item allows carrying.
    CHECK(useprompt::mayCarryMore(0x15, 0, 0) && !useprompt::mayCarryMore(0x15, 0, 1) && !useprompt::mayCarryMore(0x18, 2, 1));
    CHECK(useprompt::mayCarryMore(1, 0x0c, 4) && !useprompt::mayCarryMore(1, 0x0e, 5) && useprompt::mayCarryMore(1, 3, 49) &&
          !useprompt::mayCarryMore(1, 3, 50));
    CHECK(useprompt::mayCarryMore(0x0b, 0x28, 9) && !useprompt::mayCarryMore(0x0b, 0x28, 10) && useprompt::mayCarryMore(3, 0, 1000));
    { // A creature aimed at marks its mission's zone: not a plain townsperson, unless of class 0x9d.
        worldgen::CreatureRecord r = creature(5, 3);
        set<int32_t>(r, 0x1b0, 7), set<int32_t>(r, 0x1b4, -2);
        CHECK(!useprompt::markedZone(r.data()));
        r[worldgen::creature::CLASS] = 0x9d;
        CHECK(useprompt::markedZone(r.data()) == (std::array<int32_t, 2>{7, -2}));
        r[worldgen::creature::HOSTILITY] = 1, r[worldgen::creature::CLASS] = 0;
        CHECK(useprompt::markedZone(r.data()).has_value());
        set<int32_t>(r, 0x1b0, 0x7fffffff);
        CHECK(!useprompt::markedZone(r.data()));
    }
}

// The item model rules (docs/specs/item-models.md §1-§4) on hand-made tables.
// The sound and music rules (docs/specs/sound.md): placing, pitch, the music choice and its
// timing, and the ambient emitters' timing.
static void soundRules() {
    using namespace soundrules;
    auto about = [](float a, float b) { return std::fabs(a - b) < 1e-5f; };
    { // §2: 1 at the listener, half at d² = 2048, nothing past 64 blocks.
        CHECK(attenuate(0).audible && about(attenuate(0).attenuation, 1.0f));
        CHECK(about(attenuate(2048).attenuation, 0.5f));
        CHECK(attenuate(4096).audible && about(attenuate(4096).attenuation, 0.0f));
        CHECK(!attenuate(4096.5f).audible);
        CHECK(about(finalVolume(50, 1.0f, 0.5f), 0.25f) && about(finalVolume(100, 0.1f, 1.0f), 0.1f));
        CHECK(cappedPitch(3.0f) == 2.0f && cappedPitch(2.0f) == 2.0f && cappedPitch(1.5f) == 1.5f && cappedPitch(0.5f) == 0.5f);
        CHECK(isInstrument(0x6d) && isInstrument(0x77) && !isInstrument(0x6c) && !isInstrument(0x78));
        CHECK(about(explosionShake(0), 1.0f) && about(explosionShake(1250), 0.5f) && explosionShake(2500) == 0.0f &&
              explosionShake(9000) == 0.0f);
        CHECK(isExplosion(0x54) && isExplosion(0x55) && !isExplosion(0x56));
    }
    { // The request record: position, number, pitch at +0x1c, volume at +0x20.
        Request r;
        r.position[0] = 1, r.position[1] = -2, r.position[2] = 3 << 16;
        r.number = 0x23, r.pitch = 1.25f, r.volume = 0.75f, r.owner = 7;
        const RequestRecord record = makeRequest(r);
        float pitch, volume;
        std::memcpy(&pitch, record.data() + 0x1c, 4);
        std::memcpy(&volume, record.data() + 0x20, 4);
        CHECK(pitch == 1.25f && volume == 0.75f && record[0x18] == 0x23);
        const Request back = readRequest(record);
        CHECK(back.position[1] == -2 && back.position[2] == 3 << 16 && back.number == 0x23 && back.owner == 7);
    }
    { // §3.3: which track.
        MusicPlace p;
        p.cellX = 5, p.ground = 50;
        auto track = [&](const MusicPlace &q, const std::string &current = "greenlands") { return chooseTrack(q, current); };
        const std::pair<int, const char *> biomes[] = {{3, "woodlands"}, {4, "jungle"}, {5, "deadlands"}, {12, "deadlands"},
                                                       {6, "winter"}, {11, "winter"}, {7, "mountains"}, {9, "desert"},
                                                       {10, "savannah"}, {13, "cult of doom"}, {2, "greenlands"}, {8, "greenlands"}};
        for (auto [biome, name] : biomes) {
            p.biome = biome;
            CHECK(track(p).name == name && track(p).placeMusic);
        }
        p.biome = 3;
        p.ground = 9.5f;
        CHECK(track(p).name == "beach");
        p.ground = -1;
        CHECK(track(p).name == "ocean");
        p.ground = 50;
        const std::pair<int, const char *> kinds[] = {{4, "unholy pact"}, {9, "unholy pact"}, {8, "order of the light"},
                                                      {10, "mountains"}, {13, "druids of mana"}, {0x14, "druids of mana"},
                                                      {0x20, "druids of mana"}, {0x11, "steel empire"}, {0x12, "cult of doom"},
                                                      {0x25, "cult of doom"}, {0x19, "woodlands"}, {0x1a, "deadlands"},
                                                      {0x1b, "enchanted-forest"}, {1, "woodlands"}};
        for (auto [kind, name] : kinds) {
            p.placeKind = kind;
            CHECK(track(p).name == name);
        }
        p.placeKind = 5; // a town: the village tracks by the place's hash, the forest's otherwise
        CHECK(track(p).name == "village");
        p.placeHashOdd = true;
        CHECK(track(p).name == "village2");
        p.forestWeight = 0.6f;
        CHECK(track(p).name == "unholy pact");
        p.forestWeight = 0, p.placeHashOdd = false;
        p.ground = -5; // a place's music wins over the sea
        CHECK(track(p).name == "village" && track(p).placeMusic);
        p.ground = 50;
        MusicPlace title = p;
        title.cellX = 0;
        CHECK(track(title).name == "maintheme" && !track(title).placeMusic);
        title.placeKind = 0;
        CHECK(track(title).name == "woodlands");
        MusicPlace shrine = p;
        shrine.shrine = true;
        CHECK(track(shrine).name == "shrine" && !track(shrine).placeMusic);
        MusicPlace spirit = p;
        spirit.spiritWorld = true;
        CHECK(track(spirit).name == "deadlands" && track(spirit).threshold == 100.0f);
        CHECK(track(spirit, "deadlands").threshold == 0.0f); // only when it changes the track
        MusicPlace boss = spirit;
        boss.boss = true;
        CHECK(track(boss).name == "boss" && !track(boss).placeMusic);
        boss.success = true;
        CHECK(track(boss).name == "success" && track(boss).threshold == 500.0f);
        boss.menu = true;
        CHECK(track(boss).name == "maintheme" && !track(boss).placeMusic);
        MusicPlace menu;
        menu.menu = true;
        CHECK(track(menu, "woodlands").threshold == 500.0f && track(menu, "maintheme").threshold == 0.0f);
    }
    { // §3.4: when the music changes.
        MusicOptions options;
        options.volume = 50;
        const Track place{"woodlands", true, 0}, desert{"desert", true, 0}, boss{"boss", false, 0};
        MusicState s;
        stepMusic(s, place, false, 16, options, false, 0); // silence: starts at once at full volume
        CHECK(s.current == "woodlands" && s.playedMs == 0 && s.threshold == 300000.0f && about(s.volume, 0.5f));
        stepMusic(s, place, true, 1000, options, false, 0);
        CHECK(s.playedMs == 1000.0f && s.threshold == 300000.0f && about(s.volume, 0.5f));
        stepMusic(s, desert, true, 9000, options, false, 0); // another track at 10 s: waits for 30 s
        CHECK(s.current == "woodlands" && s.threshold == 30000.0f && !s.firstDifference && about(s.volume, 0.5f));
        stepMusic(s, desert, true, 19000, options, false, 0);
        CHECK(s.threshold == 30000.0f && about(s.volume, 0.5f)); // 29 s: still playing
        stepMusic(s, desert, true, 2000, options, false, 0);
        CHECK(s.volume < 0.5f); // past 30 s: fading
        for (int i = 0; i < 800 && s.volume >= MUSIC_STOP_VOLUME; i++)
            stepMusic(s, desert, true, 16, options, false, 0);
        CHECK(s.volume < MUSIC_STOP_VOLUME && s.current == "woodlands");
        stepMusic(s, desert, false, 16, options, false, 0); // stopped: the new track starts
        CHECK(s.current == "desert" && about(s.volume, 0.5f) && s.playedMs == 0 && s.firstDifference);

        MusicState late;
        stepMusic(late, place, false, 16, options, false, 0);
        stepMusic(late, place, true, 26000, options, false, 0);
        stepMusic(late, desert, true, 1000, options, false, 0); // 27 s in: 5 s more
        CHECK(late.threshold == 32000.0f);
        stepMusic(late, place, true, 16, options, false, 0); // wanted again: the wait is forgotten
        CHECK(late.firstDifference && late.threshold == 300000.0f);

        MusicState urgent;
        stepMusic(urgent, place, false, 16, options, false, 0);
        stepMusic(urgent, place, true, 4000, options, false, 0);
        stepMusic(urgent, boss, true, 100, options, false, 0); // a boss: half a second
        CHECK(urgent.threshold == 4600.0f);

        MusicState five; // place music fades after five minutes unless looped
        stepMusic(five, place, false, 16, options, false, 0);
        stepMusic(five, place, true, 300001, options, false, 0);
        CHECK(five.volume < 0.5f);
        options.loop = true;
        MusicState looped;
        stepMusic(looped, place, false, 16, options, false, 0);
        stepMusic(looped, place, true, 300001, options, false, 0);
        CHECK(looped.threshold == 600001.0f && about(looped.volume, 0.5f));
        options.loop = false;

        MusicState ducked; // an instrument: 0.015 while the duck timer runs
        stepMusic(ducked, place, false, 16, options, false, 0);
        for (int i = 0; i < 300; i++)
            stepMusic(ducked, place, true, 16, options, false, 1000);
        CHECK(std::fabs(ducked.volume - DUCKED_MUSIC_VOLUME) < 0.001f);

        MusicState success;
        stepMusic(success, Track{"success", false, 500}, false, 16, options, true, 0);
        CHECK(success.threshold == 10000.0f && success.current == "success");
        CHECK(about(easeMusicVolume(0.4f, 0.0f, 0), 0.4f));
        CHECK(easeMusicVolume(0.4f, 0.0f, 100000) == easeMusicVolume(0.4f, 0.0f, 1000));
        MusicState fading;
        fading.current = "old", fading.playedMs = 20000, fading.threshold = 1;
        fading.firstDifference = false, fading.volume = 0.5f;
        MusicState fast = fading, recovering = fading;
        stepMusic(fading, Track{"new", false, 0}, true, 16, options, false, 0);
        stepMusic(fast, Track{"boss", false, 0}, true, 16, options, false, 0);
        CHECK(about(fading.volume, easeMusicVolume(0.5f, 0, 16, 0.0004f)));
        CHECK(about(fast.volume, easeMusicVolume(0.5f, 0, 16, 0.005f)) && fast.volume < fading.volume);
        recovering.volume = 0.015f;
        stepMusic(recovering, Track{"old", false, 0}, true, 16, options, false, 0);
        CHECK(about(recovering.volume, easeMusicVolume(0.015f, 0.5f, 16, 0.001f)));
    }
    { // §4: the staggered ticks.
        CHECK(staggeredTick(999, 2, 0, 1000) && !staggeredTick(0, 999, 0, 1000) && staggeredTick(0, 1000, 0, 1000));
        CHECK(staggeredTick(400, 200, 500, 1000) && !staggeredTick(600, 200, 500, 1000));
        const int64_t block = 65536;
        CHECK(tickOffset(1500 * block, 0, 1, 1000) == 500 + 0x14d);
        CHECK(tickOffset(0, 1000 * block, 1, 1000) == 0x14d);
        CHECK(tickOffset(0, 0, 1, 5000) == 0x14d);
        CHECK(tickOffset(100 * block, 0, 13, 500) == 1300 % 500 + (0x14d0000 / 65536) % 500);
        CHECK(isNight(0) && isNight(21600000) && !isNight(21600100) && !isNight(75599900) && isNight(75600000));
    }
    { // §4: what the emitters play and when.
        struct Played {
            int number;
            float volume, pitch;
        };
        std::vector<Played> played;
        auto play = [&](int n, const int64_t *, float v, float p) { played.push_back({n, v, p}); };
        int value = 0;
        auto fixed = [&] { return value; };
        Emitter water;
        water.type = emitter::WATER;
        AmbientFrame frame;
        frame.timeOfDayMs = 10 * 3600000.0f;
        frame.dtMs = 500; // every frame crosses a 500 ms tick
        ambientZone({water}, frame, fixed, play);
        CHECK(played.size() == 1 && played[0].number == soundid::RIVER_FIRST && about(played[0].volume, 0.1f) &&
              about(played[0].pitch, 0.8f));
        value = 32767 * 4; // a multiple of 4: heard, as the river 131068 % 3 picks, at pitch 2.4
        played.clear();
        ambientZone({water}, frame, fixed, play);
        CHECK(played.size() == 1 && played[0].number == soundid::RIVER_FIRST + 131068 % 3 && about(played[0].pitch, 4 * 0.4f + 0.8f));
        value = 1; // not 1 in 4
        played.clear();
        ambientZone({water}, frame, fixed, play);
        CHECK(played.empty());
        value = 0;
        Emitter bird = water;
        bird.type = 0x4f;
        frame.dtMs = 1000;
        played.clear();
        ambientZone({bird}, frame, fixed, play); // by day: the second bird
        CHECK(played.size() == 1 && played[0].number == soundid::BIRD_FIRST + 1 && about(played[0].pitch, 0.9f));
        Emitter owl = bird;
        owl.type = 0x4e;
        frame.timeOfDayMs = 23 * 3600000.0f;
        played.clear();
        ambientZone({owl}, frame, fixed, play); // by night: an owl
        CHECK(played.size() == 1 && played[0].number == soundid::OWL_FIRST && about(played[0].pitch, 0.5f));
        played.clear();
        ambientZone({bird}, frame, fixed, play); // the other birds are quiet by night (and a lone one is no twentieth)
        CHECK(played.empty());
        // Crickets: every twentieth emitter, on a 5 s tick, 12:00 to 24:00.
        std::vector<Emitter> field(40, water);
        for (Emitter &e : field)
            e.type = 5;
        field[39].type = 11;
        frame.dtMs = 5000;
        played.clear();
        ambientZone(field, frame, fixed, play);
        CHECK(played.size() == 2 && played[0].number == soundid::CRICKET_SMALL && played[1].number == soundid::CRICKET_LARGE &&
              about(played[0].volume, 0.05f));
        frame.timeOfDayMs = 11 * 3600000.0f;
        played.clear();
        ambientZone(field, frame, fixed, play);
        CHECK(played.empty());
        // Over an hour a river murmurs on about a quarter of its 7200 half-second ticks.
        uint32_t state = 12345;
        auto lcg = [&] {
            state = state * 214013u + 2531011u;
            return (int)((state >> 16) & 0x7fff);
        };
        frame.timeOfDayMs = 10 * 3600000.0f;
        frame.dtMs = 16;
        played.clear();
        for (double clock = 0; clock < 3600000.0; clock += 16) {
            frame.clockMs = clock;
            ambientZone({water}, frame, lcg, play);
        }
        CHECK(played.size() > 1600 && played.size() < 2000);
    }
    { // §5.1: the wind while gliding; §5.2: an item landing.
        CHECK(!windDue(0, 400) && !windDue(400, 500) && windDue(400, 600) && !windDue(600, 900) && windDue(900, 1001));
        int draws[] = {32767, 0};
        int next = 0;
        const WindSound w = windSound([&] { return draws[next++]; });
        CHECK(about(w.pitch, 3.0f) && about(w.volume, 0.5f) && cappedPitch(w.pitch) == 2.0f);
        auto one = [] { return 32767; };
        CHECK(itemLanded(12, false, one).number == soundid::DROP_COIN && about(itemLanded(12, false, one).pitch, 1.1f));
        CHECK(itemLanded(5, false, one).number == soundid::DROP_ITEM && !itemLanded(5, false, one).chime);
        CHECK(itemLanded(5, true, one).chime && itemLanded(0x1a, true, one).chime && !itemLanded(10, true, one).chime);
    }
}

static void itemModelRules() {
    using namespace nativeitems;
    auto fields = [](uint8_t category, uint32_t id, uint8_t rarity, uint32_t modifier = 0, uint8_t material = 0) {
        ModelFields f;
        f.category = category, f.id = id, f.rarity = rarity, f.modifier = modifier, f.material = material;
        return f;
    };
    // The rarity term: min(rarity, 4); category 0x0f shifted down and capped at 3;
    // category 1 id 1 unclamped.
    CHECK(rarityTerm(fields(5, 0, 2)) == 2 && rarityTerm(fields(5, 0, 4)) == 4 && rarityTerm(fields(5, 0, 9)) == 4);
    CHECK(rarityTerm(fields(0x0f, 0, 0)) == 0 && rarityTerm(fields(0x0f, 0, 1)) == 0);
    CHECK(rarityTerm(fields(0x0f, 0, 2)) == 1 && rarityTerm(fields(0x0f, 0, 4)) == 3 && rarityTerm(fields(0x0f, 0, 200)) == 3);
    CHECK(rarityTerm(fields(1, 1, 7)) == 7 && rarityTerm(fields(1, 1, 255)) == 255);
    CHECK(rarityTerm(fields(1, 2, 7)) == 4);
    // The modifier: its period is 660, the least common multiple of 2, 3, 4, 5, 6, 10, 11.
    CHECK(modifierTerm(330u) == 330 && modifierTerm(660u) == 0 && modifierTerm(661u) == 1);
    CHECK(modifierTerm(fields(0x11, 0, 0, 330)) == 2); // 330 mod 4, where (330 mod 330) mod 4 = 0
    CHECK(modifierTerm(fields(0x16, 0, 0, 334)) == 334 % 60);
    CHECK(modifierTerm(fields(3, 0, 0, 1000)) == 1000 % 66);
    CHECK(modifierTerm(fields(0x0c, 0, 0, 1000)) == 0); // category 0x0c never reads it
    // Normalising category 0x16's stand-ins.
    {
        uint8_t item[0xa0] = {};
        item[ITEM_CATEGORY] = 0x16;
        int32_t v = 0x3f, kind = 5;
        std::memcpy(item + ITEM_ID, &v, 4);
        std::memcpy(item + ITEM_STANDS_FOR, &kind, 4);
        item[ITEM_RARITY] = 3, item[ITEM_MATERIAL] = 11;
        ModelFields f = fieldsOf(item);
        CHECK(f.category == 3 && f.id == 5 && f.rarity == 3 && f.material == 11);
        v = 0x41;
        std::memcpy(item + ITEM_ID, &v, 4);
        f = fieldsOf(item);
        CHECK(f.category == 8 && f.id == 0 && f.material == 11);
        v = 0x43;
        std::memcpy(item + ITEM_ID, &v, 4);
        CHECK(fieldsOf(item).category == 0x16 && fieldsOf(item).id == 0x43);
    }
    // A CWI3-style table: modifier period 660, ids up to 0x130 (which stands for all above).
    {
        ModelTable t;
        t.setPeriodAndIdBound(660, 0x130, true);
        std::vector<int32_t> byMod(660);
        for (int c = 0; c < 660; c++)
            byMod[c] = 100 + c % 4;
        t.add(0x11, 0, 0, 1, 660, byMod);
        CHECK(t.modelNumber(fields(0x11, 0, 0, 330)) == 102);
        CHECK(t.modelNumber(fields(0x11, 77, 4, 661)) == 101); // id and rarity not read
        std::vector<int32_t> raw(256);
        for (int r = 0; r < 256; r++)
            raw[r] = 50 + r;
        t.add(1, 1, 0, 256, 1, raw);
        CHECK(t.modelNumber(fields(1, 1, 7)) == 57 && t.modelNumber(fields(1, 1, 2)) == 52);
        t.add(0x15, 0x130, 0, 1, 1, {7});
        CHECK(t.modelNumber(fields(0x15, 0x130, 0)) == 7 && t.modelNumber(fields(0x15, 0x500, 0)) == 7);
        CHECK(t.modelNumber(fields(0x15, 0x12f, 0)) == -1);
        t.add(0x0f, 0, 0, 5, 1, {10, 10, 11, 12, 13});
        CHECK(t.modelNumber(fields(0x0f, 0, 1)) == 10 && t.modelNumber(fields(0x0f, 0, 3)) == 12);
        CHECK(t.modelNumber(fields(0x0f, 0, 9)) == 13);
        t.add(3, 0x16, 0, 1, 1, {9});
        CHECK(t.modelNumber(fields(3, 0x16, 0)) == -1); // weapons 0x16 and 0x17 have none
        CHECK(t.modelNumber(fields(0x17, 0, 0)) == -1); // not in the list
    }
    // The CWI2 file the extractor first wrote: modifier mod 330, ids 0..127, rarities 0..4.
    {
        std::vector<uint8_t> b = {'C', 'W', 'I', '2'};
        auto u32 = [&](uint32_t v) { for (int i = 0; i < 4; i++) b.push_back((uint8_t)(v >> 8 * i)); };
        u32(3);                                     // grids
        u32(2);                                     // a modifier grid: mod 4 seen through mod 330
        for (uint32_t c = 0; c < 330; c++)
            u32(100 + c % 4);
        u32(1);                                     // a rarity grid: category 1 id 1's run
        for (uint32_t r = 0; r < 5; r++)
            u32(50 + r);
        u32(0);                                     // one model
        u32(60);
        u32(3);                                     // keys
        const uint8_t keys[3][6] = {{1, 1, 0, 0, 1, 0}, {1, 2, 0, 0, 2, 0}, {0x11, 0, 0, 0, 0, 0}};
        for (auto &k : keys)
            b.insert(b.end(), k, k + 6);
        ModelTable t;
        std::string why;
        CHECK(t.parse(b, &why));
        CHECK(t.modelNumber(fields(0x11, 0, 0, 330)) == 102); // 330 -> 660: exact all the same
        CHECK(t.modelNumber(fields(0x11, 0, 0, 1000)) == 100);
        CHECK(t.modelNumber(fields(0x11, 300, 0, 3)) == 103);  // id not read: any id finds the key
        CHECK(t.modelNumber(fields(1, 1, 3)) == 53);
        CHECK(t.modelNumber(fields(1, 1, 7)) == 57);           // raw rarity, past the extracted rows
        CHECK(t.modelNumber(fields(1, 2, 7)) == 60);
        CHECK(t.modelNumber(fields(1, 200, 0)) == -1);         // id beyond the old range
        b[3] = '9';
        CHECK(!t.parse(b, &why));
    }
    // Category 0x17: the same pair always gives the same model, kept once.
    {
        const OnDemandModel &a = onDemandModel(3, 1234);
        const OnDemandModel &b = onDemandModel(3, 1234);
        CHECK(&a == &b && a.size == 24 && a.shape == 3);
        const OnDemandModel c = makeOnDemandModel(3, 1234);
        CHECK(c.colours == a.colours && c.counter == a.counter);
        CHECK(makeOnDemandModel(9, 0).shape == 7 && makeOnDemandModel(-1, 0).shape == 7);
        for (auto &colour : a.colours)
            for (float ch : colour)
                CHECK(ch >= 10 && ch <= 259);
        CHECK(makeOnDemandModel(3, 1235).colours != a.colours);
    }
}

static void useKeyPrompts() {
    using namespace usekey;
    using useprompt::Action;
    { // The player's own states come first: letting go of a wall over talking to the one aimed at.
        UseWorld w;
        w.add(creature(10, 3), glm::vec3(3, 0, 1));
        CHECK(w.prompt().text == "Talk");
        w.player[0x68] = 0x7a;
        useprompt::Prompt p = w.prompt(true);
        CHECK(p.text == "Fall" && p.takesKey && p.pressed && p.action == Action::LetGo);
        nativeai::Creature me = side(w.player);
        useprompt::applyToPlayer(p, w.scene, me);
        CHECK(w.player[0x68] == 0 && me.interactions.empty());
        // Holding the dodge or walk key: no prompt at all.
        w.player[0x68] = 0x7a;
        w.scene.dodgeHeld = true;
        p = w.prompt();
        CHECK(p.text.empty() && !p.takesKey);
        w.scene.dodgeHeld = false, w.scene.walkHeld = true;
        CHECK(w.prompt().text.empty());
    }
    { // Climbing: against a wall in the air, moving, with stamina, the ability over.
        UseWorld w;
        set<float>(w.player, 0x40, 1.0f);
        set<uint32_t>(w.player, 0x5c, 4);
        useprompt::Prompt p = w.prompt(true);
        CHECK(p.text == "Climb");
        nativeai::Creature me = side(w.player);
        useprompt::applyToPlayer(p, w.scene, me);
        CHECK(w.player[0x68] == 0x7a);
        w.player[0x68] = 0;
        set<uint32_t>(w.player, 0x5c, 5); // on the ground
        CHECK(w.prompt().text != "Climb");
        set<uint32_t>(w.player, 0x5c, 4);
        set<float>(w.player, 0x9a0, 0.0f); // no stamina
        CHECK(w.prompt().text != "Climb");
    }
    { // Dead: "Revive", before throwing and before the creature aimed at.
        UseWorld w;
        w.add(creature(10, 3), glm::vec3(3, 0, 1));
        w.player[0x68] = 0x80;
        set<int64_t>(w.player, 0x1a0, 10);
        CHECK(w.prompt().text == "Throw");
        set<float>(w.player, worldgen::creature::HEALTH, 0.0f);
        useprompt::Prompt p = w.prompt(true);
        CHECK(p.text == "Revive" && p.action == Action::Revive);
        nativeai::Creature me = side(w.player);
        useprompt::applyToPlayer(p, w.scene, me);
        float revived = 0;
        std::memcpy(&revived, &w.player[0x140], 4);
        CHECK(revived == -0.5f);
    }
    { // Throwing the carried creature, while it is there.
        UseWorld w;
        w.add(creature(10, 2), glm::vec3(30, 30, 1));
        w.player[0x68] = 0x80;
        set<int64_t>(w.player, 0x1a0, 10);
        useprompt::Prompt p = w.prompt(true);
        CHECK(p.text == "Throw");
        nativeai::Creature me = side(w.player);
        useprompt::applyToPlayer(p, w.scene, me);
        CHECK(w.player[0x68] == 0x81);
        set<int64_t>(w.player, 0x1a0, 99);
        w.player[0x68] = 0x80;
        CHECK(w.prompt().text != "Throw");
    }
    { // The creature prompts, in their order.
        UseWorld w;
        worldgen::CreatureRecord &c = w.add(creature(10, 3), glm::vec3(3, 0, 1));
        useprompt::Prompt p = w.prompt();
        CHECK(p.text == "Talk" && p.action == Action::Converse && p.creature == 10 && p.highlighted == 10);
        c[worldgen::creature::CLASS] = 0x81;
        CHECK(w.prompt().action == Action::Shop);
        c[worldgen::creature::CLASS] = 0x9a;
        CHECK(w.prompt().action == Action::FlightMaster);
        c[worldgen::creature::CLASS] = 0x83;
        CHECK(w.prompt().action == Action::ClassWindow);
        set<uint32_t>(c, worldgen::creature::FLAGS, 0x400000); // petrified
        p = w.prompt();
        CHECK(p.text == "Examine" && p.action == Action::ExaminePetrified && p.highlighted == 0);
        // One that can be carried is taken up first, within 5 blocks.
        set<uint32_t>(c, worldgen::creature::FLAGS, 0x4000);
        p = w.prompt(true);
        CHECK(p.text == "Pick up" && p.action == Action::PickUp);
        nativeai::Creature me = side(w.player);
        useprompt::applyToPlayer(p, w.scene, me);
        int64_t carried = 0;
        std::memcpy(&carried, &w.player[0x1a0], 8);
        CHECK(carried == 10 && w.player[0x68] == 0x80 && me.interactions.size() == 1 &&
              me.interactions.front()[nativeinteract::KIND] == nativeinteract::ACTIVATE &&
              std::memcmp(&me.interactions.front()[nativeinteract::TARGET], &carried, 8) == 0);
        // A townsperson out of reach (half its largest size and 5 blocks) has nothing.
        w.player[0x68] = 0;
        set<uint32_t>(c, worldgen::creature::FLAGS, 0);
        w.scene.creatures.back().at = glm::vec3(7, 0, 1);
        CHECK(w.prompt().text.empty());
    }
    { // Animals: "Pet" when resting and the player is free; otherwise "Interact".
        UseWorld w;
        worldgen::CreatureRecord &c = w.add(creature(10, 4, 0x13), glm::vec3(3, 0, 1));
        c[0x68] = 1;
        useprompt::Prompt p = w.prompt(true);
        CHECK(p.text == "Pet" && p.action == Action::Pet);
        nativeai::Creature me = side(w.player);
        useprompt::applyToPlayer(p, w.scene, me);
        CHECK(w.player[0x68] == 0xa2 && me.interactions.size() == 1);
        // (On an ability's first frame friendly creatures are left out of the crosshair.)
        w.player[0x68] = 0;
        CHECK(w.prompt().text.empty());
        set<float>(w.player, 0x6c, 20.0f);
        c[0x68] = 0;
        CHECK(w.prompt().text == "Interact");
    }
    { // Beds, shrines, campfires and things to open or examine.
        UseWorld w;
        worldgen::CreatureRecord &c = w.add(creature(10, 2, 0xbb), glm::vec3(3, 0, 1));
        CHECK(w.prompt().text == "Sleep");
        set<int64_t>(c, 0x1a8, 5); // in use
        CHECK(w.prompt().text.empty());
        set<int32_t>(c, worldgen::creature::RACE, 0x10c);
        CHECK(w.prompt().action == Action::Shrine);
        set<int32_t>(c, worldgen::creature::RACE, 0xb9);
        useprompt::Prompt p = w.prompt();
        CHECK(p.text == "Use" && p.action == Action::Crafting && p.station == 3);
        set<int32_t>(c, worldgen::creature::RACE, 0xb6);
        CHECK(w.prompt().text == "Open");
        set<int32_t>(c, worldgen::creature::RACE, 0);
        c[worldgen::creature::HOSTILITY] = 8;
        p = w.prompt();
        CHECK(p.text == "Examine" && p.action == Action::Activate);
    }
    { // Static objects: one that takes an item first, whatever its type.
        UseWorld w;
        useprompt::Object o = object(0x7e, glm::vec3(3, 0, 0), 9);
        o.item[0] = 0x0b, o.itemCount = 2, o.itemName = "Flame", o.carried = 1;
        w.scene.objects.push_back(o);
        useprompt::Prompt p = w.prompt(true);
        CHECK(p.text == "2 x Flame" && p.colour == glm::vec4(1.0f, 0.3f, 0.3f, 1.0f) && p.takesKey && p.action == Action::None &&
              p.icon == useprompt::Prompt::Icon::ObjectItem);
        w.scene.objects[0].carried = 2;
        p = w.prompt(true);
        CHECK(p.action == Action::GiveItem && p.colour == glm::vec4(1));
        nativeai::Creature me = side(w.player);
        useprompt::applyToPlayer(p, w.scene, me);
        int32_t where[3] = {};
        if (!me.interactions.empty())
            std::memcpy(where, &me.interactions.front()[nativeinteract::ZONE_X], sizeof where);
        CHECK(me.interactions.size() == 1 && me.interactions.front()[nativeinteract::KIND] == nativeinteract::USE_OBJECT &&
              where[0] == 3 && where[1] == 4 && where[2] == 9 && me.interactions.front()[nativeinteract::ITEM] == 0x0b);
        // Given (inactive), the brazier is a lore object.
        w.scene.objects[0].active = false;
        p = w.prompt();
        CHECK(p.action == Action::ExamineLore && p.text == "Examine");
    }
    { // Doors, seats, stations, chests, and types that leave the key to the ground items.
        UseWorld w;
        w.scene.objects.push_back(object(0x04, glm::vec3(3, 0, 0)));
        useprompt::Prompt p = w.prompt();
        CHECK(p.text == "Open" && p.action == Action::UseObject);
        w.scene.objects[0].active = false;
        CHECK(w.prompt().text == "Close");
        w.scene.objects[0] = object(0x1e, glm::vec3(3, 0, 0));
        CHECK(w.prompt().text == "Sit");
        w.scene.objects[0].occupant = 1; // seated: a press stands up
        p = w.prompt();
        CHECK(p.text.empty() && p.takesKey && p.action == Action::UseObject);
        w.scene.objects[0] = object(0x66, glm::vec3(3, 0, 0));
        p = w.prompt();
        CHECK(p.text == "Saw" && p.action == Action::Crafting && p.station == 5);
        w.scene.objects[0] = object(0x16, glm::vec3(3, 0, 0));
        w.scene.objects[0].chestHasItem = true;
        p = w.prompt();
        CHECK(p.text == "Open" && p.action == Action::OpenChest);
        w.scene.objects[0].chestCellKnown = true;
        p = w.prompt();
        CHECK(p.text.empty() && p.takesKey && p.action == Action::None);
        w.scene.objects[0] = object(0x00, glm::vec3(3, 0, 0)); // special item 4 is always owned
        CHECK(!w.prompt().takesKey);
        // A type with nothing for the key (0x1b) leaves it to the ground item.
        w.scene.objects[0] = object(0x1b, glm::vec3(3, 0, 0));
        useprompt::GroundItem g;
        g.type = 3, g.at = glm::vec3(-3, 0, 0);
        w.scene.groundItems.push_back(g);
        p = w.prompt();
        CHECK(!p.takesKey && p.text == "Pick up" && p.action == Action::PickUpItem && p.groundItem == 0);
    }
    { // Ground items: full when no more may be carried or the bag has no room; the open
      // inventory takes the key before them.
        UseWorld w;
        useprompt::GroundItem g;
        g.type = 0x15, g.at = glm::vec3(0, 2, 0), g.carriedCount = 1;
        w.scene.groundItems.push_back(g);
        useprompt::Prompt p = w.prompt();
        CHECK(p.text == "Full" && p.colour == glm::vec4(1.0f, 0.3f, 0.3f, 1.0f) && p.action == Action::CannotCarry);
        w.scene.groundItems[0].carriedCount = 0, w.scene.groundItems[0].bagHasRoom = false, w.scene.groundItems[0].bag = 2;
        p = w.prompt();
        CHECK(p.text == "Full" && p.action == Action::BagFull && p.bag == 2);
        w.scene.inventoryOpen = true;
        p = w.prompt();
        CHECK(p.text.empty() && p.action == Action::UseInventoryItem);
    }
    { // Nothing aimed at: the vehicle item, or "Cancel" while it is in use.
        UseWorld w;
        w.scene.vehicle[0] = 0x18;
        const int32_t glider = 1;
        std::memcpy(&w.scene.vehicle[4], &glider, 4);
        w.scene.vehicleName = "Hang Glider";
        useprompt::Prompt p = w.prompt(true);
        CHECK(p.text == "Hang Glider" && p.action == Action::Vehicle && p.icon == useprompt::Prompt::Icon::Vehicle);
        nativeai::Creature me = side(w.player);
        useprompt::applyToPlayer(p, w.scene, me);
        CHECK(w.player[0x68] == 0x78 && w.player[0x1d8] == 0x18);
        p = w.prompt(true);
        CHECK(p.text == "Cancel");
        useprompt::applyToPlayer(p, w.scene, me);
        CHECK(w.player[0x68] == 0);
        // Not while the dialog window is open.
        w.scene.dialogOpen = true;
        CHECK(w.prompt().text.empty());
    }
    { // A press is the key going down: held, it acts once.
        UseWorld w;
        useprompt::KeyState key;
        w.ready().useDown = true;
        const useprompt::Targets t = useprompt::chooseTargets(w.scene);
        CHECK(useprompt::decide(w.scene, t, key).pressed);
        CHECK(!useprompt::decide(w.scene, t, key).pressed);
        w.scene.useDown = false;
        useprompt::decide(w.scene, t, key);
        w.scene.useDown = true;
        CHECK(useprompt::decide(w.scene, t, key).pressed);
    }
}

int main(int argc, char **argv) {
    if (argc == 2 && std::string(argv[1]) == "npcspeech")
        return npcSpeechTests();
    if (argc == 2 && std::string(argv[1]) == "steamcreatures")
        return steamCreatureLightsTests();
    if (argc == 2 && std::string(argv[1]) == "steamtransients")
        return steamTransientLightsTests();
    if (argc == 2 && std::string(argv[1]) == "dialoguechoices")
        return dialogueChoicesTests();
    if (argc == 4 && std::string(argv[1]) == "steamlights")
        return steamLightOracleTests(pathOfUtf8(argv[2]), pathOfUtf8(argv[3]));
    if (argc == 2 && std::string(argv[1]) == "steampoints")
        return steamPointLightsTests();
    if (argc == 2 && std::string(argv[1]) == "steamshader")
        return steamShaderTests();
    if (argc == 2 && std::string(argv[1]) == "steamterrain")
        return steamTerrainTests();
    if (argc == 2 && std::string(argv[1]) == "steamvoxel")
        return steamVoxelTests();
    if (argc == 2 && std::string(argv[1]) == "hitui")
        return plasmaHitTests() ? 1 : 0;
    if (argc == 2 && std::string(argv[1]) == "scenecost")
        return steamMeshCostTests() ? 1 : 0;
    if (argc == 2 && std::string(argv[1]) == "sharedwater")
        return waterShaderTests();
    if (argc == 2 && std::string(argv[1]) == "alphascenery")
        return alphaSceneryTests();
    if (argc == 2 && std::string(argv[1]) == "saveformat")
        return saveFormatTests();
    if (argc == 2 && std::string(argv[1]) == "alphasceneryshader")
        return alphaSceneryShaderTests();
    if (argc == 2 && std::string(argv[1]) == "waterlook")
        return waterLookTests();
    if (argc == 2 && std::string(argv[1]) == "feattext")
        return featTextTests();
    if (argc == 2 && std::string(argv[1]) == "lootwindows")
        return lootWindowTests();
    if (argc == 2 && std::string(argv[1]) == "scenerange")
        return steamSceneRangeTests() ? 1 : 0;
    if (argc == 2 && std::string(argv[1]) == "padui")
        return gamepadUiTests() ? 1 : 0;
    if (argc == 2 && std::string(argv[1]) == "pets")
        return petTests() ? 1 : 0;
    if (argc == 2 && std::string(argv[1]) == "dialogue")
        return dialoguePageTests() ? 1 : 0;
    if (argc == 2 && std::string(argv[1]) == "playerhit")
        return playerHitTests() ? 1 : 0;
    if (argc == 2 && std::string(argv[1]) == "respawn")
        return enemyRespawnTests() ? 1 : 0;

    if (const char *day = std::getenv("CW_DAY")) // the world's day the compared zones were made on
        worldgen::setWorldDay(std::atoi(day));
    if (const char *day = std::getenv("CW_REFRESH_DAY"); day && *day) // zones as the game has them after loading (0x31c540)
        worldgen::setRefreshDay(std::atoi(day));
    if (argc >= 2 && std::string(argv[1]) != "allspawns") { // the oracle's creature tables, when there are some
        Assets a;
        if (a.locate(""))
            worldgen::loadCreatureTables(spawnsDir(a.gameDir()));
    }
    if (argc == 2 && std::string(argv[1]) == "ownersetup") // only the first-run setup tests (also in the default run)
        return ownerSetupTests() ? 1 : 0;
    if (argc >= 3 && std::string(argv[1]) == "netdecode") // <file>...: packet recordings (Extract/net/*.bin) encode back byte for byte
        return netDecodeCommand(argc, argv);
    if (argc >= 2 && std::string(argv[1]) == "rolesoracle") { // [show]: Extract/oracle/regions.txt against ours
        Assets a;
        if (!a.locate(""))
            return 1;
        std::filesystem::path dir = zonesDir(a.gameDir());
        worldgen::setWorldSeed(seedOf(dir));
        rolesOracle(a.gameDir() / "Extract" / "oracle" / "regions.txt", argc > 2 ? std::atoi(argv[2]) : 20);
        return 0;
    }
    if (argc >= 3 && std::string(argv[1]) == "gearoracle") // Extract/oracle/gear.bin [show]
        return gearOracle(argv[2], argc > 3 ? std::atoi(argv[3]) : 10) ? 0 : 1;
    if (argc >= 2 && std::string(argv[1]) == "allspawns") { // [show]: every dumped zone's spawn points against ours
        Assets a;
        if (!a.locate(""))
            return 1;
        std::filesystem::path dir = zonesDir(a.gameDir());
        worldgen::setWorldSeed(seedOf(dir));
        if (const char *day = std::getenv("CW_DAY")) // the world's day the zones were made on
            worldgen::setWorldDay(std::atoi(day));
        allSpawns(spawnsDir(a.gameDir()), argc > 2 ? std::atoi(argv[2]) : 20);
        return 0;
    }
    if (argc == 4 && std::string(argv[1]) == "zoneobjects") { // <zx> <zy>: the generated zone's static objects
        worldgen::setWorldSeed(123);
        const std::vector<worldgen::StaticObject> ours = generatedObjects(std::atoi(argv[2]), std::atoi(argv[3]));
        for (size_t i = 0; i < ours.size(); i++)
            printObject(std::to_string(i).c_str(), ours[i]);
        return 0;
    }
    if (argc == 4 && std::string(argv[1]) == "objdiff") { // a zone's static objects against the extracted ones
        int32_t zx = std::atoi(argv[2]), zy = std::atoi(argv[3]);
        bool found;
        std::vector<worldgen::StaticObject> game = readGameObjects(zx, zy, found);
        if (!found)
            return std::printf("zone not extracted\n"), 1;
        worldgen::setWorldSeed(123);
        std::vector<worldgen::StaticObject> ours = generatedObjects(zx, zy);
        std::printf("game %zu objects, ours %zu\n", game.size(), ours.size());
        for (const auto &o : objectsMissing(game, ours))
            printObject("game only", o);
        for (const auto &o : objectsMissing(ours, game))
            printObject("ours only", o);
        return 0;
    }
    if (argc == 5 && std::string(argv[1]) == "zoneitems") { // <zx> <zy> <radius>: the generator's lying items (zone+0x30) by kind
        Assets a;
        if (!a.locate("") || !worldgen::loadCreatureTables(a.gameDir() / "Extract" / "oracle"))
            return 1;
        worldgen::setWorldSeed(123);
        const int zx = std::atoi(argv[2]), zy = std::atoi(argv[3]), r = std::atoi(argv[4]);
        std::map<std::pair<int, int>, int> kinds;
        int zones = 0;
        for (int x = zx - r; x <= zx + r; x++)
            for (int y = zy - r; y <= zy + r; y++, zones++)
            {
                for (const worldgen::GroundItem &g : worldgen::zone(x, y)->items)
                    kinds[{g.item.category, g.item.id}]++;
                for (const worldgen::ZoneObject &o : worldgen::zoneObjects(x, y))
                    if (o.type == 22)
                        std::printf("reward chest at block %lld %lld %lld: category %d id %d rarity %d\n", (long long)(o.x >> 16),
                                    (long long)(o.y >> 16), (long long)(o.z >> 16), o.reward.category, o.reward.id, o.reward.rarity);
            }
        std::printf("%d zones\n", zones);
        for (auto &[k, n] : kinds)
            std::printf("category %d id %d: %d\n", k.first, k.second, n);
        return 0;
    }
    if (argc >= 5 && std::string(argv[1]) == "plxrender") { // <file> <node> <out.png> [pixels per unit]: a node drawn on the CPU
        plx::Document doc;
        std::string why;
        // (the game reads every interface file but cursor.plx with scalable text)
        const bool cursor = std::filesystem::path(argv[2]).filename() == "cursor.plx";
        if (!plx::read(argv[2], doc, {plx::keyOf("PlasmaXGraphics")}, &why, cursor ? 0 : plx::LOAD_SCALABLE_TEXT))
            return std::printf("%s\n", why.c_str()), 1;
        const int root = doc.find(argv[3]);
        if (root < 0)
            return std::printf("no node %s\n", argv[3]), 1;
        const float scale = argc >= 6 ? (float)std::atof(argv[5]) : 4.0f;
        // [text]: the string the "text" nodes are given (as the game sets the text of every node of that name).
        const std::wstring given = argc >= 7 ? std::wstring(argv[6], argv[6] + std::strlen(argv[6])) : std::wstring();
        // The text shapes' fonts, from the file's folder (the game's resource1.dat / resource2.dat).
        plasma::Font fonts[2];
        for (int f = 0; f < 2; f++) {
            std::ifstream in(std::filesystem::path(argv[2]).parent_path() / (f ? "resource2.dat" : "resource1.dat"), std::ios::binary);
            fonts[f].load(std::vector<unsigned char>(std::istreambuf_iterator<char>(in), {}));
        }
        struct Tri { glm::vec2 p[3]; glm::vec4 c[3]; const plasma::Glyph *g = nullptr; glm::vec2 uv[3]{}; };
        std::vector<Tri> tris;
        int meshes = 0, failed = 0, texts = 0;
        // the text shape drawing (the plain glyph, then the outline glyph over it), as the HUD draws it.
        auto drawText = [&](plasma::TextStyle style, const glm::mat4 &previous) {
            const plasma::Font &font = fonts[style.font == "resource2.dat" ? 1 : 0];
            if (!font.loaded() || style.pixelSize <= 0 || style.string.empty())
                return;
            texts++;
            const plasma::Raster r = plasma::rasterFor(previous, style);
            const glm::mat4 m = plasma::drawMatrix(previous, style, r);
            for (const plasma::Placed &p : plasma::layout(font, style.string, r, style.flags).glyphs)
                for (int pass = 0; pass < (r.stroke > 0 ? 2 : 1); pass++) {
                    const plasma::Glyph &g = font.glyph(p.c, r, pass == 1);
                    if (!g.exists)
                        continue;
                    const glm::vec4 c = pass ? style.strokeColor : style.color;
                    const glm::vec2 at = plasma::glyphPosition(r, p.pen, g.offset);
                    glm::vec2 corner[4], uv[4];
                    for (int k = 0; k < 4; k++) {
                        uv[k] = glm::vec2(k & 1 ? g.size.x : 0, k & 2 ? g.size.y : 0);
                        corner[k] = glm::vec2(m * glm::vec4(at + glm::vec2(k & 1 ? g.extent.x : 0.0f, k & 2 ? g.extent.y : 0.0f), 0, 1));
                    }
                    static const int halves[2][3] = {{0, 1, 2}, {1, 3, 2}};
                    for (const int *q : halves) {
                        Tri tr{{corner[q[0]], corner[q[1]], corner[q[2]]}, {c, c, c}, &g, {uv[q[0]], uv[q[1]], uv[q[2]]}};
                        tris.push_back(tr);
                    }
                }
        };
        std::function<void(int, glm::mat4, glm::vec4, bool)> walk = [&](int i, glm::mat4 m, glm::vec4 tint, bool named) {
            const plx::Node &n = doc.nodes[(size_t)i];
            named = named || n.name == "text";
            if (n.transformation >= 0 && n.transformation < (int)doc.transformations.size())
                m = m * plasma::transformation(doc.transformations[(size_t)n.transformation]);
            if (n.display >= 0 && n.display < (int)doc.displays.size()) {
                if (!plasma::displayVisible(doc.displays[(size_t)n.display]))
                    return;
                tint *= plasma::displayColor(doc.displays[(size_t)n.display]);
            }
            plasma::TextStyle style;
            if (n.shape >= 0 && n.shape < (int)doc.shapes.size() && plasma::textStyle(doc.shapes[(size_t)n.shape], doc.loaderFlags, style)) {
                if (named && !given.empty())
                    style.string = given;
                drawText(style, m);
            } else if (n.shape >= 0 && n.shape < (int)doc.shapes.size()) {
                plasma::ShapeDrawing d;
                if (plasma::buildSmoothMesh(doc.shapes[(size_t)n.shape], d)) {
                    meshes++;
                    for (const plasma::Triangles *t : {&d.fill, &d.stroke}) {
                        if ((t == &d.fill && !d.filled) || (t == &d.stroke && !d.stroked))
                            continue;
                        for (size_t k = 0; k + 2 < t->indices.size(); k += 3) {
                            Tri tr;
                            for (int j = 0; j < 3; j++) {
                                const plasma::Vertex &v = t->vertices[t->indices[k + (size_t)j]];
                                const glm::vec4 w = m * glm::vec4(v.position, 0, 1);
                                tr.p[j] = glm::vec2(w);
                                tr.c[j] = v.color * tint;
                            }
                            tris.push_back(tr);
                        }
                    }
                } else if (doc.shapes[(size_t)n.shape].kind == "SmoothMeshShape")
                    failed++;
            }
            for (int c : n.children)
                walk(c, m, tint, named);
        };
        glm::mat4 pixels(1); // the interface root's scale
        pixels[0][0] = pixels[1][1] = scale;
        walk(root, pixels, glm::vec4(1), false);
        if (tris.empty())
            return std::printf("nothing to draw (%d meshes)\n", meshes), 1;
        glm::vec2 lo = tris[0].p[0], hi = lo;
        for (auto &t : tris)
            for (auto &q : t.p)
                lo = glm::min(lo, q), hi = glm::max(hi, q);
        lo = glm::floor(lo); // (whole pixels, so the glyphs stay on the pixel grid)
        const int W = std::min(2048, (int)(hi.x - lo.x) + 4), H = std::min(2048, (int)(hi.y - lo.y) + 4);
        std::vector<glm::vec4> img((size_t)W * H, glm::vec4(0.25f, 0.25f, 0.3f, 1));
        for (auto &t : tris) {
            glm::vec2 q[3];
            for (int j = 0; j < 3; j++)
                q[j] = t.p[j] - lo + glm::vec2(2);
            const float area = (q[1].x - q[0].x) * (q[2].y - q[0].y) - (q[2].x - q[0].x) * (q[1].y - q[0].y);
            if (area == 0)
                continue;
            const int x0 = std::max(0, (int)std::floor(std::min({q[0].x, q[1].x, q[2].x}))), x1 = std::min(W - 1, (int)std::ceil(std::max({q[0].x, q[1].x, q[2].x})));
            const int y0 = std::max(0, (int)std::floor(std::min({q[0].y, q[1].y, q[2].y}))), y1 = std::min(H - 1, (int)std::ceil(std::max({q[0].y, q[1].y, q[2].y})));
            for (int y = y0; y <= y1; y++)
                for (int x = x0; x <= x1; x++) {
                    const glm::vec2 c(x + 0.5f, y + 0.5f);
                    const float w0 = ((q[1].x - c.x) * (q[2].y - c.y) - (q[2].x - c.x) * (q[1].y - c.y)) / area;
                    const float w1 = ((q[2].x - c.x) * (q[0].y - c.y) - (q[0].x - c.x) * (q[2].y - c.y)) / area;
                    const float w2 = 1 - w0 - w1;
                    if (w0 < 0 || w1 < 0 || w2 < 0)
                        continue;
                    glm::vec4 col = glm::clamp(t.c[0] * w0 + t.c[1] * w1 + t.c[2] * w2, glm::vec4(0), glm::vec4(1));
                    if (t.g) { // a glyph: its coverage at the nearest texel
                        const glm::vec2 uv = t.uv[0] * w0 + t.uv[1] * w1 + t.uv[2] * w2;
                        const int gx = std::clamp((int)uv.x, 0, t.g->size.x - 1), gy = std::clamp((int)uv.y, 0, t.g->size.y - 1);
                        col.a *= t.g->alpha.empty() ? 0.0f : t.g->alpha[(size_t)gy * t.g->size.x + gx] / 255.0f;
                    }
                    glm::vec4 &dst = img[(size_t)y * W + x];
                    dst = glm::vec4(glm::vec3(col) * col.a + glm::vec3(dst) * (1 - col.a), 1);
                }
        }
        std::vector<uint8_t> px((size_t)W * H * 4);
        for (size_t i = 0; i < img.size(); i++)
            for (int k = 0; k < 4; k++)
                px[i * 4 + (size_t)k] = (uint8_t)std::lround(img[i][k] * 255);
        stbi_write_png(argv[4], W, H, 4, px.data(), W * 4);
        std::printf("%d meshes (%d not built), %d texts, %zu triangles, %dx%d\n", meshes, failed, texts, tris.size(), W, H);
        return 0;
    }
    if (argc >= 5 && std::string(argv[1]) == "pyrogear") { // <state file> <character name> <recorded character dir>:
        // the levels the game's PyroRelativeLevels.state keeps for that character's gear
        Gear gear;
        std::string why;
        if (!gear.load(argv[4], why)) {
            std::printf("%s\n", why.c_str());
            return 1;
        }
        pyrocreatures::RelativeLevels levels;
        levels.open(0x7b, "world", argv[3], true, argv[2], {}, false);
        int gearCount = 0, kept = 0;
        for (size_t i = 0; i < gear.itemCount(); i++) {
            const Gear::Item &it = gear.item(i);
            if (it.category < 3 || it.category > 9)
                continue;
            const int level = levels.keptLevel(it.category, it.id, it.modifier, it.material);
            std::printf("item %d %d material %d modifier %u: %s\n", it.category, it.id, it.material, it.modifier,
                        level ? ("level " + std::to_string(level)).c_str() : "none");
            gearCount++, kept += level > 0;
        }
        std::printf("%d of %d gear items have a kept level\n", kept, gearCount);
        return kept > 0 ? 0 : 1;
    }
    if (argc >= 7 && std::string(argv[1]) == "pyroregions") { // <state file> <character name> <region x> <region y> <radius>:
        // which of the regions around there the game's PyroRelativeLevels.state locked for that character
        const int rx = std::atoi(argv[4]), ry = std::atoi(argv[5]), radius = std::atoi(argv[6]);
        pyrocreatures::RelativeLevels all;
        all.open(0x7b, "world", argv[3], true, argv[2], {});
        int found = 0;
        for (int x = rx - radius; x <= rx + radius; x++)
            for (int y = ry - radius; y <= ry + radius; y++)
                if (all.basis({x, y}, -1) != -1) {
                    std::printf("region %d %d: level %d\n", x, y, all.basis({x, y}, -1));
                    found++;
                }
        std::printf("%d of the file's %zu region entries found\n", found, all.size());
        return found > 0 ? 0 : 1;
    }
    if (argc >= 5 && std::string(argv[1]) == "travelzones") { // <zone x> <zone y> <radius>: zones made anew there, timed
        worldgen::setWorldSeed(0x7b);
        OverworldEvents::replaceTravelLocations(0x7b, "world");
        const int zx = std::atoi(argv[2]), zy = std::atoi(argv[3]), radius = std::atoi(argv[4]);
        int changed = 0, all = 0;
        const auto t0 = std::chrono::steady_clock::now();
        for (int x = zx - radius; x <= zx + radius; x++)
            for (int y = zy - radius; y <= zy + radius; y++)
                changed += worldgen::hooksChange(x, y), all++;
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        std::printf("%d of %d zones made anew; %.2f ms a zone to tell\n", changed, all, ms / all);
        return 0;
    }
    if (argc >= 4 && std::string(argv[1]) == "travel") { // <region x> <region y> [radius]: OverworldEvents' travel sites near there
        worldgen::setWorldSeed(0x7b);
        OverworldEvents::replaceTravelLocations(0x7b, "world");
        const int rx = std::atoi(argv[2]), ry = std::atoi(argv[3]), radius = argc >= 5 ? std::atoi(argv[4]) : 0;
        int sites = 0, zones = 0;
        for (int x = rx - radius; x <= rx + radius; x++)
            for (int y = ry - radius; y <= ry + radius; y++) {
                worldgen::RegionRoles plan;
                {
                    worldgen::OriginalGeneration original;
                    plan = worldgen::regionRoles({x, y});
                }
                for (const auto &role : plan.roles) {
                    if (role[2] < 6 || role[2] > 9)
                        continue;
                    const bool big = worldgen::bigCell(role[0], role[1]);
                    int game;
                    {
                        worldgen::OriginalGeneration original;
                        game = worldgen::placeKind(role[0], role[1], big);
                    }
                    const worldgen::Int2 c = worldgen::cellPoint(role[0], role[1], big);
                    const bool changes = worldgen::hooksChange(worldgen::floorDiv(c.x, 6), worldgen::floorDiv(c.y, 6));
                    std::printf("region %d %d: cell %d %d (%s) role %d at %d,%d: kind %d -> %d%s\n", x, y, role[0], role[1],
                                big ? "big" : "small", role[2], c.x, c.y, game, worldgen::placeKind(role[0], role[1], big),
                                changes ? ", its zone made anew" : "");
                    sites++, zones += changes;
                }
            }
        std::printf("%d travel sites, %d of their zones made anew\n", sites, zones);
        return sites > 0 && zones == sites ? 0 : 1;
    }
    if (argc >= 6 && std::string(argv[1]) == "mapregion") { // <seed> <block x> <block y> <level>: a world map region's model, timed
        worldgen::setWorldSeed(std::atoi(argv[2]));
        const int level = std::atoi(argv[5]), scale = std::max(1, (int)(std::pow(2.0f, (float)level) * 4.0f));
        const int32_t bx = (int32_t)std::atoll(argv[3]), by = (int32_t)std::atoll(argv[4]);
        auto fd = [](int32_t a, int32_t n) { return a >= 0 ? a / n : (a + 1) / n - 1; };
        const auto t0 = std::chrono::steady_clock::now();
        const worldgen::MapRegion m = worldgen::mapRegion(fd(bx, 16 * scale), fd(by, 16 * scale), level);
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        // The top voxel of each column (its height in the model, -1 for none).
        std::printf("region %dx%dx%d lowest %d in %.1f ms\n", m.side, m.side, m.sizeZ, m.lowest, ms);
        for (int y = m.side - 1; y >= 0; y--) {
            for (int x = 0; x < m.side; x++) {
                int topZ = -1;
                for (int z = m.sizeZ - 1; z >= 0 && topZ < 0; z--) {
                    const uint8_t *v = m.voxel(x, y, z);
                    if (v[0] || v[1] || v[2])
                        topZ = z;
                }
                std::printf("%3d", topZ);
            }
            std::printf("\n");
        }
        return 0;
    }
    if (argc >= 6 && std::string(argv[1]) == "mapdump") { // <seed> <block x> <block y> <level>: a map region's model and markers
        worldgen::setWorldSeed(std::atoi(argv[2]));
        const int level = std::atoi(argv[5]), unit = worldgen::mapUnit(level);
        const int32_t rx = worldgen::floorDivBy((int32_t)std::atoll(argv[3]), 16 * unit);
        const int32_t ry = worldgen::floorDivBy((int32_t)std::atoll(argv[4]), 16 * unit);
        const worldgen::MapRegion m = worldgen::mapRegion(rx, ry, level);
        // The summary a recording of the game's region record (worldmap-model.md §7) would be
        // compared with: sizes, lowest, the voxels (count and FNV-1a of every byte), the lists.
        uint32_t fnv = 2166136261u;
        size_t solid = 0;
        for (size_t i = 0; i < m.rgb.size(); i++)
            fnv = (fnv ^ m.rgb[i]) * 16777619u;
        for (size_t i = 0; i + 2 < m.rgb.size(); i += 3)
            solid += m.rgb[i] || m.rgb[i + 1] || m.rgb[i + 2];
        std::printf("region %d %d level %d unit %d reach %d\n", rx, ry, level, m.unit, m.reach);
        std::printf("model %dx%dx%d lowest %d base %d voxels %zu fnv %08x\n", m.side, m.side, m.sizeZ, m.lowest, m.base(), solid, fnv);
        static const char *LISTS[4] = {"place", "mission", "land", "flight"};
        for (const worldgen::MapRegion::Marker &k : m.markers) {
            std::printf("%s %d at %d %d %d", LISTS[k.list], k.type, k.x, k.y, k.z);
            if (k.list == 1)
                std::printf(" role %d level %d item %d/%d", k.kind, k.level, k.item.category, k.item.id);
            std::printf("\n");
        }
        for (const worldgen::MapRegion::CastleMark &c : m.castles)
            std::printf("castle %d %d %d %d at %d %d floor %d\n", c.cell[0], c.cell[1], c.cell[2], c.cell[3], c.x, c.y, c.floor);
        for (const auto &[key, points] : m.borders)
            std::printf("border of region %d %d: %zu points\n", key.first, key.second, points.size());
        return 0;
    }
    if (argc >= 7 && std::string(argv[1]) == "plxpanel") { // <file> <node> <width> <height> <out.png>: a widget resized as the game does
        plx::Document doc;
        std::string why;
        if (!plx::read(argv[2], doc, {plx::keyOf("PlasmaXGraphics")}, &why))
            return std::printf("%s\n", why.c_str()), 1;
        plasma::Scene scene(doc);
        plasma::SceneNode *panel = scene.instance(argv[3], &scene.root());
        if (!panel || !panel->widget)
            return std::printf("no widget node %s\n", argv[3]), 1;
        const glm::vec2 before = scene.viewSize(*panel);
        scene.resizeTo(*panel, glm::vec2((float)std::atof(argv[4]), (float)std::atof(argv[5])));
        scene.moveTo(*panel, glm::vec2(20, 20));
        const glm::vec2 after = scene.viewSize(*panel), at = scene.viewPosition(*panel);
        std::printf("view size %.1f x %.1f -> %.1f x %.1f at %.1f %.1f; content %.1f x %.1f\n", before.x, before.y, after.x, after.y, at.x, at.y,
                    scene.contentSize(*panel).x, scene.contentSize(*panel).y);
        plasma::Scene::Drawn drawn;
        std::function<void(const plasma::SceneNode &, glm::vec4)> walk = [&](const plasma::SceneNode &n, glm::vec4 tint) {
            if (!n.visible)
                return;
            tint *= n.fill;
            if (n.shape >= 0 && doc.shapes[(size_t)n.shape].kind == "SmoothMeshShape")
                scene.drawShape(n, tint, drawn);
            for (auto &c : n.children)
                walk(*c, tint);
        };
        walk(*panel, glm::vec4(1));
        const int W = (int)(after.x + 60), H = (int)(after.y + 60);
        std::vector<glm::vec4> img((size_t)W * H, glm::vec4(0.25f, 0.25f, 0.3f, 1));
        for (size_t k = 0; k + 2 < drawn.positions.size(); k += 3) {
            const glm::vec2 *q = &drawn.positions[k];
            const glm::vec4 *c = &drawn.colors[k];
            const float area = (q[1].x - q[0].x) * (q[2].y - q[0].y) - (q[2].x - q[0].x) * (q[1].y - q[0].y);
            if (area == 0)
                continue;
            const int x0 = std::max(0, (int)std::floor(std::min({q[0].x, q[1].x, q[2].x}))), x1 = std::min(W - 1, (int)std::ceil(std::max({q[0].x, q[1].x, q[2].x})));
            const int y0 = std::max(0, (int)std::floor(std::min({q[0].y, q[1].y, q[2].y}))), y1 = std::min(H - 1, (int)std::ceil(std::max({q[0].y, q[1].y, q[2].y})));
            for (int y = y0; y <= y1; y++)
                for (int x = x0; x <= x1; x++) {
                    const glm::vec2 p(x + 0.5f, y + 0.5f);
                    const float w0 = ((q[1].x - p.x) * (q[2].y - p.y) - (q[2].x - p.x) * (q[1].y - p.y)) / area;
                    const float w1 = ((q[2].x - p.x) * (q[0].y - p.y) - (q[0].x - p.x) * (q[2].y - p.y)) / area;
                    const float w2 = 1 - w0 - w1;
                    if (w0 < 0 || w1 < 0 || w2 < 0)
                        continue;
                    const glm::vec4 col = glm::clamp(c[0] * w0 + c[1] * w1 + c[2] * w2, glm::vec4(0), glm::vec4(1));
                    glm::vec4 &dst = img[(size_t)y * W + x];
                    dst = glm::vec4(glm::vec3(col) * col.a + glm::vec3(dst) * (1 - col.a), 1);
                }
        }
        std::vector<uint8_t> px((size_t)W * H * 4);
        for (size_t i = 0; i < img.size(); i++)
            for (int k = 0; k < 4; k++)
                px[i * 4 + (size_t)k] = (uint8_t)std::lround(img[i][k] * 255);
        stbi_write_png(argv[6], W, H, 4, px.data(), W * 4);
        return 0;
    }
    if (argc >= 4 && std::string(argv[1]) == "plxseq") { // <file> <node>: the animation sequences under a node
        plx::Document doc;
        std::string why;
        if (!plx::read(argv[2], doc, {plx::keyOf("PlasmaXGraphics")}, &why))
            return std::printf("%s\n", why.c_str()), 1;
        auto show = [&](const char *what, const plx::Object &o, int d) {
            for (const plx::Chunk &p : o.properties)
                for (const plx::Chunk &s : p.children)
                    if (s.name == "Attribute.sequence") {
                        std::printf("%*s%s %s (%zu frames):", d * 2 + 2, "", what, p.name.c_str(), p.frames().size());
                        for (const plx::Chunk &k : s.children)
                            if (k.name == "Attribute.sequence.name" || k.name == "Attribute.sequence.wname")
                                std::printf(" \"%s\"", k.text().c_str());
                            else if (k.name == "Attribute.sequence.key") {
                                const plx::Chunk *e = k.child("Attribute.sequence.key.frame"), *t = k.child("Attribute.sequence.key.time"),
                                                 *m = k.child("Attribute.sequence.key.smoothness");
                                std::printf(" [%d@%d %.2f]", e ? e->as<int32_t>() : 0, t ? t->as<int32_t>() : 0, m ? m->as<float>() : 0.0f);
                            }
                        std::printf("\n");
                    }
        };
        std::function<void(int, int)> walk = [&](int i, int d) {
            const plx::Node &n = doc.nodes[(size_t)i];
            std::printf("%*s%s\n", d * 2, "", n.name.c_str());
            if (n.transformation >= 0 && n.transformation < (int)doc.transformations.size())
                show("transformation", doc.transformations[(size_t)n.transformation], d);
            if (n.display >= 0 && n.display < (int)doc.displays.size()) {
                const plx::Object &disp = doc.displays[(size_t)n.display];
                const glm::vec4 c = plasma::displayColor(disp);
                std::printf("%*s(visible %d, clips %d, colour %.2f %.2f %.2f %.2f)\n", d * 2 + 2, "", (int)plasma::displayVisible(disp),
                            (int)plasma::displayClips(disp), c.r, c.g, c.b, c.a);
                show("display", disp, d);
            }
            if (n.shape >= 0 && n.shape < (int)doc.shapes.size())
                if (const plx::Chunk *vc = doc.shapes[(size_t)n.shape].property("SmoothMeshShape.vertexColors")) {
                    const auto fr = vc->frames();
                    for (size_t k = 0; k < fr.size(); k++) {
                        float a = 0;
                        const size_t m = fr[k]->size() / 16;
                        for (size_t q = 0; q < m; q++) {
                            float v;
                            std::memcpy(&v, fr[k]->data() + q * 16 + 12, 4);
                            a += v;
                        }
                        std::printf("%*s(colour frame %zu: %zu vertices, mean alpha %.2f)\n", d * 2 + 2, "", k, m, m ? a / (float)m : 0.0f);
                    }
                }
            if (n.shape >= 0 && n.shape < (int)doc.shapes.size())
                show("shape", doc.shapes[(size_t)n.shape], d);
            for (int c : n.children)
                walk(c, d + 1);
        };
        for (int a = 3; a < argc; a++)
            if (const int i = doc.find(argv[a]); i >= 0)
                walk(i, 0);
        return 0;
    }
    if (argc >= 4 && std::string(argv[1]) == "plxtext") { // <file> <node>: the text shapes under a node
        plx::Document doc;
        std::string why;
        if (!plx::read(argv[2], doc, {plx::keyOf("PlasmaXGraphics")}, &why))
            return std::printf("%s\n", why.c_str()), 1;
        std::function<void(int, int)> walk = [&](int i, int d) {
            const plx::Node &n = doc.nodes[(size_t)i];
            plasma::TextStyle t;
            if (n.shape >= 0 && n.shape < (int)doc.shapes.size() && plasma::textStyle(doc.shapes[(size_t)n.shape], doc.loaderFlags, t)) {
                std::string s;
                for (wchar_t ch : t.string)
                    s.push_back((char)ch);
                glm::mat4 m = n.transformation >= 0 ? plasma::transformation(doc.transformations[(size_t)n.transformation]) : glm::mat4(1);
                std::printf("%*s%s \"%s\" font %s size %g stroke %g spacing %g line %g wrap %g flags %d color %.2f %.2f %.2f %.2f stroke %.2f %.2f %.2f %.2f at %.1f %.1f scale %.3f %.3f\n",
                            d * 2, "", n.name.c_str(), s.c_str(), t.font.c_str(), t.pixelSize, t.strokeRadius, t.spacing, t.lineSpacing,
                            t.wrapWidth, t.flags, t.color.r, t.color.g, t.color.b, t.color.a, t.strokeColor.r, t.strokeColor.g,
                            t.strokeColor.b, t.strokeColor.a, m[3][0], m[3][1], m[0][0], m[1][1]);
            } else if (n.transformation >= 0) {
                const glm::mat4 m = plasma::transformation(doc.transformations[(size_t)n.transformation]);
                const glm::vec2 p = plasma::pivot(doc.transformations[(size_t)n.transformation]);
                std::printf("%*s%s at %.1f %.1f pivot %.1f %.1f scale %.3f %.3f", d * 2, "", n.name.c_str(), m[3][0], m[3][1], p.x, p.y,
                            m[0][0], m[1][1]);
                if (n.widget >= 0 && n.widget < (int)doc.widgets.size()) {
                    std::printf(" widget %d %s:", n.widget, doc.widgets[(size_t)n.widget].kind.c_str());
                    for (const plx::Chunk &c : doc.widgets[(size_t)n.widget].properties)
                        if (c.bytes.size() == 8)
                            std::printf(" %s %.1f %.1f", c.name.c_str(), c.as<float>(0), c.as<float>(4));
                }
                std::printf("\n");
            } else
                std::printf("%*s%s\n", d * 2, "", n.name.c_str());
            for (int c : n.children)
                walk(c, d + 1);
        };
        for (int a = 3; a < argc; a++)
            if (const int i = doc.find(argv[a]); i >= 0)
                walk(i, 0);
        return 0;
    }
    if (argc >= 3 && std::string(argv[1]) == "plx") { // <file> [depth]: a .plx's objects and node tree
        plx::Document doc;
        std::string why;
        if (!plx::read(argv[2], doc, {plx::keyOf("PlasmaXGraphics")}, &why))
            return std::printf("%s\n", why.c_str()), 1;
        std::printf("version %d, page %.0f x %.0f; %zu shapes, %zu transformations, %zu displays, %zu widgets, %zu textures (%zu with pixels), %zu nodes\n",
                    doc.version, doc.pageWidth, doc.pageHeight, doc.shapes.size(), doc.transformations.size(), doc.displays.size(),
                    doc.widgets.size(), doc.textures.size(), doc.pixels.size(), doc.nodes.size());
        std::map<std::string, int> kinds;
        for (const plx::Object &o : doc.shapes)
            kinds[o.kind]++;
        for (auto &[k, n] : kinds)
            std::printf("  %s: %d\n", k.c_str(), n);
        { // the smooth meshes' flags (contour 1, stroke 2, no fill 4, open 8, extruded 16), subdivisions, textures, sequences
            std::map<int, int> flags, subdivisions;
            int textured = 0, animated = 0, frames = 0;
            for (const plx::Object &o : doc.shapes) {
                if (o.kind != "SmoothMeshShape")
                    continue;
                const plx::Chunk *f = o.property("SmoothMeshShape.flags");
                flags[f ? f->as<int32_t>() : -1]++;
                const plx::Chunk *s = o.property("SmoothMeshShape.subdivisions");
                subdivisions[s ? s->as<int32_t>() : -1]++;
                textured += o.property("SmoothMeshShape.texture") != nullptr;
                for (const plx::Chunk &p : o.properties) {
                    animated += p.child("Attribute.sequence") != nullptr;
                    if (p.name == "SmoothMeshShape.vertexPositions")
                        frames += p.frames().size() > 1;
                }
            }
            for (auto &[k, n] : flags)
                std::printf("  mesh flags %d: %d\n", k, n);
            for (auto &[k, n] : subdivisions)
                std::printf("  subdivisions %d: %d\n", k, n);
            std::printf("  textured %d, properties with sequences %d, meshes with several position frames %d\n", textured, animated, frames);
        }
        for (auto &[id, px] : doc.pixels)
            std::printf("  texture %d \"%s\" %dx%d format %d, %zu bytes\n", id, px.name.c_str(), px.width, px.height, px.format, px.data.size());
        const int depth = argc >= 4 ? std::atoi(argv[3]) : 2;
        std::function<void(int, int)> show = [&](int i, int d) {
            if (i < 0 || i >= (int)doc.nodes.size() || d > depth)
                return;
            const plx::Node &n = doc.nodes[(size_t)i];
            std::printf("%*s%s (shape %d%s)\n", d * 2, "", n.name.empty() ? "-" : n.name.c_str(), n.shape,
                        n.shape >= 0 && n.shape < (int)doc.shapes.size() ? (" " + doc.shapes[(size_t)n.shape].kind).c_str() : "");
            for (int c : n.children)
                show(c, d + 1);
        };
        show(0, 0);
        return 0;
    }
    if (argc == 4 && std::string(argv[1]) == "genprops") { // a generated zone's props, as the engine gets them
        Assets a;
        if (!a.locate(""))
            return 1;
        worldgen::setWorldSeed(123);
        const int zx = std::atoi(argv[2]), zy = std::atoi(argv[3]);
        ExtractedWorld w;
        w.openGenerated(zx * 64 + 32, zy * 64 + 32, a.gameDir() / "Extract");
        std::map<std::string, int> names;
        for (const Scenery &s : w.propsAbsolute(zx, zy))
            names[s.model]++;
        for (auto &[n, c] : names)
            std::printf("%4d %s\n", c, n.c_str());
        return 0;
    }
    if (argc >= 4 && std::string(argv[1]) == "models") { // a zone's models (zone+0x18) against the extracted ones, by type
        worldgen::setWorldSeed(123);
        std::map<int, std::array<int, 4>> byType;
        if (!compareModels(std::atoi(argv[2]), std::atoi(argv[3]), byType, 10))
            return std::printf("zone not extracted\n"), 1;
        for (auto &[t, n] : byType)
            std::printf("type %d: game %d, ours %d, same place %d, same floats %d\n", t, n[0], n[1], n[2], n[3]);
        return 0;
    }
    if (argc >= 2 && std::string(argv[1]) == "allmodels") { // every extracted zone's models against ours, by type
        const int maxZones = argc > 2 ? std::atoi(argv[2]) : 1 << 30;
        Assets a;
        if (!a.locate(""))
            return 1;
        std::filesystem::path dir = zonesDir(a.gameDir());
        worldgen::setWorldSeed(seedOf(dir));
        std::map<int, std::array<int, 4>> byType;
        std::map<int, std::pair<int, int>> worstZone; // type -> (zone index of most misses... kept as first zone with a miss)
        int zones = 0;
        for (auto &e : std::filesystem::directory_iterator(dir)) {
            int ax, ay;
            if (e.path().extension() != ".cwz" || !ExtractedWorld::zoneCoordinates(e.path(), ax, ay))
                continue;
            if (std::abs((long long)ax) > 100000 || std::abs((long long)ay) > 100000)
                continue;
            if (zones++ >= maxZones)
                break;
            std::map<int, std::array<int, 4>> zoneTypes;
            if (!compareModels(ax, ay, zoneTypes, 0))
                continue;
            for (auto &[t, n] : zoneTypes) {
                for (int i = 0; i < 4; i++)
                    byType[t][i] += n[i];
                if ((n[0] != n[2] || n[1] != n[2]) && !worstZone.count(t))
                    worstZone[t] = {ax, ay};
            }
        }
        std::printf("zones %d\n", zones);
        for (auto &[t, n] : byType)
            std::printf("type %d: game %d, ours %d, same place %d, same floats %d%s\n", t, n[0], n[1], n[2], n[3],
                        worstZone.count(t) ? (" (e.g. zone " + std::to_string(worstZone[t].first) + " " + std::to_string(worstZone[t].second) + ")").c_str() : "");
        return 0;
    }
    if (argc >= 5 && std::string(argv[1]) == "rangedscan") { // <zone x> <zone y> <radius>: monsters that shoot
        Assets a;
        if (!a.locate(""))
            return 1;
        worldgen::setWorldSeed(seedOf(zonesDir(a.gameDir())));
        worldgen::loadCreatureTables(a.gameDir() / "Extract" / "oracle");
        const int cx = std::atoi(argv[2]), cy = std::atoi(argv[3]), radius = std::atoi(argv[4]);
        int shown = 0;
        std::map<int, int> byRace;
        for (int zx = cx - radius; zx <= cx + radius; zx++)
            for (int zy = cy - radius; zy <= cy + radius; zy++)
                for (const worldgen::SpawnPoint &sp : zoneSpawnList(zx, zy)) {
                    if (sp.hostility != 1)
                        continue;
                    int seed = 1;
                    const worldgen::CreatureRecord r = residentOf(sp, [&] { return (seed = seed * 1103515245 + 12345) >> 16 & 0x7fff; });
                    const uint8_t cls = r[0x14c], rightCat = r[0x6d8];
                    int32_t rightId;
                    std::memcpy(&rightId, &r[0x6dc], 4);
                    const bool ranged = cls == 2 || cls == 3 || cls == 0x97 || cls == 0x98 || sp.race == 0x75 || sp.race == 0x56 || sp.race == 0x68 || (rightCat == 3 && rightId >= 6 && rightId <= 12 && rightId != 9);
                    if (!ranged)
                        continue;
                    byRace[sp.race]++;
                    if (shown++ < 25)
                        std::printf("zone %d %d: race %d class %d weapon %d/%d at block %lld %lld\n", zx, zy, sp.race, cls, rightCat,
                                    rightId, (long long)(sp.x >> 16), (long long)(sp.y >> 16));
                }
        for (auto &[race, n] : byRace)
            std::printf("race %d: %d\n", race, n);
        return 0;
    }
    if (argc >= 2 && std::string(argv[1]) == "talkverify") // [examples]: Extract/talk-scripts.txt made again by TalkGen
        return talkVerify(argc > 2 ? std::atoi(argv[2]) : 3);
    if (argc == 4 && std::string(argv[1]) == "regionat") { // <block x> <block y>: the region (0x2ada20)
        const worldgen::Int2 r = worldgen::regionAt(std::atoi(argv[2]), std::atoi(argv[3]));
        std::printf("region %d %d\n", r.x, r.y);
        return 0;
    }
    if (argc >= 4 && std::string(argv[1]) == "braziers") { // <region x> <region y>: role 22's braziers and what they take
        Assets a;
        if (!a.locate(""))
            return 1;
        worldgen::setWorldSeed(seedOf(zonesDir(a.gameDir())));
        worldgen::loadCreatureTables(a.gameDir() / "Extract" / "oracle");
        int found = 0;
        for (const auto &r : worldgen::regionRoles({std::atoi(argv[2]), std::atoi(argv[3])}).roles) {
            if (r[2] != 22)
                continue;
            // The place's zones (a big place's 2x2 cells), the braziers in them.
            const int side = worldgen::bigCell(r[0], r[1]) ? 32 : 16;
            for (int zx = r[0] * 16; zx < r[0] * 16 + side; zx++)
                for (int zy = r[1] * 16; zy < r[1] * 16 + side; zy++)
                    for (const worldgen::StaticObject &o : generatedObjects(zx, zy))
                        if (o.type == 0x7e) {
                            std::printf("place %d %d: brazier at block %lld %lld takes %d:%d level %d\n", r[0], r[1],
                                        (long long)(o.x >> 16), (long long)(o.y >> 16), o.takes.category, o.takes.id, o.takes.rarity);
                            found++;
                        }
        }
        std::printf("%d braziers\n", found);
        return 0;
    }
    if (argc >= 6 && std::string(argv[1]) == "findrace") { // <zone x> <zone y> <radius> <race...>: where they spawn
        Assets a;
        if (!a.locate(""))
            return 1;
        worldgen::setWorldSeed(seedOf(zonesDir(a.gameDir())));
        worldgen::loadCreatureTables(a.gameDir() / "Extract" / "oracle");
        const int cx = std::atoi(argv[2]), cy = std::atoi(argv[3]), radius = std::atoi(argv[4]);
        std::set<int> wanted;
        for (int i = 5; i < argc; i++)
            wanted.insert(std::atoi(argv[i]));
        std::map<int, int> shown;
        for (int zx = cx - radius; zx <= cx + radius; zx++)
            for (int zy = cy - radius; zy <= cy + radius; zy++)
                for (const worldgen::SpawnPoint &sp : zoneSpawnList(zx, zy)) {
                    if (wanted.count(-1)) { // race -1: every spawn point, counted by hostility
                        shown[1000 + sp.hostility]++;
                        continue;
                    }
                    if (wanted.count(sp.race) && shown[sp.race]++ < 4)
                        std::printf("race %d hostility %d at block %lld %lld\n", sp.race, sp.hostility, (long long)(sp.x >> 16),
                                    (long long)(sp.y >> 16));
                }
        for (auto &[race, n] : shown)
            if (race >= 1000)
                std::printf("hostility %d: %d\n", race - 1000, n);
            else
                std::printf("race %d: %d\n", race, n);
        return 0;
    }
    if (argc >= 5 && std::string(argv[1]) == "skillscan") { // <zone x> <zone y> <radius>: monsters' special moves (+0xbd0)
        Assets a;
        if (!a.locate(""))
            return 1;
        worldgen::setWorldSeed(seedOf(zonesDir(a.gameDir())));
        worldgen::loadCreatureTables(a.gameDir() / "Extract" / "oracle");
        const int cx = std::atoi(argv[2]), cy = std::atoi(argv[3]), radius = std::atoi(argv[4]);
        std::map<int, int> bySkill;
        std::map<int, std::set<int>> races;
        std::map<int, std::pair<int, int>> where;
        int monsters = 0;
        for (int zx = cx - radius; zx <= cx + radius; zx++)
            for (int zy = cy - radius; zy <= cy + radius; zy++)
                for (const worldgen::SpawnPoint &sp : zoneSpawnList(zx, zy)) {
                    if (sp.hostility != 1)
                        continue;
                    monsters++;
                    for (int s : std::set<int>(sp.gear.abilities.begin(), sp.gear.abilities.end())) {
                        bySkill[s]++;
                        races[s].insert(sp.race);
                        where.emplace(s, std::pair<int, int>{(int)(sp.x >> 16), (int)(sp.y >> 16)});
                    }
                }
        std::printf("%d monsters\n", monsters);
        for (auto &[s, n] : bySkill) {
            std::string rs;
            for (int r : races[s])
                rs += " " + std::to_string(r);
            std::printf("skill 0x%x: %d monsters, e.g. at block %d %d; races%s\n", s, n, where[s].first, where[s].second, rs.c_str());
        }
        return 0;
    }
    if (argc >= 2 && std::string(argv[1]) == "allitems") { // every extracted zone's items (zone+0x30) against ours
        const int maxZones = argc > 2 ? std::atoi(argv[2]) : 1 << 30;
        Assets a;
        if (!a.locate(""))
            return 1;
        std::filesystem::path dir = zonesDir(a.gameDir());
        worldgen::setWorldSeed(seedOf(dir));
        worldgen::loadCreatureTables(a.gameDir() / "Extract" / "oracle");
        using Key = std::tuple<int, int, int64_t, int64_t, int64_t>; // type, kind, position
        std::map<std::pair<int, int>, std::array<int, 4>> byKind;    // game, ours, same, same angle
        int zones = 0, shown = 0;
        // CW_ITEMS_SHOW=n lists up to n unmatched items, CW_ITEMS_KIND="type/sub" only those.
        const int showMax = std::getenv("CW_ITEMS_SHOW") ? std::atoi(std::getenv("CW_ITEMS_SHOW")) : 24;
        const std::string only = std::getenv("CW_ITEMS_KIND") ? std::getenv("CW_ITEMS_KIND") : "";
        auto showItem = [&](int t, int k) { return only.empty() || only == std::to_string(t) + "/" + std::to_string(k); };
        for (auto &e : std::filesystem::directory_iterator(dir)) {
            int ax, ay;
            if (e.path().extension() != ".cwz" || !ExtractedWorld::zoneCoordinates(e.path(), ax, ay))
                continue;
            if (std::abs((long long)ax) > 100000 || std::abs((long long)ay) > 100000)
                continue;
            if (zones++ >= maxZones)
                break;
            std::filesystem::path rec = e.path();
            rec.replace_extension(".rec");
            std::ifstream in(rec, std::ios::binary);
            std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), {});
            std::multiset<Key> game, ours;
            std::map<Key, float> gameAngle, ourAngle;
            for (size_t at = 4; at + 12 <= data.size();) {
                uint32_t offset, stride, count;
                std::memcpy(&offset, &data[at], 4), std::memcpy(&stride, &data[at + 4], 4), std::memcpy(&count, &data[at + 8], 4);
                at += 12;
                if (offset == 0x30 && stride == 0xe0)
                    for (uint32_t i = 0; i < count; i++) {
                        const uint8_t *r = &data[at + (size_t)i * stride];
                        int sub;
                        int64_t p[3];
                        float angle;
                        std::memcpy(&sub, r + 4, 4), std::memcpy(p, r + 0xa0, 24), std::memcpy(&angle, r + 0xc0, 4);
                        Key k{r[0], sub, p[0], p[1], p[2]};
                        game.insert(k), gameAngle[k] = angle;
                    }
                at += (size_t)stride * count;
            }
            for (const worldgen::GroundItem &g : worldgen::zone(ax, ay)->items) {
                Key k{g.item.category, g.item.id, g.x, g.y, g.z};
                ours.insert(k), ourAngle[k] = g.angle;
            }
            for (auto &k : game)
                byKind[{std::get<0>(k), std::get<1>(k)}][0]++;
            for (auto &k : ours)
                byKind[{std::get<0>(k), std::get<1>(k)}][1]++;
            for (auto &k : game)
                if (ours.count(k)) {
                    byKind[{std::get<0>(k), std::get<1>(k)}][2]++;
                    byKind[{std::get<0>(k), std::get<1>(k)}][3] += gameAngle[k] == ourAngle[k];
                } else if (showItem(std::get<0>(k), std::get<1>(k)) && shown++ < showMax)
                    std::printf("zone %d %d game only: item %d/%d at %.2f %.2f %.2f\n", ax, ay, std::get<0>(k), std::get<1>(k),
                                std::get<2>(k) / 65536.0, std::get<3>(k) / 65536.0, std::get<4>(k) / 65536.0);
            for (auto &k : ours)
                if (!game.count(k) && showItem(std::get<0>(k), std::get<1>(k)) && shown++ < showMax)
                    std::printf("zone %d %d ours only: item %d/%d at %.2f %.2f %.2f\n", ax, ay, std::get<0>(k), std::get<1>(k),
                                std::get<2>(k) / 65536.0, std::get<3>(k) / 65536.0, std::get<4>(k) / 65536.0);
        }
        std::printf("zones %d\n", zones);
        for (auto &[k, n] : byKind)
            std::printf("item %d/%d: game %d, ours %d, same %d, same angle %d\n", k.first, k.second, n[0], n[1], n[2], n[3]);
        return 0;
    }
    if (argc >= 2 && std::string(argv[1]) == "allobjects") { // every extracted zone's static objects against ours
        const int maxZones = argc > 2 ? std::atoi(argv[2]) : 1 << 30;
        Assets a;
        if (!a.locate(""))
            return 1;
        std::filesystem::path dir = zonesDir(a.gameDir());
        worldgen::setWorldSeed(seedOf(dir));
        worldgen::loadCreatureTables(a.gameDir() / "Extract" / "oracle");
        if (const char *day = std::getenv("CW_DAY"))
            worldgen::setWorldDay(std::atoi(day));
        int zones = 0, zonesDiffer = 0, shown = 0;
        size_t game = 0, ours = 0, gameOnly = 0, oursOnly = 0;
        std::map<int, std::array<int, 2>> byType; // game only, ours only
        for (auto &e : std::filesystem::directory_iterator(dir)) {
            int ax, ay;
            if (e.path().extension() != ".cwz" || !ExtractedWorld::zoneCoordinates(e.path(), ax, ay))
                continue;
            if (std::abs((long long)ax) > 100000 || std::abs((long long)ay) > 100000)
                continue; // not vanilla (see genZonesMatchExtracted)
            bool found;
            std::vector<worldgen::StaticObject> g = readGameObjects(ax, ay, found);
            if (!found)
                continue;
            if (zones++ >= maxZones)
                break;
            std::vector<worldgen::StaticObject> o = generatedObjects(ax, ay);
            game += g.size(), ours += o.size();
            auto gOnly = objectsMissing(g, o), oOnly = objectsMissing(o, g);
            gameOnly += gOnly.size(), oursOnly += oOnly.size();
            if (gOnly.empty() && oOnly.empty())
                continue;
            zonesDiffer++;
            for (auto &x : gOnly)
                byType[x.type][0]++;
            for (auto &x : oOnly)
                byType[x.type][1]++;
            if (shown++ < 30) {
                std::printf("zone %d %d:\n", ax, ay);
                for (auto &x : gOnly)
                    printObject("game only", x);
                for (auto &x : oOnly)
                    printObject("ours only", x);
            }
        }
        std::printf("zones %d (%d differ): objects game %zu, ours %zu; game only %zu, ours only %zu\n", zones, zonesDiffer, game, ours,
                    gameOnly, oursOnly);
        for (auto &[t, n] : byType)
            std::printf("  type %d: game only %d, ours only %d\n", t, n[0], n[1]);
        return 0;
    }
    if (argc == 5 && std::string(argv[1]) == "slice") { // game and generated block types at height z, zone x y
        int32_t zx = std::atoi(argv[2]), zy = std::atoi(argv[3]), z = std::atoi(argv[4]);
        std::vector<GameColumn> game = readGameZone(zx, zy);
        if (game.empty())
            return std::printf("zone not extracted\n"), 1;
        worldgen::setWorldSeed(123);
        std::vector<worldgen::TerrainColumn> ours = worldgen::litZone(zx, zy);
        const char *sym = "0123456789abcdefghijklmnopqrstuvwxyz";
        std::printf("y runs down, x across; game | CubeVulkan; types (0 air ' '), protected (flag 2) in upper case\n");
        for (int ly = 0; ly < 64; ly++) {
            std::string a, b;
            for (int lx = 0; lx < 64; lx++) {
                worldgen::Block g = game[(size_t)lx * 64 + ly].at(z);
                const worldgen::TerrainColumn &t = ours[(size_t)lx * 64 + ly];
                worldgen::Block m = z >= t.base && z - t.base < (int)t.blocks.size() ? t.blocks[(size_t)(z - t.base)] : worldgen::Block{};
                auto ch = [&](const worldgen::Block &k) {
                    char c = k.type == 0 ? ' ' : sym[k.type % 36];
                    return (k.flags & 2) ? (char)(k.type < 26 ? 'A' + k.type : '#') : c;
                };
                a += ch(g), b += ch(m);
            }
            std::printf("%s | %s\n", a.c_str(), b.c_str());
        }
        return 0;
    }
    if (argc == 5 && std::string(argv[1]) == "makesave" && std::string(argv[4]) == "loot") // with what the loot windows take
        return makeTestSave(argv[2], argv[3]) ? 1 : addTestLoot(argv[2]);
    if (argc == 4 && std::string(argv[1]) == "makesave") // a test characters.db (tests/FrontendTests.cpp)
        return makeTestSave(argv[2], argv[3]);
    if (argc == 7 && std::string(argv[1]) == "makesave" && std::string(argv[4]) == "near") { // saved standing near the home
        const glm::vec2 offset((float)std::atof(argv[5]), (float)std::atof(argv[6]));
        return makeTestSave(argv[2], argv[3], &offset);
    }
    if (argc == 2 && std::string(argv[1]) == "castles") { // big structures that extracted zones cover
        Assets a;
        if (!a.locate(""))
            return 1;
        std::map<std::pair<int, int>, int> seen;
        for (auto &e : std::filesystem::directory_iterator(zonesDir(a.gameDir()))) {
            int ax, ay;
            if (!ExtractedWorld::zoneCoordinates(e.path(), ax, ay) || std::abs(ax) > 100000 || std::abs(ay) > 100000)
                continue;
            int32_t c1x = worldgen::floorDiv(ax * 64, 10), c1y = worldgen::floorDiv(ay * 64, 10);
            bool big = worldgen::bigCell(c1x, c1y);
            if (!big)
                continue;
            c1x = worldgen::floorDiv(c1x, 1) * 2, c1y = worldgen::floorDiv(c1y, 1) * 2;
            seen[{c1x, c1y}]++;
        }
        for (auto [c, n] : seen) {
            int kind = worldgen::placeKind(c.first, c.second, true);
            int cells = 0;
            for (int j = 0; j < 128; j++)
                for (int i = 0; i < 128; i++)
                    cells += worldgen::castleCell(c.first * 64 + i, c.second * 64 + j).kind != 0;
            std::printf("big cell %d %d: kind %d, %d extracted zones, %d castle cells\n", c.first, c.second, kind, n, cells);
        }
        return 0;
    }
    if (argc == 4 && std::string(argv[1]) == "gamecol") { // an extracted column, block by block
        int32_t x = std::atoi(argv[2]), y = std::atoi(argv[3]);
        std::vector<GameColumn> game = readGameZone(worldgen::floorDiv(x, 6), worldgen::floorDiv(y, 6));
        if (game.empty())
            return std::printf("zone not extracted\n"), 1;
        const GameColumn &c = game[(size_t)(x & 63) * 64 + (size_t)(y & 63)];
        for (size_t i = 0; i < c.blocks.size(); i++) {
            const worldgen::Block &b = c.blocks[i];
            std::printf("  z %d: %d %d %d %d %d %d\n", c.baseZ + (int)i, b.r, b.g, b.b, b.glow, b.type, b.flags);
        }
        return 0;
    }
    if (argc == 4 && std::string(argv[1]) == "encounters") { // places within r 1024-cells: kind, role, encounter, its zone (day d)
        const int r = std::atoi(argv[2]);
        worldgen::setWorldSeed(123);
        worldgen::setWorldDay(std::atoi(argv[3]));
        for (int32_t x = -r; x <= r; x++)
            for (int32_t y = -r; y <= r; y++) {
                const worldgen::PlaceEncounter e = worldgen::placeEncounter(x, y);
                if (e.kind)
                    std::printf("place %d %d kind %d big %d role %d encounter %d zone %d %d centre %d %d\n", x, y, e.kind, e.big ? 1 : 0,
                                e.role, e.encounter, e.zx, e.zy, e.point.x, e.point.y);
            }
        return 0;
    }
    if (argc == 4 && std::string(argv[1]) == "rtrace") { // a generated zone's rand() calls (z_x_y.rtrace) against ours
        Assets a;
        if (!a.locate(""))
            return 1;
        const int32_t zx = std::atoi(argv[2]), zy = std::atoi(argv[3]);
        std::ifstream in(zonesDir(a.gameDir()) / ("z_" + std::to_string(zx) + "_" + std::to_string(zy) + ".rtrace"), std::ios::binary);
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), {});
        std::vector<std::pair<uint32_t, uint16_t>> game(data.size() / 6);
        for (size_t i = 0; i < game.size(); i++)
            std::memcpy(&game[i].first, &data[i * 6], 4), std::memcpy(&game[i].second, &data[i * 6 + 4], 2);
        worldgen::setWorldSeed(123);
        if (const char *d = std::getenv("CW_DAY"))
            worldgen::setWorldDay(std::atoi(d));
        worldgen::loadCreatureTables(spawnsDir(a.gameDir()));
        const std::vector<uint16_t> ours = worldgen::zoneDraws(zx, zy);
        size_t i = 0;
        while (i < game.size() && i < ours.size() && game[i].second == ours[i])
            i++;
        std::printf("draws: game %zu, ours %zu, the same up to %zu\n", game.size(), ours.size(), i);
        for (size_t k = i > 8 ? i - 8 : 0; k < i + 12 && k < std::max(game.size(), ours.size()); k++)
            std::printf("%6zu  game %s %5d | ours %5d%s\n", k, k < game.size() ? ("0x" + [&] {
                char b[16];
                std::snprintf(b, sizeof b, "%x", game[k].first);
                return std::string(b);
            }()).c_str() : "-", k < game.size() ? game[k].second : -1, k < ours.size() ? ours[k] : -1, k == i ? "  <-" : "");
        return 0;
    }
    if (argc == 4 && std::string(argv[1]) == "height") { // a column's ground height in each of 0x35ea0's modes
        worldgen::setWorldSeed(123);
        const int32_t x = std::atoi(argv[2]), y = std::atoi(argv[3]);
        for (int mode : {0, 1, 2, 4, 8, 9})
            std::printf("mode %d: %.4f\n", mode, worldgen::groundHeight(x, y, mode));
        return 0;
    }
    if (argc >= 5 && std::string(argv[1]) == "findcells") { // findcells <zones radius> <per type> <types...>: zones with those cell objects
        const int r = std::atoi(argv[2]), per = std::atoi(argv[3]);
        worldgen::setWorldSeed(123);
        std::map<int, int> want;
        for (int k = 4; k < argc; k++)
            want[std::atoi(argv[k])] = 0;
        for (int32_t cx = -r * 4; cx < r * 4; cx++)
            for (int32_t cy = -r * 4; cy < r * 4; cy++) {
                const int t = worldgen::cellObject(cx, cy);
                auto it = want.find(t);
                if (it == want.end() || it->second >= per)
                    continue;
                it->second++;
                std::printf("object %d cell %d %d zone %d %d\n", t, cx, cy, worldgen::floorDiv(cx, 2), worldgen::floorDiv(cy, 2));
            }
        return 0;
    }
    if (argc == 6 && std::string(argv[1]) == "cellmap") { // the cell objects of cells x0..x1, y0..y1 (rows by y)
        int32_t x0 = std::atoi(argv[2]), y0 = std::atoi(argv[3]), x1 = std::atoi(argv[4]), y1 = std::atoi(argv[5]);
        worldgen::setWorldSeed(123);
        for (int32_t y = y0; y <= y1; y++) {
            std::printf("%5d:", y);
            for (int32_t x = x0; x <= x1; x++)
                std::printf(" %3d", worldgen::cellObject(x, y));
            std::printf("\n");
        }
        return 0;
    }
    if (argc == 4 && std::string(argv[1]) == "zonediff") { // the columns of a zone that differ, with their first differing block
        int32_t zx = std::atoi(argv[2]), zy = std::atoi(argv[3]);
        std::vector<GameColumn> game = readGameZone(zx, zy);
        if (game.empty())
            return std::printf("zone not extracted\n"), 1;
        worldgen::setWorldSeed(123);
        std::vector<worldgen::TerrainColumn> ours = worldgen::litZone(zx, zy);
        for (int col = 0; col < 4096; col++) {
            const GameColumn &c = game[(size_t)col];
            const worldgen::TerrainColumn &t = ours[(size_t)col];
            int lo = std::max(c.baseZ, t.base), hi = std::max(c.baseZ + (int)c.blocks.size(), t.base + (int)t.blocks.size());
            int n = 0, first = 0;
            worldgen::Block fg{}, fm{};
            for (int z = lo; z < hi; z++) {
                worldgen::Block g = c.at(z);
                worldgen::Block m = z >= t.base && z - t.base < (int)t.blocks.size() ? t.blocks[(size_t)(z - t.base)] : worldgen::Block{};
                bool lightOnly = g.g == m.g && g.b == m.b && g.type == m.type && g.flags == m.flags && (g.type == 0 || g.type == 2);
                if ((g.r == m.r && g.g == m.g && g.b == m.b && g.type == m.type && g.flags == m.flags) || lightOnly)
                    continue;
                if (!n++)
                    first = z, fg = g, fm = m;
            }
            if (n) {
                int x = zx * 64 + col / 64, y = zy * 64 + col % 64;
                std::printf("%d %d (cell object %d): %d differ, first z %d: game %d %d %d %d %d, ours %d %d %d %d %d\n", x, y,
                            worldgen::cellObject(worldgen::floorDiv(x, 4), worldgen::floorDiv(y, 4)), n, first, fg.r, fg.g, fg.b, fg.type,
                            fg.flags, fm.r, fm.g, fm.b, fm.type, fm.flags);
            }
        }
        return 0;
    }
    if (argc == 4 && std::string(argv[1]) == "cmpcol") { // an extracted column against the generated one, block by block
        int32_t x = std::atoi(argv[2]), y = std::atoi(argv[3]);
        int32_t zx = worldgen::floorDiv(x, 6), zy = worldgen::floorDiv(y, 6);
        std::vector<GameColumn> game = readGameZone(zx, zy);
        if (game.empty())
            return std::printf("zone not extracted\n"), 1;
        worldgen::setWorldSeed(123);
        std::vector<worldgen::TerrainColumn> ours = worldgen::litZone(zx, zy);
        const GameColumn &c = game[(size_t)(x & 63) * 64 + (size_t)(y & 63)];
        const worldgen::TerrainColumn &t = ours[(size_t)(x & 63) * 64 + (size_t)(y & 63)];
        int lo = std::min(c.baseZ, t.base), hi = std::max(c.baseZ + (int)c.blocks.size(), t.base + (int)t.blocks.size());
        for (int z = lo; z < hi; z++) {
            worldgen::Block g = c.at(z);
            worldgen::Block m = z >= t.base && z - t.base < (int)t.blocks.size() ? t.blocks[(size_t)(z - t.base)] : worldgen::Block{};
            bool same = g.r == m.r && g.g == m.g && g.b == m.b && g.type == m.type && g.flags == m.flags;
            std::printf("  z %d: %3d %3d %3d %3d %2d %2d | %3d %3d %3d %3d %2d %2d%s\n", z, g.r, g.g, g.b, g.glow, g.type, g.flags, m.r, m.g, m.b,
                        m.glow, m.type, m.flags, same ? "" : "  *");
        }
        return 0;
    }
    if (argc == 4 && std::string(argv[1]) == "bases") { // extracted base_z against the heights, zone x y
        Assets a;
        if (!a.locate(""))
            return 1;
        int ax = std::atoi(argv[2]), ay = std::atoi(argv[3]);
        std::filesystem::path dir = zonesDir(a.gameDir());
        std::ifstream in(dir / ("z_" + std::to_string(ax) + "_" + std::to_string(ay) + ".cwz"), std::ios::binary);
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), {});
        size_t at = 20;
        std::map<int, int> stats;
        for (int col = 0; col < 4096; col++) {
            int32_t baseZ;
            uint32_t runs;
            std::memcpy(&baseZ, &data[at], 4), std::memcpy(&runs, &data[at + 4], 4);
            at += 8 + 8 * (size_t)runs;
            int x = ax * 64 + col / 64, y = ay * 64 + col % 64;
            int fz = (int)std::floor(worldgen::groundHeight(x, y));
            int lo = std::min({(int)std::floor(worldgen::groundHeight(x - 1, y)), (int)std::floor(worldgen::groundHeight(x + 1, y)),
                               (int)std::floor(worldgen::groundHeight(x, y - 1)), (int)std::floor(worldgen::groundHeight(x, y + 1))});
            stats[std::min(baseZ - fz, 9) * 100 + std::max(-9, std::min(baseZ - std::min(lo, fz), 9))]++;
        }
        for (auto [k, n] : stats)
            std::printf("base - surface %d, base - lowest (own or neighbour) %d: %d columns\n", (k + 1000) / 100 - 10,
                        k - ((k + 1000) / 100 - 10) * 100, n);
        return 0;
    }
    if (argc == 4 && std::string(argv[1]) == "objects") { // the cell objects around a column
        int x = std::atoi(argv[2]), y = std::atoi(argv[3]);
        int cx = worldgen::floorDiv(x, 4), cy = worldgen::floorDiv(y, 4);
        for (int j = cy - 3; j <= cy + 3; j++)
            for (int i = cx - 3; i <= cx + 3; i++) {
                int type = worldgen::cellObject(i, j);
                const worldgen::CastleCell c = worldgen::castleCell(i, j);
                if (c.kind)
                    std::printf("cell %d %d: castle kind %d detail %d style %d steps %d\n", i, j, c.kind, c.detail, c.style, c.steps);
                if (!type)
                    continue;
                worldgen::Int2 p = worldgen::cellPoint(i, j);
                std::printf("cell %d %d: object %d reach %d point %d %d\n", i, j, type, worldgen::cellReach(type), p.x, p.y);
            }
        return 0;
    }
    if (argc == 4 && std::string(argv[1]) == "col") { // a generated column's field and blocks
        int x = std::atoi(argv[2]), y = std::atoi(argv[3]);
        const worldgen::ColumnField f = worldgen::columnField(x, y);
        std::printf("height4 %g\n", worldgen::groundHeight(x, y, 4));
        std::printf("place kind %d variant %d big %d weight %g dist2 %g; placeAt kind %d\n", f.place.kind, f.place.variant,
                    (int)f.place.big, f.place.weight, f.place.dist2, worldgen::placeAt(x, y).kind);
        const worldgen::ColourInputs &v = f.colours;
        std::printf("height %g base %g warmth %g dryness %g highlands %g veg %g river %g lava %g swamp %g roads %g\n",
                    f.height, f.base, v.warmth, v.dryness, v.highlands, v.vegetation, f.river, v.lava, v.swamp, v.roads);
        const int32_t zx = worldgen::floorDiv(x, 6), zy = worldgen::floorDiv(y, 6);
        const worldgen::TerrainColumn t = worldgen::zone(zx, zy)->terrain.columns[(size_t)worldgen::ZoneTerrain::index(x - zx * 64, y - zy * 64)];
        for (size_t i = 0; i < t.blocks.size(); i++) {
            const worldgen::Block &b = t.blocks[i];
            std::printf("  z %d: %d %d %d %d %d %d\n", t.base + (int)i, b.r, b.g, b.b, b.glow, b.type, b.flags);
        }
        return 0;
    }
    if (argc == 4 && std::string(argv[1]) == "rand") { // a zone's rand() draws, in order
        const std::vector<uint16_t> draws = worldgen::zoneDraws(std::atoi(argv[2]), std::atoi(argv[3]));
        for (size_t i = 0; i < draws.size(); i++)
            std::printf("%zu %d\n", i, draws[i]);
        return 0;
    }
    if (argc >= 3 && std::string(argv[1]) == "looksoracle") // <oracle dir>
        return looksOracle(argv[2]);
    if (argc >= 3 && std::string(argv[1]) == "posestateoracle") // <files...>: the pose state rule (creature-pose.md §12)
        return poseStateOracle(std::vector<std::string>(argv + 2, argv + argc));
    if (argc >= 3 && std::string(argv[1]) == "rulesoracle") // <files...>
        return rulesOracle(std::vector<std::string>(argv + 2, argv + argc));
    if (argc >= 4 && std::string(argv[1]) == "aioracle") // <tick or -1> <files...>
        return aioracle::run(std::vector<std::string>(argv + 3, argv + argc), std::atoi(argv[2]));
    if (argc >= 3 && std::string(argv[1]) == "upkeeporacle") // <files...>
        return stepOracle(std::vector<std::string>(argv + 2, argv + argc), 0x2de4c0);
    if (argc >= 3 && std::string(argv[1]) == "effectoracle") // <files...>
        return effectOracle(std::vector<std::string>(argv + 2, argv + argc));
    if (argc == 3 && std::string(argv[1]) == "controloracle") // <file>
        return controlOracle(argv[2]);
    if (argc == 3 && std::string(argv[1]) == "attackoracle") // <file>
        return attackOracle(argv[2]);
    if (argc == 3 && std::string(argv[1]) == "playerstats") // <file>
        return playerStatsOracle(argv[2]);
    if (argc == 2 && std::string(argv[1]) == "statsoracle")
        return statsOracle();
    if (argc == 3 && std::string(argv[1]) == "interactoracle") // <file>
        return interactOracle(argv[2]);
    if (argc == 3 && std::string(argv[1]) == "itemoracle") // <file>
        return itemOracle(argv[2]);
    if (argc >= 3 && std::string(argv[1]) == "droporacle") // <files...>
        return dropOracle(std::vector<std::string>(argv + 2, argv + argc));
    if (argc >= 3 && std::string(argv[1]) == "hitoracle") // <files...>
        return hitOracle(std::vector<std::string>(argv + 2, argv + argc));
    if (argc >= 3 && std::string(argv[1]) == "abilityoracle") // <files...>
        return stepOracle(std::vector<std::string>(argv + 2, argv + argc), 0x2c5250);
    if (argc >= 3 && std::string(argv[1]) == "updateoracle") // <files...>
        return updateOracle(std::vector<std::string>(argv + 2, argv + argc));
    if (argc >= 3 && std::string(argv[1]) == "animoracle") { // <state or -1> <files...>
        std::vector<std::string> files(argv + 3, argv + argc);
        return animOracle(files, std::atoi(argv[2]));
    }
    if (argc >= 2 && std::string(argv[1]) == "zones")
        return zonesPlaceKind = argc > 3 ? std::atoi(argv[3]) : 0, genZonesMatchExtracted(true, argc > 2 ? std::atoi(argv[2]) : 4) ? 1 : 0;
    if (argc >= 2 && std::string(argv[1]) == "wgterrain") // the new generator's terrain against the oracles, with timings
        return worldgenTerrainCommand(argc > 2 ? std::atoi(argv[2]) : 1000);
    if (argc >= 2 && std::string(argv[1]) == "wgstructures") // the new generator's structures against the oracles, with timings
        return worldgenStructureCommand(argc > 2 ? std::atoi(argv[2]) : 1000);
    if (argc >= 2 && std::string(argv[1]) == "wgcreatures") // the new generator's creatures against the oracles, with timings
        return worldgenCreatureCommand(argc > 2 ? std::atoi(argv[2]) : 1000);
    if (argc == 5 && std::string(argv[1]) == "wglightprobe")
        return worldgenLightProbe(std::atoi(argv[2]), std::atoi(argv[3]), std::atoi(argv[4]));
    if (argc >= 2 && std::string(argv[1]) == "wglight") // its light pass against Extract/seed_123
        return worldgenLightCommand(argc > 2 ? std::atoi(argv[2]) : 50);
    if (argc >= 2 && std::string(argv[1]) == "gen")
        return genMatchesGame(true, argc > 2 ? argv[2] : nullptr) ? 1 : 0;
    if (argc >= 2 && std::string(argv[1]) == "cells")
        return genCellsMatchGame(true) ? 1 : 0;
    if (argc >= 2 && std::string(argv[1]) == "blocks")
        return genBlocksMatchGame(true, argc > 2 ? argv[2] : nullptr) ? 1 : 0;
    if (argc == 4 && std::string(argv[1]) == "props")
        return listProps(std::strtof(argv[2], nullptr), std::strtof(argv[3], nullptr));
    cubFormat();
    facesPointOutward();
    zoneBordersAgree();
    varianceMatchesMod();
    alphaHasTallMountains();
    riversAndLilies();
    regionsAreLarge();
    allRegionsOccur();
    steamAssets();
    waterSurfaceGeometry();
    extractedPlacements();
    extractedWorldReloadAndValidation();
    recordingFormats();
    characterClipBoundaries();
    nativeAttackPlayback();
    creatureRecordings();
    nativeLootSelection();
    reflectedFaces();
    steamFailsClosed();
    damageMatchesNativeHits();
    nativeHitPaths();
    combatEffects();
    blocking();
    poisonTicks();
    genMatchesGame(false);
    genBlocksMatchGame(false);
    genCellsMatchGame(false);
    genZonesMatchExtracted(false, 4);
    failures += worldgenTerrainTests();
    failures += worldgenStructureTests();
    failures += worldgenCreatureTests();
    worldScaleWidensRegions();
    worldMapModel();
    monsterBehaviour();
    deadCreatures();
    nativeConversations();
    talkGeneration();
    gearFollowsSurvey();
    progressionRules();
    statRules();
    craftingRules();
    shopRules();
    groundItemRules();
    saveIntegrity();
    speechGrammar();
    deathEffects();
    effectObjects();
    bodyFootprint();
    walkingSpeed();
    specialSkillsAndInteractions();
    loreAndArtifacts();
    mapMarkers();
    plasmaLibrary();
    useKeyTargets();
    useKeyPrompts();
    itemModelRules();
    soundRules();
    failures += alphaRenderingTests(); // tests/AlphaRenderTests.cpp
    failures += skyTests();            // tests/SkyTests.cpp
    failures += raceKeyTests();        // tests/RaceKeyTests.cpp
    failures += worldMapLookTests();   // tests/WorldMapTests.cpp
    failures += saveFormatTests();     // tests/SaveTests.cpp
    failures += frontendTests();       // tests/FrontendTests.cpp
    failures += windContinueTests();   // tests/WindContinueTests.cpp
    failures += netTests();            // tests/NetTests.cpp
    failures += ownerSetupTests();     // tests/OwnerSetupTests.cpp
    failures += poseHudTests();        // tests/PoseHudTests.cpp
    failures += waterLookTests();
    failures += alphaSceneryTests();
    failures += featTextTests();
    failures += lootWindowTests();     // tests/LootWindowTests.cpp
    failures += enemyRespawnTests();   // tests/EnemyRespawnTests.cpp
    failures += playerHitTests();      // tests/PlayerHitTests.cpp
    failures += dialoguePageTests();   // tests/DialoguePageTests.cpp
    failures += petTests();            // tests/PetTests.cpp
    failures += steamSceneRangeTests();
    failures += steamMeshCostTests();
    failures += gamepadUiTests();      // tests/GamepadUiTests.cpp
    failures += plasmaHitTests();     // tests/PlasmaHitTests.cpp
    failures += objectVisualTests();
    failures += sceneCaptureTests();
    failures += regionLevelsTests();
    failures += sceneVisibilityTests();
    failures += steamLightingTests();
    failures += steamVoxelTests();
    failures += steamTerrainTests();
    failures += steamPointLightsTests();
    failures += steamTransientLightsTests();
    failures += steamCreatureLightsTests();
    failures += dialogueChoicesTests();
    failures += npcSpeechTests();
    failures += objectClockTests();  // tests/ObjectVisualTests.cpp
    std::printf(failures ? "%d failure(s)\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}

