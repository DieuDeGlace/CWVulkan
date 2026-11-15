#include "HudAuras.h"

#include "worldgen/Noise.h"

#include <cstring>
#include <utility>

namespace hudauras {

const char *icon(int type) {
    // The interface loader's aura table, by buff type.
    static const std::pair<int, const char *> ICONS[] = {
        {1, "aura:bulwark"},         {2, "aura:warfrenzy"},        {3, "aura:camouflage"},
        {4, "aura:poisoned"},        {6, "aura:manashield"},       {9, "aura:instantfire"},
        {10, "aura:instantshoot"},   {11, "aura:dodge"},           {12, "aura:swiftness"},
        {13, "aura:ninjutsu"},       {15, "aura:shadowclone"},     {16, "aura:flamerush"},
        {17, "aura:regeneration"},   {19, "aura:spiritworld"},     {22, "aura:lifeflask"},
        {23, "aura:attackflask"},    {24, "aura:armorflask"},      {25, "aura:resistanceflask"},
        {26, "aura:heatresistance"}, {27, "aura:coldresistance"},  {28, "aura:plagueresistance"},
        {29, "aura:frozen"},         {0x21, "aura:ancientpower"},  {0x22, "aura:manaabsorption"},
    };
    for (const auto &[t, name] : ICONS)
        if (t == type)
            return name;
    return nullptr;
}

std::wstring text(int type, float value, float remainingMs) {
    if (type == 0x21 || type == 0x22)
        return L"x " + std::to_wstring(worldgen::trunc32(value));
    // Each bias is half its unit, added before truncating, so whole units round to nearest.
    if (remainingMs > 3600000.0f)
        return std::to_wstring(worldgen::trunc32(remainingMs + 1800000.0f) / 3600000) + L" h";
    if (remainingMs > 60000.0f)
        return std::to_wstring(worldgen::trunc32(remainingMs + 30000.0f) / 60000) + L" m";
    return std::to_wstring(worldgen::trunc32(remainingMs + 500.0f) / 1000) + L" s";
}

std::vector<Entry> strip(const std::vector<Buff> &buffs, const std::function<bool(const char *)> &available) {
    std::vector<Entry> out;
    for (const Buff &b : buffs) {
        const char *name = icon(b[0]);
        if (!name || !available(name))
            continue;
        float value, remaining;
        std::memcpy(&value, &b[4], 4);
        std::memcpy(&remaining, &b[8], 4);
        out.push_back({name, text(b[0], value, remaining), FIRST_X + PITCH * (float)out.size(), Y});
    }
    return out;
}

} // namespace hudauras
