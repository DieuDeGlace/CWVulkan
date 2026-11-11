#include "WorldList.h"
#include <climits>
#include <iostream>

int worldListTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *message) {
        if (!passed) {
            std::cout << "FAIL world list: " << message << '\n';
            ++failures;
        }
    };
    using worldlist::Entry;
    const std::vector<Entry> list{{"Home", 7}, {"online_Shared", 9}, {"online", 3}, {"online_", 4}, {"Other", -2}};
    check(!worldlist::online(list[0]) && worldlist::online(list[1]) && !worldlist::online(list[2]) && worldlist::online(list[3]),
          "only names whose first seven bytes are online_ are multiplayer entries");
    check(worldlist::cards(list, false) == std::vector<int>({0, 2, 4, 5}),
          "single-player cards keep their list indices, skip online entries and end with New world at the count");
    check(worldlist::cards(list, true) == std::vector<int>({1, 3, 5}),
          "multiplayer cards keep their list indices and still end with New world");
    check(worldlist::cards({}, false) == std::vector<int>({0}) && worldlist::cards({}, true) == std::vector<int>({0}),
          "an empty list shows exactly the New world card at index zero");

    check(worldlist::createAdmitted(u"ab", u"0") && worldlist::createAdmitted(u"My World", u"0123456789012345678901"),
          "two name units and any non-empty digit string admit Create World, with no upper bound");
    check(!worldlist::createAdmitted(u"a", u"5") && !worldlist::createAdmitted(u"ab", u"") &&
              !worldlist::createAdmitted(u"ab", u"-5") && !worldlist::createAdmitted(u"ab", u"1 2") &&
              !worldlist::createAdmitted(u"ab", std::u16string(1, char16_t(0xff11))),
          "a short name, an empty seed, a sign, a space or a non-ASCII digit hides Create World");

    check(worldlist::parseSeed(u"0") == 0 && worldlist::parseSeed(u"00123") == 123 &&
              worldlist::parseSeed(u"2147483647") == INT32_MAX,
          "admitted digits read as a signed 32-bit decimal");
    check(worldlist::parseSeed(u"2147483648") == INT32_MAX && worldlist::parseSeed(u"99999999999999999999") == INT32_MAX,
          "values past the signed range give the largest value");

    check(std::string(worldlist::selectCaption(0, 2)) == "Select" && std::string(worldlist::selectCaption(-1, 2)) == "Select" &&
              std::string(worldlist::selectCaption(2, 2)) == "Create" && std::string(worldlist::selectCaption(0, 0)) == "Create",
          "the Select caption compares the signed current index with the entry count");
    check(std::string(worldlist::toggleCaption(true)) == "Singleplayer worlds..." &&
              std::string(worldlist::toggleCaption(false)) == "Multiplayer worlds..." &&
              std::string(worldlist::INITIAL_TOGGLE_CAPTION) == "Multiplayer Worlds...",
          "the toggle captions differ from the construction caption in case");

    check(worldlist::exploredText(0) == std::wstring(L"Explored: 0 km") + wchar_t(0x00b2) && worldlist::exploredText(1234567) == std::wstring(L"Explored: 1.23457e+06 km") + wchar_t(0x00b2),
          "the explored figure is a float in the default stream format");
    check(worldlist::seedText(-17) == L"Seed: -17" && worldlist::seedText(INT32_MAX) == L"Seed: 2147483647",
          "the seed line is the signed decimal");

    std::vector<Entry> removing = list;
    int32_t current = 5;
    check(!worldlist::remove(removing, current) && current == 5 && removing.size() == 5, "the New world index deletes nothing");
    current = -1;
    check(!worldlist::remove(removing, current) && removing.size() == 5, "a negative index deletes nothing");
    current = 1;
    check(worldlist::remove(removing, current) && current == -1 && removing.size() == 4 && removing[1].name == "online" &&
              removing[3].name == "Other",
          "deletion closes the gap in order and leaves no current entry");
    return failures;
}
