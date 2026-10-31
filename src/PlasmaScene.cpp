#include "PlasmaScene.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <bit>
#include <cstring>

namespace plasma {

void GuiClock::advance(double milliseconds) {
    step = (int32_t)(float)std::min(milliseconds, 1000.0);
    elapsed = std::bit_cast<int32_t>((uint32_t)elapsed + (uint32_t)step);
}

namespace {

// A point through a matrix, divided by its w (spec part 1 conventions).
glm::vec2 apply(const glm::mat4 &m, glm::vec2 p) {
    const glm::vec4 q = m * glm::vec4(p, 0, 1);
    return q.w != 0.0f ? glm::vec2(q) / q.w : glm::vec2(q);
}
// A vector through a matrix's linear part.
glm::vec2 linear(const glm::mat4 &m, glm::vec2 v) { return glm::vec2(m[0]) * v.x + glm::vec2(m[1]) * v.y; }

template <class T> T leaf(const plx::Object &o, const std::string &name, T fallback) {
    const plx::Chunk *c = o.property(name);
    return c ? c->as<T>() : fallback;
}

const plx::Object *objectAt(const std::vector<plx::Object> &objects, int index) {
    return index >= 0 && index < (int)objects.size() ? &objects[(size_t)index] : nullptr;
}

bool isTextShape(const SceneNode &n) {
    return n.doc && n.shape >= 0 && n.shape < (int)n.doc->shapes.size() && n.doc->shapes[(size_t)n.shape].kind == "TextShape";
}

bool disabled(const SceneNode &n) { return (n.flags & plx::NODE_DISABLED) != 0; }

} // namespace

// ---------------------------------------------------------------------------------------------
// Transformations and displays.

glm::mat4 Transformation::local() const {
    glm::mat4 m = pivoted(translation, pivot, rotation, deformation);
    // Flatten z: the z output row becomes (0, 0, 1, 0); the perspective row stays.
    m[0][2] = m[1][2] = m[3][2] = 0.0f;
    m[2][2] = 1.0f;
    return m;
}

Transformation transformationOf(const plx::Object &t) {
    Transformation r;
    r.translation = plx::live<glm::vec2>(t.property("Transformation.translation"), r.translation);
    r.rotation = plx::live<glm::vec3>(t.property("Transformation.rotation"), r.rotation);
    r.pivot = plx::live<glm::vec2>(t.property("Transformation.pivot"), r.pivot);
    r.deformation = plx::live<glm::mat4>(t.property("Transformation.deformation"), r.deformation);
    return r;
}

glm::mat4 transformation(const plx::Object &t, const glm::vec2 *translation, const glm::mat4 *deformation) {
    Transformation r = transformationOf(t);
    if (translation)
        r.translation = *translation;
    if (deformation)
        r.deformation = *deformation;
    return r.local();
}

glm::vec2 pivot(const plx::Object &t) { return transformationOf(t).pivot; }
glm::vec2 translation(const plx::Object &t) { return transformationOf(t).translation; }

int Display::maskMode() const {
    if (!clips)
        return 0;
    if (flags & DISPLAY_MASK_MULTIPLY)
        return 2;
    return flags & DISPLAY_MASK_ALPHA ? 3 : 1;
}

Display displayOf(const plx::Object &d) {
    Display r;
    r.visible = plx::live<int32_t>(d.property("Display.visibility"), 1) != 0;
    r.clips = plx::live<int32_t>(d.property("Display.clipping"), 0) != 0;
    r.fill = plx::live<glm::vec4>(d.property("Display.fillColor"), r.fill);
    r.stroke = plx::live<glm::vec4>(d.property("Display.strokeColor"), r.stroke);
    r.blurRadius = plx::live<float>(d.property("Display.blurRadius"), 0.0f);
    r.flags = leaf<int32_t>(d, "Display.flags", 0);
    return r;
}
glm::vec4 displayColor(const plx::Object &d) { return displayOf(d).fill; }
bool displayVisible(const plx::Object &d) { return displayOf(d).visible; }
bool displayClips(const plx::Object &d) { return displayOf(d).clips; }

// ---------------------------------------------------------------------------------------------
// Animation.

float bezier(float previous, float a, float b, float next, float smoothA, float smoothB, float u) {
    const float chord = (b - a) * 0.25f;
    const float handleA = smoothA * 0.25f * (b - previous) + (1.0f - smoothA) * chord;
    const float handleB = smoothB * 0.25f * (next - a) + (1.0f - smoothB) * chord;
    const float v = 1.0f - u;
    return a * v * v * v + (a + handleA) * 3.0f * u * v * v + (b - handleB) * 3.0f * u * u * v + b * u * u * u;
}

namespace {

// Inserts a key before the first whose time is not earlier, keeping the list sorted.
void insertByTime(std::vector<plx::Key> &keys, const plx::Key &k) {
    keys.insert(std::find_if(keys.begin(), keys.end(), [&](const plx::Key &e) { return e.time >= k.time; }), k);
}

std::vector<float> floatsOf(const std::vector<uint8_t> &bytes) {
    std::vector<float> f(bytes.size() / 4);
    if (!f.empty())
        std::memcpy(f.data(), bytes.data(), f.size() * 4);
    return f;
}

} // namespace

AttributeAnimation::AttributeAnimation(const plx::Chunk &attribute, bool isDiscrete) : discrete(isDiscrete), source(&attribute) {
    for (const std::vector<uint8_t> *slot : attribute.frames())
        slots.push_back(*slot);
    // Every attribute has at least its live value and the scratch slot.
    if (slots.empty())
        slots.emplace_back();
    if (slots.size() < 2)
        slots.push_back(slots[0]);
}

bool AttributeAnimation::play(const std::string &name, int length) {
    std::vector<plx::Key> keys;
    if (!source->sequence(name, keys) || keys.empty())
        return false;
    active.clear();
    period = length, clock = 0, segment = 0, lastJump = -1;
    for (const plx::Key &k : keys)
        if (period < 1 || k.time < period)
            insertByTime(active, k);
    if (!active.empty() && active[0].time != 0) {
        // The animation starts from wherever the value is now.
        slots[1] = slots[0];
        active.insert(active.begin(), plx::Key{1, 0, active[0].smoothness});
    }
    return !active.empty();
}

bool AttributeAnimation::evaluate(int t) {
    clock = t;
    const int n = (int)active.size();
    auto valid = [&](int slot) { return slot >= 0 && slot < (int)slots.size(); };
    // A jump to one key's pose, applied once.
    auto jump = [&](const plx::Key &k) {
        if (lastJump == k.slot || !valid(k.slot))
            return false;
        slots[0] = slots[(size_t)k.slot];
        lastJump = k.slot;
        return true;
    };
    if (n == 0)
        return false;
    if (n == 1 || t < active[0].time)
        return jump(active[0]);
    if (period == 0 && t >= active[(size_t)n - 1].time)
        return jump(active[(size_t)n - 1]);
    lastJump = -1;
    auto key = [&](int i) -> const plx::Key & {
        return active[(size_t)(period > 0 ? ((i % n) + n) % n : std::clamp(i, 0, n - 1))];
    };
    const int segments = period > 0 ? n : n - 1;
    if (segment < 0 || segment >= segments)
        segment = 0;
    for (int step = 0; step < segments; step++) {
        const int i = (segment + step) % segments;
        const plx::Key &a = key(i), &b = key(i + 1);
        if (!valid(a.slot) || !valid(b.slot))
            return false;
        int ta = a.time, tb = b.time;
        if (period > 0) {
            const int base = t / period * period;
            ta += base, tb += base;
            if (i == segments - 1)
                tb += period;
        }
        if (t < ta || t > tb)
            continue;
        // spec: open question - two keys at the same time would divide by zero; the later one
        // is taken.
        const float u = tb > ta ? (float)(t - ta) / (float)(tb - ta) : 1.0f;
        const plx::Key &before = key(i - 1), &after = key(i + 2);
        if (!valid(before.slot) || !valid(after.slot))
            return false;
        if (discrete) {
            slots[0] = slots[(size_t)(u < 1.0f ? a.slot : b.slot)];
        } else {
            const std::vector<float> pp = floatsOf(slots[(size_t)before.slot]), pa = floatsOf(slots[(size_t)a.slot]),
                                     pb = floatsOf(slots[(size_t)b.slot]), pn = floatsOf(slots[(size_t)after.slot]);
            std::vector<float> v(std::min({pp.size(), pa.size(), pb.size(), pn.size()}));
            for (size_t j = 0; j < v.size(); j++)
                v[j] = bezier(pp[j], pa[j], pb[j], pn[j], a.smoothness, b.smoothness, u);
            slots[0].assign(v.size() * 4, 0);
            if (!v.empty())
                std::memcpy(slots[0].data(), v.data(), v.size() * 4);
        }
        segment = i;
        return true;
    }
    return false;
}

bool AttributeAnimation::advance(int ms) {
    if (!active.empty() && evaluate(clock + ms))
        return true;
    active.clear(); // nothing changes any more: the animation has finished
    return false;
}

bool sampleSequence(const plx::Chunk &attribute, const std::string &name, float ms, std::vector<float> &out, bool discrete) {
    AttributeAnimation a(attribute, discrete);
    if (ms >= 0.0f) {
        if (!a.play(name))
            return false;
        a.evaluate((int)ms);
    } else {
        std::vector<plx::Key> keys;
        if (!attribute.sequence(name, keys))
            return false;
    }
    out = floatsOf(a.slots[0]);
    return true;
}

// ---------------------------------------------------------------------------------------------
// Widgets.

glm::vec2 Widget::effectiveSize() const { return bindSize + innerSize() - innerBindSize; }

glm::vec2 Widget::innerPosition() const { return framePos + innerBindPos - bindPos; }

glm::vec2 Widget::innerSize() const {
    return glm::max(frameSize + innerBindSize - bindSize, glm::max(glm::vec2(0), minimumSize + innerBindSize - bindSize));
}

Widget reboundWidget(const Widget &source) {
    Widget widget;
    widget.bind = source.bind;
    widget.inverseBind = glm::inverse(widget.bind);
    widget.bindPos = widget.framePos = source.framePos;
    widget.bindSize = widget.frameSize = source.frameSize;
    widget.innerBindPos = source.innerPosition();
    widget.innerBindSize = source.frameSize + source.innerBindSize - source.bindSize;
    widget.minimumSize = widget.bindSize - widget.innerBindSize;
    return widget;
}

Widget widgetOf(const plx::Object &o) {
    Widget w;
    w.kind = o.kind;
    w.innerBindPos = leaf(o, "Widget.innerBindPos", w.innerBindPos);
    w.innerBindSize = leaf(o, "Widget.innerBindSize", w.innerBindSize);
    w.bindPos = leaf(o, "Widget.bindPos", w.bindPos);
    w.bindSize = leaf(o, "Widget.bindSize", w.bindSize);
    w.framePos = leaf(o, "Widget.framePos", w.framePos);
    w.frameSize = leaf(o, "Widget.frameSize", w.frameSize);
    w.bind = leaf(o, "Widget.bindMatrix", w.bind);
    w.inverseBind = glm::inverse(w.bind);
    w.horizontalAlignment = leaf<int32_t>(o, "Widget.horizontalAlignment", ALIGN_LEFT);
    w.verticalAlignment = leaf<int32_t>(o, "Widget.verticalAlignment", ALIGN_TOP);
    w.flags = leaf<int32_t>(o, "Widget.flags", 0);
    w.buttonType = leaf<int32_t>(o, "Button.type", 0);
    w.direction = leaf<int32_t>(o, o.kind == "ScrollSlider" ? "ScrollSlider.direction" : "ScrollButton.direction", 0);
    if (const plx::Chunk *c = o.property("Widget.caption"))
        w.caption = c->wideText();
    w.minimumSize = w.bindSize - w.innerBindSize; // (as when the widget is set up)
    return w;
}

// ---------------------------------------------------------------------------------------------
// Nodes.

SceneNode *SceneNode::find(const std::string &n) {
    if (name == n)
        return this;
    for (auto &c : children)
        if (SceneNode *f = c->find(n))
            return f;
    return nullptr;
}

void SceneNode::findAll(const std::string &n, std::vector<SceneNode *> &out) {
    if (name == n)
        out.push_back(this);
    for (auto &c : children)
        c->findAll(n, out);
}

void SceneNode::setText(const std::string &n, const std::wstring &t) {
    if (disabled(*this))
        return;
    if (name == n) {
        setAllText(t);
        return;
    }
    for (auto &c : children)
        c->setText(n, t);
}

void SceneNode::setTextColour(const std::string &n, glm::vec4 colour) {
    if (disabled(*this))
        return;
    if (name == n) {
        setAllTextColor(colour);
        return;
    }
    for (auto &c : children)
        c->setTextColour(n, colour);
}

void SceneNode::setAllText(const std::wstring &t) {
    if (disabled(*this))
        return;
    if (isTextShape(*this))
        text = t;
    for (auto &c : children)
        c->setAllText(t);
}

void SceneNode::setAllTextColor(glm::vec4 colour) {
    if (disabled(*this))
        return;
    if (isTextShape(*this))
        textColor = colour;
    for (auto &c : children)
        c->setAllTextColor(colour);
}

int SceneNode::maskMode() const { return Display{visible, clips, fill, fill, 0, displayFlags}.maskMode(); }

// ---------------------------------------------------------------------------------------------
// The scene.

Scene::Scene(const plx::Document &document) : doc(document) { rootNode.name = "root"; }

SceneNode *Scene::create(SceneNode *parent, const std::string &name) {
    if (!parent)
        return nullptr;
    auto n = std::make_unique<SceneNode>();
    n->name = name;
    n->doc = &doc;
    n->parent = parent;
    parent->children.push_back(std::move(n));
    return parent->children.back().get();
}

SceneNode *Scene::instance(const std::string &name, SceneNode *parent) { return instance(doc, doc.find(name), parent); }
SceneNode *Scene::instance(int index, SceneNode *parent) { return instance(doc, index, parent); }
SceneNode *Scene::instance(const plx::Document &from, const std::string &name, SceneNode *parent) {
    return instance(from, from.find(name), parent);
}

SceneNode *Scene::instance(const plx::Document &from, int index, SceneNode *parent) {
    if (!parent || index < 0 || index >= (int)from.nodes.size())
        return nullptr;
    const plx::Node &source = from.nodes[(size_t)index];
    auto n = std::make_unique<SceneNode>();
    n->name = source.name;
    n->doc = &from;
    n->source = index;
    n->shape = objectAt(from.shapes, source.shape) ? source.shape : -1;
    n->flags = source.flags;
    if (const plx::Object *t = objectAt(from.transformations, source.transformation))
        n->transform = transformationOf(*t);
    if (const plx::Object *d = objectAt(from.displays, source.display)) {
        const Display display = displayOf(*d);
        n->visible = display.visible, n->clips = display.clips, n->fill = display.fill, n->displayFlags = display.flags;
        n->blurRadius = display.blurRadius;
    }
    if (const plx::Object *w = objectAt(from.widgets, source.widget))
        n->widget = widgetOf(*w);
    n->parent = parent;
    SceneNode *made = n.get();
    parent->children.push_back(std::move(n));
    for (int c : source.children)
        if (c >= 0 && c < (int)from.nodes.size() && !(from.nodes[(size_t)c].flags & plx::NODE_DISABLED))
            instance(from, c, made);
    return made;
}

SceneNode *Scene::instance(const SceneNode &from, SceneNode *parent) {
    if (!parent)
        return nullptr;
    auto n = std::make_unique<SceneNode>();
    n->name = from.name;
    n->doc = from.doc;
    n->source = from.source;
    n->shape = from.shape;
    n->flags = from.flags;
    n->transform = from.transform;
    n->local = from.local;
    n->visible = from.visible, n->clips = from.clips, n->fill = from.fill, n->displayFlags = from.displayFlags;
    n->blurRadius = from.blurRadius;
    n->widget = from.widget;
    n->text = from.text, n->textColor = from.textColor;
    n->onDraw = from.onDraw;
    n->animatedShape = from.animatedShape;
    n->animations = from.animations;
    n->parent = parent;
    SceneNode *made = n.get();
    parent->children.push_back(std::move(n));
    for (const auto &c : from.children)
        if (!disabled(*c))
            instance(*c, made);
    return made;
}

glm::mat4 Scene::transformation(const SceneNode &n) const {
    if (&n == &rootNode)
        return glm::scale(glm::mat4(1), glm::vec3(rootScale, rootScale, 1));
    return n.local ? *n.local : n.transform.local();
}

glm::mat4 Scene::matrix(const SceneNode &n) const {
    return n.parent ? matrix(*n.parent) * transformation(n) : transformation(n);
}

glm::mat4 Scene::inverseMatrix(const SceneNode &n) const {
    const auto local = glm::inverse(transformation(n));
    return n.parent ? local * inverseMatrix(*n.parent) : local;
}

glm::vec2 Scene::widgetPointer(const SceneNode &n, glm::vec2 point) const {
    const glm::vec2 displacement = point - contentPosition(n);
    return linear(inverseMatrix(n), displacement);
}

void Scene::setPosition(SceneNode &n, glm::vec2 point, glm::vec2 offset) {
    const glm::mat4 parentWorld = n.parent ? matrix(*n.parent) : glm::mat4(1);
    n.transform.translation = apply(glm::inverse(parentWorld), point) + offset - n.transform.pivot;
}

const ShapeDrawing *Scene::drawing(const SceneNode &n) const {
    if (n.animatedShape) {
        if (!n.animatedDrawing) {
            ShapeDrawing shape;
            if (!buildSmoothMesh(*n.animatedShape, shape))
                return nullptr;
            n.animatedDrawing = std::move(shape);
        }
        return &*n.animatedDrawing;
    }
    if (!n.doc || n.shape < 0 || n.shape >= (int)n.doc->shapes.size())
        return nullptr;
    const auto key = std::make_pair(n.doc, n.shape);
    auto it = drawings.find(key);
    if (it == drawings.end()) {
        ShapeDrawing d;
        if (!buildSmoothMesh(n.doc->shapes[(size_t)n.shape], d))
            return nullptr;
        it = drawings.emplace(key, std::move(d)).first;
    }
    return &it->second;
}

bool Scene::screenBounds(const SceneNode &node, glm::vec2 &minimum, glm::vec2 &maximum, const LocalBounds &localBounds) const {
    bool any = false;
    auto extend = [&](glm::vec2 lo, glm::vec2 hi) {
        minimum = any ? glm::min(minimum, lo) : lo;
        maximum = any ? glm::max(maximum, hi) : hi;
        any = true;
    };
    const glm::mat4 world = matrix(node);
    if (const auto *shape = drawing(node)) {
        auto vertices = [&](const Triangles &triangles) {
            for (const auto &vertex : triangles.vertices) {
                const glm::vec2 point = apply(world, vertex.position);
                extend(point, point);
            }
        };
        if (shape->filled)
            vertices(shape->fill);
        if (shape->stroked)
            vertices(shape->stroke);
    } else if (localBounds) {
        glm::vec2 lo, hi;
        if (localBounds(node, lo, hi))
            for (const auto corner : {lo, hi, glm::vec2(lo.x, hi.y), glm::vec2(hi.x, lo.y)}) {
                const glm::vec2 point = apply(world, corner);
                extend(point, point);
            }
    }
    if (!node.clips)
        for (const auto &child : node.children) {
            glm::vec2 lo, hi;
            if (child->visible && !disabled(*child) && screenBounds(*child, lo, hi, localBounds))
                extend(lo, hi);
        }
    if (any && (node.displayFlags & 1) && node.blurRadius > 0) {
        const float blur = (glm::length(glm::vec2(world[0])) + glm::length(glm::vec2(world[1]))) * 0.5f * node.blurRadius;
        minimum -= blur, maximum += blur;
    }
    return any;
}

bool Scene::shapeBounds(const SceneNode &n, glm::vec2 &minimum, glm::vec2 &maximum, bool deep) const {
    bool any = false;
    auto extend = [&](glm::vec2 lo, glm::vec2 hi) {
        minimum = any ? glm::min(minimum, lo) : lo;
        maximum = any ? glm::max(maximum, hi) : hi;
        any = true;
    };
    if (const ShapeDrawing *d = drawing(n); d && (d->filled || d->stroked))
        extend(d->minimum, d->maximum);
    if (deep)
        for (const auto &c : n.children) {
            glm::vec2 lo, hi;
            if (disabled(*c) || !shapeBounds(*c, lo, hi, true))
                continue;
            const glm::mat4 m = transformation(*c);
            for (glm::vec2 corner : {lo, hi, glm::vec2(lo.x, hi.y), glm::vec2(hi.x, lo.y)}) {
                const glm::vec2 p = apply(m, corner);
                extend(p, p);
            }
        }
    return any;
}

void Scene::setIcon(SceneNode &node, const std::string &targetName, const plx::Document &icons, const std::string &iconName) {
    SceneNode *target = node.find(targetName);
    if (!target)
        return;
    auto &kids = target->children;
    kids.erase(std::remove_if(kids.begin(), kids.end(), [](const auto &c) { return c->name == "icon"; }), kids.end());
    glm::vec2 lo, hi, iconLo, iconHi;
    if (!shapeBounds(*target, lo, hi, false))
        return;
    SceneNode *icon = instance(icons, iconName, target);
    if (!icon)
        return;
    icon->name = "icon";
    // The icon as its own transformation shows it (that may flip or turn it), measured in the
    // target's space, then moved and scaled onto the target's shape.
    const glm::mat4 own = transformation(*icon);
    if (!shapeBounds(*icon, iconLo, iconHi, true))
        return;
    glm::vec2 placedLo(0), placedHi(0);
    bool first = true;
    for (glm::vec2 corner : {iconLo, iconHi, glm::vec2(iconLo.x, iconHi.y), glm::vec2(iconHi.x, iconLo.y)}) {
        const glm::vec2 p = apply(own, corner);
        placedLo = first ? p : glm::min(placedLo, p), placedHi = first ? p : glm::max(placedHi, p), first = false;
    }
    if (placedHi.x <= placedLo.x || placedHi.y <= placedLo.y)
        return;
    const glm::vec2 ratio = (hi - lo) / (placedHi - placedLo);
    const float s = std::min(ratio.x, ratio.y);
    glm::mat4 m = glm::translate(glm::mat4(1), glm::vec3((lo + hi) * 0.5f, 0));
    m = glm::scale(m, glm::vec3(s, s, 1));
    icon->local = glm::translate(m, glm::vec3(-(placedLo + placedHi) * 0.5f, 0)) * own;
}

// ---------------------------------------------------------------------------------------------
// Widget geometry.

SceneNode *Scene::parentWidget(const SceneNode &n) const {
    for (SceneNode *p = n.parent; p; p = p->parent)
        if (p->widget)
            return p;
    return nullptr;
}

glm::mat4 Scene::contentMatrix(const SceneNode &n) const {
    if (!n.widget)
        return matrix(n);
    return glm::translate(matrix(n) * n.widget->inverseBind, glm::vec3(n.widget->framePos, 0));
}

// The frame matrix without the engine root's own matrix.
glm::mat4 Scene::frameToScreen(const SceneNode &n) const { return glm::inverse(transformation(rootNode)) * contentMatrix(n); }

glm::mat4 Scene::placementMatrix(const SceneNode &n) const {
    if (const SceneNode *above = parentWidget(n)) {
        const glm::vec2 origin = apply(frameToScreen(*above), glm::vec2(0));
        return glm::translate(glm::mat4(1), glm::vec3(-origin, 0)) * frameToScreen(n);
    }
    // spec: open question 9 - without a widget above, the parent's world matrix multiplies a
    // matrix that already holds it; reproduced as the game does it.
    const glm::mat4 parentWorld = n.parent ? matrix(*n.parent) : glm::mat4(1);
    return parentWorld * frameToScreen(n);
}

glm::vec2 Scene::contentPosition(const SceneNode &n) const { return apply(contentMatrix(n), glm::vec2(0)); }
glm::vec2 Scene::contentSize(const SceneNode &n) const { return n.widget ? n.widget->effectiveSize() : glm::vec2(0); }
glm::vec2 Scene::screenSize(const SceneNode &n) const { return linear(contentMatrix(n), contentSize(n)); }
glm::vec2 Scene::viewPosition(const SceneNode &n) const { return apply(placementMatrix(n), glm::vec2(0)); }
glm::vec2 Scene::viewSize(const SceneNode &n) const { return linear(placementMatrix(n), contentSize(n)); }

// Set position (spec 2.7.3): the node's translation carries the move.
void Scene::moveTo(SceneNode &n, glm::vec2 position) {
    if (!n.widget)
        return;
    const glm::vec2 current = viewPosition(n);
    if (position == current)
        return;
    const glm::mat4 back = glm::inverse(placementMatrix(n) * n.widget->bind * glm::inverse(transformation(n)));
    n.transform.translation += apply(back, position) - apply(back, current);
}

// Set size (spec 2.7.3): the frame size, and the change of the inner rectangle handed on to the
// widgets below.
void Scene::resizeTo(SceneNode &n, glm::vec2 size) {
    if (!n.widget)
        return;
    Widget &w = *n.widget;
    const glm::mat2 toView(placementMatrix(n));
    if (glm::determinant(toView) == 0.0f)
        return;
    const glm::vec2 before = w.innerSize();
    w.frameSize = glm::inverse(toView) * size;
    const glm::vec2 delta = toView * (w.innerSize() - before);
    if (!disabled(n) && delta != glm::vec2(0)) {
        w.dirty = true;
        propagateResize(n, glm::vec2(0), delta);
    }
    if (onResized)
        onResized(n);
}

// Hands a size change down to the first widget on each branch below a node.
void Scene::propagateResize(SceneNode &node, glm::vec2 offset, glm::vec2 delta) {
    for (auto &c : node.children) {
        if (c->widget)
            place(*c, offset, delta);
        else
            propagateResize(*c, offset, delta);
    }
}

// Placement on the parent's resize (spec 2.7.5).
void Scene::place(SceneNode &n, glm::vec2 offset, glm::vec2 delta) {
    if (!n.parent || !n.widget)
        return;
    const Widget w = *n.widget;
    glm::vec2 at = viewPosition(n);
    if (w.horizontalAlignment == ALIGN_RIGHT && !(w.flags & WIDGET_STRETCH_X))
        moveTo(n, glm::vec2(at.x + offset.x + delta.x, at.y));
    else if (w.horizontalAlignment == ALIGN_CENTER)
        moveTo(n, glm::vec2(at.x + 0.5f * delta.x, at.y));
    // spec: open question - the spec writes the vertical move with the position read before the
    // horizontal one, which would undo it; the position is read again here.
    at = viewPosition(n);
    if (w.verticalAlignment == ALIGN_BOTTOM && !(w.flags & WIDGET_STRETCH_Y))
        moveTo(n, glm::vec2(at.x, at.y + offset.y + delta.y));
    else if (w.verticalAlignment == ALIGN_MIDDLE)
        moveTo(n, glm::vec2(at.x, at.y + 0.5f * delta.y));
    if (w.flags & (WIDGET_STRETCH_X | WIDGET_STRETCH_Y)) {
        glm::vec2 size = linear(placementMatrix(n), w.frameSize);
        if (w.flags & WIDGET_STRETCH_X)
            size.x += delta.x;
        if (w.flags & WIDGET_STRETCH_Y)
            size.y += delta.y;
        resizeTo(n, size);
    }
    // (scroll sliders and the scroll clamp of 2.7.7 have no widgets here to act on)
}

// ---------------------------------------------------------------------------------------------
// Drawing.

namespace {

// The nine-slice stretch of a node's points by its owner widget (spec 2.7.4), as the engine's
// vertex shader applies it: only while the inner rectangle's size differs from its design size.
struct Deformer {
    glm::mat4 world{1}, toWidget{1}, fromWidget{1};
    glm::vec2 innerBindPos{0}, innerBindSize{0}, innerPosition{0}, innerSize{0};
    bool active = false;

    glm::vec2 operator()(glm::vec2 p) const {
        if (!active)
            return apply(world, p);
        glm::vec2 q = apply(toWidget, p);
        glm::vec2 t;
        for (int axis = 0; axis < 2; axis++) {
            if (q[axis] < innerBindPos[axis])
                t[axis] = 0.0f;
            else if (q[axis] >= innerBindPos[axis] + innerBindSize[axis])
                t[axis] = 1.0f;
            else
                t[axis] = (q[axis] - innerBindPos[axis]) / innerBindSize[axis];
        }
        q += innerPosition - innerBindPos + t * (innerSize - innerBindSize);
        return apply(fromWidget, q);
    }
    glm::vec2 inverse(glm::vec2 point) const {
        if (!active)
            return apply(glm::inverse(world), point);
        glm::vec2 q = apply(glm::inverse(fromWidget), point);
        glm::vec2 t;
        for (int axis = 0; axis < 2; ++axis) {
            if (q[axis] < innerPosition[axis])
                t[axis] = 0;
            else if (q[axis] >= innerPosition[axis] + innerSize[axis])
                t[axis] = 1;
            else
                t[axis] = (q[axis] - innerPosition[axis]) / innerSize[axis];
        }
        q += innerBindPos - innerPosition + t * (innerBindSize - innerSize);
        return apply(glm::inverse(toWidget), q);
    }
};

// A node's stretch: by its owner widget (its own widget, or the nearest above it).
Deformer deformerOf(const Scene &scene, const SceneNode &n) {
    Deformer d;
    d.world = scene.matrix(n);
    const SceneNode *owner = &n;
    while (owner && !owner->widget)
        owner = owner->parent;
    if (!owner || owner->widget->innerSize() == owner->widget->innerBindSize)
        return d;
    const Widget &w = *owner->widget;
    const glm::mat4 ownerWorld = scene.matrix(*owner);
    d.active = true;
    d.toWidget = w.bind * glm::inverse(ownerWorld) * d.world;
    d.fromWidget = ownerWorld * w.inverseBind;
    d.innerBindPos = w.innerBindPos, d.innerBindSize = w.innerBindSize;
    d.innerPosition = w.innerPosition(), d.innerSize = w.innerSize();
    return d;
}

} // namespace

glm::vec2 Scene::deform(const SceneNode &n, glm::vec2 local) const { return deformerOf(*this, n)(local); }

const SceneNode *Scene::hitTest(const SceneNode &node, glm::vec2 point, const TextHit &textHit,
                                uint32_t options, bool clipWidgets) const {
    if (!node.visible || (node.flags & (plx::NODE_DISABLED | plx::NODE_NO_PICK)))
        return nullptr;
    const bool wholeHit = node.widget && (node.widget->flags & WIDGET_WHOLE_HIT);
    if (clipWidgets && !wholeHit && node.widget && (node.widget->flags & WIDGET_OWN_SURFACE)) {
        const glm::vec2 origin = contentPosition(node), size = screenSize(node);
        if (point.x < origin.x || point.y < origin.y || point.x > origin.x + size.x || point.y > origin.y + size.y)
            return nullptr;
    }
    bool hit = wholeHit;
    if (const ShapeDrawing *shape = drawing(node)) {
        const glm::vec2 local = deformerOf(*this, node).inverse(point);
        hit = hit || (shape->filled && contains(shape->fill, local)) || (shape->stroked && contains(shape->stroke, local));
    } else if (textHit && node.doc && node.shape >= 0 && node.shape < (int)node.doc->shapes.size() &&
               node.doc->shapes[(size_t)node.shape].kind == "TextShape")
        hit = hit || textHit(node, apply(glm::inverse(matrix(node)), point));
    if (!node.clips || hit)
        for (auto child = node.children.rbegin(); child != node.children.rend(); ++child)
            if (const SceneNode *found = hitTest(**child, point, textHit, options, clipWidgets))
                return found;
    const bool filtered = ((node.flags & plx::NODE_PASSIVE) && !(options & 2)) ||
                          ((node.flags & plx::NODE_NO_PICK_1) && (options & 1)) ||
                          ((node.flags & plx::NODE_LOADED) && (options & 4));
    return hit && !filtered ? &node : nullptr;
}

void Scene::drawShape(const SceneNode &n, glm::vec4 tint, Drawn &out) const {
    const ShapeDrawing *d = drawing(n);
    if (!d)
        return;
    const Deformer deformer = deformerOf(*this, n);
    for (const Triangles *t : {&d->fill, &d->stroke}) {
        if ((t == &d->fill && !d->filled) || (t == &d->stroke && !d->stroked))
            continue;
        for (uint32_t i : t->indices) {
            if (i >= t->vertices.size())
                continue;
            const Vertex &v = t->vertices[i];
            out.positions.push_back(deformer(v.position) + glm::vec2(DRAW_OFFSET));
            out.colors.push_back(v.color * tint);
        }
    }
}

} // namespace plasma
