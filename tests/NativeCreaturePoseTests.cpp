#include "../src/Recording.h"
#include "../tools/ZoneExtractor/NativeCreaturePose.h"
#include <chrono>
#include <iostream>

int nativeCreaturePoseTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *label) {
        if (!passed) {
            std::cout << "FAIL native pose capture: " << label << '\n';
            ++failures;
        }
    };
    std::array<uint8_t, 0x1600> creature;
    creature.fill(0xa5); // Adjacent trails/state must never become extra pose matrices.
    const auto set = [&](size_t offset, const auto &value) {
        std::memcpy(creature.data() + 0xc38 + offset, &value, sizeof value);
    };
    const uint32_t clockBits = 0x80000000; // Float negative zero, not an integer timer.
    const int32_t state = 0x61;
    set(8, clockBits);
    set(0x0c, state);
    std::array<uintptr_t, 12> addresses;
    std::array<int32_t, 12> slots;
    for (size_t i = 0; i < addresses.size(); ++i) {
        addresses[i] = 0x100000 + i * 0x300;
        slots[i] = int32_t(i * 7);
    }
    addresses[3] = 0;
    slots[3] = -1;
    set(0xd8, addresses);
    std::array<captureabi::PoseMatrix, 18> matrices;
    for (size_t i = 0; i < matrices.size(); ++i)
        for (size_t j = 0; j < 16; ++j)
            matrices[i][j] = float(i * 16 + j) + 0.25f;
    const uint32_t nanBits = 0x7f812345; // Even signaling-NaN bits are copied without float arithmetic.
    std::memcpy(&matrices[17][15], &nanBits, 4);
    set(0x138, matrices);
    const auto before = creature;
    const captureabi::CreaturePose pose(creature.data());
    check(pose.state() == state && uint32_t(pose.clockBits()) == clockBits && std::signbit(pose.clock()) &&
              pose.sprites() == addresses && pose.sprites().size() == 12,
          "native state, float-clock bits and exactly twelve ordered/null sprite pointers");
    const auto copied = pose.matrices();
    check(copied.size() == 18 && std::memcmp(copied.data(), matrices.data(), sizeof matrices) == 0 &&
              pose.matrix(18) == captureabi::PoseMatrix{},
          "exact eighteen-matrix span preserves float bits and rejects a trail as matrix18");

    recording::Frame frame;
    frame.ms = 1000;
    frame.position[0] = INT64_MIN;
    frame.position[1] = -123;
    frame.position[2] = INT64_MAX;
    frame.flags = -5;
    frame.ability = 12;
    frame.sinceAbility = 3.5f;
    frame.yaw = -0.f;
    frame.velocity[1] = -4.25f;
    std::fill(std::begin(frame.slots), std::end(frame.slots), 88);
    for (auto &matrix : frame.matrices)
        std::fill(std::begin(matrix), std::end(matrix), 99.f);
    captureabi::copyRecordedPose(pose, slots, frame);
    check(frame.state == state && uint32_t(frame.timer) == clockBits && frame.ms == 1000 &&
              frame.position[0] == INT64_MIN && frame.position[2] == INT64_MAX && frame.flags == -5 &&
              frame.ability == 12 && frame.sinceAbility == 3.5f && std::signbit(frame.yaw) &&
              frame.velocity[1] == -4.25f,
          "pose copy retains raw clock bits and all caller-owned frame metadata");
    bool padding = frame.slots[12] == -1 && frame.slots[13] == -1;
    for (size_t i = 0; i < 12; ++i)
        padding &= frame.slots[i] == slots[i];
    for (size_t i = 18; i < 33; ++i)
        for (float value : frame.matrices[i])
            padding &= value == 0 && !std::signbit(value);
    check(padding && std::memcmp(frame.matrices, matrices.data(), sizeof matrices) == 0 && creature == before,
          "legacy surplus slots/matrices are absent/zero without copying adjacent state or mutating the creature");

    const auto bytes = recording::encodePositioned(frame);
    check(bytes.size() == 2236, "CWA2 retains its exact existing per-frame byte count");
    const auto file = std::filesystem::temp_directory_path() /
                      ("native-pose-roundtrip-" +
                       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".cwa");
    {
        std::ofstream output(file, std::ios::binary);
        output.write("CWA2", 4);
        output.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
        frame.ms += 33;
        const auto next = recording::encodePositioned(frame);
        output.write(reinterpret_cast<const char *>(next.data()), next.size());
        output.write("partial", 7); // Retain the existing live-writer partial-frame behavior.
    }
    std::vector<recording::Frame> decoded;
    bool positioned = false;
    check(recording::read(file, decoded, &positioned) && positioned && decoded.size() == 2 && decoded[0].ms == 1000 &&
              decoded[1].ms == 1033 && decoded[0].position[1] == -123 && uint32_t(decoded[0].timer) == clockBits &&
              decoded[0].slots[12] == -1 && std::memcmp(decoded[0].matrices, matrices.data(), sizeof matrices) == 0 &&
              decoded[0].matrices[32][15] == 0,
          "the existing playback reader accepts new padded captures without a format migration");
    std::error_code ignored;
    std::filesystem::remove(file, ignored);
    std::cout << "Native pose capture/legacy wire compatibility: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
