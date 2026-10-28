#include "net/Recording.h"

#include "net/Transport.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>

namespace cwnet {

Direction RecordedPacket::direction() const {
    switch (side) {
    case RecordedSide::HostReceived: return Direction::ToHost;
    case RecordedSide::ClientReceived: return Direction::ToClient;
    case RecordedSide::Sent: return channel == (uint8_t)Channel::ToHost ? Direction::ToHost : Direction::ToClient;
    }
    return Direction::ToClient;
}

bool readRecording(const std::filesystem::path &file, std::vector<RecordedPacket> &packets, std::string &error) {
    std::ifstream in(file, std::ios::binary);
    if (!in) {
        error = "cannot open " + file.string();
        return false;
    }
    const Bytes data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ByteReader r(data);
    char magic[4] = {};
    uint32_t version = 0;
    r.bytes(magic, 4);
    r.value(version);
    if (std::memcmp(magic, RECORDING_MAGIC, 4) != 0 || version != RECORDING_VERSION) {
        error = file.string() + " is not a packet recording (CNR1, version 1)";
        return false;
    }
    while (!r.atEnd()) {
        uint32_t size = 0;
        r.value(size);
        if (size < RECORD_HEADER_BYTES || size > r.remaining()) {
            error = file.string() + ": a record is cut at byte " + std::to_string(r.offset());
            return false;
        }
        RecordedPacket p;
        uint8_t side = 0;
        r.value(p.timeMs);
        r.value(side);
        r.value(p.channel);
        r.value(p.peer);
        r.value(p.readPosition);
        p.side = (RecordedSide)side;
        const size_t n = size - RECORD_HEADER_BYTES;
        p.bytes.assign(r.here(), r.here() + n);
        r.skip(n);
        packets.push_back(std::move(p));
    }
    return true;
}

Bytes writeRecording(const std::vector<RecordedPacket> &packets) {
    Bytes out;
    ByteWriter w(out);
    w.bytes(RECORDING_MAGIC, 4);
    w.u32(RECORDING_VERSION);
    for (const RecordedPacket &p : packets) {
        w.u32((uint32_t)(RECORD_HEADER_BYTES + p.bytes.size()));
        w.u32(p.timeMs);
        w.value((uint8_t)p.side);
        w.value(p.channel);
        w.value(p.peer);
        w.u32(p.readPosition);
        w.bytes(p.bytes.data(), p.bytes.size());
    }
    return out;
}

const char *roundTripName(RoundTrip r) {
    switch (r) {
    case RoundTrip::Same: return "same";
    case RoundTrip::SameBody: return "same body";
    case RoundTrip::Cut: return "cut";
    case RoundTrip::Trailing: return "trailing bytes";
    case RoundTrip::Differs: return "DIFFERS";
    }
    return "?";
}

namespace {
std::string firstDifference(const Bytes &a, const Bytes &b, const char *what) {
    size_t i = 0;
    while (i < a.size() && i < b.size() && a[i] == b[i])
        i++;
    return std::string(what) + " differs at byte " + std::to_string(i) + " (recorded " + std::to_string(a.size()) +
           " bytes, encoded " + std::to_string(b.size()) + ")";
}
} // namespace

RoundTrip checkRoundTrip(const Bytes &bytes, Direction direction, std::string *detail) {
    DecodeInfo info;
    const Packet packet = decode(bytes, direction, &info);
    if (info.cut || !info.streamComplete)
        return RoundTrip::Cut;
    const Bytes again = encode(packet, direction);
    if (again == bytes)
        return RoundTrip::Same;
    const uint32_t id = packetId(packet);
    if (id == (uint32_t)PacketId::CreatureUpdate || id == (uint32_t)PacketId::WorldUpdate) {
        // The zlib bytes may differ from the game's; the inflated body must not, nor anything
        // after the stream.
        const Bytes body = id == (uint32_t)PacketId::CreatureUpdate
                               ? encodeDeltaBody(std::get<CreatureUpdate>(packet).delta)
                               : encodeWorldEvents(std::get<WorldUpdate>(packet).events);
        if (body != info.inflated) {
            if (detail)
                *detail = firstDifference(info.inflated, body, "the inflated body");
            return RoundTrip::Differs;
        }
        return info.trailing ? RoundTrip::Trailing : RoundTrip::SameBody;
    }
    if (info.trailing && again.size() + info.trailing == bytes.size() &&
        std::equal(again.begin(), again.end(), bytes.begin()))
        return RoundTrip::Trailing;
    if (detail)
        *detail = firstDifference(bytes, again, "the packet");
    return RoundTrip::Differs;
}

} // namespace cwnet
