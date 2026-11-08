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
// Implementations own the metadata they expose; the Store never borrows an actor.
struct Metadata {
    virtual ~Metadata() = default;
    virtual void capture(nativesaves::WorldState &) const = 0;
};
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
    using Clock = std::function<uint32_t()>;
    void setMetadata(std::shared_ptr<const Metadata>);
    void setPersistence(std::filesystem::path file, bool allowWrites);
    // Configure clock selection while the loader is stopped; a captured replay clock may advance.
    void setHostClock(Clock);
    Clock hostClock() const;
    void persist(); // capture under World guard, serialize/write after releasing it
    std::vector<std::string> takeSaveErrors();
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
    std::shared_ptr<const Metadata> metadata;
    std::filesystem::path saveFile;
    bool allowWrites = false;
    Clock clock;
    std::vector<std::string> saveErrors;
};
} // namespace sourcestates
