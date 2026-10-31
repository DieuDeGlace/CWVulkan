#pragma once
// Ordered personal map history and borrowed selection (steam-map-region-history sections2–5).
#include <array>
#include <cstdint>
#include <list>
#include <memory>
#include <optional>
#include <vector>

namespace maphistory {
using Region = std::array<int32_t, 2>;
class History {
  public:
    explicit History(const std::vector<Region> &saved = {});
    History(const History &) = delete;
    History &operator=(const History &) = delete;
    History(History &&) = default;
    History &operator=(History &&) = default;
    bool observe(Region current, uint8_t ability);
    std::vector<Region> snapshot() const;
    void show(bool visible);
    // Boundary steps still refresh and return the selected region for recentering.
    std::optional<Region> older();
    std::optional<Region> newer();
    std::optional<Region> selected() const;
    struct View {
        bool shown = false, older = false, newer = false;
        std::optional<Region> caption;
    };
    const View &view() const { return display; }

  private:
    struct Entry {
        Region region;
    };
    using Entries = std::list<std::shared_ptr<Entry>>;
    Entries entries;
    std::weak_ptr<Entry> selection;
    View display;
    Entries::iterator selectedEntry();
    void refresh(Entries::iterator);
    std::optional<Region> step(bool towardOlder);
};
// The interface's strict pointer rectangle is independent of ancestor visibility and arrow hits.
bool hovered(bool fullMap, float x, float y, float height, float guiScale);
} // namespace maphistory
