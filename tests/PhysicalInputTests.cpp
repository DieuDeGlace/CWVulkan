#include "PhysicalInput.h"
#include "GameInput.h"
#include "Frontend.h"
#include <cstring>
#include <iostream>
#include <limits>

int physicalInputTests() {
    using namespace physicalinput;
    int failures = 0;
    const auto check = [&](bool value, const char *message) {
        if (!value) { std::cerr << "FAIL physical input: " << message << '\n'; ++failures; }
    };
    struct Owner {
        gameoptions::Controls bindings = gameoptions::defaultControls();
        Editor editor;
        Levels levels{};
        bool pad = true;
        Target target() { return {&bindings, &levels, &editor, &pad}; }
    } owner;
    const Current current = [&] { return owner.target(); };
    check(owner.editor.waiting == -1 && owner.editor.candidate == -1 && owner.editor.first == 0 &&
          !owner.editor.visible && owner.editor.values == gameoptions::defaultControls(), "editor constructor defaults");
    owner.bindings[0] = {99,2};
    check(owner.editor.values != owner.bindings, "pending defaults do not import active bindings");
    owner.bindings = {};
    owner.bindings[0] = {42,0}; owner.bindings[1] = {42,0}; owner.bindings[2] = {0,0};
    owner.bindings[3] = {-1,0}; owner.bindings[4] = {256,0}; owner.bindings[5] = {0,1};
    owner.bindings[6] = {3,1}; owner.bindings[7] = {4,1}; owner.bindings[8] = {42,2};
    owner.bindings[21] = {42,0}; owner.bindings[22] = {0,1};
    Keyboard keys{}; Mouse mouse{};
    keys[0] = 0xFF; keys[42] = 1; mouse[12] = 0x80; mouse[15] = 0xFF;
    const int32_t xy[2] = {-7, 16777217};
    std::memcpy(mouse.data(), xy, sizeof xy);
    Poll poll;
    std::vector<int> providerOrder;
    poll.sample(true, [&](auto &out) { providerOrder.push_back(1); out = keys; },
                [&](auto &out) { providerOrder.push_back(2); out = mouse; },
                [&] { providerOrder.push_back(3); return owner.target(); });
    check(providerOrder[0] == 1 && providerOrder[1] == 2 && providerOrder[2] == 3, "K then M then capture getter");
    check(poll.phase == Poll::Normal && owner.levels[0] == 1 && owner.levels[1] == 1 && owner.levels[2] == 0 &&
          owner.levels[3] == 0 && owner.levels[4] == 0 && owner.levels[5] == 1 && owner.levels[6] == 1 &&
          owner.levels[7] == 0 && owner.levels[8] == 0 && !owner.pad,
          "any-nonzero levels, duplicate active pairs, empty record, unsigned domains and category gates");
    check(owner.levels[21] == 1 && owner.levels[22] == 1 && poll.previousKeyboard()[42] == 0,
          "motion phase can observe mapped zoom levels before their later clears and snapshots");
    check(poll.relativeMotion() == glm::vec2(-7,16777216), "relative arguments convert signed device words to binary32");
    poll.clearZoom(current);
    check(owner.levels[21] == 0 && owner.levels[22] == 0, "later zoom clears precede XInput even on provider failure");
    poll.finish();
    check(poll.previousKeyboard() == keys && poll.previousMouse() == mouse, "common snapshot copies after admitted work");
    const auto levels = owner.levels;
    int calls = 0;
    poll.sample(false, [&](auto &) { ++calls; }, [&](auto &) { ++calls; }, [&] { ++calls; return owner.target(); });
    poll.clearZoom(current); poll.finish();
    check(calls == 0 && owner.levels == levels && poll.previousKeyboard() == keys && poll.previousMouse() == mouse,
          "inactive admission retains levels and previous snapshots without providers");
    owner.pad = true;
    poll.sample(true, [](auto &out) { out[42] = 0; }, [](auto &out) { out[12] = 0; out[15] = 0; }, current);
    check(owner.pad && owner.levels[0] == 0 && poll.keyboard[0] == 0xFF && poll.mouse[0] == mouse[0],
          "partial provider output is consumed without fresh zeroing; inactive records do not reset mode");
    poll.finish();
    owner.editor.waiting = 4;
    owner.levels.fill(7);
    const auto pending = owner.editor.values;
    poll.sample(true, [](auto &out) { out[0] = 0x80; }, [](auto &) {}, current);
    check(poll.phase == Poll::Capturing && owner.editor.waiting == 4 && owner.editor.values == pending &&
          owner.levels[0] == 7 && owner.levels[21] == 7, "held nonzero byte is not a new capture edge; levels remain retained");
    poll.finish();

    const auto captureCase = [&](int key, bool excluded, int expectedLoads) {
        Owner test; Poll sample;
        test.editor.waiting = 0;
        test.editor.candidate = 6;
        int loads = 0;
        sample.sample(true, [&](auto &out) { if (key >= 0) out[size_t(key)] = 1; },
                      [](auto &out) { out[15] = 0x80; },
                      [&] { ++loads; return test.target(); });
        const gameoptions::Binding expected = key < 0 || excluded ? gameoptions::Binding{3,1} : gameoptions::Binding{key,0};
        check(test.editor.values[0] == expected && test.editor.candidate == -1, "nested mouse scans and first K edge value");
        check(test.editor.waiting == (key < 0 ? 0 : -1) && loads == expectedLoads,
              "exact fresh-wrapper count; only K path separately cancels");
    };
    captureCase(-1,false,257); // 256 repeated mouse writes, not one.
    captureCase(5,false,8);    // Five mouse writes, key writer, separate cancel, entry getter.
    captureCase(1,true,3);     // One prior mouse write, excluded key cancels only.
    for (int excluded : {0x1C,0x9C}) {
        Owner test; Poll sample; test.editor.waiting = 3;
        const auto original = test.editor.values;
        sample.sample(true, [&](auto &out) { out[size_t(excluded)] = 0x80; }, [](auto &) {}, [&] {return test.target();});
        check(test.editor.waiting == -1 && test.editor.values == original, "excluded K codes cancel without writing a pair");
    }
    {
        Owner test; Poll sample; test.editor.waiting = 0;
        int loads = 0;
        sample.sample(true, [](auto &) {}, [](auto &out) { out[12] = 1; out[15] = 1; },
                      [&] { ++loads; return test.target(); });
        check(loads == 513 && test.editor.values[0] == gameoptions::Binding{3,1} && test.editor.waiting == 0,
              "all rising mouse buttons offered in order on all 256 K iterations");
    }
    {
        Owner test; Poll sample; test.editor.waiting = 23; test.editor.candidate = 9;
        const auto original = test.editor.values;
        test.levels.fill(9);
        sample.sample(true, [](auto &) {}, [](auto &out) { out[12] = 1; }, [&] {return test.target();});
        check(sample.phase == Poll::Capturing && test.editor.waiting == 23 && test.editor.candidate == 9 &&
              test.editor.values == original && test.levels[21] == 9, "sign-only capture branch differs from writer's upper guard");
        sample.finish();
        sample.sample(true, [](auto &out) { out[42] = 1; }, [](auto &) {}, [&] {return test.target();});
        check(test.editor.waiting == -1 && test.editor.values == original, "out-of-range row still receives independent K cancellation");
    }
    {
        Owner first, second; Poll sample;
        first.editor.waiting = second.editor.waiting = 0;
        int loads = 0;
        sample.sample(true, [](auto &out) { out[42] = 1; }, [](auto &) {}, [&] {
            ++loads;
            return loads < 3 ? first.target() : second.target();
        });
        check(first.editor.values[0] == gameoptions::Binding{42,0} && first.editor.waiting == 0 &&
              second.editor.waiting == -1, "record and cancel wrappers independently reload current editor");
    }
    Editor editor;
    editor.values.fill({42,0}); editor.values[7] = {42,1};
    editor.waiting = 6; editor.candidate = 9;
    editor.bind({42,0});
    check(editor.values[6] == gameoptions::Binding{42,0} && editor.values[7] == gameoptions::Binding{42,1} &&
          editor.values[0] == gameoptions::Binding{0,0} && editor.values[22] == gameoptions::Binding{0,0} &&
          editor.waiting == 6 && editor.candidate == -1, "record writer clears all exact pairs and candidate only");
    editor.visible = true; editor.waiting = -1; editor.first = 0;
    check(editor.rows(420) == std::array<int32_t,2>{0,10}, "draw rows use truncated height minus120 divided by30");
    editor.beginDraw(false);
    check(editor.offerRow(0,27,{100,8},640,false) && editor.candidate == 0, "draw lower-inclusive row admission");
    editor.release(0);
    check(editor.waiting == 0, "release arms cached candidate without a matching press or fresh pick");
    check(!editor.offerRow(1,57,{100,38},640,false), "capture prevents later candidate production");
    editor.release(1);
    check(editor.waiting == -1 && editor.candidate == 0, "nonleft release resets capture while retaining candidate");
    editor.waiting = 2;
    editor.visible = false; editor.release(-42);
    check(editor.waiting == -1, "every signed button release resets hidden capture");
    editor.visible = true; editor.candidate = 27; editor.release(0);
    check(editor.waiting == 27, "release arm has no writer-style upper bound");
    editor.beginDraw(true);
    check(editor.waiting == -1 && editor.candidate == -1, "selected pad clears capture during an admitted draw");
    check(!editor.offerRow(0,27,{100,8},640,true), "pad mode blocks candidate");
    check(!editor.offerRow(0,27,{600,8},640,false) && !editor.offerRow(0,27,{100,37},640,false),
          "upper edges are exclusive");
    check(editor.offerRow(0,27,{0,7},640,false), "both lower edges are inclusive");
    const float nan = std::numeric_limits<float>::quiet_NaN();
    check(!editor.offerRow(0,27,{nan,8},640,false) && !editor.offerRow(0,27,{100,nan},640,false) &&
          !editor.offerRow(0,27,{100,8},nan,false), "unordered row geometry is rejected");
    editor.first = 20;
    check(editor.rows(420) == std::array<int32_t,2>{20,23}, "begin and end clamp separately to23");

    MenuInput in; Hud hud;
    in.controlEditor.visible = true; in.controlEditor.candidate = 9; in.controlEditor.waiting = 2;
    in.gameRelease = [&](int button) {
        check(button == 0 && in.controlEditor.waiting == 2, "remaining post-GUI Game work precedes capture reset");
        in.controlEditor.candidate = 11;
    };
    in.routeInterfaceRelease(hud,0);
    check(in.controlEditor.waiting == 11, "actual MenuInput suffix uses current candidate after post-GUI callback");
    in.gameRelease = {};
    in.rawKeyMessage(MenuInput::InterfaceMessage::KeyDown, 42);
    in.controlEditor.waiting = 0;
    in.clear();
    check(in.controlEditor.waiting == 0 && in.controlEditor.candidate == 11, "ordinary frame clearing retains editor and capture");

    in.controlEditor.waiting = -1;
    in.physical.keyboard[42] = 0xFF; in.physical.mouse[12] = 0x80;
    const auto defaults = gameoptions::defaultControls();
    check(in.samplePhysical(defaults,true) && in.physical.keyboard == Keyboard{} && in.physical.mouse == Mouse{} &&
          in.bindingLevels == Levels{}, "explicit pointer fixture supplies neutral device bytes instead of background hardware");
    in.gamepadSampled = false;
    in.gamepad.connected = in.gamepad.dpadUp = true;
    in.sampleGamepad(defaults,true);
    check(!in.gamepad.connected && !in.gamepad.dpadUp, "explicit pointer fixture does not query a background controller");
    in.finishPhysical();

    GameInput gameplay;
    Poll normal; normal.phase = Poll::Normal;
    Levels held{}; held[gameoptions::UP] = 1;
    GamepadButtons pad;
    gameplay.update(normal,held,pad,true,true);
    check(gameplay.held(gameoptions::UP) && gameplay.pressed(gameoptions::UP), "ordinary UI does not suppress keyboard levels");
    gameplay.update(normal,held,pad,true,true);
    check(gameplay.held(gameoptions::UP) && !gameplay.pressed(gameoptions::UP), "held levels do not fabricate repeated press edges");
    normal.phase = Poll::Capturing; held = {};
    gameplay.update(normal,held,pad);
    check(gameplay.held(gameoptions::UP) && !gameplay.pressed(gameoptions::UP), "capturing consumer retains state without a new edge");
    normal.phase = Poll::Skipped; gameplay.update(normal,held,pad);
    check(gameplay.held(gameoptions::UP), "inactive consumer retains levels");
    normal.phase = Poll::Normal; gameplay.update(normal,held,pad);
    check(!gameplay.held(gameoptions::UP) && !gameplay.pressed(gameoptions::UP), "next admitted zero sample clears level");

    const auto name = gameoptions::bindingName;
    check(name({1,-7}) == L"Escape" && name({28,0}) == L"Return" && name({29,0}) == L"Left Control" &&
          name({86,0}) == L"<" && name({203,2}) == L"Left Arrow" && name({237,0}) == L"Media Select",
          "detailed dictionary literals stay distinct from compact/OS names");
    for (int key : {-1,0,84,85,89,99,238,std::numeric_limits<int>::max()})
        check(name({key,2}) == L"Unknown", "detailed constructor gap and unknown-category lookup");
    check(name({0,0}) == L"Unassigned" && name({0,2}) == L"Unknown" && name({0,1}) == L"Mouse 1" &&
          name({-1,1}) == L"Mouse 0" && name({std::numeric_limits<int>::max(),1}) == L"Mouse -2147483648" &&
          name({std::numeric_limits<int>::min(),1}) == L"Mouse -2147483647", "detailed formatter branch and 32-bit wrapped signed argument");
    const auto saved = gameoptions::writeControls(gameoptions::defaultControls());
    check(saved.find("ButtonQuickItem ") < saved.find("ButtonClassSkill ") &&
          saved.find("ButtonLamp ") < saved.find("ButtonUltimateSkill ") &&
          saved.find("ButtonMultiplayerMenu ") < saved.find("ButtonCraftingMenu "),
          "controls writer keeps the independently specified token order");
    if (!failures) std::cout << "Physical input: PASS buffer levels/capture/order/editor/consumer/detailed-name checks\n";
    return failures;
}
