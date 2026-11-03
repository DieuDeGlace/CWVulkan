#pragma once
// Owner-supplied palm leaves. Model bytes never belong to source or release packages.
#include <filesystem>
#include <memory>
#include <string>
struct CubModel;
namespace alphagen {
struct TreeModels {
    std::shared_ptr<const CubModel> cardinal, diagonal;
};
std::shared_ptr<const TreeModels> loadTreeModels(const std::filesystem::path &sprites, std::string &error);
} // namespace alphagen
