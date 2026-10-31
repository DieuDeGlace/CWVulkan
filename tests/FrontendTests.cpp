// Checks of the front screens' rules (src/Frontend.*) against docs/specs/frontend-title.md,
// frontend-characters.md and frontend-options.md, the settings files (src/GameOptions.*), the
// progress kept in the game's format (src/NativeProgress.*) and the class-adapted treasure item
// (NativePlayer, docs/specs/player-item-adaptation.md).
#include "Assets.h"
#include "Extracted.h"
#include "Frontend.h"
#include "Hud.h"
#include "GameInput.h"
#include "GameOptions.h"
#include "NativeInteract.h"
#include "ZoneStates.h"
#include "NativePlayer.h"
#include "NativePose.h"
#include "NativeProgress.h"
#include "TalkGen.h"
#include "worldgen/Generator.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

namespace {
int failures = 0;
#define FRONT_CHECK(...)                                                                           \
    do {                                                                                           \
        if (!(__VA_ARGS__)) {                                                                      \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #__VA_ARGS__);                    \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

bool near(double a, double b, double tolerance = 1e-4) { return std::abs(a - b) <= tolerance; }

using namespace frontend;

void titleCamera() {
    OrbitCamera c;
    c.set(90, 0, 180, 8);
    // Yaw 180, pitch 90: the eye is south of the target (−y), level with it.
    const glm::dvec3 eye = c.eye({0, 0, 0});
    FRONT_CHECK(near(eye.x, 0, 1e-3) && near(eye.y, -8, 1e-3) && near(eye.z, 0, 1e-3));
    // Yaw 0: north of it (+y). Pitch 110: above it, looking down.
    c.set(110, 0, 0, 12);
    const glm::dvec3 north = c.eye({0, 0, 0});
    FRONT_CHECK(north.y > 11 && north.z > 4 && near(north.z, -12 * std::cos(110 * 0.0174533), 1e-3));
    // The title's targets: 3 degrees a second, pitch 110, distance 12.
    c.set(90, 0, 180, 8);
    c.titleTargets(1000);
    FRONT_CHECK(near(c.targetYaw, 183) && c.targetPitch == 110 && c.targetDistance == 12 && c.yaw == 180);
    // Smoothing moves the current values part of the way; smoothness 0 is the quickest.
    OrbitCamera slow = c, quick = c;
    slow.smooth(16, 100), quick.smooth(16, 0);
    FRONT_CHECK(slow.yaw > 180 && slow.yaw < quick.yaw && quick.yaw < 183 && slow.distance > 8 && slow.distance < 12);
}

void logo() {
    // At a whole period (cos = 1): the centre is at 30 % of the height, tilted 15 degrees.
    const LogoPlacement p = logoPlacement(1920, 1080, 1, 0);
    FRONT_CHECK(near(p.centre.x, 960) && near(p.centre.y, 1080 * 0.3, 1e-2) && near(p.tiltDegrees, 15, 1e-4));
    FRONT_CHECK(near(p.scale, 0.011 * 1024 / 1080, 1e-6));
    // Half a period later: 20 % and 25 degrees; the motion repeats every 10 seconds.
    const LogoPlacement half = logoPlacement(1920, 1080, 1, 5000), again = logoPlacement(1920, 1080, 1, 10000);
    FRONT_CHECK(near(half.centre.y, 1080 * 0.2, 1e-2) && near(half.tiltDegrees, 25, 1e-3) && near(again.centre.y, p.centre.y, 1e-3));
    // The clip transform puts the model's middle at the centre.
    const glm::mat4 m = logoClip(p, {66, 31, 8}, 1920, 1080);
    const glm::vec4 middle = m * glm::vec4(33, 15.5f, 4, 1);
    FRONT_CHECK(middle.w > 0 && near(middle.x / middle.w, 0, 1e-4) && near(middle.y / middle.w, 2 * 0.3 - 1, 1e-3) &&
                middle.z / middle.w > 0 && middle.z / middle.w < 1);
    // A voxel at the middle is `scale` clip units of the height; the +z face is nearer the eye
    // than the -z face (it is the one seen, as on the game's title), x runs to the right, and a
    // voxel's side shows away from the middle.
    const glm::vec4 above = m * glm::vec4(33, 16.5f, 4, 1);
    FRONT_CHECK(near(std::abs(above.y / above.w - middle.y / middle.w), p.scale * std::cos(15 * 0.0174533), 1e-4));
    const glm::vec4 front = m * glm::vec4(33, 15.5f, 8, 1), back = m * glm::vec4(33, 15.5f, 0, 1);
    FRONT_CHECK(front.z / front.w < back.z / back.w);
    const glm::vec4 rightFront = m * glm::vec4(66, 15.5f, 8, 1), rightBack = m * glm::vec4(66, 15.5f, 0, 1);
    FRONT_CHECK(rightFront.x / rightFront.w > middle.x / middle.w);
    FRONT_CHECK(rightBack.x / rightBack.w < rightFront.x / rightFront.w); // (perspective: the far edge nearer the middle)
    FRONT_CHECK(screenModelShade(2, -1) > screenModelShade(1, 1) && screenModelShade(1, 1) > screenModelShade(0, 1));
    FRONT_CHECK(screenModelShade(2, 1, 1) > screenModelShade(2, -1, 1));
}

// The multiplayer menu's rows (netcode-transport.md §3.2): the name with " (host)", then
// " (joined)" or " (invited)", and the two buttons' words.
void friendRows() {
    FriendRow row;
    row.name = "Ann";
    FRONT_CHECK(friendRowText(row) == "Ann" && std::string(hostButtonLabel(row)) == "invite" && std::string(guestButtonLabel(row)) == "join");
    row.invited = true;
    FRONT_CHECK(friendRowText(row) == "Ann (invited)" && std::string(hostButtonLabel(row)) == "uninvite");
    row.joined = true;
    FRONT_CHECK(friendRowText(row) == "Ann (joined)");
    row = FriendRow{};
    row.name = "Bo", row.hostOfMine = true, row.invited = true;
    FRONT_CHECK(friendRowText(row) == "Bo (host) (invited)" && std::string(guestButtonLabel(row)) == "leave");
}

void menus() {
    const auto without = rootLines(false), with = rootLines(true);
    FRONT_CHECK(without.size() == 3 && with.size() == 4);
    FRONT_CHECK(without[1].index == OPTIONS && without[1].textY == 80 && without[2].textY == 130);
    FRONT_CHECK(with[1].index == CONTINUE && with[2].textY == 130 && with[3].textY == 180 && with[3].navigationY == 175);
    FRONT_CHECK(hoveredLine(with, {100, 10}, 200) == START_GAME && hoveredLine(with, {100, 39.9f}, 200) == START_GAME);
    FRONT_CHECK(hoveredLine(with, {100, 45}, 200) == -1 && hoveredLine(with, {100, 60}, 200) == CONTINUE);
    FRONT_CHECK(hoveredLine(with, {200, 15}, 200) == -1 && hoveredLine(with, {-1, 15}, 200) == -1);
    FRONT_CHECK(hoveredLine(without, {0, 115}, 200) == EXIT && hoveredLine(without, {0, 105}, 200) == -1);
    const glm::vec2 at = rootMenuPosition({1920, 1080});
    FRONT_CHECK(at.x == 860 && near(at.y, (1.6 * 1080 - 100) / 2));
    const auto system = systemLines();
    FRONT_CHECK(system.size() == 4 && hoveredLine(system, {10, 100}, 150) == SYSTEM_EXIT && hoveredLine(system, {10, 75}, 150) == SYSTEM_START_MENU);
    FRONT_CHECK(entryPosition(2) == glm::vec2(300, 510) && listHeight(3) == 680 && centredListTop(3, 1080) == 200);
    const auto buttons = characterButtons({298, 108}, {600, 220}, {200, 70}, {32, 32});
    FRONT_CHECK(buttons.select == glm::vec2(678, 238)); // actual70-high owner frame, not the requested30
    FRONT_CHECK(buttons.select.y + 70 == 108 + 220 - 20);
    FRONT_CHECK(buttons.deleteToggle == glm::vec2(846, 128) && buttons.deleteCharacter == glm::vec2(918, 108));
    const auto deformed = characterButtons({300, 100}, {640, 240}, {210, 90}, {40, 28});
    FRONT_CHECK(deformed.select == glm::vec2(710, 230) && deformed.deleteToggle == glm::vec2(880, 120));
    FRONT_CHECK(std::string(specializationWord(3, 1)) == "Water Mage" && std::string(classWord(4)) == "Rogue");
    FRONT_CHECK(entryLine(7, 2, 1) == "LV 7 Human Warrior" && entryLine(1, 14, 3) == "LV 1  Mage");
}

void creation() {
    Look l;
    l.reset();
    FRONT_CHECK(l.raceId() == 2 && l.committed[0] == 255 && l.committed[1] == 200 && l.preview[2] == 255);
    l.arrow(ROW_RACE, -1);
    FRONT_CHECK(l.race == 7 && l.raceId() == 13); // frogman
    l.arrow(ROW_RACE, 1), l.arrow(ROW_GENDER, -1);
    FRONT_CHECK(l.race == 0 && l.sex == 1 && l.raceId() == 3);
    l.arrow(ROW_CLASS, -1);
    FRONT_CHECK(l.cls == 3 && l.value(ROW_CLASS) == "Rogue");
    l.arrow(ROW_FACE, -1), l.wrap();
    FRONT_CHECK(l.face == facesOf(3) - 1 && l.value(ROW_FACE) == "Face 6");
    l.face = 6, l.wrap();
    FRONT_CHECK(l.face == 0);
    FRONT_CHECK(facesOf(7) == 2 && haircutsOf(2) == 15 && haircutsOf(9) == 3);
    l.preview = {1, 2, 3};
    const nativesaves::Looks rec = l.record();
    FRONT_CHECK(rec.people() == 0 && rec.sex() == 1 && rec.hairColour()[2] == 3);
    // The hair grid.
    FRONT_CHECK(hairCellColour(0, 3) == glm::vec3(1, 0, 0) && near(hairCellColour(0, 0).r, 0.2));
    FRONT_CHECK(near(hairCellColour(10, 5).b, 1) && near(hairCellColour(10, 5).r, 0.5));
    int c, r;
    FRONT_CHECK(hairCellAt({13, 197}, c, r) && c == 0 && r == 0);
    FRONT_CHECK(hairCellAt({13 + 17 * 15 + 16.9f, 197 + 17 * 6 + 1}, c, r) && c == 15 && r == 6);
    FRONT_CHECK(!hairCellAt({12.9f, 197}, c, r) && !hairCellAt({13 + 17 * 16, 197}, c, r) && !hairCellAt({20, 197 + 17 * 7}, c, r));
    // Names: 2 to 15 UTF-16 units, any characters.
    FRONT_CHECK(!nameAccepted("a") && nameAccepted("ab") && nameAccepted("fifteen chars!!") && !nameAccepted("sixteen chars!!!"));
    FRONT_CHECK(nameAccepted("\xc3\xa9\xc3\xa9") && nameAccepted("\xf0\x9f\x98\x80")); // two accented letters; a character beyond 0xffff is two units
}

void startingKit() {
    using nativesaves::item::MODIFIER;
    auto kitFor = [](int cls) {
        nativesaves::Character c = newCharacter({1, 2}, {0, 0, 0});
        c.classType = (uint8_t)cls;
        giveStartingKit(c, 0x12345678u, {7, 8});
        return c;
    };
    auto i32 = [](const nativesaves::Item &it, size_t at) {
        int32_t v;
        std::memcpy(&v, &it[at], 4);
        return v;
    };
    const nativesaves::Character w = kitFor(1);
    FRONT_CHECK(w.equipment[7][0] == 3 && i32(w.equipment[7], 4) == 17 && w.equipment[7][0x1c] == 2);
    FRONT_CHECK(w.equipment[6][0] == 0 && w.equipment[2][0] == 4 && w.equipment[2][0x1c] == 1);
    FRONT_CHECK(w.inventory.size() == 6 && w.inventory[0].size() == 10 && w.inventory[3].size() == 30);
    FRONT_CHECK(w.inventory[0][0].count == 1 && i32(w.inventory[0][2].item, 4) == 13 && w.inventory[0][3].count == 0);
    FRONT_CHECK(w.inventory[2][0].count == 5 && w.inventory[2][0].item[0] == 1); // five of (1, 1), stacked
    // The modifiers: rand() after srand(account), right weapon, chest, then the carried gear.
    talkgen::CRand rand;
    rand.seed(0x12345678u);
    const int first = rand.next(), second = rand.next(), third = rand.next();
    FRONT_CHECK(i32(w.equipment[7], MODIFIER) == first && i32(w.equipment[2], MODIFIER) == second &&
                i32(w.inventory[0][0].item, MODIFIER) == third);
    FRONT_CHECK(i32(w.equipment[7], 0x0c) == 7 && i32(w.equipment[7], 0x10) == 8);
    const nativesaves::Character rogue = kitFor(4);
    FRONT_CHECK(i32(rogue.equipment[6], 4) == 3 && i32(rogue.equipment[7], 4) == 3 && rogue.equipment[2][0x1c] == 28);
    const nativesaves::Character mage = kitFor(3);
    FRONT_CHECK(i32(mage.equipment[7], 4) == 10 && mage.inventory[0][0].item[0x1c] == 11 && mage.inventory[0][1].item[0x1c] == 11);
    // Recipes: the seven of argument 1, plus 7 + 2, the class's one and four of argument 0.
    FRONT_CHECK(w.recipes.size() == 7 + 9 + 4 && mage.recipes.size() == 7 + 9 + 1 + 4);
    nativesaves::Character none;
    addDefaultRecipes(none, 1);
    FRONT_CHECK(none.recipes.size() == 7 && none.recipes[0][0x14] == 1);
    nativesaves::Character loaded;
    loaded.classType = 1;
    auto modified = nativesaves::makeItem(1, 1);
    modified[8] = 42;
    modified[0x9d] = 1;
    modified[0x1d] = 7;
    loaded.recipes = {modified};
    addDefaultRecipes(loaded, 0);
    FRONT_CHECK(loaded.recipes.size() == 21 && loaded.recipes.front() == modified && loaded.recipes[1] == nativesaves::makeItem(1,1));
    loaded.recipes[1][1] = 0x55;
    loaded.recipes[1][0x16] = 0x66;
    loaded.recipes[1][0x90] = 0x77;
    const auto retained = loaded.recipes;
    addDefaultRecipes(loaded, 0);
    FRONT_CHECK(loaded.recipes == retained);
}

void optionsFiles() {
    gameoptions::Options o;
    FRONT_CHECK(o.renderDistance == 50 && o.guiScale == 1.0f && o.antiAliasingSamples == 8);
    o.cameraSmoothness = 0;
    FRONT_CHECK(near(o.smoothingConstant(), 1));
    o.minTimeStep = 16;
    FRONT_CHECK(o.fpsLimitText() == "62 fps" && gameoptions::Options{}.fpsLimitText() == "none");
    o.guiScale = 1.25f;
    const std::string text = gameoptions::writeOptions(o);
    FRONT_CHECK(text.rfind("fullscreen 1\nresolutionX 0\n", 0) == 0 && text.find("guiScale 1.25\nmapScale 1\n") != std::string::npos);
    const gameoptions::Options back = gameoptions::readOptions(text);
    FRONT_CHECK(gameoptions::writeOptions(back) == text);
    // Unknown names are skipped (their value then as a word); a bad value ends the reading.
    const gameoptions::Options odd = gameoptions::readOptions("bogus 5 musicVolume 30 soundVolume x musicLoop 1");
    FRONT_CHECK(odd.musicVolume == 30 && odd.soundVolume == 100 && odd.musicLoop == 0);
    const gameoptions::Controls def = gameoptions::defaultControls();
    FRONT_CHECK(def[gameoptions::SYSTEM_MENU].code == 0x18 && def[gameoptions::ATTACK].kind == 1 && def[gameoptions::ZOOM_IN].code == 0);
    gameoptions::Controls changed = gameoptions::readControls("ButtonJump 57 0\nButtonDodge 3 1\nNope 1 1\n");
    FRONT_CHECK(changed[gameoptions::DODGE].code == 3 && changed[gameoptions::DODGE].kind == 1 && changed[gameoptions::USE] == def[gameoptions::USE]);
    FRONT_CHECK(gameoptions::readControls(gameoptions::writeControls(changed)) == changed);
    FRONT_CHECK(gameoptions::bindingName({0, 0}) == "Unassigned" && gameoptions::bindingName({2, 1}) == "Mouse 2" &&
                gameoptions::bindingName({0x11, 0}) == "W");
    // The options screen's arrows.
    OptionsScreen s;
    s.open(gameoptions::Options{}, {{1280, 720, {60}}, {1920, 1080, {60, 144}}});
    s.arrow(OPT_RENDER_DISTANCE, 1);
    for (int i = 0; i < 20; i++)
        s.arrow(OPT_MUSIC, 1);
    s.arrow(OPT_FPS, -1), s.arrow(OPT_CPU, -1), s.arrow(OPT_CAMERA_SPEED, -1);
    FRONT_CHECK(s.values.renderDistance == 55 && s.values.musicVolume == 100 && s.values.minTimeStep == 1);
    FRONT_CHECK(s.values.cpuPerformance == 0 && s.value(OPT_CPU) == "REDUCED" && s.values.cameraSpeed == 45);
    s.values.refreshRate = 120;
    s.arrow(OPT_RESOLUTION, 1);
    FRONT_CHECK(s.values.resolutionX == 1920 && s.values.refreshRate == 144 && s.value(OPT_RESOLUTION) == "1920 x 1080");
    s.arrow(OPT_REFRESH, -1);
    FRONT_CHECK(s.values.refreshRate == 60 && s.value(OPT_REFRESH) == "60 Hz" && s.value(OPT_RARITY) == "as stars");
    FRONT_CHECK(OptionsScreen::rowY(OPT_RARITY) == 446 && ControlsScreen::rowY(1) == 57 && ControlsScreen::visibleRows() == 16);
}

void treasureItems() {
    using nativeplayer::Item;
    auto treasure = [](uint32_t modifier, int subtype) {
        return nativeplayer::item(30, subtype, 0, 3, modifier, 11, 12, 99);
    };
    auto i32 = [](const Item &it, size_t at) {
        int32_t v;
        std::memcpy(&v, &it[at], 4);
        return v;
    };
    // Not a treasure: itself.
    const Item sword = nativeplayer::item(3, 0, 1, 2, 77, 1, 1);
    FRONT_CHECK(nativeplayer::transformed(sword, 1) == sword);
    // A bag of gold: coins with rarity and modifier only.
    const Item coins = nativeplayer::transformed(treasure(1234, 1), 2);
    FRONT_CHECK(coins[0] == 31 && coins[0x14] == 3 && i32(coins, 8) == 1234 && i32(coins, 0xc) == 0 && i32(coins, 0x18) == 0);
    int kinds[7] = {};
    for (uint32_t m = 0; m < 3000; m++) {
        const uint32_t kind = talkgen::xorshift(m) % 7;
        kinds[kind]++;
        for (int cls = 0; cls <= 5; cls++) {
            const Item in = treasure(m, 0), out = nativeplayer::transformed(in, cls);
            if (kind == 5 || kind == 6) { // rings and amulets for anyone, gold or silver
                FRONT_CHECK(out[0] == (kind == 5 ? 9 : 8) && (out[0x1c] == 11 || out[0x1c] == 12));
                FRONT_CHECK(i32(out, 0xc) == 11 && i32(out, 0x10) == 12 && out[0x14] == 3 && i32(out, 8) == (int32_t)m);
                FRONT_CHECK(out[0x1c] == ((talkgen::xorshift(m + 1) & 1) ? 11 : 12));
                continue;
            }
            if (cls < 1 || cls > 4) { // no class: left as given
                FRONT_CHECK(out == in);
                continue;
            }
            FRONT_CHECK(i32(out, 0x18) == 0 && out[0x9d] == 0 && i32(out, 0xc) == 11);
            if (kind == 0) {
                const int sub = i32(out, 4), t = (int)(talkgen::xorshift(m + 1) % 3);
                FRONT_CHECK(out[0] == 3);
                if (cls == 1)
                    FRONT_CHECK((t == 0 && sub >= 0 && sub <= 2) || (t == 1 && sub >= 15 && sub <= 17) || (t == 2 && sub == 13));
                if (cls == 2)
                    FRONT_CHECK(sub == 6 + t && out[0x1c] == 2);
                if (cls == 3)
                    FRONT_CHECK(sub == 10 + t && (t < 2 ? out[0x1c] == 2 : (out[0x1c] == 11 || out[0x1c] == 12)));
                if (cls == 4)
                    FRONT_CHECK(sub == 3 + t && out[0x1c] == 1);
            } else {
                static const int MATERIALS[4][3] = {{1, 29, 4}, {27, 31, 33}, {26, 32, 35}, {28, 30, 34}};
                FRONT_CHECK(out[0] >= 4 && out[0] <= 7 && i32(out, 4) == 0);
                FRONT_CHECK(out[0x1c] == MATERIALS[cls - 1][talkgen::xorshift(m + 2) % 3]);
                FRONT_CHECK(out[0] == 4 + talkgen::xorshift(m + 1) % 4);
            }
        }
    }
    for (int k : kinds)
        FRONT_CHECK(k > 0);
}

void progress() {
    // Positions: the record stands a block above the feet; the local origin is subtracted.
    const nativeprogress::Frame frame{(int64_t)1000 << 16, (int64_t)-2000 << 16};
    const std::array<int64_t, 3> at = nativeprogress::gamePosition({10.5f, 20.25f, 200}, frame);
    FRONT_CHECK(at[0] == ((int64_t)1000 << 16) + (int64_t)(10.5 * 65536) && at[2] == (int64_t)(73 * 65536));
    const glm::vec3 back = nativeprogress::localFeet(at, frame);
    FRONT_CHECK(near(back.x, 10.5) && near(back.y, 20.25) && near(back.z, 200));
    // Looks from sprite names.
    nativesaves::Looks l;
    FRONT_CHECK(nativeprogress::looksFromSprites("", "", "elf-head-f03", "elf-hair-f02", {1, 2, 3}, l));
    FRONT_CHECK(l.people() == 1 && l.sex() == 1 && l.face() == 2 && l.haircut() == 1 && l.hairColour()[0] == 1);
    FRONT_CHECK(std::string(nativeprogress::raceWord(8)) == "Lizardman" && std::string(nativeprogress::raceWord(13)).empty());
    // A character into the progress gameplay uses and back: its fields survive.
    nativesaves::Character c = newCharacter({5, 6}, at);
    c.classType = 1;
    giveStartingKit(c, 7, {1, 1});
    c.name = "Test";
    auto hiddenRecipe = nativesaves::makeItem(49, 73, 12);
    hiddenRecipe[1] = 0x73; // padding is also retained in the saved raw list
    hiddenRecipe[0x90] = 0xf2;
    c.recipes.push_back(hiddenRecipe);
    c.level = 3, c.experience = 40, c.gold = 12;
    c.usedCells = {{5, 6}}, c.flightPoints = {{100, 200}}, c.finishedMissions = {{1, 1}};
    c.stars[nativesaves::starGroup(-5, 9)].push_back({4, -5, 9, 30});
    const SaveGame s = nativeprogress::toProgress(c, "", frame, nullptr);
    FRONT_CHECK(s.level == 3 && s.xp == 40 && s.gold == 12 && s.hasPosition && near(s.position.x, 10.5));
    FRONT_CHECK(s.recipes && *s.recipes == c.recipes);
    FRONT_CHECK(s.worn.size() == 11 && s.worn[7].category == 3 && s.worn[7].id == 17);
    FRONT_CHECK(s.bag.size() == 1 && s.bag[0].count == 5 && s.found.size() == 2 + 3); // worn gear and the carried weapons
    FRONT_CHECK(s.usedCells.size() == 1 && s.treasures.size() == 1 && s.flightPoints.size() == 1);
    nativesaves::Character again = c;
    std::string notes;
    SaveGame moved = s;
    moved.quests.push_back({42, 1, 5, 2, "Wolf", "zone"});
    moved.recipes->push_back(nativesaves::makeItem(1, 12));
    nativeprogress::fromProgress(moved, nullptr, 99.0f, frame, again, notes);
    FRONT_CHECK(again.position == c.position && again.level == 3 && again.gold == 12 && again.health == 99.0f);
    FRONT_CHECK(again.recipes == *moved.recipes && again.recipes[again.recipes.size() - 2] == hiddenRecipe);
    auto legacyCopy = again;
    SaveGame legacy;
    std::string legacyNotes;
    nativeprogress::fromProgress(legacy, nullptr, 99.0f, frame, legacyCopy, legacyNotes);
    FRONT_CHECK(legacyCopy.recipes == again.recipes); // old notes without recipe data must not clear them
    FRONT_CHECK(again.equipment[7] == c.equipment[7] && again.equipment[2] == c.equipment[2]); // the saved records, attributes and all
    FRONT_CHECK(again.inventory[0].size() == 3 && again.inventory[2].size() == 1 && again.inventory[2][0].count == 5);
    FRONT_CHECK(again.stars == c.stars && again.usedCells == c.usedCells && again.finishedMissions == c.finishedMissions);
    const SaveGame withNotes = nativeprogress::toProgress(again, notes, frame, nullptr);
    FRONT_CHECK(withNotes.quests.size() == 1 && withNotes.quests[0].giver == 42 && withNotes.quests[0].target == "Wolf");
}

// A saved character becomes a record the pose draws with a real size: its appearance names
// models and sizes, and every part matrix the pose makes has non-zero axes. (A record whose
// pose size factor was left at 0 drew the player and townsfolk with no size at all.)
void savedCharacterIsDrawn() {
    if (!worldgen::creatureTablesLoaded()) {
        std::printf("FAIL saved character drawing: the creature tables are not loaded\n");
        failures++;
        return;
    }
    nativesaves::Character c = newCharacter({0, 0}, {0, 0, 0});
    c.classType = 1;
    giveStartingKit(c, 0, {0, 0});
    std::array<uint8_t, 0xb0> looks{};
    FRONT_CHECK(nativeprogress::appearanceOf(c.looks, looks));
    nativeplayer::Who who;
    who.race = nativesaves::raceOf(c.looks.people(), c.looks.sex());
    who.cls = c.classType;
    std::array<nativeplayer::Item, 11> worn{};
    for (size_t k = 0; k < worn.size(); k++)
        std::memcpy(worn[k].data(), c.equipment[k].data(), 0xa0);
    worldgen::CreatureRecord r = nativeplayer::record(who, worn);
    std::memcpy(&r[worldgen::creature::APPEARANCE], looks.data(), looks.size());

    // The appearance: a size and a head and chest model.
    float size[3];
    std::memcpy(size, &r[worldgen::creature::SIZE], sizeof size);
    int16_t head, chest;
    std::memcpy(&head, &r[worldgen::creature::APPEARANCE + 0x18], 2);
    std::memcpy(&chest, &r[worldgen::creature::APPEARANCE + 0x20], 2);
    FRONT_CHECK(size[0] > 0 && size[1] > 0 && size[2] > 0 && head >= 0 && chest >= 0);

    // Posed with every model 10 voxels on a side: parts are drawn, none of them flat.
    nativepose::Model model;
    model.size[0] = model.size[1] = model.size[2] = 10;
    nativepose::Models models;
    models.byId = [&](int id) { return id >= 0 ? &model : nullptr; };
    models.forItem = [&](const uint8_t *) { return &model; };
    nativepose::Call call;
    call.dt = 16;
    std::vector<nativepose::Draw> draws;
    nativepose::pose(r.data(), call, models, &draws);
    FRONT_CHECK(draws.size() >= 6);
    size_t flat = 0;
    for (const nativepose::Draw &d : draws)
        for (int axis = 0; axis < 3; axis++)
            flat += std::abs(d.matrix[axis * 4]) + std::abs(d.matrix[axis * 4 + 1]) + std::abs(d.matrix[axis * 4 + 2]) < 1e-4f;
    FRONT_CHECK(flat == 0);
}

} // namespace

// What applying the options does (frontend-options.md §1.2): the zones drawn and CubeVulkan's
// view distance (400 blocks at the default 50 %), the look speed and the frame limit.
void optionsApplied() {
    gameoptions::Options o;
    FRONT_CHECK(o.zonesDrawn() == 15 && o.viewDistance() == 400.0f && o.lookSpeed() == 1.0f && o.shortestFrameMs() == 0);
    o.renderDistance = 5;
    FRONT_CHECK(o.zonesDrawn() == 3); // 1 zone is below 4: 3
    o.renderDistance = 10;
    FRONT_CHECK(o.zonesDrawn() == 3);
    o.renderDistance = 15;
    FRONT_CHECK(o.zonesDrawn() == 4);
    o.renderDistance = 100;
    FRONT_CHECK(o.zonesDrawn() == 30 && o.viewDistance() == 800.0f);
    o.cameraSpeed = 100, o.minTimeStep = 16;
    FRONT_CHECK(o.lookSpeed() == 2.0f && o.shortestFrameMs() == 16);
    // The gamepad's fixed mappings (§4.1).
    GamepadButtons pad;
    pad.back = true, pad.start = true;
    const auto held = gamepadControls(pad);
    FRONT_CHECK(held[gameoptions::CRAFTING_MENU] && held[gameoptions::INVENTORY] && held[gameoptions::SYSTEM_MENU] && !held[gameoptions::ZOOM_IN]);
    pad = {};
    pad.dpadUp = true;
    FRONT_CHECK(gamepadControls(pad)[gameoptions::ZOOM_IN] && !gamepadControls(pad)[gameoptions::ZOOM_OUT]);
    pad = {};
    pad.a = pad.b = pad.x = pad.y = pad.leftShoulder = pad.rightShoulder = true;
    pad.leftThumb = pad.rightThumb = pad.dpadLeft = pad.dpadRight = true;
    pad.leftTrigger = pad.rightTrigger = 128;
    auto gameplay = gamepadControls(pad);
    FRONT_CHECK(gameplay[gameoptions::ATTACK] && gameplay[gameoptions::USE] && gameplay[gameoptions::JUMP]);
    FRONT_CHECK(gameplay[gameoptions::QUICK_ITEM] && gameplay[gameoptions::ULTIMATE_SKILL] && gameplay[gameoptions::DODGE]);
    FRONT_CHECK(gameplay[gameoptions::LAMP] && gameplay[gameoptions::CALL_PET] && gameplay[gameoptions::RING_MENU] && gameplay[gameoptions::WORLD_MAP]);
    FRONT_CHECK(!gameplay[gameoptions::CLASS_SKILL] && !gameplay[gameoptions::SPECIAL_ATTACK]);
    pad.leftTrigger = pad.rightTrigger = 129;
    FRONT_CHECK(gamepadControls(pad)[gameoptions::CLASS_SKILL] && gamepadControls(pad)[gameoptions::SPECIAL_ATTACK]);
    const auto ui = gamepadControls(pad, true), fullMap = gamepadControls(pad, true, true);
    FRONT_CHECK(ui[gameoptions::ATTACK] && ui[gameoptions::SPECIAL_ATTACK] && !ui[gameoptions::USE] && !ui[gameoptions::LAMP]);
    FRONT_CHECK(fullMap[gameoptions::LAMP] && !fullMap[gameoptions::JUMP] && !fullMap[gameoptions::DODGE]);
    FRONT_CHECK(gamepadStick(6553, 0).x == 0 && gamepadStick(6554, 0).x == 6554.0f / 32768.0f);
    FRONT_CHECK(gamepadStick(5000, 5000).x != 0);
    const auto diagonal = gamepadStick(-32768, 32767);
    FRONT_CHECK(diagonal.x == -1 && diagonal.y == 32767.0f / 32768.0f);
    const auto turn = gamepadCamera({0.5f, -0.5f}, 16);
    frontend::ControlsScreen capture;
    const auto previouslyBound = capture.values[gameoptions::UP];
    capture.waiting = gameoptions::USE;
    capture.bind(previouslyBound);
    FRONT_CHECK(capture.values[gameoptions::USE] == previouslyBound && capture.values[gameoptions::UP] == (gameoptions::Binding{0, 0}));
    capture.bind({0, 1}); // Left mouse is valid after selecting a capture row on release.
    FRONT_CHECK(capture.values[gameoptions::USE] == (gameoptions::Binding{0, 1}) && capture.waiting == gameoptions::USE);
    FRONT_CHECK(std::abs(turn.yaw - 0.8) < 1e-6 && std::abs(turn.pitch + 0.8) < 1e-6);
    FRONT_CHECK(gamepadCamera({1, 1}, 1001).yaw == gamepadCamera({1, 1}, 1000).yaw);
    FRONT_CHECK(gamepadCamera({1, 0}, 16, 100).yaw < gamepadCamera({1, 0}, 16, 25).yaw);
    GamepadFrame frame;
    pad.connected = true;
    frame.update(pad, true, false);
    FRONT_CHECK(frame.dismiss && !frame.held[gameoptions::JUMP] && frame.movement.x == 0);
    frame.update(pad, false, false);
    FRONT_CHECK(!frame.dismiss && !frame.held[gameoptions::JUMP]);
    pad.b = false;
    frame.update(pad, false, false);
    pad.b = true;
    frame.update(pad, false, false);
    FRONT_CHECK(frame.held[gameoptions::JUMP]);
    FRONT_CHECK(GameInput::buttonMatches(gameoptions::defaultControls()[gameoptions::DODGE], 2) &&
                !GameInput::buttonMatches(gameoptions::defaultControls()[gameoptions::UP], 2));
}

// The start spot's tests and walk (frontend-characters.md §4.2) over a made-up world: towns of
// kind 5 in biome 2 on flat ground at 10, the area landmark at the area's corner cell, object 0x35
// only in areas whose x is a multiple of 3.
void startSpot() {
    StartSpotWorld w;
    w.areaCell = [](int32_t qx, int32_t qy) { return nativesaves::Cell{qx * 64, qy * 64}; };
    w.cellObject = [](int32_t cx, int32_t) { return (cx / 64) % 3 == 0 ? START_CELL_OBJECT : 60; };
    w.placeKind = [](int32_t, int32_t) { return START_PLACE_KIND; };
    w.biome = [](int32_t, int32_t) { return 2; };
    w.ground = [](int32_t, int32_t) { return 10.0f; };
    FRONT_CHECK(startCellFits(w, {0, 0}) && !startCellFits(w, {64, 0}));
    auto withGround = w;
    withGround.ground = [](int32_t x, int32_t) { return x < 0 ? -1.0f : 10.0f; };
    FRONT_CHECK(!startCellFits(withGround, {-64 * 3, 0})); // below 0 at block (16 x + 8, 16 y + 40)
    auto steep = w;
    steep.ground = [](int32_t x, int32_t) { return x >= 20 ? 15.0f : 10.0f; };
    FRONT_CHECK(!startCellFits(steep, {0, 0})); // 5 blocks over 16 in x
    auto desert = w;
    desert.biome = [](int32_t, int32_t) { return 9; };
    FRONT_CHECK(!startCellFits(desert, {0, 0}));
    auto wild = w;
    wild.placeKind = [](int32_t, int32_t) { return 0; };
    FRONT_CHECK(!startCellFits(wild, {0, 0}));
    // Angle 0 (rand 0) and the radius 64 + 0: starting in area 1, the walk reaches area 3.
    int draws = 0;
    auto rand = [&] { return draws++, 0; };
    nativesaves::Cell found{};
    FRONT_CHECK(findStartCell(w, rand, {0, 0}, 64, 1000, 64 * 4, found) && found == nativesaves::Cell{192, 0} && draws == 2);
    FRONT_CHECK(!findStartCell(w, rand, {0, 0}, 64, 1000, 64, found)); // (only area 1 in reach)
    FRONT_CHECK(startBlock({2, -1}) == (std::array<int32_t, 2>{40, 16}));
}

// The zones' stored states (native-saves.md §5.3): a chest opened and a block changed in a zone,
// recorded as it leaves memory, written to world.db's format and read back, are there again when
// the zone is made anew.
void zoneStatesRoundTrip() {
    const int32_t previousSeed = worldgen::currentWorld().seed;
    worldgen::setWorldSeed(123);
    // A castle's zone with a reward chest (object type 0x16) in the seed 123 world.
    constexpr int32_t CHEST = 0x16;
    const nativesaves::Cell zone{-41, -221};
    size_t chest = 0;
    bool found = false;
    const auto generated = worldgen::zoneObjects(zone[0], zone[1]);
    for (size_t i = 0; i < generated.size() && !found; i++)
        if (generated[i].type == CHEST && generated[i].reward.category)
            chest = i, found = true;
    FRONT_CHECK(found);
    if (!found) {
        worldgen::setWorldSeed(previousSeed);
        return;
    }
    // The zone as the engine keeps it: blocks in the world, lists in the zone states.
    auto make = [&](ExtractedWorld &ew, zonestates::ZoneStates &states) -> nativesaves::ZoneLists & {
        ew.openGenerated(zone[0] * 64 + 32, zone[1] * 64 + 32);
        float height;
        ew.surface(32, 32, height); // (reads the zone in)
        std::vector<worldgen::ZoneObject> objects;
        ew.gameObjects(zone[0], zone[1], objects);
        std::vector<nativesaves::Record> objectRecords, itemRecords;
        for (const worldgen::ZoneObject &o : objects)
            objectRecords.push_back(zonestates::staticObjectRecord(o));
        for (const worldgen::GroundItem &g : worldgen::zone(zone[0], zone[1])->items)
            itemRecords.push_back(zonestates::groundItemRecord(g, 1));
        return states.load(zone, std::move(itemRecords), std::move(objectRecords));
    };
    auto blocksOf = [](ExtractedWorld &ew) {
        return [&ew](nativesaves::Cell z, const std::vector<nativesaves::BlockChange> &changes) { ew.setBlockChanges(z[0], z[1], changes); };
    };
    const int32_t bx = zone[0] * 64 + 10, by = zone[1] * 64 + 20, bz = 300; // (open sky, well above any ground)
    const std::array<uint8_t, 6> red{200, 0, 0, 0, 1, 0};
    nativesaves::WorldState world;
    {
        ExtractedWorld ew;
        zonestates::ZoneStates states(&world, false, blocksOf(ew));
        nativesaves::ZoneLists &lists = make(ew, states);
        FRONT_CHECK(chest < lists.staticObjects.size() && lists.staticObjects[chest][nativeinteract::OBJECT_STATE] == 1);
        lists.staticObjects[chest][nativeinteract::OBJECT_STATE] = 0; // opened
        states.objectsChanged(zone);
        const ExtractedWorld::GameBlock was = ew.gameBlock(bx, by, bz);
        states.addBlockChange(zonestates::blockChange(bx, by, bz, was, red));
        FRONT_CHECK(ew.gameBlock(bx, by, bz) == red && was != red);
        states.unload(zone, 0);
        FRONT_CHECK(!states.loaded(zone) && world.zones.count(zone) && (world.zones[zone].flags & nativesaves::ZoneState::STATIC_OBJECTS_SAVED));
    }
    nativesaves::WorldState back;
    nativesaves::readWorld(nativesaves::writeWorld(world), back);
    {
        ExtractedWorld ew;
        zonestates::ZoneStates states(&back, false, blocksOf(ew));
        const nativesaves::ZoneLists &lists = make(ew, states);
        FRONT_CHECK(chest < lists.staticObjects.size() && lists.staticObjects[chest][nativeinteract::OBJECT_STATE] == 0);
        FRONT_CHECK(lists.blockChanges.size() == 1 && ew.gameBlock(bx, by, bz) == red);
        // The zone's other objects are as made.
        size_t untouched = 0;
        for (size_t i = 0; i < lists.staticObjects.size(); i++)
            untouched += i != chest && lists.staticObjects[i][nativeinteract::OBJECT_STATE] == 1;
        FRONT_CHECK(untouched + 1 == lists.staticObjects.size());
    }
    // A ground item taken stays taken: its slot is empty.
    {
        nativesaves::WorldState w2;
        zonestates::ZoneStates states(&w2);
        worldgen::GroundItem g;
        g.item.category = 0xb, g.item.id = 1;
        nativesaves::ZoneLists &lists = states.load({5, 5}, {zonestates::groundItemRecord(g, 1)}, {});
        FRONT_CHECK(zonestates::holdsItem(lists.groundItems[0]));
        nativeinteract::emptyGroundItem(lists.groundItems[0]);
        states.groundItemsChanged({5, 5});
        states.unload({5, 5});
        zonestates::ZoneStates again(&w2);
        const nativesaves::ZoneLists &reloaded = again.load({5, 5}, {zonestates::groundItemRecord(g, 1)}, {});
        FRONT_CHECK(reloaded.groundItems.size() == 1 && !zonestates::holdsItem(reloaded.groundItems[0]));
    }
    worldgen::setWorldSeed(previousSeed);
}

// Owner data stays local. Exercise minimum-size expansion and the lower half of the real
// button, which the old hard-coded 30-unit hit rectangle excluded.
void ownerCharacterButtons() {
    Assets assets;
    if (!assets.locate(""))
        return;
    Hud hud;
    std::string why;
    const bool loaded = hud.load(assets.gameDir(), why);
    FRONT_CHECK(loaded);
    if (!loaded)
        return;
    glm::vec2 previous(0);
    for (float scale : {1.0f, 1.5f}) {
        hud.setGuiScale(scale);
        const glm::vec2 frame = hud.templateSize("button2", {200, 30});
        const glm::vec2 card = hud.templateSize("blackwidget", {600, 220});
        const glm::vec2 toggle = hud.templateSize("smallbutton", {20, 20});
        const glm::vec2 remove = hud.templateSize("button", {300, 30});
        std::printf("Character frames: card %.3fx%.3f, toggle %.3fx%.3f, delete %.3fx%.3f\n", card.x, card.y, toggle.x, toggle.y, remove.x,
                    remove.y);
        FRONT_CHECK(frame.x >= 200 && frame.y >= 30);
        if (previous != glm::vec2(0))
            FRONT_CHECK(frame == previous);
        previous = frame;
        Hud::ScreenItem item;
        item.kind = Hud::ScreenItem::Template;
        item.name = "button2";
        item.text = L"Select";
        item.size = {200, 30};
        item.at = characterButtons({300, 100}, card, frame, {20, 20}).select;
        FRONT_CHECK(hud.templateHit(item, item.at + frame * 0.5f, true));
        FRONT_CHECK(hud.templateHit(item, item.at + glm::vec2(frame.x * 0.5f, frame.y - 5), true));
        FRONT_CHECK(!hud.templateHit(item, item.at - glm::vec2(100), true));
        FRONT_CHECK(!hud.templateHit(item, item.at + frame + glm::vec2(100), true));
        std::printf("Character Select owner frame %.0fx%.0f at GUI scale %.1f checked\n", frame.x, frame.y, scale);
    }
}

int frontendTests() {
    failures = 0;
    optionsApplied();
    startSpot();
    zoneStatesRoundTrip();
    titleCamera();
    logo();
    menus();
    ownerCharacterButtons();
    friendRows();
    creation();
    startingKit();
    optionsFiles();
    treasureItems();
    progress();
    savedCharacterIsDrawn();
    return failures;
}

// EngineTests makesave <folder> <recording folder> [near DX DY]: a test characters.db in `folder`
// holding one character made through the writer from a recorded character's snapshot (its name,
// class, specialisation and looks) with the starting kit, for captures of the character screens.
// With `near`, the character was saved standing on the ground DX, DY blocks from the home of the
// install's newest extracted world (for captures of Continue).
int makeTestSave(const char *folder, const char *recording, const glm::vec2 *nearHome) {
    namespace fs = std::filesystem;
    nativesaves::Character c = frontend::newCharacter({0, 0}, {0, 0, 0});
    std::ifstream in(fs::path(recording) / "snapshot.txt");
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind("name=", 0) == 0)
            c.name = line.substr(5);
        else if (line.rfind("class=", 0) == 0)
            c.classType = (uint8_t)std::atoi(line.c_str() + 6);
        else if (line.rfind("specialization=", 0) == 0)
            c.specialization = (uint8_t)std::atoi(line.c_str() + 15);
    }
    if (!nativeprogress::looksFromSnapshot(fs::path(recording) / "snapshot.txt", c.looks))
        return std::printf("no snapshot in %s\n", recording), 1;
    frontend::giveStartingKit(c, 0, {0, 0});
    c.health = nativeprogress::maxHealth(c);
    if (nearHome) {
        Assets assets;
        ExtractedWorld world;
        std::string error;
        if (!assets.locate("") || !world.open(ExtractedWorld::find(assets.gameDir()), error))
            return std::printf("no extracted world: %s\n", error.c_str()), 1;
        glm::vec3 feet = world.home() + glm::vec3(nearHome->x, nearHome->y, 0);
        float ground;
        if (!world.surface(feet.x, feet.y, ground))
            return std::printf("no ground there\n"), 1;
        feet.z = ground;
        c.position = nativeprogress::gamePosition(feet, {world.originDotsX(), world.originDotsY()});
        std::printf("saved standing at local %.2f %.2f %.2f\n", feet.x, feet.y, feet.z);
    }
    std::error_code ec;
    fs::create_directories(folder, ec);
    nativesaves::CharacterStore store;
    if (!store.open(fs::path(folder) / "characters.db"))
        return 1;
    const int index = store.create(c);
    std::printf("character %d: %s, class %d\n", index, c.name.c_str(), c.classType);
    return index >= 0 ? 0 : 1;
}
