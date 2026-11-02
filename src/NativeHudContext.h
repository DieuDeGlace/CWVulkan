#pragma once
// Derived nearby-item HUD state; steam-hud-context-item-lifecycle.md §§1–5.
#include "NativeSaves.h"
#include <functional>
#include <optional>
#include <span>
#include <string>

namespace hudcontext {
using Tabs = std::vector<std::vector<nativesaves::Stack>>;
struct Zone {
    std::span<const nativesaves::Record> ground, objects;
};
struct Environment {
    const uint8_t *player = nullptr;
    bool namedWorld = false;
    const Tabs *inventory = nullptr;
    std::function<std::optional<Zone>(nativesaves::Cell)> loadedZone;
};
struct State {
    nativesaves::Item item{};
    int32_t count = 0;
};
// A skipped slice preserves the previous result; an admitted zero step still rebuilds.
void rebuild(State &, const Environment &);
std::string countText(int32_t count);
struct Model {
    const nativesaves::Item *item = nullptr;
    int x = 0; // native integer GUI units, converted to float only at submission
};
std::vector<Model> modelRow(const State &, const Tabs &, nativesaves::Cell playerRegion);
} // namespace hudcontext
