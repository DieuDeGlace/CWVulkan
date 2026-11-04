#include <iostream>
#ifdef _WIN32
#include "../tools/ZoneExtractor/CaptureWorldSections.h"
#include <chrono>
#include <future>
#include <thread>

int captureWorldSectionTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *label) {
        if (!passed) {
            std::cout << "FAIL capture World sections: " << label << '\n';
            ++failures;
        }
    };
    alignas(CRITICAL_SECTION) std::array<std::byte, 0x500> storage{};
    const auto world = reinterpret_cast<std::uintptr_t>(storage.data());
    // Independent contract offsets, not locations queried from the guard.
    auto *zones = reinterpret_cast<CRITICAL_SECTION *>(storage.data() + 0x3f0);
    auto *mesh = reinterpret_cast<CRITICAL_SECTION *>(storage.data() + 0x3c8);
    InitializeCriticalSection(zones);
    InitializeCriticalSection(mesh);
    const auto available = [&] {
        return std::async(std::launch::async,
                          [&] {
                              std::array<bool, 2> result{};
                              result[0] = TryEnterCriticalSection(zones) != 0;
                              if (result[0])
                                  LeaveCriticalSection(zones);
                              result[1] = TryEnterCriticalSection(mesh) != 0;
                              if (result[1])
                                  LeaveCriticalSection(mesh);
                              return result;
                          })
            .get();
    };
    {
        captureabi::WorldBufferLease lease(world);
        check(bool(lease) && available() == std::array<bool, 2>{false, false},
              "successful leases retain both actual Win32 sections throughout their scope");
    }
    check(available() == std::array<bool, 2>{true, true}, "scope exit releases both sections");
    for (auto *held : {zones, mesh}) {
        std::promise<void> ready, release;
        auto released = release.get_future();
        bool timedOut = false;
        std::thread worker([&] {
            EnterCriticalSection(held);
            ready.set_value();
            timedOut = released.wait_for(std::chrono::seconds(2)) == std::future_status::timeout;
            LeaveCriticalSection(held);
        });
        ready.get_future().wait();
        {
            captureabi::WorldBufferLease lease(world);
            check(!lease, "contention never admits a partial lease");
            // A failed publication acquisition must release the earlier owner
            // acquisition immediately, while the invalid lease still exists.
            const auto state = available();
            check(state[held == zones ? 1 : 0], "the unblocked section is available after failed acquisition");
            release.set_value();
        }
        worker.join();
        check(!timedOut, "failed acquisition returns without waiting for the other owner");
        check(available() == std::array<bool, 2>{true, true}, "contention leaves no leaked section ownership");
    }
    try {
        captureabi::WorldBufferLease lease(world);
        if (lease)
            throw 1;
    } catch (int) {
    }
    check(available() == std::array<bool, 2>{true, true}, "exception unwinding releases the ordered pair");
    check(!captureabi::WorldBufferLease(0) && !captureabi::WorldBufferLease(std::numeric_limits<std::uintptr_t>::max()),
          "null and wrapping World spans make no section acquisition");
    DeleteCriticalSection(mesh);
    DeleteCriticalSection(zones);
    std::cout << "Capture World sections: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
#else
int captureWorldSectionTests() {
    std::cout << "SKIP capture World sections: Win32 contention check\n";
    return 0;
}
#endif
