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
    std::cout << "Native sprite catalog: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
