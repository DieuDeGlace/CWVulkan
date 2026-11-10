#include "GameWindowInput.h"
#include "ChatLine.h"
#include "Frontend.h"
#include <bit>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "PlasmaEdit.h"
#include "PlasmaInput.h"
#include <iostream>

int gameWindowInputTests() {
    int failures=0;
    const auto check=[&](bool value,const char *message) {
        if (!value) {std::cerr<<"FAIL Game window keyboard: "<<message<<'\n';++failures;}
    };
    using Key=gamewindow::Key;
    gamewindow::Keyboard first,second;
    bool useSecond=false;
    std::vector<std::string> order;
    uint16_t guiUnit=0;
    const auto current=[&] {
        order.push_back("reload");
        gamewindow::Target target;
        target.keyboard=useSecond ? &second : &first;
        target.special=[&](uint8_t key) {
            check((useSecond ? second : first).raw[key]==1,"raw byte precedes the F-key effect");
            order.push_back("special");
        };
        target.down=[&](uint8_t){order.push_back("down");};
        target.character=[&](uint8_t key){order.push_back("char"+std::to_string(key));};
        return target;
    };
    const auto gui=[&](Key,uint16_t unit) {guiUnit=unit;order.push_back("gui");useSecond=true;};
    gamewindow::dispatch(Key::Down,0x12340170,gui,current);
    check(guiUnit==0x0170 && first.raw[0x70]==0 && second.raw[0x70]==1,
          "GUI gets low16 and can replace the current Game before its low8 key write");
    check(order==std::vector<std::string>{"gui","reload","special","down"},"F-key phases preserve synchronous call order");
    order.clear();
    gamewindow::dispatch(Key::Up,0xABC00170,gui,current);
    check(second.raw[0x70]==0 && order==std::vector<std::string>{"gui","reload"},"keyup clears only raw state after GUI delivery");
    second.menu=0;
    gamewindow::dispatch(Key::Down,0x1B,{},current);
    check(second.menu==1 && second.raw[0x1B]==1,"first Escape toggles only its independent menu byte");
    gamewindow::dispatch(Key::Down,0x1B,{},current);
    check(second.menu==0 && second.raw[0x1B]==1,"repeated Escape is delivered and toggles again while held");
    second.menu=255;
    gamewindow::dispatch(Key::Down,0x1B,{},current);
    check(second.menu==0,"nonzero menu byte toggles to normalized zero");
    const auto rawBefore=second.raw;
    order.clear();
    gamewindow::dispatch(Key::Character,0x123400E9,gui,current);
    check(guiUnit==0xE9 && second.raw==rawBefore && order==std::vector<std::string>{"gui","reload","char233"},
          "character delivers distinct GUI16/Game8 values without changing raw keys");

    check(std::bit_cast<uint32_t>(gameoptions::GUI_SCALE_MIN)==0x3ED1B716u &&
          std::bit_cast<uint32_t>(gameoptions::GUI_SCALE_MAX)==0x40435000u &&
          std::bit_cast<uint32_t>(gameoptions::MAP_SCALE_MIN)==0x3F40562Au &&
          std::bit_cast<uint32_t>(gameoptions::MAP_SCALE_MAX)==0x3FAA5E36u,"scale bounds retain the original four binary32 words");
    check(gamewindow::guiScale(1,false)==.8f && gamewindow::guiScale(1,true)==1.25f,
          "GUI scale uses the native divide/multiply direction");
    check(gamewindow::guiScale(0,false)==gameoptions::GUI_SCALE_MIN &&
          gamewindow::guiScale(100,true)==gameoptions::GUI_SCALE_MAX,"native GUI directional clamps");
    check(gamewindow::guiScale(100,false)==80 && gamewindow::guiScale(.125f,true)==.15625f,
          "GUI keys do not add the unselected opposite-side clamp");
    check(gamewindow::mapScale(0,false)==gameoptions::MAP_SCALE_MIN &&
          gamewindow::mapScale(100,true)==gameoptions::MAP_SCALE_MAX &&
          gamewindow::mapScale(.5f,true)==.55f,"map scale keeps its directional admission");

    Hud routedHud;
    MenuInput ingress;
    using Message=MenuInput::InterfaceMessage;
    std::vector<bool> helpStates;
    ingress.gameKeyDown=[&](uint8_t key) {if (key==0x70) helpStates.push_back(routedHud.helpShown());};
    ingress.rawKeyMessage(Message::KeyDown,0x70);
    ingress.keyMessage(GLFW_KEY_F1,false); // the corresponding GLFW notification must not duplicate it
    ingress.rawKeyMessage(Message::KeyDown,0x70);
    ingress.keyMessage(GLFW_KEY_F1,true);
    ingress.routeInterfaceMessages(routedHud);
    check(helpStates==std::vector<bool>{true,false} && !routedHud.helpShown() && ingress.gameKeyboard.raw[0x70]==1,
          "actual shared message queue applies repeated F1 once each and Game runs after its effect");
    routedHud.setInterfaceShown(false);
    float guiScale=1;
    int specialCount=0;
    ingress.rawKeyMessage(Message::KeyDown,0x71);
    ingress.rawKeyMessage(Message::KeyDown,0x71);
    ingress.routeInterfaceMessages(routedHud,{},[&](uint8_t key) {
        ++specialCount;
        check(key==0x71 && ingress.gameKeyboard.raw[key]==1,"actual queue publishes raw state before the scale callback");
        guiScale=gamewindow::guiScale(guiScale,false);
    });
    check(specialCount==2 && guiScale==.64f && !routedHud.interfaceShown(),
          "repeated F2 has two synchronous effects independent of hidden root");
    ingress.rawKeyMessage(Message::KeyDown,0x1B);
    ingress.rawKeyMessage(Message::KeyDown,0x1B);
    ingress.rawKeyMessage(Message::KeyUp,0x1B);
    ingress.routeInterfaceMessages(routedHud);
    ingress.clear();
    check(ingress.gameKeyboard.menu==0 && ingress.gameKeyboard.raw[0x1B]==0 && ingress.gameKeyboard.raw[0x70]==1,
          "frame clearing does not clear unrelated native Game raw key state");

    ChatLine chat;
    const auto initialRevision=chat.drawingRevision();
    chat.keyDown(0x41,false);
    check(!chat.typing() && chat.drawingRevision()==initialRevision,"inactive non-Return key does no chat work");
    chat.keyDown(0x0D,false);
    check(chat.typing() && chat.drawingRevision()==initialRevision,"inactive Return activates without forwarding to chat editing");
    chat.type("ABC");
    chat.keyDown(0x24,true);
    chat.keyDown(0x25,false);
    check(chat.caretIndex()==0 && chat.selectedUnits()==3,"Left at zero preserves a positive selection even without Shift");
    chat.keyDown(0x25,true);
    check(chat.caretIndex()==0 && chat.selectedUnits()==3,"Shift-Left at zero does not extend selection");
    chat.keyDown(0x27,false);
    chat.keyDown(0x23,true);
    chat.keyDown(0x27,false);
    check(chat.caretIndex()==3 && chat.selectedUnits()==-2,"Right at length preserves a negative selection");
    chat.keyDown(0x27,true);
    check(chat.caretIndex()==3 && chat.selectedUnits()==-2,"Shift-Right at length does not extend selection");
    chat.keyDown(0x2E,false);
    check(chat.text()==u"A" && chat.caretIndex()==1 && chat.selectedUnits()==0,"selected Delete uses the ordered bounded span");
    const auto beforeDelete=chat.drawingRevision();
    chat.keyDown(0x2E,false);
    check(chat.text()==u"A" && chat.drawingRevision()!=beforeDelete,"Delete at end still reaches invalidation");
    chat.type("B");
    bool observedLiveText=false;
    chat.keyDown(0x0D,false,[&](const std::u16string &submitted) {
        observedLiveText=submitted==u"AB" && chat.text()==u"AB" && !chat.typing();
        chat.keyDown(0x0D,false); // source-conditional normally returning reentry
        chat.type("Z");
    });
    check(observedLiveText && chat.typing() && chat.text().empty() && chat.caretIndex()==0 && chat.selectedUnits()==0,
          "active Return submits before current-text clear and later clamp, preserving a callback's reopened active byte");

    // Actual GUI focus/edit and Game chat admission, including rereads in one message.
    plx::Document document;
    plasma::Scene scene(document);
    auto *node=scene.create(&scene.root(),"editor");
    node->widget.emplace();node->widgetOwner=node;
    plasma::Input input(scene,{});
    std::u16string caption;
    bool clearOnChange=false;
    plasma::Edit::Caption ports;
    ports.read=[&]{return caption;};
    ports.write=[&](const std::u16string &text){caption=text;};
    ports.clearFocus=[&]{input.focus(nullptr);};
    ports.changed=[&]{if (clearOnChange) input.focus(nullptr);};
    plasma::Edit editor(ports);
    input.keyboardHook=[&](plasma::Input &,plasma::SceneNode &,plasma::Input::Key kind,uint16_t key) {
        if (kind==plasma::Input::Key::Down) editor.keyDown(key,input.keyHeld(0x10));
        else if (kind==plasma::Input::Key::Character) editor.character(key);
    };
    ChatLine actual;
    gamewindow::Keyboard actualKeys;
    bool gameplaySelected=true;
    int captureIndex=-1;
    const auto actualGui=[&](Key kind,uint16_t key) {
        if (kind==Key::Down) input.keyDown(key);
        else if (kind==Key::Up) input.keyUp(key);
        else input.character(key);
    };
    const auto actualGame=[&] {
        gamewindow::Target target;
        target.keyboard=&actualKeys;
        target.down=[&](uint8_t key) {
            if (gameplaySelected && !input.focusWidget() && captureIndex<0) actual.keyDown(key,input.keyHeld(0x10));
        };
        target.character=[&](uint8_t byte) {
            if (!input.focusWidget() && actual.typing()) actual.type(std::string(1,char(byte)));
        };
        return target;
    };
    input.focus(node);
    gamewindow::dispatch(Key::Down,0x0D,actualGui,actualGame);
    check(!input.focusWidget() && actual.typing(),"GUI Enter blur precedes the Game phase's current-focus test in the same message");
    input.focus(node);
    gamewindow::dispatch(Key::Character,0x00E9,actualGui,actualGame);
    check(caption==u"\u00E9" && actual.text().empty(),"nonnull GUI focus receives the original unit and suppresses Game chat insertion");
    clearOnChange=true;
    captureIndex=5;gameplaySelected=false; // character has neither of these keydown gates
    gamewindow::dispatch(Key::Character,0x00E9,actualGui,actualGame);
    check(caption==u"\u00E9\u00E9" && actual.text()==u"\uFFE9" && !input.focusWidget(),
          "GUI callback blur is reread; Game low byte sign-extends while character ignores page/capture-index gates");
    actual.keyDown(0x0D,false);
    gamewindow::dispatch(Key::Down,0x0D,actualGui,actualGame);
    check(!actual.typing(),"Game keydown still requires its own page and capture-index admissions");
    gameplaySelected=true;captureIndex=-1;scene.root().visible=false;
    gamewindow::dispatch(Key::Down,0x0D,actualGui,actualGame);
    check(actual.typing(),"hidden GUI ancestor is not a Game chat key gate when its own gameplay value remains selected");

    MenuInput rightPress;
    Hud emptyHud;
    std::vector<std::string> dismissOrder;
    rightPress.gameKeyboard.menu=255;
    rightPress.gameDismissPanels=[&] {
        check(rightPress.gameKeyboard.menu==0,"right-press null-target clear precedes the borrowed panel adapter");
        dismissOrder.push_back("dismiss");
    };
    rightPress.rawKeyMessage(Message::KeyDown,0x1B); // toggles off; set again before actual right phase
    rightPress.routeInterfaceMessages(emptyHud);
    rightPress.gameKeyboard.menu=1;
    rightPress.interfaceMessages.emplace_back(Message::MiddleDown);
    rightPress.routeInterfaceMessages(emptyHud);
    check(rightPress.gameKeyboard.menu==1 && dismissOrder.empty(),"middle press has no right-press dismissal effect");
    rightPress.interfaceMessages.emplace_back(Message::RightDown);
    rightPress.routeInterfaceMessages(emptyHud);
    check(rightPress.gameKeyboard.menu==0 && dismissOrder==std::vector<std::string>{"dismiss"},
          "right press on a fresh null target clears the separate menu byte");


    // Actual retained queue: presses and text run in arrival order, rather than
    // clearing chat later from a second frame-based mouse list.
    MenuInput mixed;
    Hud noEditors;
    ChatLine mixedChat;
    std::vector<std::u16string> submitted;
    mixed.gamePress=[&](int) {mixedChat.stopEditing();return true;};
    mixed.gameKeyDown=[&](uint8_t key) {
        mixedChat.keyDown(key,noEditors.interfaceKeyHeld(0x10),[&](const auto &text){submitted.push_back(text);});
    };
    mixed.gameCharacter=[&](uint8_t unit){mixedChat.type(std::string(1,char(unit)));};
    mixed.interfaceMessages.emplace_back(Message::LeftDown);
    mixed.rawKeyMessage(Message::KeyDown,0x0D);
    mixed.rawKeyMessage(Message::Character,'A');
    mixed.routeInterfaceMessages(noEditors);
    check(mixedChat.typing() && mixedChat.text()==u"A" && mixed.gamePresses==std::vector<int>{0},
          "mouse-down then Enter in one burst leaves later chat open");
    mixed.rawKeyMessage(Message::KeyDown,0x0D);
    mixed.routeInterfaceMessages(noEditors);
    check(!mixedChat.typing() && submitted==std::vector<std::u16string>{u"A"},"later Return submits that retained current text");
    mixed.rawKeyMessage(Message::KeyDown,0x0D);
    mixed.rawKeyMessage(Message::Character,'B');
    mixed.interfaceMessages.emplace_back(Message::MiddleDown);
    mixed.rawKeyMessage(Message::KeyDown,0x0D);
    mixed.routeInterfaceMessages(noEditors);
    check(mixedChat.typing() && mixedChat.text()==u"B" && submitted.size()==1,
          "normal press stops editing without clearing text, so a later Enter reopens it");

    std::cout<<"Game window keyboard phases: "<<(failures ? "FAIL" : "PASS")<<'\n';
    return failures;
}
