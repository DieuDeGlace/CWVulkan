#include "PlasmaScene.h"
#include "PlasmaShape.h"
#include <cstdio>

int plasmaHitTests() {
    int failures = 0;
    auto check = [&](bool pass, const char *what) {
        if (!pass) { std::printf("FAIL Plasma hit: %s\n", what); ++failures; }
    };
    plasma::SmoothMesh rectangle;
    rectangle.positions = {{0, 0}, {10, 0}, {10, 10}, {0, 10}};
    rectangle.faces = {{0, 1, 2, 3}};
    rectangle.subdivisions = 0;
    rectangle.colors.assign(4, glm::vec4(1));
    rectangle.texCoords.assign(4, glm::vec2(0));
    rectangle.strokeColors.assign(4, glm::vec4(1));
    rectangle.strokeWidths.assign(4, 0);
    rectangle.vertexFlags.assign(4, plasma::POINT_LINEAR);
    rectangle.vertexParameters.assign(4, 0);
    const auto built = plasma::build(rectangle);
    check(plasma::contains(built.fill, {5, 5}), "drawing boundaries come from final triangles");
    check(!plasma::contains(built.fill, {0, 5}) && !plasma::contains(built.fill, {5, 0}), "parity excludes left and top edges");
    check(plasma::contains(built.fill, {10, 5}) && plasma::contains(built.fill, {5, 10}), "parity includes right and bottom edges");
    check(!plasma::contains(built.fill, {11, 5}), "drawing bounds reject outside points");
    plasma::Triangles hole = built.fill;
    hole.vertices.insert(hole.vertices.end(), {{{3, 3}}, {{7, 3}}, {{7, 7}}, {{3, 7}}});
    hole.outlines.push_back({4, 5, 6, 7});
    check(!plasma::contains(hole, {5, 5}) && plasma::contains(hole, {1, 5}), "all loops participate in even-odd containment");
    hole.outlines.push_back({1000});
    check(!plasma::contains(hole, {1, 5}), "invalid outline index rejects drawing");

    plx::Document doc;
    plasma::Scene scene(doc);
    auto *parent = scene.create(&scene.root(), "parent");
    auto *lower = scene.create(parent, "lower");
    auto *upper = scene.create(parent, "upper");
    for (auto *node : {lower, upper}) {
        node->widget.emplace();
        node->widget->flags = plasma::WIDGET_WHOLE_HIT;
    }
    check(scene.hitTest(*parent, {500, 500}) == upper, "later child wins and whole-hit flag is unconditional");
    upper->flags = plx::NODE_PASSIVE;
    check(scene.hitTest(*parent, {500, 500}) == lower, "passive bit filters self");
    check(scene.hitTest(*upper, {500, 500}, {}, 2) == upper, "option two permits passive self");
    upper->flags = 0;
    parent->flags = plx::NODE_NO_PICK;
    check(!scene.hitTest(*parent, {500, 500}), "no-pick bit prunes whole subtree");
    parent->flags = plx::NODE_PASSIVE;
    check(scene.hitTest(*parent, {500, 500}) == upper, "passive parent still visits children");
    parent->visible = false;
    check(!scene.hitTest(*parent, {500, 500}), "visibility prunes subtree");
    parent->visible = true;
    parent->clips = true;
    check(!scene.hitTest(*parent, {500, 500}), "missed clipping node gates descendants");
    parent->widget.emplace();
    parent->widget->flags = plasma::WIDGET_WHOLE_HIT;
    check(scene.hitTest(*parent, {500, 500}) == upper, "hit clip node permits descendants");
    lower->flags = upper->flags = plx::NODE_LOADED;
    check(scene.hitTest(*parent, {500, 500}) == upper, "loaded bit does not filter default picks");
    check(!scene.hitTest(*parent, {500, 500}, {}, 4), "option four filters loaded children and passive parent");
    auto *box = scene.create(&scene.root(), "box");
    box->transform.translation = {20, 30};
    box->visible = false;
    box->flags = plx::NODE_DISABLED | plx::NODE_NO_PICK;
    auto *childBox = scene.create(box, "childBox");
    childBox->transform.translation = {100, 0};
    auto localBounds = [&](const plasma::SceneNode &node, glm::vec2 &lo, glm::vec2 &hi) {
        if (node.name != "box" && node.name != "childBox")
            return false;
        lo = {0, 0}, hi = {10, 10};
        return true;
    };
    glm::vec2 lo, hi;
    check(scene.screenBounds(*box, lo, hi, localBounds) && lo == glm::vec2(20, 30) && hi == glm::vec2(130, 40),
          "bounds include a hidden disabled starting node and its eligible children");
    childBox->flags = plx::NODE_NO_PICK;
    scene.screenBounds(*box, lo, hi, localBounds);
    check(hi.x == 130, "no-pick flag does not exclude descendant bounds");
    childBox->visible = false;
    scene.screenBounds(*box, lo, hi, localBounds);
    check(hi == glm::vec2(30, 40), "hidden descendants are omitted");
    childBox->visible = true;
    box->clips = true;
    scene.screenBounds(*box, lo, hi, localBounds);
    check(hi == glm::vec2(30, 40), "clipping stops bounds recursion rather than clipping child rectangles");
    box->displayFlags = 1;
    box->blurRadius = 4;
    scene.screenBounds(*box, lo, hi, localBounds);
    check(lo == glm::vec2(16, 26) && hi == glm::vec2(34, 44), "blur expands the resulting bounds");
    std::puts("Plasma picking: drawing parity, boundaries, flags, clipping and navigation bounds checked");
    return failures;
}
