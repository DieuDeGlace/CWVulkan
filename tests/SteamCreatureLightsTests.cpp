#include "SteamCreatureLights.h"
#include "NativePlayer.h"
#include <cstring>
#include <cmath>
#include <iostream>
#include <limits>
#include <set>

namespace {
template <class T> void put(worldgen::CreatureRecord &record, size_t field, T value) {
    std::memcpy(record.data() + field, &value, sizeof value);
}
}

int steamCreatureLightsTests() {
    int failures = 0;
    auto check = [&](bool good, const char *why) {
        if (!good) { ++failures; std::cerr << "Steam creature lights: " << why << '\n'; }
    };
    using namespace steampoints;
    nativeplayer::Who who;
    who.cls = 3;
    const auto empty = nativeplayer::record(who, {});
    auto base = empty;
    const Point drawn{10*65536,20*65536,30*65536}, physical{65536,2*65536,3*65536};
    put(base, 0xb10, drawn); put(base, 0x10, physical);
    put(base, 0x80, uint32_t(0)); put(base, 0x128, uint16_t(0));
    put(base, 0x180, 100.f); put(base, 0x8c, 4.f); put(base, 0x990, .25f);
    put(base, 0x30, 0.f); put(base, 0xb3c, 0.f); put(base, 0x70, int32_t(0));
    check(abilityRange(400,15) == 256 && abilityRange(200,3) == 96 && abilityRange(70,15) == 70 &&
          abilityRange(0,15) == 0 && abilityRange(100,0x04000000) == 0,
          "ability range preserves wrapped zone units, cap and unclamped scene-range input");
    Frame frame;
    frame.abilityRange = 97;
    int visibilityCalls = 0;
    frame.visible = [&](const Point &, float, float) { ++visibilityCalls; return false; };
    std::set<int> buffs;
    const nativerules::HasBuff has = [&](int type) { return buffs.count(type) != 0; };
    auto collect = [&](const auto &record) {
        std::vector<Light> lights;
        collectCreature(record.data(), has, frame, lights);
        return lights;
    };
    auto lamps = base;
    put(lamps, 0x63c, int32_t(0x14)); put(lamps, 0x6dc, int32_t(0x14));
    put(lamps, 0x128, uint16_t(1)); put(lamps, 0x198, 37.5f); put(lamps, 0x64, int32_t(0xb9));
    buffs.insert(9);
    const auto lights = collect(lamps);
    check(lights.size() == 4 && visibilityCalls == 0, "independent lamp sources ignore geometry/frustum visibility; two hand IDs emit once");
    if (lights.size() == 4) {
        const Point anchor{drawn[0]+32768, drawn[1]+65536, drawn[2]+(int64_t)((.3f*4.f-.25f)*65536.f)};
        check(lights[0].position == anchor && lights[3].position == anchor && lights[0].radius == 37.5f &&
              lights[1].radius == 37.5f && lights[2].radius == 16 && lights[3].radius == 32,
              "native local lamp anchor and live radius stat");
        check(lights[0].colour == glm::vec3(1.2f,1,.5f) && lights[1].colour == glm::vec3(.8f,.7f,.4f),
              "hand and toggle lamps retain separate native colors");
    }
    put(lamps, 0x30, 90.f);
    const auto turned = collect(lamps);
    check(!turned.empty() && turned[0].position[0] < drawn[0] && turned[0].position[1] > drawn[1],
          "lamp uses raw yaw, not unchanged displayed yaw");
    for (float hp : {0.f,-1.f,std::numeric_limits<float>::quiet_NaN()}) {
        put(lamps, 0x180, hp);
        check(collect(lamps).empty(), "only strictly positive health enters the creature loop");
    }
    buffs.clear();
    struct RaceCase { int race; float radius; glm::vec3 colour; bool physical; };
    const RaceCase races[]{
        {0x75,16,{0,.4f,.8f},false}, {0xdd,16,{.8f,.6f,.1f},false},
        {0xde,16,{0,.4f,.8f},false}, {0xdf,16,{0,.8f,.3f},false},
        {0x10e,20,{0,.8f,.3f},false}, {0x103,16,{0,.25f,.5f},false},
        {0x121,16,{.3f,.5f,.6f},true}, {0x87,16,{.1f,.5f,.1f},true},
        {0x88,16,{.1f,.1f,.5f},true}, {0x89,16,{.5f,.1f,.2f},true}, {0x8a,16,{.8f,.8f,.6f},true}
    };
    for (const auto &test : races) {
        auto record = base;
        put(record, 0x64, (int32_t)test.race);
        record[0x19c] = 0;
        const auto got = collect(record);
        Point expected = test.physical ? physical : drawn;
        expected[2] -= 16384;
        if (test.physical) expected[2] += 98304;
        check(got.size() == 1 && got[0].position == expected && got[0].colour == test.colour && got[0].radius == test.radius,
              "race table preserves physical/drawn positions, drop and exact source attributes");
    }
    auto state = base;
    put(state, 0x80, uint32_t(0x100000 | 0x4000000 | 0x200));
    buffs.insert(0x13);
    check(collect(state).size() == 3, "state flag and matching effect are one OR source, followed by independent states");
    buffs.clear();
    put(state, 0x80, uint32_t(0)); put(state, 0x64, int32_t(0xdd)); state[0x19c] = 1;
    check(collect(state).empty(), "alternate byte suppresses its guarded race source");
    put(state, 0x64, int32_t(0x10c));
    check(collect(state).size() == 1, "race10c requires the alternate byte instead");
    state = base; put(state, 0x64, int32_t(0x108)); state[0x19c] = 0;
    state[0x7a] = 255; state[0x7b] = 128; state[0x7c] = 0;
    const auto coloured = collect(state);
    check(coloured.size() == 1 && coloured[0].colour == glm::vec3(.5f,(128.f/255.f)*.5f,0), "race byte-color division precedes half intensity");
    state = base; put(state, 0x64, int32_t(0x116));
    put(state, 0x8c, 3.f/65536.f); put(state, 0x990, .75f/65536.f);
    const auto raised = collect(state);
    check(raised.size() == 1 && raised[0].position[2] == drawn[2]+1, "negative drop and half-body height truncate separately");

    auto cast = base;
    cast[0x68] = 0x68; put(cast, 0x6c, 0.f); put(cast, 0x30, 90.f);
    const auto first = collect(cast);
    check(first.size() == 1 && first[0].position[0] == drawn[0] && first[0].position[1] == drawn[1]+65536,
          "first ability uses displayed yaw rather than raw yaw");
    put(cast, 0x6c, (float)nativerules::abilityDuration(cast.data(), has));
    check(collect(cast).empty(), "first ability gate is strict at duration equality");
    cast = base; cast[0x68] = 0x26;
    const int duration = nativerules::abilityDuration(cast.data(), has);
    const int windup = nativerules::abilityEffectTime(cast.data(), -1, has);
    check(windup > 0 && duration != windup, "windup fixture distinguishes admission and phase durations");
    const float duringWindup = (float)windup * .5f;
    put(cast, 0x6c, duringWindup);
    const auto winding = collect(cast);
    const float angle = (float)((double)(duringWindup * 2.f) * 3.141592653589793 / (double)duration);
    const auto flicker = glm::vec3(.3f,.2f,.1f) * (1.f + .1f * std::cos(angle));
    check(winding.size() == 1 && winding[0].colour == flicker,
          "windup-admitted first ability still divides its double-precision phase by total duration");
    const glm::vec3 castOffset{.25f,.5f,.75f};
    for (int ability : {0x86,0x87}) {
        cast = base; cast[0x68] = (uint8_t)ability; put(cast, 0x6c, 0.f); put(cast, 0xb4c, castOffset);
        frame.visible = [&](const Point &at, float radius, float range) {
            check(at == Point{physical[0]+16384,physical[1]+32768,physical[2]+49152} && radius == 4 && range == 97,
                  "second ability tests physical plus offset without drop, using distinct ability range");
            return true;
        };
        const auto got = collect(cast);
        const Point expected = ability == 0x87 ? Point{physical[0],physical[1],physical[2]-16384} :
            Point{physical[0]+16384,physical[1]+32768,physical[2]+32768};
        check(got.size() == 1 && got[0].position == expected, "ability87 keeps its distinct emitted point");
    }
    frame.visible = [](const Point &, float, float) { return false; };
    check(collect(cast).empty(), "second ability alone uses its sphere admission gate");

    auto equipment = base;
    put(equipment, 0x70, (int32_t)nativerules::comboThreshold(equipment.data()));
    for (size_t field : {0x6d8,0x638,0x598,0x3b8}) {
        equipment[field] = 3;
        equipment[field+0x9d] = 1;
        equipment[field+0x20] = 0x80;
    }
    put(equipment, 0x11f4, glm::vec3(100.25f,200.5f,3));
    put(equipment, 0x1374, glm::vec3(300,400,5));
    put(equipment, 0x1508, int64_t(100*65536)); put(equipment, 0x1510, int64_t(200*65536));
    const auto spirit = collect(equipment);
    check(spirit.size() == 4, "all four spirit equipment rows are independent");
    if (spirit.size() == 4) {
        check(spirit[0].position == Point{16384,32768,3*65536} && spirit[1].position == Point{200*65536,200*65536,5*65536} &&
              spirit[2].position == physical && spirit[3].position == physical, "pose weapon anchors remove only stored XY render origin");
        check(spirit[0].colour == glm::vec3(1.5f,.75f,.15f)*.3f && spirit[1].colour == glm::vec3(1.5f,.75f,.15f)*.5f,
              "equipment glow uses RGB times combo before row factor, not glow alpha");
    }
    std::cout << "Steam creature lights: " << (failures ? "FAIL\n" : "PASS\n");
    return failures;
}
