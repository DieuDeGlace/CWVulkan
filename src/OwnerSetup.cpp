// The owner's first-run setup (OwnerSetup.h): finding the game, the completeness rules, the
// journalled vanilla run and its rollback, and the headless commands.
#include "OwnerSetup.h"
#include "Assets.h"
#include "Config.h"
#include "Extracted.h"
#include "SteamSession.h"
#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <map>
#include <set>
#include <sstream>

namespace ownersetup {
namespace {
bool nonempty(const fs::path &file) {
    std::error_code ec;
    return fs::is_regular_file(file, ec) && fs::file_size(file, ec) > 0 && !ec;
}
bool startsWith(const fs::path &file, const char magic[4]) {
    std::ifstream in(file, std::ios::binary);
    char head[4]{};
    return in.read(head, 4) && std::memcmp(head, magic, 4) == 0;
}
std::string localStamp() {
    std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_s(&local, &now);
    std::ostringstream out;
    out << std::put_time(&local, "%Y-%m-%d-%H%M%S");
    return out.str();
}
// `base`, or `base-2`, `base-3`, ... whichever does not exist yet.
fs::path unusedPath(const fs::path &base) {
    std::error_code ec;
    if (!fs::exists(base, ec))
        return base;
    for (int n = 2;; n++) {
        fs::path candidate = base.parent_path() / (base.filename().wstring() + L"-" + std::to_wstring(n));
        if (!fs::exists(candidate, ec))
            return candidate;
    }
}
std::string plainError(const std::error_code &ec) { return ec ? ec.message() : std::string("unknown error"); }
} // namespace

// --- Finding the game ------------------------------------------------------------------------

std::vector<fs::path> gamesInLibraries(const std::vector<fs::path> &libraries) {
    std::vector<fs::path> games;
    for (const auto &library : libraries) {
        const fs::path dir = library / "steamapps" / "common" / "Cube World";
        std::error_code ec;
        if (fs::is_regular_file(dir / "cubeworld.exe", ec) && fs::is_directory(dir / "sprites", ec))
            games.push_back(dir);
    }
    return games;
}

GameStatus inspectGame(const fs::path &dir) {
    GameStatus s;
    s.dir = dir;
    std::error_code ec;
    s.found = !dir.empty() && fs::is_regular_file(dir / "cubeworld.exe", ec) && fs::is_directory(dir / "sprites", ec);
    if (!s.found) {
        s.problem = "No Steam Cube World installation (cubeworld.exe and sprites) was found in this folder.";
        return s;
    }
    std::string why;
    s.validExe = dataCheck::gameBuild(dir / "cubeworld.exe", s.build, why);
    s.supportedBuild = s.validExe && s.build == dataCheck::SUPPORTED_BUILD;
    s.modLoader = fs::is_regular_file(dir / "CubeModLoader.fip", ec);
    if (!s.validExe)
        s.problem = "cubeworld.exe is damaged (" + why + "). Verify the game files in Steam.";
    else if (!s.supportedBuild)
        s.problem = "This cubeworld.exe is a different build from the one the extractor knows. "
                    "CubeVulkan can read what was extracted before, but cannot extract more.";
    return s;
}

// --- What has been extracted -----------------------------------------------------------------

namespace {
// Whether one part's files are there and look whole.
bool partPresent(const fs::path &game, Part part, std::string &detail) {
    const fs::path extract = game / "Extract", oracle = extract / "oracle";
    std::error_code ec;
    switch (part) {
    case Part::GameFiles: {
        uint32_t build = 0;
        std::string why;
        size_t sprites = 0;
        for (const auto &e : fs::directory_iterator(game / "sprites", ec))
            sprites += e.path().extension() == ".cub";
        const bool exe = dataCheck::gameBuild(game / "cubeworld.exe", build, why);
        const bool fonts = nonempty(game / "resource1.dat") && nonempty(game / "resource2.dat");
        detail = std::to_string(sprites) + " models" + (fonts ? ", fonts" : ", fonts missing") + (exe ? "" : ", cubeworld.exe damaged");
        return exe && sprites > 0 && fonts;
    }
    case Part::World: {
        const fs::path dir = ExtractedWorld::find(game);
        ExtractedWorld world;
        std::string why;
        if (dir.empty()) {
            detail = "no extracted land";
            return false;
        }
        if (!world.open(dir, why)) {
            detail = why;
            return false;
        }
        detail = std::to_string(world.zoneCount()) + " zones of seed " + std::to_string(world.seed());
        return true;
    }
    case Part::SpriteList: {
        const bool ok = nonempty(extract / "sprites.txt") && nonempty(extract / "sprite-groups.txt");
        detail = ok ? "present" : "not written yet";
        return ok;
    }
    case Part::SoundList: {
        std::ifstream in(extract / "sounds.txt");
        int number = -1, lines = 0;
        std::string name;
        while (in >> number >> name)
            lines++;
        detail = lines ? std::to_string(lines) + " sounds" : "not written yet";
        return lines > 0;
    }
    case Part::CreatureTables: {
        std::vector<std::string> gaps;
        for (const char *file : {"spawn-table.txt", "races.txt", "pickers.txt", "looks-wild.txt", "gear.bin"})
            if (!nonempty(oracle / file))
                gaps.push_back(file);
        if (!startsWith(oracle / "looks.bin", "CWL1"))
            gaps.push_back("looks.bin");
        std::string list;
        for (const auto &g : gaps)
            list += (list.empty() ? "" : ", ") + g;
        detail = gaps.empty() ? "present" : "missing " + list;
        return gaps.empty();
    }
    case Part::ItemModels: {
        const bool ok = startsWith(oracle / "item-models.bin", "CWI3");
        detail = ok ? "present" : "not written yet";
        return ok;
    }
    case Part::SpecialItems: {
        const bool ok = startsWith(oracle / "items.bin", "CIG1");
        detail = ok ? "present" : "not written yet";
        return ok;
    }
    }
    return false;
}
} // namespace

Completeness assess(const fs::path &game, bool worldRequired, const dataCheck::Report *report) {
    Completeness c;
    c.game = game;
    auto add = [&](Part part, const char *name, bool required, const char *without, Collect collect) {
        PartState s{part, name};
        s.required = required;
        s.without = without;
        s.collect = std::move(collect);
        s.present = partPresent(game, part, s.detail);
        c.parts.push_back(std::move(s));
    };
    add(Part::GameFiles, "Game files", true, "CubeVulkan cannot start. Verify the game files in Steam.", {});
    add(Part::World, "Extracted land", worldRequired,
        worldRequired ? "CubeVulkan has no land to show." : "Not needed: CubeVulkan.ini builds the land itself ([World] Source).",
        {"", "", true, false});
    add(Part::SpriteList, "Model list", false, "Creatures and placed objects cannot be drawn.", {"", "", false, true});
    add(Part::SoundList, "Sound list", false, "Sound effects are silent (music still plays).",
        {"sounds.request", "Sound list:", false, false});
    add(Part::CreatureTables, "Creature tables", false, "The world has no creatures or people.",
        {"oracle.request", "Oracle: written", false, false});
    add(Part::ItemModels, "Item models", false, "Creatures' weapons and armour are not drawn.",
        {"itemmodels.request", "Item models:", false, false});
    add(Part::SpecialItems, "Special items", false, "Special drops have no kind.",
        {"items.request", "Oracle: item generators", false, false});
    if (report) {
        // DataCheck's findings this list does not already cover.
        static const std::set<std::string> covered{"Game build", "Original sprites", "Original fonts", "Terrain",
                                                   "sprite-groups.txt"};
        for (const auto &e : report->entries)
            if (e.status != dataCheck::Status::Pass && !covered.count(e.name))
                c.notes.push_back(e);
    }
    return c;
}

bool worldRequired(const fs::path &ini) {
    Config config;
    config.load(ini);
    const std::string source = config.text("World", "Source", "Extracted");
    return _stricmp(source.c_str(), "Game") != 0 && _stricmp(source.c_str(), "Generated") != 0;
}

bool Completeness::readyToPlay() const {
    return std::all_of(parts.begin(), parts.end(), [](const PartState &p) { return p.present || !p.required; });
}
std::vector<const PartState *> Completeness::missing() const {
    std::vector<const PartState *> out;
    for (const auto &p : parts)
        if (!p.present && (p.required || p.part != Part::World))
            out.push_back(&p);
    return out;
}
bool Completeness::anythingToExtract() const {
    for (const auto *p : missing())
        if (p->part != Part::GameFiles)
            return true;
    return false;
}
const PartState *Completeness::find(Part part) const {
    for (const auto &p : parts)
        if (p.part == part)
            return &p;
    return nullptr;
}
std::string Completeness::text() const {
    std::ostringstream out;
    for (const auto &p : parts) {
        out << (p.present ? "[OK]      " : p.required ? "[NEEDED]  " : "[MISSING] ") << p.name << ": " << p.detail;
        if (!p.present)
            out << (p.required ? " (needed to play)" : " (optional)") << "\n          Without it: " << p.without;
        out << '\n';
    }
    if (!notes.empty()) {
        out << "\nOther findings of the data check:\n";
        for (const auto &n : notes) {
            out << (n.status == dataCheck::Status::Error ? "  [!] " : "  [-] ") << n.name << ": " << n.detail << '\n';
            if (!n.action.empty())
                out << "      " << n.action << '\n';
        }
    }
    out << "\n" << (readyToPlay() ? "Ready to play." : "Not ready to play yet: something needed is missing.") << '\n';
    return out.str();
}

// --- The system ------------------------------------------------------------------------------

namespace {
// The ids of running processes whose image is this installation's cubeworld.exe. `unknown`
// is set when a cubeworld.exe could not be identified (then it must be assumed to be ours).
std::vector<DWORD> gameProcesses(const fs::path &game, bool &unknown) {
    std::vector<DWORD> ids;
    unknown = false;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        unknown = true;
        return ids;
    }
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof entry;
    for (BOOL more = Process32FirstW(snapshot, &entry); more; more = Process32NextW(snapshot, &entry)) {
        if (_wcsicmp(entry.szExeFile, L"cubeworld.exe") != 0)
            continue;
        HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ProcessID);
        std::wstring image(32768, L'\0');
        DWORD count = (DWORD)image.size();
        const bool queried = process && QueryFullProcessImageNameW(process, 0, image.data(), &count);
        if (process)
            CloseHandle(process);
        if (!queried) {
            unknown = true;
            ids.push_back(entry.th32ProcessID);
            continue;
        }
        image.resize(count);
        std::error_code ec;
        if (fs::equivalent(fs::path(image), game / "cubeworld.exe", ec) || ec)
            ids.push_back(entry.th32ProcessID);
    }
    CloseHandle(snapshot);
    return ids;
}

struct SystemHost : Host {
    bool gameRunning(const fs::path &game) override {
        bool unknown = false;
        return !gameProcesses(game, unknown).empty() || unknown;
    }
    bool launchGame() override {
        const auto result = (INT_PTR)ShellExecuteW(nullptr, L"open", L"steam://run/1128000", nullptr, nullptr, SW_SHOWNORMAL);
        return result > 32;
    }
    bool askGameToClose(const fs::path &game) override {
        bool unknown = false;
        struct Search {
            std::vector<DWORD> ids;
            int asked = 0;
        } search{gameProcesses(game, unknown)};
        EnumWindows(
            [](HWND window, LPARAM data) -> BOOL {
                auto *s = (Search *)data;
                DWORD pid = 0;
                GetWindowThreadProcessId(window, &pid);
                if (IsWindowVisible(window) && std::find(s->ids.begin(), s->ids.end(), pid) != s->ids.end()) {
                    PostMessageW(window, WM_CLOSE, 0, 0); // as the window's close button does
                    s->asked++;
                }
                return TRUE;
            },
            (LPARAM)&search);
        return search.asked > 0;
    }
    uint64_t nowMs() override { return GetTickCount64(); }
};
} // namespace

std::unique_ptr<Host> systemHost() { return std::make_unique<SystemHost>(); }

// --- The journal -----------------------------------------------------------------------------
//
// journal.txt is a list of lines, each written (and flushed to disk) before the step it names:
//   CubeVulkanSetup journal 1
//   run <folder name>        the run's folder under <game>\CubeVulkanSetup
//   begin <step> / done <step> / none <step>   save, mods, dxgi, extractor, launch
//   request <file name>      a request written in Extract\
// "none" says there was nothing to set aside (no Save, no Mods, no dxgi.dll).

namespace {
const char *const JOURNAL_HEADER = "CubeVulkanSetup journal 1";

bool appendLine(const fs::path &file, const std::string &line) {
    HANDLE h = CreateFileW(file.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    const std::string text = line + "\n";
    DWORD written = 0;
    const bool ok = WriteFile(h, text.data(), (DWORD)text.size(), &written, nullptr) && written == text.size() && FlushFileBuffers(h);
    CloseHandle(h);
    return ok;
}

struct Journal {
    std::string run;
    std::set<std::string> begun, done, none;
    std::vector<std::string> requests;
    bool valid = false;
};
Journal readJournal(const fs::path &file) {
    Journal j;
    std::ifstream in(file);
    std::string line;
    if (!std::getline(in, line) || line.rfind(JOURNAL_HEADER, 0) != 0)
        return j;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        const size_t space = line.find(' ');
        if (space == std::string::npos)
            continue;
        const std::string word = line.substr(0, space), rest = line.substr(space + 1);
        if (word == "run")
            j.run = rest;
        else if (word == "begin")
            j.begun.insert(rest);
        else if (word == "done")
            j.done.insert(rest), j.begun.insert(rest);
        else if (word == "none")
            j.none.insert(rest);
        else if (word == "request")
            j.requests.push_back(rest);
    }
    // The run folder is a plain name inside the setup folder.
    j.valid = !j.run.empty() && j.run.find_first_of("/\\:") == std::string::npos && j.run != "." && j.run != "..";
    return j;
}
bool plainName(const std::string &name) {
    return !name.empty() && name.find_first_of("/\\:") == std::string::npos && name != "." && name != "..";
}

// Copies a folder: into `to`.partial first, then renamed, so `to` is whole or absent.
bool copyTree(const fs::path &from, const fs::path &to, std::error_code &ec) {
    fs::path partial = to;
    partial += L".partial";
    fs::remove_all(partial, ec); // only the setup's own unfinished copy
    ec.clear();
    fs::copy(from, partial, fs::copy_options::recursive, ec);
    if (ec)
        return false;
    fs::rename(partial, to, ec);
    return !ec;
}
} // namespace

fs::path setupFolder(const fs::path &game) { return game / "CubeVulkanSetup"; }
fs::path journalFile(const fs::path &game) { return setupFolder(game) / "journal.txt"; }
bool runPending(const fs::path &game) {
    std::error_code ec;
    return fs::exists(journalFile(game), ec);
}

bool sameTree(const fs::path &a, const fs::path &b) {
    std::error_code ec;
    if (!fs::is_directory(a, ec) || !fs::is_directory(b, ec))
        return false;
    auto list = [](const fs::path &root) {
        std::map<fs::path, bool> files; // relative path -> is a directory
        std::error_code e;
        for (auto it = fs::recursive_directory_iterator(root, e); !e && it != fs::recursive_directory_iterator(); it.increment(e))
            files[fs::relative(it->path(), root, e)] = it->is_directory(e);
        return files;
    };
    const auto left = list(a), right = list(b);
    if (left != right)
        return false;
    for (const auto &[relative, directory] : left) {
        if (directory)
            continue;
        std::ifstream x(a / relative, std::ios::binary), y(b / relative, std::ios::binary);
        if (!x || !y || !std::equal(std::istreambuf_iterator<char>(x), {}, std::istreambuf_iterator<char>(y), {}))
            return false;
        // std::equal with two ranges checks the lengths too.
    }
    return true;
}

bool prepare(const fs::path &game, const PrepareOptions &options, Host &host, std::string &why) {
    std::error_code ec;
    if (runPending(game)) {
        why = "An earlier extraction run has not been put back yet. Restore it first.";
        return false;
    }
    if (host.gameRunning(game)) {
        why = "Cube World is running. Quit it first.";
        return false;
    }
    uint32_t dllBuild = 0;
    std::string dllWhy;
    if (!dataCheck::gameBuild(options.extractorDll, dllBuild, dllWhy)) {
        why = "The extractor (" + utf8(options.extractorDll) + ") is missing or is not an x64 DLL. Rebuild CubeVulkan.";
        return false;
    }
    if (!fs::is_regular_file(game / "CubeModLoader.fip", ec)) {
        why = "CubeModLoader is not installed in the game folder. It loads the extractor; install it first.";
        return false;
    }
    for (const auto &r : options.requests)
        if (!plainName(r)) {
            why = "Invalid request name " + r;
            return false;
        }

    // The run's folder and its journal.
    const fs::path setup = setupFolder(game);
    fs::create_directories(setup, ec);
    const fs::path run = unusedPath(setup / ("run-" + (options.stamp.empty() ? localStamp() : options.stamp)));
    if (ec || !fs::create_directory(run, ec) || ec) {
        why = "Cannot create " + utf8(run) + ": " + plainError(ec);
        return false;
    }
    const fs::path journal = journalFile(game);
    bool ok = appendLine(journal, JOURNAL_HEADER) && appendLine(journal, "run " + utf8(run.filename()));
    auto fail = [&](const std::string &reason) {
        why = reason;
        if (runPending(game)) {
            RestoreResult back = restore(game, host);
            if (!back.ok)
                why += " Putting the folder back also failed: " + back.why + " Open the setup again to retry.";
        }
        return false;
    };
    if (!ok)
        return fail("Cannot write the journal " + utf8(journal) + ".");

    // 1. Save, copied whole and checked.
    if (fs::exists(game / "Save", ec)) {
        if (!appendLine(journal, "begin save") || !copyTree(game / "Save", run / "Save", ec) || !sameTree(game / "Save", run / "Save") ||
            !appendLine(journal, "done save"))
            return fail("Could not back up Save: " + plainError(ec));
    } else if (!appendLine(journal, "none save"))
        return fail("Cannot write the journal.");

    // 2. The owner's Mods and 3. ReShade's dxgi.dll, moved aside (a rename: nothing is copied).
    for (const auto &[step, name] : {std::pair<const char *, const char *>{"mods", "Mods"}, {"dxgi", "dxgi.dll"}}) {
        if (fs::exists(game / name, ec)) {
            if (!appendLine(journal, std::string("begin ") + step))
                return fail("Cannot write the journal.");
            fs::rename(game / name, run / name, ec);
            if (ec || !appendLine(journal, std::string("done ") + step))
                return fail(std::string("Could not set ") + name + " aside: " + plainError(ec) + ". Is a file in it open?");
        } else if (!appendLine(journal, std::string("none ") + step))
            return fail("Cannot write the journal.");
    }

    // 4. A fresh Mods folder with only the extractor.
    if (!appendLine(journal, "begin extractor"))
        return fail("Cannot write the journal.");
    fs::create_directory(game / "Mods", ec);
    if (!ec)
        fs::copy_file(options.extractorDll, game / "Mods" / "ZoneExtractor.dll", ec);
    if (ec || !appendLine(journal, "done extractor"))
        return fail("Could not install the extractor: " + plainError(ec));

    // 5. The requests (and a copy of the old tables they will replace).
    fs::create_directories(game / "Extract", ec);
    if (options.backupOracle && fs::is_directory(game / "Extract" / "oracle", ec) &&
        !copyTree(game / "Extract" / "oracle", run / "oracle-before", ec))
        return fail("Could not copy Extract\\oracle aside: " + plainError(ec));
    for (const auto &r : options.requests) {
        if (!appendLine(journal, "request " + r))
            return fail("Cannot write the journal.");
        std::ofstream out(game / "Extract" / r);
        out << "Written by CubeVulkanSetup; the extractor answers it and removes it.\n";
        if (!out)
            return fail("Could not write Extract\\" + r + ".");
    }
    return true;
}

RestoreResult restore(const fs::path &game, Host &host) {
    RestoreResult r;
    const fs::path journal = journalFile(game);
    std::error_code ec;
    if (!fs::exists(journal, ec)) {
        r.ok = true;
        return r;
    }
    if (host.gameRunning(game)) {
        r.why = "Cube World is still running. Quit it, then try again.";
        return r;
    }
    const Journal j = readJournal(journal);
    if (!j.valid) {
        r.why = "The journal " + utf8(journal) + " cannot be read. Nothing was changed; put the folder back by hand from CubeVulkanSetup\\run-*.";
        return r;
    }
    const fs::path run = setupFolder(game) / pathOfUtf8(j.run);
    r.runFolder = run;
    auto failed = [&](const std::string &what) {
        r.why = what + ": " + plainError(ec) + ". Close any program using the game folder and try again.";
        return r;
    };

    // Requests the extractor never answered (it removes the ones it answers).
    for (const auto &name : j.requests)
        if (plainName(name) && fs::exists(game / "Extract" / name, ec)) {
            fs::remove(game / "Extract" / name, ec); // the setup's own file
            r.steps.push_back("Removed the unanswered request Extract\\" + name + ".");
        }

    // The run's Mods folder: the one the setup made, never the owner's.
    const bool ownerModsAside = fs::exists(run / "Mods", ec);
    const bool runMods = j.begun.count("extractor") && (ownerModsAside || j.none.count("mods"));
    if ((runMods || ownerModsAside) && fs::exists(game / "Mods", ec)) {
        const fs::path keep = unusedPath(run / "Mods-used");
        fs::rename(game / "Mods", keep, ec);
        if (ec)
            return failed("Could not move the run's Mods folder away");
        r.steps.push_back("Moved the run's Mods folder (with the extractor and its log) to " + utf8(keep) + ".");
    }
    if (ownerModsAside) {
        fs::rename(run / "Mods", game / "Mods", ec);
        if (ec)
            return failed("Could not put your Mods folder back");
        r.steps.push_back("Put your Mods folder back.");
    }

    // ReShade.
    if (fs::exists(run / "dxgi.dll", ec)) {
        if (fs::exists(game / "dxgi.dll", ec)) {
            fs::rename(game / "dxgi.dll", unusedPath(run / "dxgi-found.dll"), ec);
            if (ec)
                return failed("Could not move a new dxgi.dll away");
        }
        fs::rename(run / "dxgi.dll", game / "dxgi.dll", ec);
        if (ec)
            return failed("Could not put dxgi.dll (ReShade) back");
        r.steps.push_back("Put dxgi.dll (ReShade) back.");
    }

    // Save, exactly as it was.
    if (j.done.count("save")) {
        if (!sameTree(run / "Save", game / "Save")) {
            if (fs::exists(game / "Save", ec)) {
                const fs::path keep = unusedPath(run / "Save-after-run");
                fs::rename(game / "Save", keep, ec);
                if (ec)
                    return failed("Could not move the played Save away");
                r.steps.push_back("Kept the Save of this session in " + utf8(keep) + ".");
            }
            if (!copyTree(run / "Save", game / "Save", ec) || !sameTree(run / "Save", game / "Save"))
                return failed("Could not put Save back");
            r.steps.push_back("Put Save back exactly as it was.");
        } else
            r.steps.push_back("Save is unchanged.");
    } else if (j.none.count("save") && j.begun.count("launch") && fs::exists(game / "Save", ec)) {
        // There was no Save before the run: the one the game made goes with the run.
        const fs::path keep = unusedPath(run / "Save-after-run");
        fs::rename(game / "Save", keep, ec);
        if (ec)
            return failed("Could not move the session's Save away");
        r.steps.push_back("There was no Save before; the session's Save is kept in " + utf8(keep) + ".");
    }

    // Closed: the journal goes with the run.
    appendLine(journal, "restored");
    fs::rename(journal, unusedPath(run / "journal.txt"), ec);
    if (ec)
        return failed("Everything is back, but the journal could not be closed");
    r.ok = true;
    return r;
}

// --- Watching the extractor ------------------------------------------------------------------

std::string LogWatcher::poll() {
    std::error_code ec;
    const uintmax_t size = fs::file_size(file_, ec);
    if (ec || size <= offset_)
        return {};
    std::ifstream in(file_, std::ios::binary);
    in.seekg((std::streamoff)offset_);
    std::string text((std::istreambuf_iterator<char>(in)), {});
    // Only whole lines: the extractor may be half way through one.
    const size_t end = text.rfind('\n');
    if (end == std::string::npos)
        return {};
    text.resize(end + 1);
    offset_ += text.size();
    all_ += text;
    return text;
}

std::vector<ExtractionRun::Target> ExtractionRun::plan(const Completeness &data) {
    std::vector<Target> targets;
    for (const auto &p : data.parts)
        if (!p.present && p.part != Part::GameFiles && (p.part != Part::World || p.required))
            targets.push_back({p.part, p.name, p.collect});
    return targets;
}

bool ExtractionRun::start(const Completeness &data, const fs::path &extractorDll, std::string &why) {
    if (active()) {
        why = "A run is already under way.";
        return false;
    }
    targets_ = plan(data);
    if (targets_.empty()) {
        why = "Nothing is missing.";
        return false;
    }
    PrepareOptions options;
    options.extractorDll = extractorDll;
    for (const auto &t : targets_) {
        if (!t.collect.request.empty())
            options.requests.push_back(t.collect.request);
        options.backupOracle |= t.part == Part::CreatureTables;
    }
    if (!prepare(game_, options, host_, why))
        return false;
    appendLine(journalFile(game_), "begin launch");
    if (!host_.launchGame()) {
        restored_ = restore(game_, host_);
        why = "Steam did not start Cube World." + std::string(restored_.ok ? " The game folder was put back." : " " + restored_.why);
        phase_ = restored_.ok ? Phase::Idle : Phase::Failed;
        return false;
    }
    log_ = LogWatcher(game_ / "Mods" / "ZoneExtractor.log");
    started_ = host_.nowMs();
    seenGame_ = closeAsked_ = restoreStarted_ = 0;
    sweepStopped_ = stopRequested_ = false;
    message_.clear();
    restored_ = {};
    phase_ = Phase::Launching;
    return true;
}

bool ExtractionRun::checkTarget(Target &t, const std::string &log) {
    std::string detail;
    if (t.collect.sweepKey) {
        // The newest sweep line says where it is.
        const size_t started = log.rfind("Sweep started"), stopped = log.rfind("Sweep stopped");
        sweepStopped_ = stopped != std::string::npos && (started == std::string::npos || stopped > started);
        const size_t stop = log.rfind("Stop ");
        if (stop != std::string::npos && (started == std::string::npos || stop > started)) {
            int at = 0, of = 0;
            if (std::sscanf(log.c_str() + stop, "Stop %d/%d", &at, &of) == 2)
                t.progress = "stop " + std::to_string(at) + " of " + std::to_string(of);
        }
        return log.find("Sweep finished") != std::string::npos && partPresent(game_, t.part, detail);
    }
    if (t.collect.automatic)
        return partPresent(game_, t.part, detail);
    return log.find(t.collect.logDone) != std::string::npos && partPresent(game_, t.part, detail);
}

bool ExtractionRun::allCollected() const {
    return !targets_.empty() && std::all_of(targets_.begin(), targets_.end(), [](const Target &t) { return t.done; });
}

void ExtractionRun::poll() {
    if (!active())
        return;
    const uint64_t now = host_.nowMs();
    if (phase_ == Phase::Restoring) {
        finish(message_);
        return;
    }
    const bool running = host_.gameRunning(game_);
    if (phase_ == Phase::Launching) {
        if (running)
            phase_ = Phase::AtTitle, seenGame_ = now;
        else if (stopRequested_)
            finish("Stopped before the game started.");
        return;
    }
    log_.poll();
    const std::string &log = log_.all();
    if (phase_ == Phase::AtTitle && log.find("Session: seed") != std::string::npos)
        phase_ = Phase::Collecting;
    if (phase_ == Phase::Collecting || phase_ == Phase::Collected) {
        for (auto &t : targets_)
            if (!t.done)
                t.done = checkTarget(t, log);
        if (allCollected())
            phase_ = Phase::Collected;
    }
    if (!running) {
        // The game has gone: quit, closed, or crashed. Put the folder back either way.
        std::string outcome;
        if (allCollected())
            outcome = "Everything asked for was collected.";
        else {
            outcome = "The game closed before everything was collected. Missing:";
            for (const auto &t : targets_)
                if (!t.done)
                    outcome += " " + t.name + ";";
            outcome += " run the extraction again to collect the rest.";
        }
        finish(outcome);
    }
}

void ExtractionRun::stop() {
    if (!active())
        return;
    stopRequested_ = true;
    if (host_.gameRunning(game_)) {
        host_.askGameToClose(game_);
        closeAsked_ = host_.nowMs();
    } else
        poll();
}

void ExtractionRun::finish(const std::string &outcome) {
    phase_ = Phase::Restoring;
    message_ = outcome;
    restored_ = restore(game_, host_);
    if (restored_.ok) {
        phase_ = Phase::Done;
        return;
    }
    // A game that has just exited can hold its files for a moment: retry for half a minute.
    static constexpr uint64_t RETRY_MS = 30000;
    const uint64_t now = host_.nowMs();
    if (!restoreStarted_)
        restoreStarted_ = now;
    if (now - restoreStarted_ > RETRY_MS) {
        phase_ = Phase::Failed;
        message_ = outcome + " Putting the game folder back failed: " + restored_.why +
                   " Open CubeVulkanSetup again to retry; the journal remembers what to put back.";
    }
}

std::string ExtractionRun::instructions() const {
    std::ostringstream out;
    auto sweepPending = [&] {
        for (const auto &t : targets_)
            if (t.collect.sweepKey && !t.done)
                return &t;
        return (const Target *)nullptr;
    };
    switch (phase_) {
    case Phase::Idle:
        return "";
    case Phase::Launching:
        out << "Cube World is starting through Steam, without your mods and ReShade.\n"
               "If it has not opened after a minute, start it from your Steam library yourself.";
        break;
    case Phase::AtTitle:
        out << "In Cube World:\n"
               "  1. Click Start Game.\n"
               "  2. Pick any character, or create a new one, and enter the world.\n"
               "Your Save folder is put back exactly as it was afterwards, so nothing done in this session is kept "
               "(a new character disappears again).";
        break;
    case Phase::Collecting:
    case Phase::Collected:
        if (phase_ == Phase::Collected) {
            out << "Everything is collected.\n"
                   "Quit Cube World now (Esc, then Exit), or click \"Close the game\".\n"
                   "Your mods, ReShade and Save are put back as soon as it has closed.";
            break;
        }
        out << "You are in the world. The extractor is writing the game's tables: the game may stop responding "
               "for a minute or two. That is normal; leave it running.\n";
        if (const Target *sweep = sweepPending()) {
            out << (sweepStopped_ ? "\nThe sweep was stopped (F11 was pressed again). Press F11 once to start it again.\n"
                                  : "\nPress F11 once to sweep the land around your character. It is moved from place to "
                                    "place and held there at full health; this takes several minutes. Do not press F11 "
                                    "again until this window says the sweep is done.\n");
            if (!sweep->progress.empty())
                out << "Sweep: " << sweep->progress << ".\n";
        }
        break;
    case Phase::Restoring:
        out << "The game has closed. Putting your game folder back...";
        break;
    case Phase::Done:
    case Phase::Failed:
        out << message_;
        break;
    }
    if (phase_ == Phase::Collecting || phase_ == Phase::AtTitle) {
        out << "\nCollecting:\n";
        for (const auto &t : targets_)
            out << (t.done ? "  [done]    " : "  [waiting] ") << t.name << (t.progress.empty() || t.done ? "" : " (" + t.progress + ")") << '\n';
    }
    return out.str();
}

// --- Headless use ----------------------------------------------------------------------------

bool isHeadlessCommand(const std::vector<std::wstring> &args) {
    for (const auto &a : args)
        if (a == L"--setup-check" || a == L"--setup-extract" || a == L"--setup-restore" || a == L"--setup-capture")
            return true;
    return false;
}

bool steamVerified(const fs::path &game, std::string &why) {
    SteamSession steam; // closed again at the end of this call, before the game is started
    if (!steam.start(game, why))
        return false;
    steam.pump();
    return steam.check(why);
}

namespace {
std::string hex8(uint32_t v) {
    std::ostringstream s;
    s << std::hex << std::setw(8) << std::setfill('0') << v;
    return s.str();
}
} // namespace

int headless(const std::vector<std::wstring> &args, const fs::path &setupRoot, std::ostream &out) {
    bool check = false, extract = false, dryRun = false, restoreRun = false, capture = false;
    fs::path game;
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i] == L"--setup-check")
            check = true;
        else if (args[i] == L"--setup-extract")
            extract = true;
        else if (args[i] == L"--setup-capture")
            capture = true;
        else if (args[i] == L"--setup-restore")
            restoreRun = true;
        else if (args[i] == L"--dry-run")
            dryRun = true;
        else if (args[i] == L"--game" && i + 1 < args.size())
            game = args[++i];
        else {
            out << "Unknown argument. Use --setup-check, --setup-extract, --setup-capture [--dry-run] or --setup-restore, with --game PATH optional.\n";
            return 2;
        }
    }
    Config config;
    config.load(setupRoot / "CubeVulkan.ini");
    out << "CubeVulkan setup\n\nSteam libraries:\n";
    const auto libraries = Assets::steamLibraries();
    for (const auto &l : libraries)
        out << "  " << utf8(l) << '\n';
    if (game.empty()) {
        Assets assets;
        if (assets.locate(config.text("Game", "Path")))
            game = assets.gameDir();
    }
    const GameStatus status = inspectGame(game);
    out << "\nGame: " << (game.empty() ? std::string("not found") : utf8(game)) << '\n';
    if (!status.found) {
        out << status.problem << " Use --game PATH.\n";
        return 2;
    }
    out << "Build: " << hex8(status.build) << (status.supportedBuild ? " (supported)" : " (not the supported build)") << '\n'
        << "CubeModLoader: " << (status.modLoader ? "present" : "missing") << '\n';
    auto host = systemHost();
    if (restoreRun) {
        if (!runPending(game)) {
            out << "No interrupted run: nothing to put back.\n";
            return 0;
        }
        const RestoreResult r = restore(game, *host);
        for (const auto &s : r.steps)
            out << "  " << s << '\n';
        out << (r.ok ? "The game folder is back as it was.\n" : r.why + "\n");
        return r.ok ? 0 : 2;
    }
    std::string why;
    const bool steamOk = steamVerified(game, why);
    out << "Steam: " << (steamOk ? "online, Cube World licence verified" : why) << '\n';
    if (runPending(game))
        out << "An earlier extraction run was interrupted and has not been put back. Run --setup-restore.\n";
    const dataCheck::Report report = dataCheck::inspect(game, pathOfUtf8(config.text("World", "ExtractedPath")));
    const Completeness data = assess(game, worldRequired(setupRoot / "CubeVulkan.ini"), &report);
    out << "\nExtracted data:\n" << data.text();
    if (check)
        return !steamOk ? 2 : data.readyToPlay() ? 0 : 1;
    if (!extract && !capture)
        return 0;

    // --setup-extract: what a run would do.
    const auto targets = ExtractionRun::plan(data);
    const fs::path dll = setupRoot / "tools" / "ZoneExtractor.dll";
    uint32_t dllBuild = 0;
    std::string dllWhy;
    const bool dllOk = dataCheck::gameBuild(dll, dllBuild, dllWhy);
    std::vector<std::string> blockers;
    if (!steamOk)
        blockers.push_back("Steam must be online with a Cube World licence.");
    if (!status.supportedBuild)
        blockers.push_back("The game build is not the one the extractor supports.");
    if (!status.modLoader)
        blockers.push_back("CubeModLoader is not installed in the game folder.");
    if (!dllOk)
        blockers.push_back("The extractor " + utf8(dll) + " is missing or not an x64 DLL.");
    if (runPending(game))
        blockers.push_back("An earlier run must be put back first (--setup-restore).");
    if (host->gameRunning(game))
        blockers.push_back("Cube World is running; quit it first.");
    if (capture) {
        out << "\nIsolated reference capture" << (dryRun ? " (dry run)" : "") << ":\n"
            << "  Back up Save and temporarily set aside Mods and ReShade.\n"
            << "  Install only ZoneExtractor; existing Extract data stays in place.\n"
            << "  Launch Cube World; the extractor captures once a world is available.\n"
            << "  F5 and O remain available for manual refreshes. Quit afterward.\n"
            << "  Run --setup-restore to restore Mods, ReShade and Save exactly.\n";
        for (const auto &blocker : blockers)
            out << "  Cannot run now: " << blocker << '\n';
        if (dryRun || !blockers.empty())
            return blockers.empty() ? 0 : 2;
        PrepareOptions options;
        options.extractorDll = dll;
        options.requests = {"scene-reference.request"};
        if (!prepare(game, options, *host, why)) {
            out << why << '\n';
            return 2;
        }
        out << "Capture prepared. Recovery journal: " << utf8(journalFile(game)) << '\n';
        return 0;
    }
    out << "\nExtraction run" << (dryRun ? " (dry run: nothing is changed)" : "") << ":\n";
    if (targets.empty()) {
        out << "  Nothing to extract.\n";
        return 1;
    }
    std::error_code ec;
    out << "  1. Copy Save to " << utf8(setupFolder(game)) << "\\run-<time>\\Save"
        << (fs::exists(game / "Save", ec) ? "" : " (there is no Save yet)") << ".\n"
        << "  2. Move Mods aside" << (fs::exists(game / "Mods", ec) ? "" : " (there is no Mods folder)") << " and dxgi.dll aside"
        << (fs::exists(game / "dxgi.dll", ec) ? "" : " (there is none)") << ".\n"
        << "  3. Install ZoneExtractor.dll alone in a new Mods folder (loaded by CubeModLoader).\n"
        << "  4. Write the requests:";
    bool anyRequest = false;
    for (const auto &t : targets)
        if (!t.collect.request.empty())
            out << " Extract\\" << t.collect.request, anyRequest = true;
    out << (anyRequest ? "" : " none") << ".\n"
        << "  5. Start Cube World through Steam. In the game: Start Game, pick or create any character.\n";
    for (const auto &t : targets)
        if (t.collect.sweepKey)
            out << "     Then press F11 once for the sweep around home, and wait until it is done.\n";
    out << "  6. Quit the game; Mods, dxgi.dll and Save are put back exactly.\n  Collects:";
    for (const auto &t : targets)
        out << " " << t.name << ";";
    out << '\n';
    for (const auto &b : blockers)
        out << "  Cannot run now: " << b << '\n';
    if (dryRun)
        return blockers.empty() ? 0 : 2;
    if (!blockers.empty())
        return 2;

    // The real run, from the console.
    ExtractionRun run(game, *host);
    if (!run.start(data, dll, why)) {
        out << why << '\n';
        return 2;
    }
    std::string shown;
    while (run.active()) {
        run.poll();
        const std::string now = run.instructions();
        if (now != shown)
            out << "\n" << now << '\n' << std::flush, shown = now;
        Sleep(1000);
    }
    for (const auto &s : run.restored().steps)
        out << "  " << s << '\n';
    out << run.message() << '\n';
    return run.phase() == ExtractionRun::Phase::Done && run.allCollected() ? 0 : 1;
}
} // namespace ownersetup
