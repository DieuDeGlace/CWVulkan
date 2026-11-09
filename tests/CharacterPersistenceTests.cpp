#include "CharacterPersistence.h"
#include "TaskPool.h"
#include "Extracted.h"
#include "NativeProgress.h"
#include "Spawned.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <array>
#include <bit>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <future>
#include <thread>

namespace {
using namespace characterpersistence;
struct State {
    uint32_t clock = 0;
    bool named = true, failed = false;
    std::array<int32_t, 3> visible{};
    int32_t slot = 0;
    unsigned captures = 0, writes = 0;
    std::vector<std::string> trace;
};
Session::Access access(const std::shared_ptr<State> &s) {
    return {
        [s] { s->trace.push_back("name"); return s->named; },
        [s](size_t i) { s->trace.push_back(std::to_string(i)); return s->visible[i]; },
        [s]() -> std::optional<Snapshot> {
            s->trace.push_back("capture"); ++s->captures;
            if (s->slot < 0) return {};
            Snapshot value; value.slot = s->slot; return value;
        },
        [s](Snapshot &&) { s->trace.push_back("write"); ++s->writes; return !s->failed; }
    };
}
void constructionDoesNotStartClock(const auto &check) {
    auto state = std::make_shared<State>();
    unsigned reads = 0;
    auto session = std::make_shared<Session>([&] { ++reads; return state->clock; });
    check(reads == 0, "constructing storage does not sample the generation worker clock");
    state->clock = 90000;
    session->bind(access(state));
    check(reads == 0, "binding initialized sources does not implicitly enter or rebase the worker");
    state->visible[0] = 1;
    session->finish();
}
void workerEntry(const auto &check) {
    auto state = std::make_shared<State>();
    const std::array<uint32_t,3> entry{100001,100007,100019};
    unsigned reads = 0;
    auto session = std::make_shared<Session>([&] {
        const unsigned at = reads++;
        return at < entry.size() ? entry[at] : state->clock;
    });
    check(reads == 0 && !session->beginCycle(), "unentered storage takes no clock readings or generation reservation");
    session->bind(access(state));
    state->visible = {1,1,1};
    session->start();
    check(reads == 3 && state->trace.empty(), "entry samples three distinct times without polling name or visibility");
    session->start();
    check(reads == 3, "repeated entry cannot sample or rebase the worker");
    state->visible = {};
    state->clock = 160007;
    check(!session->beginCycle(), "the second entry reading supplies the strict60000 boundary after delayed initialization");
    state->clock = 160008;
    auto cycle = session->beginCycle();
    check(bool(cycle), "the first overdue cycle is measured from the second entry reading");
    if (cycle) {
        state->clock = 400000; // completion latency must not replace the carried sample
        cycle->complete();
    }
    check(state->writes == 1, "the post-entry overdue attempt performs one write");
    const unsigned previousReads = reads;
    session->bind(access(state));
    session->start();
    check(reads == previousReads, "quiescent source rebinding does not restart an entered clock");
    state->clock = 220008;
    check(!session->beginCycle(), "completion and source rebinding preserve the earlier iteration baseline");
    state->visible[0] = 1;
    session->finish();

    auto stoppedState = std::make_shared<State>();
    unsigned stoppedReads = 0;
    auto stopped = std::make_shared<Session>([&] { ++stoppedReads; return uint32_t(71); });
    stopped->bind(access(stoppedState));
    stopped->requestStop();
    stopped->start();
    check(stoppedReads == 3 && !stopped->beginCycle(), "stop before entry still permits the three samples but no ordinary cycle");
    stopped->finish(); stopped->finish(); stopped->start();
    check(stoppedState->writes == 1 && stoppedReads == 3,
          "a stopped entered worker performs its gated final call once without restarting");

    auto absentState = std::make_shared<State>();
    auto absent = std::make_shared<Session>();
    absent->bind(access(absentState));
    check(absent->force() && absentState->writes == 1, "an explicit writer remains independent of worker entry");
    absent->finish(); absent->start();
    check(absentState->writes == 1 && !absent->beginCycle(), "a worker that never entered has no worker-exit call or later restart");
}
void timing(const auto &check) {
    auto s = std::make_shared<State>();
    auto session = std::make_shared<Session>([s] { return s->clock; });
    session->start();
    s->clock = 60001;
    check(!session->beginCycle(), "unbound/no-save state starts no save work");
    session->bind(access(s));
    s->clock = 60000;
    check(!session->beginCycle(), "exact unsigned60000 equality is excluded");
    s->clock = 60001;
    auto cycle = session->beginCycle();
    check(bool(cycle) && !session->beginCycle(), "one overdue generation cycle owns the pending save");
    s->clock = 90000;
    cycle->complete(); cycle->complete();
    check(s->writes == 1 && s->trace == std::vector<std::string>{"name","0","1","2","capture","write"},
          "sample carried through generation; one ordered gate/capture/write sequence");
    s->clock = 120001;
    check(!session->beginCycle(), "baseline uses the earlier60001 sample, not post-write time");
    s->clock = 120002;
    cycle = session->beginCycle();
    check(bool(cycle), "strict boundary is measured from the earlier completed-cycle sample");
    cycle.reset(); // cancelled work must not consume the baseline
    cycle = session->beginCycle();
    check(bool(cycle), "cancelled generation releases the reservation without advancing cadence");
    s->failed = true; cycle->complete();
    check(s->writes == 2 && session->takeErrors().size() == 1 && !session->beginCycle(),
          "returned write failure reports to owner and still advances the cycle baseline");
    s->clock = 180003; s->failed = false; s->slot = -1;
    cycle = session->beginCycle(); cycle->complete();
    check(s->captures == 3 && s->writes == 2 && !session->beginCycle(),
          "negative captured index writes nothing but the normal return advances the baseline");
    s->clock = 240004; s->slot = 0; s->named = false; s->trace.clear();
    cycle = session->beginCycle(); cycle->complete();
    check(s->trace == std::vector<std::string>{"name"}, "empty World name blocks before indexed visibility reads");
    s->named = true; s->visible = {-7,0,0}; s->trace.clear();
    cycle = session->beginCycle();
    check(bool(cycle), "name-gated attempt did not advance the baseline");
    cycle->complete();
    check(s->trace == std::vector<std::string>{"name","0"}, "any nonzero first visibility stops later operand reads");
    s->visible = {0,2,0}; s->trace.clear();
    cycle = session->beginCycle(); cycle->complete();
    check(s->trace == std::vector<std::string>{"name","0","1"}, "second guard blocks in the native owner order");
    s->visible = {}; s->trace.clear();
    cycle = session->beginCycle(); cycle->complete();
    check(s->writes == 3, "clearing menu gates immediately admits the still-overdue interval");
    session->requestStop(); s->clock += 60001;
    check(!session->beginCycle(), "stopping admits no new periodic cycle");
    s->named = false; s->trace.clear();
    session->finish(); session->finish();
    check(s->writes == 4 && s->trace == std::vector<std::string>{"0","1","2","capture","write"},
          "normal exit saves once with only the three visibility gates, even unnamed/stopped");
    s->visible = {1,1,1}; s->trace.clear();
    session->force();
    check(s->writes == 5 && s->trace == std::vector<std::string>{"capture","write"},
          "explicit writer calls ignore cadence/name/menu/running gates");
    auto wrapState = std::make_shared<State>(); wrapState->clock = UINT32_MAX - 1000;
    auto wrapped = std::make_shared<Session>([wrapState] { return wrapState->clock; });
    wrapped->start();
    wrapped->bind(access(wrapState)); wrapState->clock = 59000;
    auto wrapCycle = wrapped->beginCycle();
    check(bool(wrapCycle), "ordinary uint32 timer wrap preserves the60001 difference");
    wrapCycle->complete(); wrapState->visible[2] = 1;
    wrapped->finish();
    check(wrapState->writes == 1, "nonzero final visibility suppresses the exit writer");
}
void completedGeneration(const auto &check) {
    using namespace std::chrono_literals;
    auto s = std::make_shared<State>();
    auto session = std::make_shared<Session>([s] { return s->clock; });
    session->start();
    std::mutex world;
    std::atomic<int> generated{0};
    std::atomic<unsigned> writes{0};
    auto hooks = access(s);
    hooks.capture = [&]() -> std::optional<Snapshot> {
        std::lock_guard lock(world);
        check(generated.load() == 3, "character capture waits for every admitted generation job");
        Snapshot value; value.slot = 0; value.character.health = -0.0f; return value;
    };
    hooks.write = [&](Snapshot &&snapshot) {
        const bool released = world.try_lock();
        check(released, "serialization/write begins after the capture World entry is released");
        if (released) world.unlock();
        check(std::bit_cast<uint32_t>(snapshot.character.health) == 0x80000000u, "raw float bits survive the owned snapshot");
        ++writes; return true;
    };
    session->bind(std::move(hooks)); s->clock = 60001;
    auto batch = std::make_shared<Batch>(session->beginCycle());
    TaskPool pool(2);
    std::promise<void> release, entered;
    auto gate = release.get_future().share();
    batch->add();
    auto first = pool.submit([&] {
        JobDone done{batch};
        generated.fetch_or(1); entered.set_value();
    });
    batch->add();
    auto second = pool.submit([&] {
        JobDone done{batch};
        gate.wait(); generated.fetch_or(2);
    });
    entered.get_future().wait(); first.wait();
    check(writes == 0, "partial generation completion cannot trigger a character save");
    check(!batch->seal(), "one generation job still owns completion after the submission sentinel is removed");
    std::unique_lock held(world);
    release.set_value();
    check(second.wait_for(10ms) == std::future_status::timeout && writes == 0,
          "a worker waits for the current World owner rather than reading mutable data without its guard");
    held.unlock();
    check(second.wait_for(2s) == std::future_status::ready, "generation/save work finishes after the owner releases its guard");
    second.get();
    check(writes == 1, "last generation job performs one periodic save");
    s->visible[0] = 1; session->finish();
}
void finalSaveWaitsForGeneration(const auto &check) {
    using namespace std::chrono_literals;
    auto state = std::make_shared<State>();
    auto session = std::make_shared<Session>([state] { return state->clock; });
    session->start();
    auto hooks = access(state);
    std::atomic<bool> generated{false};
    hooks.capture = [&]() -> std::optional<Snapshot> {
        check(generated.load(), "final capture observes completed admitted generation");
        Snapshot value; value.slot = 0; return value;
    };
    session->bind(std::move(hooks)); state->clock = 60001;
    auto batch = std::make_shared<Batch>(session->beginCycle());
    TaskPool pool(1);
    std::promise<void> release;
    auto gate = release.get_future().share();
    batch->add();
    auto work = pool.submit([&] {
        JobDone done{batch};
        gate.wait(); generated = true;
    });
    check(!batch->seal(), "the admitted generation job owns shutdown completion");
    session->requestStop(); // establish the native stop-before-join order before releasing generation
    auto final = std::async(std::launch::async, [session] { session->finish(); });
    check(final.wait_for(10ms) == std::future_status::timeout && state->writes == 0,
          "stopping waits for admitted work before the once-only exit writer");
    release.set_value(); work.get(); final.get();
    check(state->writes == 1, "stop cancels the periodic attempt and retains exactly one final write");
}
void ownedDatabase(const auto &check) {
    struct Input {
        std::mutex world;
        int32_t slot = 0;
        nativesaves::Character character;
        std::string notes = "owned notes";
    };
    const auto folder = std::filesystem::temp_directory_path() /
        ("CubeVulkan-character-persistence-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(folder);
    const auto path = folder / "characters.db";
    auto store = std::make_shared<nativesaves::CharacterStore>();
    check(store->open(path), "private character test database opens");
    nativesaves::Character blank; blank.name = "before";
    check(store->create(blank) == 0 && store->create(blank) == 1, "two independent numeric character slots exist");
    auto input = std::make_shared<Input>();
    input->character.name = "captured";
    input->character.position = {INT64_MAX - 17, INT64_MIN + 19, -23};
    input->character.health = -0.0f;
    input->character.field9a4 = std::bit_cast<float>(0x3f812345u);
    input->character.inventory = {{{-5, nativesaves::makeItem(2,3)}}, {}, {{0, nativesaves::makeItem(4,5)}}};
    input->character.buffs.push_back({});
    input->character.buffs[0].bytes[23] = 77;
    const auto expected = nativesaves::writeCharacter(input->character);
    auto weak = std::weak_ptr(input);
    auto state = std::make_shared<State>();
    auto session = std::make_shared<Session>([state] { return state->clock; });
    session->start();
    auto hooks = access(state);
    hooks.capture = [input]() -> std::optional<Snapshot> {
        std::lock_guard lock(input->world);
        return Snapshot{input->slot, input->character, input->notes};
    };
    hooks.write = [input, store, &check](Snapshot &&value) {
        const bool released = input->world.try_lock();
        check(released, "real database sink runs outside the source World guard");
        if (released) input->world.unlock();
        input->slot = 1; input->character.position = {}; input->character.inventory.clear(); input->notes.clear();
        return store->save(value.slot, value.character) && store->writeNotes(value.slot, value.notes);
    };
    session->bind(std::move(hooks)); input.reset();
    check(!weak.expired(), "capture retains owned input after its original owner is gone");
    state->clock = 60001; session->beginCycle()->complete();
    std::vector<uint8_t> packed;
    check(store->database().read("0", packed) && nativesaves::inflate(packed) == expected,
          "captured numeric key/raw character payload remain independent of later slot/data changes");
    std::string notes;
    check(store->readNotes(0, notes) && notes == "owned notes", "supplemental notes belong to the same captured slot");
    nativesaves::Character untouched;
    check(store->load(1, untouched) && untouched.name == "before", "changing the live selection cannot redirect an admitted write");
    state->visible[0] = 1; session->finish(); session.reset();
    check(weak.expired(), "captured inputs release after the save session lifetime ends");
    store.reset();
    std::filesystem::remove(path);
    std::filesystem::remove(folder);
}
void liveOwnedCapture(const auto &check) {
    auto progress = std::make_shared<SaveGame>();
    nativesaves::Character base;
    base.name = "LIVE_TEST"; base.experience = 71;
    base.field9a4 = std::bit_cast<float>(0x3f876543u); base.field9a8 = -0.0f;
    base.homeCell = {-73, 19};
    base.inventory = {{{-7, nativesaves::makeItem(2, 3)}}, {}, {{0, nativesaves::makeItem(4, 5)}}};
    base.gold = 199; base.fieldAb0 = -8;
    base.buffs.push_back({}); base.buffs[0].bytes[23] = 0xf7;
    progress->nativeInventory = inventoryslots::PlayerState::fromCharacter(base);
    progress->inventoryAuthoritative = true;
    progress->hasPosition = true;
    progress->position = glm::vec3(std::numeric_limits<float>::infinity()); // ignored UI representation
    progress->level = 999; progress->xp = 888; progress->gold = 777;
    progress->regionHistory = std::vector<nativesaves::Cell>{{100, 200}}; // stale UI copy
    progress->quests.push_back({}); progress->quests.back().target = "owned quest";
    ExtractedWorld blocks;
    blocks.openGenerated(0, 0);
    std::function<bool(nativesaves::Character &)> actor;
    std::shared_ptr<sourcestates::Store> world;
    const std::array<int64_t, 3> fixed{INT64_MAX - 17, INT64_MIN + 19, -1234567};
    const std::array<float, 3> orientation{1.25f, -0.0f, -279.75f};
    nativesaves::Character result;
    {
        SpawnedCreatures spawned;
        spawned.setStoredZones(blocks, nullptr, false);
        spawned.setNativePlayer(true);
        actor = spawned.playerSaveSource();
        check(!actor(result), "an uninitialized owned player source cannot expose a stale/default actor");
        spawned.seedPlayerSave(std::make_shared<const nativesaves::Character>(base));
        spawned.setPlayer(blocks, {0,0,10}, 0, {}, 100, 1, true);
        auto *record = spawned.localPlayer()->record;
        std::memcpy(record + 0x10, fixed.data(), 24);
        std::memcpy(record + 0x28, orientation.data(), 12);
        const uint32_t negativeZero = 0x80000000u;
        std::memcpy(record + 0x180, &negativeZero, 4);
        spawned.setPlayerProgress(-3, INT32_MIN);
        record[0x14c] = 4; record[0x14d] = 1;
        for (size_t i = 0; i < 11 * nativesaves::ITEM_SIZE; ++i) record[0x278 + i] = uint8_t(i * 7);
        spawned.mapHistory().observe({-4, 5}, 0);
        spawned.mapHistory().observe({6, -7}, 0);
        world = blocks.sourceStateStore();
        auto guard = world->lock();
        result = base;
        std::string notes;
        check(nativeprogress::capturePlayerData(*progress, result, notes) && actor(result),
              "canonical data and current actor fields make one owned native snapshot under the World guard");
        check(notes.find("owned quest") != std::string::npos, "live engine quest sidecar survives data-only capture");
        check(result.position == fixed && result.orientation == orientation &&
              std::bit_cast<uint32_t>(result.orientation[1]) == negativeZero &&
              std::bit_cast<uint32_t>(result.health) == negativeZero && result.level == -3 && result.experience == INT32_MIN,
              "native capture preserves raw fixed/float/stat fields and ignores clamped/stale UI values");
        check(result.name == "LIVE_TEST" && result.homeCell == base.homeCell && result.buffs[0].bytes == base.buffs[0].bytes &&
              std::bit_cast<uint32_t>(result.field9a4) == 0x3f876543u &&
              std::bit_cast<uint32_t>(result.field9a8) == negativeZero,
              "loaded bookkeeping and whole live buff records reach the snapshot without float normalization");
        check(result.inventory.size() == 3 && result.inventory[0][0].count == -7 && result.inventory[2][0].count == 0 &&
              result.gold == 199 && result.fieldAb0 == -8 &&
              result.regionHistory == std::vector<nativesaves::Cell>{{6,-7},{-4,5}},
              "canonical slot layout/counters and owned current history replace stale UI summaries");
        check(std::memcmp(result.equipment.data(), record + 0x278, 11 * nativesaves::ITEM_SIZE) == 0,
              "whole current equipment records are copied without rebuilding item fields");
    }
    nativesaves::Character retained;
    auto guard = world->lock();
    check(actor(retained) && nativesaves::writeCharacter(retained).size() > 1000 &&
          retained.position == fixed && retained.regionHistory == result.regionHistory,
          "the live player source remains valid after its Actor facade is destroyed");
}
void nativeRosterWithoutRecording(const auto &check) {
    nativesaves::Character saved;
    saved.name = "NO_RECORDING"; saved.classType = 4; saved.specialization = 1; saved.level = 7;
    saved.looks.set(1, 1, 3, 5, {91, 127, 203});
    nativeprogress::SpriteTable sprites;
    CharacterEntry entry;
    check(nativeprogress::entryFor(saved, 3, {}, sprites, entry) && entry.native && entry.slot == 3 &&
          entry.name == saved.name && entry.classType == 4 && entry.specialization == 1 && entry.level == 7,
          "a native saved character remains selectable without any recorded character");
    check(entry.dir.empty(), "an absent recording is optional rather than a fabricated filesystem path");
    CharacterEntry recorded;
    recorded.classType = 4; recorded.specialization = 1; recorded.dir = "owned-optional-recording";
    check(nativeprogress::entryFor(saved, 3, {recorded}, sprites, entry) && entry.dir == recorded.dir,
          "a matching optional recording retains the existing playback adapter");
}
void databaseFacadeLifetime(const auto &check) {
    const auto folder = std::filesystem::temp_directory_path() /
        ("CubeVulkan-owned-character-db-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::shared_ptr<nativesaves::CharacterStore> store;
    std::shared_ptr<const int> selected;
    {
        nativeprogress::ProgressFile facade;
        std::string why;
        check(facade.open(folder, folder / "owner-game-Save", {}, true, {}), "private progress facade opens");
        nativesaves::Character c; c.name = "owned";
        const int index = facade.store().create(c);
        CharacterEntry entry; entry.native = true; entry.slot = index;
        facade.play(entry);
        store = facade.sharedStore(); selected = facade.selection();
    }
    check(store && store->isOpen() && selected && *selected == 0,
          "owned database and selected-slot storage survive ProgressFile facade destruction");
    nativesaves::Character snapshot; snapshot.name = "after facade";
    auto packed = nativesaves::compress(nativesaves::writeCharacter(snapshot));
    const std::string notes = "kept";
    check(store->savePacked(*selected, packed, &notes), "completed owned snapshot writes after the original facade is gone");
    nativesaves::Character got;
    check(store->load(0, got) && got.name == "after facade", "owned database contains the captured numeric record");
    store.reset();
    for (const auto &file : std::filesystem::directory_iterator(folder)) {
        if (file.is_regular_file()) std::filesystem::remove(file.path());
    }
    std::filesystem::remove(folder);
}
void flightPointOrder(const auto &check) {
    nativesaves::Character c;
    c.flightPoints = {{9,-5},{-7,8},{9,-5},{9,-9},{INT32_MIN,INT32_MAX}};
    auto normalized = c;
    normalized.flightPoints = {{INT32_MIN,INT32_MAX},{-7,8},{9,-9},{9,-5}};
    check(nativesaves::writeCharacter(c) == nativesaves::writeCharacter(normalized),
          "flight-point writer serializes the unique signed-pair set, independently of insertion order");
    // Start from a valid complete blob; patch only this unique five-record range
    // into an unsorted/duplicate complete input, preserving its byte count.
    auto source = c;
    source.flightPoints = {{100001,200001},{100002,200002},{100003,200003},{100004,200004},{100005,200005}};
    auto blob = nativesaves::writeCharacter(source);
    std::array<uint8_t, 40> pattern{}, replacement{};
    std::memcpy(pattern.data(), source.flightPoints.data(), pattern.size());
    std::memcpy(replacement.data(), c.flightPoints.data(), replacement.size());
    const auto found = std::search(blob.begin(), blob.end(), pattern.begin(), pattern.end());
    check(found != blob.end() && std::search(found + (found != blob.end() ? 40 : 0), blob.end(), pattern.begin(), pattern.end()) == blob.end(),
          "flight-point fixture uniquely locates its complete native record range");
    if (found == blob.end()) return;
    std::copy(replacement.begin(), replacement.end(), found);
    nativesaves::Character loaded;
    nativesaves::readCharacter(blob, loaded);
    check(loaded.flightPoints == normalized.flightPoints,
          "complete unsorted/duplicate flight-point input loads through signed-pair unique insertion");
    nativesaves::Character stars;
    stars.stars[{-1,0}] = {{17,-4097,1,7},{18,-4097,1,7},{19,-4097,1,6}};
    stars.stars[{0,-1}] = {{33,1,-4097,5},{22,1,-4097,4}};
    const auto starBlob = nativesaves::writeCharacter(stars);
    loaded = nativesaves::Character{}; // reader takes a fresh destination
    nativesaves::readCharacter(starBlob, loaded);
    check(nativesaves::writeCharacter(loaded) == starBlob && loaded.stars.at({-1,0}).size() == 3 &&
          loaded.stars.at({-1,0})[0].unknown == 17 && loaded.stars.at({-1,0})[2].unknown == 19 &&
          loaded.stars.at({0,-1})[0].unknown == 33,
          "star groups keep signed outer order, inner linked order and duplicate records");
    SaveGame progress;
    progress.nativeInventory = inventoryslots::PlayerState::fromCharacter(stars);
    progress.inventoryAuthoritative = true;
    progress.treasures = {{-4097,1,7},{-4097,1,7},{-4097,1,6},{1,-4097,5},{1,-4097,4}};
    auto captured = stars;
    std::string notes;
    check(nativeprogress::capturePlayerData(progress, captured, notes) && captured.stars == stars.stars,
          "live capture preserves each duplicate star's separate unknown value and list position");

}

}
int characterPersistenceTests() {
    int failures = 0;
    const auto check = [&](bool okay, const char *message) {
        if (!okay) { std::printf("FAIL character persistence: %s\n", message); ++failures; }
    };
    constructionDoesNotStartClock(check); workerEntry(check); timing(check); completedGeneration(check); finalSaveWaitsForGeneration(check); ownedDatabase(check);
    liveOwnedCapture(check); nativeRosterWithoutRecording(check); databaseFacadeLifetime(check); flightPointOrder(check);
    std::printf("Character persistence: cadence, gates, generation, snapshot ownership and database checks: %d failure(s)\n", failures);
    return failures;
}
