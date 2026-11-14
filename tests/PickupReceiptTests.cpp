#include "GameMath.h"
#include "GroundItems.h"
#include "InventorySlots.h"
#include "net/Packets.h"
#include <cstring>
#include <iostream>
#include <limits>

int pickupReceiptTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *label) {
        if (!passed) {
            std::cout << "FAIL pickup receipt: " << label << '\n';
            ++failures;
        }
    };
    const int32_t region[2]{3, -7};
    constexpr uint16_t quantities[]{0, 32767, 32768, 65535};
    constexpr int32_t sums[]{INT32_MAX, -2147450882, -2147450881, -2147418114};
    for (size_t i = 0; i < 4; ++i) {
        cwnet::PickupRecord raw;
        raw.fill(0xa5); // Poison gaps: the decoder must read only the quantity word.
        cwnet::setField(raw, 0, int64_t(77));
        grounditems::Item item{};
        item[0] = 11;
        std::memcpy(raw.data() + 8, item.data(), item.size());
        cwnet::setField(raw, 0xa8, quantities[i]);
        const int64_t position[3]{-65536, INT64_MAX, INT64_MIN};
        std::memcpy(raw.data() + 0xb0, position, sizeof position);
        cwnet::setField(raw, 0xc8, uint32_t(0xffffff80));
        cwnet::WorldEvents outgoing;
        outgoing.pickups.push_back(raw);
        const auto bytes = cwnet::encodeWorldEvents(outgoing);
        bool cut = false;
        const auto incoming = cwnet::decodeWorldEvents(bytes.data(), bytes.size(), &cut);
        check(!cut && incoming.pickups.size() == 1, "complete pickup packet roundtrip");
        if (incoming.pickups.size() != 1)
            continue;
        const auto notice = grounditems::decodePickupNotice(incoming.pickups.front());
        check(notice.picker == 77 && notice.quantity == quantities[i] && notice.flags == 0xffffff80 &&
                  std::memcmp(notice.position.data(), position, sizeof position) == 0 && notice.item == item,
              "wire-to-receipt decoder retains unsigned quantity, flags32, item and signed positions");
        std::set<std::array<int32_t, 3>> perPlayerTaken;
        std::map<std::array<int32_t, 2>, int32_t> regionCounters;
        grounditems::Keeper keeper{perPlayerTaken, regionCounters};
        const auto receipt = grounditems::receive(keeper, notice.item, notice.item, notice.quantity,
                                                  uint8_t(notice.flags), position, region, 0, true);
        check(receipt.into == grounditems::Into::Inventory && receipt.amount == quantities[i],
              "receipt amount never sign-extends the quantity word");
        inventoryslots::Inventory inventory;
        inventory.add(item, receipt.amount);
        check(inventory.quantity(item) == quantities[i], "actual inventory insertion receives the full notice amount");
        item[0] = grounditems::type::COIN;
        item[0x1c] = grounditems::REGION_COIN_MATERIAL;
        keeper.regionCounters[{3, -7}] = INT32_MAX;
        const auto regional = grounditems::receive(keeper, item, item, notice.quantity, 0, position, region, 0, true);
        check(regional.into == grounditems::Into::RegionCounter && keeper.regionCounters[{3, -7}] == sums[i],
              "regional quantity accumulation uses native low32 addition");
        item[0x1c] = grounditems::COIN_MATERIAL;
        inventory.gold = INT32_MAX;
        inventory.add(item, notice.quantity);
        check(inventory.gold == sums[i], "direct-quantity gold insertion wraps its four-byte counter");
    }
    grounditems::Item bag{};
    bag[0] = grounditems::type::GOLD_BAG;
    std::set<std::array<int32_t, 3>> perPlayerTaken;
    std::map<std::array<int32_t, 2>, int32_t> regionCounters;
    grounditems::Keeper keeper{perPlayerTaken, regionCounters};
    const int64_t position[3]{};
    const int value = grounditems::moneyValue(bag);
    const auto zero = grounditems::receive(keeper, bag, bag, 0, 0, position, region, 0, true);
    const auto many = grounditems::receive(keeper, bag, bag, 65535, 0, position, region, 0, true);
    check(value != 0 && zero.into == grounditems::Into::Gold && many.into == grounditems::Into::Gold &&
              zero.amount == value && many.amount == value,
          "item-derived gold value is added once regardless of notice quantity");
    check(gamemath::add32(INT32_MAX, 1) == INT32_MIN && gamemath::add32(INT32_MIN, -1) == INT32_MAX,
          "signed counter extrema wrap in both directions without C++ signed overflow");
    std::cout << "Pickup receipt quantities: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
