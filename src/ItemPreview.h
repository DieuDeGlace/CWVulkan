#pragma once
// Ordinary PreviewWidget source precedence, from steam-voxel-widget-information-preview.md.
#include "NativeSaves.h"
#include <array>
#include <string>

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
// Finite, signed32-convertible tooltip values (information-preview section8.4).
float roundTenth(float value);
std::string formatNumber(float rounded);
struct Equipment {
    std::array<const nativesaves::Item *, 2> weapons{}, rings{};
    std::array<const nativesaves::Item *, 6> other{}; // native candidate order
};
const nativesaves::Item *comparison(const nativesaves::Item *original, const nativesaves::Item &adapted, const Equipment &, bool shift);
} // namespace itempreview
