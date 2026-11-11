#pragma once
// Requested and applied World seed/name pairs.
// docs/specs/steam-pending-source-selection-and-world-reconfiguration-20261110.md
// A selection records the requested pair; nothing copies it onto the applied pair.
#include <cstdint>
#include <string>

namespace worldselection {
struct Pair {
    int32_t seed = 123;         // start-up constant; signed 32-bit, copied bit for bit
    std::string name = "world"; // start-up constant
};
// The update's comparison: seeds first, names only when the seeds are equal.
inline bool matches(const Pair &requested, const Pair &applied) {
    return requested.seed == applied.seed && requested.name == applied.name;
}
} // namespace worldselection
