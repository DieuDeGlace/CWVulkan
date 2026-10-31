#include "ResidentEffects.h"
#include "GameMath.h"
#include "NativeCrt.h"
#include <bit>
#include <cmath>
#include <cstring>

namespace residentfx {
namespace {
template <class T> T read(const uint8_t *record, size_t offset) {
    T value;
    std::memcpy(&value, record + offset, sizeof value);
    return value;
}
template <class T> void write(uint8_t *record, size_t offset, const T &value) {
    std::memcpy(record + offset, &value, sizeof value);
}
int32_t centerBlock(int32_t zone) {
    return std::bit_cast<int32_t>(uint32_t(zone) * 64u + 32u);
}
} // namespace
Candidate candidate(int32_t zoneX, int32_t zoneY, int32_t day, const Ground &ground) {
    uint32_t phase = uint32_t(day);
    phase ^= phase << 13;
    phase ^= phase >> 17;
    phase ^= phase << 5;
    const float fraction = float(phase % 10000u) / 10000.0f;
    const float doubled = fraction + fraction;
    const float angle = float(double(doubled) * 3.141592653589793);
    const int32_t x = centerBlock(zoneX), y = centerBlock(zoneY);
    Candidate result;
    result.center = {int64_t(x) * 65536, int64_t(y) * 65536};
    const float height = ground(x, y) + 400.0f;
    result.position[2] = int64_t(height * 65536.0f);
    const float dx = nativecrt::cosf(angle) * 200.0f;
    const float dy = nativecrt::sinf(angle) * 200.0f;
    result.position[0] = gamemath::addFixed(result.center[0], int64_t(dx * 65536.0f));
    result.position[1] = gamemath::addFixed(result.center[1], int64_t(dy * 65536.0f));
    return result;
}
bool suppressed(const Candidate &candidate, const std::list<nativeeffects::Effect> &live) {
    for (const auto &effect : live) {
        if (read<int32_t>(effect.data(), 0x68) != 9)
            continue;
        const double dx = double(gamemath::subtractFixed(candidate.position[0], read<int64_t>(effect.data(), 8))) / 65536.0;
        const double dy = double(gamemath::subtractFixed(candidate.position[1], read<int64_t>(effect.data(), 0x10))) / 65536.0;
        if (std::abs(dx) <= 200.0 && std::abs(dy) <= 200.0 && dx * dx + dy * dy <= 40000.0)
            return true;
    }
    return false;
}
void initialize(nativeeffects::Effect &record, const Candidate &candidate) {
    auto *data = record.data();
    write(data, 0, int64_t(-1));
    write(data, 8, candidate.position);
    write(data, 0x20, std::array<int64_t, 3>{});
    write(data, 0x38, uint64_t(0));
    write(data, 0x40, uint32_t(0));
    const float dx = gamemath::fixedBlocksBetween(candidate.center[0], candidate.position[0]);
    const float dy = gamemath::fixedBlocksBetween(candidate.center[1], candidate.position[1]);
    const float squared = (dx * dx + dy * dy) + 0.0f;
    const float reciprocal = 1.0f / std::sqrt(squared);
    write(data, 0x44, (dx * reciprocal) * 20.0f);
    write(data, 0x48, (dy * reciprocal) * 20.0f);
    write(data, 0x4c, (reciprocal * 0.0f) * 20.0f);
    write(data, 0x54, 80.0f);
    write(data, 0x58, 20.0f);
    write(data, 0x5c, uint64_t(0));
    write(data, 0x64, uint8_t(0));
    write(data, 0x68, int32_t(9));
    write(data, 0x6c, uint8_t(0));
    write(data, 0x70, 0.0f);
    write(data, 0x74, int32_t(20000));
    write(data, 0x78, int32_t(0));
}
bool append(const Candidate &candidate, std::list<nativeeffects::Effect> &live, std::vector<nativeeffects::Effect> &outgoing) {
    if (suppressed(candidate, live))
        return false;
    // Owned storage is defined; this does not assert a native value for unwritten damage/padding.
    nativeeffects::Effect record{};
    initialize(record, candidate);
    live.push_back(record);
    outgoing.push_back(record);
    return true;
}
} // namespace residentfx
