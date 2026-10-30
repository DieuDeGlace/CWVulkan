#include "Speech.h"
#include "TalkScripts.h"

#include <chrono>
#include <cstdio>
#include <fstream>

int dialoguePageTests() {
    int failures = 0;
    auto check = [&](bool pass, const char *what) {
        if (!pass) {
            std::printf("FAIL dialogue pages: %s\n", what);
            ++failures;
        }
    };
    const auto file = std::filesystem::temp_directory_path() /
        ("CubeVulkan-dialogue-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".xml");
    {
        std::ofstream out(file);
        out << R"(<dictionary>
<speech id="short">Hello.</speech>
<speech id="ten">One two three four five six seven eight nine ten</speech>
<speech id="one">Next</speech>
<macro id="break">%</macro>
<macro id="percent">%%</macro>
<speech id="pages">First *break Last</speech>
<speech id="percentage">Gain 25%.</speech>
<speech id="literal">*percent</speech>
<speech id="lines">First
%
Last</speech>
<speech id="answer?">Agreed.</speech>
</dictionary>)";
    }
    Speech speech;
    std::string why;
    const bool loaded = speech.load(file, why);
    std::filesystem::remove(file);
    check(loaded, "load independent dictionary fixture");
    if (!loaded)
        return failures;
    std::mt19937 random(1);
    Speech::Context context;
    const auto nested = speech.sayPages("pages", context, random);
    check(nested == std::vector<std::string>{"First", "Last"}, "nested grammar page break");
    check(speech.sayPages("lines", context, random) == nested, "standalone dictionary break");
    check(speech.sayPages("literal", context, random) == std::vector<std::string>{"%"}, "escaped percent remains literal");
    check(speech.sayPages("percentage", context, random) == std::vector<std::string>{"Gain 25%."}, "percentage is not a break");
    auto play = [&](std::vector<std::string> tokens) {
        TalkScript script;
        script.tokens = std::move(tokens);
        return playScript(script, speech, random);
    };
    auto deferred = play({"short", "new_page", "mark_location", "1", "2", "new_page", "short", "mark_location", "3", "4"});
    check(deferred.lines.size() == 2, "deferred breaks coalesce");
    if (deferred.lineMarks.size() == 2) {
        check(deferred.lineMarks[0].size() == 1 && deferred.lineMarks[0][0].x == 1, "mark stays on preceding page");
        check(deferred.lineMarks[1].size() == 1 && deferred.lineMarks[1][0].x == 3, "later page owns later mark");
    } else
        check(false, "page effects remain aligned");
    auto grouped = play({"ten", "one", "one"});
    check(grouped.lines.size() == 2 && grouped.lines[0].find("Next") != std::string::npos,
          "exactly ten words accepts another speech; eleven starts a page");
    auto emote = play({"short", "!love", "mark_location", "5", "6", "short"});
    check(emote.pages.size() == 3, "emote page survives compilation");
    if (emote.pages.size() == 3)
        check(emote.pages[1].text.empty() && emote.pages[1].emotes.size() == 1 && emote.pages[1].marks.size() == 1,
              "empty emote page owns its metadata");
    auto consecutive = play({"!alert", "!question", "short"});
    check(consecutive.pages.size() == 3 && consecutive.pages[0].words.size() == 1 &&
          consecutive.pages[0].words[0].text == L"!alert" && consecutive.pages[1].words.size() == 1 &&
          consecutive.pages[1].words[0].text == L"!question", "each literal emote token owns a separate word page");
    auto choice = play({"short", "answer?", "pages", "mark_location", "7", "8"});
    check(choice.choices.size() == 1 && choice.choiceIds[0] == "answer?", "choice page is separate");
    check(!choice.lineMarks.empty() && choice.lineMarks.back().size() == 1, "dictionary split owns following mark");
    std::puts("dialogue pages: grammar breaks, grouping, choices and page effects checked");
    return failures;
}
