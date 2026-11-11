#include "GameWindowInput.h"
#include "GameOptions.h"
#include "ChatLine.h"

namespace gamewindow {
float guiScale(float current,bool increase) {
    float next=increase ? current*1.25f : current/1.25f;
    if (increase) {if (next>gameoptions::GUI_SCALE_MAX) next=gameoptions::GUI_SCALE_MAX;}
    else if (next<gameoptions::GUI_SCALE_MIN) next=gameoptions::GUI_SCALE_MIN;
    return next;
}
float mapScale(float current,bool increase) {
    float next=increase ? current*1.1f : current/1.1f;
    if (increase) {if (next>gameoptions::MAP_SCALE_MAX) next=gameoptions::MAP_SCALE_MAX;}
    else if (next<gameoptions::MAP_SCALE_MIN) next=gameoptions::MAP_SCALE_MIN;
    return next;
}
bool chatKeyDown(const Chat &source,uint8_t key) {
    if (!source.selected || !source.selected()) return false;
    if (source.focused && source.focused()) return false;
    if (source.capture && source.capture()>=0) return false;
    auto *chat=source.current ? source.current() : nullptr;
    if (!chat) return false; // Owned invalid-graph policy.
    const bool consumed=key==0x0d || (chat->typing() && key!=0x1b);
    chat->keyDownWithShift(key,source.shift,source.submit);
    return consumed;
}
void chatCharacter(const Chat &source,uint8_t unit) {
    if (source.focused && source.focused()) return;
    if (auto *chat=source.current ? source.current() : nullptr) chat->type(std::string(1,char(unit)));
}
void dispatch(Key kind,uint64_t parameter,const Gui &gui,const Current &current) {
    if (gui) gui(kind,uint16_t(parameter));
    // Reload after the GUI callback. A GUI receiver can change the current Game.
    const auto game=current ? current() : Target{};
    if (!game.keyboard) return; // defined engine policy outside ordinary valid receivers
    const uint8_t key=uint8_t(parameter);
    if (kind==Key::Down) {
        game.keyboard->raw[key]=1;
        if (key==0x1B) game.keyboard->menu=uint8_t(game.keyboard->menu==0);
        else if (key>=0x70 && key<=0x75 && game.special) game.special(key);
        if (game.down) game.down(key);
    } else if (kind==Key::Up) {
        game.keyboard->raw[key]=0;
    } else if (game.character) game.character(key);
}
}
