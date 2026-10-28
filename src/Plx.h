#pragma once
// The game's interface documents (.plx: gui.plx, icons.plx, cursor.plx, quest-tag.plx), read from
// the owner's installation as the interface library Plasma reads them
// (docs/specs/plasma-format-and-scene.md, part 1).
//
// A .plx is one flat stream of tagged chunks (a tag id, a payload size, the payload). Tag names
// are not stored up front: the first time a name is used, a definition carrying it precedes the
// chunk, and after the "Seal" chunk those names are obfuscated with the document's key. The top
// level holds the objects (textures, shapes, transformations, displays, widgets) and then the
// nodes that tie them into a tree.
//
// Objects are kept as their properties: a leaf property keeps its payload bytes, a container
// property (an animatable attribute, its sequences and their keys, a text shape's font block)
// keeps its sub-chunks. The shape, text and scene code read the values they need from them.
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace plx {

// A key of an attribute's animation sequence: the frame slot it shows, at a time in milliseconds,
// and how smoothly the curve passes through it (spec 1.6, 2.6.2).
struct Key {
    int slot = 0;
    int time = 0;
    float smoothness = 0;
};

// A property chunk: a leaf's payload bytes, or a container's sub-chunks.
struct Chunk {
    std::string name;
    std::vector<uint8_t> bytes;
    std::vector<Chunk> children;

    // The first sub-chunk of a name (nullptr if none).
    const Chunk *child(const std::string &n) const;
    // The payload read as a value at a byte offset (zero when the payload is too short).
    template <class T> T as(size_t offset = 0) const {
        T value{};
        if (offset + sizeof(T) <= bytes.size())
            std::memcpy(&value, bytes.data() + offset, sizeof(T));
        return value;
    }
    // An attribute's frame slots, the payload of each "Attribute.frame" or "ArrayAttribute.frame"
    // in order: slot 0 is the live value, slot 1 a scratch slot (often stale, never to be read),
    // slots 2 and up the poses that sequence keys point at (spec 1.6).
    std::vector<const std::vector<uint8_t> *> frames() const;
    // A string leaf (a byte string, or a wide one for the wide properties such as "Node.wname"),
    // as Latin-1 / as UTF-16.
    std::string text() const;
    std::wstring wideText() const;
    // The keys of this attribute's sequence of that name, sorted by time as Plasma inserts them.
    // False when the attribute has no sequence of that name.
    bool sequence(const std::string &name, std::vector<Key> &keys) const;
};

// The live value (frame slot 0) of an attribute as T, `fallback` when there is none.
template <class T> T live(const Chunk *attribute, T fallback) {
    if (!attribute)
        return fallback;
    const auto slots = attribute->frames();
    if (slots.empty() || slots[0]->size() < sizeof(T))
        return fallback;
    T value;
    std::memcpy(&value, slots[0]->data(), sizeof(T));
    return value;
}
// The live array (frame slot 0) of an array attribute, its elements as T; an
// "ArrayAttribute.size" sets the element count (new elements zero).
template <class T> std::vector<T> liveArray(const Chunk *attribute) {
    std::vector<T> out;
    if (!attribute)
        return out;
    const auto slots = attribute->frames();
    if (!slots.empty()) {
        out.resize(slots[0]->size() / sizeof(T));
        if (!out.empty())
            std::memcpy(out.data(), slots[0]->data(), out.size() * sizeof(T));
    }
    if (const Chunk *size = attribute->child("ArrayAttribute.size"))
        out.resize((size_t)std::max(size->as<int32_t>(), 0), T{});
    return out;
}
// A leaf holding an int count and that many elements (faces, vertexFlags, vertexParameters).
template <class T> std::vector<T> countedArray(const Chunk *leaf) {
    std::vector<T> out;
    if (!leaf || leaf->bytes.size() < 4)
        return out;
    const int32_t n = leaf->as<int32_t>();
    out.resize(std::min((size_t)std::max(n, 0), (leaf->bytes.size() - 4) / sizeof(T)));
    if (!out.empty())
        std::memcpy(out.data(), leaf->bytes.data() + 4, out.size() * sizeof(T));
    return out;
}

// Strings as stored in a payload: an int count, then that many bytes (or 16-bit units), cut at
// the first zero (spec 1.1).
std::string byteString(const std::vector<uint8_t> &payload, size_t offset = 0, size_t *end = nullptr);
std::wstring wideString(const std::vector<uint8_t> &payload, size_t offset = 0, size_t *end = nullptr);

// One top-level object: its kind ("Texture", "TextShape", "SmoothMeshShape", "GenericShape",
// "Transformation", "Display", "Widget", "Button", ...) and its properties in file order.
struct Object {
    std::string kind;
    std::vector<Chunk> properties;
    const Chunk *property(const std::string &n) const;
};

// Node flags (spec 2.3.4).
enum : uint32_t {
    NODE_NO_PICK_1 = 0x1,       // left out of hit tests that ask so
    NODE_PASSIVE = 0x2,         // skipped by widget visitors and most hit tests
    NODE_DISABLED = 0x4,        // not drawn, updated, hit, measured or copied
    NODE_LOADED = 0x8,          // set by the loader flag LOAD_MARK_NODES
    NODE_AUTO_FIT = 0x80,       // its shape is refitted to its children's bounds
    NODE_CACHED = 0x400,        // its subtree is drawn through a cached surface
    NODE_NO_PICK = 0x1000,      // left out of hit tests
    NODE_ALWAYS_ANIMATE = 0x2000 // keeps animating while invisible (with its subtree)
};

struct Node {
    std::string name;
    int shape = -1, transformation = -1, display = -1, widget = -1; // object indexes, -1 for none
    uint32_t flags = 0;
    std::vector<int> children; // node indexes, in drawing order
    std::vector<std::pair<std::string, std::string>> variables; // key, value
};

// The loader flags a document is read with (spec 1.5). The game reads gui.plx, icons.plx and
// quest-tag.plx with LOAD_SCALABLE_TEXT and cursor.plx with none.
enum : int {
    LOAD_MARK_NODES = 0x04,     // every node gets NODE_LOADED
    LOAD_NO_PAGE_INFO = 0x08,   // the page size, resolution, unit and colour are skipped
    LOAD_STATIC_MESHES = 0x10,  // unanimated meshes are frozen (no effect on what is drawn)
    LOAD_SCALABLE_TEXT = 0x20,  // every text shape is drawn in the scalable raster mode
    LOAD_CHECK_HEADER = 0x40    // the file must start with the "PlasmaGraphics" definition
};

struct Document {
    int version = 0;
    int loaderFlags = 0;
    // The objects by family, in file order: their index is what nodes refer to. The three shape
    // kinds share one numbering, as do the seven widget kinds.
    std::vector<Object> shapes, transformations, displays, widgets, textures;
    // nodes[0] stands for the node the document is loaded into: only its children are kept.
    std::vector<Node> nodes;
    // The page the document was designed on.
    float pageWidth = 0, pageHeight = 0, dpi = 0;
    int unit = 0;
    float pageColor[4] = {0, 0, 0, 0};
    // A texture's pixels by its Texture.id (what shapes refer to).
    struct Pixels {
        std::string name;
        int width = 0, height = 0, format = 0;
        std::vector<uint8_t> data;
    };
    std::map<int, Pixels> pixels;
    // The first node of a name, depth first from the root (each node before its children), -1
    // if none (spec 2.4).
    int find(const std::string &name) const;
};

// The 64-bit key a phrase stands for (spec 1.4). The game seals its files with the key of
// "PlasmaXGraphics" (0xFB3A260B5EC7CD5C).
uint64_t keyOf(const std::string &phrase);
// Undoes a seal: `n` obfuscated bytes with a key (spec 1.4). Key 0 leaves them unchanged.
std::vector<uint8_t> unseal(const uint8_t *in, size_t n, uint64_t key);

// Reads a .plx. `keys` are the keys to try on the seal before 0 and the reader's own
// ("PlasmaGraphics"). False (with `why`) when the file cannot be read, has a newer version, or
// its seal matches no key.
bool read(const std::filesystem::path &file, Document &doc, const std::vector<uint64_t> &keys, std::string *why = nullptr,
          int loaderFlags = 0);
bool read(const std::vector<uint8_t> &data, Document &doc, const std::vector<uint64_t> &keys, std::string *why = nullptr,
          int loaderFlags = 0);

} // namespace plx
