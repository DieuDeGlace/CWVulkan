#include "NativeTerms.h"
#include "DialogueWords.h"
#include <algorithm>
#include <charconv>
#include <fstream>
#include <iterator>
#include <sstream>
#include <utility>
#include <vector>

namespace nativeterms {
namespace {
struct Action { int32_t id; View token; };
// Constructor assignment order, including the repeated Heroic Shout assignment.
constexpr Action actions[] = {
    {55,u"SkillSmash"},{95,u"SkillCyclone"},{113,u"SkillBulwark"},{149,u"SkillHeroicShout"},
    {114,u"SkillWarFrenzy"},{22,u"SkillRangerKick"},{52,u"SkillRetreat"},{111,u"SkillAim"},
    {112,u"SkillSwiftness"},{142,u"SkillQuicksandTrap"},{143,u"SkillShadowClone"},
    {97,u"SkillFireExplosion"},{115,u"SkillManaShield"},{51,u"SkillTeleport"},
    {134,u"SkillHealingStream"},{135,u"SkillHealingStream"},{83,u"SkillFloat"},
    {19,u"SkillAttack"},{20,u"SkillAttack"},{8,u"SkillAttack"},{7,u"SkillAttack"},
    {14,u"SkillAttack"},{15,u"SkillAttack"},{68,u"SkillAttack"},{59,u"SkillAttack"},
    {58,u"SkillAttack"},{10,u"SkillAttack"},{11,u"SkillAttack"},{3,u"SkillAttack"},
    {2,u"SkillAttack"},{23,u"SkillAttack"},{27,u"SkillAttack"},{28,u"SkillChargedBoomerang"},
    {26,u"SkillSalvo"},{25,u"SkillChargedShot"},{60,u"SkillCrush"},{64,u"SkillSwirl"},
    {9,u"SkillBlock"},{6,u"SkillPerforate"},{18,u"SkillAmbush"},{21,u"SkillKick"},
    {50,u"SkillIntercept"},{108,u"SkillShuriken"},{109,u"SkillCamouflage"},{80,u"SkillSneak"},
    {140,u"SkillAssassinate"},{141,u"SkillNinjutsu"},{146,u"SkillFlameRush"},
    {144,u"SkillWaterBubbles"},{136,u"SkillFireBreath"},{98,u"SkillLavaPuddle"},
    {47,u"SkillFireMissiles"},{149,u"SkillHeroicShout"},{148,u"SkillRockFist"},{81,u"SkillSprint"},
    {160,u"SkillThrowPoisonVial"},{31,u"SkillFireSwirl"},{32,u"SkillFireVortex"},
    {39,u"SkillFirebolt"},{40,u"SkillFirebolt"},{41,u"SkillFirebolt"},{38,u"SkillFireball"},
    {105,u"SkillFireRay"},{104,u"SkillFireBeam"},{33,u"SkillWaterSwirl"},{34,u"SkillWaterVortex"},
    {45,u"SkillWaterbolt"},{42,u"SkillWaterbolt"},{43,u"SkillWaterbolt"},{44,u"SkillWaterball"},
    {106,u"SkillWaterRay"},{107,u"SkillWaterBeam"},{117,u"SkillPortCity"}
};
static_assert(std::size(actions)==73);

Text utf16(std::string_view utf8) {
    const auto wide=dialoguewords::wide(std::string(utf8));
    Text result;
    for (const auto c:wide) {
        const uint32_t unit=uint32_t(c);
        if (unit>0xffff) {
            result+=char16_t(0xd800+((unit-0x10000)>>10));
            result+=char16_t(0xdc00+((unit-0x10000)&1023));
        } else result+=char16_t(unit);
    }
    return result;
}
Text entities(std::string_view input) {
    Text result;
    size_t from=0;
    while (from<input.size()) {
        const auto amp=input.find('&',from);
        if (amp==input.npos) {result+=utf16(input.substr(from)); break;}
        result+=utf16(input.substr(from,amp-from));
        const auto end=input.find(';',amp+1);
        if (end==input.npos) {result+=utf16(input.substr(amp)); break;}
        const auto key=input.substr(amp+1,end-amp-1);
        if (key=="amp") result+=u'&';
        else if (key=="lt") result+=u'<';
        else if (key=="gt") result+=u'>';
        else if (key=="quot") result+=u'"';
        else if (key=="apos") result+=u'\'';
        else if (key.starts_with('#')) {
            auto number=key.substr(1); int base=10;
            if (number.starts_with('x')) {number.remove_prefix(1); base=16;}
            uint32_t value=0;
            const auto parsed=std::from_chars(number.data(),number.data()+number.size(),value,base);
            if (parsed.ec!=std::errc{} || parsed.ptr!=number.data()+number.size() || value>0x10ffff ||
                (value>=0xd800 && value<=0xdfff)) return {};
            if (value>0xffff) {
                result+=char16_t(0xd800+((value-0x10000)>>10));
                result+=char16_t(0xdc00+((value-0x10000)&1023));
            } else result+=char16_t(value);
        } else result+=utf16(input.substr(amp,end-amp+1));
        from=end+1;
    }
    return result;
}
bool space(char c) {return c==' ' || c=='\t' || c=='\r' || c=='\n';}
struct Element {
    std::string name;
    std::map<std::string,Text> attributes;
    Text text;
    std::vector<std::unique_ptr<Element>> children;
};
// Owned UTF-8 XML adapter: preserves direct text, child and duplicate-ID order.
// It does not load external entities or claim the original imported XML provider.
bool document(std::string_view xml, Element &root) {
    std::vector<Element *> stack{&root};
    size_t at=xml.starts_with("\xef\xbb\xbf") ? 3 : 0;
    while (at<xml.size()) {
        if (xml[at]!='<') {
            const auto end=xml.find('<',at);
            stack.back()->text+=entities(xml.substr(at,end==xml.npos ? xml.size()-at : end-at));
            at=end==xml.npos ? xml.size() : end;
        } else if (xml.substr(at,4)=="<!--") {
            const auto end=xml.find("-->",at+4); if (end==xml.npos) return false; at=end+3;
        } else if (xml.substr(at,9)=="<![CDATA[") {
            const auto end=xml.find("]]>",at+9); if (end==xml.npos) return false;
            stack.back()->text+=utf16(xml.substr(at+9,end-at-9)); at=end+3;
        } else if (xml.substr(at,2)=="<?") {
            const auto end=xml.find("?>",at+2); if (end==xml.npos) return false; at=end+2;
        } else {
            ++at;
            const bool close=at<xml.size() && xml[at]=='/';
            if (close) ++at;
            const auto begin=at;
            while (at<xml.size() && !space(xml[at]) && xml[at]!='>' && xml[at]!='/') ++at;
            const std::string name(xml.substr(begin,at-begin));
            if (name.empty() || name[0]=='!') return false;
            if (close) {
                while (at<xml.size() && space(xml[at])) ++at;
                if (at==xml.size() || xml[at++]!='>' || stack.size()<2 || stack.back()->name!=name) return false;
                stack.pop_back(); continue;
            }
            auto child=std::make_unique<Element>(); child->name=name;
            bool selfClosing=false;
            while (at<xml.size()) {
                while (at<xml.size() && space(xml[at])) ++at;
                if (at==xml.size()) return false;
                if (xml[at]=='>') {++at; break;}
                if (xml[at]=='/' && at+1<xml.size() && xml[at+1]=='>') {at+=2; selfClosing=true; break;}
                const auto key=at;
                while (at<xml.size() && !space(xml[at]) && xml[at]!='=') ++at;
                const std::string attribute(xml.substr(key,at-key));
                while (at<xml.size() && space(xml[at])) ++at;
                if (at==xml.size() || xml[at++]!='=') return false;
                while (at<xml.size() && space(xml[at])) ++at;
                if (at==xml.size() || (xml[at]!='\'' && xml[at]!='"')) return false;
                const char quote=xml[at++]; const auto value=at;
                const auto end=xml.find(quote,at); if (end==xml.npos) return false;
                child->attributes[attribute]=entities(xml.substr(value,end-value)); at=end+1;
            }
            auto *node=child.get(); stack.back()->children.push_back(std::move(child));
            if (!selfClosing) stack.push_back(node);
        }
    }
    return stack.size()==1;
}

struct Node {
    enum Kind { Word, Punctuation, Choice, Optional } kind=Word;
    Text text;
    std::set<Text> conditions, categories;
    std::map<Text,Text> aliases;
    bool inherit=false;
    std::vector<std::unique_ptr<Node>> children;
    Node *append(Kind value=Word) {
        auto child=std::make_unique<Node>(); child->kind=value;
        auto *result=child.get(); children.push_back(std::move(child)); return result;
    }
};
bool punctuation(char16_t c) {return c<64 && View(u"!'(),-./:;=?").find(c)!=View::npos;}
bool separator(char16_t c) {return View(u" \n\t{}[]|").find(c)!=View::npos || c==0 || punctuation(c);}
std::unique_ptr<Node> parse(View input) {
    auto root=std::make_unique<Node>();
    std::vector<Node *> stack{root.get()};
    size_t from=0;
    for (size_t at=0;at<=input.size();++at) {
        const char16_t c=at<input.size() ? input[at] : 0;
        if (!separator(c)) continue;
        if (c==u'{' || c==u'[') {
            auto *group=stack.back()->append(c==u'{' ? Node::Choice : Node::Optional);
            stack.push_back(group);
            if (c==u'{') stack.push_back(group->append());
        }
        const View part=input.substr(from,at-from);
        const bool condition=part.starts_with(u'#'), marker=part.starts_with(u'$');
        if (condition) stack.back()->conditions.insert(Text(part));
        // Marker metadata cannot affect the selected plain-string result.
        else if (!marker && (!part.empty() || punctuation(c))) {
            auto *word=stack.back()->append(); word->text=part;
            if (part.starts_with(u'@') && c==u'(') {
                size_t arg=++at;
                for (;at<=input.size();++at) {
                    const char16_t next=at<input.size() ? input[at] : 0;
                    if (next==u')' || next==u',' || next==u' ' || next==0) {
                        const View value=input.substr(arg,at-arg);
                        if (!value.empty()) {
                            const auto equals=value.find(u'=');
                            if (equals!=View::npos)
                                word->aliases[Text(u"@")+Text(value.substr(0,equals))]=Text(u"@")+Text(value.substr(equals+1));
                            else if (value==u"*") word->inherit=true;
                            else word->categories.insert(Text(u"#")+Text(value));
                        }
                        arg=at+1;
                        if (next==u')' || next==0) break;
                    }
                }
                if (word->categories.empty()) word->categories.insert(u"#default");
                from=at+1; continue;
            }
        }
        if (!condition && !marker && punctuation(c)) {
            auto *mark=stack.back()->append(Node::Punctuation); mark->text=Text(1,c);
        }
        if (c==u'|') {
            if (stack.size()<2) return root;
            stack.pop_back(); stack.push_back(stack.back()->append());
        } else if (c==u'}') {
            if (stack.size()<3) return root;
            stack.pop_back(); stack.pop_back();
        } else if (c==u']') {
            if (stack.size()<2) return root;
            stack.pop_back();
        }
        from=at+1;
    }
    return root;
}
struct Term {
    std::unique_ptr<Node> root;
    std::vector<std::unique_ptr<Node>> variants;
};
using Terms=std::map<Text,Term,std::less<>>;
uint32_t firstStages(uint32_t seed) {seed^=seed<<13; return seed^(seed>>17);}
uint32_t mixed(uint32_t seed) {seed=firstStages(seed); return seed^(seed<<5);}
struct Resolver {
    const Terms &terms, &macros;
    std::vector<std::vector<Text>> blocks{1};
    std::vector<const Node *> active;
    void emit(View value) {
        // Same pre-extraction stream check as the existing owned word builder.
        // The current C++ wide-stream locale is an explicit provider boundary.
        const auto end=value.find(char16_t(0xffff));
        if (end!=View::npos) value=value.substr(0,end);
        std::wistringstream stream(std::wstring(value.begin(),value.end()));
        auto &words=blocks.back();
        while (stream.good()) {
            std::wstring piece; stream>>piece;
            Text word(piece.begin(),piece.end());
            if (words.empty() || words.back().empty()) words.push_back(std::move(word));
            else if (words.back()==u"=") {
                words.pop_back(); if (!words.empty()) words.back()+=word;
            } else if (words.back().back()==u'\'' || word==u"'") words.back()+=word;
            else words.push_back(std::move(word));
        }
    }
    bool visit(const Node *node,const Context &context,uint32_t seed) {
        if (!node) return true;
        const bool conditional=!node->conditions.empty();
        if (conditional && std::none_of(node->conditions.begin(),node->conditions.end(),[&](const auto &key) {
                return context.categories.contains(key);
            })) return true;
        // Native has no cycle guard. This owned finite-data policy avoids an
        // unbounded reference cycle without changing acyclic term traversal.
        if (std::find(active.begin(),active.end(),node)!=active.end()) return true;
        active.push_back(node);
        const auto finish=[&] {active.pop_back(); return !conditional;};
        const auto &text=node->text;
        if (text==u"%") blocks.emplace_back();
        else if (text.starts_with(u'@')) {
            const auto name=context.names.find(text);
            if (name!=context.names.end()) {
                if (node->categories.empty()) emit(name->second);
                else if (const auto term=terms.find(name->second);term!=terms.end()) {
                    Context next{node->categories,context.names};
                    if (node->inherit) next.categories.insert(context.categories.begin(),context.categories.end());
                    if (const auto own=context.names.find(Text(u"!")+text);own!=context.names.end()) next.names[u"@"]=own->second;
                    for (const auto &[alias,target]:node->aliases)
                        if (const auto found=context.names.find(target);found!=context.names.end()) next.names[alias]=found->second;
                    visit(term->second.root.get(),next,seed);
                }
            }
        } else if (text.starts_with(u'*')) {
            if (const auto term=macros.find(View(text).substr(1));term!=macros.end()) visit(term->second.root.get(),context,seed);
        } else if (!text.empty()) emit(text==u"%%" ? View(u"%") : View(text));
        if (node->kind==Node::Optional) {
            const auto gate=firstStages(seed); ++seed;
            if ((gate&1)==0) return finish();
        }
        std::vector<const Node *> candidates;
        for (const auto &child:node->children) {
            if (node->kind==Node::Choice && child->conditions.empty()) candidates.push_back(child.get());
            else if (!visit(child.get(),context,seed)) return finish();
        }
        if (!candidates.empty()) visit(candidates[mixed(seed)%candidates.size()],context,seed);
        return finish();
    }
    Text assemble() const {
        Text result;
        bool first=true,suppress=false;
        for (const auto &word:blocks.front()) {
            if (word==u"=") {suppress=true; continue;}
            const bool punctuationWord=word.size()==1 && View(u".-=,;!?:'").find(word[0])!=View::npos;
            if (!first && !suppress && !punctuationWord) result+=u' ';
            result+=word;
            suppress=word==u"-" || word==u"'";
            first=false;
        }
        return result;
    }
};
} // namespace
struct Dictionary::Data {Terms terms,macros;};
Dictionary::Dictionary():data(std::make_unique<Data>()) {}
Dictionary::~Dictionary()=default;
View actionToken(int32_t action) {
    static const auto index=[] {
        std::map<int32_t,View> result;
        for (const auto &entry:actions) result[entry.id]=entry.token;
        return result;
    }();
    const auto found=index.find(action);
    return found!=index.end() ? found->second : View{};
}
bool Dictionary::load(const std::filesystem::path &path,std::string &error) {
    std::ifstream file(path,std::ios::binary);
    if (!file) {error="cannot read "+path.string(); return false;}
    const std::string xml((std::istreambuf_iterator<char>(file)),{});
    if (file.bad()) {error="cannot finish reading "+path.string(); return false;}
    return loadXml(xml,error);
}
bool Dictionary::loadXml(std::string_view input,std::string &error) {
    std::string xml; xml.reserve(input.size());
    for (size_t at=0;at<input.size();++at) {
        if (input[at]=='\r') {xml+='\n'; if (at+1<input.size() && input[at+1]=='\n') ++at;}
        else xml+=input[at];
    }
    Element root;
    if (!document(xml,root) || root.children.size()!=1) {error="unsupported or malformed term XML"; return false;}
    auto next=std::make_unique<Data>();
    for (const auto &element:root.children.front()->children) {
        if (element->name!="speech" && element->name!="name" && element->name!="macro") continue;
        const auto id=element->attributes.find("id");
        if (id==element->attributes.end() || id->second.empty()) continue;
        Term term; term.root=parse(element->text);
        if (element->name!="macro")
            for (const auto &variant:element->children)
                if (!variant->text.empty()) term.variants.push_back(parse(variant->text));
        (element->name=="macro" ? next->macros : next->terms)[id->second]=std::move(term);
    }
    data=std::move(next); error.clear(); return true;
}
size_t Dictionary::size() const {return data->terms.size();}
Text Dictionary::format(View token,const Context &context,uint32_t seed) const {
    Resolver output{data->terms,data->macros};
    if (token.starts_with(u'!')) output.blocks.front().push_back(Text(token));
    else if (token.starts_with(u'*')) output.emit(token);
    else if (const auto term=data->terms.find(token);term!=data->terms.end()) {
        const auto &variants=term->second.variants;
        const Node *node=term->second.root.get();
        if (!variants.empty()) {node=variants[mixed(seed)%variants.size()].get(); ++seed;}
        output.visit(node,context,seed);
    }
    return output.assemble();
}
Text Dictionary::describe(int32_t action) const {
    return format(actionToken(action),{{u"#default"},{{u"@",u""}}});
}
} // namespace nativeterms
