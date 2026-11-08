#include "Extracted.h"
#include "AlphaLook.h"
#include "NativeUpkeep.h"
#include "Multiplayer.h"
#include "ZoneStates.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>

namespace {
template <class T> void field(std::ostream &out, const T &value) {
    out.write(reinterpret_cast<const char *>(&value), sizeof value);
}
void zone(const std::filesystem::path &directory, int32_t x, int32_t y) {
    std::ofstream out(directory / ("z_" + std::to_string(x) + "_" + std::to_string(y) + ".cwz"), std::ios::binary);
    out.write("CWZ1", 4);
    field(out, x);
    field(out, y);
    field(out, int32_t(0));
    field(out, int32_t(0));
    for (int column = 0; column < 4096; ++column) {
        field(out, int32_t(0));
        field(out, uint32_t(4));
        for (int z = 0; z < 4; ++z) {
            field(out, uint16_t(1));
            ExtractedWorld::GameBlock block{100, 90, 80, 0, 1, 0};
            // A raw under-sea air record is still effectively canonical water.
            if (x == 0 && y == 0 && column == 36 * 64 + 32 && z == 0)
                block = {255, 255, 255, 0, 0, 0};
            out.write(reinterpret_cast<const char *>(block.data()), block.size());
        }
    }
}

int modelLightTests() {
    namespace fs = std::filesystem;
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        if (!ok) {
            ++failures;
            std::printf("FAIL model light publication: %s\n", message);
        }
    };
    const auto root = fs::temp_directory_path() /
                      ("CubeVulkan-model-light-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto seed = root / "seed";
    if (!fs::create_directories(seed))
        return 1;
    std::ofstream(seed / "manifest.txt") << "seed=0\nhomeX=32\nhomeY=32\nhomeZ=3\n";
    std::ofstream(root / "sprite-groups.txt") << "# environment\n0 sprites/fixture.cub\n";
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
            zone(seed, x, y);
    std::vector<ExtractedWorld::ModelRecord> records(11);
    const std::array<int64_t, 3> positions[] = {
        {32 * 65536, 32 * 65536, 65536}, {32 * 65536, 32 * 65536, 4 * 65536},
        {32 * 65536, 32 * 65536, -65536}, {-9 * 65536, 32 * 65536, -65536},
        {-32768, 32 * 65536, 6 * 65536}, {32 * 65536, 32 * 65536, 65536},
        {32 * 65536, 32 * 65536, 65536}, {32 * 65536, 32 * 65536, 65536},
        {32 * 65536, 32 * 65536, 65536}, {36 * 65536, 32 * 65536, 0},
        {34 * 65536, 32 * 65536, 65536}};
    for (size_t i = 0; i < records.size(); ++i) {
        auto &r = records[i];
        r.fill(0xa5);
        const auto put = [&](size_t offset, const auto &value) { std::memcpy(r.data() + offset, &value, sizeof value); };
        put(0, int32_t(i == 5 ? 0x59 : i == 6 ? -1 : 0));
        put(8, positions[i]);
        put(32, i == 7 ? std::numeric_limits<float>::quiet_NaN() : i == 0 ? 99.0f : 1.0f);
        put(36, 0.0f);
        put(40, i == 8 ? std::numeric_limits<float>::quiet_NaN() : i == 10 ? -1.0f : 91.0f);
        put(44, 1.0f);
        put(48, 1.0f);
        put(52, 1.0f);
        put(56, int32_t(4));
    }
    {
        std::ofstream out(seed / "z_0_0.rec", std::ios::binary);
        out.write("CWR1", 4);
        const uint32_t header[]{0x18, 0x40, uint32_t(records.size())};
        out.write(reinterpret_cast<const char *>(header), sizeof header);
        out.write(reinterpret_cast<const char *>(records.data()), records.size() * sizeof(records[0]));
    }
    const auto scalar = [](const ExtractedWorld::ModelRecord &record) {
        float value;
        std::memcpy(&value, record.data() + 40, 4);
        return value;
    };
    const std::array<float, 11> expected{21, 255, 0, 255, 0, 21, 21, 21, 21, 21, 21};
    const auto oldStyle = renderStyle();
    for (const auto style : {RenderStyle::Steam, RenderStyle::Alpha}) {
        setRenderStyle(style);
        ExtractedWorld world;
        std::string error;
        check(world.open(seed, error), "model fixture opens");
        for (int x = -1; x <= 1; ++x)
            for (int y = -1; y <= 1; ++y)
                world.loadSourceZone(x, y);
        world.setGameBlock(32, 32, 1, {255, 23, 45, 17, 0, 2});
        world.setGameBlock(34, 32, 1, {255, 1, 2, 0, 0, 2});
        // A negative half-block must select the solid left column at z6;
        // truncating x toward zero would select the sky above the centre zone.
        world.setGameBlock(-1, 32, 6, {100, 90, 80, 0, 1, 0});
        world.setGameBlock(-1, 32, 7, {100, 90, 80, 0, 1, 0});
        world.takeChangedZones();
        const auto before = world.gameModels(0, 0);
        const auto identity = world.gameZoneToken(0, 0);
        Mesh opaque, water;
        check(world.mesh(0, 0, opaque, water), "actual mesh prepares all models without a scenery-bake call");
        const auto after = world.gameModels(0, 0);
        check(before && after && before != after && after->size() == records.size(), "model writes publish an immutable replacement");
        if (!before || !after || after->size() != records.size())
            continue;
        check(*before == records, "held pre-mesh records remain unchanged");
        for (size_t i = 0; i < expected.size(); ++i) {
            if (scalar((*after)[i]) != expected[i])
                std::printf("model %zu: light %g expected %g\n", i, double(scalar((*after)[i])), double(expected[i]));
            check(scalar((*after)[i]) == expected[i], "every model is sampled, including filtered, beam and poisoned-cache records");
            check((*after)[i][56] == (i == 9 ? 12 : 4), "sampling preserves all other flag bits");
            for (size_t byte = 0; byte < sizeof(records[i]); ++byte)
                if ((byte < 40 || byte >= 44) && byte != 56)
                    check((*after)[i][byte] == records[i][byte], "sampling preserves unrelated semantic and padding bytes");
        }
        check(((*after)[9][56] & 8) && !((*after)[0][56] & 8), "captured effective type-two lookup sets only the matching water flag");
        const auto now = world.gameZoneToken(0, 0);
        check(identity && now && identity->order == now->order, "derived model writes preserve loaded identity");
        check(world.takeChangedZones().empty() && world.takeChangedModels().empty(), "derived model writes create no dirty/history notification");
        world.setGameBlock(32, 32, 1, {0, 23, 45, 0, 0, 2});
        world.setGameBlock(34, 32, 1, {100, 90, 80, 0, 1, 0});
        world.setGameBlock(36, 32, 0, {0, 0, 0, 0, 0, 2});
        const auto scenery = world.props(0, 0);
        check(!scenery.empty() && scenery[0].light == 21.0f / 255 && world.gameBlockLight(32, 32, 1) == 5,
              "scene retains the prepared scalar while live gameplay light applies its separate minimum five");
        check(world.gameModels(0, 0) == after, "terrain setters and scene reads do not resample or reset model light");
        world.takeChangedZones();
        opaque = {};
        water = {};
        check(world.mesh(0, 0, opaque, water), "later admitted mesh completes");
        const auto refreshed = world.gameModels(0, 0);
        check(refreshed && scalar((*refreshed)[10]) == 0, "later mesh overwrites a positive scalar with zero");
        check(refreshed && ((*refreshed)[9][56] & 8), "water flag remains ORed after the source block becomes dry");
        check(scalar((*after)[10]) == 21 && world.takeChangedZones().empty() && world.takeChangedModels().empty(),
              "refresh preserves retained snapshots and emits no new dirty request");
        opaque = {};
        water = {};
        check(world.mesh(0, 0, opaque, water) && world.gameModels(0, 0) == refreshed,
              "unchanged prepared samples reuse their immutable model vector");
    }
    setRenderStyle(oldStyle);
    const auto owned = fs::weakly_canonical(root);
    if (owned.parent_path() != fs::weakly_canonical(fs::temp_directory_path()) ||
        owned.filename().string().rfind("CubeVulkan-model-light-", 0) != 0)
        return failures + 1;
    fs::remove_all(owned);
    std::printf("Mesh-time model light publication: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
} // namespace

int lightPublicationTests() {
    namespace fs = std::filesystem;
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        if (!ok) {
            ++failures;
            std::printf("FAIL light publication: %s\n", message);
        }
    };
    const auto directory = fs::temp_directory_path() /
                           ("CubeVulkan-stored-light-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if (!fs::create_directory(directory))
        return 1;
    std::ofstream(directory / "manifest.txt") << "seed=0\nhomeX=32\nhomeY=32\nhomeZ=3\n";
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
            zone(directory, x, y);
    const auto oldStyle = renderStyle();
    for (const auto style : {RenderStyle::Steam, RenderStyle::Alpha}) {
        setRenderStyle(style);
        ExtractedWorld world;
        std::string error;
        check(world.open(directory, error), "roofed room fixture opens");
        // Load all nine inputs before a target mesh can solve its neighborhood.
        world.keepAround(0, 0, 1);
        for (int x = -1; x <= 1; ++x)
            for (int y = -1; y <= 1; ++y)
                world.propsAbsolute(x, y);
        const auto identity = world.gameZoneToken(0, 0);
        world.setGameBlock(32, 32, 1, {255, 23, 45, 17, 0, 2});
        world.setGameBlock(34, 32, 1, {255, 1, 2, 0, 0, 2});
        world.setGameBlock(-1, 0, 1, {173, 19, 29, 0, 0, 2});
        check(world.gameBlockLight(32, 32, 1) == 255, "new physical bytes are visible before solving");
        // Two solid roof layers leave every adjacent sample dark. Native spread
        // still takes max(neighbor,25)*85/100, giving the independent floor21.
        Mesh opaque, water;
        worldgen::MeshLightVolume solved;
        check(world.mesh(0, 0, opaque, water, nullptr, &solved), "actual CPU mesh job completes");
        check(world.gameBlock(32, 32, 1) == ExtractedWorld::GameBlock{21, 23, 45, 17, 0, 2},
              "completed target red is stored while other five bytes retain their values");
        check(world.gameBlockLight(32, 32, 1) == 21 && world.gameBlockLight(34, 32, 1) == 21,
              "gameplay query sees the specified enclosed-cell spread floor21 in both render styles");
        check(world.gameBlock(-1, 0, 1)[0] == 173, "target solve does not publish its halo neighbor");
        check(solved.at(36, 32, 0) == 21 && world.gameBlockLight(36, 32, 0) == 255,
              "effective sea substitution takes precedence over a raw solved-air sample");
        check(world.takeChangedZones().empty() && world.takeChangedModels().empty(), "light writeback creates no terrain/model notice");
        const auto after = world.gameZoneToken(0, 0);
        check(identity && after && identity->order == after->order, "light storage replacement preserves loaded-zone identity");
        world.setGameBlock(34, 32, 1, {88, 1, 2, 0, 0, 2});
        world.invalidateGameBlock(34, 32);
        check(world.gameBlockLight(34, 32, 1) == 88, "dirty requests retain newly written physical light bytes");
        world.takeChangedZones();
        opaque = {};
        water = {};
        check(world.mesh(0, 0, opaque, water) && world.gameBlockLight(34, 32, 1) == 21 && world.takeChangedZones().empty(),
              "later solve replaces retained red without generating another dirty request");
        {
            SpawnedCreatures authority, mirror;
            nativesaves::WorldState stored;
            Multiplayer session;
            Multiplayer::Setup setup;
            setup.hostWorld = &authority;
            setup.clientWorld = &mirror;
            setup.blocks = &world;
            check(session.start(setup, error), "combined local session starts");
            auto *host = session.hostBlocks();
            check(host && host != &world, "combined session owns an independent host terrain reader");
            if (host && host != &world) {
                check(host->directory() == world.directory() && host->seed() == world.seed() && host->zoneCount() == world.zoneCount() &&
                          host->home() == world.home() && host->originDotsX() == world.originDotsX() &&
                          host->originDotsY() == world.originDotsY(),
                      "source fork retains source metadata and coordinate rebasing");
                check(!host->gameZoneLoaded(0, 0), "source fork does not inherit another World's loaded lifetime");
                host->propsAbsolute(0, 0);
                authority.setStoredZones(*host, &stored, false);
                mirror.setStoredZones(world, nullptr, false);
                const auto hostIdentity = host->gameZoneToken(0, 0);
                check(hostIdentity && hostIdentity->order != world.gameZoneToken(0, 0)->order &&
                          host->gameBlock(32, 32, 1) == ExtractedWorld::GameBlock{100, 90, 80, 0, 1, 0},
                      "host loads its own base records rather than inheriting client physical edits or solved red");
                host->setGameBlock(32, 32, 1, {255, 23, 45, 17, 0, 2});
                opaque = {};
                water = {};
                check(world.mesh(0, 0, opaque, water) && world.gameBlockLight(32, 32, 1) == 21 && host->gameBlockLight(32, 32, 1) == 255,
                      "client mesh-light publication cannot overwrite host gameplay light");
                const auto before = host->gameBlock(33, 32, 1);
                const ExtractedWorld::GameBlock changed{88, 4, 5, 0, 0, 2};
                const auto change = zonestates::blockChange(33, 32, 1, before, changed);
                host->setBlockChanges(0, 0, {change});
                check(host->gameBlock(33, 32, 1) == changed && world.gameBlock(33, 32, 1) == before,
                      "host physical/history writes leave the client unchanged before a notice");
                cwnet::BlockChangeRecord notice{};
                std::memcpy(notice.data(), change.bytes.data(), notice.size());
                session.mirrorSide()->applyBlockChanges({notice});
                session.mirrorSide()->zoneChangeStep();
                check(world.gameBlock(33, 32, 1) == changed, "copied protocol notices update the client's own loaded record");
                world.undoBlockChanges(0, 0);
                check(world.gameBlock(33, 32, 1) == before && host->gameBlock(33, 32, 1) == changed,
                      "client history undo cannot restore host physical data");
                check(world.open(directory, error) && host->gameBlockLight(32, 32, 1) == 255 && host->gameBlock(33, 32, 1) == changed &&
                          hostIdentity && host->gameZoneToken(0, 0)->order == hostIdentity->order,
                      "client source reload preserves the host's independent data and loaded lifetime");
            }
        }
        {
            SpawnedCreatures authority;
            Multiplayer session;
            Multiplayer::Setup setup;
            setup.hostWorld = &authority;
            setup.blocks = &world;
            check(session.start(setup, error) && session.dedicated() && session.hostBlocks() == &world,
                  "dedicated host uses its provided reader without an unnecessary fork");
        }
    }
    setRenderStyle(oldStyle);
    {
        ExtractedWorld generated;
        generated.openGenerated(129, -130);
        auto other = generated.forkSource();
        check(other->generated() && other->home() == generated.home() && other->originZoneX() == 2 && other->originZoneY() == -3 &&
                  other->has(99, -99) && !other->gameZoneLoaded(2, -3),
              "generated source forks retain generation and negative-coordinate rebasing without loading a zone");
    }
    {
        std::array<uint8_t, 0x15f8> record{};
        const auto put = [&](size_t offset, const auto &value) { std::memcpy(record.data() + offset, &value, sizeof value); };
        put(0x10, -(int64_t(1) << 48));
        put(0x18, int64_t(-1));
        put(0x20, int64_t(65536));
        put(0x180, 1.0f);
        record[0x68] = 0x50; // hiding admits the channel0 query even at zero elapsed
        nativeai::Creature actor;
        actor.record = record.data();
        nativeupkeep::World world;
        world.client = true;
        std::array<int, 3> sampled{};
        int queries = 0;
        world.light = [&](int x, int y, int z) {
            sampled = {x, y, z};
            ++queries;
            return 0;
        };
        nativeupkeep::upkeep(world, &actor, 0);
        check(queries == 1 && sampled == std::array<int, 3>{-1, -1, 1},
              "upkeep applies native signed narrowing before the negative fixed-coordinate correction");
    }
    const auto owned = fs::weakly_canonical(directory);
    if (owned.parent_path() != fs::weakly_canonical(fs::temp_directory_path()) ||
        owned.filename().string().rfind("CubeVulkan-stored-light-", 0) != 0)
        return failures + 1;
    fs::remove_all(owned);
    failures += modelLightTests();
    std::printf("Stored gameplay light publication: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
