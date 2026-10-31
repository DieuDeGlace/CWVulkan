#include "ItemSelector.h"
#include "GameMath.h"
#include "GuiItem.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>

namespace itemselector {
std::vector<inventoryslots::Index> candidates(const inventoryslots::Inventory &inventory) {
    std::vector<inventoryslots::Index> result;
    for (size_t tab = 0; tab < inventory.tabs.size(); ++tab)
        for (size_t slot = 0; slot < inventory.tabs[tab].size(); ++slot) {
            const auto &stack = inventory.tabs[tab][slot];
            if (stack.count != 0 && stack.item[0] == 1)
                result.push_back({(int32_t)tab, (int32_t)slot});
        }
    return result;
}

size_t wrappedIndex(int32_t index, size_t count) {
    if (!count)
        return 0;
    const int64_t remainder = (int64_t)index % (int64_t)count;
    return (size_t)(remainder < 0 ? remainder + (int64_t)count : remainder);
}

glm::vec4 candidateColour(const nativesaves::Item &item) {
    return guiitem::textColour(item, {}); // category1 ignores region; subtype13 retains its fixed cyan
}

Update Selector::frame(const Input &input, size_t count, double elapsed, float scale) {
    elapsed = std::min(elapsed, 1000.0);
    if (input.allowToggle) {
        if (input.toggle && !previousToggle)
            shown = !shown;
        previousToggle = input.toggle;
    }
    if (input.forward || input.back || input.stickY > 0.25f || input.stickY < -0.25f || input.fullMap)
        shown = false;
    Update result;
    if (!shown) {
        neutralSeen = false;
        angle = radius = 0;
        return result;
    }
    if (!count) {
        angle = radius = 0;
        return result;
    }
    result.captionIndex = wrappedIndex(index, count);
    const bool left = input.left || input.stickX < 0;
    const bool right = input.right || input.stickX > 0;
    if (!left)
        leftRepeat = 0;
    if (!right)
        rightRepeat = 0;
    if (!left && !right)
        neutralSeen = true;
    if (neutralSeen) {
        auto select = [&](bool held, int32_t &repeat, int32_t direction) {
            if (held && repeat <= 0) {
                index = std::bit_cast<int32_t>((uint32_t)index + (uint32_t)direction);
                repeat = 200;
                ++result.selectionSounds;
            }
        };
        select(left, leftRepeat, -1);
        select(right, rightRepeat, 1);
        auto tick = [&](int32_t &repeat) { repeat = std::max(0, (int32_t)((double)repeat - elapsed)); };
        tick(leftRepeat);
        tick(rightRepeat);
    }
    const float target = ((float)index * -360.0f) / (float)count;
    const float blend = gamemath::blendFactor(elapsed, 0.01f);
    if (radius <= 0 || std::abs(angle - target) >= 270.0f)
        angle = target;
    else
        angle = (target - angle) * blend + angle;
    radius = (scale * 200.0f - radius) * blend + radius;
    return result;
}
int SoundEdges::sample(bool ordinaryUi, bool selectorShown) {
    const bool current = ordinaryUi || selectorShown;
    const int sound = current == previous ? 0 : current ? 0x56 : 0x57;
    previous = current;
    return sound;
}
} // namespace itemselector
