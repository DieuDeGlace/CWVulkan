#pragma once
// Packet recordings made in the original game by ZoneExtractor's packet recorder
// (docs/specs/netcode-compatibility.md §5.1), and the round-trip check that makes them the
// oracle for this protocol: every recorded packet must decode and encode back to the same bytes.
//
// File layout (Extract\net\net-*.bin, little-endian):
//   char[4] "CNR1", u32 format version (1)
//   records, each: u32 size of the rest of the record, u32 time (timeGetTime, ms),
//     u8 side (0 received by the host dispatcher 0xf4490, 1 received by the client dispatcher
//     0x3e450, 2 sent through SteamNetworking005's SendP2PPacket), u8 channel (0xff unknown),
//     u16 peer (0 = this machine; others numbered 1, 2, ... in order of appearance, not their
//     SteamIDs), u32 the dispatcher buffer's read position on entry (0 expected),
//     then the packet's bytes (id first).
#include "net/Packets.h"

#include <filesystem>
#include <string>
#include <vector>

namespace cwnet {

enum class RecordedSide : uint8_t { HostReceived = 0, ClientReceived = 1, Sent = 2 };

struct RecordedPacket {
    uint32_t timeMs = 0;
    RecordedSide side = RecordedSide::HostReceived;
    uint8_t channel = 0xff;
    uint16_t peer = 0;
    uint32_t readPosition = 0;
    Bytes bytes;

    // Which way the packet went: what the host receives or is sent on channel 1 went to a host.
    Direction direction() const;
};

constexpr char RECORDING_MAGIC[4] = {'C', 'N', 'R', '1'};
constexpr uint32_t RECORDING_VERSION = 1;
constexpr size_t RECORD_HEADER_BYTES = 12; // time, side, channel, peer, read position

bool readRecording(const std::filesystem::path &file, std::vector<RecordedPacket> &packets, std::string &error);
Bytes writeRecording(const std::vector<RecordedPacket> &packets); // the file's bytes (for tests and tools)

// The result of decoding a packet and encoding it again.
enum class RoundTrip {
    Same,       // the same bytes
    SameBody,   // packets 3 and 7: the same id and inflated body; only the zlib bytes differ
    Cut,        // the packet was shorter than its fields (the game would read defaults)
    Trailing,   // extra bytes after the last field (the game ignores them)
    Differs,    // a real mismatch: the codec is wrong somewhere
};
const char *roundTripName(RoundTrip r);

// Decodes `bytes` (sent in `direction`) and encodes the result again. `detail` explains a
// mismatch (the first differing offset).
RoundTrip checkRoundTrip(const Bytes &bytes, Direction direction, std::string *detail = nullptr);

} // namespace cwnet
