#pragma once
// Controller-owned creature copies: steam-use-creature-collection.md. This is
// runtime owner state; no save protocol or world-transition clearing is inferred.
#include "worldgen/Residents.h"
#include <functional>
#include <map>

namespace usecollection {
class Collection {
  public:
    Collection() = default;
    Collection(const Collection &) = delete;
    Collection &operator=(const Collection &) = delete;
    using Resolve = std::function<const uint8_t *(int64_t)>;
    using Sound = std::function<void(int)>; // immediate, retained camera/listener, volume/pitch 1

    int64_t selectedId = 0;
    int32_t selectionIndex = 0;
    uint8_t *find(int64_t id);
    const uint8_t *find(int64_t id) const;
    uint8_t *add(const uint8_t *source);
    bool remove(int64_t id);
    size_t size() const { return copies.size(); }
    const uint8_t *wrapped() const;

    // Called after prompt suppression/Fall/Climb, before Revive. Resets do not
    // consume Use or end the prompt. effectTime is 0x4ead0(player,-1), not duration.
    void complete(uint8_t *player, float effectTime, const Resolve &, const Sound &);

  private:
    std::map<int64_t, worldgen::CreatureRecord> copies;
    void selectAdded(int64_t id);
};
} // namespace usecollection
