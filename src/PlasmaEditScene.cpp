#include "PlasmaEditScene.h"
#include <algorithm>
#include <bit>

namespace plasma {
namespace {
Edit::Matrix words(const glm::mat4 &matrix) {
    Edit::Matrix result;
    for (int c=0;c<4;++c) for (int r=0;r<4;++r) result[c*4+r]=matrix[c][r];
    return result;
}
glm::mat4 matrix(const Edit::Matrix &words) {
    glm::mat4 result;
    for (int c=0;c<4;++c) for (int r=0;r<4;++r) result[c][r]=words[c*4+r];
    return result;
}
}
EditScene::EditScene(Scene &scene,SceneNode &node,Input &input,Measure measure,Bound bound)
    :edit(captionPorts()),scene(scene),node(node),input(input),measure(std::move(measure)),bound(std::move(bound)) {}

Edit::Caption EditScene::captionPorts() {
    Edit::Caption ports;
    ports.read=[this]{return text();};
    ports.write=[this](const std::u16string &text){setCaptionText(node,text);};
    ports.available=[this]{return caption;};
    ports.layout=[this]{layout();};
    ports.changed=[this]{input.dispatchWidgetEvent(node,Input::Event::TextChanged);};
    ports.clearFocus=[this]{input.focus(nullptr);};
    ports.activity=[this]{if (node.widget) node.widget->dirty=true;};
    // Scene strings are live values; no deferred shape rebuild is necessary.
    ports.refresh=[](bool){};
    return ports;
}
std::optional<TextStyle> EditScene::style() const {
    TextStyle value;
    const auto *shape=node.shapeObject();
    if (!caption || !shape || !textStyle(*shape,node.doc ? node.doc->loaderFlags : 0,value)) return {};
    if (node.text) value.string=*node.text;
    return value;
}
std::u16string EditScene::text() const {
    const auto value=style();
    if (!value) return {};
    return {value->string.begin(),value->string.end()};
}
void EditScene::attach() {
    TextStyle value;
    const auto *shape=node.shapeObject();
    caption=shape && textStyle(*shape,node.doc ? node.doc->loaderFlags : 0,value);
    edit.clamp();
    if (node.parent && node.parent->shapeObject()) node.parent->clips=true;
    baseline=node.transform.translation.x;
}
Edit::Pointer EditScene::pointerPorts() {
    Edit::Pointer ports;
    ports.selected=[this]{return input.focusWidget()==&node;};
    ports.leftHeld=[this]{return (input.heldButtons()&1)!=0;};
    ports.shiftHeld=[this]{return input.keyHeld(0x10);};
    ports.localX=[this]{
        const auto point=input.pointer();
        const auto inverse=scene.inverseMatrix(node);
        const float reciprocal=1.f/((point.y*inverse[1][3]+point.x*inverse[0][3])+inverse[3][3]);
        return ((point.y*inverse[1][0]+point.x*inverse[0][0])+inverse[3][0])*reciprocal;
    };
    ports.select=[this]{input.focus(&node);};
    ports.capture=[this]{input.capture(&node);};
    ports.release=[this]{input.clearCapture();};
    ports.beginPick=[this]{
        const auto captured=scene.matrix(node);
        return Edit::Query([this,captured](std::u16string_view text,int32_t index){return measure(node,text,index,captured);});
    };
    return ports;
}
void EditScene::pointer(Input::Hook hook) {
    const auto ports=pointerPorts();
    if (hook==Input::Hook::LeftDown) edit.leftDown(ports);
    else if (hook==Input::Hook::LeftUp) edit.leftUp(ports);
    else if (hook==Input::Hook::Move) edit.move(ports);
}
void EditScene::keyboard(Input::Key key,uint16_t unit) {
    if (key==Input::Key::Character) edit.character(unit);
    else if (key==Input::Key::Down) edit.keyDown(unit,input.keyHeld(0x10));
}
void EditScene::layout() {
    Edit::Layout ports;
    ports.hasParent=[this]{return node.parent!=nullptr;};
    ports.lower=[this]{const auto value=bound(*node.parent,false);return Edit::Point{value.x,value.y};};
    ports.upper=[this]{const auto value=bound(*node.parent,true);return Edit::Point{value.x,value.y};};
    ports.captureNode=[this]{
        const auto forward=[this]{return words(scene.matrix(node));};
        return Edit::Layout::Node{forward,forward};
    };
    ports.localMatrix=[this]{return words(scene.transformation(*node.parent)*scene.transformation(node));};
    ports.measure=[this](std::u16string_view text,int32_t index,const Edit::Matrix &m){return measure(node,text,index,matrix(m));};
    ports.x=[this]{return node.transform.translation.x;};
    ports.savedX=[this]{return baseline;};
    ports.setX=[this](float x){node.transform.translation.x=x;};
    ports.updateTransform=[this]{node.placementChanged=true;};
    edit.adjustLayout(ports);
}
std::optional<EditScene::Caret> EditScene::drawMetrics() const {
    if (input.focusWidget()!=&node || !caption) return {};
    const auto captionStyle=style();
    if (!captionStyle) return {};
    const auto captured=scene.matrix(node);
    const auto string=text();
    const int32_t endpoint=std::bit_cast<int32_t>(uint32_t(edit.caret)+uint32_t(edit.selection));
    const int32_t lower=std::max(0,std::min(edit.caret,endpoint));
    const int32_t upper=std::min(int32_t(string.size()),std::max(edit.caret,endpoint));
    measure(node,string,lower,captured);
    measure(node,string,upper,captured);
    // The ordinary manager's selection virtual is return-only. Reread after it.
    const auto current=text();
    if (edit.caret<0 || size_t(edit.caret)>current.size()) return {};
    const auto metric=measure(node,current,edit.caret,captured);
    const auto currentStyle=style();
    if (!currentStyle) return {};
    return Caret{metric.position[0],-currentStyle->pixelSize,currentStyle->pixelSize*1.25f};
}
}
