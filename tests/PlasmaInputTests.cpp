// Ordered synthetic traces from steam-gui-pointer-capture-routing; no owner assets.
#include "PlasmaInput.h"
#include <cstdio>
#include <string>
#include <vector>

namespace {
using Input = plasma::Input;
using Node = plasma::SceneNode;
Node *add(Node &parent, const char *name, bool widget = true) {
    auto node = std::make_unique<Node>();
    node->name = name;
    node->parent = &parent;
    if (widget)
        node->widget.emplace();
    node->widgetOwner = widget ? node.get() : parent.widgetOwner;
    auto *result = node.get();
    parent.children.push_back(std::move(node));
    return result;
}
const char *name(Input::Hook hook) {
    switch (hook) {
    case Input::Hook::LeftDown:
        return "down";
    case Input::Hook::LeftUp:
        return "up";
    case Input::Hook::Move:
        return "move";
    case Input::Hook::Enter:
        return "enter";
    case Input::Hook::Leave:
        return "leave";
    default:
        return "global";
    }
}
bool targeted(Input::Hook hook) {
    return hook <= Input::Hook::Leave;
}
} // namespace

int plasmaInputTests() {
    int failures = 0;
    auto check = [&](bool pass, const char *what) {
        if (!pass) {
            std::printf("FAIL GUI pointer: %s\n", what);
            ++failures;
        }
    };
    {
        Node root;
        auto *a = add(root, "A"), *one = add(*a, "one", false), *two = add(*a, "two", false);
        Node *picked = one;
        int picks = 0;
        Input input(root, [&](glm::vec2) {
            ++picks;
            return picked;
        });
        check(input.pointer() == glm::vec2(0) && input.previousPointer() == glm::vec2(0) && !input.captureNode() && !input.movementHit() &&
                  input.heldButtons() == 0,
              "native initial retained fields");
        std::vector<std::string> trace;
        input.virtualHook = [&](Input &, Node &owner, Input::Hook hook) {
            if (targeted(hook))
                trace.push_back(owner.name + ":" + name(hook));
        };
        for (const auto [event, label] :
             {std::pair{Input::Event::Enter, "enter"}, {Input::Event::Leave, "leave"}, {Input::Event::Move, "move"}})
            input.registerEvent(*a, event, [&, label](Input &, Node &node) { trace.push_back(node.name + ":registered-" + label); });
        input.move({10.25f, 20.5f});
        check(trace == std::vector<std::string>{"A:enter", "one:registered-enter", "A:move", "one:registered-move"},
              "virtual enter/move precede original-node registered callbacks");
        trace.clear();
        picked = two;
        input.move(input.pointer());
        check(trace == std::vector<std::string>{"A:move", "two:registered-move"} && input.previousPointer() == input.pointer(),
              "same-owner descendant and stationary polls do not restart hover");
        trace.clear();
        a->widget->enabled = false;
        input.registerEvent(*a, Input::Event::Move, [&](Input &, Node &) { trace.push_back("allowed"); }, false);
        input.move(input.pointer());
        check(trace == std::vector<std::string>{"allowed"}, "disabled virtual and enabled-optional registration have separate gates");
        trace.clear();
        a->widget->enabled = true;
        one->flags = plx::NODE_PASSIVE | plx::NODE_DISABLED | 0x1000;
        root.visible = false;
        input.capture(one);
        const int before = picks;
        input.move({-30, 40});
        check(picks == before && trace == std::vector<std::string>{"A:move"} && input.movementHit() == one,
              "captured hidden/excluded/passive node bypasses picking but retains independent registration gating");
    }
    {
        Node root;
        auto *a = add(root, "A"), *child = add(*a, "child", false), *b = add(root, "B");
        Node *picked = child;
        Input input(root, [&](glm::vec2) { return picked; });
        input.move({7, 9});
        std::vector<std::string> trace;
        bool nestedHeldClear = false, buttonHeldDuringNested = false;
        input.virtualHook = [&](Input &state, Node &owner, Input::Hook hook) {
            if (!targeted(hook))
                return;
            trace.push_back(owner.name + ":" + name(hook));
            if (hook == Input::Hook::LeftDown) {
                check((state.heldButtons() & 1) != 0, "down held bit precedes virtual");
                state.capture(&owner);
                owner.widget->buttonHeld = true;
            } else if (hook == Input::Hook::LeftUp) {
                state.clearCapture();
                owner.widget->buttonHeld = false;
            } else if (hook == Input::Hook::Move && &owner == b) {
                nestedHeldClear = state.heldButtons() == 0 && state.pointer() == state.previousPointer();
                buttonHeldDuringNested = a->widget->buttonHeld;
            }
        };
        input.registerEvent(*a, Input::Event::LeftDown, [&](Input &state, Node &node) {
            check(&node == child && state.captureNode() == a, "down retains selected child after virtual capture");
        });
        input.registerEvent(*a, Input::Event::LeftUp, [&](Input &state, Node &node) {
            trace.push_back("A:registered-up");
            check(&node == a && !a->widget->buttonHeld && state.movementHit() == b, "outer captured release retains original target");
        });
        input.leftDown();
        trace.clear();
        picked = b;
        input.leftUp();
        check(trace == std::vector<std::string>{"A:up", "A:leave", "B:enter", "B:move", "A:registered-up"} && nestedHeldClear &&
                  buttonHeldDuringNested && !input.captureNode(),
              "release nested refresh precedes Button-held clear and event3");
        int releaseWithoutDown = 0;
        input.registerEvent(*b, Input::Event::LeftUp, [&](Input &, Node &) { ++releaseWithoutDown; });
        input.leftUp();
        check(releaseWithoutDown == 1, "registered release has no universal matching-down gate");
        a->widget->enabled = false;
        input.capture(a);
        input.leftUp();
        check(input.captureNode() == a, "disabled release virtual does not gain an invented capture clear");
    }
    {
        Node root;
        auto *a = add(root, "A"), *child = add(*a, "child", false), *b = add(root, "B");
        Input input(root, [&](glm::vec2) { return child; });
        input.focus(a);
        input.auxiliary(b);
        a->widget->flags |= 0x20;
        int focusLoss = 0, qualifierVisits = 0;
        input.registerEvent(*a, Input::Event::FocusLost, [&](Input &state, Node &) {
            ++focusLoss;
            check(state.focusWidget() == a && a->widget->dirty, "focus-loss dirty/event precede focus clear");
        });
        input.virtualHook = [&](Input &, Node &, Input::Hook hook) { qualifierVisits += hook == Input::Hook::GlobalQualifyingDown; };
        input.leftDown();
        check(!b->visible && !input.auxiliaryWidget() && focusLoss == 1 && !input.focusWidget() && input.downOwner() == a &&
                  qualifierVisits == 2,
              "own-widget focus comparison is distinct from inherited target owner");
        input.leftUp();
        check(input.downOwner() == a, "release does not generically clear qualifying-down owner");
    }
    {
        Node root;
        auto *a = add(root, "A"), *selected = add(*a, "selected", false), *b = add(root, "B");
        Input input(root, [&](glm::vec2) { return selected; });
        bool savedMoveOwner = false;
        int newOwnerCallbacks = 0;
        input.virtualHook = [&](Input &, Node &owner, Input::Hook hook) {
            if (hook == Input::Hook::Enter)
                selected->widgetOwner = b;
            if (hook == Input::Hook::Move)
                savedMoveOwner = &owner == a;
        };
        for (auto event : {Input::Event::Enter, Input::Event::Move})
            input.registerEvent(*b, event, [&](Input &, Node &) { ++newOwnerCallbacks; });
        input.move({1, 2});
        check(savedMoveOwner && newOwnerCallbacks == 2, "saved move virtual owner and reloaded registered owner remain distinct");
    }
    {
        Node root;
        auto *a = add(root, "A"), *b = add(root, "B"), *c = add(root, "C");
        Node *picked = a;
        Input input(root, [&](glm::vec2) { return picked; });
        input.move({1, 1});
        bool nested = false;
        int cLeaves = 0;
        input.virtualHook = [&](Input &state, Node &owner, Input::Hook hook) {
            if (&owner == a && hook == Input::Hook::Leave && !nested) {
                nested = true;
                picked = c;
                state.move({2, 2});
                picked = b;
            }
        };
        input.registerEvent(*c, Input::Event::Leave, [&](Input &, Node &) { ++cLeaves; });
        picked = b;
        input.move({3, 3});
        check(cLeaves == 1 && input.movementHit() == b, "outer leave registration reloads the movement hit after nested dispatch");
    }
    {
        Node root;
        auto *a = add(root, "A"), *passive = add(root, "passive"), *skipped = add(*passive, "skipped"), *b = add(root, "B");
        (void)skipped;
        passive->flags |= plx::NODE_PASSIVE;
        a->widget->enabled = false;
        b->flags |= 0x1000;
        Input input(root, [](glm::vec2) -> Node * { return nullptr; });
        std::vector<std::string> visits;
        input.virtualHook = [&](Input &, Node &node, Input::Hook hook) {
            if (hook == Input::Hook::GlobalMove)
                visits.push_back(node.name);
        };
        input.move({});
        check(visits == std::vector<std::string>{"A", "B"},
              "global own-Widget visitor prunes passive subtrees, not disabled owners or hit-only flags");
    }
    {
        Node root;
        auto *popup = add(root, "popup"), *child = add(*popup, "child", false);
        Node *picked = child;
        Input input(root, [&](glm::vec2) { return popup->visible ? picked : nullptr; });
        popup->widget->flags |= 0x20;
        input.auxiliary(popup);
        input.leftDown();
        check(popup->visible && input.auxiliaryWidget() == popup, "auxiliary containment includes a selected descendant");
        input.capture(popup);
        input.focus(popup);
        bool hiddenBeforeUp = false;
        int releases = 0;
        input.virtualHook = [&](Input &, Node &, Input::Hook hook) {
            if (hook == Input::Hook::LeftUp)
                hiddenBeforeUp = !popup->visible;
        };
        input.registerEvent(*popup, Input::Event::LeftUp, [&](Input &, Node &) { ++releases; });
        input.leftUp();
        check(hiddenBeforeUp && releases == 1 && !input.auxiliaryWidget() && input.captureNode() == popup && input.focusWidget() == popup &&
                  input.downOwner() == popup,
              "auxiliary close is a pure hide/null before release, without capture/focus/marker cleanup");
        input.capture(nullptr);
        popup->visible = true;
        input.auxiliary(popup);
        picked = &root;
        input.leftDown();
        check(!popup->visible && !input.auxiliaryWidget(), "selecting an auxiliary ancestor is outside its subtree");
        popup->visible = true;
        input.auxiliary(popup);
        picked = nullptr;
        input.leftDown();
        check(!popup->visible && !input.auxiliaryWidget(), "null target also closes the auxiliary root");
    }
    {
        Node root;
        root.widget.emplace();
        root.widget->kind = "Widget";
        auto *inner = add(root, "inner"), *a = add(*inner, "A"), *b = add(*inner, "B"), *outerPeer = add(root, "outer");
        a->widget->kind = "Button";
        b->widget->kind = "ScrollButton";
        outerPeer->widget->kind = "PopUpButton";
        a->widget->buttonType = b->widget->buttonType = outerPeer->widget->buttonType = 2;
        b->visible = false;
        b->flags |= plx::NODE_PASSIVE;
        b->widget->enabled = false;
        a->rebuildButtonGroup();
        check(a->widget->buttonGroup == std::vector<Node *>{b}, "group frontier includes hidden/passive/disabled Button-derived peers");
        a->flags |= 0x10;
        a->rebuildButtonGroup();
        check(a->widget->buttonGroup == std::vector<Node *>{b, outerPeer},
              "mask10 group collection preserves extra-frontier before candidate order");
        b->flags |= plx::NODE_DISABLED;
        a->rebuildButtonGroup();
        check(a->widget->buttonGroup == std::vector<Node *>{outerPeer}, "excluded subtree is not a group candidate");
        b->flags &= ~plx::NODE_DISABLED;
        a->rebuildButtonGroup();
        b->widget->checked = true;
        b->widget->buttonType = 0; // Stored membership is not revalidated at press.
        auto names = std::make_shared<plasma::ButtonNames>(plasma::defaultButtonNames());
        (*names)[size_t(plasma::ButtonPose::Leave)] = "owned-peer-leave";
        b->widget->buttonNames = names;
        Input input(root, [&](glm::vec2) { return a; });
        std::vector<std::string> starts;
        bool peerClearedBeforeStart = false, ownOldCheckBeforeStart = false;
        auto sequenceStart = [&](Node &node, const std::string &sequence) {
            starts.push_back(sequence);
            if (&node == b)
                peerClearedBeforeStart = !b->widget->checked;
            if (&node == a)
                ownOldCheckBeforeStart = !a->widget->checked;
        };
        input.virtualHook = [&](Input &state, Node &node, Input::Hook hook) { plasma::buttonInput(state, node, hook, sequenceStart); };
        input.leftDown();
        check(starts == std::vector<std::string>{"owned-peer-leave", "button:press"} && peerClearedBeforeStart && ownOldCheckBeforeStart &&
                  a->widget->checked && !b->widget->checked,
              "type2 press clears borrowed peer before ordinary leave and starts own press before checking");
        check(input.captureNode() == a && a->widget->buttonHeld && a->widget->buttonDelay == -500 && a->widget->buttonInterval == 150,
              "Button down captures its own root and writes native press bookkeeping");
        starts.clear();
        input.leftDown();
        check(starts == std::vector<std::string>{"button:press:checked"} && a->widget->checked,
              "already checked type2 retains its own check state");
        a->widget->buttonType = 1;
        starts.clear();
        input.leftDown();
        check(starts == std::vector<std::string>{"button:press:checked"} && !a->widget->checked,
              "other nonzero Button type starts checked press before toggling off");
        check(!plasma::buttonInput(input, *b, Input::Hook::LeftDown), "shared Button kind does not replace ScrollButton's dynamic hook");
        a->widget->buttonType = 0;
        a->widget->checked = true;
        starts.clear();
        input.leftDown();
        check(starts == std::vector<std::string>{"button:press"} && a->widget->checked, "ordinary type0 press has no check-byte write");
    }
    std::printf("GUI pointer routing: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
