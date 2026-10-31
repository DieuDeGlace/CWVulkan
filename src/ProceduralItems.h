#pragma once
#include "ProceduralItemData.h"
#include <filesystem>
#include <map>
#include <optional>
#include <utility>

namespace proceduralitem {
// Independently implemented runtime shapes; prefer completed owner captures when available.
// Behavior: steam-procedural-item-resources.md sections7–17.
Resource generate(Key);
std::string spriteKey(Key);
std::optional<Key> parseSpriteKey(std::string_view);
class OwnerCache {
  public:
    explicit OwnerCache(std::filesystem::path directory) : directory(std::move(directory)) {}
    const Resource &get(Key, std::string *diagnostic = nullptr);

  private:
    std::filesystem::path directory;
    std::map<Key, Resource> resources;
};
} // namespace proceduralitem
