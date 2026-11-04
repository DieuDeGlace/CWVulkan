#pragma once
// Original ABI/ownership: steam-conversation-generation-protection.md.
#include "CaptureMemory.h"
#include <array>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace captureabi {
struct ScriptStorage {
    std::array<uintptr_t, 3> words{};
};
static_assert(sizeof(ScriptStorage) == 24);
using GenerateScript = ScriptStorage *(*)(void *, ScriptStorage *, void *);
using DestroyScript = void (*)(ScriptStorage *);

// Calls the original destructor only after generation returns successfully.
// Native failure cleanup before return is not established by this adapter.
class ScriptResult {
    ScriptStorage value;
    DestroyScript destroy;

  public:
    ScriptResult(void *world, void *actor, GenerateScript generate, DestroyScript dispose) : destroy(dispose) {
        generate(world, &value, actor);
    }
    ~ScriptResult() { destroy(&value); }
    ScriptResult(const ScriptResult &) = delete;
    ScriptResult &operator=(const ScriptResult &) = delete;
    uintptr_t address() const { return reinterpret_cast<uintptr_t>(&value); }
};

class ScopedScriptDay {
    int32_t &target;
    int32_t previous;

  public:
    ScopedScriptDay(int32_t &day, int32_t replacement) : target(day), previous(day) { target = replacement; }
    ~ScopedScriptDay() { target = previous; }
    ScopedScriptDay(const ScopedScriptDay &) = delete;
    ScopedScriptDay &operator=(const ScopedScriptDay &) = delete;
};
inline int32_t captureScriptDay(int32_t day, uint32_t offset) {
    const uint32_t bits = static_cast<uint32_t>(day) + offset;
    int32_t value;
    std::memcpy(&value, &bits, sizeof value);
    return value;
}

constexpr size_t MAX_SCRIPT_TOKENS = 16384; // Capture budgets, not native limits.
constexpr size_t MAX_SCRIPT_TEXT = 8 * 1024 * 1024;
template <class Read> bool readScriptTokens(uintptr_t result, std::vector<std::string> &output, Read read) {
    output.clear();
    std::array<uintptr_t, 3> span;
    if (!readField(result, 0, span.data(), sizeof span, read))
        return false;
    const auto begin = span[0], end = span[1], capacityEnd = span[2];
    if ((!begin && (end || capacityEnd)) || begin > end || end > capacityEnd || ((begin | end | capacityEnd) & 7) ||
        (end - begin) % 32 || (capacityEnd - begin) % 32 || (end - begin) / 32 > MAX_SCRIPT_TOKENS)
        return false;
    std::vector<std::string> tokens;
    const auto count = (end - begin) / 32;
    tokens.reserve(count);
    size_t total = 0;
    for (size_t i = 0; i < count; ++i) {
        std::array<uint8_t, 32> element, current;
        if (!readField(begin, i * 32, element.data(), element.size(), read))
            return false;
        uintptr_t length, capacity, pointer;
        std::memcpy(&length, element.data() + 16, 8);
        std::memcpy(&capacity, element.data() + 24, 8);
        if (length > capacity || length > MAX_SCRIPT_TEXT - total)
            return false;
        total += length;
        std::string text;
        if (capacity <= 15)
            text.assign(reinterpret_cast<const char *>(element.data()), length);
        else {
            std::memcpy(&pointer, element.data(), 8);
            text.resize(length);
            if (length && !readField(pointer, 0, text.data(), length, read))
                return false;
        }
        if (!readField(begin, i * 32, current.data(), current.size(), read) || current != element)
            return false;
        tokens.push_back(std::move(text));
    }
    std::array<uintptr_t, 3> current;
    if (!readField(result, 0, current.data(), sizeof current, read) || current != span)
        return false;
    output = std::move(tokens);
    return true;
}
} // namespace captureabi
