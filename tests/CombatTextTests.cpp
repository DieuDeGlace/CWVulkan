// Combat texts of phase 68 (docs/specs/steam-combat-floating-texts-20261113.md §§2-3, 6.1, 7).
#include "CombatText.h"
#include <cstdio>
#include <cstring>
#include <map>

int combatTextTests() {
    using namespace combattext;
    int failures = 0;
    const auto check = [&](bool passed, const char *message) {
        if (!passed) {
            std::printf("FAIL combat text: %s\n", message);
            ++failures;
        }
    };
    constexpr int64_t P = 7, MONSTER = 9, OTHER = 11, BLOCK = 65536;
    std::map<int64_t, std::array<uint8_t, 0xc00>> creatures;
    for (int64_t id : {P, MONSTER, OTHER})
        creatures[id] = {};
    const float size = 0.5f; // creature +0x8C: the text starts (1 - 0.25) blocks higher
    std::memcpy(creatures[MONSTER].data() + 0x8c, &size, 4);
    const Lookup lookup = [&](int64_t id) -> uint8_t * {
        auto it = creatures.find(id);
        return it == creatures.end() ? nullptr : it->second.data();
    };
    const uint8_t *player = creatures[P].data();
    const auto hit = [](int64_t attacker, int64_t target, float amount, uint8_t kind = 0, int32_t stun = 0, bool critical = false,
                        bool spirit = false) {
        Hit h{};
        const int64_t at[3] = {10 * BLOCK, 20 * BLOCK, 30 * BLOCK};
        std::memcpy(h.data(), &attacker, 8);
        std::memcpy(h.data() + 0x08, &target, 8);
        std::memcpy(h.data() + 0x10, &amount, 4);
        h[0x14] = critical;
        std::memcpy(h.data() + 0x18, &stun, 4);
        std::memcpy(h.data() + 0x20, at, sizeof at);
        h[0x44] = spirit, h[0x45] = kind;
        return h;
    };
    const auto dealt = [&](int64_t id) {
        float v;
        std::memcpy(&v, creatures[id].data() + 0xbc0, 4);
        return v;
    };

    // Numbers: truncated from 1 up, tenths below, no sign.
    check(number(12.9f) == L"12" && number(1.0f) == L"1" && number(999.99f) == L"999", "whole numbers truncate");
    check(number(0.35f) == L"0.3" && number(-0.35f) == L"0.3" && number(0.1f) == L"0.1", "below one prints tenths without sign");
    check(number(-25.5f) == L"25", "healing prints without a minus sign");
    check(number(0.05f) == L"0", "below a tenth prints 0");

    // P hits a monster: white, rising 100 px, two blocks plus the size term above the hit.
    auto out = visit(hit(P, MONSTER, 42.7f), lookup, player);
    check(out.size() == 1 && out[0].text == L"42" && out[0].colour == glm::vec4(1, 1, 1, 1) && out[0].drift == -100 &&
              out[0].fontSize == 20 && out[0].lifetime == 1000 && out[0].offsetX == 0 && out[0].offsetY == 0,
          "P's hit on a monster is a white 20 px number rising 100 px for 1000 ms");
    check(out.size() == 1 && out[0].anchor == feattext::Point{10 * BLOCK, 20 * BLOCK, 30 * BLOCK + 2 * BLOCK + 49152},
          "the number starts two blocks plus (1 - size/2) blocks above the hit");
    check(dealt(P) == 42.7f, "the attacker's +0xBC0 accumulates the amount");
    // Critical: font 60 and drift -200; spirit-tinted blue 0.25.
    out = visit(hit(P, MONSTER, 3, 0, 0, true, true), lookup, player);
    check(out.size() == 1 && out[0].fontSize == 60 && out[0].drift == -200 && out[0].colour == glm::vec4(1, 1, .25f, 1),
          "a critical spirit hit is 60 px, rises 200 px and is tinted");
    // A monster hits P: red, one block lower, sinking 100 px.
    out = visit(hit(MONSTER, P, 8), lookup, player);
    check(out.size() == 1 && out[0].colour == glm::vec4(1, .3f, .3f, 1) && out[0].drift == 100 &&
              out[0].anchor[2] == 30 * BLOCK + 2 * BLOCK + 65536 - BLOCK,
          "a hit on P is red, one block lower and sinks 100 px");
    // Healing: green; on P it sinks, on others it rises.
    out = visit(hit(P, P, -0.35f), lookup, player);
    check(out.size() == 1 && out[0].text == L"0.3" && out[0].colour == glm::vec4(.4f, 1, .5f, 1) && out[0].drift == 100,
          "P's own healing is green and sinks");
    out = visit(hit(P, MONSTER, -5), lookup, player);
    check(out.size() == 1 && out[0].drift == -100 && out[0].colour == glm::vec4(.4f, 1, .5f, 1), "healing another rises");
    // Labels carry a line feed; kinds 4 and 6 show nothing; tiny kind-0 hits are silent.
    const auto only = [&](const Hit &h) {
        const auto texts = visit(h, lookup, player);
        return texts.size() == 1 ? texts[0].text : std::wstring(L"(none)");
    };
    check(only(hit(MONSTER, P, 5, 1)) == L"BLOCK\n", "kind 1 is BLOCK");
    check(only(hit(P, MONSTER, 0, 3)) == L"MISS\n", "kind 3 is MISS");
    check(only(hit(MONSTER, P, 0, 5)) == L"ABSORB\n", "kind 5 is ABSORB");
    check(visit(hit(P, MONSTER, 9, 4), lookup, player).empty() && visit(hit(MONSTER, P, 9, 6), lookup, player).empty(),
          "kinds 4 and 6 show no text");
    check(visit(hit(P, MONSTER, 0.05f), lookup, player).empty(), "a kind-0 hit below 0.1 is silent");
    // Hits between others: no number, but "K.O." for any stun, sinking 50 px (rising on P).
    out = visit(hit(MONSTER, OTHER, 30, 0, 800), lookup, player);
    check(out.size() == 1 && out[0].text == L"K.O." && out[0].colour == glm::vec4(1, 1, 0, 1) && out[0].drift == 50 &&
              out[0].fontSize == 20 && out[0].anchor[2] == 30 * BLOCK + 2 * BLOCK,
          "a stun between others shows only a yellow K.O. sinking 50 px, two blocks up");
    out = visit(hit(MONSTER, P, 30, 0, 800, true), lookup, player);
    check(out.size() == 2 && out[0].text == L"K.O." && out[0].drift == -50 && out[0].fontSize == 20 && out[1].text == L"30" &&
              out[1].fontSize == 60,
          "on P: K.O. first (rising 50, never 60 px), then the number");
    // The record gate: an unknown target yields nothing, not even the attacker's bookkeeping.
    const float before = dealt(MONSTER);
    check(visit(hit(MONSTER, 404, 10, 0, 500), lookup, player).empty() && dealt(MONSTER) == before,
          "an unknown target is skipped entirely");
    // No local player: only K.O.
    check(visit(hit(MONSTER, P, 10, 0, 500), lookup, nullptr).size() == 1, "without a player only K.O. shows");

    // The store: lifetime 1000, drift applied linearly, offsets 0, removed after 1000 ms.
    feattext::Store store;
    out = visit(hit(P, MONSTER, 42), lookup, player);
    check(out.size() == 1, "the store test's record exists");
    if (out.empty())
        return failures;
    store.append(out[0]);
    scenevisibility::Projection identity{};
    identity.view = glm::mat4(1), identity.perspective = glm::mat4(1);
    identity.offset = {-10 * BLOCK, -20 * BLOCK, 0};
    auto labels = store.visit(identity, {200, 200}, 500); // drawn at age 0
    const auto first = labels.size() == 1 ? labels[0].at : glm::vec2(-1);
    labels = store.visit(identity, {200, 200}, 500); // age 500: half the drift
    check(labels.size() == 1 && labels[0].at.x == first.x && labels[0].at.y == first.y - 50 && labels[0].fill.a == .5f,
          "at age 500 the text has risen 50 px and is half faded");
    check(store.size() == 1, "kept at age 1000");
    labels = store.visit(identity, {200, 200}, 1); // age 1000: drawn once more at alpha 0, then 1001
    check(labels.size() == 1 && labels[0].fill.a == 0 && store.size() == 0, "drawn at alpha 0 at age 1000, then removed");
    return failures;
}
