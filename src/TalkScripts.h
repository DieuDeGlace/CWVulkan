#pragma once
// What townspeople say, as the game itself decides it. The game builds each
// conversation as a script (RVA 0x2d9480, its townsperson part at 0x12a6f0): speech
// ids and commands, from the person's id and race, the world's day and the player's
// gear. CubeVulkan builds the same scripts itself (TalkGen.h); ZoneExtractor can also
// ask the game for them as townspeople load and writes them to Extract/talk-scripts.txt,
// with the game's names for the people, places and items they mention
// (docs/dialogue.md "The game's conversations").
//
// A script is a list of tokens:
//   <speech id>                  words from dict_en.xml (a whole "%" node starts a page);
//                                one ending in "?" is an answer the player may choose
//   new_page                     a deferred break before the next speech
//   !alert, !love, !sweat ...    an emote over the speaker's head
//   #tag                         a grammar tag for the next line ("#inn", "#kindness")
//   define_location x y type     a place: 1024-block cell (x, y) and its kind (Rock, Rocks...)
//   define_direction dx dy       which way it is from the speaker, in blocks
//   define_item cat id modifier material rarity   an item
//   define_mob id race           a person (their name is @mob_name)
//   define_model kind            a kind of creature (@mob); negative kinds are special models
//   define_realm kind realm      a realm (@realm)
//   define_land biome seed       a land (@land): the region's biome and generated name
//   define_ruler title           a ruler's title, 0..25 (@ruler)
//   define_number n              a number (@number); define_number2 the second (@number2)
//   define_mob2 id race          a second person (@mob2_name), as the lore tales name a killer
//   define_model2 kind           a second kind of creature (@mob2)
//   define_name text             a name (@name): the next token is the text itself
//   define_faction n             a faction, 1..8 (its names are not made yet)
//   define_specialization class spec   a specialisation (@specialization)
//   mark_location x y            the place is shown on the player's map
// The exporter adds name=, item= and key= right after the command they belong to.
#include "Speech.h"
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <random>
#include <string>
#include <vector>

struct TalkScript {
    int64_t id = 0;
    int race = 0, day = 0;
    std::string speaker; // the game's name for them
    std::vector<std::string> tokens;
};

class TalkScripts {
  public:
    // Reads the exporter's file, which holds each person's script for a run of days
    // (the latest export of a day wins). False (with a reason) when there is none.
    bool load(const std::filesystem::path &file, std::string &error);
    bool parse(const std::string &text); // the same, from text (tests)
    // The person's script for the world's day. If that day wasn't exported, the
    // nearest exported day's, with exact false: the game would say something else then.
    const TalkScript *find(int64_t id, int day, bool *exact = nullptr) const;
    const TalkScript *find(int64_t id) const; // any day (the latest exported)
    size_t count() const { return scripts.size(); } // people
    // The days exported for anyone, first and last (0, -1 when none).
    std::pair<int, int> days() const;

  private:
    std::map<int64_t, std::map<int, TalkScript>> scripts; // id -> day -> script
};

// The kind of place for a define_location type (RVA 0x16b390): its <name> in the
// dictionary ("Rock", "Rocks", "Village"...), empty if the game has none.
const char *locationKind(int type);
// The kind of land for a define_land biome: its <name> in the dictionary ("Plains",
// "Woodlands"...), empty for none.
const char *landKind(int biome);

// The names a script needs when it does not carry the exporter's (a script TalkGen made).
// Each may be unset; the exporter's names win where a script has them.
struct TalkNames {
    std::function<std::string(int x, int y, int kind)> place;   // a place cell's name ("Jakor Rocks")
    std::function<std::string(int id, int race)> person;        // a person's name
    std::function<std::string(int biome, int seed)> land;       // a land's proper name ("Ikotar")
    // An item's name ("well-made Sword") and dictionary key ("Sword"), from define_item's five
    // numbers (category, id, modifier, material, rarity).
    std::function<void(const int (&item)[5], std::string &name, std::string &key)> item;
};

struct TalkPlay {
    std::vector<std::string> lines;   // what the person says, one line per press
    std::vector<std::string> choices; // the answers offered to the player ("?" tokens), as said
    std::vector<std::string> choiceIds; // ... and their speech ids ("rest_inn?")
    struct Marked {
        int x = 0, y = 0;   // 1024-block cell
        std::string place;   // "Jakor Rocks"
        std::string item;    // what is there, if the script named one
    };
    struct Page {
        std::string text, choiceId;
        std::vector<std::string> emotes;
        std::vector<Marked> marks;
    };
    std::vector<Page> pages; // Ordered logical pages, before choices are separated.
    std::vector<std::vector<Marked>> lineMarks; // Effects aligned with ordinary lines.
    std::vector<Marked> marked; // places marked on the map
    std::string unknown;        // a token CubeVulkan cannot play yet (empty: all played)
    // What could not be said in full: unknown tokens, and lines missing a value CubeVulkan
    // cannot name yet ("their_ruler_was_famous: @ruler").
    std::vector<std::string> unrendered;
};
TalkPlay playScript(const TalkScript &script, const Speech &speech, std::mt19937 &random, const TalkNames *names = nullptr);
