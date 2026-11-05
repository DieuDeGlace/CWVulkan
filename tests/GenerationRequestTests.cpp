#include "../tools/ZoneExtractor/GenerationRequest.h"
#include <iostream>
#include <limits>
#include <string>

int generationRequestTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *label) {
        if (!passed) {
            std::cout << "FAIL generation request: " << label << '\n';
            ++failures;
        }
    };
    using namespace capturepolicy;
    RetainedGenerationBudget budget;
    GenerationCoordinates work{{99, 99}};
    check(parseGenerationRequest("+1 -2\r\n-2147483648 +2147483647\t", budget.remaining(), work) &&
              work == GenerationCoordinates{{1, -2}, {INT32_MIN, INT32_MAX}},
          "complete signed coordinate pairs including extrema");
    const auto saved = work;
    for (const auto *bad : {"", " \n", "1", "1 2 3", "1 2x", "2147483648 0", "0 -2147483649", "+-1 0", "1.0 2"}) {
        check(!parseGenerationRequest(bad, budget.remaining(), work) && work == saved,
              "invalid or incomplete requests preserve previously queued work");
    }
    check(!parseGenerationRequest(std::string(MaximumGenerationRequestBytes + 1, ' '), 32, work) && work == saved,
          "oversized request preserves queued work");
    check(!parseGenerationRequest("1 2\n3 4", 1, work) && work == saved,
          "budget rejection does not accept a partial batch");
    for (size_t i = 0; i < RetainedGenerationBudget::MaximumAttempts - 1; ++i)
        check(budget.beginAttempt(), "successful and failed native attempts both consume credit");
    check(budget.remaining() == 1 && !parseGenerationRequest("1 2 3 4", budget.remaining(), work) && work == saved,
          "later requests see process-wide retained attempts");
    check(parseGenerationRequest("7 8", budget.remaining(), work) && work == GenerationCoordinates{{7, 8}} &&
              budget.beginAttempt() && budget.remaining() == 0 && !budget.beginAttempt() &&
              !parseGenerationRequest("9 10", budget.remaining(), work) && work == GenerationCoordinates{{7, 8}},
          "final credit is usable once; exhausted process cannot allocate again");
    std::cout << "Generation request admission: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
