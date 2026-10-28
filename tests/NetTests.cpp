// Checks of the game-compatible network layer (src/net/) against docs/specs/netcode-*.md: every
// packet encoded and decoded back byte for byte, the reader rule on short packets, the creature
// delta's field mask, a host and a guest talking over the loopback (and over UDP on this
// machine), the session life cycle and its timers, and the recorder file format.
//
// `EngineTests netdecode <file>...` decodes packet recordings of the original game
// (ZoneExtractor's Extract\net\*.bin) and checks that every packet encodes back to its bytes.
#include "Spawned.h"
#include "net/ClientGame.h"
#include "net/HostGame.h"
#include "net/Packets.h"
#include "net/Recording.h"
#include "net/Session.h"
#include "net/SteamTransport.h"
#include "net/UdpTransport.h"

#include <zlib.h>

#include <chrono>
#include <deque>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <string>

using namespace cwnet;

namespace {
int failures = 0;
#define NET_CHECK(...)                                                                             \
    do {                                                                                           \
        if (!(__VA_ARGS__)) {                                                                      \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #__VA_ARGS__);                     \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

// A record with a recognisable byte pattern.
template <size_t N> Record<N> patterned(uint8_t seed) {
    Record<N> r;
    for (size_t i = 0; i < N; i++)
        r[i] = (uint8_t)(seed + i * 7);
    return r;
}

template <class T> void put(uint8_t *at, T v) { std::memcpy(at, &v, sizeof v); }

// A creature snapshot with something in every field.
CreatureSnapshot testCreature(uint8_t seed) {
    CreatureSnapshot s;
    for (size_t i = 0; i < SNAPSHOT_SIZE; i++)
        s.bytes[i] = (uint8_t)(seed * 3 + i);
    // Plain floats where the tests compare floats, so no field holds a NaN by accident.
    for (const CreatureField &f : creatureFields())
        if (f.test == FieldTest::Floats)
            for (size_t i = 0; i < f.size; i += 4)
                put(s.at(f.offset + i), (float)(seed + f.bit + i));
    // The appearance block's floats too (offsets 0x0c..0x17 and 0x28..0xaf within it).
    constexpr size_t APPEARANCE = 0x78;
    for (size_t i = 0x0c; i < 0x18; i += 4)
        put(s.at(APPEARANCE + i), (float)(seed + i));
    for (size_t i = 0x28; i < 0xb0; i += 4)
        put(s.at(APPEARANCE + i), (float)(seed + i));
    put(s.at(0x958), '\0'); // an empty name
    return s;
}

WorldEvents testEvents() {
    WorldEvents e;
    e.blockChanges = {patterned<0x1c>(1), patterned<0x1c>(2)};
    e.damageNumbers = {patterned<0x48>(3)};
    e.particles = {patterned<0x48>(4)};
    e.sounds = {patterned<0x30>(5), patterned<0x30>(6)};
    e.effects = {patterned<0x80>(7)};
    e.staticObjects = {{{3, -4}, {patterned<0xe0>(8), patterned<0xe0>(9)}}};
    e.groundItems = {{{-1, 2}, {patterned<0xe0>(10)}}, {{5, 5}, {}}};
    e.zonesAnswered = {{3, -4}, {7, 8}};
    e.pickups = {patterned<0xd0>(11)};
    e.kills = {patterned<0x10>(12)};
    e.feats = {patterned<0x28>(13)};
    e.aggro = {patterned<0x18>(14)};
    e.buffs = {patterned<0x30>(15)};
    e.sleeps = {0, 3};
    e.calls = {42, -7};
    return e;
}

bool sameEvents(const WorldEvents &a, const WorldEvents &b) { return encodeWorldEvents(a) == encodeWorldEvents(b); }

// --- Packets -------------------------------------------------------------------------------------

void roundTrip(const Packet &p, Direction d) {
    const Bytes bytes = encode(p, d);
    DecodeInfo info;
    const Packet back = decode(bytes, d, &info);
    NET_CHECK(packetId(back) == packetId(p));
    NET_CHECK(!info.cut && info.trailing == 0 && info.streamComplete);
    NET_CHECK(encode(back, d) == bytes);
    NET_CHECK(checkRoundTrip(bytes, d) == RoundTrip::Same);
}

void everyPacket() {
    // Layouts with fixed sizes (id included).
    NET_CHECK(encode(Invitation{GAME_VERSION}, Direction::ToClient).size() == 8);
    NET_CHECK(encode(Join{}, Direction::ToHost).size() == 4);
    NET_CHECK(encode(Leave{}, Direction::ToHost).size() == 4);
    NET_CHECK(encode(YourId{3}, Direction::ToClient).size() == 12);
    NET_CHECK(encode(EndOfCreatureUpdates{}, Direction::ToClient).size() == 4);
    NET_CHECK(encode(Time{}, Direction::ToClient).size() == 12);
    NET_CHECK(encode(Interaction{}, Direction::ToHost).size() == 4 + 200);
    NET_CHECK(encode(DamageNumber{}, Direction::ToHost).size() == 4 + 72);
    NET_CHECK(encode(SoundRequest{}, Direction::ToHost).size() == 4 + 48);
    NET_CHECK(encode(BuffNotice{}, Direction::ToHost).size() == 4 + 48);
    NET_CHECK(encode(EffectObject{}, Direction::ToHost).size() == 4 + 128);
    NET_CHECK(encode(ZoneRequest{{1, 2}}, Direction::ToHost).size() == 12);
    NET_CHECK(encode(VersionMismatch{}, Direction::ToHost).size() == 8);
    {
        const Bytes b = encode(Invitation{GAME_VERSION}, Direction::ToClient);
        const uint8_t expected[8] = {0, 0, 0, 0, 0xa1, 0x86, 0x01, 0x00}; // 0, then 100001
        NET_CHECK(std::memcmp(b.data(), expected, 8) == 0);
    }
    {
        // Chat: client -> host is count and text; host -> client puts the speaker first.
        const Chat line{7, u"hi"};
        const Bytes up = encode(line, Direction::ToHost), down = encode(line, Direction::ToClient);
        const uint8_t upExpected[12] = {14, 0, 0, 0, 2, 0, 0, 0, 'h', 0, 'i', 0};
        NET_CHECK(up.size() == 12 && std::memcmp(up.data(), upExpected, 12) == 0);
        NET_CHECK(down.size() == 20 && down[4] == 7 && down[12] == 2);
    }
    {
        // Zone key: zone x then zone y.
        const Bytes b = encode(ZoneRequest{{-2, 5}}, Direction::ToHost);
        NET_CHECK(b[4] == 0xfe && b[7] == 0xff && b[8] == 5);
    }

    Interaction interaction;
    interaction.record = patterned<0xc8>(21);
    DamageNumber damage;
    damage.record = patterned<0x48>(22);
    SoundRequest sound;
    sound.record = patterned<0x30>(23);
    BuffNotice buff;
    buff.record = patterned<0x30>(24);
    EffectObject effectObject;
    effectObject.record = patterned<0x80>(25);
    const CreatureSnapshot a = testCreature(1);

    roundTrip(Invitation{GAME_VERSION}, Direction::ToClient);
    roundTrip(Join{}, Direction::ToHost);
    roundTrip(Leave{}, Direction::ToHost);
    roundTrip(Leave{}, Direction::ToClient);
    roundTrip(CreatureUpdate{makeDelta(5, a, nullptr)}, Direction::ToClient);
    roundTrip(CreatureUpdate{makeDelta(5, a, &a)}, Direction::ToHost);
    roundTrip(YourId{4}, Direction::ToClient);
    roundTrip(EndOfCreatureUpdates{}, Direction::ToClient);
    roundTrip(WorldUpdate{testEvents()}, Direction::ToClient);
    roundTrip(WorldUpdate{}, Direction::ToClient);
    roundTrip(Time{12, 43200000}, Direction::ToClient);
    roundTrip(interaction, Direction::ToHost);
    roundTrip(damage, Direction::ToHost);
    roundTrip(sound, Direction::ToHost);
    roundTrip(buff, Direction::ToHost);
    roundTrip(effectObject, Direction::ToHost);
    roundTrip(Chat{0, u"hello there"}, Direction::ToHost);
    roundTrip(Chat{3, u"été"}, Direction::ToClient);
    roundTrip(ZoneRequest{{-100, 200}}, Direction::ToHost);
    roundTrip(VersionMismatch{GAME_VERSION}, Direction::ToHost);
    roundTrip(CubeVulkanHello{}, Direction::ToHost);
    roundTrip(CubeVulkanMessage{"mod.example", {1, 2, 3}}, Direction::ToClient);
    roundTrip(UnknownPacket{77, {9, 9}}, Direction::ToClient);

    // The decoded contents, not just the bytes.
    {
        const WorldEvents e = testEvents();
        const Packet back = decode(encode(WorldUpdate{e}, Direction::ToClient), Direction::ToClient);
        NET_CHECK(sameEvents(std::get<WorldUpdate>(back).events, e));
        NET_CHECK(std::get<WorldUpdate>(back).events.groundItems.size() == 2);
        NET_CHECK(std::get<WorldUpdate>(back).events.staticObjects[0].key == (ZoneKey{3, -4}));
        const Packet line = decode(encode(Chat{9, u"abc"}, Direction::ToClient), Direction::ToClient);
        NET_CHECK(std::get<Chat>(line).speaker == 9 && std::get<Chat>(line).text == u"abc");
        const Packet t = decode(encode(Time{2, 1000}, Direction::ToClient), Direction::ToClient);
        NET_CHECK(std::get<Time>(t).day == 2 && std::get<Time>(t).timeOfDay == 1000);
    }
    // Packet 7's body: fifteen counts when empty.
    NET_CHECK(encodeWorldEvents(WorldEvents{}).size() == 15 * 4);
}

// The reader rule: short packets keep defaults, extra bytes are ignored.
void readerRule() {
    {
        Bytes b = encode(Time{5, 1234}, Direction::ToClient);
        b.resize(8 + 2); // the day, then half of the time
        DecodeInfo info;
        const Time t = std::get<Time>(decode(b, Direction::ToClient, &info));
        NET_CHECK(t.day == 5 && t.timeOfDay == 0 && info.cut);
        NET_CHECK(checkRoundTrip(b, Direction::ToClient) == RoundTrip::Cut);
    }
    {
        // A short sound request keeps pitch and volume 1.0.
        Bytes b = encode(SoundRequest{}, Direction::ToHost);
        b.resize(4 + 0x10);
        const SoundRecord r = std::get<SoundRequest>(decode(b, Direction::ToHost)).record;
        NET_CHECK(field<float>(r, sound::PITCH) == 1.0f && field<float>(r, sound::VOLUME) == 1.0f);
        // A short effect object keeps the constructor's values.
        Bytes e = encode(EffectObject{}, Direction::ToHost);
        e.resize(4);
        const EffectRecord er = std::get<EffectObject>(decode(e, Direction::ToHost)).record;
        NET_CHECK(field<int64_t>(er, effect::OWNER) == -1 && field<int32_t>(er, effect::LIFETIME) == 5000);
        // A short interaction keeps zone and index -1 and the 16-bit field 1.
        Bytes i = encode(Interaction{}, Direction::ToHost);
        i.resize(4 + 0xa0);
        const InteractionRecord ir = std::get<Interaction>(decode(i, Direction::ToHost)).record;
        NET_CHECK(field<int32_t>(ir, interaction::ZONE_X) == -1 && field<int32_t>(ir, interaction::INDEX) == -1);
        NET_CHECK(field<int16_t>(ir, interaction::MARKER) == 1);
    }
    {
        // An invitation with no version is a mismatch (version 0).
        const Bytes b = {0, 0, 0, 0};
        NET_CHECK(std::get<Invitation>(decode(b, Direction::ToClient)).version == 0);
    }
    {
        // Trailing bytes are ignored and reported.
        Bytes b = encode(YourId{2}, Direction::ToClient);
        b.push_back(0x55);
        DecodeInfo info;
        NET_CHECK(std::get<YourId>(decode(b, Direction::ToClient, &info)).id == 2 && info.trailing == 1);
        NET_CHECK(checkRoundTrip(b, Direction::ToClient) == RoundTrip::Trailing);
    }
    {
        // Packet 3 with a length of 0 is ignored.
        const Bytes b = {3, 0, 0, 0, 0, 0, 0, 0};
        DecodeInfo info;
        decode(b, Direction::ToHost, &info);
        NET_CHECK(info.ignored);
    }
    {
        // A list count larger than what follows stops at the end.
        Bytes body;
        ByteWriter w(body);
        w.i32(1000000);
        bool cut = false;
        const WorldEvents e = decodeWorldEvents(body.data(), body.size(), &cut);
        NET_CHECK(e.blockChanges.empty() && cut);
    }
    {
        // Fewer than four bytes: no id at all.
        const Bytes b = {1, 0};
        DecodeInfo info;
        decode(b, Direction::ToHost, &info);
        NET_CHECK(info.cut);
    }
    {
        // A stored (uncompressed) zlib stream is as good as a compressed one (open question 10 of
        // netcode-compatibility.md is about the game; our reader takes any valid stream).
        const Bytes body = encodeDeltaBody(makeDelta(8, testCreature(2), nullptr));
        Bytes stored(body.size() + 64);
        uLongf size = (uLongf)stored.size();
        NET_CHECK(compress2(stored.data(), &size, body.data(), (uLong)body.size(), 0) == Z_OK);
        stored.resize(size);
        Bytes packet;
        ByteWriter w(packet);
        w.u32(3);
        w.i32((int32_t)stored.size());
        w.bytes(stored.data(), stored.size());
        const Packet p = decode(packet, Direction::ToClient);
        NET_CHECK(encodeDeltaBody(std::get<CreatureUpdate>(p).delta) == body);
        NET_CHECK(checkRoundTrip(packet, Direction::ToClient) == RoundTrip::SameBody);
    }
}

// --- The creature delta ----------------------------------------------------------------------------

constexpr size_t ITEM_SIZE = 0xa0;
constexpr size_t ITEM_SUBTYPE = 0x04, ITEM_ATTRIBUTES = 0x1d, ITEM_ATTRIBUTE_COUNT = 0x9d;

void creatureDelta() {
    // The table: 49 fields in bit order, inside the snapshot, sizes as the spec lists.
    size_t total = 0;
    for (int i = 0; i < CREATURE_FIELD_COUNT; i++) {
        const CreatureField &f = creatureFields()[i];
        NET_CHECK(f.bit == i);
        NET_CHECK(f.offset >= SNAPSHOT_BEGIN && f.offset + f.size <= SNAPSHOT_END);
        total += f.size;
    }
    NET_CHECK(fieldBytes(ALL_CREATURE_FIELDS) == total);
    NET_CHECK(creatureFields()[13].size == 0xb0 && creatureFields()[43].size == 0x6e0);
    NET_CHECK(creatureFields()[40].offset == 0x150 && creatureFields()[39].offset == 0x1b0);

    const CreatureSnapshot a = testCreature(1);
    // Nothing changed: mask 0 and a 16-byte body (id and mask), still sent.
    {
        const CreatureDelta d = makeDelta(9, a, &a);
        NET_CHECK(d.mask == 0 && d.fields.empty() && encodeDeltaBody(d).size() == 16);
    }
    // A full update: bits 0..48.
    {
        const CreatureDelta d = makeDelta(9, a, nullptr);
        NET_CHECK(d.mask == ALL_CREATURE_FIELDS && d.fields.size() == total);
        CreatureSnapshot b;
        applyDelta(d, b);
        NET_CHECK(changedFields(b, a) == 0);
        // Bytes the table does not cover (block power at +0x184) stay the receiver's.
        NET_CHECK(*b.at(0x184) == 0);
    }
    // One field: the position.
    {
        CreatureSnapshot b = a;
        put(b.at(0x18), (int64_t)123456789);
        const CreatureDelta d = makeDelta(9, b, &a);
        NET_CHECK(d.mask == 1 && d.fields.size() == 24);
        CreatureSnapshot c = a;
        applyDelta(d, c);
        NET_CHECK(c.bytes == b.bytes);
    }
    // Floats: +0 equals -0; a NaN always counts as changed.
    {
        CreatureSnapshot b = a, c = a;
        put(b.at(0x180), 0.0f);
        put(c.at(0x180), -0.0f);
        NET_CHECK(changedFields(c, b) == 0);
        put(b.at(0x180), std::numeric_limits<float>::quiet_NaN());
        put(c.at(0x180), std::numeric_limits<float>::quiet_NaN());
        NET_CHECK(changedFields(c, b) == 1ull << 29 && changedFields(b, b) == 1ull << 29);
        // The contact flags are compared as a float: two NaN patterns differ every time.
        CreatureSnapshot n = a;
        put(n.at(0x5c), (uint32_t)0x7fc00001);
        NET_CHECK(changedFields(n, n) == 1ull << 6);
    }
    // Appearance: bytes 5..7 alone never count; byte 2 does; a model number does.
    {
        CreatureSnapshot b = a;
        b.at(0x78)[5] ^= 0xff;
        b.at(0x78)[7] ^= 0xff;
        NET_CHECK(changedFields(b, a) == 0);
        b.at(0x78)[2] ^= 1;
        NET_CHECK(changedFields(b, a) == 1ull << 13);
        CreatureSnapshot c = a;
        c.at(0x78)[0x1a] ^= 1;
        NET_CHECK(changedFields(c, a) == 1ull << 13);
        // When sent, the whole block goes, untested bytes included.
        const CreatureDelta d = makeDelta(1, b, &a);
        NET_CHECK(d.fields.size() == 0xb0 && d.fields[5] == b.at(0x78)[5]);
    }
    // Name: only up to the terminator.
    {
        CreatureSnapshot b = a, c = a;
        std::memcpy(b.at(0x958), "Bob\0xxxxxxxxxxxx", 16);
        std::memcpy(c.at(0x958), "Bob\0yyyyyyyyyyyy", 16);
        NET_CHECK(changedFields(c, b) == 0);
        c.at(0x958)[2] = 'x';
        NET_CHECK(changedFields(c, b) == 1ull << 44);
    }
    // Items: equality ignores padding and attributes past the count.
    {
        CreatureSnapshot b = a;
        uint8_t *item = b.at(0x1d8);
        std::memset(item, 0, ITEM_SIZE);
        item[0] = 3;
        item[ITEM_ATTRIBUTE_COUNT] = 1;
        CreatureSnapshot c = b;
        c.at(0x1d8)[0x02] = 0x77;                    // padding
        c.at(0x1d8)[ITEM_ATTRIBUTES + 4] = 0x55;     // the second attribute, past the count
        NET_CHECK(changedFields(c, b) == 0);
        c.at(0x1d8)[ITEM_SUBTYPE] = 9;
        NET_CHECK(changedFields(c, b) == 1ull << 42);
        // Equipment: any of the eleven slots.
        CreatureSnapshot e = b;
        std::memset(e.at(0x278), 0, 11 * ITEM_SIZE);
        CreatureSnapshot f = e;
        f.at(0x278 + 10 * ITEM_SIZE)[0] = 1;
        NET_CHECK(changedFields(f, e) == 1ull << 43);
    }
    // Wire order: bit 39 (+0x1b0) before bit 40 (+0x150).
    {
        CreatureSnapshot b = a;
        put(b.at(0x150), (int32_t)0x11111111);
        put(b.at(0x1b0), (int32_t)0x22222222);
        const CreatureDelta d = makeDelta(1, b, &a);
        NET_CHECK(d.mask == ((1ull << 39) | (1ull << 40)) && d.fields.size() == 16);
        NET_CHECK(d.fields[0] == 0x22 && d.fields[8] == 0x11);
    }
    // Several fields, applied to a whole record.
    {
        CreatureSnapshot b = a;
        put(b.at(0x144), (int32_t)50); // level
        b.at(0x14c)[0] = 2;            // class
        put(b.at(0x188), 0.5f);        // mana
        const CreatureDelta d = makeDelta(1, b, &a);
        NET_CHECK(d.mask == ((1ull << 21) | (1ull << 23) | (1ull << 30)) && d.fields.size() == 9);
        std::vector<uint8_t> record(0x15f8, 0);
        a.writeTo(record.data());
        applyDelta(d, record.data());
        NET_CHECK(CreatureSnapshot::of(record.data()).bytes == b.bytes);
    }
    // A cut body keeps the fields that came whole.
    {
        CreatureSnapshot b = a;
        put(b.at(0x10), (int64_t)1);
        put(b.at(0x180), 99.0f);
        Bytes body = encodeDeltaBody(makeDelta(1, b, &a));
        body.resize(16 + 24 + 2);
        const CreatureDelta d = decodeDeltaBody(body.data(), body.size());
        NET_CHECK(d.cut && d.fields.size() == 24);
        CreatureSnapshot c = a;
        applyDelta(d, c);
        NET_CHECK(*(int64_t *)c.at(0x10) == 1 && std::memcmp(c.at(0x180), a.at(0x180), 4) == 0);
    }
}

// --- Sessions over the loopback ----------------------------------------------------------------------

// A stand-in for a host world: which creature ids exist and their records.
struct TestWorld {
    std::map<int64_t, CreatureSnapshot> creatures;
    HostLink::World links() {
        return {[this](int64_t id) { return creatures.count(id) != 0; }, [this](int64_t id) { creatures.erase(id); }};
    }
};

constexpr PeerId HOST_MACHINE = 0x0110000100000001ull, GUEST_MACHINE = 0x0110000100000002ull;

void loopbackSession() {
    LoopbackNetwork network;
    auto hostEnd = network.endpoint(HOST_MACHINE), guestEnd = network.endpoint(GUEST_MACHINE);
    TestWorld hostWorld;
    HostLink host(*hostEnd, hostWorld.links());
    int resets = 0;
    GuestLink guest(*guestEnd, {[&] { resets++; }, nullptr});
    uint32_t now = 1000;

    // Invitation: shown once, then refreshed.
    host.invite(GUEST_MACHINE, now);
    NET_CHECK(host.isInvited(GUEST_MACHINE));
    GuestLink::Inbox in;
    guest.receive(now, in);
    NET_CHECK(in.invitations.size() == 1 && guest.hasInvitation(HOST_MACHINE));
    // Join: the host answers with id 1, the lowest free.
    guest.connectTo(HOST_MACHINE, now);
    NET_CHECK(resets == 1 && guest.isGuestElsewhere());
    HostLink::Inbox hin;
    host.receive(hin);
    NET_CHECK(hin.joined.size() == 1 && host.isConnected(GUEST_MACHINE));
    NET_CHECK(host.connection(GUEST_MACHINE)->creatureId == 1);
    in = {};
    guest.receive(now, in);
    NET_CHECK(in.givenId && *in.givenId == 1 && guest.localId() == 1);
    NET_CHECK(!guest.sendExtra({"x", {}})); // no extras before both said hello

    // The guest's creature update: whole the first time.
    const CreatureSnapshot me = testCreature(5);
    guest.outbox.chat.push_back(u"hello");
    guest.outbox.damageNumbers.push_back(patterned<0x48>(1));
    guest.outbox.neededZones = {{1, 1}, {2, 1}};
    NET_CHECK(guest.sendTick(now, me));
    NET_CHECK(!guest.sendTick(now + 32, me)); // not within 32 ms
    hin = {};
    host.receive(hin);
    NET_CHECK(hin.creatureUpdates.size() == 1 && hin.creatureUpdates[0].id == 1);
    NET_CHECK(hin.creatureUpdates[0].mask == ALL_CREATURE_FIELDS);
    NET_CHECK(hin.damageNumbers.size() == 1);
    HostLink::Connection &conn = *host.connection(GUEST_MACHINE);
    NET_CHECK(conn.requestedZones.size() == 2 && conn.chat.size() == 1 && conn.chat[0].text == u"hello");
    NET_CHECK(conn.cubeVulkan); // its hello came with the update
    applyDelta(hin.creatureUpdates[0], hostWorld.creatures[1]);
    NET_CHECK(changedFields(hostWorld.creatures[1], me) == 0);

    // The host's tick to the guest: a monster and the guest itself, then 6, 8, 7, 14.
    CreatureSnapshot monster = testCreature(9);
    hostWorld.creatures[50] = monster;
    conn.waiting = testEvents();
    host.sendTick(conn, hostWorld.creatures, Time{3, 5000});
    NET_CHECK(network.waiting(GUEST_MACHINE, Channel::ToClient) == 7); // the host's hello, then 3, 3, 6, 8, 7, 14
    in = {};
    guest.receive(now, in);
    NET_CHECK(guest.hostIsCubeVulkan());
    NET_CHECK(in.creatureUpdates.size() == 1 && in.creatureUpdates[0].id == 50); // never its own
    NET_CHECK(in.creatureUpdates[0].mask == ALL_CREATURE_FIELDS);
    NET_CHECK(in.creatureLists.size() == 1 && in.creatureLists[0] == std::set<int64_t>{50});
    NET_CHECK(in.times.size() == 1 && in.times[0].day == 3 && in.times[0].timeOfDay == 5000);
    NET_CHECK(in.worldUpdates.size() == 1 && sameEvents(in.worldUpdates[0], testEvents()));
    NET_CHECK(in.chat.size() == 1 && in.chat[0].speaker == 1 && in.chat[0].text == u"hello");
    NET_CHECK(conn.waiting.empty() && conn.known.size() == 2);

    // The next tick: only what changed travels.
    put(monster.at(0x10), (int64_t)777);
    hostWorld.creatures[50] = monster;
    host.sendTick(conn, hostWorld.creatures, Time{3, 5032});
    in = {};
    guest.receive(now + 40, in);
    NET_CHECK(in.creatureUpdates.size() == 1 && in.creatureUpdates[0].mask == 1);
    NET_CHECK(in.worldUpdates.size() == 1 && in.worldUpdates[0].empty());
    // The world events held a world reset (a sleep notice of 0), so the guest's next update is
    // whole again; the one after is a delta. The zone requests wait a second.
    NET_CHECK(guest.sendTick(now + 40, me));
    hin = {};
    host.receive(hin);
    NET_CHECK(hin.creatureUpdates.size() == 1 && hin.creatureUpdates[0].mask == ALL_CREATURE_FIELDS);
    CreatureSnapshot moved = me;
    put(moved.at(0x34), 2.5f);
    NET_CHECK(guest.sendTick(now + 80, moved));
    hin = {};
    host.receive(hin);
    NET_CHECK(hin.creatureUpdates.size() == 1 && hin.creatureUpdates[0].mask == 1ull << 2);
    NET_CHECK(conn.requestedZones.size() == 2);

    // Extras between two CubeVulkan peers.
    NET_CHECK(guest.sendExtra({"mod.example", {1, 2}}));
    hin = {};
    host.receive(hin);
    NET_CHECK(hin.extras.size() == 1 && hin.extras[0].second.channel == "mod.example");
    NET_CHECK(host.sendExtra(GUEST_MACHINE, {"engine.view", {9}}));
    in = {};
    guest.receive(now + 50, in);
    NET_CHECK(in.extras.size() == 1 && in.extras[0].payload == Bytes{9});

    // A world reset: the guest forgets nothing on its own, the host re-sends everything whole.
    host.forgetKnownCreatures();
    host.sendTick(conn, hostWorld.creatures, Time{4, 0});
    in = {};
    guest.receive(now + 60, in);
    NET_CHECK(in.creatureUpdates.size() == 1 && in.creatureUpdates[0].mask == ALL_CREATURE_FIELDS);

    // Packet 6 removes what was not heard: the monster gone from view.
    hostWorld.creatures.erase(50);
    host.sendTick(conn, hostWorld.creatures, Time{4, 32});
    in = {};
    guest.receive(now + 70, in);
    NET_CHECK(in.creatureUpdates.empty() && in.creatureLists.size() == 1 && in.creatureLists[0].empty());

    // Kick: the guest's creature goes from the host world and the guest goes home.
    host.kick(GUEST_MACHINE);
    NET_CHECK(!host.isConnected(GUEST_MACHINE) && !host.isInvited(GUEST_MACHINE) && !hostWorld.creatures.count(1));
    in = {};
    guest.receive(now + 80, in);
    NET_CHECK(in.wentHome && guest.host() == GUEST_MACHINE && !guest.hasInvitation(HOST_MACHINE));
}

void sessionTimers() {
    LoopbackNetwork network;
    auto hostEnd = network.endpoint(HOST_MACHINE), guestEnd = network.endpoint(GUEST_MACHINE);
    TestWorld hostWorld;
    HostLink host(*hostEnd, hostWorld.links());
    GuestLink guest(*guestEnd, {});
    // Re-invitation after more than 5 s, not at exactly 5 s.
    host.invite(GUEST_MACHINE, 0);
    host.reinvite(5000);
    NET_CHECK(network.waiting(GUEST_MACHINE, Channel::ToClient) == 1);
    host.reinvite(5001);
    NET_CHECK(network.waiting(GUEST_MACHINE, Channel::ToClient) == 2);
    GuestLink::Inbox in;
    guest.receive(5001, in);
    NET_CHECK(in.invitations.size() == 1); // shown once
    // An invitation lives 6 s from its last renewal.
    guest.checkTimers(11001);
    NET_CHECK(guest.hasInvitation(HOST_MACHINE));
    guest.checkTimers(11002);
    NET_CHECK(!guest.hasInvitation(HOST_MACHINE));

    // Host silence: more than 5 s without a packet sends the guest home.
    guest.connectTo(HOST_MACHINE, 20000);
    NET_CHECK(!guest.checkTimers(25000));
    NET_CHECK(guest.checkTimers(25001) && guest.host() == GUEST_MACHINE);

    // A version mismatch stops the re-invitations.
    LoopbackNetwork n2;
    auto h2 = n2.endpoint(HOST_MACHINE), g2 = n2.endpoint(GUEST_MACHINE);
    HostLink host2(*h2, hostWorld.links());
    GuestLink guest2(*g2, {});
    host2.invite(GUEST_MACHINE, 0);
    Received drop;
    g2->receive(Channel::ToClient, drop);
    h2->send(GUEST_MACHINE, Channel::ToClient, encode(Invitation{99999}, Direction::ToClient));
    in = {};
    guest2.receive(10, in);
    NET_CHECK(in.invitations.empty() && !guest2.hasInvitation(HOST_MACHINE));
    HostLink::Inbox hin;
    host2.receive(hin);
    NET_CHECK(!host2.isInvited(GUEST_MACHINE));

    // Ten players: an eleventh join gets no answer.
    LoopbackNetwork n3;
    auto h3 = n3.endpoint(HOST_MACHINE), g3 = n3.endpoint(GUEST_MACHINE);
    TestWorld full;
    for (int64_t id = 1; id <= 10; id++)
        full.creatures[id] = {};
    HostLink host3(*h3, full.links());
    GuestLink guest3(*g3, {});
    guest3.connectTo(HOST_MACHINE, 0);
    hin = {};
    host3.receive(hin);
    NET_CHECK(hin.joined.empty() && n3.waiting(GUEST_MACHINE, Channel::ToClient) == 0);
    // A free id in the middle is taken first.
    full.creatures.erase(4);
    guest3.connectTo(HOST_MACHINE, 1);
    hin = {};
    host3.receive(hin);
    NET_CHECK(host3.connection(GUEST_MACHINE) && host3.connection(GUEST_MACHINE)->creatureId == 4);
    // A packet 3 about another creature than its own is dropped.
    g3->send(HOST_MACHINE, Channel::ToHost, encode(CreatureUpdate{makeDelta(7, testCreature(1), nullptr)}, Direction::ToHost));
    hin = {};
    host3.receive(hin);
    NET_CHECK(hin.creatureUpdates.empty());
    // Packets from strangers: only join, leave and mismatch are taken; the rest is logged.
    auto stranger = n3.endpoint(0x0110000100000099ull);
    stranger->send(HOST_MACHINE, Channel::ToHost, encode(Chat{0, u"spam"}, Direction::ToHost));
    hin = {};
    host3.receive(hin);
    NET_CHECK(hin.log.size() == 1 && hin.log[0] == "MESSAGE??? 14");
    // A guest ignores everything but invitations and leaves from anyone but its host.
    stranger->send(GUEST_MACHINE, Channel::ToClient, encode(Time{9, 9}, Direction::ToClient));
    in = {};
    guest3.receive(2, in);
    NET_CHECK(in.times.empty());
}

// Single player: the client and its own host on one machine, through the loopback.
void singlePlayer() {
    LoopbackNetwork network;
    auto machine = network.endpoint(HOST_MACHINE);
    TestWorld world;
    HostLink host(*machine, world.links());
    GuestLink client(*machine, {nullptr, [&] { host.dropAll(); }});
    client.goHome(0);
    HostLink::Inbox hin;
    host.receive(hin);
    NET_CHECK(host.connection(HOST_MACHINE) && host.connection(HOST_MACHINE)->creatureId == 1);
    GuestLink::Inbox in;
    client.receive(0, in);
    NET_CHECK(client.localId() == 1 && !client.isGuestElsewhere());
    client.sendTick(0, testCreature(3));
    hin = {};
    host.receive(hin);
    NET_CHECK(hin.creatureUpdates.size() == 1);
}

// --- UDP on this machine ------------------------------------------------------------------------------

void udpTransport() {
    UdpTransport server, client;
    std::string error;
    if (!server.open(0, error) || !client.open(0, error)) {
        std::printf("net: UDP skipped (%s)\n", error.c_str());
        return;
    }
    const PeerId serverAddress = UdpTransport::resolve("127.0.0.1", server.port());
    NET_CHECK(serverAddress != 0 && UdpTransport::describe(serverAddress) == "127.0.0.1:" + std::to_string(server.port()));
    client.loss = 0.3f;
    server.loss = 0.3f;
    // A large packet (several fragments) and a run of small ones, in order despite the losses.
    Bytes big(5000);
    for (size_t i = 0; i < big.size(); i++)
        big[i] = (uint8_t)(i * 13);
    NET_CHECK(client.send(serverAddress, Channel::ToHost, big));
    for (uint8_t i = 0; i < 20; i++)
        client.send(serverAddress, Channel::ToHost, Bytes{i});
    std::vector<Bytes> got;
    PeerId clientAddress = 0;
    const auto start = std::chrono::steady_clock::now();
    uint32_t clock = 0;
    while (got.size() < 21 && std::chrono::steady_clock::now() - start < std::chrono::seconds(10)) {
        clock += 50;
        client.pump(clock);
        server.pump(clock);
        Received r;
        while (server.receive(Channel::ToHost, r)) {
            clientAddress = r.from;
            got.push_back(r.bytes);
        }
    }
    NET_CHECK(got.size() == 21);
    if (got.size() == 21) {
        NET_CHECK(got[0] == big);
        for (uint8_t i = 0; i < 20; i++)
            NET_CHECK(got[1 + i] == Bytes{i});
    }
    // And back, on the other channel, through a whole session.
    client.loss = server.loss = 0;
    TestWorld world;
    HostLink host(server, world.links());
    GuestLink guest(client, {});
    guest.connectTo(serverAddress, 0);
    for (int i = 0; i < 100 && !guest.localId(); i++) {
        client.pump(clock += 10);
        server.pump(clock);
        HostLink::Inbox hin;
        host.receive(hin);
        GuestLink::Inbox in;
        guest.receive(clock, in);
    }
    NET_CHECK(guest.localId() == 1 && host.connection(clientAddress));
}

// --- Recordings ---------------------------------------------------------------------------------------

void recordings() {
    std::vector<RecordedPacket> packets;
    auto add = [&](RecordedSide side, uint8_t channel, const Packet &p, Direction d) {
        RecordedPacket r;
        r.timeMs = (uint32_t)packets.size() * 16;
        r.side = side;
        r.channel = channel;
        r.peer = 1;
        r.bytes = encode(p, d);
        packets.push_back(r);
    };
    add(RecordedSide::HostReceived, 1, Join{}, Direction::ToHost);
    add(RecordedSide::ClientReceived, 0xff, YourId{1}, Direction::ToClient);
    add(RecordedSide::ClientReceived, 0xff, WorldUpdate{testEvents()}, Direction::ToClient);
    add(RecordedSide::Sent, 1, Chat{0, u"x"}, Direction::ToHost);
    add(RecordedSide::Sent, 0, Chat{2, u"x"}, Direction::ToClient);
    const auto file = std::filesystem::temp_directory_path() / "cubevulkan-nettest.bin";
    {
        const Bytes data = writeRecording(packets);
        std::ofstream(file, std::ios::binary).write((const char *)data.data(), (std::streamsize)data.size());
    }
    std::vector<RecordedPacket> back;
    std::string error;
    NET_CHECK(readRecording(file, back, error));
    std::filesystem::remove(file);
    NET_CHECK(back.size() == packets.size());
    for (size_t i = 0; i < back.size() && i < packets.size(); i++) {
        NET_CHECK(back[i].bytes == packets[i].bytes && back[i].side == packets[i].side);
        NET_CHECK(checkRoundTrip(back[i].bytes, back[i].direction()) == RoundTrip::Same);
    }
    NET_CHECK(back.size() == 5 && back[3].direction() == Direction::ToHost && back[4].direction() == Direction::ToClient);
}

void steamIds() {
    NET_CHECK(validSteamId(0x0110000100000001ull));  // public universe, individual, desktop instance
    NET_CHECK(!validSteamId(0x0110000100000000ull)); // account 0
    NET_CHECK(!validSteamId(0x0110000500000001ull)); // individual with instance 5
    NET_CHECK(!validSteamId(0x0010000100000001ull)); // universe 0
    NET_CHECK(validSteamId(0x0170000000000001ull));  // a chat id, instance 0
    NET_CHECK(!validSteamId(0));
}

} // namespace


// --- The host tick and the client's frame (net/HostGame.h, net/ClientGame.h) --------------------

// A host world of bare creature records, as the host tick drives it.
struct FakeHostWorld : HostWorld {
    std::vector<std::pair<int64_t, std::vector<uint8_t>>> list; // world order
    WorldEvents tickEvents;
    GuestEvents caused;
    std::vector<std::pair<int64_t, InteractionRecord>> interactions;
    std::map<std::pair<int32_t, int32_t>, ZoneAnswer> zones;
    Time time{3, 36000000};
    uint32_t advanced = 0;

    std::vector<uint8_t> *find(int64_t id) {
        for (auto &[i, r] : list)
            if (i == id)
                return &r;
        return nullptr;
    }
    void add(int64_t id, uint8_t hostility, double blockX, double blockY, uint32_t flags = 0) {
        std::vector<uint8_t> r(0x15f8, 0);
        put(&r[creature::ID], id);
        put(&r[creature::POSITION], (int64_t)(blockX * 65536.0));
        put(&r[creature::POSITION + 8], (int64_t)(blockY * 65536.0));
        r[creature::HOSTILITY] = hostility;
        put(&r[creature::FLAGS], flags);
        put(&r[creature::HEALTH], 100.0f);
        list.push_back({id, std::move(r)});
    }
    void moveTo(int64_t id, double blockX, double blockY) {
        std::vector<uint8_t> &r = *find(id);
        put(&r[creature::POSITION], (int64_t)(blockX * 65536.0));
        put(&r[creature::POSITION + 8], (int64_t)(blockY * 65536.0));
    }

    void creatures(std::vector<CreatureView> &out) override {
        out.clear();
        for (auto &[id, r] : list)
            out.push_back({id, r.data()});
    }
    bool hasCreature(int64_t id) override { return find(id) != nullptr; }
    void receiveCreature(const CreatureDelta &delta) override {
        if (!find(delta.id))
            list.push_back({delta.id, std::vector<uint8_t>(0x15f8, 0)});
        applyDelta(delta, find(delta.id)->data());
    }
    void removeCreature(int64_t id) override {
        for (auto it = list.begin(); it != list.end(); ++it)
            if (it->first == id) {
                list.erase(it);
                return;
            }
    }
    void queueInteraction(int64_t id, const InteractionRecord &r) override { interactions.push_back({id, r}); }
    void applyGuestEvents(const GuestEvents &e) override {
        caused.damageNumbers.insert(caused.damageNumbers.end(), e.damageNumbers.begin(), e.damageNumbers.end());
        caused.sounds.insert(caused.sounds.end(), e.sounds.begin(), e.sounds.end());
    }
    void advance(uint32_t dtMs) override { advanced += dtMs, tickEvents.append(pending), pending = {}; }
    WorldEvents &events() override { return tickEvents; }
    bool zoneAnswer(const ZoneKey &z, ZoneAnswer &out) override {
        auto it = zones.find({z.x, z.y});
        if (it == zones.end())
            return false;
        out = it->second;
        return true;
    }
    Time clock() override { return time; }
    WorldEvents pending; // what the next world step makes
};

// A client world that keeps what it is told.
struct FakeMirrorWorld : MirrorWorld {
    int64_t local = 0;
    std::vector<uint8_t> player = std::vector<uint8_t>(0x15f8, 0);
    std::map<int64_t, std::vector<uint8_t>> others;
    std::map<int64_t, uint64_t> lastMask;
    int32_t worldDay = 0, timeOfDay = 0, dayChanges = 0, startedOver = 0;
    std::vector<ZoneKey> needed, answered;
    std::vector<ZoneObjects> groundItems;
    std::vector<DamageRecord> shown, hitsOnMe;
    WorldEvents kept, own;
    std::deque<InteractionRecord> pending;

    void setLocalId(int64_t id) override { local = id; }
    bool localSnapshot(CreatureSnapshot &out) override {
        out = CreatureSnapshot::of(player.data());
        return true;
    }
    bool takeInteraction(InteractionRecord &out) override {
        if (pending.empty())
            return false;
        out = pending.front();
        pending.pop_front();
        return true;
    }
    void receiveCreature(const CreatureDelta &d) override {
        auto [it, made] = others.try_emplace(d.id, std::vector<uint8_t>(0x15f8, 0));
        applyDelta(d, it->second.data());
        lastMask[d.id] = d.mask;
    }
    void keepOnly(const std::set<int64_t> &heard) override {
        for (auto it = others.begin(); it != others.end();)
            it = heard.count(it->first) ? std::next(it) : others.erase(it);
    }
    void startOver() override { others.clear(), startedOver++; }
    int32_t day() override { return worldDay; }
    void dayChange(int32_t) override { dayChanges++; }
    void setClock(int32_t d, int32_t t) override { worldDay = d, timeOfDay = t; }
    void zoneAnswered(const ZoneKey &z) override {
        answered.push_back(z);
        needed.clear();
    }
    void replaceGroundItems(const ZoneObjects &z) override { groundItems.push_back(z); }
    void keepEvents(const WorldEvents &e) override { kept.append(e); }
    bool hasCreature(int64_t id) override { return id == local || others.count(id); }
    void hitLocalPlayer(const DamageRecord &d) override { hitsOnMe.push_back(d); }
    void showDamageNumber(const DamageRecord &d) override { shown.push_back(d); }
    const WorldEvents &ownEvents() override { return own; }
    std::vector<ZoneKey> neededZones() override { return needed; }
};

template <size_t N> Record<N> eventAt(double blockX, double blockY) {
    Record<N> r{};
    setField(r, 0, (int64_t)(blockX * 65536.0));
    setField(r, 8, (int64_t)(blockY * 65536.0));
    return r;
}
DamageRecord damageNumber(int64_t attacker, int64_t target, float amount) {
    DamageRecord d{};
    setField(d, 0x00, attacker);
    setField(d, 0x08, target);
    setField(d, 0x10, amount);
    return d;
}

// Single player through the loopback, then a second machine joining: interest, removal by
// omission, the event filters, zone answers, the guests' events, interactions and the clock.
void hostAndClientGames() {
    LoopbackNetwork network;
    auto a = network.endpoint(HOST_MACHINE);
    FakeHostWorld hostWorld;
    FakeMirrorWorld mirror;
    HostGame host(*a, hostWorld);
    ClientGame client(*a, mirror, [&] { host.link().dropAll(); });
    uint32_t now = 1000;
    auto frame = [&](ClientGame &c) {
        c.receive(now);
        c.consumeWorldUpdates();
        c.send(now);
    };
    auto tick = [&](int frames = 1) {
        for (int i = 0; i < frames; i++) {
            now += 33;
            host.frame(now);
            frame(client);
        }
    };
    // The world: monsters in view, one out of view, a far player and two always-sent newcomers.
    hostWorld.add(5001, 1, 100, 0);   // 100 blocks away
    hostWorld.add(5002, 1, 0, 200);   // 200
    hostWorld.add(5003, 1, 300, 0);   // too far (270)
    hostWorld.add(5004, 1, 0, 260, 0x800);  // always sent
    hostWorld.add(5005, 1, 0, -260, 0x2000);
    hostWorld.add(9, 0, 5000, 5000);  // a player, any distance
    client.link().goHome(now);
    tick(); // join, id 1, the player's first record
    NET_CHECK(client.link().localId() == 1 && mirror.local == 1);
    tick();
    NET_CHECK(hostWorld.hasCreature(1));
    // Newcomers: one a tick, the always-sent ones besides it.
    std::vector<size_t> seen{mirror.others.size()};
    for (int i = 0; i < 6; i++) {
        tick();
        seen.push_back(mirror.others.size());
    }
    NET_CHECK(seen.back() == 5 && !mirror.others.count(5003) && mirror.others.count(9));
    NET_CHECK(seen[0] == 3 && seen[1] == 4 && seen[2] == 5); // (the first with the two always sent, then one a tick)
    NET_CHECK(mirror.lastMask[5001] == 0 || mirror.lastMask[5001] == (1ull << 0)); // idle: an empty delta every tick
    // A monster that leaves view is gone from the mirror by its absence.
    hostWorld.moveTo(5001, 400, 0);
    tick(2);
    NET_CHECK(!mirror.others.count(5001) && mirror.others.count(5002));
    // Events: particles and sounds within 256 blocks only, never one's own sounds; hits to all.
    hostWorld.pending.particles = {eventAt<0x48>(10, 0), eventAt<0x48>(300, 0)};
    SoundRecord mine = eventAt<0x30>(1, 0), near = eventAt<0x30>(2, 0), far = eventAt<0x30>(0, 400);
    setField(mine, sound::OWNER, (int64_t)1);
    hostWorld.pending.sounds = {mine, near, far};
    hostWorld.pending.damageNumbers = {damageNumber(5002, 1, 12.0f), damageNumber(9, 5002, 30.0f)};
    tick(3);
    NET_CHECK(mirror.kept.particles.size() == 1 && mirror.kept.sounds.size() == 1);
    NET_CHECK(mirror.hitsOnMe.size() == 1);   // a monster's hit on the local player, applied by its client
    NET_CHECK(mirror.shown.size() == 2);      // both shown (neither made by the local player)
    // The local player's own hits and sounds go to the host, which applies them.
    mirror.own.damageNumbers = {damageNumber(1, 5002, 25.0f), damageNumber(5002, 1, 1.0f)};
    mirror.own.sounds = {mine, near};
    tick();
    mirror.own = {};
    tick();
    NET_CHECK(hostWorld.caused.damageNumbers.size() == 1 && hostWorld.caused.sounds.size() == 1);
    // One interaction a send tick.
    InteractionRecord pickUp = defaultInteraction();
    pickUp[interaction::KIND] = 7;
    mirror.pending = {pickUp, pickUp};
    tick(2); // (sent in one tick, received in the next)
    NET_CHECK(hostWorld.interactions.size() == 1 && hostWorld.interactions[0].first == 1);
    tick(2);
    NET_CHECK(hostWorld.interactions.size() == 2);
    // A zone request answered with its changed ground items.
    ZoneAnswer answer;
    answer.groundItems = std::vector<ZoneObjectRecord>(2);
    hostWorld.zones[{4, 5}] = answer;
    mirror.needed = {{4, 5}};
    tick(40); // (at most once a second)
    NET_CHECK(mirror.answered.size() == 1 && mirror.groundItems.size() == 1 && mirror.groundItems[0].records.size() == 2);
    // The clock, and a new day's change.
    NET_CHECK(mirror.worldDay == 3 && mirror.timeOfDay == 36000000);
    const int changes = mirror.dayChanges;
    hostWorld.time.day = 4;
    tick();
    NET_CHECK(mirror.dayChanges == changes + 1 && mirror.worldDay == 4);
    // A night slept: every guest gets full records again.
    hostWorld.pending.sleeps = {0};
    tick(2);
    NET_CHECK(mirror.lastMask[5002] == ALL_CREATURE_FIELDS);
    tick();
    NET_CHECK(mirror.lastMask[5002] == 0 && mirror.kept.sleeps.size() == 1);

    // A second machine joins this host; this machine's player sees it, and it sees this one.
    auto b = network.endpoint(GUEST_MACHINE);
    FakeHostWorld otherHostWorld;
    FakeMirrorWorld otherMirror;
    HostGame otherHost(*b, otherHostWorld);
    ClientGame other(*b, otherMirror, [&] { otherHost.link().dropAll(); });
    other.link().goHome(now);
    other.link().connectTo(HOST_MACHINE, now);
    NET_CHECK(other.link().isGuestElsewhere()); // (its own host idles now)
    for (int i = 0; i < 12; i++) {
        now += 33;
        host.frame(now);
        frame(client);
        frame(other);
    }
    NET_CHECK(other.link().localId() == 2 && hostWorld.hasCreature(2));
    NET_CHECK(mirror.others.count(2) && otherMirror.others.count(1));
    // It leaves: back in its own world, and gone from this host.
    other.link().goHome(now);
    for (int i = 0; i < 3; i++) {
        now += 33;
        host.frame(now);
        frame(client);
    }
    NET_CHECK(!hostWorld.hasCreature(2) && !mirror.others.count(2));
}

// A mirror's creature between its host's updates (SpawnedCreatures in mirror mode): its swing
// goes on with the frames, never steps back when an update comes, waits for the host when it has
// run 200 ms ahead, and starts again with a new swing.
void mirrorSwing() {
    constexpr int64_t MONSTER = 5000;
    constexpr size_t ABILITY = 0x68, ABILITY_TIME = 0x6c, HEALTH = 0x180;
    SpawnedCreatures world;
    world.setRemoteClient(true);
    auto update = [&](uint8_t ability, float ms) { // a packet 3 with the ability and its time
        bool made = false;
        uint8_t *r = world.networkCreature(MONSTER, made);
        r[ABILITY] = ability;
        std::memcpy(r + ABILITY_TIME, &ms, 4);
        const float health = 100.0f;
        std::memcpy(r + HEALTH, &health, 4);
        world.receivedUpdate(MONSTER, made);
    };
    auto shown = [&] {
        float ms;
        std::memcpy(&ms, world.record(MONSTER) + ABILITY_TIME, 4);
        return ms;
    };
    update(58, 100);
    for (int i = 0; i < 5; i++)
        world.smooth(16);
    NET_CHECK(shown() == 180.0f);       // drawn on between updates
    update(58, 132);
    NET_CHECK(shown() == 180.0f);       // an update behind what is shown does not pull it back
    update(58, 190);
    NET_CHECK(shown() == 190.0f);       // one ahead moves it forward
    for (int i = 0; i < 30; i++)
        world.smooth(16);
    NET_CHECK(shown() == 390.0f);       // at most 200 ms past the host's last time
    update(58, 20);
    NET_CHECK(shown() == 20.0f);        // the same move started again
    update(60, 5);
    NET_CHECK(shown() == 5.0f);         // another move
}

int netTests() {
    failures = 0;
    everyPacket();
    readerRule();
    creatureDelta();
    loopbackSession();
    sessionTimers();
    singlePlayer();
    udpTransport();
    recordings();
    steamIds();
    hostAndClientGames();
    mirrorSwing();
    return failures;
}

// EngineTests netdecode <file>...: every recorded packet decoded and encoded again.
int netDecodeCommand(int argc, char **argv) {
    struct Tally {
        int count = 0;
        int results[5] = {};
    };
    std::map<uint32_t, Tally> byId;
    int differs = 0, shown = 0;
    for (int i = 2; i < argc; i++) {
        std::vector<RecordedPacket> packets;
        std::string error;
        if (!readRecording(argv[i], packets, error)) {
            std::printf("%s\n", error.c_str());
            return 1;
        }
        std::printf("%s: %zu packets\n", argv[i], packets.size());
        for (size_t n = 0; n < packets.size(); n++) {
            const RecordedPacket &p = packets[n];
            uint32_t id = 0xffffffff;
            if (p.bytes.size() >= 4)
                std::memcpy(&id, p.bytes.data(), 4);
            std::string detail;
            const RoundTrip r = checkRoundTrip(p.bytes, p.direction(), &detail);
            Tally &t = byId[id];
            t.count++;
            t.results[(int)r]++;
            if (r == RoundTrip::Differs) {
                differs++;
                if (shown++ < 20)
                    std::printf("  packet %zu (t=%u, id %u %s, %zu bytes): %s\n", n, p.timeMs, id, packetName(id),
                                p.bytes.size(), detail.c_str());
            }
        }
    }
    for (const auto &[id, t] : byId)
        std::printf("  id %2u %-24s %6d: %d same, %d same body, %d cut, %d trailing, %d differ\n", id, packetName(id),
                    t.count, t.results[0], t.results[1], t.results[2], t.results[3], t.results[4]);
    std::printf(differs ? "netdecode: %d packet(s) differ\n" : "netdecode: every packet encodes back\n", differs);
    return differs ? 1 : 0;
}
