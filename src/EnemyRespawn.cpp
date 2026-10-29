#include "EnemyRespawn.h"
#include "Config.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace enemyrespawn {
Settings Settings::read(const Config &config) {
    Settings out;
    std::string mode = config.text("Creatures", "DeathMode", "Timed");
    std::transform(mode.begin(), mode.end(), mode.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    if (mode == "instance")
        out.mode = Mode::Instance;
    else if (mode == "permanent")
        out.mode = Mode::Permanent;
    const double delay = config.number("Creatures", "RespawnSeconds", 300);
    out.seconds = std::isfinite(delay) ? std::clamp(delay, 1.0, 604800.0) : 300;
    return out;
}

bool isEnemy(int hostility) { return hostility == 1 || hostility == 7; }

void Deaths::killed(int64_t id, std::pair<int, int> homeZone) {
    if (settings.mode != Mode::Permanent)
        pending.emplace(id, Pending{homeZone, settings.seconds});
}

std::vector<int64_t> Deaths::ids() const {
    std::vector<int64_t> result;
    result.reserve(pending.size());
    for (const auto &[id, death] : pending)
        result.push_back(id);
    return result;
}

std::vector<int64_t> Deaths::advance(double seconds) {
    std::vector<int64_t> expired;
    if (settings.mode != Mode::Timed || !std::isfinite(seconds) || seconds <= 0)
        return expired;
    for (auto it = pending.begin(); it != pending.end();) {
        it->second.remaining -= seconds;
        if (it->second.remaining <= 0) {
            expired.push_back(it->first);
            it = pending.erase(it);
        } else
            ++it;
    }
    return expired;
}

std::vector<int64_t> Deaths::unload(std::pair<int, int> zone) {
    std::vector<int64_t> expired;
    if (settings.mode != Mode::Instance)
        return expired;
    for (auto it = pending.begin(); it != pending.end();)
        if (it->second.zone == zone) {
            expired.push_back(it->first);
            it = pending.erase(it);
        } else
            ++it;
    return expired;
}
} // namespace enemyrespawn
