// Derived arithmetic references from independent drawn-interpolation §§2–8.
#include "DrawnState.h"
#include "GameAngles.h"
#include <array>
#include <cstdio>
#include <cstring>

namespace {
using Record = std::array<uint8_t, 0x2000>;
template <class T> void put(Record &record, size_t offset, T value) {
    std::memcpy(record.data() + offset, &value, sizeof value);
}
template <class T> T get(const Record &record, size_t offset) {
    T value;
    std::memcpy(&value, record.data() + offset, sizeof value);
    return value;
}
} // namespace

int drawnStateTests() {
    int failures = 0;
    auto check = [&](bool pass, const char *what) {
        if (!pass) {
            std::printf("FAIL Drawn state: %s\n", what);
            ++failures;
        }
    };
    Record record{};
    put<float>(record, 0xb58, 100);
    for (int axis = 0; axis < 3; ++axis) {
        put<int64_t>(record, 0x10 + axis * 8, 65536);
        put<float>(record, 0x174 + axis * 4, 1);
    }
    put<float>(record, 0x990, -1);
    drawnstate::update(record.data(), 16);
    check(get<int64_t>(record, 0xb10) == 17143 && get<int64_t>(record, 0xb18) == 17143 && get<int64_t>(record, 0xb20) == 29801,
          "positive control uses quantized XYZ then a second unquantized Z correction");
    check(get<float>(record, 0x6c) == 8.37070083618164f && get<float>(record, 0x990) == 0 && get<float>(record, 0xb58) == 100,
          "raw timer advances by32C, negative step clamps, smoothing control is not consumed");
    check(get<uint32_t>(record, 0xb4c) == 0x3f0067c3 && get<uint32_t>(record, 0xb50) == 0x3f0067c3,
          "kind0 drawn look uses native .05 rate");
    Record negative{};
    negative[0x60] = 3;
    put<float>(negative, 0xb58, .1f);
    for (int axis = 0; axis < 3; ++axis) {
        put<int64_t>(negative, 0x10 + axis * 8, -65536);
        put<float>(negative, 0x174 + axis * 4, 1);
    }
    drawnstate::update(negative.data(), 16);
    check(get<int64_t>(negative, 0xb10) == -17143 && get<int64_t>(negative, 0xb20) == -29801,
          "negative fixed products and Z residual truncate toward zero");
    check(get<uint32_t>(negative, 0xb4c) == 0x3e13c720 && get<float>(negative, 0xb58) == .1f,
          "other kinds use .01 look rate while positive control magnitude does not scale the blend");
    constexpr int64_t base = (int64_t(1) << 55) + 7;
    Record distant{};
    put<float>(distant, 0xb58, 1);
    for (int axis = 0; axis < 3; ++axis) {
        put<int64_t>(distant, 0xb10 + axis * 8, base);
        put<int64_t>(distant, 0x10 + axis * 8, base + 65536);
    }
    drawnstate::update(distant.data(), 16);
    check(get<int64_t>(distant, 0xb10) == base + 17143 && get<int64_t>(distant, 0xb20) == base + 29801,
          "large absolute coordinates retain their low fixed units");
    Record snap{};
    put<int64_t>(snap, 0x10, base);
    put<uint32_t>(snap, 0x28, 0x80000000u);
    put<float>(snap, 0x6c, 7);
    put<float>(snap, 0x990, -1);
    put<float>(snap, 0x174, 1);
    drawnstate::update(snap.data(), 16);
    check(get<int64_t>(snap, 0xb10) == base && get<uint32_t>(snap, 0xb34) == 0x80000000u,
          "nonpositive control copies position and angle words exactly");
    check(get<float>(snap, 0x6c) == 7 && get<float>(snap, 0x990) == -1 && get<uint32_t>(snap, 0xb4c) == 0x3f0067c3,
          "snap branch still blends look without positive-branch timer or step side effects");
    Record zero{};
    put<float>(zero, 0xb58, 1);
    put<float>(zero, 0x28, 180);
    put<float>(zero, 0x990, -1);
    drawnstate::update(zero.data(), 0);
    check(get<float>(zero, 0xb34) == 180 && get<float>(zero, 0x990) == 0 && get<float>(zero, 0xb58) == 1,
          "called zero-time pass can snap opposite angles and clamp negative step");
    check(gameangles::turn(0, 180, 0) == 180 && gameangles::turn(720, 0, 0) == 720,
          "angle helper uses its native opposite-angle decision without normalizing output");
    uint32_t targetWord = 0x431b2877;
    float thresholdTarget;
    std::memcpy(&thresholdTarget, &targetWord, sizeof thresholdTarget);
    check(gameangles::turn(1, thresholdTarget, 0) == thresholdTarget,
          "native widened-float threshold snaps the independently derived pair that literal double -.9 misses");
    std::printf("Drawn state: %s\n", failures ? "FAILED" : "passed");
    return failures;
}
