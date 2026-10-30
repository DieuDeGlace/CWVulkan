#include "SteamTransientLights.h"
#include "NativePlayer.h"
#include "NetWorlds.h"
#include <cstring>
#include <iostream>
#include <limits>

int steamTransientLightsTests() {
    int failures = 0;
    auto check = [&](bool good, const char *why) {
        if (!good) { ++failures; std::cerr << "Steam transient lights: " << why << '\n'; }
    };
    using namespace steampoints;
    nativeplayer::Item mixed{};
    mixed[0x9d] = 4;
    mixed[0x20] = 0x80; mixed[0x24] = 0x82; mixed[0x28] = 1; mixed[0x2c] = 0x90;
    const auto glow = spiritGlow(mixed.data());
    const auto expected = (glm::vec4(glm::vec3(1,.5f,.1f) * 1.5f,3) +
                           glm::vec4(glm::vec3(.3f,.5f,1) * 1.5f,3) + glm::vec4(.5f,.5f,.5f,1)) * (1.f / 3.f);
    check(glow == expected, "ordered spirit average includes unknown high materials and excludes ordinary ones");
    mixed[0x9d] = 0;
    check(spiritGlow(mixed.data()) == glm::vec4(0), "empty material list has no glow");

    std::array<nativeplayer::Item, 11> worn{};
    for (int hand : {6,7}) {
        worn[hand] = nativeplayer::item(3, 0, 0, 0, 0, 0, 0);
        worn[hand][0x9d] = 1;
        worn[hand][0x20] = hand == 6 ? 0x80 : 0x82;
    }
    nativeplayer::Who who;
    who.cls = 1;
    auto attacker = nativeplayer::record(who, worn);
    attacker[0x68] = 0x0c; // a two-hand in-use selection
    auto target = nativeplayer::record(who, {});
    std::array<uint8_t, 0x48> number{};
    int64_t attackerId = 10, targetId = 20;
    const Point position{-65537,200000,70000};
    std::memcpy(number.data(), &attackerId, 8);
    std::memcpy(number.data() + 8, &targetId, 8);
    std::memcpy(number.data() + 0x20, position.data(), sizeof position);
    auto lookup = [&](int64_t id) -> const uint8_t * {
        return id == attackerId ? attacker.data() : id == targetId ? target.data() : nullptr;
    };
    TransientLights sources;
    number[0x14] = 1;
    sources.hit(number, lookup);
    check(sources.values().empty(), "critical byte alone does not produce a spirit flash");
    number[0x44] = number[0x46] = 1;
    sources.hit(number, lookup); // damage remains zero: it is not a light admission test
    check(sources.values().size() == 3, "each selected spirit weapon precedes the independent ability flash");
    if (sources.values().size() == 3) {
        check(sources.values()[0].colour == glm::vec3(1,.5f,.1f) * 1.5f &&
              sources.values()[1].colour == glm::vec3(.3f,.5f,1) * 1.5f &&
              sources.values()[2].colour == glm::vec3(.3f,.3f,.5f), "RGB is not multiplied by alpha or damage");
        check(sources.values()[0].radius == 20 && sources.values()[2].radius == 16 &&
              sources.values()[0].position == position, "source radii and unshifted fixed hit position");
    }
    sources.hit(number, [](int64_t) -> const uint8_t * { return nullptr; });
    check(sources.values().size() == 3, "unresolved target produces no flash");
    sources.hit(number, [&](int64_t id) -> const uint8_t * { return id == targetId ? target.data() : nullptr; });
    check(sources.values().size() == 3, "unresolved attacker produces no flash");
    attacker[0x68] = 3; // only the first hand now
    TransientLights later;
    later.hit(number, lookup);
    check(later.values().size() == 2, "weapon selection is read at consumption, not copied from the hit");

    SpawnedCreatures observedWorld;
    ExtractedWorld blocks;
    MirrorSide mirror(observedWorld, blocks);
    std::vector<cwnet::DamageRecord> observed;
    observedWorld.setHitVisualObserver([&](const auto &hit) { observed.push_back(hit); });
    int shown = 0;
    mirror.damageShown = [&](const auto &) { ++shown; };
    mirror.showDamageNumber(number);
    check(observed.size() == 1 && observed[0] == number && shown == 1 && observedWorld.events().damageNumbers.empty(),
          "received visual number notifies once without adding an outgoing hit or replacing the text callback");
    cwnet::GuestEvents guest;
    guest.damageNumbers.push_back(number);
    observedWorld.applyGuestEvents(blocks, guest);
    check(observed.size() == 2 && observedWorld.events().damageNumbers.size() == 1,
          "authoritative input insertion notifies once while retaining the original event list");
    observedWorld.setHitVisualObserver({});
    mirror.showDamageNumber(number);
    check(observed.size() == 2 && shown == 2, "headless worlds need no visual observer");

    auto deathRecord = target;
    int32_t race = 0x90;
    const float body[3] = {2,6,4};
    std::memcpy(deathRecord.data() + 0x64, &race, 4);
    std::memcpy(deathRecord.data() + 0x84, body, sizeof body);
    std::memcpy(deathRecord.data() + 0xb10, position.data(), sizeof position);
    TransientLights deathFlashes;
    int deathVisibleCalls = 0;
    bool admitted = true;
    auto deathVisible = [&](const Point &at, float radius, float range) {
        ++deathVisibleCalls;
        check(radius == 3 && range == 100, "death uses half maximum body extent and fixed range100");
        return admitted;
    };
    deathFlashes.deaths({{1,deathRecord.data()}}, deathVisible);
    check(deathFlashes.values().empty() && deathVisibleCalls == 0, "unprepared pose skips death visibility and light");
    deathRecord[0x1500] = 1;
    admitted = false;
    deathFlashes.deaths({{1,deathRecord.data()}}, deathVisible);
    check(deathFlashes.values().empty() && deathVisibleCalls == 1, "previous renderer rejection suppresses death flash");
    admitted = true;
    auto secondDeath = deathRecord;
    Point secondPosition = position;
    secondPosition[0] += 123;
    std::memcpy(secondDeath.data() + 0xb10, secondPosition.data(), sizeof secondPosition);
    deathFlashes.deaths({{20,deathRecord.data()}, {10,secondDeath.data()}, {20,deathRecord.data()}}, deathVisible);
    check(deathFlashes.values().size() == 2, "death identities deduplicate before unsigned pointer-order traversal");
    if (deathFlashes.values().size() == 2) {
        check(deathFlashes.values()[0].position == secondPosition && deathFlashes.values()[1].position == position &&
              deathFlashes.values()[0].colour == glm::vec3(1,.8f,.5f) && deathFlashes.values()[0].radius == 16 &&
              deathFlashes.values()[0].lifetimeMs == 800, "death order, drawn fixed position, color, radius and lifetime");
    }
    deathFlashes.advance(800);
    check(deathFlashes.values().size() == 2 && deathRecord[0x1500] == 1, "death lifetime equality survives without clearing preparation marker");
    deathFlashes.advance(1);
    check(deathFlashes.values().empty(), "death flashes expire strictly after800ms");
    race = 0x91;
    std::memcpy(deathRecord.data() + 0x64, &race, 4);
    deathFlashes.deaths({{1,deathRecord.data()}}, deathVisible);
    check(deathFlashes.values().empty(), "other races produce no explicit death light");

    sources.advance(16.9);
    sources.advance(.9);
    check(!sources.values().empty() && sources.values()[0].elapsedMs == 16, "integer age discards each frame's fraction");
    sources.advance(384);
    check(sources.values().size() == 3 && sources.values()[0].elapsedMs == 400, "equality survives storage");
    Frame frame;
    frame.terrainRange = 73;
    int visibleCalls = 0;
    frame.visible = [&](const Point &at, float radius, float range) {
        ++visibleCalls;
        return at == position && (radius == 16 || radius == 20) && range == 73;
    };
    std::vector<Light> lights;
    collectTransient(sources.values(), frame, lights);
    check(lights.size() == 3 && visibleCalls == 3 && lights[0].colour == glm::vec3(0),
          "collector retains zero-fade slots and uses each source radius with terrain range");
    sources.advance(1);
    check(sources.values().empty(), "strictly older lights are removed");
    later.advance(1500);
    check(later.values().empty(), "a newly born hit flash expires before first collection on a capped resume frame");
    sources.append({position, {1,1,1}, 16, 0, 1200});
    sources.advance(1500);
    check(sources.values()[0].elapsedMs == 1000, "age uses the 1000-ms double frame cap");
    sources = {};
    sources.append({position, {1,1,1}, 16, std::numeric_limits<int32_t>::max(), 0});
    sources.advance(1);
    check(sources.values().size() == 1 && sources.values()[0].elapsedMs == std::numeric_limits<int32_t>::min(),
          "signed age addition wraps without undefined signed overflow");
    std::cout << "Steam transient lights: " << (failures ? "FAIL\n" : "PASS\n");
    return failures;
}
