// The frontend's screens drawn by the interface (Hud::drawScreen): the immediate-mode text of
// the game's widgets (frontend-title.md §0.1, plasma-shapes-and-text.md §7.1), flat squares,
// icons and gui.plx templates placed in interface units.
#include "Hud.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>

// The two-pass text object (0x269210): at the widget's point times the interface's scale, the
// outline alone first (a transparent text colour), then the text over it.
void Hud::frontText(std::vector<OverlayVertex> &out, const ScreenItem &item) const {
    if (item.text.empty())
        return;
    glm::mat4 m(1);
    m[0][0] = m[1][1] = guiScale;
    m[3] = glm::vec4(item.at * guiScale, 0, 1);
    plasma::TextStyle style;
    style.string = item.text;
    style.font = "resource1.dat";
    style.pixelSize = item.fontSize;
    style.lineSpacing = 1.0f;
    style.flags = item.align;
    style.scalable = true;
    if (item.outline > 0.0f) {
        style.strokeRadius = item.outline;
        style.color = glm::vec4(0);
        style.strokeColor = item.outlineColor;
        plasmaText(out, style, m);
    }
    style.strokeRadius = 0;
    style.color = item.color;
    plasmaText(out, style, m);
}

glm::vec2 Hud::templateSize(const std::string &name) {
    if (!guiLoaded)
        return glm::vec2(0);
    if (!frontScene) {
        frontScene = std::make_unique<plasma::Scene>(gui);
        frontScene->setScale(1.0f);
    }
    frontScene->setScale(1.0f);
    plasma::SceneNode *probe = frontScene->instance(name, &frontScene->root());
    if (!probe) {
        frontScene->setScale(guiScale);
        return glm::vec2(0);
    }
    const glm::vec2 size = probe->widget ? frontScene->viewSize(*probe) : glm::vec2(0);
    auto &children = frontScene->root().children;
    children.erase(std::remove_if(children.begin(), children.end(), [&](const auto &c) { return c.get() == probe; }), children.end());
    frontScene->setScale(guiScale);
    return size;
}

void Hud::drawScreen(const Screen &screen, std::vector<OverlayVertex> &out, std::vector<glm::vec2> *navigation) const {
    std::map<std::string, size_t> used; // instances of each template taken this frame
    for (const ScreenItem &item : screen) {
        switch (item.kind) {
        case ScreenItem::Text:
            frontText(out, item);
            break;
        case ScreenItem::Square: {
            const glm::vec4 c = glm::clamp(item.color, glm::vec4(0), glm::vec4(1)) * 255.0f;
            rect(out, item.at.x * guiScale, item.at.y * guiScale, item.size.x * guiScale, item.size.y * guiScale,
                 glm::u8vec4((uint8_t)c.r, (uint8_t)c.g, (uint8_t)c.b, (uint8_t)c.a));
            break;
        }
        case ScreenItem::Icon: {
            auto icon = icons.find(item.name);
            if (icon == icons.end() || icon->second.w <= 0)
                break;
            const float scale = item.size.x > 0 ? item.size.x * guiScale / (float)icon->second.w : guiScale;
            image(out, icon->second.atlas, {icon->second.w, icon->second.h}, item.at.x * guiScale, item.at.y * guiScale, scale);
            break;
        }
        case ScreenItem::Template: {
            if (!guiLoaded)
                break;
            if (!frontScene) {
                frontScene = std::make_unique<plasma::Scene>(gui);
            }
            frontScene->setScale(guiScale);
            auto &list = frontNodes[item.name];
            size_t &k = used[item.name];
            if (k >= list.size()) {
                plasma::SceneNode *made = frontScene->instance(item.name, &frontScene->root());
                if (!made)
                    break;
                // A button's "clip" node masks a highlight that only shows through it; the overlay
                // has no clipping layers, so the highlight is left out (a clipped text stays).
                std::vector<plasma::SceneNode *> clips;
                made->findAll("clip", clips);
                for (plasma::SceneNode *clip : clips)
                    for (auto &child : clip->children) {
                        plasma::TextStyle style;
                        const plx::Document &doc = child->doc ? *child->doc : gui;
                        const bool isText = child->shape >= 0 && child->shape < (int)doc.shapes.size() &&
                                            plasma::textStyle(doc.shapes[(size_t)child->shape], doc.loaderFlags, style);
                        if (!isText)
                            child->visible = false;
                    }
                list.push_back(made);
            }
            plasma::SceneNode &node = *list[k++];
            if (node.widget) {
                if (item.size.x > 0 && item.size.y > 0)
                    frontScene->resizeTo(node, item.size * guiScale);
                frontScene->moveTo(node, item.at * guiScale);
            } else
                frontScene->setPosition(node, item.at * guiScale, glm::vec2(0));
            if (!item.text.empty())
                node.setAllText(item.text);
            sceneNode(out, node, item.color, frontScene.get());
            if (navigation && item.navigation != ScreenItem::NoPoint) {
                auto localBounds = [&](const plasma::SceneNode &n, glm::vec2 &lo, glm::vec2 &hi) {
                    plasma::TextStyle style;
                    if (!n.doc || n.shape < 0 || n.shape >= (int)n.doc->shapes.size() ||
                        !plasma::textStyle(n.doc->shapes[(size_t)n.shape], n.doc->loaderFlags, style))
                        return false;
                    if (n.text)
                        style.string = *n.text;
                    plasma::TextBox box;
                    const auto &font = plasmaFonts[fontIndex(style.font)];
                    if (!plasma::textBounds(font, style, frontScene->matrix(n), box))
                        return false;
                    lo = box.minimum, hi = box.maximum;
                    return true;
                };
                auto centre = [&](const plasma::SceneNode &n) {
                    glm::vec2 lo, hi;
                    if (frontScene->screenBounds(n, lo, hi, localBounds))
                        navigation->push_back(glm::vec2(glm::ivec2((lo + hi) * 0.5f)));
                };
                if (item.navigation == ScreenItem::ShapeCentre)
                    centre(node);
                else
                    for (const auto &child : node.children)
                        centre(*child);
            }
            break;
        }
        }
    }
}
