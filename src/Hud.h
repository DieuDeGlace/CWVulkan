#pragma once
// The game's interface, rebuilt from its own art: bar frames and colours cut
// from frames captured in the running game (ZoneExtractor F10, then
// tools/HudExtract -> Extract/ui/hud), the game's two fonts (resource1.dat:
// the blocky HUD font, resource2.dat: the title font) and creature names from
// dict_en.xml. Everything is laid out as the game lays it out at 1920x1080 and
// scaled with the window height.
#include "MapMarks.h"
#include "MapPreview.h"
#include "SteamVoxel.h"
#include "InventoryUi.h"
#include "ItemPreview.h"
#include "ItemInformation.h"
#include "InventorySlots.h"
#include "Renderer.h"
#include "Plx.h"
#include "PlasmaShape.h"
#include "PlasmaText.h"
#include "PlasmaScene.h"
#include <array>
#include <memory>
#include <optional>
#include <glm/glm.hpp>
#include <filesystem>
#include <map>
#include <string>
#include <set>
#include <tuple>
#include <vector>

namespace dialoguechoice { class Panel; }
namespace dialoguepanel { class General; struct Layout; }
namespace npcspeech { struct Bubble; }
namespace identifywindow {
struct Shown;
struct Layout;
} // namespace identifywindow
namespace feattext {
struct Label;
}
namespace customization {
struct State;
struct Shown;
struct Layout;
} // namespace customization

class ChatLine;
struct CubModel;

class Hud {
  public:
    struct ScreenItem; // (below: the items of the screens drawScreen draws)
    struct Plate {      // a monster's name and health over its head
        glm::vec2 at;   // screen position of the top of its head
        std::string name;
        int level = 0;  // 0 = not shown
        float health = 1;
        float trail = 1; // health just before recent hits (the bright flash fades down to `health`)
    };
    struct Number {     // a floating damage number
        glm::vec2 at;
        int value = 0;
        float fade = 1; // 1 opaque .. 0 gone
        bool taken = false; // damage the player took (the game's red style)
    };
    // The inventory screen (B), laid out as the game's.
    struct Inventory : inventoryui::Tabs<6> {
        struct Entry {
            std::string name, icon; // icon: key given to addIcon ("" = none)
            int count = 1;
            bool allowed = true; // the player's class may use it
            int tab = 0;         // the game's inventory tab (the inventory tab of an item): 0 equipment, 1 specials,
                                 // 2 items, 3 ingredients, 4 pets, 5 artifacts
            bool stack = false;  // its count is shown (the stackable categories)
            glm::vec4 color{1};  // its rarity's colour (the rarity colour)
            int price = 0;       // what it costs (a vendor's) or sells for (the bag's, with a vendor open)
            inventoryui::Region region{};
            int slotIndex = -1; // physical index within the canonical tab; legacy lists have no backing slots
            std::optional<nativesaves::Item> original;
        };
        // The vendor beside the bag (InventoryWidget type 3): its offers on tab 0, what the
        // player sold to it on tab 1 (buy back).
        struct Vendor : inventoryui::Tabs<2> {
            bool open = false;
            std::vector<Entry> items;
            int hoverItem = -1, hoverTab = -1, hoverButton = 0;
        } vendor;
        // The crafting widget (InventoryWidget type 2, left of the bag: the formulas by tab,
        // `count` how many times each can be made) and the formula's details under it
        // (FormulaDetailsWidget).
        struct Crafting : inventoryui::Tabs<6> {
            bool open = false;
            int classType = 0; // its first two tabs' icons are the class's
            std::vector<Entry> items;
            int hoverItem = -1, hoverTab = -1, hoverButton = 0;
            struct Ingredient {
                std::string icon;
                int have = 0, need = 0;
                std::optional<nativesaves::Item> original;
            };
            struct Details {
                bool valid = false;
                std::string name, icon;
                std::optional<nativesaves::Item> original;
                glm::vec4 color{1};
                std::vector<Ingredient> ingredients;
                std::string missing;     // "Requires Anvil", "" when nothing is
                bool ready = false;      // nothing is missing: the button is green
                bool buttonOver = false; // the pointer is on the craft button (cyan)
                float progress = -1;     // the bar while crafting (-1 hidden)
                int hoverIngredient = -1;
            } details;
        } crafting;
        bool bagShown = true; // the bag, its slots and the character (B); crafting alone hides them
        int hoverTab = -1;
        inventoryui::Region activeRegion{};
        std::vector<inventoryui::Region> regionChoices;
        int hoverButton = 0;                    // the scroll button under the pointer (1 up, 2 down)
        // The hovered item's details (the item details text), shown in the preview widget: each line
        // its text, size and stroke, colour, the advance after it (from its end when `fromEnd`),
        // and whether it is drawn over the line before (`over`: the coloured rarity marks).
        using InfoLine = iteminformation::Line;
        std::vector<InfoLine> info;
        std::vector<InfoLine> comparisonInfo;
        std::string infoIcon;
        std::optional<nativesaves::Item> infoItem; // class-adapted preview record; the source stays unchanged
        itempreview::Anchor infoAnchor = itempreview::Anchor::Inventory;
        glm::vec2 pointer{0}; // the mouse (the preview follows it)
        // The character panel (the character panel's drawing).
        struct Sheet {
            bool valid = false, caster = false;
            float weaponLevel = 0, armorLevel = 0, hp = 0, power = 0, armor = 0, resistance = 0, critical = 0,
                  haste = 0, regeneration = 100, manaGeneration = 100;
            float travel[7] = {}; // climbing, swimming, diving (stamina/s), riding, gliding, sailing, light radius (m)
            int artifacts[7] = {};
        } sheet;
        std::string regionName;   // the region whose items the equipment tab shows
        std::vector<Entry> items; // the list (equipment tab)
        const std::vector<std::vector<nativesaves::Stack>> *canonicalTabs = nullptr;
        std::string slots[10]; // icons: L weapon, R weapon, L ring, R ring, neck, shoulder, chest, hands, feet, pet
        std::array<std::optional<nativesaves::Item>, 10> slotItems; // original SpriteWidget records; absent in legacy sessions
        std::string name, className, specialization;
        int level = 0;
        std::vector<std::pair<std::string, std::string>> stats; // label, value
        int hoverItem = -1, hoverSlot = -1;
    };
    struct State {
        const std::vector<feattext::Label> *feats = nullptr; // root textFX precedes all ordinary UI/lifebars
        float hp = 0, maxHp = 1, mp = 0, maxMp = 100;
        float stamina = 1;       // 0..1; a small bar under the character while below full
        glm::vec2 staminaAt{0};  // screen position of the character's feet
        bool showXp = false;     // pyro progression: the XP bar fills and shows its numbers
        int xp = 0, xpMax = 1;
        int gold = -1;           // top left, with the game's coin (-1 = hidden)
        // The quick buttons (the game's frame update): the key's short name, the line under it (the
        // item's or skill's name, a cooldown, the use key's prompt) and a count.
        struct Quick {
            std::string key, info, count;
            std::string icon; // the item shown (an icon key given to addIcon), "" for none
            std::optional<nativesaves::Item> original;
            glm::vec4 infoColor{1};
        };
        Quick quickItem, quickUse, quickSkill;
        struct Ring {
            struct Item {
                std::string icon;
                const nativesaves::Item *original = nullptr;
            };
            bool shown = false, left = false, right = false;
            std::vector<Item> items;
            std::string name, count;
            glm::vec4 colour{1};
            float angle = 0, radius = 0, worldMilliseconds = 0, farPlane = 960;
        } ring;
        // The cast bar (fill and name) and the crosshairs (Interface's update).
        bool cast = false;
        float castFill = 0;
        std::string castName;
        bool crosshair = true, zoomCrosshair = false;
        // The creatures' life bars (the creature labels): the gui.plx template by kind
        // (0 enemy, 1 friend, 2 static, 3 neutral), big for the crosshair's creature; the
        // texts its children get, the bars' fill, and whether only "info" shows.
        struct LifeBar {
            glm::vec2 at{0};
            int kind = 0;
            bool big = false, onlyInfo = false;
            std::string name, faction, info, text;
            glm::vec4 infoColor{1};
            float fraction = 1;
        };
        std::vector<LifeBar> lifeBars;
        // The info texts at the top right (Interface's update: COORDS/TIME/TEMP/HUM/ZOOM; the land's
        // name; the place's) and the location banner (its name, ms since it started; -1: never).
        std::string infoSmall, infoLarge, infoMedium;
        std::string locationName;
        float locationMs = -1;
        struct Message {         // floating text: "+5 XP", "LEVEL UP!"
            glm::vec2 at;
            std::string text;
            glm::u8vec4 color{166, 102, 255, 255};
            float fade = 1;
            bool big = false;
        };
        std::vector<Message> messages;
        struct Bubble {          // what an NPC is saying, over its head
            glm::vec2 at;
            std::string text;
            float fade = 1;
        };
        std::vector<Bubble> bubbles;
        std::vector<std::string> quest; // the quest tracker (right): objective lines
        std::vector<Plate> plates;
        std::vector<Number> numbers;
        const Inventory *inventory = nullptr; // shown when set
        // The in-game windows drawn over the inventory (the identify window, the upgrade widget,
        // the chat line): items as drawScreen takes them; none when null.
        const std::vector<ScreenItem> *windows = nullptr;
        const identifywindow::Shown *identification = nullptr;
        const customization::Shown *customization = nullptr;
        const Inventory *information = nullptr; // ordinary preview also exists without an open inventory
        std::function<void()> informationDraw;  // Read the borrowed source after earlier widget callbacks.
        bool interfaceVisible = true;           // Global GUI ancestor; own panel state/bindings survive hiding.
        // The map's markers (the map overlay's draw, MapMarks.h), far ones first, and the
        // interface's clock their icons bob by; also the quest sprites over people's heads.
        using MapMarker = mapmarks::Marker;
        std::vector<MapMarker> mapMarkers;
        std::function<std::vector<MapMarker>()> collectMapMarkers;
        bool mapInteractive = false;
        std::function<void(const mapmarks::Hover &)> mapOverlayDraw;
        int32_t guiClockMs = 0;
        int32_t guiStepMs = 0;
        int mapClockMs = 0;
        float mapWorldElapsedMs = 0;
        bool mapShrineMode = false;
        mapmarks::Cell mapShrineTarget{mapmarks::Selection::NO_TARGET, mapmarks::Selection::NO_TARGET};
        // The game's pointer (Interface's its pointer node, cursor.plx), shown in its menus, at the
        // mouse (window pixels), with its text (the map cursor's distance).
        bool pointer = false;
        glm::vec2 pointerAt{0};
        std::string pointerText;
        // The preview widget over the open map (the flight point preview): its lines,
        // at the pointer plus 30 units each way; none when empty.
        std::vector<Inventory::InfoLine> mapPreview;
        bool mapInformation = false; // Copied coordinates select this branch even when its content is rejected.
        std::function<std::vector<Inventory::InfoLine>()> mapInformationDraw;
        std::function<std::optional<mappreview::Lore>()> mapLoreDraw;
    };
    // The game's two fonts, from the owner's installation.
    bool loadMenu(const std::filesystem::path &gameDir, std::string &error);
    bool hasInventoryArt() const { return inventoryReady || (scene && panels.bag.node); }
    const mapmarks::PreviewOutputs &mapPreviewOutputs() const { return mapOutputs; }
    // An icon (RGBA, size x size) to pack into the atlas at the next layout().
    void addIcon(const std::string &key, std::vector<uint8_t> rgba, int size);
    // Retains the owner CUB allocation and native voxel data for immediate 3D widget draws.
    void addItemIcon(const std::string &key, const CubModel &, bool retainVolume = false,
                     steamvoxel::CubEncoding = steamvoxel::CubEncoding::Stock);
    void prepareItemResources(Renderer &renderer) { uploadItemModels(renderer); }
    const std::vector<guiitem::Request> &itemDraws() const { return itemRequests; }
    // OverworldEvents' quest sprites (assets/OverworldEvents/quest-icons.png, 320 x 80): the
    // orange question mark ("quest-event"), the grey one ("quest-destination") and the orange
    // exclamation mark ("quest-offer"), their green background keyed out as the mod does.
    bool loadQuestIcons(const std::filesystem::path &file);
    void addIcon(const std::string &key, std::vector<uint8_t> rgba, int w, int h);
    // What is under the mouse on the inventory screen (-1 = nothing).
    int inventoryItemAt(glm::vec2 mouse, size_t items) const;
    int inventorySlotAt(glm::vec2 mouse) const;
    // The same on the game's own panels (gui.plx), when they are built; the tab under it.
    int inventoryItemAt(glm::vec2 mouse, const Inventory &inv) const;
    inventoryslots::Index inventorySourceAt(glm::vec2 mouse, const Inventory &inv) const;
    int inventorySlotAt(glm::vec2 mouse, const Inventory &inv) const;
    int inventoryTabAt(glm::vec2 mouse, const Inventory &inv) const { return panelTabAt(mouse, inv, 0); }
    // The vendor's widget: its item, tab and scroll button under the mouse, and its rows.
    int vendorItemAt(glm::vec2 mouse, const Inventory &inv) const { return panelItemAt(mouse, inv, 1); }
    int vendorTabAt(glm::vec2 mouse, const Inventory &inv) const { return panelTabAt(mouse, inv, 1); }
    // The crafting widget's (which 2) and the formula details' craft button and ingredients.
    int craftingItemAt(glm::vec2 mouse, const Inventory &inv) const { return panelItemAt(mouse, inv, 2); }
    int craftingTabAt(glm::vec2 mouse, const Inventory &inv) const { return panelTabAt(mouse, inv, 2); }
    bool craftButtonAt(glm::vec2 mouse, const Inventory &inv) const;
    int ingredientAt(glm::vec2 mouse, const Inventory &inv) const;
    // The bag's rows to scroll through (the inventory rows: its row count), and the scroll
    // button under the mouse (1 up, 2 down, 0 none).
    int inventoryRowCount(const Inventory &inv, int which = 0) const;
    int inventoryButtonAt(glm::vec2 mouse, const Inventory &inv, int which = 0) const;
    // Tests this panel's own node tree, independent of overlapping panels (native 0x333e70).
    bool inventoryPanelEligible(glm::vec2 mouse, const Inventory &inv, int which = 0) const;
    std::vector<glm::vec2> inventoryNavigationPoints(const Inventory &inv, const std::vector<glm::vec2> &afterShop = {}) const;
    int inventoryFirstItem(const Inventory &inv, int which = 0) const;
    int inventoryRegionButtonAt(glm::vec2 mouse, const Inventory &inv) const;
    bool hasPanels() const { return scene && panels.bag.node; }
    bool hasPointer() const { return cursorNode != nullptr; } // (cursor.plx was read)
    // The interface's scale (guiScale), and how wide a map marker's label is in pixels: at the
    // font size the marker's scale gives (12 a unit, times the interface's scale).
    float interfaceScale() const { return guiScale; }
    float mapLabelWidth(const State::MapMarker &marker) const;
    // An inventory widget as built (its node, tab and cell parents, buttons, cells, tabs) and
    // the list it shows (InventoryWidget's tab, scroll and what is under the pointer).
    struct InvPanel {
        plasma::SceneNode *node = nullptr, *tabParent = nullptr, *cellParent = nullptr;
        std::map<std::string, plasma::SceneNode *> buttons;
        std::vector<plasma::SceneNode *> cells, tabs;
        int columns = 1, rows = 1;
    };
    struct InvList {
        const std::vector<Inventory::Entry> *items = nullptr;
        int tab = 0, scroll = 0, hoverItem = -1, hoverTab = -1, hoverButton = 0;
        bool all = false; // every item shows, however many (the crafting widget's formulas)
        int selected = -1;
        const inventoryui::Region *region = nullptr;
        bool regionArrows = false;
        const std::vector<std::vector<nativesaves::Stack>> *canonicalTabs = nullptr;
        bool includeZeroQuantity = false;
    };

    void advanceGui(double milliseconds) { interfaceClock.advance(milliseconds); }
    const plasma::GuiClock &guiTiming() const { return interfaceClock; }
    // Loads the extracted HUD art, fonts and names. False (with a reason) if the
    // HUD has not been extracted yet.
    bool load(const std::filesystem::path &gameDir, std::string &error);
    // Builds (or rebuilds after a resize) the atlas for a window of this size.
    void layout(Renderer &renderer, VkExtent2D screen);
    void prepareFrame(Renderer &renderer, VkExtent2D screen); // flush newly loaded icons once before emitting atlas coordinates
    // Rebuild the atlas only when newly received/editor text needs another owner glyph.
    bool requireText(const std::u16string &);
    void draw(const State &state, std::vector<OverlayVertex> &out,
              const std::function<void(std::vector<OverlayVertex> &)> &overlays = {}) const;
    // Display name for a creature from its head or body sprite ("radish-creature-head"
    // -> the dictionary's "RadishCreature" -> "Radishling"), or "" if unknown.
    std::string nameForSprite(const std::string &sprite) const;
    // The dictionary id behind that name ("RadishCreature"), "" if none.
    std::string nameIdForSprite(const std::string &sprite) const;

  private:
    struct Bar {
        glm::ivec4 frame{0}, interior{0}; // x, y, w, h at 1920x1080
        std::vector<glm::u8vec3> fill;    // per interior row
        std::vector<float> emptyAlpha;    // darkening per interior row
        glm::u8vec3 text{255}, outline{0};
        std::vector<uint8_t> image;       // frame layer, RGBA
        int imageW = 0, imageH = 0;
        glm::ivec2 atlas{0};              // where the frame layer sits in the atlas
    };
    struct Glyph {
        glm::ivec2 atlas{0}, size{0}, offset{0};
        glm::ivec2 strokeAtlas[2]{};
        int strokeRadius[2]{};
        float advance = 0;
    };
    struct Font {
        std::map<int, Glyph> glyphs;
        float ascent = 0;
        bool smoothStroke = false;
    };
    std::map<std::string, Bar> bars;
    std::vector<unsigned char> hudFont, titleFont;
    std::map<std::string, std::string> names; // dictionary id -> singular name
    Font valueFont, damageFont, plateFont;
    float scale = 1, offsetX = 0;
    glm::ivec2 white{0}; // a white texel in the atlas
    int atlasSize = 0;   // the atlas's side when the icons outgrew the usual one
    bool ready = false;
    bool atlasDirty = false;
    // Inventory screen art (Extract/ui/hud: tabs, panel, slot; inventory.txt).
    struct Picture {
        std::vector<uint8_t> rgba;
        int w = 0, h = 0;
        glm::ivec2 atlas{0};
    };
    Picture tabs, panel, slotFrame;
    std::map<std::string, Picture> icons;
    struct ItemModel {
        Mesh pending;
        glm::ivec3 allocation{0};
        uint32_t mesh = 0;
        std::shared_ptr<const CubModel> volume;
    };
    std::map<std::string, ItemModel> itemModels;
    mutable std::vector<guiitem::Request> itemRequests;
    plasma::GuiClock interfaceClock;
    mutable int32_t guiClockMs = 0;
    mutable int32_t guiStepMs = 0;
    mutable glm::vec3 recipeEuler{-120, 0, 0}; // advances only when the detail widget actually draws
    void uploadItemModels(Renderer &);
    void itemIcon(const std::string &, const nativesaves::Item &, const guiitem::Placement &, size_t beforeVertex) const;
    void ringItems(const State::Ring &) const;
    std::map<std::string, glm::u8vec3> colours;
    float panelDark = .55f, slotDark = .65f;
    bool inventoryReady = false;
    Font labelFont, nameFont;
    void drawInventory(const Inventory &inv, std::vector<OverlayVertex> &out) const;
    glm::vec2 slotPosition(int slot) const; // top left at 1920x1080

    void rect(std::vector<OverlayVertex> &out, float x, float y, float w, float h, glm::u8vec4 color) const;
    void image(std::vector<OverlayVertex> &out, glm::ivec2 atlas, glm::ivec2 size, float x, float y, float s) const;
    float textWidth(const Font &font, const std::string &text) const;
    void text(std::vector<OverlayVertex> &out, const Font &font, const std::string &s, float x, float y,
              glm::u8vec4 fill, glm::u8vec4 outline, float thickness) const;
    void bar(std::vector<OverlayVertex> &out, const Bar &b, float fraction, const std::string &value) const;

    // The game's own interface shapes (gui.plx from the owner's install, read as the game
    // reads it), placed as the interface's layout for the window size places them and scaled by the
    // game's interface scale (options.cfg guiScale; 1 without it).
  public:
    void setGuiScale(float s) { guiScale = s; }
    customization::Layout customizationLayout(bool shown) const;
    bool customizationHit(glm::vec2 pointer) const;
    bool customizationGuiHit(glm::vec2 pointer, bool identificationShown) const;

    // The frontend's screens (Frontend.h), drawn every frame the way the game's widgets draw
    // them (frontend-title.md §0.1): texts through the two-pass text helper, flat squares,
    // icons, and gui.plx templates ("button", "leftbutton", ...) placed and sized. Positions and
    // sizes are in interface units (window pixels / guiScale), drawn in the list's order.
    struct ScreenItem {
        enum Kind { Text, Square, Icon, Template } kind = Text;
        glm::vec2 at{0}, size{0}; // the text's reference point; a square's, icon's or template's top left and size
        std::wstring text;        // the text, or a template's label (empty: as the template has it)
        float fontSize = 12, outline = 3;
        glm::vec4 color{1}, outlineColor{0, 0, 0, 1}; // a template's tint is `color`
        int align = 0;            // 0 left, 1 centred, 2 right, 5 centred both ways
        std::string name;         // the template, or the icon's key
        enum Navigation { NoPoint, ShapeCentre, ChildCentres } navigation = NoPoint;
        std::string instance; // optional persistent template owner (separate panels)
        std::optional<bool> buttonHover;
        int animationStepMs = 0;
        bool initializeHidden = false;
        bool replaceWidget = false; // custom widget keeps the copied artwork's current binding
    };
    using Screen = std::vector<ScreenItem>;
    void drawDialogueChoices(dialoguechoice::Panel &, glm::vec2 screen, glm::vec2 mouse,
                             std::vector<OverlayVertex> &, std::vector<glm::vec2> *navigation = nullptr) const;
    bool dialogueChoicesHit(glm::vec2 point) const;
    bool generalDialogueHit(glm::vec2 point) const;
    void updateChat(ChatLine &, const std::function<std::optional<std::u16string>(int64_t)> & = {}) const;
    void appendChat(ChatLine &, const std::u16string &, std::array<uint8_t, 3>) const;
    void drawChat(ChatLine &, int32_t guiClock, std::vector<OverlayVertex> &) const;
    void drawFeatText(const std::vector<feattext::Label> &, std::vector<OverlayVertex> &) const;
    void drawNpcSpeech(const std::vector<npcspeech::Bubble> &, int32_t guiTime, std::vector<OverlayVertex> &,
        const std::function<void(int64_t, const dialoguepanel::Layout &, glm::vec2 frame)> &onDraw = {}) const;
    dialoguepanel::Layout drawGeneralDialogue(const dialoguepanel::General &, glm::vec2 screen, int32_t guiTime,
                                               std::vector<OverlayVertex> &) const;
    void drawScreen(const Screen &screen, std::vector<OverlayVertex> &out, std::vector<glm::vec2> *navigation = nullptr) const;
    // A gui.plx template's size (its frame, interface units); zero when there is none.
    glm::vec2 templateSize(const std::string &name, glm::vec2 requested = glm::vec2(0), bool replaceWidget = false) const;
    // Pick the copied owner shapes, inverse deformation and clipping at the same GUI-unit placement.
    bool templateHit(const ScreenItem &item, glm::vec2 point, bool passiveDescendants = false) const;
    glm::vec2 templateViewSize(const ScreenItem &) const; // live frame in screen pixels
    identifywindow::Layout identificationLayout() const;
    void drawIdentification(const identifywindow::Shown &, std::vector<OverlayVertex> &) const;
    void drawCustomization(const customization::Shown &, std::vector<OverlayVertex> &) const;
    bool hasInterface() const { return guiLoaded; }
    glm::ivec2 iconSize(const std::string &key) const {
        auto it = icons.find(key);
        return it == icons.end() ? glm::ivec2(0) : glm::ivec2(it->second.w, it->second.h);
    }

  private:
    plx::Document gui;
    bool guiLoaded = false;
    float guiScale = 1;
    glm::vec2 screen{0};
    mutable std::map<int, plasma::ShapeDrawing> shapes; // built once, by shape index
    // The interface's fonts (its text shapes name resource1.dat or resource2.dat) and their
    // glyphs as the text shapes drawn here rasterise them, in the atlas: by font, raster
    // (plasma::Raster::key), plain or outline, and character.
    plasma::Font plasmaFonts[2];
    std::set<wchar_t> additionalGlyphs;
    struct TextGlyph {
        glm::ivec2 atlas{0}, size{0}; // the glyph's texture in the atlas
        glm::vec2 offset{0}, extent{0};
    };
    std::map<std::tuple<int, std::array<int, 8>, bool, wchar_t>, TextGlyph> textGlyphs;
    float dialogueWordWidth(const std::wstring &word) const;
    float dialogueGlyphAdvance(wchar_t glyph) const;
    void drawDialogueWords(const dialoguepanel::Layout &, glm::vec2 at, int32_t guiTime, std::vector<OverlayVertex> &) const;
    int fontIndex(const std::string &name) const { return name == "resource2.dat" ? 1 : 0; }
    // What a frame changes of the nodes: a child's deformation replaced by `fills` (by child
    // name: the bars' x scale), and the string of every text shape under a node of a name
    // (as the game sets a text on every node of a name), and their colours.
    struct PlasmaFrame {
        std::map<std::string, float> fills;
        std::map<std::string, std::wstring> texts;
        std::map<std::string, glm::vec4> colors;
        std::map<std::string, glm::vec4> displayFills;  // override only this named node's live Display.fillColor
        std::map<std::string, float> hues;              // nodes (and their children) shown in this hue (degrees), shading kept
        std::set<std::string> hidden;                   // nodes made invisible (their display's visibility 0)
        std::map<std::string, glm::vec2> shifts;        // nodes moved (their translation plus this)
        std::map<std::string, std::string> sprites;     // a sprite widget's item (an icon key given to addIcon)
        std::map<std::string, nativesaves::Item> items; // original item values for SpriteWidget callbacks
        std::string sequence;                           // the animation playing (playing a node's animation), and how far into it
        float sequenceMs = -1;
    };
    // Draws a gui.plx node and its children: placed at `position` + `offset` (screen pixels,
    // the offset in interface units) when it is the top one.
    void plasmaNode(std::vector<OverlayVertex> &out, int node, const glm::mat4 &parent, glm::vec4 tint,
                    const PlasmaFrame &frame, const std::wstring *text, const glm::vec4 *color = nullptr,
                    const float *hue = nullptr) const;
    // the interface's code-made texts: a text the game makes in code (a stroked shape and a filled one
    // over it, white with a black stroke, in resource1.dat), placed by setPosition.
    plasma::TextStyle labelStyle(int size, int stroke, int flags, bool scalable) const;
    void plasmaLabel(std::vector<OverlayVertex> &out, const std::wstring &text, glm::vec2 position, glm::vec2 offset, int size,
                     int stroke, int flags = 0) const;
    bool plasmaPlaced(std::vector<OverlayVertex> &out, const char *name, glm::vec2 position, glm::vec2 offset,
                      const PlasmaFrame &frame, glm::vec2 *centre = nullptr, float deform = 1.0f) const;
    void drawMapMarkers(const State &state, std::vector<OverlayVertex> &out) const;
    // Returns where the text's last line sits (its pen's y, in the matrix's units).
    float plasmaText(std::vector<OverlayVertex> &out, const plasma::TextStyle &style, const glm::mat4 &m, glm::vec2 *pen = nullptr) const;
    // The game's panels as the interface's loading builds them (HudPanels.cpp), from gui.plx with the
    // tab icons from icons.plx.
    plx::Document iconsDoc;
    bool iconsLoaded = false;
    std::unique_ptr<plasma::Scene> scene;
    // The frontend's templates (drawScreen): a scene of their own, one instance per template
    // and use in a frame, kept for the next frames.
    mutable std::unique_ptr<plasma::Scene> frontScene;
    struct TemplateInstance {
        plasma::SceneNode *node = nullptr;
        glm::vec2 requested{-1};
        bool pointerInside = false;
    };
    mutable std::map<std::string, std::vector<TemplateInstance>> frontNodes;
    TemplateInstance *prepareTemplate(const ScreenItem &, size_t index = 0) const;

    void frontText(std::vector<OverlayVertex> &out, const ScreenItem &item) const;
    struct Panels {
        plasma::SceneNode *inGame = nullptr, *templates = nullptr, *cellTemplate = nullptr, *character = nullptr, *preview = nullptr;
        plasma::SceneNode *chat = nullptr, *identification = nullptr, *identificationFrame = nullptr, *customization = nullptr;
        InvPanel bag, vendor, crafting;
        std::vector<plasma::SceneNode *> slots;
        int cellWidth = 40, cellHeight = 40;
        // The crafting widget's selection frame; the formula details and its parts.
        plasma::SceneNode *selector = nullptr, *details = nullptr, *craftButton = nullptr, *craftBar = nullptr, *itemShadow = nullptr;
    } panels;
    mutable mapmarks::PreviewOutputs mapOutputs;
    mutable float previewLineSpacing = 4;
    mutable int craftingClass = -1; // the class the crafting tabs' icons are for
    void layoutDetails();
    void drawDetails(const Inventory &inv, std::vector<OverlayVertex> &out) const;
    InvList craftingList(const Inventory &inv) const;
    plasma::SceneNode *placeChat() const;
    float chatWordWidth(const std::wstring &) const;
    void buildPanels();
    void buildInvPanel(InvPanel &p, glm::vec2 size, const char *const *tabIcons, int tabs, const char *selection = nullptr);
    InvList listOf(const Inventory &inv, int which) const;
    void layoutInvPanel(InvPanel &p);
    void placeInvPanel(const InvPanel &p, const InvList &list) const;
    int invRowCount(const InvPanel &p, const InvList &list) const;
    void drawInvPanel(std::vector<OverlayVertex> &out, const InvPanel &p, const InvList &list, const std::string &title,
                      const std::string &region, bool vendor, bool selling, glm::vec2 pointer) const;
    InvList bagList(const Inventory &inv) const;
    InvList vendorList(const Inventory &inv) const;
    const InvPanel &invPanel(int which) const;
    int panelPhysicalAt(glm::vec2 mouse, const InvPanel &, const InvList &) const;
    void placePanels(const Inventory &inv) const;
    void drawPanels(const Inventory &inv, std::vector<OverlayVertex> &out) const;
    void drawCharacter(const Inventory &inv, std::vector<OverlayVertex> &out) const;
    // (through `from`, the gui scene when none)
    using WidgetDraw = std::function<void(const plasma::SceneNode &)>;
    void sceneNode(std::vector<OverlayVertex> &out, const plasma::SceneNode &n, glm::vec4 tint, const plasma::Scene *from = nullptr,
                   const WidgetDraw &drawWidget = {}) const;
    // The game's pointer (cursor.plx under the engine's root: not scaled by the interface).
    plx::Document cursorDoc;
    std::unique_ptr<plasma::Scene> cursorScene;
    plasma::SceneNode *cursorNode = nullptr;
    void drawPointer(const State &state, std::vector<OverlayVertex> &out) const;
    void drawMapPreview(const State &state, std::vector<OverlayVertex> &out) const;
    void drawNativeMapPreview(const State &state, std::vector<OverlayVertex> &out) const;
    // the widget text drawing; returns the y the text ends at (widget units).
    float widgetText(std::vector<OverlayVertex> &out, const plasma::SceneNode &node, const std::wstring &text, glm::vec2 at, float size,
                     float stroke, glm::vec4 color, glm::vec4 strokeColor, int flags, float wrap, glm::vec2 *pen = nullptr,
                     float lineSpacing = 1) const;
    void positionPreview(const Inventory &inv) const;
    void drawPreview(const Inventory &inv, std::vector<OverlayVertex> &out) const;
    int panelItemAt(glm::vec2 mouse, const Inventory &inv, int which = 0) const;
    int panelSlotAt(glm::vec2 mouse, const Inventory &inv) const;
    int panelTabAt(glm::vec2 mouse, const Inventory &inv, int which = 0) const;
    // Rasters the glyphs of the text shapes under the drawn nodes into the atlas.
    template <class Pack> void bakeText(Pack &pack, std::vector<uint8_t> &atlas, int width);
};
