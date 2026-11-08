#pragma once
// Independent source-map semantics: steam-source-map-traversal-20261107.md.
// Storage belongs to the reader. Native buckets preserve installation history;
// reads never promote entries and rehashing relinks existing payloads.
#include <cstdint>
#include <iterator>
#include <limits>
#include <list>
#include <stdexcept>
#include <utility>
#include <vector>

template <class Value> class NativeSourceMap {
  public:
    using Key = std::pair<int, int>;
    using Entries = std::list<std::pair<Key, Value>>;
    using Iterator = typename Entries::iterator;
    explicit NativeSourceMap(Entries &storage) : entries(storage) {
        static_assert(sizeof(int) == 4);
        Entries previous;
        previous.splice(previous.begin(), entries);
        resetBuckets(8);
        // Compatibility startup data has LRU order. Replay its oldest entries
        // first; once selected, all native insertions retain their actual order.
        for (auto it = previous.rbegin(); it != previous.rend(); ++it)
            insert(it->first, std::move(it->second));
    }
    NativeSourceMap(const NativeSourceMap &) = delete;
    NativeSourceMap &operator=(const NativeSourceMap &) = delete;
    static constexpr uint64_t hash(Key key) {
        uint64_t value = 0xcbf29ce484222325ULL;
        for (uint32_t component : {uint32_t(key.first), uint32_t(key.second)})
            for (int shift = 0; shift < 32; shift += 8) {
                value ^= uint8_t(component >> shift);
                value *= 0x100000001b3ULL;
            }
        return value;
    }
    size_t bucketCount() const { return buckets.size(); }
    Iterator find(Key key) const {
        const auto &bucket = buckets[index(key)];
        if (bucket.first == entries.end()) return entries.end();
        for (auto it = bucket.first;; ++it) {
            if (it->first == key) return it;
            if (it == bucket.last) break;
        }
        return entries.end();
    }
    // The reader holds its cache guard across this complete nine-value capture.
    void neighborhood(Key center, Value (&out)[3][3]) const {
        for (int y = -1; y <= 1; ++y)
            for (int x = -1; x <= 1; ++x) {
                const auto found = find({center.first + x, center.second + y});
                out[y + 1][x + 1] = found == entries.end() ? Value{} : found->second;
            }
    }
    std::pair<Iterator, bool> insert(Key key, Value value) {
        if (auto found = find(key); found != entries.end()) return {found, false};
        entries.emplace_front(key, std::move(value));
        const auto made = entries.begin();
        admit(made);
        if (float(entries.size()) / float(buckets.size()) > 1.0f) {
            const size_t factor = buckets.size() < 512 ? 8 : 2;
            if (buckets.size() > std::numeric_limits<size_t>::max() / factor)
                throw std::length_error("source-map bucket count");
            rehash(buckets.size() * factor);
        }
        return {made, true};
    }
    Iterator erase(Iterator entry) {
        auto &bucket = buckets[index(entry->first)];
        if (bucket.first == bucket.last) bucket = {entries.end(), entries.end()};
        else if (bucket.first == entry) bucket.first = std::next(entry);
        else if (bucket.last == entry) bucket.last = std::prev(entry);
        return entries.erase(entry);
    }
  private:
    struct Bucket { Iterator first, last; };
    Entries &entries;
    std::vector<Bucket> buckets;
    size_t index(Key key) const { return size_t(hash(key) & (buckets.size() - 1)); }
    void resetBuckets(size_t count) { buckets.assign(count, Bucket{entries.end(), entries.end()}); }
    void admit(Iterator entry) {
        auto &bucket = buckets[index(entry->first)];
        if (bucket.first == entries.end()) {
            entries.splice(entries.end(), entries, entry);
            bucket = {entry, entry};
        } else {
            entries.splice(bucket.first, entries, entry);
            bucket.first = entry;
        }
    }
    void rehash(size_t count) {
        std::vector<Iterator> previous;
        previous.reserve(entries.size());
        for (auto it = entries.begin(); it != entries.end(); ++it) previous.push_back(it);
        resetBuckets(count);
        for (auto it : previous) admit(it);
    }
};
