#include "Lore.h"
#include "TalkGen.h"
#include "worldgen/Generator.h"

#include <algorithm>

namespace lore {

using talkgen::odd;
using talkgen::xorshift;

namespace {

// The world's noise R(n) (conversation-script spec 2.4), n in wrapping 32-bit arithmetic.
uint32_t noise(uint32_t n) { return worldgen::random(n); }
// A cell's number: x + 57y, wrapping.
uint32_t cellNumber(Cell c) { return (uint32_t)c[0] + 57u * (uint32_t)c[1]; }

// --- The tales' tag tables (spec 3.2) -------------------------------------------------------------

const char *const PLACE_TAGS[9] = {"#island", "#village", "#ocean", "#mountains", "#snowlands",
                                   "#deadlands", "#farm", "#city", "#palace"};
const char *const SOURCE_TAGS[21] = {"#ancient_inscription", "#old_inscription", "#magic_inscription", "#stone_tablet",
                                     "#inscribed_tablet",    "#ancient_scroll",  "#old_scroll",        "#magic_scroll",
                                     "#ancient_book",        "#magic_book",      "#old_book",          "#witch",
                                     "#wise_man",            "#wise_woman",      "#sorcerer",          "#sorceress",
                                     "#wizard",              "#ghost",           "#spirit",            "#djinn",
                                     "#demon"};
const char *const TERRAIN_TAGS[7] = {"#island", "#mountains", "#ocean", "#snowlands", "#deadlands", "#forest", "#desert"};
const char *const BUILDING_TAGS[11] = {"#temple", "#pyramid", "#palace",   "#grotto",   "#cave",   "#citadel",
                                       "#castle", "#cathedral", "#fortress", "#tower", "#volcano"};
// A realm's capital is named from three syllables.
const char *const CAPITAL_FIRST[29] = {"Ka", "Cha", "Ca", "Cy", "Ko", "Go", "Ga", "Gro", "Te", "Tu", "Lu", "A", "E", "Y", "I",
                                       "Vy", "Wa", "No", "Le", "Ti", "Ba", "So", "Sa", "Si", "Xa", "Xy", "Xi", "Zu", "Za"};
const char *const CAPITAL_MIDDLE[10] = {"mer", "ra", "gu", "ga", "mor", "nor", "lor", "gor", "nar", "far"};
const char *const CAPITAL_LAST[9] = {"zan", "a", "o", "ia", "an", "e", "une", "unia", "nia"};

// How a realm's capital was lost, by the realm mod 8 (spec 3.3 case 4).
const char *const CAPITAL_FATES[8] = {
    "the_capital_of_the_realm_was_destroyed_during_a_war_against_the_faction",
    "the_capital_of_the_realm_sunk_in_the_ocean",
    "the_capital_of_the_realm_was_destroyed_by_a_meteor",
    "the_capital_of_the_realm_was_destroyed_by_an_earthquake",
    "the_capital_of_the_realm_was_destroyed_by_a_volcanic_eruption",
    "the_capital_of_the_realm_was_destroyed_by_a_titan",
    "the_capital_of_the_realm_was_destroyed_by_a_megasaurian",
    "the_capital_of_the_realm_was_destroyed_by_a_demon",
};

// Who killed the one under a tombstone (spec 3.5): the monsters and the poisoners, and the
// races of the killers by the realm (b adds the female race where there is one).
const int MONSTERS[47] = {96,  17,  165, 41,  269, 230, 173, 166, 276, 269, 97,  40,  94,  226, 85,  73,
                          229, 110, 225, 226, 226, 45,  43,  167, 108, 109, 114, 116, 115, 119, 262, 117,
                          118, 111, 112, 81,  102, 105, 295, 293, 104, 69,  70,  71,  72,  -99992, -99993};
const int POISONERS[11] = {230, 97, 45, 167, 43, 53, -99997, -99993, -99992, -99995, -99994};
struct KillerRace {
    int race;
    bool byGender; // plus one for a female killer
};
const KillerRace KILLER_RACES[16] = {{0, true},   {2, true},   {4, true},   {7, true},   {9, true},  {11, true},
                                     {13, true},  {15, true},  {48, false}, {51, false}, {76, false}, {199, false},
                                     {43, false}, {230, false}, {227, true}, {167, false}};
int killerRace(uint32_t realm, uint32_t n) {
    const KillerRace &k = KILLER_RACES[xorshift(realm + 0x157c) & 15];
    return k.race + (k.byGender ? (int)(xorshift(n) & 1) : 0);
}

// The tags a realm draws.
const char *placeTag(uint32_t realm) { return PLACE_TAGS[xorshift(realm + 0x334e) % 9]; }
const char *sourceTag(uint32_t realm) { return SOURCE_TAGS[xorshift(realm + 0x13031) % 21]; }
const char *terrainTag(uint32_t realm) { return TERRAIN_TAGS[xorshift(realm + 0x154e) % 7]; }
const char *buildingTag(uint32_t realm) { return BUILDING_TAGS[xorshift(realm + 0x392fa) % 11]; }
std::string capitalName(uint32_t realm) {
    return std::string(CAPITAL_FIRST[xorshift(realm + 0x18043) % 29]) + CAPITAL_MIDDLE[xorshift(realm + 0x138c) % 10] +
           CAPITAL_LAST[xorshift(realm + 0x4f4e) % 9];
}

// A script being written: tokens in the conversations' format (conversation-script spec 7).
class Tale {
  public:
    std::vector<std::string> tokens;

    void say(const char *token) { tokens.emplace_back(token); }
    void say(const std::string &token) { tokens.push_back(token); }
    void number(int32_t value) { tokens.push_back(std::to_string(value)); }
    void define(const char *command, int32_t value) {
        say(command);
        number(value);
    }

    void realm(uint32_t realm) {
        say("define_realm");
        number(talkgen::realmKind(realm));
        number((int32_t)realm);
    }
    void rulerTitle(uint32_t realm) { define("define_ruler", talkgen::rulerTitle(realm)); }
    // The realm's ruler as the person "mob".
    void ruler(uint32_t realm) {
        say("define_mob");
        number(talkgen::rulerId(realm));
        number(talkgen::rulerRace(realm));
    }
    void faction(uint32_t realm) { define("define_faction", talkgen::realmFaction(realm)); }
    // A land the realm's ruler went to, not a real one (spec 3.2).
    void fakeLand(uint32_t realm) {
        say("define_land");
        number((int32_t)(xorshift(realm + 0x14ca) % 14));
        number((int32_t)xorshift(realm + 0x18198));
    }
    void land(int biome, int32_t seed) {
        say("define_land");
        number(biome);
        number(seed);
    }
    void year(uint32_t realm) { define("define_number", talkgen::realmYear(realm)); }
    void item(const worldgen::Item &item) { // material before rarity
        say("define_item");
        number(item.category);
        number(item.id);
        number((int32_t)item.modifier);
        number(item.material);
        number(item.rarity);
    }
};

// --- The realm story's seven kinds (spec 3.3) -----------------------------------------------------

void foundersJourney(Tale &t, uint32_t realm) {
    t.ruler(realm);
    t.say(placeTag(realm));
    t.say("npc_lived_in_a_location");
    t.say("new_page");
    switch (xorshift(realm + 0x230b) % 3) {
    case 1:
        t.fakeLand(realm);
        t.say("later_he_moved_to_the_land");
        break;
    case 2:
        t.fakeLand(realm);
        t.say("one_day_he_explored_the_land");
        break;
    default:
        t.faction(realm);
        t.fakeLand(realm);
        t.say("during_the_faction_war_he_had_to_flee_to_the_land");
        break;
    }
    switch (xorshift(realm + 0x4d1) % 5) {
    case 1: t.say("there_he_explored_ancient_ruins_and_dungeons"); break;
    case 2:
        t.say(talkgen::realmMagic(realm));
        t.say("there_he_learned_about_realm_magic");
        break;
    case 3: t.say("after_some_years_he_founded_the_realm"); break;
    case 4: t.say("there_are_many_legends_about_his_adventures"); break;
    default:
        t.faction(realm);
        t.say("there_he_fought_against_the_faction");
        break;
    }
}

void sourceOfMagic(Tale &t, uint32_t realm, uint32_t s, int biome, int32_t landSeed) {
    t.ruler(realm);
    t.fakeLand(realm);
    t.say(sourceTag(realm));
    t.say(talkgen::realmMagic(realm));
    switch (xorshift(s + 1) % 6) {
    case 1: t.say("npc_used_his_realm_magic_skills_to_explore_dangerous_dungeons"); break;
    case 2:
        t.faction(realm);
        t.say("npc_used_his_realm_magic_skills_to_fight_the_faction");
        break;
    case 3: t.say("using_realm_magic_npc_was_able_to_increase_his_power"); break;
    case 4:
        t.land(biome, landSeed); // the real land, this time
        t.say("npc_used_his_realm_magic_skills_to_explore_the_land");
        break;
    case 5: t.say("npc_spent_a_lot_of_time_exploring_the_land"); break;
    default: t.say("during_his_time_in_land_npc_discovered_wisdom"); break;
    }
}

void residents(Tale &t, uint32_t realm, uint32_t s) {
    const bool perished = talkgen::perishedRealm(realm);
    if (!odd(s + 1)) {
        t.say(talkgen::realmMagic(realm));
        t.say(perished ? "some_residents_used_realm_magic" : "some_residents_use_realm_magic");
        return;
    }
    t.ruler(realm);
    if (!(perished && !odd(s + 2))) {
        t.say("praise_the_ruler");
        return;
    }
    if (talkgen::goodRuler(realm)) {
        t.say("the_residents_loved_their_ruler");
        t.say(talkgen::goodTrait(realm + 0x1e2e));
    } else {
        t.say("the_residents_feared_their_ruler");
        t.say(talkgen::badTrait(realm + 0x1e2e));
    }
    t.say("he_was_known_for_his_trait");
}

void capital(Tale &t, uint32_t realm, uint32_t s) {
    t.say("define_name");
    t.say(capitalName(realm));
    t.faction(realm);
    switch (xorshift(s + 1) & 3) {
    case 1: t.say("the_capital_of_the_realm_was_called"); break;
    case 2:
        t.say("archeologists_are_looking_for_the_ruins_of_the_capital_of_the_realm");
        if (xorshift(s + 2) != 0) {
            t.say("new_page");
            t.say("until_now_their_efforts_were_useless");
        }
        break;
    case 3:
        t.define("define_number", (int32_t)(xorshift(realm + 0xc355) % 101) + talkgen::realmYear(realm));
        t.say("the_capital_of_the_realm_was_built_in_year");
        break;
    default: t.say(CAPITAL_FATES[realm % 8]); break;
    }
}

void theLand(Tale &t, uint32_t realm, uint32_t s, int biome, int32_t landSeed) {
    t.land(biome, landSeed);
    t.year(realm);
    switch (xorshift(s + 1) % 3) {
    case 1: t.say("the_land_was_populated_by_residents_in_year"); break;
    case 2:
        t.faction(realm);
        t.say("long_ago_there_was_a_war_in_the_land_between_the_realm_and_the_faction");
        break;
    default:
        t.say(talkgen::perishedRealm(realm) ? "the_land_was_inhabited_by_residents" : "the_land_is_inhabited_by_residents_since_year");
        break;
    }
}

void rulersSeat(Tale &t, uint32_t realm) {
    t.ruler(realm);
    t.say(buildingTag(realm));
    t.say(terrainTag(realm));
    t.say(talkgen::perishedRealm(realm) ? "the_ruler_was_buried_in" : "the_ruler_lives_in");
}

void masterOfMagic(Tale &t, uint32_t realm, bool past) {
    t.say(talkgen::realmMagic(realm));
    t.say(past ? "he_was_a_master_of_realm_magic" : "he_is_a_master_of_realm_magic");
}
void knownForTrait(Tale &t, uint32_t realm) {
    t.say(talkgen::rulerTrait(realm));
    t.say("he_was_known_for_his_trait");
}

void rulers(Tale &t, uint32_t realm, uint32_t s) {
    if (!talkgen::perishedRealm(realm)) {
        uint32_t next;
        if (odd(s + 1)) {
            t.ruler(realm);
            t.say("the_realm_is_ruled_by");
            next = s + 2;
        } else {
            t.year(realm);
            t.say("the_realm_exists_since_the_year");
            if (odd(s + 2)) {
                t.say("new_page");
                t.ruler(realm);
                t.say("it_is_ruled_by");
            }
            next = s + 3;
        }
        if (odd(next))
            masterOfMagic(t, realm, false);
        return;
    }
    if (odd(s + 1)) {
        t.ruler(realm);
        if (!odd(s + 2))
            t.say("once_there_was_a_a_named_ruler");
        else {
            t.year(realm);
            t.say("in_the_year_there_was_a_named_ruler");
        }
        switch (xorshift(s + 3) % 3) {
        case 1: knownForTrait(t, realm); break;
        case 2: masterOfMagic(t, realm, true); break;
        default: t.say("he_founded_the_realm"); break;
        }
        return;
    }
    t.ruler(realm);
    if (!odd(s + 2))
        t.say("once_there_was_a_realm");
    else {
        t.year(realm);
        t.say("in_the_year_there_was_a_realm");
    }
    t.say("new_page");
    if (odd(s + 3)) {
        t.say("the_realm_was_ruled_by");
        return;
    }
    t.say("the_residents_had_a_ruler");
    switch (xorshift(s + 4) % 3) {
    case 1: // (the game goes on to the magic as well)
        knownForTrait(t, realm);
        masterOfMagic(t, realm, true);
        break;
    case 2: masterOfMagic(t, realm, true); break;
    default:
        t.ruler(realm);
        t.say("his_name_was");
        break;
    }
}

} // namespace

// --- Lore buildings and their cells -------------------------------------------------------------

bool isLoreBuilding(Cell cell) {
    const int object = worldgen::cellObject(cell[0], cell[1]);
    return std::find(std::begin(LORE_BUILDINGS), std::end(LORE_BUILDINGS), object) != std::end(LORE_BUILDINGS);
}

bool examinable(int objectType) { return (objectType >= 118 && objectType <= 126) || objectType == 130; }

Cell cellOf(int64_t x, int64_t y) {
    auto floorDiv = [](int64_t dots) { // dots to blocks, then blocks to cells, flooring both
        const int64_t block = dots >= 0 ? dots / 65536 : -((-dots + 65535) / 65536);
        return (int32_t)(block >= 0 ? block / 16 : -((-block + 15) / 16));
    };
    return {floorDiv(x), floorDiv(y)};
}

Cell placeOf(Cell cell) { return {worldgen::floorDiv(cell[0], 6), worldgen::floorDiv(cell[1], 6)}; }

uint32_t realmOf(Cell cell) {
    const Cell place = placeOf(cell);
    return worldgen::placeStyle(place[0], place[1]);
}

// --- Examining and the tales -------------------------------------------------------------------

std::vector<Cell> searchOrder(Cell cell) {
    const int reach = isLoreBuilding(cell) ? 0 : 1;
    std::vector<Cell> order;
    for (int dx = -reach; dx <= reach; dx++)
        for (int dy = -reach; dy <= reach; dy++)
            order.push_back({cell[0] + dx, cell[1] + dy});
    return order;
}

std::optional<Reading> examine(Cell cell) {
    const int reach = isLoreBuilding(cell) ? 0 : 1;
    for (int dx = -reach; dx <= reach; dx++)
        for (int dy = -reach; dy <= reach; dy++) {
            const Cell candidate{cell[0] + dx, cell[1] + dy};
            if (!isLoreBuilding(candidate))
                continue;
            Reading reading{candidate, tale(candidate)};
            if (!reading.tale.empty())
                return reading;
            break; // a building with nothing to say: on to the next column
        }
    return std::nullopt;
}

std::vector<std::string> tale(Cell cell) {
    const uint32_t n = cellNumber(cell), seed = noise(n);
    const uint32_t realm = realmOf(cell);
    if (worldgen::cellObject(cell[0], cell[1]) == TOMBSTONE_BUILDING)
        return tombstoneTale(realm, seed);
    const worldgen::Int2 home = worldgen::regionAt(16 * cell[0] + 8, 16 * cell[1] + 8);
    if (noise(n + 0x9b084) & 1) {
        // The tale of an artifact of this realm hidden in this region or one next to it.
        const worldgen::Int2 region{home.x + (int32_t)(noise(n + 0x5a278) % 3) - 1, home.y + (int32_t)(noise(n + 0x7d1b4) % 3) - 1};
        std::vector<Cell> hiding;
        for (const auto &[x, y, role] : worldgen::regionRoles(region).roles)
            if (role == 14 && worldgen::placeStyle(x, y) == realm)
                hiding.push_back({x, y});
        if (!hiding.empty()) {
            const Cell place = hiding[xorshift(noise(n + 0x8ef9b)) % hiding.size()];
            const bool big = worldgen::bigCell(place[0], place[1]);
            int kind = worldgen::placeKind(place[0], place[1], big);
            if (const int structure = worldgen::bigStructure(place[0], place[1], kind, big))
                kind = structure;
            return artifactTale(realm, worldgen::placeArtifact(place[0], place[1]), place, kind, seed);
        }
    }
    return realmStory(realm, seed, worldgen::regionBiome(home.x, home.y), (int32_t)noise((uint32_t)home.x + 57u * (uint32_t)home.y));
}

std::vector<std::string> realmStory(uint32_t realm, uint32_t seed, int biome, int32_t landSeed) {
    Tale t;
    t.realm(realm);
    t.rulerTitle(realm);
    switch (xorshift(seed) % 7) {
    case 1: foundersJourney(t, realm); break;
    case 2: sourceOfMagic(t, realm, seed, biome, landSeed); break;
    case 3: residents(t, realm, seed); break;
    case 4: capital(t, realm, seed); break;
    case 5: theLand(t, realm, seed, biome, landSeed); break;
    case 6: rulersSeat(t, realm); break;
    default: rulers(t, realm, seed); break;
    }
    return t.tokens;
}

std::vector<std::string> artifactTale(uint32_t realm, const worldgen::Item &item, Cell place, int kind, uint32_t seed) {
    static const char *const OPENINGS[3] = {"there_is_a_legend_about_an_artifact", "the_item_was_a_mighty_artifact",
                                            "there_is_a_mighty_artifact"};
    constexpr int FIRST_CASTLE = 40; // big structures 40 and up are castles and dungeons
    Tale t;
    t.realm(realm);
    t.item(item);
    t.say(OPENINGS[xorshift(seed) % 3]);
    t.say("define_location");
    t.number(place[0]);
    t.number(place[1]);
    t.number(kind);
    t.say(kind < FIRST_CASTLE ? "it_is_said_to_be_located_in_the_location" : "it_is_said_to_be_located_in_the_dungeon");
    t.say("mark_location"); // reading it marks the place on the map
    t.number(place[0]);
    t.number(place[1]);
    return t.tokens;
}

std::vector<std::string> tombstoneTale(uint32_t realm, uint32_t seed) {
    Tale t;
    t.realm(realm);
    t.say("define_mob");
    t.number((int32_t)xorshift(seed));
    t.number(talkgen::nationModel(realm, seed));
    const int32_t age = (int32_t)(xorshift(seed + 2) % 500) + 18;
    uint32_t next = seed + 4;
    switch (xorshift(seed + 3) % 3) {
    case 1: t.say("here_rests_resident"); break;
    case 2: { // born some years into the realm's history
        const int32_t born = talkgen::realmYear(realm) + (int32_t)(xorshift(seed + 4) % 1000);
        t.define("define_number", born);
        t.define("define_number2", born + age);
        t.say("npc_life_span");
        next = seed + 5;
        break;
    }
    default: t.say("here_rests_hero_of_the_realm"); break;
    }
    t.say("new_page");
    switch (xorshift(next) % 11) { // how he died
    case 1:
        t.faction(realm);
        t.say("he_was_killed_by_a_faction_member");
        break;
    case 2:
        t.say("define_mob2");
        t.number((int32_t)xorshift(next + 1));
        t.number(killerRace(realm, next + 1));
        t.say("he_was_killed_by_npc");
        break;
    case 3:
        t.define("define_model2", MONSTERS[xorshift(next + 1) % 47]);
        t.say("he_was_killed_by_a_monster");
        break;
    case 4:
        t.define("define_model2", POISONERS[xorshift(next + 1) % 11]);
        t.say("he_was_poisoned_by_a_creature");
        break;
    case 5: t.say("he_lived_a_full_life"); break;
    case 6:
        t.say(talkgen::goodTrait(next + 1));
        t.say("he_was_known_for_his_trait");
        break;
    case 7:
        t.define("define_number", age);
        t.say("he_lived_for_years");
        break;
    case 8: masterOfMagic(t, realm, true); break;
    case 9: t.say("there_are_many_legends_about_his_adventures"); break;
    default: // 0 and 10
        t.faction(realm);
        t.say("he_died_in_the_war_against_the_faction");
        break;
    }
    return t.tokens;
}

// --- The character's lore -------------------------------------------------------------------------

Book rebuild(const std::set<Cell> &usedCells) {
    Book book;
    for (const Cell &cell : usedCells)
        if (isLoreBuilding(cell))
            book[realmOf(cell)].insert(cell);
    return book;
}

Credit credit(Cell cell, std::set<Cell> &usedCells, Book &book) {
    Credit result;
    result.newlyUsed = usedCells.insert(cell).second;
    if (!isLoreBuilding(cell))
        return result;
    result.realm = realmOf(cell);
    std::set<Cell> &pieces = book[result.realm];
    result.newPiece = pieces.insert(cell).second;
    result.pieces = pieces.size();
    return result;
}

// --- Carried artifacts ----------------------------------------------------------------------------

ArtifactProgress artifactProgress(const std::vector<worldgen::Item> &stacks) {
    ArtifactProgress progress;
    for (const worldgen::Item &item : stacks)
        if (item.category == ARTIFACT_CATEGORY) {
            progress.level++;
            progress.skills[item.modifier % ARTIFACT_SKILLS]++;
        }
    return progress;
}

} // namespace lore
