#include "../tools/ZoneExtractor/CaptureTickContext.h"
#include <cstring>
#include <iostream>
#include <map>
#include <vector>

int captureTickContextTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *label) {
        if (!passed) {
            std::cout << "FAIL capture tick context: " << label << '\n';
            ++failures;
        }
    };
    constexpr std::uintptr_t module = 0x10000000, controller = 0x20000000;
    constexpr auto slot = module + captureabi::CONTROLLER_SLOT_RVA;
    constexpr auto flag = controller + captureabi::QUIT_BYTE_OFFSET;
    std::map<std::uintptr_t, std::vector<std::uint8_t>> memory;
    std::vector<std::uintptr_t> reads;
    const auto store = [&](std::uintptr_t address, auto value) {
        auto &bytes = memory[address];
        bytes.resize(sizeof value);
        std::memcpy(bytes.data(), &value, sizeof value);
    };
    const auto read = [&](std::uintptr_t address, void *destination, std::size_t count) {
        reads.push_back(address);
        const auto found = memory.find(address);
        if (found == memory.end() || found->second.size() != count)
            return false;
        std::memcpy(destination, found->second.data(), count);
        return true;
    };
    const auto accepted = [&](std::uintptr_t reported) {
        reads.clear();
        return captureabi::acceptedTickContext(module, reported, read);
    };
    store(slot, controller);
    store(flag, std::uint8_t(0));
    check(accepted(controller) == controller && reads == std::vector<std::uintptr_t>{slot, flag},
          "normal admission reads only the global pointer and its native quit byte");
    // No World field is present in this memory fixture: title/unready Worlds do
    // not change controller identity and must not become an invented admission gate.
    check(accepted(controller) == controller, "title admission does not dereference or require a World");
    check(accepted(0xbad) == 0 && reads == std::vector<std::uintptr_t>{slot},
          "unproved PostQuitMessage register bits are never dereferenced");
    store(flag, std::uint8_t(1));
    check(accepted(controller) == 0 && reads == std::vector<std::uintptr_t>{slot, flag},
          "quit admission rejects even a coincidentally preserved controller register");
    memory.erase(flag);
    check(accepted(controller) == 0, "an unreadable quit byte cannot admit capture");
    store(slot, std::uintptr_t(0));
    check(accepted(controller) == 0 && reads == std::vector<std::uintptr_t>{slot},
          "a null published controller causes no object read");
    memory.clear();
    check(accepted(controller) == 0 && reads == std::vector<std::uintptr_t>{slot},
          "an unavailable publication slot fails closed");
    check(accepted(0) == 0 && reads.empty(), "a null callback makes no native reads");
    const auto maximum = std::numeric_limits<std::uintptr_t>::max();
    reads.clear();
    check(captureabi::acceptedTickContext(maximum, controller, read) == 0 && reads.empty(),
          "module address arithmetic cannot wrap into a low readable range");
    store(slot, maximum);
    check(accepted(maximum) == 0 && reads == std::vector<std::uintptr_t>{slot},
          "controller address arithmetic cannot wrap into a low readable range");
    std::cout << "Capture tick context: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
