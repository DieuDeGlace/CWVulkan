#include "World.h"
#include "WaterLilies.h"
#include "AlphaLight.h"
#include "AlphaLook.h"
#include "Assets.h"
#include "CubModel.h"
#include "Extracted.h"
#include "worldgen/BlockColours.h"
#include "worldgen/Generator.h"
#include "worldgen/Height.h"
#include "worldgen/CellObjects.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <climits>
#include <cmath>
#include <memory>
#include <thread>

namespace {
// What the sky asks about a column of the game's world, from the generator
// (docs/specs/worldgen-terrain-height.md). Alpha's own zone factors, mountain field and ground
// are not specified (sky-and-clouds.md §D.2); the Steam generator's values stand in for them,
// as in World::zoneFactors.
SkyColumn skyColumnAt(int32_t x, int32_t y) {
    const worldgen::ColumnValues c = worldgen::columnValues(x, y);
    const float warmth = worldgen::warmth(c.regions, c.place.kind, c.place.weight);
    SkyColumn s;
    s.zone.warmth = std::clamp(warmth, 0.f, 1.f);
    s.zone.moisture = std::clamp(c.dryness, 0.f, 1.f);
    s.zone.fire = std::clamp(c.lavaRegion, 0.f, 1.f);
    s.mountain = std::clamp(c.highlands, 0.f, 1.f);
    s.region.warmth = warmth;
    s.region.dryness = c.dryness;
    s.region.ground = c.height;
    s.region.lava = c.lavaRegion;
    s.region.forest = c.forestRegion;
    s.region.kind27 = worldgen::kind27Weight(c.place.kind, c.place.weight);
    s.cloudGround = worldgen::groundHeight(x, y, worldgen::heightmode::NO_VARIANT_SHAPES);
    return s;
}
} // namespace

World::World(const Terrain &t, const Assets &a, Renderer &r, int view, ExtractedWorld *ex)
    : land(t), assets(a), renderer(r), viewZones(view), extracted(ex) {
    renderer.resetSceneRange();
    beamMesh = renderer.upload(lightbeam::mesh());
    // What the Alpha look asks about the world around the camera.
    WorldProbe probe;
    probe.water = [this](glm::vec3 p) { return waterAt(p); };
    probe.zone = [this](glm::vec3 p) { return zoneFactors(p); };
    probe.light = [this](glm::vec3 p) { return blockLight(p); };
    probe.usedCell = [this](int32_t x, int32_t y) { return usedCellProvider && usedCellProvider(x, y); };
    probe.effectLights = [this] {
        return effectLightProvider ? effectLightProvider() : std::vector<steampoints::EffectSource>{};
    };
    probe.transientLights = [this] {
        return transientLightProvider ? transientLightProvider() : std::vector<steampoints::TransientSource>{};
    };
    probe.creatureLights = [this](const steampoints::Frame &frame, std::vector<steampoints::Light> &out) {
        if (creatureLightCollector) creatureLightCollector(frame, out);
    };
    probe.groundLights = [this](steampoints::ZoneKey key) {
        return groundLightProvider ? groundLightProvider(key) : steampoints::GroundSources{};
    };
    if (extracted) {
        // The sky's questions go to the game's generator, in the game's own coordinates.
        probe.origin = glm::dvec3(extracted->originZoneX() * 64.0, extracted->originZoneY() * 64.0,
                                  -(double)ExtractedWorld::Z_OFFSET);
        probe.column = [](int32_t x, int32_t y) { return skyColumnAt(x, y); };
        probe.loadedLightZones = [this] { return extracted->loadedLightZones(); };
        probe.sceneZone = [this](int64_t x, int64_t y) -> std::optional<scenevisibility::Bounds> {
            const int64_t localX = x - extracted->originZoneX(), localY = y - extracted->originZoneY();
            if (localX < INT32_MIN || localX > INT32_MAX || localY < INT32_MIN || localY > INT32_MAX) return {};
            const Key key{(int)localX,(int)localY};
            const auto found = zones.find(key);
            const bool active = found != zones.end() && found->second.nativeBounds && found->second.nativeBounds->meshed;
            if (const auto pending = pendingBounds.find(key); pending != pendingBounds.end())
                if (auto bounds = pending->second->read()) {
                    bounds->meshed = active;
                    return bounds; // freshly completed absolute cache, independently of old GPU groups
                }
            if (found == zones.end() || !found->second.nativeBounds) return {};
            auto bounds = *found->second.nativeBounds;
            const int64_t offset[]{extracted->originDotsX(),extracted->originDotsY()};
            for (int axis = 0; axis < 2; ++axis) {
                bounds.minimum[axis] += offset[axis]; bounds.maximum[axis] += offset[axis]; bounds.center[axis] += offset[axis];
            }
            return bounds;
        };
        probe.scenePlayer = [this]() -> std::optional<scenerange::Player> {
            auto player = scenePlayerProvider ? scenePlayerProvider() : std::nullopt;
            if (player) {
                const auto &at = player->physical;
                const auto block = extracted->gameBlock(at[0] >> 16,at[1] >> 16,(int)(at[2] >> 16));
                player->blockFlags = block[5];
            }
            return player;
        };
        probe.sceneLighting = [this](glm::vec3 p) {
            const int64_t fixedX = (int64_t)((double)p.x * 65536) + extracted->originDotsX();
            const int64_t fixedY = (int64_t)((double)p.y * 65536) + extracted->originDotsY();
            // Palette metadata truncates to blocks; effective block lookup floors.
            const int32_t x = (int32_t)(fixedX / 65536), y = (int32_t)(fixedY / 65536);
            steamlighting::Environment environment;
            environment.region = skyColumnAt(x, y).region;
            const auto block = extracted->gameBlock((int64_t)std::floor((double)fixedX / 65536),
                                                    (int64_t)std::floor((double)fixedY / 65536),
                                                    (int)std::floor((double)p.z - ExtractedWorld::Z_OFFSET));
            environment.blockFlag20 = (block[5] & 0x20) != 0;
            if (environment.blockFlag20) {
                const auto place = worldgen::placeAt(x, y);
                const int32_t cellX = (int32_t)std::floor((double)x / 1024);
                const int32_t cellY = (int32_t)std::floor((double)y / 1024);
                environment.structureKind = worldgen::bigStructure(cellX, cellY, place.kind, place.big);
            }
            return environment;
        };
    }
    renderer.setWorldProbe(std::move(probe));
    loadSkyModels();
}

World::~World() {
    if (beamMesh)
        renderer.release(beamMesh);
    renderer.setWorldProbe({});
    renderer.setSkyModels({});
    for (uint32_t mesh : {skyModels.cloud, skyModels.darkCloud, skyModels.starCube})
        if (mesh)
            renderer.release(mesh);
    renderer.setUnloadedDistance(-1);
    for (auto &[key, job] : jobs)
        job.wait();
}

namespace {
// Scenery that gives off light in the Alpha look, by its sprite's name, and the colour it
// gives at night (docs/specs/alpha-rendering.md 10.1): lamps and lanterns a pale warm white,
// torches and fires orange. Both reach 20 blocks.
bool lightOf(const std::string &model, glm::vec3 &colour) {
    auto has = [&](const char *word) { return model.find(word) != std::string::npos; };
    if (has("fish")) // lantern fish are creatures, not lights
        return false;
    if (has("lamp") || has("lantern")) {
        colour = {1.2f, 1.0f, .8f};
        return true;
    }
    if (has("torch") || has("campfire") || has("fire")) {
        colour = {1.2f, .8f, .4f};
        return true;
    }
    return false;
}
} // namespace

void World::loadSkyModels() {
    auto load = [&](const char *name, CubModel &cub) {
        auto path = assets.resolve(fs::path("sprites") / name);
        return path && cub.load(*path);
    };
    CubModel cloud, dark, cube;
    if (renderStyle() == RenderStyle::Alpha) {
        // Alpha (§A.5): cloud02 as it comes, its creases shaded towards a mid blue.
        const glm::u8vec3 cloudShade(0, 100, 200);
        if (load("cloud02.cub", cloud))
            skyModels.cloud = renderer.upload(cloud.mesh(0, cloudShade));
        renderer.setSkyModels(skyModels);
        return;
    }
    // Steam (§B.7): cloud02 recoloured a grey blue, cloud-dark as it comes; the stars (§B.6).
    // (cloud-dark's shade colour, 10, 0, 20, has no counterpart in CubeVulkan's Steam meshes.)
    if (load("cloud02.cub", cloud)) {
        const uint8_t steamCloud[3] = {100, 130, 180};
        for (size_t i = 0; i + 2 < cloud.rgb.size(); i += 3)
            if (cloud.rgb[i] | cloud.rgb[i + 1] | cloud.rgb[i + 2])
                std::copy(steamCloud, steamCloud + 3, cloud.rgb.begin() + i);
        skyModels.cloud = renderer.upload(cloud.mesh());
    }
    if (load("cloud-dark.cub", dark))
        skyModels.darkCloud = renderer.upload(dark.mesh());
    if (load("cube.cub", cube))
        skyModels.starCube = renderer.upload(cube.mesh());
    renderer.setSkyModels(skyModels);
}

bool World::waterAt(glm::vec3 p) const {
    if (extracted)
        return extracted->waterAt(p.x, p.y, p.z);
    ColumnInfo c = land.column(p.x, p.y);
    return c.water && p.z < c.water && p.z >= c.ground;
}

alpha::ZoneFactors World::zoneFactors(glm::vec3 p) const {
    // The factors change slowly: work them out again only when the camera crosses into
    // another 4 x 4 block cell.
    glm::ivec2 cell(glm::floor(glm::vec2(p) / 4.f));
    if (zoneCacheValid && cell == zoneCacheCell)
        return zoneCache;
    alpha::ZoneFactors z;
    if (extracted) {
        // The generator's own column values: its warmth, its dryness value (high where the
        // ground is green and wet, low in deserts, so it serves as moisture), its lava regions
        // as fire; and the share of the ground around below the sea.
        int32_t gx = (int32_t)std::floor(p.x) + extracted->originZoneX() * 64;
        int32_t gy = (int32_t)std::floor(p.y) + extracted->originZoneY() * 64;
        const worldgen::ColumnField f = worldgen::columnField(gx, gy);
        z.warmth = std::clamp(f.colours.warmth, 0.f, 1.f);
        z.moisture = std::clamp(f.colours.dryness, 0.f, 1.f);
        z.fire = std::clamp(f.colours.lavaRegion, 0.f, 1.f);
        int below = 0;
        for (int j = -1; j <= 1; j++)
            for (int i = -1; i <= 1; i++)
                below += worldgen::plainBaseHeight(gx + i * 32, gy + j * 32) < 0 ? 1 : 0;
        z.sea = below / 9.f;
    } else {
        // CubeVulkan's own terrain: each region type's character, blended by its weight.
        struct Character {
            float warmth, moisture, fire;
        };
        static const Character kinds[BIOMES] = {
            {.5f, .5f, 0},   // plains
            {.5f, .6f, 0},   // ocean
            {.5f, .5f, 0},   // hills
            {.5f, .6f, 0},   // woodlands
            {.45f, .65f, 0}, // dark woods
            {.35f, .45f, 0}, // mountains
            {.7f, .9f, 0},   // wetlands
            {.9f, .1f, 0},   // desert
            {.1f, .5f, 0},   // snowlands
            {.8f, .2f, .6f}, // deadlands
        };
        ColumnInfo c = land.column(p.x, p.y);
        z.warmth = z.moisture = 0;
        for (int b = 0; b < BIOMES; b++) {
            z.warmth += c.weights[b] * kinds[b].warmth;
            z.moisture += c.weights[b] * kinds[b].moisture;
            z.fire += c.weights[b] * kinds[b].fire;
        }
        z.sea = c.weights[OCEAN];
    }
    zoneCache = z;
    zoneCacheCell = cell;
    zoneCacheValid = true;
    return z;
}

namespace {
// How far the flood's 16 sweeps carry light sideways from a sky-lit block, and how far up and
// down the volume around a lookup reaches.
constexpr int LIGHT_REACH = 16, LIGHT_HEIGHT = 8;

// The light read where a creature or model stands (alpha-rendering.md 11.2): an open block's
// light, at least 5; a glowing block's full. A model whose origin sits inside a solid block (a
// foot sunk into the floor) reads the block above instead.
float standingLight(const alphalight::LightVolume &volume, glm::ivec3 at) {
    const glm::ivec3 sample = volume.kind(at) == alphalight::BlockKind::Solid ? at + glm::ivec3(0, 0, 1) : at;
    const alphalight::BlockKind kind = volume.kind(sample);
    if (kind == alphalight::BlockKind::Glow)
        return 1;
    if (kind == alphalight::BlockKind::Solid)
        return 0;
    return std::max<int>(volume.light(sample), 5) / 255.f;
}
} // namespace

void World::fillLight(alphalight::LightVolume &volume) const {
    const glm::ivec3 origin = volume.origin(), size = volume.size();
    const size_t columns = (size_t)size.x * size.y;
    std::vector<uint8_t> kinds(columns * size.z), sky(columns, 1);
    if (extracted)
        extracted->lightColumns(origin.x, origin.y, size.x, size.y, origin.z, size.z, kinds.data(), sky.data());
    else
        for (int y = 0; y < size.y; y++)
            for (int x = 0; x < size.x; x++) {
                const int ground = land.column(origin.x + x + .5, origin.y + y + .5).ground;
                for (int k = 0; k < size.z; k++)
                    kinds[((size_t)y * size.x + x) * size.z + k] =
                        (uint8_t)(origin.z + k < ground ? alphalight::BlockKind::Solid : alphalight::BlockKind::Open);
            }
    for (int y = 0; y < size.y; y++)
        for (int x = 0; x < size.x; x++) {
            const size_t at = (size_t)y * size.x + x;
            for (int k = 0; k < size.z; k++)
                volume.setKind({origin.x + x, origin.y + y, origin.z + k}, (alphalight::BlockKind)kinds[at * size.z + k]);
            if (!sky[at])
                volume.setRoofed(origin.x + x, origin.y + y);
        }
}

float World::blockLight(glm::vec3 p) const {
    // The light the flood gives the block (docs/specs/alpha-rendering.md 9.2), worked out over
    // the blocks around it: as far as the flood's 16 sweeps reach sideways, half that up and
    // down, with sky light coming down every column that is open above.
    const glm::ivec3 at(glm::floor(p));
    alphalight::LightVolume volume(at - glm::ivec3(LIGHT_REACH, LIGHT_REACH, LIGHT_HEIGHT),
                                   glm::ivec3(2 * LIGHT_REACH + 1, 2 * LIGHT_REACH + 1, 2 * LIGHT_HEIGHT + 1));
    fillLight(volume);
    volume.flood();
    return standingLight(volume, at);
}

void World::lightScenery(std::vector<Scenery> &scenery) const {
    // A zone model's material takes the light where it stands, unless it shines itself
    // (vegetation-wind.md §4.5; alpha-rendering.md 11.2), as creatures do. The record's +0x28
    // factor is taken as 255 (the spec does not say what fills it). One flood covers them all.
    if (scenery.empty())
        return;
    auto base = [](const Scenery &s) { return glm::ivec3(glm::floor(glm::vec3(s.x, s.y, s.z + .1f))); };
    glm::ivec3 lo(INT32_MAX), hi(INT32_MIN);
    for (const Scenery &s : scenery)
        lo = glm::min(lo, base(s)), hi = glm::max(hi, base(s));
    lo -= glm::ivec3(LIGHT_REACH, LIGHT_REACH, LIGHT_HEIGHT);
    hi += glm::ivec3(LIGHT_REACH, LIGHT_REACH, LIGHT_HEIGHT);
    const glm::ivec3 size = hi - lo + 1;
    constexpr int MOST_COLUMNS = 160, MOST_HEIGHT = 256; // (a zone and its border; guards stray records)
    const bool oneFlood = size.x <= MOST_COLUMNS && size.y <= MOST_COLUMNS && size.z <= MOST_HEIGHT;
    std::unique_ptr<alphalight::LightVolume> volume;
    if (oneFlood) {
        volume = std::make_unique<alphalight::LightVolume>(lo, size);
        fillLight(*volume);
        volume->flood();
    }
    for (Scenery &s : scenery) {
        if (s.modelFlags & vegetationwind::modelflag::OWN_LIGHT)
            s.light = 1;
        else
            s.light = oneFlood ? standingLight(*volume, base(s)) : blockLight(glm::vec3(base(s)) + .5f);
    }
}


// Hair sprites take the character's hair colour, as the game tints them
// (shared by the world renderer and the menu's portraits).
void World::tintHair(CubModel &cub, glm::u8vec3 tint, const HairLook *look) {
    // Hair sprites: pure red, green and blue voxels are markers the
    // game does not draw (the face and eye areas); grey voxels take the
    // colour; skin-coloured voxels stay as they are.
    // Grey includes near-greys such as (242,241,241): exact equality left a
    // few voxels on top of the hair uncoloured.
    auto marker = [](const uint8_t *p) {
        int lit = (p[0] == 255) + (p[1] == 255) + (p[2] == 255), dark = !p[0] + !p[1] + !p[2];
        return lit == 1 && dark == 2;
    };
    auto grey = [](const uint8_t *p) {
        int hi = std::max({p[0], p[1], p[2]}), lo = std::min({p[0], p[1], p[2]});
        return hi > 0 && hi - lo <= 12;
    };
    // The hair's height range, for the second colour's gradient (as DualHair).
    int top = -1, bottom = cub.sz;
    for (int z = 0; z < cub.sz; z++)
        for (int i = 0; i < cub.sx * cub.sy; i++)
            if (grey(&cub.rgb[((size_t)z * cub.sx * cub.sy + i) * 3]))
                top = std::max(top, z), bottom = std::min(bottom, z);
    float span = (float)std::max(1, top - bottom);
    for (int z = 0; z < cub.sz; z++) {
        float share = 0;
        if (look) {
            float depth = (top - z) / span;
            if (look->blend <= 0)
                share = depth >= look->start ? 1.f : 0.f;
            else {
                float s = std::clamp((depth - look->start) / look->blend, 0.f, 1.f);
                share = s * s * (3 - 2 * s);
            }
        }
        for (int i = 0; i < cub.sx * cub.sy; i++) {
            uint8_t *p = &cub.rgb[((size_t)z * cub.sx * cub.sy + i) * 3];
            if (marker(p))
                p[0] = p[1] = p[2] = 0; // Not drawn (face and eye areas).
            else if (grey(p))
                for (int c = 0; c < 3; c++) {
                    float t = tint[c] + ((look ? look->second[c] : tint[c]) - tint[c]) * share;
                    p[c] = (uint8_t)std::clamp((int)std::lround(p[c] * t / 255), 1, 254);
                }
        }
    }
}

int World::loadModel(const std::string &name, glm::u8vec3 tint, bool hair, const HairLook *look) {
    bool tinted = hair;
    bool dual = hair && look && look->dual;
    std::string key = name;
    if (tinted)
        key += "#" + std::to_string(tint.r) + "," + std::to_string(tint.g) + "," + std::to_string(tint.b);
    if (dual)
        key += "+" + std::to_string(look->second.r) + "," + std::to_string(look->second.g) + "," +
               std::to_string(look->second.b) + "," + std::to_string(look->start) + "," + std::to_string(look->blend);
    if (auto it = modelIndex.find(key); it != modelIndex.end())
        return it->second;
    int index = -1;
    if (auto path = assets.resolve(fs::path("sprites") / (name + ".cub"))) {
        CubModel cub;
        if (cub.load(*path)) {
            if (tinted)
                tintHair(cub, tint, dual ? look : nullptr);
            Mesh mesh = cub.mesh();
            Model m;
            m.mesh = renderer.upload(mesh);
            m.height = mesh.max.z;
            m.size = glm::vec3(cub.sx, cub.sy, cub.sz);
            m.name = name, m.tint = tint, m.tinted = tinted;
            if (m.mesh) {
                index = (int)models.size();
                models.push_back(m);
            }
        }
    }
    if (index < 0)
        missingModels.push_back(name);
    modelIndex[key] = index;
    return index;
}

const std::vector<World::Voxel> &World::voxels(int model) const {
    static const std::vector<Voxel> none;
    if (model < 0 || model >= (int)models.size())
        return none;
    const Model &m = models[model];
    if (!m.voxelsRead) {
        m.voxelsRead = true;
        CubModel cub;
        if (auto path = assets.resolve(fs::path("sprites") / (m.name + ".cub")); path && cub.load(*path)) {
            if (m.tinted)
                tintHair(cub, m.tint, nullptr);
            for (int z = 0; z < cub.sz; z++)
                for (int y = 0; y < cub.sy; y++)
                    for (int x = 0; x < cub.sx; x++)
                        if (cub.exposed(x, y, z)) {
                            VoxelColor c = cub.color(x, y, z);
                            m.voxels.push_back({glm::vec3(x - cub.sx * .5f, y - cub.sy * .5f, z),
                                                glm::u8vec3(c.r, c.g, c.b)});
                        }
        }
    }
    return m.voxels;
}

DrawItem World::instanceDraw(const Instance &i) const {
    DrawItem d;
    d.mesh = models[i.model].mesh;
    d.model = glm::scale(glm::rotate(glm::translate(glm::mat4(1), i.position), i.yaw, glm::vec3(0, 0, 1)),
                         glm::vec3(i.scale));
    d.flags = i.flags;
    d.height = (i.flags & SWAY) ? models[i.model].height : 0;
    return d;
}

int World::spawn(int model, glm::vec3 position, float yaw, float scale, uint32_t flags) {
    if (model < 0 || model >= (int)models.size() || !(scale > 0))
        return -1;
    Instance i{model, position, yaw, scale, flags, true};
    for (size_t k = 0; k < instances.size(); k++)
        if (!instances[k].live) {
            instances[k] = i;
            return (int)k;
        }
    instances.push_back(i);
    return (int)instances.size() - 1;
}

void World::move(int instance, glm::vec3 position, float yaw) {
    if (instance >= 0 && instance < (int)instances.size() && instances[instance].live) {
        instances[instance].position = position;
        instances[instance].yaw = yaw;
    }
}

void World::remove(int instance) {
    if (instance >= 0 && instance < (int)instances.size())
        instances[instance].live = false;
}

void World::update(glm::vec3 focus) {
    int cx = (int)std::floor(focus.x / ZONE), cy = (int)std::floor(focus.y / ZONE);
    // Finish completed jobs, nearest first, a few per frame to keep frames smooth.
    int integrated = 0;
    for (auto it = jobs.begin(); it != jobs.end() && integrated < 6;) {
        if (it->second.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
            ++it;
            continue;
        }
        Generated g = it->second.get();
        Key key = it->first;
        it = jobs.erase(it);
        pendingBounds.erase(key);
        if (std::max(std::abs(key.first - cx), std::abs(key.second - cy)) > viewZones + 1)
            continue; // Walked away meanwhile.
        if (auto old = zones.find(key); old != zones.end()) { // meshed again
            renderer.release(old->second.opaque);
            renderer.release(old->second.water);
            renderer.release(old->second.lava);
        }
        Zone z;
        if (extracted && land.config().lilies) {
            const auto &settings = land.config();
            auto eligible = [&](glm::vec3 surface) {
                if (!extracted->waterAt(surface.x, surface.y, surface.z - .01f) ||
                    extracted->biomeAt(surface.x, surface.y) == OCEAN || zoneFactors(surface).warmth <= .2f)
                    return false;
                for (int depth = 1; depth <= settings.lilyMaximumDepth; depth++)
                    if (extracted->solidAt(surface.x, surface.y, surface.z - depth - .01f))
                        return true;
                return false;
            };
            auto pads = waterLilies::decorate(g.water, settings.seed, settings.lilyPatchDensity,
                                              settings.lilyMaxPadsPerZone, eligible);
            if (renderStyle() == RenderStyle::Alpha)
                lightScenery(pads);
            g.scenery.insert(g.scenery.end(), pads.begin(), pads.end());
        }
        z.opaque = renderer.upload(g.opaque);
        z.water = renderer.upload(g.water);
        z.lava = renderer.upload(g.lava);
        z.nativeLights = std::move(g.nativeLights);
        if (extracted && g.nativeBounds) {
            const int64_t shift[] = {-extracted->originDotsX(), -extracted->originDotsY(), 0};
            for (int axis = 0; axis < 3; ++axis) {
                g.nativeBounds->minimum[axis] += shift[axis];
                g.nativeBounds->maximum[axis] += shift[axis];
                g.nativeBounds->center[axis] += shift[axis];
            }
            g.nativeBounds->meshed = z.opaque || z.water || z.lava; // actual published geometry, not planned bands
            z.nativeBounds = std::make_shared<const scenevisibility::Bounds>(*g.nativeBounds);
        }
        for (auto &s : g.scenery) {
            if (s.beam) {
                DrawItem shaft;
                shaft.mesh = beamMesh;
                shaft.castShadow = false;
                shaft.kind = DrawKind::LightBeam;
                shaft.beam = *s.beam;
                shaft.nativeZone = z.nativeBounds;
                z.scenery.push_back(shaft);
                continue;
            }
            int m = loadModel(s.model);
            if (m < 0)
                continue;
            // Plants move by their model matrix (vegetation-wind.md §4), never per vertex. The
            // old generator's scenery marks its plants SWAY instead of carrying record flags.
            int32_t modelFlags = s.modelFlags;
            if (s.flags & SWAY)
                modelFlags |= vegetationwind::modelflag::SWAYS;
            Instance i{m, {s.x, s.y, s.z}, s.yaw, s.scale, (uint32_t)(s.flags & ~SWAY), true};
            z.scenery.push_back(instanceDraw(i));
            DrawItem &d = z.scenery.back();
            if (s.object.type >= 0) {
                d.model = objectvisual::matrix(s.object, i.position, models[m].size);
                z.objects.push_back({z.scenery.size() - 1, s.objectKey, s.object, i.position, models[m].size});
            }
            d.kind = DrawKind::Scenery;
            if (s.modelType >= 0 && extracted) {
                auto position = s.modelPosition;
                position[0] -= (int64_t)extracted->originZoneX() * 64 * 65536;
                position[1] -= (int64_t)extracted->originZoneY() * 64 * 65536;
                // Native CPU culling keeps game Z; GPU placement alone has Z_OFFSET.
                d.nativeScenery = steamscenery::bounds(position, models[m].size, s.scale, (uint32_t)modelFlags);
                d.alphaScenery = alphascenery::Placement{alphascenery::Kind::Model, d.nativeScenery->center};
                d.nativeZone = z.nativeBounds;
            }
            if (s.object.type >= 0 && s.objectKey.type >= 0 && extracted) {
                auto position = s.objectKey.position;
                position[0] -= (int64_t)extracted->originZoneX() * 64 * 65536;
                position[1] -= (int64_t)extracted->originZoneY() * 64 * 65536;
                d.alphaScenery = alphascenery::Placement{alphascenery::Kind::Object, position};
            }
            d.standingLight = (s.modelFlags & vegetationwind::modelflag::OWN_LIGHT) ? 1.0f : s.light;
            if (renderStyle() == RenderStyle::Steam && s.modelType >= 0 && (modelFlags & vegetationwind::modelflag::OWN_LIGHT))
                d.flags |= GLOW; // Native record flag0x01 drives glow; flag0x02 does not.
            using namespace vegetationwind::modelflag;
            if (modelFlags & (SWAYS | SWINGS_Y | SWINGS_X)) {
                vegetationwind::Placement p;
                p.position = {s.x, s.y, s.z};
                p.yawRadians = s.yaw;
                p.size = s.scale;
                p.heightVoxels = models[m].height;
                p.flags = modelFlags;
                p.listPosition = s.listPosition > 0 ? s.listPosition : (int)z.scenery.size();
                z.moving.push_back({z.scenery.size() - 1, p});
            }
            glm::vec3 colour;
            if (lightOf(s.model, colour)) { // shines from its top (10.1)
                d.lightColour = colour;
                d.lightRadius = 20;
                d.lightPosition = glm::vec3(s.x, s.y, s.z + models[m].height * s.scale);
            }
            // Lily pads and stones are flat: let them receive but not cast.
            if (s.model.rfind("water-lily", 0) == 0 || s.model.rfind("stone", 0) == 0)
                z.scenery.back().castShadow = false;
        }
        zones[key] = std::move(z);
        integrated++;
    }
    // Drop zones out of range.
    for (auto it = zones.begin(); it != zones.end();) {
        if (std::max(std::abs(it->first.first - cx), std::abs(it->first.second - cy)) > viewZones + 1) {
            renderer.release(it->second.opaque);
            renderer.release(it->second.water);
            renderer.release(it->second.lava);
            it = zones.erase(it);
        } else
            ++it;
    }
    // Queue missing zones, nearest first.
    unsigned workers = std::max(2u, std::thread::hardware_concurrency()) - 1;
    std::vector<Key> wanted;
    for (int dy = -viewZones; dy <= viewZones; dy++)
        for (int dx = -viewZones; dx <= viewZones; dx++) {
            Key k{cx + dx, cy + dy};
            if (!zones.count(k) && !jobs.count(k))
                wanted.push_back(k);
        }
    // Zones whose blocks changed are meshed again, before any other.
    for (auto it = stale.begin(); it != stale.end();) {
        if (jobs.count(*it)) {
            ++it;
            continue;
        }
        if (zones.count(*it))
            wanted.push_back(*it);
        it = stale.erase(it);
    }
    std::stable_sort(wanted.begin(), wanted.end(), [&](const Key &a, const Key &b) {
        auto d = [&](const Key &k) {
            return (k.first - cx) * (k.first - cx) + (k.second - cy) * (k.second - cy);
        };
        const bool firstMeshed = zones.count(a) != 0, secondMeshed = zones.count(b) != 0;
        if (firstMeshed != secondMeshed)
            return firstMeshed;
        return d(a) < d(b);
    });
    // The nearest ground not drawn yet, for the Alpha look's fog: the nearest corner of a
    // zone in the window that is not loaded.
    float nearest2 = -1;
    for (int dy = -viewZones; dy <= viewZones; dy++)
        for (int dx = -viewZones; dx <= viewZones; dx++) {
            Key k{cx + dx, cy + dy};
            if (zones.count(k))
                continue;
            for (int corner = 0; corner < 4; corner++) {
                glm::vec2 at(float((k.first + (corner & 1)) * ZONE), float((k.second + (corner >> 1)) * ZONE));
                glm::vec2 d = at - glm::vec2(focus);
                float d2 = glm::dot(d, d);
                if (nearest2 < 0 || d2 < nearest2)
                    nearest2 = d2;
            }
        }
    renderer.setUnloadedDistance(nearest2 < 0 ? 1e9f : std::sqrt(nearest2));
    const bool alphaLook = renderStyle() == RenderStyle::Alpha; // (models take the light where they stand)
    for (auto &k : wanted) {
        if (jobs.size() >= workers)
            break;
        const Terrain *t = &land;
        if (extracted) {
            if (!extracted->has(k.first, k.second)) {
                zones[k] = Zone{}; // Outside the extracted area: nothing to draw.
                continue;
            }
            ExtractedWorld *ex = extracted;
            auto publication = std::make_shared<scenerange::CachePublication>();
            pendingBounds[k] = publication;
            jobs[k] = std::async(std::launch::async, [this, ex, k, alphaLook, publication] {
                Generated g;
                worldgen::MeshLightVolume nativeLight;
                ex->mesh(k.first, k.second, g.opaque, g.water, alphaLook ? nullptr : &g.nativeBounds,
                         alphaLook ? nullptr : &nativeLight, &g.lava);
                if (g.nativeBounds) publication->publish(*g.nativeBounds);
                g.scenery = ex->props(k.first, k.second);
                if (alphaLook)
                    lightScenery(g.scenery);
                else {
                    ex->lightModels(k.first, k.second, g.scenery, &nativeLight);
                    g.nativeLights = ex->lightSources(k.first, k.second, nativeLight);
                }
                return g;
            });
            continue;
        }
        jobs[k] = std::async(std::launch::async, [this, t, k, alphaLook] {
            Generated g;
            ZoneData data = t->generate(k.first, k.second);
            t->mesh(data, g.opaque, g.water);
            // Zone meshes are zone-local; move them into world space.
            glm::vec3 offset((float)k.first * ZONE, (float)k.second * ZONE, 0);
            for (Mesh *m : {&g.opaque, &g.water}) {
                for (auto &v : m->vertices) {
                    v.x += offset.x;
                    v.y += offset.y;
                }
                m->min += offset;
                m->max += offset;
            }
            g.scenery = std::move(data.scenery);
            if (alphaLook)
                lightScenery(g.scenery);
            return g;
        });
    }
}

void World::refreshZone(int zx, int zy) { stale.insert({zx, zy}); }

void World::collect(std::vector<DrawItem> &opaque, std::vector<DrawItem> &water) const {
    const bool steamLook = renderStyle() == RenderStyle::Steam;
    for (auto &[key, z] : zones) {
        auto terrainDraw = [&](uint32_t mesh, std::vector<DrawItem> &draws, DrawKind kind, uint32_t flags = 0, bool castShadow = true) {
            if (!mesh)
                return;
            DrawItem draw{mesh, glm::mat4(1), flags, 0, castShadow, kind};
            if (steamLook) {
                draw.nativeZone = z.nativeBounds;
                draw.nativeLights = z.nativeLights;
            }
            draws.push_back(std::move(draw));
        };
        terrainDraw(z.opaque, opaque, DrawKind::Terrain);
        terrainDraw(z.lava, opaque, DrawKind::LavaTerrain, LAVA_LIGHTING);
        terrainDraw(z.water, water, DrawKind::Terrain, 0, false);
        const size_t first = opaque.size();
        opaque.insert(opaque.end(), z.scenery.begin(), z.scenery.end());
        for (const Moving &m : z.moving)
            opaque[first + m.draw].model = vegetationwind::modelMatrix(m.placement, clockMs, steamLook);
        for (const AnimatedObject &object : z.objects) {
            auto state = object.captured;
            if (objectProvider && object.key.index >= 0 && extracted) {
                const auto live = objectProvider(key.first + extracted->originZoneX(), key.second + extracted->originZoneY(),
                                                 object.key);
                if (live && live->type == state.type) {
                    // Placement metadata belongs to this source snapshot. In particular,
                    // regenerated lamps must not replace a captured undefined turn byte.
                    state.closed = live->closed;
                    state.elapsedMs = live->elapsedMs;
                }
            }
            opaque[first + object.draw].model = objectvisual::matrix(state, object.position, object.dimensions);
        }
    }
    for (auto &i : instances)
        if (i.live)
            opaque.push_back(instanceDraw(i));
}
