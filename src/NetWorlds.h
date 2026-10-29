#pragma once
// CubeVulkan's two worlds as the network sees them (net/Worlds.h, docs/specs/netcode-roles.md):
//
//   * HostSide: a SpawnedCreatures world that decides (the host world). Every player in it is a
//     guest's creature, the one at this machine included; it spawns near all of them, runs the AI
//     and performs everyone's interactions, and keeps Save/world.db's zone states.
//   * MirrorSide: a SpawnedCreatures world in mirror mode (the client world), with the local
//     player. It is drawn, played and heard; its creatures, zone states and clock come from
//     whichever host it is connected to.
//
// Both stand in the same block store (ExtractedWorld): the terrain is the same generator's on
// every machine, and block changes are put in by whichever world's zone log changes them.
//
// What the game frame does with what arrives (shown hits, pick-ups, kill credits, feats, a new
// day) is left to the caller through the hooks, so the engine's own systems (the interface,
// progression, mods) take them up where they belong.
#include "Spawned.h"
#include "net/Worlds.h"

#include <functional>
#include <vector>

class HostSide : public cwnet::HostWorld {
  public:
    HostSide(SpawnedCreatures &world, ExtractedWorld &blocks) : world(world), blocks(blocks) {}

    // Where the zones are kept before any player has joined (local blocks).
    glm::vec3 standIn{0};
    // Told after each tick's world step: what died (for the dead set and the save) and the world
    // resets (a night slept).
    std::function<void(const std::vector<SpawnedCreatures::Death> &)> deaths;
    std::function<void()> reset;

    void creatures(std::vector<cwnet::CreatureView> &out) override;
    bool hasCreature(int64_t id) override { return world.hasCreature(id); }
    void receiveCreature(const cwnet::CreatureDelta &delta) override;
    void removeCreature(int64_t id) override { world.remove(id); }
    void queueInteraction(int64_t creatureId, const cwnet::InteractionRecord &record) override {
        world.queueInteraction(creatureId, record);
    }
    void applyGuestEvents(const cwnet::GuestEvents &events) override { world.applyGuestEvents(blocks, events); }
    void advance(uint32_t dtMs) override;
    void advanceTimers(uint32_t elapsedMs) override { world.advanceRespawns(elapsedMs); }
    cwnet::WorldEvents &events() override { return world.events(); }
    bool zoneAnswer(const cwnet::ZoneKey &zone, cwnet::ZoneAnswer &out) override { return world.zoneAnswer(blocks, zone, out); }
    cwnet::Time clock() override { return {world.day(), (int32_t)world.timeOfDay()}; }

  private:
    SpawnedCreatures &world;
    ExtractedWorld &blocks;
};

class MirrorSide : public cwnet::MirrorWorld {
  public:
    MirrorSide(SpawnedCreatures &world, ExtractedWorld &blocks) : world(world), blocks(blocks) {}

    // Hooks for the game frame.
    std::function<void(int32_t newDay)> dayChanged;                // after the world's side of the day change
    std::function<void(const cwnet::FeatRecord &)> featDone;       // "FEAT!" for the local player
    std::function<void(const cwnet::DamageRecord &)> damageShown;  // a hit someone else made, to show
    std::function<void()> startedOver;                             // connected to a host: the mirror is empty

    void setLocalId(int64_t id) override { world.setPlayerId(id); }
    bool localSnapshot(cwnet::CreatureSnapshot &out) override;
    bool takeInteraction(cwnet::InteractionRecord &out) override { return world.takePlayerInteraction(out); }
    void receiveCreature(const cwnet::CreatureDelta &delta) override;
    void keepOnly(const std::set<int64_t> &heard) override { world.keepOnly(&heard); }
    void startOver() override;
    int32_t day() override { return world.day(); }
    void dayChange(int32_t newDay) override;
    void setClock(int32_t day, int32_t timeOfDayMs) override { world.setClock(day, (float)timeOfDayMs); }
    void applyBlockChange(const cwnet::BlockChangeRecord &change) override { world.applyBlockChange(change); }
    void zoneAnswered(const cwnet::ZoneKey &zone) override { world.zoneAnswered(zone); }
    void replaceGroundItems(const cwnet::ZoneObjects &zone) override { world.replaceGroundItems(blocks, zone); }
    void replaceStaticObjects(const cwnet::ZoneObjects &zone) override { world.replaceStaticObjects(blocks, zone); }
    void giveBuff(const cwnet::BuffRecord &notice) override { world.giveBuffNotice(notice); }
    void setAggro(const cwnet::AggroRecord &notice) override { world.setAggro(notice); }
    void zoneChangeStep() override { world.zoneChangeLists(); }
    void markDead(int64_t id) override { world.markDead(id); }
    void keepEvents(const cwnet::WorldEvents &received) override { world.keepReceived(received); }
    void feat(const cwnet::FeatRecord &f) override {
        if (featDone)
            featDone(f);
    }
    bool hasCreature(int64_t id) override { return world.hasCreature(id); }
    bool isDead(int64_t id) override { return world.isDead(id); }
    void hitLocalPlayer(const cwnet::DamageRecord &number) override {
        world.hitPlayer(number);
        hitsTaken++;
    }
    // For the log: the host's hits on the local player applied so far.
    size_t hitsTaken = 0;
    void flash(int64_t id) override { world.flash(id); }
    void addEffect(const cwnet::EffectRecord &effect) override { world.addEffect(effect); }
    void showDamageNumber(const cwnet::DamageRecord &number) override {
        if (damageShown)
            damageShown(number);
    }
    const cwnet::WorldEvents &ownEvents() override { return world.events(); }
    std::vector<cwnet::ZoneKey> neededZones() override;

  private:
    SpawnedCreatures &world;
    ExtractedWorld &blocks;
};
