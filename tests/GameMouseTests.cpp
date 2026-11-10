#include "GameMouse.h"
#include <algorithm>
#include <bit>
#include <iostream>
#include <string>
#include <vector>

int gameMouseTests() {
    using namespace gamemouse;
    int failures = 0;
    const auto check = [&](bool value,const char *why) {
        if (!value) {std::cerr << "FAIL Game mouse: " << why << '\n'; ++failures;}
    };
    Process process;
    struct Game {
        State input;
        uint8_t moved = 0;
        bool widget = false, map = false, chat = true, selector = true;
        bool firstArrow = false, secondArrow = false;
        int64_t height = 1;
        uint32_t y = 0x80000000u, now = 100;
        float maximum = 100, hp = 50;
        int gui = 0, targets = 0, items = 0, secondary = 0, suffix = 0;
    } first, second;
    first.moved = second.moved = 1;
    bool useSecond = false;
    int factories = 0;
    std::vector<std::string> trace;
    std::function<void()> inGui, inClock, inCoordinates, inItems;
    const auto ports = [&](Game &owner) {
        Game *g = &owner;
        Target t;
        t.state = &g->input;
        t.guiY = [&,g] {trace.push_back("Y");return g->y;};
        t.guiPress = [&,g](int32_t) {++g->gui;trace.push_back("GUI");if (inGui) inGui();};
        t.widgetSelected = [&,g] {trace.push_back("widget");return g->widget;};
        t.mapShown = [&,g] {trace.push_back("map");return g->map;};
        t.mapArrow = [&,g](bool a) {trace.push_back(a ? "A" : "B");return a ? g->firstArrow : g->secondArrow;};
        t.coordinatePress = [&,g](int32_t) {trace.push_back("coordinates");if (inCoordinates) inCoordinates();};
        t.clock = [&,g] {trace.push_back("clock");if (inClock) inClock();return g->now;};
        t.cursorHeight = [&,g] {trace.push_back("height");return g->height;};
        t.selectTarget = [&,g] {++g->targets;check(process.pressedAt == g->now,"clock publishes before target action");};
        t.resetMovement = [g] {g->moved=0;};
        t.maximumHealth = [&,g] {check(g->moved==0,"movement flag clears before A0");
            check(g->chat,"chat remains open during initial scalar");trace.push_back("A0");return g->maximum;};
        t.inventoryPress = [&,g](int32_t) {++g->items;trace.push_back("items");if (inItems) inItems();};
        t.secondaryPress = [&,g](int32_t) {++g->secondary;trace.push_back("secondary");};
        t.adjustHealth = [&,g](float before) {check(g->chat && g->selector,"scalar postlude precedes Chat/selector clear");
            g->hp *= g->maximum / before; trace.push_back("health");};
        t.closeChat = [&,g] {g->chat=false;trace.push_back("chat");};
        t.hideSelector = [&,g] {check(!g->chat,"Chat clear precedes selector hide");g->selector=false;trace.push_back("selector");};
        t.releasePrefix = [&](int32_t) {trace.push_back("prefix");};
        t.guiRelease = [&](int32_t) {trace.push_back("release GUI");if (inGui) inGui();};
        t.selectedTextRelease = [&](int32_t) {trace.push_back("text");};
        t.widgetRelease = [&](int32_t) {trace.push_back("widget action");};
        t.coordinateRelease = [&](int32_t) {trace.push_back("release coordinates");};
        t.itemRelease = [&](int32_t) {trace.push_back("release item");};
        t.controlsRelease = [&,g](int32_t) {++g->suffix;trace.push_back("controls");};
        return t;
    };
    const Current current = [&] {++factories;return ports(useSecond ? second : first);};
    inGui = [&] {useSecond=true;first.widget=true;};
    inItems = [&] {first.maximum=200;};
    check(press(process,current,0),"ordinary press admitted");
    check(factories==1 && first.gui==1 && second.gui==0 && first.items==1 && second.items==0 &&
          first.input.cachedY==0x80000000u && second.input.cachedY==0,
          "raw cached word and entry Game retained through GUI replacement");
    check(process.widgetAtPress==1 && first.hp==100 && !first.chat && !first.selector && first.secondary==1,
          "post-GUI snapshot, inventory change, scalar and retained-widget postlude");
    check(trace==std::vector<std::string>{"Y","GUI","widget","map","coordinates","clock","map","A0","items","secondary","health","chat","selector"},
          "ordinary owner phases remain synchronous");
    useSecond=false;inGui={};inItems={};trace.clear();
    first.chat=first.selector=true;first.map=true;first.firstArrow=true;first.moved=1;
    const auto previousTick=process.pressedAt;const auto hp=first.hp;
    first.widget=false;
    check(!press(process,current,7) && process.widgetAtPress==0 && process.pressedAt==previousTick &&
          first.chat && first.selector && first.moved==1 && first.hp==hp &&
          trace==std::vector<std::string>{"Y","widget","map","A"},
          "first arrow skips B/clock/scalar/tail but retains Y/GUI/snapshot");
    first.firstArrow=false;first.secondArrow=true;trace.clear();
    check(!press(process,current,-1) && trace.back()=="B","second arrow is the other ordinary early return");
    first.secondArrow=false;first.now=0x20;process.pressedAt=0xFFFFFFF0;trace.clear();
    const int oldGui=first.gui, oldItems=first.items;
    check(press(process,current,3) && first.targets==1 && process.pressedAt==0x20 && first.secondary==1 &&
          first.gui==oldGui && first.items==oldItems,
          "wrapped elapsed passes and unknown button still runs common tail without left-only secondary");
    first.chat=first.selector=true;first.now=232;trace.clear();
    check(press(process,current,1) && first.targets==1 && process.pressedAt==232,"exact200 rejected but clock updated");
    first.chat=first.selector=true;first.now=100;process.pressedAt=0;first.height=0;
    check(press(process,current,1) && first.targets==1,"zero target height fails strict positive gate");
    first.chat=first.selector=true;first.height=1;first.now=250;process.pressedAt=0;
    inClock=[&] {process.pressedAt=100;};
    check(press(process,current,1) && first.targets==2,"old process tick is reread after the clock provider");
    inClock={};first.chat=first.selector=true;first.map=true;
    inCoordinates=[&] {first.map=false;};
    check(press(process,current,0) && first.targets==2,"preclock action can change the later map gate");
    inCoordinates={};first.chat=first.selector=true;first.map=false;trace.clear();
    inGui=[&] {process.widgetAtPress=1;first.widget=true;};
    release(process,current,0);
    check(trace==std::vector<std::string>{"prefix","release GUI","text","widget","widget action","release coordinates","release item","controls"},
          "release prefix precedes GUI and live snapshot admission follows text work");
    trace.clear();inGui=[&] {first.widget=false;};
    release(process,current,0);
    check(process.widgetAtPress==1 && first.suffix==2 &&
          std::find(trace.begin(),trace.end(),"widget action")==trace.end(),
          "GUI-cleared current widget fails action without clearing process snapshot or controls suffix");
    trace.clear();inGui={};first.widget=true;
    release(process,current,-7);
    check(first.suffix==3 && std::find(trace.begin(),trace.end(),"release GUI")==trace.end() &&
          std::find(trace.begin(),trace.end(),"widget action")==trace.end(),
          "unknown release still completes controls without press health/Chat/selector work");
    if (!failures) std::cout << "Game mouse: PASS retained receiver/clock/snapshot/ordered phase checks\n";
    return failures;
}
