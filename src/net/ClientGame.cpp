#include "net/ClientGame.h"

namespace cwnet {

namespace {
constexpr size_t DAMAGE_TARGET = 0x08;   // i64 in a damage number
constexpr size_t DAMAGE_AMOUNT = 0x10;   // f32
constexpr size_t BUFF_TARGET = 0x08;     // i64 in a buff-change notice
constexpr size_t KILL_DEAD = 0x08;       // i64 in a kill credit: the dead creature
constexpr size_t FEAT_CREATURE = 0x00;   // i64 in a feat notice
} // namespace

ClientGame::ClientGame(Transport &transport, MirrorWorld &world, std::function<void()> ownHostDropAll)
    : world(world), guestLink(transport, GuestLink::World{[&world] { world.startOver(); }, std::move(ownHostDropAll)}) {}

void ClientGame::receive(uint32_t nowMs) {
    GuestLink::Inbox inbox;
    guestLink.receive(nowMs, inbox);
    if (inbox.givenId)
        world.setLocalId(*inbox.givenId);
    // Creature records and packet 6s in the order they came.
    for (const auto &[isUpdate, index] : inbox.creatureOrder)
        if (isUpdate)
            world.receiveCreature(inbox.creatureUpdates[index]);
        else
            world.keepOnly(inbox.creatureLists[index]);
    for (const Time &t : inbox.times) {
        if (t.day != world.day())
            world.dayChange(t.day);
        world.setClock(t.day, t.timeOfDay);
    }
    for (WorldEvents &events : inbox.worldUpdates)
        received.push_back(std::move(events));
    for (ChatLine &line : inbox.chat)
        chat.push_back(std::move(line));
    invitations.insert(invitations.end(), inbox.invitations.begin(), inbox.invitations.end());
    wentHome = wentHome || inbox.wentHome;
    if (extra)
        for (const CubeVulkanMessage &m : inbox.extras)
            extra(m);
}

void ClientGame::consumeWorldUpdates() {
    const int64_t me = guestLink.localId();
    for (const WorldEvents &e : received) {
        // 1. Blocks, 2. zones answered, 3. ground items and 4. static objects by zone.
        for (const BlockChangeRecord &b : e.blockChanges)
            world.applyBlockChange(b);
        for (const ZoneKey &zone : e.zonesAnswered)
            world.zoneAnswered(zone);
        for (const ZoneObjects &zone : e.groundItems)
            world.replaceGroundItems(zone);
        for (const ZoneObjects &zone : e.staticObjects)
            world.replaceStaticObjects(zone);
        // 5. Buffs, except one the local player gave itself (it has it already).
        for (const BuffRecord &b : e.buffs)
            if (!(field<int64_t>(b, buffnote::SOURCE) == me && field<int64_t>(b, BUFF_TARGET) == me))
                world.giveBuff(b);
        // 6. Aggro, 7. the world's own zone-change step, 8. the dead.
        for (const AggroRecord &a : e.aggro)
            world.setAggro(a);
        world.zoneChangeStep();
        for (const KillRecord &k : e.kills)
            world.markDead(field<int64_t>(k, KILL_DEAD));
        // 9. The lists the world's consumers play, show or act on.
        world.keepEvents(e);
        // 10. Feats of the local player.
        for (const FeatRecord &f : e.feats)
            if (field<int64_t>(f, FEAT_CREATURE) == me)
                world.feat(f);
        // 11. Hits: deaths seen, the local player's own hits taken, others' flashes.
        for (const DamageRecord &d : e.damageNumbers) {
            const int64_t target = field<int64_t>(d, DAMAGE_TARGET);
            if (!world.hasCreature(target))
                continue;
            if (world.isDead(target))
                world.deathSeen(target);
            if (target == me)
                world.hitLocalPlayer(d);
            else if (field<float>(d, DAMAGE_AMOUNT) > 0.0f && field<int64_t>(d, damage::ATTACKER) != me)
                world.flash(target);
        }
        // 12. Others' effect objects, 13. others' damage numbers to show.
        for (const EffectRecord &x : e.effects)
            if (field<int64_t>(x, effect::OWNER) != me)
                world.addEffect(x);
        for (const DamageRecord &d : e.damageNumbers)
            if (field<int64_t>(d, damage::ATTACKER) != me)
                world.showDamageNumber(d);
    }
    received.clear();
}

void ClientGame::send(uint32_t nowMs) {
    // What the local player caused this frame (0x3cc40).
    guestLink.queueOwnEvents(world.ownEvents());
    if (guestLink.checkTimers(nowMs))
        wentHome = true;
    guestLink.outbox.neededZones = world.neededZones();
    if (guestLink.outbox.interactions.empty()) {
        InteractionRecord record;
        if (world.takeInteraction(record))
            guestLink.outbox.interactions.push_back(record);
    }
    CreatureSnapshot me;
    if (world.localSnapshot(me))
        guestLink.sendTick(nowMs, me);
}

} // namespace cwnet
