#include "NativeParticles.h"
#include "GameMath.h"
#include "NativeCrt.h"
#include "NativePose.h"
#include "Spawned.h"
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
              "later styles retain shared expansion draws, order and constructor state");
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
    {
        for (int style = 0; style <= 8; ++style) {
            Particle p;
            p.style = style;
            p.velocity = {1, 2, 6};
            p.spin = {10, 20, 30};
            int queries = 0;
            const bool retire = System::step(p, 100, [&](int32_t, int32_t, int32_t) {
                ++queries;
                return uint8_t(0);
            });
            const bool gravity = style == 0 || style == 1 || style == 2 || style == 5 || style == 6;
            const glm::vec3 angles = style == 3 ? glm::vec3(0) : style == 7 ? glm::vec3(1, 2, 3) : glm::vec3(2, 4, 6);
            const std::array<int64_t, 3> position =
                style == 3 ? std::array<int64_t, 3>{0, 0, 0} : std::array<int64_t, 3>{6553, 13107, gravity ? 19660 : 39321};
            check(!retire && queries == 0 && p.age == 100 && p.position == position && p.angles == angles &&
                      p.velocity.z == (gravity ? 3 : 6),
                  "all nine styles retain their distinct free-motion and angular routes");
        }
        Particle unknown;
        unknown.style = 99;
        unknown.velocity = {1, 2, 3};
        check(!System::step(unknown, 100, air) && unknown.age == 100 && unknown.position == std::array<int64_t, 3>{} &&
                  !supportedParticle(unknown),
              "unknown styles age without invented motion or rendering");
    }
    {
        for (const uint8_t type : {uint8_t(7), uint8_t(8), uint8_t(11), uint8_t(12)}) {
            Particle p;
            p.style = 4;
            p.collision = true;
            p.velocity = {1, 2, 3};
            p.spin = {10, 20, 30};
            int queries = 0;
            System::step(p, 100, [&](int32_t, int32_t, int32_t) {
                ++queries;
                return type;
            });
            check(queries == 3 && p.angles == glm::vec3(1, 2, 3) &&
                      (type == 12 ? p.position == std::array<int64_t, 3>{} && p.velocity == glm::vec3(0)
                                  : p.position == std::array<int64_t, 3>{6553, 13107, 19660} && p.velocity == glm::vec3(1, 2, 3)),
                  "style4 passes7/8/11 but rejects12, with one angular advance on collision");
        }
        Particle note;
        note.style = 7;
        note.collision = true;
        note.spin = {10, 20, 30};
        System::step(note, 100, air);
        check(note.angles == glm::vec3(0), "collision-on note has neither angular advance");
    }
    {
        for (const bool collision : {false, true}) {
            Particle p;
            p.style = 6;
            p.collision = collision;
            p.velocity = {1, 2, 0};
            p.spin = {10, 20, 30};
            int queries = 0;
            const bool retire = System::step(p, 100, [&](int32_t x, int32_t y, int32_t z) {
                ++queries;
                check(x == 0 && y == 0 && z == -1, "descending query observes the full advanced XYZ");
                return uint8_t(2);
            });
            check(retire && queries == 1 && p.position == std::array<int64_t, 3>{6553, 13107, -19660} &&
                      p.velocity == glm::vec3(1, 2, -3) && p.spin == glm::vec3(10, 20, 30) && p.angles == glm::vec3(1, 2, 3) && !p.frozen,
                  "descending6 retires in water regardless of collision without rollback or freezing");
        }
        Particle p;
        p.style = 6;
        p.velocity.z = -1;
        int queries = 0;
        check(System::step(p, 0,
                           [&](int32_t, int32_t, int32_t) {
                               ++queries;
                               return uint8_t(2);
                           }) &&
                  queries == 1,
              "descending zero-time6 still queries and retires");
        p.velocity.z = -0.0f;
        queries = 0;
        check(!System::step(p, 0,
                            [&](int32_t, int32_t, int32_t) {
                                ++queries;
                                return uint8_t(2);
                            }) &&
                  queries == 0,
              "negative zero selects the ordinary free route");
        p.velocity = {30, 0, -1};
        p.spin = {10, 0, 0};
        queries = 0;
        check(!System::step(p, 100,
                            [&](int32_t, int32_t, int32_t) {
                                ++queries;
                                return uint8_t(0);
                            }) &&
                  queries == 4 && p.position == std::array<int64_t, 3>{3 * 65536, 0, -26212} && p.angles.x == 1,
              "descending air6 completes quantized joint steps with only one angular advance");
        System system;
        const std::array records{burst(6, 1), burst(3, 1)};
        system.append(records, [] { return 0; });
        system.update(100, [](int32_t, int32_t, int32_t) { return uint8_t(2); });
        check(system.states().size() == 1 && system.states().front().style == 3 && system.states().front().age == 100,
              "collision retirement is deferred until all shared-list records are visited");
    }
    {
        for (const int style : {7, 8}) {
            Particle p;
            p.style = style;
            p.size = .5f;
            p.age = 2000;
            const auto half = matrix(p, {}, {2, 4, 6});
            const float scale = style == 7 ? .0075f : .25f;
            check(half[0][0] == scale && glm::length(glm::vec3(half[3]) - glm::vec3(-scale, -2 * scale, -3 * scale)) < 1e-6f &&
                      p.size == .5f,
                  "notes and shrinking cubes scale geometry using their full header while retaining stored radius");
            p.age = 4000;
            check(supportedParticle(p) && matrix(p, {}, {2, 4, 6})[0][0] == 0,
                  "lifetime equality retains a zero-scale draw rather than inventing an age rejection");
        }
    }
    {
        uint32_t state = 123;
        for (int i = 0; i < 1037; ++i)
            nativecrt::nextRandom(state);
        check(state == 0x4deb0e3e && nativecrt::nextRandom(state) == 12519,
              "CRT transition matches the independently counted World-initialization subphase, not a first-frame seed");
        state = 0xffffffff;
        check(nativecrt::nextRandom(state) == 35 && state == 0x235ac6, "CRT state wraps unsigned32 before extracting bits16..30");
    }
    {
        SpawnedCreatures world, separateRole;
        // Explicit engine initial-state fixture; native first-frame state is unverified.
        for (const int expected : {13289, 23359, 19469, 24737})
            check(world.drawWorldRandom() == expected, "World work advances the explicit engine execution-role stream");
        int draws = 0;
        const auto random = [&] {
            ++draws;
            return world.drawWorldRandom();
        };
        System system;
        const std::array<uint8_t, 3> rgb{10, 20, 30};
        BodyPart body;
        body.dimensions = {1, 1, 1};
        body.rgb = rgb;
        system.prependBody(body, random);
        check(draws == 6 && system.states().size() == 1 && system.states().front().age == -229,
              "admitted body birth continues the preceding World draws");
        const std::array<Burst, 1> records{burst(2, 1)};
        system.append(records, random);
        system.update(0, air);
        check(draws == 18 && system.states().size() == 2, "ordinary expansion continues body draws and update adds none");

        std::array<uint8_t, nativepose::CREATURE_SIZE> record{};
        const auto put = [&](size_t offset, const auto &value) { std::memcpy(record.data() + offset, &value, sizeof value); };
        put(0xc44, int32_t(0x61));
        for (size_t offset = 0x84; offset <= 0x8c; offset += 4)
            put(offset, 2.0f);
        for (size_t offset = 0xa0; offset <= 0xc0; offset += 4)
            put(offset, 1.0f);
        nativepose::Model model;
        model.size[0] = model.size[1] = model.size[2] = 2;
        nativepose::Models models;
        models.byId = [&](int id) { return id >= 0 ? &model : nullptr; };
        nativepose::Sparks sparks;
        nativepose::Call call;
        call.rand = random;
        call.sparks = &sparks;
        nativepose::pose(record.data(), call, models, nullptr);
        const float x = (float(5209) * 20.0f) / 32767.0f - 10.0f;
        const float y = (float(6317) * 20.0f) / 32767.0f - 10.0f;
        check(draws == 30 && sparks.size() == 6 && sparks.front() == std::array<float, 3>{x, y, 20},
              "real state61 pose consumes its twelve shared draws after body/burst births");
        nativepose::pose(record.data(), call, models, nullptr);
        check(draws == 30 && world.drawWorldRandom() == 10850, "later spark updates retain the stream for subsequent frame work");
        check(separateRole.drawWorldRandom() == 13289, "separate engine host/client execution-role states do not merge");
    }
    std::printf("Ordinary burst particles: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
