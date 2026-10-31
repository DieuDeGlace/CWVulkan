// Checks of the loot windows' wiring: the identify window (loot-shops.md Â§5), the weapon upgrade
// widget (loot-crafting.md Â§4) with its picture and picking, the upgrade cubes kept with the
// gear and in the save, and the chat's input line (frontend-options.md Â§3, netcode-packets.md
// Â§2.11).
#include "ChatLine.h"
#include "ChatCommands.h"
#include "GameCraft.h"
#include "Gear.h"
#include "IdentifyWindow.h"
#include "NativePlayer.h"
#include "NativeProgress.h"
#include "UpgradeWidget.h"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <functional>

namespace {
int failures = 0;
#define LOOT_CHECK(...)                                                                            \
    do {                                                                                           \
        if (!(__VA_ARGS__)) {                                                                      \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #__VA_ARGS__);                     \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

using Item = gameshop::Item;

Item makeItem(int type, int subtype, int material, int rarity, uint32_t modifier = 0, int rx = 0, int ry = 0) {
    const nativeplayer::Item made = nativeplayer::item(type, subtype, material, rarity, modifier, rx, ry);
    Item out{};
    std::memcpy(out.data(), made.data(), out.size());
    return out;
}

// A rand() stand-in that counts its draws and starts with a chosen parity.
struct Draws {
    uint32_t state = 12345;
    int calls = 0;
    int first = -1; // the first draw's value, when set
    int operator()() {
        calls++;
        if (calls == 1 && first >= 0)
            return first;
        state = state * 1103515245u + 12345u;
        return (int)((state >> 16) & 0x7fff);
    }
};

void identifyWindowRules() {
    identifywindow::Window w;
    w.show();
    LOOT_CHECK(identifywindow::price(w) == 0); // an empty slot costs nothing
    LOOT_CHECK(!identifywindow::canIdentify(w, 1, 1000));
    // A type-15 item of rarity r costs 2^r, so the window asks max(2^(r-1), 1).
    const Item rare3 = makeItem(gameshop::UNIDENTIFIED, 0, 0, 3, 777, 4, -2);
    w.put(rare3);
    LOOT_CHECK(identifywindow::price(w) == 4);
    LOOT_CHECK(identifywindow::canIdentify(w, 1, 4));
    LOOT_CHECK(!identifywindow::canIdentify(w, 1, 3)); // not enough gold
    LOOT_CHECK(!identifywindow::canIdentify(w, 0, 100)); // the stack is gone
    w.put(makeItem(gameshop::UNIDENTIFIED, 0, 0, 0));
    LOOT_CHECK(identifywindow::price(w) == 1); // rarity 0: at least 1
    w.put(makeItem(3, 0, 1, 2));                // a sword in the slot is priced but not identified
    LOOT_CHECK(identifywindow::price(w) >= 1 && !identifywindow::canIdentify(w, 1, 100000));

    inventoryslots::Inventory live;
    live.tabs[0] = {{3, rare3}, {1, makeItem(3, 0, 1, 2)}};
    w.bind({0, 0});
    LOOT_CHECK(w.resolve(live) && identifywindow::price(w) == 4);
    live.consumeOne({0, 0});
    LOOT_CHECK(w.resolve(live) && live.slot(w.selection)->count == 2);
    std::swap(live.tabs[0][0], live.tabs[0][1]);
    LOOT_CHECK(!w.resolve(live)); // refresh changed this pair's content; do not follow the original item
    live.tabs[0][0] = {-1, rare3};
    LOOT_CHECK(w.resolve(live) && identifywindow::canIdentify(w, -1, 4));
    w.close();
    w.show();
    LOOT_CHECK((w.selection == inventoryslots::Index{0, 0}) && w.resolve(live));
    w.visibleAtDown = false;
    LOOT_CHECK(!w.acceptsRelease(0)); // opening after down does not consume that release
    w.visibleAtDown = true;
    LOOT_CHECK(w.acceptsRelease(0) && !w.acceptsRelease(1));
    w.actionGate = 2;
    LOOT_CHECK(!identifywindow::canIdentify(w, 1, 100));
    w.actionGate = 1;

    // One press: an even first draw makes a weapon, an odd one armour; the rarity and the region
    // are the type-15 item's; the class is any (-1).
    w.put(rare3);
    Draws even;
    even.first = 2;
    const identifywindow::Outcome weapon = identifywindow::identifyOnce(w, std::ref(even));
    int32_t region[2];
    std::memcpy(region, &weapon.made[0x0c], 8);
    LOOT_CHECK(weapon.paid == 4 && weapon.made[0] == 3 && weapon.made[0x14] == 3);
    LOOT_CHECK(region[0] == 4 && region[1] == -2);
    LOOT_CHECK(even.calls == 1 + 16); // loot-shops.md Â§5: 1 + the weapon generator's 16
    Draws odd;
    odd.first = 1;
    const identifywindow::Outcome armour = identifywindow::identifyOnce(w, std::ref(odd));
    LOOT_CHECK(armour.made[0] >= 4 && armour.made[0] <= 7 && armour.made[0x14] == 3);
    LOOT_CHECK(odd.calls == 1 + 28); // 1 + the armour generator's 28 with class -1

    identifywindow::Layout geometry;
    geometry.valid = true;
    geometry.size = {350, 285};
    geometry.slotSize = {80, 96};
    geometry.nodeToScreen = glm::scale(glm::translate(glm::mat4(1), glm::vec3(500, 70, 0)), glm::vec3(1.5f, 2, 1));
    geometry.frameToScreen =
        geometry.nodeToScreen * glm::translate(glm::mat4(1), glm::vec3(9, 11, 0)) * glm::scale(glm::mat4(1), glm::vec3(2, .5f, 1));
    geometry.textToScreen = geometry.nodeToScreen * glm::translate(glm::mat4(1), glm::vec3(3, 4, 0));
    auto point = [&](glm::vec2 local) {
        const auto origin = geometry.frameToScreen * glm::vec4(0, 0, 0, 1);
        return glm::vec2(origin) + glm::vec2(local.x * 1.5f, local.y * 2);
    };
    LOOT_CHECK(geometry.onButton(point({100, 265})) && !geometry.onButton(point({100, 265}), false));
    LOOT_CHECK(geometry.onGoodbye(point({250, 265})));
    for (const auto p : {glm::vec2(0, 265), {175, 265}, {350, 265}, {100, 255}, {100, 285}})
        LOOT_CHECK(!geometry.onButton(point(p)) && !geometry.onGoodbye(point(p)));
    LOOT_CHECK(geometry.local(point({100, 265})) == glm::vec2(100, 265));
    // The frame's bind-origin offset does not enter navigation: live pivot then node projection, truncated twice.
    const auto points = geometry.navigation();
    LOOT_CHECK(points[0] == glm::vec2(678, 608) && points[1] == glm::vec2(854, 608));
}

// A rod 3 x 3 x 8 voxels standing up (a stand-in for a weapon model).
CubModel rod() {
    CubModel m;
    m.sx = 3, m.sy = 3, m.sz = 8;
    m.rgb.assign((size_t)m.sx * m.sy * m.sz * 3, 0);
    for (size_t i = 0; i < m.rgb.size(); i += 3)
        m.rgb[i] = 200, m.rgb[i + 1] = 180, m.rgb[i + 2] = 160;
    return m;
}

int cubes(const Item &w) { return w[0x9d]; }

void upgradeWidgetRules() {
    using namespace upgradewidget;
    // The palette: the material cubes carried, equal stacks counted together, other items left out.
    const Item iron = makeItem(CUBE_TYPE, CUBE_SUBTYPE, 1, 0), fire = makeItem(CUBE_TYPE, CUBE_SUBTYPE, 0x80, 0);
    const Item herb = makeItem(11, 0x14, 0, 0);
    const std::vector<PaletteEntry> palette = paletteOf({{iron, 3}, {herb, 9}, {fire, 1}, {iron, 2}});
    LOOT_CHECK(palette.size() == 2 && palette[0].count == 5 && palette[1].count == 1 && palette[1].item[0x1c] == 0x80);

    // Native palette: one exact base material, then field-only spirit totals in fixed order.
    inventoryslots::Inventory canonical;
    canonical.tabs.resize(9);
    const auto nativeWeapon = nativesaves::makeItem(3, 0, 1);
    const auto base = nativesaves::makeItem(11, 10, 1);
    const auto spirit = nativesaves::makeItem(11, 14, 128);
    auto foreignSpirit = spirit;
    foreignSpirit[0xc] = 9;
    foreignSpirit[8] = 7;
    auto inactiveBase = base;
    inactiveBase[0x1d] = 91;
    canonical.tabs[0] = {{5, base}, {-2, inactiveBase}, {6, foreignSpirit}, {2, spirit}};
    canonical.tabs[8] = {{-3, spirit},
                         {-4, nativesaves::makeItem(11, 14, 129)},
                         {100, nativesaves::makeItem(11, 14, 127)},
                         {100, nativesaves::makeItem(11, 10, 2)}};
    const auto nativePalette = paletteOf(canonical, nativeWeapon);
    LOOT_CHECK(nativePalette.size() == 3);
    if (nativePalette.size() == 3) {
        LOOT_CHECK(nativePalette[0].count == 3 && nativePalette[0].item == base);
        LOOT_CHECK(nativePalette[1].count == 5 && nativePalette[1].item == spirit);
        LOOT_CHECK(nativePalette[2].count == -4 && nativePalette[2].item[0x1c] == 129);
    }
    canonical.tabs = {{{INT32_MAX, base}, {2, base}}};
    const auto wrapped = paletteOf(canonical, nativeWeapon);
    LOOT_CHECK(wrapped.size() == 1 && wrapped[0].count == INT32_MIN + 1);
    canonical.tabs = {{{1, spirit}, {-1, foreignSpirit}}};
    LOOT_CHECK(paletteOf(canonical, nativeWeapon).empty());
    LOOT_CHECK(paletteOf(canonical, Item{}).empty());
    // A stale retained selection can still place before its source search finds no match.
    Widget retained;
    retained.palette = nativePalette;
    retained.selected = 1;
    retained.pointed = Voxel{1, -1, 4};
    auto retainedWeapon = nativeWeapon;
    const auto stale = click(retained, retainedWeapon.data(), false);
    LOOT_CHECK(stale.weaponChanged && stale.takeOne && stale.taken == spirit && retainedWeapon[0x9d] == 1);

    Item sword = makeItem(3, 0, 1, 2); // one-handed: 16 cubes
    Widget w;
    w.open = true;
    w.palette = palette;
    // Placing from the palette takes one of its item.
    w.selected = 1;
    w.pointed = Voxel{1, -1, 4};
    ClickResult r = click(w, sword.data(), false);
    LOOT_CHECK(r.weaponChanged && r.takeOne && r.taken[0x1c] == 0x80 && cubes(sword) == 1);
    LOOT_CHECK((int8_t)sword[0x1d] == 1 && (int8_t)sword[0x1e] == -1 && sword[0x1f] == 4 && sword[0x20] == 0x80);
    // On the same voxel it recolours, still taking the item.
    w.selected = 0;
    r = click(w, sword.data(), false);
    LOOT_CHECK(r.takeOne && cubes(sword) == 1 && sword[0x20] == 1);
    LOOT_CHECK(w.selected == 0); // (the selection stays for the next cube)
    // Clicking off the weapon ends the selection.
    w.pointed.reset();
    r = click(w, sword.data(), false);
    LOOT_CHECK(!r.takeOne && w.selected == -1 && cubes(sword) == 1);
    // Picking the cube up changes nothing yet; putting it down moves it, for free.
    w.pointed = Voxel{1, -1, 4};
    r = click(w, sword.data(), false);
    LOOT_CHECK(w.carrying && !r.weaponChanged && w.carriedMaterial == 1);
    w.pointed = Voxel{3, 1, 2};
    r = click(w, sword.data(), false);
    LOOT_CHECK(!w.carrying && r.weaponChanged && !r.takeOne && cubes(sword) == 1 && sword[0x1d] == 3 && sword[0x20] == 1);
    // Let go over another widget: kept; let go over nothing: gone.
    r = click(w, sword.data(), false); // (picked up again)
    w.pointed.reset();
    r = click(w, sword.data(), true);
    LOOT_CHECK(!w.carrying && cubes(sword) == 1);
    w.pointed = Voxel{3, 1, 2};
    click(w, sword.data(), false);
    w.pointed.reset();
    r = click(w, sword.data(), false);
    LOOT_CHECK(r.weaponChanged && cubes(sword) == 0);
    // A one-handed weapon holds 16; the 17th is refused by the place routine but its item is
    // still taken (as the spec reads 0x292650, its open question 5).
    w.selected = 0;
    for (int8_t i = 0; i < 16; i++) {
        w.pointed = Voxel{i, 0, 0};
        click(w, sword.data(), false);
    }
    LOOT_CHECK(cubes(sword) == 16 && gamecraft::cubeCap(sword.data()) == 16);
    w.pointed = Voxel{0, 1, 0};
    r = click(w, sword.data(), false);
    LOOT_CHECK(!r.weaponChanged && r.takeOne && cubes(sword) == 16);
    // A two-handed weapon holds 32; at 32 the widget places nothing and takes nothing.
    Item greatsword = makeItem(3, 5, 1, 2);
    LOOT_CHECK(gamecraft::cubeCap(greatsword.data()) == 32);
    for (int8_t i = 0; i < 32; i++) {
        w.pointed = Voxel{i, 2, 0};
        click(w, greatsword.data(), false);
    }
    w.pointed = Voxel{0, 3, 0};
    r = click(w, greatsword.data(), false);
    LOOT_CHECK(cubes(greatsword) == 32 && !r.takeOne && !r.weaponChanged);
    // No weapon in the hand: nothing happens but the selection ending.
    w.pointed = Voxel{0, 0, 0};
    r = click(w, nullptr, false);
    LOOT_CHECK(!r.takeOne && !r.weaponChanged && w.selected == -1);
}

void upgradePicture() {
    using namespace upgradewidget;
    const CubModel model = rod();
    Item weapon = makeItem(3, 0, 1, 0);
    View view;
    view.pixels = 128;
    // The picture shows the rod at its middle and nothing in the corners.
    std::vector<uint8_t> picture = render(model, weapon.data(), view);
    LOOT_CHECK(picture.size() == 128u * 128u * 4u);
    LOOT_CHECK(picture[(64u * 128u + 64u) * 4u + 3u] == 255 && picture[3] == 0);
    // Pointing at the middle of the rod gives the empty cell in front of the face pointed at:
    // outside the rod, within one voxel of it.
    const std::optional<Voxel> front = pick(model, weapon.data(), view, {64, 64});
    LOOT_CHECK(front.has_value());
    if (front) {
        const bool outside = front->x < 0 || front->y < 0 || front->x > 2 || front->y > 2 || front->z < 0 || front->z > 7;
        LOOT_CHECK(outside && inReach(model, *front));
        // A cube placed there is drawn in its material's colour, and pointing at it gives the
        // cube itself (to recolour or pick up).
        LOOT_CHECK(gamecraft::placeCube(weapon.data(), front->x, front->y, front->z, 0x80));
        const glm::vec2 at = project(model, view, *front);
        const std::optional<Voxel> again = pick(model, weapon.data(), view, at);
        LOOT_CHECK(again && *again == *front);
        picture = render(model, weapon.data(), view);
        const uint8_t *px = &picture[((size_t)at.y * 128u + (size_t)at.x) * 4u];
        LOOT_CHECK(px[3] == 255 && px[0] > px[2]); // fire: orange
    }
    // Missing everything: no voxel.
    LOOT_CHECK(!pick(model, weapon.data(), view, {1, 1}));
    // The view turned: the same voxel lands elsewhere, and the scale does not change.
    View turned = view;
    turned.yawDegrees += 90;
    LOOT_CHECK(std::abs(voxelPixels(model, view) - voxelPixels(model, turned)) < 1e-4f);
    // The layout's parts.
    const Layout l = layout({1600, 900});
    LOOT_CHECK(l.onView(l.viewAt + glm::vec2(5)) && l.inWindow(l.viewAt + glm::vec2(5)));
    LOOT_CHECK(l.paletteBoxAt(l.paletteBox(7) + glm::vec2(2)) == 7 && l.paletteBoxAt(l.viewAt) == -1);
    LOOT_CHECK(l.turnAt(l.turnLeftAt + glm::vec2(2)) == -1 && l.turnAt(l.turnRightAt + glm::vec2(2)) == 1);
    LOOT_CHECK(glm::length(l.viewPixel(l.viewAt + glm::vec2(l.viewSize), 256) - glm::vec2(256)) < 1e-3f);
}

// The cubes go with the gear: into the record the native player wears and the save keeps, and
// back.
void cubesKept() {
    Gear::Item sword;
    sword.category = 3, sword.id = 0, sword.material = 1, sword.rarity = 2, sword.modifier = 99;
    sword.cubeCount = 2;
    const uint8_t two[8] = {1, 0xff, 4, 0x80, 2, 2, 2, 0x82};
    std::memcpy(sword.cubes.data(), two, sizeof two);
    nativeplayer::Item record = nativeplayer::item(3, 0, 1, 2, 99, 0, 0);
    Gear::cubesInto(sword, record.data());
    LOOT_CHECK(record[0x9d] == 2 && record[0x1d] == 1 && (int8_t)record[0x1e] == -1 && record[0x24] == 0x82);
    Gear::Item back;
    Gear::cubesFrom(record.data(), back);
    LOOT_CHECK(back.cubeCount == 2 && back.cubes == sword.cubes);

    // The save: what is worn keeps its cubes in characters.db's record, and loads with them.
    Gear gear;
    sword.found = sword.measured = true;
    const size_t i = gear.add(sword, Gear::WeaponRight);
    gear.equip(i);
    Gear::Item changed = gear.equipped(Gear::WeaponRight);
    changed.cubes[8] = 0, changed.cubes[9] = 1, changed.cubes[10] = 1, changed.cubes[11] = 0x81, changed.cubeCount = 3;
    gear.updateWorn(Gear::WeaponRight, changed);
    LOOT_CHECK(gear.equipped(Gear::WeaponRight).cubeCount == 3 && gear.item(i).cubeCount == 3);
    nativesaves::Character c;
    std::string notes;
    SaveGame s;
    nativeprogress::fromProgress(s, &gear, 100.0f, nativeprogress::Frame{}, c, notes);
    const nativesaves::Item &worn = c.equipment[Gear::WeaponRight];
    LOOT_CHECK(worn[0] == 3 && worn[0x9d] == 3 && worn[0x28] == 0x81);
    const SaveGame loaded = nativeprogress::toProgress(c, notes, nativeprogress::Frame{}, nullptr);
    bool found = false;
    for (const SaveGame::Found &f : loaded.found)
        if (f.category == 3 && f.cubeCount == 3 && f.cubes == changed.cubes)
            found = true;
    LOOT_CHECK(found);
}

void chatCommands() {
    using namespace chatcommand;
    LOOT_CHECK(parse(u"  /wave extra").kind == Kind::Motion && parse(u"  /wave extra").ability == 0xa1);
    LOOT_CHECK(parse(u"/sit").ability == 0x57 && parse(u"/pet").ability == 0xa2 && parse(u"/dance").ability == 0xa3);
    LOOT_CHECK(parse(u"/Wave").kind == Kind::Text && parse(u"/unknown").kind == Kind::Text);
    LOOT_CHECK(parse(u"/check ignored").kind == Kind::Check);
    LOOT_CHECK(parse(u"/namepet One Two").argument == u"One" && parse(u"/namepet").argument.empty());
    std::array<uint8_t, 0xa0> item;
    item.fill(0xa5);
    item[0] = 20;
    const auto before = item;
    LOOT_CHECK(!namePet(item.data(), u"") && !namePet(item.data(), std::u16string(16, u'X')) && item == before);
    LOOT_CHECK(namePet(item.data(), u"A\u0142"));
    for (size_t i = 0; i < item.size(); ++i) {
        const uint8_t expected = i == 0x20 ? 'A' : i == 0x24 ? 0x42 : i == 0x9d ? 2 : before[i];
        LOOT_CHECK(item[i] == expected);
    }
    item[0] = 3;
    LOOT_CHECK(!namePet(item.data(), u"Pet"));
    std::array<uint8_t, 0x100> creature{};
    float clock = 100;
    std::memcpy(creature.data() + 0x6c, &clock, 4);
    LOOT_CHECK(!startMotion(creature.data(), 100, 0xa3)); // strict clock gate
    LOOT_CHECK(startMotion(creature.data(), 99, 0xa3) && creature[0x68] == 0xa3);
    std::memcpy(&clock, creature.data() + 0x6c, 4);
    LOOT_CHECK(clock == 0 && !startMotion(nullptr, 0, 0x57));
}

void chatLine() {
    ChatLine chat;
    LOOT_CHECK(!chat.typing());
    chat.type("ignored");
    LOOT_CHECK(!chat.enter() && chat.typing() && chat.text().empty());
    chat.type("hello\x01 there");
    chat.backspace();
    LOOT_CHECK(chat.text() == u"hello ther");
    chat.key(ChatLine::Key::Home);
    chat.key(ChatLine::Key::Right, true);
    chat.key(ChatLine::Key::Right, true);
    LOOT_CHECK(chat.caretIndex() == 2 && chat.selectedUnits() == -2);
    chat.type("H");
    LOOT_CHECK(chat.text() == u"Hllo ther" && chat.caretIndex() == 1);
    chat.key(ChatLine::Key::End);
    chat.key(ChatLine::Key::Left, true);
    chat.key(ChatLine::Key::Delete);
    LOOT_CHECK(chat.text() == u"Hllo the");
    chat.stopEditing();
    LOOT_CHECK(!chat.typing() && chat.text() == u"Hllo the");
    chat.enter();
    LOOT_CHECK(chat.text() == u"Hllo the");
    const auto sent = chat.enter();
    LOOT_CHECK(sent && *sent == u"Hllo the" && !chat.typing() && chat.text().empty());
    chat.enter();
    LOOT_CHECK(!chat.enter() && !chat.typing());
    chat.enter();
    chat.type(std::string(1, (char)0xc8));
    LOOT_CHECK(chat.text() == std::u16string(1, (char16_t)0xffc8));
    LOOT_CHECK(!ChatLine::caretVisible(0) && !ChatLine::caretVisible(499));
    LOOT_CHECK(ChatLine::caretVisible(500) && ChatLine::caretVisible(999) && !ChatLine::caretVisible(1000));

    const auto measure = [](const std::wstring &) { return 10.75f; };
    ChatLine history;
    history.append(u"A B", {1, 2, 3}, 55, measure); // equality wraps: 10+5+10 == 25
    LOOT_CHECK(history.rows().size() == 2 && history.rows()[0].size() == 2);
    LOOT_CHECK(history.rows()[1][0].text == L"B");
    history.append(u"C", {4, 5, 6}, 50, measure); // old row width retains its fraction; one glue allowance
    LOOT_CHECK(history.rows().size() == 2 && history.rows()[1].size() == 2);
    LOOT_CHECK(history.rows()[1][1].colour == ChatLine::Colour({4, 5, 6}));
    history.append(u"\r\n", {1, 1, 1}, 400, measure);
    LOOT_CHECK(history.rows().size() == 4 && history.rows()[2].empty() && history.rows()[3].empty());
    history.trim(36);
    LOOT_CHECK(history.rows().size() == 2 && history.rows()[0].empty());

    ChatLine glue;
    glue.append(u"A", {0, 0, 0}, 50, measure);
    glue.append(u"\nB", {0, 0, 0}, 35, measure); // glue survives the explicit break
    LOOT_CHECK(glue.rows().size() == 2 && glue.rows()[1][0].text == L"B");
    ChatLine spaces;
    spaces.append(u"A B", {0, 0, 0}, 50, [](const auto &) { return 15.75f; });
    LOOT_CHECK(spaces.rows().size() == 3 && spaces.rows()[1].empty() && spaces.rows()[2][0].text == L"B");

    ChatLine delivery;
    delivery.receive(u"Ada", u"hello");
    delivery.receive(std::nullopt, u"\u03a9");
    delivery.consumeIncoming(400, measure);
    LOOT_CHECK(delivery.pending() == 1 && delivery.rows().size() == 2);
    LOOT_CHECK(delivery.rows()[0][0].text == L"Ada:" && delivery.rows()[0][0].colour == ChatLine::Colour({0, 255, 255}));
    LOOT_CHECK(delivery.rows()[0].back().text == L"hello" && delivery.rows()[0].back().colour == ChatLine::Colour({255, 255, 255}));
    delivery.consumeIncoming(400, measure);
    LOOT_CHECK(delivery.pending() == 0 && delivery.rows()[1][0].text == L"\u03a9");
    delivery.receive((int64_t)12, u"later");
    std::u16string currentName = u"Renamed";
    delivery.consumeIncoming(400, measure, [&](int64_t id) -> std::optional<std::u16string> {
        LOOT_CHECK(id == 12);
        return currentName;
    });
    LOOT_CHECK(delivery.rows()[2][0].text == L"Renamed:");
}

} // namespace

// `makesave <folder> <recording> loot`: the test character also carries what the loot windows
// take: an unidentified item, material cubes of four materials, and gold.
int addTestLoot(const char *folder) {
    nativesaves::CharacterStore store;
    nativesaves::Character c;
    if (!store.open(std::filesystem::path(folder) / "characters.db") || !store.load(0, c))
        return std::printf("no character 0 in %s\n", folder), 1;
    c.inventory.resize(nativesaves::INVENTORY_TABS);
    auto add = [&](size_t tab, int count, const Item &item) {
        nativesaves::Stack s;
        s.count = count;
        std::memcpy(s.item.data(), item.data(), item.size());
        c.inventory[tab].push_back(s);
    };
    add(0, 2, makeItem(gameshop::UNIDENTIFIED, 0, 0, 2, 4242));
    add(3, 6, makeItem(upgradewidget::CUBE_TYPE, upgradewidget::CUBE_SUBTYPE, 1, 0));
    add(3, 3, makeItem(upgradewidget::CUBE_TYPE, upgradewidget::CUBE_SUBTYPE, 0x80, 0));
    add(3, 3, makeItem(upgradewidget::CUBE_TYPE, upgradewidget::CUBE_SUBTYPE, 0x82, 0));
    add(3, 2, makeItem(upgradewidget::CUBE_TYPE, upgradewidget::CUBE_SUBTYPE, 0x83, 0));
    c.gold = 500;
    if (!store.save(0, c))
        return 1;
    std::printf("character 0: an unidentified item, 14 material cubes and 500 gold added\n");
    return 0;
}

int lootWindowTests() {
    failures = 0;
    identifyWindowRules();
    upgradeWidgetRules();
    upgradePicture();
    cubesKept();
    chatLine();
    chatCommands();
    std::printf("loot windows: identify, upgrade widget, cubes kept, chat line %s\n", failures ? "FAILED" : "ok");
    return failures;
}
