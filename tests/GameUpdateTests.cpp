// Game update rules (docs/specs/steam-game-update-complete-phase-schedule-20261112.md).
#include "GameMath.h"
#include "NativeWorldClock.h"
#include <cstdio>
#include <vector>

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
    using gamemath::nightLamp;
    check(nightLamp(0x40, true, 0, 82800000.0f) == 0x41, "a frontend page lights the lamp at 23:00, keeping the other toggles (phase 10)");
    check(nightLamp(0x41, true, 0, 43200000.0f) == 0x40, "a frontend page puts the lamp out at noon (phase 10)");
    check(nightLamp(0, true, 0, 17999998.0f) == 1, "just before 05:00 is night (phase 10)");
    check(nightLamp(1, true, 0, 18000000.0f) == 0, "05:00 itself is day: the night test is strictly below 18,000,000 (phase 10)");
    check(nightLamp(1, true, 0, 68399992.0f) == 0, "the binary32 just below 19:00 is day (phase 10)");
    check(nightLamp(0, true, 0, 68400000.0f) == 1, "19:00 itself is night: at least 68,400,000 (phase 10)");
    check(nightLamp(0, false, 0x9c, 0.0f) == 1, "an eagle flight lights the lamp at midnight outside the frontend (phase 10)");
    check(nightLamp(1, false, 0x9c, 43200000.0f) == 0, "an eagle flight puts the lamp out by day (phase 10)");
    check(nightLamp(1, false, 0x9d, 43200000.0f) == 1, "ability 0x9D does not drive the lamp (phase 10)");
    check(nightLamp(0, false, 0, 0.0f) == 0 && nightLamp(1, false, 0, 43200000.0f) == 1,
          "in ordinary play the lamp is the player's own toggle (phase 10)");
    { // steam-frontend-world-clock §4.2: the frontend pages step the named World at the 10x rate.
        int32_t day = 1;
        float timeOfDay = 36000000.0f, elapsed = 0;
        const bool client = true;
        std::vector<int32_t> notices;
        nativeclock::World w{day, timeOfDay, elapsed, client, true, notices, {}};
        nativeclock::advanceBatch(w, 1000.0);
        check(timeOfDay == 36010000.0f && elapsed == 1000.0f, "one second on the title is ten seconds of the day (10x, 16 ms steps)");
        timeOfDay = 86399000.0f;
        nativeclock::advanceBatch(w, 200.0);
        check(day == 2 && timeOfDay == 1000.0f, "the day wraps past 86,400,000 and the day counter advances");
        const float before = timeOfDay;
        nativeclock::advanceBatch(w, 0.0);
        check(timeOfDay == before && elapsed == 1200.0f, "a zero batch still makes its zero-input call and changes nothing");
    }
    return failures;
}
