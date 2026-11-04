#include "../tools/ZoneExtractor/NativeNameFragments.h"
#include "GameNames.h"
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>

int properNameTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *label) {
        if (!passed) {
            std::cout << "FAIL proper names: " << label << '\n';
            ++failures;
        }
    };
    const auto file = std::filesystem::temp_directory_path() /
                      ("CubeVulkan-proper-names-" +
                       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".txt");
    {
        std::ofstream stream(file);
        for (unsigned i = 0; i < 380; ++i)
            stream << 'f' << i << "|\n";
    }
    GameNames names;
    check(names.load(file), "load invented fragments in the existing owner-file order");
    std::filesystem::remove(file);
    // Independent arithmetic examples from steam-generated-proper-names §7,
    // translated only to this project's documented flat fragment-file offsets.
    // These are contract-derived fixtures, not runtime-captured name outputs.
    struct Example {
        std::int32_t seed, selector;
        unsigned prefix, suffix;
    };
    constexpr Example examples[] = {
        {0, -1, 9, 32},         {0, 0, 108, 110},         {0, 2, 61, 79},         {0, 11, 260, 270},
        {-1, -1, 13, 50},       {-1, 0, 104, 114},        {-1, 2, 68, 75},        {-1, 11, 266, 277},
        {INT32_MIN, -1, 9, 32}, {INT32_MIN, 0, 108, 110}, {INT32_MIN, 2, 61, 79}, {INT32_MIN, 11, 260, 270},
        {0, 6, 7, 50},          {0, 18, 69, 76},
    };
    const auto expected = [](unsigned prefix, unsigned suffix) {
        return "f" + std::to_string(prefix) + "|f" + std::to_string(suffix) + "|";
    };
    for (const auto &example : examples)
        check(names.name(example.seed, example.selector) == expected(example.prefix, example.suffix),
              "derived index fixture preserves unsigned hash reduction and original selector bits");
    check(names.name(0, INT32_MAX) == expected(9, 32) && names.name(0, INT32_MIN) == expected(28, 30),
          "extreme selectors wrap hash inputs while retaining generic-table dispatch");
    check(names.name(0, -1) != names.name(0, 6) && names.name(0, 2) != names.name(0, 18),
          "shared-table aliases are not canonicalized before hashing");

    constexpr std::uintptr_t module = 0x10000000;
    std::map<std::uintptr_t, std::vector<std::uint8_t>> memory;
    const auto fixture = [&] {
        memory.clear();
        for (const auto literal : captureabi::NAME_LITERALS) {
            std::u16string text(literal.units, u'x');
            text.push_back(0);
            auto &bytes = memory[module + literal.rva];
            bytes.resize(text.size() * 2);
            std::memcpy(bytes.data(), text.data(), bytes.size());
        }
        const char16_t first[] = u"ABC";
        std::memcpy(memory[module + 0x431d60].data(), first, sizeof first);
    };
    unsigned reads = 0;
    const auto read = [&](std::uintptr_t address, void *destination, std::size_t count) {
        ++reads;
        auto found = memory.find(address);
        if (found == memory.end() || count != found->second.size())
            return false;
        std::memcpy(destination, found->second.data(), count);
        return true;
    };
    std::vector<std::string> fragments;
    fixture();
    check(captureabi::readNameFragments(module, fragments, read) && fragments.size() == 380 && fragments[0] == "ABC" &&
              reads == 380,
          "all380 data-only literal spans retain legacy order and exact UTF-16 lengths");
    const auto corrupt = [&](char16_t unit, std::size_t at) {
        std::memcpy(memory[module + 0x431d60].data() + at * 2, &unit, 2);
    };
    for (const auto unit : {char16_t(0x100), u'\n', u'\r', char16_t(0)}) {
        fixture();
        corrupt(unit, 0);
        check(!captureabi::readNameFragments(module, fragments, read) && fragments.empty(),
              "legacy line format rejects unrepresentable/control units without partial output");
    }
    fixture();
    corrupt(u'X', 3);
    check(!captureabi::readNameFragments(module, fragments, read) && fragments.empty(),
          "the declared literal terminator must match");
    fixture();
    memory.erase(module + 0x432798);
    check(!captureabi::readNameFragments(module, fragments, read) && fragments.empty(),
          "an unreadable late fragment discards the complete candidate");
    reads = 0;
    check(!captureabi::readNameFragments(std::numeric_limits<std::uintptr_t>::max(), fragments, read) && reads == 0,
          "image-relative fragment spans cannot wrap");
    std::cout << "Proper names: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
