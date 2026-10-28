#pragma once
// Inspects owner-supplied files. No game exports are generated or bundled here.
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace dataCheck {
constexpr uint32_t SUPPORTED_BUILD = 0x5d93ab0d;
// Validates a PE64 header before any extractor offset is considered supported.
bool gameBuild(const std::filesystem::path &exe, uint32_t &timestamp, std::string &error);

enum class Status { Pass, Warning, Error };
struct Entry {
    Status status;
    std::string name, detail, action;
};
struct Report {
    std::filesystem::path game, world;
    bool deep = false, supportedBuild = false, gameplayData = false, frontendData = false;
    size_t zones = 0, placements = 0, characters = 0, combatCharacters = 0;
    std::vector<Entry> entries;
    std::vector<std::string> estimatedObjectTypes;
    std::string text() const;
    bool errors() const;
};
// Quick: validate home terrain, player recordings and menu; index other zones.
// Deep: also validate every terrain/placement file, sprite and creature recording.
using Progress = std::function<void(const std::string &)>;
Report inspect(const std::filesystem::path &game, const std::filesystem::path &world = {}, bool deep = false, const Progress &progress = {});
} // namespace dataCheck
