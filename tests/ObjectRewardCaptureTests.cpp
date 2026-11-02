#include "StaticObjectRewards.h"
#include "../tools/ZoneExtractor/ObjectRewardCapture.h"
#include <cstdio>
#include <tuple>

namespace {
constexpr uint64_t ZONE = 0x1000, HEAD = 0x3000, ROOT = 0x4000, LEFT = 0x5000, RIGHT = 0x2000;
struct Memory {
    std::map<uint64_t, std::vector<uint8_t>> blocks;
    template <class T> void put(uint64_t address, size_t at, T value) {
        auto &bytes = blocks.at(address);
        std::memcpy(bytes.data() + at, &value, sizeof value);
    }
    bool read(uint64_t address, void *out, size_t size) const {
        auto found = blocks.upper_bound(address);
        if (found == blocks.begin())
            return false;
        --found;
        const uint64_t offset = address - found->first;
        if (offset > found->second.size() || size > found->second.size() - offset)
            return false;
        std::memcpy(out, found->second.data() + offset, size);
        return true;
    }
    void empty() {
        blocks.clear();
        blocks[ZONE + 0x450].resize(16);
        put(ZONE + 0x450, 0, HEAD);
        put(ZONE + 0x450, 8, uint64_t(0));
        blocks[HEAD].resize(0xc0, 0xab); // sentinel key/value bytes must never become a reward
        for (size_t offset : {size_t(0), size_t(8), size_t(16)})
            put(HEAD, offset, HEAD);
        blocks[HEAD][0x18] = blocks[HEAD][0x19] = 1;
    }
    objectrewards::Map populated() {
        empty();
        put(ZONE + 0x450, 8, uint64_t(3));
        put(HEAD, 0, LEFT);
        put(HEAD, 8, ROOT);
        put(HEAD, 16, RIGHT);
        objectrewards::Map expected;
        for (const auto [address, key, category] : {std::tuple{ROOT, int32_t(0), uint8_t(23)}, std::tuple{LEFT, INT32_MIN, uint8_t(3)},
                                                    std::tuple{RIGHT, INT32_MAX, uint8_t(0)}}) {
            auto &node = blocks[address];
            node.resize(0xc0);
            put(address, 0, address == ROOT ? LEFT : HEAD);
            put(address, 8, address == ROOT ? HEAD : ROOT);
            put(address, 16, address == ROOT ? RIGHT : HEAD);
            node[0x18] = 1; // real black nodes remain entries: isNil, not color, selects the sentinel
            put(address, 0x1c, key);
            objectrewards::Item item;
            for (size_t byte = 0; byte < item.size(); ++byte)
                item[byte] = uint8_t(byte ^ category);
            item[0] = category;
            std::memcpy(node.data() + 0x20, item.data(), item.size());
            expected.emplace(key, item);
        }
        return expected;
    }
};
} // namespace

int objectRewardCaptureTests() {
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        if (!ok) {
            ++failures;
            std::printf("FAIL object reward capture: %s\n", message);
        }
    };
    Memory memory;
    const auto copy = [&](uint64_t at, void *out, size_t size) { return memory.read(at, out, size); };
    memory.empty();
    const auto empty = rewardcapture::read(ZONE, copy);
    check(empty && empty->empty(), "constructed self-linked map is known empty and never emits sentinel payload");
    memory.put(ZONE + 0x450, 0, uint64_t(0));
    check(!rewardcapture::read(ZONE, copy), "null construction/unavailable head is unknown, not known empty");
    const auto expected = memory.populated();
    const auto captured = rewardcapture::read(ZONE, copy);
    check(captured && *captured == expected,
          "signed key order, inline full items and category0 entries survive unrelated node-address order");
    const auto encoded = objectrewards::encode(expected);
    check(objectrewards::decode(encoded) == expected && objectrewards::decode({}) == objectrewards::Map{},
          "CWR payload round-trips full items and explicit empty maps");
    auto bad = encoded;
    bad.pop_back();
    check(!objectrewards::decode(bad), "truncated file record cannot become known reward metadata");
    bad = encoded;
    std::memcpy(bad.data() + objectrewards::RECORD_BYTES, bad.data(), 4);
    check(!objectrewards::decode(bad), "duplicate signed keys cannot silently overwrite a captured reward");
    memory.put(RIGHT, 0x1c, int32_t(0));
    check(!rewardcapture::read(ZONE, copy), "duplicate native keys reject an inconsistent observation");
    memory.populated();
    memory.put(LEFT, 0, ROOT);
    check(!rewardcapture::read(ZONE, copy), "cycles and wrong parent links fail within a bounded walk");
    memory.populated();
    memory.put(ZONE + 0x450, 8, uint64_t(2));
    check(!rewardcapture::read(ZONE, copy), "real entry count must agree with sentinel traversal");
    memory.populated();
    memory.blocks.erase(RIGHT);
    check(!rewardcapture::read(ZONE, copy), "unavailable native allocation does not turn into an empty-map success");
    memory.populated();
    int wrapperReads = 0;
    const auto changing = [&](uint64_t at, void *out, size_t size) {
        if (at == ZONE + 0x450 && ++wrapperReads == 2)
            memory.put(ZONE + 0x450, 8, uint64_t(0));
        return memory.read(at, out, size);
    };
    check(!rewardcapture::read(ZONE, changing), "visible before/after wrapper changes reject the capture without claiming atomicity");
    std::printf("Native object reward capture: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
