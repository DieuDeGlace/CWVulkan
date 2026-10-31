#include "PlasmaInput.h"

namespace plasma {
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
void Input::leftDown() {
    held |= 1;
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
    auto *ownWidget = selected && selected->widget ? selected : nullptr;
    if (focused && (!selected || ownWidget != focused)) {
        if (focused->widget)
            focused->widget->dirty = true;
        dispatchRegistered(focused, Event::FocusLost);
        focused = nullptr;
    }
    if (selected) {
        invoke(owner(selected), Hook::LeftDown);
        dispatchRegistered(selected, Event::LeftDown);
    }
    visit(root, Hook::GlobalDown);
}
void Input::leftUp() {
    held &= ~uint32_t(1);
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
        return true; // Ordinary Button global visitor slots are no-ops.
    }
}
} // namespace plasma
