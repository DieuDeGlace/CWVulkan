#pragma once
// The player character as the game itself poses it. tools/ZoneExtractor saves
// the player's body-part sprites (Extract/character/snapshot.txt) and, while
// recording, the game's per-part world matrices every tick (anim-*.cwa).
// CubeVulkan replays those matrices, so the character's proportions, offsets
// and animation frames are the game's own rather than a re-creation.
#include <glm/glm.hpp>
#include <filesystem>
#include <string>
#include <vector>

class CharacterRecording {
  public:
    struct Part {
        std::string sprite; // e.g. "elf-head-m04" (no folder or extension)
        int matrix = 0;     // Index into the game's animation state matrices.
        glm::u8vec3 tint{255}; // Colour the game multiplies the sprite by (hair colour).
        bool hair = false;
    };
    enum class Motion { Idle, Walk, Air };

    // Loads one recorded character folder (Extract/characters/<name>-<class>, see
    // Saves.h). False (with a reason) if missing.
    bool load(const std::filesystem::path &dir, std::string &error);
    const std::vector<Part> &parts() const { return partList; }
    // A character made in CubeVulkan wears its own head, hair (in its colour), hands
    // and feet on this recording (the game's part matrices: face 3, hair 4, hands
    // 9/10, feet 15/16). Empty names keep the recorded sprite.
    void restyle(const std::string &head, const std::string &hair, const std::string &hand, const std::string &foot,
                 glm::u8vec3 hairColor);
    size_t frameCount() const { return frames.size(); }
    // Direction the character faces in the body frame (radians from +x), from
    // the walk recording: rotate by (heading - forward()) to face `heading`.
    float forward() const { return forwardAngle; }
    // The game's combat numbers for the character (snapshot "stats" line; 0 if absent).
    float maxHp = 0, attack = 0, spell = 0, armor = 0, resistance = 0, critical = 0, haste = 0;
    float manaGeneration = 0; // snapshot "managen=" (newer captures; 0 when absent)
    // The right-hand weapon's item id (category 3; -1 for none or unrecorded): sets
    // how many hits it takes to ignore armor (combat::maxCombo).
    int weaponId = -1;
    // Body width in blocks (snapshot "scales hitbox="; 0.96 for every recorded player).
    // The game's collision keeps the body this wide, centred on the position.
    float hitboxScale = .96f;
    std::string name; // the character's name in the game
    int level = 0, specialization = 0;
    int race = 2;         // snapshot "race=" (2: human)
    uint32_t flags2 = 0;  // snapshot "flags2=": the appearance flags (+0x80)
    std::string summary() const;
    // Part matrices relative to the character: x forward along yaw 0, feet on z = 0.
    // `time` is seconds spent in this motion; `distance` blocks walked (drives
    // the walk cycle so feet do not slide).
    void pose(Motion motion, double time, double distance, std::vector<glm::mat4> &out) const;
    // The game's own animation for ability `id` (an attack, dodge, skill...), `time`
    // seconds after it started. False once the recorded ability has ended, or
    // if it was never recorded.
    bool abilityPose(int id, double time, std::vector<glm::mat4> &out) const;
    // Horizontal speed the game recorded `time` seconds into ability `id` (rolls move).
    float abilitySpeed(int id, double time) const;
    // Recorded ability ids with their durations (seconds).
    std::vector<std::pair<int, double>> abilities() const;
    // Where ability `id` was first recorded: seconds into its recording file (-1 if never).
    double abilityStart(int id) const;
    double abilityDuration(int id) const;

  private:
    struct Frame {
        double t = 0;           // seconds since recording start
        float speed = 0;        // horizontal speed, blocks per second
        float vertical = 0;     // vertical velocity
        float heading = 0;      // movement direction in the body frame (radians)
        int segment = 0;        // which recording file
        int ability = 0;        // the game's current ability id (attacks, dodge, ...)
        int state = 0;          // the game's animation state (0 when no move is playing)
        float phase = 0;        // seconds since the native ability started
        float yaw = 0;          // entity facing, in radians (the source stores degrees)
        std::vector<glm::mat4> local; // one per part, normalised to the body frame
    };
    std::vector<Part> partList;
    std::vector<Frame> frames;
    struct Range {
        size_t first = 0, count = 0;
        double duration = 0, distance = 0; // seconds and blocks covered
        bool phaseClock = false;
    };
    Range idle, walk, air;
    std::vector<std::pair<int, Range>> abilityRuns; // first recorded run of each ability id
    float forwardAngle = 0;
};
