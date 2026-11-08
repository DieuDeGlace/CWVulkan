#include "Extracted.h"
#include "NetWorlds.h"
#include "SourceStates.h"
#include "Spawned.h"
#include "ZoneStates.h"
#include "net/ClientGame.h"
#include "worldgen/Generator.h"
#include <functional>
#include <cstdio>

namespace {
nativesaves::Record marker(uint8_t value) { nativesaves::Record out{}; out[0] = value; return out; }
void factoryRole(const auto &check) {
    const nativesaves::Cell key{7, 8};
    nativesaves::WorldState saved;
    auto &state = saved.zones[key];
    state.flags = 3;
    state.groundItems = {marker(70)};
    state.staticObjects = {marker(80)};
    state.blockChanges = {zonestates::blockChange(7 * 64, 8 * 64, 0, {}, {1, 2, 3, 4, 5, 6})};
    state.placeCounters[{2, 3}] = 9; state.deadTracked = 71;
    ExtractedWorld blocks;
    blocks.openGenerated(0, 0);
    SpawnedCreatures actor;
    actor.setEnemyRespawn({enemyrespawn::Mode::Permanent, 300});
    actor.setStoredZones(blocks, &saved, false);
    actor.setClock(17, 1234); actor.markDead(999);
    const auto store = blocks.sourceStateStore();
    auto live = store->make(key, 1, {marker(1)}, {marker(2)}, {});
    store->install(live);
    check(live->lists.groundItems == state.groundItems && live->lists.staticObjects == state.staticObjects &&
          live->lists.blockChanges.size() == 1 && live->lists.placeCounters == state.placeCounters && live->lists.deadTracked == 71,
          "role-zero source factory initially restores all five retained fields");
    const auto before = nativesaves::writeWorld(store->snapshot());
    actor.setRemoteClient(true);
    check(before == nativesaves::writeWorld(store->snapshot()) && store->find(key) == live &&
          live->lists.groundItemsChanged && live->lists.groundItems == state.groundItems,
          "role transition preserves retained metadata and already loaded source lifetime/payload");
    auto fresh = store->make(key, 2, {marker(1)}, {marker(2)}, {});
    check(fresh->lists.groundItems == std::vector<nativesaves::Record>{marker(1)} &&
          fresh->lists.staticObjects == std::vector<nativesaves::Record>{marker(2)} && fresh->lists.blockChanges.empty() &&
          fresh->lists.placeCounters.empty() && fresh->lists.deadTracked == 0 && !fresh->lists.groundItemsChanged &&
          !fresh->lists.staticObjectsChanged,
          "live nonzero role skips all five retained fields at fresh physical factory admission");
    actor.setRemoteClient(false);
    auto restored = store->make(key, 3, {}, {}, {});
    check(restored->lists.groundItems == state.groundItems && restored->lists.deadTracked == 71,
          "returning to zero role restores retained state again without replacing the archive");
    ExtractedWorld second;
    second.openGenerated(0, 0);
    SpawnedCreatures alreadyNonzero;
    alreadyNonzero.setRemoteClient(true);
    alreadyNonzero.setStoredZones(second, &saved, false);
    auto initial = second.sourceStateStore()->make(key, 4, {marker(1)}, {}, {});
    check(initial->lists.groundItems == std::vector<nativesaves::Record>{marker(1)} && initial->lists.deadTracked == 0,
          "binding an archive after selecting nonzero role does not reopen restoration");
    alreadyNonzero.setStoredZones(second, &saved, true);
    alreadyNonzero.setRemoteClient(false);
    auto overridden = second.sourceStateStore()->make(key, 5, {marker(1)}, {}, {});
    check(overridden->lists.groundItems == std::vector<nativesaves::Record>{marker(1)} && overridden->lists.deadTracked == 0,
          "explicit compatibility override still suppresses restoration after a role-zero transition");
    actor.setRemoteClient(true);
    unsigned notices = 0;
    store->queueGround(key); store->queueObjects(key);
    actor.setRemoteClient(false); actor.setRemoteClient(true);
    store->publish([&](auto, const auto &) { ++notices; }, [&](auto, const auto &) { ++notices; }, {});
    check(notices == 2 && store->find(key) == live && live->lists.deadTracked == 71,
          "role changes preserve loaded payloads and pending dirty-source publication");
}
void physicalReload(const auto &check) {
    const nativesaves::Cell key{0, 0}, gate{13, -7};
    const auto raw = worldgen::zone(0, 0);
    const auto &column = raw->terrain.columns[worldgen::ZoneTerrain::index(6, 5)];
    check(!column.blocks.empty(), "actual factory test has a valid raw column");
    if (column.blocks.empty()) return;
    const int32_t z = column.base + int32_t(column.blocks.size()) - 1;
    const auto &base = column.blocks.back();
    const ExtractedWorld::GameBlock original{base.r, base.g, base.b, base.glow, base.type, base.flags};
    const ExtractedWorld::GameBlock changed{20, 30, 40, 0, 1, 0};
    nativesaves::WorldState saved;
    auto &state = saved.zones[key];
    state.flags = 3; state.groundItems = {marker(70)}; state.staticObjects = {marker(80)};
    state.blockChanges = {zonestates::blockChange(6, 5, z, original, changed)};
    state.placeCounters[gate] = 9; state.deadTracked = 71;
    ExtractedWorld blocks;
    blocks.openGenerated(0, 0); blocks.setClientStreaming(true);
    SpawnedCreatures actor;
    actor.setEnemyRespawn({enemyrespawn::Mode::Permanent, 300});
    actor.setStoredZones(blocks, &saved, false);
    actor.setRemoteClient(true);
    check(blocks.loadSourceZone(0, 0), "actual role-nonzero source factory loads raw terrain");
    const auto store = blocks.sourceStateStore();
    auto first = store->find(key);
    auto lease = blocks.gameZoneToken(0, 0);
    check(first && lease, "reader publishes source and terrain with matching owned lifetime");
    if (!first || !lease) return;
    check(!first->lists.groundItemsChanged && !first->lists.staticObjectsChanged && first->lists.blockChanges.empty() &&
          first->lists.placeCounters.empty() && first->lists.deadTracked == 0 && blocks.gameBlock(6, 5, z) == original,
          "actual nonzero-role factory skips arrays, ordered edit replay, counters and scalar");
    check(blocks.setGameBlock(6, 5, z, changed), "actual COW mutation succeeds");
    actor.setRemoteClient(false);
    check(blocks.loadSourceZone(0, 0) && store->find(key) == first && blocks.gameZoneToken(0, 0)->order == lease->order &&
          blocks.gameBlock(6, 5, z) == changed && !first->lists.groundItemsChanged,
          "role-zero loaded hit preserves existing COW identity and does not run restoration again");
    first->lists.groundItemsChanged = first->lists.staticObjectsChanged = true;
    first->lists.groundItems = {marker(90)}; first->lists.staticObjects = {marker(91)};
    first->lists.blockChanges = state.blockChanges; first->lists.placeCounters[gate] = 12; first->lists.deadTracked = 42;
    actor.setRemoteClient(true);
    check(blocks.retireClientSource({1000, 1000}, 1) && !store->find(key), "actual retirement ends loaded membership in nonzero role");
    const auto retained = store->snapshot().zones.at(key);
    check(retained.flags == 3 && retained.groundItems == first->lists.groundItems &&
          retained.staticObjects == first->lists.staticObjects && retained.blockChanges.size() == 1 &&
          retained.placeCounters.at(gate) == 12 && retained.deadTracked == 42,
          "nonzero role never suppresses retention of all five live fields");
    actor.setRemoteClient(false);
    check(blocks.loadSourceZone(0, 0), "actual zero-role reload succeeds");
    auto restored = store->find(key);
    check(restored && restored != first && restored->lists.groundItemsChanged && restored->lists.staticObjectsChanged &&
          restored->lists.groundItems == retained.groundItems && restored->lists.staticObjects == retained.staticObjects &&
          restored->lists.blockChanges.size() == 1 && restored->lists.placeCounters.at(gate) == 12 &&
          restored->lists.deadTracked == 42 && blocks.gameBlock(6, 5, z) == changed,
          "new zero-role source restores all five retained fields and replays edits before publication");
}
struct ObservedTransport : cwnet::Transport {
    cwnet::Transport &inner;
    std::function<void(cwnet::PeerId)> joining;
    explicit ObservedTransport(cwnet::Transport &inner) : inner(inner) {}
    cwnet::PeerId self() const override { return inner.self(); }
    bool valid(cwnet::PeerId peer) const override { return inner.valid(peer); }
    bool send(cwnet::PeerId to, cwnet::Channel channel, const cwnet::Bytes &bytes) override {
        if (channel == cwnet::Channel::ToHost && std::holds_alternative<cwnet::Join>(cwnet::decode(bytes, cwnet::Direction::ToHost)) && joining)
            joining(to);
        return inner.send(to, channel, bytes);
    }
    bool receive(cwnet::Channel channel, cwnet::Received &out) override { return inner.receive(channel, out); }
};
struct ObservedMirror : MirrorSide {
    using MirrorSide::MirrorSide;
    std::function<void()> resetting, publishingRole;
    void startOver() override { if (resetting) resetting(); MirrorSide::startOver(); }
    void connected() override { if (publishingRole) publishingRole(); MirrorSide::connected(); }
};
void connections(const auto &check) {
    cwnet::LoopbackNetwork network;
    auto local = network.endpoint(1), remote = network.endpoint(2);
    ObservedTransport transport(*local);
    ExtractedWorld blocks;
    blocks.openGenerated(0, 0);
    SpawnedCreatures viewed;
    nativesaves::WorldState retained;
    retained.zones[{7, 8}].deadTracked = 44;
    viewed.setStoredZones(blocks, &retained, false);
    const auto archive = nativesaves::writeWorld(blocks.sourceStateStore()->snapshot());
    ObservedMirror mirror(viewed, blocks);
    std::vector<std::string> order;
    cwnet::ClientGame client(transport, mirror, [&] { order.push_back("host-reset"); });
    cwnet::PeerId destination = 1;
    mirror.resetting = [&] {
        order.push_back("mirror-reset");
        check(!viewed.remoteClient() && client.link().host() == 0, "reset sees the old role and temporarily cleared selected peer");
    };
    transport.joining = [&](cwnet::PeerId to) {
        order.push_back("join");
        check(to == destination && !viewed.remoteClient() && client.link().host() == 0,
              "Join attempt precedes destination publication and the final World role write");
    };
    mirror.publishingRole = [&] {
        order.push_back("role");
        check(!viewed.remoteClient() && client.link().host() == destination,
              "role callback sees the published destination while the old World role is still present");
    };
    const auto completed = [&] {
        check(order == std::vector<std::string>{"mirror-reset", "host-reset", "join", "role"},
              "shared connect keeps reset, own-host reset, Join and final role publication in order");
        check(archive == nativesaves::writeWorld(blocks.sourceStateStore()->snapshot()),
              "connecting does not replace the retained source map or calendar/identifier metadata");
        order.clear();
    };
    check(!viewed.remoteClient(), "constructed World begins in role zero");
    client.link().goHome(100);
    check(viewed.remoteClient() && client.link().host() == 1, "ordinary local connection sets the viewed World role to one");
    completed();
    viewed.setRemoteClient(false); destination = 2;
    client.link().connectTo(2, 101);
    check(viewed.remoteClient() && client.link().host() == 2, "remote connection uses the same final viewed role");
    completed();
    viewed.setRemoteClient(false); destination = 1;
    cwnet::WorldEvents pending;
    cwnet::KillRecord death{};
    cwnet::setField(death, 8, int64_t(99));
    pending.kills.push_back(death);
    remote->send(1, cwnet::Channel::ToClient, cwnet::encode(cwnet::WorldUpdate{pending}, cwnet::Direction::ToClient));
    remote->send(1, cwnet::Channel::ToClient, cwnet::encode(cwnet::Leave{}, cwnet::Direction::ToClient));
    client.receive(102);
    check(viewed.remoteClient() && client.link().host() == 1 && client.wentHome,
          "current-peer Leave fallback sets role one again on the shared local route");
    completed();
    client.consumeWorldUpdates();
    check(viewed.isWorldDead(99), "role/fallback transition preserves packet7 pending category records for later consumption");
}
}
int connectionRoleTests() {
    int failures = 0;
    const auto check = [&](bool result, const char *message) {
        if (!result) { std::printf("FAIL connection role: %s\n", message); ++failures; }
    };
    factoryRole(check); physicalReload(check); connections(check);
    std::printf("Connection role and source restore gates: %d failure(s)\n", failures);
    return failures;
}
