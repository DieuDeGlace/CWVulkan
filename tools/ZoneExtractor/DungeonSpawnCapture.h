#pragma once
#include "NativeSpawnCellTrees.h"
#include <sstream>
#include <string>

namespace dungeoncapture {
inline constexpr size_t MaxSpawnCaptureBytes = 256 * 1024;
inline constexpr char SpawnCaptureHeader[] = "CW-DUNGEON-SPAWN-CELLS 1";

struct SpawnCapture {
    captureabi::SpawnCell zone{};
    uint32_t seed = 0;
    int32_t day = 0;
    std::vector<captureabi::SpawnCellRecord> records;
};

inline std::string encodeSpawnCells(const SpawnCapture &capture) {
    std::ostringstream output;
    output << SpawnCaptureHeader << '\n' << capture.zone[0] << ' ' << capture.zone[1] << ' '
           << capture.seed << ' ' << capture.day << ' ' << capture.records.size() << '\n';
    for (const auto &record : capture.records) {
        output << record.id;
        for (const auto *cells : {&record.cells.guards, &record.cells.activation}) {
            output << ' ' << cells->size();
            for (const auto &[x, y] : *cells)
                output << ' ' << x << ' ' << y;
        }
        output << '\n';
    }
    return output.str();
}

inline bool decodeSpawnCells(const std::string &text, SpawnCapture &output) {
    if (text.size() > MaxSpawnCaptureBytes)
        return false;
    std::istringstream input(text);
    std::string header;
    SpawnCapture capture;
    size_t count = 0;
    if (!std::getline(input, header) || header != SpawnCaptureHeader ||
        !(input >> capture.zone[0] >> capture.zone[1] >> capture.seed >> capture.day >> count) ||
        count > captureabi::MaxZoneSpawnRecords)
        return false;
    size_t cellCount = 0;
    for (size_t i = 0; i < count; ++i) {
        captureabi::SpawnCellRecord record;
        if (!(input >> record.id) || (!capture.records.empty() && record.id <= capture.records.back().id))
            return false;
        for (auto *cells : {&record.cells.guards, &record.cells.activation}) {
            size_t size = 0;
            if (!(input >> size) || size > captureabi::MaxSpawnCells || size > captureabi::MaxZoneSpawnCells - cellCount)
                return false;
            cellCount += size;
            for (size_t j = 0; j < size; ++j) {
                captureabi::SpawnCell cell{};
                if (!(input >> cell[0] >> cell[1]) || (!cells->empty() && !(cells->back() < cell)))
                    return false;
                cells->push_back(cell);
            }
        }
        capture.records.push_back(std::move(record));
    }
    input >> std::ws;
    if (!input.eof())
        return false;
    output = std::move(capture);
    return true;
}
} // namespace dungeoncapture
