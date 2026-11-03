#include "alphagen/SteamBridge.h"

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
        return std::nullopt; // Includes Alpha voxel-tree materials, which need their own contract.
    }
    if (input.type() == alphagen::AIR && (input.typeByte & DRY_AIR))
        result.flags = 2; // Steam keep-air flag; Alpha's 0x20/0x80 construction markers are not copied.
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

std::optional<SteamTile> steamTile(SurfaceStore &source, int32_t zx, int32_t zy, std::string &error) {
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
            error = "Alpha surface parent is unavailable";
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
} // namespace alphagen
