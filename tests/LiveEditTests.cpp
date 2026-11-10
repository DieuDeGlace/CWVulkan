// The actual owner GUI template, persistent caption and public window-event bridge.
#include "Assets.h"
#include "Frontend.h"
#include "PlasmaEditScene.h"
#include <fstream>
#include <iostream>
#include <locale>

namespace {
// Exercise the real stream boundary, without injecting a replacement formatter.
class CaptionNumber final : public std::num_put<wchar_t> {
  public:
    std::function<void()> beforeNumber;
  private:
    iter_type do_put(iter_type out,std::ios_base &stream,char_type fill,long value) const override {
        if (beforeNumber) beforeNumber();
        return std::num_put<wchar_t>::do_put(out,stream,fill,value);
    }
};
}


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

    uiroot::State pages{{false,false,true,false,false},false,false,true};
    Hud::ScreenItem back=create;
    back.instance="frontend-back";back.at={20,20};back.size={100,30};back.text=L"Back";
    back.branch=uiroot::Branch::Root;back.release={};
    int backPresses=0;
    back.press=[&]{++backPresses;uiroot::back(pages);hud.setInterfaceState(pages);};
    hud.setInterfaceState(pages);
    hud.publishFrontendScreen({name,create,back});
    hud.moveInterfacePointer({40,35});
    hud.interfaceLeftDown();
    check(backPresses==1 && pages.frontend==std::array<bool,5>{false,true,false,false,false} && pages.help,
          "actual retained Back press dispatches event2 and only the selected page changes");
    hud.interfaceLeftUp();
    check(backPresses==1,"Back is not dispatched again on release");
    hud.clearFrontendBindings();
    hud.interfaceLeftDown();hud.interfaceLeftUp();
    check(backPresses==1,"session teardown also removes its retained press callback");

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

    // Six real owner controls, not inferred fixed rectangles or synthetic icons.
    hud.setInterfaceState({{false,false,false,false,false},false,false});
    hud.setInventoryPanelVisibility(false,false,false);
    hud.setGuiScale(1);
    hud.layoutMenuControls({1280,720});
    uint8_t menuByte=0;
    hud.publishMenuControls(menuByte);
    const char *resources[]{"crafting-menu","inventory-menu","worldmap-menu",
                            "system-menu","multiplayer-menu","help-menu"};
    bool completeMenus=true;
    for (int i=0;i<6;++i) {
        const auto *control=hud.menuControl(Hud::Menu(i));
        check(control && control->widget,"owner menubutton is cloned with its own control");
        if (!control || !control->widget) {completeMenus=false;continue;}
        check(control->parent==hud.interfaceBranchRoot(uiroot::Branch::Gameplay),"six nodes belong to the actual Gameplay branch");
        check(!control->visible,"initial menu byte publishes hidden own values");
        check(control->transform.translation+control->transform.pivot==glm::vec2(490+60*i,520),
              "source-literal scale1 layout uses integer screen centre and -200 local Y");
        auto *background=const_cast<plasma::SceneNode *>(control)->find("background");
        const auto *icon=background ? background->find("icon") : nullptr;
        check(icon && icon->doc && icon->source==icon->doc->find(resources[i]),"menu index selects its original owner resource");
        if (!icon || !icon->doc || !control->doc) continue;
        // Exercise the fit entry independently on an unplaced owner template.
        plasma::Scene fit(*control->doc);
        auto *button=fit.instance("menubutton",&fit.root());
        auto *frame=button ? button->find("background") : nullptr;
        if (!frame) {check(false,"fit fixture has its original background");continue;}
        if (auto *previous=frame->find("icon")) fit.destroy(*previous);
        glm::vec2 oldMin,oldMax,newMin,newMax;
        const bool oldBounds=fit.screenBounds(*frame,oldMin,oldMax);
        fit.fitMenuIcon(*button,"background",*icon->doc,resources[i]);
        auto *fitted=frame->find("icon");
        const bool newBounds=fitted && fit.screenBounds(*fitted,newMin,newMax);
        check(oldBounds && newBounds && glm::all(glm::lessThan(glm::abs(oldMin-newMin),glm::vec2(.002f))) &&
              glm::all(glm::lessThan(glm::abs(oldMax-newMax),glm::vec2(.002f))),
              "actual icon fit fills each background extent independently");
        const auto *retained=fitted;
        fit.fitMenuIcon(*button,"background",*icon->doc,"no-such-menu-resource");
        check(frame->find("icon")==retained,"missing resource returns before old-icon removal");
    }
    auto captionControls=gameoptions::defaultControls();
    hud.setInterfaceShown(false); // Neither hidden ancestor nor hidden own nodes gate text.
    hud.updateMenuCaptions(captionControls);
    auto menuText=[&](int index) {
        const auto *node=hud.menuControl(Hud::Menu(index));
        return node ? plasma::captionText(*node) : std::u16string{};
    };
    const std::u16string initial[]{u"C",u"B",u"M",u"O",u"J",u"F1"};
    for (int i=0;i<6;++i) check(menuText(i)==initial[i],"actual owner menu TextShapes get default binding captions while hidden");
    captionControls[gameoptions::CRAFTING_MENU]={1,2};
    captionControls[gameoptions::INVENTORY]={123,0};
    captionControls[gameoptions::WORLD_MAP]={0,0};
    captionControls[gameoptions::SYSTEM_MENU]={84,0};
    captionControls[gameoptions::MULTIPLAYER_MENU]={237,0};
    hud.updateMenuCaptions(captionControls);
    const std::u16string rebound[]{u"ESC",u"NCON",u"?",u"?",u"SEL",u"F1"};
    for (int i=0;i<6;++i) check(menuText(i)==rebound[i],"rebound compact names, misses and fixed Help use their separate paths");
    auto *numberProvider=new CaptionNumber;
    int formats=0;
    bool priorTextObserved=false;
    numberProvider->beforeNumber=[&] {
        ++formats;
        if (formats==1) priorTextObserved=menuText(0)==u"ESC";
        captionControls[gameoptions::INVENTORY]={14,0};
    };
    captionControls[gameoptions::CRAFTING_MENU]={0,1};
    const auto previousLocale=std::locale::global(std::locale(std::locale::classic(),numberProvider));
    hud.updateMenuCaptions(captionControls);
    hud.updateMenuCaptions(captionControls);
    std::locale::global(previousLocale);
    check(priorTextObserved && formats==2 && menuText(0)==u"M1" && menuText(1)==u"BS",
          "formatter runs before its node publication, later bindings are read fresh, and equal updates still format");
    if (const auto *control=hud.menuControl(Hud::Menu::Inventory))
        check(!control->visible,"early caption work does not publish later menu visibility");
    hud.setInterfaceShown(true);
    hud.setGuiScale(2);
    hud.layoutMenuControls({1280,720});
    for (int i=0;i<6;++i) if (const auto *control=hud.menuControl(Hud::Menu(i)))
        check(control->transform.translation+control->transform.pivot==glm::vec2(170+60*i,160),
              "scale2 inverse-projects the screen anchor before adding local offsets");
    hud.setGuiScale(1);
    hud.layoutMenuControls({1281,720});
    if (const auto *control=hud.menuControl(Hud::Menu::Crafting))
        check(control->transform.translation+control->transform.pivot==glm::vec2(490,520),"odd screen width truncates before float conversion");
    hud.layoutMenuControls({-3,-5});
    if (const auto *control=hud.menuControl(Hud::Menu::Crafting))
        check(control->transform.translation+control->transform.pivot==glm::vec2(-151,-205),"signed odd width halves toward zero");
    hud.layoutMenuControls({1280,720});
    MenuInput nativeMenus;
    nativeMenus.rawKeyMessage(M::KeyDown,0x1B);
    nativeMenus.routeInterfaceMessages(hud);
    check(nativeMenus.gameKeyboard.menu==1,"raw Escape toggles independent byte before late publication");
    for (int i=0;i<6;++i) if (const auto *control=hud.menuControl(Hud::Menu(i)))
        check(!control->visible,"raw Escape does not synchronously publish menu nodes");
    hud.setInterfaceShown(false);
    hud.publishMenuControls(nativeMenus.gameKeyboard.menu);
    for (int i=0;i<6;++i) if (const auto *control=hud.menuControl(Hud::Menu(i)))
        check(control->visible,"late publication updates own values even when F4 hides their ancestor");
    hud.setInterfaceState({{false,false,true,false,false},false,false});
    hud.publishMenuControls(nativeMenus.gameKeyboard.menu);
    check(!hud.interfaceBranchRoot(uiroot::Branch::Gameplay)->visible,"frontend page independently hides gameplay ancestor");
    hud.setInterfaceState({{false,false,false,false,false},false,false});
    hud.setInterfaceShown(true);
    std::vector<int> menuPresses;
    hud.bindMenuActions([&](Hud::Menu which){menuPresses.push_back(int(which));});
    if (completeMenus) for (int i=0;i<6;++i) {
        const auto *control=hud.menuControl(Hud::Menu(i));
        plasma::Scene geometry(*control->doc);
        const auto point=geometry.contentPosition(*control)+geometry.screenSize(*control)*.5f;
        hud.moveInterfacePointer(point);
        hud.interfaceLeftDown();
        check(menuPresses.size()==size_t(i+1) && menuPresses.back()==i,"each actual owner shape dispatches its own event2 callback in native index order");
        hud.interfaceLeftUp();
        check(menuPresses.size()==size_t(i+1),"menu button release does not repeat its press callback");
        check(nativeMenus.gameKeyboard.menu==1,"menu callback does not clear Escape byte");
    }
    hud.bindMenuActions({});
    if (completeMenus) {
        const auto *control=hud.menuControl(Hud::Menu::Help);
        plasma::Scene geometry(*control->doc);
        const auto point=geometry.contentPosition(*control)+geometry.screenSize(*control)*.5f;
        hud.moveInterfacePointer(point);hud.interfaceLeftDown();hud.interfaceLeftUp();
        check(menuPresses.size()==6,"owner teardown removes the borrowed gameplay action callback");
    }
    nativeMenus.rawKeyMessage(M::KeyDown,0x1B);
    nativeMenus.routeInterfaceMessages(hud);
    hud.publishMenuControls(nativeMenus.gameKeyboard.menu);
    check(nativeMenus.gameKeyboard.menu==0,"repeated Escape toggles off without navigation or dismissal");
    for (int i=0;i<6;++i) if (const auto *control=hud.menuControl(Hud::Menu(i)))
        check(!control->visible,"later zero byte hides all six own values");


    if (completeMenus) {
        uint8_t visible=1;
        hud.publishMenuControls(visible);
        const auto *control=hud.menuControl(Hud::Menu::Help);
        plasma::Scene geometry(*control->doc);
        const auto point=geometry.contentPosition(*control)+geometry.screenSize(*control)*.5f;
        MenuInput right;
        int dismisses=0;
        right.gameKeyboard.menu=1;
        right.gameDismissPanels=[&]{++dismisses;};
        hud.moveInterfacePointer(point);
        right.routeInterfacePress(hud,1);
        check(right.gameKeyboard.menu==1 && dismisses==0,"ordinary post-GUI owner hit prevents right-press dismissal");
        hud.interfaceSecondaryButton(1,false);
        Hud::ScreenItem captured;
        captured.kind=Hud::ScreenItem::Template;captured.name="button";captured.instance="capture-fixture";
        captured.branch=uiroot::Branch::Root;captured.at={600,100};captured.size={100,30};captured.text=L"Capture";
        captured.press=[]{};
        hud.publishFrontendScreen({captured});
        hud.moveInterfacePointer({650,115});
        hud.interfaceLeftDown();
        hud.setInterfaceShown(false);
        hud.moveInterfacePointer({1200,20});
        right.routeInterfacePress(hud,1);
        check(right.gameKeyboard.menu==1 && dismisses==0,"captured receiver prevents dismissal even outside and under hidden root");
        hud.interfaceSecondaryButton(1,false);
        hud.interfaceLeftUp();
        auto *gameplay=const_cast<plasma::SceneNode *>(hud.interfaceBranchRoot(uiroot::Branch::Gameplay));
        std::vector<plasma::SceneNode *> panels;
        for (auto &child:gameplay->children)
            if (child->widget && child->widget->kind=="InventoryWidget") {
                child->visible=true;panels.push_back(child.get());
            }
        auto *character=gameplay->find("character");
        if (character) character->visible=true;
        right.routeInterfacePress(hud,1);
        check(right.gameKeyboard.menu==0 && dismisses==1,"after override release the fresh hidden-root miss clears menu byte");
        check(panels.size()==3 && !panels[0]->visible && !panels[1]->visible && !panels[2]->visible,
              "null-target branch clears the three current widget nodes");
        check(character && character->visible,"interface press does not broaden three node stores into unrelated siblings");
        hud.interfaceSecondaryButton(1,false);
        hud.setInterfaceShown(true);
        hud.clearFrontendBindings();
    }

    // Real owner GUI callbacks replace the current Game port during a press.
    // Mouse retains its entry Game, whereas the keyboard test fetches after GUI.
    {
        hud.setInterfaceState({});hud.setInterfaceShown(true);hud.clearFrontendBindings();
        struct Owner { gamemouse::State mouse; bool widget=false,chat=true; int items=0; } first,second;
        MenuInput messages;
        const auto ports=[&](Owner &owner) {
            auto *g=&owner; gamemouse::Target t;
            t.state=&g->mouse;
            t.widgetSelected=[g] {return g->widget;};t.mapShown=[] {return false;};
            t.mapArrow=[](bool) {return false;};t.coordinatePress=[](int) {};
            t.clock=[] {return uint32_t(99);};t.cursorHeight=[] {return int64_t(0);};
            t.selectTarget=[] {};t.resetMovement=[] {};t.maximumHealth=[] {return 100.f;};
            t.inventoryPress=[g](int) {++g->items;};t.secondaryPress=[](int) {};t.adjustHealth=[](float) {};
            t.closeChat=[g] {g->chat=false;};t.hideSelector=[&] {hud.setRingShown(false);};
            t.releasePrefix=[](int) {};t.selectedTextRelease=[](int) {};t.widgetRelease=[](int) {};
            t.coordinateRelease=[](int) {};t.itemRelease=[](int) {};
            return t;
        };
        messages.gameMouseTarget=[&] {return ports(first);};
        hud.bindMenuActions([&](Hud::Menu) {
            first.widget=true;
            messages.gameMouseTarget=[&] {return ports(second);};
        });
        uint8_t shown=1;hud.publishMenuControls(shown);hud.layoutMenuControls({1280,720});
        const auto *node=hud.menuControl(Hud::Menu::Help);
        const auto *selector=hud.ringRoot();
        check(selector && selector->parent==hud.interfaceBranchRoot(uiroot::Branch::Gameplay),
              "direct selector is retained under Gameplay independently of crafting's frame");
        if (node) {
            plasma::Scene geometry(*node->doc);
            const auto point=geometry.contentPosition(*node)+geometry.screenSize(*node)*.5f;
            hud.setRingShown(true);hud.moveInterfacePointer(point);
            messages.routeInterfacePress(hud,0);
            check(first.items==1 && second.items==0 && !first.chat && second.chat && messages.gameMouse.widgetAtPress==1,
                  "post-GUI Game mouse keeps entry owner and snapshots that owner's current widget");
            check(!hud.ringShown() && hud.ringRoot()==selector,"press hides direct selector without reconstructing it");
            messages.routeInterfaceRelease(hud,0);
            messages.routeInterfacePress(hud,3);
            check(!second.chat && second.items==0 && messages.gameMouse.widgetAtPress==0,
                  "next press fetches replacement owner; unknown button keeps tail without item callback");
        }
        hud.bindMenuActions({});hud.publishMenuControls(uint8_t(0));
    }
    std::cout<<"Live owner scene edit: "<<(failures ? "FAIL" : "PASS")<<'\n';
    return failures;
}
