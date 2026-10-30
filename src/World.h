#pragma once
// Streams terrain zones around the player (generated on worker threads),
// caches .cub models and owns every placed model instance.
#include "Renderer.h"
#include "Terrain.h"
#include "VegetationWind.h"
#include <algorithm>
#include <future>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class Assets;
namespace alphalight {
class LightVolume;
}

class World {
  public:
    // With `extracted`, zones come from the game's extracted blocks instead of
    // the generator (zones outside the extracted area stay empty).
    World(const Terrain &terrain, const Assets &assets, Renderer &renderer, int viewZones,
          class ExtractedWorld *extracted = nullptr);
    ~World();
    // Starts and finishes zone jobs around `focus`; drops zones out of range.
    void update(glm::vec3 focus);
    void collect(std::vector<DrawItem> &opaque, std::vector<DrawItem> &water) const;
    using ObjectProvider = std::function<std::optional<objectvisual::State>(int zx, int zy, const objectvisual::Key &)>;
    void setObjectProvider(ObjectProvider provider) { objectProvider = std::move(provider); }
    // Queried each frame: personal lights follow the current character immediately.
    void setUsedCellProvider(steampoints::UsedCellQuery provider) { usedCellProvider = std::move(provider); }
    using EffectLightProvider = std::function<std::vector<steampoints::EffectSource>()>;
    void setEffectLightProvider(EffectLightProvider provider) { effectLightProvider = std::move(provider); }
    using TransientLightProvider = std::function<std::vector<steampoints::TransientSource>()>;
    void setTransientLightProvider(TransientLightProvider provider) { transientLightProvider = std::move(provider); }
    using CreatureLightCollector = std::function<void(const steampoints::Frame &, std::vector<steampoints::Light> &)>;
    void setCreatureLightCollector(CreatureLightCollector collector) { creatureLightCollector = std::move(collector); }
    using ScenePlayerProvider = std::function<std::optional<scenerange::Player>()>;
    void setScenePlayerProvider(ScenePlayerProvider provider) { scenePlayerProvider = std::move(provider); }
    using GroundLightProvider = std::function<steampoints::GroundSources(steampoints::ZoneKey)>;
    void setGroundLightProvider(GroundLightProvider provider) { groundLightProvider = std::move(provider); }
    // How many zones are drawn each way around the focus (the view distance changed).
    void setViewZones(int zones) { viewZones = zones; }
    int viewZoneCount() const { return viewZones; }
    // Zone (zx, zy)'s blocks changed: it is meshed again, the old mesh drawn until then.
    void refreshZone(int zx, int zy);
    // The world's millisecond clock, advanced by each step's milliseconds; it starts at 0 with
    // the world and is not saved. The plants sway by it (vegetation-wind.md §2).
    void advanceClock(double ms) { clockMs += std::max(0.0, ms); }
    double clock() const { return clockMs; }

    // -1 if missing or invalid. `hair`: hide the sprite's marker voxels and
    // multiply its grey voxels by `tint` (the creature's hair colour).
    // Second hair colour (built-in DualHair): blended in from `start` of the way
    // down the hair over `blend` more (0-1).
    struct HairLook {
        bool dual = false;
        glm::u8vec3 second{255};
        float start = .4f, blend = .35f;
    };
    // Colours a hair sprite as the game does (look: DualHair second colour, or null).
    static void tintHair(struct CubModel &cub, glm::u8vec3 tint, const HairLook *look);
    int loadModel(const std::string &name, glm::u8vec3 tint = glm::u8vec3(255), bool hair = false,
                  const HairLook *look = nullptr);
    // Renderer mesh of a loaded model (centred on x/y, base at z = 0) and its voxel size.
    uint32_t modelMesh(int model) const { return model >= 0 && model < (int)models.size() ? models[model].mesh : 0; }
    glm::vec3 modelSize(int model) const { return model >= 0 && model < (int)models.size() ? models[model].size : glm::vec3(0); }
    // Exposed voxel origins for native death bursts, in mesh units (centred on
    // x/y, base at z = 0), with colours as drawn. Read from the .cub on first use.
    struct Voxel {
        glm::vec3 at;
        glm::u8vec3 color;
    };
    const std::vector<Voxel> &voxels(int model) const;
    int spawn(int model, glm::vec3 position, float yaw, float scale, uint32_t flags = 0);
    void move(int instance, glm::vec3 position, float yaw);
    void remove(int instance);

    const Terrain &terrain() const { return land; }
    size_t zoneCount() const { return zones.size(); }
    size_t pendingCount() const { return jobs.size(); }
    size_t modelCount() const { return models.size(); }
    std::vector<std::string> missingModels;

  private:
    struct Model {
        uint32_t mesh = 0;
        float height = 0;
        glm::vec3 size{0}; // .cub dimensions in voxels
        std::string name;  // to read its voxels again (voxels())
        glm::u8vec3 tint{255};
        bool tinted = false;
        mutable std::vector<Voxel> voxels;
        mutable bool voxelsRead = false;
    };
    struct Generated {
        Mesh opaque, water, lava;
        std::optional<scenevisibility::Bounds> nativeBounds;
        std::shared_ptr<const steampoints::ZoneSources> nativeLights;
        std::vector<Scenery> scenery;
    };
    // A zone model whose matrix changes with the clock: it sways (flag 4) or, in the Steam
    // look, swings from its top (flags 0x10, 0x20).
    struct Moving {
        size_t draw = 0; // its index in Zone::scenery
        vegetationwind::Placement placement;
    };
    struct AnimatedObject {
        size_t draw;
        objectvisual::Key key;
        objectvisual::State captured;
        glm::vec3 position, dimensions;
    };
    ObjectProvider objectProvider;
    steampoints::UsedCellQuery usedCellProvider;
    EffectLightProvider effectLightProvider;
    TransientLightProvider transientLightProvider;
    CreatureLightCollector creatureLightCollector;
    GroundLightProvider groundLightProvider;
    ScenePlayerProvider scenePlayerProvider;
    struct Zone {
        uint32_t opaque = 0, water = 0, lava = 0;
        std::shared_ptr<const scenevisibility::Bounds> nativeBounds;
        std::shared_ptr<const steampoints::ZoneSources> nativeLights;
        std::vector<DrawItem> scenery;
        std::vector<Moving> moving;
        std::vector<AnimatedObject> objects;
    };
    struct Instance {
        int model = -1;
        glm::vec3 position{0};
        float yaw = 0, scale = 1;
        uint32_t flags = 0;
        bool live = false;
    };
    using Key = std::pair<int, int>;
    const Terrain &land;
    const Assets &assets;
    Renderer &renderer;
    int viewZones;
    class ExtractedWorld *extracted;
    uint32_t beamMesh = 0;
    std::vector<Model> models;
    std::unordered_map<std::string, int> modelIndex;
    std::map<Key, Zone> zones;
    std::map<Key, std::future<Generated>> jobs;
    std::map<Key, std::shared_ptr<scenerange::CachePublication>> pendingBounds;
    std::set<Key> stale; // zones to mesh again
    std::vector<Instance> instances;
    DrawItem instanceDraw(const Instance &i) const;
    // The Alpha look's questions (Renderer's WorldProbe).
    bool waterAt(glm::vec3 p) const;
    alpha::ZoneFactors zoneFactors(glm::vec3 p) const;
    float blockLight(glm::vec3 p) const;
    // The flood's view of the blocks in [origin, origin + size) (alpha-rendering.md 9.2).
    void fillLight(alphalight::LightVolume &volume) const;
    // The light where each model stands, with one flood over them all (a zone's worth).
    void lightScenery(std::vector<Scenery> &scenery) const;
    double clockMs = 0;
    // The sky's models (docs/specs/sky-and-clouds.md), read from the owner's install for the
    // session's look and handed to the renderer.
    SkyModels skyModels;
    void loadSkyModels();
    mutable alpha::ZoneFactors zoneCache;
    mutable glm::ivec2 zoneCacheCell{0};
    mutable bool zoneCacheValid = false;
};
