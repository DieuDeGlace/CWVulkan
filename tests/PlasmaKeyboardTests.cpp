// Ordered manager-level traces from the complete independent GUI keyboard audit.
#include "PlasmaInput.h"
#include <cstdio>
#include <string>
#include <tuple>
#include <vector>

namespace {
using Input = plasma::Input;
using Node = plasma::SceneNode;
Node *add(Node &parent, const char *name) {
    auto child = std::make_unique<Node>();
    child->name = name;
    child->parent = &parent;
    child->widget.emplace();
    child->widgetOwner = child.get();
    auto *result = child.get();
    parent.children.push_back(std::move(child));
    return result;
}
}

int plasmaKeyboardTests() {
    int failures = 0;
    const auto check = [&](bool pass, const char *message) {
        if (!pass) { ++failures; std::printf("FAIL GUI keyboard: %s\n", message); }
    };
    {
        Node root;
        auto *a = add(root,"A"), *b = add(root,"B");
        Input input(root,{});
        std::vector<std::string> events;
        for (auto *node : {a,b}) {
            input.registerEvent(*node,static_cast<Input::Event>(18),[&,node](Input &state,Node &receiver) {
                check(&receiver == node && state.focusWidget() == node,"focus18 follows pointer publication");
                events.push_back("gain:"+node->name);
            });
            input.registerEvent(*node,static_cast<Input::Event>(19),[&,node](Input &state,Node &receiver) {
                check(&receiver == node && state.focusWidget() == node && node->widget->dirty,
                      "focus19 sees the dirty old owner before pointer replacement");
                events.push_back("lose:"+node->name);
            });
        }
        input.focus(a);
        input.focus(a);
        input.focus(b);
        input.focus(nullptr);
        input.focus(nullptr);
        check(events == std::vector<std::string>{"gain:A","lose:A","gain:B","lose:B"} && !input.focusWidget(),
              "synchronous focus order and already-selected/null cases");
    }
    {
        Node root;
        auto *a = add(root,"A");
        Input input(root,{});
        input.focus(a);
        root.visible = a->visible = false;
        root.flags = a->flags = plx::NODE_DISABLED | plx::NODE_PASSIVE;
        std::vector<std::tuple<Input::Key,uint16_t>> messages;
        input.keyboardHook = [&](Input &state,Node &owner,Input::Key kind,uint16_t value) {
            check(&owner == a,"keyboard uses the selected own widget independently of hidden ancestors");
            if (kind == Input::Key::Down)
                check(state.lastKey() == value && state.keyHeld(value),"down publishes zero-extended last key and membership first");
            if (kind == Input::Key::Up)
                check(state.lastKey() == value && !state.keyHeld(value),"up publishes last key and erases membership before delivery");
            messages.emplace_back(kind,value);
        };
        input.keyDown(0xabcd);
        input.keyDown(0xabcd);
        check(input.heldKeyCount() == 1,"repeated downs preserve unique membership but both dispatch");
        input.character(0xd800);
        input.character(0x001f);
        check(input.lastKey() == 0xabcd && input.heldKeyCount() == 1,
              "character units, including surrogate/control units, leave last-key and held state alone");
        input.keyUp(0xabcd);
        input.keyUp(0xabcd);
        check(messages == std::vector<std::tuple<Input::Key,uint16_t>>{
                  {Input::Key::Down,0xabcd},{Input::Key::Down,0xabcd},
                  {Input::Key::Character,0xd800},{Input::Key::Character,0x001f},
                  {Input::Key::Up,0xabcd},{Input::Key::Up,0xabcd}},
              "each repeated/absent-up message dispatches once without page or pointer gates");
        messages.clear();
        a->widget->enabled = false;
        input.keyDown(0x10);
        input.keyDown(0xa0);
        input.character('X');
        input.keyUp(0x10);
        check(messages.empty() && !input.keyHeld(0x10) && input.keyHeld(0xa0) && input.lastKey() == 0x10,
              "disabled selection suppresses its virtual only; held keys retain exact unsigned16 identity");
        input.focus(nullptr);
        input.keyDown(0xffff);
        check(input.keyHeld(0xffff) && input.heldKeyCount() == 2 && messages.empty(),
              "unfocused dispatch still retains message state without inventing a focused receiver");
    }
    {
        Node root;
        auto *a = add(root,"A");
        Input input(root,{});
        a->widget->enabled = false;
        int gained = 0;
        input.registerEvent(*a,static_cast<Input::Event>(18),[&](Input &,Node &) {++gained;});
        input.focus(a);
        check(input.focusWidget() == a && gained == 0,"disabled focus can be selected without an enabled-required callback");
        input.focus(nullptr);
        input.registerEvent(*a,static_cast<Input::Event>(18),[&](Input &,Node &) {++gained;},false);
        input.focus(a);
        check(gained == 1,"focus uses the existing connection's optional-enabled gate");
        a->widget->enabled = true;
        input.keyboardHook = [&](Input &state,Node &,Input::Key kind,uint16_t value) {
            if (kind == Input::Key::Down && value == 0x0d) state.focus(nullptr);
        };
        input.keyDown(0x0d);
        check(!input.focusWidget() && input.keyHeld(0x0d),
              "synchronous GUI callback mutations are visible to the following caller without consuming the message");
    }
    {
        Node root;
        auto *a = add(root,"A"), *b = add(root,"B"), *c = add(root,"C");
        Input input(root,{});
        input.focus(a);
        std::vector<std::string> events;
        bool nested = false;
        input.registerEvent(*a,static_cast<Input::Event>(19),[&](Input &state,Node &) {
            events.push_back(nested ? "nested-lose:A" : "lose:A");
            if (!nested) { nested = true; state.focus(c); }
        });
        for (auto *node : {b,c})
            input.registerEvent(*node,static_cast<Input::Event>(18),[&,node](Input &,Node &) {events.push_back("gain:"+node->name);});
        input.focus(b);
        check(events == std::vector<std::string>{"lose:A","nested-lose:A","gain:C","gain:B"} && input.focusWidget() == b,
              "outer focus store follows synchronous reentry without invented stabilization");
    }
    {
        Node root;
        auto *a = add(root,"A");
        Input input(root,{});
        a->widget->enabled = root.visible = false;
        a->flags = plx::NODE_DISABLED | plx::NODE_PASSIVE;
        int changes = 0;
        input.registerEvent(*a,static_cast<Input::Event>(20),[&](Input &state,Node &node) {
            check(&node == a,"direct Widget event retains its own frame receiver");
            ++changes;
            state.registerEvent(*a,static_cast<Input::Event>(20),[&](Input &,Node &) {changes += 10;});
        });
        input.dispatchRegistered(a,static_cast<Input::Event>(20));
        check(changes == 0,"frame event route keeps its own passive/enabled gates");
        input.dispatchWidgetEvent(*a,static_cast<Input::Event>(20));
        input.dispatchWidgetEvent(*a,static_cast<Input::Event>(20));
        check(changes == 11,"Edit-style direct change events bypass frame/enabled gates with synchronous callback replacement");
    }
    std::printf("GUI keyboard/focus manager: %s\n",failures ? "FAIL" : "PASS");
    return failures;
}
