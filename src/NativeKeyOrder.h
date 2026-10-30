#pragma once
// Key-only ordering shared by inventory regions and the Steam mesh budget.
// Independent contract: steam-controller-handoff.md 4; steam-scene-lighting.md 2.6.2.
#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

namespace keyorder {
namespace detail {
template <class Value, class Key> class Sorter {
  public:
    Sorter(std::vector<Value> &values, Key key) : values(values), key(std::move(key)) {}

    void sort(size_t begin, size_t end, size_t budget) {
        while (end - begin > 32) {
            if (!budget) { heap(begin, end); return; }
            const auto [equalBegin, equalEnd] = partition(begin, end);
            const size_t half = budget / 2;
            budget = half + half / 2;
            // Recurse on the shorter side; equal lengths visit the right first.
            if (equalBegin - begin < end - equalEnd) {
                sort(begin, equalBegin, budget);
                begin = equalEnd;
            } else {
                sort(equalEnd, end, budget);
                end = equalBegin;
            }
        }
        for (size_t position = begin + (begin < end); position < end; ++position) {
            auto saved = std::move(values[position]);
            size_t hole = position;
            while (hole > begin && key(saved) < key(values[hole - 1])) {
                values[hole] = std::move(values[hole - 1]);
                --hole;
            }
            values[hole] = std::move(saved);
        }
    }

    void heap(size_t begin, size_t end) {
        const size_t count = end - begin;
        for (size_t parent = count / 2; parent; --parent) {
            auto saved = std::move(values[begin + parent - 1]);
            repair(begin, count, parent - 1, std::move(saved));
        }
        for (size_t remaining = count; remaining > 1; --remaining) {
            auto saved = std::move(values[begin + remaining - 1]);
            values[begin + remaining - 1] = std::move(values[begin]);
            repair(begin, remaining - 1, 0, std::move(saved));
        }
    }

  private:
    std::vector<Value> &values;
    Key key;

    void triple(size_t first, size_t middle, size_t last) {
        if (key(values[middle]) < key(values[first])) std::swap(values[first], values[middle]);
        if (key(values[last]) < key(values[middle])) {
            std::swap(values[middle], values[last]);
            if (key(values[middle]) < key(values[first])) std::swap(values[first], values[middle]);
        }
    }

    std::pair<size_t, size_t> partition(size_t begin, size_t end) {
        const size_t count = end - begin, middle = begin + count / 2, last = end - 1;
        if (count < 42) triple(begin, middle, last);
        else {
            const size_t step = count / 8;
            triple(begin, begin + step, begin + 2 * step);
            triple(middle - step, middle, middle + step);
            triple(last - 2 * step, last - step, last);
            triple(begin + step, middle, last - step);
        }
        const auto pivot = key(values[middle]);
        auto equal = [&](size_t at) { return !(key(values[at]) < pivot) && !(pivot < key(values[at])); };
        size_t equalBegin = middle, equalEnd = middle + 1;
        while (equalBegin > begin && equal(equalBegin - 1)) --equalBegin;
        while (equalEnd < end && equal(equalEnd)) ++equalEnd;
        size_t left = equalBegin, right = equalEnd;
        for (;;) {
            while (right < end) {
                if (pivot < key(values[right])) ++right;
                else if (key(values[right]) < pivot) break;
                else { std::swap(values[equalEnd], values[right]); ++equalEnd; ++right; }
            }
            while (left > begin) {
                if (key(values[left - 1]) < pivot) --left;
                else if (pivot < key(values[left - 1])) break;
                else { --equalBegin; --left; std::swap(values[equalBegin], values[left]); }
            }
            if (left == begin && right == end) return {equalBegin, equalEnd};
            if (left == begin) {
                if (equalEnd != right) std::swap(values[equalBegin], values[equalEnd]);
                std::swap(values[equalBegin], values[right]);
                ++equalBegin; ++equalEnd; ++right;
            } else if (right == end) {
                --left; --equalBegin;
                if (left != equalBegin) std::swap(values[left], values[equalBegin]);
                --equalEnd;
                std::swap(values[equalBegin], values[equalEnd]);
            } else {
                --left;
                std::swap(values[left], values[right]);
                ++right;
            }
        }
    }

    void repair(size_t begin, size_t count, size_t top, Value saved) {
        size_t hole = top;
        while (hole * 2 + 2 < count) {
            size_t child = hole * 2 + 2;
            if (key(values[begin + child]) < key(values[begin + child - 1])) --child;
            values[begin + hole] = std::move(values[begin + child]);
            hole = child;
        }
        if (hole * 2 + 1 < count) {
            const size_t child = hole * 2 + 1;
            values[begin + hole] = std::move(values[begin + child]);
            hole = child;
        }
        while (hole > top) {
            const size_t parent = (hole - 1) / 2;
            if (!(key(values[begin + parent]) < key(saved))) break;
            values[begin + hole] = std::move(values[begin + parent]);
            hole = parent;
        }
        values[begin + hole] = std::move(saved);
    }
};
} // namespace detail

template <class Value, class Key> void sort(std::vector<Value> &values, Key key) {
    detail::Sorter<Value, Key>(values, std::move(key)).sort(0, values.size(), values.size());
}
} // namespace keyorder
