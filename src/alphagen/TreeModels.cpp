#include "alphagen/TreeModels.h"
#include "CubModel.h"
namespace alphagen {
std::shared_ptr<const TreeModels> loadTreeModels(const std::filesystem::path &sprites, std::string &error) {
    error.clear();
    auto cardinal = std::make_shared<CubModel>(), diagonal = std::make_shared<CubModel>();
    if (!cardinal->load(sprites / "palm-leaf.cub", &error) || !diagonal->load(sprites / "palm-leaf-diagonal.cub", &error))
        return {};
    return std::make_shared<const TreeModels>(TreeModels{std::move(cardinal), std::move(diagonal)});
}
} // namespace alphagen
