#include "ItemNames.h"
#include "NativeControl.h"

#include <cstring>
#include <map>
#include <set>
#include <sstream>

namespace itemnames {

namespace {
#include "ItemKeys.inc"
#include "RaceKeys.inc"

int32_t at32(const uint8_t *p, size_t o) {
    int32_t v;
    std::memcpy(&v, p + o, 4);
    return v;
}

std::set<std::string> tagsOf(const Speech &speech, const std::string &key) {
    std::set<std::string> tags;
    std::istringstream in(speech.nameTags(key));
    std::string t;
    while (in >> t)
        tags.insert(t);
    return tags;
}
} // namespace

std::string itemKey(int category, int id) {
    static const std::map<std::pair<int, int>, const char *> keys = [] {
        std::map<std::pair<int, int>, const char *> m;
        for (const auto &k : ITEM_KEYS)
            m[{k.category, k.id}] = k.key;
        return m;
    }();
    if (auto it = keys.find({category, id}); it != keys.end())
        return it->second;
    return category == 21 ? "Bait" : ""; // (pet food the game names nothing more particular)
}

std::string materialKey(int material) {
    for (const auto &m : MATERIAL_KEYS)
        if (m.material == material)
            return m.key;
    return "";
}

std::string name(const Speech &speech, const GameNames &names, const uint8_t *item) {
    const int category = item[0], id = at32(item, 4), material = item[0x1c], rarity = (int8_t)item[0x14];
    const int32_t modifier = at32(item, 8), formulaCategory = at32(item, 0x18);
    const int nameRace = (int)((uint32_t)modifier * 7u % 11u);
    if (category == 2) { // a formula: "Formula: " and what it makes
        uint8_t product[0xa0];
        std::memcpy(product, item, sizeof product);
        product[0] = (uint8_t)formulaCategory;
        std::memset(product + 0x18, 0, 4);
        return "Formula: " + name(speech, names, product);
    }
    if (category == 29) { // a glyph: of the skill its id names
        const char *skill = nativecontrol::skillKey(id);
        return std::string("Glyph of ") + (skill ? speech.text(skill, {"default"}, {}) : std::string());
    }
    if (category == 20) { // a pet: "Pet: " and its race's key as it is
        for (const auto &r : RACE_KEYS)
            if (r.first == id)
                return std::string("Pet: ") + r.second;
        return "Pet: ";
    }
    const std::string key = itemKey(category, id);
    if (category == 22 || category == 23) // lore items: the key's text, @ a generated name
        return speech.text(key, {}, {{"", names.name(modifier, nameRace)}});
    if (category == 1 || category == 15 || category == 19 || material == 0)
        return speech.text(key, {"default"}, {});
    std::map<std::string, std::string> macros = {{"name", names.name(modifier, nameRace)}, {"item", key}};
    if ((category >= 3 && category <= 9) || category == 26) {
        if (speech.hasEntry(key) && rarity >= 0 && rarity <= 4) {
            macros["materialitem"] = materialKey(material);
            const char *quality = QUALITY_KEYS[rarity][(uint32_t)modifier % 10u];
            if (*quality)
                return speech.text(quality, tagsOf(speech, key), macros);
        }
    }
    if (category == 24) // specials: named from the formula category
        return speech.text(key, {}, {{"name", names.name(formulaCategory, nameRace)}});
    if (speech.hasEntry(key)) {
        std::set<std::string> tags = tagsOf(speech, key);
        tags.insert("item");
        return speech.text(materialKey(material), tags, macros);
    }
    return "unknown";
}

} // namespace itemnames
