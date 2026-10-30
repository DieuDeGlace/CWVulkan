#include "Hud.h"
#include "ChatLine.h"
#include <glm/gtc/matrix_transform.hpp>

plasma::SceneNode *Hud::placeChat() const {
    if (!scene || !panels.chat) return nullptr;
    scene->setScale(guiScale);
    auto &node = *panels.chat;
    scene->setPosition(node, screen - scene->viewSize(node), {-20, -20});
    return &node;
}

float Hud::chatWordWidth(const std::wstring &word) const {
    if (!scene || !panels.chat) return 0;
    plasma::TextStyle style;
    style.font = "resource1.dat";
    style.pixelSize = 12;
    style.strokeRadius = 3;
    style.scalable = true;
    style.string = word;
    plasma::TextBox bounds;
    if (!plasma::textBounds(plasmaFonts[0], style, scene->matrix(*panels.chat), bounds)) return 0;
    return bounds.maximum.x - bounds.minimum.x;
}

void Hud::updateChat(ChatLine &chat, const std::function<std::optional<std::u16string>(int64_t)> &name) const {
    if (auto *node = placeChat())
        chat.consumeIncoming(scene->contentSize(*node).x,
                             [this](const auto &word) { return chatWordWidth(word); }, name);
}

void Hud::appendChat(ChatLine &chat, const std::u16string &text, std::array<uint8_t, 3> colour) const {
    if (auto *node = placeChat())
        chat.append(text, colour, scene->contentSize(*node).x,
                    [this](const auto &word) { return chatWordWidth(word); });
}

void Hud::drawChat(ChatLine &chat, int32_t guiClock, std::vector<OverlayVertex> &out) const {
    auto *node = placeChat();
    if (!node) return;
    const auto frame = scene->contentSize(*node);
    chat.trim(frame.y);
    const glm::vec4 black(0, 0, 0, 1), white(1);
    float y = 19;
    for (const auto &row : chat.rows()) {
        float x = 3;
        for (const auto &word : row) {
            if (word.text == L" ") { x += 5; continue; }
            const glm::vec4 colour(word.colour[0] / 255.0f, word.colour[1] / 255.0f, word.colour[2] / 255.0f, 1);
            widgetText(out, *node, word.text, {x, y}, 12, 3, colour, black, 0, 0);
            x += chatWordWidth(word.text);
        }
        y += 18;
    }
    const std::wstring editor(chat.text().begin(), chat.text().end());
    widgetText(out, *node, editor, {3, frame.y - 12}, 12, 3, white, black, 0, 0);
    if (chat.typing() && ChatLine::caretVisible(guiClock)) {
        plasma::TextStyle style;
        style.pixelSize = 12;
        style.scalable = true;
        const auto matrix = scene->matrix(*node);
        const auto raster = plasma::rasterFor(matrix, style);
        const auto prefix = editor.substr(0, (size_t)chat.caretIndex());
        const auto pen = plasma::layout(plasmaFonts[0], prefix, raster, 0).pen;
        const float x = pen.x * style.pixelSize / raster.size.x;
        widgetText(out, *node, L"|", {3 + x, frame.y - 12}, 12, 3, white, black, 0, 0);
    }
}

bool Hud::requireText(const std::u16string &text) {
    bool changed = false;
    for (char16_t unit : text)
        if (unit >= 32 && !(unit < 127 || unit == 0xb0 || unit == 0xae || unit == 0xb1))
            changed = additionalGlyphs.insert((wchar_t)unit).second || changed;
    return changed;
}
