#pragma once
#include <cmath>

// Observed phase durations for the recorded default-weapon attacks/casts in
// the supported Steam build. Its ability-duration query (RVA 0x4f220) adds
// three integer millisecond phases; haste divisions are truncated separately.
// The mage's 600/1200 ms middle phase is fixed, not divided by haste. These
// timings exclude time spent waiting between clicks in a recording.
inline double nativeAttackDuration(int ability, float haste) {
    if (!std::isfinite(haste) || haste < .001f || haste > 1000) return 0;
    auto scaled = [&](float ms) { return (int)(ms / haste); };
    int ms = 0;
    switch (ability) {
    case 18: ms = scaled(100) + scaled(400) + scaled(500); break;
    case 19: ms = scaled(100) + scaled(200) + scaled(200); break;
    case 20: ms = scaled(100) + scaled(200) + scaled(500); break;
    case 23: ms = scaled(400) + scaled(50) + scaled(100); break;
    case 31: case 33: ms = scaled(800) + 600 + scaled(100); break;
    case 32: case 34: ms = scaled(1600) + 1200 + scaled(100); break;
    case 56: ms = scaled(500) + scaled(500); break;
    case 58: ms = scaled(800) + scaled(300) + scaled(300); break;
    default: return 0;
    }
    return ms / 1000.;
}
