#include "../tools/ZoneExtractor/GenerationRequest.h"
#include "../tools/ZoneExtractor/DungeonCapture.h"
#include "../tools/ZoneExtractor/DungeonCeilingCapture.h"
#include "../tools/ZoneExtractor/DungeonGateCapture.h"
#include "../tools/ZoneExtractor/DungeonSpawnCapture.h"
#include "../tools/ZoneExtractor/GenerationSnapshotFile.h"
#include "../tools/ZoneExtractor/NativeObjectOccupancy.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <string>

namespace {
void dungeonSpawnCellChecks(const auto &check) {
    using namespace captureabi;
    constexpr uintptr_t zone = 0x100, spawn = 0x800;
    constexpr uintptr_t mapHead = 0x1300, mapNode = 0x1380;
    constexpr uintptr_t guardHead = 0x1400, low = 0x1480, high = 0x1500;
    constexpr uintptr_t activationHead = 0x1580, activationNode = 0x1600;
    constexpr int64_t id = INT64_MIN + 7;
    std::array<uint8_t, 0x1a00> memory{};
    const auto put = [&](uintptr_t address, const auto &value) {
        std::memcpy(memory.data() + address, &value, sizeof value);
    };
    const auto links = [&](uintptr_t address, uintptr_t left, uintptr_t parent, uintptr_t right, uint8_t nil) {
        const spawn_tree::Links fields{left, parent, right, nil, nil};
        std::memcpy(memory.data() + address, &fields, spawn_tree::LinkBytes);
    };
    const auto read = [&](uintptr_t address, void *output, size_t size) {
        if (address > memory.size() || size > memory.size() - address)
            return false;
        std::memcpy(output, memory.data() + address, size);
        return true;
    };

    put(zone + 0x460, std::array<uintptr_t, 2>{mapHead, 1});
    links(mapHead, mapNode, mapNode, mapNode, 1);
    links(mapNode, mapHead, mapHead, mapHead, 0);
    put(mapNode + 0x20, id);
    put(mapNode + 0x28, spawn);
    put(spawn + 0x38, id);

    put(spawn + 0x900, std::array<uintptr_t, 2>{guardHead, 2});
    links(guardHead, low, high, high, 1);
    links(low, guardHead, high, guardHead, 0);
    links(high, low, guardHead, guardHead, 0);
    put(low + 0x1c, SpawnCell{-7, 12});
    put(high + 0x1c, SpawnCell{-7, 13});

    put(spawn + 0x928, std::array<uintptr_t, 2>{activationHead, 1});
    links(activationHead, activationNode, activationNode, activationNode, 1);
    links(activationNode, activationHead, activationHead, activationHead, 0);
    put(activationNode + 0x1c, SpawnCell{3, -5});

    dungeoncapture::SpawnCapture capture{{-10, 12}, UINT32_MAX, -7, {}};
    const std::vector<SpawnCell> guards{{-7, 12}, {-7, 13}}, activation{{3, -5}};
    const bool captured = readZoneSpawnCells(zone, capture.records, read);
    check(captured && capture.records.size() == 1 && capture.records[0].id == id &&
              capture.records[0].cells.guards == guards && capture.records[0].cells.activation == activation,
          "fresh zone spawn capture follows signed tree order and keeps the two cell collections separate");
    const auto encoded = dungeoncapture::encodeSpawnCells(capture);
    dungeoncapture::SpawnCapture decoded;
    check(dungeoncapture::decodeSpawnCells(encoded, decoded) && decoded.zone == capture.zone &&
              decoded.seed == capture.seed && decoded.day == capture.day && decoded.records.size() == 1 &&
              decoded.records[0].id == id && decoded.records[0].cells.guards == guards &&
              decoded.records[0].cells.activation == activation,
          "spawn-cell sidecar preserves context, signed IDs and tree keys");
    check(!dungeoncapture::decodeSpawnCells(encoded + "1", decoded) && decoded.records.size() == 1,
          "trailing sidecar data is rejected without replacing a previous capture");

    constexpr uintptr_t tab = 0x1680, stacks = 0x1700;
    put(spawn + 0x7f0, std::array<uintptr_t, 3>{tab, tab + 24, tab + 24});
    put(tab, std::array<uintptr_t, 3>{stacks, stacks + 2 * STACK_BYTES, stacks + 2 * STACK_BYTES});
    ItemRecord key;
    key.setCategory(dungeoncapture::DungeonKeyCategory);
    key.setSubtype(18);
    key.setRegion(-7, 3);
    put(stacks + STACK_BYTES, int32_t{1});
    put(stacks + STACK_BYTES + 4, key);
    capture.keysRecorded = true;
    const std::vector<dungeoncapture::SpawnKey> expectedKeys{{1, 18, {-7, 3}, 0, 0, 0, 0}};
    check(dungeoncapture::readSpawnKeys(spawn, capture.keys[id], read) && capture.keys[id] == expectedKeys &&
              dungeoncapture::decodeSpawnCells(dungeoncapture::encodeSpawnCells(capture), decoded) &&
              decoded.keysRecorded && decoded.keys == capture.keys,
          "the key capture reads the spawn inventory and round-trips its semantic fields");
    check(dungeoncapture::decodeSpawnCells(encoded, decoded) && !decoded.keysRecorded && decoded.keys.empty(),
          "older spawn captures distinguish unrecorded keys from an empty inventory");

    links(low, low, high, guardHead, 0);
    check(!readZoneSpawnCells(zone, capture.records, read) && capture.records.size() == 1,
          "invalid or cyclic native links reject the whole spawn-cell capture");
    links(low, guardHead, high, guardHead, 0);
    put(spawn + 0x928, std::array<uintptr_t, 2>{activationHead, 0});
    links(activationHead, activationHead, activationHead, activationHead, 1);
    check(readZoneSpawnCells(zone, capture.records, read) && capture.records.size() == 1 &&
              capture.records[0].cells.activation.empty(),
          "a constructed empty activation tree is distinct from a missing tree");
    put(spawn + 0x928, std::array<uintptr_t, 2>{0, 0});
    check(!readZoneSpawnCells(zone, capture.records, read), "a missing activation head is rejected");

    constexpr uintptr_t thresholdHead = 0x1900, thresholdNode = 0x1980;
    put(zone + 0x4c8, std::array<uintptr_t, 2>{thresholdHead, 1});
    links(thresholdHead, thresholdNode, thresholdNode, thresholdNode, 1);
    links(thresholdNode, thresholdHead, thresholdHead, thresholdHead, 0);
    put(thresholdNode + 0x1c, SpawnCell{-5, 3});
    put(thresholdNode + 0x24, int32_t{0});
    dungeoncapture::GateCapture thresholds{{-2, 0}, 123, -2, {}}, readThresholds;
    check(dungeoncapture::readGateThresholds(zone, thresholds, read) &&
              thresholds.thresholds == std::map<SpawnCell, int32_t>{{{-5, 3}, 0}} &&
              dungeoncapture::decodeGateThresholds(dungeoncapture::encodeGateThresholds(thresholds), readThresholds) &&
              readThresholds.thresholds == thresholds.thresholds && readThresholds.zone == thresholds.zone,
          "gate threshold capture preserves an explicit zero, signed cell and owner zone");

    put(zone + 0xe8, std::array<uintptr_t, 2>{thresholdHead, 1});
    put(thresholdNode + 0x24, int32_t{-11});
    const auto marked = readFreshObjectMarks(zone, read);
    const auto encodedMarks = marked ? objectoccupancy::encode(*marked) : std::vector<uint8_t>{};
    check(marked && *marked == objectoccupancy::Positions{{-5, 3, -11}} &&
              objectoccupancy::decode(encodedMarks.data(), encodedMarks.size()) == marked,
          "fresh occupancy capture retains the signed world block coordinates");
}
} // namespace

int generationRequestTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *label) {
        if (!passed) {
            std::cout << "FAIL generation request: " << label << '\n';
            ++failures;
        }
    };
    dungeonSpawnCellChecks(check);
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
    dungeoncapture::Plan dungeon;
    check(dungeoncapture::plan("194 715\n-862 154\n194 715\n", 3, dungeon) && dungeon.size() == 3 &&
              dungeon.at({48, 178}).size() == 6 && dungeon.at({48, 179}).size() == 3 &&
              dungeon.at({-216, 38}).size() == 9,
          "dungeon neighborhoods use signed floor division and deduplicate cells and zones");
    const auto savedPlan = dungeon;
    check(!dungeoncapture::plan("194 715\n-862 154", 2, dungeon) && dungeon == savedPlan &&
              !dungeoncapture::plan("2147483647 0", 32, dungeon) && dungeon == savedPlan,
          "dungeon planning rejects budget and coordinate overflow without truncating the request");
    check(dungeoncapture::plan("CW-DUNGEON-DOME 1\r\n194 715\r\n-862 154\r\n", 3, dungeon) && dungeon == savedPlan,
          "opt-in ceiling request preserves the normal neighborhood plan");
    dungeoncapture::CeilingCapture ceilings{{-216, 38}, 123, -2, {{{-862, 154, -1, 0, 15}, {true, -21}}}};
    dungeoncapture::CeilingCapture decodedCeilings;
    const auto ceilingText = dungeoncapture::encodeCeilings(ceilings);
    check(dungeoncapture::decodeCeilings(ceilingText, decodedCeilings) && decodedCeilings.rows == ceilings.rows &&
              decodedCeilings.zone == ceilings.zone && !dungeoncapture::decodeCeilings(ceilingText + "1", decodedCeilings),
          "ceiling sidecar retains a signed layer and height and rejects trailing data");
    dungeoncapture::Zone labels{{dungeoncapture::SourceTimestamp, 0x500000, UINT32_MAX, INT32_MIN, -7, 3, 4, 1},
                               {-216, 38},
                               {{{-862, 154}, {-14, 2}, {-14, 2}, 1, 44, 44, 9, 0x00020c09, -35, -34}}};
    auto encoded = dungeoncapture::encode(labels);
    dungeoncapture::Zone decoded;
    const auto readLabels = [&](const auto &bytes) { return dungeoncapture::decode(bytes.data(), bytes.size(), decoded); };
    check(readLabels(encoded) && labels.zone == decoded.zone &&
              labels.context == decoded.context && labels.decisions == decoded.decisions,
          "dungeon sidecar preserves signed context, coordinates and packed native decisions");
    encoded.push_back(0);
    check(!readLabels(encoded), "trailing data invalidates the sidecar");
    encoded.pop_back();
    encoded[5] = 0;
    check(!readLabels(encoded), "helper-query provenance is required");
    labels.decisions.push_back(labels.decisions.front());
    check(!readLabels(dungeoncapture::encode(labels)), "duplicate decision cells invalidate the sidecar");

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
