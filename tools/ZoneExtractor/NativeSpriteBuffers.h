#pragma once
// Ordinary D3D11 backend facts: steam-sprite-buffer-resources.md §§1–6.
#include "CaptureMemory.h"
#include <array>

namespace captureabi {
struct VertexBufferView {
    std::uintptr_t wrapper = 0, context = 0, buffer = 0;
};
constexpr std::uintptr_t VERTEX_BUFFER_TABLE_RVA = 0x438390;

inline bool vertexBufferModuleFits(std::uintptr_t module) {
    return module && module <= std::numeric_limits<std::uintptr_t>::max() - VERTEX_BUFFER_TABLE_RVA;
}

// Borrowed identities only. No binding method, AddRef, GPU-readiness test or
// resource ownership is implied. The caller validates the image and lifetime.
template <class Read>
bool readVertexBufferWrapper(std::uintptr_t module, std::uintptr_t wrapper, VertexBufferView &output, Read read) {
    output = {};
    if (!vertexBufferModuleFits(module))
        return false;
    if (!wrapper)
        return true;
    const auto expectedTable = module + VERTEX_BUFFER_TABLE_RVA;
    std::uintptr_t table = 0;
    if (!readField(wrapper, 0, &table, sizeof table, read) || table != expectedTable)
        return false;
    std::array<std::uintptr_t, 2> fields{};
    if (!readField(wrapper, 0x08, fields.data(), sizeof fields, read))
        return false;
    if (!readField(wrapper, 0, &table, sizeof table, read) || table != expectedTable)
        return false;
    output = {wrapper, fields[0], fields[1]};
    return true;
}

template <class Read>
bool readSpriteVertexBuffer(std::uintptr_t module, std::uintptr_t sprite, VertexBufferView &output, Read read) {
    output = {};
    if (!vertexBufferModuleFits(module))
        return false;
    std::uintptr_t wrapper = 0;
    if (!readField(sprite, 0x70, &wrapper, sizeof wrapper, read))
        return false;
    VertexBufferView result;
    if (!readVertexBufferWrapper(module, wrapper, result, read))
        return false;
    std::uintptr_t currentWrapper = 0;
    if (!readField(sprite, 0x70, &currentWrapper, sizeof currentWrapper, read) || currentWrapper != wrapper)
        return false;
    // Rechecking rejects changed links/types; it does not synchronize remeshing
    // or extend the lifetime of a native wrapper, context or COM interface.
    output = result;
    return true;
}
} // namespace captureabi
