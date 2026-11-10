#include "SampledActions.h"
#include "GameOptions.h"
#include <algorithm>
#include <array>
#include <climits>
#include <iostream>
#include <string>
#include <vector>

int sampledActionTests() {
    using namespace sampledactions;
    int failures=0;
    const auto check=[&](bool ok,const char *what) {
        if (!ok) {std::cerr<<"FAIL sampled actions: "<<what<<'\n';++failures;}
    };
    Process process;
    std::array<uint8_t,23> levels{};
    bool chat=true;
    std::array<bool,3> pages{};
    std::vector<std::string> trace;
    std::function<void(int)> action;
    Menus game;
    game.chatEditing=[&] {trace.push_back("chat");return chat;};
    game.pageShown=[&](int page) {trace.push_back("page"+std::to_string(page));return pages[size_t(page)];};
    game.level=[&](int key) {return levels[size_t(key)];};
    game.invoke=[&](int key) {trace.push_back(std::to_string(key));if (action) action(key);};
    game.mirrorEquipment=[&] {trace.push_back("equipment");};
    game.mirrorDetails=[&] {trace.push_back("details");};
    levels[15]=0x80;levels[17]=1;
    check(!lateMenus(process,game) && trace==std::vector<std::string>{"chat"} &&
          process.menus==std::array<uint8_t,6>{},"Chat skips level tests, mirrors and every menu latch");
    chat=false;pages[1]=true;trace.clear();
    check(!lateMenus(process,game) && trace==std::vector<std::string>{"chat","page0","page1"},
          "three separate page roots short circuit before the menu block");
    pages={};for (int key:{15,17,18,16,20,19}) levels[size_t(key)]=1;
    trace.clear();action=[&](int key) {if (key==15) {levels[15]=2;levels[19]=0x80;chat=true;pages[0]=true;}};
    check(lateMenus(process,game) && trace==std::vector<std::string>{"chat","page0","page1","page2",
          "15","17","equipment","18","details","16","20","19"},
          "one outer admission precedes all six callbacks and their two ordered mirror joins");
    check(process.menus==std::array<uint8_t,6>{2,1,1,1,1,0x80},
          "post-callback stores preserve freshly read byte values without canonicalizing");
    levels={};trace.clear();
    check(!lateMenus(process,game) && process.menus[0]==2,"a zero level in a skipped block is not a reached release");
    chat=false;pages={};for (int key:{15,17,18,16,20,19}) levels[size_t(key)]=1;
    action={};trace.clear();lateMenus(process,game);
    check(trace==std::vector<std::string>{"chat","page0","page1","page2","equipment","details"},
          "held levels after skipped releases do not manufacture shared producer edges");
    levels={};lateMenus(process,game);
    check(process.menus==std::array<uint8_t,6>{},"an admitted zero visit releases each independent menu latch");
    levels[15]=1;trace.clear();action=[&](int key) {if (key==15) levels[17]=0x80;};
    lateMenus(process,game);
    check(std::find(trace.begin(),trace.end(),"17")!=trace.end() && process.menus[1]==0x80,
          "earlier callback can change the later level read in the same visit");
    Process zoom;
    int32_t index=0,count=7;
    levels={};levels[21]=0x80;levels[22]=1;
    const auto getLevel=[&](int key) {return levels[size_t(key)];};
    earlyZoom(zoom,getLevel,index,[&] {return count;});
    check(index==1 && zoom.zoom==std::array<uint8_t,2>{0x80,1},"zoom in precedes out, each retains its own current byte");
    earlyZoom(zoom,getLevel,index,[&] {return count;});
    check(index==1,"another Game visit with held levels does not create fresh zoom edges");
    levels={};earlyZoom(zoom,getLevel,index,[&] {return count;});levels[21]=1;count=0;index=9;
    earlyZoom(zoom,getLevel,index,[&] {return count;});
    check(index==-1,"zero range applies native upper correction after lower clamp");
    levels={};earlyZoom(zoom,getLevel,index,[&] {return count;});count=7;index=INT_MIN;levels[21]=1;
    earlyZoom(zoom,getLevel,index,[&] {return count;});
    check(index==6,"decrement wraps before sign and upper comparisons");
    levels={};earlyZoom(zoom,getLevel,index,[&] {return count;});index=INT_MAX;levels[22]=1;
    earlyZoom(zoom,getLevel,index,[&] {return count;});
    check(index==0,"increment wrap reaches the lower correction before the upper test");
    if (!failures) std::cout<<"Sampled actions: PASS per-consumer admission/fresh stores/source order/wrapping ranges\n";
    return failures;
}
