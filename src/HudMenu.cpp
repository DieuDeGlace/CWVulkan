// Independent menu construction/layout/publication contracts, 2026-11-09.
// Dimensions, control flags and shapes come from the owner's unmodified templates.
#include "Hud.h"
#include <utility>

void Hud::bindMenuPress(plasma::SceneNode &node,const std::function<void()> &callback) {
    const auto bind=[&](auto &&self,plasma::SceneNode &control,bool root)->void {
        if (!root && (control.flags & plx::NODE_DISABLED)) return;
        if (control.widget) {
            control.widget->frontendEventButton=true;
            interfaceInput->registerEvent(control,plasma::Input::Event::LeftDown,
                [callback](plasma::Input &,plasma::SceneNode &) {if (callback) callback();});
        }
        for (auto &child:control.children) self(self,*child,false);
    };
    bind(bind,node,true);
}

void Hud::buildMenuControls() {
    static constexpr const char *resources[]{"crafting-menu","inventory-menu","worldmap-menu",
                                            "system-menu","multiplayer-menu","help-menu"};
    for (size_t index=0;index<panels.menus.size();++index) {
        auto *node=scene->instance("menubutton",panels.inGame);
        panels.menus[index]=node;
        if (!node) continue; // Missing owner resources are an explicit engine boundary.
        if (iconsLoaded) scene->fitMenuIcon(*node,"background",iconsDoc,resources[index]);
        node->initializeWidgets();
        bindMenuPress(*node,[this,index] {
            const auto callback=menuAction;
            if (callback) callback(Menu(index));
        });
        node->visible=false;
    }
}

void Hud::bindMenuActions(std::function<void(Menu)> action) {menuAction=std::move(action);}
const plasma::SceneNode *Hud::menuControl(Menu menu) const {
    const auto index=size_t(menu);
    return index<panels.menus.size() ? panels.menus[index] : nullptr;
}
void Hud::layoutMenuControls(glm::ivec2 windowPixels) const {
    if (!scene) return;
    scene->setScale(guiScale);
    const glm::vec2 anchor(float(windowPixels.x/2),float(windowPixels.y));
    const auto count=int32_t(panels.menus.size());
    for (int32_t index=0;index<count;++index)
        if (auto *node=panels.menus[size_t(index)])
            scene->setPosition(*node,anchor,{float((2*index-count+1)*30),-200.0f});
}
void Hud::publishMenuControls(const uint8_t &menuByte) {
    for (auto *node:panels.menus)
        if (node) node->visible=menuByte!=0; // Separate late update, including while F4 hides root.
}
void Hud::drawMenuControls(std::vector<OverlayVertex> &out) const {
    if (!scene || !panels.inGame || !panels.inGame->visible) return;
    for (const auto *node:panels.menus)
        if (node) sceneNode(out,*node,glm::vec4(1));
}
