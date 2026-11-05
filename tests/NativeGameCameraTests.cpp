#include "../tools/ZoneExtractor/NativeGameCamera.h"
#include "../tools/ZoneExtractor/NativeRoleLifetime.h"
#include "../tools/ZoneExtractor/NativeWorldMetadata.h"
#include <array>
#include <cmath>
#include <iostream>

namespace {
int roleDisposals = 0;
void disposeRole(void *value) {
    ++roleDisposals;
    *static_cast<int *>(value) = 9;
}
} // namespace

int nativeGameCameraTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *label) {
        if (!passed) {
            std::cout << "FAIL camera view: " << label << '\n';
            ++failures;
        }
    };
    std::array<uint8_t, 0x1dc1> bytes;
    bytes.fill(0xa5);
    auto *record = bytes.data() + 1; // Unaligned on purpose.
    const auto store = [&](size_t offset, auto value) { std::memcpy(record + offset, &value, sizeof value); };
    constexpr double precise = 1.0000000000009094947;
    store(0x1d80, precise);
    store(0x1d88, -0.0);
    store(0x1d90, 179.25);
    store(0x1d98, -30.0);
    store(0x1da0, 42.125);
    store(0x1da8, -720.5);
    store(0x1db4, -0.f);
    store(0x1db8, 27.75f);
    captureabi::GameCamera camera(record);
    using Axis = captureabi::CameraAxis;
    check(camera.angle(Axis::Pitch) == precise && std::signbit(camera.angle(Axis::Roll)) &&
              camera.angle(Axis::Yaw) == 179.25 && camera.targetAngle(Axis::Pitch) == -30 &&
              camera.targetAngle(Axis::Roll) == 42.125 && camera.targetAngle(Axis::Yaw) == -720.5 &&
              std::signbit(camera.distance()) && camera.requestedDistance() == 27.75f,
          "displayed/target double angles and distinct float distances retain precision and signed zero");
    const auto before = bytes;
    camera.captureAngle(Axis::Yaw, 725.125);
    camera.captureDistance(50.25f);
    bool untouched = true;
    for (size_t i = 0; i < 0x1dc0; ++i) {
        const bool changed = (i >= 0x1d90 && i < 0x1d98) || (i >= 0x1da8 && i < 0x1db0) || (i >= 0x1db4 && i < 0x1dbc);
        if (!changed)
            untouched &= record[i] == before[i + 1];
    }
    check(camera.angle(Axis::Yaw) == 725.125 && camera.targetAngle(Axis::Yaw) == 725.125 &&
              camera.distance() == 50.25f && camera.requestedDistance() == 50.25f && untouched,
          "capture writes neither wrap angles nor clamp distances; retain roll, map-drag and neighboring bytes");
    const auto valid = bytes;
    camera.captureAngle(static_cast<Axis>(3), 12);
    check(bytes == valid, "invalid axis never overwrites neighboring state");
    std::array<uint8_t, 0x549> worldBytes;
    worldBytes.fill(0x5a);
    auto *worldRecord = worldBytes.data() + 1;
    const auto worldStore = [&](size_t offset, auto value) { std::memcpy(worldRecord + offset, &value, sizeof value); };
    worldStore(8, INT32_MIN);
    worldStore(0x0c, -0.f);
    worldStore(0x70, uint64_t(0xabcdef0123456789));
    worldStore(0x220, uint32_t(0xf1234567));
    worldStore(0x244, int32_t(-123));
    captureabi::WorldMetadata metadata(worldRecord);
    uint32_t labelBits;
    const auto label = metadata.seedLabel();
    std::memcpy(&labelBits, &label, sizeof labelBits);
    check(metadata.seedBits() == 0xf1234567 && labelBits == metadata.seedBits() && metadata.day() == INT32_MIN &&
              std::signbit(metadata.time()) && metadata.derivedGenerationValue() == -123,
          "World primary seed bits, derived value, signed day, float time and pointer70 remain distinct");
    const auto worldBefore = worldBytes;
    metadata.setTime(13.5f);
    bool worldUntouched = true;
    for (size_t i = 0; i < 0x548; ++i)
        if (i < 0x0c || i >= 0x10)
            worldUntouched &= worldRecord[i] == worldBefore[i + 1];
    check(metadata.time() == 13.5f && worldUntouched, "time-only write preserves every other World byte");
    const std::array<int32_t, 2> pair{-7, INT32_MIN};
    captureabi::CoordinatePair coordinates;
    std::memcpy(&coordinates, pair.data(), sizeof coordinates);
    check(coordinates.x == -7 && coordinates.y == INT32_MIN, "native coordinate pair retains signed32 order");
    check(captureabi::zoneGenerationSeed(5, 7, 100) == 7273 &&
              captureabi::zoneGenerationSeed(-1, -1, -1) == 0xfffffbfe &&
              captureabi::zoneGenerationSeed(INT32_MIN, INT32_MAX, INT32_MAX) == 0xfffffbff &&
              captureabi::zoneGenerationSeed(INT32_MAX, INT32_MIN, -1) == 0x7ffffffe &&
              captureabi::zoneGenerationSeed(1, 4194304, 9) == 10,
          "zone RNG seed preserves native32 wrap at negative and signed-overflow boundaries");
    int roleStorage = 7;
    roleDisposals = 0;
    {
        const captureabi::RegionRolesLease result(&roleStorage, disposeRole);
        check(roleDisposals == 0 && roleStorage == 7, "role destination retained through normal use");
    }
    check(roleDisposals == 1 && roleStorage == 9,
          "role cleanup invokes native callback without freeing caller storage");
    try {
        const captureabi::RegionRolesLease result(&roleStorage, disposeRole);
        throw 1;
    } catch (int) {
    }
    check(roleDisposals == 2, "role result cleanup survives consumer exception without duplicate disposal");
    std::cout << "Native Game camera/World metadata views: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
