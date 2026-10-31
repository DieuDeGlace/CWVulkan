#include "ItemPreview.h"

namespace itempreview {
Selection select(const Sources &source) {
    const Selection ground{source.ground, Anchor::Cursor, source.ground != nullptr};
    if (source.ringShown)
        return source.ring ? Selection{source.ring, Anchor::Cursor, false} : ground;
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
