#include "MapHistory.h"
#include <algorithm>
#include <iterator>

namespace maphistory {
History::History(const std::vector<Region> &saved) {
    for (const auto &region : saved)
        entries.push_back(std::make_shared<Entry>(Entry{region}));
}

bool History::observe(Region current, uint8_t ability) {
    if (ability == 0x9c || ability == 0x9d || (!entries.empty() && entries.front()->region == current))
        return false;
    const auto existing = std::find_if(entries.begin(), entries.end(), [&](const auto &entry) { return entry->region == current; });
    if (existing != entries.end())
        entries.erase(existing);
    entries.push_front(std::make_shared<Entry>(Entry{current}));
    return true;
}

std::vector<Region> History::snapshot() const {
    std::vector<Region> result;
    result.reserve(entries.size());
    for (const auto &entry : entries)
        result.push_back(entry->region);
    return result;
}

History::Entries::iterator History::selectedEntry() {
    const auto selected = selection.lock();
    return std::find(entries.begin(), entries.end(), selected);
}

void History::refresh(Entries::iterator selected) {
    if (selected == entries.end())
        return;
    display.caption = (*selected)->region;
    display.older = std::next(selected) != entries.end();
    display.newer = selected != entries.begin();
}

void History::show(bool visible) {
    if (visible && !display.shown) {
        if (entries.empty()) {
            selection.reset();
            display.older = display.newer = false;
        } else {
            selection = entries.front();
            refresh(entries.begin());
        }
    }
    display.shown = visible;
}

std::optional<Region> History::step(bool towardOlder) {
    auto selected = selectedEntry();
    // Native producer deletion can invalidate its borrowed node. Keep stale presentation,
    // but never dereference freed storage or invent a replacement destination.
    if (selected == entries.end())
        return std::nullopt;
    if (towardOlder) {
        if (std::next(selected) != entries.end())
            ++selected;
    } else if (selected != entries.begin())
        --selected;
    selection = *selected;
    refresh(selected);
    return (*selected)->region;
}
std::optional<Region> History::older() {
    return step(true);
}
std::optional<Region> History::newer() {
    return step(false);
}
std::optional<Region> History::selected() const {
    if (const auto selected = selection.lock())
        return selected->region;
    return std::nullopt;
}

bool hovered(bool fullMap, float x, float y, float height, float guiScale) {
    return fullMap && x < 300.f * guiScale && y > height - 90.f * guiScale;
}
} // namespace maphistory
