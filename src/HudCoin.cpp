// Owner-backed turning HUD coin; no CPU lighting or per-turn mesh variants.
#include "HudCoin.h"
#include "Assets.h"
#include "Renderer.h"
#include "worldgen/Noise.h"

#include <string>

namespace hudcoin {
float turnDegrees(float worldClockMs) {
    return float(worldgen::trunc32(worldClockMs) % 10000) * .0001f * 360.f;
}

std::vector<guiitem::Request> coinRequests(uint32_t mesh, glm::ivec3 allocation, float farPlane, float guiScale, float height,
                                           float worldClockMs) {
    if (!mesh || height <= 0 || allocation.x <= 0 || allocation.y <= 0 || allocation.z <= 0)
        return {};
    guiitem::Request coin;
    coin.kind = guiitem::Request::Kind::QueuedModel;
    coin.mesh = mesh;
    coin.allocation = allocation;
    coin.farPlane = farPlane;
    coin.at.euler = {-30, turnDegrees(worldClockMs), 0};
    coin.at.size = (guiScale * .004f) * (1024.f / height);
    coin.at.screen = glm::vec2(45, 52) * guiScale;
    coin.state = guiitem::overlayState();
    coin.state->shininess = 1;
    auto shadow = coin;
    shadow.at.screen = glm::vec2(50, 57) * guiScale;
    shadow.material = {0, 0, 0, 1};
    shadow.depthOffset = -.5f;
    return {shadow, coin};
}

void Coin::queue(Renderer &renderer, const Assets &assets, std::vector<guiitem::Request> &out, float height, float guiScale,
                 float worldClockMs) {
    if (!tried) {
        tried = true;
        CubModel cub;
        const auto path = assets.resolve(std::filesystem::path("sprites") / (std::string(COIN_SPRITE) + ".cub"));
        if (path && cub.load(*path)) {
            const auto source = guiitem::voxelMesh(cub);
            if (!source.empty()) {
                mesh = renderer.upload(source);
                allocation = {cub.sx, cub.sy, cub.sz};
            }
        }
    }
    const auto requests = coinRequests(mesh, allocation, float(renderer.settings.nativeZoneRadius) * 64.f, guiScale, height, worldClockMs);
    out.insert(out.end(), requests.begin(), requests.end());
}
} // namespace hudcoin
