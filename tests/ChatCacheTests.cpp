#include "ChatLine.h"
#include "PlasmaDraw.h"
#include "PlasmaScene.h"
#include <iostream>
#include <limits>

int chatCacheTests() {
    int failures = 0;
    const auto check = [&](bool pass, const char *message) {
        if (!pass) {
            std::cerr << "FAIL chat cache: " << message << '\n';
            ++failures;
        }
    };
    const auto rectangle = plasma::cacheRectangle({-1.6f, 2.49f}, {10.4f, 12.5f});
    check(rectangle && *rectangle == glm::ivec4(-1, 2, 10, 13), "cache rounding adds float0.5 then truncates signed values");
    check(!plasma::cacheRectangle({}, {.1f, 5}), "zero physical allocation is rejected");
    check(!plasma::cacheRectangle({std::numeric_limits<float>::infinity(), 0}, {5, 5}), "unrepresentable cache origin is rejected");
    auto surface = std::make_shared<plasma::RetainedSurface>();
    std::vector<plasma::LayerCommand> commands{{plasma::LayerCommand::Kind::CacheBegin, {}, 0, surface, {0, 0, 40, 30}},
                                               {plasma::LayerCommand::Kind::CacheEnd, {}, 0, surface},
                                               {plasma::LayerCommand::Kind::CacheCopy, {}, 0, surface, {4, 5, 40, 30}}};
    check(plasma::layerDepth(commands, {}).has_value(), "root cached Widget stream is balanced");
    auto invalid = commands;
    invalid[1].surface = std::make_shared<plasma::RetainedSurface>();
    check(!plasma::layerDepth(invalid, {}), "cached target end must match its owner");
    invalid = commands;
    invalid.erase(invalid.begin() + 1);
    check(!plasma::layerDepth(invalid, {}), "copy cannot sample the active attachment");
    plx::Document document;
    plasma::Scene scene(document);
    auto *node = scene.create(&scene.root(), "placement");
    scene.setPosition(*node, {3, 4}, {});
    check(node->placementChanged, "differing point placement marks its recompute indication");
    node->placementChanged = false;
    scene.setPosition(*node, {3, 4}, {});
    check(!node->placementChanged, "equal point placement does not mark the indication");
    node->placementChanged = true;
    scene.setPosition(*node, {3, 4}, {});
    check(node->placementChanged, "equal placement preserves an already pending indication");
    ChatLine chat;
    const auto initial = chat.drawingRevision();
    chat.enter();
    chat.type("a");
    chat.stopEditing();
    check(chat.drawingRevision() == initial, "opening/insertion/direct editor byte clearing are not independent dirty producers");
    chat.key(ChatLine::Key::End);
    check(chat.drawingRevision() != initial, "reached no-op short key still invalidates");
    const auto beforeAppend = chat.drawingRevision();
    chat.append(u"", {255, 255, 255}, 400, [](const auto &) { return 0.f; });
    check(chat.drawingRevision() != beforeAppend, "successful empty append invalidates");
    const auto beforeTrim = chat.drawingRevision();
    chat.trim(0);
    check(chat.drawingRevision() == beforeTrim, "draw-owned row pruning does not invalidate again");
    ChatLine empty;
    empty.enter();
    const auto beforeClose = empty.drawingRevision();
    empty.enter();
    check(empty.drawingRevision() != beforeClose, "Enter closing an empty editor invalidates");
    std::cout << "Chat cache: " << failures << " failures\n";
    return failures;
}
