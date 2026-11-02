#include "HudCompass.h"
#include "Assets.h"
#include "Renderer.h"
#include <bit>

namespace hudcompass {
std::optional<guiitem::Request> request(uint32_t mesh, glm::ivec3 allocation, glm::ivec2 viewport, float farPlane, float guiScale,
                                        float displayedYawDegrees) {
    if (!mesh || viewport.x <= 0 || viewport.y <= 0 || allocation.x <= 0 || allocation.y <= 0 || allocation.z <= 0)
        return std::nullopt;
    guiitem::Request draw;
    draw.kind = guiitem::Request::Kind::QueuedModel;
    draw.mesh = mesh;
    draw.allocation = allocation;
    draw.farPlane = farPlane;
    draw.at.screen = {float(viewport.x) - guiScale * 60.f, guiScale * 20.f};
    const float heading = std::bit_cast<float>(std::bit_cast<uint32_t>(displayedYawDegrees) ^ 0x80000000u);
    draw.at.euler = {-120, 0, heading}; // native float negation; no heading wrap or eye reconstruction
    draw.at.size = (guiScale * .003f) * (1024.f / float(viewport.y));
    draw.state = guiitem::overlayState();
    draw.state->glow = 1;
    return draw;
}

void Compass::queue(Renderer &renderer, const Assets &assets, std::vector<guiitem::Request> &out, float guiScale,
                    float displayedYawDegrees) {
    if (!tried) {
        tried = true;
        CubModel cub;
        if (const auto path = assets.resolve("sprites/compass2.cub"); path && cub.load(*path)) {
            const auto source = guiitem::voxelMesh(cub);
            if (!source.empty()) {
                mesh = renderer.upload(source);
                allocation = {cub.sx, cub.sy, cub.sz};
            }
        }
    }
    const auto viewport = renderer.screenSize();
    if (const auto draw = request(mesh, allocation, {int(viewport.width), int(viewport.height)},
                                  float(renderer.settings.nativeZoneRadius) * 64.f, guiScale, displayedYawDegrees))
        out.push_back(*draw);
}
} // namespace hudcompass
