#include "alphagen/PlantModels.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
namespace alphagen {
std::string_view plantSprite(int kind) {
    // Only the natural-cover kinds emitted by vegetation sections2-4.
    static constexpr auto names = [] {
        std::array<std::string_view, 33> result{};
        result[0] = "flowers2";
        result[1] = "flowers";
        result[2] = "grass";
        result[3] = "grass2";
        result[4] = "grass3";
        result[5] = "underwater-plant";
        result[6] = "coral";
        result[7] = "alga";
        result[9] = "lava-grass";
        result[10] = "thorn-plant";
        result[11] = "echinacea2";
        result[12] = "leaf";
        result[22] = "reed";
        result[27] = "desert-flower01";
        result[28] = "desert-flower02";
        result[31] = "water-lily01";
        result[32] = "water-lily02";
        return result;
    }();
    return kind >= 0 && kind < int(names.size()) ? names[size_t(kind)] : std::string_view{};
}
std::shared_ptr<const PlantModels> loadPlantModels(const std::filesystem::path &extract, const std::filesystem::path &sprites,
                                                   std::string &error) {
    error.clear();
    std::ifstream input(extract / "sprite-groups.txt");
    if (!input) {
        error = "Alpha ground cover needs the owner's Extract/sprite-groups.txt";
        return {};
    }
    auto result = std::make_shared<PlantModels>();
    bool environment = false;
    std::string line;
    while (std::getline(input, line)) {
        if (line.starts_with('#')) {
            environment = line.starts_with("# environment");
            continue;
        }
        if (!environment)
            continue;
        int id;
        std::string name;
        std::istringstream row(line);
        if (!(row >> id >> name) || id < 0 || id > 65535)
            continue;
        if (name.starts_with("sprites/"))
            name.erase(0, 8);
        if (name.ends_with(".cub"))
            name.resize(name.size() - 4);
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return char(std::tolower(c)); });
        for (int kind = 0; kind < int(result->environment.size()); ++kind)
            if (!plantSprite(kind).empty() && name == plantSprite(kind))
                result->environment[size_t(kind)] = id;
    }
    for (int kind = 0; kind < int(result->environment.size()); ++kind) {
        const auto name = plantSprite(kind);
        if (name.empty())
            continue;
        const auto filename = std::string(name) + ".cub";
        if (result->type(kind) < 0 || !std::filesystem::is_regular_file(sprites / filename)) {
            error = "Alpha ground cover needs owner model and environment-table entry: " + filename;
            return {};
        }
    }
    return result;
}
} // namespace alphagen
