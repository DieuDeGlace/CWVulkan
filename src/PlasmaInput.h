#pragma once
// Pointer routing and keyboard/focus delivery from the independent GUI contracts.
// steam-gui-keyboard-edit-and-creation-dispatch-20261109.md. Dynamic Widget hooks
// and actual scene picking remain supplied by the owner scene/controller.
#include "PlasmaScene.h"
#include <functional>
#include <map>
#include <set>
#include <utility>

namespace plasma {
class Input : private Scene::DeletionObserver {
  public:
    enum class Event {
        LeftDown = 2, LeftUp = 3, RightDown = 5, RightUp = 6, MiddleDown = 8, MiddleUp = 9,
        Move = 12, Enter = 13, Leave = 14, FocusGained = 18, FocusLost = 19, TextChanged = 20
    };
    enum class Hook {
        LeftDown, LeftUp, RightDown, RightUp, MiddleDown, MiddleUp, Move, Enter, Leave,
        GlobalQualifyingDown, GlobalDown, GlobalUp, GlobalRightDown, GlobalRightUp, GlobalMiddleDown, GlobalMiddleUp, GlobalMove
    };
    enum class Key { Down, Up, Character };
    using Keyboard = std::function<void(Input &, SceneNode &, Key, uint16_t)>;
    using Pick = std::function<SceneNode *(glm::vec2)>;
    using Callback = std::function<void(Input &, SceneNode &)>;
    using Virtual = std::function<void(Input &, SceneNode &, Hook)>;

    Input(SceneNode &root, Pick pick) : root(root), pick(std::move(pick)) {}
    Input(Scene &, Pick); // One input owner, bound to ordinary scene deletion/replacement.
    ~Input() override;
    Input(const Input &) = delete;
    Input &operator=(const Input &) = delete;
    Virtual virtualHook;
    Keyboard keyboardHook;
    // Each delivered message reaches the enabled selected widget once. Ancestor
    // visibility, pointer capture and duplicate held membership do not consume it.
    void keyDown(uint16_t);
    void keyUp(uint16_t);
    void character(uint16_t);
    bool keyHeld(uint16_t key) const { return keys.contains(key); }
    size_t heldKeyCount() const { return keys.size(); }
    uint32_t lastKey() const { return lastKeyboardKey; }
    void registerEvent(SceneNode &owner, Event, Callback, bool enabledRequired = true);
    void dispatchRegistered(SceneNode *, Event);
    void dispatchWidgetEvent(SceneNode &, Event); // direct Widget event; no frame/enabled gate
    void move(glm::vec2);
    void setButtonHeld(int button, bool down); // Mask publication only; widget callbacks are separate.
    void leftDown();
    void leftUp();
    void rightDown();
    void rightUp();
    void middleDown();
    void middleUp();
    SceneNode *target() const; // Capture or fresh root pick, never the retained movement hit.
    void capture(SceneNode *node) { captured = node; }
    void clearCapture(); // Includes a full unchanged-pointer move, even when capture was null.
    void focus(SceneNode *owner); // ordered synchronous lost/store/gained; no page gate
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
    Scene *scene = nullptr;
    void beforeNodeDeletion(SceneNode &) override;
    void beforeWidgetDeletion(SceneNode &) override;
    SceneNode &root;
    Pick pick;
    std::map<SceneNode *, std::map<Event, Registration>> events;
    glm::vec2 current{0}, previous{0};
    SceneNode *hovered = nullptr, *captured = nullptr, *focused = nullptr;
    SceneNode *shownAuxiliary = nullptr, *qualifyingDown = nullptr;
    uint32_t held = 0;
    uint32_t lastKeyboardKey = 0;
    std::set<uint16_t> keys;
    void dispatchKey(Key, uint16_t);
    static SceneNode *owner(SceneNode *);
    static bool contains(const SceneNode *, const SceneNode *);
    void invoke(SceneNode *owner, Hook);
    void visit(SceneNode &, Hook);
    void closeShownAuxiliary();
    void clearOtherFocus(SceneNode *selected);
    void secondaryButton(int button, bool down, Hook targeted, Event event, Hook global);
};
// Only the ordinary Button dynamic class. Derived kinds retain their own hooks;
// Move returns false so the controller can invoke the common Widget geometry hook.
using ButtonSequenceStart = std::function<void(SceneNode &, const std::string &)>;
bool buttonInput(Input &, SceneNode &, Input::Hook, const ButtonSequenceStart & = {});
} // namespace plasma
