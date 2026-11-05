#include "../tools/ZoneExtractor/GenerationRequest.h"
#include "../tools/ZoneExtractor/GenerationSnapshotFile.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
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
    const GenerationContext context{0x1234, UINT32_MAX, INT32_MIN, -7};
    GenerationContextGate gate;
    check(!gate.allow(context) && !gate.paused() && gate.bind(context) && gate.allow(context),
          "only an admitted context can execute");
    for (int field = 0; field < 4; ++field) {
        auto changed = context;
        if (field == 0)
            ++changed.worldAddress;
        if (field == 1)
            --changed.seedBits;
        if (field == 2)
            ++changed.derivedSeed;
        if (field == 3)
            ++changed.day;
        GenerationContextGate candidate;
        candidate.bind(context);
        check(!candidate.allow(changed) && candidate.paused() && !candidate.allow(context) && !candidate.bind(context),
              "World/seed/day mismatch pauses permanently, even if original values return");
    }
    gate.stop();
    check(gate.paused() && !gate.allow(context) && !gate.bind(context),
          "I/O/native failure cannot silently restart capture");
    const auto snapshot = generationRequestSnapshot(context, {{1, -2}, {1, -2}, {INT32_MIN, INT32_MAX}});
    check(snapshot.starts_with("CW-GENERATION-SNAPSHOT 1\n") &&
              snapshot.find("seed-bits 4294967295\nderived-seed -2147483648\nday -7\n") != std::string::npos &&
              snapshot.ends_with("coordinates 3\n1 -2\n1 -2\n-2147483648 2147483647\nEND\n"),
          "snapshot retains full context bits, signed extrema, coordinate order and duplicates");
#ifdef _WIN32
    const auto directory =
        std::filesystem::temp_directory_path() /
        ("cw_generation_snapshot_" + std::to_string(GetCurrentProcessId()) + "_" + std::to_string(GetTickCount64()));
    if (!std::filesystem::create_directory(directory)) {
        check(false, "exclusive temporary fixture directory");
        return failures;
    }
    const auto read = [](const auto &path) {
        std::ifstream input(path, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(input), {});
    };
    const auto final = directory / "request.txt";
    check(publishGenerationSnapshot(final.string(), snapshot) && read(final) == snapshot &&
              !std::filesystem::exists(final.string() + ".tmp"),
          "actual file publication retains exact bytes and completes the rename");
    check(!publishGenerationSnapshot(final.string(), "replacement") && read(final) == snapshot &&
              !std::filesystem::exists(final.string() + ".tmp"),
          "existing completed owner snapshot cannot be overwritten");
    const auto blocked = directory / "blocked.txt";
    {
        std::ofstream output(blocked.string() + ".tmp", std::ios::binary);
        output << "owner temporary";
    }
    check(!publishGenerationSnapshot(blocked.string(), snapshot) &&
              read(blocked.string() + ".tmp") == "owner temporary" && !std::filesystem::exists(blocked),
          "existing temporary owner file cannot be overwritten or deleted");
    check(!publishGenerationSnapshot((directory / "missing" / "request.txt").string(), snapshot),
          "unavailable output fails without publication");
    const auto artifact = directory / "z_1_-2.cwz";
    const auto writeTerrain = [](std::FILE *file) { return std::fwrite("CWZ1", 1, 4, file) == 4; };
    check(writeGenerationArtifact(artifact.string(), writeTerrain) && read(artifact) == "CWZ1",
          "capture writer reports a completed output");
    check(!writeGenerationArtifact((directory / "missing" / "z_1_-2.cwz").string(), writeTerrain),
          "a failed output open cannot be counted as a completed zone");
    check(!writeGenerationArtifact(artifact.string(), [](std::FILE *file) {
              std::fwrite("CW", 1, 2, file);
              return false;
          }),
          "a partially written capture is not successful");
    const auto inputPath = directory / "gen.request";
    {
        std::ofstream output(inputPath, std::ios::binary);
        output << "1 -2\n";
    }
    {
        GenerationRequestInput input(inputPath.string().c_str());
        std::string original;
        check(input && input.read(original) && original == "1 -2\n", "exclusive request read retains exact input");
        HANDLE replacement = CreateFileA(inputPath.string().c_str(), GENERIC_WRITE,
                                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                                         FILE_ATTRIBUTE_NORMAL, nullptr);
        check(replacement == INVALID_HANDLE_VALUE && !DeleteFileA(inputPath.string().c_str()),
              "producer cannot replace or delete the file between read and consumption");
        if (replacement != INVALID_HANDLE_VALUE)
            CloseHandle(replacement);
        check(!publishGenerationSnapshot(final.string(), original), "failed snapshot does not consume held request");
    }
    check(read(inputPath) == "1 -2\n", "unconsumed input survives handle destruction unchanged");
    {
        GenerationRequestInput input(inputPath.string().c_str());
        check(input && input.consume(), "successful admission consumes the exact held input");
    }
    check(!std::filesystem::exists(inputPath), "consumed request disappears after handle close");
    // Remove only the explicitly created fixture files, then its empty directory.
    std::filesystem::remove(inputPath);
    std::filesystem::remove(artifact);
    std::filesystem::remove(final);
    std::filesystem::remove(final.string() + ".tmp");
    std::filesystem::remove(blocked);
    std::filesystem::remove(blocked.string() + ".tmp");
    std::filesystem::remove(directory);
#endif
    std::cout << "Generation request admission: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
