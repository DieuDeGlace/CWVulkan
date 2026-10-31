#include "NetWorlds.h"

#include <algorithm>
#include <cstring>

// --- The host world ----------------------------------------------------------------------------

void HostSide::creatures(std::vector<cwnet::CreatureView> &out) {
    out.clear();
    world.forEach([&](const uint8_t *record) {
        int64_t id;
        std::memcpy(&id, record + cwnet::creature::ID, sizeof id);
        out.push_back({id, record});
    });
}

void HostSide::receiveCreature(const cwnet::CreatureDelta &delta) {
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
    // The native world advances in 16-ms slices, including its spawn and reset passes.
    // Reset notices remain queued until HostGame distributes the whole batch.
    const uint32_t frameMs = std::min(dtMs, uint32_t(1000));
    uint32_t left = frameMs;
    do {
        const uint32_t step = std::min(left, uint32_t(16));
        left -= step;
        world.advanceClock((double)step);
        world.update(blocks, standIn);
        world.think(blocks, (double)step, world.timeOfDay() / 3600000.0f);
        world.simulate(blocks, (double)step, reset);
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

// --- The client world --------------------------------------------------------------------------

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
    world.clearDead(); // the world's side of 0x2c34c0: its dead set (a mirror keeps no zone states)
    if (dayChanged)
        dayChanged(newDay);
}

std::vector<cwnet::ZoneKey> MirrorSide::neededZones() {
    const uint8_t *me = world.localRecord();
    if (!me)
        return {};
    int64_t at[2];
    std::memcpy(at, me + cwnet::creature::POSITION, sizeof at);
    // The player's block (while flying on the eagle, ability 0x9c, the pair at +0x1b0 instead).
    constexpr uint8_t EAGLE_FLIGHT = 0x9c;
    constexpr size_t ABILITY = 0x68, FLIGHT_BLOCK = 0x1b0;
    int64_t blockX = at[0] >> 16, blockY = at[1] >> 16;
    if (me[ABILITY] == EAGLE_FLIGHT) {
        int32_t pair[2];
        std::memcpy(pair, me + FLIGHT_BLOCK, sizeof pair);
        blockX = pair[0], blockY = pair[1];
    }
    return world.zonesNeedingState(blockX, blockY);
}
