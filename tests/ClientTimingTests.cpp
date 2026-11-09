#include "net/ClientGame.h"
#include <cstdio>
#include <deque>
#include <vector>
#include <string>

namespace {
using namespace cwnet;
struct TransportLog : Transport {
    struct Sent { PeerId to; Channel channel; Bytes bytes; };
    std::deque<Received> incoming;
    std::vector<Sent> sent;
    std::function<void(const Bytes &)> afterSend;
    PeerId self() const override { return 1; }
    bool valid(PeerId peer) const override { return peer != 0; }
    bool send(PeerId to, Channel channel, const Bytes &bytes) override {
        sent.push_back({to,channel,bytes});
        if (afterSend) afterSend(bytes);
        return true;
    }
    bool receive(Channel channel, Received &out) override {
        if (channel != Channel::ToClient || incoming.empty()) return false;
        out = std::move(incoming.front()); incoming.pop_front(); return true;
    }
    std::vector<uint32_t> kinds() const {
        std::vector<uint32_t> result;
        for (const auto &packet : sent) {
            if (packet.bytes.size() < 4) continue;
            const auto &b = packet.bytes;
            result.push_back(uint32_t(b[0]) | (uint32_t(b[1]) << 8) |
                             (uint32_t(b[2]) << 16) | (uint32_t(b[3]) << 24));
        }
        return result;
    }
};
struct MirrorLog : MirrorWorld {
    unsigned snapshots = 0, pulls = 0;
    WorldEvents events;
    void setLocalId(int64_t) override {}
    bool localSnapshot(CreatureSnapshot &out) override { ++snapshots; out = {}; return true; }
    bool takeInteraction(InteractionRecord &out) override { ++pulls; out = {}; return true; }
    void receiveCreature(const CreatureDelta &) override {}
    void keepOnly(const std::set<int64_t> &) override {}
    void startOver() override {}
    int32_t day() override { return 1; }
    void dayChange(int32_t) override {}
    void setClock(int32_t,int32_t) override {}
    const WorldEvents &ownEvents() override { return events; }
    std::vector<ZoneKey> neededZones() override { return {}; }
};
void unselectedFallback(const auto &check) {
    TransportLog transport;
    GuestLink guest(transport,{});
    check(!guest.checkTimers(5000), "constructor literal zero retains the exact 5000 boundary");
    check(guest.checkTimers(5001) && guest.host() == 1,
          "fallback has no selected-peer validity admission, even before first connect");
}
void fallbackPrecedesExpiry(const auto &check) {
    TransportLog transport;
    GuestLink guest(transport,{});
    guest.connectTo(3,0);
    transport.incoming.push_back({2,encode(Invitation{GAME_VERSION},Direction::ToClient)});
    GuestLink::Inbox inbox;
    guest.receive(0,inbox);
    check(guest.hasInvitation(2), "a wrong-peer invitation creates its own node clock");
    check(guest.checkTimers(6001), "an overdue selected peer takes the shared local transition");
    check(guest.hasInvitation(2), "fallback bypasses the later invitation expiry scan");
    guest.checkTimers(6002);
    check(!guest.hasInvitation(2), "the next nonfallback drain can expire that retained invitation");
}
void sendClockAdmission(const auto &check) {
    CreatureSnapshot player{};
    TransportLog transport;
    GuestLink guest(transport,{});
    guest.connectTo(2,0);
    guest.checkTimers(0);
    guest.checkTimers(32);
    check(!guest.sendTick(32,player), "initial global send time excludes exact32 rather than sending immediately");
    guest.checkTimers(33);
    check(guest.sendTick(33,player), "the first ordinary send is admitted strictly beyond32");

    TransportLog otherTransport;
    GuestLink unselected(otherTransport,{});
    unselected.checkTimers(0);
    unselected.checkTimers(33);
    check(!unselected.sendTick(33,player), "an invalid selected peer still emits no ordinary packet");
    unselected.connectTo(2,33);
    unselected.checkTimers(34);
    check(!unselected.sendTick(34,player), "the send clock advances before the selected-peer predicate");
}
void fallbackSkipsOrdinaryWork(const auto &check) {
    TransportLog transport;
    MirrorLog mirror;
    ClientGame client(transport,mirror);
    client.link().connectTo(2,0);
    client.link().outbox.chat.push_back(u"retained");
    client.link().outbox.neededZones = {{7,8}};
    transport.sent.clear();
    client.send(5001);
    check(client.wentHome && client.link().host() == 1 && transport.kinds() == std::vector<uint32_t>{2,1},
          "fallback sends only its shared-connect leave/join, bypassing ordinary output");
    check(mirror.snapshots == 0 && mirror.pulls == 0,
          "fallback does not read a player snapshot or consume an interaction");
    check(client.link().outbox.chat.size() == 1,
          "fallback retains queued chat for a later ordinary send");
}

struct StepClock {
    uint32_t current = 0;
    std::deque<uint32_t> next;
    std::vector<uint32_t> samples;
    std::function<void(uint32_t)> probe;
    uint32_t read() {
        if (!next.empty()) { current = next.front(); next.pop_front(); }
        samples.push_back(current);
        if (probe) probe(current);
        return current;
    }
};
void freshPeerSamples(const auto &check) {
    TransportLog transport;
    StepClock clock;
    std::vector<std::string> trace;
    GuestLink *owner = nullptr;
    transport.afterSend = [&](const Bytes &bytes) { trace.push_back("s" + std::to_string(bytes[0])); };
    GuestLink guest(transport, {{},{},[&] { trace.push_back("role" + std::to_string(owner->host())); }},
        {[&] { trace.push_back("clock"); return clock.read(); },{},{}});
    owner = &guest;
    check(clock.samples.empty(), "construction stores literal zero without sampling the client clock");
    clock.next = {100};
    guest.connectTo(3,0);
    check(trace == std::vector<std::string>{"s1","clock","role3"},
          "connection samples after the request attempt and before selected-peer/role publication");
    clock.next = {200};
    transport.incoming.push_back({3,{0xe7,3,0,0}}); // complete unknown kind999
    GuestLink::Inbox inbox;
    guest.receive(0,inbox);
    const size_t reads = clock.samples.size();
    transport.incoming.push_back({4,{0xe7,3,0,0}});
    guest.receive(0,inbox);
    check(clock.samples.size() == reads, "a wrong-peer unknown kind takes no common timestamp sample");
    clock.next = {5199,5199,5200};
    check(!guest.checkTimers(0) && clock.next.empty(),
          "timeout uses its comparison reading after the distinct diagnostic/static readings");
    clock.next = {5200,5201,6000};
    check(guest.checkTimers(0) && guest.host() == 1 && clock.next.empty(),
          "a matching unknown kind refreshes before content admission; strict 5001 fallback samples its own connect");
    clock.next = {10999,11000};
    check(!guest.checkTimers(0), "fallback stores the new connection sample rather than the earlier comparison value");

    TransportLog invalidTransport;
    GuestLink invalid(invalidTransport,{});
    invalidTransport.incoming.push_back({0,{}}); // owned metadata, safely rejected absent kind
    invalid.receive(1000,inbox);
    check(!invalid.checkTimers(6000) && invalid.checkTimers(6001),
          "matching invalid identifier and short prefix still refresh before safe content rejection");
}
void invitationClockOrder(const auto &check) {
    for (bool matching : {false,true}) {
        TransportLog transport;
        StepClock clock;
        GuestLink guest(transport,{}, {[&] { return clock.read(); },{},{}});
        clock.next = {100}; guest.connectTo(3,0);
        const PeerId sender = matching ? 3 : 2;
        const uint32_t stamp = matching ? 301 : 200;
        bool nodeExistsAtSample = false;
        clock.probe = [&](uint32_t value) {
            if (value == stamp) nodeExistsAtSample = guest.hasInvitation(sender);
        };
        clock.next = matching ? std::deque<uint32_t>{300,301} : std::deque<uint32_t>{200};
        transport.incoming.push_back({sender,encode(Invitation{GAME_VERSION},Direction::ToClient)});
        GuestLink::Inbox inbox; guest.receive(0,inbox);
        const auto found = guest.invitations().find(sender);
        check(nodeExistsAtSample && found != guest.invitations().end() && found->second == stamp,
              "invitation insertion precedes its independent fresh node timestamp");
        check(clock.samples.size() == (matching ? 3u : 2u),
              "matching invitations sample peer and node separately; wrong-peer invitations sample only the node");
        const uint32_t baseline = matching ? 300 : 100;
        clock.next = {baseline+4999,baseline+4999,baseline+5000};
        check(!guest.checkTimers(0), "the node clock is not substituted for the peer timeout baseline");
        clock.next = {baseline+5000,baseline+5001,baseline+6000};
        check(guest.checkTimers(0), "invitation renewal does not move the peer boundary to its node sample");
    }
}
void leaveTakesTwoSamples(const auto &check) {
    TransportLog transport;
    StepClock clock;
    GuestLink guest(transport,{}, {[&] { return clock.read(); },{},{}});
    clock.next = {100}; guest.connectTo(3,0);
    clock.next = {200,300};
    transport.incoming.push_back({3,encode(Leave{},Direction::ToClient)});
    GuestLink::Inbox inbox; guest.receive(0,inbox);
    check(inbox.wentHome && guest.host() == 1 && clock.samples == std::vector<uint32_t>{100,200,300},
          "selected Leave takes the entry refresh then the later shared-connect sample");
    clock.next = {5299,5299,5300};
    check(!guest.checkTimers(0), "the later Leave connection sample supplies the exact 5000 boundary");
}
void batchDelay(const auto &check) {
    struct SlowMirror : MirrorLog {
        StepClock &clock;
        unsigned changes = 0;
        explicit SlowMirror(StepClock &clock) : clock(clock) {}
        void setClock(int32_t,int32_t) override { ++changes; clock.current = 6000; }
    };
    TransportLog transport;
    StepClock clock; clock.current = 100;
    std::vector<uint32_t> diagnostics;
    SlowMirror mirror(clock);
    GuestLink guest(transport,{}, {[&] { return clock.read(); },{},[&](uint32_t d) { diagnostics.push_back(d); }});
    guest.connectTo(3,0);
    transport.incoming.push_back({3,encode(Time{2,200},Direction::ToClient)});
    transport.incoming.push_back({3,{0xe7,3,0,0}});
    GuestLink::Inbox inbox; guest.receive(0,inbox,&mirror);
    clock.current = 6001;
    check(mirror.changes == 1 && !guest.checkTimers(0) && guest.host() == 3,
          "a later record refreshes after an expensive earlier handler before the complete-batch timeout check");
    CreatureSnapshot player{};
    guest.sendTick(0,player);
    check(diagnostics.empty(), "the duration diagnostic excludes earlier packet-handler work rather than budgeting the batch");
}
void requestClockSamples(const auto &check) {
    for (uint32_t elapsed : {1000u,1001u}) {
        TransportLog transport;
        StepClock clock;
        auto shared = std::make_shared<GuestLink::Clocks>();
        shared->send = 0; // a process whose first send-clock initialization already happened
        std::vector<uint32_t> diagnostics;
        GuestLink guest(transport,{}, {[&] { return clock.read(); },shared,[&](uint32_t d) { diagnostics.push_back(d); }});
        clock.next = {100}; guest.connectTo(3,0);
        guest.outbox.neededZones = {{7,8}};
        clock.next = {1000,1033};
        check(!guest.checkTimers(0), "request fixture retains a live selected peer");
        clock.next = {1040,1040+elapsed,2050};
        CreatureSnapshot player{};
        check(guest.sendTick(0,player) && clock.next.empty(),
              "request initialization and comparison use distinct fresh readings inside an admitted send");
        const auto kinds = transport.kinds();
        const auto expected = elapsed == 1000 ? std::vector<uint32_t>{1,3} : std::vector<uint32_t>{1,3,15};
        check(kinds == expected, "the fresh request comparison excludes 1000 and admits 1001 after its initializer");
        check(diagnostics == std::vector<uint32_t>{1050}, "a long duration is diagnostic and does not cancel completed outgoing work");
    }
}
void requestEmptyAndDurationBoundaries(const auto &check) {
    TransportLog transport;
    auto shared = std::make_shared<GuestLink::Clocks>();
    shared->send = 0; shared->requests = 0;
    GuestLink guest(transport,{}, {{},shared,{}});
    guest.connectTo(3,0);
    CreatureSnapshot player{};
    guest.checkTimers(1001); guest.sendTick(1001,player); // no needed zones
    guest.outbox.neededZones = {{7,8}};
    guest.checkTimers(1034); guest.sendTick(1034,player);
    guest.checkTimers(2001); guest.sendTick(2001,player);
    check(transport.kinds() == std::vector<uint32_t>{1,3,3,3},
          "an empty request round advances its clock before list emptiness and excludes exact1000 later");
    guest.checkTimers(2034); guest.sendTick(2034,player);
    check(transport.kinds() == std::vector<uint32_t>{1,3,3,3,3,15},
          "the empty-round baseline admits needed zones only after the later strict1000 boundary");

    TransportLog timedTransport;
    StepClock clock;
    auto timedClocks = std::make_shared<GuestLink::Clocks>();
    timedClocks->send = 0;
    std::vector<uint32_t> diagnostics;
    GuestLink timed(timedTransport,{}, {[&] { return clock.read(); },timedClocks,
        [&](uint32_t d) { diagnostics.push_back(d); }});
    timed.connectTo(3,0);
    clock.next = {100,100,100,100,108};
    timed.checkTimers(0); timed.sendTick(0,player);
    check(diagnostics.empty() && clock.next.empty(), "duration equality8 emits no slow-drain diagnostic");
    clock.next = {200,200,200,209};
    timed.checkTimers(0); timed.sendTick(0,player);
    check(diagnostics == std::vector<uint32_t>{9} && clock.next.empty(),
          "duration9 emits a diagnostic after ordinary output without cancelling it");
}
void sharedClockLifetimes(const auto &check) {
    auto shared = std::make_shared<GuestLink::Clocks>();
    CreatureSnapshot player{};
    {
        TransportLog transport;
        GuestLink first(transport,{}, {{},shared,{}});
        first.connectTo(2,0);
        first.checkTimers(0);
        first.checkTimers(33);
        check(first.sendTick(33,player), "the first client advances shared send and initializes request time");
    }
    TransportLog transport;
    GuestLink second(transport,{}, {{},shared,{}});
    second.connectTo(3,34);
    second.checkTimers(34);
    check(!second.sendTick(34,player), "a new client retains the process send baseline");
    second.checkTimers(66);
    check(second.sendTick(66,player), "shared send time admits 33 across client lifetime changes");
    second.outbox.neededZones = {{7,8}};
    second.checkTimers(1033); second.sendTick(1033,player);
    check(transport.kinds() == std::vector<uint32_t>{1,3,3},
          "the request baseline also survives another client and excludes exact1000");
    second.checkTimers(1066); second.sendTick(1066,player);
    check(transport.kinds() == std::vector<uint32_t>{1,3,3,3,15},
          "a later admitted send advances the shared request clock and emits needed zones");
}
void wrappedBoundaries(const auto &check) {
    TransportLog transport;
    GuestLink guest(transport,{});
    guest.connectTo(3,0xffffff00u);
    check(!guest.checkTimers(0x1288u) && guest.checkTimers(0x1289u),
          "the timeout keeps unsigned32 wrap and exact 5000/5001 boundaries");

    auto shared = std::make_shared<GuestLink::Clocks>();
    shared->send = 0xffffffefu;
    TransportLog wrappedTransport;
    GuestLink wrapped(wrappedTransport,{}, {{},shared,{}});
    wrapped.connectTo(3,0xffffffefu);
    CreatureSnapshot player{};
    wrapped.checkTimers(15);
    check(!wrapped.sendTick(15,player), "wrapped send equality 32 is excluded");
    wrapped.checkTimers(16);
    check(wrapped.sendTick(16,player), "wrapped send difference 33 is admitted");
}
void deferredSources(const auto &check) {
    struct TracedMirror : MirrorLog {
        std::vector<std::string> &trace;
        explicit TracedMirror(std::vector<std::string> &trace) : trace(trace) {}
        bool localSnapshot(CreatureSnapshot &out) override { trace.push_back("snapshot"); return MirrorLog::localSnapshot(out); }
        bool takeInteraction(InteractionRecord &out) override { trace.push_back("interaction"); return MirrorLog::takeInteraction(out); }
    };
    TransportLog transport;
    std::vector<std::string> trace;
    TracedMirror mirror(trace);
    transport.afterSend = [&](const Bytes &bytes) { trace.push_back("s" + std::to_string(bytes[0])); };
    ClientGame client(transport,mirror);
    client.link().goHome(0);
    client.send(0);
    client.link().outbox.chat.push_back(u"say");
    trace.clear();
    client.send(32);
    check(mirror.snapshots == 0 && mirror.pulls == 0 && client.link().outbox.chat.size() == 1,
          "unadmitted send frames avoid snapshots and destructive interaction reads");
    client.send(33);
    check(trace == std::vector<std::string>{"snapshot","s3","s14","interaction","s9"},
          "admitted send borrows the snapshot once and pulls interaction only after player/chat output");
    check(mirror.snapshots == 1 && mirror.pulls == 1, "admission eliminates repeated per-frame snapshot/queue work");
}
}
int clientTimingTests() {
    int failures = 0;
    const auto check = [&](bool okay,const char *message) {
        if (!okay) { ++failures; std::printf("FAIL client timing: %s\n",message); }
    };
    unselectedFallback(check); fallbackPrecedesExpiry(check);
    sendClockAdmission(check); fallbackSkipsOrdinaryWork(check);
    freshPeerSamples(check); invitationClockOrder(check); leaveTakesTwoSamples(check);
    batchDelay(check); requestClockSamples(check); requestEmptyAndDurationBoundaries(check);
    sharedClockLifetimes(check);
    wrappedBoundaries(check); deferredSources(check);
    std::printf("Client timing: clocks, fallback, dispatch and send admission: %d failure(s)\n",failures);
    return failures;
}
