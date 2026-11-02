// Native frame setter identity, boundary propagation and own-hook contracts.
#include "PlasmaScene.h"
#include "PlasmaDraw.h"
#include <cstdio>

int plasmaLayoutTests() {
    int failures = 0;
    auto check = [&](bool pass, const char *what) {
        if (!pass) {
            std::printf("FAIL Plasma layout: %s\n", what);
            ++failures;
        }
    };
    plx::Document doc;
    plasma::Scene scene(doc);
    auto add = [&](plasma::SceneNode &parent, glm::vec2 size, bool widget = true) {
        auto node = std::make_unique<plasma::SceneNode>();
        node->parent = &parent;
        if (widget) {
            node->widget.emplace();
            node->widget->bindSize = node->widget->innerBindSize = node->widget->frameSize = size;
        }
        auto *result = node.get();
        parent.children.push_back(std::move(node));
        return result;
    };
    auto *panel = add(scene.root(), {100, 100});
    auto *child = add(*panel, {10, 10});
    child->widget->flags = plasma::WIDGET_STRETCH_X | plasma::WIDGET_STRETCH_Y;
    panel->widget->kind = "Button";
    panel->widget->buttonType = 2;
    panel->widget->enabled = false;
    panel->widget->checked = true;
    auto *identity = &*panel->widget;
    int hooks = 0;
    scene.onResized = [&](plasma::SceneNode &node) { hooks += &node == panel; };
    scene.resizeTo(*panel, {200, 200}, false);
    check(panel->widget->frameSize == glm::vec2(200) && child->widget->frameSize == glm::vec2(10) && hooks == 1,
          "flag0 changes the receiver and invokes its own hook without crossing child Widget boundaries");
    check(&*panel->widget == identity && panel->widget->kind == "Button" && panel->widget->buttonType == 2 && !panel->widget->enabled &&
              panel->widget->checked,
          "size setting preserves Widget identity, dynamic class and Button state");
    scene.resizeTo(*panel, {300, 300}, true);
    check(child->widget->frameSize == glm::vec2(110) && hooks == 2, "flag1 delivers the native size delta to a stretch child");
    child->flags |= plx::NODE_DISABLED;
    auto *excluded = add(*panel, {}, false);
    excluded->flags |= plx::NODE_DISABLED;
    auto *nested = add(*excluded, {10, 10});
    nested->widget->flags = plasma::WIDGET_STRETCH_X | plasma::WIDGET_STRETCH_Y;
    scene.resizeTo(*panel, {400, 400});
    check(child->widget->frameSize == glm::vec2(110) && nested->widget->frameSize == glm::vec2(10),
          "disabled Widget and non-Widget subtrees both exclude descendant placement");
    panel->widget->dirty = false;
    const int beforeSame = hooks;
    scene.resizeTo(*panel, {400, 400}, false);
    check(hooks == beforeSame + 1 && !panel->widget->dirty, "unchanged flag0 size still invokes the hook without a dirty write");
    panel->flags |= plx::NODE_DISABLED;
    const int beforeExcluded = hooks;
    scene.resizeTo(*panel, {500, 500});
    check(panel->widget->frameSize == glm::vec2(500) && !panel->widget->dirty && hooks == beforeExcluded + 1,
          "excluded receiver still gets frame data and its final own hook");
    auto *aligned = add(scene.root(), {10, 20});
    aligned->widget->horizontalAlignment = plasma::ALIGN_RIGHT;
    aligned->widget->verticalAlignment = plasma::ALIGN_BOTTOM;
    scene.moveTo(*aligned, {100, 200});
    scene.placeWidget(*aligned, {3, 4}, {20, 40});
    check(scene.viewPosition(*aligned) == glm::vec2(123, 244) && !aligned->widget->dirty,
          "native right/bottom numeric reference preserves post-horizontal X without Widget dirty");
    auto *minimum = add(scene.root(), {10, 20});
    minimum->widget->minimumSize = {30, 40};
    minimum->widget->flags = plasma::WIDGET_STRETCH_X;
    int minimumHooks = 0;
    scene.onResized = [&](plasma::SceneNode &node) { minimumHooks += &node == minimum; };
    scene.placeWidget(*minimum, {}, {5, 0});
    check(minimum->widget->frameSize == glm::vec2(15, 20) && scene.contentSize(*minimum) == glm::vec2(30, 40) && minimumHooks == 1,
          "stretch uses raw frame instead of minimum size, then runs its size hook");
    scene.placeWidget(*minimum, {}, {});
    check(minimumHooks == 2, "zero-delta stretch still reaches the size setter");
    using Layer = plasma::LayerCommand;
    const std::vector<Layer> balanced{{Layer::Kind::Push, {0, 0}},    {Layer::Kind::Push, {3, 0}}, {Layer::Kind::Pop, {6, 1}},
                                      {Layer::Kind::Mask, {6, 1}, 2}, {Layer::Kind::Pop, {9, 1}},  {Layer::Kind::Mask, {9, 1}, 3},
                                      {Layer::Kind::Mask, {12, 2}}};
    check(plasma::layerDepth(balanced, {12, 2}) == 2, "clip commands preserve nested vertex/callback cuts");
    auto invalid = balanced;
    invalid.pop_back();
    invalid.pop_back();
    invalid.pop_back();
    check(!plasma::layerDepth(invalid, {12, 2}), "unbalanced clip nesting is rejected before GPU recording");
    invalid = {{Layer::Kind::Push, {0, 0}},
               {Layer::Kind::Pop, {0, 0}},
               {Layer::Kind::Push, {0, 0}},
               {Layer::Kind::Mask, {0, 0}, 2},
               {Layer::Kind::Pop, {3, 0}}};
    check(!plasma::layerDepth(invalid, {3, 0}), "sampling the current clip attachment is rejected");
    // Exercise the rendering facade with synthetic geometry. Point queries keep
    // their own divisions and must not provide the drawing's homogeneous values.
    auto *projective = add(scene.root(), {}, false);
    projective->animatedShape.emplace();
    projective->animatedDrawing.emplace();
    auto &mesh = *projective->animatedDrawing;
    mesh.filled = true;
    mesh.fill.vertices = {{{4, 2}}, {{5, 2}}, {{4, 3}}};
    mesh.fill.indices = {0, 1, 2};
    glm::mat4 world(1);
    world[0][3] = .25f;
    world[3][2] = -3;
    projective->local = world;
    plasma::Scene::Drawn drawn;
    scene.drawShape(*projective, glm::vec4(1), drawn);
    const auto close = [](float a, float b) { return std::abs(a - b) < .00001f; };
    check(drawn.positions.size() == 3 && close(drawn.positions[0].x, 4 + plasma::DRAW_OFFSET) &&
              close(drawn.positions[0].y, 2 + plasma::DRAW_OFFSET) && drawn.positions[0].z == -3 && drawn.positions[0].w == 2,
          "ordinary drawing retains WORLD Z/W and applies an unscaled XY bias");
    check(scene.deform(*projective, {4, 2}) == glm::vec2(2, 1), "point-query division remains separate from drawing");
    projective->local = glm::mat4(1);
    projective->widget.emplace();
    auto &frame = *projective->widget;
    frame.bindSize = frame.innerBindSize = {10, 10};
    frame.frameSize = {20, 20};
    frame.framePos = {2, 0};
    frame.bind[0][3] = .25f;
    frame.inverseBind = glm::inverse(frame.bind);
    mesh.fill.vertices = {{{1, 2}}, {{2, 2}}, {{1, 3}}};
    drawn = {};
    scene.drawShape(*projective, glm::vec4(1), drawn);
    check(drawn.positions.size() == 3 && close(drawn.positions[0].x, 4 + plasma::DRAW_OFFSET) &&
              close(drawn.positions[0].y, 4 + plasma::DRAW_OFFSET) && close(drawn.positions[0].w, .25f),
          "Widget drawing preserves bind W through piecewise deformation and inverse bind");
    check(scene.deform(*projective, {1, 2}).x > 30, "the divided CPU query is a distinct observable control");
    auto *descendant = add(*projective, {}, false);
    descendant->animatedShape.emplace();
    descendant->animatedDrawing = mesh;
    glm::mat4 shifted(1);
    shifted[3][0] = .25f;
    descendant->local = shifted;
    drawn = {};
    scene.drawShape(*descendant, glm::vec4(1), drawn);
    check(drawn.positions.size() == 3 && close(drawn.positions[0].x, 4.5f + plasma::DRAW_OFFSET) &&
              close(drawn.positions[0].y, 4 + plasma::DRAW_OFFSET) && close(drawn.positions[0].w, .1875f),
          "descendant drawing uses both grouped owner mappings before its separate WORLD");
    std::printf("Plasma layout: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
