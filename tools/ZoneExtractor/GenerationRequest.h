#pragma once
// Project capture policy. This is not a native generation or memory-limit rule.
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <deque>
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
