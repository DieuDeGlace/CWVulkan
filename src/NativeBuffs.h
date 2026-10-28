#pragma once
// A creature's buffs as the special skills and the interaction handler give them: the
// buff entry (0x18 bytes: type +0, strength +4, duration in ms +8, source creature id
// +0x10), giving one to a creature (0x65390), and the buff-change notice the world keeps
// of it (world+0x150: source id +0, target id +8, the buff +0x10, a flag byte +0x28).
#include "NativeAI.h"
#include "NativeRules.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

namespace nativebuffs {

using Buff = std::array<uint8_t, 0x18>;
using Notice = std::array<uint8_t, 0x30>;

inline Buff make(uint8_t type, float strength, float duration, int64_t source) {
    Buff b{};
    b[0] = type;
    std::memcpy(&b[4], &strength, 4);
    std::memcpy(&b[8], &duration, 4);
    std::memcpy(&b[0x10], &source, 8);
    return b;
}

inline float strength(const Buff &b) {
    float v;
    std::memcpy(&v, &b[4], 4);
    return v;
}
inline float duration(const Buff &b) {
    float v;
    std::memcpy(&v, &b[8], 4);
    return v;
}
inline int64_t source(const Buff &b) {
    int64_t v;
    std::memcpy(&v, &b[0x10], 8);
    return v;
}

// The creature's first buff of `type`, searching front to back; null if it has none.
inline const Buff *find(const nativeai::Creature &c, uint8_t type) {
    for (const Buff &b : c.buffs)
        if (b[0] == type)
            return &b;
    return nullptr;
}

// Buff types that always replace their own earlier entry from the same source: 1, 2, 3,
// 6, 7, 9, 10 and 11 (the mask 0xece over the types below 12).
inline bool replacesOwn(uint8_t type) { return type < 12 && ((0xeceu >> type) & 1); }

// 0x65390: with `refresh`, or for a type that replaces its own, the first entry of the same
// type and source is overwritten; otherwise (or if there is none) the buff is appended.
inline void give(nativeai::Creature &c, const Buff &b, bool refresh) {
    if (refresh || replacesOwn(b[0]))
        for (Buff &e : c.buffs)
            if (e[0] == b[0] && source(e) == source(b)) {
                e = b;
                return;
            }
    c.buffs.push_back(b);
}

inline Notice notice(int64_t from, int64_t to, const Buff &b, uint8_t flag) {
    Notice n{};
    std::memcpy(&n[0], &from, 8);
    std::memcpy(&n[8], &to, 8);
    std::memcpy(&n[0x10], b.data(), b.size());
    n[0x28] = flag;
    return n;
}

// The buffs as the stat rules (NativeRules) read them: type and strength.
inline std::vector<nativerules::Buff> forStats(const nativeai::Creature &c) {
    std::vector<nativerules::Buff> out;
    out.reserve(c.buffs.size());
    for (const Buff &b : c.buffs)
        out.push_back({b[0], strength(b)});
    return out;
}

} // namespace nativebuffs
