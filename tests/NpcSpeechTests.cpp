#include "NpcSpeech.h"
#include "Hud.h"
#include "Assets.h"
#include <cmath>
#include <iostream>
#include <limits>

int npcSpeechTests() {
    int failures = 0;
    auto check = [&](bool good, const char *why) {
        if (!good) { ++failures; std::cerr << "NPC speech: " << why << '\n'; }
    };
    using namespace npcspeech;
    TalkPlay::Page hi, next, alert, question;
    hi.words = {{L"Hi"}};
    next.words = {{L"Next"}};
    alert.words = {{L"!alert"}};
    question.words = {{L"!question"}};
    Actor speaker;
    speaker.id = 42;
    speaker.body = {2,3,4};
    speaker.health = 1;
    bool resolved = true, modelExists = true;
    int ended = 0, effects = 0, sounds = 0;
    Environment environment;
    environment.localPlayer = 1;
    environment.find = [&](int64_t id) -> std::optional<Actor> {
        return resolved && id == speaker.id ? std::optional<Actor>(speaker) : std::nullopt;
    };
    environment.eachActor = [&](const auto &visit) { visit(speaker); };
    environment.hasModel = [&](int) { return modelExists; };
    environment.project = [](Point point) { return glm::vec2((float)point[0], (float)point[2]); };
    environment.effects = [&](int64_t, int64_t, const auto &, bool) { ++effects; };
    environment.finished = [&](int64_t id) { check(id == 42, "end request retains speaker ID"); ++ended; };
    environment.sound = [&](int sound, const Point &at) {
        check(sound == 0x3c && at == speaker.physical, "emote sound uses world-entry physical position"); ++sounds;
    };
    Playback speech;
    speech.attach(42, 1, {hi,next}, {});
    check(speech.addressedTo(1) && !speech.addressedTo(9), "addressing is distinct from root visibility");
    speech.update(1000, environment);
    speech.update(120, environment); // D120 +1000: equality survives, checked before increment
    speech.update(0, environment);
    check(speech.bubbles()[0].page == 0, "ordinary timeout is strict at D+1000");
    speech.update(1, environment);
    check(speech.bubbles()[0].page == 0, "current elapsed interval does not retroactively expire a page");
    speech.update(16, environment);
    check(speech.bubbles()[0].page == 1 && speech.bubbles()[0].elapsedMs == 16 && effects == 2,
          "timeout discards overshoot and activates the next page before increment");
    speech.repeatUse(42);
    check(speech.bubbles()[0].elapsedMs == 200, "first repeated use completes the prefix budget");
    speech.repeatUse(42);
    check(speech.bubbles()[0].page == 1 && speech.bubbles()[0].elapsedMs == 100200,
          "second repeated use adds skip time without directly advancing");
    speech.update(16, environment);
    check(speech.bubbles().empty() && ended == 1, "normal exhaustion queues one end interaction");

    speech.attach(42,1,{hi},{});
    speech.update(1,environment);
    speech.attach(42,9,{next},{100,200,300});
    check(speech.bubbles().size() == 1 && speech.bubbles()[0].pages.size() == 2 &&
          speech.bubbles()[0].elapsedMs == 1 && speech.bubbles()[0].anchor == Point{100,200,300} && speech.addressedTo(9),
          "attachment appends pages and resets anchor/recipient while preserving playback state");
    speech = {};
    speech.attach(42,1,{hi},{});
    speech.update(1,environment);
    const double halfUlpPlus = 5.960464477539063e-8 + 1e-15;
    speech.update(halfUlpPlus,environment);
    check(speech.bubbles()[0].elapsedMs == std::nextafter(1.f,2.f), "clock adds capped elapsed in double before narrowing");

    speech = {}; effects = sounds = 0;
    speech.attach(42,1,{alert,question,hi},{});
    speech.update(16,environment);
    check(speech.bubbles()[0].page == 1 && speech.emotes().at(42).model == 0xded &&
          speech.emotes().at(42).ageMs == 16 && !speech.bubbles()[0].visible && sounds == 1,
          "one emote per update advances its page, hides text and ages on its birth frame");
    speech.update(16,environment);
    check(speech.bubbles()[0].page == 2 && speech.emotes().at(42).model == 0xdec && sounds == 2,
          "consecutive command pages replace the active model on successive updates");
    speech.update(16,environment);
    speech.update(16,environment);
    check(effects == 4 && speech.bubbles()[0].elapsedMs == 0, "active emote retains literal clock-zero page-effect visits");
    speaker.health = 0;
    speech.update(1000,environment);
    check(speech.emotes().at(42).ageMs == 1048, "dead retained world entries still advance emote age");
    resolved = false;
    speech.update(1,environment);
    check(speech.emotes().at(42).ageMs == 1049 && speech.bubbles()[0].elapsedMs == 1,
          "emote timer is independent of ID-map lookup; missing speaker permits bubble clock increment");
    resolved = true;
    speech = {};
    speech.attach(42,1,{alert},{});
    speech.update(16,environment);
    speech.update(16,environment);
    check(speech.bubbles().empty() && speech.emotes().at(42).ageMs == 32,
          "final emote page removes bubble next update without deleting the creature's model");
    speech.update(1000,environment);
    speech.update(968,environment);
    check(speech.emotes().at(42).ageMs == 2000, "emote survives at exactly2000ms");
    speech.update(1,environment);
    check(speech.emotes().empty(), "emote clears strictly after2000ms");
    modelExists = false;
    speech.attach(42,1,{alert,hi},{});
    speech.update(16,environment);
    check(speech.emotes().empty() && speech.bubbles()[0].page == 1 && speech.bubbles()[0].elapsedMs == 16,
          "missing emote asset advances command page without an invented wait or replacement model");
    speech = {};
    speaker.drawn = {-1,0,0};
    speaker.drop = 0;
    speech.attach(42,1,{hi},{});
    speech.update(16,environment);
    check(speech.bubbles()[0].anchor[0] == 0, "fixed smoothing divides a negative sub-unit product toward zero");

    Projection projection;
    projection.perspective[3][3] = 0;
    projection.perspective[2][3] = 1;
    check(project({65536,-65536,2*65536},projection,{800,600},1) == glm::vec2(600,450),
          "positive-forward projection performs the native two homogeneous transforms");
    check(!project({0,0,0},projection,{800,600},1) && !project({0,0,-1},projection,{800,600},1),
          "view-Z zero and behind-camera bubbles are hidden");
    check(project({0,2*65536,2*65536},projection,{800,600},1.5f) == glm::vec2(400,270),
          "screen Y clamp uses GUI scale before literal template placement");
    check(project({0,2*65536,2*65536},projection,{800,600},4) == glm::vec2(400,600),
          "native upward then downward clamp handles minimum above viewport height");
    Projection cancellation;
    cancellation.view[0][0] = 1e20f; cancellation.view[1][0] = -1e20f;
    cancellation.view[2][0] = 3; cancellation.view[3][0] = 4;
    cancellation.view[3][3] = 2;
    check(project({65536,65536,65536},cancellation,{800,600},1)->x == 1800,
          "native product/add order and shared reciprocal retain cancellation result and unclipped X");

    check(emotePose({0xded,0}).scale == 0 && emotePose({0xded,200}).scale == 1 &&
          emotePose({0xded,1800}).scale == 1 && emotePose({0xded,2000}).scale == 0,
          "emote entrance/exit are geometric scale with strict branch endpoints");
    check(emotePose({0xdee,249}).model == 0xdee && emotePose({0xdee,250}).model == 0xdef &&
          emotePose({0xdef,500}).model == 0xdee, "angry swaps submitted resources without changing retained selection");
    check(emotePose({0xdf1,1000}).bounce == 0, "sweat drifts independently of geometric entrance");
    speaker.physical = {65536,2*65536,3*65536}; speaker.drawn = {90*65536,0,0};
    speaker.body = {2,3,4}; speaker.drop = .25f; speaker.yaw = 0;
    const auto matrix = emoteTransform(speaker,emotePose({0xded,200}),{10,20,30},{-65536,0,0});
    const auto centre = matrix * glm::vec4(5,10,15,1);
    check(centre == glm::vec4(0,2,5.75f,1) && glm::length(glm::vec3(matrix[0])) == .08f,
          "emote uses physical anchor, full header centering and fixed nominal width0.8");
    const auto ordinary = actorSphere(speaker);
    check(ordinary.centre == Point{90*65536,0,-16384} && ordinary.radius == 4,
          "emote admission uses actor's dropped drawn anchor and shared body sphere");
    speaker.race = 0x12e; speaker.extraHeight = 10;
    const auto exceptional = actorSphere(speaker);
    check(exceptional.centre[2] == 5*65536 && exceptional.radius == 16, "race12e retains its separate actor sphere exception");
    plasma::Widget oldWidget;
    oldWidget.bindPos = {4,7}; oldWidget.bindSize = {200,150};
    oldWidget.framePos = {12,18}; oldWidget.frameSize = {250,200};
    oldWidget.innerBindPos = {20,30}; oldWidget.innerBindSize = {160,100};
    oldWidget.minimumSize = {40,50}; oldWidget.flags = 0x10c;
    oldWidget.horizontalAlignment = oldWidget.verticalAlignment = 2;
    const auto fresh = plasma::reboundWidget(oldWidget);
    check(fresh.effectiveSize() == oldWidget.effectiveSize() && fresh.innerPosition() == oldWidget.innerPosition() &&
          fresh.innerSize() == oldWidget.innerSize() && fresh.framePos == oldWidget.framePos &&
          fresh.flags == 0 && fresh.horizontalAlignment == 0 && fresh.verticalAlignment == 0,
          "fresh speech widget keeps live frame/inner geometry without inherited behavior flags");
    oldWidget.frameSize = {10, 20};
    oldWidget.minimumSize = {400, 300};
    const auto constrained = plasma::reboundWidget(oldWidget);
    check(constrained.frameSize == glm::vec2(10, 20) && constrained.bindSize == glm::vec2(10, 20) &&
          constrained.innerBindSize == glm::vec2(-30, -30) && constrained.minimumSize == glm::vec2(40, 50),
          "attachment transfers raw stored geometry and derives its own minimum without old clamps");
    Assets assets;
    if (assets.locate("")) {
        Hud hud;
        std::string error;
        if (hud.load(assets.gameDir(),error)) {
            Renderer renderer;
            renderer.initHeadless(RenderSettings{});
            hud.setGuiScale(1);
            hud.layout(renderer,{1000,800});
            Bubble bubble;
            bubble.speaker = 42; bubble.pages = {hi}; bubble.visible = true;
            bubble.elapsedMs = 80; bubble.screenPoint = {500,600};
            bubble.pages[0].words[0].colour = {1,0,0,1};
            std::vector<OverlayVertex> vertices;
            glm::vec2 frame(0);
            int draws = 0;
            auto seen = [&](int64_t id, const dialoguepanel::Layout &layout, glm::vec2 size) {
                check(id == 42 && layout.runs.size() == 1 && layout.runs[0].word.text == L"Hi", "owner template renders native word prefix");
                frame = size; ++draws;
            };
            hud.drawNpcSpeech({bubble},0,vertices,seen);
            check(draws == 1 && frame.x > 0 && frame.y > 0 && !vertices.empty(), "owner speech template supplies effective frame and actual geometry");
            const auto firstFrame = frame;
            vertices.clear(); hud.setGuiScale(1.5f);
            hud.drawNpcSpeech({bubble},0,vertices,seen);
            check(draws == 2 && frame == firstFrame, "GUI scale does not turn speech wrap width into a screen AABB");
            bubble.visible = false;
            vertices.clear(); hud.drawNpcSpeech({bubble},0,vertices,seen);
            check(draws == 2 && vertices.empty(), "hidden emote bubble retains its node without painting text or background");
            hud.drawNpcSpeech({},0,vertices,seen);
            bubble.visible = true;
            hud.drawNpcSpeech({bubble},0,vertices,seen);
            check(draws == 3 && frame == firstFrame, "destroyed speech roots can be recreated with unchanged owner geometry");
            std::cout << "NPC speech owner frame " << frame.x << 'x' << frame.y << " checked at GUI scales1/1.5\n";
            renderer.shutdown();
        } else std::cout << "SKIP owner speech template: " << error << '\n';
    } else std::cout << "SKIP owner speech template: installed assets unavailable\n";

    { // Generic overhead models: steam-creature-overhead-models.md §1-2.
        const glm::vec4 white(1);
        auto chosen = [](OverheadInputs in) { return overhead(in); };
        auto model = [&](OverheadInputs in) { const auto o = chosen(in); return o ? o->pose.model : 0; };
        OverheadInputs plain;
        check(!chosen(plain), "an ordinary class selects no overhead model");
        for (uint8_t c : {128, 129, 130}) { plain.creatureClass = c; check(model(plain) == 0xc3d, "classes 128-130 select 0xc3d"); }
        plain.creatureClass = 131;
        check(model(plain) == 0xc3c, "class 131 selects 0xc3c");
        for (uint8_t c : {139, 155}) { plain.creatureClass = c; check(model(plain) == 0xdaf, "classes 139/155 select 0xdaf"); }
        plain.creatureClass = 132;
        check(!chosen(plain), "class 132 selects nothing");
        auto marker = chosen({.creatureClass = 128});
        check(marker && marker->pose.scale == 1 && marker->pose.bounce == 0 && marker->pose.tilt == 0 && marker->material == white,
              "a class marker has multiplier 1, no extra height/angle and white material");
        auto member = chosen({.creatureClass = 131, .collected = true});
        check(member && member->pose.model == 0xdc2 && member->material == white, "a collection member replaces the class marker, white");
        auto picked = chosen({.collected = true, .selected = true});
        check(picked && picked->pose.model == 0xdc2 && picked->material == glm::vec4(1, .25f, 0, 1),
              "the selected collection member is tinted (1, 0.25, 0, 1)");
        Emote love{0xdf0, 100};
        auto emoting = chosen({.creatureClass = 128, .collected = true, .selected = true, .emote = &love, .heartStrength = 2.f});
        check(emoting && emoting->pose.model == 0xdf0 && emoting->material == white &&
                  emoting->pose.scale == emotePose(love).scale && emoting->pose.bounce == emotePose(love).bounce,
              "an active emote replaces the marker, resets the material to white and blocks the heart");
        auto heart = chosen({.creatureClass = 139, .collected = true, .selected = true, .heartStrength = 1.5f});
        check(heart && heart->pose.model == 0xdf0 && heart->pose.scale == 2.f && heart->material == glm::vec4(1, .25f, 0, 1),
              "the buff-7 heart replaces the marker, scales 0.5 + strength and keeps the orange material");
        auto weak = chosen({.heartStrength = -.5f});
        check(weak && weak->pose.model == 0xdf0 && weak->pose.scale == 0.f, "the heart's strength is not clamped");
        Actor tall;
        tall.physical = {10 << 16, 20 << 16, 30 << 16};
        tall.body = {1, 1, 3};
        tall.drop = .25f;
        const auto m = emoteTransform(tall, EmotePose{0xdf0, 2.f}, {16, 12, 8}, {0, 0, 0});
        const glm::vec4 centre = m * glm::vec4(8, 6, 4, 1);
        check(std::abs(centre.x - 10) < 1e-4f && std::abs(centre.y - 20) < 1e-4f && std::abs(centre.z - (30 + 1.5f + 1 - .25f)) < 1e-4f,
              "a marker centres on body Z*0.5 + 1 - drop above the actor");
        check(std::abs(glm::length(glm::vec3(m[0])) - .8f / 16 * 2.f) < 1e-6f, "the heart's scale is 0.8 / header X times the multiplier");
    }
    std::cout << "NPC speech: " << (failures ? "FAIL\n" : "PASS\n");
    return failures;
}
