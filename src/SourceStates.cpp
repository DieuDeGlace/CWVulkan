#include "SourceStates.h"
#include <utility>
#include <stdexcept>

namespace sourcestates {
void Store::configure(const nativesaves::WorldState *initial, bool skipRestore) {
    auto guard = lock();
    archive = initial ? *initial : nativesaves::WorldState{};
    skip = skipRestore;
    // Retained-map replacement does not reset an already loaded source or its
    // sticky flags/pending publication. Only physical retirement ends that life.
    ready = true;
}
bool Store::configured() const { auto guard = lock(); return ready; }
void Store::reset() {
    auto guard = lock();
    archive = nativesaves::WorldState{};
    loaded.clear(); groundPending.clear(); objectPending.clear();
    skip = false; ready = false;
}
std::shared_ptr<Source> Store::make(Cell key, uint64_t order, std::vector<Record> ground,
                                   std::vector<Record> objects, std::shared_ptr<const worldgen::ZoneContents> catalog) {
    auto guard = lock();
    auto source = std::make_shared<Source>();
    source->key = key;
    source->order = order;
    source->catalog = std::move(catalog);
    source->lists.groundItems = std::move(ground);
    source->lists.staticObjects = std::move(objects);
    nativesaves::applyStoredZone(archive, key, source->lists, skip, nullptr);
    return source; // Restoration precedes physical installation; replay belongs to the reader.
}
void Store::install(const std::shared_ptr<Source> &source) {
    auto guard = lock();
    if (!source) return;
    if (auto existing = find(source->key); existing && existing != source)
        throw std::logic_error("A physical source must retire before its replacement installs");
    loaded[source->key] = source;
}
std::shared_ptr<Source> Store::find(Cell key) const {
    auto guard = lock();
    const auto found = loaded.find(key);
    return found == loaded.end() ? nullptr : found->second.lock();
}
bool Store::current(const std::shared_ptr<Source> &source) const {
    return source && find(source->key) == source;
}
std::vector<Cell> Store::keys() const {
    auto guard = lock();
    std::vector<Cell> out;
    for (const auto &[key, source] : loaded)
        if (!source.expired()) out.push_back(key);
    return out;
}
void Store::retain(const std::shared_ptr<Source> &source) {
    auto guard = lock();
    if (source) nativesaves::recordZone(archive, source->key, source->lists);
}
void Store::unlink(const std::shared_ptr<Source> &source) {
    auto guard = lock();
    if (!current(source)) return;
    loaded.erase(source->key);
    // Native per-source cleanup is unproved. Owned queues cannot follow a freed
    // or later reused instance, even if a separate immutable terrain lease survives.
    if (const auto at = groundPending.find(source->order); at != groundPending.end() && at->second.lock() == source)
        groundPending.erase(at);
    if (const auto at = objectPending.find(source->order); at != objectPending.end() && at->second.lock() == source)
        objectPending.erase(at);
}
void Store::clearLoaded() {
    auto guard = lock();
    loaded.clear();
    groundPending.clear();
    objectPending.clear();
}
void Store::queueGround(Cell key) {
    auto guard = lock();
    if (auto source = find(key)) groundPending[source->order] = source;
}
void Store::queueObjects(Cell key) {
    auto guard = lock();
    if (auto source = find(key)) objectPending[source->order] = source;
}
void Store::publish(const Publish &ground, const Publish &objects, const Publish &rebuild) {
    auto guard = lock();
    for (const auto &[order, weak] : groundPending)
        if (auto source = weak.lock(); current(source)) {
            source->lists.groundItemsChanged = true;
            if (ground) ground(source->key, source->lists.groundItems);
        }
    for (const auto &[order, weak] : objectPending)
        if (auto source = weak.lock(); current(source)) {
            source->lists.staticObjectsChanged = true;
            if (objects) objects(source->key, source->lists.staticObjects);
        }
    for (const auto &[order, weak] : objectPending)
        if (auto source = weak.lock(); current(source) && rebuild)
            rebuild(source->key, source->lists.staticObjects);
    groundPending.clear();
    objectPending.clear();
}
int32_t Store::scalar(Cell key) const {
    auto guard = lock();
    if (auto source = find(key)) return source->lists.deadTracked;
    const auto stored = archive.zones.find(key);
    return stored == archive.zones.end() ? 0 : stored->second.deadTracked;
}
int32_t Store::counter(Cell zone, Cell key) const {
    auto guard = lock();
    const auto read = [&](const auto &counts) {
        const auto found = counts.find(key);
        return found == counts.end() ? 0 : found->second;
    };
    if (auto source = find(zone)) return read(source->lists.placeCounters);
    const auto stored = archive.zones.find(zone);
    return stored == archive.zones.end() ? 0 : read(stored->second.placeCounters);
}
nativesaves::WorldState Store::snapshot() const { auto guard = lock(); return archive; }
} // namespace sourcestates
