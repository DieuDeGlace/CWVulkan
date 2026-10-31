// Independent frame-order §§3/8: one creature pass, retained IDs, separate depth.
#include "UsePrompt.h"
#include "NativeControl.h"
#include <array>
#include <cstdio>
#include <cstring>

namespace {
using Record = std::array<uint8_t, 0x2000>;
template <class T> void put(Record &record, size_t offset, T value) {
    std::memcpy(record.data() + offset, &value, sizeof value);
}
void setup(Record &record, int64_t id) {
    record.fill(0);
    put<int64_t>(record, 8, id);
    put<int32_t>(record, 0x64, 2);
    put<float>(record, 0x180, 100);
    put<float>(record, 0x6c, 100000);
    put<float>(record, 0x84, 2);
    put<float>(record, 0x88, 2);
    put<float>(record, 0x8c, 2);
}
void drawn(Record &record, int64_t x, int64_t y, int64_t z) {
    put<int64_t>(record, 0xb10, x);
    put<int64_t>(record, 0xb18, y);
    put<int64_t>(record, 0xb20, z);
}
} // namespace

int targetStageTests() {
    int failures = 0;
    auto check = [&](bool pass, const char *what) {
        if (!pass) {
            std::printf("FAIL Target stages: %s\n", what);
            ++failures;
        }
    };
    Record player, friendly, hostile;
    setup(player, 1);
    setup(friendly, 2);
    setup(hostile, 3);
    drawn(friendly, 5 * 65536, 0, 0);
    drawn(hostile, 8 * 65536, 0, 0);
    useprompt::Scene scene;
    scene.player = player.data();
    scene.camera.fixedEye = std::array<int64_t, 3>{0, 0, 0};
    scene.creatureById = [&](int64_t id) -> const uint8_t * { return id == 2 ? friendly.data() : id == 3 ? hostile.data() : nullptr; };
    auto creatures = [&] { scene.creatures = {{friendly.data(), {5, 0, 0}, false}, {hostile.data(), {8, 0, 0}, true}}; };
    creatures();
    int sightCalls = 0;
    scene.sight = [&](glm::vec3, glm::vec3, float) {
        ++sightCalls;
        return true;
    };
    const auto early = useprompt::chooseCreatures(scene);
    check(early.creature == 2 && early.overCreature && early.aimDepth == 8,
          "selected minimum-perpendicular creature differs from hostile minimum depth");
    const int earlyCalls = sightCalls;
    scene.creatures.clear();
    const auto late = useprompt::chooseTargets(scene, early);
    check(late.creature == 2 && late.overCreature && sightCalls == earlyCalls,
          "later pass retains early selection without creature queries or reselection");
    useprompt::Object object;
    object.at = {2, 0, -.5f};
    object.size = {1, 1, 1};
    object.item[0] = 1;
    scene.objects = {object};
    const auto objectTarget = useprompt::chooseTargets(scene, early);
    check(objectTarget.object == 0 && objectTarget.creature == 0 && objectTarget.overCreature,
          "object replacement preserves independent projected-creature byte");
    scene.objects.clear();
    creatures();
    scene.sight = [](glm::vec3, glm::vec3, float) { return false; };
    const auto blocked = useprompt::chooseCreatures(scene);
    check(blocked.overCreature && blocked.creature == 0 && blocked.aimDepth == -1 && blocked.chargeTarget == 0,
          "box is set before failed LOS without inventing creature/depth");
    scene.sight = [](glm::vec3, glm::vec3, float) { return true; };
    player[0x68] = 0x86;
    put<float>(player, 0x6c, 0);
    scene.abilityDuration = 10000;
    const auto healing = useprompt::chooseCreatures(scene);
    check(healing.creature == 2 && healing.aimDepth == 5, "healing producer inverts hostile minimum-depth filter");
    player[0x68] = 0;
    put<float>(player, 0x6c, 100000);
    scene.abilityDuration = 0;
    // sqrtf rounds both perpendicular distances to1; a smaller raw square must not
    // steal the earlier candidate selected in distance order.
    drawn(friendly, 5 * 65536, 65536, 23);
    drawn(hostile, 8 * 65536, 65536, 0);
    scene.creatures = {{friendly.data(), {5, 1, 23.0f / 65536}, true}, {hostile.data(), {8, 1, 0}, true}};
    const auto roundedTie = useprompt::chooseCreatures(scene);
    check(roundedTie.creature == 2, "strict rounded perpendicular-distance tie retains earlier candidate");
    // LOS uses the step-adjusted point; minimum depth still uses raw drawn XYZ.
    drawn(friendly, 0, 0, 5 * 65536);
    put<float>(friendly, 0x990, 1);
    scene.camera.forward = {0, 0, 1};
    scene.camera.right = {-1, 0, 0};
    scene.camera.up = {0, 1, 0};
    scene.creatures = {{friendly.data(), {0, 0, 5}, true}};
    float losZ = -100;
    scene.sight = [&](glm::vec3, glm::vec3 to, float) {
        losZ = to.z;
        return true;
    };
    const auto adjusted = useprompt::chooseCreatures(scene);
    check(adjusted.creature == 2 && adjusted.aimDepth == 5 && losZ == 4,
          "depth uses raw drawn point independently of step-adjusted LOS endpoint");
    {
        nativecontrol::Aim aim;
        aim.rayOrigin[2] = 8 * 65536;
        aim.forward[0] = aim.forward[1] = 0;
        aim.forward[2] = 1;
        aim.wallDistance = 60;
        aim.aimDepth = 20;
        put<int64_t>(player, 0x20, 7 * 65536);
        int64_t point[3];
        nativecontrol::aimPoint(aim, player.data(), point);
        check(point[2] == 20 * 65536, "depth point starts at eye rather than look-at");
        aim.override = true;
        nativecontrol::aimPoint(aim, player.data(), point);
        check(point[2] == 68 * 65536, "nonzero override retains the ordinary clear-ray60 endpoint");
        aim.override = false;
        aim.wallDistance = 5;
        nativecontrol::aimPoint(aim, player.data(), point);
        check(point[2] == 20 * 65536, "creature depth is not clipped against terrain scalar from another origin");
        aim.aimDepth = -1;
        aim.wallDistance = -1;
        nativecontrol::aimPoint(aim, player.data(), point);
        check(point[2] == 108 * 65536, "100-block fallback is reserved for invalid terrain return");
        aim.aimDepth = 20;
        aim.wallDistance = 60;
        put<int64_t>(player, 0x20, 20 * 65536);
        nativecontrol::aimPoint(aim, player.data(), point);
        check(point[2] == 68 * 65536, "zero player-relative forward dot rejects creature snap");
    }
    Record fixedPlayer, fixedCreature;
    setup(fixedPlayer, 4);
    setup(fixedCreature, 5);
    constexpr int64_t originX = int64_t(1) << 45;
    put<int64_t>(fixedPlayer, 0x10, originX);
    put<int64_t>(fixedPlayer, 0x20, 3 * 65536);
    drawn(fixedCreature, originX + 5 * 65536 + 3, 0, 3 * 65536 + 6554);
    put<float>(fixedCreature, 0x990, .1f);
    useprompt::Scene fixedScene;
    fixedScene.player = fixedPlayer.data();
    fixedScene.playerAt = {0, 0, 3};
    fixedScene.camera.eye = {0, 0, 3};
    fixedScene.camera.fixedEye = std::array<int64_t, 3>{originX, 0, 3 * 65536};
    fixedScene.creatures = {{fixedCreature.data(), {5, 0, 3.1f}, true}};
    int fixedCalls = 0, approximateCalls = 0, sphereCalls = 0;
    put<float>(fixedCreature, 0x8c, 8);
    fixedScene.sphereVisible = [&](const auto &point, float radius, float range) {
        ++sphereCalls;
        const bool outer = sphereCalls == 1;
        check(point == useprompt::FixedPoint{originX + 5 * 65536 + 3, 0, 3 * 65536 + (outer ? 6554 : 1)} && radius == (outer ? 8 : 2) &&
                  range == (outer ? 160 : 256),
              "outer/inner queries preserve distinct fixed points, height/width radii and ranges");
        return true;
    };
    fixedScene.sight = [&](glm::vec3, glm::vec3, float) {
        ++approximateCalls;
        return false;
    };
    fixedScene.fixedSight = [&](const auto &from, const auto &to, float limit) {
        ++fixedCalls;
        check(from == *fixedScene.camera.fixedEye && to == std::array<int64_t, 3>{originX + 5 * 65536 + 3, 0, 3 * 65536 + 1} &&
                  limit == 256,
              "LOS retains low fixed units and truncates the negated float step product");
        return true;
    };
    check(useprompt::chooseCreatures(fixedScene).creature == 5 && fixedCalls == 1 && approximateCalls == 0 && sphereCalls == 2,
          "fixed creature LOS replaces rather than duplicates the approximate callback");
    fixedScene.camera.eye.z = 3.1f;
    fixedScene.camera.fixedEye = useprompt::FixedPoint{originX + 111, 0, 3 * 65536 + 6554};
    useprompt::Object fixedObject;
    fixedObject.at = {2, 0, 3};
    fixedObject.size = {1, 1, .2f};
    fixedObject.fixedPosition = useprompt::FixedPoint{originX + 2 * 65536 + 3, 0, 3 * 65536};
    fixedScene.objects = {fixedObject};
    fixedCalls = 0;
    fixedScene.fixedSight = [&](const auto &from, const auto &to, float limit) {
        ++fixedCalls;
        check(from == useprompt::FixedPoint{originX, 0, 3 * 65536} &&
                  to == useprompt::FixedPoint{originX + 2 * 65536 + 3, 0, 3 * 65536 + 6553} && limit == 8,
              "both object LOS calls use physical player and truncated half-height center");
        return true;
    };
    check(useprompt::chooseTargets(fixedScene, {}).object == 0 && fixedCalls == 2 && approximateCalls == 0,
          "nearby and crosshair object LOS remain separate calls");
    fixedScene.objects.clear();
    useprompt::GroundItem fixedGround;
    fixedGround.type = 1;
    fixedGround.at = {2, 0, 3.1f};
    fixedGround.fixedPosition = useprompt::FixedPoint{originX + 2 * 65536 + 5, 7, 3 * 65536 + 6563};
    fixedGround.hasModel = true;
    fixedGround.box = useprompt::boxCorners({1.5f, -.5f, 2.6f}, {1, 1, 1});
    fixedScene.groundItems = {fixedGround};
    fixedCalls = 0;
    fixedScene.fixedSight = [&](const auto &from, const auto &to, float limit) {
        ++fixedCalls;
        check(from == *fixedScene.camera.fixedEye && to == *fixedGround.fixedPosition && limit == 200,
              "ground LOS uses eye and raw item tuple rather than its animated model");
        return false;
    };
    check(useprompt::chooseTargets(fixedScene, {}).groundItem == 0 && fixedCalls == 1 && approximateCalls == 0,
          "blocked crosshair LOS does not add a second LOS gate to nearby ground fallback");
    Record tall;
    setup(tall, 6);
    put<float>(tall, 0x84, 1);
    put<float>(tall, 0x88, 1);
    put<float>(tall, 0x8c, 10);
    put<float>(tall, 0x990, 9);
    drawn(tall, 5 * 65536, 0, 9 * 65536);
    useprompt::Scene tallScene;
    tallScene.player = player.data();
    tallScene.creatures = {{tall.data(), {5, 0, 9}, true}};
    check(useprompt::chooseCreatures(tallScene).creature == 6,
          "outer candidate sphere includes height when the center lies above the camera edge");
    Record chargePlayer, chargeHostile, chargeNeutral;
    setup(chargePlayer, 10);
    setup(chargeHostile, 11);
    setup(chargeNeutral, 12);
    chargeHostile[0x60] = 1;
    chargeNeutral[0x60] = 3;
    drawn(chargeHostile, 256, 0, 0);
    drawn(chargeNeutral, 255, 255, 255);
    useprompt::Scene chargeScene;
    chargeScene.player = chargePlayer.data();
    chargeScene.camera.fixedEye = useprompt::FixedPoint{0, 0, 0};
    auto chargeCandidates = [&](int64_t hostileX, int64_t neutralX, int64_t neutralYZ) {
        drawn(chargeHostile, hostileX, 0, 0);
        drawn(chargeNeutral, neutralX, neutralYZ, neutralYZ);
        chargeScene.creatures = {
            {chargeHostile.data(), {float(hostileX) / 65536, 0, 0}, true},
            {chargeNeutral.data(), {float(neutralX) / 65536, float(neutralYZ) / 65536, float(neutralYZ) / 65536}, false}};
    };
    chargeCandidates(256, 255, 255);
    auto charged = useprompt::chooseCreatures(chargeScene);
    check(charged.creature == 11 && charged.chargeTarget == 12,
          "later neutral can win quantized charge score without winning selected-creature rank");
    chargeCandidates(1, 2, 0);
    check(useprompt::chooseCreatures(chargeScene).chargeTarget == 11, "equal zero charge scores retain the first hostile choice");
    chargeScene.creatures.front().hostileToPlayer = false;
    check(useprompt::chooseCreatures(chargeScene).chargeTarget == 11,
          "charge seeding uses hostility kind rather than the player-relative hostility predicate");
    chargeCandidates(2, 1, 0);
    check(useprompt::chooseCreatures(chargeScene).chargeTarget == 11,
          "earlier closer neutral does not seed charge score before an admitted kind1/7");
    chargePlayer[0x68] = 0x23;
    put<float>(chargePlayer, 0x6c, 100);
    chargeScene.abilityDuration = 100;
    check(useprompt::chooseCreatures(chargeScene).chargeTarget == 11,
          "ability23 hostile exclusion is strict below duration, not inclusive at equality");
    put<float>(chargePlayer, 0x6c, 99);
    check(useprompt::chooseCreatures(chargeScene).chargeTarget == 0, "ability23 excludes hostile charge candidate below duration");
    chargePlayer[0x68] = 0;
    put<float>(chargePlayer, 0x6c, 100000);
    drawn(chargePlayer, 0, 0, 0);
    chargeScene.creatures = {{chargePlayer.data(), {0, 0, 0}, false}, {chargeHostile.data(), {2.0f / 65536, 0, 0}, true}};
    int outerQueries = 0, innerQueries = 0;
    chargeScene.sphereVisible = [&](const auto &, float, float range) {
        if (range == 160)
            ++outerQueries;
        else if (range == 256)
            ++innerQueries;
        return true;
    };
    check(useprompt::chooseCreatures(chargeScene).chargeTarget == 11 && outerQueries == 2 && innerQueries == 1,
          "local player participates in outer candidate admission before the inner skip-self gate");
    std::printf("Target stages: %s\n", failures ? "FAILED" : "passed");
    return failures;
}
