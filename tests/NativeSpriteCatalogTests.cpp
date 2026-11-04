#include "../tools/ZoneExtractor/NativeSpriteBuffers.h"
#include "../tools/ZoneExtractor/NativeSpriteCatalog.h"
#include <cstring>
#include <iostream>
#include <map>

int nativeSpriteCatalogTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *label) {
        if (!passed) {
            std::cout << "FAIL native sprite catalog: " << label << '\n';
            ++failures;
        }
    };
    constexpr std::uintptr_t world = 0x100000, catalog = 0x200000, array = 0x300000, first = 0x400000, last = 0x500000;
    std::map<std::uintptr_t, std::vector<std::uint8_t>> memory;
    std::vector<std::uintptr_t> reads;
    const auto store = [&](std::uintptr_t address, auto value) {
        auto &bytes = memory[address];
        bytes.resize(sizeof value);
        std::memcpy(bytes.data(), &value, sizeof value);
    };
    const auto read = [&](std::uintptr_t address, void *destination, std::size_t count) {
        reads.push_back(address);
        const auto found = memory.find(address);
        if (found == memory.end() || found->second.size() != count)
            return false;
        std::memcpy(destination, found->second.data(), count);
        return true;
    };
    const auto fixture = [&] {
        memory.clear();
        reads.clear();
        store(world + 0x70, catalog);
        // Capacity has two unused entries which must never be read as live slots.
        store(catalog + 0x10, std::array<std::uintptr_t, 3>{array, array + 24, array + 40});
        store(array, std::array<std::uintptr_t, 3>{first, 0, last});
        store(first + 0x88, captureabi::SpriteDimensions{10, 8, 10});
        store(last + 0x88, captureabi::SpriteDimensions{0, 0, 0});
    };
    std::vector<captureabi::PrimarySprite> output;
    const auto capture = [&] { return captureabi::readPrimarySprites(world, output, read); };
    fixture();
    check(capture() && output.size() == 3 && output[0].address == first && output[0].dimensions.x == 10 &&
              output[0].dimensions.y == 8 && output[0].dimensions.z == 10 && output[1].address == 0 &&
              output[1].dimensions.x == 0 && output[2].address == last && output[2].dimensions.z == 0,
          "catalog order, null slots and unready dimensions survive without reading capacity entries");
    fixture();
    store(catalog + 0x10, std::array<std::uintptr_t, 3>{0, 0, 0});
    check(capture() && output.empty(), "an initialized empty vector is not an invented readiness failure");
    const std::array<std::array<std::uintptr_t, 3>, 5> malformed{
        {{0, array, array},
         {array + 8, array, array + 16},
         {array, array + 24, array + 16},
         {array, array + 23, array + 40},
         {array, array + 8 * (captureabi::MAX_PRIMARY_SPRITES + 1),
          array + 8 * (captureabi::MAX_PRIMARY_SPRITES + 1)}}};
    for (const auto &bounds : malformed) {
        fixture();
        store(catalog + 0x10, bounds);
        check(!capture() && output.empty(), "corrupt or excessive ranges publish no partial metadata");
    }
    fixture();
    memory.erase(last + 0x88);
    output.push_back(captureabi::PrimarySprite{first, {99, 99, 99}});
    check(!capture() && output.empty(), "an unreadable later sprite cannot publish an earlier partial table");
    fixture();
    memory.erase(array);
    check(!capture() && output.empty(), "an inaccessible slot array fails closed");
    fixture();
    store(world + 0x70, std::uintptr_t(0));
    check(!capture(), "a null catalog never becomes a native object read");
    fixture();
    unsigned boundsReads = 0;
    const auto changingRead = [&](std::uintptr_t address, void *destination, std::size_t count) {
        if (address == catalog + 0x10 && ++boundsReads == 2)
            store(catalog + 0x10, std::array<std::uintptr_t, 3>{array, array + 16, array + 40});
        return read(address, destination, count);
    };
    check(!captureabi::readPrimarySprites(world, output, changingRead) && output.empty(),
          "a changed live range is rejected before publication");
    fixture();
    unsigned worldReads = 0;
    const auto replacingRead = [&](std::uintptr_t address, void *destination, std::size_t count) {
        if (address == world + 0x70 && ++worldReads == 2)
            store(world + 0x70, catalog + 0x1000);
        return read(address, destination, count);
    };
    check(!captureabi::readPrimarySprites(world, output, replacingRead) && output.empty(),
          "a replaced catalog cannot be paired with its predecessor's slots");
    constexpr auto maximum = std::numeric_limits<std::uintptr_t>::max();
    fixture();
    reads.clear();
    check(!captureabi::readPrimarySprites(maximum, output, read) && reads.empty(),
          "World field arithmetic never wraps into readable low memory");
    fixture();
    store(world + 0x70, maximum - 0x10);
    check(!capture(), "the complete catalog field span must fit the address space");
    fixture();
    store(array, std::array<std::uintptr_t, 3>{maximum - 0x88, 0, last});
    check(!capture(), "the complete dimensions span must fit the address space");
    // Native wrapper and COM buffer are distinct identities. No callable vtable
    // memory or RGB/mesh payload is supplied by this fixture.
    constexpr std::uintptr_t module = 0x10000000, wrapper = 0x600000, context = 0x700000, buffer = 0x800000;
    const auto vertexFixture = [&] {
        memory.clear();
        reads.clear();
        store(first + 0x70, wrapper);
        store(wrapper, module + captureabi::VERTEX_BUFFER_TABLE_RVA);
        store(wrapper + 8, std::array<std::uintptr_t, 2>{context, buffer});
    };
    captureabi::VertexBufferView vertex;
    const auto vertexCapture = [&] { return captureabi::readSpriteVertexBuffer(module, first, vertex, read); };
    vertexFixture();
    check(vertexCapture() && vertex.wrapper == wrapper && vertex.context == context && vertex.buffer == buffer,
          "the native wrapper yields its actual context/buffer fields without invoking a binding method");
    store(first + 0x80, std::array<std::uint32_t, 2>{0, 0});
    check(vertexCapture() && vertex.buffer == buffer, "zero mesh counts can retain previous wrappers");
    store(wrapper + 8, std::array<std::uintptr_t, 2>{context, 0});
    check(vertexCapture() && vertex.wrapper == wrapper && !vertex.buffer,
          "null buffer metadata does not invent successful creation or GPU readiness");
    store(first + 0x70, std::uintptr_t(0));
    check(vertexCapture() && !vertex.wrapper && !vertex.context && !vertex.buffer,
          "an absent wrapper clears an earlier resource identity");
    vertexFixture();
    store(wrapper, module + captureabi::VERTEX_BUFFER_TABLE_RVA + 32);
    check(!vertexCapture() && !vertex.wrapper, "an unproved wrapper implementation is rejected before field capture");
    vertexFixture();
    memory.erase(wrapper + 8);
    check(!vertexCapture() && !vertex.context && !vertex.buffer,
          "unreadable resource fields publish no partial identity");
    vertexFixture();
    unsigned wrapperReads = 0;
    const auto remeshingRead = [&](std::uintptr_t address, void *destination, std::size_t count) {
        if (address == first + 0x70 && ++wrapperReads == 2)
            store(first + 0x70, std::uintptr_t(0));
        return read(address, destination, count);
    };
    check(!captureabi::readSpriteVertexBuffer(module, first, vertex, remeshingRead) && !vertex.wrapper,
          "a replaced wrapper is rejected before publication");
    vertexFixture();
    unsigned typeReads = 0;
    const auto reusingRead = [&](std::uintptr_t address, void *destination, std::size_t count) {
        if (address == wrapper && ++typeReads == 2)
            store(wrapper, std::uintptr_t(0));
        return read(address, destination, count);
    };
    check(!captureabi::readSpriteVertexBuffer(module, first, vertex, reusingRead) && !vertex.wrapper,
          "a changed wrapper type is rejected before publication");
    vertexFixture();
    reads.clear();
    check(!captureabi::readSpriteVertexBuffer(maximum, first, vertex, read) && reads.empty(),
          "module-relative type identity cannot wrap into low memory");
    store(first + 0x70, maximum - 8);
    store(maximum - 8, module + captureabi::VERTEX_BUFFER_TABLE_RVA);
    check(!vertexCapture() && !vertex.wrapper, "the complete context/buffer span must fit the address space");
    constexpr uintptr_t backend = 0x900000;
    const auto graphicsFixture = [&] {
        memory.clear();
        reads.clear();
        store(world + 0x70, catalog);
        store(catalog + 8, backend);
        store(backend, module + captureabi::GRAPHICS_BACKEND_TABLE_RVA);
        store(backend + 0x10, context);
    };
    uintptr_t graphics = 123;
    const auto graphicsCapture = [&] { return captureabi::readCatalogContext(module, world, graphics, read); };
    graphicsFixture();
    check(graphicsCapture() && graphics == context,
          "catalog backend context is a borrowed data field, with no COM or virtual method call");
    store(backend + 0x10, uintptr_t(0));
    check(graphicsCapture() && !graphics, "readable null context is not invented GPU readiness");
    graphicsFixture();
    store(backend, module + captureabi::GRAPHICS_BACKEND_TABLE_RVA + 8);
    check(!graphicsCapture() && !graphics, "unproved graphics backend rejected without stale context output");
    graphicsFixture();
    memory.erase(backend + 0x10);
    check(!graphicsCapture() && !graphics, "unreadable context does not publish partial identity");
    for (const auto field : {world + 0x70, catalog + 8, backend, backend + 0x10}) {
        graphicsFixture();
        unsigned fieldReads = 0;
        const auto changingGraphics = [&](uintptr_t address, void *destination, size_t count) {
            if (address == field && ++fieldReads == 2)
                store(field, uintptr_t(0));
            return read(address, destination, count);
        };
        check(!captureabi::readCatalogContext(module, world, graphics, changingGraphics) && !graphics,
              "changed catalog/backend/table/context identity rejects publication");
    }
    graphicsFixture();
    check(!captureabi::readCatalogContext(maximum, world, graphics, read) && !graphics && reads.empty(),
          "graphics module identity arithmetic cannot wrap");
    graphicsFixture();
    check(!captureabi::readCatalogContext(module, maximum, graphics, read) && !graphics && reads.empty(),
          "graphics World link span cannot wrap");
    graphicsFixture();
    store(catalog + 8, maximum - 8);
    store(maximum - 8, module + captureabi::GRAPHICS_BACKEND_TABLE_RVA);
    check(!graphicsCapture() && !graphics, "graphics backend context span cannot wrap into low memory");
    std::cout << "Native sprite catalog: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
