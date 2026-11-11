// The host and AI threads (docs/specs/steam-in-process-host-thread-and-world-step-scheduling-20261111.md).
#include "Extracted.h"
#include "Multiplayer.h"
#include "NetWorlds.h"
#include "Spawned.h"
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

int hostThreadTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *message) {
        if (!passed) {
            std::printf("FAIL host thread: %s\n", message);
            ++failures;
        }
    };
    using namespace std::chrono_literals;
    check(Multiplayer::hostSleepAfterTick(0) == 32 && Multiplayer::hostSleepAfterTick(31) == 1 &&
              Multiplayer::hostSleepAfterTick(32) == 0 && Multiplayer::hostSleepAfterTick(500) == 0,
          "the host thread sleeps the rest of a 32 ms period, and not at all after a long tick (2.1)");
    check(Multiplayer::aiSleepAfterTick(0) == 40 && Multiplayer::aiSleepAfterTick(39) == 1 &&
              Multiplayer::aiSleepAfterTick(40) == 1 && Multiplayer::aiSleepAfterTick(90) == 1,
          "the AI thread sleeps the rest of 40 ms, and at least 1 ms (3.1)");
    check(Multiplayer::HOST_IDLE_SLEEP_MS == 33, "an idle host sleeps 33 ms without ticking (2.1)");

    {
        cwnet::LoopbackNetwork network;
        cwnet::SerializedTransport wrapped(network.endpoint(5));
        cwnet::Received received;
        check(wrapped.self() == 5 && wrapped.valid(3) && !wrapped.valid(0), "the serialized transport keeps its backend's identity");
        check(wrapped.send(5, cwnet::Channel::ToHost, {1, 2, 3}) && wrapped.receive(cwnet::Channel::ToHost, received) &&
                  received.from == 5 && received.bytes == cwnet::Bytes({1, 2, 3}),
              "the serialized transport forwards a send and a receive");
        check(wrapped.send(5, cwnet::Channel::ToClient, {4}) && wrapped.receiveBatch(cwnet::Channel::ToClient).size() == 1,
              "the serialized transport forwards a batch receive");
    }

    {
        // The live World never runs AI: the AI routine has one caller, for the host World (3.2).
        ExtractedWorld blocks;
        blocks.openGenerated(0, 0);
        SpawnedCreatures viewed, authoritative;
        for (SpawnedCreatures *each : {&viewed, &authoritative}) {
            each->setStoredZones(blocks, nullptr, true);
            each->setNativePlayer(true);
            each->setPlayer(blocks, {1, 2, 130}, 0, {}, 100, 1, true);
        }
        viewed.setRemoteClient(true);
        viewed.think(blocks, 100, 12);
        authoritative.think(blocks, 100, 12);
        check(viewed.aiTicksRun() == 0, "a viewed mirror World runs no AI");
        check(authoritative.aiTicksRun() == 1, "an authoritative World still runs its AI tick");
    }
    ExtractedWorld world;
    world.openGenerated(0, 0);
    SpawnedCreatures authority, mirror;
    Multiplayer session;
    Multiplayer::Setup setup;
    setup.hostWorld = &authority;
    setup.clientWorld = &mirror;
    setup.blocks = &world;
    std::string error;
    check(session.start(setup, error), "a combined local session starts");
    if (!session.started() || !session.hostBlocks() || !session.hostSide())
        return failures + 1;
    authority.setStoredZones(*session.hostBlocks(), nullptr, true);
    mirror.setStoredZones(world, nullptr, true);

    int posted = 0;
    session.post([&] { ++posted; });
    check(posted == 0, "a posted host result waits for the main thread");
    session.beginFrame(Multiplayer::clockNow());
    check(posted == 1, "beginFrame runs posted host results in order on the calling thread");

    const auto hostTime = [&] {
        auto hold = session.lockHost();
        return authority.elapsedWorldTime();
    };
    const float before = hostTime();
    std::this_thread::sleep_for(120ms);
    check(!session.hostThreadRunning() && hostTime() == before, "without the host thread nothing ticks the host between frames");

    session.startHostThread();
    check(session.hostThreadRunning() && session.hostSide()->separateAi, "the host and AI threads start, and AI leaves the host step");
    std::this_thread::sleep_for(400ms);
    check(hostTime() - before >= 200.0f, "the host thread advances the host World with no frame calls");
    session.stopHostThread();
    const float stopped = hostTime();
    std::this_thread::sleep_for(100ms);
    check(!session.hostThreadRunning() && hostTime() == stopped && !session.hostSide()->separateAi,
          "stopping joins both threads and returns AI to the host step");
    return failures;
}
