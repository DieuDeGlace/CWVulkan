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

// A pretend system: whether the game runs is set by the test.
struct FakeHost : Host {
    bool running = false;
    int launches = 0, closeRequests = 0;
    uint64_t now = 1000;
    bool gameRunning(const fs::path &) override { return running; }
    bool launchGame() override { return ++launches, true; }
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
    SETUP_CHECK(requests == 4 && sweeps == 1);

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
    write(oracle / "item-models.bin", "CWI3....");
    write(oracle / "items.bin", "CIG1........");
    c = assess(game, true);
    SETUP_CHECK(c.readyToPlay() && !c.anythingToExtract() && c.missing().empty());
    SETUP_CHECK(c.find(Part::World)->detail.find("1 zones of seed 7") != std::string::npos);
    SETUP_CHECK(c.find(Part::SoundList)->detail == "2 sounds");

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

// --- The journal and its rollback ------------------------------------------------------------
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

    // Prepared: Save copied, Mods and ReShade aside, the extractor alone, the requests written.
    write(game / "Extract" / "oracle" / "old.txt", "old tables");
    options.requests = {"sounds.request", "oracle.request"};
    options.backupOracle = true;
    SETUP_CHECK(prepare(game, options, host, why));
    const fs::path run = setupFolder(game) / "run-test";
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
    SETUP_CHECK(host.launches == 1 && run.phase() == ExtractionRun::Phase::Launching && runPending(game));
    SETUP_CHECK(run.instructions().find("Steam") != std::string::npos);
    run.poll();
    SETUP_CHECK(run.phase() == ExtractionRun::Phase::Launching);

    host.running = true;
    run.poll();
    SETUP_CHECK(run.phase() == ExtractionRun::Phase::AtTitle);
    SETUP_CHECK(run.instructions().find("Start Game") != std::string::npos);

    const fs::path log = game / "Mods" / "ZoneExtractor.log";
    append(log, "ZoneExtractor loaded\nSession: seed 7, 0 zones already extracted, folder Extract\\seed_7\n");
    run.poll();
    SETUP_CHECK(run.phase() == ExtractionRun::Phase::Collecting);
    SETUP_CHECK(run.instructions().find("F11") != std::string::npos);

    // The extractor answers the requests and writes the automatic lists.
    write(extract / "sprites.txt", "0 sprites/a.cub 1 1 1\n");
    write(extract / "sprite-groups.txt", "1 a\n");
    write(extract / "sounds.txt", "0 hit.wav\n");
    append(log, "Sound list: 1 of 125 numbers named in Extract\\sounds.txt\n");
    for (const char *f : {"spawn-table.txt", "races.txt", "pickers.txt", "looks-wild.txt", "gear.bin"})
        write(oracle / f, "x");
    write(oracle / "looks.bin", "CWL1....");
    append(log, "Oracle: written to Extract\\oracle (49 zones' spawn points)\n");
    write(oracle / "item-models.bin", "CWI3....");
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

    // Stopped before the game ever started.
    ExtractionRun early(game, host);
    SETUP_CHECK(early.start(assess(game, true), fake.dll, why));
    early.stop();
    SETUP_CHECK(early.phase() == ExtractionRun::Phase::Done && !runPending(game));
    SETUP_CHECK(sameTree(game / "Mods", fake.pristine / "Mods"));
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
