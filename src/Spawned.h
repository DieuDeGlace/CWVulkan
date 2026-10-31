#pragma once
// The creatures of the game's generated world ([World] Source=Game): the spawn points
// the world generator makes for each zone become creatures the way the game makes them when a
// player comes near (worldgen/Residents.h, within 4 zones), and are posed by the game's own pose code
// (NativePose) with the game's models (Extract/sprites.txt, the world's model list).
#include "Extracted.h"
#include "NativeAI.h"
#include "NativeUpkeep.h"
#include "NativeAbility.h"
#include "NativeHit.h"
#include "NativeEffects.h"
#include "NativeDrop.h"
#include "NativeControl.h"
#include "SoundRules.h"
#include "ZoneStates.h"
#include "GroundItems.h"
#include "EnemyRespawn.h"
#include <list>
#include <functional>
#include "worldgen/Residents.h"
#include "NativePose.h"
#include "Renderer.h"
#include "net/Worlds.h"
#include <filesystem>
#include <map>
#include <optional>
#include <random>
#include <string>
#include <tuple>
#include <set>
#include <utility>
#include <vector>

class World;

class SpawnedCreatures {
  public:
    // Reads the game's model list (id -> sprite, size). False without it.
    bool init(const std::filesystem::path &extractDir, std::string *why = nullptr);
    // Keeps the zones within 4 of every player in memory and lets go of farther ones. A world that
    // decides (not a remote client) makes the creatures of those zones, near every player
    // (worldgen-creatures.md §7.1); a mirror makes none. `playerLocal` stands in for the local
    // player before it exists.
    void update(ExtractedWorld &ew, glm::vec3 playerLocal);

    // --- Multiplayer: the world's side (docs/specs/netcode-roles.md) -------------------------
    // Every game has a host world, which decides, and a client world that mirrors a host (its own
    // too, through the loopback). A remote client (world+0x358) makes no creatures, its AI,
    // abilities, hits and upkeep run as a client's (only the local player decides for itself), it
    // performs no interactions (its player's go to the host), and its creatures are the host's.
    void setRemoteClient(bool on) { client = on; }
    bool remoteClient() const { return client; }
    // The local player's creature id: 1 until a host gives it another (packet 4).
    int64_t playerId() const { return localId; }
    void setPlayerId(int64_t id);
    // A creature the network brings (a guest's player in a host world, any creature in a mirror):
    // the one with this id, or a new one made with the constructor's defaults (`made` set). Such
    // creatures belong to no zone and stay until removed. Null for the local player's id.
    uint8_t *networkCreature(int64_t id, bool &made);
    // After a received update (netcode-packets.md §2.4 step 5): a creature just made or dead is
    // drawn where it is; any other eases there over 60 ms.
    void receivedUpdate(int64_t id, bool fresh);
    // Every creature but the local player whose id is not in `keep` is removed (packet 6); with no
    // `keep`, all of them (connecting to a host).
    void keepOnly(const std::set<int64_t> *keep);
    // The drawn positions and orientations ease toward the records (0x2b68e0), `dtMs` long.
    void smooth(float dtMs);
    // A creature's pending interactions (+0xab8): a guest's arrive one at a time (packet 9); the
    // local player's leave one at a time for the host.
    void queueInteraction(int64_t id, const nativeai::Interaction &in);
    bool takePlayerInteraction(nativeai::Interaction &out);
    // The world's event lists (world+0x90 .. +0x170): what its steps made since they were last
    // emptied. The host hands them to its guests each tick; a client sends its own player's.
    cwnet::WorldEvents &events() { return worldEvents; }
    // Host tick step 5 (netcode-roles.md §2): the sounds, damage numbers (applied by 0x29dcf0),
    // buffs and effect objects the guests caused.
    void applyGuestEvents(ExtractedWorld &ew, const cwnet::GuestEvents &caused);
    // What a client does with a world update (netcode-roles.md §8).
    void giveBuffNotice(const cwnet::BuffRecord &notice);  // 0x65390 with the notice's flag
    void setAggro(const cwnet::AggroRecord &notice);       // +0xb88 of the angered creature
    void hitPlayer(const std::array<uint8_t, 0x48> &number); // a monster's hit, on the local player's own record
    void flash(int64_t id);                                // +0x994 = 1.0
    steampoints::GroundSources groundLights(ExtractedWorld &ew, int zx, int zy);
    std::vector<steampoints::EffectSource> effectLights() const;
    void collectLights(const steampoints::Frame &, std::vector<steampoints::Light> &) const;
    struct SpriteAsset { int model = -1; glm::ivec3 header{0}; };
    std::optional<SpriteAsset> spriteAsset(World &, int catalogId);
    void addEffect(const cwnet::EffectRecord &effect);     // to the live effect objects
    // Whether creature `id` is in this world, the local player included.
    bool hasCreature(int64_t id) const { return (hasPlayer && id == localId) || record(id) != nullptr; }
    // A dead creature, as the frame routine collects them for their death effects (health at or
    // below 0, not race 0x11d).
    bool isDead(int64_t id) const;
    // Zone states: a host's answer to a zone request, and a client's taking of the host's lists.
    bool zoneAnswer(ExtractedWorld &ew, const cwnet::ZoneKey &zone, cwnet::ZoneAnswer &out);
    void replaceGroundItems(ExtractedWorld &ew, const cwnet::ZoneObjects &zone);
    void replaceStaticObjects(ExtractedWorld &ew, const cwnet::ZoneObjects &zone);
    void applyBlockChange(const cwnet::BlockChangeRecord &change);
    // The zones whose ground items a host's lists replaced since the last call.
    std::set<std::pair<int, int>> takeReplacedGround();
    // A monster's hit on CubeVulkan's own player (when the player is not the native one).
    void setPlayerNumber(std::function<void(const std::array<uint8_t, 0x48> &number)> f) { playerNumber = std::move(f); }
    // Observe visual-number insertion without applying damage or changing network lists.
    void setHitVisualObserver(std::function<void(const cwnet::DamageRecord &)> f) { hitVisualObserver = std::move(f); }
    void notifyHitVisual(const cwnet::DamageRecord &number) { if (hitVisualObserver) hitVisualObserver(number); }
    // The zones whose state a client still needs (zone+0xf8): set for every zone in memory on
    // connecting and for each zone that comes into memory; cleared when the host answers.
    void zoneAnswered(const cwnet::ZoneKey &zone) { stateNeeded.erase({zone.x, zone.y}); }
    std::vector<cwnet::ZoneKey> zonesNeedingState(int64_t blockX, int64_t blockY) const;
    // Connecting: every loaded zone's block changes undone and its state needed again (0x2da090).
    void undoBlockChanges(ExtractedWorld &ew);
    // The zones whose ground items or static objects changed since the last call (0x2e30b0):
    // marked as ever changed, their whole lists added to the event lists.
    void zoneChangeLists();
    // The world's clock: day (world+0x08) and time of day (world+0x0c, ms). A deciding world runs
    // it at `gameMsPerMs` game milliseconds a real one (CubeVulkan's [Time] MinutesPerSecond);
    // a mirror takes its host's (packet 8).
    int32_t day() const { return dayCount; }
    float timeOfDay() const { return dayMs; }
    float elapsedWorldTime() const { return elapsedWorldMs; }
    void setClock(int32_t day, float timeOfDayMs) { dayCount = day, dayMs = timeOfDayMs; }
    void setClockRate(double gameMsPerMs) { clockRate = gameMsPerMs; }
    void advanceClock(double dtMs);
    // The world reset (native-saves.md §5.6, 0x2c34c0, the world's side): the dead set and the
    // stored zone states emptied, every creature not a player let go (they are made again).
    void worldReset();
    const std::set<int64_t> &worldDeadSet() const { return worldDead; }
    void setEnemyRespawn(enemyrespawn::Settings settings);
    void advanceRespawns(double elapsedMs);
    // Temporary enemy deaths stay in the live world, but never become permanent save entries.
    std::set<int64_t> persistentDeadSet() const;
    // Lays an item (0xa0 bytes) on the ground at `pos` (1/65536 block), `count` of it, as a drop
    // lays one (0x2a5b70): into its zone's ground items. For mods and tests.
    void layItem(ExtractedWorld &ew, const uint8_t *item, const int64_t pos[3], int count);
    void markDead(int64_t id) { worldDead.insert(id); } // a kill credit's dead (world+0x20)
    void clearDead() { worldDead.clear(); }
    // A world update's lists the client's own consumers take (netcode-roles.md §8 step 9):
    // sounds (also heard), particles, block changes, pick-ups, kill credits, feats, calls, sleeps.
    void keepReceived(const cwnet::WorldEvents &received);
    // The local player's record, native or not (null before it exists).
    const uint8_t *localRecord() const { return hasPlayer ? player.record.data() : nullptr; }
    // Runs the game's creature update for `dtMs` milliseconds of play: its physics (0x2bb8e0,
    // NativeUpdate) in the world's steps of at most 16 ms, as the game's world update does.
    void simulate(ExtractedWorld &ew, double dtMs);
    // Runs the game's creature AI (0x13830, NativeAI): every 40 ms, as the game's AI thread
    // does, each creature picks what it does and the way it wants to go (+0x40), which the
    // update then follows. `hour` is the time of day (townsfolk sleep 20:00 to 7:00).
    void think(ExtractedWorld &ew, double dtMs, float hour);
    // [Creatures] RoadTravel, CubeVulkan's own and off by default: groups of wild creatures
    // and travellers walk the roads from place to place. The game's AI does the walking:
    // each pack keeps behind its leader (its group, as the game's travellers do), and the
    // leader heads home (+0x1b8) whenever it is more than 4 blocks from it (+0xbcc bit 1),
    // its home being moved along the road ahead of it.
    void setRoadTravel(bool on) { roadTravel = on; }
    // The player, as the AI sees it: a creature (id 1, hostility 0) at `local` (local blocks),
    // facing `headingRad`, moving at `velocity` (blocks a second), with its health and level.
    void setPlayer(const ExtractedWorld &ew, glm::vec3 local, float headingRad, glm::vec3 velocity, float health,
                   int level, bool onGround);
    // What a creature's hit on the player does (the target's side of 0x29eb60, CubeVulkan's
    // player model): whether it landed. The attacker's record is given.
    // [Player] Native: the player is the game's own creature, moved by its controller
    // (0x9c050, NativeControl) and the world's update like any other (its physics, abilities
    // and upkeep); setPlayer then places it only the first time.
    void setNativePlayer(bool on) { nativePlayer = on; }
    // Loading a character (frontend-title.md §6.1, native-saves.md §3.2): the player's record
    // and the lists beside it start again from a freshly made creature, so nothing of what it
    // was doing survives (velocity, ability and its time, the eagle's flight fields, buffs,
    // physics flags). The next setPlayer makes it and places it.
    void resetPlayer() {
        player.side = nativeai::Creature{};
        hasPlayer = placed = false;
    }
    // A creature's record stands about a block above the surface its drawn feet are on
    // (a townsperson on ground 133 is at 134.08, its feet drawn at 133.0); CubeVulkan's
    // player position is its feet, and its world points are a block below the game's.
    static constexpr float NATIVE_FEET = 1.0f;
    // The controller's frame for the player (dt in ms, once a frame as the game calls it).
    void control(const nativecontrol::Input &in, double dtMs);
    // The native player's place (local blocks), facing (radians, as setPlayer takes it),
    // velocity (+0x34, blocks a second) and whether it stands on the ground. False before it exists.
    bool playerState(const ExtractedWorld &ew, glm::vec3 &local, float &headingRad, glm::vec3 &velocity, bool &onGround) const;
    // The native player's ability (+0x68) and ms into it (+0x6c); 0 before it exists.
    int playerAbility(float &ms) const;
    // The take-off on the eagle (the mouse handling on a known flight point): the native
    // player flies from where it stands (+0x1b8) to block (x, y) (+0x1b0) in ability 0x9c.
    bool startFlight(int32_t x, int32_t y);
    // Accepted known shrine cell: ability0x75, time0 and home-cell fields; travel happens later.
    bool startShrineReturn(int32_t cellX, int32_t cellY);
    // The native player's health is its record's: the game's hits (0x29eb60) and healing
    // change it. Its maximum by the game's rules (0x5fac0), with its buffs.
    float playerHealth() const;
    float playerMaxHealth() const;
    void setPlayerHealth(float hp);
    // Using an item from the bag, as the game's input does (0x8ae50's use of an item): food
    // and potions start eating (0x55) or drinking (0x54) with the item in hand (+0x1d8),
    // which the ability step (0x2c5337) heals by. False when stunned or not native.
    bool useItem(const uint8_t *item);
    // The player activates creature `id` (an interaction of kind 3, as after talking to it
    // or using it), which the interaction step acts on.
    void activate(int64_t id);
    // The native player as the world keeps it: its record and its pending interactions
    // (+0xab8), which the interaction step (0x2c3710, NativeInteract) acts on. Null before
    // it exists or when the player is not native.
    nativeai::Creature *localPlayer();
    // The whole time of the native player's current ability in ms (0x4f220), 0 before it exists.
    float playerAbilityDuration() const;
    // A zone's static objects as the world's AI and interactions keep them (0xe0 bytes each:
    // a door's shut byte +0x28, a seat's user +0x30); null until the zone is in memory.
    const std::vector<nativeai::Object> *zoneObjects(int zx, int zy) const;
    std::optional<objectvisual::State> objectVisual(int zx, int zy, const objectvisual::Key &key) const;
    void setUsedCells(const std::set<nativesaves::Cell> *cells) { usedCells = cells; }
    // --- What changed in the zones (ZoneStates.h, native-saves.md §5.3) ---
    // The world's stored zone states (Save/world.db): put on each zone as it is made, recorded
    // into as it leaves memory. Null: zones are kept for the session only.
    void setStoredZones(ExtractedWorld &ew, nativesaves::WorldState *world, bool serverMode);
    // A zone's ground items (zone +0x30, 0xe0 bytes each; an empty slot's item type is 0), made
    // with the zone when it comes into memory; null when its blocks are not in memory yet.
    const std::vector<nativesaves::Record> *groundItems(ExtractedWorld &ew, int zx, int zy);
    // The player took ground item `index` of a zone: its slot is emptied.
    void takeGroundItem(int zx, int zy, size_t index);
    // A static object opened, or given its item (+0x28 cleared; a chest stays open).
    void setObjectActive(int zx, int zy, size_t index, bool active);
    // A block of the world changed to `block` (r, g, b, -, type, flags): kept in its zone's list.
    void changeBlock(ExtractedWorld &ew, int32_t x, int32_t y, int32_t z, const std::array<uint8_t, 6> &block);
    // The zones that left memory since the last call (their ground items are no longer kept).
    std::vector<std::pair<int, int>> takeUnloadedZones() { return std::exchange(unloadedZones, {}); }
    // Every zone's state recorded, as when the world closes or the title is shown.
    void recordAllZones();
    // The cast bar (Interface's update): shown while a spell is cast, stealth or block power
    // is not full, or an assassination strikes; its fill, and its name: a literal
    // ("Stealth", "Assassinate", "Block Power") or the ability being cast (`ability`).
    struct CastBar {
        bool shown = false;
        float fraction = 0;
        std::string label;
        int ability = 0;
    };
    CastBar castBar() const;
    // What the character panel shows of the native player (the character panel's drawing): the
    // weapon level (the weapons' rarities, averaged unless one is two-handed), the armour
    // level (shoulder, chest twice, hands, feet; / 5), resistance, haste (the attack tempo)
    // and regeneration (the regeneration: 1 plus the items').
    struct Sheet {
        bool valid = false;
        float weaponLevel = 0, armorLevel = 0, resistance = 0, tempo = 1, regeneration = 1;
    };
    Sheet sheet() const;
    // The native player's record (the creature item stats are computed for), null before it exists.
    const uint8_t *playerRecord() const { return nativePlayer && hasPlayer ? player.record.data() : nullptr; }
    // The class's special skill (0 for none) and the ms of its cooldown left.
    int specialSkill(float &cooldownMs) const;
    // Which special items (category 24) the player carries for the region it is in
    // (the special item check, 0x5e890): the hang glider (1) and the boat (2).
    void setSpecialItems(bool glider, bool boat) { hasGlider = glider, hasBoat = boat; }
    // The native player's artifact skills (the level and artifact skills, +0xb5c..+0xb74):
    // climbing, swimming, diving stamina, riding, gliding, sailing and lamp radius, one for
    // each artifact of that kind (its modifier % 7) it carries.
    void setPlayerArtifacts(const std::array<int32_t, 7> &skills);
    // The native player's mana (+0x188, 0..1) and stamina (+0x9a0, 0..1).
    float playerMana() const;
    float playerStamina() const;
    // The native player's toggles (+0x128): 1 lamp, 0x40 fast movement, 0x400 the leaning pose of aiming.
    uint32_t playerToggles() const;
    void setPlayerToggle(uint16_t bit, bool on);
    // The native player's buff of a type (0x5e3a0): its strength (+4) and ms left (+8); false
    // when it has none. Giving one replaces the type's (0x29d2f0 with replace); ending it
    // takes it away (0x2d2b40).
    bool playerBuff(int type, float &strength, float &ms) const;
    void givePlayerBuff(int type, float strength, float ms);
    void endPlayerBuff(int type);
    // The world's dead (world+0x20): creatures that died here or in the owner's game. The
    // game keeps them dead: a zone does not make them again.
    void setWorldDead(const std::set<int64_t> &ids) { worldDead = ids; }
    bool isWorldDead(int64_t id) const { return worldDead.count(id) != 0; }
    // A near zone's mission creatures (zone+0x498), by absolute zone; empty when it is not
    // near or has none.
    std::vector<int64_t> missionIds(int zx, int zy) const;
    // Who the native player is and what it wears (NativePlayer's record): race, class,
    // specialisation, level, flags, region and the 11 worn items.
    void setPlayerGear(const worldgen::CreatureRecord &from, bool clampHealth = true);
    void setPlayerHit(std::function<bool(const nativeupkeep::Hit &, const uint8_t *attacker)> f) { playerHit = std::move(f); }
    size_t roadTrips() const { return trips.size(); }
    // The first pack's waypoint (absolute blocks), for the log; false without one.
    bool firstTrip(double &x, double &y) const {
        if (trips.empty())
            return false;
        x = trips.begin()->second.x, y = trips.begin()->second.y;
        return true;
    }
    // Poses every creature within `range` blocks (dt in milliseconds) and adds its parts.
    void draw(const ExtractedWorld &ew, World &world, double dtMs, uint32_t nowMs, glm::vec3 playerLocal, float range,
              std::vector<DrawItem> &out);
    size_t count() const;
    // How many creatures stand on the ground (+0x5c bit 0), are in water (bit 1), and in all.
    void contacts(size_t &ground, size_t &water, size_t &all) const;
    // How many want to go somewhere (+0x40, from the AI) and how many follow a found path.
    void walking(size_t &moving, size_t &pathing) const;
    // How many are angry at the player (+0xb88 holds its id).
    size_t afterPlayer() const;
    // Whether a monster (hostility 1) that has the player among its targets (+0xb88) is a
    // boss (+0x80 bit 0x40000 or 0x200), which plays the boss music (sound.md §3.3 step 5).
    bool bossAfterPlayer() const;
    // The sound requests the world made since the last call (world +0xa0, 0x30 bytes each):
    // hits, abilities, footsteps and water, upkeep, effects and interactions, in the order
    // the steps made them (sound.md §5.2). Audio plays them.
    std::vector<soundrules::RequestRecord> takeSounds() { return std::exchange(sounds, {}); }
    // For the log: the nearest creature angry at the player (empty if none): its race,
    // ability, time into it, yaw, the bearing to the player (degrees) and distance.
    std::string angriest(const ExtractedWorld &ew, glm::vec3 playerLocal) const;
    // The nearest living person (townsfolk, hostility 3; travellers, 4) or object to examine
    // (8: cages, chests, sacks) within `range` blocks: its id, race
    // and place (local blocks). False if none.
    bool nearestPerson(const ExtractedWorld &ew, glm::vec3 from, float range, int64_t &id, int &race, glm::vec3 &at) const;
    // Where creature `id` is (local blocks); false if it is gone.
    bool positionOf(const ExtractedWorld &ew, int64_t id, glm::vec3 &at) const;
    // A creature made in play (worldgen::makeInPlay: the game's 0x2b67b0) at `dots` in
    // `region`: kept with the near zone it stands in, and let go with that zone. Returns its
    // id, 0 when that zone is not near.
    int64_t make(int race, int hostility, int level, const int64_t dots[3], worldgen::Int2 region);
    // A near creature's game record (null when it is gone), and letting one go at once.
    uint8_t *record(int64_t id);
    const uint8_t *record(int64_t id) const;
    void remove(int64_t id);
    // Every near creature's record.
    void forEach(const std::function<void(const uint8_t *)> &f) const;
    // A model id's sprite (sprites/<name>.cub); empty when there is none.
    std::string spriteName(int id) const { return id >= 0 && (size_t)id < sprites.size() ? sprites[(size_t)id].name : std::string(); }
    // While `change` alters what the creatures' maximum health depends on (the player's
    // level, under PyroProgression), each living creature keeps its share of its health.
    void keepHealthShares(const std::function<void()> &change);
    // Creature `id`'s class (+0x14c: 132 innkeepers, 128-130 and 136 vendors, ...); -1 if gone.
    int classOf(int64_t id) const;
    // Creature `id`'s home (+0x1b8, the origin it was made at), in blocks; false if gone.
    bool homeOf(int64_t id, int32_t &x, int32_t &y) const;
    // The living creatures the player might strike: id and place (local blocks), and the
    // armor (or, for magic, resistance) a hit on each goes through (0x4f080 / 0x64d90).
    struct Target {
        int64_t id = 0;
        glm::vec3 at{0};
        float armor = 0, resistance = 0;
        bool hostile = false; // a monster (hostility 1)
    };
    std::vector<Target> targets(const ExtractedWorld &ew, glm::vec3 near, float range) const;
    // What the creature labels reads of each live creature: its place (local blocks,
    // the record's position), height (appearance size z, +0x8c), hostility, race, level,
    // class, appearance flags, health and maximum (0x5fac0), name (+0x958), faction kind
    // (+0x970) and squared distance to `near`.
    struct Label {
        int64_t id = 0;
        glm::vec3 at{0};
        float height = 0, hp = 0, maxHp = 1, distance2 = 0;
        int hostility = 0, race = 0, level = 0, cls = 0, faction = 0;
        uint32_t flags = 0;
        std::string name;
    };
    std::vector<Label> labels(const ExtractedWorld &ew, glm::vec3 near, float range) const;
    // The player strikes creature `id` with a hit already worth `damage` (CubeVulkan's
    // player attack): the game's hit (0x29eb60, NativeHit) with the player's record as the
    // attacker, pushing it along `dir` (local blocks). Returns the health it lost, or -1 if
    // the hit did not land.
    float strike(int64_t id, float damage, bool critical, float charge, int ability, int type, glm::vec2 dir);
    // Creatures that died since the last call: id, place (local blocks), race, level, and
    // who struck them last (+0x9e0; the player is 1).
    struct Death {
        int64_t id = 0, killer = 0;
        glm::vec3 at{0};
        int race = 0, level = 0;
        std::vector<uint8_t> record; // as it died
        uintptr_t identity = 0; // actor identity retained for native visual-candidate ordering
    };
    std::vector<Death> takeDeaths(const ExtractedWorld &ew);
    // Ids the world counts as dead without a death: a creature tamed takes a new id and its
    // old one joins the dead set (creature-abilities-ranged.md §14.2.1). Since the last call.
    std::vector<int64_t> takeMarkedDead() { return std::exchange(markedDead, {}); }
    // Tames of the local player's pet food since the last call (GroundItems.h): the pet slot is
    // filled already; the old pet goes to the inventory and one pet food leaves it.
    std::vector<grounditems::Tamed> takeTames() { return std::exchange(tames, {}); }
    // Each dying creature's parts as last drawn (world model and placement), once, for the
    // burst into voxels; a dead creature is not drawn after.
    struct Part {
        int model = -1;
        glm::mat4 place{1};
    };
    std::vector<Part> takeBursts() { return std::exchange(bursts, {}); }
    // How many creatures other creatures have killed.
    size_t killed() const { return kills; }
    // For the log: how many times creatures took up a zone object (a seat, a door) and how
    // many of those swung a door.
    size_t objectUses() const { return objectChanges; }
    size_t doorSwings() const { return swings; }
    size_t interactionsHanded() const { return handed; }
    // Nights slept at an inn since the last call (an interaction of kind 5 with the
    // innkeeper at night, 0x2c3710): the day counter goes on by that many and the time of
    // day is then 7:00.
    int takeInnNights() { return std::exchange(innNights, 0); }
    // For the log: the native player's moves started and hits that landed.
    size_t playerMoves() const { return movesStarted; }
    size_t playerStrikes() const { return strikesLanded; }
    size_t playerParts() const { return playerPartsDrawn; } // the native player's parts at the last draw
    size_t heroicShouts() const { return shouts; }
    size_t summonsMade() const { return summoned; }
    // What deaths dropped (0x2a6860 on the dead's record, 0x2a5b70 laying each on the
    // ground): the dead's id and level, the drop as a zone keeps it, and where (local blocks).
    struct Dropped {
        int64_t from = 0;
        int level = 0;
        nativedrop::Drop drop{};
        glm::vec3 at{0};
        bool reward = false; // a freed resident's thanks (0x2a7ca0), not a death's
    };
    std::vector<Dropped> takeDrops() { return std::exchange(drops, {}); }
    // The world's special-item lists (world+0x360, +0x378) from the owner's game
    // (Extract/oracle/items.bin, tools/ZoneExtractor's items.request). False without them.
    bool loadSpecials(const std::filesystem::path &file);
    // The first of them (world+0x360): the creature kinds that can be pets, which are also the
    // ids of their pet food. Townspeople talk about them (TalkGen).
    const std::vector<int32_t> &petList() const { return specials; }
    // The sprite (without sprites/ and .cub) the game draws a game item (0xa0 bytes) with
    // (0x173b10, NativeItems); empty if none.
    std::string itemSprite(const uint8_t *item) const;
    // The player's class (+0x14c), which decides whom gear is for.
    void setPlayerClass(int cls);
    // For the log: how many creatures are in an ability (+0x68), and how many of them
    // started a new one since the last call.
    void abilities(size_t &using_, size_t &started);
    // The nearest creature to `from` (local blocks): its race and place; false if none.
    // With `hostility` >= 0, only creatures of that hostility (1: monsters).
    bool nearest(const ExtractedWorld &ew, glm::vec3 from, int &race, glm::vec3 &at, int hostility = -1) const;
    // Every creature of a hostility: race and place (local blocks), for the log.
    std::vector<std::pair<int, glm::vec3>> list(const ExtractedWorld &ew, int hostility) const;

  private:
    struct Sprite {
        std::string name; // without sprites/ and .cub
        int size[3] = {0, 0, 0};
    };
    struct Creature {
        worldgen::CreatureRecord record;
        nativeai::Creature side; // what the game keeps beside the record: buffs, group, ...
        nativepose::Sparks sparks; // pose state 0x61's sparks, kept between pose calls
        // A mirror's creature between its host's updates (CubeVulkan's, for drawing): the ability
        // (+0x68) and ability time (+0x6c) the host last sent, and the time the mirror shows, which
        // runs on with the frames and is only ever corrected forward (or restarted with a new
        // swing), so a swing never steps back when an update arrives.
        float sentAbilityTime = 0, shownAbilityTime = 0;
        uint8_t shownAbility = 0;
    };
    std::vector<Sprite> sprites;                          // by model id
    // Absolute zone -> its creatures. The creatures the network brings belong to no zone: they are
    // kept under NETWORK_ZONE, a key no zone has, so every walk over the world's creatures sees
    // them (first, as the map orders it) while the zones' comings and goings leave them alone.
    std::map<std::pair<int, int>, std::vector<Creature>> zones;
    static constexpr std::pair<int, int> NETWORK_ZONE{INT32_MIN, INT32_MIN};
    bool client = false;          // world+0x358: a remote client (a mirror of a host)
    int64_t localId = 1;          // the local player's creature id
    cwnet::WorldEvents worldEvents; // world+0x90 .. +0x170
    std::set<std::pair<int, int>> stateNeeded; // zone+0xf8 of the zones in memory (a mirror's)
    int32_t dayCount = 0;         // world+0x08
    double clockRate = 0;         // game ms a real ms (0: the clock is set from outside)
    // The event lists' records, from the native modules' arrays.
    void keepRecords(const std::vector<std::array<uint8_t, 0x30>> &sounds, const std::vector<std::array<uint8_t, 0x48>> &particles,
                     const std::vector<std::array<uint8_t, 0x48>> &numbers, const std::vector<std::array<uint8_t, 0x30>> &buffNotes);
    void keepBlockChanges(const std::vector<std::array<uint8_t, 0x20>> &changes);
    std::set<std::pair<int, int>> groundChanged, objectsChanged; // world+0x190, +0x1a0: zones changed this step
    std::set<std::pair<int, int>> replacedGround;                // zones whose ground items a host replaced
    std::function<void(const std::array<uint8_t, 0x48> &)> playerNumber;
    std::function<void(const cwnet::DamageRecord &)> hitVisualObserver;
    Creature *find(int64_t id); // any creature by id, the local player included
    // The hit routine's world over this world's creatures (`byId` is filled and must outlive it).
    nativehit::World hitWorld(ExtractedWorld &ew, std::map<int64_t, nativeai::Creature *> &byId);
    void keepHitRecords(nativehit::World &hw);
    std::map<int, nativepose::Model> models;              // model id -> what the pose sees
    std::map<std::tuple<int, int, int, int, bool>, int> tinted; // (model id, r, g, b, hair) -> World model
    uint32_t drawn = 0;
    double carriedMs = 0;                                 // the part of a millisecond not yet simulated
    uint32_t randState = 0x12345678;                      // rand() for the creatures' update
    std::map<int64_t, nativeai::Entry> ai;                // the AI's record of each creature
    zonestates::ZoneStates zoneStates; // the zones in memory: their objects, ground items and block changes
    const std::set<nativesaves::Cell> *usedCells = nullptr;
    nativesaves::WorldState *storedZones = nullptr;
    std::vector<std::pair<int, int>> unloadedZones;
    // The zone's lists, made with the zone (its objects from the generator, the stored state on
    // top) once its blocks are in memory; null before.
    nativesaves::ZoneLists *zoneLists(ExtractedWorld &ew, int zx, int zy);
    // The zone's count of dead tracked creatures (zone +0x490), when its creatures were made.
    std::optional<int32_t> deadTrackedIn(std::pair<int, int> zone) const;
    struct RoadTrip {
        double x = 0, y = 0; // the waypoint (absolute blocks)
        float heading = 0;   // radians
        double waitedMs = 0; // since the waypoint was set
    };
    bool roadTravel = false;
    std::map<int64_t, RoadTrip> trips; // by the first id of the pack's group
    void startTrips(std::vector<Creature> &list);
    void advanceTrips(ExtractedWorld &ew, double dtMs);
    Creature player;                                      // the player's record for the AI
    bool hasPlayer = false;
    bool nativePlayer = false;
    size_t movesStarted = 0, strikesLanded = 0, playerPartsDrawn = 0, shouts = 0, summoned = 0;
    // Moves the creatures the interactions made during a frame (shadow clones, water
    // bubbles, bombs) into the zones they stand in.
    void settle(std::list<Creature> &born);
    uint8_t lastPlayerMove = 0;
    bool placed = false; // the native player has been put where the save had it
    bool hasGlider = false, hasBoat = false;
    worldgen::CreatureRecord gearRecord; // what setPlayerGear gave, applied as the player is made
    nativecontrol::State controlState; // the controller's statics
    double aiCarried = 0;                                 // ms since the AI's last tick
    uint32_t aiClock = 0;                                 // the AI thread's timeGetTime()
    uint32_t aiRand = 1;                                  // its rand(): the C runtime's, per thread, from 1
    float dayMs = 0;                                      // the time of day (world+0xc) at the last tick
    float elapsedWorldMs = 0;                             // float world+0x440, separate from AI/GUI/calendar clocks
    std::function<bool(const nativeupkeep::Hit &, const uint8_t *)> playerHit;
    std::vector<soundrules::RequestRecord> sounds; // the world's sound requests, not yet played
    size_t kills = 0; // creatures killed by others, for the log
    size_t objectChanges = 0, swings = 0; // zone objects taken up and doors swung, for the log
    size_t handed = 0;                    // interactions waiting after AI ticks, summed, for the log
    int innNights = 0;                    // nights slept at an inn, for main's day and clock
    std::list<nativeeffects::Effect> effects; // world+0x60: shots and areas in flight
    std::set<int64_t> dead;          // creatures whose death was reported
    std::set<int64_t> rewarded;      // world+0x30: the residents who gave their reward
    std::set<int64_t> worldDead;     // the world's dead (not made again)
    enemyrespawn::Settings respawnSettings{enemyrespawn::Mode::Permanent, 300};
    enemyrespawn::Deaths enemyDeaths;
    std::set<int64_t> protectedDead; // renamed by taming: not a defeated enemy
    void releaseDeaths(const std::vector<int64_t> &ids);
    std::vector<int64_t> markedDead; // dead by taming, for the save
    std::vector<grounditems::Tamed> tames;
    std::map<std::pair<int, int>, std::vector<int64_t>> zoneMissions; // zone+0x498 of the near zones
    // The near zones as the generator made them: their spawn points (zone+0x460) and lists.
    std::map<std::pair<int, int>, std::shared_ptr<const worldgen::ZoneContents>> zoneResidents;
    std::map<std::pair<int, int>, std::mt19937> zoneRand;               // each near zone's draws for its creatures
    std::map<std::pair<int, int>, std::vector<int64_t>> zoneTracked;  // zone+0x480 of every zone made this session
    std::set<int64_t> made;                                           // the residents in the world now
    // The world's side of a near zone: the working copy of its spot map (zone+0x110), which its
    // creatures take cell spots from (0x31c0e0), and what its chests hold (zone+0x450).
    std::map<std::pair<int, int>, worldgen::SpotMap> zoneSpots;
    std::map<std::pair<int, int>, std::map<int, std::array<uint8_t, worldgen::creature::ITEM_SIZE>>> zoneChests;
    worldgen::ResidentWorld residentWorld(const std::vector<worldgen::ResidentWorld::Player> &players) const; // what the resident pass asks (0x2e17d0)
    void arenaStage(worldgen::CreatureRecord &master); // 0x2e3510's arena master
    std::set<int64_t> seenAlive;     // creatures seen alive (only their deaths are reported)
    std::set<int64_t> toBurst;       // ... and whose body has yet to burst
    std::vector<Part> bursts;
    int64_t nextMade = 1ll << 62; // ids of the creatures made in play, apart from the spawn points' (0x119530)
    std::vector<Dropped> drops;
    std::vector<int32_t> specials;                        // world+0x360
    std::map<int32_t, std::vector<int32_t>> specialsBy;   // world+0x378
    ExtractedWorld *lastWorld = nullptr;                  // for deaths outside simulate()
    void dropFrom(ExtractedWorld &ew, nativeai::Creature *dead); // 0x2a6860
    // The drops' world over the near creatures (0x2a6860, 0x2a7ca0, 0x2a5b70), and keeping
    // what it laid.
    nativedrop::World dropWorld(ExtractedWorld &ew);
    void keepDrops(ExtractedWorld &ew, const nativedrop::World &dw, int64_t from, int32_t level, bool reward);
    std::vector<nativeai::Creature *> worldList(); // the player first, then every creature
    std::map<int64_t, uint8_t> lastAbility;               // for abilities(): each one's at the last call
    int worldModel(World &world, int id, const float *tint, bool hair);
};
