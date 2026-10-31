#pragma once
// Item-detail text decisions, from steam-voxel-widget-information-preview sections7/8.
#include "NativeSaves.h"
#include <glm/glm.hpp>
#include <array>
#include <functional>
#include <string>
#include <vector>

namespace iteminformation {
struct Line {
    std::string text;
    float size = 12, stroke = 3;
    glm::vec4 color{1};
    float advance = 16;
    bool fromEnd = false, over = false;
    bool suffix = false; // Start at the preceding text's returned pen X, retaining its original Y.
    float before = 0;
};
struct Context {
    const uint8_t *player = nullptr;
    int classType = 0;
    std::array<int32_t, 2> region{};
    std::function<std::string(const nativesaves::Item &)> fullName, regionName;
    std::function<std::string(uint32_t)> realmName;
    const nativesaves::Item *comparison = nullptr; // original equipped record
    float scale = 1;
    bool classWarning = true, reserved = false, reservedForLocal = false;
};
std::vector<Line> format(const nativesaves::Item &, const Context &);
bool ridable(int32_t race);
} // namespace iteminformation
