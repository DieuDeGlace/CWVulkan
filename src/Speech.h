#pragma once
// The game's own dialogue: dict_en.xml holds every line the game says, written
// in a small grammar, and the game composes sentences from it. This expands
// that grammar the same way, so NPCs and quests speak the game's words:
//   { a | b | c }   one of the alternatives (at random)
//   #tag            inside an alternative: only when the tag holds ("#mob_f she | #mob_m he | it")
//   [ x ]           optional
//   *id             another <speech> or <macro>
//   @fn(args)       a value from the caller: @mob(), @mob_name, @zone(named), @item()...
//   $tag            the colour a part is shown in ($creature, $item, $zone, $name, $stress)
// <name id> entries give names, with "{ #plural Wizards | Wizard }" for plurals.
#include "DialogueWords.h"
#include <optional>
#include <filesystem>
#include <functional>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

class Speech {
  public:
    struct Context {
        std::set<std::string> tags; // e.g. "mob_m", "mob_an", "zone_the", "east", "plural"
        // Values for @fn(args): fn "mob", args "plural" -> "Wizards". Empty: nothing.
        std::function<std::string(const std::string &fn, const std::string &args)> value;
        // Set when a line asked for a value the caller did not have (the line is incomplete).
        mutable bool missing = false;
        std::function<std::optional<glm::vec4>(const std::string &fn, const std::string &args)> colour;
    };
    bool load(const std::filesystem::path &dictionary, std::string &error);
    bool has(const std::string &id) const { return lines.count(id) > 0; }
    // A sentence for <speech id> (or <macro id>), spaces and punctuation tidied.
    std::string say(const std::string &id, const Context &context, std::mt19937 &random) const;
    // Expanded dialogue pages. Whole-text % nodes break pages; %% is a literal percent.
    std::vector<std::string> sayPages(const std::string &id, const Context &context, std::mt19937 &random) const;
    struct StyledPage {
        std::string text;
        std::vector<dialoguewords::Word> words;
    };
    std::vector<StyledPage> sayStyledPages(const std::string &id, const Context &, std::mt19937 &,
                                          std::vector<dialoguewords::Word> initial = {}) const;
    // A <name id> ("Wizard"), or its plural.
    std::string name(const std::string &id, bool plural = false) const;
    // A place: the <name> template for its kind ("Plains": "{@ #named} Plains") with
    // `proper` as its own name ("Ikotar Plains").
    std::string place(const std::string &id, const std::string &proper) const;
    // The tags given with a <name> ("m an": male, takes "an").
    std::string nameTags(const std::string &id) const;
    size_t count() const { return lines.size(); }
    // the text generation: an entry (<name> or <speech>) with these tags and macros
    // ("name" for @name). A macro called with arguments, "@materialitem(*,item)", names
    // another entry, built with those tags ("*" also passing the caller's); one called
    // without is written as it is.
    std::string text(const std::string &key, const std::set<std::string> &tags, const std::map<std::string, std::string> &macros) const;
    bool hasEntry(const std::string &id) const { return names.count(id) || lines.count(id); }

  private:
    std::map<std::string, std::string> lines; // speech and macro bodies
    std::map<std::string, std::pair<std::string, std::string>> names; // id -> body, tags
    std::string expand(const std::string &text, size_t &at, char until, const Context &context,
                       std::mt19937 &random, int depth, bool pages = false,
                       dialoguewords::Builder *words = nullptr, dialoguewords::Style style = {}) const;
};
