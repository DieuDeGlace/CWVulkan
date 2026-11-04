#include "NativeVehicle.h"
#include "GameMath.h"
#include "ItemValue.h"
#include "NativeInteract.h"
#include <bit>
#include <cstring>
#include <tuple>

namespace nativevehicle {
namespace {
template <class T> T read(const uint8_t *record, size_t offset) {
    T value;
    std::memcpy(&value, record + offset, sizeof value);
    return value;
}
int64_t addWord(int64_t value, uint64_t delta) { return std::bit_cast<int64_t>(uint64_t(value) + delta); }
bool outsideProtectedBlock(const Environment &environment) {
    BlockPosition at;
    for (size_t axis = 0; axis < at.size(); ++axis)
        at[axis] = gamemath::floorFixedBlock(read<int64_t>(environment.player, 0x10 + axis * 8));
    const auto flags = environment.blockFlags ? environment.blockFlags(at) : std::nullopt;
    return flags && !(*flags & 0x20);
}
} // namespace

bool ownsSpecialItem(const uint8_t *player, const Tabs *inventory, int32_t subtype) {
    if (subtype == 4)
        return true;
    if (!player || !inventory)
        return false;
    auto wanted = nativesaves::makeItem(0x18, subtype);
    std::memcpy(wanted.data() + nativesaves::item::REGION, player + 0x150, 8);
    uint32_t total = 0;
    for (const auto &tab : *inventory)
        for (const auto &stack : tab)
            if (nativeinteract::sameItem(stack.item.data(), wanted.data()))
                total += uint32_t(stack.count);
    return std::bit_cast<int32_t>(total) > 0;
}

bool nearbyObject(const Environment &environment, int32_t kind, bool requireState) {
    if (!environment.player || !environment.loadedObjects)
        return false;
    std::array<int64_t, 3> physical;
    std::memcpy(physical.data(), environment.player + 0x10, sizeof physical);
    const int firstX = gamemath::floorFixedZone(addWord(physical[0], uint64_t(-0x80000ll)));
    const int lastX = gamemath::floorFixedZone(addWord(physical[0], 0x80000));
    const int firstY = gamemath::floorFixedZone(addWord(physical[1], uint64_t(-0x80000ll)));
    const int lastY = gamemath::floorFixedZone(addWord(physical[1], 0x80000));
    for (int x = firstX; x <= lastX; ++x)
        for (int y = firstY; y <= lastY; ++y)
            for (const auto &object : environment.loadedObjects({x, y})) {
                if (read<int32_t>(object.data(), 0) != kind || (requireState && !object[0x28]))
                    continue;
                std::array<float, 3> delta;
                for (size_t axis = 0; axis < delta.size(); ++axis) {
                    const auto difference = std::bit_cast<int64_t>(
                        uint64_t(read<int64_t>(object.data(), 8 + axis * 8)) - uint64_t(physical[axis]));
                    delta[axis] = float(difference) * (1.0f / 65536.0f);
                }
                const float xSquared = delta[0] * delta[0], ySquared = delta[1] * delta[1];
                const float zSquared = delta[2] * delta[2];
                const float xy = xSquared + ySquared;
                if (xy + zSquared <= 64.0f)
                    return true;
            }
    return false;
}

void rebuild(nativesaves::Item &selection, const Environment &environment) {
    auto *player = environment.player;
    if (!player || !environment.namedWorld || player[0x68] == 0x98 || player[0x68] == 0x9c || player[0x68] == 0x9d)
        return;
    const uint32_t flags = read<uint32_t>(player, 0x5c);
    const auto owns = [&](int32_t subtype) { return ownsSpecialItem(player, environment.inventory, subtype); };
    const auto ability = player[0x68];
    if ((ability == 0x76 && !owns(3)) || (ability == 0x78 && (flags & 1) && !owns(1)) || (ability == 0x79 && !owns(2)))
        player[0x68] = 0;
    const nativesaves::Item empty{};
    itemvalue::assign(selection.data(), empty.data());
    if (!(read<float>(player, 0x12c) == 0.0f))
        return;
    const float age = read<float>(player, 0x6c);
    const auto duration = [&] { return environment.wholeDuration ? environment.wholeDuration() : 0.0f; };
    if (!(age > duration()) || player[0x68] == 0x7a)
        return;
    const auto emit = [&](int32_t subtype) {
        selection[0] = 0x18;
        std::memcpy(selection.data() + 4, &subtype, sizeof subtype);
    };
    if (player[0x68] == 0x78)
        emit(1);
    else if (player[0x68] == 0x79)
        emit(2);
    for (const auto [subtype, kind, requireState] :
         {std::tuple{7, 1, false}, {13, 9, false}, {4, 0, true}, {8, 2, false}, {5, 16, true}}) {
        if (owns(subtype) &&
            (nearbyObject(environment, kind, requireState) || (subtype == 8 && nearbyObject(environment, 15, false))) &&
            (flags & 3)) {
            emit(subtype);
            return;
        }
    }
    if (owns(2) && (flags & 6) == 2 && outsideProtectedBlock(environment)) {
        emit(2);
        return;
    }
    if (owns(1) && !(flags & 3) && player[0x68] != 0x53 && (player[0x68] == 0 || age >= duration()) &&
        outsideProtectedBlock(environment))
        emit(1);
}
} // namespace nativevehicle
