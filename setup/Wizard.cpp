// The extraction panel (Wizard.h): ready or not in plain words, and one button that extracts
// what is missing. The button sets the game folder up for a vanilla run (Save copied aside, the
// owner's Mods and ReShade set aside, the extractor alone in a fresh Mods folder), starts the
// game through Steam, says what to do in it, watches the extractor and puts everything back.
#include "Wizard.h"
#include "Assets.h"
#include "OwnerSetup.h"
#include <memory>
#include <string>

namespace wizard {
namespace {
using namespace ownersetup;

// Control ids below the main window's (which start at 100).
enum Id { RUN_TEXT = 1, EXTRACT, STOP };
constexpr UINT_PTR POLL_TIMER = 1;
constexpr UINT POLL_MS = 500;

struct State {
    HWND parent = nullptr;
    HFONT font = nullptr;
    fs::path root, game;
    GameStatus status;
    bool steamOk = false;
    std::string steamWhy;
    Completeness data;
    std::unique_ptr<Host> host = systemHost();
    std::unique_ptr<ExtractionRun> run;
    std::string lastRunText, lastOutcome;
};
State *state = nullptr;

std::wstring wide(const std::string &value) {
    int n = MultiByteToWideChar(CP_UTF8, 0, value.data(), (int)value.size(), nullptr, 0);
    std::wstring out(n, L'\0');
    if (n)
        MultiByteToWideChar(CP_UTF8, 0, value.data(), (int)value.size(), out.data(), n);
    return out;
}
// Edit controls want \r\n.
void setText(const std::string &text) {
    std::string crlf;
    for (char c : text) {
        if (c == '\n')
            crlf += '\r';
        crlf += c;
    }
    SetWindowTextW(GetDlgItem(state->parent, RUN_TEXT), wide(crlf).c_str());
}
void enable(int id, bool on) { EnableWindow(GetDlgItem(state->parent, id), on); }
void show(int id, bool on) { ShowWindow(GetDlgItem(state->parent, id), on ? SW_SHOW : SW_HIDE); }
fs::path extractorDll() { return state->root / "tools" / "ZoneExtractor.dll"; }

void showButtons() {
    State &s = *state;
    const bool running = s.run && s.run->active();
    const bool pending = runPending(s.game);
    uint32_t build = 0;
    std::string why;
    const bool dll = dataCheck::gameBuild(extractorDll(), build, why);
    const bool needed = s.status.found && s.data.anythingToExtract();
    show(EXTRACT, needed || running);
    enable(EXTRACT, !running && !pending && s.status.supportedBuild && s.status.modLoader && s.steamOk && dll && needed);
    show(STOP, running);
    enable(STOP, running);
}

// The one message the owner reads: ready, or what is missing and what to do about it.
std::string summary() {
    State &s = *state;
    if (!s.status.found)
        return s.status.problem.empty() ? "Choose the folder that contains cubeworld.exe above." : s.status.problem;
    if (!s.steamOk)
        return "Start Steam and sign in with the account that owns Cube World, then reopen this window.\n(" + s.steamWhy + ")";
    if (!s.data.anythingToExtract())
        return s.data.readyToPlay() ? "Everything is ready. Click Play." : s.data.text();
    std::string names;
    for (const PartState *p : s.data.missing())
        names += (names.empty() ? "" : ", ") + p->name;
    std::string text = "Some game data is still missing (" + names + "). Click \"Extract game data\": your game opens "
                       "through Steam with only the extractor; start or continue any character, wait until this box says "
                       "it is done, then quit the game. Your saves and mods are put back exactly.";
    if (!s.status.supportedBuild)
        text = "This Cube World build is not the supported one, so its data cannot be extracted. " + s.status.problem;
    else if (!s.status.modLoader)
        text = "Install CubeModLoader in the game folder first (it loads the extractor), then click \"Extract game data\".";
    return text;
}

void recheck() {
    State &s = *state;
    std::string outcome;
    if (runPending(s.game)) {
        if (s.host->gameRunning(s.game))
            outcome = "An earlier extraction is still open. Quit Cube World; this window then puts your game folder back.\n";
        else {
            const RestoreResult back = restore(s.game, *s.host);
            outcome = back.ok ? "An earlier extraction was interrupted; your game folder has been put back.\n"
                              : "An earlier extraction was interrupted, and putting it back failed: " + back.why + "\n";
        }
    }
    SetCursor(LoadCursor(nullptr, IDC_WAIT));
    s.status = inspectGame(s.game);
    s.steamOk = s.status.found && steamVerified(s.game, s.steamWhy);
    if (s.status.found) {
        const dataCheck::Report report = dataCheck::inspect(s.game);
        s.data = assessConfigured(s.game, s.root / "CubeVulkan.ini", &report);
    } else
        s.data = {};
    setText((s.lastOutcome.empty() ? "" : s.lastOutcome + "\n") + outcome + summary());
    SetCursor(LoadCursor(nullptr, IDC_ARROW));
    showButtons();
}

void pollRun() {
    State &s = *state;
    if (!s.run)
        return;
    s.run->poll();
    const std::string text = s.run->instructions();
    if (text != s.lastRunText)
        setText(text), s.lastRunText = text;
    showButtons();
    if (s.run->active())
        return;
    KillTimer(s.parent, POLL_TIMER);
    s.lastOutcome = s.run->message();
    s.run.reset();
    recheck();
    FLASHWINFO flash{sizeof flash, s.parent, FLASHW_ALL | FLASHW_TIMERNOFG, 3, 0};
    FlashWindowEx(&flash);
}

HWND add(const wchar_t *cls, const wchar_t *caption, DWORD style, int x, int y, int w, int h, int id) {
    const DWORD ex = wcscmp(cls, L"EDIT") == 0 ? WS_EX_CLIENTEDGE : 0;
    HWND c = CreateWindowExW(ex, cls, caption, WS_CHILD | WS_VISIBLE | style, x, y, w, h, state->parent,
                             (HMENU)(INT_PTR)id, nullptr, nullptr);
    SendMessageW(c, WM_SETFONT, (WPARAM)state->font, TRUE);
    return c;
}
} // namespace

void embed(HWND parent, const fs::path &root, const fs::path &game, HFONT font, int x, int y, int width, int height) {
    state = new State;
    state->parent = parent;
    state->root = root;
    state->font = font;
    add(L"EDIT", L"", ES_READONLY | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL, x, y, width, height - 36, RUN_TEXT);
    add(L"BUTTON", L"Extract game data", BS_PUSHBUTTON | WS_TABSTOP, x, y + height - 30, 150, 28, EXTRACT);
    add(L"BUTTON", L"Stop and put back", BS_PUSHBUTTON | WS_TABSTOP, x + 158, y + height - 30, 140, 28, STOP);
    fs::path start = game;
    if (start.empty() || !inspectGame(start).found) {
        const auto found = gamesInLibraries(Assets::steamLibraries());
        if (!found.empty())
            start = found.front();
    }
    setGame(start);
}

void setGame(const fs::path &game) {
    if (!state || (state->run && state->run->active()))
        return;
    state->game = game;
    state->lastOutcome.clear();
    recheck();
}

bool command(WPARAM wp) {
    if (!state)
        return false;
    switch (LOWORD(wp)) {
    case EXTRACT: extract(); return true;
    case STOP:
        if (state->run)
            state->run->stop(), pollRun();
        return true;
    }
    return false;
}

bool timer(WPARAM wp) {
    if (!state || wp != POLL_TIMER)
        return false;
    pollRun();
    return true;
}

bool readyToPlay() {
    return state && state->status.found && state->steamOk && !runPending(state->game) && !(state->run && state->run->active()) &&
           state->data.readyToPlay();
}

void extract() {
    State &s = *state;
    if (s.run && s.run->active())
        return;
    if (MessageBoxW(s.parent,
                    L"Your game will open through Steam with only CubeVulkan's extractor:\n"
                    L"  - your Save folder is copied aside and put back exactly afterwards,\n"
                    L"  - your Mods folder and ReShade are set aside and put back afterwards.\n\n"
                    L"In the game, start or continue any character and wait until the setup says it is done, "
                    L"then quit. Continue?",
                    L"Extract game data", MB_OKCANCEL | MB_ICONINFORMATION) != IDOK)
        return;
    // The licence once more, right before the game folder is touched.
    if (!steamVerified(s.game, s.steamWhy)) {
        s.steamOk = false;
        MessageBoxW(s.parent, wide("Steam: " + s.steamWhy).c_str(), L"Extract game data", MB_ICONWARNING);
        showButtons();
        return;
    }
    s.run = std::make_unique<ExtractionRun>(s.game, *s.host);
    std::string why;
    if (!s.run->start(s.data, extractorDll(), why)) {
        MessageBoxW(s.parent, wide(why).c_str(), L"Extract game data", MB_ICONWARNING);
        s.run.reset();
        recheck();
        return;
    }
    s.lastOutcome.clear();
    s.lastRunText.clear();
    SetTimer(s.parent, POLL_TIMER, POLL_MS, nullptr);
    showButtons();
}

bool mayClose() {
    if (!state || !state->run || !state->run->active())
        return true;
    if (state->host->gameRunning(state->game)) {
        MessageBoxW(state->parent,
                    L"An extraction is running. Quit Cube World first; the setup then puts your Mods, ReShade and Save "
                    L"back. (If this window is closed anyway, the setup puts them back the next time it opens.)",
                    L"CubeVulkan setup", MB_ICONINFORMATION);
        return false;
    }
    state->run->stop();
    pollRun();
    return true;
}
} // namespace wizard
