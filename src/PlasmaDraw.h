#pragma once
// Ordered ordinary clip layers, from steam-plasma-clip-layers.md.
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace plasma {
struct DrawPoint {
    size_t vertices = 0, items = 0;
};
struct LayerCommand {
    enum class Kind { Push, Pop, Mask } kind = Kind::Push;
    DrawPoint before;
    uint32_t mode = 0;
};
// Validate the engine command stream before recording GPU work. Native allocation
// failure/unbalanced nesting is unspecified; reject malformed engine input safely.
inline std::optional<size_t> layerDepth(std::span<const LayerCommand> commands, DrawPoint end) {
    DrawPoint previous;
    size_t depth = 0, maximum = 0;
    std::optional<size_t> mask;
    for (const auto &command : commands) {
        const auto at = command.before;
        if (at.vertices < previous.vertices || at.items < previous.items || at.vertices > end.vertices || at.items > end.items ||
            at.vertices % 3)
            return {};
        previous = at;
        switch (command.kind) {
        case LayerCommand::Kind::Push:
            if (++depth > maximum)
                maximum = depth;
            break;
        case LayerCommand::Kind::Pop:
            if (!depth)
                return {};
            mask = --depth;
            break;
        case LayerCommand::Kind::Mask:
            if (command.mode > 3 || (command.mode && (!mask || (depth && *mask == depth - 1))))
                return {};
            break;
        }
    }
    return depth ? std::optional<size_t>{} : maximum;
}
} // namespace plasma
