#pragma once
// One scene/input root with the independently specified frontend/gameplay branches.
// docs/specs/steam-interface-input-root-lifetime.md. Ownership remains with Scene.
#include "PlasmaScene.h"
#include <algorithm>
#include <array>

namespace uiroot {
enum class Branch { Title, Characters, Creation, Worlds, NewWorld, Gameplay, Options, Controls, Help, Root, Unspecified };
struct State {
    std::array<bool, 5> frontend{};
    bool options = false, controls = false, help = false;
};
// Complete native Back leaf: prioritized page stores, independent of overlays.
inline bool back(State &state) {
    constexpr std::array<std::array<size_t,2>,4> transitions{{{4,3},{3,1},{2,1},{1,0}}};
    for (const auto transition : transitions)
        if (state.frontend[transition[0]]) {
            state.frontend[transition[0]]=false;
            state.frontend[transition[1]]=true;
            return true;
        }
    return false;
}
class Branches {
  public:
    explicit Branches(plasma::Scene &scene) : scene(scene) {
        static const char *const names[] = {"title",     "characters", "creation", "worlds",
                                            "new-world", "ingame",     "options",  "controls", "help"};
        for (size_t index = 0; index < roots.size(); ++index) {
            auto *parent = index == size_t(Branch::Help) ? roots[size_t(Branch::Gameplay)] : &scene.root();
            roots[index] = scene.create(parent, names[index]);
        }
        staging = scene.create(&scene.root(), "interface-probes");
        staging->visible = false; // Engine layout probes are owned but not input targets.
        apply({}); // Direct-draw/legacy callers default to gameplay; frontend publishes its state explicitly.
    }
    plasma::SceneNode *node(Branch branch) const {
        const auto index = (size_t)branch;
        return index < roots.size() ? roots[index] : branch == Branch::Root ? &scene.root() : staging;
    }
    void apply(const State &state) {
        for (size_t index = 0; index < state.frontend.size(); ++index)
            roots[index]->visible = state.frontend[index];
        roots[(size_t)Branch::Gameplay]->visible =
            std::none_of(state.frontend.begin(), state.frontend.end(), [](bool active) { return active; });
        roots[(size_t)Branch::Options]->visible = state.options;
        roots[(size_t)Branch::Controls]->visible = state.controls;
        roots[(size_t)Branch::Help]->visible = state.help;
    }

  private:
    plasma::Scene &scene;
    std::array<plasma::SceneNode *, 9> roots{};
    plasma::SceneNode *staging = nullptr;
};
} // namespace uiroot
