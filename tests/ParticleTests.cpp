#include "NativeParticles.h"
#include "GameMath.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>

int particleTests() {
    using namespace nativeparticles;
    int failures = 0;
    const auto check = [&](bool ok, const char *what) {
        if (!ok) {
            ++failures;
            std::printf("FAIL particles: %s\n", what);
        }
    };
    const auto burst = [](int32_t style, int32_t count, int64_t x = 0) {
        Burst record{};
        const auto put = [&](size_t offset, const auto &value) { std::memcpy(record.data() + offset, &value, sizeof value); };
        put(0, x);
        put(0x18, 1.0f);
        put(0x1c, 2.0f);
        put(0x20, 3.0f);
        put(0x24, .2f);
        put(0x28, .4f);
        put(0x2c, .6f);
        put(0x30, .3f);
        put(0x34, .5f);
        put(0x38, count);
        put(0x3c, style);
        put(0x40, 2.0f);
        return record;
    };
    const auto air = [](int32_t, int32_t, int32_t) { return uint8_t(0); };
    {
        System system;
        const int draws[]{32767, 0, 16384, 0, 32767, 16384, 0, 32767, 16384, 32767, 0, 16384};
        size_t at = 0;
        const std::array<Burst, 1> records{burst(2, 1)};
        system.append(records, [&] { return draws[(at++) % 12]; });
        const auto &p = system.states().front();
        check(at == 12 && p.position == std::array<int64_t, 3>{1, -32768, 65536}, "Z/Y/X draws and asymmetric unscaled position jitter");
        check(p.velocity.z == 1 && p.velocity.y == 4 && p.velocity.x > 1 && p.velocity.x < 1.001f,
              "velocity spread scales centered draws independently of position");
        check(p.angles.z == 0 && p.angles.y == 360 && p.spin.z == 50 && p.spin.y == -50,
              "angle and spin draw groups retain their axes and bounds");
        check(p.colour == glm::vec4(.2f, .4f, .6f, .3f) && p.age == 0 && !p.collision && p.lifetime == 4000,
              "style2 preserves all four color components and constructor state");
    }
    {
        System system;
        int draws = 0;
        const std::array records{burst(1, 1), burst(5, 1), burst(2, 0), burst(1, -3)};
        system.append(records, [&] { return (++draws == 13) ? 32767 : 499; });
        check(draws == 26 && system.states().size() == 2, "extra draws and nonpositive-count admission");
        check(system.states()[0].colour == glm::vec4(1, 1, 0, 1), "style1 replaces the entire stored RGBA");
        check(system.states()[1].age == 499 && system.states()[1].collision, "style5 has a positive initial age and collision");
    }
    {
        System system;
        int draws = 0;
        const std::array records{burst(4, 1), burst(2, 1)};
        system.append(records, [&] {
            ++draws;
            return 0;
        });
        check(draws == 24 && system.states().size() == 2 && system.states()[0].style == 4 && system.states()[0].age == 0 &&
                  system.states()[0].lifetime == 4000,
              "unimplemented later styles still retain shared expansion draws, order and constructor state");
    }
    {
        Particle p;
        p.style = 2;
        p.velocity = {1, 0, 3};
        p.spin = {10, 20, 30};
        System::step(p, 100, {});
        check(p.age == 100 && p.position == std::array<int64_t, 3>{6553, 0, 0} && p.angles == glm::vec3(2, 4, 6),
              "gravity precedes fixed motion and collision-off angles advance twice");
        p.frozen = true;
        const auto before = p.position;
        System::step(p, 100, {});
        check(p.age == 200 && p.position == before, "freeze does not freeze age");
    }
    {
        Particle p;
        p.style = 5;
        p.collision = true;
        p.position = {-1, 32768, 0};
        p.velocity = {3, 4, 5};
        p.spin = {1, 2, 3};
        int queries = 0;
        System::step(p, 0, [&](int32_t x, int32_t y, int32_t z) {
            check(x == -1 && y == 0 && z == 0, "zero-time collision still narrows/floors physical fixed coordinates");
            ++queries;
            return uint8_t(1);
        });
        check(queries == 3 && p.velocity == glm::vec3(0) && p.spin == glm::vec3(0), "zero-time Z rejection clears velocity and spin");
        p.velocity = {30, 0, 3};
        p.spin = {10, 0, 0};
        p.position = {16384, 0, 0};
        queries = 0;
        System::step(p, 100, [&](int32_t, int32_t, int32_t) { return uint8_t(++queries == 3 ? 1 : 2); });
        check(queries == 12 && p.position[0] == 3 * 65536 + 16384 && p.angles.x == 1 && p.velocity == glm::vec3(0),
              "water passes, Z collision does not recompute cached remaining steps, collision angles advance once");
    }
    {
        System system;
        int draws = 0;
        const std::array records{burst(2, 1, 10 * 65536), burst(2, 1001, 20 * 65536)};
        system.append(records, [&] {
            ++draws;
            return 0;
        });
        check(draws == 1002 * 12 && system.states().size() == 1002, "expansion consumes all draws before any live cap");
        system.update(0, air);
        check(system.states().size() == 1000 && system.states().front().position[0] == 10 * 65536 - 32768,
              "cap discards newest tail rather than old entries");
        system.update(4000, air);
        check(system.states().size() == 1000, "age equal to lifetime remains alive");
        system.update(1, air);
        check(system.states().empty(), "strictly older particles retire after their final movement");
    }
    {
        Particle p;
        p.position = {65536, 2 * 65536, 3 * 65536};
        p.size = .5f;
        const auto m = matrix(p, {-65536, -2 * 65536}, {3, 5, 7});
        check(glm::vec3(m[3]) == glm::vec3(-.75f, -1.25f, 1.25f) && m[0][0] == .5f,
              "full allocation centering preserves unequal header dimensions and scene offsets");
        p.position[0] = std::numeric_limits<int64_t>::max() - 100;
        const auto wrapped = matrix(p, {200, 0}, {0, 0, 0});
        check(wrapped[3].x == float(gamemath::addFixed(p.position[0], 200)) * (1.0f / 65536.0f),
              "scene offset wraps in fixed arithmetic before float conversion");
    }
    std::printf("Ordinary burst particles: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
