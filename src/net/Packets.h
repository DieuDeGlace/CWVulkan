#pragma once
// Every packet of Steam Cube World's multiplayer, byte for byte (docs/specs/netcode-packets.md),
// plus CubeVulkan's own extras.
//
// A packet is one Steam P2P message (or one loopback buffer): a u32 packet id, then a body that
// depends on the id. No length, magic, checksum or sequence number. Packets 3 and 7 carry a
// zlib-compressed body behind an i32 length. Packet 14 has a different body in each direction,
// so decoding needs to know which way the packet went.
//
// CubeVulkan's extras use ids 25 and up, which the game never sends and ignores when it gets
// them (netcode-compatibility.md §3.3). They are only sent once both peers have said "hello"
// with packet 25 (see net/Session.h).
#include "net/CreatureDelta.h"
#include "net/Records.h"
#include "net/Wire.h"

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace cwnet {

// The game's protocol version, sent in packets 0 and 24.
constexpr uint32_t GAME_VERSION = 100001;

enum class PacketId : uint32_t {
    Invitation = 0,
    Join = 1,
    Leave = 2,
    CreatureUpdate = 3,
    YourId = 4,
    EndOfCreatureUpdates = 6,
    WorldUpdate = 7,
    Time = 8,
    Interaction = 9,
    DamageNumber = 10,
    SoundRequest = 11,
    BuffNotice = 12,
    EffectObject = 13,
    Chat = 14,
    ZoneRequest = 15,
    VersionMismatch = 24,
    // CubeVulkan only.
    CubeVulkanHello = 25,
    CubeVulkanMessage = 26,
};
constexpr uint32_t FIRST_EXTRA_ID = 25;

// Which way a packet goes: to the host (sent on channel 1) or to a client (channel 0).
enum class Direction { ToHost, ToClient };

// --- The packets -----------------------------------------------------------------------------

struct Invitation { // 0, host -> client: "come and join me"
    uint32_t version = 0; // 0 if the packet is too short to hold one (a mismatch)
};
struct Join {};  // 1, client -> host
struct Leave {}; // 2, either way
struct CreatureUpdate { // 3, either way
    CreatureDelta delta;
};
struct YourId { // 4, host -> client: the joiner's creature id (1..10)
    int64_t id = 0;
};
struct EndOfCreatureUpdates {}; // 6, host -> client: remove every creature not heard since the last 6

// One zone's whole list of ground items or static objects.
struct ZoneObjects {
    ZoneKey key;
    std::vector<ZoneObjectRecord> records;
};

// The fifteen lists of packet 7, in wire order (netcode-packets.md §2.7). The host world keeps
// the same lists as its event lists; each guest gets those that concern it.
struct WorldEvents {
    std::vector<BlockChangeRecord> blockChanges;  // 1
    std::vector<DamageRecord> damageNumbers;      // 2
    std::vector<ParticleRecord> particles;        // 3
    std::vector<SoundRecord> sounds;              // 4
    std::vector<EffectRecord> effects;            // 5
    std::vector<ZoneObjects> staticObjects;       // 6
    std::vector<ZoneObjects> groundItems;         // 7
    std::vector<ZoneKey> zonesAnswered;           // 8 (answers to packet 15)
    std::vector<PickupRecord> pickups;            // 9
    std::vector<KillRecord> kills;                // 10
    std::vector<FeatRecord> feats;                // 11
    std::vector<AggroRecord> aggro;               // 12
    std::vector<BuffRecord> buffs;                // 13
    std::vector<int32_t> sleeps;                  // 14 (0 = the world reset)
    std::vector<int64_t> calls;                   // 15 (a creature calling for help)

    bool empty() const;
    // Appends every list of `other` to this one's.
    void append(const WorldEvents &other);
};

struct WorldUpdate { // 7, host -> client
    WorldEvents events;
};
struct Time { // 8, host -> client
    int32_t day = 0;
    int32_t timeOfDay = 0; // ms; a day is 86,400,000
};
struct Interaction { // 9, client -> host
    InteractionRecord record = defaultInteraction();
};
struct DamageNumber { // 10, client -> host: a hit the guest's player made
    DamageRecord record{};
};
struct SoundRequest { // 11, client -> host
    SoundRecord record = defaultSound();
};
struct BuffNotice { // 12, client -> host
    BuffRecord record{};
};
struct EffectObject { // 13, client -> host
    EffectRecord record = defaultEffect();
};
struct Chat { // 14; `speaker` travels only host -> client
    int64_t speaker = 0;
    std::u16string text; // UTF-16 code units, no terminator
};
struct ZoneRequest { // 15, client -> host
    ZoneKey zone;
};
struct VersionMismatch { // 24, client -> host
    uint32_t version = GAME_VERSION;
};

// 25: "I am CubeVulkan". The magic keeps a stray 25 from anyone else from counting.
struct CubeVulkanHello {
    static constexpr uint32_t MAGIC = 0x314b5643; // "CVK1"
    uint32_t magic = MAGIC;
    uint32_t protocol = 1;      // CubeVulkan's extras protocol version
    uint32_t capabilities = 0;  // bit flags, for extras to agree on
};
// 26: a CubeVulkan extra: a channel name (for mods and engine features) and its bytes.
struct CubeVulkanMessage {
    std::string channel;
    Bytes payload;
};

// A packet with an id this build does not know; kept whole so it can be passed on or logged.
struct UnknownPacket {
    uint32_t id = 0;
    Bytes body;
};

using Packet = std::variant<Invitation, Join, Leave, CreatureUpdate, YourId, EndOfCreatureUpdates, WorldUpdate,
                            Time, Interaction, DamageNumber, SoundRequest, BuffNotice, EffectObject, Chat,
                            ZoneRequest, VersionMismatch, CubeVulkanHello, CubeVulkanMessage, UnknownPacket>;

uint32_t packetId(const Packet &packet);
const char *packetName(uint32_t id);

// --- Encoding and decoding ---------------------------------------------------------------------

// The packet's bytes, id first.
Bytes encode(const Packet &packet, Direction direction);

// What decoding a buffer found besides the packet.
struct DecodeInfo {
    bool cut = false;            // some read ran past the end (fields kept their defaults)
    size_t trailing = 0;         // bytes left over after the last field (ignored by the game)
    bool streamComplete = true;  // packets 3 and 7: the zlib stream ended properly
    bool ignored = false;        // the game would ignore it (packet 3 with a length <= 0)
    Bytes inflated;              // packets 3 and 7: the decompressed body
};

// Decodes a received buffer under the reader rule; never fails. A buffer shorter than an id is
// an UnknownPacket with id 0 flagged as cut.
Packet decode(const uint8_t *data, size_t size, Direction direction, DecodeInfo *info = nullptr);
inline Packet decode(const Bytes &b, Direction direction, DecodeInfo *info = nullptr) {
    return decode(b.data(), b.size(), direction, info);
}

// The decompressed body of packet 7 on its own (used by the encoder and by tools).
Bytes encodeWorldEvents(const WorldEvents &events);
WorldEvents decodeWorldEvents(const uint8_t *data, size_t size, bool *cut = nullptr);

} // namespace cwnet
