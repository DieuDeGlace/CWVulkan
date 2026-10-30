#pragma once
// The owner's first-run setup: find the Steam game, say what has been extracted from it and
// what is missing, and run the extraction for the owner in one go, safely.
//
// A run sets the owner's game folder up for a vanilla session and puts it back afterwards:
//   * Save is copied aside first, and put back exactly when the run ends;
//   * the owner's Mods folder and ReShade's dxgi.dll are moved aside, and a fresh Mods folder
//     holds only ZoneExtractor.dll (loaded by the CubeModLoader the game folder already has);
//   * the extractor's .request files ask for the missing tables; the extractor answers them on
//     its own once a world is loaded, and the run watches its log and output files;
//   * every step is written to a journal before it is taken, so a run that is interrupted (the
//     setup closed, the PC restarted, the game crashed) is rolled back the next time.
// Nothing in Extract is removed: what the run collected stays.
//
// The pure parts (the library list, the completeness rules, the journal and its rollback) are
// unit tested in a fake game folder (tests/OwnerSetupTests.cpp); the system parts (processes,
// Steam) go through `Host`, which tests replace.
#include "DataCheck.h"
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace ownersetup {
namespace fs = std::filesystem;

// --- Finding the game ------------------------------------------------------------------------

// Every "Cube World" folder in the given Steam libraries that holds cubeworld.exe and sprites.
std::vector<fs::path> gamesInLibraries(const std::vector<fs::path> &libraries);

struct GameStatus {
    fs::path dir;
    bool found = false;          // cubeworld.exe and sprites/ are there
    bool validExe = false;       // a complete x64 PE image
    bool supportedBuild = false; // PE timestamp dataCheck::SUPPORTED_BUILD
    uint32_t build = 0;
    bool modLoader = false;      // CubeModLoader.fip, which loads Mods/*.dll
    std::string problem;         // plain words, empty when found and valid
};
GameStatus inspectGame(const fs::path &dir);
// Steam online with a Cube World licence, checked as the engine does. The Steam runtime is
// released again before this returns, so Steam can start the game afterwards.
bool steamVerified(const fs::path &game, std::string &why);

// --- What has been extracted -----------------------------------------------------------------

// The parts of the owner's data the engine reads, in the order the report lists them.
enum class Part { GameFiles, World, SpriteList, SoundList, CreatureTables, ItemModels, SpecialItems };

// How the extractor makes a part: a request file it answers by itself while in a world, a key
// the owner presses, or nothing (the game's own files, or written whenever a world loads).
struct Collect {
    std::string request; // file name in Extract\, e.g. "sounds.request"
    std::string logDone; // the start of the extractor's log line that says it is written
    bool sweepKey = false; // F11: the sweep around home
    bool automatic = false; // written as soon as a world is loaded
};

struct PartState {
    Part part;
    std::string name;    // "Sound list"
    bool present = false;
    bool required = false; // CubeVulkan will not start without it
    std::string detail;  // what was found
    std::string without; // what is lost without it, in plain words
    Collect collect;
};

struct Completeness {
    fs::path game;
    std::vector<PartState> parts;
    std::vector<dataCheck::Entry> notes; // DataCheck's other findings (not part of the run)
    bool readyToPlay() const;    // every required part is present
    bool anythingToExtract() const;
    std::vector<const PartState *> missing() const;
    const PartState *find(Part part) const;
    std::string text() const;    // the report, one line per part, in plain words
};

// The rules. `worldRequired` is false when CubeVulkan.ini says [World] Source=Game or
// Generated (then no extracted terrain is needed). `report` is DataCheck's quick report of the
// same folder; its findings that this list does not cover are added as notes.
Completeness assess(const fs::path &game, bool worldRequired, const dataCheck::Report *report = nullptr);
// Whether CubeVulkan.ini (`ini`) plays extracted land, the default.
bool worldRequired(const fs::path &ini);

// --- The system, behind an interface tests replace -------------------------------------------

struct Host {
    virtual ~Host() = default;
    // True when this installation's cubeworld.exe is running (or cannot be ruled out).
    virtual bool gameRunning(const fs::path &game) = 0;
    // Starts Cube World through Steam (steam://run/1128000).
    virtual bool launchGame() = 0;
    // Asks the running game to close its window, as the owner would; false if none was found.
    virtual bool askGameToClose(const fs::path &game) = 0;
    virtual uint64_t nowMs() = 0;
};
std::unique_ptr<Host> systemHost();

// --- The journal: setting the folder up for a vanilla run, and putting it back ---------------

// <game>\CubeVulkanSetup\journal.txt while a run is open; each run keeps its own folder
// <game>\CubeVulkanSetup\run-<stamp>\ with the Save copy and what was set aside.
fs::path setupFolder(const fs::path &game);
fs::path journalFile(const fs::path &game);
bool runPending(const fs::path &game); // a journal is open: something must be put back

struct PrepareOptions {
    fs::path extractorDll;                 // tools\ZoneExtractor.dll beside the setup
    std::vector<std::string> requests;     // request files to write in Extract\ (names)
    bool backupOracle = false;             // copy Extract\oracle into the run folder first
    std::string stamp;                     // the run's name; empty: the local time
};

// Takes the steps in order, journalled: Save copied, Mods and dxgi.dll moved aside, the
// extractor installed, the requests written. On any failure it rolls back what it did and
// returns false with the reason. The game must be closed (checked through `host`).
bool prepare(const fs::path &game, const PrepareOptions &options, Host &host, std::string &why);

struct RestoreResult {
    bool ok = false;
    std::vector<std::string> steps; // what was put back, in plain words
    std::string why;                // when not ok
    fs::path runFolder;             // where the run's leftovers were kept
};
// Undoes an open journal: removes the run's Mods and any request still waiting, puts the
// owner's Mods and dxgi.dll back and Save back exactly. Each step looks at the folder as it
// is, so it is safe to repeat after a crash at any point. The game must be closed.
RestoreResult restore(const fs::path &game, Host &host);

// True when two folders hold the same files with the same bytes.
bool sameTree(const fs::path &a, const fs::path &b);

// --- Watching the extractor ------------------------------------------------------------------

// Reads what the extractor appends to its log (Mods\ZoneExtractor.log) since the last call.
class LogWatcher {
  public:
    explicit LogWatcher(fs::path file = {}) : file_(std::move(file)) {}
    std::string poll();
    const std::string &all() const { return all_; }

  private:
    fs::path file_;
    uintmax_t offset_ = 0;
    std::string all_;
};

// One run, from launching the game to the folder put back. The owner's part (loading a world,
// pressing F11, quitting) is described by instructions(); poll() moves it on.
class ExtractionRun {
  public:
    enum class Phase {
        Idle,       // nothing started
        Launching,  // the game was started through Steam; waiting for its process
        AtTitle,    // the game runs; waiting for a world to be loaded
        Collecting, // in a world; the extractor is writing
        Collected,  // everything asked for is written: the owner can quit
        Restoring,  // the game is gone; putting the folder back
        Done,       // put back (with or without everything collected)
        Failed      // could not put back: see message()
    };
    struct Target {
        Part part;
        std::string name;
        Collect collect;
        bool done = false;
        std::string progress; // e.g. "stop 12 of 289"
    };

    ExtractionRun(fs::path game, Host &host) : game_(std::move(game)), host_(host) {}
    // What a run would collect for these missing parts (in order). Game files are never in it.
    static std::vector<Target> plan(const Completeness &data);
    // Prepares the folder and starts the game. False (and nothing changed) on failure.
    bool start(const Completeness &data, const fs::path &extractorDll, std::string &why);
    void poll();
    // The owner wants to stop: asks the game to close (if it runs); the folder is put back once
    // it has gone.
    void stop();
    Phase phase() const { return phase_; }
    bool active() const { return phase_ != Phase::Idle && phase_ != Phase::Done && phase_ != Phase::Failed; }
    const std::vector<Target> &targets() const { return targets_; }
    bool allCollected() const;
    std::string instructions() const; // what the owner should do now, in plain words
    const std::string &message() const { return message_; } // the outcome once Done or Failed
    const RestoreResult &restored() const { return restored_; }

  private:
    void finish(const std::string &why);
    bool checkTarget(Target &t, const std::string &log);
    fs::path game_;
    Host &host_;
    Phase phase_ = Phase::Idle;
    std::vector<Target> targets_;
    LogWatcher log_;
    uint64_t started_ = 0, seenGame_ = 0, closeAsked_ = 0, restoreStarted_ = 0;
    bool sweepStopped_ = false, stopRequested_ = false;
    std::string message_;
    RestoreResult restored_;
};

// --setup-capture prepares an isolated reference session even with complete data.
// --- Headless use ----------------------------------------------------------------------------

// `--setup-check [--game PATH]`: finds the game, checks its build, Steam and the licence, and
// prints what is extracted and missing. Exit 0 ready to play, 1 data missing, 2 cannot check.
// `--setup-extract --dry-run [--game PATH]`: prints what a run would change and ask for,
// changing nothing. Without --dry-run it runs the extraction from the console.
// `--setup-restore [--game PATH]`: puts back a run that was interrupted.
// `setupRoot` is the folder of CubeVulkan.ini and tools\ZoneExtractor.dll.
bool isHeadlessCommand(const std::vector<std::wstring> &args);
int headless(const std::vector<std::wstring> &args, const fs::path &setupRoot, std::ostream &out);
} // namespace ownersetup
