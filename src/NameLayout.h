#pragma once
// Selector facts: steam-generated-proper-names.md §2. Offsets are this project's
// legacy names.txt order: generic, then the 16 specialized prefix/suffix pairs.
#include <cstddef>
#include <cstdint>

namespace namefragments {
constexpr std::size_t COUNT = 380;
struct Layout {
    std::size_t prefix, suffix, count;
};
constexpr Layout layout(std::int32_t selector) {
    switch (selector) {
    case 2:
    case 18:
    case 43:
    case 83:
    case 202:
    case 208:
        return {60, 70, 10};
    case 3:
    case 45:
    case 84:
    case 203:
        return {80, 90, 10};
    case 0:
        return {100, 110, 10};
    case 1:
        return {120, 130, 10};
    case 4:
        return {140, 150, 10};
    case 5:
        return {160, 170, 10};
    case 7:
        return {180, 190, 10};
    case 8:
        return {200, 210, 10};
    case 9:
        return {220, 230, 10};
    case 10:
        return {240, 250, 10};
    case 11:
    case 260:
        return {260, 270, 10};
    case 12:
    case 261:
        return {280, 290, 10};
    case 15:
    case 17:
    case 96:
    case 97:
    case 165:
    case 269:
        return {300, 310, 10};
    case 16:
        return {320, 330, 10};
    case 13:
        return {340, 350, 10};
    case 14:
        return {360, 370, 10};
    default:
        return {0, 30, 30};
    }
}
} // namespace namefragments
