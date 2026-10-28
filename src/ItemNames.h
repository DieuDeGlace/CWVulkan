#pragma once
// The game's item names (the item naming): from the owner's dictionary, by the item's
// key (the item naming keys) and, for gear, a quality word chosen by its rarity and modifier
// ("shabby Iron Sword", "legendary Iron Sword of Kroria"), the material's word and a
// generated name. Items are game-layout (0xa0 bytes).
#include "GameNames.h"
#include "Speech.h"

#include <cstdint>
#include <string>

namespace itemnames {

// The dictionary key of an item (the item naming keys), "" if none.
std::string itemKey(int category, int id);
std::string materialKey(int material);
// the item naming.
std::string name(const Speech &speech, const GameNames &names, const uint8_t *item);

} // namespace itemnames
