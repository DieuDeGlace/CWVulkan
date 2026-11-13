#include "NativeWorldClock.h"
#include "NativeAI.h"
#include "GameMath.h"
#include <cstring>

namespace nativeclock {
void advance(World &world, float stepMs) {
    bool accelerated = false;
    for (const nativeai::Creature *actor : world.actors) {
        uint8_t *record = actor->record;
        float health;
        std::memcpy(&health, record + 0x180, sizeof health);
        if (record[0x60] != 0 || record[0x68] != 0x58 || !(health > 0.0f))
            continue;
        accelerated = true;
        if (record == world.local) {
            const float reference = world.maxHealth();
            const float seconds = stepMs * 0.001f;
            const float fraction = seconds * 0.05f;
            const float gain = reference * fraction;
            std::memcpy(&health, record + 0x180, sizeof health);
            health = health + gain;
            std::memcpy(record + 0x180, &health, sizeof health);
            const float limit = world.maxHealth();
            std::memcpy(&health, record + 0x180, sizeof health);
            if (health > limit) {
                health = world.maxHealth();
                std::memcpy(record + 0x180, &health, sizeof health);
            }
        }
    }
    world.elapsedMs = world.elapsedMs + stepMs;
    const float increment = stepMs * (accelerated ? 100.0f : 10.0f);
    world.timeOfDayMs = world.timeOfDayMs + increment;
    if (!world.named)
        world.timeOfDayMs = 32400000.0f;
    constexpr float DAY_MS = 86400000.0f;
    while (world.timeOfDayMs > DAY_MS) {
        world.day = gamemath::add32(world.day, 1);
        world.timeOfDayMs = world.timeOfDayMs - DAY_MS;
        if (!world.client) {
            world.reset();
            world.notices.push_back(0);
        }
    }
}
void advanceBatch(World &world, double cappedMs) {
    double left = cappedMs;
    while (left > 16.0) {
        advance(world, 16.0f);
        left -= 16.0;
    }
    advance(world, (float)left);
}

} // namespace nativeclock
