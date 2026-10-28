#pragma once
// The HUD's turning gold coin (docs/specs/hud-money.md §2, §3): the 3D coin at the top left,
// beside the money text, drawn with its black shadow in the overlay pass after the world.
//
// The game queues both as overlay models (0xee5a0) and draws them at the end of the frame
// through a 50 degree lens of its own, mirrored in x, under fixed lights. CubeVulkan draws them
// on the renderer's screen-space path (NATIVE_MENU, as the title logo), which takes a whole
// clip transform and colours already final: the lights and the model shader's colour steps
// are therefore worked out here, face by face, and baked into the mesh.
#include "MapLook.h"
#include "Renderer.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <map>
#include <vector>

class Assets;
struct CubModel;

namespace hudcoin {

// The model: the model table's entry 0xae1.
inline constexpr const char *COIN_SPRITE = "gold-coin";

// The overlay pass's lights (§3): front, back, the direction toward the light, ambient.
extern const worldmap::LightRig OVERLAY_LIGHTS;

// One overlay model draw (an entry of 0xee5a0): where its middle lands on the screen (pixels,
// y down), its turns (degrees, about x, then y, then z, in its own frame), its scale, its
// colour and the draw's glow and depth numbers.
struct OverlayDraw {
    glm::vec2 screen{0};
    glm::vec3 angles{0};
    float scale = 0;
    glm::vec4 colour{1};
    float glow = 0;
    float depth = 0;
};

// The coin's turn about its own axis for the world clock (world +0x440, ms): one turn every
// 10 s, in degrees (§2.1).
float turnDegrees(double worldClockMs);
// The two draws, the shadow first, for a window `height` pixels high at GUI scale `guiScale`
// (§2.1, §2.2).
std::vector<OverlayDraw> coinDraws(float guiScale, float height, double worldClockMs);

// The model matrix of an overlay entry (§2.3) for a model of `size` voxels: one unit in front
// of the overlay camera, turned, scaled and centred. Its corner is the model's corner.
glm::mat4 modelMatrix(const OverlayDraw &d, glm::vec3 size);
// The overlay pass's projection (§3), as Direct3D's clip space (y up, depth 0..1).
glm::mat4 overlayProjection(float width, float height);
// The whole clip transform of an entry as the renderer's screen-space path takes it (Vulkan's
// clip space, y down): the entry's shift to its screen point, its depth number and the lens.
glm::mat4 entryClip(const OverlayDraw &d, glm::vec3 size, float width, float height);

// The colour a face of the model shows (the model shader's steps, worldmap-rendering.md §3.2
// and §3.3), for a vertex colour as the shader reads it (0..1), the face's normal in the model's
// own frame and the entry.
glm::vec3 faceColour(glm::vec4 vertex, glm::vec3 modelNormal, const glm::mat4 &model, const OverlayDraw &d);

// The coin as the renderer draws it: loaded on first use, its baked meshes kept while they are
// in use.
class Coin {
  public:
    // Queues the shadow and the coin for this frame. Nothing when the sprite is missing.
    void draw(Renderer &renderer, const Assets &assets, std::vector<DrawItem> &opaque, float width, float height,
              float guiScale, double worldClockMs);

  private:
    bool tried = false, loaded = false;
    Mesh base;               // the coin meshed as every voxel model is, unlit
    glm::vec3 size{0};
    // The meshes baked so far by their colours: the lights depend on the turn only through the
    // faces' directions, so a still or glowing coin keeps reusing the same few.
    std::map<uint64_t, uint32_t> baked;
    std::vector<uint64_t> bakedOrder;
    uint32_t meshFor(Renderer &renderer, const glm::mat4 &model, const OverlayDraw &d);
};

} // namespace hudcoin
