#include "net/Packets.h"

#include <algorithm>
#include <type_traits>

namespace cwnet {

namespace {

// --- Lists of packet 7 -----------------------------------------------------------------------

// A simple list: an i32 count, then that many raw records.
template <class R> void writeList(ByteWriter &w, const std::vector<R> &list) {
    w.i32((int32_t)list.size());
    for (const R &r : list)
        w.value(r);
}

// Reads a simple list. Each record starts from `fallback` and is read under the reader rule; a
// count larger than the bytes left stops at the end (the records that would follow could only
// be defaults, and a hostile count must not make us allocate without bound).
template <class R> void readList(ByteReader &r, std::vector<R> &list, const R &fallback = R{}) {
    int32_t count = 0;
    r.value(count);
    for (int32_t i = 0; i < count && !r.atEnd(); i++) {
        R record = fallback;
        r.value(record);
        list.push_back(record);
    }
}

// A keyed list: an i32 count, then per zone its key, an i32 count and that many 0xe0 records.
void writeZoneLists(ByteWriter &w, const std::vector<ZoneObjects> &zones) {
    w.i32((int32_t)zones.size());
    for (const ZoneObjects &z : zones) {
        w.value(z.key);
        writeList(w, z.records);
    }
}

void readZoneLists(ByteReader &r, std::vector<ZoneObjects> &zones) {
    int32_t count = 0;
    r.value(count);
    for (int32_t i = 0; i < count && !r.atEnd(); i++) {
        ZoneObjects z;
        r.value(z.key);
        readList(r, z.records);
        zones.push_back(std::move(z));
    }
}

// --- Compressed bodies (packets 3 and 7) -------------------------------------------------------

void writeCompressed(ByteWriter &w, const Bytes &body) {
    const Bytes packed = compress(body);
    w.i32((int32_t)packed.size());
    w.bytes(packed.data(), packed.size());
}

// Reads i32 L and L bytes of zlib, and inflates them. Returns false when L <= 0.
bool readCompressed(ByteReader &r, Bytes &body, DecodeInfo &info) {
    int32_t length = 0;
    r.value(length);
    if (length <= 0)
        return false;
    if ((size_t)length > r.remaining()) {
        r.skip((size_t)length); // the reader rule: nothing read, position at the end
        info.streamComplete = false;
        return true;
    }
    bool complete = false;
    body = inflateStream(r.here(), (size_t)length, &complete);
    info.streamComplete = complete;
    r.skip((size_t)length);
    return true;
}

// --- Chat text -----------------------------------------------------------------------------------

void writeText(ByteWriter &w, const std::u16string &text) {
    w.i32((int32_t)text.size());
    w.bytes(text.data(), text.size() * sizeof(char16_t));
}

void readText(ByteReader &r, std::u16string &text) {
    int32_t count = 0;
    r.value(count);
    for (int32_t i = 0; i < count && !r.atEnd(); i++) {
        char16_t unit = 0;
        if (r.value(unit))
            text.push_back(unit);
    }
}

template <class> constexpr bool ALWAYS_FALSE = false;

} // namespace

// --- WorldEvents --------------------------------------------------------------------------------

bool WorldEvents::empty() const {
    return blockChanges.empty() && damageNumbers.empty() && particles.empty() && sounds.empty() &&
           effects.empty() && staticObjects.empty() && groundItems.empty() && zonesAnswered.empty() &&
           pickups.empty() && kills.empty() && feats.empty() && aggro.empty() && buffs.empty() &&
           sleeps.empty() && calls.empty();
}

void WorldEvents::append(const WorldEvents &o) {
    auto add = [](auto &to, const auto &from) { to.insert(to.end(), from.begin(), from.end()); };
    add(blockChanges, o.blockChanges);
    add(damageNumbers, o.damageNumbers);
    add(particles, o.particles);
    add(sounds, o.sounds);
    add(effects, o.effects);
    add(staticObjects, o.staticObjects);
    add(groundItems, o.groundItems);
    add(zonesAnswered, o.zonesAnswered);
    add(pickups, o.pickups);
    add(kills, o.kills);
    add(feats, o.feats);
    add(aggro, o.aggro);
    add(buffs, o.buffs);
    add(sleeps, o.sleeps);
    add(calls, o.calls);
}

Bytes encodeWorldEvents(const WorldEvents &e) {
    Bytes out;
    ByteWriter w(out);
    writeList(w, e.blockChanges);
    writeList(w, e.damageNumbers);
    writeList(w, e.particles);
    writeList(w, e.sounds);
    writeList(w, e.effects);
    writeZoneLists(w, e.staticObjects);
    writeZoneLists(w, e.groundItems);
    writeList(w, e.zonesAnswered);
    writeList(w, e.pickups);
    writeList(w, e.kills);
    writeList(w, e.feats);
    writeList(w, e.aggro);
    writeList(w, e.buffs);
    writeList(w, e.sleeps);
    writeList(w, e.calls);
    return out;
}

WorldEvents decodeWorldEvents(const uint8_t *data, size_t size, bool *cut) {
    WorldEvents e;
    ByteReader r(data, size);
    readList(r, e.blockChanges);
    readList(r, e.damageNumbers);
    readList(r, e.particles);
    readList(r, e.sounds, defaultSound());
    readList(r, e.effects, defaultEffect());
    readZoneLists(r, e.staticObjects);
    readZoneLists(r, e.groundItems);
    readList(r, e.zonesAnswered);
    readList(r, e.pickups);
    readList(r, e.kills);
    readList(r, e.feats);
    readList(r, e.aggro);
    readList(r, e.buffs);
    readList(r, e.sleeps);
    readList(r, e.calls);
    if (cut)
        *cut = r.wasShort();
    return e;
}

// --- Ids and names ---------------------------------------------------------------------------------

uint32_t packetId(const Packet &packet) {
    return std::visit(
        [](const auto &p) -> uint32_t {
            using T = std::decay_t<decltype(p)>;
            if constexpr (std::is_same_v<T, Invitation>) return (uint32_t)PacketId::Invitation;
            else if constexpr (std::is_same_v<T, Join>) return (uint32_t)PacketId::Join;
            else if constexpr (std::is_same_v<T, Leave>) return (uint32_t)PacketId::Leave;
            else if constexpr (std::is_same_v<T, CreatureUpdate>) return (uint32_t)PacketId::CreatureUpdate;
            else if constexpr (std::is_same_v<T, YourId>) return (uint32_t)PacketId::YourId;
            else if constexpr (std::is_same_v<T, EndOfCreatureUpdates>) return (uint32_t)PacketId::EndOfCreatureUpdates;
            else if constexpr (std::is_same_v<T, WorldUpdate>) return (uint32_t)PacketId::WorldUpdate;
            else if constexpr (std::is_same_v<T, Time>) return (uint32_t)PacketId::Time;
            else if constexpr (std::is_same_v<T, Interaction>) return (uint32_t)PacketId::Interaction;
            else if constexpr (std::is_same_v<T, DamageNumber>) return (uint32_t)PacketId::DamageNumber;
            else if constexpr (std::is_same_v<T, SoundRequest>) return (uint32_t)PacketId::SoundRequest;
            else if constexpr (std::is_same_v<T, BuffNotice>) return (uint32_t)PacketId::BuffNotice;
            else if constexpr (std::is_same_v<T, EffectObject>) return (uint32_t)PacketId::EffectObject;
            else if constexpr (std::is_same_v<T, Chat>) return (uint32_t)PacketId::Chat;
            else if constexpr (std::is_same_v<T, ZoneRequest>) return (uint32_t)PacketId::ZoneRequest;
            else if constexpr (std::is_same_v<T, VersionMismatch>) return (uint32_t)PacketId::VersionMismatch;
            else if constexpr (std::is_same_v<T, CubeVulkanHello>) return (uint32_t)PacketId::CubeVulkanHello;
            else if constexpr (std::is_same_v<T, CubeVulkanMessage>) return (uint32_t)PacketId::CubeVulkanMessage;
            else if constexpr (std::is_same_v<T, UnknownPacket>) return p.id;
            else static_assert(ALWAYS_FALSE<T>);
        },
        packet);
}

const char *packetName(uint32_t id) {
    switch ((PacketId)id) {
    case PacketId::Invitation: return "invitation";
    case PacketId::Join: return "join";
    case PacketId::Leave: return "leave";
    case PacketId::CreatureUpdate: return "creature update";
    case PacketId::YourId: return "your id";
    case PacketId::EndOfCreatureUpdates: return "end of creature updates";
    case PacketId::WorldUpdate: return "world update";
    case PacketId::Time: return "time";
    case PacketId::Interaction: return "interaction";
    case PacketId::DamageNumber: return "damage number";
    case PacketId::SoundRequest: return "sound request";
    case PacketId::BuffNotice: return "buff-change notice";
    case PacketId::EffectObject: return "effect object";
    case PacketId::Chat: return "chat";
    case PacketId::ZoneRequest: return "zone request";
    case PacketId::VersionMismatch: return "version mismatch";
    case PacketId::CubeVulkanHello: return "CubeVulkan hello";
    case PacketId::CubeVulkanMessage: return "CubeVulkan message";
    }
    return "unknown";
}

// --- Encoding --------------------------------------------------------------------------------------

Bytes encode(const Packet &packet, Direction direction) {
    Bytes out;
    ByteWriter w(out);
    w.u32(packetId(packet));
    std::visit(
        [&](const auto &p) {
            using T = std::decay_t<decltype(p)>;
            if constexpr (std::is_same_v<T, Invitation> || std::is_same_v<T, VersionMismatch>)
                w.u32(p.version);
            else if constexpr (std::is_same_v<T, CreatureUpdate>)
                writeCompressed(w, encodeDeltaBody(p.delta));
            else if constexpr (std::is_same_v<T, YourId>)
                w.i64(p.id);
            else if constexpr (std::is_same_v<T, WorldUpdate>)
                writeCompressed(w, encodeWorldEvents(p.events));
            else if constexpr (std::is_same_v<T, Time>) {
                w.i32(p.day);
                w.i32(p.timeOfDay);
            } else if constexpr (std::is_same_v<T, Interaction> || std::is_same_v<T, DamageNumber> ||
                                 std::is_same_v<T, SoundRequest> || std::is_same_v<T, BuffNotice> ||
                                 std::is_same_v<T, EffectObject>)
                w.value(p.record);
            else if constexpr (std::is_same_v<T, Chat>) {
                if (direction == Direction::ToClient)
                    w.i64(p.speaker);
                writeText(w, p.text);
            } else if constexpr (std::is_same_v<T, ZoneRequest>)
                w.value(p.zone);
            else if constexpr (std::is_same_v<T, CubeVulkanHello>) {
                w.u32(p.magic);
                w.u32(p.protocol);
                w.u32(p.capabilities);
            } else if constexpr (std::is_same_v<T, CubeVulkanMessage>) {
                w.u32((uint32_t)p.channel.size());
                w.bytes(p.channel.data(), p.channel.size());
                w.u32((uint32_t)p.payload.size());
                w.bytes(p.payload.data(), p.payload.size());
            } else if constexpr (std::is_same_v<T, UnknownPacket>)
                w.bytes(p.body.data(), p.body.size());
            // Join, Leave and EndOfCreatureUpdates have no body.
        },
        packet);
    return out;
}

// --- Decoding --------------------------------------------------------------------------------------

Packet decode(const uint8_t *data, size_t size, Direction direction, DecodeInfo *infoOut) {
    DecodeInfo local;
    DecodeInfo &info = infoOut ? *infoOut : local;
    info = DecodeInfo{};
    ByteReader r(data, size);
    uint32_t id = 0;
    if (!r.value(id)) {
        info.cut = true;
        return UnknownPacket{0, Bytes(data, data + size)};
    }
    Packet result;
    switch ((PacketId)id) {
    case PacketId::Invitation: {
        Invitation p;
        r.value(p.version);
        result = p;
        break;
    }
    case PacketId::Join: result = Join{}; break;
    case PacketId::Leave: result = Leave{}; break;
    case PacketId::CreatureUpdate: {
        CreatureUpdate p;
        if (!readCompressed(r, info.inflated, info))
            info.ignored = true;
        p.delta = decodeDeltaBody(info.inflated.data(), info.inflated.size());
        info.cut = info.cut || p.delta.cut;
        result = std::move(p);
        break;
    }
    case PacketId::YourId: {
        YourId p;
        r.value(p.id);
        result = p;
        break;
    }
    case PacketId::EndOfCreatureUpdates: result = EndOfCreatureUpdates{}; break;
    case PacketId::WorldUpdate: {
        WorldUpdate p;
        readCompressed(r, info.inflated, info);
        bool cut = false;
        p.events = decodeWorldEvents(info.inflated.data(), info.inflated.size(), &cut);
        info.cut = info.cut || cut;
        result = std::move(p);
        break;
    }
    case PacketId::Time: {
        Time p;
        r.value(p.day);
        r.value(p.timeOfDay);
        result = p;
        break;
    }
    case PacketId::Interaction: {
        Interaction p;
        r.value(p.record);
        result = p;
        break;
    }
    case PacketId::DamageNumber: {
        DamageNumber p;
        r.value(p.record);
        result = p;
        break;
    }
    case PacketId::SoundRequest: {
        SoundRequest p;
        r.value(p.record);
        result = p;
        break;
    }
    case PacketId::BuffNotice: {
        BuffNotice p;
        r.value(p.record);
        result = p;
        break;
    }
    case PacketId::EffectObject: {
        EffectObject p;
        r.value(p.record);
        result = p;
        break;
    }
    case PacketId::Chat: {
        Chat p;
        if (direction == Direction::ToClient)
            r.value(p.speaker);
        readText(r, p.text);
        result = std::move(p);
        break;
    }
    case PacketId::ZoneRequest: {
        ZoneRequest p;
        r.value(p.zone);
        result = p;
        break;
    }
    case PacketId::VersionMismatch: {
        VersionMismatch p;
        r.value(p.version);
        result = p;
        break;
    }
    case PacketId::CubeVulkanHello: {
        CubeVulkanHello p;
        p.magic = 0; // a hello too short to carry the magic is no hello
        r.value(p.magic);
        r.value(p.protocol);
        r.value(p.capabilities);
        result = p;
        break;
    }
    case PacketId::CubeVulkanMessage: {
        CubeVulkanMessage p;
        uint32_t n = 0;
        r.value(n);
        n = (uint32_t)std::min<size_t>(n, r.remaining());
        p.channel.assign((const char *)r.here(), n);
        r.skip(n);
        n = 0;
        r.value(n);
        n = (uint32_t)std::min<size_t>(n, r.remaining());
        p.payload.assign(r.here(), r.here() + n);
        r.skip(n);
        result = std::move(p);
        break;
    }
    default: {
        UnknownPacket p;
        p.id = id;
        p.body.assign(r.here(), r.here() + r.remaining());
        r.skip(r.remaining());
        result = std::move(p);
        break;
    }
    }
    info.cut = info.cut || r.wasShort();
    info.trailing = r.remaining();
    return result;
}

} // namespace cwnet
