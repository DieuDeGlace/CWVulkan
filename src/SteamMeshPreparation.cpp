#include "SteamSceneRange.h"

namespace scenerange {
PreparationBudget::PreparationBudget(int32_t radius) {
    // Original worker contract: steam-world-zone-window-demand.md §5.
    float budget = float(int64_t(radius) * radius * 98304);
    budget *= .8f;
    budget *= .8f;
    limit = int64_t(budget);
}
bool PreparationBudget::consider(int32_t cost) {
    if (cost == 0 && used > limit)
        return false;
    used += cost;
    return true;
}
} // namespace scenerange
