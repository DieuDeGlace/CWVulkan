// Literal conditional vectors from the independent source-side text metric audit.
#include "PlasmaTextMetric.h"
#include <bit>
#include <cstdio>
#include <tuple>
#include <vector>

namespace {
namespace metricmodel = plasma::textmetric;
metricmodel::Matrix identity() {return {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};}
struct Font {
    metricmodel::State state;
    float a=2.5f,b=3.75f,space=1,cr=0;
    std::vector<std::pair<uint16_t,bool>> fetched;
    std::vector<std::pair<uint16_t,uint16_t>> pairs;
    int32_t rawPair=0;
    bool secondaryMissing=false;
    Font() {
        state.mapped=true;state.width=state.height=8;state.gap=.25f;state.extraLine=2;state.spanUpper=8;
    }
    metricmodel::Providers ports() {
        return {
            [this](uint16_t code,bool secondary)->std::optional<metricmodel::Glyph> {
                fetched.emplace_back(code,secondary);
                if (secondary && secondaryMissing) return {};
                const float advance=code=='A' ? a : code=='B' ? b : code==32 ? space : code==13 ? cr : 0;
                if (!advance) return {};
                metricmodel::Glyph g;
                g.origin=state.mapped ? metricmodel::Point{0,0} : metricmodel::Point{-1,-state.height-1};
                g.size={advance+2,state.height+2};g.advance=advance;
                g.mappedLower={0,-state.height};g.mappedUpper={advance,0};
                return g;
            },
            [this](uint16_t current,uint16_t next) {
                pairs.emplace_back(current,next);
                return metricmodel::pairValue(rawPair,state.integer,state.width,16);
            }};
    }
};
}

int plasmaTextMetricTests() {
    int failures=0;
    const auto check=[&](bool pass,const char *message) {
        if (!pass) {++failures;std::printf("FAIL native text index: %s\n",message);}
    };
    struct Case {std::u16string text;int index;metricmodel::Result expected;};
    const Case cases[]{
        {u"AB",-1,{{0,-8},{0,8}}},{u"AB",0,{{0,-8},{2.5f,8}}},
        {u"AB",1,{{2.5f,-8},{3.75f,8}}},{u"AB",2,{{6.5f,-8},{0,8}}},
        {u"AB",7,{{6.5f,-8},{0,8}}},{u"",0,{{0,-8},{0,8}}},
        {u"A\nB",1,{{2.5f,-8},{0,8}}},{u"A\nB",2,{{0,2},{3.75f,8}}},
        {u"A\nB",3,{{3.75f,2},{0,8}}}
    };
    for (const auto &value:cases) {
        Font font;
        const auto actual=metricmodel::measure(font.state,font.ports(),value.text,value.index,0,8,0,false);
        check(actual.position==value.expected.position && actual.size==value.expected.size,
              "independently supplied index/default/newline/end literal vector");
        if (value.index<0)
            check(!font.fetched.empty(),"negative requested index still performs both preceding bounds queries");
    }
    {
        Font font;font.a=2;
        const auto wrapped=metricmodel::measure(font.state,font.ports(),u"A A",3,16,8,3,false);
        const auto unwrapped=metricmodel::measure(font.state,font.ports(),u"A A",3,16,8,4,false);
        check(wrapped.position==metricmodel::Point{2,2} && wrapped.size==metricmodel::Point{0,8} &&
                  unwrapped.position==metricmodel::Point{5.5f,-8} && unwrapped.size==metricmodel::Point{0,8},
              "wrap compares before the final glyph, replaces the previous space and preserves source input");
    }
    {
        Font font;font.state.mapped=false;font.state.integer=true;font.state.gap=.5f;font.rawPair=-128;
        const auto integer=metricmodel::index(font.state,font.ports(),u"AB",2,0);
        check(integer.position==metricmodel::Point{4.75f,-8} && integer.size==metricmodel::Point{0,8} &&
                  metricmodel::pairValue(-128,true,8,16)==-2,
              "signed pair shift and intermediate X truncation match the independent literal vector");
        font.state.integer=false;font.rawPair=-65;
        const auto pixel=metricmodel::index(font.state,font.ports(),u"AB",2,0);
        check(pixel.position==metricmodel::Point{-25.75f,-8} && metricmodel::pairValue(-65,false,8,16)==-32.5f,
              "pixel pair conversion multiplies before division without integer-mode truncation");
    }
    {
        Font font;
        const auto centred=metricmodel::index(font.state,font.ports(),u"AB",0,1);
        const auto both=metricmodel::index(font.state,font.ports(),u"AB",0,3);
        check(centred.position==metricmodel::Point{-3.25f,-8} && both.position==centred.position,
              "ink-bound alignment uses one-over-two flag priority rather than adding both shifts");
        font.fetched.clear();
        const auto pen=metricmodel::bounds(font.state,font.ports(),u"AB",0,-1,true);
        check(pen.lower==metricmodel::Point{0,-8} && pen.upper==metricmodel::Point{6.75f,0} && font.fetched.back().first==0,
              "pen bounds include the last-unit gap and ending pen, obtaining its zero-unit glyph before stopping");
        font.state.expansion=1;font.secondaryMissing=true;
        font.fetched.clear();
        const auto missing=metricmodel::bounds(font.state,font.ports(),u"A",0,-1,false);
        check(missing.lower==metricmodel::Point{0,-8} && missing.upper==metricmodel::Point{0,0} &&
                  font.fetched==std::vector<std::pair<uint16_t,bool>>{{'A',false},{'A',true}},
              "secondary miss replaces the already fetched primary without a fallback box");
    }
    {
        Font font;font.cr=4;
        const auto atCR=metricmodel::index(font.state,font.ports(),u"A\rB",1,0);
        check(atCR.position==metricmodel::Point{2.5f,-8} && atCR.size==metricmodel::Point{4,8},
              "carriage return is an ordinary target glyph rather than a line transition");
        std::u16string embedded{u'A',0,u'B'};
        font.fetched.clear();
        const auto ended=metricmodel::index(font.state,font.ports(),embedded,2,0);
        check(ended.position==metricmodel::Point{2.75f,-8} && ended.size[0]==0,
              "indexed loop stops at embedded zero after its earlier bounds side effects");
        font.fetched.clear();metricmodel::wrap(font.state,font.ports(),embedded,100);
        check(font.fetched.size()==3 && font.fetched[1].first==0 && font.fetched[2].first=='B',
              "wrapping keeps visiting stored units after an embedded zero");
    }
    {
        metricmodel::State state;
        auto matrix=identity();matrix[0]=2;matrix[5]=3;
        metricmodel::configure(state,matrix,10,.5f,.25f,.5f,true);
        check(state.width==20 && state.height==30 && state.expansion==1.25f && state.gap==.5f &&
                  state.extraLine==1.5f && state.integer && state.matrix==identity(),
              "independent two/three matrix configuration vector and derived operands");
        check(std::bit_cast<uint32_t>(10.f/state.width)==0x3f000000u &&
                  std::bit_cast<uint32_t>(10.f/state.height)==0x3eaaaaabu,
              "outer horizontal and vertical scale words are kept separately");
        matrix=identity();matrix[0]=matrix[5]=.01f;
        metricmodel::configure(state,matrix,10,2,4,8,true);
        check(state.width==1 && state.height==1 && state.expansion==.01875f &&
                  state.gap==.0375f && state.extraLine==.075f,
              "ratios derive before the later one-pixel dimension clamp");
        matrix=identity();matrix[0]=3;matrix[1]=4;matrix[5]=2;
        state.matrix[12]=7;
        metricmodel::configure(state,matrix,8,2,6,9,false);
        check(state.width==8 && state.height==8 && state.expansion==4 && state.gap==6 &&
                  state.extraLine==9 && !state.integer && state.matrix[12]==7 &&
                  state.fixed==std::array<int32_t,4>{196608,0,-262144,131072},
              "pixel configuration uses signed fixed words and retains untouched matrix words");
        state.width=9;state.height=10;state.expansion=11;state.integer=true;
        metricmodel::configure(state,identity(),0,2,3,4,false);
        check(state.width==9 && state.height==10 && state.expansion==11 && state.integer && state.gap==3 && state.extraLine==4,
              "rejected pixel size still stores later gap and extra-line operands");
    }
    std::printf("Native text-index metric: %s\n",failures ? "FAIL" : "PASS");
    return failures;
}
