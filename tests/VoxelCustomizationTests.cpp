#include "VoxelCustomization.h"
#include "GameCraft.h"
#include "Assets.h"
#include "ItemPreview.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>

int voxelCustomizationTests() {
    using namespace customization;
    int failures = 0;
    auto check = [&](bool pass, const char *what) {
        if (!pass) {
            std::printf("FAIL customization: %s\n", what);
            ++failures;
        }
    };
    auto weapon = nativesaves::makeItem(3, 0, 1);
    check(gamecraft::placeCube(weapon.data(), 129, 0, 0, 7) && weapon[0x1d] == 0x81, "placement stores the low coordinate byte");
    check(!gamecraft::removeCube(weapon.data(), 129, 0, 0) && weapon[0x9d] == 1,
          "removal compares the full argument to signed stored coordinates");
    upgradewidget::Widget moving;
    moving.pointed = upgradewidget::Voxel{129, 0, 0};
    upgradewidget::click(moving, weapon.data(), true);
    check(moving.carrying && moving.carried.x == 129, "pickup matches bytes but remembers full picked coordinates");
    moving.pointed = upgradewidget::Voxel{2, 0, 0};
    upgradewidget::click(moving, weapon.data(), true);
    check(!moving.carrying && weapon[0x9d] == 2 && weapon[0x1d] == 0x81 && weapon[0x21] == 2,
          "move still places when full-coordinate removal cannot match");
    check(gamecraft::removeCube(weapon.data(), -127, 0, 0) && weapon[0x9d] == 1, "signed stored coordinate can remove the original");

    State state;
    state.edit.hand = 1;
    state.edit.selected = 2;
    state.edit.carrying = true;
    state.euler = {10, 20, 30};
    state.toggleBench();
    check(state.edit.open && state.edit.selected == -1 && !state.edit.carrying && state.edit.hand == 1 &&
              state.euler == glm::vec3(10, 20, 30),
          "bench toggle resets selection/carry only");
    state.toggleBench();
    check(!state.edit.open && state.euler == glm::vec3(10, 20, 30), "closing bench preserves the pose");
    state.rotatePointer({4, -2}, 50);
    check(state.euler == glm::vec3(10.5f, 20, 31), "pointer rotation uses option and float operations without a pitch clamp");
    state.rotatePad(1.25, -2.5);
    check(state.euler == glm::vec3(11.75f, 20, 28.5f), "pad pose updates X/Z independently");

    CubModel model, cube;
    model.sx = model.sy = model.sz = 2;
    model.rgb.assign(24, 200);
    cube.sx = cube.sy = cube.sz = 1;
    cube.rgb.assign(3, 255);
    Resource body{11, {2, 2, 2}, &model}, cubeResource{12, {1, 1, 1}, &cube};
    inventoryslots::Inventory inventory;
    nativesaves::Stack stack;
    stack.count = 3;
    stack.item = nativesaves::makeItem(11, 10, 1);
    inventory.tabs[3].push_back(stack);
    std::array<Item, 2> equipped{nativesaves::makeItem(3, 0, 1), {}};
    const Resolve resolve = [&](const Item &item) { return item[0] == 3 ? body : cubeResource; };
    Layout layout;
    layout.root = glm::translate(glm::mat4(1), {500, 500, 0});
    layout.viewport = {1000, 1000};
    state = {};
    state.edit.open = true;
    state.euler = {};
    state.edit.selected = 0;
    auto plan = draw(state, layout, equipped, inventory, {500, 500}, 0, resolve, cubeResource);
    auto rawCount = [](const Plan &value) {
        return std::count_if(value.images.begin(), value.images.end(),
                             [](const auto &request) { return request.kind == guiitem::Request::Kind::RawEdit; });
    };
    check(rawCount(plan) == 2 && state.edit.pointed && state.edit.pointed->z == -1,
          "draw picks the actual model and mutates placement to its outward neighbor");
    check(plan.upgrades == std::optional(std::pair(0, 16)), "native count/cap are separate from missing or capped palette quantities");
    check(!plan.images.empty() && plan.images.front().at.screen == glm::vec2(440, 420) && plan.images.front().at.screenDepth == -.05f,
          "equipped thumbnail uses its distinct owner-relative placement and depth");
    state.paletteHover = 0;
    plan = draw(state, layout, equipped, inventory, {500, 500}, 0, resolve, cubeResource);
    check(rawCount(plan) == 1 && state.paletteHover == -1, "candidate reads prior palette hover before the later reset");
    plan = draw(state, layout, equipped, inventory, {520, 540}, 0, resolve, cubeResource);
    check(state.paletteHover == 0 && state.hoveredItem == stack.item, "physical palette half-open bounds update the cached item");
    check(plan.images.back().at.screen == glm::vec2(540, 560) && plan.images.back().at.size == .15f,
          "palette uses unscaled physical pitch and hover size");
    const auto cachedPoint = state.edit.pointed;
    draw(state, layout, equipped, inventory, {900, 900}, 0, [](const Item &) { return Resource{}; }, {});
    check(state.edit.pointed == cachedPoint, "missing raw resource does not reset the prior hit");
    bool sound = false;
    state.paletteHover = 0;
    state.handHover = 1;
    state.edit.selected = -1;
    state.edit.carrying = true;
    state.press(&equipped[0], true, sound);
    check(sound && state.edit.selected == 0 && state.edit.hand == 0 && state.edit.carrying,
          "cached palette input precedes hand input and preserves carry");
    state.press(&equipped[0], true, sound);
    check(state.edit.selected == -1, "a second cached palette press toggles selection off");
    state.paletteHover = -1;
    state.press(&equipped[0], true, sound);
    check(sound && state.edit.hand == 1 && state.edit.carrying, "hand selection preserves carry and pose");
    check(consumePalette(inventory, stack.item) && inventory.tabs[3][0].count == 2, "palette consumes one canonical equal stack");
    inventory.tabs[3][0].count = -1;
    check(consumePalette(inventory, stack.item) && inventory.tabs[3][0].count == 0 && inventory.tabs[3][0].item[0] == 0,
          "negative canonical quantities use the native decrement/clear path");
    equipped[0] = {};
    state.edit.hand = 0;
    equipped[1] = nativesaves::makeItem(3, 5, 1);
    plan = draw(state, layout, equipped, inventory, {}, 0, resolve, cubeResource);
    check(state.edit.hand == 1 && plan.upgrades->second == 32 && plan.images.front().at.screen.x == 440,
          "left-only fallback keeps the first thumbnail position and two-handed cap");
    Assets assets;
    if (assets.locate("")) {
        Hud hud;
        std::string error;
        check(hud.load(assets.gameDir(), error), "owner UI load");
        Renderer renderer;
        renderer.initHeadless(RenderSettings{});
        hud.addItemIcon("synthetic-customization", model, true);
        hud.addItemIcon("cube", cube);
        hud.setGuiScale(1);
        hud.layout(renderer, {1920, 1080});
        State ownerState;
        ownerState.edit.open = true;
        Shown shown;
        shown.state = &ownerState;
        shown.inventory = &inventory;
        shown.equipment[0] = nativesaves::makeItem(3, 0, 1);
        shown.icon = [](const Item &item) { return item[0] == 3 ? "synthetic-customization" : "cube"; };
        const auto frame = hud.customizationLayout(true);
        const auto anchor = frame.root * glm::vec4(frame.pivot, 0, 1);
        const auto ownerSize = hud.templateSize("blackwidget", {800, 800}, true);
        const auto inventorySize = hud.templateSize("blackwidget", {400, 290}, true);
        check(frame.parentSize == ownerSize && std::abs(anchor.x - (960 - ownerSize.x * .5f)) < .001f &&
                  std::abs(anchor.y - (540 - inventorySize.y - 50)) < .001f,
              "owner root uses the native request and inventory-height placement");
        check(hud.customizationHit({960, 500}) && !hud.customizationHit({20, 20}),
              "owner shape hit does not use a new viewport-sized rectangle");
        Hud::State ui;
        ui.customization = &shown;
        std::vector<OverlayVertex> overlay;
        hud.draw(ui, overlay);
        const auto draws = hud.itemDraws();
        check(draws.size() == 2 && draws[0].kind == guiitem::Request::Kind::Widget && draws[1].kind == guiitem::Request::Kind::RawEdit,
              "admitted owner callback emits original thumbnail then raw resource");
        if (draws.size() == 2)
            check(draws[0].beforeVertex > 0 && draws[0].beforeVertex == draws[1].beforeVertex && draws[1].beforeVertex < overlay.size(),
                  "owner art and heading precede images, while manual counter follows");
        inventoryslots::Inventory liveInventory;
        liveInventory.tabs.resize(4);
        const auto baseMaterial = nativesaves::makeItem(11, 10, 1);
        liveInventory.tabs[3].push_back({1, baseMaterial});
        liveInventory.tabs[3].push_back({1, nativesaves::makeItem(11, 14, 128)});
        shown.inventory = &liveInventory;
        shown.pointer = {645, 260}; // The second palette cell in the owner frame.
        ownerState.paletteHover = 0;
        ownerState.hoveredItem = baseMaterial;
        const auto *borrowed = &ownerState.hoveredItem;
        Hud::Inventory liveInformation;
        liveInformation.infoItem = baseMaterial;
        liveInformation.infoIcon = "cube";
        liveInformation.infoAnchor = itempreview::Anchor::Cursor;
        liveInformation.pointer = {900, 500};
        ui.information = &liveInformation;
        int displayedMaterial = -1;
        int collectedMaterial = -1;
        ui.collectMapMarkers = [&] {
            collectedMaterial = (*borrowed)[0x1c];
            return std::vector<mapmarks::Marker>{};
        };
        ui.informationDraw = [&] {
            displayedMaterial = (*borrowed)[0x1c];
            check(collectedMaterial == displayedMaterial, "map collection follows game UI and precedes information");
            liveInformation.infoItem = *borrowed;
            liveInformation.info = {{"Live palette", 12, 3, glm::vec4(1), 16}};
        };
        hud.draw(ui, overlay);
        check(displayedMaterial == 128 && collectedMaterial == 128 && ownerState.paletteHover == 1,
              "information reads its borrowed member after the earlier palette callback updates it");
        ui.interfaceVisible = false;
        shown.pointer = {600, 260};
        displayedMaterial = -1;
        const auto retainedPoint = ownerState.edit.pointed;
        hud.draw(ui, overlay);
        check(overlay.empty() && hud.itemDraws().empty() && displayedMaterial == -1 && ownerState.paletteHover == 1 &&
                  ownerState.hoveredItem[0x1c] == 128 && ownerState.edit.pointed == retainedPoint,
              "hidden GUI ancestor skips widget hooks without clearing their retained draw state");
        ui.interfaceVisible = true;
        hud.draw(ui, overlay);
        check(displayedMaterial == 1 && ownerState.paletteHover == 0, "resumed drawing reads the current live palette member");
        ui.information = nullptr;
        ui.informationDraw = {};
        ui.collectMapMarkers = {};
        ownerState.edit.open = false;
        hud.draw(ui, overlay);
        check(hud.itemDraws().empty() && !hud.customizationHit({960, 500}), "simple hide removes rendering/hits without clearing state");
        renderer.shutdown();
    } else
        std::puts("SKIP customization owner tree: owner installation unavailable");
    std::printf("Native customization state/draw: %s\n", failures ? "FAILED" : "PASS");
    return failures;
}
