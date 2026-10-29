#pragma once
// The equipped-pet pass, from remaining-runtime-gaps.md §7.
#include "NativeBuffs.h"
#include <functional>

namespace nativepet {
struct World {
    bool client = false, active = true;
    nativeai::Creature *local = nullptr;
    std::vector<nativeai::Creature *> creatures;
    std::function<nativeai::Creature *(int64_t)> construct;
    std::function<void(nativeai::Creature *)> added;
    std::function<void(int64_t owner, nativeai::Creature *)> setup;
    std::function<float(const nativeai::Creature &)> maximumHealth;
    std::vector<nativebuffs::Notice> buffNotes;
};
void resetTransient(nativeai::Creature &creature);
void step(World &world);
} // namespace nativepet
