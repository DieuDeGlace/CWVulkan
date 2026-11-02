#include "NativeHudContext.h"
#include "GameMath.h"
#include "ItemTypes.h"
#include "NativeInteract.h"
#include <bit>
#include <cstring>

namespace hudcontext {
namespace {
void copy(nativesaves::Item &destination, const uint8_t *source) {
    using namespace nativesaves::item;
    destination[TYPE] = source[TYPE];
    std::memcpy(destination.data() + SUBTYPE, source + SUBTYPE, RARITY - SUBTYPE + 1);
    std::memcpy(destination.data() + FORMULA, source + FORMULA, 4);
    std::memcpy(destination.data() + MATERIAL, source + MATERIAL, ATTRIBUTE_COUNT - MATERIAL + 1);
}
int32_t quantity(const Tabs *inventory, const nativesaves::Item &item) {
    const size_t tab = size_t(itemtypes::inventoryTab(item[0]));
    uint32_t total = 0;
    if (inventory && tab < inventory->size())
        for (const auto &stack : (*inventory)[tab])
            if (nativeinteract::sameItem(stack.item.data(), item.data()))
                total += uint32_t(stack.count);
    return std::bit_cast<int32_t>(total);
}
} // namespace

void rebuild(State &state, const Environment &environment) {
    const auto *player = environment.player;
    if (!player || !environment.namedWorld || player[0x68] == 0x98 || player[0x68] == 0x9c || player[0x68] == 0x9d)
        return;
    const nativesaves::Item empty{};
    copy(state.item, empty.data());
    state.count = 0;
    int64_t physical[2];
    std::memcpy(physical, player + 0x10, sizeof physical);
    const nativesaves::Cell center{gamemath::floorFixedZone(physical[0]), gamemath::floorFixedZone(physical[1])};
    for (int x = center[0] - 1; x <= center[0] + 1; ++x)
        for (int y = center[1] - 1; y <= center[1] + 1; ++y) {
            const auto zone = environment.loadedZone ? environment.loadedZone({x, y}) : std::nullopt;
            if (!zone)
                continue;
            for (const auto &ground : zone->ground)
                if (ground[0] == 0x16)
                    copy(state.item, ground.data());
            for (const auto &object : zone->objects)
                if (object[0x38] && object[0x28])
                    copy(state.item, object.data() + 0x38);
        }
    state.count = quantity(environment.inventory, state.item);
    if (state.count < 1)
        copy(state.item, empty.data()); // retain the just-computed signed total
}

std::string countText(int32_t count) {
    return count > 1 ? std::to_string(count) : std::string();
}

std::vector<Model> modelRow(const State &context, const Tabs &inventory, nativesaves::Cell playerRegion) {
    std::vector<Model> out;
    int x = 225;
    if (context.item[0]) {
        out.push_back({&context.item, x});
        x = 325;
    }
    if (inventory.size() <= 1)
        return out;
    for (const auto &stack : inventory[1]) {
        if (!stack.count || stack.item[0] != 24)
            continue;
        nativesaves::Cell region;
        std::memcpy(region.data(), stack.item.data() + nativesaves::item::REGION, sizeof region);
        if (region != playerRegion)
            continue;
        uint32_t subtype;
        std::memcpy(&subtype, stack.item.data() + nativesaves::item::SUBTYPE, sizeof subtype);
        if (subtype >= 14 && subtype <= 17)
            continue;
        out.push_back({&stack.item, x});
        x += 50;
    }
    return out;
}
} // namespace hudcontext
