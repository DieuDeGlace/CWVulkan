#pragma once
// Independent native record: creature-pose.md sections1.4,2,3.
#include "../../src/RecordingFormat.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>

namespace captureabi {
constexpr size_t POSE_SPRITES = 12, POSE_MATRICES = 18;
using PoseMatrix = std::array<float, 16>;
class CreaturePose {
    static constexpr size_t ANIMATION = 0xc38;
    const uint8_t *record;
    template <class T> T read(size_t offset) const {
        T value;
        std::memcpy(&value, record + ANIMATION + offset, sizeof value);
        return value;
    }

  public:
    // Borrowed readable span; caller retains creature lifetime and applicable
    // guard through each read. This does not snapshot every animation field.
    explicit CreaturePose(const void *creature) : record(static_cast<const uint8_t *>(creature)) {}
    int32_t state() const { return read<int32_t>(0x0c); }
    float clock() const { return read<float>(8); }
    int32_t clockBits() const { return read<int32_t>(8); }
    std::array<uintptr_t, POSE_SPRITES> sprites() const { return read<std::array<uintptr_t, POSE_SPRITES>>(0xd8); }
    PoseMatrix matrix(size_t index) const {
        return index < POSE_MATRICES ? read<PoseMatrix>(0x138 + index * sizeof(PoseMatrix)) : PoseMatrix{};
    }
    std::array<PoseMatrix, POSE_MATRICES> matrices() const {
        return read<std::array<PoseMatrix, POSE_MATRICES>>(0x138);
    }
};
static_assert(sizeof(PoseMatrix) == 64);
static_assert(0xd8 + POSE_SPRITES * sizeof(uintptr_t) == 0x138);
static_assert(0x138 + POSE_MATRICES * sizeof(PoseMatrix) == 0x5b8);

// Preserve the old CWA2 shape without reading its surplus slots/matrices from
// adjacent native state. The state clock retains its float bits in the legacy
// timer word. Other frame metadata belongs to the caller and is unchanged.
inline void copyRecordedPose(const CreaturePose &pose, const std::array<int32_t, POSE_SPRITES> &slots,
                             recording::Frame &frame) {
    frame.state = pose.state();
    frame.timer = pose.clockBits();
    std::fill(std::begin(frame.slots), std::end(frame.slots), -1);
    std::copy(slots.begin(), slots.end(), std::begin(frame.slots));
    for (auto &matrix : frame.matrices)
        std::fill(std::begin(matrix), std::end(matrix), 0.f);
    const auto matrices = pose.matrices();
    std::memcpy(frame.matrices, matrices.data(), POSE_MATRICES * sizeof(PoseMatrix));
}
} // namespace captureabi
