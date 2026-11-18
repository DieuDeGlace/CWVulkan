#include "GameMath.h"
#include "GroundItems.h"
#include "InventorySlots.h"
#include "net/Packets.h"
#include "NativeInteract.h"
#include "UsePrompt.h"
#include "worldgen/Residents.h"
#include <algorithm>
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
    // Manual requests run on a rising edge and take priority over the pending tail.
    {
        auto player = worldgen::newCreature(9);
        const float health = 100.f;
        std::memcpy(player.data() + worldgen::creature::HEALTH, &health, sizeof health);
        nativeai::Creature actor;
        actor.record = player.data();
        useprompt::Scene scene;
        inventoryslots::Inventory inventory;
        scene.player = player.data();
        scene.inventory = &inventory;
        useprompt::KeyState key;
        useprompt::Targets selected;
        selected.groundItem = 0;
        auto &ground = scene.groundItems.emplace_back();
        auto item = nativesaves::makeItem(11, 40, 3);
        ground.type = item[0];
        ground.subtype = 40;
        ground.worldItem = &item;
        ground.zoneSlot = {2, -3, 7};
        const auto older = nativeinteract::fresh(nativeinteract::ACTIVATE);
        const auto request = useprompt::groundRequest(item, ground.zoneSlot);
        actor.interactions = {older, request};
        const auto update = [&](bool down) {
            scene.useDown = down;
            const auto prompt = useprompt::decide(scene, selected, key);
            useprompt::applyToPlayer(prompt, scene, actor);
            return prompt;
        };
        check(update(false).text == "Pick up" && actor.interactions.size() == 2, "hover leaves pending actions intact");
        check(update(true).action == useprompt::Action::PickUpItem &&
              actor.interactions == std::list<nativeai::Interaction>{request, older, request},
              "a manual press prepends even when an equivalent pickup is pending");
        check(cwnet::field<int32_t>(request, nativeinteract::ZONE_X) == 2 &&
              cwnet::field<int32_t>(request, nativeinteract::ZONE_Y) == -3 &&
              cwnet::field<int32_t>(request, nativeinteract::OBJECT) == 7 &&
              cwnet::field<uint16_t>(request, nativeinteract::ALWAYS_ONE) == 1 &&
              std::equal(item.begin(), item.end(), request.begin()), "manual request retains slot, item and quantity one");
        update(true);
        check(actor.interactions.size() == 3, "holding Use queues no further request");
        inventory.tabs.resize(3);
        check(update(false).text == "Full" && update(true).action == useprompt::Action::BagFull &&
              actor.interactions.size() == 3, "a missing Ingredients tab refuses the press");
        inventory.tabs.resize(4);
        inventory.tabs[3] = {{9, item}, {1, nativesaves::makeItem(2, 3)}};
        inventory.cursor = {1, item};
        ground.carriedCount = useprompt::groundCarryCount(inventory, item);
        check(ground.carriedCount == 10 && update(false).action == useprompt::Action::CannotCarry,
              "the matched cursor counts at the ten-ingredient limit");
        inventory.cursor.item[nativesaves::item::REGION] = 9;
        ground.carriedCount = useprompt::groundCarryCount(inventory, item);
        check(ground.carriedCount == 9 && update(true).action == useprompt::Action::PickUpItem &&
              actor.interactions.size() == 4, "a different-region cursor does not fill the limit or an occupied tab");
        item = nativesaves::makeItem(12, 0, 11);
        ground.type = 12;
        ground.noAutoPickup = true;
        ground.carriedCount = 0;
        check(update(false).action == useprompt::Action::PickUpItem && inventory.gold == 1,
              "the selected personal gold coin checks one unit for the prompt");
        update(true);
        check(inventory.gold == 3 && std::memcmp(player.data() + worldgen::creature::UNSET_AAC, &inventory.gold, sizeof(int32_t)) == 0,
              "a new press checks one unit for the prompt and one for the request");
        item = nativesaves::makeItem(13, 0);
        ground.type = 13;
        update(false);
        update(true);
        check(inventory.fieldAb0 == 3 && std::memcmp(player.data() + worldgen::creature::UNSET_AB0, &inventory.fieldAb0, sizeof(int32_t)) == 0 &&
              inventory.quantity(item) == 0, "category 13 checks its counter while the bag awaits the notice");
        inventory.tabs.resize(2);
        update(false);
        check(update(true).action == useprompt::Action::BagFull && inventory.fieldAb0 == 3,
              "a missing Items tab prevents exceptional counter writes");
        ground.type = 24;
        ground.subtype = 4;
        check(!useprompt::groundItemValid(ground, 0), "special subtype four is hidden with an empty bag");
        const int32_t region[2]{-3, 8};
        auto special = nativesaves::makeItem(24, 2);
        std::memcpy(special.data() + nativesaves::item::REGION, region, sizeof region);
        inventory.cursor = {1, special};
        check(!useprompt::ownsGroundSpecial(inventory, 2, region) && useprompt::groundCarryCount(inventory, special) == 1,
              "a cursor-only regional special affects carrying and remains visible");
        inventory.add(special, 1);
        const int32_t elsewhere[2]{-3, 7};
        check(useprompt::ownsGroundSpecial(inventory, 2, region) && !useprompt::ownsGroundSpecial(inventory, 2, elsewhere),
              "regional ownership uses a canonical tab item for the player's current region");
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
