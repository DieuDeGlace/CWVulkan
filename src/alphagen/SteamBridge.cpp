#include "alphagen/SteamBridge.h"
#include <algorithm>
#include <limits>
#include "alphagen/ReliefStore.h"
#include "alphagen/LandscapeStore.h"

namespace alphagen {
std::optional<worldgen::Block> steamBlock(Block input) {
    worldgen::Block result{input.r, input.g, input.b, 0, 0, 0};
    switch (input.type()) {
    case alphagen::AIR:
        result.type = worldgen::blocktype::AIR;
        break;
    case alphagen::SOLID:
        result.type = worldgen::blocktype::SOLID;
        break;
    case alphagen::WATER:
        result.type = worldgen::blocktype::WATER;
        break;
    case alphagen::LAKE_BED:
        result.type = worldgen::blocktype::WET_GROUND;
        break;
    case alphagen::GRASS:
        result.type = worldgen::blocktype::GRASS;
        break;
    case alphagen::FIELD:
    case alphagen::SAND:
        result.type = worldgen::blocktype::GROUND;
        break;
    case alphagen::ROCK:
        result.type = worldgen::blocktype::ROCK;
        break;
    case alphagen::WOOD_TYPE:
        result.type = worldgen::blocktype::WOOD;
        break;
    case alphagen::LEAVES_TYPE:
        result.type = worldgen::blocktype::LEAVES;
        result.flags = 1; // Steam leaf material metadata, not the Alpha construction flags.
        break;
    case alphagen::SNOW:
        result.type = worldgen::blocktype::SNOW;
        break;
    case alphagen::TOWN_GROUND:
        result.type = worldgen::blocktype::ROAD;
        break;
    case alphagen::FIRE_GROUND:
        result.type = worldgen::blocktype::LAVA_GROUND;
        break;
    case alphagen::GLOWING:
        result.type = worldgen::blocktype::LIGHT;
        result.glow = 255;
        break;
    default:
        return std::nullopt; // Unspecified material semantics must not be inferred from matching IDs.
    }
    if (input.type() == alphagen::AIR && (input.typeByte & DRY_AIR))
        result.flags = 2; // Steam keep-air flag; Alpha's 0x20/0x80 construction markers are not copied.
    return result;
}

std::optional<worldgen::TerrainColumn> shiftedTerrainColumn(const worldgen::TerrainColumn &source, int32_t shift) {
    using namespace worldgen;
    const int64_t oldTop = int64_t(source.base) + int64_t(source.blocks.size());
    const int64_t newBase = int64_t(source.base) + shift;
    const int64_t newShift = int64_t(source.sourceShift) + shift;
    const int64_t newTop = oldTop + shift;
    if (source.blocks.size() > size_t(INT32_MAX) || oldTop > INT32_MAX || newBase < INT32_MIN ||
        newBase > INT32_MAX || newTop > INT32_MAX || newShift < INT32_MIN || newShift > INT32_MAX)
        return std::nullopt;
    TerrainColumn result = source;
    if (!shift)
        return result;
    result.base = int32_t(newBase);
    result.sourceShift = int32_t(newShift);

    int64_t waterLevel = 0;
    worldgen::Block water = SEA_WATER;
    for (size_t i = 0; i < source.blocks.size(); ++i) {
        if (source.blocks[i].type != blocktype::WATER)
            continue;
        const int64_t z = int64_t(source.base) + int64_t(i);
        if (z > waterLevel) {
            waterLevel = z;
            water = source.blocks[i];
        }
        result.blocks[i] = AIR_BLOCK;
    }

    // Sea water remains implicit at z <= 0. Retain an explicit raised pool at its
    // original level, filling only open cells down to its newly translated floor.
    if (waterLevel > 0 && waterLevel >= newBase) {
        const int64_t required = waterLevel - newBase + 1;
        if (required > INT32_MAX)
            return std::nullopt;
        if (required > int64_t(result.blocks.size()))
            result.blocks.resize(size_t(required), AIR_BLOCK);
        for (int64_t z = waterLevel; z >= std::max<int64_t>(1, newBase); --z) {
            auto &block = result.blocks[size_t(z - newBase)];
            if (!isOpen(block) || (block.type == blocktype::AIR && (block.flags & 2)))
                break;
            const int64_t oldIndex = z - source.base;
            block = oldIndex >= 0 && oldIndex < int64_t(source.blocks.size()) &&
                            source.blocks[size_t(oldIndex)].type == blocktype::WATER
                        ? source.blocks[size_t(oldIndex)]
                        : water;
        }
    }
    // Preserve original stored sea-water colors where the translated solid has
    // left their old world coordinates open; never move those cells above sea.
    for (size_t i = 0; i < source.blocks.size(); ++i) {
        const auto &block = source.blocks[i];
        const int64_t z = int64_t(source.base) + int64_t(i);
        const int64_t index = z - newBase;
        if (block.type == blocktype::WATER && z <= 0 && index >= 0 && index < int64_t(result.blocks.size())) {
            auto &destination = result.blocks[size_t(index)];
            if (isOpen(destination) && !(destination.type == blocktype::AIR && (destination.flags & 2)))
                destination = block;
        }
    }
    while (!result.blocks.empty()) {
        const auto &last = result.blocks.back();
        if (last.type != blocktype::AIR || last.flags || last.glow)
            break;
        result.blocks.pop_back();
    }
    // A nonempty source also owns implicit rock below its base. An all-water
    // stored run still needs one open cell to retain that translated boundary.
    if (result.blocks.empty() && !source.blocks.empty())
        result.blocks.push_back(AIR_BLOCK);
    return result;
}

std::optional<worldgen::Model> steamPlant(const Plant &plant, const PlantModels &models, int32_t shift) {
    if (models.type(plant.kind) < 0 || (plant.flags & ~uint32_t(4)))
        return std::nullopt;
    worldgen::Model result;
    result.type = models.type(plant.kind);
    result.x = int64_t(plant.x) * 65536 + 32768;
    result.y = int64_t(plant.y) * 65536 + 32768;
    const bool floating = plant.kind == 31 || plant.kind == 32;
    result.z = (int64_t(plant.z) + (floating ? 0 : shift)) * 65536;
    if (floating)
        result.z -= 6553;
    result.size = plant.scale;
    result.yaw = plant.rotation;
    result.r = result.g = result.b = 1;
    result.flags = plant.kind == 22 ? 4 : int32_t(2 | plant.flags);
    result.groundCover = true;
    return result;
}
SteamTile::SteamTile(int32_t zx, int32_t zy) {
    terrain.zx = zx;
    terrain.zy = zy;
    terrain.columns.resize(worldgen::ZONE_COLUMNS);
}
bool SteamTile::covers(int32_t x, int32_t y) const {
    const auto index = worldgen::setterColumn(terrain.zx, terrain.zy, x, y);
    return index && present[*index];
}
worldgen::Block SteamTile::read(int32_t x, int32_t y, int32_t z) const {
    return covers(x, y) ? terrain.read(x, y, z) : worldgen::SOLID_GREY;
}

struct SteamTileBuilder {
    template <class Source> static std::optional<SteamTile> build(Source &source, int32_t zx, int32_t zy, std::string &error) {
        error.clear();
        const auto coverage = source.coverage(zx, zy);
        if (!coverage) {
            error = "invalid Alpha coordinate frame or Steam zone range";
            return std::nullopt;
        }
        SteamTile result(zx, zy);
        for (std::size_t i = 0; i < coverage->count; ++i) {
            const auto &patch = coverage->patches[i];
            const auto stage = source.parent(patch.zoneX, patch.zoneY);
            if (!stage) {
                error = "Alpha terrain parent is unavailable";
                return std::nullopt;
            }
            for (int x = 0; x < patch.width; ++x)
                for (int y = 0; y < patch.height; ++y) {
                    const int alphaX = patch.zoneX * alphacoords::ALPHA_ZONE + patch.alphaX + x;
                    const int alphaY = patch.zoneY * alphacoords::ALPHA_ZONE + patch.alphaY + y;
                    const auto &original = stage->zone->column(alphaX, alphaY);
                    const auto index = worldgen::ZoneTerrain::index(patch.sourceX + x, patch.sourceY + y);
                    auto &column = result.terrain.columns[index];
                    column.base = original.bottom;
                    column.fullColour = true;
                    column.blocks.reserve(original.blocks.size());
                    for (const auto block : original.blocks) {
                        const auto converted = steamBlock(block);
                        if (!converted) {
                            error = "unmapped Alpha material " + std::to_string(block.type());
                            return std::nullopt;
                        }
                        column.blocks.push_back(*converted);
                    }
                    result.present[index] = true;
                }
        }
        return result;
    }
};
std::optional<SteamTile> steamTile(SurfaceStore &source, int32_t zx, int32_t zy, std::string &error) {
    return SteamTileBuilder::build(source, zx, zy, error);
}
std::optional<SteamTile> steamTile(ReliefStore &source, int32_t zx, int32_t zy, std::string &error) {
    return SteamTileBuilder::build(source, zx, zy, error);
}
std::optional<SteamTile> steamTile(LandscapeStore &source, int32_t zx, int32_t zy, std::string &error) {
    return SteamTileBuilder::build(source, zx, zy, error);
}
} // namespace alphagen
