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
    const auto flat=[](const ChatLine &line) {
        std::wstring result; for (const auto &row:line.rows()) for (const auto &word:row) result+=word.text;
        return result;
    };
    const auto metric=[](const std::wstring &word) {return float(word.size()*5);};
    ChatLine tail;
    tail.receive(int64_t(-1),u"A"); tail.receive(int64_t(-1),u"B");
    tail.consumeIncoming(400,metric);
    check(tail.pending()==1 && flat(tail)==L"B","one update consumes the newest pending tail record");
    tail.consumeIncoming(400,metric);
    check(tail.pending()==0 && flat(tail)==L"BA","the older pending record remains for the next update");
    ChatLine afterLog;
    afterLog.receive(int64_t(-1),u"first"); bool enqueued=false;
    const auto enqueueDuringMetric=[&](const std::wstring &word) {
        if (!enqueued && word==L"first") {enqueued=true; afterLog.receive(int64_t(-1),u"new");}
        return metric(word);
    };
    afterLog.consumeIncoming(400,enqueueDuringMetric);
    afterLog.consumeIncoming(400,enqueueDuringMetric);
    check(flat(afterLog)==L"firstfirst" && afterLog.pending()==0,"current tail removal follows logging and its reached callback effects");
    ChatLine lookup;
    lookup.receive(int64_t(1),u"ONE"); std::vector<int64_t> ids;
    lookup.consumeIncoming(400,metric,[&](int64_t id)->std::optional<std::u16string> {
        ids.push_back(id);
        if (ids.size()==1) {lookup.receive(int64_t(2),u"TWO"); return u"first";}
        return u"second";
    });
    check(ids==std::vector<int64_t>{1,2} && flat(lookup)==L"second: TWO" && lookup.pending()==1,
          "prefix and body use the reached fresh tail and second name lookup");
    ChatLine measured;
    std::vector<std::wstring> tokens;
    measured.append(u"A B\nC",{255,255,255},400,[&](const auto &word) {tokens.push_back(word); return metric(word);});
    check(tokens==std::vector<std::wstring>{L"A",L" ",L"B",L"\n",L"C"},"new token metrics precede space and row-break decisions");
    ChatLine negative;
    negative.append(u"kept",{255,255,255},400,metric); negative.trim(-18);
    check(flat(negative)==L"kept","finite negative row capacity follows signed-to-unsigned native comparison");
    ChatLine unordered;
    unordered.append(u"A ",{255,255,255},400,metric);
    unordered.append(u"B",{255,255,255},400,[&](const auto &word) {
        return word==L"A" ? std::numeric_limits<float>::quiet_NaN() : metric(word);
    });
    check(unordered.rows().size()==1 && flat(unordered)==L"A B","unordered carried width follows the append comparison branch");
    std::cout << "Chat cache: " << failures << " failures\n";
    return failures;
}
