#pragma once
// The game records that travel inside packets (docs/specs/netcode-packets.md §2.7, §2.9, §2.10).
// Each travels as the raw bytes of its in-memory layout, padding included, so the network layer
// keeps them as fixed-size byte arrays: the field layouts belong to the modules that make and
// use them (creature-hit.md §0.3/§0.4, effect-objects.md §0.1, zone-object-interaction.md §6,
// skills-and-interactions.md §2.1, ...). Keeping them whole is also what lets a received packet
// be re-encoded byte for byte.
#include <array>
#include <cstdint>
#include <cstring>
#include <tuple>

namespace cwnet {

template <size_t N> using Record = std::array<uint8_t, N>;

using BlockChangeRecord = Record<0x1c>;  // the first 28 bytes of a block change (effect-objects.md §0.5)
using DamageRecord = Record<0x48>;       // a damage number (creature-hit.md §0.3)
using ParticleRecord = Record<0x48>;     // a particle burst (creature-hit.md §0.4)
using SoundRecord = Record<0x30>;        // a sound request (creature-hit.md §0.4)
using EffectRecord = Record<0x80>;       // an effect object (effect-objects.md §0.1)
using ZoneObjectRecord = Record<0xe0>;   // a ground item or a static object of a zone
using PickupRecord = Record<0xd0>;       // a pick-up notice (zone-object-interaction.md §6)
using KillRecord = Record<0x10>;         // a kill credit (creature-hit.md §0.4)
using FeatRecord = Record<0x28>;         // a feat notice: creature id, then 32 untraced bytes
using AggroRecord = Record<0x18>;        // an aggro notice (creature-hit.md §0.4)
using BuffRecord = Record<0x30>;         // a buff-change notice (creature-hit.md §0.4)
using InteractionRecord = Record<0xc8>;  // a pending interaction (skills-and-interactions.md §2.1)

// A zone's key on the wire: zone x then zone y, zones being 64 x 64 blocks.
struct ZoneKey {
    int32_t x = 0, y = 0;
    bool operator<(const ZoneKey &o) const { return std::tie(x, y) < std::tie(o.x, o.y); }
    bool operator==(const ZoneKey &o) const { return x == o.x && y == o.y; }
};
static_assert(sizeof(ZoneKey) == 8);

// Field offsets the network layer itself reads in these records.
namespace sound {
constexpr size_t PITCH = 0x1c;  // f32
constexpr size_t VOLUME = 0x20; // f32
constexpr size_t OWNER = 0x28;  // i64: the creature that made the sound
} // namespace sound
namespace damage {
constexpr size_t ATTACKER = 0x00; // i64
} // namespace damage
namespace buffnote {
constexpr size_t SOURCE = 0x00; // i64
} // namespace buffnote
namespace effect {
constexpr size_t OWNER = 0x00;    // i64
constexpr size_t RADIUS = 0x54;   // f32
constexpr size_t SIZE = 0x58;     // f32
constexpr size_t CHARGE = 0x5c;   // f32
constexpr size_t LIFETIME = 0x74; // i32 (ms)
} // namespace effect
namespace interaction {
constexpr size_t ZONE_X = 0xa0;  // i32
constexpr size_t ZONE_Y = 0xa4;  // i32
constexpr size_t INDEX = 0xa8;   // i32: the object's index in the zone
constexpr size_t TARGET = 0xb0;  // i64
constexpr size_t KIND = 0xc4;    // byte
constexpr size_t MARKER = 0xc6;  // i16: always 1 when created
} // namespace interaction

template <class T, size_t N> inline T field(const Record<N> &r, size_t offset) {
    T v;
    std::memcpy(&v, r.data() + offset, sizeof v);
    return v;
}
template <class T, size_t N> inline void setField(Record<N> &r, size_t offset, T v) {
    std::memcpy(r.data() + offset, &v, sizeof v);
}

// What a receiver starts from before reading a record, so that a short packet leaves the
// right defaults (netcode-packets.md §2.9, §2.10). Records without listed defaults start zeroed.

// A sound request: pitch and volume 1.0, owner 0.
inline SoundRecord defaultSound() {
    SoundRecord r{};
    setField(r, sound::PITCH, 1.0f);
    setField(r, sound::VOLUME, 1.0f);
    return r;
}
// An effect object as its constructor leaves it: owner -1, radius 0.5, size and charge 1.0,
// lifetime 5000 ms.
inline EffectRecord defaultEffect() {
    EffectRecord r{};
    setField(r, effect::OWNER, (int64_t)-1);
    setField(r, effect::RADIUS, 0.5f);
    setField(r, effect::SIZE, 1.0f);
    setField(r, effect::CHARGE, 1.0f);
    setField(r, effect::LIFETIME, (int32_t)5000);
    return r;
}
// An interaction: zone x, zone y and object index -1, the 16-bit field 1.
inline InteractionRecord defaultInteraction() {
    InteractionRecord r{};
    setField(r, interaction::ZONE_X, (int32_t)-1);
    setField(r, interaction::ZONE_Y, (int32_t)-1);
    setField(r, interaction::INDEX, (int32_t)-1);
    setField(r, interaction::MARKER, (int16_t)1);
    return r;
}

} // namespace cwnet
