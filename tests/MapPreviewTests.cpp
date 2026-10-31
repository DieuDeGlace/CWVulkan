#include "MapPreview.h"
#include "Assets.h"
#include "Hud.h"
#include <cstdio>
#include <limits>
#include <algorithm>
#include <initializer_list>
#include <utility>
#include <cmath>

int mapPreviewTests() {
    int failures = 0;
    auto check = [&](bool pass, const char *what) {
        if (!pass) {
            std::printf("FAIL map preview: %s\n", what);
            ++failures;
        }
    };
    using namespace mappreview;
    check(pointerDistance({655359999, 0, 0}, {0, 0, 0}, true) == "5 km",
          "pointer distance uses float32 fixed conversion/norm before double metre conversion");
    check(pointerDistance({655359999, 0, 0}, {0, 0, -1}, true).empty() && pointerDistance({655359999, 0, 0}, {0, 0, 0}, false).empty(),
          "pointer text admits only own-shown Overlay and nonnegative fixed cursor Z");
    for (const auto &[metres, text] : std::initializer_list<std::pair<int, const char *>>{{9, "9 m"},
                                                                                          {19, "10 m"},
                                                                                          {499, "490 m"},
                                                                                          {599, "500 m"},
                                                                                          {999, "900 m"},
                                                                                          {1099, "1 km"},
                                                                                          {1199, "1.1 km"},
                                                                                          {9999, "9.9 km"},
                                                                                          {10999, "10 km"}})
        check(distanceText(metres) == text, "distance unit boundaries");
    FlightContext context;
    check(flightPrice({100000, 0}, context) == 1000, "player distance is not halved for price");
    std::set<Cell> known{{60000, 0}};
    context.known = &known;
    check(flightPrice({100000, 0}, context) == 200, "known-point distance is halved before taking the minimum");
    known = {{std::numeric_limits<int32_t>::max(), 0}};
    check(flightPrice({std::numeric_limits<int32_t>::min(), 0}, context) == 100, "known-point difference wraps signed32");
    context.known = nullptr;
    check(flightPrice({1677721700, 0}, context) == 16777216, "price retains the integer-to-float32 round trip");
    context.money = 99;
    auto rows = flight({5000, 0}, false, context);
    check(rows.size() == 3 && rows[0].text == "Flight point" && rows[1].text == "Distance: 2.5 km" && rows[2].text == "Cost: 100 gold" &&
              rows[2].color == glm::vec4(1, 0, 0, 1),
          "unknown unaffordable flight rows");
    float height = 25;
    for (const auto &row : rows) {
        check(row.size == 12 && row.stroke == 3 && row.fromEnd, "flight rows retain native font and returned-pen advancement");
        height += row.advance;
    }
    check(height == 115, "unwrapped price geometry has no item minimum");
    context.money = 100;
    check(flight({5000, 0}, false, context)[2].text == "Purchase for: 100 gold", "money equal to price is enough");
    context.physical[0] = -1;
    rows = flight({5000, 0}, true, context);
    check(rows[2].text == "Flight time: 12 seconds" && rows[2].advance == 60, "live player-target flag selects time and extra height");
    known = {{5000, 0}};
    context.known = &known;
    check(flight({5000, 0}, false, context)[2].text == "Flight time: 12 seconds", "known target selects time regardless of live flag");
    context.ability = 0x9c;
    context.destination = {5000, 0};
    context.start[0] = -1;
    context.abilityMs = 15000;
    check(flight({5000, 0}, false, context)[2].text == "Flight time: -2 seconds",
          "remaining flight time is not clamped and division truncates");
    context.destination = {5001, 0};
    check(flight({5000, 0}, false, context)[2].text == "Flight time: 12 seconds", "a different flight destination uses current position");
    check(lorePlace({-1, 64}) == (Cell{-1, 1}) && lorePlace({0x10000000, 0x0fffffff}) == (Cell{0, -1}),
          "lore realm place floors wrapped block coordinates");
    check(lorePercent(11) == 110 && lorePercent(0x100000003ull) == 30 && lorePercent(0x10000000ull) == -1610612736,
          "lore percentage retains low32 wrap without a100 percent cap");
    Lore content;
    check(layoutLore(content, 270, {}).end == glm::vec2(14, 73), "no pages retain the initial body pen");
    content.pages = {{}};
    check(layoutLore(content, 270, {}).end == glm::vec2(14, 94), "an empty page still advances21");
    auto width = [](const std::wstring &word) { return float(word.size() * 10) + .9f; };
    content.pages = {{{L"A"}, {L"-"}, {L"B"}, {L"/"}, {L"C"}, {L"("}, {L"D"}, {L"."}}};
    auto laidOut = layoutLore(content, 270, width);
    const float positions[] = {14, 27, 37, 50, 63, 85, 98, 111};
    check(laidOut.words.size() == 8 && laidOut.end == glm::vec2(14, 94), "all static words share a single fitting page");
    for (size_t i = 0; i < laidOut.words.size(); ++i)
        check(laidOut.words[i].at == glm::vec2(positions[i], 73), "native punctuation, hyphen and slash/parenthesis offsets");
    content.pages = {{{L"A"}, {L"'"}, {L"A"}, {L""}}};
    laidOut = layoutLore(content, 270, width);
    check(laidOut.words.size() == 4 && laidOut.words[1].at.x == 36 && laidOut.words[2].at.x == 58 && laidOut.words[3].at.x == 80,
          "apostrophes have no special width/glue and empty words are retained");
    content.pages = {{{L"AA"}, {L"BB", {.1f, .2f, .3f, .4f}, 255}}, {}};
    laidOut = layoutLore(content, 20, width);
    check(laidOut.words[0].at == glm::vec2(14, 73) && laidOut.words[1].at == glm::vec2(14, 87) &&
              laidOut.words[1].word.colour == glm::vec4(.1f, .2f, .3f, .4f) && laidOut.end == glm::vec2(14, 129),
          "strict wrap, stored color and every page's final spacing");
    content.pages = {{{L"AAA"}}};
    check(layoutLore(content, 20, width).words[0].at == glm::vec2(14, 87), "overwide first words also wrap");
    content.pages = {{{L"."}}};
    check(layoutLore(content, 0, width).words[0].at == glm::vec2(14, 73), "narrow punctuation never wraps");

    Assets assets;
    if (assets.locate("")) {
        Hud hud;
        std::string error;
        check(hud.load(assets.gameDir(), error), "owner UI load");
        Renderer renderer;
        renderer.initHeadless(RenderSettings{});
        hud.setGuiScale(1);
        hud.layout(renderer, {1920, 1080});
        auto original = nativesaves::makeItem(3, 13, 1);
        Hud::State ui;
        ui.mapInteractive = true;
        ui.pointerAt = {100, 100};
        mapmarks::Marker marker;
        marker.at = ui.pointerAt;
        marker.block = {300, 400, 0};
        marker.flightTarget = marker.reportsHover = true;
        marker.originalItem = &original;
        std::vector<mapmarks::Marker> currentMarkers{marker};
        int collectionDraws = 0, overlayDraws = 0, informationDraws = 0;
        ui.collectMapMarkers = [&] {
            ++collectionDraws;
            return currentMarkers;
        };
        ui.mapOverlayDraw = [&](const auto &) { ++overlayDraws; };
        ui.informationDraw = [&] {
            ++informationDraws;
            check(hud.mapPreviewOutputs().item[0] == original[0], "information sees the preceding map draw's live item write");
        };
        const auto *borrowed = &hud.mapPreviewOutputs().item;
        std::vector<OverlayVertex> overlay;
        hud.draw(ui, overlay);
        const auto copied = hud.mapPreviewOutputs().flight;
        check(copied == (Cell{300, 400}) && hud.mapPreviewOutputs().flightToPlayer && (*borrowed)[0] == 3,
              "admitted owner map draw publishes its current marker outputs");
        itempreview::Sources sources;
        sources.map = borrowed;
        const auto selected = itempreview::select(sources);
        original[0] = 28;
        currentMarkers[0].block = {500, 600, 0};
        hud.draw(ui, overlay);
        check(selected.item == borrowed && (*selected.item)[0] == 28 && copied == (Cell{300, 400}) &&
                  hud.mapPreviewOutputs().flight == (Cell{500, 600}),
              "selected item stays live while earlier scalar coordinates stay copied");
        ui.interfaceVisible = false;
        original[0] = 24;
        hud.draw(ui, overlay);
        check(collectionDraws == 2 && overlayDraws == 2 && informationDraws == 2 && (*borrowed)[0] == 28,
              "hidden ancestor skips collection, map and information callbacks");
        ui.interfaceVisible = true;
        ui.mapInteractive = false;
        ui.informationDraw = [&] { check((*borrowed)[0] == 0, "reached minimap reset precedes information drawing"); };
        hud.draw(ui, overlay);
        check(collectionDraws == 3 && overlayDraws == 3 && hud.mapPreviewOutputs().flight[0] == mapmarks::PreviewOutputs::NONE &&
                  hud.mapPreviewOutputs().flightToPlayer,
              "reached minimap clears the pair without clearing the live flag");
        ui.mapMarkers.clear();
        ui.collectMapMarkers = {};
        ui.mapOverlayDraw = {};
        ui.informationDraw = {};
        hud.draw(ui, overlay);
        const size_t beforePreview = overlay.size();
        Hud::Inventory information;
        information.infoAnchor = itempreview::Anchor::Cursor;
        information.pointer = {500, 200};
        ui.information = &information;
        ui.mapInformation = true;
        float background = 0;
        bool timeRow = false, rejected = false;
        ui.mapInformationDraw = [&] {
            float top = std::numeric_limits<float>::infinity(), bottom = -top;
            for (size_t i = beforePreview; i < overlay.size(); ++i) {
                top = std::min(top, overlay[i].y);
                bottom = std::max(bottom, overlay[i].y);
            }
            background = bottom - top;
            return rejected ? std::vector<iteminformation::Line>{} : flight({5000, 0}, timeRow, FlightContext{});
        };
        hud.draw(ui, overlay);
        const float initialBackground = background;
        hud.draw(ui, overlay);
        const float priceBackground = background;
        check(std::abs(initialBackground - priceBackground - 185) < .01f, "flight callback resizes 300 to115 after the current background");
        timeRow = true;
        hud.draw(ui, overlay);
        check(std::abs(background - priceBackground) < .01f, "flight time still draws the preceding price background");
        hud.draw(ui, overlay);
        check(std::abs(background - priceBackground - 30) < .01f, "time retains145 instead of the item minimum200");
        const float timeBackground = background;
        int itemDraws = 0;
        ui.informationDraw = [&] { ++itemDraws; };
        rejected = true;
        hud.draw(ui, overlay);
        hud.draw(ui, overlay);
        check(std::abs(background - timeBackground) < .01f && itemDraws == 0,
              "rejected map content retains background size and suppresses item drawing");
        ui.mapInformationDraw = {};
        Lore shownLore;
        shownLore.realm = "Synthetic realm";
        shownLore.percent = 110;
        shownLore.pages = {{{L"M", {.2f, .8f, .4f, 1}, 255}}};
        bool validLore = true;
        ui.mapLoreDraw = [&]() -> std::optional<Lore> {
            float top = std::numeric_limits<float>::infinity(), bottom = -top;
            for (size_t i = beforePreview; i < overlay.size(); ++i) {
                top = std::min(top, overlay[i].y);
                bottom = std::max(bottom, overlay[i].y);
            }
            background = bottom - top;
            return validLore ? std::optional(shownLore) : std::nullopt;
        };
        auto bodyTop = [&] {
            float top = std::numeric_limits<float>::infinity();
            for (const auto &vertex : overlay)
                if (vertex.r == 51 && vertex.g == 204 && vertex.b == 102)
                    top = std::min(top, vertex.y);
            return top;
        };
        hud.draw(ui, overlay);
        check(std::abs(background - timeBackground) < .01f, "lore starts with the previous flight background");
        const auto shortHeaderBody = bodyTop();
        hud.draw(ui, overlay);
        check(std::abs(timeBackground - background - 21) < .01f && std::isfinite(shortHeaderBody),
              "one-page lore retains124 height and draws its stored word color");
        shownLore.realm = std::string(120, 'M');
        hud.draw(ui, overlay);
        check(bodyTop() == shortHeaderBody, "a wrapped realm header never moves the fixed lore body origin");
        shownLore.pages.clear();
        hud.draw(ui, overlay);
        hud.draw(ui, overlay);
        check(std::abs(timeBackground - background - 42) < .01f, "zero lore pages retain103 height");
        validLore = false;
        hud.draw(ui, overlay);
        hud.draw(ui, overlay);
        check(std::abs(timeBackground - background - 42) < .01f && itemDraws == 0,
              "actual lore rejection preserves stored height without an item fallback");
        renderer.shutdown();
    } else
        std::puts("SKIP map preview owner tree: owner installation unavailable");
    std::printf("Map preview: %s\n", failures ? "FAILED" : "PASS");
    return failures;
}
