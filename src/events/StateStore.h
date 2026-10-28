#pragma once
#include "EventRules.h"
#include <array>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace encounters {
struct EventKey {
    uint64_t world = 0;
    Region region{};
    unsigned slot = 0;
    bool operator==(const EventKey &other) const {
        return world == other.world && region.x == other.region.x && region.y == other.region.y &&
               slot == other.slot;
    }
};
struct EventKeyHash {
    size_t operator()(const EventKey &key) const {
        return static_cast<size_t>(identity(key.world, key.region, key.slot));
    }
};
struct EventRecord {
    uint64_t world = 0;
    Region region{};
    unsigned slot = 0;
    unsigned cycle = 0;
    Kind kind = Kind::Battle;
    Site site{}, previousSite{};
    unsigned attempt = 0;
    unsigned enemyCount = 0;
    std::array<unsigned, 6> enemyRaces{};
    std::array<unsigned, 6> enemyStars{};
    std::array<float, 6> enemyHPFraction{1.f, 1.f, 1.f, 1.f, 1.f, 1.f};
    float npcHPFraction = 1.f;
    unsigned defeatedMask = 0;
    bool keyOwned = false;
    bool keySearchFound = false;
    bool objectiveDone = false;
    bool rewardClaimed = false;
    uint64_t deadlineUTCsecs = 0;
    uint64_t emptySince = 0;
    bool planned = false;
    bool failed = false;
    // Accepted village objectives survive world reloads, including plans whose
    // terrain has not loaded yet. Native actor IDs are always session-local.
    bool townQuest = false, awaitingSite = false;
    Site town{};
    unsigned difficultyLevel = 1;
};

class StateStore {
  public:
    using Records = std::unordered_map<EventKey, EventRecord, EventKeyHash>;
    static constexpr unsigned MaxSlots = 32;
    static constexpr size_t MaxRecords = 65536;

    EventRecord &get(uint64_t world, Region region, unsigned slot);
    const EventRecord *find(uint64_t world, Region region, unsigned slot) const;
    size_t size() const {
        return records_.size();
    }
    const Records &entries() const {
        return records_;
    }
    // An encounter is stable within a world session. A new session rerolls
    // unfinished instances without discarding completed rewards/cooldowns.
    size_t restartUnfinished(uint64_t world);
    void clear() {
        records_.clear();
    }

    // Explicit load/save keeps disk operations out of the frame loop. Loading a
    // malformed file preserves the existing in-memory state in its entirety.
    bool load(const std::filesystem::path &path, std::string *error = nullptr);
    bool save(const std::filesystem::path &path, std::string *error = nullptr) const;
    static bool valid(const EventRecord &record);

  private:
    Records records_;
};
} // namespace encounters
