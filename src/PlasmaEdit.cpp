#include "PlasmaEdit.h"
#include "PlasmaScene.h"
#include <algorithm>
#include <bit>
#include <climits>
#include <cmath>
#include <stdexcept>

namespace plasma {
namespace {
int32_t word(int64_t value) { return std::bit_cast<int32_t>(uint32_t(value)); }
int32_t length(const std::u16string &text) {
    if (text.size() > INT32_MAX)
        throw std::length_error("Edit caption exceeds the supported signed-index domain");
    return int32_t(text.size());
}
}
std::u16string captionText(const SceneNode &node) {
    if (const auto *shape = node.shapeObject(); shape && shape->kind == "TextShape") {
        if (node.text)
            return std::u16string(node.text->begin(), node.text->end());
        if (const auto *string = shape->property("TextShape.string")) {
            const auto frames = string->frames();
            if (!frames.empty()) {
                const auto text = plx::wideString(*frames[0]);
                return std::u16string(text.begin(), text.end());
            }
        }
        return {};
    }
    for (const auto &child : node.children)
        if (auto text = captionText(*child); !text.empty())
            return text;
    return {};
}
void setCaptionText(SceneNode &node, std::u16string_view text) {
    node.setAllText(std::wstring(text.begin(), text.end()));
}

void Edit::clamp() {
    if (!attached()) {
        caret = selection = 0;
        return;
    }
    const int32_t size = length(caption.read());
    caret = std::clamp(caret, 0, size);
    if (word(int64_t(caret) + selection) < 0)
        selection = word(-int64_t(caret));
    if (word(int64_t(caret) + selection) > size)
        caret = word(int64_t(size) - selection);
}
void Edit::erase() {
    clamp();
    auto text = caption.read();
    const int32_t start = selection < 0 ? word(int64_t(caret) + selection) : caret;
    // Native invalid indices reach a string failure provider. Keep those outside
    // the supported short-caption domain without recreating an unsafe access.
    if (caret < 0 || caret > length(text) || start < 0 || start > length(text))
        return;
    const uint64_t count = selection == 0 ? 1 : selection < 0 ? uint64_t(-int64_t(selection)) : uint64_t(selection);
    text.erase(size_t(start), size_t(count));
    caption.write(text);
    if (selection < 0)
        caret = word(int64_t(caret) + selection);
    selection = 0;
    refresh();
    changed();
    layout();
}
void Edit::character(uint16_t unit) {
    if (!attached() || unit < 0x20)
        return;
    activate();
    const std::u16string incoming(1, char16_t(unit));
    if (selection && caption.validate && !caption.validate(incoming))
        return;
    if (selection)
        erase();
    auto candidate = caption.read();
    clamp();
    if (caret < 0 || size_t(caret) > candidate.size())
        return;
    candidate.insert(size_t(caret), incoming);
    if (caption.validate && !caption.validate(candidate))
        return;
    caption.write(candidate);
    caret = word(int64_t(caret) + 1);
    layout();
    changed();
}
void Edit::keyDown(uint16_t key, bool shiftHeld) {
    if (!attached())
        return;
    clamp();
    activate();
    switch (key) {
    case 0x0d:
        if (caption.clearFocus) caption.clearFocus();
        layout();
        break;
    case 0x25:
        if (caret > 0) {
            --caret;
            selection = shiftHeld ? word(int64_t(selection) + 1) : 0;
        }
        layout();
        break;
    case 0x27:
        if (caret < length(caption.read())) {
            ++caret;
            selection = shiftHeld ? word(int64_t(selection) - 1) : 0;
        }
        layout();
        break;
    case 0x2e:
        erase();
        layout();
        break;
    case 0x08:
        if (selection) {
            erase();
            layout();
        } else if (caret >= 1) {
            auto text = caption.read();
            if (size_t(caret) > text.size()) return;
            text.erase(size_t(caret - 1), 1);
            caption.write(text);
            --caret;
            refresh();
            layout();
            changed();
        }
        break;
    case 0x24:
        selection = shiftHeld ? word(int64_t(selection) + caret) : 0;
        caret = 0;
        layout();
        break;
    case 0x23: {
        const int32_t size = length(caption.read());
        selection = shiftHeld ? word(int64_t(selection) + caret - size) : 0;
        caret = size;
        layout();
        break;
    }
    default:
        break;
    }
}

void Edit::pick(float point, const Pointer &pointer) {
    const auto snapshot = caption.read();
    caret = 0;
    const int32_t count = length(snapshot);
    const auto query = pointer.beginPick();
    for (int32_t index = 0; index < count; ++index) {
        const auto metric = query(snapshot, index);
        const float threshold = metric.position[0] + metric.size[0] * 0.7f;
        if (point > threshold)
            caret = index + 1;
    }
    clamp();
    activate();
}
void Edit::leftDown(const Pointer &pointer) {
    if (!attached()) return;
    dragging = false;
    const float point = pointer.localX();
    if (!pointer.selected()) {
        caret = length(caption.read());
        selection = word(-int64_t(length(caption.read())));
        pointer.select();
    } else {
        if (pointer.shiftHeld()) {
            const int32_t previous = caret;
            pick(point, pointer);
            selection = word(int64_t(selection) - word(int64_t(caret) + previous));
        } else {
            pick(point, pointer);
            selection = 0;
        }
        dragging = true;
    }
    clamp();
    layout();
    pointer.capture();
}
void Edit::move(const Pointer &pointer) {
    if (!pointer.leftHeld() || !pointer.selected() || !dragging || !attached()) return;
    const int32_t previous = caret;
    const float point = pointer.localX();
    pick(point, pointer);
    selection = word(int64_t(selection) + word(int64_t(previous) - caret));
    clamp();
    layout();
}
void Edit::leftUp(const Pointer &pointer) { pointer.release(); }

namespace {
Edit::Point layoutPoint(const Edit::Matrix &matrix, Edit::Point point) {
    const float denominator = (point[1] * matrix[7] + point[0] * matrix[3]) + matrix[15];
    const float reciprocal = 1.0f / denominator;
    return {((point[1] * matrix[4] + point[0] * matrix[0]) + matrix[12]) * reciprocal,
            ((point[1] * matrix[5] + point[0] * matrix[1]) + matrix[13]) * reciprocal};
}
Edit::Point layoutSize(const Edit::Matrix &matrix, Edit::Point size) {
    return {size[1] * matrix[4] + size[0] * matrix[0], size[1] * matrix[5] + size[0] * matrix[1]};
}
}
void Edit::adjustLayout(const Layout &geometry) {
    if (!attached() || !geometry.hasParent()) return;
    const auto lower = geometry.lower();
    const auto upper = geometry.upper();
    const auto node = geometry.captureNode();
    const auto forward = node.forward();
    const auto first = geometry.measure(caption.read(), caret, forward);
    const auto retained = geometry.localMatrix();
    const auto position = layoutPoint(retained, first.position);
    const auto size = layoutSize(retained, first.size);
    const bool upperRoute = position[0] + size[0] >= upper[0];
    // The second native unordered comparison exits, whereas an unordered first
    // comparison proceeds to this lower-side test.
    if (!upperRoute && !(lower[0] > position[0])) return;
    const auto probeMatrix = upperRoute ? node.forward() : node.chain();
    auto text = caption.read();
    text.push_back(u' ');
    const int32_t probeIndex = word(int64_t(caret) + (upperRoute ? 6 : -6));
    const auto second = geometry.measure(text, probeIndex, probeMatrix);
    const auto probePosition = layoutPoint(retained, second.position);
    const float adjustment = probePosition[0] - (upperRoute ? upper[0] : lower[0]);
    // Native out-of-range conversion/provider failures are outside the valid
    // finite caption domain. Do not turn them into an unsafe C++ conversion.
    if (!std::isfinite(adjustment) || double(adjustment) < INT32_MIN || double(adjustment) > INT32_MAX) return;
    geometry.setX(geometry.x() - float(int32_t(adjustment)));
    if (word(int64_t(caret) - 6) <= 0 || geometry.x() > geometry.savedX())
        geometry.setX(geometry.savedX());
    geometry.updateTransform();
}
}
