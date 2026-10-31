#pragma once
// The VoxelWidget's retained draw/input state, from the independent customization specs.
#include "GuiItem.h"
#include "UpgradeWidget.h"
#include "VoxelEdit.h"
#include <array>
#include <functional>

namespace customization {
using Item = nativesaves::Item;
struct State {
    upgradewidget::Widget edit;
    glm::vec3 euler{-90, 0, 90};
    int paletteHover = -1, handHover = -1;
    Item detail{}, hoveredItem{};
    void toggleBench();
    void rotatePointer(glm::vec2 delta, float cameraSpeed);
    void rotatePad(double xDegrees, double zDegrees);
    upgradewidget::ClickResult press(Item *weapon, bool guiNode, bool &sound);
};
struct Layout {
    glm::mat4 root{1};
    glm::vec2 pivot{0}, parentSize{0};
    glm::ivec2 viewport{0};
    float scale = 1;
};
struct Resource {
    uint32_t mesh = 0;
    glm::ivec3 allocation{0};
    const CubModel *volume = nullptr;
};
using Resolve = std::function<Resource(const Item &)>;
struct Shown {
    State *state = nullptr;
    const inventoryslots::Inventory *inventory = nullptr;
    std::array<Item, 2> equipment{};
    std::function<std::string(const Item &)> icon;
    glm::vec2 pointer{0};
};
struct Plan {
    std::vector<guiitem::Request> images;
    std::optional<std::pair<int, int>> upgrades;
};
// Called only by an admitted owner draw. Clicks continue to use the preceding draw's state.
Plan draw(State &, const Layout &, const std::array<Item, 2> &equipment, const inventoryslots::Inventory &, glm::vec2 pointer,
          int32_t guiClock, const Resolve &, Resource cube);
bool consumePalette(inventoryslots::Inventory &, const Item &);
} // namespace customization
