// The actual owner GUI template, persistent caption and public window-event bridge.
#include "Assets.h"
#include "Frontend.h"
#include "PlasmaEditScene.h"
#include <fstream>
#include <iostream>

int liveEditTests() {
    int failures=0;
    auto check=[&](bool ok,const char *message) {if (!ok) {std::cerr<<"FAIL live edit: "<<message<<'\n';++failures;}};
    Assets assets;
    if (!assets.locate("")) {std::cout<<"SKIP live owner edit: no local installation\n";return failures;}
    Hud hud;
    std::string error;
    if (!hud.load(assets.gameDir(),error)) {check(false,error.c_str());return failures;}
    hud.setInterfaceState({{false,false,true,false,false},false,false});
    Hud::ScreenItem name;
    name.kind=Hud::ScreenItem::Template;
    name.name="edit";
    name.instance="creation-name";
    name.branch=uiroot::Branch::Creation;
    name.at={100,100};name.size={300,30};name.retainText=true;
    Hud::ScreenItem create;
    create.kind=Hud::ScreenItem::Template;
    create.name="button";create.instance="creation-confirm";
    create.branch=uiroot::Branch::Creation;
    create.at={500,100};create.size={200,30};create.text=L"Create Character";
    int confirmations=0;
    std::u16string committed;
    create.release=[&]{++confirmations;committed=hud.frontendCaption("creation-name").value_or(u"MISSING");};
    Hud::Screen screen{name,create};
    hud.publishFrontendScreen(screen);
    check(hud.frontendCaption("creation-name")==u"","empty caption is retained without an invented space");
    hud.moveInterfacePointer({180,115});
    const auto *target=hud.interfacePointerTarget();
    check(target && target->widget && target->widget->kind=="Edit","empty field picks the actual inner Edit through its clip");
    if (!target || !target->widget || !target->widget->edit) return failures+1;
    const auto editor=target->widget->edit;
    hud.interfaceLeftDown();hud.interfaceLeftUp();
    check(hud.interfaceHasFocus(),"pointer press selects the retained editor");
    MenuInput messages;
    using M=MenuInput::InterfaceMessage;
    for (char16_t c : std::u16string(u"ABCDEFGHIJKLMNOPQ")) messages.interfaceMessages.emplace_back(M::Character,0,uint16_t(c));
    messages.routeInterfaceMessages(hud);
    check(hud.frontendCaption("creation-name")==u"ABCDEFGHIJKLMNOPQ" && editor->edit.caret==17,"actual scene accepts 17 UTF16 units before the later visibility poll");
    hud.publishFrontendScreen(screen);
    check(hud.frontendCaption("creation-name")==u"ABCDEFGHIJKLMNOPQ","publishing another frame does not overwrite edited text");
    hud.interfaceKeyboard(plasma::Input::Key::Down,0x24); // Home
    hud.interfaceKeyboard(plasma::Input::Key::Down,0x10); // GUI Shift membership
    hud.interfaceKeyboard(plasma::Input::Key::Down,0x27);
    hud.interfaceKeyboard(plasma::Input::Key::Up,0x10);
    hud.interfaceKeyboard(plasma::Input::Key::Character,0xD801);
    auto caption=hud.frontendCaption("creation-name").value();
    check(caption.size()==17 && caption[0]==0xD801 && caption[1]==u'B',"literal surrogate replaces the actual selected unit without scalar conversion");
    const int32_t caret=editor->edit.caret;
    auto *page=const_cast<plasma::SceneNode *>(hud.interfaceBranchRoot(uiroot::Branch::Creation));
    page->visible=false;
    hud.interfaceKeyboard(plasma::Input::Key::Character,0x0100);
    check(hud.frontendCaption("creation-name")->at(size_t(caret))==0x0100,"ancestor hiding does not consume focused GUI text");
    page->visible=true;
    hud.interfaceKeyboard(plasma::Input::Key::Down,0x0D);
    check(!hud.interfaceHasFocus() && confirmations==0,"Enter clears focus and does not invoke Create");
    hud.setFrontendCaption("creation-name",u"AB");
    hud.moveInterfacePointer({180,115});hud.interfaceLeftDown();hud.interfaceLeftUp();
    check(editor->edit.caret==2 && editor->edit.selection==-2,"a new focus-acquiring press selects the current caption");
    hud.interfaceKeyboard(plasma::Input::Key::Character,u'C');
    check(hud.frontendCaption("creation-name")==u"C","first-focus selection is used by live character editing");
    hud.setFrontendCaption("creation-name",u"AB");
    hud.moveInterfacePointer({550,115});hud.interfaceLeftDown();
    // A changed caption and outside pointer do not introduce a second length/hit gate.
    hud.setFrontendCaption("creation-name",u"X");
    hud.moveInterfacePointer({900,400});hud.interfaceLeftUp();
    check(confirmations==1 && committed==u"X","captured event3 reads current caption and confirms outside the button");
    hud.clearFrontendBindings();
    hud.moveInterfacePointer({550,115});hud.interfaceLeftDown();hud.interfaceLeftUp();
    check(confirmations==1,"session teardown removes its retained release callback");

    // Independent held attachment example: visibility0/clipping0 -> visibility0/clipping1.
    auto *clip=const_cast<plasma::SceneNode *>(target->parent);
    const bool savedVisible=clip->visible,savedClipping=clip->clips;
    clip->visible=false;clip->clips=false;
    editor->attach();
    check(!clip->visible && clip->clips,"attachment enables parent clipping without making a hidden parent visible");
    auto *textNode=const_cast<plasma::SceneNode *>(target);
    const int savedShape=textNode->shape;
    textNode->shape=-1;
    editor->edit.caret=5;editor->edit.selection=2;clip->clips=false;
    editor->attach();
    check(editor->edit.caret==0 && editor->edit.selection==0 && clip->clips && !clip->visible,
          "missing own caption still clamps selection and independently enables parent clipping");
    textNode->shape=savedShape;editor->attach();
    clip->visible=savedVisible;clip->clips=savedClipping;

    // Real imported-face adapter: cache hits after a different size must still configure
    // the correct metrics. These are local font consistency checks, not native pixels.
    std::ifstream stream(assets.gameDir()/"resource1.dat",std::ios::binary);
    plasma::Font font;
    check(font.load(std::vector<unsigned char>(std::istreambuf_iterator<char>(stream),{})),"owner font loads for indexed measurement");
    plasma::TextStyle style;style.pixelSize=12;style.scalable=true;style.spacing=.5f;
    const auto before=font.indexMetric(u"AB",2,style,glm::mat4(1));
    style.pixelSize=24;font.indexMetric(u"AB",2,style,glm::mat4(1));
    style.pixelSize=12;
    const auto after=font.indexMetric(u"AB",2,style,glm::mat4(1));
    check(before.position==after.position && before.size==after.size,"cached glyph measurement survives an intervening font configuration");
    const auto glyph=font.glyph(L'A',plasma::rasterFor(glm::mat4(1),style));
    const auto atStart=font.indexMetric(u"A",0,style,glm::mat4(1));
    check(atStart.position==plasma::textmetric::Point{0,-12} && atStart.size==plasma::textmetric::Point{glyph.advance,12},"real face supplies indexed first-unit width and baseline");
    std::cout<<"Live owner scene edit: "<<(failures ? "FAIL" : "PASS")<<'\n';
    return failures;
}
