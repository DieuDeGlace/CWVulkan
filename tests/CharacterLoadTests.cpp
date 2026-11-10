#include "Extracted.h"
#include "NativeProgress.h"
#include "Spawned.h"
#include <array>
#include <bit>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>

namespace {
void rawNativeLoad(const auto &check) {
    ExtractedWorld blocks;
    blocks.openGenerated(0, 0);
    nativesaves::Character saved;
    saved.name = "RAW_LOAD";
    saved.position = {(int64_t(1) << 36) + 1, -(int64_t(1) << 36) + 3, 123 * 65536 + 7};
    saved.orientation = {17.25f, -0.0f, -93.5f};
    saved.level = -3; saved.experience = INT32_MIN;
    saved.classType = 4; saved.specialization = 1;
    saved.field9a4 = std::bit_cast<float>(0x3f876543u); saved.field9a8 = -0.0f;
    saved.homeCell = {-73, 91};
    saved.regionHistory = {{7,-2},{-5,8},{7,-2}};
    saved.buffs.resize(2);
    saved.buffs[0].bytes[0] = 0x13; saved.buffs[0].bytes[23] = 0xf7;
    saved.buffs[1].bytes = saved.buffs[0].bytes; saved.buffs[1].bytes[23] = 0x89;
    for (auto &item : saved.equipment)
        for (size_t i = 1; i < item.size(); ++i) item[i] = uint8_t(i * 7); // empty type; whole-record preservation
    SpawnedCreatures actor;
    actor.setStoredZones(blocks, nullptr, false);
    actor.setNativePlayer(true);
    const auto capture = actor.playerSaveSource();
    const std::array<float, 4> health{0.0f, -7.25f, 99999.5f, -0.0f};
    for (float hp : health) {
        saved.health = hp;
        actor.seedPlayerSave(std::make_shared<const nativesaves::Character>(saved));
        actor.setPlayer(blocks, {1,2,3}, 1.5f, {9,8,7}, 123.f, 99, true);
        nativesaves::Character current;
        check(capture(current), "complete native load produces the local actor");
        check(std::bit_cast<uint32_t>(current.health) == std::bit_cast<uint32_t>(hp),
              "saved zero/negative/above-maximum/signed-zero health survives the load boundary raw");
        check(current.position == saved.position && current.orientation == std::array<float,3>{},
              "saved fixed position bypasses float placement while saved orientation is consumed and zeroed");
        check(current.level == -3 && current.experience == INT32_MIN &&
              current.classType == 4 && current.specialization == 1,
              "saved core scalars survive legacy level/class metadata at the load boundary");
        check(std::memcmp(current.equipment.data(), saved.equipment.data(), sizeof(saved.equipment)) == 0 &&
              current.buffs.size() == 2 && current.buffs[0].bytes == saved.buffs[0].bytes &&
              current.buffs[1].bytes == saved.buffs[1].bytes && current.regionHistory == saved.regionHistory,
              "load keeps complete equipment, separate decoded buffs and ordered duplicate history");
        check(current.name == saved.name && current.homeCell == saved.homeCell &&
              std::bit_cast<uint32_t>(current.field9a4) == 0x3f876543u &&
              std::bit_cast<uint32_t>(current.field9a8) == 0x80000000u,
              "bookkeeping/name/home values remain separate from startup reconstruction");
    }
    saved.position = {}; saved.health = -11.0f;
    actor.seedPlayerSave(std::make_shared<const nativesaves::Character>(saved));
    actor.setPlayer(blocks, {15,25,35}, 2.f, {}, 80.f, 3, false);
    nativesaves::Character origin;
    check(capture(origin) && origin.position == saved.position && origin.health == -11.0f,
          "zero fixed coordinates are saved coordinates rather than a missing-position sentinel");
}
void resetRetainsSeparateContainers(const auto &check) {
    ExtractedWorld blocks; blocks.openGenerated(0,0);
    SpawnedCreatures actor;
    actor.setStoredZones(blocks, nullptr, false); actor.setNativePlayer(true);
    nativesaves::Character saved;
    saved.name = "RESET_TEST"; saved.position = {17,29,41}; saved.health = -3.0f;
    actor.seedPlayerSave(std::make_shared<const nativesaves::Character>(saved));
    actor.setPlayer(blocks, {}, 0, {}, 100, 1, false);
    auto *local = actor.localPlayer();
    local->interactions.push_back({});
    local->aggro[13] = 2.0f; local->damage[14] = 3.0f; local->cooldowns[7] = 80.0f;
    local->counted = {31}; local->struck = {32}; local->skills = {33};
    local->summonRaces = {34}; local->group = {35};
    local->record[0x68] = 0x9c;
    const float velocity = 99.0f;
    std::memcpy(local->record + 0x34, &velocity, 4);
    const int64_t relationship = 71;
    std::memcpy(local->record + 0x1a0, &relationship, 8);
    std::memcpy(local->record + 0x9d8, &relationship, 8);
    const uint32_t untouchedTimer = 0xa5a5a5a5u, untouchedFlags = 0x00cc00ccu, untouchedRangeTag = 77;
    std::memcpy(local->record + 0xbbc, &untouchedTimer, 4);
    std::memcpy(local->record + 0xbcc, &untouchedFlags, 4);
    std::memcpy(local->record + 0xbe8, &untouchedRangeTag, 4);
    const std::array<size_t,26> gaps{
        0x61,0x62,0x63, 0x69,0x6a,0x6b, 0x7d,0x7e,0x7f, 0x12a,0x12b,
        0x14e,0x14f, 0x19e,0x19f, 0x971,0x972,0x973,
        0x1d9,0x1da,0x1db, 0x1ed,0x1ee,0x1ef, 0x276,0x277};
    std::memset(local->record + 0x1d8, 0x5a, 160);
    std::memset(local->record + 0xa0c, 0x6b, 160);
    for (size_t at : gaps) local->record[at] = 0xa7;
    const uint32_t oldState = 0xffffffffu;
    std::memcpy(local->record + 0x5c, &oldState, 4);
    const uint64_t rangeBegin = 0x123456789abcdef0ull, rangeCapacity = 0xfedcba9876543210ull;
    std::memcpy(local->record + 0xc20, &rangeBegin, 8);
    std::memcpy(local->record + 0xc30, &rangeCapacity, 8);
    actor.seedPlayerSave(std::make_shared<const nativesaves::Character>(saved));
    actor.setPlayer(blocks, {}, 0, {}, 100, 1, false);
    local = actor.localPlayer();
    float currentVelocity; int64_t currentRelationship, currentOuter;
    std::memcpy(&currentVelocity, local->record + 0x34, 4);
    std::memcpy(&currentRelationship, local->record + 0x1a0, 8);
    std::memcpy(&currentOuter, local->record + 0x9d8, 8);
    check(local->interactions.empty() && local->aggro.empty() && local->damage.empty() && local->cooldowns.empty(),
          "load reset clears queued work and the three traced maps without dispatching them");
    check(local->record[0x68] == 0 && currentVelocity == 0 && currentRelationship == 0 && currentOuter == 0,
          "load reset clears the traced action/motion/relationship operands before saved fields");
    check(local->counted == std::set<int64_t>{31} && local->struck == std::set<int64_t>{32} &&
          local->skills == std::vector<int32_t>{33} && local->summonRaces == std::vector<int32_t>{34} &&
          local->group == std::set<int64_t>{35},
          "the separate identifier sets and retained ranges survive reset rather than whole-side reconstruction");
    uint32_t timer, flags, tag;
    std::memcpy(&timer, local->record + 0xbbc, 4); std::memcpy(&flags, local->record + 0xbcc, 4);
    std::memcpy(&tag, local->record + 0xbe8, 4);
    check(timer == untouchedTimer && flags == untouchedFlags && tag == untouchedRangeTag,
          "nearby outer timer/flag/range-tag fields are preserved by their actual reset widths");
    bool gapsKept = true;
    for (size_t at : gaps) gapsKept = gapsKept && local->record[at] == 0xa7;
    check(gapsKept, "all omitted core and active-item padding bytes survive the selective load assignment");
    const auto word = [&](size_t at) {
        uint32_t value; std::memcpy(&value, local->record + at, 4); return value;
    };
    check(word(0x5c) == 0 && word(0x130) == 0xc53b8000u && word(0x184) == 0x3f800000u &&
          word(0x194) == 0x3f800000u && word(0x198) == 0x41200000u &&
          word(0x1b0) == INT32_MAX && word(0x1b4) == INT32_MAX && word(0xa08) == 0,
          "selected default-core values differ from reset-alone without normalizing decoded health");
    bool emptyItems = true;
    for (size_t at : {size_t(0x1d8),size_t(0xa0c)})
        for (size_t byte = 0; byte < 160; ++byte) {
            const bool padding = (byte >= 1 && byte <= 3) || (byte >= 0x15 && byte <= 0x17) || byte >= 0x9e;
            if (!padding) emptyItems = emptyItems && local->record[at + byte] == 0;
        }
    check(emptyItems, "active and cursor items clear every defined component even when their type becomes empty");
    uint64_t begin, end, capacity;
    std::memcpy(&begin, local->record + 0xc20, 8); std::memcpy(&end, local->record + 0xc28, 8);
    std::memcpy(&capacity, local->record + 0xc30, 8);
    check(begin == rangeBegin && end == begin && capacity == rangeCapacity,
          "the separately reset range becomes empty while its begin and capacity slots survive");

}
void controlledInventoryUsesCurrentState(const auto &check) {
    ExtractedWorld blocks; blocks.openGenerated(0,0);
    SpawnedCreatures actor;
    actor.setStoredZones(blocks, nullptr, false); actor.setNativePlayer(true);
    nativesaves::Character saved;
    saved.name = "LIVE_INVENTORY"; saved.health = 1.0f;
    saved.inventory[0] = {{5, nativesaves::makeItem(1,1)}};
    actor.seedPlayerSave(std::make_shared<const nativesaves::Character>(saved));
    actor.setPlayer(blocks, {}, 0, {}, 1, 1, false);
    auto canonical = std::make_shared<inventoryslots::PlayerState>(inventoryslots::PlayerState::fromCharacter(saved));
    canonical->inventory.tabs[0][0].count = 2; // gameplay changed the live slot after load
    std::weak_ptr<inventoryslots::PlayerState> retained = canonical;
    actor.setPlayerInventorySource([owner = canonical](const nativeai::InventoryVisitor &visit) {
        for (auto &tab : owner->inventory.tabs)
            for (auto &stack : tab)
                if (!visit({&stack.count, stack.item.data()})) return;
    });
    canonical.reset(); // the original owner may disappear before the AI pass
    auto *local = actor.localPlayer();
    const uint16_t controlled = uint16_t(1u << 9);
    const float elapsed = 100000.0f;
    std::memcpy(local->record + 0x128, &controlled, 2);
    std::memcpy(local->record + 0x6c, &elapsed, 4);
    nativeai::World world;
    world.client = true; world.local = local; world.creatures = {local}; world.byId = {{1,local}};
    world.rand = [] { return 0; }; world.clock = [] { return uint32_t(0); };
    world.block = [](int,int,int) { return nativeai::Block{0,0,0,0,1,0}; };
    world.zone = [](int,int) { return true; };
    world.objects = [](int,int) -> const std::vector<nativeai::Object> * { return nullptr; };
    std::map<int64_t,nativeai::Entry> ai;
    nativeai::tick(world, ai, 40);
    check(local->record[0x68] == 0x54, "the admitted controlled actor consumes a healing potion");
    const auto current = retained.lock();
    check(current && current->inventory.tabs[0][0].count == 1,
          "controlled-player AI consumes the current canonical slot rather than an earlier inventory copy");
}
void potionCountWidths(const auto &check) {
    const std::array<std::array<int32_t,2>,6> cases{{{0,0},{1,0},{2,1},{-1,0},{INT32_MIN,INT32_MAX},{INT32_MAX,INT32_MAX-1}}};
    for (bool canonicalSource : {false,true}) for (const auto &test : cases) {
        ExtractedWorld blocks; blocks.openGenerated(0,0);
        SpawnedCreatures actor;
        actor.setStoredZones(blocks, nullptr, false); actor.setNativePlayer(true);
        nativesaves::Character saved;
        saved.name = "POTION_COUNT"; saved.health = 1.0f;
        auto potion = nativesaves::makeItem(1,1);
        potion[1] = 0xa7;
        for (size_t i = 0x1d; i < 0x9d; ++i) potion[i] = uint8_t(i * 9);
        saved.inventory[0] = {{canonicalSource ? 5 : test[0], potion}};
        actor.seedPlayerSave(std::make_shared<const nativesaves::Character>(saved));
        actor.setPlayer(blocks, {}, 0, {}, 1, 1, false);
        auto live = std::make_shared<inventoryslots::PlayerState>(inventoryslots::PlayerState::fromCharacter(saved));
        live->inventory.tabs[0][0].count = test[0];
        if (canonicalSource)
            actor.setPlayerInventorySource([live](const nativeai::InventoryVisitor &visit) {
                for (auto &tab : live->inventory.tabs)
                    for (auto &stack : tab) if (!visit({&stack.count,stack.item.data()})) return;
            });
        auto *local = actor.localPlayer();
        const uint16_t controlled = uint16_t(1u << 9); const float elapsed = 100000.f;
        std::memcpy(local->record + 0x128, &controlled, 2);
        std::memcpy(local->record + 0x6c, &elapsed, 4);
        local->record[0x1d9] = 0x6b; // destination padding must not come from the source item
        nativeai::World world;
        world.client = true; world.local = local; world.creatures = {local}; world.byId = {{1,local}};
        unsigned draws = 0;
        world.rand = [&] { return ++draws == 1 ? 0 : 32767; };
        world.clock = [] { return uint32_t(0); }; world.day = 48600000.0f;
        world.block = [](int,int,int) { return nativeai::Block{0,0,0,0,1,0}; };
        world.zone = [](int,int) { return true; };
        world.objects = [](int,int) -> const std::vector<nativeai::Object> * { return nullptr; };
        std::map<int64_t,nativeai::Entry> ai; nativeai::tick(world, ai, 40);
        int32_t count; const uint8_t *item;
        if (canonicalSource) { count = live->inventory.tabs[0][0].count; item = live->inventory.tabs[0][0].item.data(); }
        else { const auto &slot = local->inventory[0][0]; std::memcpy(&count,slot.data(),4); item = slot.data()+4; }
        check(count == test[1], "canonical and byte-record AI slots use the proved 32-bit post-decrement signed comparison");
        check(item[1] == 0xa7, "clearing a counted slot preserves its item padding");
        if (test[0] != 0) {
            int32_t subtype; std::memcpy(&subtype,item+4,4);
            check(local->record[0x68] == 0x54 && draws == 2 &&
                  item[0] == (test[1] ? 1 : 0) && subtype == (test[1] ? 1 : 0),
                  "each admitted count copies/consumes once with exactly two ordered random calls");
            check(local->record[0x1d8] == 1 && local->record[0x1d9] == 0x6b &&
                  std::memcmp(local->record+0x1d8+0x1d,potion.data()+0x1d,128) == 0,
                  "the full active-item component area is copied before clearing and destination padding survives");
        } else check(local->record[0x68] != 0x54 && item[0] == 1, "zero-count slots are skipped without consuming their item");
    }
}
void rawProgressOwnership(const auto &check) {
    const auto dir = std::filesystem::temp_directory_path() /
        ("CubeVulkan-raw-progress-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    {
        nativeprogress::ProgressFile file;
        check(file.open(dir, dir / "unused-owner-Save", {}, true, {}), "private native progress opens");
        nativesaves::Character saved;
        saved.name = "RAW_PROGRESS"; saved.health = -0.0f; saved.level = -9;
        saved.position = {}; saved.experience = INT32_MIN;
        saved.gold = 193; saved.fieldAb0 = -17;
        saved.inventory = {{{-3, nativesaves::makeItem(2,3)}}, {}, {{0, nativesaves::makeItem(4,5)}}};
        CharacterEntry entry; entry.native = true; entry.slot = file.store().create(saved);
        file.play(entry);
        SaveGame progress;
        progress.nativeInventory.emplace();
        progress.nativeInventory->inventory.cursor.count = 19;
        progress.nativeInventory->inventory.cursor.item = nativesaves::makeItem(3,7);
        nativesaves::Character raw;
        check(file.load(progress, {}, "world", {}, &raw), "progress load returns the complete decoded native value");
        check(raw.position == saved.position && raw.level == -9 && raw.experience == INT32_MIN &&
              std::bit_cast<uint32_t>(raw.health) == 0x80000000u,
              "owned raw load remains independent of legacy UI zero-position/health/level conversion");
        check(progress.nativeInventory && progress.nativeInventory->inventory.cursor.count == 0 &&
              progress.nativeInventory->inventory.cursor.item == nativesaves::Item{} &&
              progress.nativeInventory->inventory.tabs.size() == 3 &&
              progress.nativeInventory->inventory.tabs[0][0].count == -3 &&
              progress.nativeInventory->inventory.tabs[2][0].count == 0 &&
              progress.nativeInventory->inventory.gold == 193 && progress.nativeInventory->inventory.fieldAb0 == -17,
              "character load resets transient cursor while replacing complete tabs and both saved counters");
    }
    for (const auto &file : std::filesystem::directory_iterator(dir))
        if (file.is_regular_file()) std::filesystem::remove(file.path());
    std::filesystem::remove(dir);
}
void retainedPlayerAttachment(const auto &check) {
    ExtractedWorld blocks;
    blocks.openGenerated(0,0);
    std::shared_ptr<SpawnedCreatures::PlayerStorage> retained;
    std::function<bool(nativesaves::Character &)> observer;
    const uint8_t *original=nullptr;
    {
        SpawnedCreatures first;
        first.setStoredZones(blocks,nullptr,false); first.setNativePlayer(true);
        first.setPlayer(blocks,{1,2,130},0,{},100,1,true);
        auto *actor=first.localPlayer();
        original=actor->record;
        actor->counted={31}; actor->struck={32}; actor->group={35};
        actor->skills={33}; actor->summonRaces={34};
        actor->aggro[7]=3; actor->damage[8]=4; actor->cooldowns[9]=5;
        actor->record[0x61]=0xa7;
        actor->record[0x7d]=0xb8;
        const auto gear=worldgen::newCreature(1);
        first.setPlayerGear(gear);
        check(actor->record[0x7d]==0xb8,
              "native startup gear bridge preserves appearance padding omitted by defined-field assignment");
        retained=first.retainedPlayer();
        observer=first.playerSaveSource();
    }
    {
        SpawnedCreatures next(retained);
        next.setStoredZones(blocks,nullptr,false); next.setNativePlayer(true);
        auto *actor=next.localPlayer();
        check(actor && actor->record==original && actor->counted==std::set<int64_t>{31},
              "reattachment preserves the actual CPU record and its separately owned collections");
        nativesaves::Character saved;
        saved.name="ATTACHED"; saved.position={17,-29,41}; saved.health=-2.5f;
        next.seedPlayerSave(std::make_shared<const nativesaves::Character>(saved));
        next.setPlayer(blocks,{1,2,3},0,{},100,1,true);
        check(actor->record==original && actor->record[0x61]==0xa7 && actor->struck==std::set<int64_t>{32} &&
              actor->group==std::set<int64_t>{35} && actor->skills==std::vector<int32_t>{33} &&
              actor->summonRaces==std::vector<int32_t>{34},
              "reached load preserves exactly the separately retained sets, ranges and omitted bytes");
        check(actor->aggro.empty() && actor->damage.empty() && actor->cooldowns.empty(),
              "reached load still clears the three reset-owned maps after reattachment");
        nativesaves::Character current;
        check(observer(current) && current.name==saved.name && current.position==saved.position && current.health==-2.5f,
              "an existing save observer follows the same live owner after adapter teardown and later loading");
        bool made=false;
        next.networkCreature(72,made);
        std::vector<int64_t> order;
        next.forEachWorldCreature([&](const uint8_t *record) {
            int64_t id; std::memcpy(&id,record+8,8); order.push_back(id);
        });
        check(made && order==std::vector<int64_t>{1,72},
              "fresh owned World registration admits retained player once before later residents");
        SpawnedCreatures detached;
        detached.setNativePlayer(true);
        detached.setPlayer(blocks,{1,2,130},0,{},100,1,true);
        check(detached.localPlayer()->record!=original && detached.localPlayer()->group.empty(),
              "unbound simulations have independent player ownership");
    }
}
void frontendLoadOwnership(const auto &check) {
    ExtractedWorld blocks; blocks.openGenerated(0,0);
    SpawnedCreatures player;
    player.setNativePlayer(true); player.setPlayer(blocks,{1,2,130},0,{},100,1,true);
    auto data=std::make_shared<nativesaves::Character>();
    data->health=-3.25f; data->inventory.resize(6); data->inventory[0].push_back({3,{}});
    std::weak_ptr<nativesaves::Character> retained=data;
    const auto *record=player.localPlayer()->record;
    check(player.loadPlayerSave(data) && player.localPlayer()->record==record && player.playerHealth()==-3.25f,
          "frontend load applies raw data into its retained actor rather than the placement bridge");
    data.reset();
    auto *actor=player.localPlayer();
    actor->inventorySource([](nativeai::InventorySlot slot) {slot.setCount(4); return false;});
    nativesaves::Character snapshot;
    check(!retained.expired() && player.playerCharacterSource()(snapshot) && !snapshot.inventory.empty() &&
          !snapshot.inventory[0].empty() && snapshot.inventory[0][0].count==4,
          "replaced CharacterData owns the canonical mutable inventory and survives the caller's temporary owner");
}
void absentRecord(const auto &check) {
    const auto dir = std::filesystem::temp_directory_path() /
        ("CubeVulkan-character-load-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(dir);
    {
        nativesaves::CharacterStore store;
        check(store.open(dir / "characters.db"), "private load database opens");
        nativesaves::Character current;
        current.name = "RETAIN"; current.health = -9.25f; current.position = {23,-44,61};
        const auto original = nativesaves::writeCharacter(current);
        check(!store.load(37, current) && nativesaves::writeCharacter(current) == original,
              "missing numeric lookup skips decode/reset and preserves the destination entity state");
    }
    std::filesystem::remove(dir / "characters.db");
    std::filesystem::remove(dir);
}
}
int characterLoadTests() {
    int failures = 0;
    const auto check = [&](bool okay, const char *message) {
        if (!okay) { std::printf("FAIL character load: %s\n", message); ++failures; }
    };
    rawNativeLoad(check); resetRetainsSeparateContainers(check); controlledInventoryUsesCurrentState(check); potionCountWidths(check); rawProgressOwnership(check); retainedPlayerAttachment(check); frontendLoadOwnership(check); absentRecord(check);
    std::printf("Character load: raw fields, placement, reset/load ownership: %d failure(s)\n", failures);
    return failures;
}
