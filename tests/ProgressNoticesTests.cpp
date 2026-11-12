// Progress notices (docs/specs/steam-progress-notices-and-floating-text-20261112.md).
#include "ProgressNotices.h"
#include <cstdio>
#include <vector>

int progressNoticesTests() {
    using namespace progressnotices;
    int failures = 0;
    const auto check = [&](bool passed, const char *message) {
        if (!passed) {
            std::printf("FAIL progress notices: %s\n", message);
            ++failures;
        }
    };
    check(totalExperience(1, 7) == 7 && totalExperience(0, 7) == 7, "level 1 or below adds nothing to the XP field (7.2)");
    check(totalExperience(2, 7) == 17 && totalExperience(3, 0) == 24 && totalExperience(4, 0) == 44,
          "each level below the current adds trunc(10 * 2^((k-1)/2)) (7.2)");
    check(totalExperience(11, 3) == 750, "the first ten terms are 10 14 20 28 40 56 80 113 160 226 (7.2)");

    const feattext::Point player{100, 200, 300};
    check(anchor(player, 32767, 0, 32767) == feattext::Point{100 - 65536, 200 + 65536, 300 + 65536},
          "the first draw moves Y, the second X, the third Z upward, each up to one block (3.2)");
    check(anchor(player, 0, 32767, 0) == feattext::Point{100 + 65536, 200 - 65536, 300},
          "horizontal draws span -1..+1 block and the vertical one 0..1 (3.2)");

    int draws = 0;
    const auto rand = [&] { ++draws; return 0; };
    Progress before;
    before.gold = 10, before.platinum = 2, before.zone = {4, 5}, before.level = 3, before.experience = 1;
    Progress now = before;
    now.gold = 25;
    auto out = compare(beforeWorld(before), afterCamera(before), now, player, rand);
    check(out.size() == 1 && out[0].record.text == L"+15 GOLD" && out[0].chat.empty() && out[0].sound == 0,
          "a gold increase gives \"+N GOLD\" without a chat line or sound (phase 90)");
    check(out.size() == 1 && out[0].record.colour == glm::vec4(1, .9f, .5f, 1) && out[0].record.offsetY == -100 &&
              out[0].record.fontSize == 16 && draws == 3,
          "the gold record is (1, 0.9, 0.5), 100 px up, font 16, after three draws (3.3)");
    check(out.size() == 1 && out[0].record.anchor == feattext::Point{100 - 65536, 200 - 65536, 300},
          "the record is anchored at the drawn offset from P; three zero draws give (-1, -1, 0) blocks (3.2)");

    draws = 0;
    now = before, now.gold = 3;
    check(compare(beforeWorld(before), afterCamera(before), now, player, rand).empty() && draws == 0,
          "a decrease (a purchase) gives nothing and draws nothing (7.1)");

    now = before, now.platinum = 5;
    out = compare(beforeWorld(before), afterCamera(before), now, player, rand);
    check(out.size() == 1 && out[0].record.text == L"+3 PLATINUM" && out[0].record.colour == glm::vec4(.8f, .9f, .9f, 1),
          "a platinum counter increase in the same zone gives \"+N PLATINUM\" (phase 91)");
    now.zone = {4, 6};
    check(compare(beforeWorld(before), afterCamera(before), now, player, rand).empty(),
          "a zone change during the frame suppresses the platinum notice (7.4)");

    now = before, now.experience = 9;
    out = compare(beforeWorld(before), afterCamera(before), now, player, rand);
    check(out.size() == 1 && out[0].record.text == L"+8 XP" && out[0].record.colour == glm::vec4(.65f, .4f, 1, 1) &&
              out[0].chat == u"You gain 8 XP.\n" && out[0].chatColour == std::array<uint8_t, 3>{150, 128, 255} && out[0].sound == 0,
          "an XP increase gives \"+N XP\" and \"You gain N XP.\" (phase 89, section 1)");

    draws = 0;
    now = before, now.level = 4;
    out = compare(beforeWorld(before), afterCamera(before), now, player, rand);
    check(out.size() == 2 && out[0].record.text == L"+20 XP" && out[1].record.text == L"LEVEL UP!",
          "a level rise also raises total XP: XP precedes the level notice (phases 89, 92)");
    check(out.size() == 2 && out[1].record.offsetY == -150 && out[1].record.colour == glm::vec4(1, .9f, .5f, 1) &&
              out[1].sound == 0x1d && out[1].chat == u"You reached level 4!\n" &&
              out[1].chatColour == std::array<uint8_t, 3>{255, 220, 128} && draws == 6,
          "the level notice is 150 px up with sound 0x1D and \"You reached level N!\" (phase 92, section 1)");

    draws = 0;
    now = before, now.gold = 11, now.platinum = 3, now.level = 4;
    out = compare(beforeWorld(before), afterCamera(before), now, player, rand);
    check(out.size() == 4 && out[0].record.text == L"+20 XP" && out[1].record.text == L"+1 GOLD" &&
              out[2].record.text == L"+1 PLATINUM" && out[3].record.text == L"LEVEL UP!" && draws == 12,
          "all four fire in phase order, three draws each (3.3)");

    const auto feat = feattext::feat({1, 2, 3});
    check(feat.text == L"FEAT!" && feat.fontSize == 30 && feat.offsetY == -100 && feattext::outlineRadius(30) == 3.75f &&
              feattext::outlineRadius(16) == 3.0f,
          "the feat record keeps font 30; outlines are font/8, at least 3 (section 4 step 7)");
    std::printf("Progress notices: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
