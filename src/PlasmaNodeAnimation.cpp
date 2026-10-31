// Live instance properties and subtree sequence operations (plasma-format-and-scene, 2.6).
#include "PlasmaScene.h"
#include <algorithm>
#include <cstring>

namespace plasma {
namespace {
using Target = NodeAnimation::Target;

template <class T> void store(std::vector<uint8_t> &bytes, const T &value) {
    bytes.resize(sizeof value);
    std::memcpy(bytes.data(), &value, sizeof value);
}
template <class T> void load(const std::vector<uint8_t> &bytes, T &value) {
    if (bytes.size() >= sizeof value)
        std::memcpy(&value, bytes.data(), sizeof value);
}
const plx::Object *sourceObject(const SceneNode &node, Target target) {
    if (!node.doc)
        return nullptr;
    if (target == Target::Shape)
        return node.shape >= 0 && node.shape < (int)node.doc->shapes.size() ? &node.doc->shapes[(size_t)node.shape] : nullptr;
    if (node.source < 0 || node.source >= (int)node.doc->nodes.size())
        return nullptr;
    const auto &source = node.doc->nodes[(size_t)node.source];
    const auto &objects = target == Target::Shape       ? node.doc->shapes
                          : target == Target::Transform ? node.doc->transformations
                                                        : node.doc->displays;
    const int index = target == Target::Shape ? node.shape : target == Target::Transform ? source.transformation : source.display;
    return index >= 0 && index < (int)objects.size() ? &objects[(size_t)index] : nullptr;
}
bool discrete(const std::string &name) {
    return name == "Display.visibility" || name == "Display.clipping" || name.ends_with(".string") || name.ends_with(".texture") ||
           name.ends_with(".strokeTexture") || name.ends_with(".vertexFlags") || name.ends_with(".flags");
}
void currentFrame(const SceneNode &node, Target target, const std::string &name, std::vector<uint8_t> &bytes) {
    if (target == Target::Transform) {
        if (name == "Transformation.translation")
            store(bytes, node.transform.translation);
        else if (name == "Transformation.rotation")
            store(bytes, node.transform.rotation);
        else if (name == "Transformation.pivot")
            store(bytes, node.transform.pivot);
        else if (name == "Transformation.deformation")
            store(bytes, node.transform.deformation);
    } else if (target == Target::Display) {
        if (name == "Display.visibility")
            store(bytes, int32_t(node.visible));
        else if (name == "Display.clipping")
            store(bytes, int32_t(node.clips));
        else if (name == "Display.fillColor")
            store(bytes, node.fill);
        else if (name == "Display.blurRadius")
            store(bytes, node.blurRadius);
    }
}
void applyFrame(SceneNode &node, const NodeAnimation &track) {
    const auto *source = sourceObject(node, track.target);
    if (!source || track.property >= source->properties.size() || track.value.slots.empty())
        return;
    const auto &name = source->properties[track.property].name;
    const auto &bytes = track.value.slots[0];
    if (track.target == Target::Shape) {
        if (!node.animatedShape)
            node.animatedShape = *source;
        for (auto &frame : node.animatedShape->properties[track.property].children)
            if (frame.name == "Attribute.frame" || frame.name == "ArrayAttribute.frame") {
                frame.bytes = bytes;
                node.animatedDrawing.reset();
                break;
            }
    } else if (track.target == Target::Transform) {
        // Apply only this channel: layout may have moved the node since play().
        if (name == "Transformation.translation")
            load(bytes, node.transform.translation);
        else if (name == "Transformation.rotation")
            load(bytes, node.transform.rotation);
        else if (name == "Transformation.pivot")
            load(bytes, node.transform.pivot);
        else if (name == "Transformation.deformation")
            load(bytes, node.transform.deformation);
    } else {
        int32_t integer = 0;
        if (name == "Display.visibility") {
            load(bytes, integer);
            node.visible = integer != 0;
        } else if (name == "Display.clipping") {
            load(bytes, integer);
            node.clips = integer != 0;
        } else if (name == "Display.fillColor")
            load(bytes, node.fill);
        else if (name == "Display.blurRadius")
            load(bytes, node.blurRadius);
    }
}
void update(SceneNode &node, int milliseconds, bool absolute, bool force = false) {
    // Absolute seek admits a node's own force flag; ordinary advance tests incoming force first.
    if (absolute && (node.flags & plx::NODE_ALWAYS_ANIMATE))
        force = true;
    if ((node.flags & plx::NODE_DISABLED) || (!node.visible && !force))
        return;
    for (auto &track : node.animations)
        if (absolute ? track.value.evaluate(milliseconds) : track.value.advance(milliseconds))
            applyFrame(node, track);
    for (auto &child : node.children)
        update(*child, milliseconds, absolute, force || (node.flags & plx::NODE_ALWAYS_ANIMATE));
}
} // namespace

const plx::Object *SceneNode::shapeObject() const {
    return animatedShape ? &*animatedShape : sourceObject(*this, Target::Shape);
}
bool SceneNode::playSequence(const std::string &name, int period) {
    bool started = false;
    for (Target target : {Target::Shape, Target::Transform, Target::Display}) {
        const auto *object = sourceObject(*this, target);
        if (!object)
            continue;
        for (size_t i = 0; i < object->properties.size(); i++) {
            const auto &property = object->properties[i];
            std::vector<plx::Key> keys;
            if (!property.sequence(name, keys) || keys.empty())
                continue;
            AttributeAnimation animation(property, discrete(property.name));
            if (target == Target::Shape && animatedShape) {
                const auto frames = animatedShape->properties[i].frames();
                if (!frames.empty())
                    animation.slots[0] = *frames[0];
            }
            currentFrame(*this, target, property.name, animation.slots[0]);
            animation.play(name, period);
            auto found = std::find_if(animations.begin(), animations.end(),
                                      [&](const auto &track) { return track.target == target && track.property == i; });
            if (found == animations.end())
                animations.push_back({target, i, std::move(animation)});
            else
                found->value = std::move(animation);
            started = true;
        }
    }
    // Play itself includes hidden and disabled descendants; update/duration/stop have their own gates.
    for (auto &child : children)
        started = child->playSequence(name, period) || started;
    return started;
}
int SceneNode::sequenceDuration(const std::string &name) const {
    int duration = 0;
    for (Target target : {Target::Shape, Target::Transform, Target::Display})
        if (const auto *object = sourceObject(*this, target))
            for (const auto &property : object->properties) {
                std::vector<plx::Key> keys;
                if (property.sequence(name, keys) && !keys.empty())
                    duration = std::max(duration, keys.back().time);
            }
    for (const auto &child : children)
        if (!(child->flags & plx::NODE_DISABLED))
            duration = std::max(duration, child->sequenceDuration(name));
    return duration;
}
void SceneNode::setSequenceTime(int milliseconds) {
    update(*this, milliseconds, true);
}
void SceneNode::advanceSequences(int milliseconds) {
    update(*this, milliseconds, false);
}
void SceneNode::stopSequences() {
    for (auto &track : animations)
        track.value.active.clear();
    for (auto &child : children)
        if (!(child->flags & plx::NODE_DISABLED))
            child->stopSequences();
}
void SceneNode::poseSequence(const std::string &name) {
    playSequence(name);
    setSequenceTime(sequenceDuration(name));
    stopSequences();
}
void SceneNode::rebuildButtonGroup() {
    if (!widget || !widget->hasButtonState())
        return;
    widget->buttonGroup.clear();
    auto ancestor = [](SceneNode *node) {
        for (; node && !node->widget; node = node->parent) {
        }
        return node;
    };
    auto *container = ancestor(parent);
    const bool extraLevel = (flags & 0x10) != 0;
    if (extraLevel && container)
        container = ancestor(container->parent);
    if (!container)
        return;
    auto frontier = [](auto &&self, SceneNode &node, const auto &accept) -> void {
        if (node.flags & plx::NODE_DISABLED)
            return;
        for (auto &child : node.children) {
            if (child->flags & plx::NODE_DISABLED)
                continue;
            if (child->widget)
                accept(*child);
            else
                self(self, *child, accept);
        }
    };
    auto append = [&](SceneNode &member) {
        if (&member != this && member.widget && member.widget->hasButtonState() && member.widget->buttonType == 2)
            widget->buttonGroup.push_back(&member);
    };
    frontier(frontier, *container, [&](SceneNode &candidate) {
        if (extraLevel)
            frontier(frontier, candidate, append);
        append(candidate);
    });
}

void SceneNode::setWidgetEnabled(bool enabled) {
    if (!widget)
        return;
    widget->enabled = enabled;
    poseSequence(enabled ? "widget:enable" : "widget:disable");
}

void SceneNode::initializeWidgets() {
    if (flags & plx::NODE_DISABLED)
        return;
    if (widget) {
        widget->minimumSize = widget->bindSize - widget->innerBindSize;
        if (!widget->enabled) {
            // The base initializer seeks the disabled pose but does not stop it.
            playSequence("widget:disable");
            setSequenceTime(sequenceDuration("widget:disable"));
        }
        if (widget->kind == "Button") {
            rebuildButtonGroup();
            if (widget->enabled) {
                if (widget->buttonType != 0) {
                    poseSequence(widget->buttonSequence(ButtonPose::PressChecked));
                    poseSequence(widget->buttonSequence(ButtonPose::ReleaseChecked));
                } else {
                    poseSequence(widget->buttonSequence(ButtonPose::Release));
                }
                poseSequence(widget->buttonSequence(ButtonPose::Release));
                poseSequence(widget->buttonSequence(ButtonPose::Leave));
                if (widget->checked) {
                    poseSequence(widget->buttonSequence(ButtonPose::Press));
                    poseSequence(widget->buttonSequence(ButtonPose::ReleaseChecked));
                    poseSequence(widget->buttonSequence(ButtonPose::LeaveChecked));
                }
            }
        }
        if (!widget->caption.empty())
            setText("caption", widget->caption);
    }
    widgetOwner = widget ? this : parent ? parent->widgetOwner : nullptr;
    for (auto &child : children)
        child->initializeWidgets();
}
} // namespace plasma
