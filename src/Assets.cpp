#include "Assets.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <set>
#include <sstream>
#include <windows.h>

static bool isGame(const fs::path &dir) {
    std::error_code ec;
    return fs::exists(dir / "cubeworld.exe", ec) && fs::is_directory(dir / "sprites", ec);
}

// Steam's .vdf text: quoted keys and values, nested { } blocks, // comments and backslash
// escapes. The library roots are the "path" values of libraryfolders.vdf's numbered blocks,
// or, in the older layout, the values of the numbered keys themselves.
std::vector<fs::path> Assets::parseLibraryFolders(const std::string &text) {
    std::vector<fs::path> found;
    std::vector<std::string> blocks; // the names of the open blocks, outermost first
    std::string key;
    bool haveKey = false;
    auto numeric = [](const std::string &s) {
        return !s.empty() && std::all_of(s.begin(), s.end(), [](char c) { return c >= '0' && c <= '9'; });
    };
    auto lower = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](char c) { return (char)std::tolower((unsigned char)c); });
        return s;
    };
    for (size_t i = 0; i < text.size(); i++) {
        const char c = text[i];
        if (c == '/' && i + 1 < text.size() && text[i + 1] == '/') {
            while (i < text.size() && text[i] != '\n')
                i++;
        } else if (c == '{') {
            blocks.push_back(haveKey ? lower(key) : std::string());
            haveKey = false;
        } else if (c == '}') {
            if (!blocks.empty())
                blocks.pop_back();
            haveKey = false;
        } else if (c == '"') {
            std::string token;
            for (i++; i < text.size() && text[i] != '"'; i++) {
                if (text[i] == '\\' && i + 1 < text.size()) {
                    const char escaped = text[++i];
                    token += escaped == 'n' ? '\n' : escaped == 't' ? '\t' : escaped;
                } else
                    token += text[i];
            }
            if (!haveKey) {
                key = token;
                haveKey = true;
                continue;
            }
            haveKey = false;
            const bool inLibraries = !blocks.empty() && blocks[0] == "libraryfolders";
            const bool current = blocks.size() == 2 && numeric(blocks[1]) && lower(key) == "path";
            const bool old = blocks.size() == 1 && numeric(key);
            if (inLibraries && (current || old) && !token.empty())
                found.push_back(pathOfUtf8(token));
        }
    }
    return found;
}

std::vector<fs::path> Assets::steamLibraries() {
    std::vector<fs::path> roots;
    wchar_t buffer[MAX_PATH]{};
    DWORD size = sizeof(buffer);
    if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath", RRF_RT_REG_SZ,
                     nullptr, buffer, &size) == ERROR_SUCCESS)
        roots.emplace_back(buffer);
    roots.emplace_back("C:/Program Files (x86)/Steam");
    std::vector<fs::path> libraries;
    for (auto &root : roots) {
        libraries.push_back(root);
        std::ifstream in(root / "steamapps" / "libraryfolders.vdf", std::ios::binary);
        const std::string text((std::istreambuf_iterator<char>(in)), {});
        for (auto &library : parseLibraryFolders(text))
            libraries.push_back(library);
    }
    std::vector<fs::path> unique;
    std::set<std::wstring> seen;
    for (auto &l : libraries) {
        std::error_code ec;
        auto key = fs::weakly_canonical(l, ec).wstring();
        std::transform(key.begin(), key.end(), key.begin(), ::towlower);
        if (seen.insert(key).second)
            unique.push_back(l);
    }
    return unique;
}

bool Assets::locate(const std::string &configured) {
    if (!configured.empty() && isGame(pathOfUtf8(configured))) {
        game = pathOfUtf8(configured);
        return true;
    }
    for (auto &library : steamLibraries()) {
        auto dir = library / "steamapps" / "common" / "Cube World";
        if (isGame(dir)) {
            game = dir;
            return true;
        }
    }
    return false;
}

void Assets::addOverrides(const fs::path &modsDir) {
    std::error_code ec;
    if (!fs::is_directory(modsDir, ec))
        return;
    std::vector<fs::path> dirs;
    for (auto &entry : fs::directory_iterator(modsDir, ec))
        if (entry.is_directory())
            dirs.push_back(entry.path());
    std::sort(dirs.begin(), dirs.end());
    layers.insert(layers.end(), dirs.begin(), dirs.end());
}

std::optional<fs::path> Assets::resolve(const fs::path &relative) const {
    std::error_code ec;
    for (auto it = layers.rbegin(); it != layers.rend(); ++it)
        if (fs::is_regular_file(*it / relative, ec))
            return *it / relative;
    if (!game.empty() && fs::is_regular_file(game / relative, ec))
        return game / relative;
    return std::nullopt;
}

std::vector<std::string> Assets::list(const fs::path &folder, const std::string &extension) const {
    std::set<std::string> names;
    auto scan = [&](const fs::path &dir) {
        std::error_code ec;
        if (!fs::is_directory(dir, ec))
            return;
        for (auto &entry : fs::directory_iterator(dir, ec))
            if (entry.is_regular_file() && entry.path().extension() == extension)
                names.insert(utf8(entry.path().filename()));
    };
    if (!game.empty())
        scan(game / folder);
    for (auto &layer : layers)
        scan(layer / folder);
    return {names.begin(), names.end()};
}
