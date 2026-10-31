// The owner identification tree and manual text, from steam-identification-ui.md.
#include "Hud.h"
#include "IdentifyWindow.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

identifywindow::Layout Hud::identificationLayout() const {
    identifywindow::Layout layout;
    if (!scene || !panels.bag.node || !panels.identification || !panels.identification->widget || !panels.identificationFrame ||
        !panels.identificationFrame->widget)
        return layout;
    auto &tree = *scene;
    auto &root = *panels.identification;
    auto &frame = *panels.identificationFrame;
    tree.setScale(guiScale);
    const auto inventory = tree.viewSize(*panels.bag.node);
    const glm::vec2 half{(float)((int32_t)screen.x / 2), (float)((int32_t)screen.y / 2)};
    tree.setPosition(root, {half.x + inventory.x * .5f, half.y - inventory.y}, {10, -50});
    layout.size = tree.contentSize(root);
    tree.moveTo(frame, {(layout.size.x - tree.viewSize(frame).x) * .5f, 50});
    layout.frameToScreen = tree.contentMatrix(root);
    layout.nodeToScreen = tree.matrix(root);
    layout.textToScreen = glm::translate(tree.matrix(root), glm::vec3(root.transform.pivot, 0));
    layout.slotToScreen = tree.contentMatrix(frame);
    layout.slotSize = tree.contentSize(frame);
    layout.valid = true;
    return layout;
}

void Hud::drawIdentification(const identifywindow::Shown &shown, std::vector<OverlayVertex> &out) const {
    const auto layout = identificationLayout();
    if (!layout.valid)
        return;
    auto &root = *panels.identification;
    root.visible = true;
    sceneNode(out, root, glm::vec4(1)); // copied owner fills/outlines/inner geometry, including the child itemframe
    root.visible = false;
    auto draw = [&](const std::wstring &text, glm::vec2 at, float size, float outline, glm::vec4 colour, int flags = 0, float wrap = 0) {
        return widgetText(out, root, text, at, size, outline, colour, {0, 0, 0, 1}, flags, wrap);
    };
    auto wide = [](const std::string &text) {
        std::wstring result;
        for (unsigned char byte : text)
            result.push_back((wchar_t)byte);
        return result;
    };
    if (shown.slotted) {
        const glm::vec2 centre = scene->contentPosition(*panels.identificationFrame) + scene->screenSize(*panels.identificationFrame) * .5f;
        if (shown.original) {
            guiitem::Placement at;
            at.screen = centre;
            at.euler = {225, 0, guiitem::phase(guiClockMs)};
            at.size = 0.06f * layout.nodeToScreen[0][0];
            at.darkPass = true;
            itemIcon(shown.icon, *shown.original, at, out.size());
        } else if (const auto found = icons.find(shown.icon); found != icons.end() && found->second.w > 0) {
            // Older recording-only sessions have no original item record.
            const auto &icon = found->second;
            image(out, icon.atlas, {icon.w, icon.h}, centre.x - (float)icon.w * guiScale * .5f, centre.y - (float)icon.h * guiScale * .5f,
                  guiScale);
        }
        const float lastBaseline = draw(wide(shown.name), {15, 180}, 15, 4, shown.colour, plasma::TEXT_WRAP, 300);
        if (!shown.regionDescription.empty())
            draw(wide(shown.regionDescription), {15, lastBaseline + 24}, 12, 3, shown.regionColour, plasma::TEXT_WRAP, 300);
    }
    draw(L"Identification", {15, 25}, 12, 3, {1, .8f, .5f, 1}, shown.slotted ? plasma::TEXT_WRAP : 0, 300);
    const float footer = layout.size.y - 20;
    const glm::vec4 identifyColour = !shown.slotted ? glm::vec4(.7f, .7f, .7f, 1) : shown.overButton ? glm::vec4(0, 1, 1, 1) : glm::vec4(1);
    draw(L"Identify", {layout.size.x / 3, footer}, 12, 3, identifyColour, plasma::TEXT_CENTER);
    draw(L"Goodbye!", {(layout.size.x * 2) / 3, footer}, 12, 3, shown.overGoodbye ? glm::vec4(0, 1, 1, 1) : glm::vec4(1),
         plasma::TEXT_CENTER);
    if (shown.slotted) {
        const glm::vec2 at{15, (float)(int32_t)(layout.size.y - 50)};
        draw(L"COST: ", at, 12, 3, glm::vec4(1));
        plasma::TextStyle style;
        style.pixelSize = 12;
        style.scalable = true;
        style.lineSpacing = 1;
        const auto matrix = glm::translate(layout.textToScreen, glm::vec3(at, 0));
        const auto raster = plasma::rasterFor(matrix, style);
        const float advance = plasma::layout(plasmaFonts[0], L"COST: ", raster, 0).pen.x * (12 / raster.size.x);
        draw(std::to_wstring(shown.price) + L" GOLD ", {at.x + advance, at.y}, 12, 3, {1, .9f, 0, 1});
    }
}
