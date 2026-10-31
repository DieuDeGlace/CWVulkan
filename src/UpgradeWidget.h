#pragma once
// The weapon upgrade widget (docs/specs/loot-crafting.md §4; the widget's click routine
// 0x292650, its drawing 0x292990): the player sets material cubes from the inventory into the
// voxels of a worn weapon. Each cube is an attribute entry of the item record (x, y, z and a
// material from +0x1d, the count at +0x9d), which the weapon damage reads as its upgrade count
// and the creature pose draws as a coloured voxel on the weapon (creature-pose-items.md §9.1).
//
// The native owner window and retained draw/input state are in VoxelCustomization/HudCustomization.
// The orthographic picture and layout below remain for older recording-only sessions.
#include "CubModel.h"
#include "GameShop.h"
#include "Hud.h"

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace upgradewidget {

using Item = gameshop::Item;

// The widget's hand (widget +0x2c0): the right weapon (0) or the left (1).
constexpr int RIGHT_HAND = 0, LEFT_HAND = 1;
// The material cubes the palette lists: type 11, subtype 10, any material.
constexpr uint8_t CUBE_TYPE = 11;
constexpr int32_t CUBE_SUBTYPE = 10;
// The widget places nothing on a weapon holding this many cubes or more (0x292650). The weapon's
// own cap (16, or 32 two-handed: gamecraft::cubeCap) is the place routine's.
constexpr int WIDGET_LIMIT = 32;

// Full picked/carried coordinates. Placement stores low bytes; removal compares full integers.
struct Voxel {
    int32_t x = 0, y = 0, z = 0;
    bool operator==(const Voxel &o) const { return x == o.x && y == o.y && z == o.z; }
};

// A palette entry (widget +0x1f8, 0xa4 bytes each: a quantity, then the item).
struct PaletteEntry {
    int count = 0;
    Item item{};
};

// Whether an item is a base material cube (type11/subtype10).
bool isMaterialCube(const Item &item);
// Legacy recording-only palette when no canonical inventory is available.
std::vector<PaletteEntry> paletteOf(const std::vector<std::pair<Item, int>> &carried);
// Native palette: selected-weapon material followed by the four spirit materials.
// Aggregation and later consumption deliberately use different equality rules (palette spec §§1-3).
std::vector<PaletteEntry> paletteOf(const inventoryslots::Inventory &, const Item &weapon);

// The widget's state.
struct Widget {
    bool open = false;
    int hand = RIGHT_HAND;              // +0x2c0
    std::vector<PaletteEntry> palette;  // +0x1f8, rebuilt every frame from the inventory
    int selected = -1;                  // +0x210: the palette entry being placed
    std::optional<Voxel> pointed;       // +0x2c8..+0x2d0 and the flag +0x2d4: the voxel under the pointer
    bool carrying = false;              // +0x37c: a placed cube picked up to move
    Voxel carried;                      // +0x380..+0x388
    uint8_t carriedMaterial = 0;        // +0x38c
    bool selectionValid() const { return selected >= 0 && selected < (int)palette.size(); }
};

// What a click changed, for the caller to apply: one of the palette's items to take from the
// inventory (the first equal stack loses one), and whether the weapon's cubes changed.
struct ClickResult {
    bool weaponChanged = false;
    bool takeOne = false;
    Item taken{};
};

// One call of the click routine 0x292650 (§4). `weapon` is the hand's item record, null when the
// slot is empty; `overOtherWidget` tells whether the pointer is over another part of the
// interface (the routine's query 0x33f1e0), which keeps a carried cube from being dropped.
ClickResult click(Widget &widget, uint8_t *weapon, bool overOtherWidget);

// --- The weapon on screen -------------------------------------------------------------------

// The view of the weapon: turned `yawDegrees` about its up axis and looked at from
// `pitchDegrees` above, drawn into a square picture `pixels` wide. Cubes can go one voxel
// outside the model's box, so the picture fits that larger box.
struct View {
    float yawDegrees = 45.0f, pitchDegrees = 30.0f;
    int pixels = 256;
};
// The cells the picture covers: the model's box grown by one voxel each way.
bool inReach(const CubModel &model, Voxel v);

// What is under a point of the picture (pixels from its top left), §4's "voxel under the
// pointer": a placed cube itself, or the empty cell in front of the face of the weapon that the
// pointer is on. None when the pointer misses the weapon and its cubes.
std::optional<Voxel> pick(const CubModel &model, const uint8_t *weapon, const View &view, glm::vec2 pixel);
// Where the middle of a voxel lands in the picture (pixels), and a voxel's size there.
glm::vec2 project(const CubModel &model, const View &view, Voxel v);
float voxelPixels(const CubModel &model, const View &view);
// The picture (RGBA, pixels x pixels, transparent where nothing is): the weapon's voxels in their
// colours and its cubes in their materials' colours (nativepose::materialColour), faces shaded
// by how they face the light.
std::vector<uint8_t> render(const CubModel &model, const uint8_t *weapon, const View &view);
// The colour a cube of a material shows (0..1).
glm::vec4 cubeColour(uint8_t material);

// --- The window -----------------------------------------------------------------------------

// Placed by CubeVulkan where the vendor's widget goes, right of the bag (interface units).
constexpr glm::vec2 WINDOW_SIZE{300, 450};
constexpr int PALETTE_COLUMNS = 6, PALETTE_ROWS = 2;
constexpr float PALETTE_BOX = 40, PALETTE_GAP = 5;

struct Layout {
    glm::vec2 at{0};      // the window's top left
    glm::vec2 viewAt{0};  // the weapon's picture
    float viewSize = 256;
    glm::vec2 turnLeftAt{0}, turnRightAt{0}; // the two arrows that turn the view
    glm::vec2 paletteAt{0};
    bool inWindow(glm::vec2 p) const;
    bool onView(glm::vec2 p) const;
    glm::vec2 viewPixel(glm::vec2 p, int pixels) const; // a point of the window in the picture's pixels
    int turnAt(glm::vec2 p) const;                      // -1 the left arrow, +1 the right, 0 none
    int paletteBoxAt(glm::vec2 p) const;                // the palette box under p, -1 for none
    glm::vec2 paletteBox(int index) const;
};
// For a window `window` interface units in size.
Layout layout(glm::vec2 window);

// What the window shows.
struct Shown {
    std::string title, weaponName, pictureIcon; // the picture as an icon key given to Hud::addIcon
    glm::vec4 weaponColour{1};
    int cubes = 0, cap = 0;
    struct Box {
        std::string icon;
        int count = 0;
        glm::vec4 colour{1};
    };
    std::vector<Box> palette;
    int selected = -1, hoverBox = -1, hoverTurn = 0;
    std::optional<glm::vec2> marker;  // the pointed voxel's middle (interface units)
    float markerSize = 0;
    glm::vec4 markerColour{1, 1, 1, 0.45f};
    std::string hint;
};
Hud::Screen screen(const Layout &layout, const Shown &shown);

} // namespace upgradewidget
