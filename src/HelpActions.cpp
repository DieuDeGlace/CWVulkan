#include "HelpActions.h"
#include "NativeBuffs.h"
#include <bit>
#include <cmath>
#include <limits>

namespace helpactions {
namespace {
constexpr glm::vec4 bindingColour{.25f,1,1,1},white{1},staminaColour{.6f,1,1,1},
    manaColour{.6f,.6f,1,1},chargeColour{1,.5f,1,1};
float afterAnnotation(float firstFlow) {
    // Native truncates to signed DWORD, adds twenty at DWORD width, then converts.
    // Invalid conversion has no proved process exception policy. Our finite-safe
    // adapter chooses the masked x86 indefinite value instead of C++ undefined behavior.
    int32_t truncated=std::numeric_limits<int32_t>::min();
    if (std::isfinite(firstFlow) && firstFlow>=-2147483648.f && firstFlow<2147483648.f)
        truncated=static_cast<int32_t>(firstFlow);
    return float(std::bit_cast<int32_t>(uint32_t(truncated)+20u));
}
bool held(int32_t action) {
    return action==9 || action==0x19 || action==0x1a || action==0x1c || action==0x3c || action==0x40;
}
bool hasBoost(const nativeai::Creature *actor) {
    return actor && nativebuffs::find(*actor,0x0d)!=nullptr;
}
} // namespace
void draw(const Sources &source,const Draw &publish) {
    const uint8_t modifier=source.modifier ? source.modifier() : 0;
    const auto current=[&] {return source.actor ? source.actor() : nullptr;};
    const auto description=[&](int32_t id,float y) {
        publish(source.description ? source.description(id) : std::wstring{},{180,y},white);
    };
    const auto binding=[&](int index,const wchar_t *prefix,float y,float x=0) {
        publish(std::wstring(prefix)+(source.binding ? source.binding(index) : std::wstring{}),{x,y},bindingColour);
    };
    const auto cost=[&](const wchar_t *text,float y,glm::vec4 colour=staminaColour,float x=320) {
        return publish(text,{x,y},colour);
    };
    auto *actor=current();
    const int32_t first=actor && actor->record && source.primary ? source.primary(actor,modifier) : 0;
    binding(0,L"",0);
    description(first,0);
    if (first==0x6c && !hasBoost(current())) cost(L"[25 STAMINA]",0);

    binding(1,held(first) ? L"HOLD " : L"",20,held(first) ? 180.f : 0.f);
    actor=current();
    const int32_t second=actor && actor->record && source.secondary ? source.secondary(actor,modifier) : 0;
    description(second,20);
    if (second==6 || second==0x12 || second==0x15) cost(L"[ALL MP]",20,manaColour);
    else if (held(second)) cost(L"[CHARGE]",20,chargeColour);
    else if (second==0x32) {
        const float nextX=afterAnnotation(cost(L"[ALL MP]",20,manaColour).x);
        if (!hasBoost(current())) cost(L"[25 STAMINA]",20,staminaColour,nextX);
    } else if (second==0x6c && !hasBoost(current())) cost(L"[25 STAMINA]",20);

    binding(2,L"Hold ",40);
    actor=current();
    // Null records are an owned adapter boundary; the valid native graph has an actor.
    const uint8_t cls=actor && actor->record ? actor->record[0x14c] : 0;
    if (cls==1) {description(95,40); cost(L"[STAMINA]",40);}
    else if (cls==2 || cls==4) {
        const bool sprint=actor->record[0x14d]==1;
        description(sprint ? 81 : 80,40);
        if (sprint) cost(L"[STAMINA]",40);
    } else if (cls==3) description(83,40);

    binding(10,L"Move + ",60);
    publish(L"Dodge",{180,60},white);
    cost(L"[25 STAMINA]",60);

    actor=current();
    const uint8_t tapClass=actor && actor->record ? actor->record[0x14c] : 0;
    const bool hasTap=tapClass==1 || tapClass==2 || tapClass==3 ||
        (tapClass==4 && actor->record[0x14d]==0);
    if (hasTap) {
        binding(10,L"Tap ",80);
        int32_t tap=55; const wchar_t *tapCost=L"[100 STAMINA]";
        if (tapClass==2) {
            actor=current(); // Class C is retained; specialization is a later fresh load.
            const bool scout=actor && actor->record && actor->record[0x14d]==1;
            tap=scout ? 22 : 52; tapCost=scout ? L"[50 STAMINA]" : L"[100 STAMINA]";
        } else if (tapClass==3) {tap=51; tapCost=L"[100 MANA]";}
        else if (tapClass==4) tap=160;
        description(tap,80);
        cost(tapCost,80);
    }
    const float lastY=hasTap ? 100.f : 80.f;
    binding(14,L"",lastY);
    publish(L"Call/Ride Pet",{180,lastY},white);
}
} // namespace helpactions
