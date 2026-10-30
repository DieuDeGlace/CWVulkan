#pragma once
#include "SteamPointLights.h"
// Vulkan 1.3 renderer: sky, voxel scene, water, and two directional shadow
// cascades (MoonandShadow's near/far maps) drawn with dynamic rendering. It draws the world
// in one of two looks ([Render] Style): Cube World Alpha's (the default) or the Steam
// release's.
#include "AlphaLook.h"
#include "AlphaMoon.h"
#include "Clouds.h"
#include "DayNight.h"
#include "SteamSky.h"
#include "Mesher.h"
#include "LightBeam.h"
#include "SteamScenery.h"
#include "SceneVisibility.h"
#include "SteamLighting.h"
#include <memory>
#include <volk.h>
#include <glm/glm.hpp>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct GLFWwindow;
class Config;

// Depth format of the scene and shadow maps.
inline constexpr VkFormat DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;

struct RenderSettings {
    // [Render] Style=alpha|steam: Cube World Alpha's lighting, sky and fog, or the Steam
    // release's. Fixed for the session (meshes bake their light for it).
    RenderStyle style = RenderStyle::Alpha;
    // [Render] Moon, MoonBrightness, MoonShadowStrength, NightAmbient: CubeVulkan's moon in
    // the Alpha look (src/AlphaMoon.h). The Steam look keeps its own MoonLight moon ([Moon]).
    alpha::MoonSettings moon;
    bool vsync = true;
    // [Shadows]: same keys as the installed Mods/SteamShadows.ini. CubeVulkan's own: neither
    // game has shadow maps (docs/specs/shadows.md §1), but they are on by default at the
    // user's request; `[Shadows] Enabled=0` turns them off for the vanilla look. The near map
    // covers 25 blocks, twice as fine as the far one (shadows.md §4, cascades).
    bool shadows = true, cascades = true;
    float sunShadowStrength = .9f; // [Shadows] StrengthPercent, as the Alpha look's sun uses it
    uint32_t resolution = 4096, nearResolution = 2048;
    float radius = 120, nearRadius = 25;
    float sunWarmth = .03f, fadeStart = .2f;
    bool cameraFade = true;
    // [Motion] LeafSway: the WorldMotion mod's sway of leaf blocks, a CubeVulkan option off by
    // default (vanilla never moves a block, vegetation-wind.md §1). The game's own plant sway
    // is World's (VegetationWind.h) and always on.
    bool wind = false;
    float leafAmplitude = .16f, windSpeed = 1, leafRange = 96;
    // [Graphics]
    float fov = 60, fogDistance = 400, sceneryDistance = 110;
    // [AntiAliasing]: TAA as the user's Steam TemporalAA mod (its Mods/TemporalAA.ini is
    // imported), then SMAA Ultra as CWSMTAA. Defaults give the softer Omega-style edges. Both
    // are CubeVulkan's (the games only multisample, docs/specs/shadows.md §3); edges keep 80 %
    // history rather than the mod's 90 %, so moving creatures and swaying plants trail less.
    bool taa = true, smaa = true;
    float historyWeight = .65f, edgeHistoryWeight = .80f, resolveBlur = .12f, jitterScale = .75f;
    void read(const Config &config);
};

struct Camera {
    glm::vec3 eye{0}, target{0, 1, 0};
    float aspect = 16.f / 9;
    std::optional<float> displayedYawDegrees; // Steam beam orientation, before eye collision adjustments
};

// One corner of an interface quad, in screen pixels (origin top left).
struct OverlayVertex {
    float x, y, u, v;
    uint8_t r, g, b, a;
};

// What a draw is, for the Alpha look: terrain carries its light baked into its vertices,
// scenery is lit as it stands, and other models (creatures, items) take the light of the block
// they are in.
enum class DrawKind : uint8_t { Model, Terrain, Scenery, LightBeam, LavaTerrain };
constexpr uint32_t LAVA_LIGHTING = 1u << 8; // draw-only flag, not a vertex byte or Glow
constexpr uint32_t STEAM_WORLD = 1u << 9; // ordinary world/water pass; excludes sky and menu art

struct DrawItem {
    uint32_t mesh = 0;
    glm::mat4 model{1};
    uint32_t flags = 0;  // Draw flags combined with each vertex's flags.
    float height = 0;    // Model height in local units (plants bend from the base).
    bool castShadow = true;
    DrawKind kind = DrawKind::Model;
    lightbeam::Placement beam; // only DrawKind::LightBeam consumes this
    std::optional<steamscenery::Bounds> nativeScenery;
    float opacity = 1; // independent from the baked sunlight/material factor
    // A point light the object gives off in the Alpha look (lamps, torches, fires): its colour
    // at night, its reach in blocks (0 = none) and where it is.
    glm::vec3 lightColour{0};
    float lightRadius = 0;
    glm::vec3 lightPosition{0};
    // Alpha look: the hit flash (0..1, towards white) and the shimmer of shiny items.
    float flash = 0;
    bool shimmer = false;
    // Alpha look: the light where it stands (0..1, alpha-rendering.md 11.2), when the world
    // already knows it (zone models); below 0, a model (creature, item) reads it here.
    float standingLight = -1;
    // The world map's draws (flags WORLD_MAP; src/WorldMap.cpp, shaders/map.*): `model` takes
    // the model to the map camera's space, and these complete it. They are drawn after the
    // world, over a cleared depth, with the map's own light whatever the world's look.
    struct MapDraw {
        glm::vec4 lens{0};          // -f / aspect, f, the screen shift x, y (MapLook.h)
        float yaw = 0, pitch = 0;   // the map view's turn, radians
        float opacity = 1;          // below 1: blended (a region fading in)
        float depthOffset = 0;      // taken off the clip depth: larger draws over
        glm::vec3 colour{1};        // the instance's colour
        float white = 0;            // how far toward white (an outline's pulse)
    } map;
    std::shared_ptr<const scenevisibility::Bounds> nativeZone; // containing zone for native model/beam gates
    std::shared_ptr<const steampoints::ZoneSources> nativeLights;
    // Reflections reverse triangle winding (the native right foot uses one).
    bool mirrored() const {
        return glm::dot(glm::cross(glm::vec3(model[0]), glm::vec3(model[1])), glm::vec3(model[2])) < 0;
    }
};

// What the sky asks about a column of the world (docs/specs/sky-and-clouds.md): the Alpha
// clouds' moisture, mountains and ground, and the Steam palette's and clouds' region values.
struct SkyColumn {
    alpha::ZoneFactors zone;  // the Alpha look's zone factors (the clouds read the moisture)
    float mountain = 0;       // Alpha clouds: 0..1, rising in mountains
    steamsky::Region region;  // the Steam look's region values (ground height in mode 0)
    float cloudGround = 0;    // Steam clouds: the ground height in mode 1
};

// World supplies terrain and live scene data; every callback is optional.
struct WorldProbe {
    std::function<bool(glm::vec3)> water;               // the block there is water
    std::function<alpha::ZoneFactors(glm::vec3)> zone;  // the zone factors of that column
    std::function<float(glm::vec3)> light;              // the block's light, 0..1
    // The sky's questions, by the game's block column; `origin` is where the renderer's origin
    // is in the game's blocks (game = renderer position + origin).
    std::function<SkyColumn(int32_t x, int32_t y)> column;
    glm::dvec3 origin{0};
    std::function<steamlighting::Environment(glm::vec3)> sceneLighting;
    std::function<std::vector<steampoints::ZoneKey>()> loadedLightZones;
    steampoints::UsedCellQuery usedCell; // local player's personal discovery state
    std::function<std::vector<steampoints::EffectSource>()> effectLights;
    std::function<std::vector<steampoints::TransientSource>()> transientLights;
    std::function<steampoints::GroundSources(steampoints::ZoneKey)> groundLights;
};

// The models the sky draws, as uploaded meshes (World makes them from the owner's install).
struct SkyModels {
    uint32_t cloud = 0;     // cloud02.cub: Alpha's shading, or Steam's recoloured cloud
    uint32_t darkCloud = 0; // cloud-dark.cub (Steam look)
    uint32_t starCube = 0;  // cube.cub, the Steam look's stars
};

class Renderer {
  public:
    RenderSettings settings;
    // The Alpha look's questions about the world (see WorldProbe).
    void setWorldProbe(WorldProbe probe) {
        worldProbe = std::move(probe);
        blockLight.clear();
        cloudCells.clear();
        skyColumnValid = false;
    }
    void setSkyModels(SkyModels models) { skyModels = models; }
    // The view cut to somewhere else (the title to the game): the TAA history is dropped.
    void cutHistory() { historyValid = false; }
    // Horizontal distance from the player to the nearest ground not loaded yet (blocks); the
    // Alpha look's fog closes in on it.
    void setUnloadedDistance(float blocks) { unloadedDistance = blocks; }
    // The world's day, for the moon's phase in the Alpha look.
    void setWorldDay(int day) { worldDay = day; }
    bool init(GLFWwindow *window, const RenderSettings &settings, std::string &error);
    // A dedicated server draws nothing: no Vulkan device at all, so it runs on a
    // machine without a graphics card. Uploads return ids that draw nothing.
    void initHeadless(const RenderSettings &settings);
    bool headless() const { return noGpu; }
    void shutdown();
    // Uploads happen at the start of the next frame; the id is usable at once.
    uint32_t upload(const Mesh &mesh);
    // Destroyed once the GPU no longer uses it.
    void release(uint32_t id);
    void frame(const Camera &camera, const Lighting &lighting, glm::vec3 focus, double time,
               const std::vector<DrawItem> &opaque, const std::vector<DrawItem> &water);
    const std::string &deviceName() const { return gpuName; }
    unsigned drawsLastFrame() const { return lastDraws; }
    // Saves the next presented frame as a PNG (for screenshots and tests).
    void capture(const std::string &path) { capturePath = path; }
    // GPU time per frame (milliseconds, averaged since start): the whole frame and
    // the anti-aliasing passes (TAA, SMAA, the interface draw).
    double averageGpuMs() const { return timingSamples ? gpuMsSum / timingSamples : 0; }
    double averagePostMs() const { return timingSamples ? postMsSum / timingSamples : 0; }
    // Interface: an RGBA atlas (uploaded once, replaced on a later call) and the
    // triangles to draw over the next frame (3 vertices each, atlas texels as uv).
    bool setOverlayAtlas(uint32_t width, uint32_t height, const uint8_t *rgba);
    void overlay(std::vector<OverlayVertex> triangles) { overlayVertices = std::move(triangles); }
    VkExtent2D screenSize() const { return extent; }

  private:
    struct Buffer {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        void *mapped = nullptr;
    };
    struct GpuMesh {
        Buffer buffer;
        VkDeviceSize indexOffset = 0;
        uint32_t indexCount = 0;
        glm::vec3 min{0}, max{0};
        std::optional<glm::vec3> nativeCentre;
        bool live = false;
    };
    struct Image {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
    };
    struct PendingUpload {
        Buffer staging;
        uint32_t mesh;
        VkDeviceSize size;
    };
    static constexpr int FRAMES = 2;
    struct Frame {
        VkCommandBuffer cmd = VK_NULL_HANDLE;
        VkFence fence = VK_NULL_HANDLE;
        VkSemaphore acquired = VK_NULL_HANDLE;
        Buffer uniforms, nativePoints;
        VkDeviceSize nativePointCapacity = 0;
        VkDescriptorSet set = VK_NULL_HANDLE;
        std::vector<Buffer> staging;      // Freed when this frame's fence signals.
        std::vector<uint32_t> releases;   // Meshes released while this frame was in flight.
    };

    GLFWwindow *window = nullptr;
    std::string gpuName;
    bool noGpu = false;
    uint32_t headlessIds = 0;
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice gpu = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    uint32_t queueFamily = 0;
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    bool depthClamp = false;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat swapFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkExtent2D extent{};
    std::vector<VkImage> swapImages;
    std::vector<VkImageView> swapViews;
    std::vector<VkSemaphore> renderDone; // One per swapchain image.
    // Anti-aliasing (RendererPost.cpp): the scene renders off-screen with jitter, TAA
    // resolves it into alternating history images (depth alternates too, giving last
    // frame's depth for reprojection), SMAA writes the swapchain, then the interface.
    Image sceneColor, depthImages[2], history[2], smaaEdges, smaaWeights, areaTex, searchTex;
    VkSampler linearSampler = VK_NULL_HANDLE, pointSampler = VK_NULL_HANDLE;
    VkDescriptorSetLayout postSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout postLayout = VK_NULL_HANDLE;
    VkPipeline taaPipeline = VK_NULL_HANDLE, copyPipeline = VK_NULL_HANDLE, smaaEdgePipeline = VK_NULL_HANDLE,
               smaaWeightPipeline = VK_NULL_HANDLE, smaaBlendPipeline = VK_NULL_HANDLE;
    VkDescriptorPool postPool = VK_NULL_HANDLE;
    VkDescriptorSet taaSets[2]{}, copySets[2]{}, edgeSets[2]{}, weightSet = VK_NULL_HANDLE, blendSets[2]{};
    int parity = 0;             // which depth/history image is this frame's
    bool postFresh = true;      // targets just created: last frame's images hold nothing
    bool historyValid = false;
    uint32_t aaFrame = 0;
    glm::mat4 previousClip{1};  // last frame's jittered view-projection
    glm::vec2 previousJitter{0};
    glm::vec3 previousEye{0};
    bool previousTaa = true;
    VkQueryPool timing = VK_NULL_HANDLE;
    float timestampPeriod = 1; // nanoseconds per tick
    bool timingWritten[FRAMES] = {};
    double gpuMsSum = 0, postMsSum = 0;
    uint32_t timingSamples = 0;
    bool createPostResources(std::string &error);
    bool createPostTargets();
    void destroyPostTargets();
    void destroyPostResources();
    Image uploadImage(uint32_t w, uint32_t h, VkFormat format, const void *data, size_t bytes);
    void recordPost(VkCommandBuffer cmd, uint32_t imageIndex, glm::vec2 jitter, const glm::mat4 &clipToPrevious,
                    bool reproject);
    void drawOverlay(VkCommandBuffer cmd);
    Image shadowFar, shadowNear;
    VkSampler shadowSampler = VK_NULL_HANDLE;

    VkCommandPool pool = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline scenePipeline = VK_NULL_HANDLE, mirroredScenePipeline = VK_NULL_HANDLE, menuPipeline = VK_NULL_HANDLE,
               menuBackgroundPipeline = VK_NULL_HANDLE,
               waterPipeline = VK_NULL_HANDLE, waterDepthPipeline = VK_NULL_HANDLE, shadowPipeline = VK_NULL_HANDLE, skyPipeline = VK_NULL_HANDLE;
    // The Alpha look's pipelines: world, world with reversed winding, water, sky, and the
    // stars and sun sprite.
    VkPipeline beamPipeline = VK_NULL_HANDLE;
    VkPipeline sceneryDepthPipeline = VK_NULL_HANDLE, sceneryMirroredDepthPipeline = VK_NULL_HANDLE,
               sceneryFadePipeline = VK_NULL_HANDLE, sceneryMirroredFadePipeline = VK_NULL_HANDLE;
    VkPipeline alphaScenePipeline = VK_NULL_HANDLE, alphaMirroredPipeline = VK_NULL_HANDLE,
               alphaWaterPipeline = VK_NULL_HANDLE, alphaSkyPipeline = VK_NULL_HANDLE,
               alphaSpritePipeline = VK_NULL_HANDLE, alphaCloudPipeline = VK_NULL_HANDLE;
    // The Steam look's clouds (blended, they fade near the camera) and star cubes.
    VkPipeline steamCloudPipeline = VK_NULL_HANDLE, steamStarPipeline = VK_NULL_HANDLE;
    Buffer starBuffer; // the 3000 stars (position on the sky sphere, size)
    uint32_t starCount = 0;
    WorldProbe worldProbe;
    float unloadedDistance = -1;   // -1: not told; the fog then opens to the render radius
    int worldDay = 4;              // the moon is full until told otherwise
    alpha::FogDistance fogDistance; // the Alpha look's smoothed fog distance F
    double lastFrameTime = -1;
    // The sky (docs/specs/sky-and-clouds.md): its models, the clouds of the cells already
    // worked out (they never change), the Steam palette as eased frame to frame, the column
    // the camera was last in, and the Steam star dome.
    SkyModels skyModels;
    std::map<std::pair<int32_t, int32_t>, std::optional<clouds::Cloud>> cloudCells;
    steamsky::EasedPalette steamPalette;
    steamlighting::EasedColours steamLights;
    SkyColumn skyColumn;
    glm::ivec2 skyColumnAt{0};
    bool skyColumnValid = false;
    std::vector<steamsky::Star> steamStars;
    const SkyColumn &cameraColumn(glm::dvec3 game);
    const std::optional<clouds::Cloud> &cloudOfCell(int32_t cx, int32_t cy, bool alphaLook);
    // Light of the blocks models were drawn in (a block key -> 0..1), asked of the world once.
    std::unordered_map<uint64_t, float> blockLight;
    int blockLightBudget = 0;  // new blocks still to work out this frame
    float lastBlockLight = 1;
    float lightOfBlock(glm::vec3 p);
    Frame frames[FRAMES];
    uint32_t frameIndex = 0;
    bool shadowsInitialized = false;
    // Interface overlay.
    Image overlayImage;
    uint32_t overlayWidth = 1, overlayHeight = 1;
    VkSampler overlaySampler = VK_NULL_HANDLE;
    VkDescriptorSetLayout overlaySetLayout = VK_NULL_HANDLE;
    VkDescriptorPool overlayPool = VK_NULL_HANDLE;
    VkDescriptorSet overlaySet = VK_NULL_HANDLE;
    VkPipelineLayout overlayLayout = VK_NULL_HANDLE;
    VkPipeline overlayPipeline = VK_NULL_HANDLE;
    Buffer overlayBuffers[FRAMES];
    VkDeviceSize overlayCapacity[FRAMES] = {};
    std::vector<OverlayVertex> overlayVertices;
    bool createOverlayPipeline(std::string &error);
    // The world map's pass (RendererMap.cpp): its own shaders, blending for the fade-in, and
    // push constants that carry the map's camera and the instance.
    VkPipelineLayout mapLayout = VK_NULL_HANDLE;
    VkPipeline mapPipeline = VK_NULL_HANDLE;
    bool createMapPipeline(std::string &error);
    void destroyMapPipeline();
    // `jitter`: the TAA jitter as a clip-space shift per unit of w. Returns the draws made.
    unsigned drawMap(VkCommandBuffer cmd, const std::vector<DrawItem> &items, glm::vec3 fogColour, glm::vec2 jitter);

    std::vector<GpuMesh> meshes{1}; // Index 0 is "no mesh".
    std::vector<uint32_t> freeIds;
    std::vector<PendingUpload> pending;
    unsigned lastDraws = 0;
    std::string capturePath;
    bool canCapture = false;

    bool createDevice(std::string &error);
    bool createSwapchain();
    void destroySwapchain();
    bool createPipelines(std::string &error);
    Buffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags);
    void destroyBuffer(Buffer &b);
    Image createImage(uint32_t w, uint32_t h, VkFormat format, VkImageUsageFlags usage,
                      VkImageAspectFlags aspect);
    void destroyImage(Image &i);
    uint32_t memoryType(uint32_t bits, VkMemoryPropertyFlags flags) const;
    void destroyMesh(uint32_t id);
};
