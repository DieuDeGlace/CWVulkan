// The zones' stored states applied and recorded, from docs/specs/native-saves.md §5.2 and §5.3.
#include "ZoneStates.h"

#include "NativeInteract.h"
#include "NativePlayer.h"

#include <cstring>

namespace zonestates {

namespace {

// The ground item record (drops-and-items.md §0.4).
constexpr size_t GROUND_POSITION = 0xa0, GROUND_FROM = 0xb8, GROUND_ANGLE = 0xc0, GROUND_FLAGS = 0xc4, GROUND_DAY = 0xd0,
                 GROUND_COUNT = 0xd4, GROUND_OWNER = 0xd8;
constexpr int32_t NO_CREATURE = -1; // +0xb8, +0xbc without a dropping creature
// The static object record (use-prompt.md §0.3).
constexpr size_t OBJECT_TYPE = 0x00, OBJECT_POSITION = 0x08, OBJECT_TURN = 0x20, OBJECT_SCALE = 0x24;
// A block change entry (§5.3).
constexpr size_t CHANGE_BLOCK = 0x00, CHANGE_WAS = 0x0c, CHANGE_NOW = 0x12;
constexpr int ZONE_SHIFT = 6; // 64 blocks a zone

template <class T> void put(uint8_t *at, size_t offset, T value) { std::memcpy(at + offset, &value, sizeof value); }
template <class T> T get(const uint8_t *at, size_t offset) {
    T value;
    std::memcpy(&value, at + offset, sizeof value);
    return value;
}

nativeplayer::Item gameItem(const worldgen::Item &it) {
    return nativeplayer::item(it.category, it.id, it.material, it.rarity, it.modifier, it.region.x, it.region.y, (int)it.realm);
}

} // namespace

Record groundItemRecord(const worldgen::GroundItem &g, int32_t day) {
    Record r{};
    const nativeplayer::Item item = gameItem(g.item);
    std::memcpy(r.data(), item.data(), item.size());
    put<int64_t>(r.data(), GROUND_POSITION, g.x);
    put<int64_t>(r.data(), GROUND_POSITION + 8, g.y);
    put<int64_t>(r.data(), GROUND_POSITION + 16, g.z);
    put<int32_t>(r.data(), GROUND_FROM, NO_CREATURE);
    put<int32_t>(r.data(), GROUND_FROM + 4, NO_CREATURE);
    put<float>(r.data(), GROUND_ANGLE, g.angle);
    r[GROUND_FLAGS] = (uint8_t)g.flags;
    put<int32_t>(r.data(), GROUND_DAY, day);
    put<int32_t>(r.data(), GROUND_COUNT, 1);
    put<int64_t>(r.data(), GROUND_OWNER, 0);
    return r;
}

Record staticObjectRecord(const worldgen::ZoneObject &o) {
    Record r{};
    put<int32_t>(r.data(), OBJECT_TYPE, o.type);
    put<int64_t>(r.data(), OBJECT_POSITION, o.x);
    put<int64_t>(r.data(), OBJECT_POSITION + 8, o.y);
    put<int64_t>(r.data(), OBJECT_POSITION + 16, o.z);
    r[OBJECT_TURN] = o.turn;
    put<float>(r.data(), OBJECT_SCALE, o.scale);
    r[nativeinteract::OBJECT_STATE] = 1;
    put<float>(r.data(), nativeinteract::OBJECT_SWING, nativeinteract::FULL_SWING); // (the generator's doors start with a finished swing)
    if (o.takes.category) {
        const nativeplayer::Item item = gameItem(o.takes);
        std::memcpy(r.data() + nativeinteract::OBJECT_ITEM, item.data(), item.size());
    }
    return r;
}

bool holdsItem(const Record &groundItem) { return groundItem[0] != 0; }

std::array<int64_t, 3> groundItemPosition(const Record &groundItem) {
    return {get<int64_t>(groundItem.data(), GROUND_POSITION), get<int64_t>(groundItem.data(), GROUND_POSITION + 8),
            get<int64_t>(groundItem.data(), GROUND_POSITION + 16)};
}

Cell zoneOfBlock(int64_t x, int64_t y) {
    auto zone = [](int64_t block) { return (int32_t)(block >> ZONE_SHIFT); }; // (an arithmetic shift rounds down)
    return {zone(x), zone(y)};
}

BlockChange blockChange(int32_t x, int32_t y, int32_t z, const std::array<uint8_t, 6> &was, const std::array<uint8_t, 6> &now) {
    BlockChange c;
    put<int32_t>(c.bytes.data(), CHANGE_BLOCK, x);
    put<int32_t>(c.bytes.data(), CHANGE_BLOCK + 4, y);
    put<int32_t>(c.bytes.data(), CHANGE_BLOCK + 8, z);
    std::memcpy(c.bytes.data() + CHANGE_WAS, was.data(), was.size());
    std::memcpy(c.bytes.data() + CHANGE_NOW, now.data(), now.size());
    return c;
}

ZoneStates::ZoneStates(nativesaves::WorldState *w, bool server, BlocksChanged b) : world(w), serverMode(server), blocks(std::move(b)) {}

nativesaves::ZoneLists *ZoneStates::find(Cell zone) {
    auto it = zones.find(zone);
    return it == zones.end() ? nullptr : &it->second;
}

const nativesaves::ZoneLists *ZoneStates::find(Cell zone) const {
    auto it = zones.find(zone);
    return it == zones.end() ? nullptr : &it->second;
}

std::vector<Cell> ZoneStates::zonesInMemory() const {
    std::vector<Cell> out;
    for (const auto &[zone, lists] : zones)
        out.push_back(zone);
    return out;
}

nativesaves::ZoneLists &ZoneStates::load(Cell zone, std::vector<Record> groundItems, std::vector<Record> staticObjects) {
    if (auto known = zones.find(zone); known != zones.end())
        return known->second;
    nativesaves::ZoneLists &lists = zones[zone];
    lists.groundItems = std::move(groundItems);
    lists.staticObjects = std::move(staticObjects);
    if (world)
        nativesaves::applyStoredZone(*world, zone, lists, serverMode, nullptr);
    // The block changes go back in order; whoever keeps the blocks puts each "now" block back.
    if (blocks && !lists.blockChanges.empty())
        blocks(zone, lists.blockChanges);
    return lists;
}

void ZoneStates::unload(Cell zone, std::optional<int32_t> deadTracked) {
    auto it = zones.find(zone);
    if (it == zones.end())
        return;
    if (deadTracked)
        it->second.deadTracked = *deadTracked;
    if (world && !serverMode)
        nativesaves::recordZone(*world, zone, it->second);
    zones.erase(it);
}

void ZoneStates::unloadAll() {
    while (!zones.empty())
        unload(zones.begin()->first);
}

void ZoneStates::objectsChanged(Cell zone) {
    if (nativesaves::ZoneLists *lists = find(zone))
        lists->staticObjectsChanged = true;
}

void ZoneStates::groundItemsChanged(Cell zone) {
    if (nativesaves::ZoneLists *lists = find(zone))
        lists->groundItemsChanged = true;
}

void ZoneStates::addBlockChange(const BlockChange &change) {
    const std::array<int32_t, 3> at = change.block();
    const Cell zone = zoneOfBlock(at[0], at[1]);
    std::vector<BlockChange> *list = nullptr;
    if (nativesaves::ZoneLists *lists = find(zone))
        list = &lists->blockChanges;
    else if (world && !serverMode)
        list = &world->zones[zone].blockChanges; // (a zone not in memory keeps it in its stored state)
    if (!list)
        return;
    list->push_back(change);
    if (blocks)
        blocks(zone, *list);
}

} // namespace zonestates
