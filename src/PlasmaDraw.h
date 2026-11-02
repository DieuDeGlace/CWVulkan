#pragma once
// Ordered ordinary clip layers, from steam-plasma-clip-layers.md.
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <memory>
#include <glm/glm.hpp>
#include <cmath>

namespace plasma {
struct DrawPoint {
    size_t vertices = 0, items = 0;
};
inline std::optional<glm::ivec4> cacheRectangle(glm::vec2 origin, glm::vec2 size) {
    const glm::vec4 values(origin, size);
    glm::ivec4 result;
    for (int i = 0; i < 4; ++i) {
        const float rounded = values[i] + .5f;
        if (!std::isfinite(rounded) || rounded < -2147483648.f || rounded >= 2147483648.f)
            return {};
        result[i] = (int32_t)rounded;
    }
    return result.z > 0 && result.w > 0 ? std::optional<glm::ivec4>(result) : std::nullopt;
}
struct RetainedSurface {
    bool painted = false; // invalidated by renderer resource destruction
};
struct LayerCommand {
    enum class Kind { Push, Pop, Mask, CacheBegin, CacheEnd, CacheCopy } kind = Kind::Push;
    DrawPoint before;
    uint32_t mode = 0;
    std::shared_ptr<RetainedSurface> surface;
    glm::ivec4 rectangle{0}; // independently rounded physical origin and extent
};
// Validate the engine command stream before recording GPU work. Native allocation
// failure/unbalanced nesting is unspecified; reject malformed engine input safely.
inline std::optional<size_t> layerDepth(std::span<const LayerCommand> commands, DrawPoint end) {
    DrawPoint previous;
    size_t depth = 0, maximum = 0;
    std::optional<size_t> mask;
    const RetainedSurface *cached = nullptr;
    for (const auto &command : commands) {
        const auto at = command.before;
        if (at.vertices < previous.vertices || at.items < previous.items || at.vertices > end.vertices || at.items > end.items ||
            at.vertices % 3)
            return {};
        previous = at;
        switch (command.kind) {
        case LayerCommand::Kind::CacheBegin:
            if (cached || depth || !command.surface || command.rectangle.z <= 0 || command.rectangle.w <= 0)
                return {};
            cached = command.surface.get();
            break;
        case LayerCommand::Kind::CacheEnd:
            if (!cached || depth || command.surface.get() != cached)
                return {};
            cached = nullptr;
            break;
        case LayerCommand::Kind::CacheCopy:
            if (cached || !command.surface || command.rectangle.z <= 0 || command.rectangle.w <= 0)
                return {};
            break;
        case LayerCommand::Kind::Push:
            if (cached)
                return {}; // Nested clip/cache projection is outside the admitted chat command stream.
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
    return depth || cached ? std::optional<size_t>{} : maximum;
}
} // namespace plasma
