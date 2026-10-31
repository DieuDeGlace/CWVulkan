#include "ItemPreview.h"
#include "NativeRules.h"
#include "ItemSelector.h"
#include <cstring>
#include <sstream>

namespace itempreview {
RingSource sampleRing(const inventoryslots::Inventory &inventory, bool shown, int32_t runningIndex) {
    RingSource source;
    source.shown = shown;
    if (shown) {
        const auto candidates = itemselector::candidates(inventory);
        if (!candidates.empty())
            source.selected = candidates[itemselector::wrappedIndex(runningIndex, candidates.size())];
    }
    return source;
}
float roundTenth(float value) {
    const bool negative = value < 0;
    const float magnitude = negative ? -value : value;
    const float scaled = magnitude * 10.0f;
    const float biased = scaled + .5f;
    const auto integer = (int32_t)biased;
    const float result = (float)integer * .1f;
    return result == 0 ? 0 : negative ? -result : result;
}
std::string formatNumber(float rounded) {
    std::ostringstream text;
    text << (double)rounded;
    return text.str();
}
const nativesaves::Item *comparison(const nativesaves::Item *original, const nativesaves::Item &adapted, const Equipment &equipment,
                                    bool shift) {
    if (!original)
        return nullptr;
    const nativesaves::Item *result = nullptr;
    const auto matches = [&](const auto *item) { return item && (*item)[0] == adapted[0] && item != original; };
    int32_t subtype;
    std::memcpy(&subtype, adapted.data() + 4, sizeof subtype);
    if (original != equipment.weapons[0] && original != equipment.weapons[1]) {
        if (matches(equipment.weapons[0]) && (subtype != 13 || nativerules::twoHandedWeapon(equipment.weapons[0]->data())))
            result = equipment.weapons[0];
        if (matches(equipment.weapons[1]) && (shift || !result))
            result = equipment.weapons[1];
    }
    for (const auto *item : equipment.other)
        if (matches(item))
            result = item;
    if (original != equipment.rings[0] && original != equipment.rings[1]) {
        if (matches(equipment.rings[0]))
            result = equipment.rings[0];
        if (matches(equipment.rings[1]) && (shift || !result))
            result = equipment.rings[1];
    }
    return result;
}
Selection select(const Sources &source) {
    const Selection ground{source.ground, Anchor::Cursor, source.ground != nullptr};
    if (source.ringShown)
        return source.ring ? Selection{source.ring, Anchor::Cursor, source.ground != nullptr} : ground;
    if (source.map && (*source.map)[0])
        return {source.map, Anchor::Cursor};
    if (source.inventory)
        return {source.inventory, Anchor::Inventory};
    if (source.equipment && (*source.equipment)[0])
        return {source.equipment, Anchor::Inventory};
    if (source.crafting && (*source.crafting)[0])
        return {source.crafting, Anchor::Crafting};
    if (source.shop)
        return {source.shop, Anchor::Shop};
    if (source.formulaShown) {
        if (source.formula && (*source.formula)[0])
            return {source.formula, Anchor::Crafting};
    } else if (source.voxel)
        return {source.voxel, Anchor::Crafting};
    return ground;
}
} // namespace itempreview
