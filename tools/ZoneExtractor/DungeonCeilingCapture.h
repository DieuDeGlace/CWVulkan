#pragma once
#include "DungeonCapture.h"
#include <sstream>

namespace dungeoncapture {
// Cell X/Y, floor index, local X/Y. The value is the dome flag and final ceiling Z.
using CeilingKey = std::array<int32_t, 5>;
using CeilingValue = std::pair<bool, int32_t>;
using CeilingRows = std::map<CeilingKey, CeilingValue>;
inline constexpr size_t MaxCeilingRows = 16 * 256;
inline constexpr size_t MaxCeilingBytes = 256 * 1024;
inline constexpr char CeilingHeader[] = "CW-DUNGEON-CEILINGS 1";

struct CeilingCapture {
    Point zone{};
    uint32_t seed = 0;
    int32_t day = 0;
    CeilingRows rows;
};

inline std::string encodeCeilings(const CeilingCapture &capture) {
    std::ostringstream output;
    output << CeilingHeader << '\n' << capture.zone.first << ' ' << capture.zone.second << ' '
           << capture.seed << ' ' << capture.day << ' ' << capture.rows.size() << '\n';
    for (const auto &[key, value] : capture.rows)
        output << key[0] << ' ' << key[1] << ' ' << key[2] << ' ' << key[3] << ' ' << key[4] << ' '
               << int(value.first) << ' ' << value.second << '\n';
    return output.str();
}

inline bool decodeCeilings(const std::string &text, CeilingCapture &output) {
    if (text.size() > MaxCeilingBytes)
        return false;
    std::istringstream input(text);
    CeilingCapture capture;
    std::string header;
    size_t count = 0;
    if (!std::getline(input, header) || header != CeilingHeader ||
        !(input >> capture.zone.first >> capture.zone.second >> capture.seed >> capture.day >> count) ||
        count > MaxCeilingRows)
        return false;
    for (size_t i = 0; i < count; ++i) {
        CeilingKey key{};
        int dome = 0;
        int32_t ceiling = 0;
        if (!(input >> key[0] >> key[1] >> key[2] >> key[3] >> key[4] >> dome >> ceiling) ||
            Point{key[0] >> 2, key[1] >> 2} != capture.zone || key[2] >= 0 ||
            key[3] < 0 || key[3] >= 16 || key[4] < 0 || key[4] >= 16 || dome < 0 || dome > 1 ||
            !capture.rows.emplace(key, CeilingValue{dome != 0, ceiling}).second)
            return false;
    }
    input >> std::ws;
    if (!input.eof())
        return false;
    output = std::move(capture);
    return true;
}
} // namespace dungeoncapture
