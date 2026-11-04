#include "../tools/ZoneExtractor/NativeCreatureEffects.h"
#include "../tools/ZoneExtractor/NativeEventQueues.h"
#include <cmath>
#include <iostream>
#include <map>

namespace {
template <class T> void store(std::vector<uint8_t> &bytes, size_t offset, const T &value) {
    std::memcpy(bytes.data() + offset, &value, sizeof value);
}
struct Fixture {
    static constexpr uintptr_t BUNDLE = 0x1000;
    std::map<uintptr_t, std::vector<uint8_t>> blocks;
    uintptr_t nextNode = 0x10000;
    Fixture() {
        blocks[BUNDLE].resize(0x100, 0xa5);
        for (const auto &layout : captureabi::SIMPLE_EVENT_LAYOUTS) {
            const uintptr_t head = 0x2000 + layout.slot * 0x100;
            blocks[head].resize(16);
            store(blocks[head], 0, head);
            store(blocks[head], 8, head);
            store(blocks[BUNDLE], layout.slot * 16, head);
            store(blocks[BUNDLE], layout.slot * 16 + 8, uint64_t(0));
        }
        // Excluded keyed/string headers remain poisoned and unreadable.
    }
    uint64_t word(uintptr_t base, size_t offset) const {
        uint64_t value;
        std::memcpy(&value, blocks.at(base).data() + offset, 8);
        return value;
    }
    uintptr_t append(uint8_t slot, const std::vector<uint8_t> &payload) {
        const auto head = word(BUNDLE, slot * 16), tail = word(head, 8);
        const auto node = nextNode;
        nextNode += 0x1000; // Sparse allocations, unrelated to native node stride.
        auto &bytes = blocks[node];
        bytes.resize(16 + payload.size());
        store(bytes, 0, head);
        store(bytes, 8, tail);
        std::memcpy(bytes.data() + 16, payload.data(), payload.size());
        store(blocks[tail], 0, node);
        store(blocks[head], 8, node);
        store(blocks[BUNDLE], slot * 16 + 8, word(BUNDLE, slot * 16 + 8) + 1);
        return node;
    }
    bool read(uintptr_t address, void *out, size_t count) const {
        auto found = blocks.upper_bound(address);
        if (found == blocks.begin())
            return false;
        --found;
        const auto offset = address - found->first;
        if (offset > found->second.size() || count > found->second.size() - offset)
            return false;
        std::memcpy(out, found->second.data() + offset, count);
        return true;
    }
};
} // namespace

int nativeEventQueueTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *label) {
        if (!passed) {
            std::cout << "FAIL event queues: " << label << '\n';
            ++failures;
        }
    };
    Fixture fixture;
    const auto read = [&](uintptr_t at, void *out, size_t count) { return fixture.read(at, out, count); };
    std::vector<captureabi::CapturedEvent> events{{255, {1}}};
    check(captureabi::readEventQueues(Fixture::BUNDLE, events, read) && events.empty(),
          "valid empty lists exclude sentinel payloads and poisoned keyed/owning-string headers");
    std::vector<uint8_t> sound(0x30, 0xa5), particle(0x48, 0xb6), pickup(0xd0, 0xc7);
    const captureabi::FixedPosition position{INT64_MIN, -65536, INT64_MAX};
    store(sound, 0, position);
    store(sound, 0x18, int32_t(-7));
    store(sound, 0x1c, 1.5f);
    store(sound, 0x20, -0.f);
    store(sound, 0x28, int64_t(-99));
    store(particle, 0, position);
    store(particle, 0x18, std::array<float, 3>{1, 2, 3});
    store(particle, 0x24, std::array<float, 4>{4, 5, 6, 7});
    store(particle, 0x34, uint32_t(0xdeadbeef));
    store(particle, 0x38, int32_t(-3));
    store(particle, 0x3c, int32_t(81));
    store(particle, 0x40, uint32_t(0xcafef00d));
    captureabi::ItemRecord item;
    item.bytes.fill(0x5a);
    item.setCategory(3);
    item.setSubtype(-31);
    store(pickup, 0, int64_t(77));
    std::memcpy(pickup.data() + 8, item.bytes.data(), item.bytes.size());
    store(pickup, 0xa8, uint16_t(65535));
    store(pickup, 0xb0, position);
    store(pickup, 0xc8, uint32_t(0x80000002));
    // Append in a different slot order; traversal preserves order within each list.
    const auto pickupNode = fixture.append(8, pickup);
    fixture.append(2, particle);
    const auto soundNode = fixture.append(1, sound);
    fixture.append(1, sound);
    check(captureabi::readEventQueues(Fixture::BUNDLE, events, read) && events.size() == 4 && events[0].slot == 1 &&
              events[1].bytes == events[0].bytes && events[2].slot == 2 && events[3].slot == 8,
          "sparse linked lists preserve duplicate records and established bundle-slot order");
    captureabi::SoundEvent decodedSound;
    captureabi::ParticleBurstEvent decodedParticle;
    captureabi::PickupEvent decodedPickup;
    check(events.size() == 4 && captureabi::decodeSoundEvent(events[0], decodedSound) &&
              decodedSound.position.x == INT64_MIN && decodedSound.position.z == INT64_MAX &&
              decodedSound.selector == -7 && decodedSound.owner == -99 && decodedSound.parameter1c == 1.5f &&
              std::signbit(decodedSound.volume20),
          "sound fields retain fixed64 positions, owner, selector, float bits and exclude the gap");
    check(events.size() == 4 && captureabi::decodeParticleBurstEvent(events[2], decodedParticle) &&
              decodedParticle.vector18 == std::array<float, 3>{1, 2, 3} &&
              decodedParticle.vector24 == std::array<float, 4>{4, 5, 6, 7} && decodedParticle.count == -3 &&
              decodedParticle.field34 == 0xdeadbeef && decodedParticle.field40 == 0xcafef00d,
          "burst vectors, signed count and unresolved four-byte fields remain distinct");
    check(events.size() == 4 && captureabi::decodePickupEvent(events[3], decodedPickup) &&
              decodedPickup.item.bytes == item.bytes && decodedPickup.quantity == 65535 && decodedPickup.actor == 77 &&
              decodedPickup.position.y == -65536 && decodedPickup.flags == 0x80000002 && events[3].bytes == pickup,
          "pickup preserves full item bytes and gaps, unsigned16 quantity, fixed position and flags");
    fixture.blocks[pickupNode][16] ^= 1;
    check(events.size() == 4 && events[3].bytes == pickup, "owned records survive later native-storage changes");
    fixture.blocks.erase(soundNode);
    check(!captureabi::readEventQueues(Fixture::BUNDLE, events, read) && events.empty(),
          "unreadable node publishes no partial bundle");
    fixture = Fixture{};
    const auto node = fixture.append(1, sound);
    store(fixture.blocks[node], 8, uintptr_t(0));
    check(!captureabi::readEventQueues(Fixture::BUNDLE, events, read), "broken previous link rejected");
    store(fixture.blocks[node], 8, fixture.word(Fixture::BUNDLE, 16));
    store(fixture.blocks[node], 0, node);
    check(!captureabi::readEventQueues(Fixture::BUNDLE, events, read), "cycle rejected by exact-count bound");
    fixture = Fixture{};
    fixture.append(1, sound);
    store(fixture.blocks[Fixture::BUNDLE], 24, uint64_t(2));
    check(!captureabi::readEventQueues(Fixture::BUNDLE, events, read), "premature sentinel/count mismatch rejected");
    fixture = Fixture{};
    store(fixture.blocks[Fixture::BUNDLE], 8, uint64_t(8193));
    store(fixture.blocks[Fixture::BUNDLE], 24, uint64_t(8192));
    check(!captureabi::readEventQueues(Fixture::BUNDLE, events, read),
          "combined event budget checked before traversal");
    fixture = Fixture{};
    int headerReads = 0;
    const auto changing = [&](uintptr_t at, void *out, size_t count) {
        if (at == Fixture::BUNDLE && ++headerReads == 2)
            store(fixture.blocks[Fixture::BUNDLE], 8, uint64_t(1));
        return read(at, out, count);
    };
    check(!captureabi::readEventQueues(Fixture::BUNDLE, events, changing) && events.empty(),
          "changed bundle header rejected after copying");
    check(captureabi::worldEventBundle(0x10000) == 0x10090 && captureabi::receivedEventBundle(0x10000) == 0x11970 &&
              !captureabi::worldEventBundle(0) && !captureabi::receivedEventBundle(UINTPTR_MAX - 15),
          "distinct owner paths reject null and overflowing bundle spans");
    check(!captureabi::decodeSoundEvent({1, {}}, decodedSound) &&
              !captureabi::decodePickupEvent({1, pickup}, decodedPickup),
          "typed views reject wrong extent or slot");
    // Effect lists use the same link/header shape, with their own 24-byte payload
    // and per-Creature ownership. Reuse sparse storage, not the event decoder.
    fixture = Fixture{};
    constexpr uintptr_t creature = Fixture::BUNDLE - 0x980;
    std::vector<captureabi::CreatureEffect> effects;
    check(captureabi::readCreatureEffects(creature, effects, read) && effects.empty(),
          "empty Creature effect list never treats sentinel bytes as a record");
    std::vector<uint8_t> effect(24, 0xd8);
    effect[0] = 0x1e;
    store(effect, 4, -0.f);
    store(effect, 8, -11.f);
    store(effect, 16, int64_t(INT64_MIN));
    const auto effectNode = fixture.append(0, effect);
    fixture.append(0, effect);
    check(captureabi::readCreatureEffects(creature, effects, read) && effects.size() == 2 &&
              effects[0].kind() == 0x1e && effects[0].time() == -11 && effects[0].source() == INT64_MIN &&
              std::signbit(effects[0].magnitude()) && effects[0].bytes == effects[1].bytes &&
              std::memcmp(effects[0].bytes.data(), effect.data(), effect.size()) == 0,
          "effect copies retain order, duplicates, nonpositive time, signed zero, source and unresolved bytes");
    fixture.blocks[effectNode][16] = 1;
    check(effects.size() == 2 && effects[0].kind() == 0x1e, "effect snapshots own bytes after source replacement");
    fixture.blocks.erase(effectNode);
    check(!captureabi::readCreatureEffects(creature, effects, read) && effects.empty(),
          "unreadable effect node rejects the whole result");
    fixture = Fixture{};
    const auto effectCycle = fixture.append(0, effect);
    store(fixture.blocks[effectCycle], 0, effectCycle);
    check(!captureabi::readCreatureEffects(creature, effects, read), "effect cycle bounded by native count");
    fixture = Fixture{};
    store(fixture.blocks[Fixture::BUNDLE], 8, uint64_t(captureabi::MAX_CAPTURE_EFFECTS + 1));
    check(!captureabi::readCreatureEffects(creature, effects, read), "effect budget checked before allocation");
    fixture = Fixture{};
    headerReads = 0;
    check(!captureabi::readCreatureEffects(creature, effects, changing) && effects.empty(),
          "effect header change during capture publishes nothing");
    std::cout << "Native event/effect queue capture: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
