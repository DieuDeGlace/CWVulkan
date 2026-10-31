#include "ResidentPass.h"
#include "GameMath.h"
#include <cmath>
#include <cstring>

namespace residentpass {
namespace {
template <class T> T read(const uint8_t *record, size_t offset) {
    T value;
    std::memcpy(&value, record + offset, sizeof value);
    return value;
}
Region regionOf(const uint8_t *record) {
    return read<Region>(record, 0x150);
}
} // namespace
void Collection::collect(const uint8_t *record) {
    const int64_t id = read<int64_t>(record, 8);
    ids_.insert(id); // Every nonnull registered actor suppresses duplicates, even dead actors.
    if (record[0x60] != 0)
        return;
    players_.push_back({id, record});
    const uint16_t flags = read<uint16_t>(record, 0x128);
    if (flags & 0x800)
        portals_.insert(regionOf(record));
    if (flags & 0x8000)
        released_.insert(regionOf(record));
}
const uint8_t *Collection::recordOf(const Player &player) const {
    return resolve_ ? resolve_(player.id) : player.record;
}
std::vector<const uint8_t *> Collection::players() const {
    std::vector<const uint8_t *> result;
    result.reserve(players_.size());
    for (const auto &player : players_)
        if (const uint8_t *record = recordOf(player))
            result.push_back(record);
    return result;
}
bool Collection::withinRange(int64_t x, int64_t y, int64_t z) const {
    const std::array<int64_t, 3> point{x, y, z};
    for (const auto &player : players_) {
        const uint8_t *record = recordOf(player);
        if (!record)
            continue;
        const auto position = read<std::array<int64_t, 3>>(record, 0x10);
        std::array<double, 3> delta;
        for (int axis = 0; axis < 3; ++axis)
            delta[axis] = double(gamemath::subtractFixed(point[axis], position[axis])) * (1.0 / 65536.0);
        if (std::abs(delta[0]) > 256.0 || std::abs(delta[1]) > 256.0)
            continue;
        if ((delta[1] * delta[1] + delta[0] * delta[0]) + delta[2] * delta[2] <= 65536.0)
            return true;
    }
    return false;
}
void Collection::releasePetrified(uint8_t *record) const {
    uint32_t flags = read<uint32_t>(record, 0x80);
    if ((flags & 0x400000) && released_.count(regionOf(record))) {
        flags &= ~0x400000u;
        std::memcpy(record + 0x80, &flags, sizeof flags);
    }
}
} // namespace residentpass
