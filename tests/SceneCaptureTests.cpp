#include "SceneCapture.h"
#include <cstdio>
#include <limits>

int sceneCaptureTests() {
    int failures = 0;
    auto check = [&](bool result, const char *message) {
        if (!result) { std::printf("FAIL scene capture: %s\n", message); ++failures; }
    };
    scenecapture::MeshCache original;
    original.minimum = {-128LL * 65536, -64LL * 65536, -10LL * 65536};
    original.maximum = {-63LL * 65536, 65536, 90LL * 65536};
    original.center = {-95LL * 65536 - 32768, -31LL * 65536 - 32768, 40LL * 65536};
    original.activeGroups = 0x100000001ull;
    original.pendingGroups = 2;
    original.radius = 112.5f;
    original.dirty = 1;
    auto bytes = scenecapture::encode(original);
    auto restored = scenecapture::decode(bytes.data(), bytes.size());
    check(restored && restored->minimum == original.minimum && restored->maximum == original.maximum &&
          restored->center == original.center && restored->activeGroups == original.activeGroups &&
          restored->pendingGroups == 2 && restored->radius == 112.5f && restored->dirty == 1,
          "negative fixed coordinates, full 64-bit group counts and capture state round-trip");
    check(bytes[0] == 1 && bytes[8] == 1 && bytes[12] == 1 && bytes[100] == 1,
          "project schema is little endian and does not truncate the active count");
    bytes[0] = 2;
    check(!scenecapture::decode(bytes.data(), bytes.size()), "unknown format version rejected");
    bytes = scenecapture::encode(original);
    bytes[4] = 0;
    check(!scenecapture::decode(bytes.data(), bytes.size()), "unlocked snapshot is not coherent mesh evidence");
    check(!scenecapture::decode(bytes.data(), bytes.size() - 1), "truncated payload rejected");
    original.activeGroups = original.pendingGroups = 0;
    bytes = scenecapture::encode(original);
    check(!scenecapture::decode(bytes.data(), bytes.size()), "zero counters do not establish initialized bounds");
    original.pendingGroups = 1;
    original.radius = std::numeric_limits<float>::quiet_NaN();
    bytes = scenecapture::encode(original);
    check(!scenecapture::decode(bytes.data(), bytes.size()), "nonfinite radius rejected");
    original.radius = 1;
    original.minimum[2] = original.maximum[2] + 1;
    bytes = scenecapture::encode(original);
    check(!scenecapture::decode(bytes.data(), bytes.size()), "inverted box rejected");
    std::puts("scene capture: coherent versioned metadata, signed positions and full-width counts checked");
    return failures;
}
