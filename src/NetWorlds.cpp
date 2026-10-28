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
    }
}

void HostSide::advance(uint32_t dtMs) {
    // The zones near every player, the AI (its own thread in the game, every 40 ms), the world's
    // 16-ms steps, the clock, the drawn positions and the lists of the zones that changed.
    world.update(blocks, standIn);
    world.think(blocks, (double)dtMs, world.timeOfDay() / 3600000.0f);
    world.simulate(blocks, (double)dtMs);
    world.advanceClock((double)dtMs);
    world.smooth((float)dtMs);
    world.zoneChangeLists();
    // Nothing hears or draws the host world: what it keeps for that is let go.
    world.takeSounds();
    world.takeDrops();
    world.takeTames();
    world.takeInnNights();
    std::vector<SpawnedCreatures::Death> died = world.takeDeaths(blocks);
    if (deaths && !died.empty())
        deaths(died);
    // A night slept (a sleep notice of 0) resets the world (native-saves.md §5.6).
    const std::vector<int32_t> &sleeps = world.events().sleeps;
    if (std::find(sleeps.begin(), sleeps.end(), 0) != sleeps.end()) {
        world.worldReset();
        if (reset)
            reset();
    }
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
