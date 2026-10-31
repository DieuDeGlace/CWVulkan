#pragma once
// Cube World .cub sprite: int32 size x, y, z, then x*y*z RGB triples with x
// varying fastest, then y, then z. Black (0,0,0) is empty.
#include "Mesher.h"
#include "SteamVoxel.h"
#include <glm/gtc/type_precision.hpp>
#include <filesystem>
#include <string>

struct CubModel {
    int sx = 0, sy = 0, sz = 0;
    std::vector<uint8_t> rgb; // 3 bytes per voxel
    bool load(const std::filesystem::path &file, std::string *error = nullptr);
    bool loadMemory(const uint8_t *data, size_t size, std::string *error = nullptr);
    bool solid(int x, int y, int z) const;
    bool exposed(int x, int y, int z) const;
    VoxelColor color(int x, int y, int z) const;
    // Mesh in voxel units, centred on x/y with its base at z = 0. In the Alpha look each corner
    // fades towards `shade`, the model's shade colour, as it is closed in (black for all but
    // the clouds).
    Mesh mesh(uint8_t flags = 0, glm::u8vec3 shade = glm::u8vec3(0), steamvoxel::CubEncoding = steamvoxel::CubEncoding::Stock) const;
};
