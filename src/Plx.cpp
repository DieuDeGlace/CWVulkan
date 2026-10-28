#include "Plx.h"
#include "Inflate.h"

#include <algorithm>
#include <fstream>
#include <set>

namespace plx {

namespace {

// The phrase that is the seal's plaintext, the header's name and the reader's own key phrase.
const char *const GRAPHICS_NAME = "PlasmaGraphics";
// The highest document version this reader accepts.
constexpr int HIGHEST_VERSION = 1;
// The key hash (spec 1.4): h = h * 31 + byte from this seed, wrapping at 64 bits.
constexpr uint64_t KEY_SEED = 0x0003FFFFFFFFFFE5ull;
constexpr uint64_t KEY_MULTIPLIER = 31;

// The properties that are containers of chunks (spec 1.6, 1.7.5): every animatable attribute,
// the sequences inside them and the keys inside those, and the text shape's legacy font block.
// Every other property is a leaf. The reader must enter all of them, because a tag definition
// may sit inside any of them.
const std::set<std::string> CONTAINER_PROPERTIES = {
    "Attribute.sequence",
    "Attribute.sequence.key",
    "Transformation.translation",
    "Transformation.rotation",
    "Transformation.pivot",
    "Transformation.deformation",
    "Display.visibility",
    "Display.clipping",
    "Display.fillColor",
    "Display.strokeColor",
    "Display.blurRadius",
    "TextShape.string",
    "TextShape.color",
    "TextShape.strokeColor",
    "TextShape.extrusionColor",
    "TextShape.font",
    "SmoothMeshShape.vertexPositions",
    "SmoothMeshShape.vertexTexCoords",
    "SmoothMeshShape.vertexColors",
    "SmoothMeshShape.strokeColors",
    "SmoothMeshShape.extrusionFrontColors",
    "SmoothMeshShape.extrusionBackColors",
    "SmoothMeshShape.strokeWidths",
    "SmoothMeshShape.texture",
    "SmoothMeshShape.strokeTexture",
    "SmoothMeshShape.textureTranslation",
    "SmoothMeshShape.texturePivot",
    "SmoothMeshShape.textureRotation",
    "SmoothMeshShape.textureDeformation",
    "SmoothMeshShape.extrusionMatrix",
    "SmoothMeshShape.textureOpacity",
    "SmoothMeshShape.textureBrightness",
    "SmoothMeshShape.textureContrast",
    "SmoothMeshShape.textureSaturation",
    "SmoothMeshShape.strokeTextureOpacity",
    "SmoothMeshShape.strokeTextureBrightness",
    "SmoothMeshShape.strokeTextureContrast",
    "SmoothMeshShape.strokeTextureSaturation",
    "SmoothMeshShape.strokeTextureStretch",
};

// The top-level object kinds by the family whose numbering they share (spec 1.5).
const std::set<std::string> SHAPE_KINDS = {"TextShape", "SmoothMeshShape", "GenericShape"};
const std::set<std::string> WIDGET_KINDS = {"Widget", "Button", "ListWidget", "Edit", "ScrollButton", "ScrollSlider", "PopUpButton"};

// String leaves stored as wide strings: the property names' last part (spec 1.1, 1.7).
const std::set<std::string> WIDE_STRING_PROPERTIES = {"wname", "wfontName", "wfileName", "wsource", "caption"};

std::string lastPart(const std::string &name) {
    const size_t dot = name.rfind('.');
    return dot == std::string::npos ? name : name.substr(dot + 1);
}

// The chunk stream (spec 1.2): tags with their definitions, nested chunk ends, and the seal key
// the definitions' names are read with.
class Stream {
  public:
    explicit Stream(const std::vector<uint8_t> &bytes) : data(bytes) {}

    bool failed() const { return broken; }
    bool atFileEnd() const { return broken || position >= data.size(); }

    int32_t readInt() {
        int32_t v = 0;
        if (position + 4 > data.size()) {
            broken = true;
            position = data.size();
            return 0;
        }
        std::memcpy(&v, data.data() + position, 4);
        position += 4;
        return v;
    }

    // A tag: its id, read after the definition that may precede it, as a name (an id never
    // defined reads as the empty name, which matches nothing).
    std::string readTag() {
        int32_t id = readInt();
        if (id == 0 && !broken) {
            const size_t size = (size_t)std::max(readInt(), 0);
            const size_t definitionEnd = position + size;
            const int32_t newId = readInt();
            names[newId] = readName();
            position = std::min(definitionEnd, data.size());
            id = readInt();
        }
        auto it = names.find(id);
        return it == names.end() ? std::string() : it->second;
    }

    // Containers: enter reads the size and remembers where the chunk ends; ended() is true (and
    // forgets that end) once the stream has reached it.
    void enter() {
        const size_t size = (size_t)std::max(readInt(), 0);
        ends.push_back(std::min(position + size, data.size()));
    }
    bool ended() {
        if (ends.empty())
            return atFileEnd();
        if (broken || position >= ends.back()) {
            position = std::max(position, ends.back());
            ends.pop_back();
            return true;
        }
        return false;
    }
    // A leaf: its payload, the stream left at the chunk's end.
    std::vector<uint8_t> leaf() {
        const size_t size = (size_t)std::max(readInt(), 0);
        const size_t end = std::min(position + size, data.size());
        if (position + size > data.size())
            broken = true;
        std::vector<uint8_t> payload(data.begin() + (ptrdiff_t)position, data.begin() + (ptrdiff_t)end);
        position = end;
        return payload;
    }

    void setKey(uint64_t k) { key = k; }

  private:
    const std::vector<uint8_t> &data;
    size_t position = 0;
    bool broken = false;
    std::vector<size_t> ends;
    std::map<int32_t, std::string> names; // one table for the whole file, never cleared
    uint64_t key = 0;

    // A definition's name: a plain byte string while there is no key, otherwise a count and that
    // many sealed bytes (spec 1.4).
    std::string readName() {
        const size_t n = (size_t)std::max(readInt(), 0);
        if (position + n > data.size()) {
            broken = true;
            return {};
        }
        std::vector<uint8_t> bytes = unseal(data.data() + position, n, key);
        position += n;
        if (key == 0) // a plain string ends at its first zero byte
            bytes.erase(std::find(bytes.begin(), bytes.end(), (uint8_t)0), bytes.end());
        return std::string(bytes.begin(), bytes.end());
    }
};

// A property and everything under it.
Chunk readProperty(Stream &s, const std::string &name) {
    Chunk c;
    c.name = name;
    if (!CONTAINER_PROPERTIES.count(name)) {
        c.bytes = s.leaf();
        return c;
    }
    s.enter();
    while (!s.ended()) {
        const std::string tag = s.readTag();
        c.children.push_back(readProperty(s, tag));
    }
    return c;
}

Object readObject(Stream &s, const std::string &kind) {
    Object o;
    o.kind = kind;
    s.enter();
    while (!s.ended()) {
        const std::string tag = s.readTag();
        o.properties.push_back(readProperty(s, tag));
    }
    return o;
}

// Node.variable: two byte strings. spec: open question 1 (format spec) - the second string is
// most likely the key; no shipped file has a variable.
std::pair<std::string, std::string> variableOf(const std::vector<uint8_t> &payload) {
    size_t end = 0;
    const std::string first = byteString(payload, 0, &end);
    const std::string second = byteString(payload, end);
    return {second, first};
}

Node nodeOf(const Object &o) {
    Node n;
    for (const Chunk &p : o.properties) {
        if (p.name == "Node.name" || p.name == "Node.wname")
            n.name = p.text();
        else if (p.name == "Node.shape")
            n.shape = p.as<int32_t>();
        else if (p.name == "Node.transformation")
            n.transformation = p.as<int32_t>();
        else if (p.name == "Node.display")
            n.display = p.as<int32_t>();
        else if (p.name == "Node.widget")
            n.widget = p.as<int32_t>();
        else if (p.name == "Node.child")
            n.children.push_back(p.as<int32_t>());
        else if (p.name == "Node.flags")
            n.flags = p.as<uint32_t>();
        else if (p.name == "Node.variable") {
            auto v = variableOf(p.bytes);
            // a key already present keeps its first value
            if (std::none_of(n.variables.begin(), n.variables.end(), [&](const auto &e) { return e.first == v.first; }))
                n.variables.push_back(v);
        }
    }
    return n;
}

// A byte array leaf: an int count, then that many bytes.
std::vector<uint8_t> byteArray(const std::vector<uint8_t> &payload) {
    if (payload.size() < 4)
        return {};
    int32_t n = 0;
    std::memcpy(&n, payload.data(), 4);
    const size_t count = std::min((size_t)std::max(n, 0), payload.size() - 4);
    return std::vector<uint8_t>(payload.begin() + 4, payload.begin() + 4 + (ptrdiff_t)count);
}

// A texture's pixels. spec: open question (the texture format is outside the two specs) - both
// pixel leaves are taken to be byte arrays, "Texture.compressedPixels" holding a zlib stream (as
// the shipped ones do); pixels that do not decode are kept empty.
void keepPixels(const Object &texture, Document &doc) {
    Document::Pixels px;
    int id = -1;
    for (const Chunk &p : texture.properties) {
        if (p.name == "Texture.name" || p.name == "Texture.wname")
            px.name = p.text();
        else if (p.name == "Texture.id")
            id = p.as<int32_t>();
        else if (p.name == "Texture.width")
            px.width = p.as<int32_t>();
        else if (p.name == "Texture.height")
            px.height = p.as<int32_t>();
        else if (p.name == "Texture.format.pixelFormat")
            px.format = p.as<int32_t>();
        else if (p.name == "Texture.pixels")
            px.data = byteArray(p.bytes);
        else if (p.name == "Texture.compressedPixels") {
            const std::vector<uint8_t> packed = byteArray(p.bytes);
            if (!inflate::zlib(packed.data(), packed.size(), px.data))
                px.data.clear();
        }
    }
    doc.pixels[id] = std::move(px);
}

// The seal (spec 1.4): the first candidate key that turns its bytes into "PlasmaGraphics".
bool openSeal(const std::vector<uint8_t> &payload, const std::vector<uint64_t> &keys, uint64_t &key) {
    const size_t n = (size_t)std::max(Chunk{"", payload, {}}.as<int32_t>(), 0);
    if (payload.size() < 4 + n)
        return false;
    std::vector<uint64_t> candidates = keys;
    candidates.push_back(0);
    candidates.push_back(keyOf(GRAPHICS_NAME));
    const std::string expected = GRAPHICS_NAME;
    for (uint64_t k : candidates) {
        const std::vector<uint8_t> plain = unseal(payload.data() + 4, n, k);
        if (std::string(plain.begin(), plain.end()) == expected) {
            key = k;
            return true;
        }
    }
    return false;
}

} // namespace

const Chunk *Chunk::child(const std::string &n) const {
    for (const Chunk &c : children)
        if (c.name == n)
            return &c;
    return nullptr;
}

std::vector<const std::vector<uint8_t> *> Chunk::frames() const {
    std::vector<const std::vector<uint8_t> *> out;
    for (const Chunk &c : children)
        if (c.name == "Attribute.frame" || c.name == "ArrayAttribute.frame")
            out.push_back(&c.bytes);
    return out;
}

std::string byteString(const std::vector<uint8_t> &payload, size_t offset, size_t *end) {
    int32_t n = 0;
    if (offset + 4 <= payload.size())
        std::memcpy(&n, payload.data() + offset, 4);
    const size_t count = std::min((size_t)std::max(n, 0), payload.size() >= offset + 4 ? payload.size() - offset - 4 : 0);
    std::string s;
    for (size_t i = 0; i < count && payload[offset + 4 + i]; i++)
        s.push_back((char)payload[offset + 4 + i]);
    if (end)
        *end = offset + 4 + count;
    return s;
}

std::wstring wideString(const std::vector<uint8_t> &payload, size_t offset, size_t *end) {
    int32_t n = 0;
    if (offset + 4 <= payload.size())
        std::memcpy(&n, payload.data() + offset, 4);
    const size_t count = std::min((size_t)std::max(n, 0), payload.size() >= offset + 4 ? (payload.size() - offset - 4) / 2 : 0);
    std::wstring s;
    for (size_t i = 0; i < count; i++) {
        const wchar_t unit = (wchar_t)(payload[offset + 4 + i * 2] | payload[offset + 5 + i * 2] << 8);
        if (!unit)
            break;
        s.push_back(unit);
    }
    if (end)
        *end = offset + 4 + count * 2;
    return s;
}

std::wstring Chunk::wideText() const {
    if (WIDE_STRING_PROPERTIES.count(lastPart(name)))
        return wideString(bytes);
    const std::string narrow = byteString(bytes);
    return std::wstring(narrow.begin(), narrow.end()); // the game widens byte strings unit by unit
}

std::string Chunk::text() const {
    std::string s;
    for (wchar_t c : wideText())
        s.push_back((char)c);
    return s;
}

// Sequences (spec 1.6): a sequence's name comes before its keys and creates or replaces the
// sequence of that name; each key is inserted before the first key whose time is not earlier.
bool Chunk::sequence(const std::string &wanted, std::vector<Key> &keys) const {
    bool found = false;
    for (const Chunk &s : children) {
        if (s.name != "Attribute.sequence")
            continue;
        std::string name;
        std::vector<Key> sorted;
        for (const Chunk &part : s.children) {
            if (part.name == "Attribute.sequence.name" || part.name == "Attribute.sequence.wname") {
                name = part.text();
                sorted.clear();
            } else if (part.name == "Attribute.sequence.key") {
                Key k;
                for (const Chunk &field : part.children) {
                    if (field.name == "Attribute.sequence.key.frame")
                        k.slot = field.as<int32_t>();
                    else if (field.name == "Attribute.sequence.key.time")
                        k.time = field.as<int32_t>();
                    else if (field.name == "Attribute.sequence.key.smoothness")
                        k.smoothness = field.as<float>();
                }
                auto at = std::find_if(sorted.begin(), sorted.end(), [&](const Key &e) { return e.time >= k.time; });
                sorted.insert(at, k);
            }
        }
        if (name == wanted) {
            keys = std::move(sorted);
            found = true;
        }
    }
    return found;
}

const Chunk *Object::property(const std::string &n) const {
    for (const Chunk &c : properties)
        if (c.name == n)
            return &c;
    return nullptr;
}

int Document::find(const std::string &name) const {
    std::vector<int> pending = {0};
    std::vector<bool> seen(nodes.size(), false);
    while (!pending.empty()) {
        const int i = pending.back();
        pending.pop_back();
        if (i < 0 || i >= (int)nodes.size() || seen[(size_t)i])
            continue;
        seen[(size_t)i] = true;
        if (nodes[(size_t)i].name == name)
            return i;
        const std::vector<int> &children = nodes[(size_t)i].children;
        for (auto it = children.rbegin(); it != children.rend(); ++it)
            pending.push_back(*it);
    }
    return -1;
}

uint64_t keyOf(const std::string &phrase) {
    uint64_t h = KEY_SEED;
    for (unsigned char b : phrase)
        h = h * KEY_MULTIPLIER + b;
    return h;
}

// The writer rotated the plaintext left by s = (low 32 bits of the key) mod n and added the
// key's bytes, indexed by plaintext position; this takes them away again.
std::vector<uint8_t> unseal(const uint8_t *in, size_t n, uint64_t key) {
    std::vector<uint8_t> out(n);
    if (!n)
        return out;
    const size_t start = (size_t)((uint32_t)key % n);
    for (size_t i = 0; i < n; i++) {
        const size_t j = (start + i) % n;
        const uint8_t keyByte = (uint8_t)(key >> (8 * (j % 8)));
        out[j] = (uint8_t)(in[i] - keyByte);
    }
    return out;
}

bool read(const std::vector<uint8_t> &data, Document &doc, const std::vector<uint64_t> &keys, std::string *why, int loaderFlags) {
    doc = Document{};
    doc.loaderFlags = loaderFlags;
    auto fail = [&](const std::string &reason) {
        if (why)
            *why = reason;
        return false;
    };
    if (loaderFlags & LOAD_CHECK_HEADER) { // the first definition's name, 16 bytes in
        const std::string name = GRAPHICS_NAME;
        if (data.size() < 16 + name.size() || std::memcmp(data.data() + 16, name.data(), name.size()) != 0)
            return fail("has invalid format");
    }
    Stream s(data);
    std::vector<Object> nodeObjects;
    while (!s.atFileEnd()) {
        const std::string tag = s.readTag();
        if (s.failed())
            break;
        if (tag == "PlasmaGraphics") {
            doc.version = Chunk{"", s.leaf(), {}}.as<int32_t>();
            if (doc.version > HIGHEST_VERSION)
                return fail("has invalid version");
        } else if (tag == "Seal") {
            uint64_t key = 0;
            if (!openSeal(s.leaf(), keys, key))
                return fail("was created with a different demo license");
            s.setKey(key);
        } else if (tag == "Texture") {
            doc.textures.push_back(readObject(s, tag));
            keepPixels(doc.textures.back(), doc);
        } else if (SHAPE_KINDS.count(tag))
            doc.shapes.push_back(readObject(s, tag));
        else if (tag == "Transformation")
            doc.transformations.push_back(readObject(s, tag));
        else if (tag == "Display")
            doc.displays.push_back(readObject(s, tag));
        else if (WIDGET_KINDS.count(tag))
            doc.widgets.push_back(readObject(s, tag));
        else if (tag == "Node")
            nodeObjects.push_back(readObject(s, tag));
        else if (tag == "pageWidth" || tag == "pageHeight" || tag == "dpi" || tag == "unit" || tag == "pageColor") {
            const Chunk value{tag, s.leaf(), {}};
            if (loaderFlags & LOAD_NO_PAGE_INFO)
                continue;
            if (tag == "pageWidth")
                doc.pageWidth = value.as<float>();
            else if (tag == "pageHeight")
                doc.pageHeight = value.as<float>();
            else if (tag == "dpi")
                doc.dpi = value.as<float>();
            else if (tag == "unit")
                doc.unit = value.as<int32_t>();
            else
                for (int k = 0; k < 4; k++)
                    doc.pageColor[k] = value.as<float>((size_t)k * 4);
        } else
            s.leaf(); // anything else is skipped
    }
    if (s.failed())
        return fail("is truncated");
    for (const Object &o : nodeObjects) {
        Node n = nodeOf(o);
        if (loaderFlags & LOAD_MARK_NODES)
            n.flags |= NODE_LOADED;
        doc.nodes.push_back(std::move(n));
    }
    // The first node stands for the node the document is loaded into: everything it carries but
    // its children is read and discarded (spec 1.7.1).
    if (!doc.nodes.empty()) {
        Node root;
        root.children = std::move(doc.nodes[0].children);
        doc.nodes[0] = std::move(root);
    }
    return true;
}

bool read(const std::filesystem::path &file, Document &doc, const std::vector<uint64_t> &keys, std::string *why, int loaderFlags) {
    std::ifstream in(file, std::ios::binary);
    if (!in) {
        if (why)
            *why = file.string() + " cannot be opened";
        return false;
    }
    const std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), {});
    std::string reason;
    if (!read(data, doc, keys, &reason, loaderFlags)) {
        if (why)
            *why = file.string() + " " + reason + ".";
        return false;
    }
    return true;
}

} // namespace plx
