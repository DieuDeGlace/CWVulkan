// The Android program: NativeActivity's thread runs the same main() as CubeVulkan.exe (src/main.cpp,
// compiled as cubevulkanMain) in the app's own folder, with the window, input and folders of
// AndroidGlfw.cpp.
#include "AndroidWindow.h"
#include "Config.h"

#include <android/asset_manager.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

int cubevulkanMain(int argc, char **argv);

namespace {
namespace fs = std::filesystem;

// CubeVulkan's own files packed in the APK (the top-level assets/ and config/ folders): copied
// beside the program as the Windows build places them, without replacing the owner's edits to
// CubeVulkan.ini.
void unpackAppFiles(AAssetManager *manager, const fs::path &root) {
    struct Folder { const char *packed; fs::path placed; bool replace; };
    const Folder folders[] = {{"", root, false}, {"OverworldEvents", root / "assets" / "OverworldEvents", true}};
    for (const Folder &folder : folders) {
        AAssetDir *dir = AAssetManager_openDir(manager, folder.packed);
        if (!dir)
            continue;
        std::error_code ec;
        fs::create_directories(folder.placed, ec);
        while (const char *name = AAssetDir_getNextFileName(dir)) {
            const fs::path target = folder.placed / name;
            if (!folder.replace && fs::exists(target, ec))
                continue;
            const std::string packed = *folder.packed ? std::string(folder.packed) + "/" + name : std::string(name);
            if (AAsset *asset = AAssetManager_open(manager, packed.c_str(), AASSET_MODE_STREAMING)) {
                std::ofstream out(target, std::ios::binary | std::ios::trunc);
                char buffer[16384];
                for (int n; (n = AAsset_read(asset, buffer, sizeof buffer)) > 0;)
                    out.write(buffer, n);
                AAsset_close(asset);
            }
        }
        AAssetDir_close(dir);
    }
}
} // namespace

void android_main(android_app *app) {
    androidwindow::attach(app);
    const fs::path root = androidwindow::filesDirectory();
    std::error_code ec;
    fs::create_directories(root, ec);
    if (chdir(root.c_str()) != 0)
        __android_log_print(ANDROID_LOG_ERROR, "CubeVulkan", "Cannot enter %s", root.c_str());
    unpackAppFiles(app->activity->assetManager, root);
    // The owner's game folder, read where it is: the folder chosen in the picker, one pushed into
    // the app's own folder over USB, or Documents/Cube World. CubeVulkan.ini's [Game] Path names it.
    const auto isGame = [&](const fs::path &dir) {
        return !dir.empty() && fs::is_regular_file(dir / "cubeworld.exe", ec) && fs::is_directory(dir / "sprites", ec);
    };
    const auto find = [&]() -> fs::path {
        for (const fs::path dir : {fs::path(androidwindow::chosenGameFolder()), root / "Cube World",
                                   fs::path("/storage/emulated/0/Documents/Cube World")})
            if (isGame(dir))
                return dir;
        return {};
    };
    fs::path game = find();
    bool asked = false;
    while (game.empty()) {
        if (!asked) {
            androidwindow::requestGameFolder(
                "CubeVulkan plays from your own copy of Steam Cube World, prepared on Windows. Put its folder "
                "(the one with cubeworld.exe, sprites and Extract) on this device, for example in Documents, "
                "and choose it. It is read where it is; nothing is copied or changed.");
            asked = true;
        }
        if (!androidwindow::waitEvents(250))
            return;
        if (androidwindow::takeFolderAnswered()) {
            game = find();
            asked = false; // ask again if that was not a game folder
        }
    }
    if (!Config::write(root / "CubeVulkan.ini", "Game", "Path", game.string()))
        __android_log_print(ANDROID_LOG_ERROR, "CubeVulkan", "Cannot record the game folder in CubeVulkan.ini");
    // No Steam client runs on Android: single player through the loopback, as --offline on Windows.
    std::vector<std::string> arguments{"cubevulkan", "--offline"};
    std::vector<char *> argv;
    for (auto &a : arguments)
        argv.push_back(a.data());
    argv.push_back(nullptr);
    const int code = cubevulkanMain(int(arguments.size()), argv.data());
    __android_log_print(ANDROID_LOG_INFO, "CubeVulkan", "The game ended (%d)", code);
    if (!androidwindow::errorShown()) // (an error's dialog closes the app when dismissed)
        ANativeActivity_finish(app->activity);
    while (!app->destroyRequested) { // let the activity close before the thread returns
        android_poll_source *source = nullptr;
        if (ALooper_pollOnce(-1, nullptr, nullptr, reinterpret_cast<void **>(&source)) == ALOOPER_POLL_ERROR)
            break;
        if (source)
            source->process(app, source);
    }
}
