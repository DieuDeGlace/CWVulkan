// The game's panels drawn from its own gui.plx (and icons.plx), built as the interface's loading
// builds them: the inventory widgets (InventoryWidget: the black widget resized, its
// "wideitembox" cells, scroll and region buttons and tabs with their icons) for the
// equipment (type 0), crafting (type 2) and the vendor (type 3), the ten equipment slots
// around the character, the character panel, the item details and the formula details. Positions follow
// the interface's layout for the window size; the drawing the inventory panel's drawing and its neighbours.
#include "Hud.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace {
// A narrow string as the font reads it: Latin-1 (the rarity mark is 0xb1, the coin 0xae).
std::wstring latin1(const std::string &s) {
    std::wstring w;
    for (unsigned char c : s)
        w.push_back((wchar_t)c);
    return w;
}
const char *const BAG_ICONS[] = {"equipment", "questitems", "consumables", "ingredients", "pets", "artifacts"};
const char *const BAG_TITLES[] = {"Equipment", "Specials", "Items", "Ingredients", "Pets", "Artifacts"};
const char *const VENDOR_ICONS[] = {"vendor-buy", "vendor-undo"};
// The crafting tabs (the interface's class style): the class's weapons and armour, then the common
// four.
const char *const CRAFT_ICONS[] = {nullptr, nullptr, "craft-jewels", "craft-cooking", "craft-alchemy", "craft-ingredients"};
const char *const CRAFT_TITLES[] = {"Weapons", "Armor", "Amulets", "Cooking", "Alchemy", "Ingredients"};
const char *const SLOT_INFO[10] = {"Left\nWeapon", "Right\nWeapon", "Left Ring", "Right Ring", "Neck",
                                   "Shoulder", "Chest", "Hands", "Feet", "Pet"};
const glm::vec4 BLACK(0, 0, 0, 1);
} // namespace

// An inventory widget (InventoryWidget's constructor, resizeTo and the tab nodes).
void Hud::buildInvPanel(InvPanel &p, glm::vec2 size, const char *const *tabIcons, int tabs, const char *selection) {
    plasma::Scene &s = *scene;
    plasma::SceneNode *node = s.instance("blackwidget", panels.inGame);
    if (!node || !node->widget)
        return;
    p.node = node;
    p.tabParent = s.create(node);
    p.cellParent = s.create(node);
    if (selection && (panels.selector = s.instance(selection, node))) // the selection frame, hidden until placed
        panels.selector->visible = false;
    for (const char *name : {"upbutton", "downbutton", "scrollbutton", "leftbutton", "rightbutton"})
        p.buttons[name] = s.instance(name, node);
    s.resizeTo(*node, size);
    layoutInvPanel(p);
    // The tabs side by side above the widget, 2 units apart.
    float x = 4;
    for (int t = 0; t < tabs; t++) {
        plasma::SceneNode *tab = s.instance("tab", p.tabParent);
        if (!tab || !tab->widget)
            break;
        if (iconsLoaded && tabIcons && tabIcons[t])
            s.setIcon(*tab, "background", iconsDoc, tabIcons[t]);
        s.moveTo(*tab, glm::vec2(x, -s.viewSize(*tab).y - 3.0f));
        x = s.viewSize(*tab).x + 2.0f + x;
        p.tabs.push_back(tab);
    }
}

void Hud::buildPanels() {
    panels = {};
    if (!guiLoaded)
        return;
    scene = std::make_unique<plasma::Scene>(gui);
    plasma::Scene &s = *scene;
    s.setScale(1.0f); // (the game builds its interface before the scale option applies)
    panels.inGame = s.create(&s.root(), "ingame");
    panels.templates = s.create(&s.root(), "templates");
    panels.templates->visible = false;

    // InventoryWidget's constructor: its cell size from the cell template's view.
    plasma::SceneNode *cell = s.instance("wideitembox", panels.templates);
    if (!cell || !cell->widget)
        return;
    panels.cellWidth = (int)s.viewSize(*cell).x;
    panels.cellHeight = (int)s.viewSize(*cell).y;
    panels.cellTemplate = cell;

    buildInvPanel(panels.bag, glm::vec2(400, 290), BAG_ICONS, 6);
    buildInvPanel(panels.crafting, glm::vec2(400, 285), CRAFT_ICONS, 6, "itemselector");
    buildInvPanel(panels.vendor, glm::vec2(400, 285), VENDOR_ICONS, 2);
    if (!panels.bag.node)
        return;
    // The formula details (FormulaDetailsWidget: a black widget with the item's shadow, the
    // craft button and bar, resized to 400 x 274).
    if ((panels.details = s.instance("blackwidget", panels.inGame)) && panels.details->widget) {
        panels.itemShadow = s.instance("itemshadow", panels.details);
        panels.craftButton = s.instance("craftbutton", panels.details);
        panels.craftBar = s.instance("craftbar", panels.details);
        s.resizeTo(*panels.details, glm::vec2(400, 274));
        layoutDetails();
    } else
        panels.details = nullptr;

    // The equipment slots: "equipmentboxright" for even slots, "equipmentboxleft" for odd.
    for (int i = 0; i < 10; i++) {
        plasma::SceneNode *slot = s.instance(i % 2 ? "equipmentboxleft" : "equipmentboxright", panels.inGame);
        if (!slot)
            break;
        slot->setText("info", latin1(SLOT_INFO[i]));
        panels.slots.push_back(slot);
    }
    // The item details (the interface's loading: "preview" under the root, resized to 300 x 300), its
    // rarity stars hidden (the preview panel's drawing).
    if ((panels.preview = s.instance("preview", &s.root()))) {
        for (const char *star : {"star1", "star2", "star3", "star4"})
            if (plasma::SceneNode *n = panels.preview->find(star))
                n->visible = false;
        if (panels.preview->widget)
            s.resizeTo(*panels.preview, glm::vec2(300, 300));
        panels.preview->visible = false;
    }
    // The character panel (CharacterWidget: a node with no shape, drawn in code).
    panels.character = s.create(panels.inGame, "character");
    panels.inGame->visible = false;
}

// the formula details layout: the shadow under the item, the button at the bottom right,
// the bar left of it.
void Hud::layoutDetails() {
    plasma::Scene &s = *scene;
    const glm::vec2 view = s.viewSize(*panels.details);
    if (panels.itemShadow && panels.itemShadow->widget)
        s.moveTo(*panels.itemShadow, glm::vec2((view.x - s.viewSize(*panels.itemShadow).x) * 0.5f, 120.0f));
    if (!panels.craftButton || !panels.craftButton->widget)
        return;
    const glm::vec2 button = s.viewSize(*panels.craftButton);
    s.moveTo(*panels.craftButton, glm::vec2(view.x - button.x - 20.0f, view.y - button.y - 20.0f));
    if (panels.craftBar && panels.craftBar->widget) {
        s.moveTo(*panels.craftBar, glm::vec2(20.0f, view.y - button.y - 20.0f));
        s.resizeTo(*panels.craftBar, glm::vec2(view.x - button.x - 40.0f - 10.0f, button.y));
    }
}

// the inventory layout: the cells as many as fit, the buttons at the edges.
void Hud::layoutInvPanel(InvPanel &p) {
    plasma::Scene &s = *scene;
    const glm::vec2 content = s.contentSize(*p.node);
    const int cw = panels.cellWidth, ch = panels.cellHeight;
    p.columns = std::max(1, (int)((content.x - 10.0f) / (float)(cw + 5)));
    p.rows = std::max(1, (int)((content.y - 40.0f) / (float)(ch + 5)));
    p.cellParent->children.clear();
    p.cells.clear();
    for (int x = 0; x < p.columns; x++)
        for (int y = 0; y < p.rows; y++) {
            plasma::SceneNode *cell = s.instance(*panels.cellTemplate, p.cellParent);
            s.moveTo(*cell, glm::vec2((float)(x * (cw + 5) + 10), (float)(y * (ch + 5) + 40)));
            p.cells.push_back(cell);
        }
    if (auto *b = p.buttons["upbutton"])
        s.moveTo(*b, glm::vec2(content.x - 30.0f, 10.0f));
    if (auto *b = p.buttons["downbutton"])
        s.moveTo(*b, glm::vec2(content.x - 30.0f, content.y - 30.0f));
    if (auto *b = p.buttons["leftbutton"])
        s.moveTo(*b, glm::vec2(15.0f, content.y - 25.0f));
    if (auto *b = p.buttons["rightbutton"])
        s.moveTo(*b, glm::vec2(content.x - 60.0f, content.y - 25.0f));
}

// the widget text drawing (the two-pass text object, plasma-shapes-and-text.md 7.1, scalable mode):
// a text at a point of the widget (its pivot plus the point), its outline alone first (a
// transparent fill), then the fill.
float Hud::widgetText(std::vector<OverlayVertex> &out, const plasma::SceneNode &node, const std::wstring &text, glm::vec2 at,
                      float size, float stroke, glm::vec4 color, glm::vec4 strokeColor, int flags, float wrap) const {
    glm::mat4 m = scene->matrix(node);
    m = glm::translate(m, glm::vec3(node.transform.pivot + at, 0));
    plasma::TextStyle style;
    style.string = text;
    style.font = "resource1.dat";
    style.pixelSize = size;
    style.flags = flags;
    style.wrapWidth = wrap;
    style.scalable = true;
    if (stroke > 0.0f) {
        style.strokeRadius = stroke;
        style.color = glm::vec4(0);
        style.strokeColor = strokeColor;
        plasmaText(out, style, m);
    }
    style.strokeRadius = 0;
    style.color = color;
    return at.y + plasmaText(out, style, m);
}

void Hud::sceneNode(std::vector<OverlayVertex> &out, const plasma::SceneNode &n, glm::vec4 tint, const plasma::Scene *from) const {
    if (!n.visible)
        return;
    const plasma::Scene &in = from ? *from : *scene;
    tint *= n.fill;
    const plx::Document &doc = n.doc ? *n.doc : gui;
    plasma::TextStyle style;
    if (n.shape >= 0 && n.shape < (int)doc.shapes.size() && plasma::textStyle(doc.shapes[(size_t)n.shape], doc.loaderFlags, style)) {
        if (n.text)
            style.string = *n.text;
        if (n.textColor)
            style.color = *n.textColor;
        style.color.a *= tint.a, style.strokeColor.a *= tint.a;
        plasmaText(out, style, in.matrix(n));
    } else if (n.shape >= 0 && !n.clips) {
        plasma::Scene::Drawn d;
        in.drawShape(n, tint, d);
        const float u = white.x + 2.f, v = white.y + 2.f;
        for (size_t i = 0; i < d.positions.size(); i++) {
            const glm::vec4 c = glm::clamp(d.colors[i], glm::vec4(0), glm::vec4(1)) * 255.0f;
            out.push_back({d.positions[i].x, d.positions[i].y, u, v, (uint8_t)c.r, (uint8_t)c.g, (uint8_t)c.b, (uint8_t)c.a});
        }
    }
    if (n.onDraw)
        n.onDraw(const_cast<plasma::SceneNode &>(n));
    for (auto &c : n.children)
        sceneNode(out, *c, tint, from);
}

// The items a widget shows on its tab, in order (the indices into the list).
static std::vector<int> shownOf(const Hud::InvList &list) {
    std::vector<int> shown;
    for (size_t i = 0; i < list.items->size(); i++)
        if ((*list.items)[i].tab == list.tab && (list.all || (*list.items)[i].count != 0))
            shown.push_back((int)i);
    return shown;
}

int Hud::invRowCount(const InvPanel &p, const InvList &list) const {
    if (!p.node)
        return 1;
    const int count = (int)shownOf(list).size();
    if (count == 0)
        return 0;
    return std::max(1, (count - 1) / p.columns - p.rows + 2);
}

// The cells' faint fill, the tabs lit when chosen or under the pointer, the scroll button
// (the inventory scroll button: the track 35 from the top, the height less 70, divided
// by the rows), the buttons lit under the pointer (the inventory panel's drawing).
void Hud::placeInvPanel(const InvPanel &p, const InvList &list) const {
    plasma::Scene &s = *scene;
    for (plasma::SceneNode *c : p.cells)
        c->fill = glm::vec4(1, 1, 1, 0.25f);
    for (size_t t = 0; t < p.tabs.size(); t++) {
        const bool chosen = (int)t == list.tab, over = (int)t == list.hoverTab;
        p.tabs[t]->fill = chosen || over ? glm::vec4(1) : glm::vec4(0.8f, 0.8f, 0.8f, 1);
        if (plasma::SceneNode *frame = p.tabs[t]->find("frame"))
            frame->fill = over ? glm::vec4(0, 1, 1, 1) : chosen ? glm::vec4(0, 1, 0, 1) : glm::vec4(1);
    }
    if (auto it = p.buttons.find("scrollbutton"); it != p.buttons.end() && it->second && it->second->widget) {
        plasma::SceneNode &button = *it->second;
        const int rowCount = invRowCount(p, list);
        const glm::vec2 content = s.contentSize(*p.node);
        if (rowCount) {
            const int track = (int)(content.y - 70.0f), buttonHeight = track / rowCount;
            const float y = rowCount > 1 ? (float)(std::clamp(list.scroll, 0, rowCount - 1) * (track - buttonHeight) / (rowCount - 1) + 35) : 35.0f;
            s.moveTo(button, glm::vec2(content.x - 28.0f, y));
            s.resizeTo(button, glm::vec2(s.contentSize(button).x, (float)buttonHeight));
        }
        button.visible = rowCount > 0;
    }
    for (auto [name, value] : {std::pair{"upbutton", 1}, std::pair{"downbutton", 2}})
        if (auto it = p.buttons.find(name); it != p.buttons.end() && it->second)
            it->second->fill = list.hoverButton == value ? glm::vec4(0, 1, 1, 1) : glm::vec4(1);
    // The region buttons show for the bag's first two tabs with other regions (none yet:
    // CubeVulkan's items do not keep their region apart).
    for (const char *b : {"leftbutton", "rightbutton"})
        if (auto it = p.buttons.find(b); it != p.buttons.end() && it->second)
            it->second->visible = false;
}

// Places the panels for this frame (the interface's layout for the window size).
void Hud::placePanels(const Inventory &inv) const {
    plasma::Scene &s = *scene;
    s.setScale(guiScale);
    const float w = std::floor(screen.x / 2) * 2, h = std::floor(screen.y / 2) * 2;
    plasma::SceneNode &bag = *panels.bag.node;
    const glm::vec2 bagView = s.viewSize(bag);
    s.setPosition(bag, glm::vec2(w / 2 - bagView.x * 0.5f, h / 2 - bagView.y), glm::vec2(0, -50));
    if (panels.vendor.node)
        s.setPosition(*panels.vendor.node, glm::vec2(bagView.x * 0.5f + w / 2, h / 2 - bagView.y), glm::vec2(10, -50));
    for (size_t i = 0; i < panels.slots.size(); i++)
        s.setPosition(*panels.slots[i], glm::vec2(w / 2, h / 2), glm::vec2((float)((int)(i % 2) * 400 - 200), (float)((int)(i / 2) * 56 - 40)));
    if (panels.character)
        s.setPosition(*panels.character, glm::vec2(0, 0), glm::vec2(20, 170));
    placeInvPanel(panels.bag, bagList(inv));
    if (panels.vendor.node) {
        panels.vendor.node->visible = inv.vendor.open;
        placeInvPanel(panels.vendor, vendorList(inv));
    }
    bag.visible = inv.bagShown;
    for (plasma::SceneNode *slot : panels.slots)
        slot->visible = inv.bagShown;
    if (panels.character)
        panels.character->visible = inv.bagShown;
    // The crafting widget left of the bag, the formula details under it.
    const Inventory::Crafting &c = inv.crafting;
    if (plasma::SceneNode *crafting = panels.crafting.node) {
        crafting->visible = c.open;
        const glm::vec2 view = s.viewSize(*crafting);
        s.setPosition(*crafting, glm::vec2(w / 2 - bagView.x * 0.5f - view.x, h / 2 - bagView.y), glm::vec2(-10, -50));
        if (craftingClass != c.classType && iconsLoaded) { // the interface's class style
            static const char *const classes[5] = {nullptr, "warrior", "ranger", "mage", "rogue"};
            const char *name = c.classType >= 1 && c.classType <= 4 ? classes[c.classType] : nullptr;
            for (int t = 0; t < 2 && name && t < (int)panels.crafting.tabs.size(); t++)
                s.setIcon(*panels.crafting.tabs[(size_t)t], "background", iconsDoc,
                          std::string("craft-") + name + (t ? "-armor" : "-weapons"));
            craftingClass = c.classType;
        }
        const InvList list = craftingList(inv);
        placeInvPanel(panels.crafting, list);
        // the inventory selection frame: the frame on the selected cell, hidden off the page.
        if (panels.selector && panels.selector->widget) {
            const std::vector<int> shown = shownOf(list);
            const auto at = std::find(shown.begin(), shown.end(), c.selected);
            const int first = panels.crafting.columns * c.scroll, index = (int)(at - shown.begin()) - first;
            const bool on = at != shown.end() && index >= 0 && index < panels.crafting.columns * panels.crafting.rows;
            panels.selector->visible = on;
            if (on)
                s.moveTo(*panels.selector, glm::vec2((float)(index % panels.crafting.columns * (panels.cellWidth + 5) + 10),
                                                     (float)(index / panels.crafting.columns * (panels.cellHeight + 5) + 40)));
        }
    }
    if (plasma::SceneNode *details = panels.details) {
        details->visible = c.open; // (Game: shown with the crafting widget)
        const glm::vec2 view = s.viewSize(*details);
        s.setPosition(*details, glm::vec2(w / 2 - bagView.x * 0.5f - view.x, h / 2), glm::vec2(-10, -40));
        // The craft button: cyan under the pointer when crafting is possible, green
        // otherwise, white when something is missing.
        if (panels.craftButton)
            if (plasma::SceneNode *frame = panels.craftButton->find("frame"))
                frame->fill = !c.details.ready ? glm::vec4(1) : c.details.buttonOver ? glm::vec4(0, 1, 1, 1) : glm::vec4(0, 1, 0, 1);
        // the crafting progress: the bar's inner part stretched to the progress.
        if (panels.craftBar) {
            panels.craftBar->visible = c.details.progress >= 0.0f;
            if (plasma::SceneNode *bar = panels.craftBar->find("bar"); bar && bar->widget && panels.craftBar->widget) {
                const float width = s.contentSize(*bar).x;
                const float sx = width > 0 ? (s.viewSize(*panels.craftBar).x - 8.0f) * std::max(c.details.progress, 0.0f) / width : 0.0f;
                bar->transform.deformation = glm::scale(glm::mat4(1), glm::vec3(sx, 1, 1));
            }
        }
    }
}

// the inventory panel's drawing's items and texts: the items in their cells, the title, the region's
// name (the bag's first two tabs), the names (a count for stacks, the rarity's colour, red
// when the class may not use them) and, with a vendor, the prices.
void Hud::drawInvPanel(std::vector<OverlayVertex> &out, const InvPanel &p, const InvList &list, const std::string &title,
                       const std::string &region, bool vendor, bool selling) const {
    if (!p.node || !p.node->visible)
        return;
    plasma::Scene &s = *scene;
    const plasma::SceneNode &node = *p.node;
    const float scaleNow = s.matrix(node)[0][0];
    const glm::vec2 position = s.contentPosition(node), content = s.contentSize(node);
    const int cw = panels.cellWidth, ch = panels.cellHeight, columns = p.columns, rows = p.rows;
    const std::vector<int> shown = shownOf(list);
    const int first = columns * list.scroll, last = std::min((int)shown.size(), first + columns * rows);
    for (int i = first; i < last; i++) {
        const int index = i - first;
        const float x = std::floor(((float)(index % columns * (cw + 5) + 10)) * scaleNow + position.x);
        const float y = std::floor(((float)(index / columns * (ch + 5) + 40)) * scaleNow + position.y);
        const Inventory::Entry &e = (*list.items)[(size_t)shown[(size_t)i]];
        // The item turning in its cell, larger under the pointer (CubeVulkan: its still icon).
        if (auto icon = icons.find(e.icon); icon != icons.end()) {
            const float side = (float)ch * scaleNow * 0.8f * (shown[(size_t)i] == list.hoverItem ? 1.25f : 1.0f);
            const float centre = (float)(ch / 2) * scaleNow;
            image(out, icon->second.atlas, {icon->second.w, icon->second.h}, std::round(x + centre - side / 2),
                  std::round(y + centre - side / 2), side / (float)icon->second.w);
        }
    }
    widgetText(out, node, latin1(title), {15.0f, 25.0f}, 12, 3, glm::vec4(1, 0.8f, 0.5f, 1), BLACK, 0, 0);
    if (!region.empty())
        widgetText(out, node, latin1(region), {(content.x + 15.0f - 60.0f) * 0.5f, content.y - 12.0f}, 12, 3, glm::vec4(1), BLACK,
                   plasma::TEXT_CENTER, 0);
    for (int i = first; i < last; i++) {
        const Inventory::Entry &e = (*list.items)[(size_t)shown[(size_t)i]];
        const int column = (i - first) % columns, row = (i - first) / columns;
        std::string name = e.name;
        if (vendor || list.all ? e.count > 0 : e.stack)
            name += " (" + std::to_string(e.count) + ")";
        widgetText(out, node, latin1(name), {(float)(ch + 20 + column * (cw + 5)), (float)(ch + row * (ch + 5) + (list.all ? 13 : 10))}, 12, 3,
                   e.allowed ? e.color : glm::vec4(1, 0.25f, 0.25f, 1), BLACK, plasma::TEXT_WRAP, (float)(cw - ch - 20));
        // What it costs (a vendor's), or sells for (the bag's, with a vendor open).
        if (e.price > 0 && (vendor || selling)) {
            const float right = vendor ? (float)(column * (cw + 5) + cw + 10) : (float)(cw + column * (cw + 5) + 6);
            widgetText(out, node, latin1(std::to_string(e.price) + " \xae"), {right, (float)(ch + 37 + row * (ch + 5))}, 12, 3,
                       glm::vec4(1, 0.9f, 0, 1), BLACK, plasma::TEXT_RIGHT, 0);
        }
    }
}

void Hud::drawPanels(const Inventory &inv, std::vector<OverlayVertex> &out) const {
    if (!scene || !panels.bag.node)
        return;
    placePanels(inv);
    panels.inGame->visible = true;
    sceneNode(out, *panels.inGame, glm::vec4(1));
    panels.inGame->visible = false;
    drawCharacter(inv, out);

    const InvList bag = bagList(inv);
    const int tab = std::clamp(inv.tab, 0, 5);
    if (inv.bagShown)
        drawInvPanel(out, panels.bag, bag, BAG_TITLES[tab], tab <= 1 ? inv.regionName : std::string(), false,
                 inv.vendor.open && tab != 1 && tab != 5);
    if (inv.vendor.open)
        drawInvPanel(out, panels.vendor, vendorList(inv), "Vendor", std::string(), true, false);
    if (inv.crafting.open) {
        drawInvPanel(out, panels.crafting, craftingList(inv), std::string("Crafting: ") + CRAFT_TITLES[std::clamp(inv.crafting.tab, 0, 5)],
                     std::string(), false, false);
        drawDetails(inv, out);
    }
    // The equipment slots' items (SpriteWidget: in each box's content).
    plasma::Scene &s = *scene;
    for (size_t k = 0; k < panels.slots.size() && k < 10 && inv.bagShown; k++) {
        const plasma::SceneNode &slot = *panels.slots[k];
        auto icon = icons.find(inv.slots[k]);
        if (!slot.widget || icon == icons.end())
            continue;
        const glm::vec2 at = s.contentPosition(slot), size = s.screenSize(slot);
        const float side = std::min(std::abs(size.x), std::abs(size.y)) * 0.8f;
        const glm::vec2 centre = at + size * 0.5f;
        image(out, icon->second.atlas, {icon->second.w, icon->second.h}, std::round(centre.x - side / 2), std::round(centre.y - side / 2),
              side / (float)icon->second.w);
    }
    drawPreview(inv, out);
}

// the formula details drawing: the result turning at the top (larger under the pointer), the
// ingredients in a grid (70 x 45 a cell) at the bottom with "have/need" under each (green when
// there are enough), the result's name in its colour, and what is missing.
void Hud::drawDetails(const Inventory &inv, std::vector<OverlayVertex> &out) const {
    const Inventory::Crafting::Details &d = inv.crafting.details;
    if (!panels.details || !d.valid)
        return;
    plasma::Scene &s = *scene;
    const plasma::SceneNode &node = *panels.details;
    const float scaleNow = s.matrix(node)[0][0];
    const glm::vec2 position = s.contentPosition(node), content = s.contentSize(node), view = s.viewSize(node);
    auto icon = [&](const std::string &key, glm::vec2 centre, float side) {
        if (auto it = icons.find(key); it != icons.end())
            image(out, it->second.atlas, {it->second.w, it->second.h}, std::round(centre.x - side / 2), std::round(centre.y - side / 2),
                  side / (float)it->second.w);
    };
    // (CubeVulkan draws the items' still icons where the game turns their models)
    const bool overResult = inv.pointer.x > position.x + (content.x * 0.5f - 60.0f) * scaleNow && inv.pointer.y > position.y + 50.0f * scaleNow &&
                            inv.pointer.x < position.x + (content.x * 0.5f + 60.0f) * scaleNow && inv.pointer.y < position.y + 170.0f * scaleNow;
    icon(d.icon, position + glm::vec2(view.x * 0.5f, 110.0f * scaleNow), 64.0f * scaleNow * (overResult ? 1.5f : 1.0f));
    const int columns = std::max(1, (int)((view.x - 10.0f) / 45.0f));
    const glm::vec2 grid = position + glm::vec2(0, (content.y - 130.0f) * scaleNow);
    for (size_t i = 0; i < d.ingredients.size(); i++) {
        const float x = std::floor((float)(20 + 70 * ((int)i % columns)) * scaleNow + grid.x);
        const float y = std::floor((float)(10 + 45 * ((int)i / columns)) * scaleNow + grid.y);
        icon(d.ingredients[i].icon, glm::vec2(x, y) + 20.0f * scaleNow, 32.0f * scaleNow * ((int)i == d.hoverIngredient ? 1.5f : 1.0f));
    }
    widgetText(out, node, latin1(d.name), {content.x * 0.5f, 30.0f}, 12, 3, d.color, BLACK, plasma::TEXT_CENTER | plasma::TEXT_WRAP,
               content.x - 10.0f);
    for (size_t i = 0; i < d.ingredients.size(); i++) {
        const Inventory::Crafting::Ingredient &g = d.ingredients[i];
        const int row = (int)i / columns;
        widgetText(out, node, latin1(std::to_string(g.have) + "/" + std::to_string(g.need)),
                   {(float)(40 + 70 * ((int)i % columns)), std::floor(content.y - 130.0f + (float)(45 * row) + 55.0f)}, 12, 3,
                   g.have < g.need ? glm::vec4(1) : glm::vec4(0, 1, 0, 1), BLACK, plasma::TEXT_CENTER, 0);
    }
    if (!d.missing.empty())
        widgetText(out, node, latin1(d.missing), {15.0f, content.y - 40.0f}, 12, 3, glm::vec4(1, 0.25f, 0.25f, 1), BLACK, plasma::TEXT_WRAP,
                   content.x - 10.0f);
}

// the preview panel's drawing for an item: its details from (14, 25), 260 wide, the box as tall as
// they need (200 at least); the item turning at the right. Placed beside the hovered panel
// (Game's preview placement while in a menu).
void Hud::drawPreview(const Inventory &inv, std::vector<OverlayVertex> &out) const {
    if (!panels.preview || !panels.preview->widget || inv.info.empty())
        return;
    plasma::Scene &s = *scene;
    plasma::SceneNode &preview = *panels.preview;
    const plasma::SceneNode &hovered = inv.vendor.open && inv.vendor.hoverItem >= 0 && panels.vendor.node ? *panels.vendor.node
                                                                                                          : *panels.bag.node;
    // Two passes: the first sizes the box from where the lines end, the second draws it.
    for (int pass = 0; pass < 2; pass++) {
        std::vector<OverlayVertex> lines;
        float y = 25.0f, lineY = y;
        for (const Inventory::InfoLine &l : inv.info) {
            const float at = l.over ? lineY : y;
            const float end = widgetText(lines, preview, latin1(l.text), {14.0f, at}, l.size, l.stroke, l.color, BLACK, plasma::TEXT_WRAP, 260.0f);
            lineY = at;
            if (!l.over)
                y = (l.fromEnd ? end : at) + l.advance;
        }
        if (pass == 0) {
            const float scaleNow = s.matrix(*preview.parent)[0][0];
            s.resizeTo(preview, glm::vec2(s.contentSize(preview).x, std::max(200.0f, y)) * scaleNow);
            const glm::vec2 pointer = inv.pointer;
            if (s.contentPosition(hovered).x + s.screenSize(hovered).x * 0.5f > pointer.x)
                s.setPosition(preview, glm::vec2(0, pointer.y), glm::vec2(s.viewPosition(hovered).x - 10.0f - s.contentSize(preview).x, 30.0f));
            else
                s.setPosition(preview, glm::vec2(0, pointer.y), glm::vec2(s.viewPosition(hovered).x + s.screenSize(hovered).x + 10.0f, 30.0f));
            continue;
        }
        preview.visible = true;
        sceneNode(out, preview, glm::vec4(1));
        preview.visible = false;
        out.insert(out.end(), lines.begin(), lines.end());
        // The item at the right of the box.
        if (auto icon = icons.find(inv.infoIcon); icon != icons.end()) {
            const glm::mat4 m = s.matrix(preview);
            const glm::vec2 origin = glm::vec2(m * glm::vec4(preview.transform.pivot, 0, 1));
            const float width = s.viewSize(preview).x, side = width * 0.3f;
            image(out, icon->second.atlas, {icon->second.w, icon->second.h}, std::round(origin.x + width * 0.75f - side / 2),
                  std::round(origin.y + width * 0.3f - side / 2), side / (float)icon->second.w);
        }
    }
}

void Hud::drawMapPreview(const State &state, std::vector<OverlayVertex> &out) const {
    if (!scene || !panels.preview || !panels.preview->widget || state.mapPreview.empty())
        return;
    plasma::Scene &s = *scene;
    plasma::SceneNode &preview = *panels.preview;
    // Two passes, as for an item: the first sizes the box to where the lines end (plus 30).
    for (int pass = 0; pass < 2; pass++) {
        std::vector<OverlayVertex> lines;
        float y = 25.0f;
        for (const Inventory::InfoLine &l : state.mapPreview) {
            const float end = widgetText(lines, preview, latin1(l.text), {14.0f, y}, l.size, l.stroke, l.color, BLACK, plasma::TEXT_WRAP,
                                         s.contentSize(preview).x - 30.0f);
            y = (l.fromEnd ? end : y) + l.advance;
        }
        if (pass == 0) {
            const float scaleNow = s.matrix(*preview.parent)[0][0];
            s.resizeTo(preview, glm::vec2(s.contentSize(preview).x, y + 30.0f) * scaleNow);
            s.setPosition(preview, state.pointerAt, glm::vec2(30.0f, 30.0f));
            continue;
        }
        preview.visible = true;
        sceneNode(out, preview, glm::vec4(1));
        preview.visible = false;
        out.insert(out.end(), lines.begin(), lines.end());
    }
}

// the character panel's drawing: the name, level and equipment levels, class, the stats in
// alternating colours, and the travel skills with their artifact bonuses.
void Hud::drawCharacter(const Inventory &inv, std::vector<OverlayVertex> &out) const {
    const Inventory::Sheet &c = inv.sheet;
    if (!panels.character || !c.valid || !inv.bagShown)
        return;
    const plasma::SceneNode &node = *panels.character;
    float size = 16, stroke = 4;
    auto put = [&](const std::string &s, float x, float y, glm::vec4 colour) { widgetText(out, node, latin1(s), {x, y}, size, stroke, colour, BLACK, 0, 0); };
    auto tenth = [](float v) { // rounded to tenths, shown as the stream prints it
        char b[32];
        std::snprintf(b, sizeof b, "%g", std::round(v * 10.0f) / 10.0f);
        return std::string(b);
    };
    static const glm::vec4 rarity[6] = {{0.7f, 0.7f, 0.7f, 1}, {1, 1, 1, 1}, {0, 1, 0, 1}, {0.3f, 0.45f, 1, 1}, {0.55f, 0.2f, 1, 1}, {1, 1, 0, 1}};
    auto stars = [&](float level, float y) { // five grey marks, then the level's in its rarity colour
        put(std::string(5, (char)0xb1), 240, y, glm::vec4(0.3f, 0.3f, 0.3f, 1));
        const int n = (int)std::round(level);
        put(std::string((size_t)std::max(0, n + 1), (char)0xb1), 240, y, rarity[std::clamp(n, -1, 4) + 1]);
    };
    put(inv.name, 0, 0, glm::vec4(1, 0.8f, 0.2f, 1));
    put("LEVEL " + std::to_string(inv.level), 0, 24, glm::vec4(1));
    put("Weapon Level", 0, 48, glm::vec4(1));
    stars(c.weaponLevel, 48);
    put("Armor Level", 0, 68, glm::vec4(1));
    stars(c.armorLevel, 68);
    size = 12, stroke = 3;
    const glm::vec4 grey(0.6f, 0.6f, 0.7f, 1);
    put("Class", 0, 88, grey);
    put(inv.className, 240, 88, grey);
    put("Specialization", 0, 104, grey);
    put(inv.specialization, 240, 104, grey);
    glm::vec4 rowColour(0.6f, 1, 0.6f, 1), otherColour(1, 1, 0.6f, 1);
    float y = 124;
    auto row = [&](const std::string &label, const std::string &value) {
        put(label, 0, y, rowColour);
        put(value, 240, y, rowColour);
        std::swap(rowColour, otherColour);
        y += 16;
    };
    row("HIT POINTS", std::to_string((int)c.hp));
    row(c.caster ? "SPELL POWER " : "ATTACK POWER ", tenth(c.power));
    row("ARMOR", tenth(c.armor));
    row("RESISTANCE", tenth(c.resistance));
    row("CRITICAL", tenth(c.critical) + "%");
    row("HASTE", tenth(c.haste) + "%");
    row("REGENERATION", tenth(c.regeneration) + "%");
    row("MANA GENERATION", tenth(c.manaGeneration) + "%");
    // The travel skills (ArtifactEffect_ClimbingSpeed .. LampRadius), 20 apart from 260.
    static const char *const skills[7] = {"Climbing Speed", "Swimming Speed", "Diving Skill", "Riding Speed",
                                          "Hang Gliding Speed", "Sailing Speed", "Light Radius"};
    rowColour = glm::vec4(0.45f, 0.6f, 1, 1), otherColour = glm::vec4(0.6f, 0.45f, 1, 1);
    float ty = 240;
    for (int k = 0; k < 7; k++) {
        std::swap(rowColour, otherColour);
        ty += 20;
        put(skills[k], 0, ty, rowColour);
        // (the "+N" artifact bonus after the name needs the name's measured end: shown at its right)
        if (c.artifacts[k])
            put(" +" + std::to_string(c.artifacts[k]), 150, ty, glm::vec4(0.5f, 1, 0.5f, 1));
        char value[32];
        if (k == 2)
            std::snprintf(value, sizeof value, "%s stam/s", tenth(c.travel[k]).c_str());
        else if (k == 6)
            std::snprintf(value, sizeof value, "%s m", tenth(c.travel[k]).c_str());
        else
            std::snprintf(value, sizeof value, "%.0f%%", std::round(c.travel[k]));
        put(value, 240, ty, rowColour);
    }
}

// --- What the pointer is on --------------------------------------------------------------

Hud::InvList Hud::bagList(const Inventory &inv) const {
    return {&inv.items, inv.tab, inv.scroll, inv.hoverItem, inv.hoverTab, inv.hoverButton};
}
Hud::InvList Hud::vendorList(const Inventory &inv) const {
    return {&inv.vendor.items, inv.vendor.tab, inv.vendor.scroll, inv.vendor.hoverItem, inv.vendor.hoverTab, inv.vendor.hoverButton};
}
Hud::InvList Hud::craftingList(const Inventory &inv) const {
    const Inventory::Crafting &c = inv.crafting;
    return {&c.items, c.tab, c.scroll, c.hoverItem, c.hoverTab, c.hoverButton, true, c.selected};
}
Hud::InvList Hud::listOf(const Inventory &inv, int which) const {
    return which == 1 ? vendorList(inv) : which == 2 ? craftingList(inv) : bagList(inv);
}
const Hud::InvPanel &Hud::invPanel(int which) const { return which == 1 ? panels.vendor : which == 2 ? panels.crafting : panels.bag; }

static bool inside(glm::vec2 m, glm::vec2 at, glm::vec2 size) { return m.x >= at.x && m.x < at.x + size.x && m.y >= at.y && m.y < at.y + size.y; }

int Hud::inventoryRowCount(const Inventory &inv, int which) const {
    return hasPanels() ? invRowCount(invPanel(which), listOf(inv, which)) : 1;
}

int Hud::inventoryButtonAt(glm::vec2 m, const Inventory &inv, int which) const {
    const InvPanel &p = invPanel(which);
    if (!hasPanels() || !p.node || !p.node->visible)
        return 0;
    placePanels(inv);
    int found = 0;
    for (auto [name, value] : {std::pair{"upbutton", 1}, std::pair{"downbutton", 2}})
        if (auto it = p.buttons.find(name); it != p.buttons.end() && it->second && it->second->widget &&
                                            inside(m, scene->contentPosition(*it->second), scene->screenSize(*it->second)))
            found = value;
    return found;
}

int Hud::panelItemAt(glm::vec2 m, const Inventory &inv, int which) const {
    const InvPanel &p = invPanel(which);
    if (!hasPanels() || !p.node || !p.node->visible)
        return -1;
    placePanels(inv);
    const InvList list = listOf(inv, which);
    const float scaleNow = scene->matrix(*p.node)[0][0];
    const glm::vec2 position = scene->contentPosition(*p.node);
    const std::vector<int> shown = shownOf(list);
    const int first = p.columns * list.scroll, last = std::min((int)shown.size(), first + p.columns * p.rows);
    for (int i = first; i < last; i++) {
        const int index = i - first;
        const glm::vec2 at((float)(index % p.columns * (panels.cellWidth + 5) + 10) * scaleNow + position.x,
                           (float)(index / p.columns * (panels.cellHeight + 5) + 40) * scaleNow + position.y);
        if (inside(m, at, glm::vec2((float)panels.cellWidth, (float)panels.cellHeight) * scaleNow))
            return shown[(size_t)i];
    }
    return -1;
}

bool Hud::craftButtonAt(glm::vec2 m, const Inventory &inv) const {
    if (!hasPanels() || !panels.details || !panels.craftButton || !panels.craftButton->widget || !inv.crafting.open)
        return false;
    placePanels(inv);
    return inside(m, scene->contentPosition(*panels.craftButton), scene->screenSize(*panels.craftButton));
}

int Hud::ingredientAt(glm::vec2 m, const Inventory &inv) const {
    if (!hasPanels() || !panels.details || !inv.crafting.open)
        return -1;
    placePanels(inv);
    const plasma::SceneNode &node = *panels.details;
    const float scaleNow = scene->matrix(node)[0][0];
    const glm::vec2 position = scene->contentPosition(node), content = scene->contentSize(node);
    const int columns = std::max(1, (int)((scene->viewSize(node).x - 10.0f) / 45.0f));
    for (int i = 0; i < (int)inv.crafting.details.ingredients.size(); i++) {
        const glm::vec2 at(std::floor((float)(20 + 70 * (i % columns)) * scaleNow + position.x),
                           std::floor((float)(10 + 45 * (i / columns)) * scaleNow + position.y + (content.y - 130.0f) * scaleNow));
        if (inside(m, at, glm::vec2(40.0f * scaleNow)))
            return i;
    }
    return -1;
}

int Hud::panelSlotAt(glm::vec2 m, const Inventory &inv) const {
    if (!hasPanels() || !inv.bagShown)
        return -1;
    placePanels(inv);
    for (size_t i = 0; i < panels.slots.size(); i++) {
        const plasma::SceneNode &slot = *panels.slots[i];
        if (slot.widget && inside(m, scene->contentPosition(slot), scene->screenSize(slot)))
            return (int)i;
    }
    return -1;
}

int Hud::panelTabAt(glm::vec2 m, const Inventory &inv, int which) const {
    const InvPanel &p = invPanel(which);
    if (!hasPanels() || !p.node || !p.node->visible)
        return -1;
    placePanels(inv);
    for (size_t t = 0; t < p.tabs.size(); t++)
        if (inside(m, scene->contentPosition(*p.tabs[t]), scene->screenSize(*p.tabs[t])))
            return (int)t;
    return -1;
}
