#pragma once
// Ordinary D3D11 backend facts: steam-sprite-buffer-resources.md §§1–6.
#include "NativeSpriteCatalog.h"

namespace captureabi {
struct SpriteVertexBuffer {
    std::uintptr_t wrapper = 0, context = 0, buffer = 0;
};
constexpr std::uintptr_t VERTEX_BUFFER_TABLE_RVA = 0x438390;

// Borrowed identities only: do not call the native binding methods or infer GPU
// readiness from wrapper presence, mesh counts, or a nonzero interface pointer.
// The caller validates the supported image. Reads must be guarded byte copies.
template <class Read>
bool readSpriteVertexBuffer(std::uintptr_t module, std::uintptr_t sprite, SpriteVertexBuffer &output, Read read) {
    output = {};
    constexpr auto maximum = std::numeric_limits<std::uintptr_t>::max();
    if (!module || module > maximum - VERTEX_BUFFER_TABLE_RVA)
        return false;
    std::uintptr_t wrapper = 0;
    if (!readSpriteField(sprite, 0x70, &wrapper, sizeof wrapper, read))
        return false;
    if (!wrapper)
        return true;
    const auto expectedTable = module + VERTEX_BUFFER_TABLE_RVA;
    std::uintptr_t table = 0;
    if (!readSpriteField(wrapper, 0, &table, sizeof table, read) || table != expectedTable)
        return false;
    std::array<std::uintptr_t, 2> fields{};
    if (!readSpriteField(wrapper, 0x08, fields.data(), sizeof fields, read))
        return false;
    std::uintptr_t currentWrapper = 0, currentTable = 0;
    if (!readSpriteField(sprite, 0x70, &currentWrapper, sizeof currentWrapper, read) || currentWrapper != wrapper ||
        !readSpriteField(wrapper, 0, &currentTable, sizeof currentTable, read) || currentTable != expectedTable)
        return false;
    // A recheck can reject a changed link/type; it cannot extend a COM or wrapper
    // lifetime, synchronize remeshing, or establish successful buffer creation.
    output = {wrapper, fields[0], fields[1]};
    return true;
}
} // namespace captureabi
