#include "CreatureScene.h"
#include "Assets.h"
#include "GameMath.h"
#include "Spawned.h"
#include "World.h"
#include <cstdio>
#include <cstring>
#include <limits>

int creatureSceneTests() {
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        if (!ok) {
            ++failures;
            std::printf("FAIL creature scene: %s\n", message);
        }
    };
    {
        creaturescene::Actor actor;
        actor.physical = {999 * 65536, 0, 0};
        actor.drawn = {65536, 2 * 65536, 3 * 65536};
        actor.body = {1, 2, 3};
        actor.drop = .25f;
        actor.health = 1;
        auto sphere = creaturescene::actorSphere(actor);
        check(sphere.centre == creaturescene::Point{65536, 2 * 65536, 180224} && sphere.radius == 3,
              "normal sphere uses drawn position and step offset, with the corrected body-height minimum");
        actor.body.x = 10;
        check(creaturescene::actorSphere(actor).radius == 6, "width/depth radius can exceed body height");
        actor.body.x = 1;
        actor.race = 0x12e;
        actor.extraHeight = 2;
        sphere = creaturescene::actorSphere(actor);
        check(sphere.centre[2] == 4 * 65536 && sphere.radius == 5.4f,
              "race12e adds its own center/height term without the ordinary step subtraction");
        actor.race = 0;
        actor.drawn[2] = INT64_MAX;
        actor.drop = -1.f / 65536.f;
        check(creaturescene::actorSphere(actor).centre[2] == INT64_MIN, "sphere-center addition wraps in low64 arithmetic");

        creaturescene::View view;
        int queries = 0;
        view.visible = [&](const auto &) {
            ++queries;
            return true;
        };
        check(creaturescene::admit(actor, true, view).has_value() && queries == 1, "ordinary mask0 admits the local pointer");
        view.flags = 1;
        check(!creaturescene::admit(actor, true, view) && queries == 1, "bit01 excludes local before visibility");
        view.flags = 0x80;
        check(!creaturescene::admit(actor, false, view) && queries == 1 && creaturescene::admit(actor, true, view).has_value(),
              "bit80 admits only local");
        view.flags = 0x81;
        const int before = queries;
        check(!creaturescene::admit(actor, true, view) && !creaturescene::admit(actor, false, view) && queries == before,
              "both local filters together admit nothing");
        view.flags = 0;
        actor.health = std::numeric_limits<float>::quiet_NaN();
        check(!creaturescene::admit(actor, false, view) && queries == before, "nonpositive/unordered health precedes visibility");
    }
    {
        scenevisibility::SphereView view;
        view.camera = {INT64_MAX - 128, 0, 0};
        view.offset[0] = gamemath::subtractFixed(0, view.camera[0]);
        scenevisibility::Sphere sphere{{INT64_MIN + 127, 0, 0}, 0}; // exactly256 fixed units past camera
        check(!view.visible(sphere, 0) && view.visible(sphere, .004f),
              "exact fixed camera distance survives large coordinates and wrapped subtraction");
        view.planes[0] = {1, 0, 0, -.01f};
        check(!view.visible(sphere, 100), "planes receive the camera-rebased fixed point before float conversion");
        sphere.radius = .01f;
        check(view.visible(sphere, 100), "plane admission includes radius while distance does not expand by it");
    }
    {
        Renderer renderer;
        renderer.initHeadless(RenderSettings{});
        Assets assets;
        Terrain terrain;
        ExtractedWorld blocks;
        World scene(terrain, assets, renderer, 0, &blocks);
        SpawnedCreatures creatures;
        const auto put = [](uint8_t *record, size_t offset, const auto &value) { std::memcpy(record + offset, &value, sizeof value); };
        for (const int64_t id : {90, 10, 70}) {
            bool made = false;
            auto *record = creatures.networkCreature(id, made);
            check(record && made, "synthetic network record registers");
            if (!record)
                continue;
            put(record, 0xb10, creaturescene::Point{id * 65536, 0, 65536});
            put(record, 0x180, 1.f);
            put(record, 0x84, 1.f);
            put(record, 0x88, 1.f);
            put(record, 0x8c, 2.f);
        }
        std::vector<int64_t> visits;
        creaturescene::View view;
        view.visible = [&](const auto &sphere) {
            visits.push_back(sphere.centre[0] / 65536);
            return visits.back() != 10;
        };
        std::vector<DrawItem> draws;
        creatures.draw(blocks, scene, 0, 0, view, draws);
        check(visits == std::vector<int64_t>{90, 10, 70}, "real draw visits registration order rather than ID or zone order");
        check(creatures.record(90)[0x1500] == 1 && creatures.record(10)[0x1500] == 0 && creatures.record(70)[0x1500] == 1,
              "culled records retain an unprepared cache instead of executing the pose");
        bool made = true;
        auto *updated = creatures.networkCreature(90, made);
        check(updated && !made, "existing network update retains registration");
        if (updated)
            put(updated, 0xb10, creaturescene::Point{91 * 65536, 0, 65536});
        auto *dead = creatures.networkCreature(70, made);
        if (dead)
            put(dead, 0x180, 0.f);
        visits.clear();
        view.visible = [&](const auto &sphere) {
            visits.push_back(sphere.centre[0] / 65536);
            return true;
        };
        creatures.draw(blocks, scene, 0, 0, view, draws);
        check(visits == std::vector<int64_t>{91, 10} && creatures.record(70)[0x1500] == 1,
              "updates retain order, health rejects before visibility, and the preparation marker remains sticky");
        view.flags = 0x80;
        visits.clear();
        creatures.draw(blocks, scene, 0, 0, view, draws);
        check(visits.empty(), "real draw's local-only mask rejects a world with no local player");
    }
    std::printf("Current-scene creature pose admission: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
