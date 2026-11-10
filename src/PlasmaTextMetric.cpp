#include "PlasmaTextMetric.h"
#include <bit>
#include <cmath>
#include <climits>
#include <stdexcept>

namespace plasma::textmetric {
namespace {
int32_t length(std::u16string_view text) {
    if (text.size()>=INT32_MAX) throw std::length_error("text metric signed-index domain");
    return int32_t(text.size());
}
uint16_t unit(std::u16string_view text,int32_t index) {
    return index>=0 && size_t(index)<text.size() ? uint16_t(text[size_t(index)]) : 0;
}
float negative(float value) { return std::bit_cast<float>(std::bit_cast<uint32_t>(value)^0x80000000u); }
int32_t truncate(float value) {
    if (!std::isfinite(value) || double(value)<INT32_MIN || double(value)>INT32_MAX)
        throw std::domain_error("text metric conversion outside finite signed domain");
    return int32_t(value);
}
float cut(float value) { return float(truncate(value)); }
float factor(const State &state) { return state.mapped ? state.height/(state.spanUpper-state.spanLower) : 1.f; }
float pair(const Providers &ports,uint16_t current,uint16_t next) {
    return ports.pair ? ports.pair(current,next) : 0.f;
}
void align(Point &point,const Box &whole,int flags) {
    if (flags&1) point[0]-=(whole.lower[0]+whole.upper[0])*0.5f;
    else if (flags&2) point[0]-=whole.upper[0];
    if (flags&4) point[1]-=(whole.lower[1]+whole.upper[1])*0.5f;
    else if (flags&8) point[1]-=whole.upper[1];
}
void alignLine(Point &point,const Box &line,const Box &whole,int flags) {
    const float correction=((line.upper[0]-line.lower[0])-whole.upper[0])+whole.lower[0];
    if (flags&1) point[0]-=correction*0.5f;
    else if (flags&2) point[0]-=correction;
}
float horizontalNorm(const Matrix &m) {
    const float y=m[5]*0.f+m[1],x=m[4]*0.f+m[0];
    const float yy=y*y,xx=x*x;
    return std::sqrt(yy+xx);
}
float verticalNorm(const Matrix &m) {
    const float y=m[1]*0.f+m[5],x=m[0]*0.f+m[4];
    const float yy=y*y,xx=x*x;
    return std::sqrt(yy+xx);
}
void sizes(State &state,float x,float y,float expansion) {
    if (y<=0) y=x;
    if (x<=0) x=y;
    if (x<=0 || y<=0 || expansion<0) return;
    state.width=x; state.height=y; state.expansion=expansion;
    state.matrix={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    state.integer=true;
}
}
float pairValue(int32_t word,bool integer,float horizontalSize,uint16_t unitsPerEm) {
    return integer ? float(word>>6) : (float(word)*horizontalSize)/float(unitsPerEm);
}
void configure(State &state,const Matrix &m,float size,float expansion,float gap,float extraLine,bool scalable) {
    if (state.mapped) {
        state.extraLine=extraLine; state.width=size; state.height=size;
        state.expansion=expansion; state.gap=gap; state.integer=false;
    } else if (scalable) {
        const float horizontal=horizontalNorm(m),vertical=verticalNorm(m);
        float y=vertical*size;
        float x=(cut(horizontal*size*64.f+0.5f))*(1.f/64.f);
        y=(cut(y*64.f+0.5f))*(1.f/64.f);
        if (x>1000) x=1000;
        if (y>1000) y=1000;
        const float ry=y/size,rx=x/size;
        float radius=((ry+rx)*0.5f)*expansion;
        const float scaledGap=rx*gap,scaledExtra=ry*extraLine;
        if (x<1) x=1;
        if (y<1) y=1;
        if (radius>500) radius=500;
        sizes(state,x,y,radius);
        state.gap=scaledGap; state.extraLine=scaledExtra;
    } else {
        state.matrix[0]=m[0]; state.matrix[1]=m[1]; state.matrix[5]=m[5]; state.matrix[4]=m[4];
        const auto negate=[](int32_t value) { return std::bit_cast<int32_t>(0u-uint32_t(value)); };
        state.fixed[0]=truncate(m[0]*65536.f);
        state.fixed[2]=negate(truncate(m[1]*65536.f));
        state.fixed[3]=truncate(m[5]*65536.f);
        state.fixed[1]=negate(truncate(m[4]*65536.f));
        if (size>0) {
            state.integer=false; state.width=state.height=size;
            const float vertical=verticalNorm(m),horizontal=horizontalNorm(m);
            state.expansion=expansion*(vertical<horizontal ? vertical : horizontal);
        }
        state.extraLine=extraLine; state.gap=gap;
    }
}
Box bounds(State &state,const Providers &ports,std::u16string_view text,int flags,int32_t start,bool pen) {
    const int32_t count=length(text),begin=start<0 ? 0 : start;
    int32_t end=count;
    if (start>=0 && start<=count)
        for (int32_t at=start;at<=count;++at)
            if (unit(text,at)==10) {end=at;break;}
    const int32_t limit=pen ? end+1 : end;
    Box box{{0,negative(state.height)},{0,0}};
    Point position{};
    bool first=true;
    const float scale=factor(state);
    for (int32_t at=begin;at<limit;++at) {
        const uint16_t current=unit(text,at);
        auto glyph=ports.glyph(current,false);
        if (state.expansion>0) glyph=ports.glyph(current,true);
        if (pen) {
            if (position[0]>box.upper[0]) box.upper[0]=position[0];
            if (position[1]>box.upper[1]) box.upper[1]=position[1];
        } else if (glyph && current!=10 && current!=13) {
            Point lower{},upper{};
            for (int axis=0;axis<2;++axis) {
                lower[axis]=state.mapped ? (glyph->mappedLower[axis]+glyph->origin[axis])*scale+position[axis]
                                         : (position[axis]+glyph->origin[axis])+1.f;
                upper[axis]=state.mapped ? (glyph->mappedUpper[axis]+glyph->origin[axis])*scale+position[axis]
                                         : ((position[axis]+glyph->origin[axis])+glyph->size[axis])-1.f;
            }
            if (first) {box={lower,upper};first=false;}
            else for (int axis=0;axis<2;++axis) {
                if (lower[axis]<box.lower[axis]) box.lower[axis]=lower[axis];
                if (upper[axis]>box.upper[axis]) box.upper[axis]=upper[axis];
            }
        }
        if (current==10) {
            position[0]=0;
            position[1]+=state.extraLine+state.height;
        } else {
            if (!current) break;
            if (glyph) position[0]+=scale*glyph->advance;
            if (at<limit-1 && unit(text,at+1)!=10) {
                if (!state.mapped) position[0]+=pair(ports,current,unit(text,at+1));
                position[0]+=state.gap;
                if (state.integer) position[0]=cut(position[0]);
            }
        }
    }
    if (flags) {
        const auto whole=bounds(state,ports,text,0,-1,pen);
        align(box.lower,whole,flags); align(box.upper,whole,flags);
    }
    return box;
}
void wrap(State &state,const Providers &ports,std::u16string &text,float width) {
    const int32_t count=length(text);
    const float scale=factor(state);
    float carried=0,saved=0;
    int32_t previous=-1;
    for (int32_t at=0;at<count;++at) {
        const uint16_t current=uint16_t(text[size_t(at)]);
        if (current==32 || current==10 || current==13 || at==count-1) {
            if (carried>=width) {
                if (previous>=0) text[size_t(previous)]=u'\n';
                carried-=saved;
            }
            previous=at; saved=carried;
        }
        if (current==10 || current==13) {carried=saved=0;continue;}
        const auto glyph=ports.glyph(current,false);
        float advance=glyph ? glyph->advance : 0.f;
        if (at<count-1) {
            if (!state.mapped) advance+=pair(ports,current,unit(text,at+1));
            carried+=state.gap;
        }
        carried=state.integer ? cut(carried+advance) : carried+advance*scale;
    }
}
Result index(State &state,const Providers &ports,std::u16string_view text,int32_t requested,int flags) {
    const float scale=factor(state);
    const int32_t count=length(text),target=requested>count ? count : requested;
    Result result{{0,negative(state.height)},{0,state.height}};
    const auto whole=bounds(state,ports,text,0,-1,false);
    auto line=bounds(state,ports,text,flags,0,false);
    alignLine(result.position,line,whole,flags);
    for (int32_t at=0;at<=target;++at) {
        const uint16_t current=unit(text,at);
        if (current==10 && at<target) {
            result.position[0]=0;
            line=bounds(state,ports,text,flags,at+1,false);
            alignLine(result.position,line,whole,flags);
            result.position[1]+=state.extraLine+state.height;
        } else {
            if (!current) break;
            const auto glyph=ports.glyph(current,false);
            if (glyph) {
                const float advance=glyph->advance*scale;
                if (at<target) result.position[0]+=advance;
                else result.size[0]=advance;
            }
            if (at<target-1) {
                if (!state.mapped) result.position[0]+=pair(ports,current,unit(text,at+1));
                result.position[0]+=state.gap;
                if (state.integer) result.position[0]=cut(result.position[0]);
            }
        }
    }
    if (flags) align(result.position,bounds(state,ports,text,0,-1,false),flags);
    return result;
}
Result measure(State &state,const Providers &ports,std::u16string_view text,int32_t requested,int flags,float size,float width,bool scalable) {
    const float vertical=size/state.height;
    const float horizontal=size/state.width;
    if (scalable) width/=horizontal;
    Result result;
    if (flags&16) {
        std::u16string temporary(text);
        wrap(state,ports,temporary,width);
        result=index(state,ports,temporary,requested,flags);
    } else result=index(state,ports,text,requested,flags);
    result.position[0]*=horizontal; result.position[1]*=vertical;
    result.size[0]*=horizontal; result.size[1]*=vertical;
    return result;
}
}
