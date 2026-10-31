#pragma once
// Map information and pointer text from the independent Preview/map-phase specifications.
#include "ItemInformation.h"
#include "MapMarks.h"
#include "DialogueWords.h"
#include <set>
#include <functional>

namespace mappreview {
using Cell = mapmarks::Cell;
struct FlightContext {
    std::array<int64_t, 3> physical{}, start{};
    const std::set<Cell> *known = nullptr;
    int32_t money = 0;
    int ability = 0;
    float abilityMs = 0;
    Cell destination{};
};
std::string distanceText(int32_t metres);
std::string pointerDistance(const std::array<int64_t, 3> &physical, const std::array<int64_t, 3> &cursor, bool overlayShown);
int32_t flightPrice(Cell target, const FlightContext &);
std::vector<iteminformation::Line> flight(Cell target, bool playerTarget, const FlightContext &);
struct Lore {
    std::string realm;
    int32_t percent = 0;
    std::vector<std::vector<dialoguewords::Word>> pages;
};
Cell lorePlace(Cell cell);
int32_t lorePercent(uint64_t count);
struct PlacedWord {
    dialoguewords::Word word;
    glm::vec2 at{};
};
struct LoreLayout {
    std::vector<PlacedWord> words;
    glm::vec2 end{};
};
LoreLayout layoutLore(const Lore &, float availableWidth, const std::function<float(const std::wstring &)> &measure);
} // namespace mappreview
