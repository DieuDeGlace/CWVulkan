#include "../tools/ZoneExtractor/CaptureActorSection.h"
#include "../tools/ZoneExtractor/NativeCreatureViews.h"
#include <algorithm>
#include <cstring>
#include <future>
#include <iostream>
#include <limits>

namespace {
constexpr uintptr_t GAME = 0x100, WORLD = 0x1000, LIST = 0x2000, TREE = 0x3000;
struct Memory {
    std::vector<uint8_t> bytes = std::vector<uint8_t>(0xa000);
    template <class T> void set(uintptr_t at, T value) { std::memcpy(bytes.data() + at, &value, sizeof value); }
    bool read(uintptr_t at, void *destination, size_t count) const {
        if (!at || at > bytes.size() || count > bytes.size() - at)
            return false;
        // Neither sentinel payload was initialized by its constructor.
        for (auto range : {std::pair<uintptr_t, uintptr_t>{LIST + 0x10, LIST + 0x18}, {TREE + 0x20, TREE + 0x30}})
            if (at < range.second && count > range.first - std::min(range.first, at))
                return false;
        std::memcpy(destination, bytes.data() + at, count);
        return true;
    }
    Memory() {
        set(GAME + 8, WORLD);
        set(WORLD + 0x448, uintptr_t(0x7000));
        set(WORLD + 0x40, LIST);
        set(WORLD + 0x48, uint64_t(0));
        set(LIST, LIST);
        set(LIST + 8, LIST);
        set(WORLD + 0x450, TREE);
        set(WORLD + 0x458, uint64_t(0));
        set(TREE, TREE);
        set(TREE + 8, TREE);
        set(TREE + 16, TREE);
        bytes[TREE + 0x18] = bytes[TREE + 0x19] = 1;
    }
};
} // namespace

int nativeCreatureViewsTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *label) {
        if (!passed) {
            std::cout << "FAIL creature views: " << label << '\n';
            ++failures;
        }
    };
    Memory memory;
    const auto read = [&](uintptr_t at, void *destination, size_t count) {
        return memory.read(at, destination, count);
    };
    uintptr_t world = 123, local = 123;
    check(captureabi::readClientWorld(GAME, world, read) && world == WORLD &&
              captureabi::readLocalCreature(world, local, read) && local == 0x7000,
          "Game owns a pointer to client World, which separately publishes its local creature");
    memory.set(WORLD + 0x448, uintptr_t(0));
    check(captureabi::readLocalCreature(WORLD, local, read) && local == 0,
          "constructed-null local player is a readable value, not gameplay readiness");
    check(!captureabi::readClientWorld(0, world, read) && world == 0 &&
              !captureabi::readLocalCreature(std::numeric_limits<uintptr_t>::max() - 1, local, read) && local == 0,
          "primitive pointer reads reject null/overflow without publishing partial output");
    std::vector<uintptr_t> actors{123};
    check(captureabi::readRegisteredCreatures(WORLD, actors, read) && actors.empty(),
          "constructed empty registration list uses self links without reading header payload");
    const std::array<uintptr_t, 3> nodes{0x2100, 0x2500, 0x2900};
    const std::array<uintptr_t, 3> payloads{0x6000, 0, 0x6000};
    memory.set(WORLD + 0x48, uint64_t(3));
    memory.set(LIST, nodes.front());
    memory.set(LIST + 8, nodes.back());
    for (size_t i = 0; i < nodes.size(); ++i) {
        memory.set(nodes[i], i + 1 == nodes.size() ? LIST : nodes[i + 1]);
        memory.set(nodes[i] + 8, i == 0 ? LIST : nodes[i - 1]);
        memory.set(nodes[i] + 16, payloads[i]);
    }
    check(captureabi::readRegisteredCreatures(WORLD, actors, read) &&
              actors == std::vector<uintptr_t>(payloads.begin(), payloads.end()),
          "noncontiguous list nodes preserve registration order, duplicate values and null consumer-policy input");
    memory.set(nodes[1] + 8, LIST);
    check(!captureabi::readRegisteredCreatures(WORLD, actors, read) && actors.empty(),
          "broken backlinks reject the entire read");
    memory.set(nodes[1] + 8, nodes[0]);
    memory.set(nodes[2], nodes[0]);
    check(!captureabi::readRegisteredCreatures(WORLD, actors, read), "cyclic non-sentinel chain is bounded");
    memory.set(nodes[2], LIST);
    const auto changedList = [&](uintptr_t at, void *destination, size_t count) {
        const bool copied = memory.read(at, destination, count);
        if (at == nodes[2])
            memory.set(WORLD + 0x48, uint64_t(4));
        return copied;
    };
    check(!captureabi::readRegisteredCreatures(WORLD, actors, changedList), "changed list metadata rejects capture");
    memory.set(WORLD + 0x48, uint64_t(captureabi::MAX_CAPTURE_CREATURES + 1));
    check(!captureabi::readRegisteredCreatures(WORLD, actors, read),
          "capture budget is enforced independently of map count");

    captureabi::CreatureLookup found{true, 123};
    check(captureabi::lookupCreature(WORLD, 0, found, read) && !found.present && !found.creature,
          "empty tree ignores uninitialized sentinel key/value");
    const std::array<uintptr_t, 3> treeNodes{0x3100, 0x3500, 0x3900};
    const std::array<int64_t, 3> ids{INT64_MIN, 0, INT64_MAX};
    const std::array<uintptr_t, 3> values{0, 0x7000, 0x9000};
    memory.set(WORLD + 0x458, uint64_t(3));
    memory.set(TREE, treeNodes[0]);
    memory.set(TREE + 8, treeNodes[1]);
    memory.set(TREE + 16, treeNodes[2]);
    for (size_t i = 0; i < treeNodes.size(); ++i) {
        const auto node = treeNodes[i];
        memory.set(node, i == 1 ? treeNodes[0] : TREE);
        memory.set(node + 8, i == 1 ? TREE : treeNodes[1]);
        memory.set(node + 16, i == 1 ? treeNodes[2] : TREE);
        memory.bytes[node + 0x18] = 1;
        memory.set(node + 0x20, ids[i]);
        memory.set(node + 0x28, values[i]);
    }
    for (size_t i = 0; i < ids.size(); ++i)
        check(captureabi::lookupCreature(WORLD, ids[i], found, read) && found.present && found.creature == values[i],
              "signed64 lookup accepts high-bit/zero IDs and distinguishes a present-null entry");
    check(captureabi::lookupCreature(WORLD, -1, found, read) && !found.present && !found.creature,
          "absent signed key does not insert or expose another entry");
    memory.set(treeNodes[0] + 8, TREE);
    check(!captureabi::lookupCreature(WORLD, INT64_MIN, found, read) && !found.present && !found.creature,
          "corrupt tree parent links fail without stale output");
    memory.set(treeNodes[0] + 8, treeNodes[1]);
    const auto changedTree = [&](uintptr_t at, void *destination, size_t count) {
        const bool copied = memory.read(at, destination, count);
        if (at == treeNodes[1] + 0x28)
            memory.set(WORLD + 0x458, uint64_t(4));
        return copied;
    };
    check(!captureabi::lookupCreature(WORLD, 0, found, changedTree) && !found.present,
          "changed tree metadata rejects a candidate result");
    alignas(8) std::array<uint8_t, 0x500> guardedWorld{};
    auto *section = reinterpret_cast<CRITICAL_SECTION *>(guardedWorld.data() + captureabi::WORLD_ACTOR_SECTION);
    InitializeCriticalSectionAndSpinCount(section, 16);
    const auto worldAddress = reinterpret_cast<uintptr_t>(guardedWorld.data());
    std::array<uint8_t, 16> game{};
    std::array<uintptr_t, 3> head{}, node{};
    const auto gameAddress = reinterpret_cast<uintptr_t>(game.data());
    const auto headAddress = reinterpret_cast<uintptr_t>(head.data());
    const auto nodeAddress = reinterpret_cast<uintptr_t>(node.data());
    std::memcpy(game.data() + 8, &worldAddress, 8);
    std::memcpy(guardedWorld.data() + 0x40, &headAddress, 8);
    const uint64_t one = 1;
    std::memcpy(guardedWorld.data() + 0x48, &one, 8);
    head = {nodeAddress, nodeAddress, 0};
    node = {headAddress, headAddress, 0x7000};
    const std::array<std::pair<uintptr_t, size_t>, 4> spans{{{gameAddress, game.size()},
                                                             {worldAddress, guardedWorld.size()},
                                                             {headAddress, sizeof head},
                                                             {nodeAddress, sizeof node}}};
    const auto clientRead = [&](uintptr_t at, void *destination, size_t count) {
        for (const auto &[start, size] : spans)
            if (at >= start && at - start <= size && count <= size - (at - start)) {
                std::memcpy(destination, reinterpret_cast<const void *>(at), count);
                return true;
            }
        return false;
    };
    std::promise<void> entered, release;
    auto releaseSignal = release.get_future();
    auto worker = std::async(std::launch::async, [&] {
        EnterCriticalSection(section);
        entered.set_value();
        releaseSignal.wait();
        LeaveCriticalSection(section);
    });
    entered.get_future().wait();
    {
        captureabi::WorldActorLease contended(worldAddress);
        check(!contended, "native frame guard capture is nonblocking when another thread owns it");
        captureabi::ClientCreatureCapture unavailable(gameAddress, clientRead);
        check(!unavailable.ready() && !unavailable.world() && unavailable.addresses().empty(),
              "busy client capture is unavailable, not a valid empty actor observation");
    }
    release.set_value();
    worker.get();
    const auto otherThreadCanEnter = [&] {
        return std::async(std::launch::async,
                          [&] {
                              const bool acquired = TryEnterCriticalSection(section) != 0;
                              if (acquired)
                                  LeaveCriticalSection(section);
                              return acquired;
                          })
            .get();
    };
    {
        captureabi::WorldActorLease held(worldAddress);
        check(bool(held) && !otherThreadCanEnter(), "guard remains held through borrowed-pointer use scope");
    }
    check(otherThreadCanEnter(), "ordinary scope exit releases the native frame guard");
    try {
        captureabi::WorldActorLease held(worldAddress);
        throw 1;
    } catch (int) {
    }
    check(otherThreadCanEnter(), "exception unwinding releases the frame guard");
    {
        captureabi::ClientCreatureCapture captured(gameAddress, clientRead);
        check(captured.ready() && captured.world() == worldAddress &&
                  captured.addresses() == std::vector<uintptr_t>{0x7000} && !otherThreadCanEnter(),
              "client collection retains the guard after copying addresses and through consumer use");
    }
    check(otherThreadCanEnter(), "client collection releases its guard after consumer scope");
    int gameReads = 0;
    const auto changedClient = [&](uintptr_t at, void *destination, size_t count) {
        if (at == gameAddress + 8 && ++gameReads == 2) {
            std::memset(destination, 0, count);
            return true;
        }
        return clientRead(at, destination, count);
    };
    {
        captureabi::ClientCreatureCapture changed(gameAddress, changedClient);
        check(!changed.ready() && !changed.world() && changed.addresses().empty(),
              "changed client World identity does not publish actor pointers");
    }
    node[1] = 0;
    {
        captureabi::ClientCreatureCapture malformed(gameAddress, clientRead);
        check(!malformed.ready() && malformed.addresses().empty(), "malformed client list remains unavailable");
    }
    const uint64_t zero = 0;
    std::memcpy(guardedWorld.data() + 0x48, &zero, 8);
    head = {headAddress, headAddress, 0};
    {
        captureabi::ClientCreatureCapture empty(gameAddress, clientRead);
        check(empty.ready() && empty.world() == worldAddress && empty.addresses().empty() && !otherThreadCanEnter(),
              "a valid empty client list is distinguishable from unavailable capture");
    }
    try {
        const auto throwsDuringRead = [&](uintptr_t at, void *destination, size_t count) {
            if (at == worldAddress + 0x40)
                throw 1;
            return clientRead(at, destination, count);
        };
        captureabi::ClientCreatureCapture interrupted(gameAddress, throwsDuringRead);
    } catch (int) {
    }
    check(otherThreadCanEnter(), "failed collection construction unwinds the acquired guard");
    DeleteCriticalSection(section);

    alignas(8) std::array<uint8_t, 0x1000> hostGame{};
    const auto hostGameAddress = reinterpret_cast<uintptr_t>(hostGame.data());
    const auto hostWorld = hostGameAddress + 0x6f0;
    auto *outerSection = reinterpret_cast<CRITICAL_SECTION *>(hostGame.data() + 0xce0);
    auto *innerSection = reinterpret_cast<CRITICAL_SECTION *>(hostGame.data() + 0xab8);
    InitializeCriticalSectionAndSpinCount(outerSection, 16);
    InitializeCriticalSectionAndSpinCount(innerSection, 16);
    std::memcpy(hostGame.data() + 0x730, &headAddress, 8);
    std::memcpy(hostGame.data() + 0x738, &one, 8);
    head = {nodeAddress, nodeAddress, 0};
    node = {headAddress, headAddress, 0x9000};
    const auto hostRead = [&](uintptr_t at, void *destination, size_t count) {
        if (at >= hostGameAddress && at - hostGameAddress <= hostGame.size() &&
            count <= hostGame.size() - (at - hostGameAddress)) {
            std::memcpy(destination, reinterpret_cast<const void *>(at), count);
            return true;
        }
        return clientRead(at, destination, count);
    };
    const auto canEnter = [](CRITICAL_SECTION *lock) {
        return std::async(std::launch::async,
                          [=] {
                              const bool acquired = TryEnterCriticalSection(lock) != 0;
                              if (acquired)
                                  LeaveCriticalSection(lock);
                              return acquired;
                          })
            .get();
    };
    {
        captureabi::HostCreatureCapture captured(hostGameAddress, hostRead);
        check(captured.ready() && captured.world() == hostWorld &&
                  captured.addresses() == std::vector<uintptr_t>{0x9000} && !canEnter(outerSection) &&
                  !canEnter(innerSection),
              "host capture uses the embedded World and retains both distinct guards through pointer use");
    }
    check(canEnter(outerSection) && canEnter(innerSection), "host capture releases both guards after use");
    for (auto *busySection : {outerSection, innerSection}) {
        std::promise<void> acquired, finish;
        auto finishSignal = finish.get_future();
        auto owner = std::async(std::launch::async, [&] {
            EnterCriticalSection(busySection);
            acquired.set_value();
            finishSignal.wait();
            LeaveCriticalSection(busySection);
        });
        acquired.get_future().wait();
        {
            captureabi::HostCreatureCapture busy(hostGameAddress, hostRead);
            check(!busy.ready() && !busy.world() && busy.addresses().empty(),
                  "either busy host guard produces unavailable rather than an empty observation");
            if (busySection == outerSection)
                check(canEnter(innerSection), "busy outer host guard prevents taking the World guard");
            else
                check(!canEnter(outerSection), "outer guard is acquired before attempting the busy World guard");
        }
        if (busySection == innerSection)
            check(canEnter(outerSection), "failed inner acquisition releases the outer guard on scope exit");
        finish.set_value();
        owner.get();
    }
    try {
        const auto throwingHostRead = [&](uintptr_t at, void *destination, size_t count) {
            if (at == hostWorld + 0x40)
                throw 1;
            return hostRead(at, destination, count);
        };
        captureabi::HostCreatureCapture interrupted(hostGameAddress, throwingHostRead);
    } catch (int) {
    }
    check(canEnter(outerSection) && canEnter(innerSection), "host construction unwinds both acquired guards");
    {
        captureabi::HostCreatureCapture missing(0, hostRead);
        captureabi::HostCreatureCapture overflow(std::numeric_limits<uintptr_t>::max() - 1, hostRead);
        check(!missing.ready() && !overflow.ready(), "invalid host owner arithmetic never reaches a section");
    }
    DeleteCriticalSection(innerSection);
    DeleteCriticalSection(outerSection);
    std::cout << "Native creature registry views: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
