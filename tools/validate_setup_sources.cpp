// Windows release check. Build with cl /EHsc /std:c++17 validate_setup_sources.cpp user32.lib
// Pass the directory containing CubeVulkanSetup.exe and its packaged CubeVulkan.ini.
#define NOMINMAX
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
namespace {
constexpr int sourceId = 106, styleId = 107, saveId = 114;
void require(bool ok, const char *message) {
    if (!ok) throw std::runtime_error(message);
}

struct Fixture {
    std::wstring name = L"CubeVulkanSetupTest-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64());
    fs::path folder = fs::temp_directory_path() / name;
    fs::path ini = folder / L"CubeVulkan.ini";
    HDESK desktop = nullptr;
    explicit Fixture(const fs::path &package) {
        require(fs::create_directory(folder), "could not create a fresh fixture");
        fs::copy_file(package / L"CubeVulkanSetup.exe", folder / L"CubeVulkanSetup.exe");
        fs::copy_file(package / L"CubeVulkan.ini", ini);
        // An explicit synthetic installation keeps the first-run wizard in this
        // folder. It contains no game, Steam helper, extractor or owner data.
        fs::path game = folder / L"fixture";
        fs::create_directories(game / L"sprites");
        require(std::ofstream(game / L"cubeworld.exe").good(), "could not create synthetic installation marker");
        set(L"Game", L"Path", game.c_str());
        desktop = CreateDesktopW(name.c_str(), nullptr, nullptr, 0, GENERIC_ALL, nullptr);
        require(desktop != nullptr, "could not create a private test desktop");
    }
    ~Fixture() {
        if (desktop) CloseDesktop(desktop);
        std::error_code ec;
        fs::remove_all(folder, ec);
    }
    void set(const wchar_t *section, const wchar_t *key, const wchar_t *value) const {
        require(WritePrivateProfileStringW(section, key, value, ini.c_str()) != 0, "could not prepare fixture configuration");
    }
    std::wstring get(const wchar_t *section, const wchar_t *key) const {
        wchar_t buffer[512]{};
        GetPrivateProfileStringW(section, key, L"", buffer, 512, ini.c_str());
        return buffer;
    }
};

struct WindowSearch { DWORD pid; const wchar_t *className; HWND result; };
BOOL CALLBACK matchWindow(HWND window, LPARAM param) {
    auto &search = *reinterpret_cast<WindowSearch *>(param);
    DWORD pid = 0;
    wchar_t name[96]{};
    GetWindowThreadProcessId(window, &pid);
    if (pid == search.pid && GetClassNameW(window, name, 96) && std::wstring(name) == search.className) {
        search.result = window;
        return FALSE;
    }
    return TRUE;
}
struct Session {
    const Fixture &fixture;
    PROCESS_INFORMATION process{};
    explicit Session(const Fixture &f) : fixture(f) {
        STARTUPINFOW startup{};
        startup.cb = sizeof startup;
        startup.lpDesktop = const_cast<wchar_t *>(f.name.c_str());
        require(CreateProcessW((f.folder / L"CubeVulkanSetup.exe").c_str(), nullptr, nullptr, nullptr, FALSE,
                               0, nullptr, f.folder.c_str(), &startup, &process) != 0, "could not launch Setup");
        CloseHandle(process.hThread);
    }
    ~Session() {
        if (WaitForSingleObject(process.hProcess, 0) == WAIT_TIMEOUT) {
            TerminateProcess(process.hProcess, 1);
            WaitForSingleObject(process.hProcess, 5000);
        }
        CloseHandle(process.hProcess);
    }
    HWND window(const wchar_t *name) const {
        WindowSearch search{process.dwProcessId, name, nullptr};
        const auto deadline = GetTickCount64() + 10000;
        do {
            EnumDesktopWindows(fixture.desktop, matchWindow, reinterpret_cast<LPARAM>(&search));
            if (search.result) return search.result;
        } while (WaitForSingleObject(process.hProcess, 20) == WAIT_TIMEOUT && GetTickCount64() < deadline);
        throw std::runtime_error("Setup window did not appear");
    }
    void finish(HWND root) const {
        PostMessageW(root, WM_CLOSE, 0, 0);
        DWORD exitCode = 1;
        require(WaitForSingleObject(process.hProcess, 5000) == WAIT_OBJECT_0 &&
                GetExitCodeProcess(process.hProcess, &exitCode) && exitCode == 0, "Setup did not exit normally");
    }
};

void check(Fixture &f, const wchar_t *expect, const wchar_t *change = nullptr) {
    Session session(f);
    HWND root = session.window(L"CubeVulkanSetup");
    HWND wizard = session.window(L"CubeVulkanSetupWizard");
    SendMessageW(wizard, WM_CLOSE, 0, 0);
    HWND source = GetDlgItem(root, sourceId);
    wchar_t selected[64]{};
    const auto index = SendMessageW(source, CB_GETCURSEL, 0, 0);
    require(index != CB_ERR && SendMessageW(source, CB_GETLBTEXTLEN, index, 0) < 64 &&
            SendMessageW(source, CB_GETLBTEXT, index, reinterpret_cast<LPARAM>(selected)) != CB_ERR, "no selected source");
    if (change) {
        auto choice = SendMessageW(source, CB_FINDSTRINGEXACT, -1, reinterpret_cast<LPARAM>(change));
        require(choice != CB_ERR && SendMessageW(source, CB_SETCURSEL, choice, 0) == choice, "source choice missing");
        SendMessageW(root, WM_COMMAND, MAKEWPARAM(sourceId, CBN_SELCHANGE), reinterpret_cast<LPARAM>(source));
    }
    bool generatorEnabled = IsWindowEnabled(GetDlgItem(root, styleId)) != 0;
    require(PostMessageW(GetDlgItem(root, saveId), BM_CLICK, 0, 0) != 0, "could not click Save");
    HWND modal = session.window(L"#32770");
    PostMessageW(GetDlgItem(modal, IDOK), BM_CLICK, 0, 0);
    session.finish(root);
    auto saved = f.get(L"World", L"Source");
    auto expectedSave = std::wstring(change ? change : expect);
    std::wcout << L"Expected " << expect << L", displayed " << selected << L", saved " << saved << std::endl;
    require(selected == std::wstring(expect) && saved == expectedSave, "Setup changed the world source");
    require(generatorEnabled == (expectedSave == L"Generated"), "legacy terrain control applied to wrong source");
}

} // namespace
int wmain(int argc, wchar_t **argv) {
    if (argc != 2) { std::cerr << "Usage: validate_setup_sources <runtime directory>\n"; return 2; }
    try {
        Fixture f(argv[1]);
        require(f.get(L"World", L"Source") == L"Alpha", "fixture must start with the shipped Alpha default");
        f.set(L"World", L"Seed", L"8921");
        f.set(L"Terrain", L"Style", L"Variance");
        f.set(L"Render", L"Style", L"Steam");
        f.set(L"Water", L"Style", L"Revised");
        check(f, L"Alpha");
        for (auto destination : {L"Game", L"Extracted", L"Generated", L"Alpha"}) {
            auto initial = f.get(L"World", L"Source");
            check(f, initial.c_str(), destination);
            check(f, destination);
        }
        f.set(L"World", L"Source", nullptr);
        check(f, L"Alpha");
        f.set(L"World", L"Source", L"alpha");
        check(f, L"Extracted");
        require(f.get(L"World", L"Seed") == L"8921" && f.get(L"Terrain", L"Style") == L"Variance" &&
                f.get(L"Render", L"Style") == L"Steam" && f.get(L"Water", L"Style") == L"Revised",
                "unrelated world or appearance setting changed");
        std::cout << "Setup source round trips passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
