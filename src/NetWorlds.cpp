#include "NetWorlds.h"
#include "SteamZoneDemand.h"

#include <algorithm>
#include <cstring>

// --- The host world ----------------------------------------------------------------------------

HostSide::HostSide(SpawnedCreatures &world, ExtractedWorld &blocks) : world(world), blocks(blocks) {
    blocks.setHostStreaming(true);
    ownsSourceWorker = blocks.hostSourceStreaming();
}
HostSide::~HostSide() { stopSources(); }
void HostSide::stopSources() {
    if (ownsSourceWorker) {
        blocks.setHostStreaming(false);
        ownsSourceWorker = false;
    }
}

void HostSide::creatures(std::vector<cwnet::CreatureView> &out) {
    out.clear();
    world.forEach([&](const uint8_t *record) {
        int64_t id;
        std::memcpy(&id, record + cwnet::creature::ID, sizeof id);
        out.push_back({id, record});
    });
}

void HostSide::receiveCreature(const cwnet::CreatureDelta &delta) {
    auto guard = blocks.lockSourceState();
    bool made = false;
    if (uint8_t *record = world.networkCreature(delta.id, made)) {
        cwnet::applyDelta(delta, record);
        world.receivedUpdate(delta.id, made);
        if (!delta.cut) {
            auto &fields = playerFields[delta.id];
            fields |= delta.mask;
            constexpr uint64_t REQUIRED = (1ull << 0) | (1ull << 7) | (1ull << 21); // position, hostility, level
            if ((fields & REQUIRED) == REQUIRED && record[cwnet::creature::HOSTILITY] == 0 && playerObserved)
                playerObserved(record);
        }
    }
}

void HostSide::removeCreature(int64_t id) {
    if (playerLeft)
        playerLeft(id);
    playerFields.erase(id);
    world.remove(id);
}

void HostSide::advance(uint32_t dtMs) {
    auto sourceGuard = blocks.lockSourceState();
    // The native world advances in 16-ms slices, including its spawn and reset passes.
    // Reset notices remain queued until HostGame distributes the whole batch.
    const uint32_t frameMs = std::min(dtMs, uint32_t(1000));
    const glm::vec3 at = standInPosition();
    uint32_t left = frameMs;
    do {
        const uint32_t step = std::min(left, uint32_t(16));
        left -= step;
        // The existing AI timer remains separate from the World-step phases.
        if (!separateAi)
            world.think(blocks, (double)step, world.timeOfDay() / 3600000.0f);
        world.simulate(blocks, (double)step, reset, [&] { world.update(blocks, at); });
        std::vector<SpawnedCreatures::Death> died = world.takeDeaths(blocks);
        if (deaths && !died.empty())
            deaths(died);
    } while (left);
    // Native host f693e draws once with the full capped interval, after all substeps.
    world.smooth((float)frameMs);
    world.zoneChangeLists();
    // Host audio and other engine-local consumer queues are not presented here.
    world.takeSounds();
    world.takeDrops();
    world.takeTames();
    world.takeInnNights();
}

// The AI thread's pass (host-threading contract 3.1): the elapsed real time since the last.
void HostSide::think(uint32_t elapsedMs) {
    auto sourceGuard = blocks.lockSourceState();
    world.think(blocks, (double)elapsedMs, world.timeOfDay() / 3600000.0f);
}

// --- The client world --------------------------------------------------------------------------

void MirrorSide::applyBlockChanges(const std::vector<cwnet::BlockChangeRecord> &changes) {
    auto edits = blocks.editBlocks();
    for (const auto &change : changes)
        world.applyBlockChange(change);
}

bool MirrorSide::localSnapshot(cwnet::CreatureSnapshot &out) {
    const uint8_t *record = world.localRecord();
    if (!record)
        return false;
    out = cwnet::CreatureSnapshot::of(record);
    return true;
}

void MirrorSide::receiveCreature(const cwnet::CreatureDelta &delta) {
    bool made = false;
    if (uint8_t *record = world.networkCreature(delta.id, made)) {
        cwnet::applyDelta(delta, record);
        world.receivedUpdate(delta.id, made);
    }
}

void MirrorSide::startOver() {
    world.keepOnly(nullptr);
    world.undoBlockChanges(blocks);
    if (startedOver)
        startedOver();
}

void MirrorSide::dayChange(int32_t newDay) {
    world.resetWorldState(); // packet8 resets retained metadata before its new scalar stores
    if (dayChanged)
        dayChanged(newDay);
}

std::vector<cwnet::ZoneKey> MirrorSide::neededZones() {
    const uint8_t *me = world.localRecord();
    if (!me || !world.remoteClient())
        return {};
    std::array<int64_t, 3> at;
    std::memcpy(at.data(), me + cwnet::creature::POSITION, sizeof at);
    std::optional<zonedemand::BlockPoint> anchor;
    if (me[0x68] == 0x9c) {
        zonedemand::BlockPoint destination;
        std::memcpy(destination.data(), me + 0x1b0, sizeof destination);
        anchor = destination;
    }
    const auto point = zonedemand::sourcePoint(at, anchor);
    return world.zonesNeedingState(point[0], point[1]);
}
