#include "NativeWorldObjects.h"
#include "NativeInteract.h"
#include "NativePose.h"
#include "Spawned.h"
#include "ZoneStates.h"
#include "Extracted.h"
#include "GameMath.h"
#include "ObjectOccupancyCapture.h"
#include "worldgen/Generator.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <tuple>

namespace {
constexpr int64_t BLOCK = 65536;
template <class T> void put(uint8_t *record, size_t offset, T value) {
    std::memcpy(record + offset, &value, sizeof value);
}
template <class T> T get(const uint8_t *record, size_t offset) {
    T value;
    std::memcpy(&value, record + offset, sizeof value);
    return value;
}
struct Actor {
    std::array<uint8_t, nativepose::CREATURE_SIZE> record{};
    nativeai::Creature side;
    Actor(int64_t id, uint8_t kind, int64_t x = 0, int64_t y = 0, int64_t z = 0) {
        side.record = record.data();
        put(record.data(), worldgen::creature::ID, id);
        record[0x60] = kind;
        put(record.data(), 0x10, x);
        put(record.data(), 0x18, y);
        put(record.data(), 0x20, z);
        put(record.data(), 0x180, 100.0f);
    }
};
nativeai::Object object(int type, uint8_t state, float timer) {
    nativeai::Object result{};
    put(result.data(), 0, int32_t(type));
    result[nativeinteract::OBJECT_STATE] = state;
    put(result.data(), nativeinteract::OBJECT_SWING, timer);
    return result;
}
struct Scene {
    nativeobjects::World world;
    std::vector<nativeai::Creature *> actors;
    std::vector<nativeupkeep::Hit> hits;
    std::vector<soundrules::Request> sounds;
    std::vector<std::string> order;
    std::vector<int> draws;
    size_t randomCalls = 0;
    int changes = 0;
    Scene() {
        world.creatures = &actors;
        world.maxHealth = [](const nativeai::Creature &) { return 100.0f; };
        world.hit = [&](nativeai::Creature &, const nativeupkeep::Hit &hit) {
            hits.push_back(hit);
            order.push_back("hit");
        };
        world.rand = [&] {
            order.push_back("rand");
            const size_t index = randomCalls++;
            return index < draws.size() ? draws[index] : 0;
        };
        world.sound = [&](const soundrules::RequestRecord &record) {
            sounds.push_back(soundrules::readRequest(record));
            order.push_back("sound" + std::to_string(sounds.back().number));
        };
        world.objectsChanged = [&] {
            ++changes;
            order.push_back("changed");
        };
    }
};

void writeFixture(const std::filesystem::path &root, uint8_t type, int32_t columnBase = 0, bool empty = false,
                  uint8_t flags = 0, int32_t zoneX = 0) {
    std::filesystem::create_directories(root);
    std::ofstream(root / "manifest.txt") << "seed=123\nhomeX=0\nhomeY=0\nhomeZ=1\n";
    std::ofstream out(root / ("z_" + std::to_string(zoneX) + "_0.cwz"), std::ios::binary);
    out.write("CWZ1", 4);
    const int32_t header[] = {zoneX, 0, 0, 0};
    out.write(reinterpret_cast<const char *>(header), sizeof header);
    for (int i = 0; i < 4096; ++i) {
        const int32_t base = columnBase;
        const uint32_t runs = empty ? 0 : 1;
        const uint16_t count = 1;
        const uint8_t block[] = {50, 60, 70, 0, type, flags};
        out.write(reinterpret_cast<const char *>(&base), sizeof base);
        out.write(reinterpret_cast<const char *>(&runs), sizeof runs);
        if (!empty) {
            out.write(reinterpret_cast<const char *>(&count), sizeof count);
            out.write(reinterpret_cast<const char *>(block), sizeof block);
        }
    }
}

void capturedGateFootprint(const std::filesystem::path &root, const auto &check) {
    writeFixture(root, 1, 1, false, 0xc0);
    writeFixture(root, 1, 1, false, 0xc0, -1);
    const auto clearFootprint = [&](uint8_t expected) {
        ExtractedWorld terrain;
        std::string why;
        if (!terrain.open(root, why)) {
            check(false, "captured gate fixture opens");
            return;
        }
        auto gate = object(8, 0, 1000);
        put(gate.data(), nativeinteract::OBJECT_POSITION + 16, BLOCK);
        nativesaves::WorldState retained;
        auto &saved = retained.zones[{0, 0}];
        saved.flags = nativesaves::ZoneState::STATIC_OBJECTS_SAVED;
        saved.staticObjects.push_back(gate);
        SpawnedCreatures actors;
        actors.setStoredZones(terrain, &retained, false);
        cwnet::ZoneAnswer restored;
        check(actors.zoneAnswer(terrain, {0, 0}, restored) && restored.staticObjects &&
                  restored.staticObjects->size() == 1 && !restored.staticObjects->front()[nativeinteract::OBJECT_STATE] &&
                  terrain.gameBlock(0, 0, 1)[5] == 0xc0,
              "a saved open gate restores its state while retaining the generated footprint");
        cwnet::ZoneObjects received{{0, 0}, {gate}};
        float neighborTop = 0;
        check(terrain.surface(-1, 0, neighborTop) && terrain.gameBlock(-1, 0, 1)[5] == 0xc0,
              "neighbor footprint fixture is loaded and marked");
        actors.replaceStaticObjects(terrain, received);
        check(terrain.gameBlock(0, 0, 1)[5] == expected && terrain.gameBlock(0, 1, 1)[5] == 0xc0,
              "the rebuild clears only the tracked object footprint bit");
        check(terrain.gameBlock(-1, 0, 1)[5] == 0xc0, "a tracked key outside its owning zone leaves the neighbor untouched");
    };
    clearFootprint(0xc0); // A legacy capture does not identify which stored flags came from objects.
    {
        const objectoccupancy::Positions marks{{-1, 0, 1}, {0, 0, 1}};
        const auto payload = objectoccupancy::encode(marks);
        const uint32_t chunk[] = {objectoccupancy::Chunk, objectoccupancy::RecordBytes, uint32_t(marks.size())};
        std::ofstream out(root / "z_0_0.rec", std::ios::binary);
        out.write("CWR1", 4);
        out.write(reinterpret_cast<const char *>(chunk), sizeof chunk);
        out.write(reinterpret_cast<const char *>(payload.data()), payload.size());
    }
    clearFootprint(0x80);
}

void worldStepAdmissionOrder(const std::filesystem::path &root, const auto &check) {
    writeFixture(root, 1);
    ExtractedWorld terrain;
    std::string why;
    check(terrain.open(root, why), "World-step fixture opens");
    nativesaves::WorldState archive;
    auto &saved = archive.zones[{0, 0}];
    saved.flags = nativesaves::ZoneState::STATIC_OBJECTS_SAVED;
    saved.staticObjects.push_back(object(31, 0, 100));
    SpawnedCreatures actors;
    actors.setStoredZones(terrain, &archive, false);
    bool fresh = false;
    auto *centre = actors.networkCreature(2, fresh);
    check(centre && fresh, "World-step fixture registers a centre");
    if (!centre) return;
    centre[0x60] = 0;
    put(centre, 0x180, 0.0f);
    const std::array<int64_t, 3> position{};
    std::memcpy(centre + 0x10, position.data(), sizeof position);
    std::vector<float> clocks;
    std::vector<float> times;
    const auto admit = [&] {
        const auto *objects = actors.zoneObjects(0, 0);
        check(objects && objects->size() == 1, "source objects exist before admission callback");
        clocks.push_back(objects && !objects->empty() ? get<float>(objects->front().data(), nativeinteract::OBJECT_SWING) : -1);
        times.push_back(actors.elapsedWorldTime());
        if (clocks.size() == 1) {
            bool added = false;
            auto *born = actors.networkCreature(3, added); // Forces the first network vector to grow.
            check(born && added, "admission callback appends a resident");
            if (born) {
                born[0x60] = 0;
                put(born, 0x180, 0.0f);
                put(born, 0x128, uint16_t(0x10));
                std::memcpy(born + 0x10, position.data(), sizeof position);
            }
        } else if (clocks.size() == 2) {
            actors.remove(2); // Moves the later record while old simulation views still exist.
            if (auto *born = actors.record(3))
                put(born, 0x128, uint16_t(0x10));
        }
    };
    actors.simulate(terrain, 32, {}, admit);
    check(clocks == std::vector<float>{116, 132} && times == std::vector<float>{16, 32},
          "each 16-ms source prelude completes before its zero-mode admission callback");
    check(!actors.record(2) && actors.record(3) && !(get<uint16_t>(actors.record(3), 0x128) & 0x10),
          "body visits refresh after admission growth and removal, including the complete flag-clear pass");
    actors.simulate(terrain, 0, {}, admit);
    check(clocks == std::vector<float>{116, 132, 132} && times.back() == 32,
          "an admitted zero remainder still runs source work and admission without advancing its clock");
    actors.setRemoteClient(true);
    actors.simulate(terrain, 0, {}, admit);
    check(clocks.size() == 3, "nonzero viewed mode skips the resident-admission callback");
}

void selectedMovementIntegration(const std::filesystem::path &root, const auto &check) {
    writeFixture(root, 1);
    for (bool alreadySelected : {false, true}) {
        ExtractedWorld terrain;
        std::string why;
        check(terrain.open(root, why) && terrain.loadSourceZone(0, 0), "selected-movement terrain opens");
        SpawnedCreatures actors;
        const auto make = [&](int64_t id, uint8_t kind) {
            bool fresh = false;
            auto *record = actors.networkCreature(id, fresh);
            check(record && fresh, "selected-movement actor registers");
            if (record) {
                record[0x60] = kind;
                put(record, 0x180, 100.0f);
                for (int axis = 0; axis < 3; ++axis)
                    put(record, 0x84 + 4 * axis, 1.0f);
            }
            return record;
        };
        auto *mover = make(10, 0);
        if (!mover) continue;
        const std::array<int64_t, 3> at{BLOCK, BLOCK, 4 * BLOCK};
        std::memcpy(mover + 0x10, at.data(), sizeof at);
        const auto placeLadder = [&](uint8_t *record) {
            put(record, 0x64, int32_t(0x12e));
            put(record, 0x17c, 8.0f);
            const std::array<int64_t, 3> position{BLOCK + BLOCK / 4, BLOCK, 0};
            std::memcpy(record + 0x10, position.data(), sizeof position);
        };
        if (alreadySelected) {
            auto *record = make(11, 8);
            if (!record) continue;
            put(record, 0x80, uint32_t(0x800000));
            put(record, 0x10, int64_t(30 * BLOCK)); // Outside the later ladder contact.
        }
        actors.simulate(terrain, 0, {}, [&] {
            auto *record = alreadySelected ? actors.record(11) : make(11, 8);
            if (record) {
                placeLadder(record);
                put(record, 0x80, uint32_t(alreadySelected ? 0 : 0x800000));
            }
        });
        const auto contact = [&] { return get<uint32_t>(actors.record(10), 0x5c) & 0x1104; };
        check(bool(contact()) == alreadySelected,
              "movement uses pre-admission selection with live pointed fields, excluding new registrations");
        if (!alreadySelected) {
            actors.simulate(terrain, 0, {}, [&] { put(actors.record(11), 0x80, uint32_t(0)); });
            check(contact() == 0x1104, "the next prelude selects a late registration before its eligibility changes");
        }
    }
}

void sourceSummaryValues(const auto &check) {
    worldgen::ZoneSpawns catalog;
    catalog.points[11].id = 999; // Membership uses the catalog key, not the template's own ID.
    catalog.points[11].area = {{-1, 2}, {3, -4}};
    catalog.points[22].area = {{-1, 2}};
    catalog.points[33].area = {{9, 9}};
    catalog.tracked = {41, 41, 22, 1234};
    nativesaves::ZoneLists live;
    live.groundItems.resize(1);
    live.groundItems[0][0] = 0x55;
    live.staticObjects.resize(1);
    live.staticObjects[0][0] = 0x66;
    live.groundItemsChanged = true;
    live.blockChanges.resize(1);
    live.blockChanges[0].bytes[0] = 0x77;
    live.placeCounters[{88, 99}] = 123;
    live.deadTracked = 456;
    zonestates::rebuildSourceSummaries(live, catalog, {11, 22, 41, 1000});
    const std::map<nativesaves::Cell, int32_t> expected{{{-1, 2}, 2}, {{3, -4}, 1}};
    check(live.placeCounters == expected && live.deadTracked == 3,
          "catalog-key visits and tracked-list identities rebuild independent source summaries");
    check(live.groundItems[0][0] == 0x55 && live.staticObjects[0][0] == 0x66 &&
              live.groundItemsChanged && !live.staticObjectsChanged && live.blockChanges[0].bytes[0] == 0x77,
          "source-summary rebuild preserves arrays, override flags and edit history");
    zonestates::rebuildSourceSummaries(live, catalog, {999, 33});
    check(live.placeCounters == std::map<nativesaves::Cell, int32_t>{{{9, 9}, 1}} && live.deadTracked == 0,
          "template identity alone does not admit its outer catalog key, and old scalar is cleared");
    zonestates::rebuildSourceSummaries(live, catalog, {41});
    check(live.placeCounters.empty() && live.deadTracked == 2,
          "repeated tracked-list entries count independently of an empty admitted key map");
    zonestates::rebuildSourceSummaries(live, {}, {11, 41});
    check(live.placeCounters.empty() && live.deadTracked == 0,
          "empty selected source inputs replace both old summaries with empty or zero");
}

void generatedGateIntegration(const auto &check) {
    struct SeedScope {
        int32_t saved = worldgen::currentWorld().seed;
        SeedScope() { worldgen::setWorldSeed(123); }
        ~SeedScope() { worldgen::setWorldSeed(saved); }
    } seedScope;
    // The entrance and castle passage are both covered by the Steam threshold capture.
    struct GateCase {
        std::array<int32_t, 2> cell;
        std::array<int32_t, 2> zone;
        std::array<int32_t, 3> approach;
        int32_t threshold;
        int32_t type;
        bool countRoute;
    };
    constexpr GateCase gates[] = {
        {{-569, -718}, {-143, -180}, {-9096, -11464, 0}, 1, 8, true},
        {{-714, -1065}, {-179, -267}, {-11414, -17032, 0}, 4, 10, true},
        {{247, 180}, {61, 45}, {3969, 2888, 50}, 5, 8, false},
    };
    for (const auto &gate : gates) {
        ExtractedWorld terrain;
        terrain.openGenerated(gate.approach[0], gate.approach[1], {});
        terrain.propsAbsolute(gate.zone[0], gate.zone[1]);
        terrain.propsAbsolute(gamemath::floorFixedZone(gate.approach[0] * BLOCK),
                              gamemath::floorFixedZone(gate.approach[1] * BLOCK));
        const auto source = worldgen::zone(gate.zone[0], gate.zone[1]);
        const auto door = std::find_if(source->objects.begin(), source->objects.end(), [&](const worldgen::StaticObject &o) {
            return o.type == gate.type && gamemath::floorFixedCell(o.x) == gate.cell[0] && gamemath::floorFixedCell(o.y) == gate.cell[1];
        });
        const auto threshold = source->gateThresholds.find(gate.cell);
        check(door != source->objects.end() && threshold != source->gateThresholds.end() && threshold->second == gate.threshold,
              "generated gate and owner threshold match the recorded test case");
        if (door == source->objects.end() || threshold == source->gateThresholds.end())
            continue;
        if (gate.countRoute && gate.threshold == 1) {
            std::set<int64_t> identifiers;
            for (const auto &[id, point] : source->spawns.points)
                if (std::any_of(point.area.begin(), point.area.end(), [&](const auto &cell) {
                        return cell.x == gate.cell[0] && cell.y == gate.cell[1];
                    }))
                    identifiers.insert(id);
            check(!identifiers.empty(), "catalog fixture supplies a source key for the gate");
            if (!identifiers.empty()) {
                auto markedTerrain = terrain.forkSource();
                markedTerrain->loadSourceZone(gate.zone[0], gate.zone[1]);
                markedTerrain->loadSourceZone(gamemath::floorFixedZone(gate.approach[0] * BLOCK),
                                             gamemath::floorFixedZone(gate.approach[1] * BLOCK));
                nativesaves::WorldState archive;
                SpawnedCreatures marked;
                marked.setStoredZones(*markedTerrain, &archive, false);
                marked.setNativePlayer(true);
                const int64_t at[] = {int64_t(gate.approach[0]) * BLOCK, int64_t(gate.approach[1]) * BLOCK, door->z};
                marked.setPlayer(*markedTerrain, markedTerrain->toLocal(at), 0, {}, 100, 1, true);
                marked.setWorldDead(identifiers);
                check(std::none_of(identifiers.begin(), identifiers.end(), [&](int64_t id) { return marked.hasCreature(id); }),
                      "recorded source identifiers have no admitted live actors");
                marked.simulate(*markedTerrain, 0);
                marked.zoneChangeLists();
                cwnet::ZoneAnswer reply;
                const bool answered = marked.zoneAnswer(*markedTerrain, {gate.zone[0], gate.zone[1]}, reply);
                check(answered && reply.staticObjects && std::any_of(reply.staticObjects->begin(), reply.staticObjects->end(), [&](const auto &o) {
                          return get<int32_t>(o.data(), nativeinteract::OBJECT_TYPE) == door->type &&
                                 get<int64_t>(o.data(), nativeinteract::OBJECT_POSITION) == door->x &&
                                 get<int64_t>(o.data(), nativeinteract::OBJECT_POSITION + 8) == door->y &&
                                 !o[nativeinteract::OBJECT_STATE];
                      }), "recorded catalog identifiers open the gate without admitted live residents");
            }
        }
        const int32_t dx = gamemath::floorFixedBlock(door->x), dy = gamemath::floorFixedBlock(door->y);
        const int32_t dz = gamemath::floorFixedBlock(door->z);
        check(terrain.gameBlock(dx, dy, dz)[5] & 0x40, "closed generated gate marks its footprint");

        const auto [px, py, pz] = gate.approach;
        const int64_t position[] = {px * BLOCK, py * BLOCK, gate.countRoute ? door->z : pz * BLOCK};
        const auto approachBlock = terrain.gameBlock(px, py, pz);
        if (!gate.countRoute) {
            check(approachBlock[5] & 0x20, "the higher-step side has dungeon air");
            auto outsideDungeon = approachBlock;
            outsideDungeon[5] &= uint8_t(~0x20);
            terrain.setGameBlock(px, py, pz, outsideDungeon);
        }

        nativesaves::WorldState retained;
        SpawnedCreatures actors;
        actors.setStoredZones(terrain, &retained, false);
        actors.setNativePlayer(true);
        actors.setPlayer(terrain, terrain.toLocal(position), 0, {}, 100, 1, true);
        std::memcpy(actors.localPlayer()->record + worldgen::creature::POSITION, position, sizeof position);
        actors.simulate(terrain, 0);
        actors.zoneChangeLists();
        check(actors.events().staticObjects.empty() && (terrain.gameBlock(dx, dy, dz)[5] & 0x40),
              "the gate remains closed before its count or proximity condition is met");

        if (gate.countRoute) {
            // Four zones away is outside the live recount window but inside the gate's sum.
            retained.zones[{gate.zone[0] + 4, gate.zone[1]}].placeCounters[gate.cell] = gate.threshold;
        } else {
            terrain.setGameBlock(px, py, pz, approachBlock);
            std::memcpy(actors.localPlayer()->record + worldgen::creature::POSITION, position, sizeof position);
        }
        actors.simulate(terrain, 0);
        actors.zoneChangeLists();
        const auto &changes = actors.events().staticObjects;
        const auto changed = std::find_if(changes.begin(), changes.end(), [&](const cwnet::ZoneObjects &zone) {
            return zone.key.x == gate.zone[0] && zone.key.y == gate.zone[1];
        });
        const bool opened = changed != changes.end() &&
                            std::any_of(changed->records.begin(), changed->records.end(), [&](const auto &o) {
                                return get<int32_t>(o.data(), nativeinteract::OBJECT_TYPE) == door->type &&
                                       get<int64_t>(o.data(), nativeinteract::OBJECT_POSITION) == door->x &&
                                       get<int64_t>(o.data(), nativeinteract::OBJECT_POSITION + 8) == door->y &&
                                       !o[nativeinteract::OBJECT_STATE];
                            });
        check(opened, gate.countRoute ? "retained guard count opens the gate and publishes its state"
                                      : "higher-step dungeon air opens the gate and publishes its state");
        check(!(terrain.gameBlock(dx, dy, dz)[5] & 0x40), "opening the gate clears the generated footprint");
        const auto sounds = actors.takeSounds();
        const int expectedSound = gate.type == 8 ? 0x36 : 0x39;
        check(std::count_if(sounds.begin(), sounds.end(), [&](const auto &sound) {
                  return soundrules::readRequest(sound).number == expectedSound;
              }) == 1, "gate transition delivers its sound once to local audio");
    }
}
} // namespace

int worldGateCaptureCommand(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input || input.tellg() < 0 || input.tellg() > 16 * 1024)
        return std::puts("Invalid gate transition capture size"), 1;
    input.seekg(0);
    std::string line;
    if (!std::getline(input, line) || line != "CW-DUNGEON-GATE-LIVE 1")
        return std::puts("Invalid gate transition capture header"), 1;
    size_t counts[2]{}, differences = 0;
    while (std::getline(input, line)) {
        std::istringstream row(line);
        int route = 0, type = 0;
        std::array<int64_t, 3> position{};
        unsigned before = 0, after = 0;
        uint32_t oldSwing = 0, newSwing = 0;
        if (!(row >> route >> type >> position[0] >> position[1] >> position[2] >> before >> after >> oldSwing >> newSwing) ||
            route < 1 || route > 2 || (type != 8 && type != 10) || !before || before > 255 || after > 255 ||
            counts[0] + counts[1] >= 64)
            return std::puts("Invalid gate transition row"), 1;
        row >> std::ws;
        if (!row.eof())
            return std::puts("Unexpected trailing gate transition fields"), 1;
        ++counts[route - 1];
        float timer = 0;
        std::memcpy(&timer, &oldSwing, sizeof timer);
        auto gate = object(type, uint8_t(before), timer);
        std::memcpy(gate.data() + nativeinteract::OBJECT_POSITION, position.data(), sizeof position);
        std::vector<nativeinteract::Sound> sounds;
        if (!nativeinteract::openOrShut(gate, 0, sounds) || gate[nativeinteract::OBJECT_STATE] != after ||
            get<uint32_t>(gate.data(), nativeinteract::OBJECT_SWING) != newSwing) {
            std::printf("Gate transition differs at row %zu\n", counts[0] + counts[1]);
            ++differences;
        }
    }
    std::printf("Gate transition setter replay: %zu count-route rows, %zu proximity-route rows, %zu differences\n",
                counts[0], counts[1], differences);
    return differences || counts[0] + counts[1] == 0 ? 1 : 0;
}

int worldObjectTests() {
    int failures = 0;
    auto check = [&](bool result, const char *message) {
        if (!result) {
            std::printf("FAIL World objects: %s\n", message);
            ++failures;
        }
    };
    {
        Scene scene;
        Actor npc(2, 1), deadPlayer(3, 0, -281474976710656LL, -281474976710656LL), other(4, 2);
        scene.actors = {&npc.side, &deadPlayer.side, &other.side};
        put(deadPlayer.record.data(), 0x180, 0.0f);
        for (auto *actor : scene.actors)
            put(actor->record, 0x128, uint16_t(0xffff));
        std::vector<nativeobjects::Cell> lookups;
        nativeobjects::collectZones(scene.world, [&](auto key) {
            check(get<uint16_t>(other.record.data(), 0x128) == 0xdffd, "all flags clear before zone lookup begins");
            lookups.push_back(key);
        });
        check(lookups.size() == 49 && lookups.front() == nativeobjects::Cell{-4, -4} && lookups[1] == nativeobjects::Cell{-4, -3} &&
                  lookups.back() == nativeobjects::Cell{2, 2},
              "dead kind0 player selects native narrowed7x7 keys in X/Y lookup order");
        check(get<uint16_t>(npc.record.data(), 0x128) == 0xdffd, "nonplayers also clear0x2002");
        scene.actors = {&npc.side};
        lookups.clear();
        nativeobjects::collectZones(scene.world, [&](auto key) { lookups.push_back(key); });
        check(lookups.empty() && scene.world.gatePlayers.empty(), "nonplayers do not select zones or join the gate actor snapshot");
    }
    {
        Scene scene;
        std::vector<nativeobjects::Cell> searched;
        scene.world.gateThreshold = [](nativeobjects::Cell cell) -> std::optional<int32_t> {
            return cell == nativeobjects::Cell{-1, 0} ? std::optional<int32_t>{3} : std::nullopt;
        };
        scene.world.guardCount = [&](nativeobjects::Cell zone, nativeobjects::Cell cell) {
            check(cell == nativeobjects::Cell{-1, 0}, "all guard-count lookups use the gate's signed cell");
            searched.push_back(zone);
            return zone == nativeobjects::Cell{-17, -16} ? 2 : zone == nativeobjects::Cell{15, 16} ? 1 : 0;
        };
        auto gate = object(8, 1, 600);
        put(gate.data(), nativeinteract::OBJECT_POSITION, -BLOCK);
        nativeobjects::step(scene.world, gate, 20);
        check(searched.size() == 1089 && searched.front() == nativeobjects::Cell{-17, -16} &&
                  searched[1] == nativeobjects::Cell{-17, -15} && searched.back() == nativeobjects::Cell{15, 16},
              "guard count includes the whole 33 by 33 rectangle in X/Y order");
        check(gate[nativeinteract::OBJECT_STATE] == 0 && get<float>(gate.data(), nativeinteract::OBJECT_SWING) == 380 &&
                  scene.order == std::vector<std::string>{"sound54", "changed"},
              "threshold equality opens a type-8 gate after advancing the timer and publishes the change");
        nativeobjects::step(scene.world, gate, 20);
        check(searched.size() == 1089 && scene.changes == 1, "an open gate does not query counts or toggle back");
    }
    {
        Scene scene;
        Actor player(1, 0, 15 * BLOCK), npc(2, 1);
        scene.actors = {&npc.side, &player.side};
        nativeobjects::collectZones(scene.world, {});
        scene.world.actor = [&](int64_t id) { return id == 1 ? player.record.data() : nullptr; };
        scene.world.guardCount = [](auto, auto) { return 0; };
        scene.world.castleSteps = [](nativeobjects::Cell cell) { return cell[0] < 0 ? 1 : 2; };
        bool dungeonBlock = false;
        scene.world.block = [&](int, int, int) {
            nativeai::Block result{};
            result[5] = dungeonBlock ? 0x20 : 0;
            return result;
        };
        auto gate = object(10, 1, 900);
        put(gate.data(), nativeinteract::OBJECT_POSITION, -BLOCK);
        nativeobjects::step(scene.world, gate, 0);
        check(scene.changes == 0, "missing threshold suppresses both gate paths");
        scene.world.gateThreshold = [](auto) -> std::optional<int32_t> { return 1; };
        dungeonBlock = true;
        nativeobjects::step(scene.world, gate, 0);
        check(scene.changes == 0, "proximity excludes exactly sixteen blocks");
        put(player.record.data(), 0x10, 14 * BLOCK);
        dungeonBlock = false;
        nativeobjects::step(scene.world, gate, 0);
        check(scene.changes == 0, "proximity requires the player's dungeon block flag");
        dungeonBlock = true;
        scene.world.client = true;
        nativeobjects::step(scene.world, gate, 0);
        check(scene.changes == 0, "client gate state is supplied by the host");
        scene.world.client = false;
        nativeobjects::step(scene.world, gate, 0);
        check(scene.world.gatePlayers == std::vector<int64_t>{1} && gate[nativeinteract::OBJECT_STATE] == 0 &&
                  scene.order == std::vector<std::string>{"sound57", "changed"},
              "below-threshold type-10 gate opens for the admitted player with a higher descriptor");
    }
    {
        Scene scene;
        Actor victim(2, 2, 3 * BLOCK);
        scene.actors = {&victim.side};
        scene.world.clockMs = 199.75f;
        scene.draws = {0, 32767};
        scene.world.hit = [&](nativeai::Creature &, const nativeupkeep::Hit &hit) {
            scene.hits.push_back(hit);
            scene.order.push_back("hit");
            scene.world.rand(); // Shared hit callbacks can consume their own random draws.
            soundrules::Request nested;
            nested.number = 0x1b;
            scene.world.sound(soundrules::makeRequest(nested));
        };
        auto trap = object(0x12, 1, 1000);
        nativeobjects::step(scene.world, trap, 0.5f);
        check(scene.hits.size() == 1 && scene.randomCalls == 2 && scene.sounds.size() == 2,
              "fractional forward clock crossing invokes hit and then direct sound draw");
        check(scene.order == std::vector<std::string>{"hit", "rand", "sound27", "rand", "sound1"},
              "nested hit/death outputs precede direct impact sound without replay");
        if (!scene.hits.empty()) {
            const auto &hit = scene.hits[0];
            check(hit.target == 2 && hit.attacker == 0 && hit.type == 0 && hit.last == 1 && hit.damage == 10 && hit.push[0] == 20 &&
                      hit.push[1] == 0 && hit.push[2] == 5 && hit.critical == 0 && hit.flag == 0 && hit.charge == 0 && hit.ability == 0 &&
                      hit.extra == 0,
                  "trap uses specified null-attacker hit parameters and horizontal push");
        }
        check(scene.sounds.size() == 2 && scene.sounds.back().number == 1 && scene.sounds.back().pitch == 1.3f &&
                  scene.sounds.back().volume == 1 && scene.sounds.back().owner == 0,
              "impact pitch uses post-hit draw and native0.3 multiply/divide/add");
    }
    {
        Scene scene;
        Actor edge(2, 2, 5 * BLOCK), inside(3, 3, 5 * BLOCK - 1), diagonal(4, 2, 3 * BLOCK, 4 * BLOCK);
        scene.actors = {&edge.side, &inside.side, &diagonal.side};
        scene.world.clockMs = 199;
        auto trap = object(0x11, 1, 1000);
        nativeobjects::step(scene.world, trap, 1);
        check(scene.hits.size() == 1 && scene.hits[0].target == 3 && scene.hits[0].type == 1,
              "strict nominal5-block test rejects axis and diagonal equality but accepts one dot inside");
        check(scene.sounds.empty() && scene.randomCalls == 0, "type11 has no direct impact sound/random draw");
    }
    {
        Scene scene;
        Actor activator(2, 0, 19 * BLOCK), dead(3, 2, BLOCK), friendly(4, 1), appended(5, 3, -BLOCK);
        put(activator.record.data(), 0x180, 0.0f);
        put(dead.record.data(), 0x180, 0.0f);
        scene.actors = {&activator.side, &dead.side, &friendly.side};
        scene.world.hit = [&](nativeai::Creature &target, const nativeupkeep::Hit &hit) {
            scene.hits.push_back(hit);
            scene.order.push_back("hit");
            if (&target == &dead.side)
                scene.actors.push_back(&appended.side);
            put(target.record, 0x10, int64_t(7 * BLOCK));
        };
        auto trap = object(0x13, 0, 2001);
        nativeobjects::step(scene.world, trap, 0);
        check(scene.changes == 1 && trap[nativeinteract::OBJECT_STATE] == 1 && get<float>(trap.data(), nativeinteract::OBJECT_SWING) == 0,
              "dead kind0 actor can activate a trap after strict2000ms");
        check(scene.hits.size() == 2 && scene.hits[0].target == 3 && scene.hits[1].target == 5,
              "hit tail skips only kind1; includes dead targets and callback tail births in registration order");
        check(!scene.hits.empty() && scene.hits[0].damage == 50 && scene.hits[0].push[0] == 40 && scene.hits[0].push[2] == 10,
              "type13 applies sequential maxHP fraction and doubled push");
        check(scene.order.size() >= 2 && scene.order[0] == "sound57" && scene.order[1] == "changed",
              "state-change sound and dirty notice precede trap hits");
        check(scene.sounds.size() == 3 && scene.sounds[1].position[0] == 7 * BLOCK && scene.sounds[2].position[0] == 7 * BLOCK,
              "impact sounds copy each target position after its hit callback");
    }
    {
        Scene scene;
        Actor target(2, 2);
        scene.actors = {&target.side};
        auto trap = object(0x13, 1, 0);
        nativeobjects::step(scene.world, trap, 0);
        nativeobjects::step(scene.world, trap, 0);
        check(scene.hits.size() == 2, "active type13 zero-time visits have no synthetic consumed-hit latch");
        nativeobjects::step(scene.world, trap, 0.25f);
        check(scene.hits.size() == 2, "positive timer advance precedes type13 exact-zero test");
        auto periodic = object(0x12, 1, 1000);
        scene.world.clockMs = 200;
        nativeobjects::step(scene.world, periodic, 0);
        check(scene.hits.size() == 2, "periodic traps do not fire at zero interval");
    }
    {
        Scene scene;
        Actor player(2, 0, 20 * BLOCK);
        scene.actors = {&player.side};
        auto trap = object(0x13, 0, 2001);
        nativeobjects::step(scene.world, trap, 0);
        check(scene.changes == 0, "activation rejects exact20-block distance");
        auto threshold = object(0x13, 1, 2000);
        nativeobjects::step(scene.world, threshold, 0);
        check(scene.changes == 0, "automatic transition rejects exact2000ms");
        nativeobjects::step(scene.world, threshold, 0.25f);
        check(scene.changes == 1 && threshold[nativeinteract::OBJECT_STATE] == 0, "nonzero state returns to state0 after strict threshold");
        put(player.record.data(), 0x10, int64_t(1) << 40);
        nativeobjects::step(scene.world, trap, 0);
        check(scene.changes == 2 && scene.hits.size() == 1,
              "distance keeps low64 squares and per-axis division rather than substituting a float sphere");
    }
    {
        Scene scene;
        Actor actor(2, 0);
        scene.actors = {&actor.side};
        scene.world.client = true;
        auto trap = object(0x13, 1, 2001);
        nativeobjects::step(scene.world, trap, 1);
        check(scene.hits.empty() && scene.changes == 0 && get<float>(trap.data(), nativeinteract::OBJECT_SWING) == 2002,
              "mirror advances clocks but does not cycle or hit traps");
        std::set<nativeobjects::Cell> used{{-1, 0}};
        scene.world.usedCells = &used;
        auto personal = object(0x16, 1, 1000);
        put(personal.data(), 8, -281474976710656LL);
        nativeobjects::step(scene.world, personal, 0);
        check(personal[nativeinteract::OBJECT_STATE] == 0 && get<float>(personal.data(), nativeinteract::OBJECT_SWING) == 0 &&
                  scene.sounds.empty() && scene.changes == 0,
              "mirror personal-state reset is silent and does not mark dirty");
    }
    {
        Scene scene;
        Actor actor(2, 2);
        scene.actors = {&actor.side};
        auto trap = object(0x12, 1, 1000);
        scene.world.clockMs = std::numeric_limits<float>::quiet_NaN();
        nativeobjects::step(scene.world, trap, 16);
        check(scene.hits.empty(), "invalid SSE clock conversions share the indefinite bucket");
        scene.world.clockMs = 2147483520.0f;
        nativeobjects::step(scene.world, trap, 128);
        check(scene.hits.size() == 1, "future clock conversion uses signed32 indefinite instead of saturation");
    }
    {
        Scene scene;
        Actor actor(2, 2);
        scene.actors = {&actor.side};
        const uint32_t healthBits = 0x3fa0013e;
        float health;
        std::memcpy(&health, &healthBits, sizeof health);
        scene.world.maxHealth = [=](const nativeai::Creature &) { return health; };
        auto trap = object(0x13, 1, 0);
        nativeobjects::step(scene.world, trap, 0);
        uint32_t damageBits = 0;
        if (!scene.hits.empty())
            std::memcpy(&damageBits, &scene.hits[0].damage, sizeof damageBits);
        check(damageBits == 0x3f20013f, "type13 retains separate0.1 then5 rounding instead of folding to0.5");
    }
    {
        Scene scene;
        Actor player(2, 0);
        scene.actors = {&player.side};
        auto first = object(0x13, 0, 2001);
        auto second = object(3, 1, 100);
        bool beforeNextClock = false;
        scene.world.hit = [&](nativeai::Creature &, const nativeupkeep::Hit &) {
            beforeNextClock = get<float>(second.data(), nativeinteract::OBJECT_SWING) == 100;
        };
        nativeobjects::step(scene.world, first, 16);
        nativeobjects::step(scene.world, second, 16);
        check(beforeNextClock && get<float>(second.data(), nativeinteract::OBJECT_SWING) == 116,
              "each object's state/hit callbacks finish before the next object's clock advances");
    }
    {
        // Actual facade: a loaded zone's native object list, local actor, hit and event queues.
        const auto root = std::filesystem::temp_directory_path() /
                          ("CubeVulkan-world-objects-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        writeFixture(root, 1);
        {
            ExtractedWorld terrain;
            std::string why;
            check(terrain.open(root, why), "facade fixture opens");
            SpawnedCreatures actors;
            check(!actors.cameraSubject(), "camera source is absent before a canonical local player exists");
            actors.setNativePlayer(true);
            actors.setPlayer(terrain, {1, 1, ExtractedWorld::Z_OFFSET + 1}, 0, {}, 100, 1, true);
            actors.setPlayerHealth(actors.playerMaxHealth());
            auto *cameraRecord = actors.localPlayer()->record;
            int64_t previousDrawnX;
            std::memcpy(&previousDrawnX, cameraRecord + 0xb10, 8);
            const int64_t otherDrawnX = 1234LL * 65536;
            std::memcpy(cameraRecord + 0xb10, &otherDrawnX, 8);
            const auto camera = actors.cameraSubject(true);
            check(camera && camera->physical[0] == 65536 && camera->physical[1] == 65536 && camera->creationLayerShown,
                  "camera facade selects physical coordinates rather than a different drawn pose and carries creation visibility");
            std::memcpy(cameraRecord + 0xb10, &previousDrawnX, 8);
            const float before = actors.playerHealth();
            auto trap = object(0x13, 1, 0);
            std::memcpy(trap.data() + 8, actors.localRecord() + 0x10, 24);
            cwnet::ZoneObjects objects;
            objects.key = {0, 0};
            objects.records.push_back(trap);
            actors.replaceStaticObjects(terrain, objects);
            actors.simulate(terrain, 0);
            check(actors.playerHealth() < before && !actors.events().damageNumbers.empty(),
                  "facade zero-time trap reaches actual local-player hit/health/event path");
            const auto first = actors.takeSounds();
            check(!first.empty() && actors.takeSounds().empty(), "facade delivers trap sounds once to local audio");
            check(first == actors.events().sounds, "local audio and outgoing world retain the same trap/hit sound order");
            hudcontext::Tabs inventory(3);
            const auto requirement = nativesaves::makeItem(22, 7);
            inventory[1].push_back({3, requirement});
            actors.setHudContextSource(true, &inventory);
            auto required = object(0xfe, 1, 0);
            std::copy(requirement.begin(), requirement.end(), required.begin() + 0x38);
            objects.records = {required};
            actors.replaceStaticObjects(terrain, objects);
            actors.simulate(terrain, 0);
            check(actors.hudContext().item == requirement && actors.hudContext().count == 3,
                  "actual zero-time World facade derives the context from loaded requirements and canonical inventory");
            inventory[1][0].count = 0;
            check(actors.hudContext().count == 3, "inventory edits retain the prior snapshot until a World rebuild");
            actors.simulate(terrain, 0);
            check(actors.hudContext().item[0] == 0 && actors.hudContext().count == 0,
                  "the next zero-time World pass clears the unavailable selected requirement");
            auto glider = nativesaves::makeItem(24, 1);
            std::memcpy(glider.data() + 0x0c, actors.localRecord() + 0x150, 8);
            inventory[1].push_back({1, glider});
            put(actors.localPlayer()->record, 0x5c, uint32_t(0));
            put(actors.localPlayer()->record, 0x6c, 1000.0f);
            actors.simulate(terrain, 0);
            check(actors.vehicleItem()[0] == 24 && get<int32_t>(actors.vehicleItem().data(), 4) == 1,
                  "actual zero-time World facade derives the vehicle before local input");
            inventory[1].back().count = 0;
            check(actors.vehicleItem()[0] == 24, "inventory changes do not replace the retained vehicle cache");
            actors.simulate(terrain, 0);
            check(actors.vehicleItem()[0] == 0, "next World pass clears an unavailable provisional-free vehicle");
            inventory[1].back().count = 1;
            actors.simulate(terrain, 0);
            check(actors.vehicleItem()[0] == 24, "availability can repopulate the admitted cache");
            put(actors.localPlayer()->record, 0x10, int64_t(128 * 65536));
            put(actors.localPlayer()->record, 0x5c, uint32_t(0));
            put(actors.localPlayer()->record, 0x6c, 1000.0f);
            bool loaded = true;
            const auto missing = terrain.gameBlock(128, 0, 1, &loaded);
            check(!loaded && missing == ExtractedWorld::GameBlock{200, 200, 200, 0, 1, 0},
                  "missing-zone effective lookup returns the initialized gray-solid copy");
            actors.simulate(terrain, 0);
            check(actors.vehicleItem()[0] == 24 && get<int32_t>(actors.vehicleItem().data(), 4) == 1,
                  "actual World vehicle selection uses missing-zone fallback flags instead of deferring");
            actors.resetPlayer();
            check(actors.vehicleItem()[0] == 0 && actors.hudContext().item[0] == 0 && actors.hudContext().count == 0,
                  "fresh-character reset initializes both derived CharacterData item caches");
        }
        {
            ExtractedWorld terrain;
            std::string why;
            check(terrain.open(root, why), "legacy facade fixture opens");
            SpawnedCreatures actors;
            actors.setNativePlayer(false);
            actors.setPlayerId(77);
            actors.setPlayer(terrain, {1, 1, ExtractedWorld::Z_OFFSET + 1}, 0, {}, 100, 1, true);
            int64_t hitTarget = -1;
            actors.setPlayerHit([&](const nativeupkeep::Hit &hit, const uint8_t *attacker) {
                hitTarget = hit.target;
                check(attacker == nullptr, "legacy trap callback keeps null attacker");
                return false;
            });
            auto trap = object(0x13, 1, 0);
            std::memcpy(trap.data() + 8, actors.localRecord() + 0x10, 24);
            cwnet::ZoneObjects objects;
            objects.key = {0, 0};
            objects.records.push_back(trap);
            actors.replaceStaticObjects(terrain, objects);
            actors.simulate(terrain, 0);
            check(hitTarget == 77 && !actors.takeSounds().empty(),
                  "actual legacy bridge receives native record ID and emits impact even for a rejected hit");
        }
        writeFixture(root, 17);
        {
            ExtractedWorld terrain;
            std::string why;
            check(terrain.open(root, why), "lava fixture opens");
            SpawnedCreatures actors;
            actors.setNativePlayer(true);
            actors.setPlayer(terrain, {1, 1, ExtractedWorld::Z_OFFSET + 1}, 0, {}, 100, 1, true);
            actors.setPlayerHealth(actors.playerMaxHealth());
            const float before = actors.playerHealth();
            actors.simulate(terrain, 200);
            check(actors.elapsedWorldTime() == 200 && actors.playerHealth() < before && !actors.events().damageNumbers.empty(),
                  "facade physics receives the advanced world clock for200ms lava damage");
        }
        for (const auto [base, z, type] : {std::tuple{0, -1, 1}, {0, 0, 2}, {0, 1, 0},
                                           {4, 0, 1}, {4, 3, 1}, {4, 4, 0}, {4, 5, 0},
                                           {-4, -5, 1}, {-4, -4, 2}, {-4, -3, 2}, {-4, 0, 2}, {-4, 1, 0}}) {
            writeFixture(root, 0, base, true);
            ExtractedWorld terrain;
            std::string why;
            check(terrain.open(root, why), "empty-column fallback fixture opens");
            terrain.propsAbsolute(0, 0); // load only this physical zero-record zone
            bool loaded = false;
            const auto result = terrain.gameBlock(0, 0, z, &loaded);
            const uint8_t color = type == 1 ? 200 : 255;
            check(loaded && result == ExtractedWorld::GameBlock{color, color, color, 0, uint8_t(type), 0},
                  "empty effective columns preserve base-before-sea ordering and exact fallback bytes");
        }
        worldStepAdmissionOrder(root, check);
        selectedMovementIntegration(root, check);
        capturedGateFootprint(root, check);
        std::filesystem::remove_all(root);
    }
    sourceSummaryValues(check);
    generatedGateIntegration(check);
    std::printf("World objects, gate transitions, trap state/hits/sounds and facade timing: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
