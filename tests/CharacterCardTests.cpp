#include "CharacterCard.h"
#include "GuiItem.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

int characterCardTests() {
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        if (!ok) {
            ++failures;
            std::printf("FAIL character card: %s\n", message);
        }
    };
    std::array<uint8_t, nativepose::CREATURE_SIZE> record{};
    const auto put = [&](size_t offset, const auto &value) { std::memcpy(record.data() + offset, &value, sizeof value); };
    const auto number = [&](size_t offset) {
        float value;
        std::memcpy(&value, record.data() + offset, sizeof value);
        return value;
    };
    put(0x12c, uint32_t(-1));
    put(0x130, uint32_t(-1));
    put(0x5c, uint32_t(-1));
    put(0x128, uint16_t(-1));
    record[0x68] = 17;
    put(0x6c, 77.f);
    put(nativepose::ANIM + 8, 123.f);
    put(nativepose::ANIM + 12, int32_t(9));
    put(0x38, 4.f);
    charactercard::advance(record.data(), 16, true);
    check(record[0x68] == 0 && record[0x128] == 0 && record[0x12c] == 0 && record[0x130] == 0 && record[0x5c] == 0,
          "admitted card clears its listed control fields");
    check(std::abs(number(0x99c) - .12f) < 1e-7f && number(0xb3c) == -.8f && number(0x998) == .2f,
          "GUI integer step advances walk phase and hover yaw without the fractional renderer interval");
    check(number(0x44) == 1.1545143127441406f && number(0x38) == 3.4227428436279297f,
          "native0.01 easing moves intent toward8 while physical velocity eases to zero");
    check(number(0x6c) == 77.f && number(nativepose::ANIM + 8) == 123.f,
          "card updates do not assign the world's ability clock to cached pose time");
    charactercard::advance(record.data(), 16, false);
    check(number(0xb3c) > -.8f && number(0xb3c) < 0 && number(0x998) < .2f && number(0x44) < 1.1545143127441406f,
          "leaving the subtree eases the persistent yaw, motion scalar and intent back");
    for (size_t offset : {size_t(0x84), size_t(0x88), size_t(0x8c)})
        put(offset, 1.f);
    for (size_t offset = 0xa0; offset <= 0xc0; offset += 4)
        put(offset, 1.f);
    put(nativepose::ANIM + 12, int32_t(0));
    const std::array<int64_t, 3> position{123, -456, 789}, drawn{111, 222, 333};
    put(0x10, position);
    put(0xb10, drawn);
    put(0x180, 0.f); // the card has no living-world health gate
    nativepose::Models models;
    nativepose::Sparks sparks;
    std::vector<nativepose::Draw> draws;
    charactercard::prepare(record.data(), {16, 16.75f, false, 42}, models, [] { return 0; }, sparks, draws);
    std::array<int64_t, 3> after{}, physical{};
    std::memcpy(after.data(), record.data() + 0xb10, sizeof after);
    std::memcpy(physical.data(), record.data() + 0x10, sizeof physical);
    std::array<int64_t, 2> origin{};
    std::memcpy(origin.data(), record.data() + 0x1508, sizeof origin);
    check(after == drawn && physical == position && origin == std::array<int64_t, 2>{} && record[0x1500] == 1,
          "direct zero-origin pose restores drawn position, retains its cache and admits zero health without a world gate");

    glm::mat4 node(1);
    node[0] = {2, 1, 0, .25f};
    node[1] = {.5f, 3, 0, .5f};
    node[3] = {30, 40, 0, 2};
    const auto shape = charactercard::geometry(node, {4, 6}, {800, 600}, 15);
    check(shape && shape->scale == 2 && std::abs(shape->screen.x - 206.833333f) < .0001f &&
              std::abs(shape->screen.y - 230.333333f) < .0001f && shape->base[3].z == 17,
          "projected pivot uses homogeneous divide and signed M00 scale, without allocation fitting or scaled depth17");
    const auto plain = charactercard::geometry(glm::mat4(1), {}, {800, 600}, 15);
    if (plain) {
        const auto clip = plain->projection * glm::vec4(0, 0, 17, 1);
        check(std::abs(plain->projection[1][1] - 2.1445069f) < .000001f && std::abs(clip.x / clip.w + .75f) < .000001f &&
                  std::abs(clip.y / clip.w - .633333333f) < .000001f && clip.z / clip.w > .49f && clip.z / clip.w < .5f,
              "effective50-degree overlay projects the submitted pixel center with homogeneous minus-half depth");
    } else
        check(false, "valid full-window card geometry exists");
    glm::mat4 reflected(1);
    reflected[0][0] = -1;
    const auto negative = charactercard::geometry(reflected, {}, {800, 600}, 15);
    check(negative && negative->scale == -1 && negative->screen.x == -100 && negative->screen.y == -110 && negative->base[3].z == 17 &&
              plain && negative->base[0] == -plain->base[0],
          "negative M00 remains a signed uniform scale instead of a basis length");
    std::printf("Native character-card preview: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
