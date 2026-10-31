// The game's world map: its regions, their draw, its markers and its cursor (WorldMap.h;
// docs/specs/worldmap-rendering.md cited as §n, docs/specs/map-markers.md as "markers §n").
#include "WorldMap.h"

#include "MapMarks.h"
#include "World.h"
#include "MapControls.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>

namespace worldmap {

namespace {

constexpr int REGIONS_PER_REQUEST = 8;   // the nearest missing regions asked for at once (model §1)
constexpr int REQUEST_REACH = 8;         // regions asked for: -8 .. +7 each way of the centre
constexpr double SWEEP_MS = 5000;        // how often regions not drawn lately are dropped
constexpr float PICK_REACH = 5000;       // map units the cursor's ray goes (0x42e5e4)
// The flight route's dots (not in the spec): spacing in map units, size, depth offset.
constexpr float ROUTE_SPACING = 1.0f, ROUTE_DOT = 0.25f, ROUTE_DEPTH_OFFSET = 1.0f;

double nowMs() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

// Integer division rounding toward minus infinity.
int64_t floorDivide(int64_t a, int64_t b) { return a / b - ((a % b != 0) && ((a < 0) != (b < 0))); }

// The draw item of one map model: `world` places it in map units relative to the centre.
DrawItem mapItem(uint32_t mesh, const MapCamera &camera, const glm::mat4 &world, float opacity, float depthOffset,
                 glm::vec3 colour = glm::vec3(1), float white = 0) {
    DrawItem d;
    d.mesh = mesh;
    d.model = camera.view * world;
    d.flags = WORLD_MAP;
    d.castShadow = false;
    d.map.lens = camera.lens;
    d.map.yaw = glm::radians(camera.yawDegrees);
    d.map.pitch = glm::radians(camera.pitchDegrees);
    d.map.opacity = opacity;
    d.map.depthOffset = depthOffset;
    d.map.colour = colour;
    d.map.white = white;
    return d;
}

} // namespace

int scaleOf(int level) { return worldgen::mapUnit(level); }

Built build(int rx, int ry, int level) {
    auto region = worldgen::mapRegion(rx, ry, level);
    Built b;
    b.x = rx, b.y = ry, b.level = level;
    b.baseHeight = region.base();
    b.sizeZ = region.sizeZ;
    b.markers = std::move(region.markers);
    b.borders = std::move(region.borders);
    if (region.sizeZ <= 0 || region.rgb.empty())
        return b;
    const VoxelModel model{region.side, region.side, region.sizeZ, region.rgb.data()};
    // Only the region's own columns are drawn; the border feeds their occlusion (§2.2).
    const int first = BORDER, last = BORDER + REGION - 1;
    meshModel(model, first, last, first, last, false, b.mesh);
    b.solid.assign((size_t)REGION * REGION * region.sizeZ, 0);
    for (int k = 0; k < region.sizeZ; k++)
        for (int j = 0; j < REGION; j++)
            for (int i = 0; i < REGION; i++)
                b.solid[((size_t)k * REGION + j) * REGION + i] = !model.empty(i + BORDER, j + BORDER, k);
    return b;
}

// --- The workers and the regions' lifetime ----------------------------------------------------

Map::~Map() {
    {
        std::lock_guard<std::mutex> guard(lock);
        stopping = true;
    }
    wake.notify_all();
    for (std::thread &t : workers)
        t.join();
}

void Map::work() {
    for (;;) {
        Key key;
        {
            std::unique_lock<std::mutex> guard(lock);
            wake.wait(guard, [&] { return stopping || !queue.empty(); });
            if (stopping)
                return;
            key = queue.front();
            queue.pop_front();
        }
        Built b = build(key[0], key[1], key[2]);
        std::lock_guard<std::mutex> guard(lock);
        done.push_back(std::move(b));
    }
}

void Map::update(Renderer &renderer, glm::dvec3 center, int level) {
    if (workers.empty()) { // (the game builds its missing regions in parallel)
        const unsigned count = std::max(1u, std::thread::hardware_concurrency() / 2);
        for (unsigned i = 0; i < count; i++)
            workers.emplace_back([this] { work(); });
    }
    // Ask for the nearest missing regions (model §1).
    const int64_t side = (int64_t)REGION * scaleOf(level);
    const int64_t cx = floorDivide((int64_t)center.x, side), cy = floorDivide((int64_t)center.y, side);
    std::vector<std::pair<int64_t, Key>> missing;
    for (int64_t rx = cx - REQUEST_REACH; rx < cx + REQUEST_REACH; rx++)
        for (int64_t ry = cy - REQUEST_REACH; ry < cy + REQUEST_REACH; ry++) {
            const Key key{(int)rx, (int)ry, level};
            if (!asked.count(key))
                missing.push_back({(rx - cx) * (rx - cx) + (ry - cy) * (ry - cy), key});
        }
    std::stable_sort(missing.begin(), missing.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
    std::vector<Built> finished;
    {
        std::lock_guard<std::mutex> guard(lock);
        for (const auto &[distance, key] : missing) {
            if ((int)queue.size() >= REGIONS_PER_REQUEST)
                break;
            asked[key] = true;
            queue.push_back(key);
        }
        finished.swap(done);
    }
    wake.notify_all();
    const double now = nowMs();
    for (Built &b : finished) {
        Region &r = regions[{b.x, b.y, b.level}];
        if (r.mesh)
            renderer.release(r.mesh);
        r = Region{};
        r.baseHeight = b.baseHeight;
        r.sizeZ = b.sizeZ;
        r.empty = b.mesh.empty();
        r.mesh = r.empty ? 0 : renderer.upload(b.mesh);
        r.solid = std::move(b.solid);
        r.markers = std::move(b.markers);
        r.borders = std::move(b.borders);
        r.builtMs = now;
        r.drawnFrame = frame;
    }
    sweep(renderer, now);
}

void Map::sweep(Renderer &renderer, double now) {
    if (now - lastSweepMs < SWEEP_MS)
        return;
    // Drop the regions not drawn since the last sweep (that arrived before it).
    for (auto it = regions.begin(); it != regions.end();) {
        if (it->second.drawnFrame < frameAtSweep && it->second.builtMs < lastSweepMs) {
            if (it->second.mesh)
                renderer.release(it->second.mesh);
            asked.erase(it->first);
            it = regions.erase(it);
        } else
            ++it;
    }
    lastSweepMs = now;
    frameAtSweep = frame;
}

void Map::clear(Renderer &renderer) {
    std::lock_guard<std::mutex> guard(lock);
    for (auto &[key, r] : regions)
        if (r.mesh)
            renderer.release(r.mesh);
    regions.clear();
    asked.clear();
    queue.clear();
    done.clear();
}

// --- The camera and the regions' places -------------------------------------------------------

MapCamera Map::cameraOf(const View &view) {
    return mapCamera(view.yaw, view.pitch, view.viewScale, view.cursor, view.screen);
}

glm::dvec3 Map::regionOffset(const View &view, int rx, int ry, int baseHeight) {
    // §4.3: the model's corner, three units outside the region, at its base, in map units.
    const double unit = scaleOf(view.level);
    const glm::dvec3 corner(REGION * unit * rx - BORDER * unit, REGION * unit * ry - BORDER * unit, baseHeight * unit);
    return (corner - view.center) / unit;
}

template <class Visit> void Map::forEachRegion(const View &view, Visit &&visit) const {
    // The centre's region, by divisions that truncate toward zero (§4.3, markers §0.6).
    const int64_t side = (int64_t)REGION * scaleOf(view.level);
    const int64_t cx = (int64_t)view.center.x / side, cy = (int64_t)view.center.y / side;
    for (int64_t rx = cx - view.radius; rx < cx + view.radius; rx++)
        for (int64_t ry = cy - view.radius; ry < cy + view.radius; ry++)
            visit((int)rx, (int)ry, Key{(int)rx, (int)ry, view.level});
}

// --- The draw ---------------------------------------------------------------------------------

std::optional<worldgen::Int2> Map::collect(const View &view, std::vector<DrawItem> &out) {
    if (view.screen.y < 0.0001f)
        return view.hoveredLand;
    frame++;
    const MapCamera camera = cameraOf(view);
    const double now = nowMs();
    std::vector<DrawItem> fading;
    auto selected = view.hoveredLand;
    forEachRegion(view, [&](int rx, int ry, const Key &key) {
        auto it = regions.find(key);
        if (it == regions.end())
            return;
        if (view.forcedLand && view.forcedLand->x != std::numeric_limits<int32_t>::max())
            selected = view.forcedLand;
        Region &r = it->second;
        if (r.empty)
            return;
        r.drawnFrame = frame;
        const glm::mat4 world = glm::translate(glm::mat4(1), glm::vec3(regionOffset(view, rx, ry, r.baseHeight)));
        const float fade = regionFade(now - r.builtMs);
        if (fade <= 0)
            return;
        // A region still fading in goes with the blended draws, after the whole ones.
        (fade < 1 ? fading : out).push_back(mapItem(r.mesh, camera, world, fade, REGION_DEPTH_OFFSET));
    });
    out.insert(out.end(), fading.begin(), fading.end());
    if (selected && selected->x != std::numeric_limits<int32_t>::max())
        drawBorders(view, camera, *selected, out);
    drawMarks(view, camera, out);
    drawRoute(view, camera, out);
    return selected;
}

void Map::drawBorders(const View &view, const MapCamera &camera, worldgen::Int2 selected, std::vector<DrawItem> &out) const {
    const float opacity = borderOpacity(view.elapsedWorldMs);
    if (!cube.mesh || !(opacity > 0))
        return;
    const auto center = mapcontrol::fromBlocks(view.center);
    const int unit = scaleOf(view.level);
    forEachRegion(view, [&](int, int, const Key &key) {
        const auto tile = regions.find(key);
        if (tile == regions.end())
            return;
        const auto border = tile->second.borders.find({selected.x, selected.y});
        if (border == tile->second.borders.end())
            return;
        for (const auto &point : border->second) {
            const auto placed = borderPlacement(point, center, unit, view.viewScale, cube.size);
            out.push_back(mapItem(cube.mesh, camera, placed, opacity, 1.6f));
        }
    });
}

void Map::drawMarks(const View &view, const MapCamera &camera, std::vector<DrawItem> &out) const {
    const double unit = scaleOf(view.level);
    const double reach = REGION * unit * view.radius;
    // The view's depth axis (markers §2.1 step 5: far ones are drawn first).
    const glm::vec3 depthAxis(camera.view[0][2], camera.view[1][2], camera.view[2][2]);
    struct Record {
        const View::Mark *mark;
        glm::vec3 at; // map units from the centre
        float depth;
    };
    std::vector<Record> records;
    for (const View::Mark &m : view.marks) {
        if (m.looks < 0 || m.looks >= (int)markLooks.size())
            continue;
        const glm::vec3 at = glm::vec3(mapmarks::pullToSquare(m.at - view.center, reach) / unit);
        records.push_back({&m, at, glm::dot(at, depthAxis)});
    }
    std::stable_sort(records.begin(), records.end(), [](const Record &a, const Record &b) { return a.depth > b.depth; });
    const float scale = mapmarks::creatureMarkerScale(view.viewScale);
    float layer = FIRST_MARKER_LAYER;
    for (const Record &record : records) {
        const float pulse = mapmarks::markerPulse(view.worldClockMs, record.mark->pulses);
        glm::mat4 placed = glm::translate(glm::mat4(1), record.at);
        placed = glm::scale(placed, glm::vec3(scale));
        placed = glm::rotate(placed, glm::radians(record.mark->turnDegrees), glm::vec3(0, 0, 1));
        for (const Sprite &sprite : markLooks[record.mark->looks]) {
            if (!sprite.mesh)
                continue;
            // The outline: 26 black copies one voxel off each way, white as far as the pulse.
            for (int i = -1; i <= 1; i++)
                for (int j = -1; j <= 1; j++)
                    for (int k = -1; k <= 1; k++) {
                        if (i == 0 && j == 0 && k == 0)
                            continue;
                        const glm::vec3 shift = -0.5f * sprite.size + glm::vec3(i, j, k);
                        out.push_back(mapItem(sprite.mesh, camera, glm::translate(placed, shift), 1, layer, glm::vec3(0), pulse));
                    }
            out.push_back(mapItem(sprite.mesh, camera, glm::translate(placed, -0.5f * sprite.size), 1, layer + 0.5f,
                                  sprite.colour));
        }
        layer += 1;
    }
}

void Map::drawRoute(const View &view, const MapCamera &camera, std::vector<DrawItem> &out) const {
    if (!view.route || !cube.mesh)
        return;
    const double unit = scaleOf(view.level);
    const glm::vec3 from = glm::vec3((view.player - view.center) / unit);
    const glm::vec3 to = glm::vec3((glm::dvec3(view.routeTarget.x, view.routeTarget.y, view.routeTargetZ) - view.center) / unit);
    const float length = glm::length(to - from), half = (float)(REGION * view.radius);
    const float size = ROUTE_DOT / std::max(1.0f, cube.size.x);
    for (float t = 0; t <= length; t += ROUTE_SPACING) {
        const glm::vec3 p = length > 0 ? from + (to - from) * (t / length) : from;
        if (std::abs(p.x) > half || std::abs(p.y) > half)
            continue;
        glm::mat4 world = glm::translate(glm::mat4(1), p);
        world = glm::scale(world, glm::vec3(size));
        world = glm::translate(world, -0.5f * cube.size);
        out.push_back(mapItem(cube.mesh, camera, world, 1, ROUTE_DEPTH_OFFSET, cube.colour));
    }
}

Map::Sprite Map::spriteOf(Renderer &renderer, const CubModel &model, glm::vec3 colour) {
    Sprite s;
    s.size = glm::vec3(model.sx, model.sy, model.sz);
    s.colour = colour;
    if (model.sx <= 0 || model.sy <= 0 || model.sz <= 0)
        return s;
    // .cub models keep the halving the shader doubles back (§2.1).
    Mesh mesh;
    meshModel(VoxelModel{model.sx, model.sy, model.sz, model.rgb.data()}, 0, model.sx - 1, 0, model.sy - 1, true, mesh);
    if (!mesh.empty())
        s.mesh = renderer.upload(mesh);
    return s;
}

void Map::setCube(Renderer &renderer, const CubModel &model) {
    if (cube.mesh)
        renderer.release(cube.mesh);
    cube = spriteOf(renderer, model, glm::vec3(1));
}

int Map::looks(Renderer &renderer, const CubModel &first, const CubModel *hairModel, glm::u8vec3 hairColor, const hair::Style *style) {
    std::vector<Sprite> sprites{spriteOf(renderer, first, glm::vec3(1))};
    if (hairModel) {
        if (style && style->dual) {
            auto tinted = *hairModel;
            World::tintHair(tinted, hairColor, style);
            sprites.push_back(spriteOf(renderer, tinted, glm::vec3(1)));
        } else
            sprites.push_back(spriteOf(renderer, *hairModel, glm::vec3(hairColor) / 255.0f));
    }
    markLooks.push_back(std::move(sprites));
    return (int)markLooks.size() - 1;
}

// --- Markers, points and the cursor -----------------------------------------------------------

bool Map::project(const View &view, glm::dvec3 at, glm::vec2 &screen, float *depth) const {
    // markers §0.7: the offset in fixed point, times trunc(65536 / unit), shifted back.
    const int unit = scaleOf(view.level);
    const int64_t perUnit = (int64_t)(1.0 / unit * 65536.0);
    glm::vec3 offset;
    for (int i = 0; i < 3; i++) {
        const int64_t dots = (int64_t)((at[i] - view.center[i]) * 65536.0);
        offset[i] = (float)((dots * perUnit) >> 16) / 65536.0f;
    }
    const MapCamera camera = cameraOf(view);
    const glm::vec3 v = glm::vec3(camera.view * glm::vec4(offset, 1));
    if (v.z <= 0)
        return false;
    const glm::vec4 clip = clipOf(camera, v);
    const glm::vec3 device = glm::vec3(clip) / clip.w;
    if (device.x < -1 || device.x > 1 || device.y < -1 || device.y > 1)
        return false;
    screen = glm::vec2(view.screen.x * 0.5f * device.x + view.screen.x * 0.5f,
                       -view.screen.y * 0.5f * device.y + view.screen.y * 0.5f);
    if (depth)
        *depth = device.z * 0.5f;
    return true;
}

std::vector<Map::Projected> Map::markers(const View &view) const {
    std::vector<Projected> shown;
    forEachRegion(view, [&](int, int, const Key &key) {
        auto it = regions.find(key);
        if (it == regions.end())
            return;
        for (const worldgen::MapRegion::Marker &m : it->second.markers) {
            Projected p;
            p.marker = m;
            if (project(view, glm::dvec3(m.x, m.y, m.z), p.at, &p.depth))
                shown.push_back(p);
        }
    });
    return shown;
}

namespace {

// Walks a ray (model voxel units) through a region's own solid voxels; the distance along it
// to the first one met, or false.
bool castIntoRegion(glm::vec3 origin, glm::vec3 direction, const std::vector<uint8_t> &solid, int sizeZ, float reach,
                    float &hit) {
    const glm::ivec3 lo(BORDER, BORDER, 0), hi(BORDER + REGION, BORDER + REGION, sizeZ);
    float enter = 0, leave = reach;
    for (int a = 0; a < 3; a++) {
        if (std::abs(direction[a]) < 1e-9f) {
            if (origin[a] < lo[a] || origin[a] >= hi[a])
                return false;
            continue;
        }
        float t0 = (lo[a] - origin[a]) / direction[a], t1 = (hi[a] - origin[a]) / direction[a];
        if (t0 > t1)
            std::swap(t0, t1);
        enter = std::max(enter, t0);
        leave = std::min(leave, t1);
    }
    if (enter > leave)
        return false;
    const glm::vec3 start = origin + direction * (enter + 1e-4f);
    glm::ivec3 cell = glm::clamp(glm::ivec3(glm::floor(start)), lo, hi - 1);
    glm::ivec3 step;
    glm::vec3 next, delta;
    for (int a = 0; a < 3; a++) {
        step[a] = direction[a] > 0 ? 1 : -1;
        if (std::abs(direction[a]) < 1e-9f) {
            next[a] = delta[a] = 1e30f;
            continue;
        }
        next[a] = ((float)cell[a] + (step[a] > 0 ? 1.0f : 0.0f) - origin[a]) / direction[a];
        delta[a] = std::abs(1.0f / direction[a]);
    }
    float t = enter;
    while (t <= leave) {
        const size_t index = ((size_t)cell.z * REGION + (cell.y - BORDER)) * REGION + (cell.x - BORDER);
        if (solid[index]) {
            hit = t;
            return true;
        }
        const int a = next.x < next.y ? (next.x < next.z ? 0 : 2) : (next.y < next.z ? 1 : 2);
        t = next[a];
        cell[a] += step[a];
        if (cell[a] < lo[a] || cell[a] >= hi[a])
            return false;
        next[a] += delta[a];
    }
    return false;
}

} // namespace

bool Map::pick(const View &view, glm::vec2 pointer, glm::dvec3 &at) const {
    if (view.screen.x <= 0 || view.screen.y <= 0)
        return false;
    const MapCamera camera = cameraOf(view);
    // The pointer's ray in the camera's space, back through the lens.
    const glm::vec2 device(2 * pointer.x / view.screen.x - 1, 1 - 2 * pointer.y / view.screen.y);
    const glm::vec3 ray((device.x - camera.lens.z) / camera.lens.x, (device.y - camera.lens.w) / camera.lens.y, 1);
    const glm::mat4 back = glm::inverse(camera.view);
    const glm::vec3 origin = glm::vec3(back * glm::vec4(0, 0, 0, 1));
    const glm::vec3 direction = glm::normalize(glm::mat3(back) * ray);
    const double unit = scaleOf(view.level);
    bool found = false;
    forEachRegion(view, [&](int rx, int ry, const Key &key) {
        auto it = regions.find(key);
        if (it == regions.end() || it->second.empty)
            return;
        const Region &r = it->second;
        const glm::vec3 offset = glm::vec3(regionOffset(view, rx, ry, r.baseHeight));
        float t;
        if (castIntoRegion(origin - offset, direction, r.solid, r.sizeZ, PICK_REACH, t)) {
            at = view.center + glm::dvec3(origin + direction * t) * unit; // the last region met wins
            found = true;
        }
    });
    return found;
}

} // namespace worldmap
