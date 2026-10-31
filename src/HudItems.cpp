// Owner CUB resources and immediate widget requests (steam-gui-item-icons.md).
#include "Hud.h"
#include "ItemIcon.h"
#include "MapLook.h"

void Hud::addItemIcon(const std::string &key, const CubModel &cub) {
    if (itemModels.contains(key))
        return;
    // Compatibility consumers retain their atlas image until their own caller is converted.
    addIcon(key, renderItemIcon(cub, 64), 64);
    auto &model = itemModels[key];
    model.allocation = {cub.sx, cub.sy, cub.sz};
    worldmap::meshModel({cub.sx, cub.sy, cub.sz, cub.rgb.data()}, 0, cub.sx - 1, 0, cub.sy - 1, true, model.pending);
}

void Hud::uploadItemModels(Renderer &renderer) {
    for (auto &[key, model] : itemModels)
        if (!model.mesh && !model.pending.empty()) {
            model.mesh = renderer.upload(model.pending);
            if (model.mesh)
                model.pending = {};
        }
}

void Hud::itemIcon(const std::string &key, const nativesaves::Item &item, const guiitem::Placement &at, size_t beforeVertex) const {
    const auto found = itemModels.find(key);
    if (found == itemModels.end() || !found->second.mesh)
        return;
    guiitem::Request request;
    request.beforeVertex = beforeVertex;
    request.mesh = found->second.mesh;
    request.allocation = found->second.allocation;
    request.item = item;
    request.at = at;
    if (const auto cube = itemModels.find("cube"); cube != itemModels.end())
        request.cubeMesh = cube->second.mesh;
    itemRequests.push_back(request);
}
