#pragma once

#include "ObjectVisual.h"
#include "worldgen/ZoneRecords.h"
#include <algorithm>
#include <bit>
#include <span>
#include <vector>

namespace objectoracle {
using objectvisual::Placement;

struct Match {
    Placement captured, generated;
    bool turnInitialized;
};

struct Comparison {
    std::vector<Match> matches;
    std::vector<Placement> unmatchedCaptured, unmatchedGenerated;
};

inline Comparison compare(std::vector<Placement> captured, std::span<const worldgen::StaticObject> generated) {
    Comparison out;
    // Claim exact initialized records before allowing an unwritten turn to
    // match the remaining record with the same type, position and scale.
    for (const bool initialized : {true, false}) {
        for (const auto &object : generated) {
            if (object.nativeTurnInitialized != initialized)
                continue;
            const Placement key{object.type, {object.x, object.y, object.z}, std::bit_cast<uint32_t>(object.scale), object.turn};
            const auto match = std::find_if(captured.begin(), captured.end(), [&](const auto &candidate) {
                return key.type == candidate.type && key.position == candidate.position && key.scaleBits == candidate.scaleBits &&
                       (!initialized || key.turn == candidate.turn);
            });
            if (match == captured.end()) {
                out.unmatchedGenerated.push_back(key);
            } else {
                out.matches.push_back({*match, key, initialized});
                captured.erase(match);
            }
        }
    }
    out.unmatchedCaptured = std::move(captured);
    return out;
}
} // namespace objectoracle
