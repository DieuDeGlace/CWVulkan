#pragma once
#include "Combat.h"
#include "EnemyRespawn.h"
// Live creatures and NPCs built on the recordings in Creatures.h. The game's
// AI is code inside cubeworld.exe and cannot be extracted, so behaviour here is
// CubeVulkan's own, kept simple and driven by the game's own data:
//   * every pose comes from the creature's recording (idle, walk and move
//     clips cut from it), so animation, proportions and gait are the game's;
//   * walking speed is the speed the game moved that creature at;
//   * reactions follow the game's hostility type for the creature.
#include "Creatures.h"
#include <functional>
#include <random>
#include <utility>
#include <vector>

class Actors {
  public:
    // Ground under a point for walking creatures (top of the floor at or below z).
    using Floor = std::function<bool(float x, float y, float z, float &height)>;

    void build(const CreatureRecordings &recordings, bool none = false);
    void setEnemyRespawn(enemyrespawn::Settings settings) { respawnSettings = settings; }
    // `playerDefense` and `playerDamage`: monsters in reach hit the player; the
    // damage dealt this frame is added to `playerDamage`.
    void update(double dt, glm::vec3 player, const Floor &floor, combat::Defender playerDefense, float &playerDamage,
                double *playerStun = nullptr);
    // Several players (multiplayer host): each creature reacts to the nearest one;
    // `damage[p]` gets what player p took this frame.
    void update(double dt, const std::vector<glm::vec3> &players, const Floor &floor,
                const std::vector<combat::Defender> &defenses, std::vector<float> &damage,
                std::vector<double> *stuns = nullptr); // stuns[p]: seconds player p is stunned for (0: not)
    // Multiplayer: what the host sends about creature i, and what a client applies;
    // clients only advance animation between updates (advance), the host decides.
    struct Sync {
        glm::vec3 position{0};
        float heading = 0, stateTime = 0, animTime = 0, walked = 0, hp = 0, deathTime = 0;
        int state = 0;
        bool dead = false;
    };
    Sync sync(size_t i) const;
    void apply(size_t i, const Sync &s);
    void advance(double dt);
    bool driven(size_t i) const { return actors[i].netDriven; }
    // Combat: only monsters can be hurt (the game's townspeople and pets cannot).
    bool hittable(size_t i) const { return actors[i].kind == Kind::Monster && !actors[i].dead; }
    bool townsperson(size_t i) const { return actors[i].kind == Kind::Townsperson && !actors[i].dead; }
    // Applies `amount` of damage; true if it killed the creature.
    bool hurt(size_t i, float amount);
    float armor(size_t i) const { return actors[i].armor; }
    combat::Defender defense(size_t i) const { return {actors[i].armor, actors[i].resistance, &actors[i].effects}; }
    unsigned flags2(size_t i) const { return actors[i].flags2; }
    // The game's id of the creature (from its recording; 0 if unknown).
    int64_t creatureId(size_t i) const;
    // Marks a creature that died before (in the game or an earlier session): it stays
    // dead, as the game keeps it.
    void alreadyDead(size_t i);
    // Stuns it for `seconds` (nothing happens within 3 s of its last stun ending, as in
    // the game); true if it took.
    bool stun(size_t i, double seconds);
    // Poisons it for 3 s worth `value` a tick (Combat.h poisonTick; ability 18).
    void poison(size_t i, float value);
    // The poison hits since the last call (creature, damage after armor), for the
    // caller to apply like any hit (hurt, numbers, kills).
    std::vector<std::pair<size_t, float>> takePoisonHits() { return std::exchange(poisonHits, {}); }
    // Scales the creature's recorded health, attack and armor (its level changed).
    void strength(size_t i, float factor);
    float health(size_t i) const { return actors[i].hp; }
    float maxHealth(size_t i) const { return actors[i].maxHp; }
    double deathTime(size_t i) const { return actors[i].deathTime; } // seconds since it died
    // The game shows no body once a creature dies: it bursts into its voxels at once
    // (DeathEffects.h), and pose() still gives where they were.
    bool visible(size_t i) const { return !actors[i].dead; }
    // Fighting or recently struck (its health plate is shown).
    bool engaged(size_t i) const {
        const Actor &a = actors[i];
        return !a.dead && a.kind == Kind::Monster && (a.state == State::Chase || a.state == State::Attack || a.sinceHurt < 5);
    }
    size_t count() const { return actors.size(); }
    // Body-part transforms (local blocks) for actor `i`, parts as in its recording.
    void pose(size_t i, std::vector<glm::mat4> &out) const;
    size_t recordingOf(size_t i) const { return actors[i].recording; }
    glm::vec3 position(size_t i) const { return actors[i].position; }
    // Human-readable counts of what the actors are doing (for the log).
    std::string summary() const;

  private:
    // A run of recorded frames, in the creature's body frame (facing +x).
    struct Clip {
        std::vector<size_t> frames;  // indices into the recording
        std::vector<double> times;   // seconds from the clip start
        std::vector<double> covered; // blocks travelled from the clip start
        double duration = 0, distance = 0;
        bool empty() const { return frames.size() < 2; }
    };
    enum class Kind { Monster, Water, Townsperson, Companion, Fixed };
    enum class State { Idle, Wander, Chase, Face, Flee, Attack };
    struct Actor {
        size_t recording = 0;
        Kind kind = Kind::Fixed;
        Clip idle, walk, move; // move: a recorded attack or other action
        float speed = 0;       // blocks per second the game walked it at
        float forward = 0;     // movement direction in the body frame
        float lift = 0;        // root height above the floor it stood on
        glm::vec3 home{0}, position{0}, target{0};
        float heading = 0;
        State state = State::Idle;
        double stateTime = 0, stateLength = 0, animTime = 0, walked = 0;
        bool grounded = true;
        float hp = 0, maxHp = 0, attack = 0, spell = 0, armor = 0, resistance = 0, critical = 0;
        float baseHp = 0, baseAttack = 0, baseSpell = 0, baseArmor = 0, baseResistance = 0; // as recorded
        int ability = 0;       // the game's ability id of its recorded attack (sets its damage multiplier)
        int combo = 0, maxCombo = 30; // landed hits; armor stops counting as it reaches maxCombo
        double sinceHit = 99;  // seconds since it last landed a hit (the combo resets after 4)
        unsigned flags2 = 0;   // the game's appearance flags (0x40000: elite)
        combat::Effects effects;
        double stunned = 0, sinceStun = 99; // stun left; seconds since the last one ended
        bool returning = false; // gave up a chase: walking home
        bool netDriven = false;  // multiplayer client: the host moves it
        glm::vec3 netTarget{0};  // where the host last put it
        bool dead = false;
        double deathTime = 0, cooldown = 0, sinceHurt = 99;
    };
    const CreatureRecordings *source = nullptr;
    std::vector<Actor> actors;
    std::vector<std::pair<size_t, float>> poisonHits;
    std::mt19937 random{1234};
    enemyrespawn::Settings respawnSettings{enemyrespawn::Mode::Permanent, 300};
    void revive(Actor &actor);
    std::vector<glm::mat4> bodyFrame(const CreatureRecordings::Creature &c, size_t frame) const;
    void enter(Actor &a, State s, double length);
};
