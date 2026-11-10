#pragma once
// UTF-16 caption editing from the complete independent GUI keyboard contract.
// Scene attachment, font layout and pointer picking are separate owner adapters.
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <utility>

namespace plasma {
struct SceneNode;
// A present own text caption wins even when empty; otherwise the first nonempty
// child result wins. Reading does not inherit setter visibility/disabled gates.
std::u16string captionText(const SceneNode &);
void setCaptionText(SceneNode &, std::u16string_view);

class Edit {
  public:
    struct Caption {
        std::function<std::u16string()> read;
        std::function<void(const std::u16string &)> write;
        std::function<void()> layout, changed, clearFocus;
        std::function<void(bool)> refresh;
        std::function<bool(std::u16string_view)> validate;
        std::function<bool()> available; // current borrowed caption cache; absent means bound
        std::function<void()> activity; // owner mirrors the activity-byte write immediately
    };
    using Point = std::array<float, 2>;
    using Matrix = std::array<float, 16>; // native flat words; translation at12/13
    struct Metric { Point position{}, size{}; };
    using Query = std::function<Metric(std::u16string_view, int32_t)>;
    struct Pointer {
        std::function<bool()> selected, leftHeld, shiftHeld;
        std::function<float()> localX;
        std::function<void()> select, capture, release;
        // Capture the node matrix once; each returned query still obtains the
        // current caption/font/style operands from its owner.
        std::function<Query()> beginPick;
    };
    struct Layout {
        struct Node {
            std::function<Matrix()> forward, chain;
        };
        std::function<bool()> hasParent;
        std::function<Point()> lower, upper;
        std::function<Node()> captureNode;
        std::function<Matrix()> localMatrix; // then-current parent-local * own-local
        std::function<Metric(std::u16string_view, int32_t, const Matrix &)> measure;
        std::function<float()> x, savedX;
        std::function<void(float)> setX;
        std::function<void()> updateTransform;
    };
    explicit Edit(Caption caption) : caption(std::move(caption)) {}
    Edit(const Edit &) = delete;
    Edit &operator=(const Edit &) = delete;
    int32_t caret = 0, selection = 0; // endpoint is caret + signed displacement
    bool dirty = false, dragging = false;
    void clamp(); // ordered native pass, not a repeated repair of corrupt state
    void character(uint16_t);
    void keyDown(uint16_t, bool shiftHeld);
    void leftDown(const Pointer &);
    void move(const Pointer &);
    void leftUp(const Pointer &); // logical release only; dragging is retained
    void adjustLayout(const Layout &); // provider outputs stay explicit inputs


  private:
    Caption caption; // owner keeps callbacks and their caption alive during a call
    bool attached() const { return caption.read && caption.write && (!caption.available || caption.available()); }
    void activate() { dirty = true; if (caption.activity) caption.activity(); }
    void erase();
    void pick(float, const Pointer &);
    void layout() { if (caption.layout) caption.layout(); }
    void changed() { if (caption.changed) caption.changed(); }
    void refresh() { if (caption.refresh) caption.refresh(true); }
};
}
