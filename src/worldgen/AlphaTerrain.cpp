#include "worldgen/AlphaTerrain.h"
#include "alphagen/SteamBridge.h"
#include "worldgen/MapModel.h"
#include "worldgen/Regions.h"
#include <map>
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <tuple>
namespace worldgen {
namespace {
constexpr int BLEND = 256;
// Parallel terrain/neighbor requests need more than sixteen Alpha parents.
// LandscapeStore also bounds the estimated allocated ready-stage payload.
constexpr std::size_t LANDSCAPE_CACHE_PARENTS = 64;
struct Box {
    int64_t x0, y0, x1, y1;
    bool operator==(const Box &) const = default;
};
std::vector<Box> constructionBoxes(const WorldValues &world, int32_t zx, int32_t zy) {
    std::vector<Box> boxes;
    auto &cells = cellObjectsFor(world);
    BuildingRules rules(cells);
    // Include the whole halo plus the conservative native admission margin.
    constexpr int padding = BLEND / 16 + 4;
    for (int dx = -padding; dx < 4 + padding; ++dx)
        for (int dy = -padding; dy < 4 + padding; ++dy) {
            const int32_t cx = int32_t(int64_t(zx) * 4 + dx), cy = int32_t(int64_t(zy) * 4 + dy);
            const int type = rules.type(cx, cy);
            if (type < 22 && type != 6 && type != 7 && rules.castleCell(cx, cy).kind == 0)
                continue;
            // Preserve every whole tile admitted by the native cell pass. A guessed
            // model/building bounding box could omit rooms or overhanging walls.
            const int reach = cellReach(type);
            const int lowX = -floorDivBy(-(cx - 3 - reach), 4), highX = floorDivBy(cx + reach, 4);
            const int lowY = -floorDivBy(-(cy - 3 - reach), 4), highY = floorDivBy(cy + reach, 4);
            boxes.push_back({int64_t(lowX) * 64, int64_t(lowY) * 64, int64_t(highX + 1) * 64, int64_t(highY + 1) * 64});
        }
    std::sort(boxes.begin(), boxes.end(), [](const Box &a, const Box &b) {
        return std::tie(a.x0, a.y0, a.x1, a.y1) < std::tie(b.x0, b.y0, b.x1, b.y1);
    });
    boxes.erase(std::unique(boxes.begin(), boxes.end()), boxes.end());
    return boxes;
}
float constructionWeight(const std::vector<Box> &boxes, int32_t x, int32_t y) {
    int64_t nearest = int64_t(BLEND) * BLEND;
    for (const auto &box : boxes) {
        const int64_t dx = std::max({box.x0 - x, int64_t(x) - box.x1 + 1, int64_t(0)});
        const int64_t dy = std::max({box.y0 - y, int64_t(y) - box.y1 + 1, int64_t(0)});
        if (!dx && !dy)
            return 1;
        if (dx >= BLEND || dy >= BLEND)
            continue;
        nearest = std::min(nearest, dx * dx + dy * dy);
    }
    if (nearest >= int64_t(BLEND) * BLEND)
        return 0;
    const float t = std::sqrt(float(nearest)) / BLEND;
    return 1.f - t * t * (3.f - 2.f * t);
}
float ground(const TerrainColumn &column) {
    for (int64_t i = int64_t(column.blocks.size()) - 1; i >= 0; --i) {
        const auto &block = column.blocks[size_t(i)];
        if (!isOpen(block) && block.type != blocktype::WOOD && block.type != blocktype::LEAVES && block.type != 11)
            return float(int64_t(column.base) + i + 1);
    }
    return float(column.base);
}
float alphaGround(const alphagen::Column &column) {
    for (int64_t i = int64_t(column.blocks.size()) - 1; i >= 0; --i) {
        const auto type = column.blocks[size_t(i)].type();
        if (!alphagen::clear(type) && type != alphagen::WOOD_TYPE && type != alphagen::LEAVES_TYPE)
            return float(int64_t(column.bottom) + i + 1);
    }
    return float(column.bottom);
}
struct Blend {
    float construction = 0, land = 0, savannah = 0;
    int32_t shift(float alpha, float native) const {
        const float difference = native - alpha;
        // Steam land retains its floor; Alpha can add higher relief. Savannah
        // keeps its gentler native profile. The liquid planes remain independent.
        float delta = std::max(difference, 0.f) * land;
        delta += (difference - delta) * savannah;
        return int32_t(std::lround(delta + (difference - delta) * construction));
    }
};
Blend blendAt(const std::vector<Box> &boxes, int32_t x, int32_t y) {
    const auto regions = neighbourhood(x, y);
    float landDistance = std::numeric_limits<float>::max();
    float savannahDistance = landDistance;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            if (regions.biome[i][j] != biomes::OCEAN)
                landDistance = std::min(landDistance, regions.dist2[i][j]);
            if (regions.biome[i][j] == 10)
                savannahDistance = std::min(savannahDistance, regions.dist2[i][j]);
        }
    const float nearest = std::sqrt(std::max(0.f, regions.nearestDist2));
    const auto halo = [nearest](float distance) {
        const float t = std::clamp((std::sqrt(distance) - nearest) / BLEND, 0.f, 1.f);
        return 1.f - t * t * (3.f - 2.f * t);
    };
    return {constructionWeight(boxes, x, y), halo(landDistance), halo(savannahDistance)};
}
bool soil(const Block &block) {
    return !isOpen(block) && block.type != blocktype::WOOD && block.type != blocktype::LEAVES && block.type != 11 &&
           block.type != blocktype::LIGHT;
}
void biomePalette(TerrainColumn &column, const TerrainColumn &native, int32_t alphaGround, int32_t nativeGround) {
    // Geometry stays Alpha, while soil samples retain the Steam biome's actual
    // stored palette. All resulting bytes use the ordinary Steam encoding.
    const auto at = [&](int64_t z) {
        if (native.blocks.empty())
            return SOLID_GREY;
        const auto index = size_t(std::clamp(z - native.base, int64_t(0), int64_t(native.blocks.size()) - 1));
        const auto &block = native.blocks[index];
        if (soil(block))
            return block;
        const auto surface = int64_t(nativeGround) - 1 - native.base;
        return surface >= 0 && surface < int64_t(native.blocks.size()) ? native.blocks[size_t(surface)] : SOLID_GREY;
    };
    for (size_t i = 0; i < column.blocks.size(); ++i) {
        auto &block = column.blocks[i];
        if (!soil(block)) {
            block.r /= 2;
            block.g /= 2;
            block.b /= 2;
            continue;
        }
        const int64_t depth = int64_t(alphaGround) - 1 - (int64_t(column.base) + int64_t(i));
        const auto colour = at(int64_t(nativeGround) - 1 - depth);
        block.r = colour.r;
        block.g = colour.g;
        block.b = colour.b;
        block.glow = colour.glow;
        if (!depth)
            block.type = colour.type;
    }
    column.fullColour = false;
}
} // namespace
AlphaTerrain::AlphaTerrain(uint32_t seed, std::shared_ptr<const alphagen::TreeModels> palms, int32_t steamSeed,
                           std::shared_ptr<const alphagen::PlantModels> plants)
    : seed(seed), steamSeed(steamSeed), source(seed, {}, LANDSCAPE_CACHE_PARENTS, true, std::move(palms)), plantModels(std::move(plants)) {
}
std::string AlphaTerrain::identity(double scale) const {
    return "alpha-v" + std::to_string(alphagen::LandscapeStage::VERSION) + "-compose-v3-seed-" + std::to_string(seed) + "-steam-seed-" +
           std::to_string(steamSeed) + "-scale-bits-" + std::to_string(std::bit_cast<uint64_t>(scale));
}
bool AlphaTerrain::coversZone(int32_t zx, int32_t zy) const {
    const auto coverage = source.coverage(zx, zy);
    if (!coverage)
        return false;
    int columns = 0;
    for (size_t i = 0; i < coverage->count; ++i)
        columns += coverage->patches[i].width * coverage->patches[i].height;
    return columns == ZONE_COLUMNS;
}
std::optional<Block> AlphaTerrain::rock(int32_t x, int32_t y, int32_t z) {
    const auto block = source.rock(x, y, z);
    return block ? alphagen::steamBlock(*block) : std::nullopt;
}
std::optional<glm::vec3> AlphaTerrain::implicitColour(const TerrainColumn &column, int32_t x, int32_t y, int32_t z) {
    if (!column.fullColour || z >= column.base)
        return std::nullopt;
    const auto sourceZ = int64_t(z) - column.sourceShift;
    if (sourceZ < INT32_MIN || sourceZ > INT32_MAX)
        return std::nullopt;
    const auto rock = source.rockColour(x, y, int32_t(sourceZ));
    if (!rock)
        return std::nullopt;
    // Keep scalar division and the two contributions separate until vertex packing.
    // docs/specs/alpha-implicit-terrain-colour.md; this file uses strict float arithmetic.
    if (column.blocks.empty() || column.blocks.front().type == 0 || column.blocks.front().type == 2)
        return glm::vec3(rock->r / 255.f, rock->g / 255.f, rock->b / 255.f);
    const auto &reference = column.blocks.front();
    const float k = std::min(float(int64_t(column.base) - z) * .1f, 1.f);
    const auto blend = [k](uint8_t reference, float rock) {
        const float referencePart = (float(reference) * (1.f - k)) / 255.f;
        const float rockPart = (rock * k) / 255.f;
        return referencePart + rockPart;
    };
    return glm::vec3(blend(reference.r, rock->r), blend(reference.g, rock->g), blend(reference.b, rock->b));
}
void AlphaTerrain::composeMap(MapRegion &map) {
    struct Projection {
        bool retained = false, present = false;
        int32_t top = 0;
        std::array<uint8_t, 3> rgb{};
    };
    std::vector<Projection> columns(size_t(map.side) * map.side);
    std::map<std::pair<int32_t, int32_t>, std::vector<Box>> boxes;
    int32_t low = INT32_MAX, high = INT32_MIN;
    const auto oldBase = map.base();
    const auto world = worldValues(steamSeed);
    for (int i = 0; i < map.side; ++i)
        for (int j = 0; j < map.side; ++j) {
            auto &projection = columns[size_t(j) * map.side + i];
            const int64_t X = int64_t(map.unit) * (i - MAP_BORDER + int64_t(MAP_REGION_UNITS) * map.rx);
            const int64_t Y = int64_t(map.unit) * (j - MAP_BORDER + int64_t(MAP_REGION_UNITS) * map.ry);
            if (X < INT32_MIN || X > INT32_MAX || Y < INT32_MIN || Y > INT32_MAX)
                continue;
            const int32_t x = int32_t(X), y = int32_t(Y);
            const auto key = std::make_pair(floorDivBy(x, ZONE_SIZE), floorDivBy(y, ZONE_SIZE));
            auto [entry, fresh] = boxes.try_emplace(key);
            if (fresh)
                entry->second = constructionBoxes(world, key.first, key.second);
            const auto blend = blendAt(entry->second, x, y);
            const auto field = source.sample(x, y);
            if (!field)
                continue;
            projection.present = true;
            projection.retained = blend.construction == 1 || blend.savannah == 1;
            if (projection.retained) {
                low = std::min(low, oldBase);
                high = std::max(high, oldBase + map.sizeZ - 1);
                continue;
            }
            // Close map views sample real landscape voxels, including tree crowns.
            // Distant views use the cheaper H overview rather than generating every
            // off-screen parent. Neither path moves the Steam marker XY/layouts.
            float top = std::floor(field->height) + 1;
            float baseGround = top, waterTop = 1;
            auto colour = field->colour;
            if (map.unit <= 16) {
                if (const auto column = source.column(x, y)) {
                    const auto &raw = column->raw();
                    baseGround = alphaGround(raw);
                    top = column->surface(false);
                    colour = column->block(int32_t(top) - 1);
                    for (size_t k = 0; k < raw.blocks.size(); ++k)
                        if (raw.blocks[k].type() == alphagen::WATER)
                            waterTop = std::max(waterTop, float(int64_t(raw.bottom) + int64_t(k) + 1));
                }
            }
            const float native = std::floor(groundHeight(x, y)) + 1;
            top += blend.shift(baseGround, native);
            const bool waterVisible = waterTop > top;
            top = std::max(top, waterTop);
            projection.top = int32_t(std::floor(top / map.unit));
            // The Steam map vertex encoder expects half-intensity bytes.
            projection.rgb = waterVisible
                                 ? std::array<uint8_t, 3>{38, 51, 127}
                                 : std::array<uint8_t, 3>{uint8_t(colour.r / 2), uint8_t(colour.g / 2), uint8_t(colour.b / 2)};
            if (!waterVisible && native > 1) {
                for (int k = map.sizeZ - 1; k >= 0; --k) {
                    const auto *rgb = map.voxel(i, j, k);
                    if (rgb[0] || rgb[1] || rgb[2]) {
                        std::copy_n(rgb, 3, projection.rgb.begin());
                        break;
                    }
                }
            }
            if (projection.rgb == std::array<uint8_t, 3>{})
                projection.rgb = {1, 1, 1};
            low = std::min(low, projection.top);
            high = std::max(high, projection.top);
        }
    if (low == INT32_MAX) {
        map.sizeZ = 0;
        map.rgb.clear();
        map.markers.clear();
        map.borders.clear();
        return;
    }
    low -= 4;
    const int size = std::clamp(high - low + 8, 0, MAP_MOST_VOXELS);
    std::vector<uint8_t> rgb(size_t(map.side) * map.side * size * 3, 0);
    for (int i = 0; i < map.side; ++i)
        for (int j = 0; j < map.side; ++j) {
            const auto &column = columns[size_t(j) * map.side + i];
            if (!column.present)
                continue;
            for (int k = 0; k < size; ++k) {
                auto *out = &rgb[((size_t(k) * map.side + j) * map.side + i) * 3];
                const auto oldK = k + low - oldBase;
                if (column.retained && oldK >= 0 && oldK < map.sizeZ)
                    std::copy_n(map.voxel(i, j, oldK), 3, out);
                else if (!column.retained && k + low <= column.top)
                    std::copy(column.rgb.begin(), column.rgb.end(), out);
            }
        }
    map.lowest = low * map.unit;
    map.sizeZ = size;
    map.rgb = std::move(rgb);
    const auto moveHeight = [&](int32_t x, int32_t y, int32_t old) {
        const auto field = source.sample(x, y);
        if (!field)
            return old;
        const auto key = std::make_pair(floorDivBy(x, ZONE_SIZE), floorDivBy(y, ZONE_SIZE));
        auto [entry, fresh] = boxes.try_emplace(key);
        if (fresh)
            entry->second = constructionBoxes(world, key.first, key.second);
        const auto blend = blendAt(entry->second, x, y);
        if (blend.construction == 1 || blend.savannah == 1)
            return old;
        float alpha = std::floor(field->height) + 1;
        if (map.unit <= 16)
            if (const auto column = source.column(x, y))
                alpha = alphaGround(column->raw());
        const float native = std::floor(groundHeight(x, y)) + 1;
        return old + int32_t(std::lround(alpha + blend.shift(alpha, native) - native));
    };
    for (auto &marker : map.markers)
        marker.z = moveHeight(marker.x, marker.y, marker.z);
    for (auto &[key, border] : map.borders)
        for (auto &point : border)
            point[2] = moveHeight(point[0], point[1], point[2]);
}
bool AlphaTerrain::compose(ZoneContents &zone, const WorldValues &world, std::string &error) {
    if (world.seed != steamSeed) {
        error = "Steam layout seed differs from the Alpha composition profile";
        return false;
    }
    const auto zx = zone.terrain.zx, zy = zone.terrain.zy;
    auto tile = alphagen::steamTile(source, zx, zy, error);
    if (!tile)
        return false;
    if (zone.terrain.columns.size() != ZONE_COLUMNS) {
        error = "Steam base zone has incomplete column storage";
        return false;
    }
    const auto boxes = constructionBoxes(world, zx, zy);
    std::array<int32_t, ZONE_COLUMNS> offsets{};
    std::array<Blend, ZONE_COLUMNS> blends{};
    ZoneContents result = zone;
    for (int lx = 0; lx < ZONE_SIZE; ++lx)
        for (int ly = 0; ly < ZONE_SIZE; ++ly) {
            const int32_t x = int32_t(int64_t(zx) * ZONE_SIZE + lx), y = int32_t(int64_t(zy) * ZONE_SIZE + ly);
            if (!tile->covers(x, y)) {
                error = "Alpha landscape ends at the finite-world boundary";
                return false;
            }
            const auto index = ZoneTerrain::index(lx, ly);
            const auto blend = blends[index] = blendAt(boxes, x, y);
            if (blend.construction == 1 || blend.savannah == 1)
                continue; // Exact construction cores and gentler native savannah columns.
            const auto &original = tile->columns()[index];
            const float alpha = ground(original), native = ground(zone.terrain.columns[index]);
            const auto shift = blend.shift(alpha, native);
            auto column = alphagen::shiftedTerrainColumn(original, shift);
            if (!column) {
                error = "Alpha transition exceeds the supported height range";
                return false;
            }
            offsets[index] = int32_t(std::lround(alpha + shift - native));
            biomePalette(*column, zone.terrain.columns[index], int32_t(alpha) + shift, int32_t(native));
            result.terrain.columns[index] = std::move(*column);
        }
    std::map<std::pair<int32_t, int32_t>, std::vector<Box>> surroundingBoxes;
    const auto blendFor = [&](int32_t x, int32_t y) {
        const auto key = std::make_pair(floorDivBy(x, ZONE_SIZE), floorDivBy(y, ZONE_SIZE));
        if (const auto index = setterColumn(zx, zy, x, y))
            return blends[*index];
        auto [entry, fresh] = surroundingBoxes.try_emplace(key);
        if (fresh)
            entry->second = constructionBoxes(world, key.first, key.second);
        return blendAt(entry->second, x, y);
    };
    std::map<std::pair<int32_t, int32_t>, bool> coverDecisions;
    const auto retainCover = [&](int32_t x, int32_t y) {
        auto [entry, fresh] = coverDecisions.try_emplace({x, y}, false);
        if (!fresh)
            return entry->second;
        const auto blend = blendFor(x, y);
        if (blend.construction == 1 || blend.savannah == 1)
            return entry->second = true;
        if (blend.land != 1)
            return false;
        const auto height = source.height(x, y);
        return entry->second = height && *height < 1;
    };
    const auto offsetAt = [&](int64_t fx, int64_t fy) -> int32_t {
        const int64_t bx = fx >> 16, by = fy >> 16;
        if (bx < INT32_MIN || bx > INT32_MAX || by < INT32_MIN || by > INT32_MAX)
            return 0;
        const auto x = int32_t(bx), y = int32_t(by);
        if (const auto index = setterColumn(zx, zy, x, y))
            return offsets[*index];
        const auto blend = blendFor(x, y);
        if (blend.construction == 1 || blend.savannah == 1)
            return 0;
        const auto column = source.column(x, y);
        if (!column)
            return 0;
        const float native = std::floor(groundHeight(x, y)) + 1;
        const float alpha = alphaGround(column->raw());
        return int32_t(std::lround(alpha + blend.shift(alpha, native) - native));
    };
    if (plantModels) {
        const auto replaceCover = [&](const Model &model) {
            return model.groundCover && !retainCover(blocksOf(model.x), blocksOf(model.y));
        };
        std::erase_if(result.models, replaceCover);
        std::erase_if(result.modelLights, replaceCover);
        std::erase_if(result.emitters, replaceCover);
    }
    const auto reanchor = [&](auto &record) { record.z += int64_t(offsetAt(record.x, record.y)) * DOTS; };
    for (auto &record : result.models)
        reanchor(record);
    for (auto &record : result.modelLights)
        reanchor(record);
    for (auto &record : result.emitters)
        reanchor(record);
    for (auto &record : result.objects)
        reanchor(record);
    for (auto &record : result.items)
        reanchor(record);
    for (auto &record : result.lists.objectsCopy)
        reanchor(record);
    for (auto &record : result.lists.itemsCopy)
        reanchor(record);
    for (auto &record : result.lists.lights)
        reanchor(record);
    for (auto &record : result.lists.treasures)
        reanchor(record);
    for (auto &[id, point] : result.spawns.points)
        reanchor(point);
    for (auto &point : result.spawns.shopSpots)
        reanchor(point);
    for (auto &[key, points] : result.spots)
        for (auto &point : points)
            reanchor(point);
    for (auto &[key, points] : result.working)
        for (auto &point : points)
            reanchor(point);
    if (plantModels) {
        for (const auto &plant : source.plantsForTile(zx, zy)) {
            const auto index = setterColumn(zx, zy, plant.x, plant.y);
            if (!index || retainCover(plant.x, plant.y))
                continue;
            const bool floating = plant.kind == 31 || plant.kind == 32;
            if (floating && (plant.z <= 0 ||
                             result.terrain.read(plant.x, plant.y, plant.z - 1).type != blocktype::WATER ||
                             result.terrain.read(plant.x, plant.y, plant.z).type != blocktype::AIR))
                continue; // The composed floor or scenery can cover the former water surface.
            const int32_t shift = result.terrain.columns[*index].sourceShift - tile->columns()[*index].sourceShift;
            const auto model = alphagen::steamPlant(plant, *plantModels, shift);
            if (!model) {
                error = "unmapped Alpha ground-cover model or flags";
                return false;
            }
            result.models.push_back(*model);
        }
    }
    // Refresh copies and working spots now share the same frame as generated objects.
    result.lists.marked.clear();
    markObjectBlocks(
        result.objects, result.lists.marked, [&](int32_t x, int32_t y, int32_t z) { return result.terrain.read(x, y, z); },
        [&](int32_t x, int32_t y, int32_t z, Block b) {
            const auto index = setterColumn(zx, zy, x, y);
            if (!index)
                return;
            const auto &column = result.terrain.columns[*index];
            ColumnFiller filler;
            if (column.fullColour && !column.empty() && int64_t(z) + 1 < column.base)
                filler = [this, x, y, shift = column.sourceShift](int32_t height) -> std::optional<Block> {
                    const auto sourceZ = int64_t(height) - shift;
                    if (sourceZ < INT32_MIN || sourceZ > INT32_MAX)
                        return std::nullopt;
                    return rock(x, y, int32_t(sourceZ));
                };
            result.terrain.store(x, y, z, b, filler ? &filler : nullptr);
        });
    zone = std::move(result);
    error.clear();
    return true;
}
} // namespace worldgen
