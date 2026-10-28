#pragma once
// Locates the Steam Cube World install and resolves asset files, letting
// folders under mods/ override any Steam file of the same relative path.
// Assets are only ever read from the user's own install; none are bundled.
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// UTF-8 text of a path (C++20 u8string returns char8_t).
inline std::string utf8(const fs::path &p) {
    auto s = p.u8string();
    return std::string(s.begin(), s.end());
}
// A path from UTF-8 text (what fs::u8path did before C++20).
inline fs::path pathOfUtf8(const std::string &s) { return fs::path(std::u8string(s.begin(), s.end())); }

class Assets {
  public:
    // `configured` (CubeVulkan.ini [Game] Path) wins when it holds cubeworld.exe;
    // otherwise every Steam library is searched.
    bool locate(const std::string &configured);
    // Adds each directory in `modsDir` as an override layer (alphabetical).
    void addOverrides(const fs::path &modsDir);
    const fs::path &gameDir() const { return game; }
    const std::vector<fs::path> &overrides() const { return layers; }
    // Path of `relative` (e.g. "sprites/lily.cub"), honouring overrides.
    std::optional<fs::path> resolve(const fs::path &relative) const;
    // File names (no directory) with `extension` in `folder` across all layers.
    std::vector<std::string> list(const fs::path &folder, const std::string &extension) const;

    // Steam library roots from the registry and libraryfolders.vdf.
    static std::vector<fs::path> steamLibraries();
    // The library roots listed in the text of a libraryfolders.vdf (current or older layout).
    static std::vector<fs::path> parseLibraryFolders(const std::string &vdfText);

  private:
    fs::path game;
    std::vector<fs::path> layers; // Later entries override earlier ones.
};
