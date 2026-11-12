#include "ProgressNotices.h"
#include <bit>
#include <cmath>

namespace progressnotices {
namespace {
int32_t wrappingAdd(int32_t a, int32_t b) { return std::bit_cast<int32_t>((uint32_t)a + (uint32_t)b); }
int32_t wrappingSub(int32_t a, int32_t b) { return std::bit_cast<int32_t>((uint32_t)a - (uint32_t)b); }
// Truncation toward zero to signed 32; an out-of-range value gives the indefinite integer.
int32_t truncated32(double value) {
    return value > -2147483649.0 && value < 2147483648.0 ? (int32_t)value : INT32_MIN;
}
int64_t truncated64(float value) {
    return value >= -9223372036854775808.0f && value < 9223372036854775808.0f ? (int64_t)value : INT64_MIN;
}
std::u16string text16(const std::string &narrow) { return std::u16string(narrow.begin(), narrow.end()); }
feattext::Record record(std::wstring text, glm::vec4 colour, float offsetY = -100) {
    feattext::Record r;
    r.text = std::move(text), r.colour = colour, r.offsetY = offsetY, r.fontSize = 16;
    return r;
}
} // namespace

BeforeWorld beforeWorld(const Progress &p) { return {p.gold, p.platinum, p.zone}; }
AfterCamera afterCamera(const Progress &p) { return {totalExperience(p.level, p.experience), p.level}; }

int32_t totalExperience(int32_t level, int32_t experience) {
    int32_t total = experience;
    for (int32_t k = 1; k < level; ++k) {
        const float exponent = (float)(k - 1) * 0.5f;
        total = wrappingAdd(total, truncated32(std::pow(2.0, (double)exponent) * 10.0));
    }
    return total;
}

feattext::Point anchor(const feattext::Point &player, int r1, int r2, int r3) {
    const auto signedUnit = [](int r) {
        const float f = (float)r;
        return (f + f) / 32767.0f - 1.0f;
    };
    const float y = signedUnit(r1), x = signedUnit(r2), z = (float)r3 / 32767.0f;
    const float offset[3] = {x, y, z};
    feattext::Point at;
    for (int axis = 0; axis < 3; ++axis)
        at[axis] = std::bit_cast<int64_t>((uint64_t)player[axis] + (uint64_t)truncated64(offset[axis] * 65536.0f));
    return at;
}

std::vector<Notice> compare(const BeforeWorld &before, const AfterCamera &after, const Progress &now, const feattext::Point &player,
                            const std::function<int()> &rand) {
    std::vector<Notice> out;
    const auto drawn = [&](Notice notice) {
        const int r1 = rand(), r2 = rand(), r3 = rand();
        notice.record.anchor = anchor(player, r1, r2, r3);
        out.push_back(std::move(notice));
    };
    // Phase 89: total XP against the phase-75 snapshot.
    if (const int32_t total = totalExperience(now.level, now.experience); total > after.totalExperience) {
        const int32_t n = wrappingSub(total, after.totalExperience);
        Notice notice{record(L"+" + std::to_wstring(n) + L" XP", {.65f, .4f, 1, 1})};
        notice.chat = u"You gain " + text16(std::to_string(n)) + u" XP.\n", notice.chatColour = {150, 128, 255};
        drawn(std::move(notice));
    }
    // Phase 90: gold against the phase-17 snapshot.
    if (now.gold > before.gold)
        drawn(Notice{record(L"+" + std::to_wstring(wrappingSub(now.gold, before.gold)) + L" GOLD", {1, .9f, .5f, 1})});
    // Phase 91: the current zone's platinum counter, only while P stays in the snapshot's zone.
    if (now.platinum > before.platinum && now.zone == before.zone)
        drawn(Notice{record(L"+" + std::to_wstring(wrappingSub(now.platinum, before.platinum)) + L" PLATINUM", {.8f, .9f, .9f, 1})});
    // Phase 92: level against the phase-75 snapshot; sound 0x1D precedes the record.
    if (now.level > after.level) {
        Notice notice{record(L"LEVEL UP!", {1, .9f, .5f, 1}, -150)};
        notice.sound = 0x1d;
        notice.chat = u"You reached level " + text16(std::to_string(now.level)) + u"!\n", notice.chatColour = {255, 220, 128};
        drawn(std::move(notice));
    }
    return out;
}
} // namespace progressnotices
