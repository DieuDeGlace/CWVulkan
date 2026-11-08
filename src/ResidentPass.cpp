#include "ResidentPass.h"
#include "GameMath.h"
#include <cmath>
#include <algorithm>
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
void orderZones(std::vector<Zone> &zones) {
    std::erase_if(zones, [](const Zone &zone) { return zone.identity == 0; });
    std::sort(zones.begin(), zones.end(), [](const Zone &a, const Zone &b) { return a.identity < b.identity; });
    zones.erase(std::unique(zones.begin(), zones.end(), [](const Zone &a, const Zone &b) { return a.identity == b.identity; }),
                zones.end());
}

std::vector<int64_t> residentsOutsideRetention(std::span<const uint8_t *const> actors) {
    std::vector<const uint8_t *> centres;
    for (const auto *actor : actors)
        if (actor[0x60] == 0)
            centres.push_back(actor);
    std::vector<int64_t> out;
    for (const auto *actor : actors) {
        if (actor[0x60] == 0)
            continue;
        const int64_t x = read<int64_t>(actor, 0x10), y = read<int64_t>(actor, 0x18);
        bool retained = false;
        for (const auto *centre : centres) {
            const double dx = double(gamemath::subtractFixed(x, read<int64_t>(centre, 0x10))) * (1.0 / 65536.0);
            const double dy = double(gamemath::subtractFixed(y, read<int64_t>(centre, 0x18))) * (1.0 / 65536.0);
            if (std::abs(dx) <= 320.0 && std::abs(dy) <= 320.0 && dx * dx + dy * dy <= 102400.0) {
                retained = true;
                break;
            }
        }
        if (!retained)
            out.push_back(read<int64_t>(actor, 8));
    }
    return out;
}

bool selectedForMovement(const uint8_t *record) {
    if (!(read<uint32_t>(record, 0x80) & 0x800000) || !(read<float>(record, 0x180) > 0))
        return false;
    const uint32_t code = read<uint32_t>(record, 0x64);
    return !(record[0x60] == 8 && code >= 244 && code <= 247 && record[0x19c] != 0);
}

void SelectedPointers::collect(const uint8_t *record, uint64_t registration) {
    if (selectedForMovement(record))
        members.emplace_back(read<int64_t>(record, 8), registration);
}

void SelectedPointers::resolve(std::vector<const uint8_t *> &out, const Resolve &lookup) const {
    out.clear();
    if (!lookup)
        return;
    for (const auto &[id, registration] : members) {
        const auto current = lookup(id);
        if (current.record && current.registration == registration)
            out.push_back(current.record);
    }
}

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
