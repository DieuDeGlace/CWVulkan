#include "../tools/ZoneExtractor/NativeConversation.h"
#include <iostream>
#include <map>

namespace {
int disposed = 0;
int32_t observedDay = 0;
captureabi::ScriptStorage *generate(void *world, captureabi::ScriptStorage *out, void *) {
    observedDay = *static_cast<int32_t *>(world);
    out->words = {0, 0, 0};
    return out;
}
void destroy(captureabi::ScriptStorage *out) {
    ++disposed;
    out->words = {};
}
} // namespace

int nativeConversationTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *label) {
        if (!passed) {
            std::cout << "FAIL conversation capture: " << label << '\n';
            ++failures;
        }
    };
    int32_t day = 7;
    disposed = 0;
    {
        const auto result = [&] {
            const captureabi::ScopedScriptDay temporary(day, -9);
            return captureabi::ScriptResult(&day, nullptr, generate, destroy);
        }();
        check(day == 7 && observedDay == -9 && disposed == 0,
              "generation sees temporary day; day restored before result consumption and no early destruction");
    }
    check(disposed == 1, "successful native result disposed exactly once at consumer scope end");
    try {
        const auto result = [&] {
            const captureabi::ScopedScriptDay temporary(day, 33);
            return captureabi::ScriptResult(&day, nullptr, generate, destroy);
        }();
        throw 1;
    } catch (int) {
    }
    check(day == 7 && disposed == 2, "consumer exception preserves day and disposes owned native result");
    try {
        const captureabi::ScopedScriptDay temporary(day, 44);
        throw 1;
    } catch (int) {
    }
    check(day == 7 && captureabi::captureScriptDay(INT32_MAX, 1) == INT32_MIN &&
              captureabi::captureScriptDay(-1, 1) == 0,
          "day override unwinds and the capture window wraps signed32 without overflow");

    try {
        const captureabi::ScopedScriptDay temporary(day, 55);
        const auto throwsBeforeReturn =
            +[](void *, captureabi::ScriptStorage *, void *) -> captureabi::ScriptStorage * { throw 9; };
        const captureabi::ScriptResult incomplete(&day, nullptr, throwsBeforeReturn, destroy);
    } catch (int) {
    }
    check(day == 7 && disposed == 2,
          "generation exception restores day without claiming ownership of an unreturned result");

    constexpr uintptr_t resultAddress = 0x100000, strings = 0x200000, heap = 0x300001;
    std::map<uintptr_t, std::vector<uint8_t>> memory;
    const auto store = [&](uintptr_t at, const auto &value) {
        auto &bytes = memory[at];
        bytes.resize(sizeof value);
        std::memcpy(bytes.data(), &value, sizeof value);
    };
    const auto read = [&](uintptr_t at, void *destination, size_t count) {
        auto found = memory.upper_bound(at);
        if (found == memory.begin())
            return false;
        --found;
        const auto offset = at - found->first;
        if (offset > found->second.size() || count > found->second.size() - offset)
            return false;
        std::memcpy(destination, found->second.data() + offset, count);
        return true;
    };
    std::array<uint8_t, 96> entries{};
    const std::array<char, 17> text{'a', 'b', 'c', '\0', 'e', 'f', 'g', 'h', 'i',
                                    'j', 'k', 'l', 'm',  'n', 'o', 'p', 'q'};
    const auto fixture = [&] {
        memory.clear();
        entries = {};
        std::memcpy(entries.data(), "0123456789ABCDE", 15);
        const uintptr_t fifteen = 15, seventeen = 17, thirtyOne = 31, zero = 0;
        std::memcpy(entries.data() + 16, &fifteen, 8);
        std::memcpy(entries.data() + 24, &fifteen, 8);
        std::memcpy(entries.data() + 32, &heap, 8);
        std::memcpy(entries.data() + 48, &seventeen, 8);
        std::memcpy(entries.data() + 56, &thirtyOne, 8);
        std::memcpy(entries.data() + 80, &zero, 8);
        std::memcpy(entries.data() + 88, &fifteen, 8);
        store(strings, entries);
        store(heap, text);
        store(resultAddress, std::array<uintptr_t, 3>{strings, strings + 96, strings + 128});
    };
    std::vector<std::string> tokens;
    const auto capture = [&] { return captureabi::readScriptTokens(resultAddress, tokens, read); };
    fixture();
    check(capture() && tokens.size() == 3 && tokens[0] == "0123456789ABCDE" &&
              tokens[1] == std::string(text.data(), text.size()) && tokens[2].empty(),
          "15-byte inline, unaligned heap bytes, embedded NUL, empty and capacity-only unused element decode exactly");
    fixture();
    memory.erase(heap);
    check(!capture() && tokens.empty(), "unreadable later text discards earlier partial tokens");
    fixture();
    uintptr_t bad = 16;
    std::memcpy(entries.data() + 16, &bad, 8);
    store(strings, entries);
    check(!capture() && tokens.empty(), "inline length cannot exceed its declared capacity");
    fixture();
    bad = captureabi::MAX_SCRIPT_TEXT + 1;
    std::memcpy(entries.data() + 48, &bad, 8);
    std::memcpy(entries.data() + 56, &bad, 8);
    store(strings, entries);
    check(!capture() && tokens.empty(), "text budget rejects oversized ownership metadata before allocation");
    fixture();
    store(resultAddress, std::array<uintptr_t, 3>{strings, strings + 33, strings + 64});
    check(!capture(), "string-vector bounds require complete 32-byte elements");
    fixture();
    const auto changedString = [&](uintptr_t at, void *out, size_t count) {
        const bool copied = read(at, out, count);
        if (at == heap) {
            entries[0] ^= 1;
            entries[32] ^= 1;
            store(strings, entries);
        }
        return copied;
    };
    check(!captureabi::readScriptTokens(resultAddress, tokens, changedString) && tokens.empty(),
          "changed heap-string metadata rejects a candidate token");
    fixture();
    const auto changedVector = [&](uintptr_t at, void *out, size_t count) {
        const bool copied = read(at, out, count);
        if (at == heap)
            store(resultAddress, std::array<uintptr_t, 3>{strings, strings + 64, strings + 128});
        return copied;
    };
    check(!captureabi::readScriptTokens(resultAddress, tokens, changedVector) && tokens.empty(),
          "changed result-vector metadata rejects all borrowed string inputs");
    fixture();
    store(resultAddress, std::array<uintptr_t, 3>{0, 0, 0});
    check(capture() && tokens.empty(), "native empty result is a successful empty token sequence");
    const auto maximum = std::numeric_limits<uintptr_t>::max();
    check(!captureabi::readScriptTokens(maximum, tokens, read) && tokens.empty(),
          "result span cannot wrap into low memory");
    std::cout << "Native conversation ownership/text: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
