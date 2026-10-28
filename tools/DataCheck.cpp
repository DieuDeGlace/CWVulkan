// Console companion to Setup. Reads owner data without opening a Vulkan window.
#include "Assets.h"
#include "Config.h"
#include "DataCheck.h"
#include "OwnerSetup.h"
#include "SteamSession.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <windows.h>

int wmain(int argc, wchar_t **argv) {
    namespace fs = std::filesystem;
    // The setup wizard's headless commands (--setup-check, --setup-extract [--dry-run],
    // --setup-restore), here too because a console program's exit code is easy to script.
    const std::vector<std::wstring> args(argv + 1, argv + argc);
    if (ownersetup::isHeadlessCommand(args)) {
        SetConsoleOutputCP(CP_UTF8);
        wchar_t self[32768];
        GetModuleFileNameW(nullptr, self, 32768);
        return ownersetup::headless(args, fs::path(self).parent_path(), std::cout);
    }
    fs::path game, world, output;
    bool deep = false;
    for (int i = 1; i < argc; i++) {
        std::wstring arg = argv[i];
        if (arg == L"--deep") deep = true;
        else if ((arg == L"--game" || arg == L"--world" || arg == L"--report") && i + 1 < argc) {
            auto &path = arg == L"--game" ? game : arg == L"--world" ? world : output;
            path = argv[++i];
        } else if (arg == L"--help") {
            std::cout << "CubeVulkanDataCheck [--game PATH] [--world SEED_FOLDER] [--deep] [--report FILE]\n"
                         "CubeVulkanDataCheck --setup-check | --setup-extract [--dry-run] | --setup-restore [--game PATH]\n"
                         "Reads the owner's game and exports; Steam must be online with a Cube World license.\n"
                         "Quick mode validates home terrain, player clips and menu; --deep also checks all zones, records, sprites and creatures.\n"
                         "Exit 0: no blocking data errors, 1: data errors, 2: arguments, Steam or report error.\n";
            return 0;
        } else { std::cerr << "Unknown or incomplete argument. Use --help.\n"; return 2; }
    }
    SetConsoleOutputCP(CP_UTF8);
    if (game.empty()) {
        wchar_t self[32768]; GetModuleFileNameW(nullptr, self, 32768);
        Config config; config.load(fs::path(self).parent_path() / "CubeVulkan.ini");
        Assets assets;
        if (!assets.locate(config.text("Game", "Path"))) { std::cerr << "Steam Cube World was not found. Use --game PATH.\n"; return 2; }
        game = assets.gameDir();
        if (world.empty()) world = fs::u8path(config.text("World", "ExtractedPath"));
    }
    try {
        SteamSession steam; std::string why;
        uint32_t stamp = 0;
        if (!dataCheck::gameBuild(game / "cubeworld.exe", stamp, why)) { std::cerr << why << '\n'; return 2; }
        if (!output.empty() && fs::exists(output)) { std::cerr << "Report already exists; choose a new file.\n"; return 2; }
        if (!steam.start(game, why)) { std::cerr << "Steam: " << why << '\n'; return 2; }
        std::cout << "Steam: online account and Cube World license verified.\nChecking owner data...\n" << std::flush;
        auto report = dataCheck::inspect(game, world, deep, [](const std::string &phase) { std::cout << phase << '\n' << std::flush; });
        steam.pump();
        if (!steam.check(why)) { std::cerr << "Steam: " << why << '\n'; return 2; }
        std::string text = report.text();
        std::cout << text;
        if (!output.empty()) {
            // A report must never overwrite one of the owner's existing files.
            HANDLE file = CreateFileW(output.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file == INVALID_HANDLE_VALUE) { std::cerr << "Cannot create report; choose a new, writable filename.\n"; return 2; }
            DWORD written = 0;
            bool saved = WriteFile(file, text.data(), (DWORD)text.size(), &written, nullptr) && written == text.size();
            CloseHandle(file);
            if (!saved) { std::cerr << "Cannot finish writing report.\n"; return 2; }
        }
        return report.errors() ? 1 : 0;
    } catch (const std::exception &e) { std::cerr << "Data check failed: " << e.what() << '\n'; return 2; }
}
