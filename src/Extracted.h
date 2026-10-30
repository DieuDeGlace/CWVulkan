#pragma once
// Zones extracted from the user's own Steam Cube World by tools/ZoneExtractor:
// the game's real generated blocks (exact colours and types, trees and
// buildings included). CubeVulkan renders these instead of its generator.
//
// Coordinates are re-centred on the extraction's home zone so floats stay
// precise (the game's block coordinates are in the hundreds of millions).
// Game z = 0 is sea level; CubeVulkan adds Z_OFFSET to keep z positive.
#include "NativeSaves.h"
#include "worldgen/Generator.h"
#include "Mesher.h"
#include "Terrain.h" // Biome, for skies and music
#include "SoundRules.h"
#include "SceneCapture.h"
#include "SceneVisibility.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

class ExtractedWorld {
  public:
    ~ExtractedWorld();
    static constexpr int Z_OFFSET = 128;
    static constexpr int SEA = Z_OFFSET + 1; // Water fills game z <= 0.

    // Opens <dir> (a seed_<n> folder). Returns false when it holds no zones.
    bool open(const std::filesystem::path &dir, std::string &error);
    // Instead of files, makes every zone with CubeVulkan's reimplementation of the game's
    // generator (worldgen/Generator.h), centred on game block (homeX, homeY): blocks, objects and
    // models. The models and objects are named from the game's sprite tables in
    // `spriteDir` (Extract, where sprite-groups.txt is), when the owner extracted them.
    void openGenerated(int homeX, int homeY, const std::filesystem::path &spriteDir = {});
    bool generated() const { return generatedMode; }
    // Newest seed folder under <game>/Extract, or empty.
    static std::filesystem::path find(const std::filesystem::path &gameDir);
    // Strict exporter filename and structural validation, shared with DataCheck.
    static bool zoneCoordinates(const std::filesystem::path &file, int &x, int &y);
    static bool validateZoneFile(const std::filesystem::path &file, std::string &error);

    int seed() const { return seedValue; }
    size_t zoneCount() const { return zones.size(); }
    // Home position (local blocks) where the extraction started.
    glm::vec3 home() const { return homeLocal; }
    // Game position in dots (65536 per block) to local blocks.
    glm::vec3 toLocal(const int64_t *dots) const {
        return glm::vec3(float(double(dots[0] - (int64_t)originX * 64 * 65536) / 65536),
                         float(double(dots[1] - (int64_t)originY * 64 * 65536) / 65536),
                         float(double(dots[2]) / 65536 + Z_OFFSET));
    }
    // The local origin in game dots (what toLocal subtracts), and the home zone (absolute).
    int64_t originDotsX() const { return (int64_t)originX * 64 * 65536; }
    int64_t originDotsY() const { return (int64_t)originY * 64 * 65536; }
    int originZoneX() const { return originX; }
    int originZoneY() const { return originY; }
    const std::filesystem::path &directory() const { return folder; }
    // Optional coherent native mesh-cache evidence from newer owner captures.
    std::optional<scenecapture::MeshCache> meshCapture(int zx, int zy) const;
    // Zone coordinates are local (home zone = 0, 0). Also the zones the generator's hooks
    // make anew (load).
    bool has(int zx, int zy) const {
        return generatedMode || zones.count({zx + originX, zy + originY}) > 0 || worldgen::hooksChange(zx + originX, zy + originY);
    }
    // Opaque and water meshes in local world coordinates (thread-safe).
    bool mesh(int zx, int zy, Mesh &opaque, Mesh &water, std::optional<scenevisibility::Bounds> *nativeBounds = nullptr);
    // Independent mesh-prepass bounds in absolute native fixed coordinates.
    // Missing neighbor data yields no assertion of native bounds.
    std::optional<scenevisibility::Bounds> meshBounds(int zx, int zy);
    // Surface height (top of the highest solid block, or the water) at a local column.
    bool surface(double x, double y, float &height);
    // True if the block containing local point (x, y, z) is solid. Unloaded
    // or unextracted columns count as empty.
    bool solidAt(double x, double y, double z);
    // True if the block containing local point (x, y, z) is water (a water block, or air
    // not marked dry at or below the sea). Unloaded columns count as dry.
    bool waterAt(double x, double y, double z);
    // True if nothing above the block containing (x, y, z) in its column stops light (the
    // Alpha look's sky light; lava stops it too).
    bool openToSky(double x, double y, double z);
    // The Alpha look's view of one column: what blocks [z0, z0 + count) (local z) are to the
    // light, written to `out`; returns false if something above them stops sky light.
    bool lightColumn(double x, double y, int z0, int count, uint8_t *out);
    // The same for every column of the box [x0, x0 + width) x [y0, y0 + height) at once, each
    // zone read once: column (x, y)'s blocks go to out[((y - y0) * width + (x - x0)) * count ..],
    // and sky[(y - y0) * width + (x - x0)] says whether sky light comes down it.
    void lightColumns(int x0, int y0, int width, int height, int z0, int count, uint8_t *out, uint8_t *sky);
    // Walkable floor under a point: the top of the highest solid block at or
    // below `fromZ` with `headroom` blocks of open space above it (so a roof
    // overhead is not mistaken for the floor). Below sea level the water
    // surface is used. False if the column is not extracted.
    bool floorAt(double x, double y, double fromZ, int headroom, float &height);
    // Region type at a local column (CubeVulkan's Biome order).
    int biomeAt(double x, double y);
    // The game's own region type there (cube::Zone::RegionType), and the game's
    // 1024-block region cell (16 zones) that holds it. -1 where nothing was extracted.
    int regionTypeAt(double x, double y, int &cellX, int &cellY);
    // The zone's placed sprites (grass, flowers, lanterns, signs...) and
    // objects (doors, windows, furniture, statues), from its .rec file and the
    // game's own sprite tables (Extract/sprite-groups.txt).
    std::vector<Scenery> props(int zx, int zy);
    // Bake native model light from current neighboring terrain, when all nine zones exist.
    void lightModels(int zx, int zy, std::vector<Scenery> &scenery);
    // The block the game's lookup (0xe9a0, 0xeb90) gives at absolute game block (x, y, z):
    // r, g, b, -, type, flags. Only zones already in memory are read; elsewhere the game's
    // default for a missing zone (grey stone, type 1) and `loaded` false.
    using GameBlock = std::array<uint8_t, 6>;
    GameBlock gameBlock(int64_t x, int64_t y, int z, bool *loaded = nullptr);
    // The zones within `radius` of absolute zone (ax, ay) are kept in memory, as the game
    // keeps the zones around its player: those missing are read (or made) on a thread of
    // their own, and none of them is let go to make room for others (the creatures there
    // walk and see by their blocks). Called every frame.
    void keepAround(int ax, int ay, int radius);
    // The same for several centres, on behalf of `owner`: each owner (a world, the view) keeps its
    // own centres, which replace its previous ones, and a zone stays while any owner keeps it.
    void keepAround(const void *owner, const std::vector<std::pair<int, int>> &centres, int radius);
    // The block changes of absolute zone (ax, ay) (zone +0xd8, native-saves.md §5.3): each
    // change's "now" block is put in, in order, whenever the zone's blocks are in memory or
    // read again. Replaces the zone's list.
    void setBlockChanges(int ax, int ay, const std::vector<nativesaves::BlockChange> &changes);
    // Undoes them (0x2da090): each change's "was" block goes back, last first, and the zone's list
    // is emptied.
    void undoBlockChanges(int ax, int ay);
    // The absolute zones whose blocks changed since the last call (their meshes are stale).
    std::vector<std::pair<int, int>> takeChangedZones();
    // A loaded generated zone's static objects (zone+0x60) at absolute zone (ax, ay); false
    // when the zone is not in memory.
    bool gameObjects(int ax, int ay, std::vector<worldgen::ZoneObject> &out);
    // The zone's ambient sound emitters (zone +0xa8, sound.md §4) at absolute zone (ax, ay),
    // from its .rec file; empty when the extraction did not record that list.
    std::vector<soundrules::Emitter> soundEmitters(int ax, int ay) const;
    // Inspector/importer access using native zone coordinates, without a float round trip.
    std::vector<Scenery> propsAbsolute(int ax, int ay);
    size_t propNames() const { return environment.size() + buildingParts.size(); }

  private:
    struct Block {
        uint8_t r, g, b, field3, type, breakable;
    };
    struct Column {
        int baseZ = 0;
        std::vector<Block> blocks;
    };
    // One stored column as the light sees it (lightColumn).
    static bool columnLight(const Column &c, int z0, int count, uint8_t *out);
    struct Zone {
        int region = 0;
        Column columns[4096]; // index = localX * 64 + localY (the game's order)
        std::shared_ptr<const worldgen::ZoneContents> things; // generated zones only: models, items, spawn points
        std::vector<worldgen::ZoneObject> objects;           // the generator's static objects (zone+0x60)
        bool generated = false;     // made by the generator rather than read
        bool objectsMade = false;   // objects made (for a read zone, once gameObjects asks)
        struct ModelLightState {
            std::mutex mutex;
            std::set<int> waterModels; // flag8 is ORed, retained for this zone's lifetime
        };
        // Block edits copy the terrain snapshot; runtime model state stays shared.
        std::shared_ptr<ModelLightState> modelLights = std::make_shared<ModelLightState>();
    };
    static std::optional<scenevisibility::Bounds> boundsFor(int ax, int ay, const std::shared_ptr<Zone> (&around)[3][3]);
    using Key = std::pair<int, int>;
    std::filesystem::path folder;
    // Game sprite tables: environment type -> sprite, building part -> sprite
    // (names without "sprites/" and ".cub"; empty when unknown).
    std::vector<std::string> environment, buildingParts;
    // Sprite name -> blocks per voxel, measured from the game's own draw calls
    // (Extract/sprite-scales.txt). Building parts are sized per type in code.
    std::map<std::string, float> measuredScale;
    mutable std::map<std::string, float> estimatedScale; // guarded by cacheMutex
    std::filesystem::path spriteFolder;
    mutable std::map<std::string, int> voxelWidths; // owner model headers, guarded by cacheMutex
    float partScale(const std::string &name) const;
    std::pair<float, bool> objectScale(int type, const std::string &name, float scale) const;
    // Reads the sprite tables (sprite-groups.txt, sprite-scales.txt) in `dir`.
    void loadSpriteTables(const std::filesystem::path &dir);
    std::set<Key> zones; // Absolute zone coordinates present on disk.
    int originX = 0, originY = 0, seedValue = 0;
    bool generatedMode = false;
    glm::vec3 homeLocal{0};
    // Small cache of decoded zones shared by the worker threads.
    mutable std::mutex cacheMutex;
    std::list<std::pair<Key, std::shared_ptr<Zone>>> cache;
    struct Kept {
        std::vector<Key> centres;
        int radius = -1;
    };
    std::map<const void *, Kept> keptBy; // guarded by cacheMutex
    bool kept(const Key &key) const;
    std::thread keeper;               // reads the kept zones that are missing
    std::atomic<bool> keeping{false}; // while it runs
    std::set<Key> absent;             // kept zones with nothing to read (guarded by cacheMutex)
    std::shared_ptr<Zone> load(int ax, int ay); // absolute coordinates; null if missing
    // The block changes by absolute zone, put on every copy of the zone read or made.
    mutable std::mutex changesMutex;
    std::map<Key, std::vector<nativesaves::BlockChange>> blockChanges; // guarded by changesMutex
    std::set<Key> changedZones;                                          // guarded by changesMutex
    std::atomic<uint32_t> blocksRevision{0}; // bumped when a zone's blocks change (gameBlock's memo)
    void applyBlockChanges(int ax, int ay, Zone &zone) const;
    std::shared_ptr<Zone> cached(int ax, int ay); // as load, but only from the cache
    static std::shared_ptr<Zone> readZone(const std::filesystem::path &file, bool decode, std::string &error);
    static std::shared_ptr<Zone> generateZone(int ax, int ay);
};
