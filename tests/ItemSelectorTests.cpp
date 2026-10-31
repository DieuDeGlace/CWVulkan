#include "ItemSelector.h"
#include "UsePrompt.h"
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstring>

int itemSelectorTests() {
    using namespace itemselector;
    int failures = 0;
    auto check = [&](bool passed, const char *message) {
        if (!passed) {
            std::printf("FAIL item selector: %s\n", message);
            ++failures;
        }
    };
    inventoryslots::Inventory inventory;
    inventory.tabs = {{{1, nativesaves::makeItem(1, 9)}, {0, nativesaves::makeItem(1, 0)}},
                      {{-2, nativesaves::makeItem(1, 13)}, {5, nativesaves::makeItem(28, 0)}}};
    const auto before = inventory.tabs;
    const auto choices = candidates(inventory);
    bool unchanged = inventory.tabs.size() == before.size();
    for (size_t tab = 0; unchanged && tab < before.size(); ++tab)
        for (size_t slot = 0; slot < before[tab].size(); ++slot)
            unchanged &=
                inventory.tabs[tab][slot].count == before[tab][slot].count && inventory.tabs[tab][slot].item == before[tab][slot].item;
    check(choices == std::vector<inventoryslots::Index>{{0, 0}, {1, 0}} && unchanged,
          "category1 candidates retain canonical order, bombs, portal stones and negative nonzero counts");
    check(wrappedIndex(INT_MIN, 3) == 1 && wrappedIndex(-1, 3) == 2 && wrappedIndex(INT_MAX, 3) == 1,
          "running signed indices wrap without signed negation overflow");
    auto portal = nativesaves::makeItem(1, 13);
    portal[nativesaves::item::RARITY] = 255;
    check(candidateColour(portal) == glm::vec4(.5f, 1, 1, 1), "portal text has fixed cyan before rarity selection");
    portal[4] = 0;
    check(candidateColour(portal) == glm::vec4(1, 1, 0, 1), "ordinary category1 text uses unsigned rarity with palette clamping");
    inventoryslots::PlayerState player;
    player.inventory.tabs = inventory.tabs;
    std::array<uint8_t, 0x1600> actor{};
    auto putFloat = [&](size_t at, float value) { std::memcpy(actor.data() + at, &value, sizeof value); };
    check(useprompt::inputAdmitted(actor.data(), false, false) && !useprompt::inputAdmitted(actor.data(), true, false) &&
              !useprompt::inputAdmitted(actor.data(), false, true),
          "use admission retains distinct editor/chat gates");
    actor[0x68] = 0x78;
    putFloat(0x9a0, .001f);
    check(useprompt::inputAdmitted(actor.data(), false, false), "glider stamina equality remains admitted");
    putFloat(0x9a0, .0009f);
    check(!useprompt::inputAdmitted(actor.data(), false, false), "exhausted airborne glider returns before action edge stores");
    actor[0x5c] = 1;
    check(useprompt::inputAdmitted(actor.data(), false, false), "grounded glider retains use input");
    actor[0x68] = 0x7d;
    putFloat(0x6c, 1999.99f);
    check(!useprompt::inputAdmitted(actor.data(), false, false), "ability7d wait is strict below2000ms");
    putFloat(0x6c, 2000);
    check(useprompt::inputAdmitted(actor.data(), false, false), "ability7d equality is admitted");
    actor[0x68] = 0;
    auto used = inventoryslots::useItem(player, {0, 0}, actor.data());
    check(used.interaction && (*used.interaction)[4] == 9 && player.inventory.tabs[0][0].count == 0,
          "selected bomb queues its original item before clearing the source");
    used = inventoryslots::useItem(player, {1, 0}, actor.data());
    check(used.action == inventoryslots::Action::Map && used.interaction && player.inventory.tabs[1][0].count == -2,
          "selected portal opens its continuation without consuming negative nonzero quantity");
    Selector selector;
    Input input;
    input.toggle = true;
    input.left = true;
    auto frame = selector.frame(input, 3, 16, 1);
    check(selector.shown && selector.index == 0 && frame.selectionSounds == 0, "held-through-opening direction waits for neutral");
    input.toggle = input.left = false;
    selector.frame(input, 3, 16, 1);
    input.right = true;
    frame = selector.frame(input, 3, 16, 1);
    check(selector.index == 1 && frame.captionIndex == 0 && frame.selectionSounds == 1,
          "caption precedes selection and a latched direction selects immediately");
    frame = selector.frame(input, 3, 183.5, 1);
    check(selector.index == 1 && frame.selectionSounds == 0, "repeat does not catch up within elapsed frame");
    frame = selector.frame(input, 3, 1, 1);
    check(selector.index == 2 && frame.selectionSounds == 1, "countdown truncates and the next frame repeats");
    input.left = true;
    input.right = false;
    selector.frame(input, 3, 16, 1);
    input.right = true;
    frame = selector.frame(input, 3, 1000, 1);
    check(frame.selectionSounds == 1, "left and right retain independent repeat clocks");
    frame = selector.frame(input, 3, 1000, 1);
    check(frame.selectionSounds == 2, "both directions may select and sound in one frame, left first");
    const auto retainedIndex = selector.index;
    selector.hide();
    selector.frame(input, 3, 16, 1);
    check(selector.index == retainedIndex && selector.angle == 0 && selector.radius == 0, "hidden resets animation, preserves index");
    input = {};
    input.allowToggle = false;
    input.toggle = true;
    selector.frame(input, 3, 16, 1);
    check(!selector.shown, "blocked toggle does not update its edge latch");
    input.allowToggle = true;
    frame = selector.frame(input, 3, 16, .5f);
    check(selector.shown && selector.radius > 0 && selector.radius < 100 && frame.captionIndex == wrappedIndex(retainedIndex, 3),
          "same held toggle opens on newly admitted edge and scales radius");
    input.toggle = false;
    selector.frame(input, 0, 16, 1);
    input.left = true;
    frame = selector.frame(input, 3, 16, 1);
    check(frame.selectionSounds == 1, "shown empty interval preserves established neutral latch");
    input = {};
    input.stickY = .25f;
    selector.frame(input, 3, 16, 1);
    check(selector.shown, "movement close has a strict analog threshold");
    input.stickY = .25001f;
    selector.frame(input, 3, 16, 1);
    check(!selector.shown, "forward analog movement closes selector");
    selector.shown = true;
    selector.index = INT_MAX;
    input = {};
    selector.frame(input, 3, 16, 1);
    input.right = true;
    selector.frame(input, 3, 16, 1);
    check(selector.index == INT_MIN && std::isfinite(selector.angle), "selection increment wraps32 and keeps finite displayed angle");
    SoundEdges sound;
    check(sound.sample(false, true) == 0x56 && sound.sample(true, true) == 0 && sound.sample(true, false) == 0 &&
              sound.sample(false, false) == 0x57 && sound.sample(false, false) == 0,
          "one combined UI sound edge suppresses duplicate Ring opens/closes behind other panels");
    std::printf("Item selector: %s\n", failures ? "FAILED" : "PASS");
    return failures;
}
