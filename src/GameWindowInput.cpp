#include "GameWindowInput.h"
#include "GameOptions.h"

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
