#pragma once
// DieuDeGlace's OverworldEvents (https://github.com/DieuDeGlace/OverworldEvents, MIT) as
// part of the world: encounters out in the overworld (adventurers fighting a pack, a
// captive in a guarded cage, a bounty, a locked chest with its key in a sack or on a
// guardian, a group holding a natural landmark) and villagers' quests that send the
// player to them. The rules are the mod's own (src/events); this runs them on CubeVulkan's
// reimplementation of the game, where the Steam mod went through CubeMegaMod's bridge:
// its creatures are made as the game makes creatures in play (worldgen::makeInPlay), its
// guardians' and enemies' strength goes through the mod loader's callbacks
// (nativerules::Callbacks), its lines through the speech box, its rewards come from the
// game's item generators (NativeDrop), and its objectives show on the game's map.
// [Events] Enabled=1 turns it on; it is off by default, as the vanilla game has none.
#include "GameShop.h"
#include "events/DialogueRules.h"
#include "events/StateStore.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

class ExtractedWorld;
class SpawnedCreatures;

class OverworldEvents {
  public:
    struct Settings {
        bool enabled = false;
        unsigned slots = 3;       // EventsPerRegion (1-8)
        unsigned cap = 2;         // ActiveEventCap (1-3)
        unsigned cooldown = 120;  // CooldownMinutes (30-1440)
        unsigned landmarkCap = 2; // LandmarkEventCap (0-4; 0: no landmark encounters)
    };
    // What the world gives the events (the Steam mod's CubeMegaMod bridge).
    struct World {
        SpawnedCreatures &creatures;
        ExtractedWorld &terrain;
        encounters::Site player;  // game blocks
        int playerLevel = 1, playerClass = 1;
        // The bag: an item received (as a picked-up item), how many of an item it holds
        // (the whole item compared), and one of it taken.
        std::function<bool(const gameshop::Item &)> give;
        std::function<int(const gameshop::Item &)> holds;
        std::function<bool(const gameshop::Item &)> take;
        // A creature's numeric level under progression rules (PyroProgression's
        // PyroProgressionCreatureLevel); unset: the mod's 1 + 8 * stars.
        std::function<int(const uint8_t *creature)> creatureLevel;
    };

    // The mod's travel locations ([Events] ReplaceTravelLocations): the sites of the region
    // planner's travel quests (glider, boat, climbing boots, reins) become a Castle, Ancient
    // Ruins, Crypt or Tower chosen by the world, and generation leaves out the travel items
    // at those sites and the regions' boats; every other place stays the game's. It goes into
    // the generator's hooks, so it is set before the world is generated.
    static void replaceTravelLocations(int32_t seed, const std::string &worldName);

    ~OverworldEvents();
    // The settings, where the state is kept, and the world's identity (its seed and name).
    void configure(const Settings &settings, const std::filesystem::path &stateFile, int32_t seed, const std::string &worldName);
    bool enabled() const { return settings.enabled && usable; }
    // Every frame: event objects held where they were placed, captives bound in their cages.
    // Once a second: progress, planning, making the actors, saving.
    void update(World &world, double nowSeconds);
    // A creature died (the world's death reports).
    void death(int64_t id);
    // The use key on creature `id`: the line said and who says it. False when the events have
    // nothing to say (the creature keeps its own interaction).
    struct Said {
        int64_t speaker = 0;
        std::string line;
    };
    bool interact(World &world, int64_t id, Said &said);
    // A villager's quest sprite (TownRules' QuestMarker): 0 none, 1 an offer (the orange "!"),
    // 2 a reward to hand in (the orange "?"), 3 under way (the grey "?").
    int questMarker(World &world, int64_t id);
    // The objectives on the map: where (game blocks), and whether it is an accepted village
    // quest (the grey "?") or an encounter (the orange one).
    struct Pin {
        int64_t x = 0, y = 0, z = 0;
        bool destination = false;
    };
    const std::vector<Pin> &pins() const { return mapPins; }
    // The mod's chat commands (/events test battle|rescue|bounty|chest-sack|chest-boss,
    // /events landmarks, /events status); true when handled. `reply` gets what to tell.
    bool command(const std::string &line, std::string &reply);
    // /events status's details for the log: each active instance and its actors.
    void report(World &world);
    // Saves now if anything changed.
    void flush();
    // For the log: what happened since the last call; and what a test command answered.
    std::vector<std::string> takeLog() { return std::exchange(logLines, {}); }
    std::vector<std::string> takeReplies() { return std::exchange(said, {}); }

  private:
    // Actor slots: 0-5 enemies, then the event NPC, its ally, the cage or sack, and the chest.
    static constexpr unsigned Npc = 6, Companion = 7, Object = 8, Container = 9, ActorCount = 10;
    struct Active {
        encounters::EventKey key;
        std::array<int64_t, ActorCount> actors{};
        Pin marker{};
        bool markerKnown = false;
        std::array<std::array<int64_t, 3>, ActorCount> anchors{}; // objects' places (dots)
        unsigned anchoredMask = 0;
    };
    struct Request {
        encounters::EventKey key;
        std::array<std::array<int64_t, 3>, ActorCount> points{};
    };
    struct Referral {
        encounters::Site town;
        encounters::EventKey objective;
        unsigned cycle = 0;
    };
    struct TownSite {
        encounters::Site site{};
        bool found = false;
        double until = 0;
    };
    struct Assets {
        int cageRace = -1, sackRace = -1, chestRace = 181;
        std::vector<int> npcRaces, aquaticRaces;
        int keyCategory = -1, keyId = -1;
    };

    Settings settings;
    bool usable = false, loaded = false, dirty = false;
    int callbacks = 0; // the stat callbacks' handle (nativerules::addCallbacks)
    std::filesystem::path stateFile;
    uint64_t worldId = 0;
    encounters::StateStore state;
    std::vector<Active> active;
    std::vector<Referral> referrals;
    std::set<int64_t> spawned;                   // every actor made this session
    std::set<int64_t> guardians;                 // named leaders (their health rule)
    std::map<int64_t, unsigned> enemyDifficulty; // encounter enemies and their level
    std::map<uint64_t, TownSite> townSites;      // villagers of a town share a search
    std::map<int64_t, std::pair<int, double>> hints; // villagers' quest sprites, a second each
    Assets assets;
    double nextTick = 0, now = 0;
    encounters::Kind requestedTest = encounters::Kind::Count;
    unsigned testAttempt = 0;
    bool reportLandmarks = false, reportStatus = false;
    std::vector<Pin> mapPins;
    std::vector<std::string> logLines;
    std::vector<std::string> said; // what a test command answered (told on the next update)
    encounters::dialogue::Picker picker;
    uint64_t lineCounter = 0;

    void log(const std::string &line) { logLines.push_back(line); }
    encounters::EventRecord &record(const encounters::EventKey &key) { return state.get(key.world, key.region, key.slot); }
    unsigned unfinished(bool landmarks) const;
    bool owned(int64_t id) const;
    std::vector<const uint8_t *> enemyPool(World &world, encounters::Region region) const;
    bool chooseEnemies(World &world, encounters::Region region, uint64_t roll, encounters::EventRecord &saved,
                       bool repairAquatic = false);
    encounters::EventRecord *unusedSlot(encounters::Region region);
    bool available(encounters::Kind kind) const;
    bool selectTownSite(encounters::EventRecord &candidate, encounters::Site village, int radius);
    bool place(World &world, encounters::EventRecord &saved, std::array<std::array<int64_t, 3>, ActorCount> &points);
    bool sackPoint(World &world, const encounters::EventRecord &saved, int center, encounters::Site &point, int &height);
    uint8_t *make(World &world, Active &instance, unsigned index, const std::array<int64_t, 3> &point, int race,
                  int behaviour, int stars);
    bool spawn(World &world, encounters::EventRecord &saved, const std::array<std::array<int64_t, 3>, ActorCount> &points,
               Active *existing = nullptr);
    void release(World &world, Active &instance, unsigned index);
    void bindRescue(World &world, const Active &instance);
    void freeRescue(World &world, const Active &instance);
    void objectiveMarker(World &world, const encounters::EventRecord &saved, Active &instance);
    void publishPins();
    gameshop::Item keyItem(const encounters::EventRecord &saved) const;
    bool grantKey(World &world, encounters::EventRecord &saved);
    bool deliver(World &world, encounters::EventRecord &saved);
    std::string line(int64_t speaker, encounters::dialogue::Topic topic, encounters::Kind kind);
    std::string referralLine(World &world, int64_t speaker, const encounters::EventRecord &saved, bool offer);
    bool townOffer(World &world, encounters::Site village, int radius, encounters::EventRecord &candidate);
    bool townInteraction(World &world, int64_t id, Said &said);
    encounters::EventRecord *landmarkRecord(encounters::Region area, encounters::Site center);
    void planLandmarks(World &world, uint64_t utc, std::vector<Request> &pending);
    void progress(World &world);
    void testEncounter(World &world, encounters::Region region);
    void hold(World &world);
    void tick(World &world);
    void resolveAssets();
};
