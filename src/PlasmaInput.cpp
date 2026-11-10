#include "PlasmaInput.h"
#include <stdexcept>

namespace plasma {
Input::Input(Scene &scene, Pick pick) : Input(scene.root(), std::move(pick)) {
    if (scene.deletionObserver)
        throw std::logic_error("A GUI scene already has an input owner");
    this->scene = &scene;
    scene.deletionObserver = this;
}
Input::~Input() {
    if (scene)
        scene->deletionObserver = nullptr;
}
void Input::beforeNodeDeletion(SceneNode &node) {
    if (captured == &node)
        clearCapture(); // Run the unchanged-pointer pass before detachment.
    if (hovered == &node)
        hovered = nullptr;
    if (!node.widget)
        events.erase(&node);
}
void Input::beforeWidgetDeletion(SceneNode &node) {
    if (focused == &node)
        focused = nullptr;
    if (shownAuxiliary == &node)
        shownAuxiliary = nullptr;
    if (qualifyingDown == &node)
        qualifyingDown = nullptr;
    events.erase(&node); // Widget registrations cannot survive replacement/address reuse.
}
SceneNode *Input::owner(SceneNode *node) {
    return node ? node->widgetOwner : nullptr;
}
bool Input::contains(const SceneNode *parent, const SceneNode *node) {
    for (; node; node = node->parent)
        if (node == parent)
            return true;
    return false;
}
SceneNode *Input::target() const {
    return captured ? captured : pick ? pick(current) : nullptr;
}
void Input::focus(SceneNode *next) {
    if (next && next == focused)
        return;
    if (focused) {
        if (focused->widget)
            focused->widget->dirty = true;
        dispatchRegistered(focused, Event::FocusLost);
    }
    focused = next;
    if (next)
        dispatchRegistered(next, Event::FocusGained);
}
void Input::dispatchKey(Key kind, uint16_t value) {
    auto *selected = focused;
    if (selected && selected->widget && selected->widget->enabled && keyboardHook)
        keyboardHook(*this, *selected, kind, value);
}
void Input::keyDown(uint16_t key) {
    lastKeyboardKey = key;
    keys.insert(key);
    dispatchKey(Key::Down, key);
}
void Input::keyUp(uint16_t key) {
    lastKeyboardKey = key;
    keys.erase(key);
    dispatchKey(Key::Up, key);
}
void Input::character(uint16_t unit) { dispatchKey(Key::Character, unit); }

void Input::registerEvent(SceneNode &node, Event event, Callback callback, bool enabledRequired) {
    events[&node][event] = {std::move(callback), enabledRequired};
}
void Input::dispatchRegistered(SceneNode *node, Event event) {
    if (!node || (node->flags & plx::NODE_PASSIVE))
        return;
    auto *widgetNode = owner(node);
    if (!widgetNode || !widgetNode->widget)
        return;
    const auto at = events.find(widgetNode);
    if (at == events.end())
        return;
    const auto entry = at->second.find(event);
    if (entry != at->second.end() && entry->second.callback && (widgetNode->widget->enabled || !entry->second.enabledRequired))
        entry->second.callback(*this, *node);
}
void Input::dispatchWidgetEvent(SceneNode &node, Event event) {
    const auto at = events.find(&node);
    if (at == events.end()) return;
    const auto entry = at->second.find(event);
    if (entry == at->second.end()) return;
    // Safe engine callback ownership: replacement during this invocation cannot
    // invalidate the callable. No native allocator/lifetime behavior is claimed.
    const auto callback = entry->second.callback;
    if (callback) callback(*this, node);
}
void Input::invoke(SceneNode *node, Hook hook) {
    if (node && node->widget && node->widget->enabled && virtualHook)
        virtualHook(*this, *node, hook);
}
void Input::visit(SceneNode &node, Hook hook) {
    if (!node.visible || (node.flags & (plx::NODE_DISABLED | plx::NODE_PASSIVE)))
        return;
    if (node.widget && virtualHook)
        virtualHook(*this, node, hook); // Global visitors have no independent enabled gate.
    for (auto &child : node.children)
        visit(*child, hook);
}
void Input::closeShownAuxiliary() {
    if (shownAuxiliary)
        shownAuxiliary->visible = false;
    shownAuxiliary = nullptr;
}
void Input::move(glm::vec2 point) {
    previous = current;
    current = point;
    auto *selected = target();
    auto *oldOwner = owner(hovered), *newOwner = owner(selected);
    if (oldOwner != newOwner) {
        invoke(oldOwner, Hook::Leave);
        dispatchRegistered(hovered, Event::Leave); // The leave virtual may have performed a nested move.
        invoke(newOwner, Hook::Enter);
        dispatchRegistered(selected, Event::Enter);
    }
    if (selected) {
        invoke(newOwner, Hook::Move);
        dispatchRegistered(selected, Event::Move);
    }
    hovered = selected;
    visit(root, Hook::GlobalMove);
}
void Input::clearCapture() {
    captured = nullptr;
    move(current);
}
void Input::setButtonHeld(int button, bool down) {
    if (button < 0 || button > 2)
        return;
    const uint32_t bit = uint32_t(1) << button;
    held = down ? held | bit : held & ~bit;
}
void Input::leftDown() {
    setButtonHeld(0, true);
    auto *selected = target();
    auto *widgetNode = owner(selected);
    if (widgetNode && widgetNode->widget && widgetNode->widget->enabled && (widgetNode->widget->flags & 0x20)) {
        qualifyingDown = widgetNode;
        visit(root, Hook::GlobalQualifyingDown);
    } else {
        qualifyingDown = nullptr;
    }
    if (shownAuxiliary && !contains(shownAuxiliary, selected))
        closeShownAuxiliary();
    clearOtherFocus(selected);
    if (selected) {
        invoke(owner(selected), Hook::LeftDown);
        dispatchRegistered(selected, Event::LeftDown);
    }
    visit(root, Hook::GlobalDown);
}
void Input::leftUp() {
    setButtonHeld(0, false);
    auto *selected = target();
    if (shownAuxiliary && qualifyingDown)
        closeShownAuxiliary();
    if (selected) {
        invoke(owner(selected), Hook::LeftUp);
        dispatchRegistered(selected, Event::LeftUp);
    }
    auto *after = target();
    auto *underPointer = pick ? pick(current) : nullptr;
    if (owner(after) != owner(underPointer)) {
        invoke(owner(after), Hook::Leave);
        dispatchRegistered(after, Event::Leave);
    }
    visit(root, Hook::GlobalUp);
}
void Input::clearOtherFocus(SceneNode *selected) {
    auto *ownWidget = selected && selected->widget ? selected : nullptr;
    if (focused && (!selected || ownWidget != focused)) {
        if (focused->widget)
            focused->widget->dirty = true;
        dispatchRegistered(focused, Event::FocusLost);
        focused = nullptr;
    }
}
void Input::secondaryButton(int button, bool down, Hook targeted, Event event, Hook global) {
    setButtonHeld(button, down);
    auto *selected = target();
    if (down)
        clearOtherFocus(selected);
    if (selected) {
        invoke(owner(selected), targeted);
        dispatchRegistered(selected, event); // Resolve the associated owner and admission again after the virtual hook.
    }
    visit(root, global);
}
void Input::rightDown() {
    secondaryButton(1, true, Hook::RightDown, Event::RightDown, Hook::GlobalRightDown);
}
void Input::rightUp() {
    secondaryButton(1, false, Hook::RightUp, Event::RightUp, Hook::GlobalRightUp);
}
void Input::middleDown() {
    secondaryButton(2, true, Hook::MiddleDown, Event::MiddleDown, Hook::GlobalMiddleDown);
}
void Input::middleUp() {
    secondaryButton(2, false, Hook::MiddleUp, Event::MiddleUp, Hook::GlobalMiddleUp);
}

bool buttonInput(Input &input, SceneNode &node, Input::Hook hook, const ButtonSequenceStart &start) {
    if (!node.widget || node.widget->kind != "Button")
        return false;
    auto &button = *node.widget;
    auto play = [&](SceneNode &owner, ButtonPose pose) {
        const auto &name = owner.widget->buttonSequence(pose);
        if (start)
            start(owner, name);
        else
            owner.playSequence(name);
    };
    switch (hook) {
    case Input::Hook::LeftDown:
        input.capture(&node);
        button.buttonDelay = -500;
        button.buttonInterval = 150;
        button.buttonHeld = true;
        if (button.buttonType == 0) {
            play(node, ButtonPose::Press);
        } else {
            if (button.buttonType == 2)
                for (auto *member : button.buttonGroup)
                    if (member && member->widget && member->widget->checked) {
                        member->widget->checked = false;
                        play(*member, ButtonPose::Leave);
                    }
            play(node, button.checked ? ButtonPose::PressChecked : ButtonPose::Press);
            if (!button.checked)
                button.checked = true;
            else if (button.buttonType != 2)
                button.checked = false;
        }
        return true;
    case Input::Hook::LeftUp:
        input.clearCapture();
        button.buttonHeld = false;
        play(node, button.buttonType != 0 && button.checked ? ButtonPose::ReleaseChecked : ButtonPose::Release);
        return true;
    case Input::Hook::Enter:
        play(node, button.buttonType != 0 && button.checked ? ButtonPose::EnterChecked : ButtonPose::Enter);
        return true;
    case Input::Hook::Leave:
        play(node, button.buttonType != 0 && button.checked ? ButtonPose::LeaveChecked : ButtonPose::Leave);
        return true;
    case Input::Hook::Move:
        return false;
    default:
        return true; // Other hooks have no Button-specific handler here.
    }
}
} // namespace plasma
