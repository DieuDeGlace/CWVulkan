#pragma once
// Mutable source payloads and retained values have one owned World lifetime.
// Callers keep lock() across raw list views and compound World operations.
#include "NativeSaves.h"
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

namespace worldgen { struct ZoneContents; }
namespace sourcestates {
using nativesaves::Cell;
using nativesaves::Record;
struct Source {
    Cell key{};
    uint64_t order = 0;
    nativesaves::ZoneLists lists;
    std::shared_ptr<const worldgen::ZoneContents> catalog;
};
class Store {
  public:
    using Guard = std::unique_lock<std::recursive_mutex>;
    using Publish = std::function<void(Cell, const std::vector<Record> &)>;
    Guard lock() const { return Guard(mutex); }
    // Replace retained values and the fresh-source gate; loaded instances remain intact.
    void configure(const nativesaves::WorldState *initial, bool skipRestore);
    bool configured() const;
    void reset();
    std::shared_ptr<Source> make(Cell, uint64_t order, std::vector<Record> ground,
                                 std::vector<Record> objects, std::shared_ptr<const worldgen::ZoneContents> catalog);
    void install(const std::shared_ptr<Source> &);
    std::shared_ptr<Source> find(Cell) const;
    std::vector<Cell> keys() const;
    void retain(const std::shared_ptr<Source> &);
    void unlink(const std::shared_ptr<Source> &);
    void clearLoaded();
    void queueGround(Cell);
    void queueObjects(Cell);
    // Snapshot current arrays, then rebuild objects, then clear pending membership.
    // Retired owned instances are discarded safely rather than dereferenced.
    void publish(const Publish &ground, const Publish &objects, const Publish &rebuild);
    int32_t scalar(Cell) const;
    int32_t counter(Cell zone, Cell key) const;
    nativesaves::WorldState snapshot() const;
    // Raw owner access requires lock(); the Store owns this value, never a save-file pointer.
    nativesaves::WorldState &retained() { return archive; }
    const nativesaves::WorldState &retained() const { return archive; }
  private:
    using Pending = std::map<uint64_t, std::weak_ptr<Source>>;
    bool current(const std::shared_ptr<Source> &) const;
    mutable std::recursive_mutex mutex;
    bool ready = false, skip = false;
    nativesaves::WorldState archive;
    std::map<Cell, std::weak_ptr<Source>> loaded;
    Pending groundPending, objectPending;
};
} // namespace sourcestates
