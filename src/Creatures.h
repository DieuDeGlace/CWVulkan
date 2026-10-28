#pragma once
// Creatures and NPCs recorded from the user's game by tools/ZoneExtractor (F3):
// each one's sprites, hair colour, and ten seconds of its real positions and
// poses (Extract/seed_<n>/creatures). CubeVulkan replays them in a loop where
// the game had them, so townspeople, animals and enemies stand, walk and look
// exactly as the game drew them.
#include <glm/glm.hpp>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

class CreatureRecordings {
  public:
    struct Part {
        std::string sprite;
        glm::u8vec3 tint{255};
        bool hair = false; // Hair sprite: marker voxels hidden, greys tinted.
        int matrix = -1;   // which animation state matrix the game draws it with
    };
    struct Creature {
        uint64_t id = 0; // the game's creature id
        int race = 0, hostility = 0;
        int level = 0;
        // The game's combat numbers (0 when the capture predates stat recording).
        float maxHp = 0, attack = 0, spell = 0, armor = 0, resistance = 0, critical = 0;
        // Appearance flags used by the native drop rules. Older exports omit
        // them; unknown must remain distinguishable from a recorded zero.
        std::optional<uint32_t> flags2;
        // What it wears, in the game's 11 equipment slots (unk, neck, chest, feet,
        // hands, shoulder, weapon left, weapon right, ring left, ring right, pet);
        // category 0 = empty. From the survey's equip= field.
        struct Worn {
            int category = 0, id = 0, material = 0, rarity = 0;
            std::optional<uint32_t> modifier; // fifth field in newer exports
        };
        std::vector<Worn> worn;
        std::vector<Part> parts;
        // Per frame: time (s), position (local blocks) and one matrix per part
        // relative to the position (a zero matrix hides the part that frame).
        std::vector<double> times;
        std::vector<glm::vec3> positions;
        std::vector<std::vector<glm::mat4>> poses;
        std::vector<float> angles; // facing per frame (radians, from the root matrix)
        std::vector<int> states;   // the game's animation state per frame (0 = no move playing)
        std::vector<int> abilities; // the game's ability id per frame (0 = none)
    };
    struct LoadStats {
        size_t listed = 0, loaded = 0, missingAppearance = 0, missingFrames = 0, missingPose = 0;
    };
    // `toLocal` converts a game position in dots to local blocks.
    size_t load(const std::filesystem::path &dir, const std::function<glm::vec3(const int64_t *)> &toLocal);
    const std::vector<Creature> &all() const { return creatures; }
    const LoadStats &stats() const { return loadStats; }
    // Frame of creature `c` at `time` (looping its recording).
    size_t frameAt(const Creature &c, double time) const;

  private:
    std::vector<Creature> creatures;
    LoadStats loadStats;
};
