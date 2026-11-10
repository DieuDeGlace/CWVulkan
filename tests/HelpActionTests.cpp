#include "Assets.h"
#include "GameOptions.h"
#include "HelpActions.h"
#include "NativeTerms.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <limits>

int helpActionTests() {
    int failures=0;
    const auto check=[&](bool value,const char *label) {
        if (!value) {++failures; std::cerr<<"Help actions: "<<label<<'\n';}
    };
    using namespace nativeterms;
    check(actionToken(55)==u"SkillSmash" && actionToken(149)==u"SkillHeroicShout" &&
          actionToken(134)==actionToken(135) && actionToken(2)==u"SkillAttack", "numeric identities and repeated/alias assignments");
    int mapped=0;
    for (int id=0;id<256;++id) mapped+=!actionToken(id).empty();
    check(mapped==72 && actionToken(-1).empty() && actionToken(0).empty() &&
          actionToken(std::numeric_limits<int32_t>::min()).empty() && actionToken(108+256).empty(),
          "exact full signed-width key lookup without fallback or truncation");
    Dictionary dictionary;
    std::string error;
    check(dictionary.describe(55).empty(),"missing term contributes empty text despite a known action token");
    const char *xml=R"(<root>
      <speech id="SkillSmash">lower case &amp; exact</speech>
      <speech id="SkillAttack">old</speech><name id="SkillAttack">replacement</name>
      <speech id=""><variant>ignored</variant></speech>
      <speech id="choice">{ #other #default admitted | fallback }</speech>
      <speech id="stop">{ #default first | #default second | free }</speech>
      <speech id="free">{ first | second }</speech>
      <speech id="optional">base [ extra ]</speech>
      <speech id="variant">base<arbitrary>first [ extra ]</arbitrary><other>second</other></speech>
      <speech id="rejected">base<child>#other rejected</child></speech>
      <speech id="blocks">first % second</speech><speech id="empty-first">% second</speech>
      <speech id="punct">a/b(c) d-e, f! g?</speech>
      <speech id="glue">James's @who=s</speech><speech id="lost">=x kept</speech>
      <speech id="macro">plain *same</speech><speech id="same">term</speech><macro id="same">macro</macro>
      <speech id="plain-name">@who @</speech>
      <speech id="reference">@target(*, alias=who)</speech>
      <speech id="target">{ #default @alias @ | other }<child>variant</child></speech>
      <speech id="empty-params">@target()</speech>
      <speech id="partial">before ] after</speech><speech id="unclosed">{ accepted</speech>
      <speech id="unicode">&#x1f600; &#x3a9;</speech>
      <speech id="loop">before *loop after</speech><macro id="loop">*loop</macro>
      <macro id="variant_free">base<child>ignored</child></macro>
      <speech id="use-macro">*variant_free</speech>
    </root>)";
    check(dictionary.loadXml(xml,error),"synthetic dictionary loads");
    const Context context{{u"#default"},{{u"@",u"root"},{u"@who",u"value"},{u"@target",u"target"},
                                        {u"!@target",u"own"}}};
    const auto text=[&](View key,uint32_t seed=0) {return dictionary.format(key,context,seed);};
    check(dictionary.describe(55)==u"lower case & exact" && dictionary.describe(2)==u"replacement",
          "resource order, exact case, decoded entities and no capitalization");
    check(text(u"skillSmash").empty() && text(u"missing").empty(),"case-sensitive term miss has no literal fallback");
    check(text(u"choice")==u"admitted" && text(u"stop")==u"first", "OR condition keys and first admitted conditional branch stop traversal");
    check(text(u"free")==u"first" && text(u"free",1)==u"second", "unsigned xorshift choice without global RNG");
    check(text(u"optional")==u"base" && text(u"optional",1)==u"base extra", "square child gate uses first two stages and a by-value seed");
    check(text(u"variant")==u"first extra" && text(u"variant",1)==u"second", "arbitrary child names create ordered variants and advance only the variant seed");
    check(text(u"rejected").empty(),"selected rejected variant does not try another variant or base root");
    check(text(u"blocks")==u"first" && text(u"empty-first").empty(),"plain assembly uses only the first block including an empty first block");
    check(text(u"punct")==u"a / b ( c ) d-e, f! g?", "parser punctuation and plain spacing exemptions remain distinct");
    check(text(u"glue")==u"James's values" && text(u"lost")==u"kept", "apostrophe and equals word joins including lost initial equals piece");
    check(text(u"macro")==u"plain macro" && text(u"same")==u"term" && text(u"use-macro")==u"base", "term and macro namespaces and macro child omission");
    check(text(u"plain-name")==u"value root" && text(u"reference")==u"value own" && text(u"target")==u"variant",
          "plain names, copied alias/own-name substitution and referenced base-root variant bypass");
    check(text(u"empty-params")==u"own", "empty parameter clause inserts the default category");
    check(text(u"! literal")==u"! literal" && text(u"* literal")==u"* literal", "top-level literal prefixes are retained and bypass grammar lookup");
    check(text(u"partial")==u"before" && text(u"unclosed")==u"accepted", "partial parse root is retained and unclosed opening groups are not rejected");
    check(text(u"unicode")==u"\U0001f600 \u03a9", "UTF-8 resource adapter retains UTF-16 surrogate units");
    check(text(u"loop")==u"before after", "owned reference cycle policy terminates without a guessed substitute");
    check(!dictionary.loadXml("<root><speech>",error) && dictionary.describe(55)==u"lower case & exact",
          "owned failed-resource reload policy retains the previous complete dictionary");

    struct Published {std::wstring text; glm::vec2 at; glm::vec4 colour;};
    std::vector<Published> rows;
    std::vector<int32_t> descriptions;
    std::array<uint8_t,0x970> firstRecord{},secondRecord{};
    nativeai::Creature first,second; first.record=firstRecord.data(); second.record=secondRecord.data();
    nativeai::Creature *current=&first;
    uint8_t modifier=128; int modifierReads=0,actorReads=0;
    int32_t primary=0x6c,secondary=0x32;
    bool replaceActor=false;
    float annotationFlow=339.875f;
    std::function<void(const Published &)> afterPublish;
    helpactions::Sources source;
    source.modifier=[&] {++modifierReads; return modifier;};
    source.actor=[&] {++actorReads; return current;};
    source.binding=[](int index) {return std::to_wstring(index);};
    source.description=[&](int32_t id) {
        descriptions.push_back(id);
        if (replaceActor && id==primary) {current=&second; modifier=0;}
        return L"action"+std::to_wstring(id);
    };
    source.primary=[&](auto *actor,uint8_t cached) {
        check(actor==&first && cached==128,"primary query uses its current actor and full modifier byte"); return primary;
    };
    source.secondary=[&](auto *actor,uint8_t cached) {
        check(actor==(replaceActor ? &second : &first) && cached==128,"secondary query reloads the actor and retains the entry modifier"); return secondary;
    };
    const auto draw=[&] {
        rows.clear(); descriptions.clear(); modifierReads=actorReads=0; modifier=128; current=&first;
        helpactions::draw(source,[&](const auto &value,auto at,auto colour) {
            rows.push_back({value,at,colour});
            if (afterPublish) afterPublish(rows.back());
            return glm::vec2(value==L"[ALL MP]" ? annotationFlow : -777.f,999.f);
        });
    };
    const auto has=[&](const wchar_t *value,glm::vec2 at) {
        return std::any_of(rows.begin(),rows.end(),[&](const auto &row) {return row.text==value && row.at==at;});
    };
    firstRecord[0x14c]=4; firstRecord[0x14d]=1;
    draw();
    check(descriptions==std::vector<int32_t>{108,50,81} && modifierReads==1,
          "rogue specialization and exact primary/secondary/held description query order");
    check(has(L"[25 STAMINA]",{320,0}) && has(L"[ALL MP]",{320,20}) && has(L"[25 STAMINA]",{359,20}) &&
          has(L"[STAMINA]",{320,40}) && has(L"Call/Ride Pet",{180,80}),"second annotation continues from the first annotation flow; final row stays compact");
    const auto mana=std::find_if(rows.begin(),rows.end(),[](const auto &row) {return row.text==L"[ALL MP]";});
    check(mana!=rows.end() && mana->colour==glm::vec4(.6f,.6f,1,1) &&
          std::next(mana)!=rows.end() && std::next(mana)->text==L"[25 STAMINA]" &&
          rows.front().colour==glm::vec4(.25f,1,1,1),"annotation publication order and native binding/cost colours");
    for (const uint8_t cls:{uint8_t(1),uint8_t(2),uint8_t(3),uint8_t(4),uint8_t(0)})
        for (const uint8_t spec:{uint8_t(0),uint8_t(1),uint8_t(2)}) {
            firstRecord[0x14c]=cls; firstRecord[0x14d]=spec; draw();
            const bool tap=cls==1 || cls==2 || cls==3 || (cls==4 && spec==0);
            check(has(L"Call/Ride Pet",{180,tap ? 100.f : 80.f}) && has(L"Hold 2",{0,40}) &&
                  has(L"Dodge",{180,60}) && has(L"[25 STAMINA]",{320,60}),"class/spec matrix keeps unconditional held/Dodge rows and compact final row");
            const int heldId=cls==1 ? 95 : (cls==2 || cls==4) ? (spec==1 ? 81 : 80) : cls==3 ? 83 : 0;
            const int tapId=cls==1 ? 55 : cls==2 ? (spec==1 ? 22 : 52) : cls==3 ? 51 : cls==4 && spec==0 ? 160 : 0;
            std::vector<int32_t> expected{primary,secondary};
            if (heldId) expected.push_back(heldId);
            if (tapId) expected.push_back(tapId);
            check(descriptions==expected,"all held and tap description identifiers without a default description call");
        }
    primary=0x19; secondary=0x19; draw();
    check(has(L"HOLD 1",{180,20}) && has(L"[CHARGE]",{320,20}),"uppercase branch keys off primary A and preserves its unusual x coordinate");
    primary=0x119; secondary=0x132; draw();
    check(has(L"1",{0,20}) && !has(L"HOLD 1",{180,20}) && !has(L"[ALL MP]",{320,20}) &&
          descriptions[0]==0x119 && descriptions[1]==0x132,"full 32-bit helper results do not alias low-byte branches");
    primary=108; secondary=50; replaceActor=true;
    secondRecord[0x14c]=3; secondRecord[0x14d]=1;
    std::array<uint8_t,0x18> boost{}; const int32_t boostType=0x0d;
    std::memcpy(boost.data(),&boostType,4);
    boost[1]=0xa7; boost[2]=0xb8; boost[3]=0xc9; // Loaded buff padding is preserved, not a wider type.
    second.buffs.push_back(boost);
    draw();
    check(modifierReads==1 && actorReads>=5 && descriptions==std::vector<int32_t>{108,50,83,51} &&
          !has(L"[25 STAMINA]",{320,0}) && !has(L"[25 STAMINA]",{359,20}) &&
          has(L"[100 MANA]",{320,80}),"description callback mutation affects fresh buff/class receivers but not cached modifier");
    replaceActor=false;
    source.binding=[&](int index) {
        if (index==2) {firstRecord[0x14c]=1; firstRecord[0x14d]=0;}
        return std::to_wstring(index);
    };
    firstRecord[0x14c]=0; draw();
    check(descriptions==std::vector<int32_t>{108,50,95,55},"third binding publication precedes its current-class query");

    source.binding=[](int index) {return std::to_wstring(index);};
    source.secondary=[&](auto *,uint8_t cached) {check(cached==128,"cached modifier survives binding publication"); return secondary;};
    firstRecord[0x14c]=4; firstRecord[0x14d]=1;
    for (const auto [flow,expected]:std::array<std::pair<float,float>,3>{{{339.875f,359.f},{-1.75f,19.f},{0.f,20.f}}}) {
        annotationFlow=flow; draw();
        check(has(L"[25 STAMINA]",{expected,20}),"annotation uses truncated first flow component plus twenty without a second column offset");
    }
    annotationFlow=339.875f;
    bool bindingPublished=false,queriedAfterBinding=false;
    afterPublish=[&](const Published &row) {
        if (row.text==L"1" && row.at==glm::vec2(0,20)) {bindingPublished=true; current=&second; modifier=0;}
    };
    source.secondary=[&](auto *actor,uint8_t cached) {
        queriedAfterBinding=bindingPublished && actor==&second && cached==128;
        return secondary;
    };
    draw();
    check(queriedAfterBinding,"secondary binding is published before the fresh secondary actor query");
    source.secondary=[&](auto *,uint8_t) {return secondary;};
    afterPublish=[&](const Published &row) {
        if (row.text==L"Tap 10") {current=&second; secondRecord[0x14c]=1; secondRecord[0x14d]=1;}
    };
    firstRecord[0x14c]=2; firstRecord[0x14d]=0; draw();
    check(descriptions==std::vector<int32_t>{108,50,80,22} && has(L"[50 STAMINA]",{320,80}),
          "tap retains its captured class but reloads ranger specialization after binding publication");
    afterPublish={};

    Assets assets;
    if (assets.locate("")) {
        Dictionary owned;
        check(owned.load(assets.gameDir()/"dict_en.xml",error),"owned dictionary resource loads");
        check(owned.size()>900 && !owned.describe(55).empty() && !owned.describe(95).empty() &&
              !owned.describe(108).empty() && owned.describe(-1).empty(),"actual installed resource supplies Help descriptions without missing-ID fallbacks");
        std::cout<<"Help owner dictionary: "<<owned.size()<<" terms checked\n";
    } else std::cout<<"SKIP Help owner dictionary: installed assets unavailable\n";
    std::cout<<"Help actions: "<<(failures ? "FAIL\n" : "PASS\n");
    return failures;
}
