#include "GameFixtures.h"

#include "GameMath.h"
#include "GroundItems.h"
#include "NativeBuffs.h"
#include "NativeInteract.h"
#include "NpcSpeech.h"
#include "ShrineRespawn.h"
#include "TerrainRay.h"
#include "worldgen/Generator.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>

void GameFixtures::run() {
    notice();
    lamp();
    shrine();
    revive();
    overhead();
    combatText();
    frameBudget();
    maxHealth();
    doorSeat();
}

void GameFixtures::notice() {
    if (noticeTest && spawned.localPlayer() && nativeInventory && session.hostSide() && !session.guestElsewhere()) {
        // (testing: three gold and two platinum coins at the player's feet, laid by the host
        // world; every gain must arrive as its notice, with no chat line, phases 90-91)
        static int stage = 0;
        static double stageAt = 0;
        static int64_t gold = 0, platinum = 0;
        const auto platinumTotal = [&] {
            int64_t sum = 0;
            for (const auto &[zone, count] : saveGame.regionCounters)
                sum += count;
            return sum;
        };
        if (stage == 0 && time > 4.0 && hostHoldsPlayerZone()) {
            gold = playerGold, platinum = platinumTotal(), stage = 1, stageAt = time;
            auto coinsHold = session.lockHost();
            int64_t at[3];
            playerUnits(at);
            for (int k = 0; k < 5; k++) {
                const nativesaves::Item coin =
                    nativesaves::makeItem(grounditems::type::COIN, 0, k < 3 ? grounditems::COIN_MATERIAL : 0x0d);
                const int64_t spot[3] = {at[0] + k * 20000, at[1], at[2]};
                hostWorld.layItem(authorityTerrain(), coin.data(), spot, 1);
            }
            logLine("Notice test: three gold and two platinum coins laid by the host at " + std::to_string(time) + " s");
        }
        if (stage == 1) { // when each coin arrives (pick-up rate evidence)
            static int64_t seen = -1;
            const int64_t got = (playerGold - gold) + (platinumTotal() - platinum);
            if (got != seen) {
                if (seen >= 0)
                    logLine("Notice test: " + std::to_string(got) + " coin(s) after " + std::to_string((int)((time - stageAt) * 1000)) + " ms");
                seen = got;
            }
        }
        if (stage == 1 && ((playerGold - gold >= 3 && platinumTotal() - platinum >= 2) || time - stageAt > 30.0)) {
            // (each coin is its own queued pick-up interaction with the host, so the check
            // waits for all five, up to 30 s)
            const int64_t goldGain = playerGold - gold, platinumGain = platinumTotal() - platinum;
            const bool passed = goldGain == 3 && platinumGain == 2 && noticedGold == goldGain && noticedPlatinum == platinumGain &&
                                noticeChatLines == 0;
            logLine(std::string("Notice test: ") + (passed ? "PASS" : "FAIL") + " gold +" + std::to_string(goldGain) + " noticed " +
                    std::to_string(noticedGold) + ", platinum +" + std::to_string(platinumGain) + " noticed " +
                    std::to_string(noticedPlatinum) + ", chat lines " + std::to_string(noticeChatLines) + ", after " +
                    std::to_string((int)(time - stageAt)) + " s");
            if (!passed)
                exitCode = 1;
            noticeTest = false;
            glfwSetWindowShouldClose(window, 1);
        }
    }
}

void GameFixtures::lamp() {
    if (lampTest && spawned.localPlayer() && session.hostSide() && !session.guestElsewhere()) {
        // (testing: with the lamp set the wrong way for the time of day, an eagle flight
        // must set it by night and clear it by day, while ordinary play leaves it, phase 10)
        static int stage = 0;
        static double stageAt = 0;
        static bool expected = false;
        float abilityMs = 0;
        const int ability = spawned.playerAbility(abilityMs);
        const float dayMs = spawned.timeOfDay();
        if (stage == 0 && time > 5.0 && hostHoldsPlayerZone()) {
            expected = dayMs < 18000000.0f || dayMs >= 68400000.0f;
            spawned.setPlayerToggle(1, !expected);
            stage = 1, stageAt = time;
        } else if (stage == 1 && time - stageAt > 1.0) {
            const bool kept = ((spawned.playerToggles() & 1) != 0) == !expected; // ordinary play: untouched
            const int32_t x = (int32_t)std::floor(playerPosition.x) + (int32_t)(extractedWorld.originDotsX() >> 16) + 400;
            const int32_t y = (int32_t)std::floor(playerPosition.y) + (int32_t)(extractedWorld.originDotsY() >> 16) + 400;
            const bool off = spawned.startFlight(x, y);
            logLine(std::string("Lamp test: before the flight the lamp ") + (kept ? "kept" : "CHANGED") + "; flight " +
                    (off ? "off" : "not started") + " at " + std::to_string(dayMs) + " ms of day");
            stage = kept && off ? 2 : 3, stageAt = time;
        } else if ((stage == 2 && ability == 0x9c && abilityMs > 1000.0f) || (stage >= 2 && time - stageAt > 30.0) || stage == 3) {
            const bool lit = (spawned.playerToggles() & 1) != 0;
            const bool passed = stage == 2 && ability == 0x9c && lit == expected;
            logLine(std::string("Lamp test: ") + (passed ? "PASS" : "FAIL") + " ability " + std::to_string(ability) + " at " +
                    std::to_string(dayMs) + " ms of day, lamp " + (lit ? "on" : "off") + ", expected " + (expected ? "on" : "off"));
            if (!passed)
                exitCode = 1;
            lampTest = false;
            glfwSetWindowShouldClose(window, 1);
        }
    }
}

void GameFixtures::shrine() {
    if (shrineTest && spawned.localPlayer() && session.hostSide() && !session.guestElsewhere()) {
        // (testing: the start shrine is forgotten, the player returns to it (ability 0x75)
        // and plays the flute with the use key; ability 0x89 must register the shrine's
        // cell again at 5000 ms, shrine contract §3)
        static int stage = 0;
        static double stageAt = 0;
        static shrinerespawn::Cell home{};
        const uint8_t *me = spawned.playerRecord();
        float abilityMs = 0;
        const int ability = spawned.playerAbility(abilityMs);
        if (stage == 0 && time > 5.0 && hostHoldsPlayerZone() && me) {
            std::memcpy(home.data(), me + 0x1d0, sizeof home);
            // (a character whose home cell holds no shrine, such as the Alpha generation
            // save at 0,0, uses the nearest Shrine-of-Life site of its land instead, as
            // the revive's second pass finds it)
            if (worldgen::cellObject(home[0], home[1]) != 53) {
                int64_t at[3];
                playerUnits(at);
                shrinerespawn::Cell land{};
                std::memcpy(land.data(), me + worldgen::creature::REGION, sizeof land);
                shrinerespawn::World shrines;
                shrines.landOf = [](int32_t x, int32_t y) {
                    const worldgen::Int2 r = worldgen::regionAt(x, y);
                    return shrinerespawn::Cell{r.x, r.y};
                };
                shrines.isShrine = [](int32_t cx, int32_t cy) { return worldgen::cellObject(cx, cy) == 53; };
                const shrinerespawn::Cell site = shrinerespawn::choose(shrines, {}, at[0], at[1], land);
                logLine("Shrine test: home cell " + std::to_string(home[0]) + " " + std::to_string(home[1]) +
                        " holds no shrine; nearest site " + std::to_string(site[0]) + " " + std::to_string(site[1]) +
                        (worldgen::cellObject(site[0], site[1]) == 53 ? " is a shrine" : " is NOT a shrine"));
                home = site;
            }
            const bool forgot = usedCells.erase(home) > 0;
            const bool off = spawned.startShrineReturn(home[0], home[1]);
            logLine("Shrine test: start shrine cell " + std::to_string(home[0]) + " " + std::to_string(home[1]) +
                    (forgot ? " forgotten" : " was not known") + ", return " + (off ? "started" : "NOT started"));
            stage = off ? 1 : 9, stageAt = time;
        } else if (stage == 1 && time - stageAt > 5.0 && ability != 0x75 && hostHoldsPlayerZone()) {
            const auto &offer = spawned.vehicleItem();
            int32_t subtype;
            std::memcpy(&subtype, offer.data() + 4, sizeof subtype);
            int64_t at[3];
            playerUnits(at);
            std::string statues;
            const int zx = gamemath::floorFixedZone(at[0]), zy = gamemath::floorFixedZone(at[1]);
            for (int x = zx - 1; x <= zx + 1; ++x)
                for (int y = zy - 1; y <= zy + 1; ++y)
                    if (const auto *objects = spawned.zoneObjects(x, y))
                        for (const auto &o : *objects) {
                            int32_t type;
                            std::memcpy(&type, o.data(), 4);
                            if (type != 0)
                                continue;
                            int64_t p[3];
                            std::memcpy(p, o.data() + 8, sizeof p);
                            const double d = std::sqrt(std::pow((p[0] - at[0]) / 65536.0, 2) + std::pow((p[1] - at[1]) / 65536.0, 2) +
                                                       std::pow((p[2] - at[2]) / 65536.0, 2));
                            statues += " statue " + std::to_string((int)d) + " blocks, active " + std::to_string(o[0x28]) + ";";
                        }
            uint32_t flags = 0;
            if (me)
                std::memcpy(&flags, me + 0x5c, 4);
            logLine("Shrine test: use-key item type " + std::to_string(offer[0]) + " subtype " + std::to_string(subtype) +
                    ", ability " + std::to_string(ability) + ", contact " + std::to_string(flags & 3) + ";" + statues);
            autoUsePress = true;
            stage = 2, stageAt = time;
        } else if (stage == 2 && ability == 0x89) {
            logLine("Shrine test: playing the flute");
            stage = 3;
        } else if ((stage >= 2 && stage <= 3 && time - stageAt > 12.0) || stage == 9) {
            const bool known = usedCells.count(home) != 0;
            const bool passed = stage == 3 && known;
            logLine(std::string("Shrine test: ") + (passed ? "PASS" : "FAIL") + " flute " + (stage == 3 ? "played" : "not played") +
                    ", start shrine cell " + (known ? "registered" : "not registered"));
            if (!passed)
                exitCode = 1;
            shrineTest = false;
            glfwSetWindowShouldClose(window, 1);
        }
    }
}

void GameFixtures::revive() {
    if (reviveTest && spawned.localPlayer() && session.hostSide() && !session.guestElsewhere()) {
        // (testing: the player's health is set to 0; it must stay dead with "Revive" on the
        // use key, and the press must revive it healed at the cell 0x2C2860 chooses, with
        // +0x140 at -0.5 and a tombstone where it died, shrine contract §6)
        static int stage = 0;
        static double stageAt = 0;
        static int64_t died[3], pressedAt[3];
        static shrinerespawn::Cell expected{};
        static bool anyKnown = false;
        const uint8_t *me = spawned.playerRecord();
        int64_t at[3];
        playerUnits(at);
        const auto blocksApart = [](const int64_t *a, double x, double y) {
            return std::hypot(a[0] / 65536.0 - x, a[1] / 65536.0 - y);
        };
        if (stage == 0 && time > 5.0 && hostHoldsPlayerZone() && me && playerHp > 0 && playerMaxHp > 0) {
            shrinerespawn::Cell land{};
            std::memcpy(land.data(), me + worldgen::creature::REGION, sizeof land);
            shrinerespawn::World shrines;
            shrines.landOf = [](int32_t x, int32_t y) {
                const worldgen::Int2 r = worldgen::regionAt(x, y);
                return shrinerespawn::Cell{r.x, r.y};
            };
            shrines.isShrine = [](int32_t cx, int32_t cy) { return worldgen::cellObject(cx, cy) == 53; };
            for (const auto &c : usedCells)
                if (shrines.isShrine(c[0], c[1]) && shrines.landOf(c[0] * 16 + 8, c[1] * 16 + 8) == land)
                    anyKnown = true;
            std::memcpy(died, at, sizeof died);
            expected = shrinerespawn::choose(shrines, usedCells, at[0], at[1], land);
            logLine("Revive test: dying at " + std::to_string(at[0] >> 16) + " " + std::to_string(at[1] >> 16) +
                    ", land " + std::to_string(land[0]) + " " + std::to_string(land[1]) + ", " +
                    (anyKnown ? "a known shrine" : "no known shrine") + " in the land, expected cell " +
                    std::to_string(expected[0]) + " " + std::to_string(expected[1]) +
                    (worldgen::cellObject(expected[0], expected[1]) == 53 ? " (a shrine)" : " (NOT a shrine)"));
            spawned.setPlayerHealth(0);
            stage = 1, stageAt = time;
        } else if (stage == 1 && time - stageAt > 3.0) {
            const bool dead = playerHp <= 0;
            const bool prompt = usePrompt.action == useprompt::Action::Revive && usePrompt.text == "Revive";
            const double moved = blocksApart(at, died[0] / 65536.0, died[1] / 65536.0);
            logLine(std::string("Revive test: after 3 s ") + (dead ? "still dead" : "ALIVE") + ", use key '" + usePrompt.text +
                    "', moved " + std::to_string(moved) + " blocks");
            if (dead && prompt) {
                std::memcpy(pressedAt, at, sizeof pressedAt);
                autoUsePress = true;
                stage = 2, stageAt = time;
            } else
                stage = 9;
        } else if (stage == 2 && playerHp > 0 && me) {
            float revived = 0;
            std::memcpy(&revived, me + 0x140, sizeof revived);
            // (§6.2: the position becomes exactly (cx*16+6, cy*16+8, cell level) in fixed point)
            const int64_t point[3] = {((int64_t)expected[0] * 16 + 6) * 65536, ((int64_t)expected[1] * 16 + 8) * 65536,
                                      (int64_t)worldgen::cellHeight(expected[0], expected[1]) * 65536};
            const bool exact = at[0] == point[0] && at[1] == point[1] && at[2] == point[2];
            logLine("Revive test: record at " + std::to_string(at[0]) + " " + std::to_string(at[1]) + " " + std::to_string(at[2]) +
                    ", respawn point " + std::to_string(point[0]) + " " + std::to_string(point[1]) + " " + std::to_string(point[2]) +
                    (exact ? " (exact)" : " (DIFFERENT)"));
            const bool healed = playerHp == playerMaxHp;
            // (the tombstone contract: the anchor is the body's exact 64-bit position when Revive
            // was accepted, the press frame's or the next, never the destination or a whole block)
            const mapmarks::Tombstone tomb = spawned.tombstone();
            const bool anchored = lastReviveCorpse && tomb.at == *lastReviveCorpse &&
                                  std::abs(tomb.at[0] - pressedAt[0]) < 65536 / 4 &&
                                  std::abs(tomb.at[1] - pressedAt[1]) < 65536 / 4 &&
                                  std::abs(tomb.at[2] - pressedAt[2]) < 65536 / 4 &&
                                  !(tomb.at[0] == point[0] && tomb.at[1] == point[1]);
            const bool timed = tomb.ms > mapmarks::TOMBSTONE_MS - 2000 && tomb.ms <= mapmarks::TOMBSTONE_MS;
            const bool tombOk = anchored && (timed || tomb.ms == 0); // (zero: the destination is within 20 blocks)
            logLine("Revive test: revived, +0x140 " + std::to_string(revived) + ", health " + std::to_string(playerHp) + " of " +
                    std::to_string(playerMaxHp) + ", tombstone at " + std::to_string(tomb.at[0]) + " " + std::to_string(tomb.at[1]) +
                    " " + std::to_string(tomb.at[2]) + " with " + std::to_string(tomb.ms) + " ms" +
                    (tombOk ? " (the accepted body)" : " (WRONG)"));
            stage = exact && revived < 0 && revived >= -0.5f && healed && tombOk ? 3 : 9, stageAt = time;
        } else if (stage == 3 && time - stageAt > 1.0) {
            // (after a second of the world's own physics the player still stands there)
            const double apart = blocksApart(at, expected[0] * 16.0 + 6, expected[1] * 16.0 + 8);
            const bool shrine = worldgen::cellObject(expected[0], expected[1]) == 53;
            // (the World steps keep the tombstone beyond 20 blocks and clear it within; a second
            // of steps takes about 1000 ms off)
            const mapmarks::Tombstone tomb = spawned.tombstone();
            double d2 = 0;
            for (int i = 0; i < 3; i++)
                d2 += std::pow((double)(tomb.at[i] - at[i]) / 65536.0, 2);
            const bool clear = std::abs(std::sqrt(d2) - 20.0) > 0.5;
            const bool kept = tomb.ms > 0 && tomb.ms < mapmarks::TOMBSTONE_MS - 900;
            const bool tombOk = clear && (d2 < 400 ? tomb.ms == 0 : kept);
            const bool passed = apart < 1.0 && shrine && tombOk;
            logLine(std::string("Revive test: ") + (passed ? "PASS" : "FAIL") + " standing " + std::to_string(apart) +
                    " blocks from the respawn point of cell " + std::to_string(expected[0]) + " " + std::to_string(expected[1]) +
                    (shrine ? ", a Shrine of Life" : ", not a shrine") + (anyKnown ? " (known shrine, pass 1)" : " (shrine site, pass 2)") +
                    ", tombstone " + std::to_string(std::sqrt(d2)) + " blocks away with " + std::to_string(tomb.ms) + " ms" +
                    (tombOk ? "" : " (WRONG)"));
            if (!passed)
                exitCode = 1;
            reviveTest = false;
            glfwSetWindowShouldClose(window, 1);
        }
        if (stage == 9 || (stage >= 1 && stage <= 2 && time - stageAt > 20.0)) {
            logLine("Revive test: FAIL at stage " + std::to_string(stage));
            exitCode = 1;
            reviveTest = false;
            glfwSetWindowShouldClose(window, 1);
        }
    }
}

void GameFixtures::overhead() {
    if (overheadTest && spawned.localPlayer() && spawned.playerRecord()) {
        // (testing: a buff-7 heart of strength 1 on the player draws 0xdf0 at 1.5 in white; making the
        // player the selected collection member turns it orange; without the buff the orange 0xdc2
        // marker remains, overhead-models contract §1)
        static int stage = 0;
        static double stageAt = 0;
        nativeai::Creature &me = *spawned.localPlayer();
        const int64_t id = spawned.playerId();
        const auto drawnFor = [&]() -> const OverheadDrawn * {
            for (const auto &d : overheadDrawn)
                if (d.id == id) return &d;
            return nullptr;
        };
        const auto expect = [&](int model, float scale, glm::vec4 material, const char *what) {
            const OverheadDrawn *d = drawnFor();
            const bool good = d && d->model == model && d->scale == scale && d->material == material;
            logLine(std::string("Overhead test: ") + what + (good ? " PASS" : " FAIL") +
                    (d ? " model " + std::to_string(d->model) + " scale " + std::to_string(d->scale) + " material " +
                             std::to_string(d->material.r) + "," + std::to_string(d->material.g) + "," +
                             std::to_string(d->material.b)
                       : std::string(" nothing drawn")));
            return good;
        };
        const glm::vec4 orange = npcspeech::SELECTED_MATERIAL;
        // (an overhead model is drawn only with its admitted body, so start once the body is drawn)
        if (stage == 0 && time > 5.0 && hostHoldsPlayerZone() && playerHp > 0 && spawned.playerParts() > 0) {
            nativebuffs::give(me, nativebuffs::make(7, 1.f, 600000.f, id), false);
            stage = 1, stageAt = time;
        } else if (stage == 1 && time - stageAt > .5) {
            stage = expect(0xdf0, 1.5f, glm::vec4(1), "heart") ? 2 : 9, stageAt = time;
            creatureCollection.add(spawned.playerRecord());
            creatureCollection.selectedId = id;
        } else if (stage == 2 && time - stageAt > .5) {
            stage = expect(0xdf0, 1.5f, orange, "selected heart") ? 3 : 9, stageAt = time;
            std::erase_if(me.buffs, [](const nativebuffs::Buff &b) { return b[0] == 7; });
        } else if (stage == 3 && time - stageAt > .5) {
            const bool marker = expect(0xdc2, 1.f, orange, "selected collection marker");
            size_t markers = 0;
            for (const auto &d : overheadDrawn) markers += d.id != id;
            // (0xd3170: each model is drawn inside its actor's admitted route, after the body,
            // so it shares that actor's queue position and sphere; culled actors draw none)
            const bool queued = overheadQueue.models > 0 && overheadQueue.withBody == overheadQueue.models;
            const bool passed = marker && queued;
            logLine(std::string("Overhead test: ") + (passed ? "PASS" : "FAIL") + ", " + std::to_string(markers) +
                    " other actors drew an overhead model, " + std::to_string(overheadQueue.withBody) + " of " +
                    std::to_string(overheadQueue.models) + " queued with their admitted body");
            creatureCollection.remove(id);
            creatureCollection.selectedId = 0;
            if (!passed) exitCode = 1;
            overheadTest = false;
            glfwSetWindowShouldClose(window, 1);
        }
        if (stage == 9 || (stage == 0 && time > 60.0)) {
            logLine("Overhead test: FAIL at stage " + std::to_string(stage));
            exitCode = 1;
            overheadTest = false;
            glfwSetWindowShouldClose(window, 1);
        }
    }
}

void GameFixtures::combatText() {
    if (combatTextTest == 1 && spawned.localPlayer() && spawned.playerRecord() && time > 5.0 && hostHoldsPlayerZone()) {
        // (testing: three hit records on the player join this frame's hit list: a 12.75 hit
        // with a stun, a block and a 0.35 heal; phase 68 must make "K.O.", "12", "BLOCK"
        // and "0.3" drawn this same frame, combat-text contract §§3.2-3.3)
        const auto hitOnPlayer = [&](float amount, uint8_t kind, int32_t stun) {
            cwnet::DamageRecord r{};
            const int64_t target = spawned.playerId();
            std::memcpy(r.data() + 0x08, &target, 8);
            std::memcpy(r.data() + 0x10, &amount, 4);
            std::memcpy(r.data() + 0x18, &stun, 4);
            std::memcpy(r.data() + 0x20, spawned.playerRecord() + 0x10, 24);
            r[0x45] = kind;
            hitVisuals.push_back(r);
        };
        hitOnPlayer(12.75f, 0, 500), hitOnPlayer(5.0f, 1, 0), hitOnPlayer(-0.35f, 0, 0);
        logLine("Combat text test: three hit records on the player");
        combatTextTest = 2;
    }
}

void GameFixtures::frameBudget() {
    if (frameBudgetTest > 0 && spawned.localPlayer()) {
        // (testing: with an N ms budget, every frame sleeps before present and no frame is
        // shorter than its budget on average, frame driver §2.3)
        static double startedAt = -1;
        static uint32_t startTick = 0;
        static uint64_t startFrames = 0, startBeforePresent = 0;
        if (startedAt < 0 && time > 4.0)
            startedAt = time, startTick = frameEntryTick, startFrames = pacedFrames, startBeforePresent = pacedBeforePresent;
        if (startedAt >= 0 && time - startedAt > 6.0) {
            const uint64_t frames = pacedFrames - startFrames, before = pacedBeforePresent - startBeforePresent;
            const double mean = frames ? double(frameEntryTick - startTick) / double(frames) : 0.0;
            const bool passed = frames > 0 && before == frames && mean >= frameBudgetTest - 0.5;
            char line[200];
            std::snprintf(line, sizeof line, "Frame budget: %s %llu frames, %llu slept before present, mean %.2f ms for a %d ms budget",
                          passed ? "PASS" : "FAIL", (unsigned long long)frames, (unsigned long long)before, mean, frameBudgetTest);
            logLine(line);
            if (!passed)
                exitCode = 1;
            frameBudgetTest = 0;
            glfwSetWindowShouldClose(window, 1);
        }
    }
}

void GameFixtures::maxHealth() {
    if (maxHealthTest && spawned.localPlayer()) {
        // (testing: a health buff that doubles the maximum, with health full, runs out inside
        // the world batch; the player's health must follow the maximum down, Game update phase 27)
        static int stage = 0;
        static double startedAt = -1, stageAt = 0;
        static float maximum = 0;
        if (startedAt < 0)
            startedAt = time;
        auto *me = spawned.localPlayer();
        const auto hasBuff = [&] {
            for (const auto &b : me->buffs)
                if (b[0] == 0x16)
                    return true;
            return false;
        };
        auto finish = [&](bool passed, const std::string &why) {
            logLine(std::string("Max health: ") + (passed ? "PASS " : "FAIL ") + why);
            if (!passed)
                exitCode = 1;
            maxHealthTest = false;
            glfwSetWindowShouldClose(window, 1);
        };
        if (stage == 0 && time - startedAt > 4.0 && spawned.playerHealth() > 0 && !hasBuff()) {
            maximum = spawned.playerMaxHealth();
            nativebuffs::Buff b{};
            b[0] = 0x16;
            const float value = 2.0f, duration = 300.0f;
            std::memcpy(b.data() + 4, &value, 4);
            std::memcpy(b.data() + 8, &duration, 4);
            me->buffs.push_back(b);
            const float doubled = spawned.playerMaxHealth();
            spawned.setPlayerHealth(doubled);
            logLine("Max health: buff added, maximum " + std::to_string(maximum) + " -> " + std::to_string(doubled) + ", health set full");
            stageAt = time;
            stage = doubled == 2 * maximum ? 1 : 3;
            if (stage == 3)
                finish(false, "the buff did not double the maximum");
        } else if (stage == 1) {
            if (!hasBuff()) {
                const float hp = spawned.playerHealth(), most = spawned.playerMaxHealth();
                const std::string values = "health " + std::to_string(hp) + ", maximum " + std::to_string(most);
                if (most != maximum)
                    finish(false, "the maximum did not return after the buff (" + values + ")");
                else
                    finish(hp == most, "health followed the maximum down when the buff ran out (" + values + ")");
            } else if (time - stageAt > 5.0)
                finish(false, "the buff never ran out");
        }
    }
}

void GameFixtures::doorSeat() {
    if (doorSeatTest && session.hostSide() && !session.guestElsewhere() && spawned.localPlayer()) {
        // (testing: the player's use of a door, twice, and of a seat goes to the host as
        // interactions, and the host's changed zone comes back to the viewed World)
        namespace ni = nativeinteract;
        static int stage = 0;
        static double stageAt = 0, startedAt = -1;
        static int frames = 0, stageFrame = 0;
        ++frames;
        struct Spot { int zx = 0, zy = 0, index = -1; };
        static Spot door, seat;
        static uint8_t doorBefore = 0;
        if (startedAt < 0)
            startedAt = time;
        auto viewed = [&](const Spot &at) -> const nativeai::Object * {
            const auto *objects = spawned.zoneObjects(at.zx, at.zy);
            return objects && at.index >= 0 && at.index < (int)objects->size() ? &(*objects)[(size_t)at.index] : nullptr;
        };
        auto press = [&](const Spot &at) {
            nativeai::Interaction in = ni::fresh(ni::USE_OBJECT);
            std::memcpy(in.data() + ni::ZONE_X, &at.zx, 4);
            std::memcpy(in.data() + ni::ZONE_Y, &at.zy, 4);
            std::memcpy(in.data() + ni::OBJECT, &at.index, 4);
            spawned.localPlayer()->interactions.push_back(in); // as the use key hands it over
            stageAt = time, stageFrame = frames;
        };
        auto finish = [&](bool passed, const std::string &why) {
            logLine(std::string("Door seat: ") + (passed ? "PASS " : "FAIL ") + why);
            if (!passed)
                exitCode = 1;
            doorSeatTest = false;
            glfwSetWindowShouldClose(window, 1);
        };
        const auto type = [](const nativeai::Object &o) { int32_t t; std::memcpy(&t, o.data() + ni::OBJECT_TYPE, 4); return t; };
        const auto user = [](const nativeai::Object &o) { int64_t u; std::memcpy(&u, o.data() + ni::OBJECT_USER, 8); return u; };
        const auto isDoor = [&](const nativeai::Object &o) {
            const int32_t t = type(o);
            return o[ni::OBJECT_ITEM] == 0 && (t == 3 || t == 4 || t == 5 || t == 6 || t == 0x54 || t == 0x55 || t == 0x56 || t == 0x57 || t == 0x69);
        };
        const auto isSeat = [&](const nativeai::Object &o) {
            const int32_t t = type(o);
            return user(o) == 0 && (t == 0x1c || t == 0x1e || t == 0x1f || t == 0x73);
        };
        const auto took = [&] { return std::to_string((int)((time - stageAt) * 1000)) + " ms, " + std::to_string(frames - stageFrame) + " frames"; };
        const bool late = time - stageAt > 3.0;
        if (stage == 0 && time - startedAt > 4.0) {
            // The nearest zones (in rings) with a plain door and with a free seat, the same
            // list on both sides.
            int64_t dots[3];
            std::memcpy(dots, spawned.localPlayer()->record + worldgen::creature::POSITION, sizeof dots);
            const auto zoneOfDots = [](int64_t v) { const int32_t b = terrainray::blockCoordinate(v); return b / 64 - (b < 0 && b % 64 != 0 ? 1 : 0); };
            const int px = zoneOfDots(dots[0]), py = zoneOfDots(dots[1]);
            door = seat = {};
            std::map<int32_t, int> seen; // (for a failure: the object types near the player)
            for (int r = 0; r <= 6 && (door.index < 0 || seat.index < 0); r++)
                for (int dx = -r; dx <= r; dx++)
                    for (int dy = -r; dy <= r; dy++) {
                        if (std::max(std::abs(dx), std::abs(dy)) != r)
                            continue;
                        const auto *objects = spawned.zoneObjects(px + dx, py + dy);
                        for (int i = 0; objects && i < (int)objects->size(); i++) {
                            ++seen[type((*objects)[(size_t)i])];
                            if (door.index < 0 && isDoor((*objects)[(size_t)i]))
                                door = {px + dx, py + dy, i};
                            if (seat.index < 0 && isSeat((*objects)[(size_t)i]))
                                seat = {px + dx, py + dy, i};
                        }
                    }
            if (door.index < 0 || seat.index < 0) {
                if (time - startedAt > 30.0)
                    {
                    std::string types;
                    for (const auto &[t, n] : seen)
                        types += " " + std::to_string(t) + "x" + std::to_string(n);
                    finish(false, std::string("no zone within 6 of the player has a ") + (door.index < 0 ? "plain door" : "free seat") + "; types seen:" + types);
                }
            } else {
                logLine("Door seat: door " + std::to_string(door.index) + " in zone " + std::to_string(door.zx) + ", " + std::to_string(door.zy) +
                        ", seat " + std::to_string(seat.index) + " in zone " + std::to_string(seat.zx) + ", " + std::to_string(seat.zy) +
                        (session.hostThreadRunning() ? " (host thread running)" : " (no host thread)"));
                doorBefore = (*viewed(door))[ni::OBJECT_STATE];
                press(door);
                stage = 1;
            }
        } else if (stage == 1) {
            if (viewed(door) && (*viewed(door))[ni::OBJECT_STATE] != doorBefore) {
                logLine(std::string("Door seat: PASS the first press ") + (doorBefore ? "opened" : "shut") + " the door in the viewed World after " + took());
                press(door);
                stage = 2;
            } else if (late)
                finish(false, "the first door press never reached the viewed World");
        } else if (stage == 2) {
            if (viewed(door) && (*viewed(door))[ni::OBJECT_STATE] == doorBefore) {
                logLine("Door seat: PASS the second press put the door back after " + took());
                press(seat);
                stage = 3;
            } else if (late)
                finish(false, "the second door press never reached the viewed World");
        } else if (stage == 3) {
            if (viewed(seat) && user(*viewed(seat)) == spawned.playerId()) {
                logLine("Door seat: PASS the seat's user is the player in the viewed World after " + took());
                auto hold = session.lockHost();
                const auto hostObject = [&](const Spot &at) -> const nativeai::Object * {
                    const auto *objects = hostWorld.zoneObjects(at.zx, at.zy);
                    return objects && at.index < (int)objects->size() ? &(*objects)[(size_t)at.index] : nullptr;
                };
                const nativeai::Object *hostDoor = hostObject(door), *hostSeat = hostObject(seat);
                finish(hostDoor && hostSeat && (*hostDoor)[ni::OBJECT_STATE] == doorBefore && user(*hostSeat) == spawned.playerId(),
                       "the host World holds the same door and seat");
            } else if (late)
                finish(false, "the seat press never reached the viewed World");
        }
    }
}
