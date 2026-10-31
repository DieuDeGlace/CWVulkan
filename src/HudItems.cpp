// Owner CUB resources and immediate widget requests (steam-gui-item-icons.md).
#include "Hud.h"
#include "ItemIcon.h"
#include "MapLook.h"
#include <algorithm>

void Hud::addItemIcon(const std::string &key, const CubModel &cub, bool retainVolume, steamvoxel::CubEncoding encoding) {
    if (auto found = itemModels.find(key); found != itemModels.end()) {
        if (retainVolume && !found->second.volume)
            found->second.volume = std::make_shared<CubModel>(cub);
        return;
    }
    // Compatibility consumers retain their atlas image until their own caller is converted.
    addIcon(key, renderItemIcon(cub, 64), 64);
    auto &model = itemModels[key];
    if (retainVolume)
        model.volume = std::make_shared<CubModel>(cub);
    model.allocation = {cub.sx, cub.sy, cub.sz};
    worldmap::meshModel({cub.sx, cub.sy, cub.sz, cub.rgb.data()}, 0, cub.sx - 1, 0, cub.sy - 1, encoding == steamvoxel::CubEncoding::Stock,
                        model.pending);
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

void Hud::ringItems(const State::Ring &ring) const {
    if (!ring.shown)
        return;
    std::vector<guiitem::Request> requests;
    for (size_t i = 0; i < ring.items.size(); ++i) {
        const auto &item = ring.items[i];
        const auto model = itemModels.find(item.icon);
        if (!item.original || model == itemModels.end() || !model->second.mesh)
            continue; // missing resources preserve their positions in the original count
        guiitem::Request request;
        request.kind = guiitem::Request::Kind::Ring;
        request.mesh = model->second.mesh;
        request.allocation = model->second.allocation;
        request.item = *item.original;
        request.farPlane = ring.farPlane;
        request.at = guiitem::ringPlacement(i, ring.items.size(), request.allocation, glm::ivec2(screen), ring.radius, ring.angle, guiScale,
                                            ring.worldMilliseconds);
        requests.push_back(request);
    }
    // Opaque resource groups precede Plasma; equal resources retain candidate submission order.
    std::stable_sort(requests.begin(), requests.end(), [](const auto &a, const auto &b) { return a.mesh < b.mesh; });
    itemRequests.insert(itemRequests.end(), requests.begin(), requests.end());
}
