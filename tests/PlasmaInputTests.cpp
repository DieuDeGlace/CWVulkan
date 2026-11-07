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
    // Native keys come from the independent secondary-button contract rather
    // than the enum, so incorrect numeric bindings cannot silently agree here.
    struct Secondary {
        int button, key;
        bool down;
        Input::Hook target, global;
        void (Input::*send)();
    };
    const Secondary secondary[] = {
        {1, 5, true, Input::Hook::RightDown, Input::Hook::GlobalRightDown, &Input::rightDown},
        {1, 6, false, Input::Hook::RightUp, Input::Hook::GlobalRightUp, &Input::rightUp},
        {2, 8, true, Input::Hook::MiddleDown, Input::Hook::GlobalMiddleDown, &Input::middleDown},
        {2, 9, false, Input::Hook::MiddleUp, Input::Hook::GlobalMiddleUp, &Input::middleUp}
    };
    for (const auto &test : secondary) {
        Node root;
        root.name = "root";
        root.widget.emplace();
        root.widget->enabled = false; // Global traversal has a separate admission rule.
        auto *a = add(root, "A"), *child = add(*a, "child", false), *b = add(root, "B");
        Node *picked = b;
        int picks = 0;
        Input input(root, [&](glm::vec2) { ++picks; return picked; });
        input.move({7, 9});
        b->widget->flags |= 0x20;
        input.leftDown(); // Retain a qualifying owner that secondary buttons must not replace.
        input.auxiliary(b);
        input.focus(b);
        input.setButtonHeld(1, true);
        input.setButtonHeld(2, true);
        input.setButtonHeld(test.button, !test.down);
        picked = child; // The retained movement hit is B, but button selection must pick afresh.
        const uint32_t expectedMask = test.down ? 7u : 7u & ~(1u << test.button);
        std::vector<std::string> trace;
        input.registerEvent(*b, static_cast<Input::Event>(19), [&](Input &state, Node &) {
            trace.push_back("focus");
            check(state.heldButtons() == expectedMask && state.focusWidget() == b && b->widget->dirty,
                  "secondary down marks old focus and dispatches key19 after held publication, before focus clear");
            state.focus(a); // The outer direct null store follows the callback even if it assigned new focus.
        });
        input.virtualHook = [&](Input &state, Node &node, Input::Hook hook) {
            if (hook == test.target) {
                trace.push_back("target");
                check(&node == a && state.heldButtons() == expectedMask, "secondary virtual uses the associated owner after mask update");
            } else if (hook == test.global)
                trace.push_back("global:" + node.name);
        };
        input.registerEvent(*a, static_cast<Input::Event>(test.key), [&](Input &, Node &node) {
            trace.push_back("registered");
            check(&node == child, "secondary registered callback keeps the originally selected node");
        });
        const int before = picks;
        (input.*test.send)();
        std::vector<std::string> expected;
        if (test.down) expected.push_back("focus");
        expected.insert(expected.end(), {"target", "registered", "global:root", "global:A", "global:B"});
        check(trace == expected, "secondary focus/virtual/registered/global order and root-first own-widget traversal");
        check(picks == before + 1 && input.movementHit() == b, "secondary selection uses one fresh pick without overwriting retained hover");
        check(input.heldButtons() == expectedMask && input.auxiliaryWidget() == b && b->visible && input.downOwner() == b,
              "secondary buttons preserve other held bits, the auxiliary target and the left qualifying owner");
        check(test.down ? (!input.focusWidget() && b->widget->dirty) : (input.focusWidget() == b && !b->widget->dirty),
              "focused-widget cleanup belongs only to secondary down, with no direct hide");
    }
    for (const auto &test : secondary) {
        Node root;
        auto *a = add(root, "A"), *child = add(*a, "child", false), *b = add(root, "B");
        int picks = 0;
        Input input(root, [&](glm::vec2) { ++picks; return b; });
        input.capture(child);
        child->flags = plx::NODE_PASSIVE | plx::NODE_DISABLED;
        root.visible = false;
        std::vector<std::string> trace;
        input.virtualHook = [&](Input &, Node &node, Input::Hook hook) {
            if (hook == test.target) trace.push_back("target:" + node.name);
            if (hook == test.global) trace.push_back("global:" + node.name);
        };
        input.registerEvent(*a, static_cast<Input::Event>(test.key), [&](Input &, Node &) { trace.push_back("registered"); }, false);
        (input.*test.send)();
        check(picks == 0 && trace == std::vector<std::string>{"target:A"},
              "secondary capture bypasses pick/root visibility while passive registration and global gates stay separate");
        trace.clear();
        child->flags = 0;
        root.visible = true;
        a->widget->enabled = false;
        (input.*test.send)();
        check(trace == std::vector<std::string>{"registered", "global:A", "global:B"},
              "optional-enabled secondary registration and global hooks survive a disabled targeted owner");
    }
    {
        Node root;
        auto *a = add(root, "A"), *child = add(*a, "child", false), *b = add(root, "B");
        int picks = 0;
        Input input(root, [&](glm::vec2) { ++picks; return b; });
        input.capture(child);
        std::vector<std::string> trace;
        input.virtualHook = [&](Input &, Node &node, Input::Hook hook) {
            if (hook == Input::Hook::RightDown) {
                trace.push_back("target:" + node.name);
                child->widgetOwner = b;
                b->widget->enabled = false;
            } else if (hook == Input::Hook::GlobalRightDown)
                trace.push_back("global:" + node.name);
        };
        input.registerEvent(*b, static_cast<Input::Event>(5), [&](Input &, Node &node) {
            trace.push_back("registered:B");
            check(&node == child, "secondary registration rereads owner after the virtual without replacing its selected node");
            root.visible = false;
        }, false);
        input.rightDown();
        check(trace == std::vector<std::string>{"target:A", "registered:B"} && picks == 0,
              "secondary admission and global visibility are read after earlier callbacks");
        trace.clear();
        child->widgetOwner = nullptr;
        root.visible = true;
        input.rightDown();
        check(picks == 0 && trace == std::vector<std::string>{"global:A", "global:B"},
              "an ownerless secondary capture does not fall back to picking and still attempts global traversal");
    }
    for (const auto &test : secondary) {
        if (test.down) continue;
        Node root;
        auto *a = add(root, "A"), *b = add(root, "B");
        int picks = 0;
        Input input(root, [&](glm::vec2) { ++picks; return b; });
        input.capture(a);
        input.move({11, 13});
        input.setButtonHeld(test.button, true);
        std::vector<std::string> trace;
        input.virtualHook = [&](Input &state, Node &node, Input::Hook hook) {
            if (hook == test.target) {
                trace.push_back("up");
                check(state.heldButtons() == 0, "secondary release clears its held bit before callbacks");
                state.clearCapture();
            } else if (hook == Input::Hook::Leave && &node == a)
                trace.push_back("nested-leave");
            else if (hook == Input::Hook::Move && &node == b)
                trace.push_back("nested-move");
            else if (hook == test.global)
                trace.push_back("global:" + node.name);
        };
        input.registerEvent(*a, static_cast<Input::Event>(test.key), [&](Input &state, Node &node) {
            trace.push_back("registered-up");
            check(&node == a && !state.captureNode() && state.movementHit() == b,
                  "secondary up retains its entry target across a callback-induced capture release");
        });
        (input.*test.send)();
        check(trace == std::vector<std::string>{"up", "nested-leave", "nested-move", "registered-up", "global:A", "global:B"} && picks == 1,
              "secondary up has no extra left-button owner-comparison pick after nested movement");
        input.capture(a);
        int leaves = 0, releases = 0;
        input.virtualHook = [&](Input &, Node &, Input::Hook hook) { leaves += hook == Input::Hook::Leave; };
        input.registerEvent(*a, static_cast<Input::Event>(test.key), [&](Input &, Node &) { ++releases; });
        const int before = picks;
        (input.*test.send)();
        check(input.captureNode() == a && releases == 1 && leaves == 0 && picks == before,
              "secondary release has no matching-down, automatic capture clear or outside-owner leave rule");
    }
    for (const auto &test : secondary) {
        if (!test.down) continue;
        Node root;
        auto *a = add(root, "A"), *child = add(*a, "child", false);
        Node *picked = a;
        Input input(root, [&](glm::vec2) { return picked; });
        int lost = 0;
        input.focus(a);
        input.registerEvent(*a, static_cast<Input::Event>(19), [&](Input &, Node &) { ++lost; });
        (input.*test.send)();
        check(input.focusWidget() == a && !a->widget->dirty && lost == 0,
              "secondary down preserves focus when the selected node owns that exact widget");
        picked = child;
        (input.*test.send)();
        check(!input.focusWidget() && a->widget->dirty && lost == 1,
              "secondary focus comparison uses the node's direct widget, not its associated owner");
    }
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
    {
        // Native root/lifetime §§5–6: only destruction performs input cleanup.
        plx::Document document;
        plasma::Scene scene(document), other(document);
        auto *panel = scene.create(&scene.root(), "panel");
        auto *child = scene.create(panel, "child");
        auto *peer = scene.create(&scene.root(), "peer");
        panel->widget.emplace();
        panel->widget->flags |= 0x20;
        peer->widget.emplace();
        scene.root().initializeWidgets();
        Node *picked = panel;
        Input input(scene, [&](glm::vec2) { return picked; });
        input.move({17, 23});
        input.leftDown();
        input.capture(panel);
        input.focus(panel);
        input.auxiliary(panel);
        panel->visible = false;
        check(scene.reparent(*panel, nullptr) && !panel->parent && scene.root().children.size() == 1,
              "detached tree stays scene-owned outside root traversal");
        check(input.target() == panel && input.focusWidget() == panel && input.downOwner() == panel && input.heldButtons() == 1,
              "hide/detach preserve capture, focus, down owner and held state");
        check(scene.reparent(*panel, &scene.root()) && scene.root().children.back().get() == panel &&
                  panel->children.front().get() == child,
              "reattachment preserves node and descendant identities");
        check(scene.reparent(*peer, &scene.root()) && scene.root().children.back().get() == peer,
              "same-parent reattachment appends without recreating a node");
        check(!scene.reparent(*panel, child) && !scene.reparent(*panel, &other.root()) && !other.destroy(*panel) &&
                  !scene.destroy(scene.root()),
              "cycles, cross-scene mutation and engine-root destruction are rejected");
        int focusLost = 0, childMoves = 0;
        input.registerEvent(*panel, Input::Event::FocusLost, [&](Input &, Node &) { ++focusLost; });
        input.registerEvent(*panel, Input::Event::Move, [&](Input &, Node &) { ++childMoves; });
        check(scene.replaceWidget(*panel, plasma::reboundWidget(*panel->widget)), "replace the wrapper without replacing its node");
        check(!input.focusWidget() && !input.auxiliaryWidget() && !input.downOwner() && input.captureNode() == panel &&
                  input.movementHit() == panel && input.heldButtons() == 1 && focusLost == 0,
              "Widget deletion clears only matching Widget references without focus event or capture reset");
        input.move(input.pointer());
        check(childMoves == 0, "replaced Widget registrations do not remain bound to its node address");
        panel->visible = true;
        panel->widget->flags |= 0x20;
        input.registerEvent(*panel, Input::Event::FocusLost, [&](Input &, Node &) { ++focusLost; });
        input.focus(panel);
        input.auxiliary(panel);
        input.leftDown();
        picked = peer;
        bool beforeDetach = false, structuralReentryBlocked = false;
        input.virtualHook = [&](Input &state, Node &node, Input::Hook hook) {
            if (&node == peer && hook == Input::Hook::Move) {
                beforeDetach = panel->parent == &scene.root() && panel->children.front().get() == child && !state.captureNode() &&
                               state.focusWidget() == panel && state.previousPointer() == state.pointer();
                structuralReentryBlocked = !scene.destroy(*panel) && !scene.reparent(*peer, nullptr);
            }
        };
        check(scene.destroy(*panel), "destroy an owned node subtree");
        check(beforeDetach && structuralReentryBlocked && input.movementHit() == peer && !input.captureNode() && !input.focusWidget() &&
                  !input.auxiliaryWidget() && !input.downOwner() && input.heldButtons() == 1 && focusLost == 0,
              "capture release repicks before detach, retains replacement hover, then clears deleted Widget references");
        // A stationary release pass may pick the deleting node again; clear that retained hit.
        auto *again = scene.create(&scene.root(), "again");
        again->widget.emplace();
        again->initializeWidgets();
        picked = again;
        input.capture(again);
        input.move(input.pointer());
        check(scene.destroy(*again) && !input.movementHit() && !input.captureNode(),
              "deletion clears a hit repicked onto the same dying node");
        picked = nullptr;
        input.capture(peer);
        scene.clearChildren(scene.root());
        check(scene.root().children.empty() && !input.captureNode() && input.heldButtons() == 1,
              "child clearing uses the ordinary deletion path without resetting held buttons");
        auto *detached = scene.create(nullptr, "source");
        detached->widget.emplace();
        detached->widget->flags = plasma::WIDGET_WHOLE_HIT;
        detached->widget->bindSize = detached->widget->innerBindSize = detached->widget->frameSize = {20, 20};
        detached->initializeWidgets();
        check(!detached->parent && scene.root().children.empty() && scene.hitTest(*detached, {5, 5}) == detached &&
                  !scene.hitTest(scene.root(), {5, 5}),
              "registered detached sources can be queried independently but are outside live root picking");
        auto *copy = scene.instance(*detached, &scene.root());
        copy->initializeWidgets();
        check(copy != detached && scene.hitTest(scene.root(), {5, 5}) == copy && scene.destroy(*detached) &&
                  scene.hitTest(scene.root(), {5, 5}) == copy,
              "a live instance has its own identity and survives destruction of its detached source");
    }
    std::printf("GUI pointer routing: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
