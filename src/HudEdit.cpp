#include "Hud.h"
#include "PlasmaEditScene.h"
#include <algorithm>

void Hud::bindEdits(plasma::SceneNode &node) const {
    if (node.widget && node.widget->kind=="Edit" && !node.widget->edit) {
        auto measure=[this](const plasma::SceneNode &node,std::u16string_view text,int32_t index,const glm::mat4 &matrix) {
            plasma::TextStyle style;
            const auto *shape=node.shapeObject();
            if (!shape || !plasma::textStyle(*shape,node.doc ? node.doc->loaderFlags : 0,style)) return plasma::Edit::Metric{};
            const auto &font=plasmaFonts[fontIndex(style.font,style.templateFont)];
            const auto result=font.indexMetric(text,index,style,matrix);
            return plasma::Edit::Metric{result.position,result.size};
        };
        // Ordinary edit/clip has its own rectangle. Attachment suppresses child
        // aggregation, so the editable text does not enlarge its scroll bounds.
        // Existing scene shape/point mapping supplies the geometric boundary.
        auto bound=[this](const plasma::SceneNode &clip,bool upper) {
            glm::vec2 lower(0),higher(0);
            if (!scene->shapeBounds(clip,lower,higher,false)) return glm::vec2(0);
            const auto inverse=clip.parent ? scene->inverseMatrix(*clip.parent) : glm::mat4(1);
            glm::vec2 result(0);
            bool first=true;
            for (const auto corner : {lower,higher,glm::vec2(lower.x,higher.y),glm::vec2(higher.x,lower.y)}) {
                const auto mapped=scene->deform(clip,corner);
                const auto point=inverse*glm::vec4(mapped,0,1);
                const glm::vec2 value=glm::vec2(point)/point.w;
                result=first ? value : upper ? glm::max(result,value) : glm::min(result,value);
                first=false;
            }
            return result;
        };
        node.widget->edit=std::make_shared<plasma::EditScene>(*scene,node,*interfaceInput,measure,bound);
        node.widget->edit->attach();
    }
    for (const auto &child : node.children) bindEdits(*child);
}

void Hud::interfaceKeyboard(plasma::Input::Key key,uint16_t unit) {
    if (!interfaceInput) return;
    if (key==plasma::Input::Key::Down) interfaceInput->keyDown(unit);
    else if (key==plasma::Input::Key::Up) interfaceInput->keyUp(unit);
    else interfaceInput->character(unit);
    if (const auto *focus=interfaceInput->focusWidget()) requireText(plasma::captionText(*focus));
}
bool Hud::interfaceHasFocus() const {return interfaceInput && interfaceInput->focusWidget();}
bool Hud::interfaceKeyHeld(uint16_t key) const {return interfaceInput && interfaceInput->keyHeld(key);}

std::optional<std::u16string> Hud::frontendCaption(const std::string &owner) const {
    const auto found=frontNodes.find(owner);
    if (found==frontNodes.end() || found->second.empty()) return {};
    return plasma::captionText(*found->second.front().node);
}
void Hud::setFrontendCaption(const std::string &owner,std::u16string_view text) {
    const auto found=frontNodes.find(owner);
    if (found!=frontNodes.end() && !found->second.empty()) plasma::setCaptionText(*found->second.front().node,text);
}
void Hud::clearFrontendBindings() {
    if (!interfaceInput) return;
    for (auto &[owner,instances] : frontNodes)
        for (auto &instance : instances) {
            interfaceInput->registerEvent(*instance.node,plasma::Input::Event::LeftDown,{});
            interfaceInput->registerEvent(*instance.node,plasma::Input::Event::LeftUp,{});
            instance.press={};
            instance.release={};
        }
}

void Hud::drawEdit(std::vector<OverlayVertex> &out,const plasma::SceneNode &node) const {
    if (!node.widget || !node.widget->edit) return;
    const auto style=node.widget->edit->style();
    if (!style || !plasmaFonts[fontIndex(style->font,style->templateFont)].loaded()) return;
    const auto caret=node.widget->edit->drawMetrics();
    if (!caret || ((interfaceClock.elapsed/500)&1)==0) return;
    const auto matrix=scene->matrix(node); // current manager draw matrix, after metrics
    const auto a=matrix*glm::vec4(caret->x,caret->y,0,1);
    const auto b=matrix*glm::vec4(caret->x+1,caret->y,0,1);
    const auto c=matrix*glm::vec4(caret->x+1,caret->y+caret->height,0,1);
    const auto d=matrix*glm::vec4(caret->x,caret->y+caret->height,0,1);
    const float u=white.x+2.f,v=white.y+2.f;
    for (const auto &point : {a,b,c,a,c,d})
        out.push_back({point.x,point.y,u,v,255,0,0,255,point.w,point.z});
}
