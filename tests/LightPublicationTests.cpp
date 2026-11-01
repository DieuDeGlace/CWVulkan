#include "Extracted.h"
#include "AlphaLook.h"
#include "NativeUpkeep.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>

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
    }
    setRenderStyle(oldStyle);
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
    std::printf("Stored gameplay light publication: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
