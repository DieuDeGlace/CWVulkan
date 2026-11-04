#pragma once
// Original ABI/ownership: steam-native-generated-text.md.
#include "CaptureMemory.h"
#include <array>
#include <cstring>
#include <string>
#include <utility>

namespace captureabi {
struct WideTextStorage {
    std::array<uint64_t, 4> words{};
};
static_assert(sizeof(WideTextStorage) == 0x20 && sizeof(char16_t) == 2);
using DestroyWideText = void (*)(WideTextStorage *);
using GenerateProperName = WideTextStorage *(*)(WideTextStorage *, uint32_t, int32_t);
using GenerateItemText = WideTextStorage *(*)(void *, WideTextStorage *, const void *);
using LookupItemTextKey = WideTextStorage *(*)(void *, WideTextStorage *, int32_t, int32_t);
constexpr uintptr_t GENERATE_NAME_RVA = 0x611d0, GENERATE_ITEM_TEXT_RVA = 0x1628b0;
constexpr uintptr_t ITEM_TEXT_KEY_RVA = 0x16af30, DESTROY_WIDE_TEXT_RVA = 0x15c9a0;
inline uintptr_t textResourceOwner(uintptr_t game) {
    return game && game <= std::numeric_limits<uintptr_t>::max() - 0x50 ? game + 0x50 : 0;
}

class WideTextResult {
    WideTextStorage value;
    DestroyWideText dispose;

  public:
    // Both native generators construct fresh destination storage themselves.
    // Disposal applies only after successful return, not a failing constructor.
    template <class Generate> WideTextResult(Generate generate, DestroyWideText destroy) : dispose(destroy) {
        generate(&value);
    }
    ~WideTextResult() { dispose(&value); }
    WideTextResult(const WideTextResult &) = delete;
    WideTextResult &operator=(const WideTextResult &) = delete;
    uintptr_t address() const { return reinterpret_cast<uintptr_t>(&value); }
};

constexpr size_t MAX_CAPTURE_TEXT_UNITS = 1024 * 1024; // Tool budget, not a native maximum.
template <class Read> bool readWideText(uintptr_t result, std::u16string &output, Read read) {
    output.clear();
    std::array<uint8_t, 32> record, current;
    if (!readField(result, 0, record.data(), record.size(), read))
        return false;
    uint64_t length, capacity;
    std::memcpy(&length, record.data() + 16, 8);
    std::memcpy(&capacity, record.data() + 24, 8);
    if (length > capacity || length > MAX_CAPTURE_TEXT_UNITS)
        return false;
    std::u16string text(length + 1, u'\0');
    const size_t bytes = text.size() * sizeof(char16_t);
    if (capacity < 8)
        std::memcpy(text.data(), record.data(), bytes);
    else {
        uintptr_t pointer;
        std::memcpy(&pointer, record.data(), 8);
        if (!readField(pointer, 0, text.data(), bytes, read))
            return false;
    }
    if (text.back() != u'\0' || !readField(result, 0, current.data(), current.size(), read) || current != record)
        return false;
    text.pop_back();
    output = std::move(text);
    return true;
}
} // namespace captureabi
