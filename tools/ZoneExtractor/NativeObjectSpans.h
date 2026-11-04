#pragma once
// Ordinary supported-image allocation: steam-native-object-allocation-spans.md.
#include <cstddef>

namespace captureabi {
// Storage extent only. Gaps need not be initialized; embedded owning headers
// and child pointers do not become independently owned by copying these bytes.
constexpr size_t CREATURE_STORAGE_BYTES = 0x15f8;
} // namespace captureabi
