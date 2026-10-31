#pragma once
// Ordinary PreviewWidget source precedence, from steam-voxel-widget-information-preview.md.
#include "NativeSaves.h"

namespace itempreview {
enum class Anchor { Cursor, Inventory, Shop, Crafting };
struct Sources {
    const nativesaves::Item *map = nullptr, *inventory = nullptr, *equipment = nullptr;
    const nativesaves::Item *crafting = nullptr, *shop = nullptr, *formula = nullptr, *voxel = nullptr;
    const nativesaves::Item *ring = nullptr, *ground = nullptr;
    bool ringShown = false, formulaShown = false;
};
struct Selection {
    const nativesaves::Item *item = nullptr; // borrowed original, not an adapted copy
    Anchor anchor = Anchor::Cursor;
    bool groundOrigin = false; // Ring may retain this even when its selected item differs.
};
Selection select(const Sources &);
} // namespace itempreview
