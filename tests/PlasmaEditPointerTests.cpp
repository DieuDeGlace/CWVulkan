// Conditional source-derived call/number traces with supplied metric outputs.
// These exercise the Edit caller, without asserting native font-provider parity.
#include "PlasmaEdit.h"
#include <cstdio>
#include <limits>
#include <vector>

namespace {
using Edit = plasma::Edit;
Edit::Matrix identity() { Edit::Matrix result{}; result[0]=result[5]=result[10]=result[15]=1; return result; }
struct Caption {
    std::u16string text = u"ABCDEF";
    std::vector<std::string> trace;
    std::function<void()> onLayout;
    Edit edit;
    Caption() : edit({[this] { return text; },[this](const auto &value) { text=value; },
                      [this] { trace.emplace_back("layout"); if (onLayout) onLayout(); }}) {}
};
struct Pointer {
    Caption &caption;
    bool selected=false, left=false, shift=false;
    float point=0;
    int captureOwner=1, captured=0, captures=0, releases=0, begins=0, queries=0, localQueries=0;
    std::vector<bool> localDragging;
    std::vector<std::u16string> strings;
    std::function<void()> onSelect,onRelease;
    std::function<Edit::Metric(int32_t)> metric;
    Edit::Pointer ports() {
        return {
            [this] { caption.trace.emplace_back("selected"); return selected; },
            [this] { return left; },[this] { return shift; },
            [this] { ++localQueries; localDragging.push_back(caption.edit.dragging); caption.trace.emplace_back("local"); return point; },
            [this] { selected=true; caption.trace.emplace_back("select"); if (onSelect) onSelect(); },
            [this] { ++captures; captured=captureOwner; caption.trace.emplace_back("capture"); },
            [this] { ++releases; captured=0; caption.trace.emplace_back("release"); if (onRelease) onRelease(); },
            [this] {
                ++begins; caption.trace.emplace_back("begin");
                return Edit::Query([this](std::u16string_view text,int32_t index) {
                    ++queries; strings.emplace_back(text);
                    return metric ? metric(index) : Edit::Metric{{float(index)*10,0},{10,10}};
                });
            }};
    }
};
struct Layout {
    Caption &caption;
    bool parent=true;
    int liveNode=1, calls=0, updates=0, parentChecks=0;
    Edit::Point lower{10,0},upper{100,100};
    Edit::Matrix local=identity();
    Edit::Metric first{{10,0},{2,1}},second{{20,0},{0,1}};
    float positions[4]{},baseline=0;
    std::vector<std::string> trace;
    std::vector<int32_t> indices;
    std::vector<std::u16string> strings;
    std::vector<float> writes;
    std::vector<int> writeNodes;
    std::function<Edit::Metric(int)> metric;
    Edit::Layout ports() {
        return {
            [this] { ++parentChecks; return parent; },
            [this] { trace.emplace_back("lower"); return lower; },
            [this] { trace.emplace_back("upper"); return upper; },
            [this] {
                const int captured=liveNode; trace.emplace_back("node:"+std::to_string(captured));
                return Edit::Layout::Node{
                    [this,captured] { trace.emplace_back("forward:"+std::to_string(captured)); return identity(); },
                    [this,captured] { trace.emplace_back("chain:"+std::to_string(captured)); return identity(); }};
            },
            [this] { trace.emplace_back("local:"+std::to_string(liveNode)); return local; },
            [this](std::u16string_view text,int32_t index,const Edit::Matrix &) {
                strings.emplace_back(text); indices.push_back(index); ++calls;
                trace.emplace_back("metric:"+std::to_string(calls));
                return metric ? metric(calls) : calls==1 ? first : second;
            },
            [this] { return positions[liveNode]; },[this] { return baseline; },
            [this](float value) { positions[liveNode]=value; writes.push_back(value); writeNodes.push_back(liveNode); },
            [this] { ++updates; trace.emplace_back("update:"+std::to_string(liveNode)); }};
    }
};
}

int plasmaEditPointerTests() {
    int failures=0;
    const auto check=[&](bool pass,const char *message) {
        if (!pass) { ++failures; std::printf("FAIL Edit pointer/layout: %s\n",message); }
    };
    {
        Caption c; Pointer p{c}; c.edit.dragging=true;
        c.onLayout=[&] { p.captureOwner=7; };
        c.edit.leftDown(p.ports());
        check(c.edit.caret==6 && c.edit.selection==-6 && !c.edit.dragging && p.captured==7 && p.queries==0 &&
                  p.localDragging==std::vector<bool>{false} &&
                  c.trace==std::vector<std::string>{"local","selected","select","layout","capture"},
              "first press clears drag, selects all without pick, then clamps/layouts/captures using the later owner");
    }
    {
        Caption c; Pointer p{c};
        p.onSelect=[&] { c.text=u"XYZ"; c.edit.dragging=true; };
        c.edit.leftDown(p.ports());
        check(c.edit.caret==3 && c.edit.selection==-3 && c.edit.dragging && p.captures==1,
              "first-press continuation rereads changed caption and preserves a callback's later drag write");
    }
    {
        Caption c; Pointer p{c}; c.text=u"0123456789"; c.edit.caret=3;
        p.selected=p.shift=true; p.point=57;
        c.edit.leftDown(p.ports());
        check(c.edit.caret==5 && c.edit.selection==-5 && c.edit.dragging && p.begins==1 && p.queries==10,
              "selected Shift press subtracts old-plus-picked index, followed by the common clamp");
        p.left=true; p.point=77;
        c.edit.move(p.ports());
        check(c.edit.caret==7 && c.edit.selection==-7 && p.captures==1,
              "drag motion adds old-minus-picked index and does not assign capture again");
        p.left=false; const int before=p.queries;
        p.onRelease=[&] { c.edit.move(p.ports()); };
        c.edit.leftUp(p.ports());
        check(p.releases==1 && p.captured==0 && c.edit.dragging && p.queries==before,
              "release clears logical capture through its provider, retains drag and respects an already cleared button bit");
    }
    {
        Caption c; Pointer p{c}; c.text=u"AB"; c.edit.caret=1; c.edit.selection=1;
        p.selected=true; p.point=17;
        c.edit.leftDown(p.ports());
        check(c.edit.caret==1 && c.edit.selection==0 && p.queries==2,
              "0.7-width threshold equality does not advance and ordinary selected press clears selection after picking");
    }
    {
        Caption c; Pointer p{c}; c.text=u"ABC"; p.selected=true;
        p.metric=[](int32_t index) { return Edit::Metric{{index==1 ? -10.f : 100.f,0},{0,1}}; };
        c.edit.leftDown(p.ports());
        check(c.edit.caret==2 && p.queries==3,"pick visits every unit despite nonmonotone thresholds and an earlier failed comparison");
    }
    {
        Caption c; Pointer p{c}; c.text=u"AB"; p.selected=true; p.point=100;
        p.metric=[&](int32_t index) {
            if (!index) c.text=u"Z";
            return Edit::Metric{{float(index)*10,0},{10,1}};
        };
        c.edit.leftDown(p.ports());
        check(p.begins==1 && p.strings==std::vector<std::u16string>{u"AB",u"AB"} && c.edit.caret==1,
              "pick retains one copied string/matrix session, queries live providers and clamps against the later caption");
    }
    {
        Caption c; Pointer p{c}; p.left=true; c.edit.dragging=true;
        c.edit.move(p.ports());
        p.selected=true; p.left=false; c.edit.move(p.ports());
        p.left=true; c.edit.dragging=false; c.edit.move(p.ports());
        check(p.localQueries==0 && p.queries==0,"button, selected-owner and drag gates precede motion geometry");
        Edit detached({}); detached.dragging=true;
        detached.leftDown(p.ports()); detached.leftUp(p.ports());
        check(detached.dragging && p.localQueries==0 && p.releases==1,
              "missing-caption press returns before state/geometry while release has no caption admission");
    }
    {
        Caption c; Layout g{c}; c.edit.caret=3;
        c.edit.adjustLayout(g.ports());
        check(g.calls==1 && g.writes.empty() && g.updates==0 &&
                  g.trace==std::vector<std::string>{"lower","upper","node:1","forward:1","metric:1","local:1"},
              "ordered bounds/metric/current local matrix precede lower-bound equality's no-adjustment return");
    }
    {
        Caption c; Layout g{c}; c.text=u"abcdefghijklmnopqrst"; c.edit.caret=10;
        g.lower={0,0}; g.upper={12,100};
        g.metric=[&](int call) {
            if (call==1) {
                g.liveNode=2; g.local[0]=2; c.edit.caret=11; c.text=u"new";
                return Edit::Metric{{5,0},{1,1}};
            }
            g.liveNode=3; g.local[0]=99;
            return Edit::Metric{{20.4f,0},{0,1}};
        };
        c.edit.adjustLayout(g.ports());
        check(g.indices==std::vector<int32_t>{10,17} && g.strings==std::vector<std::u16string>{u"abcdefghijklmnopqrst",u"new "} &&
                  g.writes==std::vector<float>{-28} && g.writeNodes==std::vector<int>{3} && g.updates==1 &&
                  g.trace==std::vector<std::string>{"lower","upper","node:1","forward:1","metric:1","local:2","forward:1","metric:2","update:3"},
              "upper equality probes current index plus six/current text plus space with captured node, retained local matrix and late live store");
    }
    {
        Caption c; Layout g{c}; c.text=u"ABCD"; c.edit.caret=3;
        g.first={{5,0},{1,1}}; g.second={{1.8f,0},{1,1}};
        c.edit.adjustLayout(g.ports());
        check(g.indices==std::vector<int32_t>{3,-3} && g.strings.back()==u"ABCD " &&
                  g.writes==std::vector<float>{8,0} && g.updates==1 && g.trace[6]=="chain:1",
              "lower route forwards negative index-minus-six, truncates adjustment and resets the leading-six baseline");
    }
    {
        Caption c; Layout g{c}; c.text=std::u16string(20,u'X'); c.edit.caret=10;
        g.first={{5,0},{1,1}}; g.second={{1.8f,0},{1,1}};
        c.edit.adjustLayout(g.ports());
        check(g.writes==std::vector<float>{8,0} && g.updates==1,
              "later caret still resets an adjusted position strictly above its saved baseline");
    }
    {
        Caption c; Layout g{c}; g.first.position[0]=std::numeric_limits<float>::quiet_NaN();
        c.edit.adjustLayout(g.ports());
        check(g.calls==1 && g.writes.empty() && !g.updates,
              "masked unordered first/lower comparisons follow their separate native no-adjustment path");
        g.parent=false; g.trace.clear(); g.calls=0;
        c.edit.adjustLayout(g.ports());
        check(g.calls==0 && g.trace.empty(),"missing parent exits before bounds or font queries");
        Edit detached({}); g.parentChecks=0;
        detached.adjustLayout(g.ports());
        check(g.parentChecks==0,"missing caption exits before even the parent query");
    }
    std::printf("Edit pointer/layout: %s\n",failures ? "FAIL" : "PASS");
    return failures;
}
