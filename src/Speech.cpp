#include "Speech.h"
#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

namespace {
std::string unescape(std::string s) {
    static const std::pair<const char *, const char *> entities[] = {
        {"&amp;", "&"}, {"&lt;", "<"}, {"&gt;", ">"}, {"&apos;", "'"}, {"&quot;", "\""}};
    for (auto &[from, to] : entities)
        for (size_t p; (p = s.find(from)) != std::string::npos;)
            s.replace(p, std::strlen(from), to);
    return s;
}
std::string attribute(const std::string &tag, const char *name) {
    std::string key = std::string(name) + "=\"";
    size_t p = tag.find(key);
    if (p == std::string::npos)
        return "";
    p += key.size();
    return tag.substr(p, tag.find('"', p) - p);
}
bool identChar(char c) { return std::isalnum((unsigned char)c) || c == '_' || c == ':' || c == '-'; }
// The extent of a bracketed group starting at text[at] (an opening bracket):
// the index of its matching closing bracket.
size_t closing(const std::string &text, size_t at) {
    int depth = 0;
    for (size_t i = at; i < text.size(); i++) {
        char c = text[i];
        if (c == '{' || c == '[' || c == '(')
            depth++;
        else if (c == '}' || c == ']' || c == ')') {
            if (--depth == 0)
                return i;
        }
    }
    return text.size();
}
// Alternatives of a { a | b } group (between its brackets), split at its own level.
std::vector<std::string> alternatives(const std::string &inner) {
    std::vector<std::string> out(1);
    int depth = 0;
    for (char c : inner) {
        if (c == '{' || c == '[' || c == '(')
            depth++;
        else if (c == '}' || c == ']' || c == ')')
            depth--;
        if (c == '|' && depth == 0)
            out.emplace_back();
        else
            out.back() += c;
    }
    return out;
}
// The #tags an alternative depends on (its own level only).
std::vector<std::string> tagsOf(const std::string &alternative) {
    std::vector<std::string> tags;
    int depth = 0;
    for (size_t i = 0; i < alternative.size(); i++) {
        char c = alternative[i];
        if (c == '{' || c == '[' || c == '(')
            depth++;
        else if (c == '}' || c == ']' || c == ')')
            depth--;
        else if (c == '#' && depth == 0) {
            size_t j = i + 1;
            while (j < alternative.size() && identChar(alternative[j]))
                j++;
            tags.push_back(alternative.substr(i + 1, j - i - 1));
            i = j - 1;
        }
    }
    return tags;
}
// Spaces collapsed, none before punctuation, the first letter capitalised.
std::string tidy(const std::string &s) {
    std::string out;
    for (char c : s) {
        if (std::isspace((unsigned char)c)) {
            if (!out.empty() && out.back() != ' ')
                out += ' ';
            continue;
        }
        if ((c == '.' || c == ',' || c == '!' || c == '?') && !out.empty() && out.back() == ' ')
            out.pop_back();
        out += c;
    }
    while (!out.empty() && out.back() == ' ')
        out.pop_back();
    if (!out.empty())
        out[0] = (char)std::toupper((unsigned char)out[0]);
    return out;
}
} // namespace

bool Speech::load(const fs::path &dictionary, std::string &error) {
    std::ifstream in(dictionary, std::ios::binary);
    if (!in) {
        error = "cannot read " + dictionary.string();
        return false;
    }
    std::ostringstream all;
    all << in.rdbuf();
    std::string xml = all.str();
    for (const char *kind : {"speech", "macro", "name"}) {
        std::string open = std::string("<") + kind + " ", close = std::string("</") + kind + ">";
        for (size_t p = 0; (p = xml.find(open, p)) != std::string::npos;) {
            size_t tagEnd = xml.find('>', p), end = xml.find(close, tagEnd);
            if (tagEnd == std::string::npos || end == std::string::npos)
                break;
            std::string tag = xml.substr(p, tagEnd - p), body = unescape(xml.substr(tagEnd + 1, end - tagEnd - 1));
            std::string id = attribute(tag, "id");
            if (std::string(kind) == "name")
                names[id] = {body, attribute(tag, "tags")};
            else
                lines[id] = body;
            p = end + close.size();
        }
    }
    if (lines.empty()) {
        error = "no speech found in " + dictionary.string();
        return false;
    }
    return true;
}

std::string Speech::expand(const std::string &text, size_t &at, char until, const Context &context,
                           std::mt19937 &random, int depth) const {
    std::string out;
    if (depth > 24)
        return out; // a rule that refers to itself
    while (at < text.size() && text[at] != until) {
        char c = text[at];
        if (c == '{') {
            size_t end = closing(text, at);
            auto alts = alternatives(text.substr(at + 1, end - at - 1));
            // The first alternative whose tags all hold; else one without tags, at random.
            const std::string *chosen = nullptr;
            std::vector<const std::string *> free;
            for (auto &a : alts) {
                auto tags = tagsOf(a);
                if (tags.empty())
                    free.push_back(&a);
                else if (!chosen) {
                    bool all = true;
                    for (auto &t : tags)
                        all = all && context.tags.count(t);
                    if (all)
                        chosen = &a;
                }
            }
            if (!chosen && !free.empty())
                chosen = free[std::uniform_int_distribution<size_t>(0, free.size() - 1)(random)];
            if (chosen) {
                size_t inner = 0;
                out += expand(*chosen, inner, '\0', context, random, depth + 1);
            }
            at = end + 1;
        } else if (c == '[') {
            size_t end = closing(text, at);
            if (std::uniform_int_distribution<int>(0, 1)(random)) {
                std::string inner = text.substr(at + 1, end - at - 1);
                size_t i = 0;
                out += expand(inner, i, '\0', context, random, depth + 1);
            }
            at = end + 1;
        } else if (c == '*' || c == '@' || c == '$' || c == '#') {
            size_t j = at + 1;
            while (j < text.size() && identChar(text[j]))
                j++;
            std::string id = text.substr(at + 1, j - at - 1);
            at = j;
            if (c == '*') {
                auto it = lines.find(id);
                if (it != lines.end()) {
                    size_t inner = 0;
                    out += expand(it->second, inner, '\0', context, random, depth + 1);
                } else
                    out += name(id);
            } else if (c == '@') {
                std::string args;
                bool possessive = false;
                if (at < text.size() && text[at] == '(') {
                    size_t end = closing(text, at);
                    args = text.substr(at + 1, end - at - 1);
                    at = end + 1;
                }
                if (text.compare(at, 2, "=s") == 0)
                    possessive = true, at += 2;
                std::string value = context.value ? context.value(id, args) : "";
                if (value.empty())
                    context.missing = true;
                out += value;
                if (possessive && !value.empty())
                    out += value.back() == 's' ? "'" : "'s";
            }
            // $colour and #condition tags print nothing.
        } else {
            out += c;
            at++;
        }
    }
    return out;
}

std::string Speech::say(const std::string &id, const Context &context, std::mt19937 &random) const {
    auto it = lines.find(id);
    if (it == lines.end())
        return "";
    size_t at = 0;
    return tidy(expand(it->second, at, '\0', context, random, 0));
}

std::string Speech::name(const std::string &id, bool plural) const {
    auto it = names.find(id);
    if (it == names.end())
        return "";
    Context context;
    if (plural)
        context.tags.insert("plural");
    std::mt19937 fixed(0);
    size_t at = 0;
    std::string s = expand(it->second.first, at, '\0', context, fixed, 0);
    // Names keep their own capitals; only spaces are tidied.
    std::string out;
    for (char c : s) {
        if (std::isspace((unsigned char)c)) {
            if (!out.empty() && out.back() != ' ')
                out += ' ';
        } else
            out += c;
    }
    while (!out.empty() && out.back() == ' ')
        out.pop_back();
    return out;
}

std::string Speech::text(const std::string &key, const std::set<std::string> &tags, const std::map<std::string, std::string> &macros) const {
    const std::string *body = nullptr;
    if (auto it = names.find(key); it != names.end())
        body = &it->second.first;
    else if (auto line = lines.find(key); line != lines.end())
        body = &line->second;
    if (!body)
        return key;
    Context context;
    context.tags = tags;
    context.value = [&](const std::string &fn, const std::string &args) -> std::string {
        auto it = macros.find(fn);
        if (it == macros.end())
            return "";
        if (args.empty())
            return it->second;
        std::set<std::string> passed;
        std::string arg;
        std::istringstream in(args);
        while (std::getline(in, arg, ',')) {
            arg.erase(0, arg.find_first_not_of(' '));
            arg.erase(arg.find_last_not_of(' ') + 1);
            if (arg == "*")
                passed.insert(tags.begin(), tags.end());
            else if (!arg.empty())
                passed.insert(arg);
        }
        return hasEntry(it->second) ? text(it->second, passed, macros) : it->second;
    };
    std::mt19937 fixed(0);
    size_t at = 0;
    const std::string s = expand(*body, at, '\0', context, fixed, 0);
    std::string out;
    for (char c : s) {
        if (std::isspace((unsigned char)c)) {
            if (!out.empty() && out.back() != ' ')
                out += ' ';
        } else
            out += c;
    }
    while (!out.empty() && out.back() == ' ')
        out.pop_back();
    return out;
}

std::string Speech::nameTags(const std::string &id) const {
    auto it = names.find(id);
    return it == names.end() ? "" : it->second.second;
}

std::string Speech::place(const std::string &id, const std::string &proper) const {
    auto it = names.find(id);
    if (it == names.end())
        return proper;
    Context context;
    context.tags.insert("named");
    context.value = [&](const std::string &fn, const std::string &) { return fn.empty() ? proper : std::string(); };
    std::mt19937 fixed(0);
    size_t at = 0;
    std::string s = expand(it->second.first, at, '\0', context, fixed, 0), out;
    for (char c : s)
        if (!std::isspace((unsigned char)c) || (!out.empty() && out.back() != ' '))
            out += std::isspace((unsigned char)c) ? ' ' : c;
    while (!out.empty() && out.back() == ' ')
        out.pop_back();
    return out;
}
