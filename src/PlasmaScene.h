#pragma once
// The interface's scene graph as Plasma keeps it (docs/specs/plasma-format-and-scene.md, part 2):
// nodes with a transformation, a display, a shape and a widget; their matrices; the animation of
// attributes along their sequences; and the widgets' geometry (frame, bind and inner rectangles,
// moving and resizing with the propagation to child widgets, and the nine-slice stretch of a
// resized widget's shapes).
//
// A scene node made from a document node is an instance of it: it shares the document's shape and
// keeps its own copy of the transformation, display and widget values, so the interface code can
// move, resize, colour and hide every instance on its own (the game gives instances their own
// objects when it does that).
#include "Plx.h"
#include "PlasmaShape.h"

#include <glm/glm.hpp>

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace plasma {

// ---------------------------------------------------------------------------------------------
// Transformations and displays (spec 1.7.2, 1.7.3, 2.2).

struct Transformation {
    glm::vec2 translation{0}, pivot{0};
    glm::vec3 rotation{0}; // degrees about x, y and z
    glm::mat4 deformation{1};
    // The local matrix (spec 2.2.1): deformed and rotated about the pivot, then moved by the
    // translation; z flattened.
    glm::mat4 local() const;
};
// A Transformation object's live values.
Transformation transformationOf(const plx::Object &t);
// Its local matrix, with `translation` or `deformation` standing for its own when given.
glm::mat4 transformation(const plx::Object &t, const glm::vec2 *translation = nullptr, const glm::mat4 *deformation = nullptr);
glm::vec2 pivot(const plx::Object &t);
glm::vec2 translation(const plx::Object &t);

// Display.flags (spec 2.3.3).
enum : int { DISPLAY_BLUR = 0x1, DISPLAY_MASK_MULTIPLY = 0x2, DISPLAY_MASK_ALPHA = 0x4 };

struct Display {
    bool visible = true, clips = false;
    glm::vec4 fill{1}, stroke{1};
    float blurRadius = 0;
    int flags = 0;
    // How a clipping node's shape takes in its children's layer (spec 2.3.3): 0 when it does not
    // clip, else 1 to 3.
    int maskMode() const;
};
Display displayOf(const plx::Object &d);
glm::vec4 displayColor(const plx::Object &d);
bool displayVisible(const plx::Object &d);
bool displayClips(const plx::Object &d);

// ---------------------------------------------------------------------------------------------
// Animation (spec 2.6).

// The curve between two keys (spec 2.6.2): a cubic Bezier from a to b whose handles a quarter of
// the way along are pulled towards the neighbouring keys by the keys' smoothness.
float bezier(float previous, float a, float b, float next, float smoothA, float smoothB, float u);

// An attribute playing one of its sequences (spec 2.6.3 to 2.6.5). Its frame slots are kept as
// their bytes; continuous values (floats, vectors, matrices) are interpolated float by float,
// discrete ones (ints, strings) step.
struct AttributeAnimation {
    std::vector<std::vector<uint8_t>> slots; // slot 0 is the live value
    bool discrete = false;
    const plx::Chunk *source = nullptr;      // the attribute, with its sequences
    std::vector<plx::Key> active;            // the playing sequence's keys
    int period = 0, clock = 0, lastJump = -1, segment = 0;

    explicit AttributeAnimation(const plx::Chunk &attribute, bool discrete = false);
    // Starts the named sequence (false if the attribute has none with keys); `period` > 0 loops.
    bool play(const std::string &name, int period = 0);
    // The live value at time t of the playing sequence; true if it changed.
    bool evaluate(int t);
    // Moves the clock on; when nothing changes any more the animation has finished (no keys).
    bool advance(int ms);
    bool playing() const { return !active.empty(); }
};

// The live value of an attribute `ms` into its sequence of that name, played from its stored
// live value (as floats). Before the play (ms < 0) the live value itself. False when the
// attribute has no such sequence.
bool sampleSequence(const plx::Chunk &attribute, const std::string &name, float ms, std::vector<float> &out, bool discrete = false);

// ---------------------------------------------------------------------------------------------
// Widgets (spec 1.7.4, 2.7).

enum : int { ALIGN_LEFT = 0, ALIGN_RIGHT = 1, ALIGN_CENTER = 2, ALIGN_TOP = 0, ALIGN_BOTTOM = 1, ALIGN_MIDDLE = 2 };
// Widget.flags (spec 2.7.6).
enum : int {
    WIDGET_STRETCH_X = 0x4,   // stretches with its parent horizontally (and is not right-aligned)
    WIDGET_STRETCH_Y = 0x8,   // stretches with its parent vertically (and is not bottom-aligned)
    WIDGET_OWN_SURFACE = 0x40,
    WIDGET_WHOLE_HIT = 0x100,
    WIDGET_DRAW_FIRST = 0x200
};

struct Widget {
    std::string kind; // "Widget", "Button", ...
    glm::vec2 innerBindPos{0}, innerBindSize{1}, bindPos{0}, bindSize{1}, framePos{0}, frameSize{1};
    glm::vec2 minimumSize{0}; // bindSize - innerBindSize once the widget is set up
    glm::mat4 bind{1}, inverseBind{1}; // node space to widget space and back
    int horizontalAlignment = ALIGN_LEFT, verticalAlignment = ALIGN_TOP, flags = 0;
    int buttonType = 0, direction = 0;
    std::wstring caption;
    bool dirty = false;

    // The frame's size as drawn: never below the design size around the inner rectangle.
    glm::vec2 effectiveSize() const;
    // The inner (stretchable) rectangle as the frame moves and resizes it.
    glm::vec2 innerPosition() const;
    glm::vec2 innerSize() const;
};
Widget widgetOf(const plx::Object &w);
// New widget baseline from an existing live frame, without its behavior flags/alignment.
Widget reboundWidget(const Widget &);

// ---------------------------------------------------------------------------------------------
// The tree.

struct NodeAnimation {
    enum class Target { Shape, Transform, Display } target;
    size_t property;
    AttributeAnimation value;
};

struct SceneNode {
    std::string name;
    const plx::Document *doc = nullptr; // the file it comes from (gui.plx, icons.plx)
    int source = -1;                    // the document node it is an instance of
    int shape = -1;                     // the document's shape (shared)
    uint32_t flags = 0;                 // node flags (plx::NODE_...)
    Transformation transform;
    std::optional<glm::mat4> local;     // replaces the transformation's matrix (an icon fitted to its place)
    bool visible = true, clips = false; // the display's values
    glm::vec4 fill{1};
    int displayFlags = 0;
    float blurRadius = 0;
    std::optional<Widget> widget;
    // A text shape's live string and colour when the interface set them (setText, setTextColour).
    std::optional<std::wstring> text;
    std::optional<glm::vec4> textColor;
    SceneNode *parent = nullptr;
    std::vector<std::unique_ptr<SceneNode>> children; // drawn in order, later ones on top
    // Drawn after the node's shape (a widget's own drawing).
    std::function<void(SceneNode &)> onDraw;
    // Unanimated shapes share the owner document; an animated instance owns its live properties.
    std::optional<plx::Object> animatedShape;
    mutable std::optional<ShapeDrawing> animatedDrawing;
    std::vector<NodeAnimation> animations;
    const plx::Object *shapeObject() const;
    bool playSequence(const std::string &, int period = 0);
    int sequenceDuration(const std::string &) const;
    void setSequenceTime(int milliseconds);
    void advanceSequences(int milliseconds);
    void stopSequences();
    void poseSequence(const std::string &); // play, seek to the subtree duration, stop
    void initializeWidgets();

    // The first node of a name, this node first and then each child's subtree in order.
    SceneNode *find(const std::string &n);
    void findAll(const std::string &n, std::vector<SceneNode *> &out);
    // The text (or text colour) of every text shape under each node of a name (spec 2.4, the
    // subtree form); a matched node's subtree is not searched for further matches.
    void setText(const std::string &n, const std::wstring &t);
    void setTextColour(const std::string &n, glm::vec4 c);
    // The text (or text colour) of every text shape in this subtree.
    void setAllText(const std::wstring &t);
    void setAllTextColor(glm::vec4 c);
    int maskMode() const;
};

class Scene {
  public:
    explicit Scene(const plx::Document &document);
    const plx::Document &document() const { return doc; }
    // The engine's root: every node the interface makes hangs below it. Its matrix is the
    // interface's scale.
    SceneNode &root() { return rootNode; }
    void setScale(float s) { rootScale = s; }
    float scale() const { return rootScale; }

    // An instance of a document node and its children (those not disabled) under `parent`; by
    // name the first node of that name. Nullptr if there is none.
    SceneNode *instance(const std::string &name, SceneNode *parent);
    SceneNode *instance(int index, SceneNode *parent);
    // From another document (icons.plx).
    SceneNode *instance(const plx::Document &from, const std::string &name, SceneNode *parent);
    SceneNode *instance(const plx::Document &from, int index, SceneNode *parent);
    // A copy of a scene node and its children under `parent`.
    SceneNode *instance(const SceneNode &from, SceneNode *parent);
    // A new empty node.
    SceneNode *create(SceneNode *parent, const std::string &name = "");
    // The interface's icon setter: an icons.plx node, named "icon", under the node of `targetName`
    // in `node`, fitted to that node's shape (centred, scaled to its bounds); a previous icon there
    // is replaced.
    void setIcon(SceneNode &node, const std::string &targetName, const plx::Document &icons, const std::string &iconName);
    // The bounds of a node's shape (with its children's, `deep`) in the node's own space.
    bool shapeBounds(const SceneNode &n, glm::vec2 &minimum, glm::vec2 &maximum, bool deep) const;
    using LocalBounds = std::function<bool(const SceneNode &, glm::vec2 &, glm::vec2 &)>;
    bool screenBounds(const SceneNode &node, glm::vec2 &minimum, glm::vec2 &maximum,
                      const LocalBounds &localBounds = {}) const;

    // A node's local matrix, and its world matrix (through its parents and the root).
    glm::mat4 transformation(const SceneNode &n) const;
    glm::mat4 matrix(const SceneNode &n) const;
    // The interface's placing of a node: its translation becomes a window point (in its parent's
    // units) plus an offset, less its pivot.
    void setPosition(SceneNode &n, glm::vec2 point, glm::vec2 offset);

    // Widget geometry (spec 2.7.2; the node must have a widget).
    SceneNode *parentWidget(const SceneNode &n) const;     // the nearest ancestor with a widget
    glm::mat4 contentMatrix(const SceneNode &n) const;     // frame space to window pixels
    glm::vec2 contentPosition(const SceneNode &n) const;   // the frame's top left, window pixels
    glm::vec2 screenSize(const SceneNode &n) const;        // the frame's size, window pixels
    glm::vec2 contentSize(const SceneNode &n) const;       // the frame's size, widget units
    glm::mat4 placementMatrix(const SceneNode &n) const;   // frame space to the parent widget's frame
    glm::vec2 viewPosition(const SceneNode &n) const;      // the frame's top left in its parent widget's
    glm::vec2 viewSize(const SceneNode &n) const;          // the frame's size there
    void moveTo(SceneNode &n, glm::vec2 position);         // sets viewPosition (moving the node)
    void resizeTo(SceneNode &n, glm::vec2 size);           // sets viewSize (and places child widgets)
    // Called after a widget's size changes, by node.
    std::function<void(SceneNode &)> onResized;

    // The triangles of a node's shape in window pixels: its fill, then its stroke, through its
    // matrix, stretched by its owner widget when that is resized; coloured by the tint.
    struct Drawn {
        std::vector<glm::vec2> positions;
        std::vector<glm::vec4> colors;
    };
    void drawShape(const SceneNode &n, glm::vec4 tint, Drawn &out) const;
    // A point of a node's space in window pixels, stretched by its owner widget (spec 2.7.4).
    glm::vec2 deform(const SceneNode &n, glm::vec2 local) const;
    // Independent subtree picking, with native flags, clipping and inverse widget stretch.
    // Text containment uses the caller's font-backed cached bounds; ordinary widget callbacks are false.
    using TextHit = std::function<bool(const SceneNode &, glm::vec2 local)>;
    const SceneNode *hitTest(const SceneNode &node, glm::vec2 point, const TextHit &textHit = {},
                             uint32_t options = 0, bool clipWidgets = true) const;

  private:
    const plx::Document &doc;
    SceneNode rootNode;
    float rootScale = 1;
    mutable std::map<std::pair<const plx::Document *, int>, ShapeDrawing> drawings;
    const ShapeDrawing *drawing(const SceneNode &n) const;
    glm::mat4 frameToScreen(const SceneNode &n) const;
    void propagateResize(SceneNode &node, glm::vec2 offset, glm::vec2 delta);
    void place(SceneNode &n, glm::vec2 offset, glm::vec2 delta);
};

} // namespace plasma
