#pragma once
// Map independent Alpha plant kinds to the owner's exported Steam model table.
// The mapping contains indices only, never model bytes or replacement assets.
#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
namespace alphagen {
std::string_view plantSprite(int kind);
struct PlantModels {
    PlantModels() { environment.fill(-1); }
    std::array<int, 33> environment;
    int type(int kind) const { return kind >= 0 && kind < int(environment.size()) ? environment[size_t(kind)] : -1; }
};
std::shared_ptr<const PlantModels> loadPlantModels(const std::filesystem::path &extract, const std::filesystem::path &sprites,
                                                   std::string &error);
} // namespace alphagen
