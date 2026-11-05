#pragma once
// Project capture policy. This is not a native generation or memory-limit rule.
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <string_view>
#include <utility>

namespace capturepolicy {
class RetainedGenerationBudget {
    size_t attempts = 0;

  public:
    // Failed construction can also retain partial native storage. Count attempts,
    // never return credits, and reset only when the extractor process ends.
    static constexpr size_t MaximumAttempts = 32;
    size_t remaining() const { return MaximumAttempts - attempts; }
    bool beginAttempt() {
        if (!remaining())
            return false;
        ++attempts;
        return true;
    }
};
using GenerationCoordinates = std::deque<std::pair<int32_t, int32_t>>;
inline constexpr size_t MaximumGenerationRequestBytes = 4096;

// Copied capture key, not a native record or an allocation-lifetime pin.
struct GenerationContext {
    uintptr_t worldAddress = 0;
    uint32_t seedBits = 0;
    int32_t derivedSeed = 0, day = 0;
    bool operator==(const GenerationContext &other) const {
        return worldAddress == other.worldAddress && seedBits == other.seedBits && derivedSeed == other.derivedSeed &&
               day == other.day;
    }
};
class GenerationContextGate {
    GenerationContext expected;
    bool bound = false, stopped = false;

  public:
    bool bind(const GenerationContext &context) {
        if (stopped)
            return false;
        expected = context;
        bound = true;
        return true;
    }
    bool allow(const GenerationContext &context) {
        if (!bound || stopped)
            return false;
        if (!(context == expected)) {
            stopped = true;
            return false;
        }
        return true;
    }
    void stop() { stopped = true; }
    bool paused() const { return stopped; }
};
// Immutable local diagnostic receipt. It includes every admitted coordinate in
// order, including duplicates; it is not an automatically replayable gen.request.
inline std::string generationRequestSnapshot(const GenerationContext &context,
                                             const GenerationCoordinates &coordinates) {
    std::string text = "CW-GENERATION-SNAPSHOT 1\nworld-address " + std::to_string(context.worldAddress) +
                       "\nseed-bits " + std::to_string(context.seedBits) + "\nderived-seed " +
                       std::to_string(context.derivedSeed) + "\nday " + std::to_string(context.day) + "\ncoordinates " +
                       std::to_string(coordinates.size()) + '\n';
    for (const auto &[x, y] : coordinates)
        text += std::to_string(x) + ' ' + std::to_string(y) + '\n';
    return text + "END\n";
}

inline bool generationWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f';
}
// Read complete coordinate pairs, preserving the caller's work on any rejection.
// Whitespace-separated signed decimal tokens retain the former request grammar.
inline bool parseGenerationRequest(std::string_view text, size_t maximumPairs, GenerationCoordinates &output) {
    if (text.empty() || text.size() > MaximumGenerationRequestBytes)
        return false;
    const char *next = text.data(), *end = next + text.size();
    const auto skipSpace = [&] {
        while (next != end && generationWhitespace(*next))
            ++next;
    };
    const auto readCoordinate = [&](int32_t &value) {
        skipSpace();
        if (next == end)
            return false;
        if (*next == '+') {
            ++next;
            if (next == end || *next < '0' || *next > '9')
                return false;
        }
        const auto result = std::from_chars(next, end, value);
        if (result.ec != std::errc{} || (result.ptr != end && !generationWhitespace(*result.ptr)))
            return false;
        next = result.ptr;
        return true;
    };
    GenerationCoordinates parsed;
    skipSpace();
    while (next != end) {
        int32_t x, y;
        if (parsed.size() >= maximumPairs || !readCoordinate(x) || !readCoordinate(y))
            return false;
        parsed.emplace_back(x, y);
        skipSpace();
    }
    if (parsed.empty())
        return false;
    output.swap(parsed);
    return true;
}
} // namespace capturepolicy
