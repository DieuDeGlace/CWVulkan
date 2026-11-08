#include "Extracted.h"
#include "NetWorlds.h"
#include "Spawned.h"
#include "ZoneStates.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>

namespace {
void writeSource(const std::filesystem::path &directory, int32_t x, int32_t y) {
    std::ofstream out(directory / ("z_" + std::to_string(x) + "_" + std::to_string(y) + ".cwz"), std::ios::binary);
    const auto field = [&](const auto &value) { out.write(reinterpret_cast<const char *>(&value), sizeof value); };
    out.write("CWZ1", 4);
    field(x); field(y); field(int32_t(0)); field(int32_t(0));
    for (int column = 0; column < 4096; ++column) {
        field(int32_t(0)); field(uint32_t(1)); field(uint16_t(1));
        const ExtractedWorld::GameBlock block{100, 90, 80, 0, 1, 0};
        out.write(reinterpret_cast<const char *>(block.data()), block.size());
    }
}
using Key = std::pair<int, int>;
std::vector<Key> keys(const std::vector<cwnet::ZoneKey> &zones) {
    std::vector<Key> out;
    for (const auto &zone : zones) out.emplace_back(zone.x, zone.y);
    return out;
}
} // namespace

int sourceControlTests() {
    namespace fs = std::filesystem;
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        if (!ok) { ++failures; std::printf("FAIL source control: %s\n", message); }
    };
    const auto directory = fs::temp_directory_path() /
        ("CubeVulkan-source-state-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if (!fs::create_directory(directory)) return 1;
    std::ofstream(directory / "manifest.txt") << "seed=0\nhomeX=0\nhomeY=0\nhomeZ=1\n";
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y) writeSource(directory, x, y);
    writeSource(directory, 4, 0);
    writeSource(directory, -5, 0);
    {
        ExtractedWorld blocks;
        std::string error;
        check(blocks.open(directory, error), "fixture opens");
        SpawnedCreatures mirror, authority;
        mirror.setStoredZones(blocks, nullptr, false);
        mirror.setRemoteClient(true);
        const std::vector<Key> home{{0, 0}};
        check(keys(mirror.zonesNeedingState(0, 0)) == home,
              "a fresh physical source needs state before logical lists exist");
        check(keys(mirror.zonesNeedingState(0, 0)) == home,
              "request selection does not clear the source flag");
        auto independent = blocks.forkSource();
        check(independent->loadSourceZone(0, 0), "independent host source is explicitly admitted");
        mirror.zoneAnswered({0, 0});
        check(mirror.zonesNeedingState(0, 0).empty(), "a loaded coordinate reply clears the flag");
        check(independent->sourceStateRequests(0, 0) == home,
              "a viewed clear cannot change the independent host source control");
        independent->sourceStateReceived(0, 0);
        const auto identity = blocks.gameZoneToken(0, 0);
        blocks.setGameBlock(0, 0, 0, {9, 8, 7, 0, 1, 0});
        check(mirror.zonesNeedingState(0, 0).empty(), "COW column replacement preserves a cleared flag");
        blocks.loadSourceZone(0, 0);
        check(blocks.gameZoneToken(0, 0)->order == identity->order && mirror.zonesNeedingState(0, 0).empty(),
              "an existing-source factory hit neither changes identity nor resets the flag");
        mirror.undoBlockChanges(blocks);
        check(keys(mirror.zonesNeedingState(0, 0)) == home,
              "connection reset flags physical sources with no logical lists or edits");
        check(independent->sourceStateRequests(0, 0).empty(),
              "a viewed reset cannot flag the independent host source");
        blocks.loadSourceZone(4, 0);
        nativesaves::WorldState stored;
        auto &saved = stored.zones[{4, 0}];
        saved.flags = nativesaves::ZoneState::GROUND_ITEMS_SAVED;
        saved.groundItems.resize(1);
        saved.groundItems[0][0] = 0x55;
        authority.setStoredZones(blocks, &stored, false);
        cwnet::ZoneAnswer answer;
        check(authority.zoneAnswer(blocks, {4, 0}, answer), "loaded host source admits a reply");
        mirror.zoneAnswered({4, 0});
        cwnet::ZoneAnswer clearedControl;
        check(authority.zoneAnswer(blocks, {4, 0}, clearedControl) && clearedControl.groundItems &&
                  *clearedControl.groundItems == saved.groundItems,
              "host F8 does not admit replies or alter the independent saved-array flag");
        const auto lease = blocks.gameZoneToken(4, 0);
        blocks.setClientStreaming(true);
        check(blocks.retireClientSource({0, 0}, 1) == Key{4, 0}, "far physical source retires");
        cwnet::ZoneAnswer missing;
        check(!authority.zoneAnswer(blocks, {4, 0}, missing) && missing.blockChanges.empty() &&
                  !missing.groundItems && !missing.staticObjects && !blocks.gameZoneLoaded(4, 0),
              "host skips a missing physical source even when logical lists remain");
        mirror.zoneAnswered({4, 0});
        blocks.loadSourceZone(4, 0);
        check(blocks.gameZoneToken(4, 0)->order != lease->order &&
                  keys(mirror.zonesNeedingState(256, 0)) == std::vector<Key>{{0, 0}, {4, 0}},
              "reload has a fresh flag despite an old lease and a reply received while absent");
        for (int x = 1; x >= -1; --x)
            for (int y = 1; y >= -1; --y) blocks.loadSourceZone(x, y);
        blocks.loadSourceZone(-5, 0);
        mirror.undoBlockChanges(blocks);
        std::vector<Key> expected;
        for (int x = -1; x <= 1; ++x)
            for (int y = -1; y <= 1; ++y) expected.emplace_back(x, y);
        expected.emplace_back(4, 0);
        check(keys(mirror.zonesNeedingState(0, 0)) == expected,
              "loaded-only nine-by-nine request scan is X-major, independent of insertion order");
        mirror.setPlayer(blocks, {-0.5f, -0.5f, 167}, 0, {}, 100, 1, true);
        MirrorSide side(mirror, blocks);
        check(keys(side.neededZones()) == expected,
              "negative fractional player position truncates before request-window bounds");
        mirror.setRemoteClient(false);
        check(side.neededZones().empty(), "zero viewed role gate skips request construction");
        mirror.setRemoteClient(true);
        auto *player = const_cast<uint8_t *>(mirror.localRecord());
        const std::array<int64_t, 3> far{int64_t(2048) * 65536, 0, 65536};
        std::memcpy(player + 0x10, far.data(), sizeof far);
        player[0x68] = 0x9c;
        const std::array<int32_t, 2> destination{0, 0};
        std::memcpy(player + 0x1b0, destination.data(), sizeof destination);
        check(keys(side.neededZones()) == expected, "eagle anchor replaces the physical request-window centre");
        player[0x68] = 0;
        const auto count = blocks.loadedSourceCount();
        check(side.neededZones().empty() && blocks.loadedSourceCount() == count,
              "request selection never generates or prefetches missing sources");
        const auto was = blocks.gameBlock(0, 0, 0);
        const ExtractedWorld::GameBlock middle{11, 12, 13, 0, 1, 0}, after{21, 22, 23, 0, 1, 0};
        blocks.setBlockChanges(0, 0, {zonestates::blockChange(0, 0, 0, was, middle),
                                    zonestates::blockChange(0, 0, 0, middle, after)});
        for (Key key : expected) mirror.zoneAnswered({key.first, key.second});
        mirror.undoBlockChanges(blocks);
        check(blocks.gameBlock(0, 0, 0) == was && keys(mirror.zonesNeedingState(0, 0)) == expected,
              "reset reverses loaded edit history and flags edited and unedited sources together");
    }
    const auto owned = fs::weakly_canonical(directory);
    if (owned.parent_path() != fs::weakly_canonical(fs::temp_directory_path()) ||
        owned.filename().string().rfind("CubeVulkan-source-state-", 0) != 0) return failures + 1;
    fs::remove_all(owned);
    std::printf("Physical source state control: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
