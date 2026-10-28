#include "TalkGen.h"
#include "worldgen/Generator.h"
#include "worldgen/Residents.h"

#include <algorithm>
#include <cstring>

namespace talkgen {

uint32_t xorshift(uint32_t n) {
    n ^= n << 13;
    n ^= n >> 17;
    n ^= n << 5;
    return n;
}

uint32_t scriptSeed(uint64_t id, int day) { return xorshift((uint32_t)(id % 333333u) + 13u * (uint32_t)day); }

Speaker speakerOf(const uint8_t *record) {
    namespace nc = worldgen::creature;
    Speaker s;
    auto read = [&](size_t offset, auto &field) { std::memcpy(&field, record + offset, sizeof field); };
    read(nc::ID, s.id);
    read(nc::POSITION, s.x);
    read(nc::POSITION + 8, s.y);
    s.hostility = record[nc::HOSTILITY];
    read(nc::RACE, s.race);
    read(nc::FLAGS, s.appearance);
    read(nc::TOGGLES, s.toggles);
    s.cls = record[nc::CLASS];
    s.stage = record[nc::STAGE];
    read(nc::HOME, s.homeX);
    read(nc::HOME + 8, s.homeY);
    return s;
}

bool femaleRace(int race) { return (race >= 0 && race < 17 && ((0x1552au >> race) & 1)) || race == 84 || race == 228; }

std::vector<int> siblingGroup(int race) {
    // Races that come as a family, each in the game's order (0x13f0d0).
    static const std::vector<std::vector<int>> families = {
        {0, 1},   {2, 3},           {4, 5},     {7, 8},     {9, 10},    {11, 12},   {13, 14},  {15, 16},  {22, 23},
        {40, 38, 39, 37}, {77, 78}, {83, 84},   {163, 164}, {202, 203, 208},        {227, 228}, {236, 237}, {260, 261}};
    // A special model that stands for one of five creatures.
    if (race == -99991)
        return {243, 143, 204, 280, 242};
    for (const std::vector<int> &family : families)
        if (std::find(family.begin(), family.end(), race) != family.end())
            return family;
    return {race};
}

namespace {

// --- The game's tables (spec 4, spec 6) -----------------------------------------------------------

const char *const CLASS_NAMES[5] = {"", "warrior", "ranger", "mage", "rogue"};

// What a village's people say they are there for (spec 6.1), a grammar tag for the next line.
const char *const ACTIVITY_TAGS[15] = {"#glassflasks", "#sugarcubes", "#bombs",     "#petfood",  "#flight",
                                       "#inn",         "#buyweapon",  "#craftweapon", "#buyarmor", "#craftarmor",
                                       "#identify",    "#specialization", "#villagers", "#friends", "#sell"};
// A famous ruler's magic (spec 6.4) and traits (spec 6.5, spec 6.6).
const char *const MAGIC_TAGS[15] = {"#rune_magic",         "#blood_magic",           "#gem_magic",      "#dark_magic",
                                    "#black_magic",        "#unholy_magic",          "#holy_magic",     "#divine_magic",
                                    "#life_magic",         "#death_magic",           "#mana_transformation",
                                    "#matter_transformation", "#mana_infusion",      "#particle_fusion", "#advanced_alchemy"};
const char *const GOOD_TRAITS[10] = {"#kindness", "#friendliness", "#goodness", "#gentleness", "#honesty",
                                     "#pureness", "#loyalty",      "#fairness", "#generosity", "#happyness"};
const char *const BAD_TRAITS[9] = {"#brutality",    "#cruelty", "#harshness", "#relentlessness", "#unmercifulness",
                                   "#ruthlessness", "#badmood", "#badness",   "#viciousness"};
// The male races of the people (elf, human, goblin, lizardman, dwarf, orc, frogman, undead):
// rulers and nations are drawn from them, the next race being the female.
const int PEOPLE_RACES[8] = {0, 2, 4, 7, 9, 11, 13, 15};

// The creature lists of the "strong creatures" small talk (spec 5.7), in the game's order.
const std::vector<int> BEASTS = {226, 85, 73, 229, 225, 227};
const std::vector<int> GOLEMS = {70, 69, 287, 71, 291, 290};
const std::vector<int> UNDEAD = {96, 17, 165, 41, 269, 97, 230, 173};

// A line and the emote after it (or none).
struct Remark {
    const char *line;
    const char *emote = nullptr;
};
// What people say about the player's special items (spec 4.6), and in small talk (spec 4.10).
const Remark GLIDER_REMARKS[5] = {{"how_does_it_feel_to_fly"}, {"i_wish_i_could_fly"},
                                  {"arent_you_afraid_of_heights", "!sweat"}, {"how_far_can_you_fly"},
                                  {"how_high_can_you_fly"}};
const Remark CLIMBING_REMARKS[3] = {{"whats_the_tallest_mountain_that_you_climbed"}, {"be_careful_during_climbing"},
                                    {"arent_you_afraid_of_heights", "!sweat"}};
const Remark REINS_REMARKS[5] = {{"whats_your_favorite_pet_to_ride"}, {"i_wish_i_could_ride"},
                                 {"i_would_ride_through_the_land"}, {"how_fast_can_you_ride"}, {"where_will_you_ride_next"}};
// Sailing: the boat remark takes the first four (r & 3); the small talk all five (r mod 5).
const Remark SAILING_REMARKS[5] = {{"i_wonder_whats_on_the_other_side_of_the_ocean"}, {"i_wish_you_a_safe_journey"},
                                   {"id_love_to_sail_from_island_to_island"}, {"my_relative_was_a_sailor"},
                                   {"i_dont_have_a_boat"}};
const char *const NATURE_LOVES[6] = {"i_love_nature", "i_love_the_beach",  "i_love_the_river",
                                     "i_love_the_lake", "i_love_the_forest", "i_love_the_mountains"};

// The special item (category 24) a help request of role 2-13 is about (spec 4.6).
int specialItemOfRole(int role) {
    static const int ids[12] = {14, 15, 16, 17, 1, 2, 18, 3, 5, 7, 8, 9};
    return ids[role - 2];
}
enum SpecialItem { HANG_GLIDER = 1, BOAT = 2, REINS = 3, HARP = 5, SKY_WHISTLE = 7, SPIRIT_BELL = 8, CLIMBING_SPIKES = 18 };
bool craftingBook(int id) { return id >= 14 && id <= 17; }

// A mission resident's task by its place's encounter (spec 4.4): the creatures at the place (0 for
// none), which line says so, and the objects to destroy (0 for none; -1 for the three kinds
// of the place's own choice).
struct ResidentTask {
    int encounter, creatures;
    const char *fullOf;
    int objects;
};
const ResidentTask RESIDENT_TASKS[] = {
    {2, -99996, "the_location_is_full_of_creatures", -1},
    {3, -99997, "the_location_is_full_of_creatures", 175},
    {4, 96, "the_location_is_full_of_creatures", 178},
    {5, 17, "the_location_is_full_of_creatures", 179},
    {6, 37, "the_location_is_full_of_creatures", 176},
    {7, 87, "the_location_is_full_of_creatures", 180},
    {8, 53, "the_location_is_full_of_creatures", 177},
    {18, 37, "the_location_is_full_of_creatures", 0},
    {19, -99994, "the_location_is_full_of_creatures", 0},
    {20, -99995, "the_location_is_full_of_creatures", 0},
    {21, -99996, "the_location_is_full_of_creatures", 0},
    {22, 0, "the_location_is_full_of_insects", 0},
    {23, 0, "the_location_is_full_of_plant_creatures", 0},
};

// Who holds a missing gnome, by the place's encounter (spec 4.6 role 1).
int captorModel(int encounter) {
    switch (encounter) {
    case 10: case 14: return 46;
    case 11: return 45;
    case 12: return 17;
    case 13: return 269;
    case 15: return 227;
    case 16: return 226;
    default: return -99993;
    }
}

// --- Numbers and places ---------------------------------------------------------------------

// The world's integer noise R(n) (spec 2.4, worldgen::random): 0..100000, n built in wrapping 32-bit arithmetic.
uint32_t noise(uint32_t n) { return worldgen::random(n); }
// The noise of a place cell with a salt: R(x + 57y + salt).
uint32_t cellNoise(Cell c, uint32_t salt) { return noise((uint32_t)c[0] + 57u * (uint32_t)c[1] + salt); }

// Blocks from a position in 1/65536 blocks, rounded toward zero.
int32_t blocksOf(int64_t dots) { return (int32_t)(dots / 65536); }
// The 1024-block place cell of a block coordinate, and rounding a cell to even (floor both).
int32_t placeCellOf(int32_t block) { return worldgen::floorDiv(block, 10); }
int32_t evenCell(int32_t c) { return worldgen::floorDiv(c, 1) * 2; }

bool bigAt(Cell c) { return worldgen::bigCell(c[0], c[1]); }
int kindAt(Cell c) { return worldgen::placeKind(c[0], c[1], bigAt(c)); }

// --- The builder ----------------------------------------------------------------------------

class Builder {
  public:
    Builder(const Speaker &speaker, const Player &player, const World &world, CRand &rand)
        : speaker(speaker), player(player), world(world), runtime(rand) {}

    std::vector<std::string> build() {
        if (speaker.cls == 155)
            specialisationTrainer();
        else if (speaker.cls == 132)
            innkeeper();
        else
            townsperson();
        return std::move(out);
    }
    const std::string &branchTaken() const { return branch; }

  private:
    const Speaker &speaker;
    const Player &player;
    const World &world;
    CRand &runtime; // the C runtime's rand(), shared with the rest of the session
    std::vector<std::string> out;
    std::string branch; // which part spoke (for tests)
    uint32_t c = 0; // the script's seed (spec 2.2)

    // The speaker's home (spec 4): its place cell H, whether that is big, and H' (H rounded to
    // even when big); the planner of its region.
    Cell home{}, homePlace{};
    bool homeBig = false;
    worldgen::Int2 region;
    std::vector<Cell> helpTargets, artifactPlaces; // the planner's lists (spec 1.5)
    Cell specialTown{};

    // --- Draws and output ---
    void say(const char *token) { out.emplace_back(token); }
    void say(const std::string &token) { out.push_back(token); }
    void number(int32_t value) { out.push_back(std::to_string(value)); }
    void say(const Remark &r) {
        say(r.line);
        if (r.emote)
            say(r.emote);
    }
    void defineItem(const worldgen::Item &item) { // spec 7.1: material before rarity
        say("define_item");
        number(item.category);
        number(item.id);
        number((int32_t)item.modifier);
        number(item.material);
        number(item.rarity);
    }
    void defineSpecial(int id) { // a special item: category 24, the rest 0
        worldgen::Item item;
        item.category = 24, item.id = id;
        defineItem(item);
    }
    void defineModel(int kind) {
        say("define_model");
        number(kind);
    }
    void defineLocation(Cell place, int kind) {
        say("define_location");
        number(place[0]);
        number(place[1]);
        number(kind);
    }
    void mark(Cell place) {
        say("mark_location");
        number(place[0]);
        number(place[1]);
    }

    // --- The player's missions (each false when the caller gave no answer) ---
    bool finished(Cell place) const { return player.finished && player.finished(place); }
    bool completed(Cell place) const { return player.completed && player.completed(place); }
    bool known(Cell place) const { return player.known && player.known(place); }
    bool hasSpecial(int id) const { return id == 4 || (player.hasSpecial && player.hasSpecial(id)); }
    bool carries(const worldgen::Item &item) const { return player.carries && player.carries(item); }

    // A creature kind from the world's pet list (0x12a160).
    int pet(uint32_t n) const {
        // spec: open point: the game divides by the list's length; with no list, 0.
        return world.pets.empty() ? 0 : world.pets[xorshift(n) % world.pets.size()];
    }
    // A relative of a race (spec 5.6), drawn from the session's C rand().
    int siblingRace(int race) {
        const std::vector<int> group = siblingGroup(race);
        return group[(size_t)runtime.next() % group.size()];
    }

    // --- spec 3: the two special speakers ---

    void specialisationTrainer() {
        branch = "trainer";
        say("hello");
        say("how_can_i_help_you");
        say("define_specialization");
        number((uint8_t)player.cls);
        number(1 - (uint8_t)player.spec);
        say("change_specialization?");
        say("goodbye?");
    }

    void innkeeper() {
        branch = "innkeeper";
        say("define_number");
        number(10); // a night's price in gold
        say("hello");
        say("new_page");
        const float evening = 64800000.0f, morning = 21600000.0f; // 18:00 and 6:00
        if (world.timeOfDayMs >= evening || world.timeOfDayMs <= morning) {
            if (player.gold < 10)
                say("you_dont_have_enough_gold_to_rest_here");
            else {
                say("how_can_i_help_you");
                say("rest_inn?");
                say("goodbye?");
            }
        } else
            say("you_can_rest_here_between_six_and_six");
    }

    // --- spec 4: everyone else ---

    void townsperson() {
        const int32_t homeX = blocksOf(speaker.homeX), homeY = blocksOf(speaker.homeY);
        region = worldgen::regionAt(homeX, homeY);
        const worldgen::RegionRoles planner = worldgen::regionRoles(region);
        for (const auto &[x, y, role] : planner.roles) {
            // Everyone asks for help with any place but these (in practice roles 1-12, 16, 18-20).
            const bool notAsked = role == 0 || role == 13 || role == 14 || role == 15 || role == 17 || role == 21 ||
                                  role == 22 || role == 23;
            if (!notAsked)
                helpTargets.push_back({x, y});
            if (role == 14)
                artifactPlaces.push_back({x, y});
        }
        specialTown = {planner.special.x, planner.special.y};
        home = {placeCellOf(homeX), placeCellOf(homeY)};
        homeBig = bigAt(home);
        homePlace = homeBig ? Cell{evenCell(home[0]), evenCell(home[1])} : home;

        c = scriptSeed(speaker.id, world.day);
        if (speaker.toggles & 0x2) { // spec 4.1: calling for help
            branch = "alarm call";
            uint32_t g = c;
            alarmCall(g);
            return;
        }
        if ((uint32_t)(speaker.race - 257) < 2) { // spec 4.2
            branch = "races 257-258";
            say("hello");
            return;
        }
        if (speaker.cls == 161) { // spec 4.3
            branch = "arena master";
            arenaMaster();
            return;
        }
        if (speaker.hostility == 3 && (speaker.appearance & 0x2000)) {
            missionResident();
            return;
        }
        if (artifactRumour() || helpRequest())
            return;
        uint32_t u = greetingStep(), v = 0, w = 0;
        if (adventurerRemarks(u, v) || villageActivity(v, w))
            return;
        smallTalk(w);
    }

    // spec 5.1: hello, perhaps a name and "nice to meet you"; the counter moves on by two.
    void greeting(uint32_t &g) {
        say("hello");
        if ((xorshift(g) & 3) == 0) {
            say("define_mob");
            number((int32_t)(uint32_t)speaker.id);
            number(speaker.race);
            say("my_name_is");
        }
        if ((xorshift(g + 1) & 3) == 0) {
            say("new_page");
            say("nice_to_meet_you");
        }
        g += 2;
    }

    // spec 5.2: a call for help; the counter moves on by three.
    void alarmCall(uint32_t &g) {
        if (odd(g))
            say("!alert");
        if (odd(g + 1)) {
            say("hey_you");
            say("new_page");
        }
        say("help");
        say(odd(g + 2) ? "!sweat" : "!angry");
        g += 3;
    }

    void arenaMaster() {
        if (speaker.stage == 0xff) {
            say("congratulations");
            if (odd(c)) {
                say("new_page");
                say("good_work");
            }
        } else
            say("a_new_challenger_entered_the_arena");
    }

    // spec 4.4: a resident of a mission place: its task, or thanks once it is done.
    void missionResident() {
        const int kind = worldgen::placeKind(homePlace[0], homePlace[1], homeBig);
        if (!(speaker.toggles & 0x100) && !finished(homePlace)) {
            branch = "resident: task";
            defineLocation(homePlace, kind);
            const int encounter = worldgen::plannerEncounter(region, homePlace[0], homePlace[1], world.day);
            for (const ResidentTask &task : RESIDENT_TASKS) { // any other encounter: nothing more
                if (task.encounter != encounter)
                    continue;
                uint32_t g = c;
                alarmCall(g);
                say("new_page");
                if (task.creatures)
                    defineModel(task.creatures);
                say(task.fullOf);
                say("new_page");
                if (task.objects) {
                    // 0x12a690: one of three kinds of object, by the place
                    defineModel(task.objects > 0 ? task.objects : 239 + (int)(cellNoise(homePlace, 0x1c52c9) % 3));
                    say("you_must_destroy_the_objects");
                } else
                    say("you_must_defeat_them");
                break;
            }
            return;
        }
        branch = "resident: thanks";
        say("thank_you");
        if (odd(c)) {
            say("new_page");
            say("good_work");
        }
        if (odd(c + 1))
            say("!love");
    }

    // spec 4.5: one draw in eight tells of an artifact of the home's realm.
    bool artifactRumour() {
        if ((xorshift(c) & 7) != 0)
            return false;
        const uint32_t realm = worldgen::placeStyle(home[0], home[1]);
        std::vector<Cell> candidates;
        for (const Cell &place : artifactPlaces)
            if (worldgen::placeStyle(place[0], place[1]) == realm)
                candidates.push_back(place);
        if (candidates.empty())
            return false;
        const Cell target = candidates[xorshift(c + 1) % candidates.size()];
        const worldgen::Item item = worldgen::placeArtifact(target[0], target[1]);
        artifactTale(realm, item, c + 1, carries(item));
        return true;
    }

    // spec 5.3: the tale of an artifact, in the villagers' (class 138) or everyone else's words.
    void artifactTale(uint32_t realm, const worldgen::Item &item, uint32_t s, bool carried) {
        branch = std::string(speaker.cls == 138 ? "artifact rumour, villager" : "artifact rumour") + (carried ? ", carried" : "");
        out.clear();
        defineRealm(realm);
        defineItem(item);
        if (speaker.cls == 138) {
            say(odd(s) ? "there_is_a_mighty_artifact" : "there_is_a_legend_about_an_artifact");
            const bool b = odd(s + 1);
            if (!carried) {
                if (b) {
                    say("!question");
                    say("i_wonder_if_it_really_exists");
                } else
                    say("are_you_looking_for_it");
                return;
            }
            say(b ? "!alert" : "!question");
            say("you_found_the_artifact");
            say("new_page");
            say("nice");
            if (odd(s + 2))
                say("!love");
            return;
        }
        static const char *const openings[3] = {"there_is_a_legend_about_an_artifact", "im_looking_for_an_artifact",
                                                "there_is_a_mighty_artifact"};
        say(openings[xorshift(s) % 3]);
        if (!carried) {
            say("!question");
            say("i_wonder_where_it_could_be");
            return;
        }
        say(odd(s + 1) ? "!alert" : "!question");
        say("you_found_the_artifact");
        say("new_page");
        if (odd(s + 2)) {
            say("nice");
            if (odd(s + 3))
                say("!love");
        } else {
            say("im_jealous");
            if (odd(s + 3))
                say("!angry");
        }
    }

    // --- spec 4.6: a request for help with a place of the home region ---

    bool helpRequest() {
        if ((xorshift(c + 1) & 3) != 0)
            return false;
        // spec: open point 2: the game divides by zero on an empty list; skip the request.
        if (helpTargets.empty())
            return false;
        // Not from the script's seed: a person points at the same place all day.
        const uint32_t pick = xorshift(13u * (uint32_t)world.day + (uint32_t)speaker.id);
        const Cell target = helpTargets[pick % helpTargets.size()];
        const bool big = bigAt(target);
        const int kind = worldgen::placeKind(target[0], target[1], big);
        const worldgen::Int2 point = worldgen::cellPoint(target[0], target[1], big);
        say("new_page");
        defineLocation(target, kind);
        say("define_direction");
        number((int32_t)(((int64_t)point.x * 65536 - speaker.x) / 65536));
        number((int32_t)(((int64_t)point.y * 65536 - speaker.y) / 65536));
        const int role = worldgen::plannerRole(region, target[0], target[1]);
        branch = std::string("help: ") + helpSubject(role);
        if (role >= 2 && role <= 13)
            specialItemRequest(target, specialItemOfRole(role));
        else if (role == 1)
            missingGnome(target);
        else if (role == 16)
            monster(target, kind);
        else if (role == 18)
            itemsLyingAbout(target, kind);
        else if (role == 19)
            witch(target);
        else if (role == 20)
            demonPortal(target);
        else
            mark(target);
        return true;
    }

    static const char *helpSubject(int role) {
        if (role >= 2 && role <= 13)
            return "special item";
        switch (role) {
        case 1: return "gnome";
        case 16: return "monster";
        case 18: return "items lying about";
        case 19: return "witch";
        case 20: return "demon portal";
        default: return "other role";
        }
    }

    // The greeting of a help request: from c+2, leaving the counter at c+4.
    void requestGreeting() {
        uint32_t g = c + 2;
        greeting(g);
    }

    void specialItemRemark(int id, uint32_t r) {
        switch (id) {
        case HANG_GLIDER: say(GLIDER_REMARKS[r % 5]); break;
        case BOAT: say(SAILING_REMARKS[r & 3]); break;
        case CLIMBING_SPIKES: say(CLIMBING_REMARKS[r % 3]); break;
        case SKY_WHISTLE: say(r & 1 ? "it_has_a_nice_sound" : "i_wonder_if_the_bird_statues_have_to_do_with_it"); break;
        case HARP: say(r & 1 ? "it_has_a_nice_sound" : "it_looks_so_nice"); break;
        case REINS: say(REINS_REMARKS[r % 5]); break;
        default: break;
        }
    }

    // Roles 2-13: a special item (a crafting book, a glider, a boat...) lies at the place.
    void specialItemRequest(Cell target, int id) {
        defineSpecial(id);
        if (hasSpecial(id)) {
            branch += ", has";
            if (odd(c + 2))
                say("!alert");
            say("oh_you_already_have_the_item");
            say("new_page");
            if (id == SPIRIT_BELL) {
                say("new_page");
                say("spooky");
                say("!sweat");
                return;
            }
            say("nice");
            if (odd(c + 3))
                say("!love");
            uint32_t k = c + 4;
            if (craftingBook(id)) {
                if (odd(c + 4)) {
                    say("new_page");
                    say("im_curious_what_you_will_craft_with_it");
                }
                k = c + 5;
            }
            const bool remarked = id == HANG_GLIDER || id == BOAT || id == CLIMBING_SPIKES || id == SKY_WHISTLE ||
                                  id == HARP || id == REINS;
            if (remarked && odd(k)) {
                say("new_page");
                specialItemRemark(id, xorshift(k + 1));
            }
            return;
        }
        if (!known(target)) {
            itemTip();
            mark(target);
            return;
        }
        branch += ", known";
        if (odd(c + 2)) {
            if (odd(c + 3))
                say("!question");
            say("havent_you_found_the_item_yet");
            say("new_page");
            switch (xorshift(c + 4) % 3) {
            case 1:
                if (odd(c + 5))
                    say("!angry");
                say("what_a_shame");
                break;
            case 2:
                say("i_wish_you_good_luck_finding_it");
                if (odd(c + 5))
                    say("!love");
                break;
            default: say("it_should_be_in_the_location"); break;
            }
        } else {
            say("i_wish_you_good_luck_finding_the_item");
            if (odd(c + 3))
                say("!love");
        }
    }

    // How someone came to know of the item (spec 4.6, "does not have it, not known").
    void itemTip() {
        requestGreeting();
        say("new_page");
        const uint32_t u = c + 4;
        say("define_mob"); // the friend who saw it
        number((int32_t)xorshift(u));
        number(siblingRace(speaker.race));
        switch (xorshift(u + 1) % 5) {
        case 1:
            if (odd(u + 2))
                say("!alert");
            say("i_saw_an_item_in_the_zone");
            if (odd(u + 3)) {
                say("new_page");
                say("i_dont_need_it_and_left_it_there");
            }
            break;
        case 2: {
            uint32_t k = u + 3;
            if (odd(u + 2)) {
                say(odd(u + 3) ? "!question" : "!alert");
                k = u + 4;
            }
            say("do_you_need_an_item");
            say("new_page");
            static const char *const sources[3] = {"i_found_one_in_the_location", "my_friend_found_one_in_the_location",
                                                   "there_are_rumors_about_an_item_in_the_location"};
            say(sources[xorshift(k) % 3]);
            break;
        }
        case 3: {
            uint32_t k = u + 3;
            if (odd(u + 2)) {
                if (odd(u + 3))
                    say("!question");
                say("i_dont_know_if_its_true_but");
                say("new_page");
                k = u + 4;
            }
            say(odd(k) ? "my_friend_found_an_item_in_the_location" : "there_are_rumors_about_an_item_in_the_location");
            break;
        }
        case 4:
            say("my_friend_is_an_adventurer");
            say("new_page");
            say("when_he_was_exploring_the_location_he_found_an_item");
            break;
        default:
            say(odd(u + 2) ? "i_was_exploring_the_location" : "i_was_gathering_ingredients_in_the_location");
            say("new_page");
            if (odd(u + 3))
                say("!alert");
            say("i_saw_an_item");
            if (odd(u + 4)) {
                say("new_page");
                say("i_dont_need_it_and_left_it_there");
            }
            break;
        }
    }

    // Role 1: a supplier gnome went missing at the place.
    void missingGnome(Cell target) {
        if (completed(target)) {
            branch += ", completed";
            say("so_you_already_helped_the_gnome_in_location");
            say("new_page");
            say("nice");
            if (odd(c + 2))
                say("!love");
            mark(target);
            return;
        }
        if (known(target)) {
            branch += ", known";
            if (odd(c + 2)) {
                if (odd(c + 3))
                    say("!question");
                say("havent_you_found_the_gnome_yet");
                say("new_page");
                say("he_should_be_in_the_location");
            } else {
                say("i_wish_you_good_luck_rescuing_the_gnome_in_the_location");
                if (odd(c + 3))
                    say("!love");
            }
            return;
        }
        requestGreeting();
        say("new_page");
        const uint32_t u = c + 4;
        uint32_t k = u + 1;
        if (odd(u))
            say("a_supplier_gnome_went_missing_in_the_location");
        else {
            say(odd(u + 1) ? "i_was_exploring_the_location" : "i_was_gathering_ingredients_in_the_location");
            say("new_page");
            if (odd(u + 2))
                say("!alert");
            say("i_heard_a_gnome_screaming");
            k = u + 3;
        }
        say("new_page");
        defineModel(captorModel(worldgen::plannerEncounter(region, target[0], target[1], world.day)));
        say("he_was_captured_by_creatures");
        switch (xorshift(k) & 3) {
        case 0: say("!angry"); break;
        case 1: say("!sweat"); break;
        default: break;
        }
        if (odd(k + 1)) {
            say("new_page");
            say("i_hope_someone_is_going_to_help_him");
        }
        mark(target);
    }

    // Role 16: a monster was sighted at the place (0x1292f0 chooses it).
    void monster(Cell target, int kind) {
        // spec: open point: the spec lists twelve models (108-113, 115 twice, 116-119, "114 is
        // absent") for a draw mod 13. The game's own scripts name 114 for draw 6, so the list is
        // taken as 108-114, 115 twice, then 116-119.
        static const int monsters[13] = {108, 109, 110, 111, 112, 113, 114, 115, 115, 116, 117, 118, 119};
        const int model = kind == 23 || kind == 36 ? 107 : monsters[cellNoise(target, 0x2b3a706) % 13];
        defineModel(model);
        if (completed(target)) {
            branch += ", completed";
            switch (xorshift(c + 2) & 3) {
            case 0: say("!alert"); break;
            case 1: say("!question"); break;
            default: break;
            }
            say("so_you_defeated_that_monster_in_the_location");
            say("new_page");
            say("nice");
            if (odd(c + 3))
                say("!love");
        } else if (known(target)) {
            branch += ", known";
            say("i_wish_you_good_luck_finding_that_monster_in_the_location");
            switch (xorshift(c + 2) & 3) {
            case 0: say("!love"); break;
            case 1: say("!sweat"); break;
            default: break;
            }
        } else {
            requestGreeting(); // with no pause after it
            switch (xorshift(c + 4) & 3) {
            case 0: say("!alert"); break;
            case 1: say("!sweat"); break;
            default: break;
            }
            say("a_monster_was_sighted_in_the_location");
        }
        mark(target);
    }

    // Role 18 (never planned in practice): items lie about at the place.
    void itemsLyingAbout(Cell target, int kind) {
        worldgen::Item item;
        item.category = 22;
        item.id = (kind == 20 ? 48 : 27) + (target[0] & 1);
        item.modifier = cellNoise(target, 0xb9ad0e);
        item.rarity = (uint8_t)worldgen::plannerLevel(region, target[0], target[1]);
        defineItem(item); // (its region and +0x18 field are set too, but not printed)
        uint32_t u = c + 2;
        if (!known(target)) {
            requestGreeting();
            u = c + 4;
        } else
            branch += ", known";
        if (!odd(u))
            say("you_can_find_items_in_the_location");
        else {
            say("are_you_looking_for_items");
            if (odd(u + 1))
                say("!alert");
            say("new_page");
            say("you_can_find_some_in_the_location");
        }
        mark(target);
    }

    // Role 19: a witch who petrified the villagers of the region's special town.
    void witch(Cell target) {
        if (completed(target)) {
            branch += ", completed";
            say("!alert");
            say("so_you_defeated_the_witch");
            say("new_page");
            say("nice");
            if (odd(c + 2))
                say("!love");
            return;
        }
        say("a_witch_lives_in_location_direction_of_here");
        mark(target);
        defineLocation(specialTown, kindAt(specialTown));
        say("she_has_petrified_the_villagers_of_the_location");
        say("!sweat");
        say("you_must_defeat_her_to_break_the_curse");
    }

    // Role 20: a demon portal that possesses the land's people.
    void demonPortal(Cell target) {
        if (completed(target)) {
            branch += ", completed";
            say("!alert");
            say("so_you_destroyed_the_demon_portal");
            say("new_page");
            say("nice");
            if (odd(c + 2))
                say("!love");
            return;
        }
        say("a_demon_portal_was_seen_in_location_direction_of_here");
        defineLand();
        mark(target);
        say("the_demons_are_possessing_the_residents_of_the_land");
        say("!sweat");
    }

    // --- spec 4.7 to spec 4.10: the everyday conversation ---

    // spec 4.7: two in three times a greeting; returns the counter after it (c+3 or c+5).
    uint32_t greetingStep() {
        uint32_t g = c + 3;
        if (xorshift(c + 2) % 3 != 0) {
            greeting(g);
            say("new_page");
        }
        return g;
    }

    // spec 4.8: adventurers (classes 1-4) remark on the player's gear or class. True when they
    // said something; otherwise `v` is where the counter goes on.
    bool adventurerRemarks(uint32_t u, uint32_t &v) {
        v = u;
        if (speaker.cls < 1 || speaker.cls > 4)
            return false;
        if ((xorshift(u) & 3) == 0 && equipmentRemark(u))
            return true;
        if ((xorshift(u + 1) & 3) == 0) {
            classRemark(u);
            return true;
        }
        v = u + 2;
        return false;
    }

    bool equipmentRemark(uint32_t u) {
        std::vector<const worldgen::Item *> worn;
        for (const worldgen::Item &item : player.equipment)
            if (item.category != 0)
                worn.push_back(&item);
        if (worn.empty())
            return false;
        branch = "equipment remark";
        if (odd(u + 1))
            say("!alert");
        defineItem(*worn[xorshift(u + 2) % worn.size()]);
        say("nice_equipment_item");
        if (odd(u + 3)) {
            switch (xorshift(u + 4) % 3) {
            case 1:
                say("new_page");
                say("im_jealous");
                if (odd(u + 5))
                    say("!angry");
                break;
            case 2:
                say("new_page");
                say("nice");
                if (odd(u + 5))
                    say("!love");
                break;
            default: say("!love"); break;
            }
        }
        return true;
    }

    void classRemark(uint32_t u) {
        const std::string mine = CLASS_NAMES[speaker.cls];
        branch = "class remark";
        if (odd(u + 2)) {
            branch = "class remark, ambition";
            say("i_want_to_be_the_best_" + mine);
            return;
        }
        if (odd(u + 3))
            say("!alert");
        if (player.cls >= 1 && player.cls <= 4)
            say(std::string("so_you_are_a_") + CLASS_NAMES[player.cls]);
        say("new_page");
        if (player.cls == speaker.cls) {
            say("!love");
            if (odd(u + 4)) {
                say("good_choice");
                return;
            }
        } else
            say("!angry");
        say(mine + "s_are_the_strongest");
    }

    // spec 4.9: near the middle of a village, what the speaker is in the village for. True when
    // said; otherwise `w` is where the counter goes on.
    bool villageActivity(uint32_t v, uint32_t &w) {
        const Cell village = homePlace;
        const bool big = bigAt(village);
        const int kind = worldgen::placeKind(village[0], village[1], big);
        const worldgen::Int2 point = worldgen::cellPoint(village[0], village[1], big);
        const float d2 = worldgen::warpedDist2(point, blocksOf(speaker.homeX), blocksOf(speaker.homeY));
        const float weight = worldgen::placeWeight(kind, homeBig, d2);
        w = v;
        if (!(weight > 0.5f) || kind != 5)
            return false;
        w = v + 1;
        if ((xorshift(v) & 3) != 0)
            return false;
        branch = "village activity";
        defineLocation(village, 5);
        if (speaker.cls != 138) {
            say(ACTIVITY_TAGS[xorshift(v + 1) % 15]);
            say("im_in_this_zone_to_do_tasks");
        } else
            say(odd(v + 1) ? "welcome_to_location" : "how_do_you_like_our_zone");
        return true;
    }

    // spec 4.10: one of nineteen topics.
    void smallTalk(uint32_t w) {
        const uint32_t topic = xorshift(w) % 19;
        branch = topic == 2 ? "land history" : "small talk " + std::to_string(topic);
        switch (topic) {
        case 0: { // what pets eat
            const int food = pet(w + 1);
            defineModel(food);
            say("do_you_know_what_creatures_eat");
            say("new_page");
            if (odd(w + 2))
                say(odd(w + 3) ? "!question" : "!love");
            worldgen::Item item;
            item.category = 21, item.id = food; // pet food carries the pet's kind as its id
            defineItem(item);
            say("items_of_course");
            break;
        }
        case 1:
            defineModel(pet(w + 1));
            say("my_favorite_pet_is");
            if (odd(w + 2))
                say("!love");
            break;
        case 2:
            landHistory(worldgen::placeStyle(home[0], home[1]), w + 1);
            break;
        case 3: weather(w); break;
        case 4:
            if (odd(w + 1))
                say("!alert");
            say("i_like_your_style");
            if (odd(w + 2))
                say("!love");
            break;
        case 5:
            if (odd(w + 1))
                say("!question");
            say("you_look_strange");
            if (odd(w + 2))
                say("!sweat");
            break;
        case 6:
            if (odd(w + 1))
                say("!question");
            say("do_we_know_each_other");
            break;
        case 7: playersPet(w); break;
        case 8:
            defineModel(pet(w + 1));
            say("i_have_a_new_pet");
            if (odd(w + 2)) {
                say("new_page");
                if (odd(w + 3)) {
                    say("he_is_cheeky");
                    say(odd(w + 4) ? "!sweat" : "!angry");
                } else {
                    say("i_love_him");
                    say("!love");
                }
            }
            break;
        case 9: say("i_like_your_name"); break;
        case 10: say("are_you_an_adventurer"); break;
        case 11: say("i_love_hang_gliding"); break;
        case 12:
            say("do_you_like_hang_gliding");
            if (odd(w + 1)) {
                say("new_page");
                specialItemRemark(HANG_GLIDER, xorshift(w + 2));
            }
            break;
        case 13:
            say("are_you_a_good_climber");
            if (odd(w + 1)) {
                say("new_page");
                specialItemRemark(CLIMBING_SPIKES, xorshift(w + 2));
            }
            break;
        case 14:
            say("do_you_like_sailing");
            if (odd(w + 1)) {
                say("new_page");
                const uint32_t pick = xorshift(w + 2) % 5;
                say(SAILING_REMARKS[pick].line);
                if (pick == 4) // no boat
                    say(odd(w + 3) ? "!sweat" : "!angry");
            }
            break;
        case 15:
            say(NATURE_LOVES[xorshift(w + 1) % 6]);
            if (odd(w + 2))
                say("!love");
            break;
        case 16:
            defineModel(pet(w + 1));
            say("i_want_to_have_a_pet");
            break;
        case 17:
            say(odd(w + 1) ? "!sweat" : "!angry");
            say("i_dont_like_walking");
            if (odd(w + 2)) {
                say("new_page");
                say("i_prefer_riding");
                say("!love");
            }
            break;
        default: strongCreatures(w); break;
        }
    }

    // Topic 3: the warmth where the speaker stands.
    void weather(uint32_t w) {
        const int32_t x = blocksOf(speaker.x), y = blocksOf(speaker.y);
        const worldgen::Place place = worldgen::placeAt(x, y);
        const float warmth = worldgen::warmth(worldgen::neighbourhood(x, y), place.kind, place.weight);
        if (warmth > 0.8f) {
            say("its_hot");
            say("!sweat");
        } else if (warmth > 0.6f) {
            say("its_warm");
            if (odd(w + 1))
                say("!love");
        } else if (warmth >= 0.3f) {
            say("its_pleasant");
            if (odd(w + 1))
                say("!love");
        } else {
            say("its_cold");
            if (odd(w + 1))
                say("!angry");
        }
    }

    // Topic 7: the player's pet, or the lack of one.
    void playersPet(uint32_t w) {
        if (player.pet.category == 0) {
            if (odd(w + 1))
                say("!question");
            say("dont_you_have_a_pet");
            if (odd(w + 2)) {
                if (odd(w + 3))
                    say("!angry");
                say("what_a_shame");
            }
            return;
        }
        defineModel(player.pet.id);
        say("your_pet_is_adorable");
        if (odd(w + 1))
            say("!love");
        if (odd(w + 2))
            say("may_i_pet_it");
    }

    // Topic 18: beasts, golems or undead are strong (spec 5.7), drawn from a freshly seeded rand().
    void strongCreatures(uint32_t w) {
        runtime.seed(w + 1);
        static const std::vector<int> *const lists[3] = {&UNDEAD, &BEASTS, &GOLEMS};
        const std::vector<int> &list = *lists[xorshift(w + 1) % 3];
        const int entry = list[(size_t)runtime.next() % list.size()];
        defineModel(siblingRace(entry));
        say("creatures_are_really_strong");
        if (odd(w + 2))
            say("!sweat");
    }

    // --- spec 5.4: the history of the land ---

    void defineRealm(uint32_t realm) {
        say("define_realm");
        number(realmKind(realm));
        number((int32_t)realm);
    }
    void defineLand() { // the planner region's biome and its land seed
        say("define_land");
        number(worldgen::regionBiome(region.x, region.y));
        number((int32_t)noise((uint32_t)region.x + 57u * (uint32_t)region.y));
    }
    // How the speaker feels about the ruler's trait.
    void rulerTraitEmote(uint32_t realm) { say(goodRuler(realm) ? "!love" : "!sweat"); }

    void landHistory(uint32_t realm, uint32_t s) {
        out.clear(); // it replaces the greeting
        defineRealm(realm);
        defineLand();
        say("define_number");
        number(realmYear(realm));
        say("define_ruler");
        number(rulerTitle(realm));
        say("define_mob");
        number(rulerId(realm));
        number(rulerRace(realm));
        if (odd(s)) {
            say("history_time");
            say("new_page");
        }
        if (perishedRealm(realm))
            perishedRealmHistory(realm, s);
        else
            livingRealm(realm, s);
    }

    void perishedRealmHistory(uint32_t realm, uint32_t s) {
        if (odd(s + 1)) {
            if (odd(s + 2)) {
                say("have_you_heard_about_the_realm");
                say("new_page");
                say(odd(s + 3) ? "there_are_legends_about_realm_artifacts" : "you_can_still_find_realm_ruins_everywhere");
            } else {
                say("there_is_a_perished_civilization");
                say("new_page");
                say("you_can_still_find_realm_ruins_everywhere");
            }
            return;
        }
        say("many_years_ago_the_land_was_full_of_residents");
        say("new_page");
        say("their_ruler_was_famous");
        if (!odd(s + 2))
            return;
        say("new_page");
        if (odd(s + 3)) {
            say(realmMagic(realm));
            say("he_was_a_master_of_realm_magic");
        } else {
            say(rulerTrait(realm));
            say("he_was_known_for_his_trait");
            if (odd(s + 4))
                rulerTraitEmote(realm);
        }
    }

    void livingRealm(uint32_t realm, uint32_t s) {
        if (odd(s + 1)) {
            say("have_you_heard_about_the_realm");
            say("new_page");
            switch (xorshift(s + 2) & 3) {
            case 1: say("there_are_legends_about_realm_artifacts"); break;
            case 2: say("you_can_find_realm_ruins_everywhere"); break;
            case 3:
                defineModel(nationModel(realm, s + 3));
                say(odd(s + 3) ? "the_residents_are_a_realm_of_creatures" : "its_a_nation_of_creatures");
                break;
            default:
                say("there_are_still_some_residents_in_the_land");
                if (odd(s + 3)) {
                    say("new_page");
                    say("did_you_meet_any");
                }
                break;
            }
            return;
        }
        uint32_t t = s + 3;
        if (odd(s + 2)) {
            say("there_are_still_some_residents_in_the_land");
            t = s + 4;
            if (odd(s + 3)) {
                say("new_page");
                say("did_you_meet_any");
            }
        } else
            say("did_you_meet_any_residents");
        say("new_page");
        switch (xorshift(t) % 3) {
        case 1: say("there_are_legends_about_realm_artifacts"); break;
        case 2: say("you_can_find_realm_ruins_everywhere"); break;
        default:
            say("their_ruler_is_famous");
            if (!odd(t + 1))
                return;
            say("new_page");
            if (odd(t + 2)) {
                say(realmMagic(realm));
                say("he_is_a_master_of_realm_magic");
            } else {
                say(rulerTrait(realm));
                say("he_is_known_for_his_trait");
                if (odd(t + 3))
                    rulerTraitEmote(realm);
            }
            break;
        }
    }

};

} // namespace

// --- A realm's draws (spec 5.4) --------------------------------------------------------------

int realmKind(uint32_t realm) { return 12 + (int)(xorshift(realm + 0x7b2650) % 11); }

int rulerRace(uint32_t realm) {
    return PEOPLE_RACES[xorshift(realm + 0x80ea) & 7] + (int)(xorshift(realm + 0x4bc) & 1);
}

int rulerTitle(uint32_t realm) { return 2 * (int)(xorshift(realm + 0xa306b) % 13) + (femaleRace(rulerRace(realm)) ? 1 : 0); }

int32_t rulerId(uint32_t realm) { return (int32_t)xorshift(realm + 0x8144); }

int nationModel(uint32_t realm, uint32_t n) {
    const uint32_t k = xorshift(realm + 0x1337c) % 11;
    const int variant = (int)(xorshift(n) & 1);
    if (k >= 1 && k <= 7)
        return PEOPLE_RACES[k] + variant;
    static const int others[3] = {48, 51, 76}; // k = 8, 9, 10
    return k == 0 ? variant : others[k - 8];
}

int32_t realmYear(uint32_t realm) { return (int32_t)(xorshift(realm + 0x13309) % 9000) + 100; }

bool perishedRealm(uint32_t realm) { return odd(realm + 0xbd8b9); }

int realmFaction(uint32_t realm) { return (int)(xorshift(realm + 0x18365) & 7) + 1; }

const char *realmMagic(uint32_t realm) { return MAGIC_TAGS[xorshift(realm + 0x392fa) % 15]; }

const char *goodTrait(uint32_t n) { return GOOD_TRAITS[xorshift(n + 0x20b3) % 10]; }

const char *badTrait(uint32_t n) { return BAD_TRAITS[xorshift(n + 0x140e) % 9]; }

bool goodRuler(uint32_t realm) { return !odd(realm + 0xe6cb); }

const char *rulerTrait(uint32_t realm) { return goodRuler(realm) ? goodTrait(realm + 0x1e2e) : badTrait(realm + 0x1e2e); }

std::vector<std::string> script(const Speaker &speaker, const Player &player, const World &world, CRand &rand,
                                std::string *branch) {
    Builder builder(speaker, player, world, rand);
    std::vector<std::string> tokens = builder.build();
    if (branch)
        *branch = builder.branchTaken();
    return tokens;
}

} // namespace talkgen
