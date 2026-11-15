#pragma once
// The game-run test scenarios (--notice-test, --lamp-test, --shrine-test, --revive-test,
// --overhead-test, --combat-text-test, --frame-budget-test, --max-health-test and
// --door-seat-test). Each drives the live session through its ordinary inputs, logs
// "PASS" or "FAIL", sets the exit code and closes the window. Testing only: the frame
// builds GameFixtures and calls run() only while one of the switches is set.
#include "Multiplayer.h"
#include "Saves.h"
#include "Spawned.h"
#include "UseCreatureCollection.h"
#include "UsePrompt.h"

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <vector>

struct GLFWwindow;

// The last frame's overhead submissions and how many sat in the actor pass beside a body part
// of the same admitted actor (recorded by the frame for --overhead-test).
struct OverheadDrawn {
    int64_t id;
    int model;
    float scale;
    glm::vec4 material;
};
struct OverheadQueue {
    size_t models = 0, withBody = 0;
};

// The session state the scenarios read and drive, by reference into the running session.
struct GameFixtures {
    // The scenario switches; a scenario clears its own when it ends.
    bool &noticeTest, &lampTest, &shrineTest, &reviveTest, &overheadTest;
    int &combatTextTest, &frameBudgetTest;
    bool &maxHealthTest, &doorSeatTest;
    // The worlds and the session.
    SpawnedCreatures &spawned, &hostWorld;
    Multiplayer &session;
    ExtractedWorld &extractedWorld;
    SaveGame &saveGame;
    bool nativeInventory;
    usecollection::Collection &creatureCollection;
    useprompt::Prompt &usePrompt;
    std::set<std::array<int32_t, 2>> &usedCells;
    std::vector<cwnet::DamageRecord> &hitVisuals;
    // The player and what the frame records about it.
    const glm::vec3 &playerPosition;
    float &playerHp, &playerMaxHp;
    int &playerGold;
    int64_t &noticedGold, &noticedPlatinum;
    int &noticeChatLines;
    std::optional<std::array<int64_t, 3>> &lastReviveCorpse;
    std::vector<OverheadDrawn> &overheadDrawn;
    OverheadQueue &overheadQueue;
    // The frame: its clock, pacing counters, input and outcome.
    double time;
    uint32_t frameEntryTick;
    uint64_t &pacedFrames, &pacedBeforePresent;
    bool &autoUsePress;
    int &exitCode;
    GLFWwindow *window;
    std::function<void(const std::string &)> logLine;
    std::function<void(int64_t *)> playerUnits;
    std::function<bool()> hostHoldsPlayerZone;
    std::function<ExtractedWorld &()> authorityTerrain;

    // The scenarios, in the frame's order.
    void run();

  private:
    // A scenario's end: a failure sets the exit code; its switch goes off and the window closes.
    void end(bool passed, bool &on);
    void end(bool passed, int &on);
    void notice();
    void lamp();
    void shrine();
    void revive();
    void overhead();
    void combatText();
    void frameBudget();
    void maxHealth();
    void doorSeat();
};
