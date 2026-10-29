#pragma once
// CubeVulkan's optional enemy lifecycle. Native permanent deaths remain selectable.
// Timers use elapsed host time, independent of the world's day/night clock.
#include <cstdint>
#include <map>
#include <utility>
#include <vector>

class Config;

namespace enemyrespawn {
enum class Mode { Permanent, Instance, Timed };
struct Settings {
    Mode mode = Mode::Timed;
    double seconds = 300;
    static Settings read(const Config &config);
};
bool isEnemy(int hostility);

class Deaths {
  public:
    explicit Deaths(Settings settings = {Mode::Permanent, 300}) : settings(settings) {}
    // Repeated observations of the same corpse do not restart its timer.
    void killed(int64_t id, std::pair<int, int> homeZone);
    bool contains(int64_t id) const { return pending.count(id) != 0; }
    void forget(int64_t id) { pending.erase(id); }
    void clear() { pending.clear(); }
    std::vector<int64_t> advance(double seconds);
    std::vector<int64_t> unload(std::pair<int, int> zone);
    std::vector<int64_t> ids() const;

  private:
    struct Pending {
        std::pair<int, int> zone;
        double remaining;
    };
    Settings settings;
    std::map<int64_t, Pending> pending;
};
} // namespace enemyrespawn
