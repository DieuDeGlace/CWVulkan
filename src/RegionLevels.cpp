#include "RegionLevels.h"
#include <algorithm>

namespace regionlevels {
namespace {
constexpr int MAX_LEVEL = 1000000;
constexpr uint32_t MAX_ACTIVE_REGIONS = 4096;
int lookup(const Levels &levels, worldgen::Int2 region) {
    auto found = levels.find({region.x, region.y});
    return found == levels.end() ? 1 : found->second;
}
}

void Occupancy::leaveLocked(int64_t player) {
    auto previous = players.find(player);
    if (previous == players.end())
        return;
    auto region = regions.find(previous->second);
    if (region != regions.end()) {
        region->second.players.erase(player);
        if (region->second.players.empty())
            regions.erase(region);
    }
    players.erase(previous);
}

void Occupancy::update(int64_t player, worldgen::Int2 region, int level) {
    if (player <= 0)
        return;
    std::lock_guard hold(mutex);
    const Region key{region.x, region.y};
    auto previous = players.find(player);
    if (previous != players.end() && previous->second == key)
        return;
    leaveLocked(player);
    auto active = regions.try_emplace(key, Active{std::clamp(level, 1, MAX_LEVEL), {}}).first;
    active->second.players.insert(player);
    players[player] = key;
}

void Occupancy::leave(int64_t player) {
    std::lock_guard hold(mutex);
    leaveLocked(player);
}

void Occupancy::unload(worldgen::Int2 region) {
    std::lock_guard hold(mutex);
    auto active = regions.find({region.x, region.y});
    if (active == regions.end())
        return;
    for (int64_t player : active->second.players)
        players.erase(player);
    regions.erase(active);
}

void Occupancy::clear() {
    std::lock_guard hold(mutex);
    players.clear();
    regions.clear();
}

int Occupancy::basis(worldgen::Int2 region) const {
    std::lock_guard hold(mutex);
    auto found = regions.find({region.x, region.y});
    return found == regions.end() ? 1 : found->second.level;
}

Levels Occupancy::snapshot() const {
    std::lock_guard hold(mutex);
    Levels result;
    for (const auto &[key, active] : regions)
        result.emplace(key, active.level);
    return result;
}

std::optional<Region> Occupancy::regionOf(int64_t player) const {
    std::lock_guard hold(mutex);
    auto found = players.find(player);
    return found == players.end() ? std::nullopt : std::optional<Region>(found->second);
}

int Snapshot::basis(worldgen::Int2 region) const { return lookup(levels, region); }

cwnet::Bytes encode(const Snapshot &snapshot) {
    cwnet::Bytes bytes;
    cwnet::ByteWriter writer(bytes);
    writer.value(uint32_t(1));
    writer.value(uint32_t(snapshot.enabled ? 1 : 0));
    writer.value((uint32_t)snapshot.levels.size());
    for (const auto &[region, level] : snapshot.levels) {
        writer.value(region.first);
        writer.value(region.second);
        writer.value((int32_t)level);
    }
    return bytes;
}

std::optional<Snapshot> decode(const cwnet::Bytes &bytes) {
    cwnet::ByteReader reader(bytes);
    uint32_t version = 0, enabled = 0, count = 0;
    if (!reader.value(version) || !reader.value(enabled) || !reader.value(count) || version != 1 || enabled > 1 ||
        count > MAX_ACTIVE_REGIONS || reader.remaining() != (size_t)count * 12)
        return std::nullopt;
    Snapshot result;
    result.enabled = enabled != 0;
    for (uint32_t i = 0; i < count; ++i) {
        int32_t x = 0, y = 0, level = 0;
        if (!reader.value(x) || !reader.value(y) || !reader.value(level) || level < 1 || level > MAX_LEVEL ||
            !result.levels.emplace(Region{x, y}, level).second)
            return std::nullopt;
    }
    if (!result.enabled && !result.levels.empty())
        return std::nullopt;
    return result;
}
} // namespace regionlevels
