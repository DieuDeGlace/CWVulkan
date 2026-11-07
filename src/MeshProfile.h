#pragma once
// Optional owned-engine phase timing. CW_PROFILE_MESH names a private CSV file;
// when unset, no file is opened and no clock is sampled in the mesh path.
#include <array>
#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <thread>

namespace meshprofile {
enum Stage { Load, Bounds, Light, PublishLight, Setup, AlphaLight, Solid, Water, SourceGenerate, AlphaCompose, Refresh, Store, Finish, COUNT };
struct Sink {
    std::ofstream output;
    bool enabled = false;
    std::mutex mutex;
    Sink() {
        const char *path = std::getenv("CW_PROFILE_MESH");
        if (!path || !*path) return;
        output.open(path, std::ios::out | std::ios::trunc);
        if (output) {
            enabled = true;
            output << "scope,thread,zone_x,zone_y,alpha_context,success,lo,hi,cache_version,option_version,original,solid_triangles,lava_triangles,water_triangles,"
                      "load_ms,bounds_ms,light_ms,publish_light_ms,setup_ms,alpha_light_ms,solid_ms,water_ms,source_generate_ms,alpha_compose_ms,refresh_ms,store_ms,finish_ms,total_ms\n"
                   << std::fixed << std::setprecision(3);
        }
    }
};
inline Sink &sink() { static Sink value; return value; }
class Sample {
    using Clock = std::chrono::steady_clock;
  public:
    Sample(int x, int y, bool alpha, const char *scope = "mesh") : scope(scope), x(x), y(y), alpha(alpha) {
        auto &target = sink();
        if (target.enabled) {
            destination = &target;
            started = last = Clock::now();
        }
    }
    void mark(Stage stage) {
        if (!destination) return;
        const auto now = Clock::now();
        milliseconds[stage] += std::chrono::duration<double, std::milli>(now - last).count();
        last = now;
    }
    void context(uint32_t version, uint32_t options, bool original) { key = {version, options, uint32_t(original)}; }
    void complete(int lower, int upper, size_t solid, size_t lava, size_t water) {
        success = true;
        lo = lower; hi = upper;
        triangles = {solid / 3, lava / 3, water / 3};
    }
    ~Sample() {
        if (!destination) return;
        mark(Finish);
        const double total = std::chrono::duration<double, std::milli>(last - started).count();
        std::lock_guard lock(destination->mutex);
        auto &out = destination->output;
        out << scope << ',' << std::hash<std::thread::id>{}(std::this_thread::get_id()) << ',' << x << ',' << y << ',' << alpha << ',' << success
            << ',' << lo << ',' << hi;
        for (auto value : key) out << ',' << value;
        for (auto count : triangles) out << ',' << count;
        for (auto ms : milliseconds) out << ',' << ms;
        out << ',' << total << '\n';
        out.flush();
    }
  private:
    Sink *destination = nullptr;
    const char *scope;
    Clock::time_point started{}, last{};
    std::array<double, COUNT> milliseconds{};
    std::array<size_t, 3> triangles{};
    std::array<uint32_t, 3> key{};
    int x, y, lo = 0, hi = 0;
    bool alpha, success = false;
};
} // namespace meshprofile
