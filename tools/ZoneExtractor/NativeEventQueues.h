#pragma once
// Original layout/lifecycle: steam-native-event-queues.md.
#include "CaptureMemory.h"
#include "NativeCreatureScalars.h"
#include "NativeItemRecord.h"
#include <array>
#include <utility>
#include <vector>

namespace captureabi {
struct EventLayout {
    uint8_t slot;
    size_t bytes;
    const char *label; // Existing diagnostic labels, not a universal taxonomy.
};
inline constexpr std::array<EventLayout, 13> SIMPLE_EVENT_LAYOUTS{{{0, 0x48, "hit"},
                                                                   {1, 0x30, "sound"},
                                                                   {2, 0x48, "particle"},
                                                                   {3, 0x1c, "block"},
                                                                   {4, 0x80, "effect"},
                                                                   {7, 8, "zone_reply"},
                                                                   {8, 0xd0, "pickup"},
                                                                   {9, 0x10, "p9"},
                                                                   {10, 0x28, "p10"},
                                                                   {11, 0x18, "p11"},
                                                                   {12, 0x30, "p12"},
                                                                   {13, 4, "p13"},
                                                                   {14, 8, "p14"}}};
constexpr size_t MAX_CAPTURE_EVENTS = 16384; // Combined tool budget, not a native maximum.
struct CapturedEvent {
    uint8_t slot;
    std::vector<uint8_t> bytes;
};
inline uintptr_t eventBundleAddress(uintptr_t owner, size_t offset) {
    constexpr auto maximum = std::numeric_limits<uintptr_t>::max();
    return owner && offset <= maximum - owner && 0xff <= maximum - (owner + offset) ? owner + offset : 0;
}
inline uintptr_t worldEventBundle(uintptr_t world) { return eventBundleAddress(world, 0x90); }
inline uintptr_t receivedEventBundle(uintptr_t game) { return eventBundleAddress(game, 0x1970); }

// Caller retains the live owner and the observed applicable guard(s) through
// this copy. Metadata rechecks do not pin nodes or freeze in-place payload writes.
// Nested keyed slots5/6 and owning wide-string slot15 are deliberately excluded.
template <class Read> bool readEventQueues(uintptr_t bundle, std::vector<CapturedEvent> &output, Read read) {
    output.clear();
    struct Snapshot {
        std::array<uintptr_t, 2> header, ends;
    };
    std::array<Snapshot, SIMPLE_EVENT_LAYOUTS.size()> snapshots;
    size_t total = 0;
    for (size_t i = 0; i < snapshots.size(); ++i) {
        auto &snapshot = snapshots[i];
        if (!readField(bundle, SIMPLE_EVENT_LAYOUTS[i].slot * 16, snapshot.header.data(), 16, read) ||
            !snapshot.header[0] || snapshot.header[1] > MAX_CAPTURE_EVENTS - total ||
            !readField(snapshot.header[0], 0, snapshot.ends.data(), 16, read))
            return false;
        total += snapshot.header[1];
    }
    std::vector<CapturedEvent> events;
    events.reserve(total);
    for (size_t i = 0; i < snapshots.size(); ++i) {
        const auto &layout = SIMPLE_EVENT_LAYOUTS[i];
        const auto &snapshot = snapshots[i];
        const auto sentinel = snapshot.header[0], count = snapshot.header[1];
        auto previous = sentinel, node = snapshot.ends[0];
        size_t visited = 0;
        while (node != sentinel) {
            if (!node || visited >= count)
                return false;
            std::array<uintptr_t, 2> links;
            CapturedEvent event{layout.slot, std::vector<uint8_t>(layout.bytes)};
            if (!readField(node, 0, links.data(), 16, read) || links[1] != previous ||
                !readField(node, 16, event.bytes.data(), event.bytes.size(), read))
                return false;
            events.push_back(std::move(event));
            previous = node;
            node = links[0];
            ++visited;
        }
        if (visited != count || previous != snapshot.ends[1])
            return false;
    }
    for (size_t i = 0; i < snapshots.size(); ++i) {
        std::array<uintptr_t, 2> header, ends;
        if (!readField(bundle, SIMPLE_EVENT_LAYOUTS[i].slot * 16, header.data(), 16, read) ||
            header != snapshots[i].header || !readField(header[0], 0, ends.data(), 16, read) ||
            ends != snapshots[i].ends)
            return false;
    }
    output = std::move(events);
    return true;
}

namespace detail {
// Used only after the decoder validates the complete fixed payload extent.
template <class T> T eventValue(const CapturedEvent &event, size_t offset) {
    T value;
    std::memcpy(&value, event.bytes.data() + offset, sizeof value);
    return value;
}
} // namespace detail
struct SoundEvent {
    FixedPosition position;
    int32_t selector;
    float parameter1c, volume20;
    int64_t owner;
};
inline bool decodeSoundEvent(const CapturedEvent &event, SoundEvent &output) {
    if (event.slot != 1 || event.bytes.size() != 0x30)
        return false;
    output = {detail::eventValue<FixedPosition>(event, 0), detail::eventValue<int32_t>(event, 0x18),
              detail::eventValue<float>(event, 0x1c), detail::eventValue<float>(event, 0x20),
              detail::eventValue<int64_t>(event, 0x28)};
    return true;
}
struct ParticleBurstEvent {
    FixedPosition position;
    std::array<float, 3> vector18;
    std::array<float, 4> vector24;
    uint32_t field34;
    int32_t count, selector;
    uint32_t field40;
};
inline bool decodeParticleBurstEvent(const CapturedEvent &event, ParticleBurstEvent &output) {
    if (event.slot != 2 || event.bytes.size() != 0x48)
        return false;
    output = {detail::eventValue<FixedPosition>(event, 0),
              detail::eventValue<std::array<float, 3>>(event, 0x18),
              detail::eventValue<std::array<float, 4>>(event, 0x24),
              detail::eventValue<uint32_t>(event, 0x34),
              detail::eventValue<int32_t>(event, 0x38),
              detail::eventValue<int32_t>(event, 0x3c),
              detail::eventValue<uint32_t>(event, 0x40)};
    return true;
}
struct PickupEvent {
    int64_t actor;
    ItemRecord item;
    uint16_t quantity;
    FixedPosition position;
    uint32_t flags;
};
inline bool decodePickupEvent(const CapturedEvent &event, PickupEvent &output) {
    if (event.slot != 8 || event.bytes.size() != 0xd0)
        return false;
    PickupEvent value;
    value.actor = detail::eventValue<int64_t>(event, 0);
    std::memcpy(value.item.bytes.data(), event.bytes.data() + 8, value.item.bytes.size());
    value.quantity = detail::eventValue<uint16_t>(event, 0xa8);
    value.position = detail::eventValue<FixedPosition>(event, 0xb0);
    value.flags = detail::eventValue<uint32_t>(event, 0xc8);
    output = value;
    return true;
}
} // namespace captureabi
