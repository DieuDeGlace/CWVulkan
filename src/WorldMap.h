#pragma once
// The game's world map (class WorldMap): the world around the map's centre as small voxel
// models, one per map region (16 x 16 map units of 4 * 2^level blocks each, with a 3-unit
// border; worldgen::mapRegion, docs/specs/worldmap-model.md), drawn as the game draws them
// (docs/specs/worldmap-rendering.md, cited as §n): meshed with soft corner occlusion, lit by
// the map's own rig in shaders/map.*, through a 45 degree camera 300 units back that tilts
// and turns with the world camera. The minimap in the top right and the open map in the
// middle are the same draw; only its screen centre and scale move between them.
#include "CubModel.h"
#include "HairStyle.h"
#include "MapLook.h"
#include <optional>
#include "Mesher.h"
#include "Renderer.h"
#include "worldgen/MapModel.h"

#include <glm/glm.hpp>

#include <array>
#include <condition_variable>
#include <deque>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

namespace worldmap {

constexpr int LEVELS = 9;      // the zoom levels
constexpr int MAX_ZOOM = 8;    // Nine native layers; the wheel independently clamps to6.
constexpr int REGION = 16;     // map units a region
constexpr int BORDER = 3;      // the border built around a region
constexpr int RADIUS = 8;      // regions each way of the centre (§4.6)

// Blocks per map unit at a level.
int scaleOf(int level);

// One region, built and meshed (on a worker thread): its mesh in map units relative to the
// model's corner, its lowest voxel's height (map units), which of its own columns' voxels are
// solid (REGION x REGION x sizeZ, x fastest, for the cursor's ray) and its markers.
struct Built {
    int x = 0, y = 0, level = 0, baseHeight = 0;
    Mesh mesh;
    std::vector<uint8_t> solid;
    int sizeZ = 0;
    std::vector<worldgen::MapRegion::Marker> markers;
    worldgen::MapBorders borders;
};
Built build(int rx, int ry, int level);

class Map {
  public:
    ~Map();
    // Asks for the regions missing around `center` (game blocks) at the level, the nearest
    // first, and uploads what the workers finished. Regions not drawn for a while are dropped.
    void update(Renderer &renderer, glm::dvec3 center, int level);

    // One frame's view of the map.
    struct View {
        glm::dvec3 center{0};     // game blocks
        int level = 0;
        int radius = RADIUS;      // regions drawn each way (half while Control is held, §4.6)
        float pitch = 0, yaw = 0; // the world camera's, as CubeVulkan keeps them (radians)
        glm::vec2 cursor{0.5f};   // the screen centre (fractions of the window, from the bottom left)
        float viewScale = 1;      // options mapScale x the eased map scale (§4.1)
        glm::vec2 screen{1};      // window pixels
        glm::dvec3 player{0};     // where the player is (game blocks), the flight route's start
        // The flight route: from the player to `routeTarget` (blocks), on the ground at
        // `routeTargetZ`. (The spec leaves it out; this is CubeVulkan's own dotted line.)
        bool route = false;
        glm::ivec2 routeTarget{0};
        int routeTargetZ = 0;
        // The creatures and the tombstone shown (map-markers.md §2.2, §2.3): their models (an
        // index from Map::looks), where they stand (game blocks), their turn (degrees) and
        // whether their outline pulses (the tombstone and anyone but a player).
        struct Mark {
            int looks = -1;
            glm::dvec3 at{0};
            float turnDegrees = 0;
            bool pulses = false;
        };
        std::vector<Mark> marks;
        double worldClockMs = 0;  // the pulse's clock
        float elapsedWorldMs = 0; // Native World+0x440, distinct from GUI/calendar clocks.
        std::optional<worldgen::Int2> hoveredLand, forcedLand;
    };
    // The map's draw items for this frame (flag WORLD_MAP): whole models first, then the
    // regions still fading in.
    std::optional<worldgen::Int2> collect(const View &view, std::vector<DrawItem> &out);
    // The route's dot (cube.cub).
    void setCube(Renderer &renderer, const CubModel &cube);
    // A marker's models, for View::Mark: a creature's head (or chest) and its hair in the
    // hair's colour, or the tombstone alone.
    int looks(Renderer &renderer, const CubModel &first, const CubModel *hairModel, glm::u8vec3 hairColor,
              const hair::Style *style = nullptr);
    // The region markers around the view's centre on the screen (map-markers.md §0.6, §0.7):
    // where (window pixels from the top left) and how deep, in the regions' order.
    struct Projected {
        worldgen::MapRegion::Marker marker;
        glm::vec2 at{0};
        float depth = 0;
    };
    std::vector<Projected> markers(const View &view) const;
    // A point (game blocks) on the screen through the map's camera (map-markers.md §0.7), and
    // how deep; false when it is not shown.
    bool project(const View &view, glm::dvec3 at, glm::vec2 &screen, float *depth = nullptr) const;
    // The map's cursor (§6 step 2): the pointer (window pixels from the top left) cast into
    // the regions' models, at most 5000 map units; where it meets one, in game blocks. Where
    // it meets several, the last region in the drawing order gives it. False when none.
    bool pick(const View &view, glm::vec2 pointer, glm::dvec3 &at) const;
    void clear(Renderer &renderer);

  private:
    struct Region {
        uint32_t mesh = 0;
        int baseHeight = 0, sizeZ = 0;
        bool empty = true;
        std::vector<uint8_t> solid;
        std::vector<worldgen::MapRegion::Marker> markers;
        worldgen::MapBorders borders;
        double builtMs = 0;      // when it arrived, for the fade-in
        uint64_t drawnFrame = 0; // the map frame it was last drawn in, for the sweep
    };
    using Key = std::array<int, 3>; // rx, ry, level
    std::map<Key, Region> regions;
    std::map<Key, bool> asked; // built or being built
    uint64_t frame = 0, frameAtSweep = 0;
    double lastSweepMs = 0;

    // A model as the map draws it, with its size in voxels and its colour.
    struct Sprite {
        uint32_t mesh = 0;
        glm::vec3 size{0};
        glm::vec3 colour{1};
    };
    std::vector<std::vector<Sprite>> markLooks;
    Sprite cube;
    static Sprite spriteOf(Renderer &renderer, const CubModel &model, glm::vec3 colour);

    // The camera of a view, and where a region's model sits in map units.
    static MapCamera cameraOf(const View &view);
    static glm::dvec3 regionOffset(const View &view, int rx, int ry, int baseHeight);
    // The regions in the drawing order (§4.3): rx outer, ry inner, around the centre.
    template <class Visit> void forEachRegion(const View &view, Visit &&visit) const;
    void drawMarks(const View &view, const MapCamera &camera, std::vector<DrawItem> &out) const;
    void drawBorders(const View &view, const MapCamera &camera, worldgen::Int2 selected, std::vector<DrawItem> &out) const;
    void drawRoute(const View &view, const MapCamera &camera, std::vector<DrawItem> &out) const;
    void sweep(Renderer &renderer, double nowMs);

    // The workers.
    std::mutex lock;
    std::condition_variable wake;
    std::deque<Key> queue;
    std::vector<Built> done;
    std::vector<std::thread> workers;
    bool stopping = false;
    void work();
};

} // namespace worldmap
