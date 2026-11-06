#pragma once
// Project diagnostic format for steam-dungeon-oracle.md. Decision rows are
// helper queries. Their companion CWZ1 files are fresh generated storage.
#include "GenerationRequest.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <map>
#include <set>
#include <tuple>
#include <vector>

namespace dungeoncapture {
using Point = std::pair<int32_t, int32_t>;
using Plan = std::map<Point, std::set<Point>>;
inline constexpr size_t MaxTargets = 16;
inline constexpr size_t MaxStoredBytes = 256 * 1024 * 1024;
inline constexpr int32_t MaxTargetCell = 400000; // Diagnostic range, including safe native block-coordinate conversion.
inline constexpr uint32_t SourceTimestamp = 0x5d93ab0d;
inline constexpr std::string_view Completed = "CW-DUNGEON-COMPLETE 1\n";
inline constexpr std::string_view CompletedWithSpawnCells = "CW-DUNGEON-COMPLETE 2\n";
inline constexpr std::string_view CompletedWithSpawnKeys = "CW-DUNGEON-COMPLETE 3\n";
inline constexpr std::string_view CompletedWithGateThresholds = "CW-DUNGEON-COMPLETE 4\n";

inline bool plan(std::string_view text, size_t zoneBudget, Plan &output) {
    capturepolicy::GenerationCoordinates targets;
    if (!capturepolicy::parseGenerationRequest(text, MaxTargets, targets))
        return false;
    Plan next;
    for (const auto &[x, y] : targets) {
        if (x < -MaxTargetCell || x > MaxTargetCell || y < -MaxTargetCell || y > MaxTargetCell)
            return false;
        for (int32_t cx = x - 1; cx <= x + 1; ++cx)
            for (int32_t cy = y - 1; cy <= y + 1; ++cy)
                next[{cx >> 2, cy >> 2}].emplace(cx, cy); // Floor-divide signed 16-block cells into 64-block zones.
    }
    if (next.size() > zoneBudget)
        return false;
    output.swap(next);
    return true;
}

struct Context {
    uint32_t timestamp = 0, imageSize = 0, seed = 0;
    int32_t derivedSeed = 0, day = 0, sandX = 0, sandY = 0;
    uint32_t vanillaCandidate = 0; // Mod-file inventory at admission, not active-module attestation.
    bool operator==(const Context &other) const {
        return std::tie(timestamp, imageSize, seed, derivedSeed, day, sandX, sandY, vanillaCandidate) ==
               std::tie(other.timestamp, other.imageSize, other.seed, other.derivedSeed, other.day,
                        other.sandX, other.sandY, other.vanillaCandidate);
    }
    bool operator!=(const Context &other) const { return !(*this == other); }
};
struct Decision {
    Point cell, place, roundedPlace;
    uint32_t big = 0;
    int32_t kind = 0, structure = 0, building = 0;
    uint32_t castle = 0; // Four bytes, in order kind, detail, style and steps.
    int32_t top = 0, floor = 0;
    bool operator==(const Decision &other) const {
        return std::tie(cell, place, roundedPlace, big, kind, structure, building, castle, top, floor) ==
               std::tie(other.cell, other.place, other.roundedPlace, other.big, other.kind, other.structure,
                        other.building, other.castle, other.top, other.floor);
    }
};
struct Zone {
    Context context;
    Point zone;
    std::vector<Decision> decisions;
};

inline constexpr std::array<uint8_t, 8> Header{'C', 'W', 'D', 'H', 1, 1, 1, 0};
// Version 1, decision provenance = helper query, block stage = fresh generation.
inline constexpr size_t PrefixSize = Header.size() + 11 * 4;
inline constexpr size_t RowSize = 13 * 4;

inline std::vector<uint8_t> encode(const Zone &zone) {
    std::vector<uint8_t> bytes(Header.begin(), Header.end());
    const auto word = [&](uint32_t value) {
        for (unsigned shift = 0; shift < 32; shift += 8)
            bytes.push_back(uint8_t(value >> shift));
    };
    const auto point = [&](Point value) { word(uint32_t(value.first)); word(uint32_t(value.second)); };
    const auto &c = zone.context;
    word(c.timestamp); word(c.imageSize); word(c.seed);
    word(uint32_t(c.derivedSeed)); word(uint32_t(c.day)); word(uint32_t(c.sandX)); word(uint32_t(c.sandY));
    word(c.vanillaCandidate);
    point(zone.zone);
    word(uint32_t(zone.decisions.size()));
    for (const auto &d : zone.decisions) {
        point(d.cell); point(d.place); point(d.roundedPlace);
        word(d.big); word(uint32_t(d.kind)); word(uint32_t(d.structure)); word(uint32_t(d.building));
        word(d.castle); word(uint32_t(d.top)); word(uint32_t(d.floor));
    }
    return bytes;
}

inline bool decode(const uint8_t *bytes, size_t size, Zone &output) {
    if (size < PrefixSize || !std::equal(Header.begin(), Header.end(), bytes))
        return false;
    size_t offset = Header.size();
    const auto word = [&] {
        uint32_t value = 0;
        for (unsigned shift = 0; shift < 32; shift += 8)
            value |= uint32_t(bytes[offset++]) << shift;
        return value;
    };
    const auto number = [&] {
        const auto bits = word();
        int32_t value;
        std::memcpy(&value, &bits, sizeof value);
        return value;
    };
    const auto point = [&] { return Point{number(), number()}; };
    Zone zone;
    zone.context = {word(), word(), word(), number(), number(), number(), number(), word()};
    zone.zone = point();
    const uint32_t count = word();
    if (!count || count > 16 || size != PrefixSize + count * RowSize ||
        zone.context.timestamp != SourceTimestamp || !zone.context.imageSize || zone.context.vanillaCandidate > 1)
        return false;
    std::set<Point> seen;
    for (uint32_t i = 0; i < count; ++i) {
        Decision d{point(), point(), point(), word(), number(), number(), number(), word(), number(), number()};
        const auto [x, y] = d.cell;
        if (x < -MaxTargetCell - 1 || x > MaxTargetCell + 1 || y < -MaxTargetCell - 1 || y > MaxTargetCell + 1 ||
            Point{x >> 2, y >> 2} != zone.zone || d.big > 1 || !seen.insert(d.cell).second)
            return false;
        zone.decisions.push_back(d);
    }
    output = std::move(zone);
    return true;
}
} // namespace dungeoncapture
