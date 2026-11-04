#include "../tools/ZoneExtractor/NativeCreatureAppearance.h"
#include <iostream>

namespace {
template <class T> void store(uint8_t *record, size_t offset, const T &value) {
    std::memcpy(record + offset, &value, sizeof value);
}
} // namespace

int nativeCreatureAppearanceTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *label) {
        if (!passed) {
            std::cout << "FAIL creature appearance: " << label << '\n';
            ++failures;
        }
    };
    // Deliberately unaligned Creature address; poison padding and adjacent state.
    std::array<uint8_t, 0x12b> bytes;
    bytes.fill(0xa5);
    auto *record = bytes.data() + 1;
    const std::array<uint8_t, 3> tint{0, 127, 255};
    const captureabi::AppearanceTriple extents{7, 11, 13};
    const std::array<int16_t, 8> models{-1, INT16_MIN, 0, 1, 32767, -2, 3, 4};
    // Documented native constructor bit patterns; separate from physical extents.
    const std::array<uint32_t, 8> scaleBits{0x3f8147ae, 0x3f800000, 0x3f800000, 0x3f7ae148,
                                            0x3f800000, 0x3f733333, 0x3f4ccccd, 0x3f800000};
    const std::array<float, 7> controls{-0.f, 13, -90, 3, 4, 5, 6};
    std::array<captureabi::AppearanceTriple, 6> placements;
    for (size_t i = 0; i < placements.size(); ++i)
        for (size_t j = 0; j < placements[i].size(); ++j)
            placements[i][j] = float(i * 3 + j) - 8.5f;
    store(record, 0x7a, tint);
    store(record, 0x80, uint32_t(0x82400201));
    store(record, 0x84, extents);
    store(record, 0x90, models);
    store(record, 0xa0, scaleBits);
    store(record, 0xc0, 27.f);
    store(record, 0xc4, controls);
    store(record, 0xe0, placements);
    const auto before = bytes;
    const captureabi::CreatureAppearance appearance(record);
    check(appearance.tint() == tint && appearance.flags() == 0x82400201,
          "three tint bytes exclude poisoned padding; flags retain high bits");
    check(appearance.extents() == extents && appearance.modelIds() == models,
          "physical extents stay separate; signed16 IDs retain absent/negative/boundary values");
    const auto scalars = appearance.modelScalars();
    check(std::memcmp(scalars.data(), scaleBits.data(), sizeof scaleBits) == 0 && appearance.scalarC0() == 27.f,
          "eight model scalars retain native float bits and do not absorb unresolved scalar C0");
    const auto capturedControls = appearance.poseControls();
    check(std::memcmp(capturedControls.data(), controls.data(), sizeof controls) == 0 &&
              appearance.placementInputs() == placements,
          "controls retain signed zero and all six distinct placement triples");
    check(bytes == before, "borrowed reads preserve the record, padding and following physical state");
    std::cout << "Native creature appearance views: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
