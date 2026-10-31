#pragma once
// Pointer routing from steam-gui-pointer-capture-routing. Dynamic Widget hooks
// and actual scene picking remain supplied by the owner scene/controller.
#include "PlasmaScene.h"
#include <functional>
#include <map>
#include <utility>

namespace plasma {
class Input {
  public:
    enum class Event { LeftDown = 2, LeftUp = 3, Move = 12, Enter = 13, Leave = 14, FocusLost = 19 };
    enum class Hook { LeftDown, LeftUp, Move, Enter, Leave, GlobalQualifyingDown, GlobalDown, GlobalUp, GlobalMove };
    using Pick = std::function<SceneNode *(glm::vec2)>;
    using Callback = std::function<void(Input &, SceneNode &)>;
    using Virtual = std::function<void(Input &, SceneNode &, Hook)>;

    Input(SceneNode &root, Pick pick) : root(root), pick(std::move(pick)) {}
    Input(const Input &) = delete;
    Input &operator=(const Input &) = delete;
    Virtual virtualHook;
    void registerEvent(SceneNode &owner, Event, Callback, bool enabledRequired = true);
    void dispatchRegistered(SceneNode *, Event);
    void move(glm::vec2);
    void leftDown();
    void leftUp();
    SceneNode *target() const; // Capture or fresh root pick, never the retained movement hit.
    void capture(SceneNode *node) { captured = node; }
    void clearCapture(); // Includes a full unchanged-pointer move, even when capture was null.
    void focus(SceneNode *owner) { focused = owner; }
    void auxiliary(SceneNode *owner) { shownAuxiliary = owner; }

    glm::vec2 pointer() const { return current; }
    glm::vec2 previousPointer() const { return previous; }
    uint32_t heldButtons() const { return held; }
    SceneNode *movementHit() const { return hovered; }
    SceneNode *captureNode() const { return captured; }
    SceneNode *focusWidget() const { return focused; }
    SceneNode *auxiliaryWidget() const { return shownAuxiliary; }
    SceneNode *downOwner() const { return qualifyingDown; }

  private:
    struct Registration {
        Callback callback;
        bool enabledRequired = true;
    };
    SceneNode &root;
    Pick pick;
    std::map<SceneNode *, std::map<Event, Registration>> events;
    glm::vec2 current{0}, previous{0};
    SceneNode *hovered = nullptr, *captured = nullptr, *focused = nullptr;
    SceneNode *shownAuxiliary = nullptr, *qualifyingDown = nullptr;
    uint32_t held = 0;
    static SceneNode *owner(SceneNode *);
    static bool contains(const SceneNode *, const SceneNode *);
    void invoke(SceneNode *owner, Hook);
    void visit(SceneNode &, Hook);
    void closeShownAuxiliary();
};
// Only the ordinary Button dynamic class. Derived kinds retain their own hooks;
// Move returns false so the controller can invoke the common Widget geometry hook.
using ButtonSequenceStart = std::function<void(SceneNode &, const std::string &)>;
bool buttonInput(Input &, SceneNode &, Input::Hook, const ButtonSequenceStart & = {});
} // namespace plasma
