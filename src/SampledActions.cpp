#include "SampledActions.h"
#include "GameOptions.h"
#include "GameMath.h"

namespace sampledactions {
bool lateMenus(Process &process, const Menus &game) {
    if (game.chatEditing() || game.pageShown(0) || game.pageShown(1) || game.pageShown(2))
        return false;
    static constexpr std::array<int,6> order{gameoptions::WORLD_MAP,gameoptions::INVENTORY,
        gameoptions::CRAFTING_MENU,gameoptions::RING_MENU,gameoptions::MULTIPLAYER_MENU,gameoptions::SYSTEM_MENU};
    for (size_t slot=0;slot<order.size();++slot) {
        const int action=order[slot];
        if (game.level(action)!=0 && process.menus[slot]==0)
            game.invoke(action);
        process.menus[slot]=game.level(action); // Fresh byte AFTER the synchronous call.
        if (slot==1) game.mirrorEquipment();
        if (slot==2) game.mirrorDetails();
    }
    return true;
}

void earlyZoom(Process &process,const Level &level,int32_t &index,const std::function<int32_t()> &count) {
    static constexpr std::array<int,2> order{gameoptions::ZOOM_IN,gameoptions::ZOOM_OUT};
    for (size_t slot=0;slot<order.size();++slot) {
        if (level(order[slot])!=0 && process.zoom[slot]==0) {
            index=gamemath::add32(index,slot==0 ? -1 : 1);
            if (index<0) index=0;
            const int32_t available=count();
            if (index>=available) index=gamemath::add32(available,-1);
        }
        process.zoom[slot]=level(order[slot]);
    }
}
} // namespace sampledactions
