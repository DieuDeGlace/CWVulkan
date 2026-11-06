#pragma once
// Native key and value layout: steam-dungeon-gate-threshold-writers.md.
#include "NativeSpawnCellTrees.h"
#include <map>
#include <sstream>
#include <string>

namespace dungeoncapture {
inline constexpr size_t MaxGateThresholds = 256;
inline constexpr size_t MaxGateCaptureBytes = 16 * 1024;
inline constexpr char GateCaptureHeader[] = "CW-DUNGEON-GATE-THRESHOLDS 1";

struct GateCapture {
    captureabi::SpawnCell zone{};
    uint32_t seed = 0;
    int32_t day = 0;
    std::map<captureabi::SpawnCell, int32_t> thresholds;
};

template <class Read>
bool readGateThresholds(uintptr_t zone, GateCapture &capture, Read read) {
    std::map<captureabi::SpawnCell, int32_t> thresholds;
    if (!captureabi::spawn_tree::visit(zone, 0x4c8, MaxGateThresholds, read, [&](uintptr_t node) {
            captureabi::SpawnCell cell{};
            int32_t threshold = 0;
            if (!captureabi::readField(node, 0x1c, cell.data(), sizeof cell, read) ||
                !captureabi::readField(node, 0x24, &threshold, sizeof threshold, read) ||
                (!thresholds.empty() && !(thresholds.rbegin()->first < cell)))
                return false;
            thresholds.emplace(cell, threshold);
            return true;
        }))
        return false;
    capture.thresholds = std::move(thresholds);
    return true;
}

inline std::string encodeGateThresholds(const GateCapture &capture) {
    std::ostringstream output;
    output << GateCaptureHeader << '\n' << capture.zone[0] << ' ' << capture.zone[1] << ' '
           << capture.seed << ' ' << capture.day << ' ' << capture.thresholds.size() << '\n';
    for (const auto &[cell, threshold] : capture.thresholds)
        output << cell[0] << ' ' << cell[1] << ' ' << threshold << '\n';
    return output.str();
}

inline bool decodeGateThresholds(const std::string &text, GateCapture &output) {
    if (text.size() > MaxGateCaptureBytes)
        return false;
    std::istringstream input(text);
    std::string header;
    GateCapture capture;
    size_t count = 0;
    if (!std::getline(input, header) || header != GateCaptureHeader ||
        !(input >> capture.zone[0] >> capture.zone[1] >> capture.seed >> capture.day >> count) ||
        count > MaxGateThresholds)
        return false;
    for (size_t i = 0; i < count; ++i) {
        captureabi::SpawnCell cell{};
        int32_t threshold = 0;
        if (!(input >> cell[0] >> cell[1] >> threshold) ||
            (!capture.thresholds.empty() && !(capture.thresholds.rbegin()->first < cell)))
            return false;
        capture.thresholds.emplace(cell, threshold);
    }
    input >> std::ws;
    if (!input.eof())
        return false;
    output = std::move(capture);
    return true;
}
} // namespace dungeoncapture
