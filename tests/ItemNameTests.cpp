#include "ItemNames.h"
#include "NativeItems.h"
#include <array>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>

int itemNameTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *label) {
        if (!passed) {
            std::cout << "FAIL item names: " << label << '\n';
            ++failures;
        }
    };
    using Record = std::array<uint8_t, 0xa0>;
    const auto setWord = [](Record &record, std::size_t offset, uint32_t word) {
        std::memcpy(record.data() + offset, &word, sizeof word);
    };
    constexpr std::array<std::pair<uint32_t, uint8_t>, 4> mappings{{{63, 3}, {64, 4}, {65, 8}, {66, 9}}};
    for (const auto [subtype, category] : mappings) {
        Record original;
        for (std::size_t i = 0; i < original.size(); ++i)
            original[i] = uint8_t(i * 37 + 11);
        original[0] = 22;
        original[0x9d] = 32;
        setWord(original, 4, subtype);
        setWord(original, 0x18, 0xfedcba98);
        auto expected = original;
        expected[0] = category;
        setWord(expected, 4, subtype == 63 ? 0xfedcba98 : 0);
        check(nativeitems::normalisedRecord(original.data()) == expected,
              "stand-in copy changes only category/subtype and retains wrapped type, material, all32 attributes and "
              "padding");
    }
    for (const auto category : {3, 22, 23}) {
        Record original{};
        original[0] = uint8_t(category);
        setWord(original, 4, 62);
        check(nativeitems::normalisedRecord(original.data()) == original,
              "ordinary records are not rewritten as stand-ins");
    }
    const auto dictionary = std::filesystem::temp_directory_path() /
                            ("CubeVulkan-item-names-" +
                             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".xml");
    constexpr std::array<const char *, 4> words{{"blade", "coat", "pendant", "band"}};
    {
        std::ofstream stream(dictionary);
        stream << "<dictionary>";
        for (std::size_t i = 0; i < mappings.size(); ++i)
            stream << "<speech id=\"" << itemnames::itemKey(mappings[i].second, 0) << "\">" << words[i] << "</speech>";
        stream << "<speech id=\"Material\">generic @item</speech>"
                  "<speech id=\"MaterialFire\">ember @item</speech>"
                  "<speech id=\"MaterialUnholy\">shade @item</speech>"
                  "<speech id=\"MaterialIce\">frost @item</speech>"
                  "<speech id=\"MaterialWind\">gust @item</speech>"
                  "<speech id=\"item:used\">quality @materialitem @item</speech></dictionary>";
        check(bool(stream), "write an independent dictionary fixture");
    }
    Speech speech;
    GameNames names;
    std::string error;
    const bool loaded = speech.load(dictionary, error);
    std::filesystem::remove(dictionary);
    check(loaded, "load the independent dictionary without owner assets");
    if (!loaded)
        return failures;
    for (std::size_t i = 0; i < mappings.size(); ++i) {
        Record original{};
        original[0] = 22;
        setWord(original, 4, mappings[i].first);
        const auto before = original;
        check(itemnames::name(speech, names, original.data()) == words[i],
              "full naming normalizes special gear before choosing a dictionary branch");
        check(original == before, "naming leaves the original live record untouched");
    }
    Record formula{};
    formula[0] = 2;
    setWord(formula, 0x18, 3);
    check(itemnames::name(speech, names, formula.data()) == "Formula: blade",
          "formula names retain the represented item branch");
    setWord(formula, 4, 64);
    setWord(formula, 0x18, 22);
    const auto before = formula;
    check(itemnames::name(speech, names, formula.data()) == "Formula: coat",
          "recursive formula naming performs the semantic stand-in copy too");
    check(formula == before, "recursive naming never clears the caller's wrapped category");
    const auto baseKey = itemnames::itemKey(3, 0);
    Record gear{};
    gear[0] = 3;
    gear[0x1c] = 3; // a material outside the explicit native key table
    for (const auto rarity : {5, 255}) {
        gear[0x14] = uint8_t(rarity);
        check(itemnames::name(speech, names, gear.data()) == "generic " + baseKey,
              "unrecognized rarity uses the generic material-key fallback without a quality clamp");
    }
    check(itemnames::materialKey(3) == "Material" && itemnames::materialKey(255) == "Material",
          "unlisted material bytes resolve the native generic key");
    constexpr std::array<const char *, 4> spirits{{"ember", "shade", "frost", "gust"}};
    gear[0x14] = 5;
    gear[0x9d] = 32;
    for (unsigned i = 0; i < 32; ++i)
        gear[0x1d + i * 4 + 3] = 129;
    for (unsigned i = 0; i < spirits.size(); ++i) {
        gear[0x1c] = uint8_t(128 + i);
        check(itemnames::name(speech, names, gear.data()) == std::string(spirits[i]) + " " + baseKey,
              "unsigned own-material bytes select the name; attribute tallies add no suffix or override");
    }
    gear[0x1c] = 128;
    gear[0x14] = 0;
    setWord(gear, 8, 0xffffffff);
    check(itemnames::name(speech, names, gear.data()) == "quality MaterialFire " + baseKey,
          "unsigned modifier remainder selects the quality key with distinct materialitem/item macros");
    std::cout << "Item names: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
