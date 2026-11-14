#include "Assets.h"
#include "Extracted.h"
#include "World.h"
#include "Spawned.h"
#include "UsePrompt.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <thread>

namespace {
template <class T> void field(std::ostream &out, const T &value) {
    out.write(reinterpret_cast<const char *>(&value), sizeof value);
}
template <class T> void put(std::array<uint8_t, 0xe0> &record, size_t offset, const T &value) {
    std::memcpy(record.data() + offset, &value, sizeof value);
}
void fixtureZone(const fs::path &folder, int32_t x, int32_t y, int32_t objectX, bool originObject = false,
                 const std::optional<objectrewards::Map> &rewards = std::nullopt) {
    const auto stem = "z_" + std::to_string(x) + "_" + std::to_string(y);
    std::ofstream terrain(folder / (stem + ".cwz"), std::ios::binary);
    terrain.write("CWZ1", 4);
    field(terrain, x);
    field(terrain, y);
    field(terrain, int32_t(0));
    field(terrain, int32_t(0));
    for (int column = 0; column < 4096; ++column) {
        field(terrain, int32_t(0));
        field(terrain, uint32_t(0));
    }
    std::ofstream records(folder / (stem + ".rec"), std::ios::binary);
    records.write("CWR1", 4);
    field(records, uint32_t(0x60));
    field(records, uint32_t(0xe0));
    field(records, uint32_t(originObject ? 2 : 1));
    for (int i = 0; i < (originObject ? 2 : 1); ++i) {
        std::array<uint8_t, 0xe0> object{};
        put(object, 0, int32_t(3));
        const std::array<int64_t, 3> point = i ? std::array<int64_t, 3>{} : std::array<int64_t, 3>{int64_t(objectX) * 65536, 65536, 65536};
        put(object, 8, point);
        put(object, 0x24, 1.f);
        object[0x28] = 1;
        object[0xdf] = 0x7e; // A byte outside render placement still belongs to the source record.
        records.write(reinterpret_cast<const char *>(object.data()), object.size());
    }
    field(records, uint32_t(0x30));
    field(records, uint32_t(0xe0));
    field(records, uint32_t(1));
    nativesaves::Record ground{};
    ground[0] = 12;
    ground[0x1c] = 11;
    put(ground, 0xa0, std::array<int64_t, 3>{int64_t(objectX) * 65536, 65536, 65536});
    put(ground, 0xd4, int32_t(-2));
    records.write(reinterpret_cast<const char *>(ground.data()), ground.size());
    if (rewards) {
        field(records, objectrewards::CHUNK);
        field(records, objectrewards::RECORD_BYTES);
        field(records, uint32_t(rewards->size()));
        const auto bytes = objectrewards::encode(*rewards);
        if (!bytes.empty())
            records.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
    }
    if (x == -1) {
        field(records, uint32_t(0xa8));
        field(records, uint32_t(0x40));
        field(records, uint32_t(1));
        std::array<uint8_t, 0x40> emitter{};
        const uint32_t type = 77;
        const std::array<int64_t, 3> position{-1, -65536, 3 * 65536};
        const std::array<float, 3> colour{.15f, .4f, .8f};
        std::memcpy(emitter.data(), &type, sizeof type);
        std::memcpy(emitter.data() + 8, position.data(), sizeof position);
        std::memcpy(emitter.data() + 0x2c, colour.data(), 3 * sizeof(float));
        records.write(reinterpret_cast<const char *>(emitter.data()), emitter.size());
    }
}
std::vector<DrawItem> objects(World &world) {
    std::vector<DrawItem> opaque, water;
    world.collect(opaque, water);
    std::erase_if(opaque, [](const DrawItem &draw) { return !draw.nativeObject; });
    return opaque;
}
} // namespace

int staticObjectStreamingTests() {
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        if (!ok) {
            ++failures;
            std::printf("FAIL static-object streaming: %s\n", message);
        }
    };
    const auto root = fs::temp_directory_path() /
                      ("CubeVulkan-object-streaming-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto folder = root / "Extract" / "fixture";
    fs::create_directories(folder);
    fs::create_directories(root / "sprites");
    std::ofstream(root / "cubeworld.exe") << "asset resolver fixture; never executable";
    std::ofstream(root / "Extract" / "sprite-groups.txt") << "# building-part\n3 fixture-door.cub\n";
    std::ofstream(folder / "manifest.txt") << "seed=0\nhomeX=1\nhomeY=1\nhomeZ=1\n";
    {
        std::ofstream cube(root / "sprites" / "fixture-door.cub", std::ios::binary);
        field(cube, int32_t(2));
        field(cube, int32_t(1));
        field(cube, int32_t(3));
        for (int i = 0; i < 6; ++i) {
            const uint8_t rgb[]{80, 70, 60};
            cube.write(reinterpret_cast<const char *>(rgb), 3);
        }
    }
    fixtureZone(folder, 0, 0, 1, true);
    auto capturedReward = nativesaves::makeItem(23, 4);
    capturedReward[0x50] = 0xcd;
    fixtureZone(folder, -1, 0, -63, false, objectrewards::Map{});
    fixtureZone(folder, 1, 0, 65, false, objectrewards::Map{{0, capturedReward}});
    const auto oldStyle = renderStyle();
    {
        Renderer renderer;
        RenderSettings settings;
        settings.style = RenderStyle::Steam;
        renderer.initHeadless(settings);
        Assets assets;
        ExtractedWorld blocks;
        std::string error;
        const bool ready = assets.locate(utf8(root)) && blocks.open(folder, error);
        check(ready, "synthetic assets and captured zones open");
        if (ready) {
            check(!blocks.ambientEmitters(-1, 0) && !blocks.gameZoneLoaded(-1, 0),
                  "ambient eligibility does not generate or load its source");
            blocks.propsAbsolute(-1, 0); // Load data only; no World terrain mesh exists.
            const auto emitters = blocks.ambientEmitters(-1, 0);
            check(emitters && emitters->size() == 1 && emitters->front().type == 77 &&
                      emitters->front().position[0] == -1 && emitters->front().colour[2] == .8f,
                  "fresh capture exposes the exact loaded emitter record");
            auto source = blocks.gameZoneToken(-1, 0);
            check(source.has_value(), "the admitted source has a stable identity");
            std::array<scenevisibility::ZoneDistance, 1> sceneDistance{{{{-1, 0},
                source ? source->identity : nullptr, 4096}}};
            blocks.publishAmbientSceneDistances(sceneDistance);
            check(!blocks.ambientEmitters(-1, 0), "stored scene sample at equality gates the subsequent visit");
            sceneDistance.front().squaredDistance = 4095;
            blocks.publishAmbientSceneDistances(sceneDistance);
            check(blocks.ambientEmitters(-1, 0) == emitters, "an admitted scene update reuses the loaded zone's list");
            sceneDistance.front().identity = std::make_shared<int>(1);
            sceneDistance.front().squaredDistance = 6000;
            blocks.publishAmbientSceneDistances(sceneDistance);
            check(blocks.ambientEmitters(-1, 0) == emitters, "a stale scene identity cannot update another loaded zone");
            check(!blocks.loadedObjectProps(1, 0) && !blocks.gameZoneLoaded(1, 0), "object lookup does not load a present disk zone");
            Terrain terrain;
            World world(terrain, assets, renderer, 0, &blocks);
            scenerange::Player player;
            player.physical = {65536, 65536, 65536};
            world.setScenePlayerProvider([&]() -> std::optional<scenerange::Player> { return player; });
            const glm::vec3 remote{64000, 64000, 129};
            world.update(remote);
            auto draws = objects(world);
            int liveQueries = 0;
            std::vector<DrawItem> initialOpaque, initialWater;
            world.collect(initialOpaque, initialWater);
            const bool terrainDraw =
                std::any_of(initialOpaque.begin(), initialOpaque.end(), [](const auto &draw) { return draw.kind == DrawKind::Terrain; });
            check(!terrainDraw && initialWater.empty() && draws.size() == 3,
                  "loaded objects render before any terrain mesh publication, including a valid all-zero position");
            if (draws.size() == 3) {
                check((*draws[0].nativePosition)[0] == -63 * 65536 && (*draws[1].nativePosition)[0] == 65536 &&
                          *draws[2].nativePosition == steampoints::Point{},
                      "absolute zone XY order and source record order are retained");
                const auto closed = draws[1].model;
                world.setObjectProvider([&](int, int, const objectvisual::Key &) -> std::optional<objectvisual::State> {
                    ++liveQueries;
                    return objectvisual::State{3, 0, 0, 1, 0};
                });
                draws = objects(world);
                check(liveQueries == 3 && std::memcmp(&closed, &draws[1].model, sizeof closed) != 0,
                      "independent records retain live door clocks without duplicate provider calls");
            }
            player.physical[0] = 4 * 64 * 65536;
            world.update(remote);
            check(objects(world).empty() && !blocks.gameZoneLoaded(1, 0),
                  "physical player square retires old records without loading new zones");
            player.physical[0] = 65536;
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
            do {
                world.update(remote);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            } while (world.pendingCount() && std::chrono::steady_clock::now() < deadline);
            check(world.pendingCount() == 0, "remote empty mesh job completes before source reset");
            if (!world.pendingCount()) {
                const auto before = blocks.gameZoneToken(0, 0);
                fixtureZone(folder, 0, 0, 8);
                check(blocks.open(folder, error), "source reset reloads fixture");
                const auto after = blocks.gameZoneToken(0, 0);
                world.update(remote);
                draws = objects(world);
                check(before && after && before->order != after->order && draws.size() == 1 && (*draws[0].nativePosition)[0] == 8 * 65536,
                      "zone lifetime changes replace old object records and discard unloaded neighbors");
                const auto meshDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
                do {
                    world.update({1, 1, 129});
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                } while (world.pendingCount() && std::chrono::steady_clock::now() < meshDeadline);
                liveQueries = 0;
                draws = objects(world);
                check(world.pendingCount() == 0 && draws.size() == 3 && liveQueries == 3,
                      "terrain publication does not duplicate static records or their live callbacks");
                fixtureZone(folder, 0, 0, 9);
                world.refreshModels(0, 0);
                world.update({1, 1, 129});
                draws = objects(world);
                const auto current = blocks.gameZoneToken(0, 0);
                check(
                    current && after && current->order == after->order &&
                        std::any_of(draws.begin(), draws.end(), [](const auto &draw) { return (*draw.nativePosition)[0] == 9 * 65536; }) &&
                        std::none_of(draws.begin(), draws.end(), [](const auto &draw) { return (*draw.nativePosition)[0] == 8 * 65536; }),
                    "explicit model invalidation refreshes object records within the same zone lifetime");
                SpawnedCreatures live;
                nativesaves::WorldState saved;
                live.setStoredZones(blocks, &saved, false);
                const auto captured = blocks.capturedLists(0, 0);
                const auto *lying = live.groundItems(blocks, 0, 0);
                const auto *statics = live.zoneObjects(0, 0);
                check(captured && lying && statics && *lying == captured->groundItems && *statics == captured->staticObjects &&
                          statics->size() == 1 && (*statics)[0][0xdf] == 0x7e,
                      "captured ground and object records seed gameplay byte-for-byte instead of regenerating them");
                if (captured) {
                    world.setObjectListProvider([&](int x, int y) { return live.zoneObjects(x, y); });
                    world.setObjectProvider({});
                    auto replacement = captured->staticObjects.front();
                    put(replacement, 8, std::array<int64_t, 3>{11 * 65536, 65536, 65536});
                    put(replacement, 0x24, 2.f);
                    replacement[0x20] = 191;
                    const int64_t linked = 0x123456789abcde;
                    put(replacement, 0x30, linked);
                    auto required = nativesaves::makeItem(22, 7);
                    required[0x50] = 0xab;
                    std::memcpy(replacement.data() + 0x38, required.data(), required.size());
                    replacement[0xd8] = 255;
                    live.replaceStaticObjects(blocks, {{0, 0}, {replacement}});
                    const auto prompt = useprompt::objectRecord(live.zoneObjects(0, 0)->front(), 0, 0, 0);
                    check(prompt.fixedPosition == useprompt::FixedPoint{11 * 65536, 65536, 65536} && prompt.size == glm::vec3(4, 2, 6) &&
                              prompt.quarterTurn && prompt.occupant == linked && prompt.item == required && prompt.itemCount == 255 &&
                              prompt.active,
                          "interaction fields follow current replacement placement, full requirement, unsigned count and link");
                    check(!live.objectContents(blocks, 0, 0, 0).known && !live.objectContents(blocks, 99, 99, 0).known,
                          "raw-only captured or unloaded zones cannot turn an unknown reward map into generated contents");
                    const auto contents = live.objectContents(blocks, 1, 0, 0);
                    const auto absent = live.objectContents(blocks, 1, 0, 99);
                    const auto empty = live.objectContents(blocks, -1, 0, 0);
                    check(contents.known && contents.item == capturedReward && absent.known && absent.item[0] == 0 && empty.known &&
                              empty.item[0] == 0,
                          "captured full reward items, missing keys and explicit empty maps remain distinct from unknown metadata");
                    world.update({1, 1, 129});
                    draws = objects(world);
                    check(std::any_of(
                              draws.begin(), draws.end(),
                              [](const auto &draw) { return (*draw.nativePosition)[0] == 11 * 65536 && draw.nativeObject->radius == 7; }) &&
                              std::none_of(draws.begin(), draws.end(),
                                           [](const auto &draw) { return (*draw.nativePosition)[0] == 9 * 65536; }),
                          "live whole-list replacement updates position, scale, turn and sphere without a terrain notice");
                    const auto currentDoor =
                        std::find_if(draws.begin(), draws.end(), [](const auto &draw) { return (*draw.nativePosition)[0] == 11 * 65536; });
                    if (currentDoor != draws.end()) {
                        const auto closed = currentDoor->model;
                        auto openedRecord = replacement;
                        openedRecord[0x28] = 0;
                        put(openedRecord, 0x2c, 0.f);
                        live.replaceStaticObjects(blocks, {{0, 0}, {openedRecord}});
                        check(!useprompt::objectRecord(live.zoneObjects(0, 0)->front(), 0, 0, 0).active,
                              "interaction activity follows the current received state independently of generation");
                        draws = objects(world);
                        const auto opened = std::find_if(draws.begin(), draws.end(),
                                                         [](const auto &draw) { return (*draw.nativePosition)[0] == 11 * 65536; });
                        check(opened != draws.end() && std::memcmp(&closed, &opened->model, sizeof closed) != 0,
                              "live interaction state animates through the list provider without rebuilding placement");
                    }
                    live.replaceStaticObjects(blocks, {{0, 0}, {}});
                    world.update({1, 1, 129});
                    check(objects(world).size() == 2,
                          "an authoritative empty list removes objects instead of falling back to captured ghosts");
                    saved.zones[{0, 0}].flags = nativesaves::ZoneState::STATIC_OBJECTS_SAVED;
                    saved.zones[{0, 0}].staticObjects = {replacement};
                    SpawnedCreatures restored;
                    restored.setStoredZones(blocks, &saved, false);
                    restored.groundItems(blocks, 0, 0);
                    check(restored.zoneObjects(0, 0) && *restored.zoneObjects(0, 0) == std::vector<nativesaves::Record>{replacement},
                          "stored object state overlays the captured base through the existing native save contract");
                    world.setObjectListProvider({});
                    world.setObjectProvider({});
                }
            }
        }
    }
    setRenderStyle(oldStyle);
    fs::remove_all(root);
    std::printf("Native static-object streaming: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
