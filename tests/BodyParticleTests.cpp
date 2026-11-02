#include "NativeParticles.h"
#include "NativePose.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>

int bodyParticleTests() {
    using namespace nativeparticles;
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        if (!ok) {
            ++failures;
            std::printf("FAIL body particles: %s\n", message);
        }
    };
    const auto air = [](int32_t, int32_t, int32_t) { return uint8_t(0); };
    const std::array<uint8_t, 6> rgb{10, 20, 30, 40, 50, 60};
    BodyPart part;
    part.dimensions = {2, 1, 1};
    part.rgb = rgb;
    part.transform = glm::scale(glm::translate(glm::mat4(1), {10, 20, 30}), {.25f, .5f, .75f});
    part.tint = {.5f, 1, 2, 1};
    part.gameOffset = {-100 * 65536, -200 * 65536};
    {
        System particles;
        const std::array<int, 12> values{0, 17, 32767, 0, 16384, 0, 0, 23, 0, 32767, 16384, 1};
        size_t cursor = 0;
        particles.prependBody(part, [&] { return values[(cursor++) % values.size()]; });
        const auto &states = particles.states();
        check(cursor == 12 && states.size() == 2, "accepted voxels take six draws apiece");
        if (states.size() == 2) {
            const auto &a = states[0];
            check(a.position == std::array<int64_t, 3>{110 * 65536, 220 * 65536, 30 * 65536} && a.birthPosition == a.position,
                  "corner spawn subtracts Game XY offsets and retains a second initialized tuple");
            check(a.velocity == glm::vec3(-.5f, -.5f, -.75f), "outward velocity uses separately quantized full-allocation center");
            check(a.style == 0 && a.collision && a.age == -17 && a.lifetime == 5000 && a.size == 1,
                  "death constructor retains delay, collision and its five-second lifetime");
            check(a.spin.z == 50 && a.spin.y == -50 && a.angles == glm::vec3(0), "body spin draws run Z/Y/X without random initial angles");
            check(a.colour.r == (10.0f / 255.0f) * .5f && a.colour.b == (30.0f / 255.0f) * 2 && a.colour.a == 1,
                  "raw RGB division precedes caller tint without half-color encoding");
            check(a.extra[0][0] == .25f && a.extra[1][1] == .5f && a.extra[2][2] == .75f && a.extra[3] == glm::vec4(0, 0, 0, 1),
                  "extra matrix copies the prepared basis while clearing only translation XYZ");
            check(states[1].age == -23 && states[1].position[0] == a.position[0] + 16384,
                  "random temporary head insertion followed by live head copies preserves the specified reversal");
        }
        const std::array<uint8_t, 3> nextRgb{70, 80, 90};
        BodyPart next = part;
        next.dimensions = {1, 1, 1};
        next.rgb = nextRgb;
        particles.prependBody(next, [] { return 0; });
        check(particles.states().size() == 3 && particles.states()[0].colour.r == (70.0f / 255.0f) * .5f,
              "later part fragments precede earlier parts");
    }
    {
        BodyPart retained = part;
        retained.dimensions = {1, 1, 1};
        retained.transform = glm::translate(glm::mat4(1), {.25f, 0, 0});
        retained.gameOffset = {-101 * 65536, 0};
        System particles;
        particles.prependBody(retained, [] { return 0; });
        check(particles.states().size() == 1 && particles.states()[0].position[0] == 6635520,
              "retained quarter-block matrix subtracts the current Game origin:101.25, without restoring cached origin minus100");
        retained.gameOffset[0] = -102 * 65536;
        particles.prependBody(retained, [] { return 0; });
        check(particles.states().size() == 2 && particles.states()[0].position[0] == 6701056 &&
                  particles.states()[1].position[0] == 6635520,
              "the next birth uses the changed Game origin while existing fragments retain their fixed positions");
    }
    {
        System particles;
        std::vector<uint8_t> source(2 * 2 * 2 * 3, 100);
        BodyPart ordered;
        ordered.dimensions = {2, 2, 2};
        ordered.rgb = source;
        const std::array<int, 13> values{1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1};
        size_t cursor = 0;
        particles.prependBody(ordered, [&] { return values[(cursor++) % values.size()]; });
        check(cursor == 13 && particles.states().size() == 1 && particles.states()[0].position == std::array<int64_t, 3>{0, 0, 65536},
              "surface traversal is X/Y/Z rather than raw allocation order");
        System interiors;
        source.assign(3 * 3 * 3 * 3, 100);
        ordered.dimensions = {3, 3, 3};
        ordered.rgb = source;
        int draws = 0;
        interiors.prependBody(ordered, [&] {
            ++draws;
            return 1;
        });
        check(draws == 26 && interiors.states().empty(), "interior voxels consume no admission draw");
        const std::array<uint8_t, 6> marker{255, 0, 0, 1, 2, 3};
        ordered.dimensions = {2, 1, 1};
        ordered.rgb = marker;
        draws = 0;
        interiors.prependBody(ordered, [&] {
            ++draws;
            return 1;
        });
        check(draws == 1, "explicit policy0 attachment markers consume no draw");
    }
    {
        System particles;
        particles.prependBody(part, [] { return 0; });
        const auto before = particles.states()[0].position;
        // A nonzero delay remains drawable while the entire first frame is skipped.
        Particle delayed = particles.states()[0];
        delayed.age = -100;
        delayed.velocity = {0, 0, 0};
        delayed.spin = {10, 20, 30};
        System::step(delayed, 80, air);
        check(delayed.position == before && delayed.angles == glm::vec3(0), "negative age delays motion without removing the live record");
        System::step(delayed, 20, air);
        check(delayed.age == 0 && std::abs(delayed.velocity.z + .6f) < 1e-6f && delayed.position[2] < before[2],
              "crossing the delay runs the full integer frame step");
        Particle observed;
        observed.style = 0;
        observed.collision = true;
        observed.lifetime = 5000;
        observed.position = {0, 0, 10 * 65536};
        observed.age = 11;
        observed.velocity = {.3179931640625f, -.03424072265625f, -.7522473335266113f};
        observed.angles = {-.643208146f, .681344092f, .460243523f};
        observed.spin = {-40.2005081f, 42.5840034f, 28.7652187f};
        System::step(observed, 16, air);
        System::step(observed, 16, air);
        check(observed.age == 43 && observed.position == std::array<int64_t, 3>{666, -70, 652273} &&
                  glm::length(observed.angles - glm::vec3(-1.929624438f, 2.044032335f, 1.380730629f)) < 1e-6f,
              "shared body updater matches the existing relative native falling sample");
        for (int frame = 0; frame < 5; ++frame)
            particles.update(1000, air);
        check(particles.states().size() == 2, "body lifetime equality remains alive");
        particles.update(1, air);
        check(particles.states().empty(), "body retirement is strictly after5000");
    }
    {
        Particle fragment;
        fragment.style = 0;
        fragment.collision = true;
        fragment.size = 1;
        fragment.position = {65536, 2 * 65536, 3 * 65536};
        fragment.extra = glm::scale(glm::mat4(1), {-.25f, .5f, .75f});
        const auto transform = matrix(fragment, {0, 0}, {3, 4, 5});
        check(glm::vec3(transform[3]) == glm::vec3(1.375f, 1.0f, 1.125f) && transform[0][0] == -.25f,
              "retained reflected part basis precedes full cube-header centering without a second translation");
    }
    {
        System shared;
        Burst old{};
        const int32_t count = 1000, style = 2;
        std::memcpy(old.data() + 0x38, &count, 4);
        std::memcpy(old.data() + 0x3c, &style, 4);
        const std::array<Burst, 1> oldRecords{old};
        int draws = 0;
        shared.append(oldRecords, [&] {
            ++draws;
            return 0;
        });
        shared.prependBody(part, [&] {
            ++draws;
            return 0;
        });
        const int32_t one = 1;
        std::memcpy(old.data() + 0x38, &one, 4);
        const std::array<Burst, 1> newRecords{old};
        shared.append(newRecords, [&] {
            ++draws;
            return 0;
        });
        check(shared.states().size() == 1003 && draws == 12024, "body and burst emissions all consume draws before the shared cap");
        shared.update(0, air);
        check(shared.states().size() == 1000 && shared.states()[0].style == 0 && shared.states()[1].style == 0 &&
                  shared.states().back().style == 2,
              "body head entries displace old/burst tail entries in the same1000-slot list");
    }
    {
        std::array<uint8_t, nativepose::CREATURE_SIZE> record{};
        const auto put = [&](size_t offset, const auto &value) { std::memcpy(record.data() + offset, &value, sizeof value); };
        for (int slot = 0; slot < 8; ++slot)
            put(0x90 + slot * 2, int16_t(slot + 1));
        for (size_t offset = 0x84; offset <= 0x8c; offset += 4)
            put(offset, 2.0f);
        for (size_t offset = 0xa0; offset <= 0xc0; offset += 4)
            put(offset, 1.0f);
        std::array<nativepose::Model, 9> models{};
        for (size_t i = 1; i < models.size(); ++i) {
            models[i].size[0] = models[i].size[1] = models[i].size[2] = 2;
            models[i].user = reinterpret_cast<const void *>(intptr_t(i));
        }
        nativepose::Models lookup;
        lookup.byId = [&](int id) { return id > 0 && size_t(id) < models.size() ? &models[size_t(id)] : nullptr; };
        nativepose::Call call;
        nativepose::PreparedSprites prepared{};
        nativepose::pose(record.data(), call, lookup, nullptr, &prepared);
        check(prepared[0] == &models[1] && prepared[1] == &models[2] && prepared[2] == &models[5] && prepared[3] == &models[7] &&
                  prepared[4] == &models[3] && prepared[7] == &models[4],
              "pose exposes its cached model bindings independently of emitted draw order");
        call.hide = 2;
        nativepose::pose(record.data(), call, lookup, nullptr, &prepared);
        check(prepared[1] == nullptr && record[0x1500] == 1,
              "hidden prepared parts clear bindings while retaining the lifetime preparation marker");
    }
    std::printf("Shared body-voxel particles: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
