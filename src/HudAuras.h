#pragma once
// The HUD aura strip, top left under the money (steam-hud-poi-count-widget-20261115.md and
// steam-hud-aura-strip-constants-20261115.md): one copy of the aura's gui.plx template per
// buff of the local player that has one, rebuilt every HUD update, in buff-list order.
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace hudauras {

using Buff = std::array<uint8_t, 0x18>; // type +0, value +4, remaining ms +8

// The gui.plx template of a buff type ("aura:<name>"), or null for a type with no icon.
const char *icon(int type);

// The text under the icon: "x " and the value truncated for types 0x21 and 0x22 (the
// ancient power and mana absorption counts); otherwise the remaining time rounded to the
// nearest hour above an hour, minute above a minute, or else second ("2 h", "15 m", "30 s").
std::wstring text(int type, float value, float remainingMs);

// The placement under the interface root: anchor (0, 0), x 45 then every 50, y 120.
constexpr float FIRST_X = 45.0f, PITCH = 50.0f, Y = 120.0f;
// The copy's deformation (its first two rows) is multiplied by 3: three times the size.
constexpr float SCALE = 3.0f;

struct Entry {
    const char *icon;
    std::wstring text;
    float x, y;
};

// The entries in order. A buff whose template is missing (no icon, or `available` says the
// interface has none) is skipped and takes no slot.
std::vector<Entry> strip(const std::vector<Buff> &buffs, const std::function<bool(const char *)> &available);

} // namespace hudauras
