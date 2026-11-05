#pragma once
// One scene/input root with the independently specified frontend/gameplay branches.
// docs/specs/steam-interface-input-root-lifetime.md. Ownership remains with Scene.
#include "PlasmaScene.h"
#include <algorithm>
#include <array>

namespace uiroot {
enum class Branch { Title, Characters, Creation, Worlds, NewWorld, Gameplay, Options, Controls, Root, Unspecified };
struct State {
    std::array<bool, 5> frontend{};
    bool options = false, controls = false;
};
class Branches {
  public:
    explicit Branches(plasma::Scene &scene) : scene(scene) {
        static const char *const names[] = {"title",     "characters", "creation", "worlds",
                                            "new-world", "ingame",     "options",  "controls"};
        for (size_t index = 0; index < roots.size(); ++index)
            roots[index] = scene.create(&scene.root(), names[index]);
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
    }

  private:
    plasma::Scene &scene;
    std::array<plasma::SceneNode *, 8> roots{};
    plasma::SceneNode *staging = nullptr;
};
} // namespace uiroot
