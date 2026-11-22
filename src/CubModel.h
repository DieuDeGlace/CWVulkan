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
    static bool isAttachmentMarker(uint8_t r, uint8_t g, uint8_t b) {
        return (r == 255 && g == 0 && b == 0) || (r == 0 && g == 255 && b == 0) || (r == 0 && g == 0 && b == 255);
    }
    bool attachmentMarker(int x, int y, int z) const;
    VoxelColor color(int x, int y, int z) const;
    // Mesh in voxel units, centred on x/y with its base at z = 0. Alpha corners fade toward
    // shade as they close in. The Steam CUB bake uses shade as its additive RGB.
    Mesh mesh(uint8_t flags = 0, glm::u8vec3 shade = glm::u8vec3(0), steamvoxel::CubEncoding = steamvoxel::CubEncoding::Stock,
              bool hideAttachmentMarkers = false) const;
};
