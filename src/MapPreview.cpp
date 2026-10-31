#include "MapPreview.h"
#include "NativeAbility.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <utility>
#include <string_view>

namespace mappreview {
namespace {
constexpr double BLOCK = 1.0 / 65536.0;
int32_t difference(int32_t a, int32_t b) {
    return std::bit_cast<int32_t>(uint32_t(a) - uint32_t(b));
}
int32_t block(int64_t fixed) {
    return std::bit_cast<int32_t>(uint32_t(fixed / 65536));
}
int32_t duration(const std::array<int64_t, 3> &from, Cell target) {
    return nativeability::flightTime(block(from[0]), block(from[1]), target[0], target[1]);
}
} // namespace
std::string distanceText(int32_t metres) {
    if (metres < 10)
        return std::to_string(metres) + " m";
    if (metres < 500)
        return std::to_string(metres / 10 * 10) + " m";
    if (metres < 1000)
        return std::to_string(metres / 100 * 100) + " m";
    const int32_t km = metres / 1000, tenth = metres / 100 - km * 10;
    return std::to_string(km) + (metres < 10000 && tenth ? "." + std::to_string(tenth) : "") + " km";
}
std::string pointerDistance(const std::array<int64_t, 3> &physical, const std::array<int64_t, 3> &cursor, bool overlayShown) {
    if (!overlayShown || cursor[2] < 0)
        return {};
    const float x = float(physical[0] - cursor[0]) * float(BLOCK);
    const float y = float(physical[1] - cursor[1]) * float(BLOCK);
    const float norm = std::sqrt(x * x + y * y);
    return distanceText(int32_t(double(norm) * .5));
}
int32_t flightPrice(Cell target, const FlightContext &context) {
    const double x = double(context.physical[0]) * BLOCK - double(target[0]);
    const double y = double(context.physical[1]) * BLOCK - double(target[1]);
    double best = std::sqrt(x * x + y * y);
    if (context.known)
        for (Cell known : *context.known) {
            const double dx = difference(known[0], target[0]), dy = difference(known[1], target[1]);
            const double leg = std::sqrt(dx * dx + dy * dy) * .5;
            if (leg < best)
                best = leg;
        }
    const int32_t price = std::max(100, int32_t(std::max(0.0, best)) / 100);
    return int32_t(float(price));
}
std::vector<iteminformation::Line> flight(Cell target, bool playerTarget, const FlightContext &context) {
    const double x = double(int64_t(target[0]) * 65536 - context.physical[0]) * BLOCK;
    const double y = double(int64_t(target[1]) * 65536 - context.physical[1]) * BLOCK;
    const int32_t metres = int32_t(std::sqrt(x * x + y * y) * .5);
    std::vector<iteminformation::Line> lines;
    auto add = [&](std::string text, glm::vec4 color, float advance = 30) {
        lines.push_back({std::move(text), 12, 3, color, advance, true});
    };
    add("Flight point", {.5f, 1, 1, 1});
    add("Distance: " + distanceText(metres), glm::vec4(1));
    if ((!context.known || !context.known->contains(target)) && !playerTarget) {
        const auto price = flightPrice(target, context);
        const bool enough = context.money >= price;
        add(std::string(enough ? "Purchase for: " : "Cost: ") + std::to_string(price) + " gold",
            enough ? glm::vec4(1, .9f, 0, 1) : glm::vec4(1, 0, 0, 1));
    } else {
        int32_t ms = duration(context.physical, target);
        if (context.ability == 0x9c && context.destination == target)
            ms = int32_t(float(duration(context.start, target)) - context.abilityMs);
        add("Flight time: " + std::to_string(ms / 1000) + " seconds", glm::vec4(1), 60);
    }
    return lines;
}
Cell lorePlace(Cell cell) {
    for (auto &coordinate : cell) {
        const int32_t block = std::bit_cast<int32_t>(uint32_t(coordinate) << 4);
        coordinate = block / 1024 - (block % 1024 < 0);
    }
    return cell;
}
int32_t lorePercent(uint64_t count) {
    return std::bit_cast<int32_t>(uint32_t(count) * 10u);
}
LoreLayout layoutLore(const Lore &content, float availableWidth, const std::function<float(const std::wstring &)> &measure) {
    LoreLayout layout;
    float y = 73;
    for (const auto &page : content.pages) {
        float x = 14;
        bool first = true, afterHyphen = false;
        for (const auto &word : page) {
            const bool punctuation = word.text.size() == 1 && std::wstring_view(L".-,;!?:/)").find(word.text[0]) != std::wstring_view::npos;
            const float width = float(int32_t(measure ? measure(word.text) : 0));
            if (!first && !afterHyphen)
                x += punctuation ? 3.f : 12.f;
            if (!punctuation && x + width > 14 + availableWidth) {
                y += 2;
                y += 12;
                x = 14;
            }
            layout.words.push_back({word, {x, y}});
            x += width;
            if (word.text == L"/" || word.text == L"(")
                x += -9;
            first = false;
            afterHyphen = word.text == L"-";
        }
        y += (2.f + 12.f) * 1.5f;
    }
    layout.end = {14, float(int32_t(y))};
    return layout;
}
} // namespace mappreview
