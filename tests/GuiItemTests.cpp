// Mathematical checks derived from steam-gui-item-icons.md; no game asset is distributed.
#include "GuiItem.h"
#include "Assets.h"
#include "CubModel.h"
#include "Hud.h"
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {
int failures = 0;
#define GUI_CHECK(...)                                                                                                                     \
    do {                                                                                                                                   \
        if (!(__VA_ARGS__)) {                                                                                                              \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #__VA_ARGS__);                                                             \
            failures++;                                                                                                                    \
        }                                                                                                                                  \
    } while (0)
bool near(float a, float b) {
    return std::abs(a - b) < 1e-6f;
}
void equipmentCallbacks() {
    Assets assets;
    if (!assets.locate("")) {
        std::puts("SKIP equipment callbacks: owner install unavailable");
        return;
    }
    Hud hud;
    std::string error;
    GUI_CHECK(hud.load(assets.gameDir(), error));
    if (!hud.hasPanels())
        return;
    Renderer renderer;
    renderer.initHeadless(RenderSettings{});
    CubModel cub;
    cub.sx = cub.sy = cub.sz = 1;
    cub.rgb = {120, 80, 40};
    hud.addItemIcon("synthetic-equipment", cub);
    hud.setGuiScale(1);
    hud.layout(renderer, {1920, 1080});
    Hud::Inventory inventory;
    inventory.sheet.valid = true;
    inventory.slots[0] = inventory.slots[1] = "synthetic-equipment";
    auto weapon = nativesaves::makeItem(3, 0, 1);
    weapon[0x9d] = 1;
    weapon[0x1d] = uint8_t(-2);
    inventory.slotItems[0] = weapon;
    inventory.slotItems[1] = nativesaves::makeItem(9, 0);
    Hud::State state;
    state.inventory = &inventory;
    state.guiClockMs = 2500;
    std::vector<OverlayVertex> vertices;
    hud.draw(state, vertices);
    const auto requests = hud.itemDraws();
    GUI_CHECK(requests.size() == 2);
    if (requests.size() == 2) {
        GUI_CHECK(requests[0].item == weapon && requests[1].item == *inventory.slotItems[1]);
        GUI_CHECK(requests[0].beforeVertex < requests[1].beforeVertex && requests[1].beforeVertex < vertices.size());
        GUI_CHECK(requests[0].at.size > 0 && requests[0].at.euler == glm::vec3(225, 0, 90) && requests[0].at.darkPass);
        inventory.hoverSlot = 0;
        hud.draw(state, vertices);
        GUI_CHECK(hud.itemDraws().size() == 2 && hud.itemDraws()[0].at.size == requests[0].at.size);
    }
    inventory.bagShown = false;
    hud.draw(state, vertices);
    GUI_CHECK(hud.itemDraws().empty());
    inventory.bagShown = true;
    inventory.slotItems[0] = nativesaves::Item{};
    inventory.slots[1] = "missing-owner-resource";
    hud.draw(state, vertices);
    GUI_CHECK(hud.itemDraws().empty());
    // The grid's canonical positions include holes; navigation includes the unused tail.
    inventory.slotItems = {};
    inventoryslots::Tabs tabs;
    tabs.resize(6);
    inventory.canonicalTabs = &tabs;
    inventory.tab = 2;
    const auto points = hud.inventoryNavigationPoints(inventory);
    const int capacity = (int)points.size() - 10;
    GUI_CHECK(capacity >= 3);
    if (capacity >= 3) {
        int columns = 1;
        while (columns < capacity && points[10 + columns].y == points[10].y)
            ++columns;
        const int rows = capacity / columns;
        const auto populate = [&](int tab, int length) {
            tabs[(size_t)tab].assign((size_t)length, nativesaves::Stack{1, weapon});
            inventory.items.clear();
            for (int i = 0; i < length; ++i) {
                Hud::Inventory::Entry entry;
                entry.icon = "synthetic-equipment";
                entry.name = "Synthetic";
                entry.tab = tab;
                entry.slotIndex = i;
                entry.original = weapon;
                inventory.items.push_back(entry);
            }
        };
        populate(2, capacity - 1);
        tabs[2][1].count = inventory.items[1].count = 0;
        tabs[2][2].count = inventory.items[2].count = -2;
        inventory.pointer = points[11];
        GUI_CHECK(hud.inventorySourceAt(points[11], inventory) == inventoryslots::Index{2, 1});
        GUI_CHECK(hud.inventorySourceAt(points.back(), inventory) == inventoryslots::Index{});
        hud.draw(state, vertices);
        GUI_CHECK(hud.itemDraws().size() == (size_t)capacity - 2);
        if (hud.itemDraws().size() >= 2) {
            GUI_CHECK(hud.itemDraws()[0].at.screen == points[10] && hud.itemDraws()[1].at.screen == points[12]);
            GUI_CHECK(hud.itemDraws()[1].item == weapon);
        }
        inventory.tab = 0;
        populate(0, capacity * 2 + 1);
        // A foreign region cuts drawing, but later matching slots still affect the range.
        const int32_t foreign = 7;
        std::memcpy(tabs[0][1].item.data() + nativesaves::item::REGION, &foreign, sizeof foreign);
        hud.draw(state, vertices);
        GUI_CHECK(hud.itemDraws().size() == 1);
        const int matching = (int)tabs[0].size() - 1;
        GUI_CHECK(hud.inventoryRowCount(inventory) == std::max(1, (matching - 1) / columns - rows + 2));
        inventory.scroll = 2; // the earlier mismatch must not cut a later viewport
        hud.draw(state, vertices);
        GUI_CHECK(hud.itemDraws().size() == (size_t)std::min(capacity, (int)tabs[0].size() - 2 * columns));
        for (auto &slot : tabs[0])
            slot.count = 0;
        GUI_CHECK(hud.inventoryRowCount(inventory) >= 1);
        tabs[0].clear();
        GUI_CHECK(hud.inventoryRowCount(inventory) == 0);
        inventory.tab = 6;
        GUI_CHECK(hud.inventoryRowCount(inventory) == 1);
        inventory.tab = 0;

        inventory.bagShown = false;
        inventory.vendor.open = true;
        inventory.vendor.items = {inventory.items[0]};
        inventory.vendor.items[0].count = 0;
        hud.draw(state, vertices);
        GUI_CHECK(hud.itemDraws().size() == 1); // shop icons admit zero quantity
        inventory.vendor.open = false;
        inventory.crafting.open = true;
        inventory.crafting.items = inventory.vendor.items;
        hud.draw(state, vertices);
        GUI_CHECK(hud.itemDraws().size() == 1); // crafting does too
        auto &details = inventory.crafting.details;
        details.valid = true;
        details.original = weapon;
        details.icon = "synthetic-equipment";
        details.ingredients.push_back({"synthetic-equipment", 0, 20, weapon});
        state.guiStepMs = 50;
        hud.draw(state, vertices);
        GUI_CHECK(hud.itemDraws().size() == 3);
        if (hud.itemDraws().size() == 3) {
            GUI_CHECK(hud.itemDraws()[1].at.euler == glm::vec3(-120, 0, 1) && !hud.itemDraws()[1].at.darkPass);
            GUI_CHECK(hud.itemDraws()[2].at.euler == glm::vec3(225, 0, 90) && hud.itemDraws()[2].at.darkPass);
            GUI_CHECK(hud.itemDraws()[1].beforeVertex <= hud.itemDraws()[2].beforeVertex);
        }
        details.icon = "missing-owner-resource";
        hud.draw(state, vertices);
        GUI_CHECK(hud.itemDraws().size() == 1);
        inventory.crafting.open = false;
        state.guiStepMs = 1000;
        hud.draw(state, vertices); // no hidden-time catch-up
        inventory.crafting.open = true;
        details.icon = "synthetic-equipment";
        state.guiStepMs = 25;
        hud.draw(state, vertices);
        GUI_CHECK(hud.itemDraws().size() == 3 && hud.itemDraws()[1].at.euler.z == 1.5f);
        inventory.crafting.open = false;
        inventory.info.push_back({"Synthetic preview"});
        inventory.infoItem = weapon;
        inventory.infoIcon = "synthetic-equipment";
        hud.draw(state, vertices);
        GUI_CHECK(hud.itemDraws().size() == 1);
        if (hud.itemDraws().size() == 1)
            GUI_CHECK(hud.itemDraws()[0].item == weapon && hud.itemDraws()[0].at.screenDepth == -.05f &&
                      hud.itemDraws()[0].at.euler == glm::vec3(225, 0, 90) && hud.itemDraws()[0].beforeVertex < vertices.size());
    }
    state.inventory = nullptr;
    state.ring.shown = true;
    state.ring.radius = 200;
    state.ring.worldMilliseconds = 1000;
    state.ring.items = {{"synthetic-equipment", &weapon}, {"missing-owner-resource", &weapon}, {"synthetic-equipment", &weapon}};
    state.ring.name = "Synthetic";
    state.quickItem.icon = "synthetic-equipment";
    state.quickItem.original = weapon;
    hud.draw(state, vertices);
    const auto &ringRequests = hud.itemDraws();
    GUI_CHECK(ringRequests.size() == 3);
    if (ringRequests.size() == 3) {
        GUI_CHECK(ringRequests[0].kind == guiitem::Request::Kind::Ring && ringRequests[1].kind == guiitem::Request::Kind::Ring &&
                  ringRequests[2].kind == guiitem::Request::Kind::Widget);
        GUI_CHECK(ringRequests[0].beforeVertex == 0 && ringRequests[1].beforeVertex == 0 && ringRequests[2].beforeVertex > 0);
        GUI_CHECK(std::abs(ringRequests[0].at.euler.z - 36) < .00001f && ringRequests[2].at.euler.z == 90);
        const auto expected = guiitem::ringPlacement(2, 3, {1, 1, 1}, {1920, 1080}, 200, 0, 1, 1000);
        GUI_CHECK(ringRequests[1].at.screen == expected.screen); // absent resource leaves a hole rather than reducingN
    }
    renderer.shutdown();
}
} // namespace
int guiItemTests() {
    using namespace guiitem;
    failures = 0;
    equipmentCallbacks();
    plasma::GuiClock clock;
    clock.advance(1.99999999);
    GUI_CHECK(clock.step == 2 && clock.elapsed == 2); // float narrowing precedes integer truncation
    clock.advance(.9);
    GUI_CHECK(clock.step == 0 && clock.elapsed == 2);
    clock.advance(5000);
    GUI_CHECK(clock.step == 1000 && clock.elapsed == 1002);
    clock.elapsed = INT32_MAX - 2;
    clock.advance(5);
    GUI_CHECK(clock.elapsed == INT32_MIN + 2);
    clock.advance(-1.75);
    GUI_CHECK(clock.step == -1 && clock.elapsed == INT32_MIN + 1);
    glm::mat4 previewRoot(1);
    previewRoot[0][0] = 2;
    previewRoot[1][1] = 3;
    previewRoot[3] = {10, 20, 0, 2};
    const auto preview = previewPlacement(previewRoot, {4, 6}, 100, 2500);
    GUI_CHECK(preview.screen == glm::vec2(84, 49) && near(preview.size, .15f) && preview.screenDepth == -.05f && preview.darkPass);
    const auto result = recipePlacement({10, 20}, 200, .5f, {-120, 0, 37}, false);
    GUI_CHECK(result.screen == glm::vec2(110, 75) && near(result.size, .05f) && result.euler == glm::vec3(-120, 0, 37) && !result.darkPass);
    GUI_CHECK(near(recipePlacement({10, 20}, 200, .5f, {}, true).size, .075f));
    const auto ingredient = ingredientCell({-11.75f, -41.75f}, 100, 200, .5f, 3, {43, 20}, -1);
    GUI_CHECK(ingredient && ingredient->origin == glm::vec2(33, 20) && ingredient->icon.screen == glm::vec2(43, 30) &&
              ingredient->hovered && near(ingredient->icon.size, .01875f) && ingredient->icon.darkPass);
    GUI_CHECK(!ingredientCell({}, 40, 200, 1, 0, {}, 0));
    plx::Document document;
    plasma::Scene graph(document);
    auto *parent = graph.create(&graph.root());
    parent->local = glm::mat4(1);
    (*parent->local)[0] = {0, 2, 0, 0};
    (*parent->local)[1] = {-3, 0, 0, 0};
    (*parent->local)[3] = {13, 7, 0, 1};
    auto *child = graph.create(parent);
    child->local = glm::mat4(1);
    (*child->local)[0][0] = 4;
    (*child->local)[1][1] = 5;
    child->widget.emplace();
    child->widget->framePos = {7, -8};
    const auto localPointer = graph.widgetPointer(*child, graph.contentPosition(*child) + glm::vec2(-45, 16));
    GUI_CHECK(glm::length(localPointer - glm::vec2(2, 3)) < .00001f);
    const auto cell = gridCell({-11.75f, -41.75f}, .5f, {120, 41}, {0, 0}, 9, {-6, -21}, -1);
    GUI_CHECK(cell.origin == glm::vec2(-6, -21) && cell.extent == glm::vec2(60, 20.5f));
    GUI_CHECK(cell.icon.screen == glm::vec2(4, -11) && cell.hovered && near(cell.icon.size, .5f * .03f * 1.5f * .75f));
    GUI_CHECK(cell.icon.euler == glm::vec3(225, 0, phase(-1)) && cell.icon.darkPass);
    GUI_CHECK(!gridCell({-11.75f, -41.75f}, .5f, {120, 41}, {0, 0}, 9, {54, -21}, 0).hovered);
    const auto mirrored = gridCell({0, 0}, -1, {120, 41}, {1, 2}, 3, {-135, -132}, 0);
    GUI_CHECK(!mirrored.hovered && mirrored.extent == glm::vec2(-120, -41) && near(mirrored.icon.size, -.03f));
    GUI_CHECK(textColour(nativesaves::makeItem(23, 0), {}) == glm::vec4(1, .6f, .25f, 1));
    GUI_CHECK(textColour(nativesaves::makeItem(21, 0), {}) == glm::vec4(.7f, 1, .25f, 1));
    GUI_CHECK(textColour(nativesaves::makeItem(1, 13), {}) == glm::vec4(.5f, 1, 1, 1));
    auto unsignedRarity = nativesaves::makeItem(1, 0);
    unsignedRarity[nativesaves::item::RARITY] = 0xff;
    GUI_CHECK(textColour(unsignedRarity, {}) == glm::vec4(1, 1, 0, 1));
    GUI_CHECK(near(phase(-1), -0.036f) && phase(10000) == 0 && near(phase(2500), 90));
    const auto sprite = spritePlacement({30.25f, -20.5f}, {100, 40}, 2500);
    GUI_CHECK(sprite.screen == glm::vec2(80.25f, -0.5f) && near(sprite.size, 0.06f) && sprite.euler == glm::vec3(225, 0, 90) &&
              sprite.darkPass && sprite.screenDepth == 0);
    GUI_CHECK(near(spritePlacement({0, 0}, {-100, 40}, -1).size, -0.06f)); // signed width, no absolute/height substitute
    const auto ringAt = ringPlacement(0, 4, {2, 4, 8}, {1001, 1001}, 200, 0, 1, 2500.75f);
    GUI_CHECK(std::abs(ringAt.screen.x - 500.5f) < 0.0001f && ringAt.screen.y == 300.5f && ringAt.euler == glm::vec3(225, 0, 90));
    GUI_CHECK(near(ringAt.size, ((1024.0f / 1001.0f) * .09f) / 8) && !ringAt.darkPass);
    const auto selectedAt = ringPlacement(1, 4, {2, 4, 8}, {1001, 1001}, 200, -90, 1, 0);
    GUI_CHECK(selectedAt.screen == ringAt.screen);
    const auto ringTransform = ringGeometry({2, 4, 8}, {1001, 1001}, ringAt, 960);
    GUI_CHECK(ringTransform);
    if (ringTransform) {
        const auto middle = ringTransform->model * glm::vec4(1, 2, 4, 1);
        const auto clip = ringTransform->projection * middle;
        GUI_CHECK(glm::length(middle - glm::vec4(0, 0, 1, 1)) < .000001f);
        GUI_CHECK(std::abs(clip.y / clip.w - (400.0f / 1001.0f)) < .000001f && clip.z / clip.w > .449f && clip.z / clip.w < .451f);
    }
    Placement at;
    at.screen = {800, 512};
    at.size = 0.1f;
    const auto one = geometry(3, {1, 1, 1}, {1600, 1024}, at);
    GUI_CHECK(one && near(one->model[0][0], 0.02f)); // small-Z increases divisor1 to5
    GUI_CHECK(one && one->projection[0][0] < 0 && one->projection[2][3] == 1);
    GUI_CHECK(one && std::bit_cast<uint32_t>(one->projection[2][2]) == 0x3f800347u);
    GUI_CHECK(one && std::bit_cast<uint32_t>(one->projection[3][2]) == 0xbdccd20bu);
    if (one) {
        const glm::vec4 centre = one->model * glm::vec4(0.5f, 0.5f, 0.5f, 1);
        GUI_CHECK(near(centre.x, 0) && near(centre.y, 0) && near(centre.z, 1));
        const auto projected = one->projection * centre;
        GUI_CHECK(near(projected.x / projected.w, 0) && near(projected.y / projected.w, 0));
    }
    const auto armour = geometry(4, {1, 1, 1}, {1600, 1024}, at);
    GUI_CHECK(armour && near(armour->model[0][0], 0.1f / 7.0f)); // trunc(5*1.5), not7.5
    const auto flat = geometry(3, {10, 20, 3}, {1600, 1024}, at);
    const auto solid = geometry(3, {10, 20, 4}, {1600, 1024}, at);
    GUI_CHECK(flat && solid && near(flat->model[0][0], 0.1f / 24) && near(solid->model[0][0], 0.1f / 20));
    at.screen = {400, 256};
    at.screenDepth = 0.2f;
    at.darkPass = true;
    const auto shifted = geometry(3, {1, 1, 1}, {1600, 1024}, at);
    if (shifted) {
        const auto p = shifted->projection * shifted->model * glm::vec4(0.5f, 0.5f, 0.5f, 1);
        GUI_CHECK(near(p.x / p.w, -0.5f) && near(p.y / p.w, 0.5f));
        GUI_CHECK(near(p.z / p.w, 1.10009f));
        GUI_CHECK(near(shifted->darkView[3][0], -0.01f) && near(shifted->darkView[3][1], -0.01f));
        GUI_CHECK(shifted->depthOffset == 0.1f && one && shifted->model == one->model);
    }
    at.darkPass = false;
    const auto ring = geometry(9, {2, 2, 2}, {1600, 1024}, at);
    GUI_CHECK(ring && near(ring->model[0][0], 0) && near(ring->model[0][1], 0) && near(ring->model[0][2], -0.1f / 6));
    GUI_CHECK(!geometry(3, {0, 1, 1}, {1600, 1024}, at) && !geometry(3, {1, 1, 1}, {1600, 0}, at));
    auto item = nativesaves::makeItem(0, 0, 1);
    GUI_CHECK(shininess(item) == 0);
    for (int material : {1, 11, 12, 13, 23})
        GUI_CHECK(shininess(nativesaves::makeItem(3, 0, material)) == 1);
    for (int material : {4, 29})
        GUI_CHECK(shininess(nativesaves::makeItem(3, 0, material)) == 0.5f);
    for (int material : {33, 35})
        GUI_CHECK(shininess(nativesaves::makeItem(3, 0, material)) == 0.25f);
    GUI_CHECK(shininess(nativesaves::makeItem(15, 0)) == 0 && shininess(nativesaves::makeItem(23, 0)) == 1);
    GUI_CHECK(shininess(nativesaves::makeItem(24, 18)) == 1 && shininess(nativesaves::makeItem(24, 19)) == 0);
    glm::mat4 basis;
    basis[0] = {2, 3, 4, 5};
    basis[1] = {6, 7, 8, 9};
    basis[2] = {10, 11, 12, 13};
    basis[3] = {14, 15, 16, 17};
    auto weapon = nativesaves::makeItem(3, 0);
    weapon[nativesaves::item::ATTRIBUTE_COUNT] = 2;
    weapon[0x1d] = uint8_t(-2);
    weapon[0x1e] = 3;
    weapon[0x1f] = uint8_t(-4);
    weapon[0x20] = 1;
    weapon[0x21] = 1;
    const auto cubes = spiritCubes(weapon, basis);
    GUI_CHECK(cubes.size() == 2 && cubes[0].model[3] == glm::vec4(-12, -14, -16, -18));
    GUI_CHECK(cubes.size() == 2 && cubes[1].model[3] == glm::vec4(16, 18, 20, 22));
    GUI_CHECK(cubes.size() == 2 && cubes[0].model[0] == basis[0] && cubes[0].model[2] == basis[2]);
    weapon[nativesaves::item::TYPE] = 15;
    GUI_CHECK(spiritCubes(weapon, basis).empty());
    weapon[nativesaves::item::TYPE] = 3;
    weapon[nativesaves::item::ATTRIBUTE_COUNT] = 1;
    weapon[0x1d] = weapon[0x1e] = weapon[0x1f] = 1;
    basis[0].x = 1e10f;
    basis[1].x = -1e10f;
    basis[2].x = 1;
    basis[3].x = 1;
    const auto grouped = spiritCubes(weapon, basis);
    GUI_CHECK(grouped.size() == 1 && grouped[0].model[3].x == 2); // XYZ sum before original translation
    Request request;
    request.mesh = 7;
    request.cubeMesh = 8;
    request.allocation = {1, 1, 1};
    request.item = weapon;
    request.at = at;
    request.at.darkPass = true;
    auto state = overlayState();
    state.white = 0.4f;
    state.glow = 0.25f;
    state.shininess = 0.5f;
    std::vector<Draw> draws;
    appendDraws(request, {1600, 1024}, state, draws);
    GUI_CHECK(draws.size() == 3 && draws[0].state.white == 0.4f && draws[0].state.glow == 0.25f && draws[0].state.shininess == 0.5f);
    GUI_CHECK(draws.size() == 3 && draws[0].material == glm::vec4(0, 0, 0, 1) && draws[0].depthOffset == 0);
    GUI_CHECK(draws.size() == 3 && draws[1].state.white == 0 && draws[1].state.glow == 0 && draws[1].depthOffset == 0.1f);
    GUI_CHECK(draws.size() == 3 && draws[2].mesh == 8 && draws[2].depthOffset == draws[1].depthOffset);
    const auto count = draws.size();
    request.mesh = 0;
    appendDraws(request, {1600, 1024}, state, draws);
    GUI_CHECK(draws.size() == count);
    request.kind = Request::Kind::Ring;
    request.mesh = 7;
    request.cubeMesh = 8;
    request.at.size = .05f;
    draws.clear();
    appendDraws(request, {1600, 1024}, state, draws);
    GUI_CHECK(draws.size() == 1 && draws[0].material == glm::vec4(1) && draws[0].view == glm::mat4(1) && draws[0].depthOffset == 0);
    GUI_CHECK(draws.size() == 1 && draws[0].state.direction == overlayState().direction && state.shininess == shininess(request.item));
    const auto light = lightDirection();
    GUI_CHECK(near(glm::length(light), 1) && near(light.y / light.x, 0.4f) && near(light.z, -light.x));
    std::printf("GUI item transforms: %s\n", failures ? "FAILED" : "PASS");
    return failures;
}
