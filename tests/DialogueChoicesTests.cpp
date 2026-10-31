#include "DialogueChoices.h"
#include "DialoguePanel.h"
#include "NativeProgress.h"
#include "NativeControl.h"
#include "NativePlayer.h"
#include <cstring>
#include <cmath>
#include "Hud.h"
#include "Speech.h"
#include "Assets.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>

int dialogueChoicesTests() {
    int failures = 0;
    auto check = [&](bool ok, const char *what) {
        if (!ok) { ++failures; std::cerr << "Dialogue choices: " << what << '\n'; }
    };
    using namespace dialoguechoice;
    Panel panel;
    auto offer = [&](std::string id) {
        panel.offer(42, {{std::move(id), {{L"translated label", glm::vec4(1)}}}});
        panel.updateVisibility(false);
        const auto layout = panel.layout({800,600}, [](const std::wstring &) { return 30.9f; });
        panel.updateHover(layout, true, 1);
    };
    for (float time : {21600000.f, 64800000.f}) {
        offer("rest_inn?");
        const auto action = panel.release(true, 10, time, 0);
        check(action.kind == Action::Rest && action.coins == 10 && action.speaker == 42 && panel.empty(),
              "rest permits both inclusive night endpoints and debits once");
        check(panel.release(true, 100, time, 0).kind == Action::None, "a second release cannot pay again");
    }
    for (float time : {21600002.f, 64799996.f}) {
        offer("rest_inn?");
        check(panel.release(true, 100, time, 0).kind == Action::None && panel.empty(),
              "daytime failure consumes the offered answers");
    }
    offer("rest_inn?");
    check(panel.release(true, 9, 0, 0).kind == Action::None && panel.empty(), "insufficient coins consume answers");
    offer("change_specialization?");
    const auto changed = panel.release(true, 0, 0, 0);
    check(changed.kind == Action::ChangeSpecialization && changed.specialization == 1, "action uses id, not translated label");
    offer("change_specialization?");
    check(panel.release(true, 0, 0, 2).specialization == 255, "specialization assignment retains native byte arithmetic");
    for (const auto *id : {"change_specialization?", "rest_inn?", "goodbye?", "unknown?"}) {
        offer(id);
        check(panel.release(false, 100, 0, 0).kind == Action::None && panel.empty(), "missing speaker still consumes all answers");
    }
    offer("rest_inn?");
    panel.updateHover({}, false, 0);
    check(panel.release(true, 100, 0, 0).kind == Action::None && panel.empty(), "outside-row release consumes answers");
    offer("rest_inn?");
    panel.updateVisibility(true);
    panel.dismissForMovement();
    check(!panel.visible() && !panel.empty(), "an addressed bubble hides answers without movement dismissal");
    check(panel.release(true, 100, 0, 0).kind == Action::None && !panel.empty(), "hidden answers ignore mouse release");
    panel.updateVisibility(false);
    const auto movementLayout = panel.layout({800, 600}, [](const std::wstring &) { return 10.f; });
    check(!movementLayout.rows.empty(), "movement fixture has a visible answer row");
    panel.updateHover(movementLayout, true, movementLayout.rows.empty() ? 0 : movementLayout.rows[0].top + 1);
    const int retainedHover = panel.hovered();
    check(retainedHover == 0, "movement fixture starts with a selected answer");
    const int64_t retainedSpeaker = panel.speaker();
    panel.dismissForMovement();
    check(panel.empty() && !panel.visible(), "movement dismisses visible answers");
    check(panel.hovered() == retainedHover && panel.speaker() == retainedSpeaker,
          "movement clears pages without resetting the speaker or hover cache");

    panel = Panel{}; // the geometry case starts with the native constructor width 300
    panel.offer(7, {{"a", {{L"long"}, {L"long"}}}, {"b", {{L"x"}}}});
    panel.updateVisibility(false);
    const auto layout = panel.layout({800,600}, [](const std::wstring &word) { return word == L"long" ? 150.9f : 10.f; });
    check(layout.size == glm::vec2(200,102) && layout.at == glm::vec2(300,348), "native two-pass dimensions and bottom placement");
    check(layout.rows.size() == 2 && layout.rows[0].next == 48 && layout.rows[0].measuredHeight == 50,
          "measurement and drawing retain different wrap advances");
    panel.updateHover(layout, true, 49);
    check(panel.hovered() == 1, "later overlapping measured row replaces hover");
    check(layout.navigation == std::vector<glm::vec2>{{100,32},{100,72}}, "rows register native center navigation points");

    dialoguewords::Builder words;
    words.literal(L"don't! well-known 18=:=00", {});
    const auto &tokens = words.pages[0];
    check(tokens.size() == 6 && tokens[0].text == L"don't" && tokens[1].text == L"!" &&
          tokens[3].text == L"-" && tokens[5].text == L"18:00", "literal lexer, apostrophes and glue preserve native words");
    dialoguewords::Builder substituted;
    substituted.fragment(L"a-b.", {});
    check(substituted.pages[0].size() == 1 && substituted.pages[0][0].text == L"a-b.", "substitution punctuation is not re-lexed");
    dialoguewords::Builder empty;
    empty.fragment(L"", {});
    check(empty.pages[0].size() == 1 && empty.pages[0][0].text.empty(), "native empty-fragment extraction is retained");
    dialoguewords::Builder joined;
    joined.literal(L"owner", {{1,1,0,1}, 1});
    joined.literal(L"'s", {{0,0,1,1}, 0});
    check(joined.pages[0].size() == 1 && joined.pages[0][0].text == L"owner's" &&
          joined.pages[0][0].colour == glm::vec4(1,1,0,1) && joined.pages[0][0].flag == 1,
          "concatenation preserves the preceding word's metadata");

    const auto root = std::filesystem::temp_directory_path() /
        ("CubeVulkan-dialogue-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(root);
    {
        std::ofstream xml(root / "dict.xml");
        xml << "<dictionary><speech id=\"scope\">Before {$yellow inside @name $blue} after</speech>"
               "<speech id=\"glue\">@name=s</speech><speech id=\"explicit\">@name's</speech></dictionary>";
    }
    Speech speech;
    std::string error;
    check(speech.load(root / "dict.xml", error), "synthetic dictionary loads");
    Speech::Context context;
    context.value = [](const std::string &, const std::string &) { return "James"; };
    std::mt19937 random(5);
    const auto styled = speech.sayStyledPages("scope", context, random);
    check(styled.size() == 1 && styled[0].words.size() == 4, "styled expansion preserves word count");
    if (styled.size() == 1 && styled[0].words.size() == 4)
        check(styled[0].words[0].colour == glm::vec4(1) && styled[0].words[1].colour == glm::vec4(1,1,0,1) &&
              styled[0].words[2].colour == glm::vec4(1,1,0,1) && styled[0].words[3].colour == glm::vec4(1),
              "lexical tag precedence applies to the full scope and restores parent color");
    check(speech.say("glue", context, random) == "Jamess" && speech.say("explicit", context, random) == "James's",
          "equals-s is native glue, without invented English possessive rules");
    SaveGame progress;
    progress.specialization = 1;
    SaveGame restored;
    check(restored.parse(progress.text()) && restored.specialization == 1, "current specialization survives text-save roundtrip");
    check(restored.parse("cubevulkan-save 1\ngold 0\n") && restored.specialization == -1,
          "older saves keep the imported profile specialization");
    restored = progress;
    nativesaves::Character profile;
    profile.classType = 3;
    profile.specialization = 0;
    profile.level = 1;
    std::string notes;
    nativeprogress::fromProgress(restored, nullptr, 100, {}, profile, notes);
    check(profile.specialization == 1, "saving updates native character specialization");
    const auto reloaded = nativeprogress::toProgress(profile, notes, {}, {});
    check(reloaded.specialization == 1, "native character loading restores changed specialization");

    nativeplayer::Who who;
    who.cls = 4;
    auto record = nativeplayer::record(who, {});
    nativeai::Creature creature;
    creature.record = record.data();
    nativecontrol::State controls;
    controls.lastAttack = controls.lastDodge = controls.lastSkill = true;
    nativecontrol::Input keys;
    keys.ordinaryUi = true;
    nativecontrol::attack(keys, controls, creature);
    check(controls.lastAttack && controls.lastDodge && controls.lastSkill,
          "visible answers preserve late action previous states even with every binding released");
    keys.ordinaryUi = false;
    nativecontrol::attack(keys, controls, creature);
    check(!controls.lastAttack && !controls.lastDodge && !controls.lastSkill,
          "late release consumers resume when ordinary UI closes");
    keys.ordinaryUi = true;
    keys.forward = keys.ultimate = keys.lamp = true;
    nativecontrol::World controlWorld;
    nativecontrol::frame(keys, controls, creature, 16, controlWorld);
    float intent[3];
    std::memcpy(intent, record.data() + 0x40, sizeof intent);
    check(intent[0] != 0 || intent[1] != 0 || intent[2] != 0,
          "answer-visible ordinary UI retains keyboard movement");
    check(controls.lastUltimate && controls.lastLamp && !creature.buffs.empty(),
          "answer-visible ordinary UI retains earlier ultimate and lamp consumers");

    dialoguepanel::General general;
    TalkPlay::Page page;
    page.words = {{L"Hi"}, {L"there"}};
    general.applyCompilation({page});
    check(general.duration() == 360, "general duration includes a separator unit after every word");
    general.update(120, false, false);
    auto drawn = general.layout([](const std::wstring &text) { return text.size() * 10.f; });
    check(drawn.runs.size() == 2 && drawn.runs[0].word.text == L"Hi" && drawn.runs[1].word.text.empty() &&
          drawn.runs[1].at == glm::vec2(44,24) && drawn.revealed == 4,
          "budget3 lays out a zero-prefix second word without revealing its first character");
    int randomDraws = 0;
    auto randomPitch = [&] { ++randomDraws; return 32767; };
    const auto voice = general.voice(drawn, {}, randomPitch);
    check(voice && *voice == 1.5f && randomDraws == 1, "one voice pulse for crossed character group, one normal random draw");
    check(!general.voice(drawn, {}, randomPitch), "unchanged reveal count does not repeat voice");
    general.update(340, true, false); // exactly D+100: previous bits still forced zero
    check(general.indicator() == dialoguepanel::General::Hidden, "general maturity gate is strict");
    check(!general.update(0, false, false) && general.visible(), "early release is ignored");
    general.update(1, true, false);
    check(general.indicator() == dialoguepanel::General::End, "mature final page shows end indicator");
    const auto finished = general.update(0, false, false);
    check(finished == 0x7a && !general.visible() && general.elapsed() == 0, "mature release clears and hides final page immediately");
    TalkPlay::Page emptyWord;
    emptyWord.words = {{L""}};
    general.applyCompilation({emptyWord});
    check(general.duration() == 40 && general.layout([](const auto &) { return 1.f; }).revealed == 0,
          "actual empty words contribute duration but skip layout/reveal consumption");
    TalkPlay::Page existing;
    existing.words.assign(10, dialoguewords::Word{L"x"});
    existing.text = "Existing";
    TalkScript append;
    append.tokens = {"explicit"};
    auto compiled = playScript(append, speech, random, nullptr, {existing});
    check(compiled.pages.size() == 1 && compiled.pages[0].words.size() == 11, "phrase appends to a last page of exactly10 stored words");
    compiled = playScript(append, speech, random, nullptr, compiled.pages);
    check(compiled.pages.size() == 2, "phrase starts a new page when last page already exceeds10 words");

    // Appending lore must retain the running page and place credit on the resulting final page.
    dialoguepanel::General lorePanel;
    lorePanel.applyCompilation({existing});
    lorePanel.update(1, false, false);
    const double halfUlpPlus = 5.960464477539063e-8 + 1e-15;
    lorePanel.update(halfUlpPlus, false, false);
    check(lorePanel.elapsed() == std::nextafter(1.f, 2.f), "general clock adds in double before float storage");
    lorePanel.append(append, speech, random, nullptr, std::array<int32_t,2>{5,6});
    check(lorePanel.page() == 0 && lorePanel.elapsed() == std::nextafter(1.f, 2.f) && !lorePanel.activation(),
          "appending lore leaves running playback and its activation guard unchanged");
    lorePanel.append(append, speech, random, nullptr, std::array<int32_t,2>{7,8});
    check(lorePanel.pages().size() == 2 && lorePanel.pages()[0].loreCells == std::vector<std::array<int32_t,2>>{{5,6}} &&
          lorePanel.pages()[1].loreCells == std::vector<std::array<int32_t,2>>{{7,8}}, "lore attaches after ordinary dictionary page splitting");
    lorePanel.update(100000, true, false);
    check(lorePanel.elapsed() < 1002, "general update applies native 1000-ms elapsed cap");
    lorePanel.update(1000, true, false); // the full first page needs a second capped update
    check(lorePanel.update(0, false, false) == 0x7b && lorePanel.page() == 1 && lorePanel.activation(),
          "release schedules next-page credit for its zero-clock update");
    lorePanel.update(0, false, false);
    check(lorePanel.activation().has_value(), "zero elapsed does not manufacture a once-only page-effect bit");

    Assets assets;
    if (assets.locate("")) {
        Hud hud;
        std::string hudError;
        if (hud.load(assets.gameDir(), hudError)) {
            Renderer renderer;
            renderer.initHeadless(RenderSettings{});
            hud.setGuiScale(1); // owner options.cfg may have a different scale
            hud.layout(renderer, {800,600});
            Panel picked;
            picked.offer(1, {{"rest_inn?", {{L"test"}}}});
            picked.updateVisibility(false);
            std::vector<OverlayVertex> vertices;
            std::vector<glm::vec2> navigation;
            hud.drawDialogueChoices(picked, {800,600}, {400,420}, vertices, &navigation);
            check(picked.hovered() == 0 && hud.dialogueChoicesHit({400,420}) && navigation.size() == 1,
                  "owner blackwidget subtree picking and font-backed row navigation work");
            hud.setGuiScale(1.5f);
            navigation.clear();
            hud.drawDialogueChoices(picked, {800,600}, {400,327}, vertices, &navigation);
            check(picked.hovered() == 0 && navigation.size() == 1 && navigation[0].y == 333,
                  "scaled owner-template picking and navigation use interface units");
            dialoguepanel::General feedback;
            feedback.applyCompilation({page});
            const auto feedbackLayout = hud.drawGeneralDialogue(feedback, {800,600}, 0, vertices);
            check(hud.generalDialogueHit({400,150}) && hud.dialogueChoicesHit({400,327}) &&
                  feedbackLayout.runs.size() == 1,
                  "general and answer template instances retain independent geometry and subtree hits");
            renderer.shutdown();
        } else std::cout << "SKIP owner answer template check: " << hudError << '\n';
        Speech owner;
        if (owner.load(assets.gameDir() / "dict_en.xml", error)) {
            context.value = [](const std::string &fn, const std::string &) { return fn == "number" ? "10" : ""; };
            const auto answer = owner.sayStyledPages("rest_inn?", context, random);
            bool match = answer.size() == 1 && answer[0].words.size() == 11;
            if (match) {
                for (int i = 0; i < 11; ++i)
                    match &= answer[0].words[i].colour == (i < 9 ? glm::vec4(1) : glm::vec4(1,1,0,1));
                match &= answer[0].words[9].text == L"10" && answer[0].words[10].text == L"\u00ae";
            }
            if (!match && !answer.empty()) {
                std::cerr << "Owner answer word count " << answer[0].words.size() << '\n';
                for (const auto &word : answer[0].words)
                    std::cerr << "  length=" << word.text.size() << " first=" << (word.text.empty() ? -1 : (int)word.text[0])
                              << " colour=" << word.colour.r << ',' << word.colour.g << ',' << word.colour.b << '\n';
            }
            check(match, "owner inn answer matches independently specified word boundaries and colors");
        } else std::cout << "SKIP owner dictionary comparison: " << error << '\n';
    } else std::cout << "SKIP owner dialogue comparisons: installed game assets unavailable\n";
    if (!failures && root.filename().string().starts_with("CubeVulkan-dialogue-") &&
        std::filesystem::equivalent(root.parent_path(), std::filesystem::temp_directory_path()))
        std::filesystem::remove_all(root);
    std::cout << "Dialogue choices: " << (failures ? "FAIL\n" : "PASS\n");
    return failures;
}
