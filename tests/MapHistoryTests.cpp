#include "MapHistory.h"
#include "Assets.h"
#include "Hud.h"
#include "Frontend.h"
#include "ChatLine.h"
#include "worldgen/Regions.h"
#include "worldgen/Places.h"
#include <cstdio>
#include <cstring>

namespace {
template <class T> std::vector<uint8_t> legacyBytes(const T &value) {
    std::vector<uint8_t> bytes(sizeof value);
    std::memcpy(bytes.data(), &value, sizeof value);
    return bytes;
}
template <class T> plx::Chunk legacyChannel(const char *name, T initial, T final, int time) {
    const std::string sequence = "location";
    auto text = legacyBytes(int32_t(sequence.size()));
    text.insert(text.end(), sequence.begin(), sequence.end());
    return {name,
            {},
            {{"Attribute.frame", legacyBytes(initial), {}},
             {"Attribute.frame", legacyBytes(initial), {}},
             {"Attribute.frame", legacyBytes(final), {}},
             {"Attribute.sequence",
              {},
              {{"Attribute.sequence.name", text, {}},
               {"Attribute.sequence.key",
                {},
                {{"Attribute.sequence.key.frame", legacyBytes(int32_t(2)), {}},
                 {"Attribute.sequence.key.time", legacyBytes(int32_t(time)), {}}}}}}}};
}
} // namespace

int mapHistoryTests() {
    using namespace maphistory;
    int failures = 0;
    auto check = [&](bool pass, const char *message) {
        if (!pass) {
            std::printf("FAIL map history: %s\n", message);
            ++failures;
        }
    };
    const Region a{3, -7}, b{-4, 9}, c{8, 2};
    History history({a, b, c});
    check(history.snapshot() == std::vector<Region>{a, b, c} && !history.selected(), "saved order loads without selecting a node");
    history.show(true);
    check(history.selected() == a && history.view().older && !history.view().newer, "own-hidden to shown selects the first node");
    check(history.older() == b && history.older() == c && history.older() == c,
          "older navigation stops at the final node and returns its point");
    check(!history.view().older && history.view().newer, "boundary refresh hides only the older arrow");
    check(history.newer() == b && history.newer() == a && history.newer() == a, "newer navigation has no wrap");
    history.older();
    history.show(true);
    check(history.selected() == b, "reached shown updates retain selection without reopening");
    history.show(false);
    history.show(true);
    check(history.selected() == a, "closing and reopening reinitializes selection");
    check(!history.observe(a, 0) && history.snapshot() == std::vector<Region>{a, b, c},
          "an unchanged front region does not allocate or reorder");
    check(!history.observe(c, 0x9c) && !history.observe(c, 0x9d), "excluded ability modes preserve history");
    history.older();
    check(history.observe(c, 0) && history.snapshot() == std::vector<Region>{c, a, b} && history.selected() == b,
          "a revisit prepends a new node while unmatched selection identity survives");
    const auto caption = history.view().caption;
    check(history.observe(b, 0) && !history.selected() && !history.older() && history.view().caption == caption,
          "revisiting the selected node invalidates its borrow without automatic reselection or fabricated destination");
    history.show(false);
    history.show(true);
    check(history.selected() == b, "a later own-visible transition establishes a new valid selection");

    History duplicates({a, b, a, c});
    check(!duplicates.observe(a, 0) && duplicates.snapshot() == std::vector<Region>{a, b, a, c},
          "a matching front leaves saved duplicates alone");
    check(duplicates.observe(b, 0) && duplicates.snapshot() == std::vector<Region>{b, a, a, c},
          "revisits remove only the first older matching node");
    History empty;
    empty.show(true);
    check(!empty.selected() && !empty.older() && !empty.newer() && !empty.view().caption,
          "empty history provides no region-zero target or invented caption");

    check(hovered(true, 299.99f, 910.01f, 1000, 1) && !hovered(true, 300, 950, 1000, 1) && !hovered(true, 100, 910, 1000, 1),
          "label pointer rectangle uses strict native boundaries");
    check(hovered(true, -100, 2000, 1000, 1) && hovered(true, 449, 866, 1000, 1.5f) && !hovered(false, 0, 1000, 1000, 1),
          "label hover has no extra lower-X, upper-Y or unscaled-height gates");
    check(worldgen::regionAtFixed(-65535, 65535, false) == worldgen::regionAt(0, 0),
          "fractional physical fixed coordinates truncate toward zero before querying");
    const auto point = worldgen::cellPoint(1, 0, worldgen::bigCell(1, 0));
    check(worldgen::regionAtFixed(int64_t(1024) * 65536, 0, true) == worldgen::regionAt(point.x, point.y),
          "flagged blocks use their original landmark cells before the helper's internal even rounding");
    Assets assets;
    if (assets.locate("")) {
        Hud hud;
        std::string error;
        check(hud.load(assets.gameDir(), error), "owner map UI loads");
        Renderer renderer;
        renderer.initHeadless(RenderSettings{});
        for (float scale : {1.f, 1.5f}) {
            hud.setGuiScale(scale);
            hud.layout(renderer, {1920, 1080});
            History owner({a, b, c});
            owner.show(true);
            hud.bindMapHistory(owner.view(), "Test Land", true);
            const auto *root = hud.mapHistoryRoot();
            const auto *left = root ? const_cast<plasma::SceneNode *>(root)->find("leftbutton") : nullptr;
            const auto *right = root ? const_cast<plasma::SceneNode *>(root)->find("rightbutton") : nullptr;
            check(root && left && right && root->transform.translation == glm::vec2(150, 1080.f / scale - 50) &&
                      left->transform.translation == glm::vec2(-100, 2) - left->transform.pivot &&
                      right->transform.translation == glm::vec2(100, 2) - right->transform.pivot,
                  "owner arrows retain pivot transforms under the physical-height label anchor");
            check(left && right && left->visible && !right->visible && left->doc && right->doc && left->widget && right->widget,
                  "initial native selection exposes only the older owner widget");
            owner.older();
            hud.bindMapHistory(owner.view(), "Other Land", true);
            check(left && right && left->visible && right->visible, "callback refresh exposes both arrows at an interior entry");
            glm::vec2 olderPoint{};
            bool found = false;
            for (int dy = -40; dy <= 40 && !found; dy += 2)
                for (int dx = -40; dx <= 40 && !found; dx += 2) {
                    const glm::vec2 point((50 + dx) * scale, 1080 + (dy - 48) * scale);
                    if (hud.mapHistoryAction(point) == -1) {
                        olderPoint = point;
                        found = true;
                    }
                }
            check(found && hud.mapHistoryArrowHit(true, olderPoint), "owner arrow geometry produces the native mouse-down action");
            hud.bindMapHistory(owner.view(), "Other Land", false);
            check(found && hud.mapHistoryArrowHit(true, olderPoint) && !hud.mapHistoryAction(olderPoint) && owner.selected() == b,
                  "hidden ancestors suppress dispatch while standalone arrow hits and selection remain live");

            // Complete native press admission: event2 changes the actual arrow
            // visibility before the Game queries A then B at their retained point.
            hud.bindMapHistory(owner.view(),"Other Land",true);
            ChatLine chat;
            MenuInput messages;
            bool fullMap=true,captionSawRefresh=false;
            hud.bindMapHistoryActions([&](bool older) {
                if (older) owner.older();else owner.newer();
                hud.refreshMapHistory(owner.view(),[&] {
                    captionSawRefresh=left && right && !left->visible && right->visible;
                    return std::string("Refreshed");
                });
            });
            messages.gamePress=[&](int) {
                if (!hud.mapPressContinues(fullMap)) return false;
                chat.stopEditing();return true;
            };
            chat.keyDown(0x0D,false);chat.type("A");
            hud.moveInterfacePointer(olderPoint);
            messages.interfaceMessages.emplace_back(MenuInput::InterfaceMessage::LeftDown);
            messages.routeInterfaceMessages(hud);
            check(found && owner.selected()==c && captionSawRefresh && left && !left->visible && right && right->visible,
                  "live event2 reaches last history cell and publishes B/A before its caption provider");
            check(!chat.typing() && messages.gamePresses==std::vector<int>{0},
                  "post-callback hidden A and missed B reach the ordinary chat-clear continuation");
            hud.interfaceLeftUp();
            owner=History({a,b,c});owner.show(true);
            hud.bindMapHistory(owner.view(),"First Land",true);
            chat.keyDown(0x0D,false);
            hud.moveInterfacePointer(olderPoint);
            messages.interfaceMessages.emplace_back(MenuInput::InterfaceMessage::MiddleDown);
            messages.routeInterfaceMessages(hud);
            check(chat.typing() && messages.gamePresses.empty() && owner.selected()==a,
                  "middle press also takes A's early return without an event2 history step or chat clear");
            hud.interfaceSecondaryButton(2,false);
            if (root) const_cast<plasma::SceneNode *>(root)->visible=false;
            if (left && left->widget) const_cast<plasma::SceneNode *>(left)->widget->enabled=false;
            messages.interfaceMessages.emplace_back(MenuInput::InterfaceMessage::RightDown);
            messages.routeInterfaceMessages(hud);
            check(chat.typing() && messages.gamePresses.empty(),
                  "direct A query ignores hidden parent and disabled control after independent GUI work");
            hud.interfaceSecondaryButton(1,false);
            fullMap=false;
            messages.interfaceMessages.emplace_back(MenuInput::InterfaceMessage::MiddleDown);
            messages.routeInterfaceMessages(hud);
            check(!chat.typing() && messages.gamePresses==std::vector<int>{2},
                  "zero map byte bypasses both direct-node queries and reaches normal tail");
            hud.interfaceSecondaryButton(2,false);
            if (left && left->widget) const_cast<plasma::SceneNode *>(left)->widget->enabled=true;
            hud.bindMapHistoryActions({});

            // Exercise actual HUD emission, not a duplicate color-multiplication helper.
            hud.bindMapHistory(owner.view(), "Other Land", true);
            if (!root || !left || !left->doc)
                continue;
            auto *mutableRoot = const_cast<plasma::SceneNode *>(root);
            auto glyph = std::make_unique<plasma::SceneNode>();
            glyph->doc = left->doc;
            glyph->parent = mutableRoot;
            for (size_t index = 0; index < glyph->doc->shapes.size(); ++index) {
                plasma::TextStyle style;
                if (plasma::textStyle(glyph->doc->shapes[index], glyph->doc->loaderFlags, style)) {
                    glyph->shape = int(index);
                    break;
                }
            }
            check(glyph->shape >= 0, "owner document has a TextShape for tint integration");
            glyph->text = L"T";
            glyph->textColor = glm::vec4(1);
            auto *textNode = glyph.get();
            mutableRoot->children.push_back(std::move(glyph));
            hud.requireText(u"T");
            hud.prepareFrame(renderer, {1920, 1080});
            const auto snapshot = [&] {
                Hud::State state;
                std::vector<OverlayVertex> vertices;
                hud.draw(state, vertices);
                return vertices;
            };
            const auto same = [](const auto &a, const auto &b) {
                return a.size() == b.size() && (a.empty() || std::memcmp(a.data(), b.data(), a.size() * sizeof(OverlayVertex)) == 0);
            };
            const auto baseline = snapshot();
            mutableRoot->fill = glm::vec4(0);
            const auto parentTint = snapshot();
            mutableRoot->fill = glm::vec4(1);
            textNode->fill = glm::vec4(0);
            const auto glyphTint = snapshot();
            textNode->visible = false;
            const auto withoutGlyph = snapshot();
            check(!baseline.empty() && !same(baseline, withoutGlyph), "synthetic glyph is actually emitted through the owner HUD");
            check(same(baseline, parentTint), "shape-less parent display tint does not alter descendant shape or glyph colors");
            check(same(baseline, glyphTint), "TextShape glyphs ignore their own display fill color");
            textNode->visible = true;
            std::vector<int> hooks;
            auto previousWidget = std::move(mutableRoot->widget);
            auto previousHook = std::move(mutableRoot->onDraw);
            mutableRoot->widget.emplace();
            mutableRoot->widget->flags = plasma::WIDGET_DRAW_FIRST;
            mutableRoot->onDraw = [&](plasma::SceneNode &) { hooks.push_back(1); };
            textNode->onDraw = [&](plasma::SceneNode &) { hooks.push_back(2); };
            mutableRoot->clips = true;
            const auto clipped = snapshot();
            const auto layers = plasma::layerDepth(hud.clipDraws(), {clipped.size(), hud.itemDraws().size()});
            check(layers && *layers >= 1 && hooks == std::vector<int>{2, 1},
                  "clip children precede the own DRAW_FIRST hook and emit balanced vertex/callback cuts");
            const auto displayFlags = mutableRoot->displayFlags;
            const float blurRadius = mutableRoot->blurRadius;
            mutableRoot->displayFlags |= plasma::DISPLAY_BLUR;
            mutableRoot->blurRadius = 4;
            size_t ownHookCut = 0;
            mutableRoot->onDraw = [&](plasma::SceneNode &) {
                hooks.push_back(1);
                ownHookCut = hud.clipDraws().size();
            };
            for (bool clip : {false, true}) {
                mutableRoot->clips = clip;
                hooks.clear();
                const auto blurred = snapshot();
                const auto depth = plasma::layerDepth(hud.clipDraws(), {blurred.size(), hud.itemDraws().size()});
                check(depth && *depth >= (clip ? 2 : 1) && hooks == std::vector<int>{2, 1},
                      "blur children run offscreen before the own DRAW_FIRST hook, including nested clips");
                const auto &commands = hud.clipDraws();
                using Kind = plasma::LayerCommand::Kind;
                check(ownHookCut >= 3 && commands[ownHookCut - 3].kind == Kind::Pop && commands[ownHookCut - 2].kind == Kind::Push &&
                          commands[ownHookCut - 1].kind == Kind::Pop,
                      "blur restores the parent and clears the retained final mask before its own hook");
            }
            mutableRoot->displayFlags = displayFlags;
            mutableRoot->blurRadius = blurRadius;
            mutableRoot->clips = false;
            hooks.clear();
            snapshot();
            check(hooks == std::vector<int>{1, 2}, "ordinary DRAW_FIRST placement still precedes children");
            mutableRoot->widget = std::move(previousWidget);
            mutableRoot->onDraw = std::move(previousHook);
            const int ownerGlyph = textNode->shape;
            mutableRoot->children.pop_back();

            // The location banner still uses the document-placement adapter. Its
            // owner-authored mask must reach the renderer, not become a mean tint.
            std::vector<OverlayVertex> earlier;
            size_t masks = 0;
            bool animated = false;
            for (float time : {0.f, 600.f, 3000.f}) {
                Hud::State state;
                state.locationName = "T";
                state.locationMs = time;
                std::vector<OverlayVertex> vertices;
                hud.draw(state, vertices);
                const auto depth = plasma::layerDepth(hud.clipDraws(), {vertices.size(), hud.itemDraws().size()});
                check(depth.has_value(), "legacy location emission keeps balanced mask and callback cuts");
                for (const auto &command : hud.clipDraws())
                    masks += command.kind == plasma::LayerCommand::Kind::Mask && command.mode != 0;
                if (!earlier.empty())
                    animated = animated || !same(earlier, vertices);
                earlier = std::move(vertices);
            }
            check(masks > 0, "owner location banner emits real mask composition commands");
            check(animated, "owner location shape sequence changes emitted geometry or colors");

            // Isolate synthetic channels in the real document adapter, retaining
            // the owner's glyph/font binding. Restore the document before re-layout.
            auto &document = *const_cast<plx::Document *>(left->doc);
            const int location = document.find("location");
            check(location >= 0, "owner document provides the placed location root");
            if (location >= 0) {
                const auto saved = document.nodes[(size_t)location];
                const size_t nodeCount = document.nodes.size(), transformCount = document.transformations.size(),
                             displayCount = document.displays.size();
                document.transformations.push_back(
                    {"Transformation", {legacyChannel("Transformation.translation", glm::vec2(0), glm::vec2(24, 0), 100)}});
                document.displays.push_back({"Display",
                                             {legacyChannel("Display.visibility", int32_t(1), int32_t(0), 200),
                                              legacyChannel("Display.clipping", int32_t(0), int32_t(1), 100)}});
                plx::Node probe;
                probe.name = "name";
                probe.shape = ownerGlyph;
                probe.transformation = int(transformCount);
                probe.display = int(displayCount);
                document.nodes.push_back(probe);
                document.nodes[(size_t)location] = {};
                document.nodes[(size_t)location].name = "location";
                document.nodes[(size_t)location].children = {int(nodeCount)};
                const auto frame = [&](float time) {
                    Hud::State state;
                    state.locationName = "T";
                    state.locationMs = time;
                    std::vector<OverlayVertex> vertices;
                    hud.draw(state, vertices);
                    return vertices;
                };
                const auto initial = frame(0), moved = frame(100);
                bool translated = initial.size() == moved.size();
                size_t changed = 0;
                for (size_t i = 0; translated && i < initial.size(); ++i) {
                    if (std::memcmp(&initial[i], &moved[i], sizeof(OverlayVertex)) == 0)
                        continue;
                    auto aligned = moved[i];
                    aligned.x -= 24 * scale;
                    translated = std::abs(aligned.x - initial[i].x) < .001f && aligned.y == initial[i].y && aligned.u == initial[i].u &&
                                 aligned.v == initial[i].v && aligned.r == initial[i].r && aligned.g == initial[i].g &&
                                 aligned.b == initial[i].b && aligned.a == initial[i].a;
                    ++changed;
                }
                check(translated && changed > 0, "legacy translation sequence moves only the emitted probe glyph");
                const auto commands = hud.clipDraws().size();
                check(commands >= 4, "legacy clipping sequence changes the actual layer command stream");
                const auto hidden = frame(200);
                document.nodes[nodeCount].flags |= plx::NODE_DISABLED;
                const auto disabled = frame(0);
                check(hidden.size() < initial.size() && same(hidden, disabled),
                      "animated visibility and disabled-node admission both remove the legacy probe");
                document.nodes[(size_t)location] = saved;
                document.nodes.resize(nodeCount);
                document.transformations.resize(transformCount);
                document.displays.resize(displayCount);
            }
        }
        renderer.shutdown();
    }
    std::printf("Map history: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
