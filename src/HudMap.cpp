// Code-created recent-land text and owner arrow templates (steam-map-region-history sections1–5).
#include "Hud.h"
#include "DialogueWords.h"

void Hud::buildMapHistory() {
    historyPanel = {};
    if (!scene || !panels.inGame)
        return;
    historyPanel.root = scene->create(panels.inGame, "recentlands");
    historyPanel.root->visible = false;
    historyPanel.left = scene->instance("leftbutton", historyPanel.root);
    historyPanel.right = scene->instance("rightbutton", historyPanel.root);
    if (historyPanel.left)
        historyPanel.left->transform.translation = glm::vec2(-100, 2) - historyPanel.left->transform.pivot;
    if (historyPanel.right)
        historyPanel.right->transform.translation = glm::vec2(100, 2) - historyPanel.right->transform.pivot;
    historyPanel.root->initializeWidgets();
}

void Hud::bindMapHistory(const maphistory::History::View &view, const std::string &caption, bool interfaceVisible) {
    if (!historyPanel.root)
        return;
    historyPanel.ancestorVisible = interfaceVisible;
    historyPanel.root->visible = view.shown;
    if (view.caption)
        historyPanel.caption = dialoguewords::wide(caption);
    if (historyPanel.left)
        historyPanel.left->visible = view.older;
    if (historyPanel.right)
        historyPanel.right->visible = view.newer;
    scene->setScale(guiScale);
    scene->setPosition(*historyPanel.root, {0, screen.y}, {150, -50});
}

bool Hud::mapHistoryArrowHit(bool older, glm::vec2 pointer) const {
    const auto *arrow = older ? historyPanel.left : historyPanel.right;
    // Native standalone subtree queries use ancestor matrices, not ancestor visibility.
    return arrow && scene->hitTest(*arrow, pointer) != nullptr;
}

int Hud::mapHistoryAction(glm::vec2 pointer) const {
    if (!historyPanel.root || !historyPanel.ancestorVisible || !historyPanel.root->visible)
        return 0;
    for (const auto *hit = scene->hitTest(*historyPanel.root, pointer); hit; hit = hit->parent) {
        if (hit == historyPanel.left)
            return -1;
        if (hit == historyPanel.right)
            return 1;
    }
    return 0;
}

void Hud::mapHistoryPointer(glm::vec2 pointer, bool highlighted) {
    historyPanel.textColor = highlighted ? glm::vec4(0, 1, 1, 1) : glm::vec4(1);
    if (!historyPanel.root || !historyPanel.root->visible)
        return;
    if (historyPanel.left)
        historyPanel.left->fill = mapHistoryArrowHit(true, pointer) ? glm::vec4(0, 1, 1, 1) : glm::vec4(1);
    if (historyPanel.right)
        historyPanel.right->fill = mapHistoryArrowHit(false, pointer) ? glm::vec4(0, 1, 1, 1) : glm::vec4(1);
}

void Hud::drawMapHistory(std::vector<OverlayVertex> &out) const {
    if (!historyPanel.root || !historyPanel.root->visible || !historyPanel.ancestorVisible)
        return;
    auto style = labelStyle(16, 3, 0x15, false);
    style.wrapWidth = 180;
    style.string = historyPanel.caption;
    style.color = historyPanel.textColor;
    const auto matrix = scene->matrix(*historyPanel.root);
    plasmaText(out, style, matrix);
    style.strokeRadius = 0;
    plasmaText(out, style, matrix);
    sceneNode(out, *historyPanel.root, glm::vec4(1));
}
