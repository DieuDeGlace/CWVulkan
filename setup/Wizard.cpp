// The first-run wizard: from a fresh install to playing in one window.
//   1. the game: its folder, build, CubeModLoader, Steam and the licence;
//   2. the data: what is extracted and what is missing, in plain words;
//   3. the extraction: one button sets the folder up for a vanilla run, starts the game, says
//      what to do in it, watches the extractor and puts everything back afterwards;
//   4. Play.
#include "Wizard.h"
#include "Assets.h"
#include "OwnerSetup.h"
#include <shellapi.h>
#include <memory>
#include <string>

namespace wizard {
namespace {
using namespace ownersetup;

enum Id { GAME_TEXT = 1, DATA_TEXT, RUN_TEXT, EXTRACT, CLOSE_GAME, STOP, RECHECK, PLAY, CLOSE };
constexpr UINT_PTR POLL_TIMER = 1;
constexpr UINT POLL_MS = 500;

struct State {
    HWND window = nullptr, owner = nullptr;
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
void setText(int id, const std::string &text) {
    std::string crlf;
    for (char c : text) {
        if (c == '\n')
            crlf += '\r';
        crlf += c;
    }
    SetWindowTextW(GetDlgItem(state->window, id), wide(crlf).c_str());
}
void enable(int id, bool on) { EnableWindow(GetDlgItem(state->window, id), on); }
fs::path extractorDll() { return state->root / "tools" / "ZoneExtractor.dll"; }

void showButtons() {
    State &s = *state;
    const bool running = s.run && s.run->active();
    const bool pending = runPending(s.game);
    uint32_t build = 0;
    std::string why;
    const bool dll = dataCheck::gameBuild(extractorDll(), build, why);
    enable(EXTRACT, !running && !pending && s.status.supportedBuild && s.status.modLoader && s.steamOk && dll &&
                        s.data.anythingToExtract());
    enable(CLOSE_GAME, running && s.run->phase() != ExtractionRun::Phase::Launching);
    enable(STOP, running);
    enable(RECHECK, !running);
    enable(PLAY, !running && !pending && s.status.found && s.steamOk && s.data.readyToPlay());
}

// Looks at everything again: an interrupted run first, then the game, Steam and the data.
void recheck() {
    State &s = *state;
    std::string runText;
    if (runPending(s.game)) {
        if (s.host->gameRunning(s.game))
            runText = "An earlier extraction run is still open. Quit Cube World, then click Recheck to put your game "
                      "folder back.";
        else {
            const RestoreResult back = restore(s.game, *s.host);
            runText = back.ok ? "An earlier extraction run was interrupted. Your game folder has been put back:\n"
                              : "An earlier extraction run was interrupted, and putting it back failed: " + back.why + "\n";
            for (const auto &step : back.steps)
                runText += "  " + step + "\n";
        }
    }
    SetCursor(LoadCursor(nullptr, IDC_WAIT));
    s.status = inspectGame(s.game);
    std::string game = "Folder: " + (s.game.empty() ? std::string("not found") : utf8(s.game)) + "\n";
    if (!s.status.found)
        game += s.status.problem + " Set the folder in the setup window.";
    else {
        s.steamOk = steamVerified(s.game, s.steamWhy);
        game += s.status.problem.empty() ? "Supported Cube World build. " : s.status.problem + " ";
        game += s.status.modLoader ? "CubeModLoader is installed.\n" : "CubeModLoader is missing (the extractor needs it).\n";
        game += s.steamOk ? "Steam is online and this account owns Cube World." : "Steam: " + s.steamWhy;
    }
    setText(GAME_TEXT, game);
    if (s.status.found) {
        const dataCheck::Report report = dataCheck::inspect(s.game);
        s.data = assess(s.game, worldRequired(s.root / "CubeVulkan.ini"), &report);
        setText(DATA_TEXT, s.data.text());
    } else {
        s.data = {};
        setText(DATA_TEXT, "");
    }
    if (runText.empty()) {
        if (!s.lastOutcome.empty())
            runText = s.lastOutcome;
        else if (!s.status.found)
            runText = "";
        else if (!s.data.anythingToExtract())
            runText = "Nothing is missing. Click Play.";
        else
            runText = "Click \"Extract missing data\". The setup copies your Save aside, sets your Mods folder and "
                      "ReShade (dxgi.dll) aside, installs the extractor and starts Cube World through Steam. It tells "
                      "you what to do in the game, and puts everything back exactly when you quit.";
    }
    setText(RUN_TEXT, runText);
    SetCursor(LoadCursor(nullptr, IDC_ARROW));
    showButtons();
}

void startExtraction() {
    State &s = *state;
    if (MessageBoxW(s.window,
                    L"The setup will now:\n"
                    L"  - copy your Save folder aside (it is put back exactly afterwards),\n"
                    L"  - set your Mods folder and dxgi.dll (ReShade) aside for a vanilla session,\n"
                    L"  - install ZoneExtractor.dll alone in a fresh Mods folder,\n"
                    L"  - start Cube World through Steam.\n\n"
                    L"When you quit the game, everything is put back. Continue?",
                    L"Extract missing data", MB_OKCANCEL | MB_ICONINFORMATION) != IDOK)
        return;
    // The licence once more, right before the game folder is touched.
    if (!steamVerified(s.game, s.steamWhy)) {
        s.steamOk = false;
        MessageBoxW(s.window, wide("Steam: " + s.steamWhy).c_str(), L"Extract missing data", MB_ICONWARNING);
        showButtons();
        return;
    }
    s.run = std::make_unique<ExtractionRun>(s.game, *s.host);
    std::string why;
    if (!s.run->start(s.data, extractorDll(), why)) {
        MessageBoxW(s.window, wide(why).c_str(), L"Extract missing data", MB_ICONWARNING);
        s.run.reset();
        recheck();
        return;
    }
    s.lastOutcome.clear();
    s.lastRunText.clear();
    SetTimer(s.window, POLL_TIMER, POLL_MS, nullptr);
    showButtons();
}

void pollRun() {
    State &s = *state;
    if (!s.run)
        return;
    s.run->poll();
    const std::string text = s.run->instructions();
    if (text != s.lastRunText)
        setText(RUN_TEXT, text), s.lastRunText = text;
    showButtons();
    if (s.run->active())
        return;
    KillTimer(s.window, POLL_TIMER);
    std::string outcome = s.run->message() + "\n";
    for (const auto &step : s.run->restored().steps)
        outcome += "  " + step + "\n";
    s.lastOutcome = outcome;
    s.run.reset();
    recheck(); // the final report
    FLASHWINFO flash{sizeof flash, s.window, FLASHW_ALL | FLASHW_TIMERNOFG, 3, 0};
    FlashWindowEx(&flash);
}

void play() {
    State &s = *state;
    WritePrivateProfileStringW(L"Game", L"Path", s.game.c_str(), (s.root / "CubeVulkan.ini").c_str());
    ShellExecuteW(s.window, L"open", (s.root / "CubeVulkan.exe").c_str(), nullptr, s.root.c_str(), SW_SHOWNORMAL);
    PostMessageW(s.owner, WM_CLOSE, 0, 0);
}

// Closing while a run is open: the folder must be put back first.
bool mayClose() {
    State &s = *state;
    if (!s.run || !s.run->active())
        return true;
    if (s.host->gameRunning(s.game)) {
        MessageBoxW(s.window,
                    L"An extraction is running. Quit Cube World first; the setup then puts your Mods, ReShade and Save "
                    L"back. (If this window is closed anyway, the setup puts them back the next time it opens.)",
                    L"CubeVulkan setup", MB_ICONINFORMATION);
        return false;
    }
    s.run->stop();
    pollRun();
    return true;
}

HWND add(const wchar_t *cls, const wchar_t *caption, DWORD style, int x, int y, int w, int h, int id = 0) {
    const DWORD ex = wcscmp(cls, L"EDIT") == 0 ? WS_EX_CLIENTEDGE : 0;
    HWND c = CreateWindowExW(ex, cls, caption, WS_CHILD | WS_VISIBLE | style, x, y, w, h, state->window,
                             (HMENU)(INT_PTR)id, nullptr, nullptr);
    SendMessageW(c, WM_SETFONT, (WPARAM)state->font, TRUE);
    return c;
}

LRESULT CALLBACK proc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case EXTRACT: startExtraction(); return 0;
        case CLOSE_GAME:
            if (state->run && !state->host->askGameToClose(state->game))
                MessageBoxW(h, L"No Cube World window was found. Quit the game yourself.", L"CubeVulkan setup", MB_ICONINFORMATION);
            return 0;
        case STOP:
            if (state->run)
                state->run->stop(), pollRun();
            return 0;
        case RECHECK: state->lastOutcome.clear(); recheck(); return 0;
        case PLAY: play(); return 0;
        case CLOSE:
            if (mayClose())
                DestroyWindow(h);
            return 0;
        }
        break;
    case WM_TIMER:
        if (wp == POLL_TIMER)
            pollRun();
        return 0;
    case WM_CLOSE:
        if (mayClose())
            DestroyWindow(h);
        return 0;
    case WM_CTLCOLORSTATIC:
        if ((HWND)lp != GetDlgItem(h, DATA_TEXT) && (HWND)lp != GetDlgItem(h, RUN_TEXT)) {
            SetBkMode((HDC)wp, TRANSPARENT);
            return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
        }
        break;
    case WM_DESTROY:
        KillTimer(h, POLL_TIMER);
        delete state;
        state = nullptr;
        return 0;
    }
    return DefWindowProcW(h, msg, wp, lp);
}
} // namespace

void open(HWND owner, const fs::path &root, const fs::path &game, HFONT font) {
    if (state) {
        SetForegroundWindow(state->window);
        return;
    }
    WNDCLASSW wc{};
    wc.lpfnWndProc = proc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.lpszClassName = L"CubeVulkanSetupWizard";
    RegisterClassW(&wc);
    state = new State;
    state->owner = owner;
    state->root = root;
    state->font = font;
    state->game = game;
    if (state->game.empty() || !inspectGame(state->game).found) {
        const auto found = gamesInLibraries(Assets::steamLibraries());
        if (!found.empty())
            state->game = found.front();
    }
    state->window = CreateWindowW(wc.lpszClassName, L"CubeVulkan - get ready to play", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                                  CW_USEDEFAULT, CW_USEDEFAULT, 780, 760, owner, nullptr, wc.hInstance, nullptr);
    constexpr DWORD READ_ONLY = ES_READONLY | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL;
    int y = 10;
    add(L"BUTTON", L"1. The game", BS_GROUPBOX, 10, y, 744, 92);
    add(L"STATIC", L"", 0, 22, y + 22, 720, 62, GAME_TEXT);
    y += 102;
    add(L"BUTTON", L"2. What is extracted", BS_GROUPBOX, 10, y, 744, 250);
    add(L"EDIT", L"", READ_ONLY, 22, y + 22, 720, 216, DATA_TEXT);
    y += 260;
    add(L"BUTTON", L"3. Extract what is missing", BS_GROUPBOX, 10, y, 744, 272);
    add(L"EDIT", L"", READ_ONLY, 22, y + 22, 720, 200, RUN_TEXT);
    add(L"BUTTON", L"Extract missing data", BS_DEFPUSHBUTTON | WS_TABSTOP, 22, y + 232, 160, 28, EXTRACT);
    add(L"BUTTON", L"Close the game", BS_PUSHBUTTON | WS_TABSTOP, 190, y + 232, 130, 28, CLOSE_GAME);
    add(L"BUTTON", L"Stop and put back", BS_PUSHBUTTON | WS_TABSTOP, 328, y + 232, 140, 28, STOP);
    y += 282;
    add(L"BUTTON", L"Recheck", BS_PUSHBUTTON | WS_TABSTOP, 22, y, 100, 30, RECHECK);
    add(L"BUTTON", L"Play", BS_PUSHBUTTON | WS_TABSTOP, 552, y, 96, 30, PLAY);
    add(L"BUTTON", L"Close", BS_PUSHBUTTON | WS_TABSTOP, 656, y, 96, 30, CLOSE);
    ShowWindow(state->window, SW_SHOW);
    UpdateWindow(state->window);
    recheck();
}

bool dialogMessage(MSG &msg) { return state && IsDialogMessageW(state->window, &msg); }
} // namespace wizard
