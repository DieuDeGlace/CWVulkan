// CubeVulkan Setup: the launcher/configuration tool (like OpenMW's launcher).
// Finds and checks the Steam Cube World install, shows the extracted world,
// installs or removes the ZoneExtractor mod on request, edits CubeVulkan.ini
// and starts CubeVulkan. Its first-run wizard (Wizard.cpp) does the whole extraction in one
// go; `--setup-check`, `--setup-extract [--dry-run]` and `--setup-restore` run it headless.
#include "Assets.h"
#include "Extracted.h"
#include "DataCheck.h"
#include "OwnerSetup.h"
#include "SteamSession.h"
#include "Wizard.h"
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shellapi.h>
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace {
enum Id {
    PATH_EDIT = 100, BROWSE, STATUS, WORLD_STATUS, INSTALL_EXTRACTOR, REMOVE_EXTRACTOR, SOURCE, STYLE, WIDTH,
    HEIGHT, VSYNC, VIEW, SHADOWS, SHADOW_RES, SAVE, PLAY, CLOSE_BUTTON, CHECK_DATA, WIZARD
};

HWND window, controls[200];
HWND reportPane;
fs::path root, ini;
HFONT font;

std::wstring text(HWND h) {
    wchar_t buffer[1024]{};
    GetWindowTextW(h, buffer, 1024);
    return buffer;
}
std::wstring readIni(const wchar_t *section, const wchar_t *key, const wchar_t *fallback) {
    wchar_t buffer[512]{};
    GetPrivateProfileStringW(section, key, fallback, buffer, 512, ini.c_str());
    return buffer;
}
void writeIni(const wchar_t *section, const wchar_t *key, const std::wstring &value) {
    WritePrivateProfileStringW(section, key, value.c_str(), ini.c_str());
}

HWND add(const wchar_t *cls, const wchar_t *caption, DWORD style, int x, int y, int w, int h, int id = 0) {
    HWND c = CreateWindowExW(cls == std::wstring(L"EDIT") ? WS_EX_CLIENTEDGE : 0, cls, caption,
                             WS_CHILD | WS_VISIBLE | style, x, y, w, h, window, (HMENU)(INT_PTR)id, nullptr, nullptr);
    SendMessageW(c, WM_SETFONT, (WPARAM)font, TRUE);
    if (id)
        controls[id - 100] = c;
    return c;
}
HWND control(int id) { return controls[id - 100]; }

std::wstring wide(const std::string &value) {
    int n = MultiByteToWideChar(CP_UTF8, 0, value.data(), (int)value.size(), nullptr, 0);
    std::wstring out(n, L'\0');
    if (n) MultiByteToWideChar(CP_UTF8, 0, value.data(), (int)value.size(), out.data(), n);
    return out;
}

LRESULT CALLBACK reportProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_SIZE) {
        MoveWindow(GetDlgItem(h, 1), 10, 10, std::max(10, (int)LOWORD(lp) - 20), std::max(10, (int)HIWORD(lp) - 58), TRUE);
        MoveWindow(GetDlgItem(h, 2), std::max(10, (int)LOWORD(lp) - 100), std::max(10, (int)HIWORD(lp) - 38), 90, 28, TRUE);
    } else if (msg == WM_COMMAND && LOWORD(wp) == 2) { DestroyWindow(h); return 0; }
    else if (msg == WM_DESTROY && h == reportPane) reportPane = nullptr;
    return DefWindowProcW(h, msg, wp, lp);
}

void showReport(const std::string &report) {
    if (reportPane) DestroyWindow(reportPane);
    WNDCLASSW wc{}; wc.lpfnWndProc = reportProc; wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW); wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    wc.lpszClassName = L"CubeVulkanDataReport"; RegisterClassW(&wc);
    HWND pane = CreateWindowW(wc.lpszClassName, L"CubeVulkan - owner data check", WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, 860, 680, window, nullptr, wc.hInstance, nullptr);
    reportPane = pane;
    std::string crlf;
    for (char c : report) { if (c == '\n') crlf += '\r'; crlf += c; }
    HWND edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", wide(crlf).c_str(), WS_CHILD | WS_VISIBLE | WS_VSCROLL |
                                ES_READONLY | ES_MULTILINE | ES_AUTOVSCROLL, 10, 10, 820, 570, pane, (HMENU)1, wc.hInstance, nullptr);
    SendMessageW(edit, EM_SETLIMITTEXT, 1024 * 1024, 0);
    SendMessageW(edit, WM_SETFONT, (WPARAM)font, TRUE);
    HWND close = CreateWindowW(L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                               730, 590, 90, 28, pane, (HMENU)2, wc.hInstance, nullptr);
    SendMessageW(close, WM_SETFONT, (WPARAM)font, TRUE);
    RECT area; GetClientRect(pane, &area);
    SendMessageW(pane, WM_SIZE, 0, MAKELPARAM(area.right, area.bottom));
    ShowWindow(pane, SW_SHOW);
}

void checkData() {
    SteamSession steam; std::string why;
    fs::path game = text(control(PATH_EDIT));
    if (!steam.start(game, why)) { MessageBoxW(window, wide(why).c_str(), L"CubeVulkan data check", MB_ICONERROR); return; }
    EnableWindow(control(CHECK_DATA), FALSE); SetWindowTextW(control(CHECK_DATA), L"Checking..."); UpdateWindow(window);
    try {
        auto report = dataCheck::inspect(game, fs::path(readIni(L"World", L"ExtractedPath", L"")));
        steam.pump();
        if (steam.check(why)) showReport(report.text());
        else MessageBoxW(window, wide(why).c_str(), L"CubeVulkan data check", MB_ICONERROR);
    } catch (const std::exception &e) { MessageBoxW(window, wide(e.what()).c_str(), L"CubeVulkan data check", MB_ICONERROR); }
    SetWindowTextW(control(CHECK_DATA), L"Check data..."); EnableWindow(control(CHECK_DATA), TRUE);
}

// Query the exact installation; never replace a DLL in a running game.
bool gameClosed(const fs::path &game, std::wstring &error) {
    const bool closed = !ownersetup::systemHost()->gameRunning(game);
    if (!closed) error = L"Close Cube World before installing, updating or removing its extractor.";
    if (closed && ownersetup::runPending(game)) {
        error = L"An extraction run from the setup wizard is still open. Open the wizard to put the game folder back first.";
        return false;
    }
    return closed;
}

void combo(int id, std::initializer_list<const wchar_t *> items, const std::wstring &selected) {
    HWND c = control(id);
    int index = 0, i = 0;
    for (auto item : items) {
        SendMessageW(c, CB_ADDSTRING, 0, (LPARAM)item);
        if (selected == item)
            index = i;
        i++;
    }
    SendMessageW(c, CB_SETCURSEL, index, 0);
}
std::wstring selection(int id) {
    HWND c = control(id);
    int i = (int)SendMessageW(c, CB_GETCURSEL, 0, 0);
    wchar_t buffer[128]{};
    SendMessageW(c, CB_GETLBTEXT, i, (LPARAM)buffer);
    return buffer;
}

// Checks the game folder and describes what was found.
void refresh() {
    fs::path game = text(control(PATH_EDIT));
    std::wstring status;
    std::error_code ec;
    bool ok = fs::exists(game / "cubeworld.exe", ec);
    bool supported = false;
    if (!ok)
        status = L"cubeworld.exe not found in this folder.";
    else {
        uint32_t stamp = 0; std::string why;
        ok = dataCheck::gameBuild(game / "cubeworld.exe", stamp, why);
        supported = ok && stamp == dataCheck::SUPPORTED_BUILD;
        size_t sprites = 0, music = 0;
        for (auto &e : fs::directory_iterator(game / "sprites", ec))
            sprites += e.path().extension() == ".cub";
        for (auto &e : fs::directory_iterator(game / "music", ec))
            music += e.path().extension() == ".ogg";
        status = L"Steam Cube World found: " + std::to_wstring(sprites) + L" sprites, " + std::to_wstring(music) +
                 L" music tracks. Game build " +
                 (supported ? std::wstring(L"supported (extractor works).")
                                               : std::wstring(L"differs from the supported one; extraction is unavailable."));
        if (!ok) status = wide(why);
    }
    SetWindowTextW(control(STATUS), status.c_str());

    std::wstring world;
    fs::path dir = ok ? ExtractedWorld::find(game) : fs::path();
    ExtractedWorld ex;
    std::string why;
    if (!dir.empty() && ex.open(dir, why))
        world = L"Extracted world: " + std::to_wstring(ex.zoneCount()) + L" zones from seed " + std::to_wstring(ex.seed()) +
                L" (" + dir.filename().wstring() + L").";
    else
        world = L"No extracted world yet. Install the extractor, play Cube World once, press F11 in game to sweep the area "
                L"around your character, then remove the extractor.";
    bool installed = ok && fs::exists(game / "Mods" / "ZoneExtractor.dll", ec);
    if (installed)
        world += L"  ZoneExtractor is installed in the game's Mods folder.";
    SetWindowTextW(control(WORLD_STATUS), world.c_str());
    SetWindowTextW(control(INSTALL_EXTRACTOR), installed ? L"Update extractor" : L"Install extractor");
    EnableWindow(control(INSTALL_EXTRACTOR), supported);
    EnableWindow(control(REMOVE_EXTRACTOR), installed);
    EnableWindow(control(PLAY), ok);
    EnableWindow(control(CHECK_DATA), ok);
}

// Headless commands write to the console that started the setup (or to a redirected file).
int runHeadless(const std::vector<std::wstring> &args) {
    DWORD type = GetFileType(GetStdHandle(STD_OUTPUT_HANDLE));
    if (type != FILE_TYPE_DISK && type != FILE_TYPE_PIPE) {
        if (!AttachConsole(ATTACH_PARENT_PROCESS))
            AllocConsole();
        FILE *stream = nullptr;
        freopen_s(&stream, "CONOUT$", "w", stdout);
        std::cout.clear();
    }
    SetConsoleOutputCP(CP_UTF8);
    const int code = ownersetup::headless(args, root, std::cout);
    std::cout << std::flush;
    return code;
}

// The wizard opens by itself on a first run: an interrupted run, or data still missing.
bool wizardNeeded() {
    fs::path game = text(control(PATH_EDIT));
    if (!ownersetup::inspectGame(game).found)
        return true;
    return ownersetup::runPending(game) || !ownersetup::assess(game, ownersetup::worldRequired(ini)).readyToPlay();
}

void save() {
    writeIni(L"Game", L"Path", text(control(PATH_EDIT)));
    writeIni(L"World", L"Source", selection(SOURCE));
    writeIni(L"Terrain", L"Style", selection(STYLE));
    writeIni(L"Graphics", L"Width", text(control(WIDTH)));
    writeIni(L"Graphics", L"Height", text(control(HEIGHT)));
    writeIni(L"Graphics", L"VSync", SendMessageW(control(VSYNC), BM_GETCHECK, 0, 0) == BST_CHECKED ? L"1" : L"0");
    writeIni(L"Graphics", L"ViewDistance", text(control(VIEW)));
    writeIni(L"Shadows", L"Enabled", SendMessageW(control(SHADOWS), BM_GETCHECK, 0, 0) == BST_CHECKED ? L"1" : L"0");
    writeIni(L"Shadows", L"Resolution", selection(SHADOW_RES));
}

void browse() {
    BROWSEINFOW info{};
    info.hwndOwner = window;
    info.lpszTitle = L"Select the folder that contains cubeworld.exe";
    info.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    if (PIDLIST_ABSOLUTE id = SHBrowseForFolderW(&info)) {
        wchar_t path[MAX_PATH];
        if (SHGetPathFromIDListW(id, path))
            SetWindowTextW(control(PATH_EDIT), path);
        CoTaskMemFree(id);
        refresh();
    }
}

void extractor(bool install) {
    fs::path game = text(control(PATH_EDIT));
    std::error_code ec;
    std::wstring whyClosed;
    if (!gameClosed(game, whyClosed)) { MessageBoxW(window, whyClosed.c_str(), L"CubeVulkan Setup", MB_ICONWARNING); return; }
    if (install) {
        uint32_t stamp = 0; std::string why;
        if (!dataCheck::gameBuild(game / "cubeworld.exe", stamp, why) || stamp != dataCheck::SUPPORTED_BUILD) {
            MessageBoxW(window, L"This game build is not supported by the extractor. No files were changed.", L"CubeVulkan Setup", MB_ICONWARNING);
            return;
        }
        fs::path dll = root / "tools" / "ZoneExtractor.dll";
        if (!fs::exists(dll, ec)) {
            MessageBoxW(window, L"tools\\ZoneExtractor.dll is missing next to CubeVulkanSetup.exe.", L"CubeVulkan Setup", MB_ICONWARNING);
            return;
        }
        uint32_t dllStamp = 0;
        if (!dataCheck::gameBuild(dll, dllStamp, why)) {
            MessageBoxW(window, L"The packaged extractor DLL is incomplete or is not an x64 build. Rebuild or replace it before installing.", L"CubeVulkan Setup", MB_ICONERROR);
            return;
        }
        if (!fs::is_regular_file(game / "CubeModLoader.fip", ec)) {
            MessageBoxW(window, L"CubeModLoader is required and was not found in this game folder. Install that dependency before installing ZoneExtractor.", L"CubeVulkan Setup", MB_ICONWARNING);
            return;
        }
        if (MessageBoxW(window,
                        L"Install or update ZoneExtractor.dll in the game's Mods folder?\n\nClose Cube World first. "
                        L"It needs CubeModLoader. Back up your save first. "
                        L"For a vanilla-identical world, disable mods that change terrain (TerrainVariance, WaterLilies) "
                        L"while extracting. In game: F11 sweeps the area around your character (held in place with full "
                        L"health), F9 returns home. F3 records creatures/NPCs; F5 saves your character's appearance; F4 records animation.",
                        L"CubeVulkan Setup", MB_OKCANCEL | MB_ICONINFORMATION) != IDOK)
            return;
        if (!gameClosed(game, whyClosed)) { MessageBoxW(window, whyClosed.c_str(), L"CubeVulkan Setup", MB_ICONWARNING); return; }
        fs::create_directories(game / "Mods", ec);
        fs::path pending = game / "Mods" / (L"ZoneExtractor.dll.new-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()));
        bool copied = !ec && fs::copy_file(dll, pending, fs::copy_options::none, ec);
        if (copied) {
            // An interrupted copy must leave the existing extractor intact.
            if (!MoveFileExW(pending.c_str(), (game / "Mods/ZoneExtractor.dll").c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
                ec = std::error_code(GetLastError(), std::system_category());
            std::error_code cleanup;
            fs::remove(pending, cleanup); // only the newly created temporary DLL
        }
    } else
        fs::remove(game / "Mods" / "ZoneExtractor.dll", ec);
    if (ec) MessageBoxW(window, wide(ec.message()).c_str(), L"CubeVulkan Setup", MB_ICONERROR);
    refresh();
}

LRESULT CALLBACK proc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case BROWSE: browse(); return 0;
        case INSTALL_EXTRACTOR: extractor(true); return 0;
        case REMOVE_EXTRACTOR: extractor(false); return 0;
        case CHECK_DATA: checkData(); return 0;
        case WIZARD: wizard::open(window, root, text(control(PATH_EDIT)), font); return 0;
        case PATH_EDIT:
            if (HIWORD(wp) == EN_KILLFOCUS)
                refresh();
            return 0;
        case SAVE:
            save();
            MessageBoxW(h, L"Settings saved to CubeVulkan.ini.", L"CubeVulkan Setup", MB_OK);
            return 0;
        case PLAY:
            save();
            ShellExecuteW(h, L"open", (root / "CubeVulkan.exe").c_str(), nullptr, root.c_str(), SW_SHOWNORMAL);
            DestroyWindow(h);
            return 0;
        case CLOSE_BUTTON: DestroyWindow(h); return 0;
        }
        break;
    case WM_CTLCOLORSTATIC:
        SetBkMode((HDC)wp, TRANSPARENT);
        return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(h, msg, wp, lp);
}
} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    INITCOMMONCONTROLSEX icc{sizeof icc, ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&icc);
    wchar_t self[MAX_PATH];
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    root = fs::path(self).parent_path();
    ini = root / "CubeVulkan.ini";
    {
        int count = 0;
        LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &count);
        std::vector<std::wstring> args(argv ? argv + 1 : nullptr, argv ? argv + count : nullptr);
        LocalFree(argv);
        if (ownersetup::isHeadlessCommand(args))
            return runHeadless(args);
    }

    WNDCLASSW wc{};
    wc.lpfnWndProc = proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    wc.lpszClassName = L"CubeVulkanSetup";
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    RegisterClassW(&wc);
    window = CreateWindowW(wc.lpszClassName, L"CubeVulkan Setup", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                           CW_USEDEFAULT, CW_USEDEFAULT, 640, 560, nullptr, nullptr, instance, nullptr);
    NONCLIENTMETRICSW metrics{sizeof metrics};
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof metrics, &metrics, 0);
    font = CreateFontIndirectW(&metrics.lfMessageFont);

    // Game folder: the ini's path, else the Steam libraries.
    std::wstring path = readIni(L"Game", L"Path", L"");
    if (path.empty()) {
        Assets assets;
        if (assets.locate(""))
            path = assets.gameDir().wstring();
    }
    int y = 14;
    add(L"STATIC", L"Steam Cube World folder (CubeVulkan reads its sprites, music and sounds there):", 0, 16, y, 600, 18);
    y += 22;
    add(L"EDIT", path.c_str(), ES_AUTOHSCROLL, 16, y, 500, 24, PATH_EDIT);
    add(L"BUTTON", L"Browse...", BS_PUSHBUTTON, 524, y, 84, 24, BROWSE);
    y += 32;
    add(L"STATIC", L"", 0, 16, y, 592, 36, STATUS);
    y += 44;
    add(L"BUTTON", L"World", BS_GROUPBOX, 10, y, 604, 160);
    add(L"STATIC", L"", 0, 22, y + 22, 580, 52, WORLD_STATUS);
    add(L"BUTTON", L"Install extractor", BS_PUSHBUTTON, 22, y + 80, 130, 26, INSTALL_EXTRACTOR);
    add(L"BUTTON", L"Remove extractor", BS_PUSHBUTTON, 160, y + 80, 130, 26, REMOVE_EXTRACTOR);
    add(L"STATIC", L"Source:", 0, 310, y + 84, 50, 20);
    add(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL, 362, y + 80, 110, 120, SOURCE);
    combo(SOURCE, {L"Extracted", L"Generated"}, readIni(L"World", L"Source", L"Extracted"));
    add(L"STATIC", L"Generator:", 0, 482, y + 84, 60, 20);
    add(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL, 544, y + 80, 62, 120, STYLE);
    combo(STYLE, {L"Alpha", L"Variance"}, readIni(L"Terrain", L"Style", L"Alpha"));
    add(L"BUTTON", L"Check data...", BS_PUSHBUTTON, 22, y + 118, 130, 26, CHECK_DATA);
    add(L"BUTTON", L"Get ready to play...", BS_PUSHBUTTON, 160, y + 118, 150, 26, WIZARD);
    add(L"STATIC", L"Extracts what is missing in one go.", 0, 320, y + 122, 280, 22);
    y += 172;
    add(L"BUTTON", L"Graphics", BS_GROUPBOX, 10, y, 604, 120);
    add(L"STATIC", L"Window:", 0, 22, y + 26, 60, 20);
    add(L"EDIT", readIni(L"Graphics", L"Width", L"1600").c_str(), ES_NUMBER, 84, y + 22, 60, 24, WIDTH);
    add(L"STATIC", L"x", 0, 150, y + 26, 10, 20);
    add(L"EDIT", readIni(L"Graphics", L"Height", L"900").c_str(), ES_NUMBER, 162, y + 22, 60, 24, HEIGHT);
    add(L"BUTTON", L"VSync", BS_AUTOCHECKBOX, 240, y + 24, 80, 22, VSYNC);
    SendMessageW(control(VSYNC), BM_SETCHECK, readIni(L"Graphics", L"VSync", L"1") != L"0", 0);
    add(L"STATIC", L"View distance:", 0, 340, y + 26, 90, 20);
    add(L"EDIT", readIni(L"Graphics", L"ViewDistance", L"400").c_str(), ES_NUMBER, 432, y + 22, 60, 24, VIEW);
    add(L"BUTTON", L"Shadows (F7 in game)", BS_AUTOCHECKBOX, 22, y + 64, 170, 22, SHADOWS);
    SendMessageW(control(SHADOWS), BM_SETCHECK, readIni(L"Shadows", L"Enabled", L"1") != L"0", 0);
    add(L"STATIC", L"Shadow map:", 0, 210, y + 66, 80, 20);
    add(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL, 292, y + 62, 90, 140, SHADOW_RES);
    combo(SHADOW_RES, {L"2048", L"3072", L"4096", L"8192"}, readIni(L"Shadows", L"Resolution", L"4096"));
    add(L"STATIC", L"Other settings (time, audio, mods) are in CubeVulkan.ini.", 0, 22, y + 94, 500, 20);
    y += 134;
    add(L"BUTTON", L"Play", BS_DEFPUSHBUTTON, 344, y, 84, 30, PLAY);
    add(L"BUTTON", L"Save", BS_PUSHBUTTON, 436, y, 84, 30, SAVE);
    add(L"BUTTON", L"Close", BS_PUSHBUTTON, 528, y, 84, 30, CLOSE_BUTTON);

    refresh();
    ShowWindow(window, show);
    if (wizardNeeded())
        wizard::open(window, root, text(control(PATH_EDIT)), font);
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        if (reportPane && IsDialogMessageW(reportPane, &msg)) continue;
        if (wizard::dialogMessage(msg)) continue;
        if (!IsDialogMessageW(window, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    return 0;
}
