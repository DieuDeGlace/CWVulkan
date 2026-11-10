// Mathematical checks derived from steam-gui-item-icons.md; no game asset is distributed.
#include "GuiItem.h"
#include "Assets.h"
#include "CubModel.h"
#include "Frontend.h"
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
    GUI_CHECK(hud.inventoryTabCount(2) == 4);
    hud.configureCraftingTabs(1);
    GUI_CHECK(hud.inventoryTabCount(2) == 6);
    hud.configureCraftingTabs(1);
    GUI_CHECK(hud.inventoryTabCount(2) == 6); // Replacement does not accumulate another six roots.
    hud.configureCraftingTabs(255);
    GUI_CHECK(hud.inventoryTabCount(2) == 4);
    hud.configureCraftingTabs(1);
    const auto *optionsRoot = hud.interfaceBranchRoot(uiroot::Branch::Options);
    const auto *gameplayRoot = hud.interfaceBranchRoot(uiroot::Branch::Gameplay);
    GUI_CHECK(optionsRoot && gameplayRoot && optionsRoot->parent == gameplayRoot->parent);
    Hud::ScreenItem control;
    control.kind = Hud::ScreenItem::Template;
    control.name = "button2";
    control.instance = "interface-branch-test";
    control.size = {200, 30};
    control.branch = uiroot::Branch::Options;
    hud.publishFrontendScreen({control});
    GUI_CHECK(optionsRoot && optionsRoot->children.size() == 1);
    if (optionsRoot && optionsRoot->children.size() == 1) {
        const auto *retainedControl = optionsRoot->children.front().get();
        GUI_CHECK(retainedControl->visible && retainedControl->parent == optionsRoot && !optionsRoot->visible);
        hud.publishFrontendScreen({});
        GUI_CHECK(!retainedControl->visible && optionsRoot->children.size() == 1);
        hud.publishFrontendScreen({control});
        GUI_CHECK(retainedControl == optionsRoot->children.front().get() && retainedControl->visible);
    }
    hud.publishFrontendScreen({});
    hud.setInterfaceState({});
    Renderer renderer;
    renderer.initHeadless(RenderSettings{});
    CubModel cub;
    cub.sx = cub.sy = cub.sz = 1;
    cub.rgb = {120, 80, 40};
    hud.addItemIcon("synthetic-equipment", cub);
    hud.setGuiScale(1);
    hud.layout(renderer, {1920, 1080});
    // CharacterStyleWidget effects belong to the admitted draw, never layout publication.
    {
        frontend::Look style;
        style.reset();
        int draws = 0, rebuilds = 0;
        glm::vec2 pointer{-100, -100};
        Hud::ScreenItem panel;
        panel.kind = Hud::ScreenItem::Template;
        panel.name = "blackwidget";
        panel.instance = "character-style-phase-test";
        panel.branch = uiroot::Branch::Creation;
        panel.size = frontend::LOOK_PANEL_SIZE;
        panel.drawModels = [&](const Hud::ScreenItem::ModelContext &) {
            ++draws;
            if (style.drawPalette(pointer)) ++rebuilds;
            return std::vector<guiitem::Request>{};
        };
        uiroot::State state;
        state.frontend[2] = true;
        hud.setInterfaceState(state);
        hud.publishFrontendScreen({panel});
        GUI_CHECK(draws == 0 && rebuilds == 0 && style.preview == (std::array<uint8_t, 3>{255, 255, 255}));
        std::vector<OverlayVertex> vertices;
        hud.drawScreen({panel}, vertices);
        GUI_CHECK(draws == 1 && rebuilds == 1 && style.preview == style.committed);
        pointer = {13, 197};
        hud.publishFrontendScreen({panel});
        GUI_CHECK(draws == 1 && rebuilds == 1 && style.preview == style.committed);
        hud.drawScreen({panel}, vertices);
        GUI_CHECK(draws == 2 && rebuilds == 2 && style.preview == (std::array<uint8_t, 3>{51, 0, 0}));
        hud.drawScreen({panel}, vertices);
        GUI_CHECK(draws == 3 && rebuilds == 2);
        hud.setInterfaceShown(false);
        pointer = {-100, -100};
        hud.drawScreen({panel}, vertices);
        GUI_CHECK(draws == 3 && rebuilds == 2 && style.preview == (std::array<uint8_t, 3>{51, 0, 0}));
        hud.setInterfaceShown(true);
        hud.setInterfaceState({}); // hidden creation ancestor, independent of principal visibility
        hud.drawScreen({panel}, vertices);
        GUI_CHECK(draws == 3 && rebuilds == 2);
        hud.setInterfaceState(state);
        const auto *branch = hud.interfaceBranchRoot(uiroot::Branch::Creation);
        GUI_CHECK(branch && !branch->children.empty());
        if (branch && !branch->children.empty()) {
            auto &owner = *branch->children.back();
            const auto flags = owner.flags;
            owner.flags |= plx::NODE_DISABLED;
            hud.drawScreen({panel}, vertices);
            GUI_CHECK(draws == 3 && rebuilds == 2);
            owner.flags = flags;
        }
        hud.drawScreen({panel}, vertices);
        GUI_CHECK(draws == 4 && rebuilds == 3 && style.preview == style.committed);
        hud.publishFrontendScreen({});
        hud.setInterfaceState({});
    }
    // A motion-only retained Button still receives its native virtual press/capture.
    // The port owns the template's current control, independently of the picked child.
    {
        Hud::ScreenItem thumb;
        thumb.kind=Hud::ScreenItem::Template;
        thumb.name="scrollbutton";
        thumb.instance="frontend-motion-owner-test";
        thumb.branch=uiroot::Branch::Controls;
        thumb.at={20,20};
        thumb.size={12,100};
        int motion=0;
        bool receiver=true;
        thumb.motion=[&](plasma::Input &input,plasma::Scene &,plasma::SceneNode &node) {
            receiver=receiver && &node==hud.frontendNode(thumb.instance);
            if (input.heldButtons() & 1) ++motion;
        };
        uiroot::State state;
        state.controls=true;
        hud.setInterfaceState(state);
        hud.publishFrontendScreen({thumb});
        const auto *node=hud.frontendNode(thumb.instance);
        GUI_CHECK(node && node->widget && node->widget->kind=="Button");
        if (node && node->widget) {
            const auto frame=hud.frontendFrame(thumb.instance);
            GUI_CHECK(frame.has_value());
            const auto centre=frame->position+frame->size*.5f;
            hud.moveInterfacePointer(centre);
            GUI_CHECK(motion==0);
            hud.interfaceLeftDown();
            GUI_CHECK(node->widget->buttonHeld && hud.interfacePointerTarget()==node);
            hud.setInterfaceShown(false);
            hud.moveInterfacePointer(centre+glm::vec2(500,500));
            GUI_CHECK(motion==1 && receiver && hud.interfacePointerTarget()==node);
            hud.interfaceLeftUp();
            GUI_CHECK(!node->widget->buttonHeld && !(hud.interfaceHeldButtons() & 1));
            hud.clearFrontendBindings();
            hud.setInterfaceShown(true);
            hud.moveInterfacePointer(centre);
            hud.interfaceLeftDown();
            hud.moveInterfacePointer(centre+glm::vec2(0,1));
            GUI_CHECK(motion==1); // No callback into a finished Session.
            hud.interfaceLeftUp();
        }
        hud.clearFrontendBindings();
        hud.publishFrontendScreen({});
        hud.setInterfaceState({});
    }
    // Help keeps its own selected value under the HUD across frontend/global hiding.
    {
        Hud::ScreenItem sheet;
        sheet.kind = Hud::ScreenItem::Template;
        sheet.name = "blackwidget";
        sheet.instance = "help-visibility-test";
        sheet.branch = uiroot::Branch::Help;
        sheet.size = {40, 40};
        std::vector<OverlayVertex> vertices;
        hud.setHelpShown(true);
        uiroot::State state;
        state.frontend[2] = true;
        hud.setInterfaceState(state);
        const auto *help = hud.interfaceBranchRoot(uiroot::Branch::Help);
        GUI_CHECK(help && help->visible && help->parent == hud.interfaceBranchRoot(uiroot::Branch::Gameplay));
        const size_t before = vertices.size();
        hud.drawScreen({sheet}, vertices);
        GUI_CHECK(vertices.size() == before && hud.helpShown());
        hud.setInterfaceState({});
        hud.drawScreen({sheet}, vertices);
        GUI_CHECK(vertices.size() > before && hud.helpShown());
        hud.setInterfaceShown(false);
        const size_t hidden = vertices.size();
        hud.drawScreen({sheet}, vertices);
        GUI_CHECK(vertices.size() == hidden && hud.helpShown());
        hud.setInterfaceShown(true);
        hud.setHelpShown(false);
        hud.publishFrontendScreen({});
    }
    // Window and controller callbacks share one stored mask, independent of physical polling.
    {
        MenuInput messages;
        using Message = MenuInput::InterfaceMessage;
        hud.moveInterfacePointer({-1000, -1000});
        GUI_CHECK(hud.interfaceHeldButtons() == 0 && !hud.interfacePointerTarget());
        messages.interfaceMessages = {Message::RightDown, Message::MiddleDown};
        const auto physical = messages.windowMouseEdges();
        GUI_CHECK(physical.pressed == std::vector<int>({1, 2}) && physical.released.empty());
        messages.routeInterfaceMessages(hud);
        GUI_CHECK(hud.interfaceHeldButtons() == 6);
        messages.gamepadPointer.pressed[1] = true;
        messages.routeGamepadButtons(hud); // A second source sets the same bit without a press count.
        GUI_CHECK(hud.interfaceHeldButtons() == 6);
        messages.gamepadPointer = {};
        messages.gamepadPointer.released[1] = true;
        messages.routeGamepadButtons(hud);
        GUI_CHECK(hud.interfaceHeldButtons() == 4); // Controller release clears the window's right-button bit.
        messages.gamepadPointer = {};
        messages.gamepadPointer.pressed[0] = true;
        messages.routeGamepadButtons(hud);
        GUI_CHECK(hud.interfaceHeldButtons() == 5 && messages.interfacePresses.size() == 1);
        messages.released = {0, 1, 2}; // GLFW focus-loss releases are not actual engine button-up messages.
        GUI_CHECK(messages.windowMouseEdges().released.empty());
        messages.routeInterfaceMessages(hud);
        messages.clear();
        GUI_CHECK(hud.interfaceHeldButtons() == 5);
        messages.interfaceMessages = {Message::MiddleUp, Message::LeftUp};
        messages.routeInterfaceMessages(hud);
        GUI_CHECK(hud.interfaceHeldButtons() == 0);
    }
    // Independent ordinary-wheel contract: each message reaches game routing
    // in order, using the then-current retained GUI coordinates and state.
    {
        MenuInput messages;
        using Message = MenuInput::InterfaceMessage;
        const glm::vec2 first(-1000, -1000), later(-900, -800);
        hud.moveInterfacePointer(first);
        messages.pointerX = 1234;
        messages.pointerY = 5678; // A newer adapter sample is not the retained GUI cursor.
        messages.interfaceMessages.push_back(Message::RightDown);
        messages.wheelMessage(360);
        messages.interfaceMessages.push_back(Message::MiddleDown);
        messages.wheelMessage(-120);
        messages.interfaceMessages.push_back(Message::RightUp);
        messages.wheelMessage(0);
        messages.interfaceMessages.push_back(Message::ToggleVisibility);
        messages.wheelMessage(1);
        messages.interfaceMessages.push_back(Message::ToggleVisibility);
        messages.interfaceMessages.push_back(Message::MiddleUp);
        const auto edges = messages.windowMouseEdges();
        GUI_CHECK(edges.pressed == std::vector<int>({1, 2}) && edges.released == std::vector<int>({1, 2}));
        struct Delivery { int delta; uint32_t held; bool visible; glm::vec2 point; };
        std::vector<Delivery> deliveries;
        messages.routeInterfaceMessages(hud, [&](int delta, glm::vec2 point) {
            deliveries.push_back({delta, hud.interfaceHeldButtons(), hud.interfaceShown(), point});
            if (deliveries.size() == 1)
                hud.moveInterfacePointer(later);
        });
        GUI_CHECK(deliveries.size() == 4);
        const Delivery expected[] = {{1, 2, true, first}, {-1, 6, true, later}, {0, 4, true, later}, {1, 4, false, later}};
        for (size_t i = 0; i < deliveries.size() && i < 4; ++i) {
            GUI_CHECK(deliveries[i].delta == expected[i].delta);
            GUI_CHECK(deliveries[i].held == expected[i].held && deliveries[i].visible == expected[i].visible);
            GUI_CHECK(deliveries[i].point == expected[i].point);
        }
        GUI_CHECK(hud.interfaceShown() && hud.interfaceHeldButtons() == 0 && messages.interfaceMessages.empty());
        messages.routeInterfaceMessages(hud, [&](int, glm::vec2) { GUI_CHECK(false); });
        std::vector<int> normalized;
        for (int16_t raw : {int16_t(32767), int16_t(-32768), int16_t(120), int16_t(0), int16_t(1), int16_t(-1), int16_t(360)})
            messages.wheelMessage(raw);
        messages.routeInterfaceMessages(hud, [&](int delta, glm::vec2 point) {
            normalized.push_back(delta);
            GUI_CHECK(point == later && hud.interfaceHeldButtons() == 0);
        });
        GUI_CHECK(normalized == std::vector<int>({1, -1, 1, 0, 1, -1, 1}));
        messages.wheelMessage(120);
        messages.clear();
        messages.routeInterfaceMessages(hud, [&](int, glm::vec2) { GUI_CHECK(false); });
    }
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
    GUI_CHECK(gameplayRoot && gameplayRoot->visible);
    hud.setInterfaceState({{true, false, false, false, false}});
    hud.draw(state, vertices);
    GUI_CHECK(gameplayRoot && !gameplayRoot->visible && hud.itemDraws().empty());
    hud.setInterfaceState({});
    inventory.bagShown = false;
    hud.draw(state, vertices);
    GUI_CHECK(hud.itemDraws().empty());
    inventory.bagShown = true;
    inventory.slotItems[0] = nativesaves::Item{};
    inventory.slots[1] = "missing-owner-resource";
    hud.draw(state, vertices);
    GUI_CHECK(hud.itemDraws().empty());
    // The grid's canonical positions include holes; navigation includes the unused tail.
    hud.addItemIcon("synthetic-late", cub);
    inventory.slotItems[0] = weapon;
    inventory.slots[0] = "synthetic-late";
    hud.prepareFrame(renderer, {1920, 1080});
    hud.draw(state, vertices);
    GUI_CHECK(hud.itemDraws().size() == 1 && hud.itemDraws()[0].item == weapon);
    hud.prepareFrame(renderer, {1920, 1080}); // repeated preparation retains the same uploaded resource
    hud.draw(state, vertices);
    GUI_CHECK(hud.itemDraws().size() == 1);
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
        inventory.nativeCollections = true;
        hud.draw(state, vertices);
        GUI_CHECK(hud.cachedInventoryCell(0) == inventoryslots::Index{2, 1});
        GUI_CHECK(hud.inventorySourceAt(points[11], inventory) == inventoryslots::Index{}); // Zero quantity.
        inventory.pointer = points[12];
        GUI_CHECK(hud.inventorySourceAt(points[12], inventory) == inventoryslots::Index{}); // Still the prior draw.
        hud.draw(state, vertices);
        GUI_CHECK(hud.inventorySourceAt(points[10], inventory) == inventoryslots::Index{2, 2});
        auto oldRows = std::move(tabs[2]);
        tabs[2].assign(oldRows.begin(), oldRows.begin() + 2);
        GUI_CHECK(hud.cachedInventoryCell(0) == inventoryslots::Index{2, 2});
        GUI_CHECK(hud.inventorySourceAt(points[10], inventory) == inventoryslots::Index{}); // Live bounds changed.
        tabs[2] = std::move(oldRows);
        GUI_CHECK(hud.inventorySourceAt({-1000, -1000}, inventory) == inventoryslots::Index{});
        inventory.bagShown = false;
        hud.draw(state, vertices);
        GUI_CHECK(hud.cachedInventoryCell(0) == inventoryslots::Index{2, 2});
        GUI_CHECK(hud.inventorySourceAt(points[10], inventory) == inventoryslots::Index{});
        inventory.bagShown = true;
        inventory.canonicalTabs = nullptr;
        hud.draw(state, vertices);
        GUI_CHECK(hud.cachedInventoryCell(0) == inventoryslots::Index{2, 2});
        inventory.canonicalTabs = &tabs;
        inventory.pointer = points.back();
        hud.draw(state, vertices);
        GUI_CHECK(hud.cachedInventoryCell(0) == inventoryslots::Index{}); // Unoccupied tail cell.

        // Find interior points in two owner tabs above the first grid cell.
        const auto tabPoint = [&](int tab) -> std::optional<glm::vec2> {
            for (int y = -68; y >= -120; y -= 4)
                for (int x = -24; x <= 110; x += 4) {
                    const auto point = points[10] + glm::vec2(x, y);
                    if (hud.inventoryTabAt(point, inventory) == tab &&
                        hud.inventoryTabAt(point - glm::vec2(8), inventory) == tab &&
                        hud.inventoryTabAt(point + glm::vec2(8), inventory) == tab)
                        return point;
                }
            return {};
        };
        const auto firstTab = tabPoint(0), secondTab = tabPoint(1);
        GUI_CHECK(firstTab && secondTab);
        if (firstTab && secondTab) {
            const auto tabForPress = [&](glm::vec2 point) {
                const auto press = hud.nativeInventoryTabForPress(point, inventory);
                return press && press->panel == 0 ? press->tab : -1;
            };
            GUI_CHECK(tabForPress(*firstTab) == -1);
            inventory.pointer = *firstTab;
            hud.draw(state, vertices);
            GUI_CHECK(tabForPress(*secondTab) == 0);
            GUI_CHECK(tabForPress(points[10]) == -1);
            inventory.pointer = points[10];
            hud.draw(state, vertices); // Missing all tabs does not clear the retained ordinal.
            GUI_CHECK(tabForPress(*secondTab) == 0);
            inventory.canonicalTabs = nullptr;
            hud.draw(state, vertices);
            GUI_CHECK(tabForPress(*secondTab) == -1);
            inventory.canonicalTabs = &tabs;
            GUI_CHECK(tabForPress(*secondTab) == 0);
            inventory.tab = 0;
            GUI_CHECK(tabForPress(*secondTab) == -1);
            inventory.tab = 2;

            control.at = *secondTab - glm::vec2(100, 30);
            hud.publishFrontendScreen({control});
            uiroot::State overlays;
            overlays.options = true;
            hud.setInterfaceState(overlays);
            GUI_CHECK(tabForPress(*secondTab) == -1); // The common root picks the foreground control.
            hud.setInterfaceState({});
            GUI_CHECK(tabForPress(*secondTab) == 0); // Hidden foreground controls do not take the hit.
            hud.setInterfaceShown(false);
            GUI_CHECK(!hud.interfaceShown() && gameplayRoot && gameplayRoot->visible);
            GUI_CHECK(tabForPress(*secondTab) == -1); // Fresh picking stops at the hidden principal root.
            hud.setInterfaceState(overlays);
            GUI_CHECK(!hud.interfaceShown() && optionsRoot && optionsRoot->visible);
            // Wheel eligibility starts at the panel subtree; neither an unrelated
            // foreground control nor hidden ancestors replace that fresh pick.
            GUI_CHECK(hud.inventoryWheelTarget(*secondTab) == 0);
            const bool gameplayVisible = gameplayRoot->visible;
            const_cast<plasma::SceneNode *>(gameplayRoot)->visible = false;
            GUI_CHECK(hud.inventoryWheelTarget(*secondTab) == 0);
            hud.setInventoryPanelVisibility(false, false, false);
            GUI_CHECK(hud.inventoryWheelTarget(*secondTab) == -1);
            const_cast<plasma::SceneNode *>(gameplayRoot)->visible = gameplayVisible;
            hud.setInventoryPanelVisibility(true, false, false);
            hud.setInterfaceState({});
            hud.setInterfaceShown(true);
            GUI_CHECK(tabForPress(*secondTab) == 0); // No child selection or tab hover was cleared.
            hud.publishFrontendScreen({});

            MenuInput messages;
            using Message = MenuInput::InterfaceMessage;
            hud.moveInterfacePointer(*secondTab);
            messages.interfaceMessages = {Message::LeftDown, Message::LeftUp, Message::ToggleVisibility, Message::LeftDown,
                                          Message::LeftUp, Message::ToggleVisibility, Message::LeftDown, Message::LeftUp};
            messages.routeInterfaceMessages(hud);
            hud.moveInterfacePointer(points[10]);
            GUI_CHECK(messages.interfaceMessages.empty() && messages.interfacePresses.size() == 3 && hud.interfaceShown());
            if (messages.interfacePresses.size() == 3) {
                const auto &first = messages.interfacePresses[0], &hidden = messages.interfacePresses[1],
                           &last = messages.interfacePresses[2];
                GUI_CHECK(first.point == *secondTab && first.tab && first.tab->panel == 0 && first.tab->tab == 0);
                GUI_CHECK(hidden.point == *secondTab && !hidden.tab);
                GUI_CHECK(last.point == *secondTab && last.tab && last.tab->tab == 0);
            }
            messages.interfaceMessages = {Message::LeftDown, Message::LeftUp};
            messages.routeInterfaceMessages(hud);
            GUI_CHECK(messages.interfacePresses.size() == 1 && messages.interfacePresses[0].point == points[10] &&
                      !messages.interfacePresses[0].tab);
        }
        inventory.nativeCollections = false;
        inventory.pointer = points[11];
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
        GUI_CHECK(hud.inventoryRowCount(inventory) == 1); // Drawing did not refresh constructor A.
        hud.refreshInventoryRange(inventory);
        GUI_CHECK(hud.inventoryRowCount(inventory) == std::max(1, (matching - 1) / columns - rows + 2));
        const auto bag = std::find_if(gameplayRoot->children.begin(), gameplayRoot->children.end(), [](const auto &child) {
            return child->widget && child->widget->kind == "InventoryWidget";
        });
        const auto *thumb = bag != gameplayRoot->children.end() ? (*bag)->find("scrollbutton") : nullptr;
        GUI_CHECK(thumb && thumb->widget && thumb->widget->kind == "Button");
        if (thumb && thumb->widget) {
            inventory.nativeCollections = true;
            inventory.scroll = 0;
            inventory.retainScroll();
            const int available = inventory.availableRows;
            hud.updateInventoryThumb(inventory, 0, available);
            const plx::Document emptyDocument;
            const plasma::Scene geometry(emptyDocument);
            const auto start = geometry.contentPosition(*thumb);
            const auto size = geometry.screenSize(*thumb);
            const auto centre = start + size * 0.5f;
            hud.moveInterfacePointer(centre, &inventory);
            const auto press = hud.interfaceLeftDown();
            GUI_CHECK(press.thumbCaptured && thumb->widget->buttonHeld && !press.tab);
            GUI_CHECK(hud.interfacePointerTarget() == thumb && (hud.interfaceHeldButtons() & 1));
            hud.moveInterfacePointer(centre + glm::vec2(0, 11.5f), &inventory);
            const auto moved = geometry.contentPosition(*thumb);
            GUI_CHECK(near(moved.x, start.x) && near(moved.y, start.y + 11.5f));
            GUI_CHECK(geometry.screenSize(*thumb) == size && inventory.availableRows == available);
            hud.setInterfaceShown(false);
            const auto outside = centre + glm::vec2(450, 800);
            hud.moveInterfacePointer(outside, &inventory);
            const auto bottom = geometry.contentPosition(*thumb);
            GUI_CHECK(inventory.scroll == available - 1 && inventory.rowOffsets[0] == inventory.scroll && bottom.y > moved.y);
            GUI_CHECK(hud.interfacePointerTarget() == thumb); // Capture bypasses the hidden root's fresh-pick rejection.
            GUI_CHECK(hud.inventoryWheelTarget(outside) == -1 && (hud.interfaceHeldButtons() & 1));
            GUI_CHECK(hud.inventoryWheelTarget(centre) == 0 && hud.interfacePointerTarget() == thumb);
            GUI_CHECK(hud.interfacePointerDelta() == outside - (centre + glm::vec2(0, 11.5f)));

            // GLFW's synthetic releases stay in its ordinary per-frame list.
            MenuInput messages;
            messages.released = {0};
            messages.routeInterfaceMessages(hud);
            GUI_CHECK(thumb->widget->buttonHeld);
            messages.interfaceMessages = {MenuInput::InterfaceMessage::LeftUp};
            messages.routeInterfaceMessages(hud);
            GUI_CHECK(!hud.interfacePointerTarget() && !(hud.interfaceHeldButtons() & 1)); // Hidden uncaptured root picks nothing.
            hud.setInterfaceShown(true);
            hud.moveInterfacePointer(centre, &inventory);
            GUI_CHECK(!thumb->widget->buttonHeld && geometry.contentPosition(*thumb) == bottom);
            const auto padCentre = geometry.contentPosition(*thumb) + size * .5f;
            hud.moveInterfacePointer(padCentre, &inventory);
            MenuInput controller;
            controller.gamepadPointer.pressed[0] = true;
            controller.routeGamepadButtons(hud);
            GUI_CHECK(controller.interfacePresses.size() == 1 && controller.interfacePresses[0].thumbCaptured &&
                      hud.interfacePointerTarget() == thumb && thumb->widget->buttonHeld);
            controller.gamepadPointer = {};
            controller.gamepadPointer.released[0] = true;
            controller.routeGamepadButtons(hud);
            GUI_CHECK(!thumb->widget->buttonHeld && !(hud.interfaceHeldButtons() & 1));

            inventory.nativeCollections = false;
        }
        inventory.scroll = 2; // the earlier mismatch must not cut a later viewport
        hud.draw(state, vertices);
        GUI_CHECK(hud.itemDraws().size() == (size_t)std::min(capacity, (int)tabs[0].size() - 2 * columns));
        for (auto &slot : tabs[0])
            slot.count = 0;
        hud.refreshInventoryRange(inventory);
        GUI_CHECK(hud.inventoryRowCount(inventory) >= 1);
        tabs[0].clear();
        hud.draw(state, vertices);
        GUI_CHECK(hud.inventoryRowCount(inventory) >= 1); // Empty draw preserves the prior range.
        hud.refreshInventoryRange(inventory);
        GUI_CHECK(hud.inventoryRowCount(inventory) == 0 && inventory.scroll == 0);
        inventory.tab = 6;
        hud.refreshInventoryRange(inventory);
        GUI_CHECK(hud.inventoryRowCount(inventory) == 1);
        inventory.tab = 0;

        inventory.bagShown = false;
        inventory.vendor.open = true;
        inventory.vendor.items = {inventory.items[0]};
        inventory.vendor.items[0].count = 0;
        hud.draw(state, vertices);
        GUI_CHECK(hud.itemDraws().size() == 1); // shop icons admit zero quantity
        inventoryslots::Tabs vendorTabs(2);
        vendorTabs[0].assign(300, nativesaves::Stack{-1, weapon});
        inventory.vendor.canonicalTabs = &vendorTabs;
        inventory.vendor.items[0].slotIndex = 0;
        hud.refreshInventoryRange(inventory, 1);
        const int vendorRows = hud.inventoryRowCount(inventory, 1);
        GUI_CHECK(vendorRows > 1 && inventory.vendor.rowOffsets.size() == 2);
        vendorTabs[0].clear();
        hud.draw(state, vertices);
        GUI_CHECK(hud.inventoryRowCount(inventory, 1) == vendorRows);
        hud.refreshInventoryRange(inventory, 1);
        GUI_CHECK(hud.inventoryRowCount(inventory, 1) == 0);
        inventory.vendor.canonicalTabs = nullptr;
        inventory.vendor.open = false;
        inventory.crafting.open = true;
        inventory.crafting.items = inventory.vendor.items;
        hud.draw(state, vertices);
        GUI_CHECK(hud.itemDraws().size() == 1); // crafting does too
        // A bound derived collection counts physical rows independently of its
        // presentation entries and quantities, just like the ordinary bag source.
        inventoryslots::Tabs craftingTabs(7);
        craftingTabs[0].assign(300, nativesaves::Stack{0, weapon});
        inventory.crafting.canonicalTabs = &craftingTabs;
        hud.refreshInventoryRange(inventory, 2);
        const int craftingRows = hud.inventoryRowCount(inventory, 2);
        GUI_CHECK(craftingRows > 1);
        const auto savedCraftingEntries = inventory.crafting.items;
        inventory.crafting.items.clear();
        GUI_CHECK(hud.inventoryRowCount(inventory, 2) == craftingRows);
        inventory.crafting.items = savedCraftingEntries;
        inventory.crafting.items[0].slotIndex = 0;
        inventory.crafting.items[0].count = -7;
        hud.draw(state, vertices);
        GUI_CHECK(hud.itemDraws().size() == 1);
        craftingTabs[0].clear();
        GUI_CHECK(hud.inventoryRowCount(inventory, 2) == craftingRows);
        hud.refreshInventoryRange(inventory, 2);
        GUI_CHECK(hud.inventoryRowCount(inventory, 2) == 0);
        inventory.crafting.canonicalTabs = nullptr;
        inventory.crafting.items = savedCraftingEntries;
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
        // Formula hover is a panel-owned draw publication, not a fresh selector hit.
        // Exercise the real owner scene with synthetic items and resources only.
        state.guiStepMs = 0;
        const auto *formula = &hud.formulaPreviewItem();
        GUI_CHECK((*formula)[0] == 0);
        auto result = nativesaves::makeItem(3, 4, 17);
        result[1] = 0xa5; // Assignment must leave destination padding alone.
        auto expectedResult = result;
        expectedResult[1] = 0;
        auto ingredient = nativesaves::makeItem(11, 2, 29);
        ingredient[2] = 0xb6;
        auto expectedIngredient = ingredient;
        expectedIngredient[2] = 0;
        details.original = result;
        details.ingredients[0].original = ingredient;
        if (hud.itemDraws().size() == 3) {
            const auto resultCentre = hud.itemDraws()[1].at.screen;
            const auto ingredientCentre = hud.itemDraws()[2].at.screen;
            inventory.pointer = resultCentre;
            hud.draw(state, vertices);
            GUI_CHECK(hud.formulaPreviewItem() == expectedResult);
            details.original->at(4) ^= 0x7f;
            GUI_CHECK(hud.formulaPreviewItem() == expectedResult); // Source replacement cannot change the owned copy.
            itempreview::Sources sources;
            sources.formulaShown = true;
            sources.formula = formula;
            sources.voxel = &weapon;
            const auto selected = itempreview::select(sources);
            GUI_CHECK(selected.item == formula);
            bool lateRead = false;
            state.informationDraw = [&] {
                lateRead = selected.item == &hud.formulaPreviewItem() && *selected.item == expectedIngredient;
            };
            inventory.pointer = ingredientCentre;
            hud.draw(state, vertices);
            GUI_CHECK(lateRead && hud.formulaPreviewItem() == expectedIngredient);
            state.informationDraw = {};
            details.ingredients[0].icon = "missing-owner-resource";
            hud.draw(state, vertices);
            GUI_CHECK(hud.itemDraws().size() == 2 && hud.formulaPreviewItem() == expectedIngredient);
            details.ingredients[0].icon = "synthetic-equipment";
            state.interfaceVisible = false;
            inventory.pointer = {-1000, -1000};
            details.ingredients[0].original->at(4) ^= 0x31;
            hud.draw(state, vertices);
            GUI_CHECK(hud.formulaPreviewItem() == expectedIngredient && itempreview::select(sources).item == formula);
            state.interfaceVisible = true;
            hud.setInterfaceShown(false);
            hud.draw(state, vertices);
            GUI_CHECK(hud.formulaPreviewItem() == expectedIngredient && itempreview::select(sources).item == formula);
            hud.setInterfaceShown(true);
            details.valid = false;
            hud.draw(state, vertices);
            GUI_CHECK(hud.formulaPreviewItem() == expectedIngredient);
            details.valid = true;
            details.icon = "missing-owner-resource";
            hud.draw(state, vertices);
            GUI_CHECK(hud.formulaPreviewItem() == expectedIngredient);
            details.icon = "synthetic-equipment";
            inventory.crafting.open = false;
            hud.draw(state, vertices);
            GUI_CHECK(hud.formulaPreviewItem() == expectedIngredient);
            sources.formulaShown = false;
            GUI_CHECK(itempreview::select(sources).item == &weapon);
            inventory.crafting.open = true;
            sources.formulaShown = true;
            hud.draw(state, vertices);
            auto cleared = expectedIngredient;
            cleared[0] = 0; // An admitted miss clears category, retaining the other semantic bytes.
            GUI_CHECK(hud.formulaPreviewItem() == cleared && itempreview::select(sources).item == nullptr);
            GUI_CHECK(formula == &hud.formulaPreviewItem());
            // Result edges exclude hits; ingredient left/top admit and right/bottom exclude.
            for (const auto offset : {glm::vec2(-60, 0), glm::vec2(60, 0), glm::vec2(0, -60), glm::vec2(0, 60)}) {
                inventory.pointer = resultCentre + offset;
                hud.draw(state, vertices);
                GUI_CHECK(hud.formulaPreviewItem()[0] == 0);
            }
            details.ingredients[0].original = ingredient;
            inventory.pointer = ingredientCentre - glm::vec2(20);
            hud.draw(state, vertices);
            GUI_CHECK(hud.formulaPreviewItem() == expectedIngredient);
            inventory.pointer = ingredientCentre + glm::vec2(20, 0);
            hud.draw(state, vertices);
            GUI_CHECK(hud.formulaPreviewItem()[0] == 0);
            inventory.pointer = ingredientCentre + glm::vec2(0, 20);
            hud.draw(state, vertices);
            GUI_CHECK(hud.formulaPreviewItem()[0] == 0);
        }
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
    // Each admitted InventoryWidget samples the same embedded stack at its own tail.
    inventory = Hud::Inventory{};
    nativesaves::Stack retained{-7, {}}; // Category0 and negative quantity are both admitted.
    inventory.retainedCursor = &retained;
    bool originalAddress = true;
    inventory.retainedSprite = [&](const nativesaves::Item &item) {
        originalAddress &= &item == &retained.item;
        return std::string("synthetic-equipment");
    };
    inventory.pointer = {-17.75f, 29.125f}; // No pointer-inside or integer snapping gate.
    inventory.vendor.open = inventory.crafting.open = true;
    state.guiClockMs = -1;
    for (const float scale : {1.f, 1.5f}) {
        hud.setGuiScale(scale);
        hud.layout(renderer, {1920, 1080});
        retained.count = 0;
        hud.draw(state, vertices);
        const auto flatCount = vertices.size();
        GUI_CHECK(hud.itemDraws().empty());
        retained.count = -7;
        hud.draw(state, vertices);
        GUI_CHECK(hud.itemDraws().size() == 3 && vertices.size() == flatCount); // No cursor quantity label.
        size_t previous = 0;
        for (const auto &request : hud.itemDraws()) {
            GUI_CHECK(request.item == retained.item && request.kind == guiitem::Request::Kind::Widget);
            GUI_CHECK(request.at.screen == inventory.pointer && request.at.size == 0.05f && request.at.screenDepth == 0 &&
                      request.at.darkPass && request.at.euler == glm::vec3(225, 0, guiitem::phase(-1)));
            GUI_CHECK(request.beforeVertex >= previous && request.beforeVertex < vertices.size());
            previous = request.beforeVertex;
        }
    }
    GUI_CHECK(originalAddress);
    inventory.vendor.open = inventory.crafting.open = false;
    inventory.canonicalTabs = &tabs;
    inventory.tab = 9999;
    retained.item = weapon;
    hud.draw(state, vertices);
    GUI_CHECK(hud.itemDraws().size() == 1 && hud.itemDraws()[0].item == weapon); // High tab still reaches the cursor tail.
    inventory.bagShown = false;
    hud.draw(state, vertices);
    GUI_CHECK(hud.itemDraws().empty() && retained.count == -7);
    inventory.bagShown = true;
    inventory.retainedSprite = [](const nativesaves::Item &) { return std::string("missing-owner-resource"); };
    hud.draw(state, vertices);
    GUI_CHECK(hud.itemDraws().empty() && retained.count == -7 && retained.item == weapon);
    inventory.retainedCursor = nullptr;
    hud.draw(state, vertices);
    GUI_CHECK(hud.itemDraws().empty());
    hud.setGuiScale(1);
    hud.layout(renderer, {1920, 1080});
    state.guiClockMs = 2500;
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
    hud.discardRendererBindings();
    renderer.shutdown();
    GUI_CHECK(hud.ensureLoaded(assets.gameDir(), error));
    GUI_CHECK(hud.interfaceBranchRoot(uiroot::Branch::Gameplay) == gameplayRoot);
    GUI_CHECK(hud.interfaceBranchRoot(uiroot::Branch::Options) == optionsRoot);
    Renderer nextRenderer;
    nextRenderer.initHeadless(RenderSettings{});
    hud.prepareFrame(nextRenderer, {1920, 1080});
    hud.draw(state, vertices);
    GUI_CHECK(hud.itemDraws().empty()); // No requests may reference the destroyed renderer.
    hud.addItemIcon("synthetic-equipment", cub);
    hud.prepareFrame(nextRenderer, {1920, 1080});
    hud.draw(state, vertices);
    GUI_CHECK(hud.itemDraws().size() == 3);
    nextRenderer.shutdown();
    GUI_CHECK(hud.load(assets.gameDir(), error)); // Explicit reload retires the old input owner before the scene.
    const auto *reloadedOptions = hud.interfaceBranchRoot(uiroot::Branch::Options);
    GUI_CHECK(reloadedOptions && reloadedOptions->children.empty());
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
    const auto cursor = cursorPlacement({-17.75f, 29.125f}, -1);
    GUI_CHECK(cursor.screen == glm::vec2(-17.75f, 29.125f) && cursor.euler == glm::vec3(225, 0, phase(-1)) && cursor.darkPass &&
              std::bit_cast<uint32_t>(cursor.size) == 0x3d4ccccd && cursor.screenDepth == 0);
    const auto sprite = spritePlacement({30.25f, -20.5f}, {100, 40}, 2500);
    GUI_CHECK(sprite.screen == glm::vec2(80.25f, -0.5f) && near(sprite.size, 0.06f) && sprite.euler == glm::vec3(225, 0, 90) &&
              sprite.darkPass && sprite.screenDepth == 0);
    GUI_CHECK(near(spritePlacement({0, 0}, {-100, 40}, -1).size, -0.06f)); // signed width, no absolute/height substitute
    const auto ringAt = ringPlacement(0, 4, {2, 4, 8}, {1001, 1001}, 200, 0, 1, 2500.75f);
    GUI_CHECK(std::abs(ringAt.screen.x - 500.5f) < 0.0001f && ringAt.screen.y == 300.5f && ringAt.euler == glm::vec3(225, 0, 90));
    GUI_CHECK(near(ringAt.size, ((1024.0f / 1001.0f) * .09f) / 8) && !ringAt.darkPass);
    const auto selectedAt = ringPlacement(1, 4, {2, 4, 8}, {1001, 1001}, 200, -90, 1, 0);
    GUI_CHECK(selectedAt.screen == ringAt.screen);
    const auto ringTransform = queuedGeometry({2, 4, 8}, {1001, 1001}, ringAt, 960);
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
    request.kind = Request::Kind::RawEdit;
    request.allocation = {0, 0, 0}; // no implicit resource normalization at this helper
    request.model[3] = {3, 4, 5, 1};
    request.material = {.7f, .2f, .1f, .5f};
    draws.clear();
    appendDraws(request, {1600, 1024}, state, draws);
    GUI_CHECK(draws.size() == 1 && draws[0].model == request.model && draws[0].material == request.material && draws[0].depthOffset == 0);
    GUI_CHECK(state.front == glm::vec4(1.1f, 1.1f, .9f, 0) && state.back == glm::vec4(.5f, .6f, .8f, 1) &&
              state.ambient == glm::vec4(.4f, .4f, .4f, 1) && state.white == 0 && state.shininess == 0 && state.glow == 0);
    request.kind = Request::Kind::Widget;
    request.allocation = {1, 1, 1};
    request.at.darkPass = true;
    appendDraws(request, {1600, 1024}, state, draws);
    GUI_CHECK(draws.size() >= 2 && draws[1].state.front == editState().front); // later black pass inherits the raw palette
    request.kind = Request::Kind::Character;
    request.material = {1, 1, 1, .25f};
    request.opacity = 1;
    request.mirrored = true;
    request.depthOffset = .125f;
    draws.clear();
    appendDraws(request, {1600, 1024}, state, draws);
    GUI_CHECK(draws.size() == 1 && draws[0].material.a == .25f && draws[0].opacity == 1 && draws[0].mirrored &&
              draws[0].depthOffset == .125f && state.whiteSky);
    GUI_CHECK(state.front == glm::vec4(1) && state.back == glm::vec4(.2f, .2f, .2f, 1) && state.ambient == glm::vec4(.4f, .4f, .4f, 1) &&
              state.direction.y > 0 && state.direction.z < 0);
    request.mesh = 0;
    state = overlayState();
    draws.clear();
    appendDraws(request, {1600, 1024}, state, draws);
    GUI_CHECK(draws.empty() && state.whiteSky && state.front == glm::vec4(1));
    request.kind = Request::Kind::Widget;
    appendDraws(request, {1600, 1024}, state, draws);
    GUI_CHECK(draws.empty() && state.whiteSky && state.front == glm::vec4(1)); // null item resource skips setup
    Request first, second, third, widget;
    first.kind = third.kind = Request::Kind::QueuedModel;
    second.kind = Request::Kind::Ring;
    first.mesh = third.mesh = 9;
    second.mesh = 3;
    first.material.x = .25f;
    third.material.x = .75f;
    std::vector<Request> queue{first, second, third, widget, second};
    orderScreenQueue(queue, 3);
    GUI_CHECK(queue[0].mesh == 3 && queue[1].mesh == 9 && queue[1].material.x == .25f && queue[2].material.x == .75f);
    GUI_CHECK(queue[3].kind == Request::Kind::Widget && queue[4].mesh == 3); // later callbacks retain their stream order
    queue = {first, second, third};
    orderScreenQueue(queue, 1);
    GUI_CHECK(queue[0].mesh == 9 && queue[1].mesh == 3); // a clip/cache cut inside the prefix prevents reordering
    second.beforeVertex = 6;
    queue = {first, second, third};
    orderScreenQueue(queue, 3);
    GUI_CHECK(queue[0].mesh == 9 && queue[1].mesh == 3); // queued-looking callbacks after Plasma are not scene submissions
    const auto light = lightDirection();
    GUI_CHECK(near(glm::length(light), 1) && near(light.y / light.x, 0.4f) && near(light.z, -light.x));
    std::printf("GUI item transforms: %s\n", failures ? "FAILED" : "PASS");
    return failures;
}
