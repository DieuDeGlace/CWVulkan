// The frontend's screens drawn by the interface (Hud::drawScreen): the immediate-mode text of
// the game's widgets (frontend-title.md §0.1, plasma-shapes-and-text.md §7.1), flat squares,
// icons and gui.plx templates placed in interface units.
#include "Hud.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>

// The two-pass text object (0x269210): at the widget's point times the interface's scale, the
// outline alone first (a transparent text colour), then the text over it.
glm::vec2 Hud::frontText(std::vector<OverlayVertex> &out, const ScreenItem &item) const {
    if (item.branch==uiroot::Branch::Help) return helpText(out,item);
    if (item.text.empty())
        return {};
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
    return {};
}

void Hud::layoutHelpNode(glm::ivec2 windowPixels) const {
    if (!scene || !interfaceBranches) return;
    scene->setScale(guiScale);
    // Selected 0x101BD0 placement: a window point and a separate local offset.
    scene->setPosition(*interfaceBranches->node(uiroot::Branch::Help),{0,float(windowPixels.y)},{20,-250});
}

glm::vec2 Hud::helpText(std::vector<OverlayVertex> &out,const ScreenItem &item) const {
    if (!scene || !interfaceBranches) return {};
    const auto *node=interfaceBranches->node(uiroot::Branch::Help);
    const int font=fontIndex("resource1.dat");
    if (!node || !plasmaFonts[font].loaded()) return {};
    const glm::mat4 matrix=scene->matrix(*node);
    const float factor=matrix[0][0];
    const glm::vec2 pivot=node->transform.pivot;
    const glm::vec2 point{(item.at.x+pivot.x)*factor,(item.at.y+pivot.y)*factor};
    plasma::TextStyle style;
    style.string=item.text;
    style.font="resource1.dat";
    style.pixelSize=item.fontSize;
    style.flags=item.align;
    style.scalable=true;
    style.strokeColor=item.outlineColor;
    glm::vec2 flow;
    if (item.outline>0.0f) {
        style.strokeRadius=item.outline;
        style.color=glm::vec4(0);
        plasmaText(out,style,matrix,nullptr,point,&flow); // Native return uses the first/outline pass.
        style.strokeRadius=0;
        style.color=item.color;
        plasmaText(out,style,matrix,nullptr,point);
    } else {
        style.color=item.color;
        plasmaText(out,style,matrix,nullptr,point,&flow);
    }
    const glm::vec2 currentPivot=node->transform.pivot;
    return {flow.x/factor-currentPivot.x,flow.y/factor-currentPivot.y};
}

void Hud::setInterfaceState(const uiroot::State &state) {
    if (interfaceBranches) {
        auto current = state;
        current.help = helpRootShown;
        interfaceBranches->apply(current);
    }
}
void Hud::setInterfaceShown(bool shown) {
    interfaceRootShown = shown;
    if (scene)
        scene->root().visible = shown;
}

void Hud::setHelpShown(bool shown) {
    helpRootShown = shown;
    if (interfaceBranches)
        interfaceBranches->node(uiroot::Branch::Help)->visible = shown;
}

const plasma::SceneNode *Hud::interfaceBranchRoot(uiroot::Branch branch) const {
    return interfaceBranches ? interfaceBranches->node(branch) : nullptr;
}

Hud::TemplateInstance *Hud::prepareTemplate(const ScreenItem &item, size_t index, uiroot::Branch fallback) const {
    if (!guiLoaded || !scene)
        return nullptr;
    const auto branch = item.branch == uiroot::Branch::Unspecified ? fallback : item.branch;
    const std::string &owner = item.instance.empty() ? item.name : item.instance;
    auto &instances = frontNodes[owner];
    while (instances.size() <= index) {
        auto *parent = interfaceBranches ? interfaceBranches->node(branch) : &scene->root();
        auto *node = scene->instance(item.name, parent);
        if (!node)
            return nullptr;
        instances.push_back({node});
    }
    auto &instance = instances[index];
    auto &node = *instance.node;
    if (interfaceBranches && branch != uiroot::Branch::Unspecified) {
        auto *parent = interfaceBranches->node(branch);
        if (node.parent != parent)
            scene->reparent(node, parent);
    }
    const bool first = instance.requested.x < 0;
    if (first && item.replaceWidget && node.widget)
        scene->replaceWidget(node, plasma::reboundWidget(*node.widget));
    if (first || instance.requested != item.size) {
        // Constructor requests precede the global scale and the Button's keyed poses.
        // Later draws preserve that evaluated frame instead of requesting the size again.
        scene->setScale(1);
        if (node.widget && item.size.x > 0 && item.size.y > 0)
            scene->resizeTo(node, item.size);
        instance.requested = item.size;
    }
    if (first) {
        const bool visible = node.visible;
        if (item.initializeHidden)
            node.visible = false;
        node.initializeWidgets();
        if (item.retainText) node.setAllText(item.text);
        bindEdits(node);
        node.visible = visible;
        if (item.name == "button" || item.name == "button2" || item.name == "smallbutton") {
            auto passive = [](auto &&self, plasma::SceneNode &parent) -> void {
                for (auto &child : parent.children) {
                    child->flags |= plx::NODE_PASSIVE;
                    self(self, *child);
                }
            };
            passive(passive, node);
        }
        // Keep authored child visibility; clip/blur traversal owns composition.
    }
    instance.press=item.press;
    instance.release=item.release;
    instance.motion=item.motion;
    if (node.widget) node.widget->frontendEventButton=bool(item.press || item.release || item.motion);
    if (item.press) {
        interfaceInput->registerEvent(node,plasma::Input::Event::LeftDown,
            [this,owner,index](plasma::Input &,plasma::SceneNode &) {
                const auto found=frontNodes.find(owner);
                if (found!=frontNodes.end() && index<found->second.size()) {
                    const auto callback=found->second[index].press;
                    if (callback) callback();
                }
            });
    }
    if (item.release) {
        interfaceInput->registerEvent(node,plasma::Input::Event::LeftUp,
            [this,owner,index](plasma::Input &,plasma::SceneNode &) {
                const auto found=frontNodes.find(owner);
                if (found!=frontNodes.end() && index<found->second.size()) {
                    const auto callback=found->second[index].release;
                    if (callback) callback();
                }
            });
    }
    if (item.motion) {
        interfaceInput->registerEvent(node,plasma::Input::Event::Move,
            [this,owner,index](plasma::Input &input,plasma::SceneNode &) {
                const auto found=frontNodes.find(owner);
                if (found!=frontNodes.end() && index<found->second.size()) {
                    const auto callback=found->second[index].motion;
                    if (callback) callback(input,*scene,*found->second[index].node);
                }
            });
    }
    scene->setScale(guiScale);
    return &instance;
}

const plasma::SceneNode *Hud::frontendNode(const std::string &owner) const {
    const auto found=frontNodes.find(owner);
    return found!=frontNodes.end() && !found->second.empty() ? found->second.front().node : nullptr;
}

std::optional<Hud::FrontendFrame> Hud::frontendFrame(const std::string &owner) const {
    const auto *node=frontendNode(owner);
    if (!scene || !node || !node->widget) return std::nullopt;
    return FrontendFrame{scene->contentPosition(*node),scene->screenSize(*node)};
}

void Hud::publishFrontendGeometry(const std::string &owner, glm::vec2 position, glm::vec2 size) const {
    const auto found=frontNodes.find(owner);
    if (!scene || found==frontNodes.end() || found->second.empty()) return;
    auto &instance=found->second.front();
    if (!instance.node->widget) return;
    scene->moveTo(*instance.node,position*guiScale); // Position publication precedes size.
    scene->setScale(1);
    scene->resizeTo(*instance.node,size);
    instance.requested=size;
    scene->setScale(guiScale);
}

glm::vec2 Hud::templateViewSize(const ScreenItem &item) const {
    auto *instance = prepareTemplate(item);
    return instance && instance->node->widget ? scene->viewSize(*instance->node) : glm::vec2(0);
}

glm::vec2 Hud::templateContentSize(const ScreenItem &item) const {
    const auto *instance = prepareTemplate(item);
    return instance ? scene->contentSize(*instance->node) : glm::vec2(0);
}

glm::vec2 Hud::templateLocalPoint(const ScreenItem &item, glm::vec2 point) const {
    auto *instance = prepareTemplate(item);
    if (!instance) return {};
    placeTemplate(*instance, item);
    return scene->widgetPointer(*instance->node, point);
}

glm::vec2 Hud::templateSize(const std::string &name, glm::vec2 requested, bool replaceWidget) const {
    if (!guiLoaded || !scene)
        return glm::vec2(0);
    scene->setScale(1.0f);
    plasma::SceneNode *probe = scene->instance(name, nullptr); // Sizing is independent of the live root.
    if (!probe) {
        scene->setScale(guiScale);
        return glm::vec2(0);
    }
    if (replaceWidget && probe->widget)
        scene->replaceWidget(*probe, plasma::reboundWidget(*probe->widget));
    if (probe->widget && requested.x > 0 && requested.y > 0)
        scene->resizeTo(*probe, requested);
    probe->initializeWidgets();
    glm::vec2 size = probe->widget ? scene->viewSize(*probe) : glm::vec2(0);
    if (!probe->widget) {
        glm::vec2 minimum, maximum;
        if (scene->screenBounds(*probe, minimum, maximum))
            size = maximum - minimum;
    }
    scene->destroy(*probe);
    scene->setScale(guiScale);
    return size;
}

bool Hud::templateHit(const ScreenItem &item, glm::vec2 point, bool passiveDescendants) const {
    if (!guiLoaded || item.kind != ScreenItem::Template)
        return false;
    ScreenItem probe = item;
    if (probe.instance.empty())
        probe.instance = "hit/" + item.name + (passiveDescendants ? "/passive" : "/ordinary");
    auto *instance = prepareTemplate(probe);
    if (!instance)
        return false;
    auto *node = instance->node;
    if (passiveDescendants) {
        auto passive = [](auto &&self, plasma::SceneNode &parent) -> void {
            for (auto &child : parent.children) {
                child->flags |= plx::NODE_PASSIVE;
                self(self, *child);
            }
        };
        passive(passive, *node);
    }
    if (node->widget)
        scene->moveTo(*node, item.at * guiScale);
    else
        scene->setPosition(*node, item.at * guiScale, glm::vec2(0));
    if (!item.text.empty())
        node->setAllText(item.text);
    return scene->hitTest(*node, point * guiScale, frontTextHit()) != nullptr;
}

plasma::Scene::TextHit Hud::frontTextHit() const {
    return [this](const plasma::SceneNode &n, glm::vec2 local) {
        plasma::TextStyle style;
        const auto *shape = n.shapeObject();
        if (!shape || !plasma::textStyle(*shape, n.doc->loaderFlags, style))
            return false;
        if (n.text)
            style.string = *n.text;
        const auto &font = plasmaFonts[fontIndex(style.font, style.templateFont)];
        plasma::TextBox box;
        return font.loaded() && plasma::textBounds(font, style, scene->matrix(n), box) && local.x >= box.minimum.x &&
               local.y >= box.minimum.y && local.x <= box.maximum.x && local.y <= box.maximum.y;
    };
}

void Hud::placeTemplate(TemplateInstance &instance, const ScreenItem &item) const {
    auto &node = *instance.node;
    if (node.widget)
        scene->moveTo(node, item.at * guiScale);
    else
        scene->setPosition(node, item.at * guiScale, glm::vec2(0));
    if (!item.retainText && !item.text.empty())
        node.setAllText(item.text);
}

void Hud::publishFrontendScreen(const Screen &screen) const {
    for (auto &[owner, instances] : frontNodes)
        for (auto &instance : instances)
            if (instance.frontendManaged)
                instance.node->visible = false;
    std::map<std::string, size_t> used;
    for (const auto &item : screen) {
        if (item.kind != ScreenItem::Template)
            continue;
        const auto &owner = item.instance.empty() ? item.name : item.instance;
        if (auto *instance = prepareTemplate(item, used[owner]++, uiroot::Branch::Root)) {
            instance->frontendManaged = true;
            instance->node->visible = item.visible;
            placeTemplate(*instance, item);
        }
    }
}

void Hud::drawScreen(const Screen &screen, std::vector<OverlayVertex> &out, std::vector<glm::vec2> *navigation) const {
    std::map<std::string, size_t> used; // instances of each template taken this frame
    for (const ScreenItem &item : screen) {
        switch (item.kind) {
        case ScreenItem::Text:
        case ScreenItem::TextGroup: {
            bool admitted=true;
            if (item.branch==uiroot::Branch::Help && interfaceBranches)
                for (const auto *node=interfaceBranches->node(uiroot::Branch::Help);node;node=node->parent)
                    admitted=admitted && node->visible;
            if (!admitted) break;
            if (item.kind==ScreenItem::Text) frontText(out,item);
            else if (item.drawTexts) {
                ScreenItem part;
                part.branch=item.branch;
                part.fontSize=item.fontSize;
                part.outline=item.outline;
                part.outlineColor=item.outlineColor;
                part.align=item.align;
                item.drawTexts([&](const auto &value,auto at,auto colour) {
                    part.text=value; part.at=at; part.color=colour;
                    return frontText(out,part);
                });
            }
            break;
        }
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
            const std::string &owner = item.instance.empty() ? item.name : item.instance;
            auto *instance = prepareTemplate(item, used[owner]++, uiroot::Branch::Root);
            if (!instance)
                break;
            auto &node = *instance->node;
            node.visible = item.visible;
            placeTemplate(*instance, item);
            if (!node.visible) break;
            bool active = true;
            for (const auto *parent = node.parent; parent; parent = parent->parent)
                active = active && parent->visible;
            if (!active)
                break;
            if (item.buttonHover && node.widget && node.widget->kind == "Button") {
                if (*item.buttonHover != instance->pointerInside) {
                    node.playSequence(*item.buttonHover ? "button:enter" : "button:leave");
                    instance->pointerInside = *item.buttonHover;
                }
                node.advanceSequences(item.animationStepMs);
            }
            sceneNode(out, node, item.color, scene.get(), [&](const plasma::SceneNode &drawn) {
                if (&drawn != &node || !item.drawModels)
                    return;
                ScreenItem::ModelContext context;
                context.matrix = scene->matrix(drawn);
                context.pivot = drawn.transform.pivot;
                context.hovered = item.modelPointer && scene->hitTest(drawn, *item.modelPointer, frontTextHit());
                auto requests = item.drawModels(context);
                for (auto &request : requests) {
                    request.beforeVertex = out.size();
                    itemRequests.push_back(std::move(request));
                }
            });
            if (navigation && item.navigation != ScreenItem::NoPoint) {
                auto localBounds = [&](const plasma::SceneNode &n, glm::vec2 &lo, glm::vec2 &hi) {
                    plasma::TextStyle style;
                    const auto *shape = n.shapeObject();
                    if (!shape || !plasma::textStyle(*shape, n.doc->loaderFlags, style))
                        return false;
                    if (n.text)
                        style.string = *n.text;
                    plasma::TextBox box;
                    const auto &font = plasmaFonts[fontIndex(style.font, style.templateFont)];
                    if (!plasma::textBounds(font, style, scene->matrix(n), box))
                        return false;
                    lo = box.minimum, hi = box.maximum;
                    return true;
                };
                auto centre = [&](const plasma::SceneNode &n) {
                    glm::vec2 lo, hi;
                    if (scene->screenBounds(n, lo, hi, localBounds))
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
