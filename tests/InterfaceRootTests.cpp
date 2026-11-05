#include "InterfaceRoot.h"
#include "PlasmaInput.h"
#include <iostream>

int interfaceRootTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *message) {
        if (!passed) {
            std::cout << "FAIL interface root: " << message << '\n';
            ++failures;
        }
    };
    plx::Document document;
    plasma::Scene scene(document);
    uiroot::Branches branches(scene);
    const auto target = [&](uiroot::Branch branch, const char *name) {
        auto *node = scene.create(branches.node(branch), name);
        node->widget.emplace();
        node->widgetOwner = node; // Synthetic counterpart of event-target initialization during scene cloning.
        node->widget->flags = plasma::WIDGET_WHOLE_HIT;
        return node;
    };
    auto *title = target(uiroot::Branch::Title, "title-control");
    auto *gameplay = target(uiroot::Branch::Gameplay, "bag-control");
    auto *options = target(uiroot::Branch::Options, "options-control");
    target(uiroot::Branch::Unspecified, "unattached-probe");
    plasma::Input input(
        scene, [&](glm::vec2 point) { return const_cast<plasma::SceneNode *>(scene.hitTest(scene.root(), point)); });
    int titlePresses = 0, gameplayPresses = 0;
    input.registerEvent(*title, plasma::Input::Event::LeftDown, [&](auto &, auto &) { ++titlePresses; });
    input.registerEvent(*gameplay, plasma::Input::Event::LeftDown, [&](auto &, auto &) { ++gameplayPresses; });
    const auto *root = &scene.root();
    uiroot::State state;
    state.frontend[0] = true;
    branches.apply(state);
    input.leftDown();
    check(titlePresses == 1 && gameplayPresses == 0 && gameplay->visible,
          "active frontend ancestry excludes gameplay even when its own control remains active");
    input.capture(gameplay);
    input.leftDown();
    check(gameplayPresses == 1, "internal capture is distinct from fresh root picking and ancestor visibility");
    input.capture(nullptr);
    state.frontend = {};
    state.options = true;
    branches.apply(state);
    check(branches.node(uiroot::Branch::Gameplay)->visible && scene.hitTest(scene.root(), {0, 0}) == options,
          "options is an independent root branch above the active gameplay branch");
    state.options = false;
    branches.apply(state);
    input.leftDown();
    check(gameplayPresses == 2 && &scene.root() == root && branches.node(uiroot::Branch::Gameplay) == gameplay->parent,
          "state transitions reuse one principal root and existing controls/input owner");
    state.frontend[2] = true;
    branches.apply(state);
    check(!branches.node(uiroot::Branch::Gameplay)->visible && !scene.hitTest(scene.root(), {0, 0}),
          "creation branch excludes gameplay without exposing inactive title or probe nodes");
    std::cout << "Interface root branches: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
