// Instance isolation, interrupted transitions and channel ownership, from Plasma spec2.6.
#include "PlasmaScene.h"
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {
template <class T> std::vector<uint8_t> bytes(const T &v) {
    std::vector<uint8_t> b(sizeof v);
    std::memcpy(b.data(), &v, sizeof v);
    return b;
}
plx::Chunk key(int slot, int time) {
    return {"Attribute.sequence.key",
            {},
            {{"Attribute.sequence.key.frame", bytes(slot), {}}, {"Attribute.sequence.key.time", bytes(time), {}}}};
}
plx::Chunk sequence(const std::string &name, int slot, int time) {
    std::vector<uint8_t> text(4 + name.size());
    int size = (int)name.size();
    std::memcpy(text.data(), &size, 4);
    std::memcpy(text.data() + 4, name.data(), name.size());
    return {"Attribute.sequence", {}, {{"Attribute.sequence.name", text, {}}, key(slot, time)}};
}
template <class T> plx::Chunk attribute(const std::string &name, T initial, T idle, T hover) {
    return {name,
            {},
            {{"Attribute.frame", bytes(initial), {}},
             {"Attribute.frame", bytes(initial), {}},
             {"Attribute.frame", bytes(idle), {}},
             {"Attribute.frame", bytes(hover), {}},
             sequence("idle", 2, 0),
             sequence("enter", 3, 100)}};
}
float colour(const plasma::SceneNode &n) {
    return plx::live(n.shapeObject()->property("SmoothMeshShape.vertexColors"), glm::vec4(-1)).r;
}
} // namespace

int plasmaNodeAnimationTests() {
    int failures = 0;
    auto check = [&](bool pass, const char *what) {
        if (!pass) {
            std::printf("FAIL Plasma node animation: %s\n", what);
            failures++;
        }
    };
    auto near = [](float a, float b) { return std::abs(a - b) < 1e-6f; };
    plx::Document doc;
    doc.shapes.push_back({"SmoothMeshShape", {attribute("SmoothMeshShape.vertexColors", glm::vec4(.8f), glm::vec4(0), glm::vec4(.8f))}});
    doc.transformations.push_back(
        {"Transformation", {attribute("Transformation.rotation", glm::vec3(0), glm::vec3(0), glm::vec3(0, 0, 90))}});
    doc.nodes.resize(2);
    doc.nodes[1].name = "sample";
    doc.nodes[1].shape = doc.nodes[1].transformation = 0;
    doc.shapes[0].properties[0].children.push_back(sequence("button:release", 3, 0));
    doc.shapes[0].properties[0].children.push_back(sequence("button:leave", 2, 0));
    doc.widgets.push_back({"Button", {{"Button.type", bytes(int32_t(0)), {}}}});
    doc.nodes[1].widget = 0;
    plasma::Scene scene(doc);
    auto *first = scene.instance(1, &scene.root());
    auto *second = scene.instance(1, &scene.root());
    check(first && second, "two instances");
    if (!first || !second)
        return 1;
    first->initializeWidgets();
    check(colour(*first) == 0, "ordinary Button initializes the owner leave pose");
    first->poseSequence("idle");
    check(colour(*first) == 0 && near(colour(*second), .8f), "pose must not change another instance");
    check(near(plx::live(doc.shapes[0].property("SmoothMeshShape.vertexColors"), glm::vec4(-1)).r, .8f), "owner document is immutable");
    first->transform.translation = {12, 34};
    check(first->playSequence("enter"), "play named channels");
    first->transform.translation = {45, 67}; // layout writes after play, before the next animation tick
    first->advanceSequences(50);
    check(near(colour(*first), .4f), "transition begins at the current idle value");
    check(near(first->transform.rotation.z, 45), "rotation channel is evaluated");
    check(first->transform.translation == glm::vec2(45, 67), "rotation must preserve an independent layout write");
    first->playSequence("enter");
    first->advanceSequences(50);
    check(near(colour(*first), .6f), "restarted transition begins at the interrupted value");
    auto *copy = scene.instance(*first, &scene.root());
    first->advanceSequences(50);
    check(near(colour(*first), .8f) && near(colour(*copy), .6f), "copy owns its live shape and animation state");
    check(near(colour(*second), .8f), "unanimated instance keeps shared source drawing");
    copy->advanceSequences(50);
    check(near(colour(*copy), .8f), "copied active animation remains usable");
    auto *child = scene.instance(1, first);
    child->flags |= plx::NODE_DISABLED;
    first->playSequence("idle");
    check(!child->animations.empty(), "play visits disabled children");
    first->setSequenceTime(0);
    check(colour(*first) == 0 && near(colour(*child), .8f), "set time skips disabled subtrees");
    first->stopSequences();
    check(!child->animations[0].value.active.empty(), "stop skips disabled children");
    first->visible = false;
    first->playSequence("enter");
    first->advanceSequences(100);
    check(colour(*first) == 0, "invisible subtree does not advance");
    first->flags |= plx::NODE_ALWAYS_ANIMATE;
    first->advanceSequences(100);
    check(colour(*first) == 0, "own force flag does not admit hidden ordinary advance");
    first->setSequenceTime(100);
    check(near(colour(*first), .8f), "absolute seek admits the hidden node's own force flag");
    first->visible = true;
    first->advanceSequences(100);
    check(near(colour(*first), .8f), "visible subtree resumes its clock");
    check(!first->playSequence("missing"), "absent sequence does not start");
    std::printf("Plasma node animations: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
