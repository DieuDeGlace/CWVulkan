#pragma once
// Original field contract: steam-gameplay-camera.md section1.
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace captureabi {
enum class CameraAxis : size_t { Pitch, Roll, Yaw };
class GameCamera {
    uint8_t *game;
    template <class T> T read(size_t offset) const {
        T value;
        std::memcpy(&value, game + offset, sizeof value);
        return value;
    }
    template <class T> void write(size_t offset, T value) { std::memcpy(game + offset, &value, sizeof value); }

  public:
    // Borrowed live Game storage; no camera simulation, clamping or lifetime pin.
    explicit GameCamera(void *record) : game(static_cast<uint8_t *>(record)) {}
    double angle(CameraAxis axis) const { return size_t(axis) < 3 ? read<double>(0x1d80 + size_t(axis) * 8) : 0; }
    double targetAngle(CameraAxis axis) const { return size_t(axis) < 3 ? read<double>(0x1d98 + size_t(axis) * 8) : 0; }
    float distance() const { return read<float>(0x1db4); }
    float requestedDistance() const { return read<float>(0x1db8); }
    // Explicit capture controls retain the existing target-before-display writes.
    // These are recording aids, not the native ordinary-input smoothing policy.
    void captureAngle(CameraAxis axis, double degrees) {
        if (size_t(axis) < 3) {
            write(0x1d98 + size_t(axis) * 8, degrees);
            write(0x1d80 + size_t(axis) * 8, degrees);
        }
    }
    void captureDistance(float blocks) {
        write(0x1db8, blocks);
        write(0x1db4, blocks);
    }
};
} // namespace captureabi
