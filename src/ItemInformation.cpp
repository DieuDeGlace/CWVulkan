#include "ItemInformation.h"
#include "ItemPreview.h"
#include "NativeRules.h"
#include <algorithm>
#include <bit>
#include <cstring>
#include <iterator>
#include <utility>

namespace iteminformation {
namespace {
template <class T> T field(const nativesaves::Item &item, size_t offset) {
    T value;
    std::memcpy(&value, item.data() + offset, sizeof value);
    return value;
}
glm::vec4 rarityColor(int rarity) {
    static const glm::vec4 colors[] = {{.7f, .7f, .7f, 1}, {1, 1, 1, 1}, {0, 1, 0, 1}, {.3f, .45f, 1, 1}, {.55f, .2f, 1, 1}, {1, 1, 0, 1}};
    return colors[std::clamp(rarity, -1, 4) + 1];
}
} // namespace
bool ridable(int32_t race) {
    static constexpr std::array ranges = {
        std::array{0x13, 0x14}, std::array{0x16, 0x17},   std::array{0x19, 0x28},   std::array{0x3f, 0x43},  std::array{0x4a, 0x4b},
        std::array{0x62, 0x64}, std::array{0x66, 0x69},   std::array{0x97, 0x97},   std::array{0x9e, 0x9e},  std::array{0xa3, 0xa4},
        std::array{0xa6, 0xa6}, std::array{0xad, 0xad},   std::array{0xc8, 0xc9},   std::array{0xd1, 0xd3},  std::array{0xd5, 0xd5},
        std::array{0xd7, 0xd7}, std::array{0x106, 0x107}, std::array{0x10e, 0x10e}, std::array{0x125, 0x127}};
    return std::any_of(ranges.begin(), ranges.end(), [race](auto range) { return race >= range[0] && race <= range[1]; });
}
std::vector<Line> format(const nativesaves::Item &item, const Context &context) {
    std::vector<Line> lines;
    if (!context.player)
        return lines;
    const float scale = context.scale;
    auto add = [&](std::string text, float size, float stroke, glm::vec4 color, float advance, bool fromEnd = false,
                   bool over = false) -> Line & {
        lines.push_back({std::move(text), size * scale, stroke, color, advance * scale, fromEnd, over});
        return lines.back();
    };
    auto number = [](float value) { return itempreview::formatNumber(itempreview::roundTenth(value)); };
    const int category = item[0], rarity = item[0x14];
    const auto subtype = field<int32_t>(item, 4);
    const auto seed = field<uint32_t>(item, 8);
    const int effective = nativerules::itemLevel(item.data(), context.region.data());
    const bool equipment = (category >= 3 && category <= 9) || category == 26;
    const bool special = equipment && seed % ((uint32_t)(rarity + 1) * 10) == 0;
    add((context.fullName ? context.fullName(item) : std::string()) + (special ? "+" : ""), 15, 4, rarityColor(effective), 24, true);
    if (category == 23)
        add(context.realmName ? context.realmName(field<uint32_t>(item, 0x18)) : std::string(), 12, 3, {1, .9f, .5f, 1}, 24);
    if (category < 31 && ((0x454083fcu >> category) & 1)) {
        if (equipment) {
            add(std::string((size_t)rarity + 1, (char)0xb1), 18, 3, {.3f, .3f, .3f, 1}, 24);
            add(std::string((size_t)std::max(0, effective + 1), (char)0xb1), 18, 3, rarityColor(effective), 0, false, true);
        }
        add((context.regionName ? context.regionName(item) : std::string()) + (special ? "+" : ""), 12, 3,
            effective == rarity ? glm::vec4(1, .9f, .5f, 1) : glm::vec4(.75f, .75f, .75f, 1), 24);
    }
    if (category == 23) {
        static const char *effects[] = {"You can climb faster.",
                                        "Increases swimming speed.",
                                        "You consume less stamina when diving.",
                                        "Increases riding speed.",
                                        "Increases hang gliding speed.",
                                        "Increases sailing speed.",
                                        "Increases the radius of your lamp."};
        add(effects[seed % 7], 12, 3, {.2f, 1, .6f, 1}, 0);
    }
    if (category == 24 && subtype == 4)
        add("RANK " + std::to_string(std::bit_cast<int32_t>(seed + 1u)), 12, 3, {1, .9f, .6f, 1}, 0);
    if (context.reserved)
        add("", 9, 2, context.reservedForLocal ? glm::vec4(1) : glm::vec4(1, 0, 0, 1), 20);
    if (context.classWarning && !nativerules::classMayUse(item.data(), context.classType)) {
        std::string classes;
        static const char *names[] = {"Warrior ", "Ranger ", "Mage ", "Rogue "};
        for (int cls = 1; cls <= 4; ++cls)
            if (nativerules::classMayUse(item.data(), cls))
                classes += names[cls - 1];
        add(classes, 12, 3, {1, 0, 0, 1}, 20);
    }
    if (category == 20 && ridable(subtype))
        add("Ridable", 12, 3, {.75f, .5f, 1, 1}, 14);
    using Stat = float (*)(const uint8_t *, const uint8_t *);
    struct Row {
        const char *prefix;
        Stat value;
        float threshold, font, advance;
        bool percent;
    };
    static const Row rows[] = {{"DAMAGE ", nativerules::weaponDamage, 0, 15, 20, false},
                               {"ARMOR ", nativerules::itemArmor, .1f, 15, 20, false},
                               {"RESISTANCE ", nativerules::itemResistance, .1f, 15, 20, false},
                               {"HP ", nativerules::itemHealth, .1f, 15, 16, false},
                               {"HASTE ", nativerules::itemHaste, .001f, 12, 16, true},
                               {"CRITICAL ", nativerules::itemCritical, .001f, 12, 16, true},
                               {"REGENERATION ", nativerules::itemStaminaRegen, .001f, 12, 16, true}};
    for (size_t i = 0; i < std::size(rows); ++i) {
        const auto &row = rows[i];
        const float own = row.value(item.data(), context.player);
        const float other = context.comparison ? row.value(context.comparison->data(), context.player) : 0;
        if (i == 0 ? category != 3 : !(own >= row.threshold || other >= row.threshold))
            continue;
        const float shown = itempreview::roundTenth(row.percent ? own * 100 : own);
        add(row.prefix + itempreview::formatNumber(shown) + (row.percent ? "%" : ""), row.font, 3,
            i < 3 ? glm::vec4(0, 1, 1, 1) : glm::vec4(.75f, .5f, 1, 1), row.advance);
        if (context.comparison) {
            const float compared = itempreview::roundTenth(row.percent ? other * 100 : other);
            const float difference = itempreview::roundTenth(shown - compared);
            if (difference != 0)
                add((difference > 0 ? " +" : " ") + itempreview::formatNumber(difference), 12, 3,
                    difference > 0 ? glm::vec4(.3f, 1, .3f, 1) : glm::vec4(1, .3f, .3f, 1), 0, false, true)
                    .suffix = true;
        }
        if (i == 0) {
            const float bonus = itempreview::roundTenth(own - nativerules::baseWeaponDamage(item.data(), context.player));
            if (bonus > 0)
                add("+DAMAGE " + itempreview::formatNumber(bonus), 12, 3, {1, .2f, 1, 1}, 20).before = -(scale + scale);
        }
    }
    if (category == 1 && nativerules::healAmount(item.data()) >= .1f)
        add("Restores health over " + std::to_string(nativerules::useTime(item.data()) / 1000) + " seconds", 12, 3, {.25f, 1, .25f, 1}, 16);
    if (const float demon = nativerules::demonDamage(item.data()); demon > 0)
        add("Increases damage against demons by " + number(demon * 100) + "%", 12, 3, {1, .75f, .5f, 1}, 16);
    if (category == 28) {
        static const char *effects[] = {"HP +20%",
                                        "DAMAGE +20%",
                                        "ARMOR +20%",
                                        "RESISTANCE +20%",
                                        "A refreshing drink that helps against extreme heat.",
                                        "A warming drink that helps against extreme coldness.",
                                        "A very healthy drink that helps against toxic environments."};
        add(subtype >= 0 && subtype < 7 ? effects[subtype] : "", 12, 3, {.6f, .5f, 1, 1}, 20, true);
        add("Effect lasts 10 Minutes", 12, 3, {0, 1, 1, 1}, 16);
    }
    return lines;
}
} // namespace iteminformation
