#pragma once
// Native text-index/bounds/wrap callers, from the complete held 20261109 audit.
// Glyph and converted pair values are supplied boundaries, not font algorithms.
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace plasma::textmetric {
using Point = std::array<float,2>;
using Matrix = std::array<float,16>;
struct Glyph {
    Point origin{}, size{};
    float advance=0;
    Point mappedLower{}, mappedUpper{};
};
struct State {
    float width=1, height=1, expansion=0, gap=0, extraLine=0;
    bool mapped=false, integer=false;
    float spanLower=0, spanUpper=1;
    Matrix matrix{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    std::array<int32_t,4> fixed{65536,0,0,65536};
};
struct Providers {
    // Bounds obtains primary first, then replaces it with secondary on expansion.
    std::function<std::optional<Glyph>(uint16_t,bool secondary)> glyph;
    // Composite boundary includes next-map/current-map and native mode conversion.
    std::function<float(uint16_t current,uint16_t next)> pair;
};
struct Box { Point lower{},upper{}; };
struct Result { Point position{},size{}; };
// Mode conversion of an independently supplied first signed pair-output word.
float pairValue(int32_t word,bool integer,float horizontalSize,uint16_t unitsPerEm);
void configure(State &,const Matrix &,float size,float expansion,float gap,float extraLine,bool scalable);
Box bounds(State &,const Providers &,std::u16string_view,int flags,int32_t start,bool pen);
void wrap(State &,const Providers &,std::u16string &,float width);
Result index(State &,const Providers &,std::u16string_view,int32_t index,int flags);
Result measure(State &,const Providers &,std::u16string_view,int32_t index,int flags,float size,float wrapWidth,bool scalable);
}
