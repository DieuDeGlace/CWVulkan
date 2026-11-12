// Game update rules (docs/specs/steam-game-update-complete-phase-schedule-20261112.md).
#include "GameMath.h"
#include <cstdio>

int gameUpdateTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *message) {
        if (!passed) {
            std::printf("FAIL game update: %s\n", message);
            ++failures;
        }
    };
    using gamemath::rescaledHealth;
    check(rescaledHealth(150, 100, 100) == 150, "an unchanged maximum leaves health alone, even above it (phase 27)");
    check(rescaledHealth(50, 100, 200) == 100, "a raised maximum scales health by new/old (phase 27)");
    check(rescaledHealth(150, 200, 100) == 75, "a lowered maximum scales health by new/old (phase 27)");
    check(rescaledHealth(120, 100, 50) == 50, "a scaled result above the new maximum is clamped to it (phase 27)");
    check(rescaledHealth(100, 100, 50) == 50, "full health stays full when the maximum falls (phase 27)");
    check(rescaledHealth(-10, 100, 200) == -20, "health below zero keeps its sign; there is no lower clamp (phase 27)");
    const float ratio = 7.0f / 3.0f;
    check(rescaledHealth(2, 3, 7) == 2 * ratio, "the ratio is taken in binary32 before the multiply (phase 27)");
    using gamemath::frameBudgetSleep;
    check(frameBudgetSleep(1005, 1000, 16) == 11, "a frame below its budget sleeps for the rest of it (section 2.3 step 8)");
    check(frameBudgetSleep(1016, 1000, 16) == 0, "a frame that spent exactly its budget does not sleep (section 2.3 step 8)");
    check(frameBudgetSleep(1040, 1000, 16) == 0, "a frame over its budget does not sleep or carry the excess (section 2.3 step 8)");
    check(frameBudgetSleep(2, 0xFFFFFFF0u, 20) == 2, "the spent time wraps in 32 bits across a tick rollover (section 2.3 step 8)");
    check(frameBudgetSleep(1000, 1000, 0) == 0, "a zero budget never sleeps (section 2.3 step 8)");
    check(frameBudgetSleep(1000, 1000, 33) == 33, "a frame that spent no whole millisecond sleeps the whole budget (section 2.3 step 8)");
    return failures;
}
