#include "Hud.h"
#include "DialogueChoices.h"
#include "DialoguePanel.h"
#include "NpcSpeech.h"
#include <set>
#include <algorithm>
#include <cmath>

float Hud::dialogueWordWidth(const std::wstring &word) const {
    plasma::TextStyle style;
    style.font = "resource1.dat";
    style.pixelSize = 12;
    style.strokeRadius = 0; // the bounds query excludes the drawn outline
    style.lineSpacing = 1;
    style.scalable = true;
    style.string = word;
    glm::mat4 matrix(1);
    matrix[0][0] = matrix[1][1] = guiScale;
    plasma::TextBox bounds;
    if (!plasma::textBounds(plasmaFonts[0], style, matrix, bounds)) return 0;
    return bounds.maximum.x - bounds.minimum.x;
}

void Hud::drawNpcSpeech(const std::vector<npcspeech::Bubble> &bubbles, int32_t guiTime,
                        std::vector<OverlayVertex> &out,
                        const std::function<void(int64_t, const dialoguepanel::Layout &, glm::vec2)> &onDraw) const {
    if (!guiLoaded) return;
    if (!frontScene) frontScene = std::make_unique<plasma::Scene>(gui);
    frontScene->setScale(guiScale);
    const std::string prefix = "npc-speech-";
    std::set<std::string> retained;
    for (const auto &bubble : bubbles) retained.insert(prefix + std::to_string(bubble.speaker));
    for (auto it = frontNodes.begin(); it != frontNodes.end();) {
        if (!it->first.starts_with(prefix) || retained.count(it->first)) { ++it; continue; }
        const auto nodes = it->second;
        auto &children = frontScene->root().children;
        std::erase_if(children, [&](const auto &node) { return std::find(nodes.begin(),nodes.end(),node.get()) != nodes.end(); });
        it = frontNodes.erase(it);
    }
    for (const auto &bubble : bubbles) {
        auto &nodes = frontNodes[prefix + std::to_string(bubble.speaker)];
        if (nodes.empty()) {
            auto *node = frontScene->instance("speech", &frontScene->root());
            if (!node) continue;
            if (node->widget) {
                node->widget = plasma::reboundWidget(*node->widget);
                node->widget->kind = "DialogWidget";
            }
            nodes.push_back(node);
        }
        auto &node = *nodes.front();
        node.visible = bubble.visible;
        if (!node.visible || !node.widget) continue;
        // moveTo compensates the existing frame origin; offsets remain GUI units.
        frontScene->moveTo(node, bubble.screenPoint + glm::vec2(-125,-170) * guiScale);
        sceneNode(out, node, glm::vec4(1), frontScene.get());
        if (bubble.page >= bubble.pages.size()) continue;
        const auto frame = frontScene->contentSize(node);
        const auto layout = dialoguepanel::layout(bubble.pages[bubble.page].words, bubble.elapsedMs, frame.x,
            [this](const auto &word) { return dialogueWordWidth(word); });
        const glm::vec2 at = frontScene->viewPosition(node) / guiScale;
        drawDialogueWords(layout, at, guiTime, out);
        if (onDraw) onDraw(bubble.speaker, layout, frame);
    }
}

bool Hud::dialogueChoicesHit(glm::vec2 point) const {
    const auto found = frontNodes.find("dialogue-answers");
    return frontScene && found != frontNodes.end() && !found->second.empty() &&
           frontScene->hitTest(*found->second.front(), point) != nullptr;
}

bool Hud::generalDialogueHit(glm::vec2 point) const {
    const auto found = frontNodes.find("dialogue-general");
    return frontScene && found != frontNodes.end() && !found->second.empty() &&
           frontScene->hitTest(*found->second.front(), point) != nullptr;
}

void Hud::drawDialogueChoices(dialoguechoice::Panel &panel, glm::vec2 screen, glm::vec2 mouse,
                              std::vector<OverlayVertex> &out, std::vector<glm::vec2> *navigation) const {
    if (!panel.visible()) return;
    const auto layout = panel.layout(screen / guiScale, [this](const std::wstring &word) {
        return dialogueWordWidth(word);
    });
    ScreenItem background;
    background.kind = ScreenItem::Template;
    background.name = "blackwidget";
    background.instance = "dialogue-answers";
    background.at = layout.at;
    background.size = layout.size;
    drawScreen({background}, out);
    panel.updateHover(layout, dialogueChoicesHit(mouse), mouse.y / guiScale - layout.at.y);
    for (const auto &placed : layout.words) {
        ScreenItem label;
        label.text = placed.word.text;
        label.at = layout.at + placed.at;
        label.color = (int)placed.row == panel.hovered() ? glm::vec4(0,1,1,1) : placed.word.colour;
        frontText(out, label);
    }
    if (navigation)
        for (auto point : layout.navigation) navigation->push_back((layout.at + point) * guiScale);
}

float Hud::dialogueGlyphAdvance(wchar_t glyph) const {
    plasma::TextStyle style;
    style.pixelSize = 12;
    style.scalable = true;
    glm::mat4 matrix(1);
    matrix[0][0] = matrix[1][1] = guiScale;
    const auto raster = plasma::rasterFor(matrix, style);
    return plasma::layout(plasmaFonts[0], std::wstring(1, glyph), raster, 0).pen.x * (12 / raster.size.x);
}

dialoguepanel::Layout Hud::drawGeneralDialogue(const dialoguepanel::General &dialogue, glm::vec2 screen,
                                               int32_t guiTime, std::vector<OverlayVertex> &out) const {
    if (!dialogue.visible()) return {};
    const auto layout = dialogue.layout([this](const std::wstring &word) { return dialogueWordWidth(word); });
    const glm::vec2 window = screen / guiScale;
    const glm::vec2 at{(window.x - 320) / 2, window.y - 150 - 180};
    ScreenItem background;
    background.kind = ScreenItem::Template;
    background.name = "blackwidget";
    background.instance = "dialogue-general";
    background.at = at;
    background.size = {320,180};
    drawScreen({background}, out);
    drawDialogueWords(layout, at, guiTime, out);
    if (const auto indicator = dialogue.indicator(); indicator != dialoguepanel::General::Hidden) {
        const float nextWidth = templateSize("speech-next").x;
        ScreenItem symbol;
        symbol.kind = ScreenItem::Template;
        symbol.name = indicator == dialoguepanel::General::Next ? "speech-next" : "speech-end";
        symbol.instance = "dialogue-indicator-" + symbol.name;
        symbol.at = at + glm::vec2((320 - nextWidth) / 2, 180 - 50);
        if (indicator == dialoguepanel::General::Next) symbol.at.y += 5 * std::cos((float)guiTime * .01f);
        drawScreen({symbol}, out); // retain the copied owner's primitive dimensions
    }
    return layout;
}

void Hud::drawDialogueWords(const dialoguepanel::Layout &layout, glm::vec2 at, int32_t guiTime,
                            std::vector<OverlayVertex> &out) const {
    for (const auto &run : layout.runs) {
        ScreenItem label;
        label.at = at + run.at;
        label.color = run.word.colour;
        if (run.word.flag != 1) {
            label.text = run.word.text;
            frontText(out, label);
            continue;
        }
        float pen = 0;
        for (size_t unit = 0; unit < run.word.text.size(); ++unit) {
            const wchar_t glyph = run.word.text[unit];
            label.text.assign(1, glyph);
            const float shake = std::cos((float)unit + (float)guiTime * .01f) * (12.f * .1f);
            label.at = at + run.at + glm::vec2(pen, shake);
            frontText(out, label);
            pen += dialogueGlyphAdvance(glyph); // single-unit draws do not pair-kern
        }
    }
}
