// Checks of the owner's setup (src/OwnerSetup.*): the Steam library list, the completeness
// rules, the journalled vanilla run and its rollback, and a whole extraction run driven by a
// pretend game. Everything happens in a fake game folder under the temporary directory; the
// real game folder is never touched, and no process is started.
#include "Assets.h"
#include "OwnerSetup.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>
#include <windows.h>

using namespace ownersetup;

namespace {
int failures = 0;
#define SETUP_CHECK(...)                                                                           \
    do {                                                                                           \
        if (!(__VA_ARGS__)) {                                                                      \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #__VA_ARGS__);                     \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

void write(const fs::path &file, const std::string &bytes) {
    fs::create_directories(file.parent_path());
    std::ofstream(file, std::ios::binary) << bytes;
}
void append(const fs::path &file, const std::string &text) {
    std::ofstream(file, std::ios::binary | std::ios::app) << text;
}
std::string read(const fs::path &file) {
    std::ifstream in(file, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), {});
}

// The smallest file dataCheck::gameBuild takes for a complete x64 image with this timestamp.
std::string fakePe(uint32_t stamp) {
    std::string b(512, '\0');
    b[0] = 'M', b[1] = 'Z';
    const uint32_t offset = 64;
    const uint16_t machine = 0x8664, optionalSize = 240, magic = 0x20b;
    std::memcpy(&b[0x3c], &offset, 4);
    std::memcpy(&b[64], "PE\0\0", 4);
    std::memcpy(&b[68], &machine, 2);
    std::memcpy(&b[72], &stamp, 4);
    std::memcpy(&b[84], &optionalSize, 2);
    std::memcpy(&b[88], &magic, 2);
    return b;
}

// An extracted zone with empty columns, as ZoneExtractor writes it.
std::string emptyZone(int x, int y) {
    std::string b = "CWZ1";
    const int32_t head[4] = {x, y, 0, 0};
    b.append((const char *)head, sizeof head);
    b.append(4096 * 8, '\0'); // baseZ 0, no runs
    return b;
}

// A complete CWI3 table with a single model grid and one key.
std::string itemModelTable() {
    const uint32_t fields[] = {660, 0x130, 1, 1, 1, 0, 1, 1, 0};
    return std::string("CWI3") + std::string(reinterpret_cast<const char *>(fields), sizeof fields);
}

// A pretend system: whether the game runs is set by the test.
struct FakeHost : Host {
    bool running = false, canLaunch = true;
    fs::path launchedGame;
    int launches = 0, closeRequests = 0;
    uint64_t now = 1000;
    bool gameRunning(const fs::path &) override { return running; }
    bool launchGame(const fs::path &game, std::string &why) override {
        launchedGame = game;
        ++launches;
        if (!canLaunch) {
            why = "Fixture launch failed.";
            return false;
        }
        running = true;
        return true;
    }
    bool askGameToClose(const fs::path &) override { return ++closeRequests, running; }
    uint64_t nowMs() override { return now += 500; }
};

struct FakeGame {
    fs::path root, game, pristine, dll;
    explicit FakeGame(const char *name) {
        root = fs::temp_directory_path() / ("CubeVulkanSetupTests-" + std::to_string(GetCurrentProcessId())) / name;
        std::error_code ec;
        fs::remove_all(root, ec); // the test's own folder from an earlier run
        game = root / "Cube World";
        dll = root / "build" / "tools" / "ZoneExtractor.dll";
        write(dll, fakePe(0x12345678));
        write(game / "cubeworld.exe", fakePe(dataCheck::SUPPORTED_BUILD));
        write(game / "sprites" / "logo.cub", "cub");
        write(game / "resource1.dat", "font");
        write(game / "resource2.dat", "font");
        write(game / "CubeModLoader.fip", "loader");
    }
    // The owner's own things the run must put back exactly.
    void ownerFiles() {
        write(game / "Save" / "characters.db", "every character");
        write(game / "Save" / "world.db", "the world");
        write(game / "Save" / "steam_autocloud.vdf", "cloud");
        write(game / "Mods" / "SomeMod.dll", "a mod");
        write(game / "Mods" / "SomeMod.ini", "[Mod]\nOn=1\n");
        write(game / "Mods" / "CubeMegaMod" / "data.bin", "nested");
        write(game / "dxgi.dll", "reshade");
        pristine = root / "pristine";
        fs::create_directories(pristine);
        fs::copy(game / "Save", pristine / "Save", fs::copy_options::recursive);
        fs::copy(game / "Mods", pristine / "Mods", fs::copy_options::recursive);
    }
    ~FakeGame() {
        std::error_code ec;
        fs::remove_all(root, ec);
    }
};

// --- The Steam library list ------------------------------------------------------------------
void libraryParsing() {
    const std::string current = "\"libraryfolders\"\n{\n"
                                "\t\"0\"\n\t{\n\t\t\"path\"\t\t\"C:\\\\Program Files (x86)\\\\Steam\"\n"
                                "\t\t\"label\"\t\t\"\"\n\t\t\"apps\"\n\t\t{\n\t\t\t\"440\"\t\t\"33279160746\"\n\t\t}\n\t}\n"
                                "\t\"1\"\n\t{\n\t\t\"path\"\t\t\"D:\\\\SteamLibrary\"\n"
                                "\t\t\"apps\"\n\t\t{\n\t\t\t\"1128000\"\t\t\"1\"\n\t\t}\n\t}\n}\n";
    const auto found = Assets::parseLibraryFolders(current);
    SETUP_CHECK(found.size() == 2);
    SETUP_CHECK(found.size() == 2 && found[0] == fs::path("C:\\Program Files (x86)\\Steam"));
    SETUP_CHECK(found.size() == 2 && found[1] == fs::path("D:\\SteamLibrary"));

    // The older layout: numbered keys whose values are the paths; other keys ignored.
    const std::string old = "// written by Steam\n\"LibraryFolders\"\n{\n\t\"TimeNextStatsReport\"\t\t\"1700000000\"\n"
                            "\t\"ContentStatsID\"\t\t\"-123\"\n\t\"1\"\t\t\"E:\\\\Games\\\\Steam\"\n}\n";
    const auto older = Assets::parseLibraryFolders(old);
    SETUP_CHECK(older.size() == 1 && older[0] == fs::path("E:\\Games\\Steam"));

    // A "path" outside libraryfolders, broken text and nothing at all give nothing.
    SETUP_CHECK(Assets::parseLibraryFolders("\"other\" { \"0\" { \"path\" \"X:\\\\no\" } }").empty());
    SETUP_CHECK(Assets::parseLibraryFolders("\"libraryfolders\" { \"0\" { \"path\" ").empty());
    SETUP_CHECK(Assets::parseLibraryFolders("").empty());
    // Escaped quotes in a path stay in it.
    const auto quoted = Assets::parseLibraryFolders("\"libraryfolders\"{\"0\"{\"path\" \"F:\\\\a \\\"b\\\"\"}}");
    SETUP_CHECK(quoted.size() == 1 && quoted[0] == fs::path("F:\\a \"b\""));

    // The game folders in a list of libraries.
    FakeGame fake("libraries");
    const fs::path library = fake.root / "Library";
    fs::create_directories(library / "steamapps" / "common");
    fs::rename(fake.game, library / "steamapps" / "common" / "Cube World");
    const auto games = gamesInLibraries({fake.root / "Empty", library});
    SETUP_CHECK(games.size() == 1 && games[0] == library / "steamapps" / "common" / "Cube World");
}

// --- The completeness rules ------------------------------------------------------------------
void completenessRules() {
    FakeGame fake("completeness");
    const fs::path &game = fake.game;

    GameStatus status = inspectGame(game);
    SETUP_CHECK(status.found && status.validExe && status.supportedBuild && status.modLoader && status.problem.empty());
    write(game / "cubeworld.exe", fakePe(0x11111111));
    status = inspectGame(game);
    SETUP_CHECK(status.found && status.validExe && !status.supportedBuild && !status.problem.empty());
    write(game / "cubeworld.exe", fakePe(dataCheck::SUPPORTED_BUILD));
    SETUP_CHECK(!inspectGame(fake.root / "nowhere").found);

    // Nothing extracted: the land is needed, the rest is optional.
    Completeness c = assess(game, true);
    SETUP_CHECK(c.find(Part::GameFiles) && c.find(Part::GameFiles)->present);
    SETUP_CHECK(c.find(Part::World) && !c.find(Part::World)->present && c.find(Part::World)->required);
    SETUP_CHECK(!c.readyToPlay());
    SETUP_CHECK(c.anythingToExtract());
    SETUP_CHECK(c.missing().size() == 6);
    SETUP_CHECK(c.text().find("Extracted land") != std::string::npos && c.text().find("Not ready") != std::string::npos);
    // Without extracted land in the ini, it can be played already.
    Completeness generated = assess(game, false);
    SETUP_CHECK(generated.readyToPlay());
    SETUP_CHECK(ExtractionRun::plan(generated).size() == 5); // the land is not asked for

    // The plan: each part with its request or key, never the game's own files.
    const auto plan = ExtractionRun::plan(c);
    SETUP_CHECK(plan.size() == 6);
    int requests = 0, sweeps = 0;
    for (const auto &t : plan) {
        requests += !t.collect.request.empty();
        sweeps += t.collect.sweepKey;
        SETUP_CHECK(t.part != Part::GameFiles);
    }
    SETUP_CHECK(requests == 5 && sweeps == 1); // the land is asked for by sweep.request, no key needed

    // Damaged game files are needed and cannot be extracted.
    fs::remove(game / "resource2.dat");
    c = assess(game, false);
    SETUP_CHECK(!c.find(Part::GameFiles)->present && !c.readyToPlay());
    write(game / "resource2.dat", "font");

    // Each part once its files are whole.
    const fs::path extract = game / "Extract", oracle = extract / "oracle";
    write(extract / "seed_7" / "manifest.txt", "seed=7\nhomeX=32\nhomeY=32\nhomeZ=10\n");
    write(extract / "seed_7" / "z_0_0.cwz", emptyZone(0, 0));
    write(extract / "sprites.txt", "0 sprites/a.cub 1 1 1\n");
    write(extract / "sprite-groups.txt", "1 a\n");
    write(extract / "sounds.txt", "0 hit.wav\n1 blade1.wav\n");
    for (const char *f : {"spawn-table.txt", "races.txt", "pickers.txt", "looks-wild.txt", "gear.bin"})
        write(oracle / f, "x");
    write(oracle / "looks.bin", "CWL1....");
    write(oracle / "item-models.bin", itemModelTable());
    write(oracle / "items.bin", "CIG1........");
    c = assess(game, true);
    SETUP_CHECK(c.readyToPlay() && !c.anythingToExtract() && c.missing().empty());
    SETUP_CHECK(c.find(Part::World)->detail.find("1 zones of seed 7") != std::string::npos);
    SETUP_CHECK(c.find(Part::SoundList)->detail == "2 sounds");

    // An interrupted export may still carry its full CWI3 header.
    const auto completeModels = itemModelTable();
    write(oracle / "item-models.bin", completeModels.substr(0, completeModels.size() - 1));
    const auto interrupted = assess(game, true);
    SETUP_CHECK(!interrupted.find(Part::ItemModels)->present);
    SETUP_CHECK(interrupted.find(Part::ItemModels)->detail.find("truncated") != std::string::npos);
    const auto pending = ExtractionRun::plan(interrupted);
    SETUP_CHECK(pending.size() == 1 && pending.front().part == Part::ItemModels);

    // Files that are there but wrong count as missing.
    write(oracle / "looks.bin", "XXXX");
    write(extract / "sounds.txt", "");
    write(oracle / "item-models.bin", "CWI2");
    write(extract / "seed_7" / "z_0_0.cwz", "CWZ1short");
    c = assess(game, true);
    SETUP_CHECK(!c.find(Part::CreatureTables)->present && c.find(Part::CreatureTables)->detail.find("looks.bin") != std::string::npos);
    SETUP_CHECK(!c.find(Part::SoundList)->present && !c.find(Part::ItemModels)->present);
    SETUP_CHECK(!c.find(Part::World)->present && !c.readyToPlay());

    // DataCheck's other findings are notes; those this list covers are not repeated.
    const dataCheck::Report report = dataCheck::inspect(game);
    c = assess(game, true, &report);
    bool covered = false, characters = false;
    for (const auto &n : c.notes)
        covered |= n.name == "Terrain" || n.name == "Original fonts", characters |= n.name == "Characters";
    SETUP_CHECK(!covered && characters);
}

// Alpha (the Steam world with Alpha's hills) needs neither captured terrain nor extra owner models.
void generationReadiness() {
    FakeGame fake("generation-readiness");
    const fs::path ini = fake.root / "CubeVulkan.ini";
    const auto alpha = assessConfigured(fake.game, ini); // same Alpha default as startup, including absent ini
    SETUP_CHECK(!worldRequired(ini) && !alpha.find(Part::World)->required);
    SETUP_CHECK(!alpha.find(Part::SpriteList)->required && alpha.find(Part::GameFiles)->present && alpha.readyToPlay());
    SETUP_CHECK(!alpha.find(Part::World)->present); // generated land needs no world sweep

    // Other generation modes retain their existing requirements.
    write(ini, "[World]\nSource=Game\n");
    const auto steam = assessConfigured(fake.game, ini);
    SETUP_CHECK(steam.readyToPlay() && !steam.find(Part::SpriteList)->required && !worldRequired(ini));
    write(ini, "[World]\nSource=Alpha\n");
    SETUP_CHECK(assessConfigured(fake.game, ini).readyToPlay() && !worldRequired(ini));
    write(ini, "[World]\nSource=Generated\n");
    SETUP_CHECK(assessConfigured(fake.game, ini).readyToPlay() && !worldRequired(ini));
    // Values follow the same exact spelling as the startup generation selector.
    write(ini, "[World]\nSource=alpha\n");
    SETUP_CHECK(worldRequired(ini) && !assessConfigured(fake.game, ini).readyToPlay());
    write(ini, "[World]\nSource=Extracted\n");
    SETUP_CHECK(worldRequired(ini) && !assessConfigured(fake.game, ini).readyToPlay());
}

// --- The journal and its rollback ------------------------------------------------------------
// A resource-only run must preserve the reference corpus and any competing request.
void proceduralCapture() {
    FakeGame fake("procedural-only");
    fake.ownerFiles();
    FakeHost host;
    const fs::path extract = fake.game / "Extract";
    write(extract / "seed_123" / "z_0_0.cwz", "owner terrain");
    write(extract / "oracle" / "gear.bin", "owner tables");
    PrepareOptions options;
    options.extractorDll = fake.dll;
    options.stamp = "procedural";
    options.proceduralItems = "CWP1\n-1 ffffffff\n3 80000000\n";
    std::string why;
    const fs::path requestFile = fake.root / "keys.txt";
    write(requestFile, options.proceduralItems);
    for (const auto &args : std::vector<std::vector<std::wstring>>{
             {L"--setup-capture", L"--procedural-items", L""},
             {L"--setup-extract", L"--procedural-items", requestFile.wstring()},
             {L"--setup-capture", L"--title-only", L"--procedural-items", requestFile.wstring()},
             {L"--setup-capture", L"--dungeon-targets", requestFile.wstring(), L"--procedural-items", requestFile.wstring()}}) {
        std::ostringstream output;
        SETUP_CHECK(headless(args, fake.root, output) == 2 && !runPending(fake.game));
        SETUP_CHECK(output.str().find("requires") != std::string::npos);
    }
    for (const std::string invalid : {std::string("CWP1\n"), std::string("CWP1\n3 not-hex\n"),
                                      std::string(65537, 'x')}) {
        auto bad = options;
        bad.proceduralItems = invalid;
        SETUP_CHECK(!prepare(fake.game, bad, host, why) && !runPending(fake.game));
    }
    auto mixed = options;
    mixed.requests = {"scene-reference.request"};
    SETUP_CHECK(!prepare(fake.game, mixed, host, why) && !runPending(fake.game));
    mixed = options;
    mixed.dungeonTargets = "0 0\n";
    SETUP_CHECK(!prepare(fake.game, mixed, host, why) && !runPending(fake.game));
    for (const char *name : {"procedural-items.request", "procedural-items.processing", "procedural-only.request", "ui-only.request"}) {
        write(extract / name, "existing owner request");
        SETUP_CHECK(!prepare(fake.game, options, host, why) && !runPending(fake.game));
        SETUP_CHECK(read(extract / name) == "existing owner request");
        fs::remove(extract / name);
    }
    SETUP_CHECK(sameTree(fake.game / "Save", fake.pristine / "Save") &&
                sameTree(fake.game / "Mods", fake.pristine / "Mods"));
    SETUP_CHECK(prepare(fake.game, options, host, why));
    SETUP_CHECK(fs::is_regular_file(extract / "procedural-only.request"));
    SETUP_CHECK(read(extract / "procedural-items.request") == options.proceduralItems);
    SETUP_CHECK(!fs::exists(extract / "scene-reference.request") && !fs::exists(extract / "oracle.request"));
    SETUP_CHECK(read(extract / "seed_123" / "z_0_0.cwz") == "owner terrain" &&
                read(extract / "oracle" / "gear.bin") == "owner tables");
    write(fake.game / "Save" / "world.db", "temporary world progress");
    write(extract / "procedural-items" / "completed.cwp", "completed owner resource");
    SETUP_CHECK(restore(fake.game, host).ok && !runPending(fake.game));
    SETUP_CHECK(!fs::exists(extract / "procedural-only.request") && !fs::exists(extract / "procedural-items.request"));
    SETUP_CHECK(read(extract / "procedural-items" / "completed.cwp") == "completed owner resource");
    SETUP_CHECK(sameTree(fake.game / "Save", fake.pristine / "Save") &&
                sameTree(fake.game / "Mods", fake.pristine / "Mods"));

    // An interrupted exporter retains its resumable processing request; setup removes its mode marker.
    SETUP_CHECK(prepare(fake.game, options, host, why));
    fs::rename(extract / "procedural-items.request", extract / "procedural-items.processing");
    SETUP_CHECK(restore(fake.game, host).ok && !fs::exists(extract / "procedural-only.request"));
    SETUP_CHECK(read(extract / "procedural-items.processing") == options.proceduralItems);
    SETUP_CHECK(!prepare(fake.game, options, host, why) && !runPending(fake.game));
}

void journalAndRollback() {
    FakeGame fake("journal");
    fake.ownerFiles();
    const fs::path &game = fake.game;
    FakeHost host;
    std::string why;

    // Refused: no extractor, the game running, CubeModLoader missing. Nothing changes.
    PrepareOptions options;
    options.extractorDll = fake.root / "missing.dll";
    options.stamp = "test";
    SETUP_CHECK(!prepare(game, options, host, why) && !runPending(game));
    options.extractorDll = fake.dll;
    host.running = true;
    SETUP_CHECK(!prepare(game, options, host, why) && !runPending(game));
    host.running = false;
    fs::rename(game / "CubeModLoader.fip", game / "loader.off");
    SETUP_CHECK(!prepare(game, options, host, why) && !runPending(game));
    fs::rename(game / "loader.off", game / "CubeModLoader.fip");
    options.requests = {"..\\escape.request"};
    SETUP_CHECK(!prepare(game, options, host, why) && !runPending(game));
    SETUP_CHECK(fs::exists(game / "Mods" / "SomeMod.dll") && fs::exists(game / "dxgi.dll"));

    options.requests.clear();
    options.dungeonTargets = "194 715\n-862 154\n";
    const auto dungeonRequest = game / "Extract" / "gen-dungeon.request";
    write(dungeonRequest, "existing owner request");
    SETUP_CHECK(!prepare(game, options, host, why) && !runPending(game) && read(dungeonRequest) == "existing owner request");
    fs::remove(dungeonRequest);
    SETUP_CHECK(prepare(game, options, host, why) && read(dungeonRequest) == options.dungeonTargets);
    SETUP_CHECK(!fs::exists(game / "Extract" / "scene-reference.request"));
    SETUP_CHECK(restore(game, host).ok && !fs::exists(dungeonRequest) && !runPending(game));
    options.dungeonTargets.clear();
    options.requests = {"ui-only.request"};
    const auto titleRequest = game / "Extract" / "ui-only.request";
    SETUP_CHECK(prepare(game, options, host, why) && fs::exists(titleRequest) &&
                !fs::exists(game / "Extract" / "scene-reference.request"));
    SETUP_CHECK(restore(game, host).ok && !fs::exists(titleRequest) && !runPending(game));
    options.stamp = "standard";

    // Prepared: Save copied, Mods and ReShade aside, the extractor alone, the requests written.
    write(game / "Extract" / "oracle" / "old.txt", "old tables");
    options.requests = {"sounds.request", "oracle.request"};
    options.backupOracle = true;
    SETUP_CHECK(prepare(game, options, host, why));
    const fs::path run = setupFolder(game) / "run-standard";
    SETUP_CHECK(runPending(game));
    SETUP_CHECK(sameTree(run / "Save", fake.pristine / "Save"));
    SETUP_CHECK(sameTree(run / "Mods", fake.pristine / "Mods"));
    SETUP_CHECK(read(run / "dxgi.dll") == "reshade" && !fs::exists(game / "dxgi.dll"));
    SETUP_CHECK(fs::exists(game / "Mods" / "ZoneExtractor.dll") && !fs::exists(game / "Mods" / "SomeMod.dll"));
    SETUP_CHECK(std::distance(fs::directory_iterator(game / "Mods"), fs::directory_iterator()) == 1);
    SETUP_CHECK(fs::exists(game / "Extract" / "sounds.request") && fs::exists(game / "Extract" / "oracle.request"));
    SETUP_CHECK(read(run / "oracle-before" / "old.txt") == "old tables");
    // A second run cannot start over an open one.
    SETUP_CHECK(!prepare(game, options, host, why));

    // The session: the game changes Save, the extractor logs and answers one request.
    write(game / "Save" / "characters.db", "a new character was made");
    write(game / "Save" / "extra.tmp", "new file");
    append(game / "Mods" / "ZoneExtractor.log", "ZoneExtractor loaded\n");
    fs::remove(game / "Extract" / "sounds.request");
    write(game / "Extract" / "sounds.txt", "0 hit.wav\n");

    // Not while the game runs.
    host.running = true;
    RestoreResult back = restore(game, host);
    SETUP_CHECK(!back.ok && runPending(game) && fs::exists(game / "Mods" / "ZoneExtractor.dll"));
    host.running = false;
    back = restore(game, host);
    SETUP_CHECK(back.ok);
    SETUP_CHECK(!runPending(game));
    SETUP_CHECK(sameTree(game / "Save", fake.pristine / "Save"));
    SETUP_CHECK(sameTree(game / "Mods", fake.pristine / "Mods"));
    SETUP_CHECK(read(game / "dxgi.dll") == "reshade");
    SETUP_CHECK(!fs::exists(game / "Extract" / "oracle.request"));             // unanswered: removed
    SETUP_CHECK(read(game / "Extract" / "sounds.txt") == "0 hit.wav\n");       // collected: kept
    SETUP_CHECK(fs::exists(run / "Mods-used" / "ZoneExtractor.log"));          // the run's log kept
    SETUP_CHECK(read(run / "Save-after-run" / "characters.db") == "a new character was made");
    SETUP_CHECK(fs::exists(run / "journal.txt"));
    // Repeating it does nothing.
    back = restore(game, host);
    SETUP_CHECK(back.ok && back.steps.empty() && sameTree(game / "Mods", fake.pristine / "Mods"));
}

// A run cut short at each point in the middle of the preparation, as after a crash or power
// loss: the journal and the folder as they were left, then the rollback of the next start.
void interruptedRuns() {
    const std::string header = "CubeVulkanSetup journal 1\nrun run-cut\n";
    {
        FakeGame fake("cut-request");
        fake.ownerFiles();
        FakeHost host;
        const auto staged = setupFolder(fake.game) / "run-cut" / "requests-staged" / "gen-dungeon.request";
        const auto request = fake.game / "Extract" / "gen-dungeon.request";
        write(staged, "staged coordinates");
        write(request, "independent owner request");
        write(journalFile(fake.game), header + "request-staged gen-dungeon.request\n");
        SETUP_CHECK(restore(fake.game, host).ok && read(request) == "independent owner request");
    }
    {
        // Mods moved aside, its "done" never written.
        FakeGame fake("cut-mods");
        fake.ownerFiles();
        FakeHost host;
        const fs::path run = setupFolder(fake.game) / "run-cut";
        fs::create_directories(run);
        fs::copy(fake.game / "Save", run / "Save", fs::copy_options::recursive);
        fs::rename(fake.game / "Mods", run / "Mods");
        write(journalFile(fake.game), header + "begin save\ndone save\nbegin mods\n");
        SETUP_CHECK(runPending(fake.game));
        const RestoreResult back = restore(fake.game, host);
        SETUP_CHECK(back.ok && !runPending(fake.game));
        SETUP_CHECK(sameTree(fake.game / "Mods", fake.pristine / "Mods") && read(fake.game / "dxgi.dll") == "reshade");
        SETUP_CHECK(sameTree(fake.game / "Save", fake.pristine / "Save"));
    }
    {
        // "begin mods" written, the rename never happened: the owner's Mods is left alone.
        FakeGame fake("cut-before-rename");
        fake.ownerFiles();
        FakeHost host;
        fs::create_directories(setupFolder(fake.game) / "run-cut");
        write(journalFile(fake.game), header + "none save\nbegin mods\n");
        SETUP_CHECK(restore(fake.game, host).ok);
        SETUP_CHECK(sameTree(fake.game / "Mods", fake.pristine / "Mods"));
    }
    {
        // Everything set aside and the extractor half installed; the game never started.
        FakeGame fake("cut-extractor");
        fake.ownerFiles();
        FakeHost host;
        const fs::path run = setupFolder(fake.game) / "run-cut";
        fs::create_directories(run);
        fs::copy(fake.game / "Save", run / "Save", fs::copy_options::recursive);
        fs::rename(fake.game / "Mods", run / "Mods");
        fs::rename(fake.game / "dxgi.dll", run / "dxgi.dll");
        fs::create_directory(fake.game / "Mods");
        write(journalFile(fake.game), header + "begin save\ndone save\nbegin mods\ndone mods\nbegin dxgi\ndone dxgi\nbegin extractor\n");
        SETUP_CHECK(restore(fake.game, host).ok);
        SETUP_CHECK(sameTree(fake.game / "Mods", fake.pristine / "Mods") && read(fake.game / "dxgi.dll") == "reshade");
    }
    {
        // A half-made Save copy: the game never ran, so Save itself is left as it is.
        FakeGame fake("cut-save");
        fake.ownerFiles();
        FakeHost host;
        const fs::path run = setupFolder(fake.game) / "run-cut";
        write(run / "Save.partial" / "characters.db", "half");
        write(journalFile(fake.game), header + "begin save\n");
        SETUP_CHECK(restore(fake.game, host).ok);
        SETUP_CHECK(sameTree(fake.game / "Save", fake.pristine / "Save"));
    }
    {
        // An owner with no Save, Mods or ReShade: what the session made goes with the run.
        FakeGame fake("bare");
        FakeHost host;
        PrepareOptions options;
        options.extractorDll = fake.dll;
        options.stamp = "bare";
        std::string why;
        SETUP_CHECK(prepare(fake.game, options, host, why));
        append(journalFile(fake.game), "begin launch\n");
        write(fake.game / "Save" / "characters.db", "made in the session");
        SETUP_CHECK(restore(fake.game, host).ok);
        SETUP_CHECK(!fs::exists(fake.game / "Mods") && !fs::exists(fake.game / "Save") && !fs::exists(fake.game / "dxgi.dll"));
    }
    {
        // An unreadable journal: nothing is touched, and it stays open.
        FakeGame fake("garbled");
        fake.ownerFiles();
        FakeHost host;
        write(journalFile(fake.game), "not a journal\n");
        SETUP_CHECK(!restore(fake.game, host).ok && runPending(fake.game));
        SETUP_CHECK(sameTree(fake.game / "Mods", fake.pristine / "Mods"));
    }
}

// --- A whole run against a pretend game ------------------------------------------------------
void extractionRun() {
    FakeGame fake("run");
    fake.ownerFiles();
    const fs::path &game = fake.game, extract = game / "Extract", oracle = extract / "oracle";
    FakeHost host;

    // The log watcher hands over whole lines only.
    {
        const fs::path file = fake.root / "watch.log";
        LogWatcher watch(file);
        SETUP_CHECK(watch.poll().empty());
        append(file, "first\nsecond, half");
        SETUP_CHECK(watch.poll() == "first\n");
        append(file, " done\n");
        SETUP_CHECK(watch.poll() == "second, half done\n" && watch.all() == "first\nsecond, half done\n");
    }

    ExtractionRun run(game, host);
    std::string why;
    SETUP_CHECK(run.start(assess(game, true), fake.dll, why));
    SETUP_CHECK(host.launches == 1 && host.launchedGame == game && runPending(game));
    run.poll();
    SETUP_CHECK(run.phase() == ExtractionRun::Phase::AtTitle);
    SETUP_CHECK(run.instructions().find("Start Game") != std::string::npos);

    const fs::path log = game / "Mods" / "ZoneExtractor.log";
    append(log, "ZoneExtractor loaded\nSession: seed 7, 0 zones already extracted, folder Extract\\seed_7\n");
    run.poll();
    SETUP_CHECK(run.phase() == ExtractionRun::Phase::Collecting);
    SETUP_CHECK(run.instructions().find("sweeps the land around your character by itself") != std::string::npos);
    SETUP_CHECK(fs::exists(game / "Extract" / "sweep.request"));

    // The extractor answers the requests and writes the automatic lists.
    write(extract / "sprites.txt", "0 sprites/a.cub 1 1 1\n");
    write(extract / "sprite-groups.txt", "1 a\n");
    write(extract / "sounds.txt", "0 hit.wav\n");
    append(log, "Sound list: 1 of 125 numbers named in Extract\\sounds.txt\n");
    for (const char *f : {"spawn-table.txt", "races.txt", "pickers.txt", "looks-wild.txt", "gear.bin"})
        write(oracle / f, "x");
    write(oracle / "looks.bin", "CWL1....");
    append(log, "Oracle: written to Extract\\oracle (49 zones' spawn points)\n");
    write(oracle / "item-models.bin", itemModelTable());
    append(log, "Item models: 712013 keys, 3000 grids written\n");
    write(oracle / "items.bin", "CIG1........");
    append(log, "Oracle: item generators written to Extract\\oracle\\items.bin\n");
    run.poll();
    SETUP_CHECK(run.phase() == ExtractionRun::Phase::Collecting && !run.allCollected());

    // The sweep: its progress, a stop part way, then to the end.
    append(log, "Sweep started: 289 stops\nStop 1/289 done, 49 zones\nStop 2/289 done, 60 zones\n");
    run.poll();
    bool progress = false;
    for (const auto &t : run.targets())
        progress |= t.collect.sweepKey && t.progress == "stop 2 of 289";
    SETUP_CHECK(progress);
    append(log, "Sweep stopped at stop 2\n");
    run.poll();
    SETUP_CHECK(run.instructions().find("start it again") != std::string::npos);
    write(extract / "seed_7" / "manifest.txt", "seed=7\nhomeX=32\nhomeY=32\nhomeZ=10\n");
    write(extract / "seed_7" / "z_0_0.cwz", emptyZone(0, 0));
    append(log, "Sweep started: 289 stops\nStop 289/289 done, 6000 zones\nSweep finished; 6000 zones extracted\nReturned home\n");
    run.poll();
    SETUP_CHECK(run.allCollected() && run.phase() == ExtractionRun::Phase::Collected);
    SETUP_CHECK(run.instructions().find("Quit Cube World") != std::string::npos);

    // The owner closes the game through the setup; once it has gone, the folder is put back.
    run.stop();
    SETUP_CHECK(host.closeRequests == 1 && run.active());
    host.running = false;
    run.poll();
    SETUP_CHECK(run.phase() == ExtractionRun::Phase::Done && run.restored().ok);
    SETUP_CHECK(!runPending(game));
    SETUP_CHECK(sameTree(game / "Mods", fake.pristine / "Mods") && sameTree(game / "Save", fake.pristine / "Save"));
    SETUP_CHECK(read(game / "dxgi.dll") == "reshade");
    SETUP_CHECK(assess(game, true).readyToPlay() && !assess(game, true).anythingToExtract());

    // A crash part way: the game vanishes before everything is collected.
    fs::remove(oracle / "item-models.bin");
    ExtractionRun crashed(game, host);
    SETUP_CHECK(crashed.start(assess(game, true), fake.dll, why));
    host.running = true;
    crashed.poll();
    append(game / "Mods" / "ZoneExtractor.log", "Session: seed 7\n");
    write(game / "Save" / "world.db", "changed by the session");
    crashed.poll();
    host.running = false;
    crashed.poll();
    SETUP_CHECK(crashed.phase() == ExtractionRun::Phase::Done && !crashed.allCollected());
    SETUP_CHECK(crashed.message().find("Item models") != std::string::npos);
    SETUP_CHECK(!runPending(game) && sameTree(game / "Save", fake.pristine / "Save") && sameTree(game / "Mods", fake.pristine / "Mods"));
    SETUP_CHECK(!fs::exists(extract / "itemmodels.request"));

    // The process starts and exits between preparation and the first setup poll.
    ExtractionRun early(game, host);
    SETUP_CHECK(early.start(assess(game, true), fake.dll, why));
    host.running = false;
    early.poll();
    SETUP_CHECK(early.phase() == ExtractionRun::Phase::Done && !early.allCollected() && !runPending(game));
    SETUP_CHECK(early.message().find("Item models") != std::string::npos);
    SETUP_CHECK(sameTree(game / "Mods", fake.pristine / "Mods"));

    host.canLaunch = false;
    ExtractionRun rejected(game, host);
    SETUP_CHECK(!rejected.start(assess(game, true), fake.dll, why));
    SETUP_CHECK(why == "Fixture launch failed. The game folder was put back.");
    SETUP_CHECK(!rejected.active() && rejected.restored().ok && !runPending(game));
    SETUP_CHECK(sameTree(game / "Mods", fake.pristine / "Mods") && sameTree(game / "Save", fake.pristine / "Save"));
}
} // namespace

int ownerSetupTests() {
    failures = 0;
    SETUP_CHECK(isHeadlessCommand({L"--setup-capture"}));
    SETUP_CHECK(isHeadlessCommand({L"--setup-capture", L"--dry-run"}));
    std::printf("owner setup tests...\n");
    std::fflush(stdout);
    try {
        libraryParsing();
        completenessRules();
        generationReadiness();
        proceduralCapture();
        journalAndRollback();
        interruptedRuns();
        extractionRun();
    } catch (const std::exception &e) { // a file operation that threw: a failure, not a crash
        std::printf("FAIL owner setup: %s\n", e.what());
        failures++;
    }
    std::error_code ec;
    fs::remove_all(fs::temp_directory_path() / ("CubeVulkanSetupTests-" + std::to_string(GetCurrentProcessId())), ec);
    if (failures == 0)
        std::printf("owner setup: library list, completeness, journal and rollback, extraction run ok\n");
    return failures;
}
