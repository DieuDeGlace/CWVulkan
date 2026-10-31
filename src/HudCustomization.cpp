// Owner VoxelWidget tree and after-children customization callback.
#include "Hud.h"
#include "IdentifyWindow.h"
#include "VoxelCustomization.h"

customization::Layout Hud::customizationLayout(bool shown) const {
    customization::Layout layout;
    if (!scene || !panels.customization || !panels.bag.node)
        return layout;
    auto &tree = *scene;
    auto &root = *panels.customization;
    root.visible = shown;
    tree.setScale(guiScale);
    const auto size = tree.viewSize(root), inventory = tree.viewSize(*panels.bag.node);
    tree.setPosition(root, {(float)((int32_t)screen.x / 2) - size.x * .5f, (float)((int32_t)screen.y / 2) - inventory.y}, {0, -50});
    layout.root = tree.matrix(root);
    layout.pivot = root.transform.pivot;
    layout.parentSize = tree.viewSize(root);
    layout.viewport = glm::ivec2(screen);
    layout.scale = layout.root[0][0];
    return layout;
}

bool Hud::customizationHit(glm::vec2 pointer) const {
    return scene && panels.customization && panels.customization->visible && scene->hitTest(*panels.customization, pointer) != nullptr;
}

bool Hud::customizationGuiHit(glm::vec2 pointer, bool identificationShown) const {
    if (customizationHit(pointer))
        return true;
    if (!scene)
        return false;
    if (identificationShown && panels.identification) {
        identificationLayout();
        auto &root = *panels.identification;
        const bool wasVisible = root.visible;
        root.visible = true;
        const bool hit = scene->hitTest(root, pointer) != nullptr;
        root.visible = wasVisible;
        if (hit)
            return true;
    }
    const auto *chat = placeChat();
    return chat && scene->hitTest(*chat, pointer) != nullptr;
}

void Hud::drawCustomization(const customization::Shown &shown, std::vector<OverlayVertex> &out) const {
    if (!shown.state || !shown.inventory || !panels.customization)
        return;
    const auto layout = customizationLayout(shown.state->edit.open);
    auto &root = *panels.customization;
    auto resolve = [&](const std::string &key) -> customization::Resource {
        const auto found = itemModels.find(key);
        if (found == itemModels.end())
            return {};
        return {found->second.mesh, found->second.allocation, found->second.volume.get()};
    };
    sceneNode(out, root, glm::vec4(1), nullptr, [&](const plasma::SceneNode &node) {
        if (&node != &root)
            return;
        auto text = [&](const std::wstring &value, glm::vec2 at, glm::vec4 color, int flags = 0) {
            widgetText(out, root, value, at, 12, 3, color, {0, 0, 0, 1}, flags, 0);
        };
        text(L"Weapon Customization", {15, 25}, {1, .8f, .5f, 1});
        auto plan = customization::draw(
            *shown.state, layout, shown.equipment, *shown.inventory, shown.pointer, guiClockMs,
            [&](const auto &item) { return shown.icon ? resolve(shown.icon(item)) : customization::Resource{}; }, resolve("cube"));
        for (auto &image : plan.images) {
            image.beforeVertex = out.size();
            itemRequests.push_back(std::move(image));
        }
        if (plan.upgrades)
            text(L"Upgrades " + std::to_wstring(plan.upgrades->first) + L"/" + std::to_wstring(plan.upgrades->second) + L"\n",
                 {20, scene->contentSize(root).y - 20}, glm::vec4(1));
        const auto &palette = shown.state->edit.palette;
        if (plan.upgrades)
            for (size_t i = 0; i < palette.size(); ++i)
                text(std::to_wstring(palette[i].count) + L"\n", {float(60 + 45 * (int)(i % 8)), float(80 + 45 * (int)(i / 8))},
                     glm::vec4(1), plasma::TEXT_RIGHT);
    });
}
